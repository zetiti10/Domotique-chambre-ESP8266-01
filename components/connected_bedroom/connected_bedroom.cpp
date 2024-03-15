#include "connected_bedroom.h"
#include "esphome/core/log.h"

namespace esphome {
namespace connected_bedroom {

static const char *TAG = "connected.bedroom";

std::string addZeros(int number, int length) {
  std::string result = std::to_string(number);

  // Ajouter des zéros au début jusqu'à ce que la longueur soit atteinte
  while (result.length() < static_cast<size_t>(length)) {
    result = "0" + result;
  }

  return result;
}

int getIntFromVector(std::vector<uint8_t> &string, int position, int lenght) {
  int result = 0;

  for (int i = 0; i < lenght; i++)
    result += (string[position + i] - '0') * pow(10, ((lenght - i) - 1));

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

  if (receivedMessage != "2")
    ESP_LOGW(TAG, "Received unknow message: %s", receivedMessage);

  // Response.
  this->write_byte(2);*/

  ESP_LOGD(TAG, "Setup finished");
}

void ConnectedBedroom::loop() {
  while (this->available()) {
    uint8_t letter = this->read();

    if (letter == '\r')
      continue;

    if (letter == '\n')
      this->process_message_();

    else
      this->receivedMessage_.push_back(letter);
  }
}

void ConnectedBedroom::process_message_() {
  ESP_LOGD(TAG, "Received message: %s", this->receivedMessage_);

  int ID = getIntFromVector(this->receivedMessage_, 1, 2);

  switch (getIntFromVector(this->receivedMessage_, 0, 1)) {
    case 1: {
      switch (getIntFromVector(this->receivedMessage_, 3, 2)) {
        case 1: {
          switch_::Switch *switch_ = this->get_switch_from_communication_id_(ID);

          if (switch_ != nullptr)
            switch_->publish_state(getIntFromVector(this->receivedMessage_, 5, 1));
        }

        case 7: {
          binary_sensor::BinarySensor *binary_sensor = this->get_binary_sensor_from_communication_id_(ID);

          if (binary_sensor != nullptr)
            binary_sensor->publish_state(getIntFromVector(this->receivedMessage_, 5, 1));

          break;
        }
      }
      break;
    }
  }

  this->receivedMessage_.clear();
}

void ConnectedBedroom::dump_config() {
  ESP_LOGCONFIG(TAG, "Connected bedroom");

  ESP_LOGCONFIG(TAG, "Analog sensors");
  for (auto sens : this->analog_sensors_) {
    ESP_LOGCONFIG(TAG, "Communication id : %d", sens.first);
    LOG_SENSOR(TAG, "", sens.second);
  }

  ESP_LOGCONFIG(TAG, "Binary sensors");
  for (auto sens : this->binary_sensors_) {
    ESP_LOGCONFIG(TAG, "Communication id : %d", sens.first);
    LOG_BINARY_SENSOR(TAG, "", sens.second);
  }

  ESP_LOGCONFIG(TAG, "Switches");
  for (auto sens : this->switches_) {
    ESP_LOGCONFIG(TAG, "Communication id : %d", sens.first);
    LOG_SWITCH(TAG, "", sens.second);
  }
}

void ConnectedBedroom::add_analog_sensor(int communication_id, sensor::Sensor *analog_sensor) {
  this->analog_sensors_.push_back(std::make_pair(communication_id, analog_sensor));
}

void ConnectedBedroom::add_binary_sensor(int communication_id, binary_sensor::BinarySensor *binary_sensor) {
  this->binary_sensors_.push_back(std::make_pair(communication_id, binary_sensor));
}

void ConnectedBedroom::add_switch(int communication_id, switch_::Switch *switch_) {
  this->switches_.push_back(std::make_pair(communication_id, switch_));
}

sensor::Sensor *ConnectedBedroom::get_analog_sensor_from_communication_id_(int ID) {
  auto it = std::find_if(analog_sensors_.begin(), analog_sensors_.end(),
                         [ID](const std::pair<int, sensor::Sensor *> &element) { return element.first == ID; });
  if (it != analog_sensors_.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

binary_sensor::BinarySensor *ConnectedBedroom::get_binary_sensor_from_communication_id_(int ID) {
  auto it =
      std::find_if(binary_sensors_.begin(), binary_sensors_.end(),
                   [ID](const std::pair<int, binary_sensor::BinarySensor *> &element) { return element.first == ID; });
  if (it != binary_sensors_.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

switch_::Switch *ConnectedBedroom::get_switch_from_communication_id_(int ID) {
  auto it = std::find_if(switches_.begin(), switches_.end(),
                         [ID](const std::pair<int, switch_::Switch *> &element) { return element.first == ID; });
  if (it != switches_.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

void ConnectedBedroomSwitch::dump_config() { LOG_SWITCH("", "ConnectedBedroomSwitch", this); }

void ConnectedBedroomSwitch::set_communication_id(int communication_id) { this->communication_id_ = communication_id; }

void ConnectedBedroomSwitch::set_parent(ConnectedBedroom *parent) {
  this->parent_ = parent;
  this->parent_->add_switch(this->communication_id_, this);
}

void ConnectedBedroomSwitch::write_state(bool state) {
  this->parent_->write_byte(0);
  this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());
  this->parent_->write_byte(0);
  this->parent_->write_byte(0);
  this->parent_->write_byte(state);
  this->parent_->write_byte('\n');

  ESP_LOGD(TAG, "Message sent with id %s", addZeros(this->communication_id_, 2).c_str());
}

}  // namespace connected_bedroom
}  // namespace esphome