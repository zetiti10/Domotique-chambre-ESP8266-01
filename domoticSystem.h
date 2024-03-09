#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace DomoticSystem {

class DomoticSystemDevice {
 public:
  virtual void setID(int ID);
  static String addZeros(int number, int length);
  static int getIntFromString(String &string, int position, int lenght);

 protected:
  int ID_{0};
};

class DomoticSystemSwitch : public switch_::Switch, public DomoticSystemDevice, public uart::UARTDevice {
 protected:
  virtual void write_state(bool state) override;
};

// Communication class.
class DomoticSystem : public Component, public uart::UARTDevice, public DomoticSystemDevice {
 public:
  virtual void setLEDCube(DomoticSystemSwitch *LEDCube);
  virtual void setup() override;
  virtual void loop() override;

 protected:
  DomoticSystemSwitch *LEDCube_{nullptr};
};

}  // namespace DomoticSystem
}  // namespace esphome

#endif  // USE_ARDUINO
