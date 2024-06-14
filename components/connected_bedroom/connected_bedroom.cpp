#include "connected_bedroom.h"
#include "esphome/core/log.h"
#include <sstream>

namespace esphome {
namespace connected_bedroom {

static const char *TAG = "connected_bedroom";

std::string addZeros(int number, int length) {
  std::string result = std::to_string(number);

  while (result.length() < static_cast<size_t>(length))
    result = "0" + result;

  return result;
}

int getIntFromVector(std::vector<uint8_t> &string, int position, int lenght) {
  int result = 0;

  for (int i = 0; i < lenght; i++) {
    int power = 1;

    for (int j = 0; j < ((lenght - i) - 1); j++)
      power *= 10;

    result += (string[position + i] - '0') * power;
  }

  return result;
}

float ConnectedBedroom::get_setup_priority() const { return setup_priority::DATA; }

void ConnectedBedroom::setup() {
  this->register_service(&esphome::connected_bedroom::ConnectedBedroom::send_message_to_Arduino_,
                         "print_message_on_display", {"title", "message"});

  for (auto connected_light : this->connected_lights_) {
    std::string &entity_id = std::get<1>(connected_light);

    this->subscribe_homeassistant_state(&esphome::connected_bedroom::ConnectedBedroom::update_connected_light_state_,
                                        entity_id);

    switch (std::get<2>(connected_light)) {
      case TEMPERATURE_VARIABLE_CONNECTED_LIGHT: {
        this->subscribe_homeassistant_state(
            &esphome::connected_bedroom::ConnectedBedroom::update_connected_light_temperature_, entity_id,
            "color_temp_kelvin");
        this->subscribe_homeassistant_state(
            &esphome::connected_bedroom::ConnectedBedroom::update_connected_light_brightness_, entity_id, "brightness");
        break;
      }

      case COLOR_VARIABLE_CONNECTED_LIGHT: {
        this->subscribe_homeassistant_state(
            &esphome::connected_bedroom::ConnectedBedroom::update_connected_light_color_, entity_id, "rgb_color");
        this->subscribe_homeassistant_state(
            &esphome::connected_bedroom::ConnectedBedroom::update_connected_light_temperature_, entity_id,
            "color_temp_kelvin");
        this->subscribe_homeassistant_state(
            &esphome::connected_bedroom::ConnectedBedroom::update_connected_light_brightness_, entity_id, "brightness");
        break;
      }

      case BINARY_CONNECTED_LIGHT: {
        break;
      }
    }
  }
}

void ConnectedBedroom::loop() {
  if (!synchronized_) {
    this->write('3');
    this->write('0');
    this->write('0');
    this->write('\n');

    synchronized_ = true;
  }

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
  String message;
  for (int i = 0; i < this->receivedMessage_.size(); i++)
    message += char(this->receivedMessage_[i]);
  ESP_LOGD(TAG, "Message received from Arduino: '%s'.", message.c_str());

  switch (getIntFromVector(this->receivedMessage_, 0, 1)) {
    case 0: {
      int communication_id = getIntFromVector(this->receivedMessage_, 1, 2);

      switch (getIntFromVector(this->receivedMessage_, 3, 2)) {
        case 0: {
          std::string connected_light_entity_id = this->get_connected_light_from_communication_id_(communication_id);

          if (connected_light_entity_id == "")
            break;

          switch (getIntFromVector(this->receivedMessage_, 5, 1)) {
            case 0: {
              this->call_homeassistant_service("light.turn_off", {{"entity_id", connected_light_entity_id}});
              break;
            }

            case 1: {
              this->call_homeassistant_service("light.turn_on", {{"entity_id", connected_light_entity_id}});
              break;
            }

            case 2: {
              this->call_homeassistant_service("light.toggle", {{"entity_id", connected_light_entity_id}});
              break;
            }
          }

          break;
        }

        case 4: {
          std::string connected_light_entity_id = this->get_connected_light_from_communication_id_(communication_id);

          if (connected_light_entity_id == "")
            break;

          switch (getIntFromVector(this->receivedMessage_, 5, 1)) {
            case 0: {
              this->call_homeassistant_service("light.turn_on",
                                               {{"entity_id", connected_light_entity_id},
                                                {"kelvin", to_string(getIntFromVector(this->receivedMessage_, 6, 4))}});
              break;
            }

            case 1: {
              this->call_homeassistant_service(
                  "light.turn_on", {{"entity_id", connected_light_entity_id},
                                    {"brightness", to_string(getIntFromVector(this->receivedMessage_, 6, 3))}});
              break;
            }
          }

          break;
        }

        case 5: {
          std::string connected_light_entity_id = this->get_connected_light_from_communication_id_(communication_id);

          if (connected_light_entity_id == "")
            break;

          switch (getIntFromVector(this->receivedMessage_, 5, 1)) {
            case 0: {
              this->call_homeassistant_service("script.esphome_changer_de_couleur",
                                               {{"light", connected_light_entity_id},
                                                {"r", to_string(getIntFromVector(this->receivedMessage_, 6, 3))},
                                                {"g", to_string(getIntFromVector(this->receivedMessage_, 9, 3))},
                                                {"b", to_string(getIntFromVector(this->receivedMessage_, 12, 3))}});
              break;
            }

            case 1: {
              this->call_homeassistant_service("light.turn_on",
                                               {{"entity_id", connected_light_entity_id},
                                                {"kelvin", to_string(getIntFromVector(this->receivedMessage_, 6, 4))}});
              break;
            }

            case 2: {
              this->call_homeassistant_service(
                  "light.turn_on", {{"entity_id", connected_light_entity_id},
                                    {"brightness", to_string(getIntFromVector(this->receivedMessage_, 6, 3))}});
              break;
            }
          }

          break;
        }
      }

      break;
    }

    case 1: {
      int communication_id = getIntFromVector(this->receivedMessage_, 1, 2);

      switch (getIntFromVector(this->receivedMessage_, 3, 2)) {
        case 1: {
          switch_::Switch *switch_ = this->get_switch_from_communication_id_(communication_id);

          if (switch_ != nullptr) {
            switch_->publish_state(getIntFromVector(this->receivedMessage_, 5, 1));

            break;
          }

          alarm_control_panel::AlarmControlPanel *alarm = this->get_alarm_from_communication_id_(communication_id);

          if (alarm != nullptr) {
            if (getIntFromVector(this->receivedMessage_, 5, 1) == 0)
              alarm->publish_state(alarm_control_panel::ACP_STATE_DISARMED);

            else if (getIntFromVector(this->receivedMessage_, 5, 1) == 1)
              alarm->publish_state(alarm_control_panel::ACP_STATE_ARMED_AWAY);

            break;
          }

          ConnectedBedroomTelevision *television = this->get_television_from_communication_id_(communication_id);

          if (television != nullptr) {
            television->state->publish_state(getIntFromVector(this->receivedMessage_, 5, 1));

            break;
          }

          ConnectedBedroomRGBLEDStrip *strip = this->get_RGB_LED_strip_from_communication_id(communication_id);

          if (strip != nullptr) {
            auto call = strip->state->make_call();
            call.set_state(getIntFromVector(this->receivedMessage_, 5, 1));
            strip->block_next_write();
            call.perform();
          }

          break;
        }

        case 2: {
          ConnectedBedroomRGBLEDStrip *strip = this->get_RGB_LED_strip_from_communication_id(communication_id);

          if (strip == nullptr)
            break;

          switch (getIntFromVector(this->receivedMessage_, 5, 1)) {
            case 0: {
              int r_int = getIntFromVector(this->receivedMessage_, 6, 3);
              int g_int = getIntFromVector(this->receivedMessage_, 9, 3);
              int b_int = getIntFromVector(this->receivedMessage_, 12, 3);

              float r_float = float(r_int) / 255.0f;
              float g_float = float(g_int) / 255.0f;
              float b_float = float(b_int) / 255.0f;

              auto call = strip->state->make_call();
              call.set_rgb(r_float, g_float, b_float);
              call.set_effect(0u);
              call.set_state(true);
              strip->block_next_write();
              call.perform();

              break;
            }

            case 1: {
              auto call = strip->state->make_call();
              call.set_effect("Arc-en-ciel");
              call.set_state(true);
              call.perform();

              break;
            }

            case 2: {
              auto call = strip->state->make_call();
              call.set_effect("Son-réaction");
              call.set_state(true);
              call.perform();

              break;
            }

            case 3: {
              auto call = strip->state->make_call();
              call.set_effect("Alarme");
              call.set_state(true);
              call.perform();

              break;
            }
          }

          break;
        }

        case 3: {
          switch (getIntFromVector(this->receivedMessage_, 5, 1)) {
            case 0: {
              alarm_control_panel::AlarmControlPanel *alarm = this->get_alarm_from_communication_id_(communication_id);

              if (alarm == nullptr)
                break;

              alarm->publish_state(alarm_control_panel::ACP_STATE_ARMED_AWAY);

              break;
            }

            case 1: {
              alarm_control_panel::AlarmControlPanel *alarm = this->get_alarm_from_communication_id_(communication_id);

              if (alarm == nullptr)
                break;

              alarm->publish_state(alarm_control_panel::ACP_STATE_TRIGGERED);

              break;
            }

            case 2: {
              number::Number *button = this->get_missile_launcher_base_number_from_communication_id_(communication_id);

              if (button == nullptr)
                break;

              button->publish_state(float(getIntFromVector(receivedMessage_, 6, 3)));

              break;
            }

            case 3: {
              number::Number *button = this->get_missile_launcher_angle_number_from_communication_id_(communication_id);

              if (button == nullptr)
                break;

              button->publish_state(float(getIntFromVector(receivedMessage_, 6, 3)));

              break;
            }

            case 4: {
              sensor::Sensor *sensor =
                  this->get_missile_launcher_available_missiles_sensor_from_communication_id_(communication_id);

              if (sensor == nullptr)
                break;

              int count = getIntFromVector(receivedMessage_, 6, 1) + getIntFromVector(receivedMessage_, 7, 1) +
                          getIntFromVector(receivedMessage_, 8, 1);

              sensor->publish_state(float(count));

              break;
            }
          }

          break;
        }

        case 4: {
          ConnectedBedroomTelevision *television = this->get_television_from_communication_id_(communication_id);

          if (television == nullptr)
            break;

          switch (getIntFromVector(this->receivedMessage_, 5, 1)) {
            case 0: {
              television->volume->publish_state(getIntFromVector(this->receivedMessage_, 6, 2));

              break;
            }

            case 1: {
              television->muted->publish_state(true);

              break;
            }

            case 2: {
              television->muted->publish_state(false);

              break;
            }
          }

          break;
        }

        case 7: {
          binary_sensor::BinarySensor *binary_sensor = this->get_binary_sensor_from_communication_id_(communication_id);

          if (binary_sensor == nullptr)
            break;

          binary_sensor->publish_state(getIntFromVector(this->receivedMessage_, 5, 1));

          break;
        }

        case 8: {
          sensor::Sensor *analog_sensor = this->get_analog_sensor_from_communication_id_(communication_id);

          if (analog_sensor == nullptr)
            break;

          analog_sensor->publish_state(getIntFromVector(this->receivedMessage_, 5, 4));

          break;
        }

        case 9: {
          sensor::Sensor *analog_sensor = this->get_analog_sensor_from_communication_id_(communication_id);

          if (analog_sensor == nullptr)
            break;

          analog_sensor->publish_state(float(getIntFromVector(this->receivedMessage_, 5, 4)) / float(100));

          analog_sensor = this->get_analog_sensor_from_communication_id_(communication_id + 1);

          if (analog_sensor == nullptr)
            break;

          analog_sensor->publish_state(float(getIntFromVector(this->receivedMessage_, 9, 4)) / float(100));

          break;
        }
      }
      break;
    }

    case 2: {
      std::string message;
      for (int i = 1; i < this->receivedMessage_.size(); i++)
        message.push_back(this->receivedMessage_[i]);

      this->call_homeassistant_service(
          "script.emettre_un_message",
          {{"message", message}, {"enceinte", "media_player.reveil_google_cast_de_la_chambre_de_louis"}});

      break;
    }

    case 3: {
      switch (getIntFromVector(receivedMessage_, 1, 2)) {
        case 1:
          this->write('3');
          this->write('0');
          this->write('0');
          this->write('\n');
          break;

        case 2:
          this->call_homeassistant_service("script.arreter_le_systeme_de_domotique_de_la_chambre_de_louis",
                                           {{"redemarrer", bool(getIntFromVector(receivedMessage_, 3, 1))}});
          break;
      }

      break;
    }

    case 4: {
      std::string url;
      for (int i = 1; i < this->receivedMessage_.size(); i++)
        url.push_back(this->receivedMessage_[i]);

      this->call_homeassistant_service(
        "script.jouer_musique_domotique_louis",
        {{"url", url}});

      break;
    }
  }

  this->receivedMessage_.clear();
}

void ConnectedBedroom::send_message_to_Arduino_(std::string title, std::string message) {
  std::replace(title.begin(), title.end(), '/', '.');
  std::replace(message.begin(), message.end(), '/', '.');

  this->write('2');
  this->write_str(title.c_str());
  this->write('/');
  this->write_str(message.c_str());
  this->write('\n');
}

void ConnectedBedroom::update_connected_light_state_(std::string entity_id, std::string state) {
  if (state == "None")
    return;

  this->write('1');
  this->write_str(addZeros(this->get_communication_id_from_connected_light_entity_id_(entity_id), 2).c_str());
  this->write('0');
  this->write('1');

  if (state == "on")
    this->write('1');

  else
    this->write('0');

  this->write('\n');
}

void ConnectedBedroom::update_connected_light_brightness_(std::string entity_id, std::string state) {
  if (state == "None")
    return;

  this->write('1');

  int id = this->get_communication_id_from_connected_light_entity_id_(entity_id);
  this->write_str(addZeros(id, 2).c_str());

  switch (this->get_type_from_connected_light_communication_id_(id)) {
    case TEMPERATURE_VARIABLE_CONNECTED_LIGHT: {
      this->write('0');
      this->write('5');
      this->write('3');
      break;
    }

    case COLOR_VARIABLE_CONNECTED_LIGHT: {
      this->write('0');
      this->write('6');
      this->write('4');
      break;
    }

    case BINARY_CONNECTED_LIGHT:
      break;
  }

  this->write_str(addZeros(std::stoi(state), 3).c_str());

  this->write('\n');
}

void ConnectedBedroom::update_connected_light_temperature_(std::string entity_id, std::string state) {
  if (state == "None")
    return;

  this->write('1');

  int id = this->get_communication_id_from_connected_light_entity_id_(entity_id);
  this->write_str(addZeros(id, 2).c_str());

  switch (this->get_type_from_connected_light_communication_id_(id)) {
    case TEMPERATURE_VARIABLE_CONNECTED_LIGHT: {
      this->write('0');
      this->write('5');
      this->write('2');
      break;
    }

    case COLOR_VARIABLE_CONNECTED_LIGHT: {
      this->write('0');
      this->write('6');
      this->write('3');
      break;
    }

    case BINARY_CONNECTED_LIGHT:
      break;
  }

  this->write_str(addZeros(std::stoi(state), 4).c_str());

  this->write('\n');
}

void ConnectedBedroom::update_connected_light_color_(std::string entity_id, std::string state) {
  if (state == "None")
    return;

  this->write('1');

  int id = this->get_communication_id_from_connected_light_entity_id_(entity_id);
  this->write_str(addZeros(id, 2).c_str());

  this->write('0');
  this->write('6');
  this->write('2');

  std::istringstream ss(state);
  char discard;
  int r, g, b;
  ss >> discard >> r >> discard >> g >> discard >> b >> discard;

  this->write_str(addZeros(r, 3).c_str());
  this->write_str(addZeros(g, 3).c_str());
  this->write_str(addZeros(b, 3).c_str());

  this->write('\n');
}

void ConnectedBedroom::dump_config() {
  /*ESP_LOGCONFIG(TAG, "Connected bedroom:");

  ESP_LOGCONFIG(TAG, "  Analog sensors:");
  for (auto entity : this->analog_sensors_) {
    ESP_LOGCONFIG(TAG, "    Communication id: %d", entity.first);
    LOG_SENSOR("", "      ", entity.second);
  }

  ESP_LOGCONFIG(TAG, "  Binary sensors:");
  for (auto entity : this->binary_sensors_) {
    ESP_LOGCONFIG(TAG, "    Communication id: %d", entity.first);
    LOG_BINARY_SENSOR("", "      ", entity.second);
  }

  ESP_LOGCONFIG(TAG, "  Switches:");
  for (auto entity : this->switches_) {
    ESP_LOGCONFIG(TAG, "    Communication id: %d", entity.first);
    LOG_SWITCH(TAG, "      ", entity.second);
  }

  ESP_LOGCONFIG(TAG, "  Alarms:");
  for (auto entity : this->alarms_) {
    ESP_LOGCONFIG(TAG, "    Communication id: %d", entity.first);
  }

  ESP_LOGCONFIG(TAG, "  Connected lights:");
  for (auto entity : this->connected_lights_) {
    ESP_LOGCONFIG(TAG, "    Communication id: %d", std::get<0>(entity));
    ESP_LOGCONFIG(TAG, "      Entity id: %s", std::get<1>(entity).c_str());
  }*/
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

void ConnectedBedroom::add_alarm(int communication_id, alarm_control_panel::AlarmControlPanel *alarm) {
  auto it =
      std::find_if(alarms_.begin(), alarms_.end(),
                   [communication_id](const std::tuple<int, alarm_control_panel::AlarmControlPanel *, number::Number *,
                                                       number::Number *, button::Button *, sensor::Sensor *> &element) {
                     return std::get<0>(element) == communication_id;
                   });

  if (it != alarms_.end()) {
    std::get<1>(*it) = alarm;
  } else {
    this->alarms_.push_back(std::make_tuple(communication_id, alarm, nullptr, nullptr, nullptr, nullptr));
  }
}

void ConnectedBedroom::add_alarm_missile_launcher_base_number(int communication_id, number::Number *number) {
  auto it =
      std::find_if(alarms_.begin(), alarms_.end(),
                   [communication_id](const std::tuple<int, alarm_control_panel::AlarmControlPanel *, number::Number *,
                                                       number::Number *, button::Button *, sensor::Sensor *> &element) {
                     return std::get<0>(element) == communication_id;
                   });

  if (it != alarms_.end()) {
    std::get<2>(*it) = number;
  } else {
    this->alarms_.push_back(std::make_tuple(communication_id, nullptr, number, nullptr, nullptr, nullptr));
  }
}

void ConnectedBedroom::add_alarm_missile_launcher_angle_number(int communication_id, number::Number *number) {
  auto it =
      std::find_if(alarms_.begin(), alarms_.end(),
                   [communication_id](const std::tuple<int, alarm_control_panel::AlarmControlPanel *, number::Number *,
                                                       number::Number *, button::Button *, sensor::Sensor *> &element) {
                     return std::get<0>(element) == communication_id;
                   });

  if (it != alarms_.end()) {
    std::get<3>(*it) = number;
  } else {
    this->alarms_.push_back(std::make_tuple(communication_id, nullptr, nullptr, number, nullptr, nullptr));
  }
}

void ConnectedBedroom::add_alarm_missile_launcher_launch_button(int communication_id, button::Button *button) {
  auto it =
      std::find_if(alarms_.begin(), alarms_.end(),
                   [communication_id](const std::tuple<int, alarm_control_panel::AlarmControlPanel *, number::Number *,
                                                       number::Number *, button::Button *, sensor::Sensor *> &element) {
                     return std::get<0>(element) == communication_id;
                   });

  if (it != alarms_.end()) {
    std::get<4>(*it) = button;
  } else {
    this->alarms_.push_back(std::make_tuple(communication_id, nullptr, nullptr, nullptr, button, nullptr));
  }
}

void ConnectedBedroom::add_alarm_missile_launcher_available_missiles_sensor(int communication_id,
                                                                            sensor::Sensor *sensor) {
  auto it =
      std::find_if(alarms_.begin(), alarms_.end(),
                   [communication_id](const std::tuple<int, alarm_control_panel::AlarmControlPanel *, number::Number *,
                                                       number::Number *, button::Button *, sensor::Sensor *> &element) {
                     return std::get<0>(element) == communication_id;
                   });

  if (it != alarms_.end()) {
    std::get<5>(*it) = sensor;
  } else {
    this->alarms_.push_back(std::make_tuple(communication_id, nullptr, nullptr, nullptr, nullptr, sensor));
  }
}

void ConnectedBedroom::add_television(int communication_id, ConnectedBedroomTelevision *television) {
  this->televisions_.push_back(std::make_pair(communication_id, television));
}

void ConnectedBedroom::add_connected_light(int communication_id, std::string entity_id, ConnectedLightTypes type) {
  this->connected_lights_.push_back(std::make_tuple(communication_id, entity_id, type));
}

void ConnectedBedroom::add_RGB_LED_strip(int communication_id, ConnectedBedroomRGBLEDStrip *light) {
  this->RGB_LED_strips_.push_back(std::make_pair(communication_id, light));
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

alarm_control_panel::AlarmControlPanel *ConnectedBedroom::get_alarm_from_communication_id_(int communication_id) const {
  auto it =
      std::find_if(alarms_.begin(), alarms_.end(),
                   [communication_id](const std::tuple<int, alarm_control_panel::AlarmControlPanel *, number::Number *,
                                                       number::Number *, button::Button *, sensor::Sensor *> &element) {
                     return std::get<0>(element) == communication_id;
                   });

  if (it != alarms_.end()) {
    return std::get<1>(*it);
  } else {
    return nullptr;
  }
}

number::Number *ConnectedBedroom::get_missile_launcher_base_number_from_communication_id_(int communication_id) const {
  auto it =
      std::find_if(alarms_.begin(), alarms_.end(),
                   [communication_id](const std::tuple<int, alarm_control_panel::AlarmControlPanel *, number::Number *,
                                                       number::Number *, button::Button *, sensor::Sensor *> &element) {
                     return std::get<0>(element) == communication_id;
                   });

  if (it != alarms_.end()) {
    return std::get<2>(*it);
  } else {
    return nullptr;
  }
}

number::Number *ConnectedBedroom::get_missile_launcher_angle_number_from_communication_id_(int communication_id) const {
  auto it =
      std::find_if(alarms_.begin(), alarms_.end(),
                   [communication_id](const std::tuple<int, alarm_control_panel::AlarmControlPanel *, number::Number *,
                                                       number::Number *, button::Button *, sensor::Sensor *> &element) {
                     return std::get<0>(element) == communication_id;
                   });

  if (it != alarms_.end()) {
    return std::get<3>(*it);
  } else {
    return nullptr;
  }
}

button::Button *ConnectedBedroom::get_missile_launcher_launch_button_from_communication_id_(
    int communication_id) const {
  auto it =
      std::find_if(alarms_.begin(), alarms_.end(),
                   [communication_id](const std::tuple<int, alarm_control_panel::AlarmControlPanel *, number::Number *,
                                                       number::Number *, button::Button *, sensor::Sensor *> &element) {
                     return std::get<0>(element) == communication_id;
                   });

  if (it != alarms_.end()) {
    return std::get<4>(*it);
  } else {
    return nullptr;
  }
}

sensor::Sensor *ConnectedBedroom::get_missile_launcher_available_missiles_sensor_from_communication_id_(
    int communication_id) const {
  auto it =
      std::find_if(alarms_.begin(), alarms_.end(),
                   [communication_id](const std::tuple<int, alarm_control_panel::AlarmControlPanel *, number::Number *,
                                                       number::Number *, button::Button *, sensor::Sensor *> &element) {
                     return std::get<0>(element) == communication_id;
                   });

  if (it != alarms_.end()) {
    return std::get<5>(*it);
  } else {
    return nullptr;
  }
}

ConnectedBedroomTelevision *ConnectedBedroom::get_television_from_communication_id_(int communication_id) const {
  auto it = std::find_if(televisions_.begin(), televisions_.end(),
                         [communication_id](const std::pair<int, ConnectedBedroomTelevision *> &element) {
                           return element.first == communication_id;
                         });

  if (it != televisions_.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

ConnectedBedroomRGBLEDStrip *ConnectedBedroom::get_RGB_LED_strip_from_communication_id(int communication_id) const {
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

std::string ConnectedBedroom::get_connected_light_from_communication_id_(int communication_id) const {
  auto it = std::find_if(connected_lights_.begin(), connected_lights_.end(),
                         [communication_id](const std::tuple<int, std::string, ConnectedLightTypes> &element) {
                           return std::get<0>(element) == communication_id;
                         });

  if (it != connected_lights_.end()) {
    return std::get<1>(*it);
  } else {
    return "";
  }
}

int ConnectedBedroom::get_communication_id_from_connected_light_entity_id_(std::string entity_id) const {
  auto it = std::find_if(connected_lights_.begin(), connected_lights_.end(),
                         [entity_id](const std::tuple<int, std::string, ConnectedLightTypes> &element) {
                           return std::get<1>(element) == entity_id;
                         });

  if (it != connected_lights_.end()) {
    return std::get<0>(*it);
  } else {
    return -1;
  }
}

ConnectedLightTypes ConnectedBedroom::get_type_from_connected_light_communication_id_(int communication_id) const {
  auto it = std::find_if(connected_lights_.begin(), connected_lights_.end(),
                         [communication_id](const std::tuple<int, std::string, ConnectedLightTypes> &element) {
                           return std::get<0>(element) == communication_id;
                         });

  if (it != connected_lights_.end()) {
    return std::get<2>(*it);
  } else {
    return BINARY_CONNECTED_LIGHT;
  }
}

void ConnectedBedroomDevice::set_communication_id(int communication_id) { this->communication_id_ = communication_id; }

void ConnectedBedroomDevice::set_parent(ConnectedBedroom *parent) {
  this->parent_ = parent;

  this->register_device();
}

ConnectedBedroom *ConnectedBedroomDevice::get_parent() const { return this->parent_; }

void ConnectedBedroomSwitch::register_device() { this->parent_->add_switch(this->communication_id_, this); }

void ConnectedBedroomSwitch::write_state(bool state) {
  this->parent_->write('0');
  this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());
  this->parent_->write('0');
  this->parent_->write('0');
  this->parent_->write(state ? '1' : '0');
  this->parent_->write('\n');
}

void ConnectedBedroomAlarmControlPanel::register_device() { this->parent_->add_alarm(this->communication_id_, this); }

uint32_t ConnectedBedroomAlarmControlPanel::get_supported_features() const {
  uint32_t features = alarm_control_panel::ACP_FEAT_ARM_AWAY | alarm_control_panel::ACP_FEAT_TRIGGER;
  return features;
}

bool ConnectedBedroomAlarmControlPanel::get_requires_code() const { return !this->codes_.empty(); }

bool ConnectedBedroomAlarmControlPanel::get_requires_code_to_arm() const { return true; }

void ConnectedBedroomAlarmControlPanel::add_code(const std::string &code) { this->codes_.push_back(code); }

void ConnectedBedroomAlarmControlPanel::control(const alarm_control_panel::AlarmControlPanelCall &call) {
  if (!call.get_state())
    return;

  if (!this->codes_.empty()) {
    if (!call.get_code().has_value())
      return;

    if (!(std::count(this->codes_.begin(), this->codes_.end(), call.get_code().value()) == 1))
      return;
  }

  this->parent_->write('0');
  this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());

  if (call.get_state() == alarm_control_panel::ACP_STATE_ARMED_AWAY &&
      this->current_state_ == alarm_control_panel::ACP_STATE_DISARMED) {
    this->parent_->write('0');
    this->parent_->write('0');
    this->parent_->write('1');
  }

  else if (call.get_state() == alarm_control_panel::ACP_STATE_ARMED_AWAY &&
           this->current_state_ == alarm_control_panel::ACP_STATE_TRIGGERED) {
    this->parent_->write('0');
    this->parent_->write('2');
    this->parent_->write('0');
  }

  else if (call.get_state() == alarm_control_panel::ACP_STATE_DISARMED &&
           this->current_state_ != alarm_control_panel::ACP_STATE_DISARMED) {
    this->parent_->write('0');
    this->parent_->write('0');
    this->parent_->write('0');
  }

  else if (call.get_state() == alarm_control_panel::ACP_STATE_PENDING &&
           this->current_state_ != alarm_control_panel::ACP_STATE_TRIGGERED) {
    this->parent_->write('0');
    this->parent_->write('2');
    this->parent_->write('1');
  }

  this->parent_->write('\n');
}

void ConnectedBedroomMissileLauncherBaseNumber::register_device() {
  this->parent_->add_alarm_missile_launcher_base_number(this->communication_id_, this);
}

void ConnectedBedroomMissileLauncherBaseNumber::control(float value) {
  this->parent_->write('0');
  this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());
  this->parent_->write('0');
  this->parent_->write('2');
  this->parent_->write('2');
  this->parent_->write_str(addZeros(value, 3).c_str());
  this->parent_->write('\n');
}

void ConnectedBedroomMissileLauncherAngleNumber::register_device() {
  this->parent_->add_alarm_missile_launcher_angle_number(this->communication_id_, this);
}

void ConnectedBedroomMissileLauncherAngleNumber::control(float value) {
  this->parent_->write('0');
  this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());
  this->parent_->write('0');
  this->parent_->write('2');
  this->parent_->write('3');
  this->parent_->write_str(addZeros(value, 3).c_str());
  this->parent_->write('\n');
}

void ConnectedBedroomMissileLauncherLaunchButton::register_device() {
  this->parent_->add_alarm_missile_launcher_launch_button(this->communication_id_, this);
}

void ConnectedBedroomMissileLauncherLaunchButton::press_action() {
  this->parent_->write('0');
  this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());
  this->parent_->write('0');
  this->parent_->write('2');
  this->parent_->write('4');
  this->parent_->write('\n');
}

void TelevisionComponent::set_parent(ConnectedBedroomTelevision *parent) {
  this->parent_ = parent;

  this->register_component();
}

void TelevisionState::register_component() { this->parent_->state = this; }

void TelevisionState::write_state(bool state) {
  this->parent_->parent_->write('0');
  this->parent_->parent_->write_str(addZeros(this->parent_->communication_id_, 2).c_str());
  this->parent_->parent_->write('0');
  this->parent_->parent_->write('0');
  this->parent_->parent_->write(state ? '1' : '0');
  this->parent_->parent_->write('\n');
}

void TelevisionMuted::register_component() { this->parent_->muted = this; }

void TelevisionMuted::write_state(bool state) {
  this->parent_->parent_->write('0');
  this->parent_->parent_->write_str(addZeros(this->parent_->communication_id_, 2).c_str());
  this->parent_->parent_->write('0');
  this->parent_->parent_->write('3');
  this->parent_->parent_->write(state ? '2' : '3');
  this->parent_->parent_->write('\n');
}

void TelevisionVolumeUp::register_component() { this->parent_->volume_up = this; }

void TelevisionVolumeUp::press_action() {
  this->parent_->parent_->write('0');
  this->parent_->parent_->write_str(addZeros(this->parent_->communication_id_, 2).c_str());
  this->parent_->parent_->write('0');
  this->parent_->parent_->write('3');
  this->parent_->parent_->write('1');
  this->parent_->parent_->write('\n');
}

void TelevisionVolumeDown::register_component() { this->parent_->volume_down = this; }

void TelevisionVolumeDown::press_action() {
  this->parent_->parent_->write('0');
  this->parent_->parent_->write_str(addZeros(this->parent_->communication_id_, 2).c_str());
  this->parent_->parent_->write('0');
  this->parent_->parent_->write('3');
  this->parent_->parent_->write('0');
  this->parent_->parent_->write('\n');
}

void ConnectedBedroomTelevision::register_device() { this->parent_->add_television(this->communication_id_, this); }

void ConnectedBedroomTelevision::setVolumeSensor(sensor::Sensor *sens) { this->volume = sens; }

void ConnectedBedroomRGBLEDStrip::register_device() { this->parent_->add_RGB_LED_strip(this->communication_id_, this); }

light::LightTraits ConnectedBedroomRGBLEDStrip::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::RGB});
  return traits;
}

