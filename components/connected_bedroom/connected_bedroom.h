#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/api/custom_api_device.h"

namespace esphome {
namespace connected_bedroom {

enum ConnectedLightTypes {
  BINARY_CONNECTED_LIGHT,
  TEMPERATURE_VARIABLE_CONNECTED_LIGHT,
  COLOR_VARIABLE_CONNECTED_LIGHT
};

class ConnectedBedroomRGBLEDStrip;

class ConnectedBedroom : public Component, public uart::UARTDevice, public api::CustomAPIDevice {
 public:
  float get_setup_priority() const override;
  void setup() override;
  void loop() override;
  void dump_config() override;

  void add_analog_sensor(int communication_id, sensor::Sensor *analog_sensor);
  void add_binary_sensor(int communication_id, binary_sensor::BinarySensor *binary_sensor);
  void add_switch(int communication_id, switch_::Switch *switch_);
  void add_connected_light(int communication_id, std::string entity_id, ConnectedLightTypes type);

 protected:
  void process_message_();
  void send_message_to_Arduino_(std::string title, std::string message);
  void update_connected_light_state_(std::string entity_id, std::string state);
  void update_connected_light_brightness_(std::string entity_id, std::string state);
  void update_connected_light_temperature_(std::string entity_id, std::string state);
  void update_connected_light_color_(std::string entity_id, std::string state);

  sensor::Sensor *get_analog_sensor_from_communication_id_(int communication_id) const;
  binary_sensor::BinarySensor *get_binary_sensor_from_communication_id_(int communication_id) const;
  switch_::Switch *get_switch_from_communication_id_(int communication_id) const;
  std::string get_connected_light_from_communication_id_(int communication_id) const;
  int get_communication_id_from_connected_light_entity_id_(std::string entity_id) const;

  std::vector<uint8_t> receivedMessage_;

  std::vector<std::pair<int, sensor::Sensor *>> analog_sensors_;
  std::vector<std::pair<int, binary_sensor::BinarySensor *>> binary_sensors_;
  std::vector<std::pair<int, switch_::Switch *>> switches_;
  std::vector<std::tuple<int, std::string, ConnectedLightTypes>> connected_lights_;
};

class ConnectedBedroomDevice {
 public:
  void set_communication_id(int communication_id);
  void set_parent(ConnectedBedroom *parent);
  virtual void register_device() = 0;

 protected:
  ConnectedBedroom *parent_;
  int communication_id_;
};

class ConnectedBedroomSwitch : public Component, public switch_::Switch, public ConnectedBedroomDevice {
 public:
  void dump_config() override;
  void register_device() override;

 protected:
  void write_state(bool state) override;
};

// Binary light
// RGB LED strip
// Alarm
// Television

}  // namespace connected_bedroom
}  // namespace esphome