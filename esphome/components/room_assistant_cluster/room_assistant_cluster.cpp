// #include "room_assistant_cluster.h"
// #include "esphome/core/log.h"
// #include "esphome/core/helpers.h"
// #include "esphome/core/util.h"
// #include "democracy.h"

// namespace esphome {
// namespace room_assistant_cluster {

// static const char *TAG = "room_assistant_cluster";

// Democracy democracy;
// bool started = false;

// void RoomAssistantCluster::dump_config() {
//   ESP_LOGCONFIG(TAG, "Room Assistant Cluster:");
//   // ESP_LOGCONFIG(TAG, "  Address: %s:%u", network_get_address().c_str(), this->port_);
// }

// void RoomAssistantCluster::setup() {
//   ESP_LOGI(TAG, "Setting up Room Assistant Cluster...");
//   this->setup_controller();
// }

// void RoomAssistantCluster::loop() {
//   if (!started && network_is_connected()){
//     DemocracyOptions options;
//     options.name = this->name_;
//     options.port = this->port_;
//     options.timeout = this->timeout_;
//     options.weight = this->weight_;
//     options.quorum = this->quorum_;
//     democracy.start(options);
//     started = true;
//   } else if (started && !network_is_connected()) {
//     // todo: democracy stop
//     started = false;
//   }

//   // ESP_LOGI(TAG, "loop");
// }

// void RoomAssistantCluster::set_name(std::string name) {this->name_ = name;}
// void RoomAssistantCluster::set_port(uint16_t port) {this->port_ = port;}
// void RoomAssistantCluster::set_timeout(uint32_t timeout) {this->timeout_ = timeout;}
// void RoomAssistantCluster::set_weight(uint32_t weight) {this->weight_ = weight;}
// void RoomAssistantCluster::set_quorum(uint16_t quorum) {this->quorum_ = quorum;}
// void RoomAssistantCluster::set_auto_discovery(bool auto_discovery) {this->auto_discovery_ = auto_discovery;}

// #ifdef USE_SENSOR
// void RoomAssistantCluster::on_sensor_update(sensor::Sensor *sensor, float value) {
//   ESP_LOGD(TAG, "Sensor update: %s -- %.5f", sensor->get_name().c_str(), value);
// }
// #endif

// }  // namespace room_assistant_cluster
// }  // namespace esphome