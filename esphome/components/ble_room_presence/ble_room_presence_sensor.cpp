#include "ble_room_presence_sensor.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

#ifdef ARDUINO_ARCH_ESP32

namespace esphome {
namespace ble_room_presence {

static const char *TAG = "ble_room_presence";
static const int8_t default_tx_power = -72;

void BLERoomPresenceSensor::dump_config() { LOG_SENSOR("", "BLE Room Presence Sensor", this); }

std::string BLERoomPresenceSensor::format_for_publish(std::string value) {
    if (std::string("nan").compare(value) == 0) {
      return "not_home";
    }
  return sanitize_string_whitelist(App.get_name(), HOSTNAME_CHARACTER_WHITELIST);
//   return "{\"id\":\"ffff0005db11\", \"distance\": " + value + "}";
}

float BLERoomPresenceSensor::calculate_distance(int rssi, std::vector<int8_t> tx_powers) {
    float dist;

    if (rssi == 0) {
        return -1.0;
    }

    int8_t tx_power = default_tx_power;

    // if (!tx_powers.empty()) {
    //   tx_power = tx_powers.front();
    //   ESP_LOGD(TAG, "received tx power %d ... overriding default tx_power %d", tx_power, default_tx_power);
    // }

    if (tx_power > 0) {
      tx_power = tx_power * -1;
	}

    const float ratio = rssi * 1.0 / tx_power;
    if (ratio < 1.0) {
        dist = pow(ratio, 10);
    } else {
        dist = (0.89976) * pow(ratio, 7.7095) + 0.111;
    }

    ESP_LOGI(TAG, "tx_power: %d ; ratio: %f ; dist: %f", tx_power, ratio, dist);

    return round(dist * 100) / 100;
}

}  // namespace ble_room_presence
}  // namespace esphome

#endif
