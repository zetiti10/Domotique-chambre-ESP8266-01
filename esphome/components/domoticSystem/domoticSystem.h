#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace DomoticSystem {

// Class for domoticSystem device (switch, light...).
class DomoticSystemDevice {
 public:
  virtual void setID(int ID);
  // Methods used to format messages in the communication.
  static String addZeros(int number, int length);
  static int getIntFromString(String &string, int position, int lenght);

 protected:
  // The ID used in the communication between the ESP and the Arduino.
  int ID_{0};
};

// Class for a switch from the Arduino domotic system.
class DomoticSystemSwitch : public switch_::Switch, public DomoticSystemDevice, public uart::UARTDevice {
 protected:
  // Send a UART message on control.
  virtual void write_state(bool state) override;
};

// Communication class that receives messages from the Arduino.
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
