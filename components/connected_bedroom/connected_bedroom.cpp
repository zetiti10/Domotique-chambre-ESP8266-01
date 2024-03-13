#include "connected_bedroom.h"
#include "esphome/core/log.h"

namespace esphome {
namespace connected_bedroom {

static const char *TAG = "connected.bedroom";

// I know that delays are not recommended, but the ESP will be only used for the communication.
static const int UART_WAITING_TIME = 10;

String addZeros(int number, int length) {
  String result = String(number);
  while (result.length() < (unsigned int) length) {
    result = "0" + result;
  }
  return result;
}

int getIntFromString(String &string, int position, int lenght) {
  int result = 0;

  for (int i = 0; i < lenght; i++)
    result += (string.charAt(position + i) - '0') * pow(10, ((lenght - i) - 1));

  return result;
}

void ConnectedBedroom::setup() {
  /*unsigned long initialTime = millis();

  while (!this->available()) {
    if ((millis() - initialTime) >= 10000) {
      ESP_LOGE(TAG, "Unable to start communication with the Arduino Mega.");
      this->mark_failed();
      return;
    }
  }

  delay(UART_WAITING_TIME);

  String receivedMessage;
  while (this->available() > 0) {
    char letter = this->read();

    if (letter == '\n')
      break;

    receivedMessage += letter;
  }

  // At starting, "2" is sent by the Arduino.
  if (receivedMessage != "2")
    ESP_LOGW(TAG, "Received unknow message: %s", receivedMessage);

  // Response.
  this->write_byte(2);*/

  ESP_LOGD(TAG, "Setup finished");
}

void ConnectedBedroom::loop() {
  // Receive and process messages.
  if (!this->available())
    return;

  delay(UART_WAITING_TIME);

  String receivedMessage;
  while (this->available() > 0) {
    char letter = this->read();

    if (letter == '\n')
      break;

    receivedMessage += letter;
  }

  ESP_LOGD(TAG, "Received message: %s", receivedMessage);

  int ID = getIntFromString(receivedMessage, 1, 2);

  switch (getIntFromString(receivedMessage, 0, 1)) {
    case 1: {
      switch (getIntFromString(receivedMessage, 3, 2)) {
        case 1: {
          sensor::Sensor *sens = this->get_sensor_from_communication_id(ID);
          if (sens != nullptr)
            sens->publish_state(getIntFromString(receivedMessage, 5, 1));
          break;
        }
      }
      break;
    }
  }
}

void ConnectedBedroom::add_sensor(int communication_id, sensor::Sensor *sens) {
  this->sensors_.push_back(std::make_pair(communication_id, sens));
}

sensor::Sensor *ConnectedBedroom::get_sensor_from_communication_id(int ID) {
  for (int i = 0; i < sensors_.size(); i++) {
    if (sensors_[i].first == ID)
      return sensors_[i].second;
  }
  return nullptr;
}

}  // namespace connected_bedroom
}  // namespace esphome