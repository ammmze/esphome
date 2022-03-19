#include <ArduinoJson.h>
#include <Ticker.h>
#include <string>
#include <sstream>
#include "esphome/core/log.h"
#include "EventEmitter.hpp"
#include "democracy.h"

#ifdef ARDUINO_ARCH_ESP32
#include <AsyncUDP.h>
#endif
#ifdef ARDUINO_ARCH_ESP8266
#include <ESPAsyncUDP.h>
#endif

namespace esphome {
namespace cluster {

void Democracy::process_event(JsonObject &root) {

  auto event = root["event"].as<std::string>();
  // ignore messages from ourself
  if (self.id.compare(root["id"].as<std::string>()) == 0) {
    return;
  }

  // for debugging...
  // root.printTo(Serial);

  if (event.compare("hello") == 0) {
    handle_hello_event(root);
  } else if (event.compare("vote") == 0) {
    handle_vote_event(root);
  } else if (event.compare("leader") == 0) {
    handle_leader_event(root);
  } else {
    JsonVariant extra = root["extra"];
    emit(event, extra);
  }
}

void Democracy::handle_hello_event(JsonObject &root) {
  auto sender_id = root["id"].as<std::string>();
  auto sender_source = root["source"].as<std::string>();
  auto sender_state = root["state"].as<std::string>();

  Node *sender_ptr = NULL;
  for(auto &peer : peers) {
    char buffer[45];
    sprintf(buffer, "%s:%u", peer.address.toString().c_str(), peer.port);
    if (sender_source.compare(buffer) == 0) {
      sender_ptr = &peer;
      break;
    }
  }

  boolean added = false;

  // we don't know this device yet...
  if (sender_ptr == NULL) {
    // parse ip and port from "source"
    size_t delim_pos = sender_source.find_first_of(":");
    IPAddress addr;
    if (!addr.fromString(sender_source.substr(0, delim_pos).c_str())) {
      ESP_LOGE(TAG_DEMOCRACY, "Failed to parse address from source %s", sender_source.c_str());
      return;
    }
    std::string portStr = sender_source.substr(delim_pos + 1);
    std::istringstream iss(portStr);
    uint16_t port;
    iss >> port;
    if (iss.fail()) {
      ESP_LOGE(TAG_DEMOCRACY, "Failed to parse port from source %s", sender_source.c_str());
      return;
    }

    // add the sender as a peer
    if (add_peer(addr, port)) {
      added = true;
      sender_ptr = &peers.back();
    }
  }

  // at this point we probably have a sender node
  if (sender_ptr != NULL) {
    Node &sender = *sender_ptr;

    if (sender.id.length() == 0 || STATE_REMOVED.compare(sender.state) == 0) {
      added = true;
    }

    // lets update it to make sure it is current
    sender.id = sender_id;
    sender.state = sender_state;
    sender.voters.clear();
    sender.last = std::time(nullptr);
    sender.weight = root["weight"].as<unsigned long>();

    if (added) {
      sender.channels.clear();
      JsonArray &channels = root["channels"].as<JsonArray>();
      for(auto channel : channels) {
        sender.channels.push_back(channel.as<std::string>());
      }
      emit("added", sender);
    }
  }

  // we are both leaders ... time for elections
  if (is_leader() && STATE_LEADER.compare(sender_state) == 0) {
    hold_elections();
  }

  // we had elections but have no leader ... time for elections
  if (election_count_down <= 0 && get_leader() == NULL) {
    hold_elections();
  }

  if (get_leader() != NULL) {
    election_count_down = 0;
  }
}

void Democracy::handle_vote_event(JsonObject &root) {
  std::string sender_id = root["id"].as<std::string>();
  std::string candidate_id = root["candidate"].as<std::string>();
  ESP_LOGI(TAG_DEMOCRACY, "received vote to remove peer %s", candidate_id.c_str());

  // find the peer who is the subject of the vote (i.e. the candidate)
  Node *peer_ptr = get_peer_by_id(candidate_id);
  if (peer_ptr == NULL) {
    return;
  }
  Node peer = *peer_ptr;

  // add the sender to the list of peers voting for the removal of this node
  if (std::count(peer.voters.begin(), peer.voters.end(), sender_id) == 0) {
    peer.voters.push_back(candidate_id);
    check_ballots(candidate_id);
  }
}

void Democracy::handle_leader_event(JsonObject &root) {
  auto sender_id = root["id"].as<std::string>();

  // TODO: add peer if we don't know them yet
  Node *leader_ptr = get_peer_by_id(sender_id);
  if (leader_ptr == NULL) {
    return;
  }

  Node leader = *leader_ptr;

  // we already knew them as the leader
  if (STATE_LEADER.compare(leader.state) == 0) {
    return;
  }

  leader.state = STATE_LEADER;
  emit("leader", leader);
}

void Democracy::do_hello() {
  send("hello");
  check_for_missing_peers();

  if (election_count_down > 0) {
    election_count_down--;
  } else if (!is_leader() && get_leader() == NULL) {
    hold_elections();
  }
}

void Democracy::start_hello() {
  helloTicker.attach_ms(options.interval, &Democracy::do_hello, this);
  do_hello(this);
}

void Democracy::check_for_missing_peers() {
  // TODO: send votes for each node we think is down (i.e. haven't seen them checkin within the timeout)
  std::time_t stale_time = std::time(nullptr) - options.timeout;
  for (auto &peer : peers) {
    if (peer.last < stale_time && std::count(peer.voters.begin(), peer.voters.end(), options.name) == 0) {
      ESP_LOGD(TAG_DEMOCRACY, "voting for removal of peer %s", peer.id.c_str());
      // peer hasn't checked in for longer than the timeout and we haven't cast our vote saying we haven't heard from
      // it

      // add our vote
      peer.voters.push_back(options.name);

      // send out the vote
      StaticJsonBuffer<200> jsonBuffer;
      JsonObject& extra = jsonBuffer.createObject();
      extra["candidate"] = peer.id.c_str();
      JsonVariant extra_v(extra);
      send("vote", extra_v);
      check_ballots(peer.id);
    }
  }
}

void Democracy::check_ballots(std::string candidate_id) {
  Node *peer_ptr = get_peer_by_id(candidate_id);
  if (peer_ptr == NULL) {
    return;
  }
  Node &peer = *peer_ptr;
  auto removed_peer_state = peer.state;

  // count the number of peers without votes for removal (i.e. ones that are known to be active)
  int vote_count = 0;
  for (const auto &peer : peers) {
    if (peer.voters.size() == 0) {
      vote_count++;
    }
  }

  if (peer.voters.size() >= vote_count) {
    peer.state = STATE_REMOVED;
    emit("removed", peer);
  }

  if (STATE_LEADER.compare(removed_peer_state) == 0) {
    hold_elections();
  }
}

void Democracy::hold_elections() {
  Node *new_leader_ptr = NULL;
  unsigned long max_weight = options.weight;

  // find peer with highest weight
  for (auto &peer : peers) {
    if (peer.weight > max_weight && STATE_REMOVED.compare(peer.state) != 0) {
      max_weight = peer.weight;
      new_leader_ptr = &peer;
    }
  }

  // we are currently leader but there is a peer with a higher weight who needs to become leader
  if (is_leader() && new_leader_ptr != NULL) {
    resign();
  }

  if (new_leader_ptr == NULL) {
    if (!is_leader()) {
      self.state = STATE_LEADER;
      emit("elected", self);
    }
    // we are the leader
    send("leader");
  } else {
    ESP_LOGD(TAG_DEMOCRACY, "setting node as leader %s", (*new_leader_ptr).id.c_str());
    // proactively mark the node as the new leader
    (*new_leader_ptr).state = STATE_LEADER;
  }
}

void Democracy::resign() {
  if (is_leader()) {
    self.state = STATE_CITIZEN;
    emit("resigned", self);
    send("hello");
  }
}

boolean Democracy::is_leader() { return STATE_LEADER.compare(self.state) == 0; }

Node *Democracy::get_peer_by_id(std::string id) {
  auto idc = id.c_str();
  for (auto &peer : peers) {
    if (peer.id.compare(id) == 0) {
      return &peer;
    }
  }
  return NULL;
}

Node *Democracy::get_leader() {
  if (STATE_LEADER.compare(self.state) == 0) {
    return &self;
  }
  for (auto &peer : peers) {
    if (STATE_LEADER.compare(peer.state) == 0) {
      return &peer;
    }
  }
  return NULL;
}

void Democracy::start(DemocracyOptions democracyOptions) {
  options = democracyOptions;
  self.id = options.name;
  self.address = options.address;
  self.port = options.port;
  self.weight = options.weight;

  if (udp.listen(options.port)) {
    udp.onPacket([&](AsyncUDPPacket packet) {
      std::string data = (char *) packet.data();
      // TODO: eventually probably remove this block
      // if (data.find("\"event\":\"hello\"") == std::string::npos) {
      //   ESP_LOGD(TAG_DEMOCRACY, "received: %s", data.c_str());
      // }

      DynamicJsonBuffer jb(500);
      JsonObject &root = jb.parseObject(data);
      process_event(root);

      // json::parse_json(data, [&](JsonObject &root) { process_event(root); });

      // reply to the client
      // packet.printf("Got %u bytes of data", packet.length());
    });
    start_hello();
    emit("online", self);
  }
}

boolean Democracy::add_peer(const IPAddress addr, uint16_t port) {
  if (self.address == addr && self.port == port) {
    return false;
  }
  for(auto& peer : peers) {
    // if peer already exists
    if (peer.address == addr && peer.port == port) {
      return false;
    }
  }

  Node peer;
  peer.address = addr;
  peer.port = port;
  peers.push_back(peer);
  return true;
}

void Democracy::send(const char *event, std::string id) {
  JsonVariant extra;
  send(event, extra, id);
}

void Democracy::send(const char *event, JsonVariant &extra, std::string id) {
  StaticJsonBuffer<500> jb;
  JsonObject &root = jb.createObject();
  
  root["event"] = event;
  root["id"] = self.id;
  root["source"] = self.address.toString() + ":" + self.port;

  if (strcmp(event, "vote") == 0) {
    root["candidate"] = extra["candidate"];
  } else {
    root["weight"] = self.weight;
    root["state"] = self.state;
    JsonArray& channels = root.createNestedArray("channels");
    for (const auto &channel: self.channels) {
      channels.add(channel);
    }

    if (extra.success()) {
      root["extra"] = extra;
    }
  }
  

  AsyncUDPMessage message;
  root.printTo(message);

  // TODO: send to specific node when given an id
  for(const auto& peer : peers) {
    // TODO: eventually probably remove this block
    // if (strcmp(event, "hello") != 0) {
    //  ESP_LOGD(TAG_DEMOCRACY, "sending: %s to %s(%s:%u)", event, peer.id.c_str(), peer.address.toString().c_str(), peer.port);
    // }
    udp.sendTo(message, peer.address, peer.port);
  }
}

void Democracy::subscribe(std::string channel) {
  self.channels.push_back(channel);

  StaticJsonBuffer<100> buff;
  JsonObject& extra = buff.createObject();
  extra["channel"] = channel;
  JsonVariant extra_v(extra);
  send("subscribe", extra_v);
}

void Democracy::publish(char* channel, JsonVariant &message) {
  for(const auto &peer: peers) {
    if (std::find(peer.channels.begin(), peer.channels.end(), channel) != peer.channels.end()) {
      send(channel, message, peer.id);
    }
  }
}

} // cluster
} // esphome