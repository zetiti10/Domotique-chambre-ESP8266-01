#include "connected_bedroom.h"
#include "esphome/core/log.h"

namespace esphome {
namespace connected_bedroom {

static const char *TAG = "connected.bedroom";

std::string addZeros(int number, int length) {
  std::string result = std::to_string(number);

  while (result.length() < static_cast<size_t>(length))
    result = "0" + result;

  return result;
}

int getIntFromVector(std::vector<uint8_t> &string, int position, int lenght) {
  int result = 0;

  for (int i = 0; i < lenght; i++)
    result += (string[position + i] - '0') * pow(10, ((lenght - i) - 1));

  return result;
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float ConnectedBedroom::get_setup_priority() const { return setup_priority::DATA; }

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
  switch (getIntFromVector(this->receivedMessage_, 0, 1)) {
    case 1: {
      int ID = getIntFromVector(this->receivedMessage_, 1, 2);

      switch (getIntFromVector(this->receivedMessage_, 3, 2)) {
        case 1: {
          switch_::Switch *switch_ = this->get_switch_from_communication_id_(ID);

          if (switch_ != nullptr)
            switch_->publish_state(getIntFromVector(this->receivedMessage_, 5, 1));
        }

        case 2: {
          ConnectedBedroomRGBLEDStrip *RGB_LED_strip_ = this->get_RGB_LED_strip_from_communication_id_(ID);

          switch (getIntFromVector(this->receivedMessage_, 5, 1)) {
            case 0:
              float r = mapfloat(getIntFromVector(this->receivedMessage_, 6, 3), 0.0f, 255.0f, 0.0f, 1.0f);
              float g = mapfloat(getIntFromVector(this->receivedMessage_, 9, 3), 0.0f, 255.0f, 0.0f, 1.0f);
              float b = mapfloat(getIntFromVector(this->receivedMessage_, 12, 3), 0.0f, 255.0f, 0.0f, 1.0f);

              // Not finished : update state and effects (animations).

              break;
          }

          break;
        }

        case 7: {
          binary_sensor::BinarySensor *binary_sensor = this->get_binary_sensor_from_communication_id_(ID);

          if (binary_sensor != nullptr)
            binary_sensor->publish_state(getIntFromVector(this->receivedMessage_, 5, 1));

          break;
        }

        case 8: {
          sensor::Sensor *analog_sensor = this->get_analog_sensor_from_communication_id_(ID);

          if (analog_sensor != nullptr)
            analog_sensor->publish_state(getIntFromVector(this->receivedMessage_, 5, 4));

          break;
        }

        case 9: {
          sensor::Sensor *analog_sensor = this->get_analog_sensor_from_communication_id_(ID);

          if (analog_sensor != nullptr)
            analog_sensor->publish_state(float(getIntFromVector(this->receivedMessage_, 5, 4)) / float(100));

          else
            break;

          analog_sensor = this->get_analog_sensor_from_communication_id_(ID + 1);

          if (analog_sensor != nullptr)
            analog_sensor->publish_state(float(getIntFromVector(this->receivedMessage_, 9, 4)) / float(100));

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

void ConnectedBedroom::add_RGB_LED_strip(int communication_id, ConnectedBedroomRGBLEDStrip *RGB_LED_strip) {
  this->RGB_LED_strips_.push_back(std::make_pair(communication_id, RGB_LED_strip));
}

sensor::Sensor *ConnectedBedroom::get_analog_sensor_from_communication_id_(int communication_id) const {
  auto it = std::find_if(analog_sensors_.begin(), analog_sensors_.end(),
                         [communication_id](const std::pair<int, sensor::Sensor *> &element) {
                           return element.first == communication_id;
                         });

  if (it != analog_sensors_.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

binary_sensor::BinarySensor *ConnectedBedroom::get_binary_sensor_from_communication_id_(int communication_id) const {
  auto it = std::find_if(binary_sensors_.begin(), binary_sensors_.end(),
                         [communication_id](const std::pair<int, binary_sensor::BinarySensor *> &element) {
                           return element.first == communication_id;
                         });

  if (it != binary_sensors_.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

switch_::Switch *ConnectedBedroom::get_switch_from_communication_id_(int communication_id) const {
  auto it = std::find_if(switches_.begin(), switches_.end(),
                         [communication_id](const std::pair<int, switch_::Switch *> &element) {
                           return element.first == communication_id;
                         });

  if (it != switches_.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

ConnectedBedroomRGBLEDStrip *ConnectedBedroom::get_RGB_LED_strip_from_communication_id_(int communication_id) const {
  auto it = std::find_if(RGB_LED_strips_.begin(), RGB_LED_strips_.end(),
                         [communication_id](const std::pair<int, ConnectedBedroomRGBLEDStrip *> &element) {
                           return element.first == communication_id;
                         });

  if (it != RGB_LED_strips_.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

void ConnectedBedroomDevice::set_communication_id(int communication_id) { this->communication_id_ = communication_id; }

void ConnectedBedroomDevice::set_parent(ConnectedBedroom *parent) {
  this->parent_ = parent;

  this->register_device();
}

void ConnectedBedroomSwitch::dump_config() { LOG_SWITCH("", "ConnectedBedroomSwitch", this); }

void ConnectedBedroomSwitch::register_device() { this->parent_->add_switch(this->communication_id_, this); }

void ConnectedBedroomSwitch::write_state(bool state) {
  this->parent_->write('0');
  this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());
  this->parent_->write('0');
  this->parent_->write('0');
  this->parent_->write(state ? '1' : '0');
  this->parent_->write('\n');
}

light::LightTraits ConnectedBedroomRGBLEDStrip::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::RGB});
  return traits;
}

void ConnectedBedroomRGBLEDStrip::write_state(light::LightState *state) {
  this->parent_->write('0');
  this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());
  this->parent_->write('0');
  this->parent_->write('1');

  if (state->get_effect_name() == "None") {
    this->parent_->write('0');

    float *r_initial;
    float *g_initial;
    float *b_initial;

    state->current_values_as_rgb(r_initial, g_initial, b_initial, false);

    int r = mapfloat(*r_initial, 0.0f, 1.0f, 0.0f, 255.0f);
    int g = mapfloat(*g_initial, 0.0f, 1.0f, 0.0f, 255.0f);
    int b = mapfloat(*b_initial, 0.0f, 1.0f, 0.0f, 255.0f);

    this->parent_->write_str(addZeros(r, 3).c_str());
    this->parent_->write_str(addZeros(g, 3).c_str());
    this->parent_->write_str(addZeros(b, 3).c_str());
  }

  else if (state->get_effect_name() == "Rainbow") {
    this->parent_->write('1');
  }

  else if (state->get_effect_name() == "Soundreact") {
    this->parent_->write('2');
  }

  this->parent_->write('\n');
}

void ConnectedBedroomRGBLEDStrip::register_device() { this->parent_->add_RGB_LED_strip(this->communication_id_, this); }

void ConnectedBedroomRGBLEDStrip::set_light_state_object(light::LightState *light_state) {
  this->light_state_ = light_state;
}

light::LightState *ConnectedBedroomRGBLEDStrip::get_light_state_object() { return light_state_; }

}  // namespace connected_bedroom
}  // namespace esphome

// Se baser sur tuya light
// Comment update le state ?