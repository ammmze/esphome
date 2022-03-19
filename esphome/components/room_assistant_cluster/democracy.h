// // #include <ArduinoJson.h>
// #include <Ticker.h>
// // #include <WiFi.h>
// #include <string>
// #include "esphome/components/wifi/wifi_component.h"
// #include "esphome/components/json/json_util.h"
// #include "esphome/core/log.h"

// #ifdef ARDUINO_ARCH_ESP32
// #include <AsyncUDP.h>
// #endif
// #ifdef ARDUINO_ARCH_ESP8266
// #include <ESPAsyncUDP.h>
// #endif

// namespace esphome {
// namespace room_assistant_cluster {

// struct DemocracyOptions
// {
//   std::string name;
//   int port = 6425;
//   int interval = 1000;
//   int timeout = 60;
//   int weight = 0;
//   int quorum = 0;
// };

// struct Node
// {
//   IPAddress address;
//   int port = 6425;
// };

// JsonObject emptyExtra;

// class Democracy {
//  private:
//   DemocracyOptions options;
//   AsyncUDP udp;
//   Ticker helloTicker;

//  public:
//   // Democracy(DemocracyOptions democracyOptions)
//   // {
//   //     options = democracyOptions;
//   // };

//   void loop() {}

//   void start(DemocracyOptions democracyOptions) {
//     options = democracyOptions;

//     if (udp.listen(options.port)) {
//       udp.onPacket([](AsyncUDPPacket packet) {
//         std::string data = (char *) packet.data();
//         ESP_LOGD("democracy", "packet: %s", data.c_str());
//         json::parse_json(data, [&](JsonObject &root) {
//           ESP_LOGD("democracy", "event: %s", root["event"].as<char *>());
//           ESP_LOGD("democracy", "id: %s", root["id"].as<char *>());
//           ESP_LOGD("democracy", "source: %s", root["source"].as<char *>());
//         });

//         // reply to the client
//         packet.printf("Got %u bytes of data", packet.length());
//       });
//       hello();
//     }
//   }

//   void send(const char *event, JsonObject& extra = emptyExtra, std::string id = NULL) {
//     std::string json = json::build_json([&](JsonObject &root) {
//       root["event"] = event;
//       root["id"] = options.name;
//       root["source"] = WiFi.localIP().toString() + ":" + options.port;

//       if (strcmp(event, "vote") == 0) {
//         // TODO: data["candidate"]
//         root["candidate"] = extra["candidate"];
//       } else {
//         root["weight"] = 1;
//         root["state"] = "citizen";
//         root.createNestedArray("channels");

//         if (extra != emptyExtra) {
//           root["extra"] = extra;
//         }
//       }
//     });

//     AsyncUDPMessage message;
//     message.print(json.c_str());
//     udp.sendTo(message, IPAddress(192, 168, 0, 206), 56359);
//     udp.sendTo(message, IPAddress(192, 168, 0, 10), 6425);
//   };

//   static void doHello(Democracy *democracy) { democracy->send("hello"); }

//   void hello() {
//     helloTicker.attach_ms(options.interval, &Democracy::doHello, this);
//     doHello(this);
//   };

//   void check() {
//     // TODO: send votes for each node we think is down (i.e. haven't seen them checkin within the timeout)
//   }

//   void subscribe(std::string channel) {
//     StaticJsonBuffer<100> buff;
//     JsonObject& extra = buff.createObject();
//     extra["channel"] = channel;
//     send("subscribe", extra);
//   }

//   void publish(char* channel, JsonObject &message) {
//     // TODO: loop through nodes to figure out which ones are subscribed to this channel
//     send(channel, message, "");
//   };
// };

// } // room_assistant_cluster
// } // esphome