void ConnectedBedroomRGBLEDStrip::setup_state(light::LightState *state) { this->state = state; }

void ConnectedBedroomRGBLEDStrip::write_state(light::LightState *state) {
  if (this->block_next_write_) {
    this->block_next_write_ = false;

    return;
  }

  if (!this->previous_state_ && this->state->remote_values.get_state()) {
    previous_state_ = true;

    this->parent_->write('0');
    this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());
    this->parent_->write('0');
    this->parent_->write('0');
    this->parent_->write('1');
    this->parent_->write('\n');
  }

  else if (this->previous_state_ && !this->state->remote_values.get_state()) {
    previous_state_ = false;

    this->parent_->write('0');
    this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());
    this->parent_->write('0');
    this->parent_->write('0');
    this->parent_->write('0');
    this->parent_->write('\n');

    return;
  }

  if (state->get_effect_name() == "Arc-en-ciel") {
    this->parent_->write('0');
    this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());
    this->parent_->write('0');
    this->parent_->write('1');
    this->parent_->write('1');
    this->parent_->write('\n');

    return;
  }

  else if (state->get_effect_name() == "Son-réaction") {
    this->parent_->write('0');
    this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());
    this->parent_->write('0');
    this->parent_->write('1');
    this->parent_->write('2');
    this->parent_->write('\n');

    return;
  }

  else if (state->get_effect_name() == "Alarme") {
    this->parent_->write('0');
    this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());
    this->parent_->write('0');
    this->parent_->write('1');
    this->parent_->write('3');
    this->parent_->write('\n');

    return;
  }

  float r, g, b;
  state->current_values_as_rgb(&r, &g, &b, false);

  int red = int(r * 255.0f);
  int green = int(g * 255.0f);
  int blue = int(b * 255.0f);

  this->parent_->write('0');
  this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());
  this->parent_->write('0');
  this->parent_->write('1');
  this->parent_->write('0');
  this->parent_->write_str(addZeros(red, 3).c_str());
  this->parent_->write_str(addZeros(green, 3).c_str());
  this->parent_->write_str(addZeros(blue, 3).c_str());
  this->parent_->write('\n');
}

void ConnectedBedroomRGBLEDStrip::block_next_write() { this->block_next_write_ = true; }

ConnectedBedroomRGBLEDStripRainbowEffect::ConnectedBedroomRGBLEDStripRainbowEffect(const std::string &name)
    : LightEffect(name) {}

void ConnectedBedroomRGBLEDStripRainbowEffect::apply() {}

ConnectedBedroomRGBLEDStripSoundreactEffect::ConnectedBedroomRGBLEDStripSoundreactEffect(const std::string &name)
    : LightEffect(name) {}
void ConnectedBedroomRGBLEDStripSoundreactEffect::apply() {}

ConnectedBedroomRGBLEDStripAlarmEffect::ConnectedBedroomRGBLEDStripAlarmEffect(const std::string &name)
    : LightEffect(name) {}

void ConnectedBedroomRGBLEDStripAlarmEffect::apply() {}

}  // namespace connected_bedroom
}  // namespace esphome