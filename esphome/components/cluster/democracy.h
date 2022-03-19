#pragma once

#include <ArduinoJson.h>
#include <Ticker.h>
#include <string>
#include <sstream>
#include "EventEmitter.hpp"

#ifdef ARDUINO_ARCH_ESP32
#include <AsyncUDP.h>
#endif
#ifdef ARDUINO_ARCH_ESP8266
#include <ESPAsyncUDP.h>
#endif

namespace esphome {
namespace cluster {

static const char *TAG_DEMOCRACY = "democracy";


static const std::string STATE_CITIZEN = "citizen";
static const std::string STATE_LEADER = "leader";
static const std::string STATE_REMOVED = "removed";

struct DemocracyOptions
{
  std::string name;
  IPAddress address;
  int port = 6425;
  int interval = 1000;
  int timeout = 60;
  unsigned long weight = 0;
  int quorum = 0;
};

struct Node
{
  std::string id;
  IPAddress address;
  uint16_t port = 6425;
  unsigned long weight = 0;
  std::string state = STATE_CITIZEN;
  std::time_t last = std::time(nullptr);
  std::vector<std::string> voters{};
  std::vector<std::string> channels{};

  std::string toString() {
    std::ostringstream node;
    node << "Node[" 
         << "id='" << id << "'"
         << ", address=" << address 
         << ", port=" << port 
         << ", state='" << state << "'"
         << ", weight=" << weight 
         << "]";
    return node.str();
    // return sprintf("Node[id='%s', address=%s, port=%d, state='%s', weight=%d]", id.c_str(), address.toString().c_str(), port, state.c_str(), weight);
    // "Node[id='" + id + "', address=" + std::string(address.toString().c_str()) + ", port=" + to_string(port) +
    //     ", state='" + state + "', weight=" + to_string(weight) + "]";
  }
};

class Democracy: public EventEmitter {
 private:
  DemocracyOptions options;
  AsyncUDP udp;
  Ticker helloTicker;
  Node self;
  std::vector<Node> peers{};
  int election_count_down = 5;

  // static method that will be invoked on every interval to send "hello" to all the peers
  static void do_hello(Democracy *democracy) { democracy->do_hello(); }

  // processes an event
  void process_event(JsonObject &root);

  // handles received "hello" events
  void handle_hello_event(JsonObject &root);

  // handles received "vote" events
  void handle_vote_event(JsonObject &root);

  // handles received "leader" events
  void handle_leader_event(JsonObject &root);

  // actually send the hello and then initiate peer activity check
  void do_hello();

  // starts sending "hello" to all the peers on an interval
  void start_hello();

  // check to see if there are any nodes who have not checked in for longer than the timeout
  void check_for_missing_peers();

  // checks to see if the nodes unanimously decide a peer is gone
  void check_ballots(std::string candidate_id);

  // perform elections by finding the node with the highest weight and electing that node as leader
  void hold_elections();

  // changes our own status from leader to citizen
  void resign();

  // checks if we are the leader
  boolean is_leader();

  // gets a pointer to the peer with the given id
  Node *get_peer_by_id(std::string id);

  // gets a pointer to the leader peer
  Node *get_leader();

 public:
  void loop() {}

  // starts the cluster
  void start(DemocracyOptions democracyOptions);

  // adds a peer to the cluster
  boolean add_peer(const IPAddress addr, uint16_t port);

  // send message to the cluster
  void send(const char *event, std::string id = "");

  // send a message with a payload to the cluster
  void send(const char *event, JsonVariant &extra, std::string id = "");

  void subscribe(std::string channel);

  void publish(char *channel, JsonVariant &message);
};

} // cluster
} // esphome