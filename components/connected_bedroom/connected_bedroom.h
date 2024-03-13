#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace connected_bedroom {

class ConnectedBedroom : public Component, public uart::UARTDevice {
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }
  void setup() override;
  void loop() override;
  void add_sensor(int communication_id, sensor::Sensor *sens);

 protected:
  sensor::Sensor *get_sensor_from_communication_id(int ID);

  std::vector<std::pair<int, sensor::Sensor *>> sensors_;
};

}  // namespace connected_bedroom
}  // namespace esphome