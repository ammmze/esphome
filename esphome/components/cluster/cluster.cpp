#include "cluster.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/util.h"
#include "esphome/components/wifi/wifi_component.h"
#include "democracy.h"
#include "mdns.h"

#ifdef ARDUINO_ARCH_ESP32
#include <ESPmDNS.h>
#endif
#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266mDNS.h>
#endif

namespace esphome {
namespace cluster {

static const char *TAG = "cluster";

Democracy democracy;
bool started = false;

static const char * if_str[] = {"STA", "AP", "ETH", "MAX"};
static const char * ip_protocol_str[] = {"V4", "V6", "MAX"};

void mdns_process_results(mdns_result_t * results){
    mdns_result_t * r = results;
    mdns_ip_addr_t * a = NULL;
    int i = 1, t;
    while(r){
        // printf("%d: Interface: %s, Type: %s\n", i++, if_str[r->tcpip_if], ip_protocol_str[r->ip_protocol]);
        // if(r->instance_name){
        //     printf("  PTR : %s\n", r->instance_name);
        // }
        // if(r->hostname){
        //     printf("  SRV : %s.local:%u\n", r->hostname, r->port);
        // }
        // if(r->txt_count){
        //     printf("  TXT : [%u] ", r->txt_count);
        //     for(t=0; t<r->txt_count; t++){
        //         printf("%s=%s; ", r->txt[t].key, r->txt[t].value);
        //     }
        //     printf("\n");
        // }
        a = r->addr;
        IPAddress addr;
        while (a) {
          if (a->addr.type == IPADDR_TYPE_V6) {
            // printf("  AAAA: " IPV6STR "\n", IPV62STR(a->addr.u_addr.ip6));
          } else {
            addr = IPAddress(a->addr.u_addr.ip4.addr);
            // printf("  A   : " IPSTR "\n", IP2STR(&(a->addr.u_addr.ip4)));
            break;
          }
          a = a->next;
        }
        democracy.add_peer(addr, r->port);
        r = r->next;
    }
}

void find_mdns_service(const char * service_name, const char * proto)
{
    // ESP_LOGD(TAG, "Query PTR: %s.%s.local", service_name, proto);

    mdns_result_t * results = NULL;
    esp_err_t err = mdns_query_ptr(service_name, proto, 3000, 20,  &results);
    if(err){
        ESP_LOGE(TAG, "Query Failed");
        return;
    }
    if(!results){
        // ESP_LOGW(TAG, "No results found!");
        return;
    }

    mdns_process_results(results);
    mdns_query_results_free(results);
}

void start_mdns_service(const char *instance_name, const char *service_type, const char *proto, const uint16_t port)
{
    //initialize mDNS service
    esp_err_t err = mdns_init();
    if (err) {
        ESP_LOGE(TAG, "mDNS Init failed: %d\n", err);
        return;
    }

    mdns_hostname_set(instance_name);
    mdns_instance_name_set(instance_name);
    mdns_service_add(NULL, service_type, proto, port, NULL, 0);
}

Cluster::Cluster() { global_cluster_component = this; }

void Cluster::dump_config() {
  ESP_LOGCONFIG(TAG, "Cluster:");
  ESP_LOGCONFIG(TAG, "  Name: %s", this->name_.c_str());
  ESP_LOGCONFIG(TAG, "  Port: %u", this->port_);
  ESP_LOGCONFIG(TAG, "  Timeout: %u", this->timeout_);
  ESP_LOGCONFIG(TAG, "  Weight: %u", this->weight_);
  ESP_LOGCONFIG(TAG, "  Quorum: %u", this->quorum_);
}

void Cluster::setup() {
  ESP_LOGI(TAG, "Setting up Cluster...");
  this->setup_controller();

  democracy.on("online", [](Node &node) { ESP_LOGI(TAG, "We are online %s", node.toString().c_str()); });

  democracy.on("added", [&](Node &node) { ESP_LOGI(TAG, "%s has been added. %s", node.id.c_str(), node.toString().c_str()); });

  democracy.on("removed", [&](Node &node) { ESP_LOGI(TAG, "%s has been removed. %s", node.id.c_str(), node.toString().c_str()); });

  democracy.on("elected", [&](Node &node) { ESP_LOGI(TAG, "%s has been elected leader. %s", node.id.c_str(), node.toString().c_str()); });

  democracy.on("leader", [&](Node &node) { ESP_LOGI(TAG, "%s has been elected leader. %s", node.id.c_str(), node.toString().c_str()); });

  democracy.on("peers", [&](JsonVariant &peers_v) {
    if (!peers_v.is<JsonArray>()) {
      return;
    }
    // JsonArray &peers_a = peers_v.as<JsonArray>();
    for(const auto &peers_peers_v: peers_v.as<JsonArray&>()) {
      if (!peers_peers_v.is<JsonArray>()) {
        continue;
      }
      JsonArray &peer = peers_peers_v.as<JsonArray>();
      IPAddress addr;
      if (!addr.fromString(peer.get<char *>(0))) {
        continue;
      }
      std::istringstream iss(peer.get<char *>(1));
      uint16_t port;
      iss >> port;
      if (iss.fail()) {
        continue;
      }
      democracy.add_peer(addr, port);
    }
    // std::string peerStr;
    // peers_v.printTo(peerStr);
    // Receive peer event [["192.168.0.167","6425"],["192.168.0.206","6425"]]
    // ESP_LOGD(TAG, "Receive peer event %s", peerStr.c_str());
  });

  democracy.on("bluetooth-low-energy.new-distance", [&](JsonVariant &extra) {
    std::string extra_str;
    extra.printTo(extra_str);
    ESP_LOGD(TAG, "bluetooth-low-energy.new-distance %s", extra_str.c_str());
    // [D][cluster:150]: bluetooth-low-energy.new-distance {"instanceName":"piz0","tagId":"ffff0005db11","tagName":"Green Tag","distance":1.4,"outOfRange":false}
    // [D][cluster:150]: bluetooth-low-energy.new-distance {"instanceName":"brandens-mbp","tagId":"fd27cbcb582245029c833d88b7014ce2","tagName":"Green Tag","distance":0.2,"outOfRange":false}
  });

  democracy.subscribe("bluetooth-low-energy.new-distance");
}

void Cluster::loop() {
  if (!started && network_is_connected()){
    DemocracyOptions options;
    options.name = this->name_;
    options.address = WiFi.localIP();
    options.port = this->port_;
    options.timeout = this->timeout_;
    options.weight = this->weight_;
    options.quorum = this->quorum_;
    democracy.start(options);
    started = true;
  } else if (started && !network_is_connected()) {
    // todo: democracy stop
    started = false;
  }

  if (auto_discovery_) {
    find_mdns_service("_room-assistant", "_udp");
    start_mdns_service(this->name_.c_str(), "_room-assistant", "_udp", this->port_);
  }
  // ESP_LOGI(TAG, "loop");
}

void Cluster::set_name(std::string name) {this->name_ = name;}
void Cluster::set_cluster_name(std::string cluster_name) {this->cluster_name_ = cluster_name;}
std::string Cluster::get_cluster_name() { return this->cluster_name_;}
void Cluster::set_port(uint16_t port) {this->port_ = port;}
void Cluster::set_timeout(uint32_t timeout) {this->timeout_ = timeout;}
void Cluster::set_weight(uint32_t weight) {this->weight_ = weight;}
void Cluster::set_quorum(uint16_t quorum) {this->quorum_ = quorum;}
void Cluster::set_auto_discovery(bool auto_discovery) {this->auto_discovery_ = auto_discovery;}

#ifdef USE_SENSOR
void Cluster::on_sensor_update(sensor::Sensor *sensor, float value) {
  ESP_LOGD(TAG, "Sensor update: %s -- %.5f", sensor->get_name().c_str(), value);
}
#endif

Cluster *global_cluster_component;

}  // namespace cluster
}  // namespace esphome