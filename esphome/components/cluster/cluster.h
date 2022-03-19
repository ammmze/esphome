#pragma once

#include "esphome/core/component.h"
#include "esphome/core/controller.h"


namespace esphome {
namespace cluster {

class Cluster : public Controller, public Component {
  public:
   Cluster();
   void dump_config() override;
   void setup() override;
   void loop() override;
   void set_name(std::string name);
   void set_cluster_name(std::string name);
   std::string get_cluster_name();
   void set_port(uint16_t port);
   void set_timeout(uint32_t timeout);
   void set_weight(uint32_t weight);
   void set_quorum(uint16_t quorum);
   void set_auto_discovery(bool auto_discovery);

#ifdef USE_SENSOR
    void on_sensor_update(sensor::Sensor *obj, float state) override;
#endif
  protected:
    std::string name_;
    std::string cluster_name_;
    uint16_t port_{6425};
    uint32_t timeout_{1000};
    uint32_t weight_{0};
    uint16_t quorum_{0};
    boolean auto_discovery_{true};
    // TODO peers
};

extern Cluster *global_cluster_component;

}  // namespace cluster
}  // namespace esphome
