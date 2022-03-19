// #pragma once

// #include "esphome/core/component.h"
// #include "esphome/core/controller.h"
// #include "esphome/core/log.h"


// namespace esphome {
// namespace room_assistant_cluster {

// class RoomAssistantCluster : public Controller, public Component {
//   public:
//    void dump_config() override;
//    void setup() override;
//    void loop() override;
//    void set_name(std::string name);
//    void set_port(uint16_t port);
//    void set_timeout(uint32_t timeout);
//    void set_weight(uint32_t weight);
//    void set_quorum(uint16_t quorum);
//    void set_auto_discovery(bool auto_discovery);

// #ifdef USE_SENSOR
//     void on_sensor_update(sensor::Sensor *obj, float state) override;
// #endif
//   protected:
//    std::string name_;
//    uint16_t port_{6425};
//    uint32_t timeout_{1000};
//    uint32_t weight_{0};
//    uint16_t quorum_{0};
//    boolean auto_discovery_{true};
//    // TODO peers
// };

// }  // namespace room_assistant_cluster
// }  // namespace esphome
