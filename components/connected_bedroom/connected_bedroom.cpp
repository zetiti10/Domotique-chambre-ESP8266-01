#include "connected_bedroom.h"
#include "esphome/core/log.h"
#include <sstream>

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

  for (auto light : this->connected_lights_) {
    std::string &entity_id = std::get<1>(light);

    this->subscribe_homeassistant_state(&esphome::connected_bedroom::ConnectedBedroom::update_connected_light_state_,
                                        entity_id);

    switch (std::get<2>(light)) {
      case TEMPERATURE_VARIABLE_CONNECTED_LIGHT:
        this->subscribe_homeassistant_state(
            &esphome::connected_bedroom::ConnectedBedroom::update_connected_light_temperature_, entity_id,
            "color_temp_kelvin");
        this->subscribe_homeassistant_state(
            &esphome::connected_bedroom::ConnectedBedroom::update_connected_light_brightness_, entity_id, "brightness");
        break;

      case COLOR_VARIABLE_CONNECTED_LIGHT:
        this->subscribe_homeassistant_state(
            &esphome::connected_bedroom::ConnectedBedroom::update_connected_light_color_, entity_id, "rgb_color");
        this->subscribe_homeassistant_state(
            &esphome::connected_bedroom::ConnectedBedroom::update_connected_light_temperature_, entity_id,
            "color_temp_kelvin");
        this->subscribe_homeassistant_state(
            &esphome::connected_bedroom::ConnectedBedroom::update_connected_light_brightness_, entity_id, "brightness");
        break;

      case BINARY_CONNECTED_LIGHT:
        break;
    }
  }
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
  switch (getIntFromVector(this->receivedMessage_, 0, 1)) {
    case 0: {
      int ID = getIntFromVector(this->receivedMessage_, 1, 2);

      switch (getIntFromVector(this->receivedMessage_, 3, 2)) {
        case 0: {
          std::string light = this->get_connected_light_from_communication_id_(ID);

          if (light != "") {
            switch (getIntFromVector(this->receivedMessage_, 5, 1)) {
              case 0: {
                this->call_homeassistant_service("light.turn_off", {{"entity_id", light}});
                break;
              }

              case 1: {
                this->call_homeassistant_service("light.turn_on", {{"entity_id", light}});
                break;
              }

              case 2: {
                this->call_homeassistant_service("light.toggle", {{"entity_id", light}});
                break;
              }
            }
          }

          break;
        }

        case 4: {
          std::string light = this->get_connected_light_from_communication_id_(ID);

          switch (getIntFromVector(this->receivedMessage_, 5, 1)) {
            case 0: {
              this->call_homeassistant_service(
                  "light.turn_on",
                  {{"entity_id", light}, {"kelvin", to_string(getIntFromVector(this->receivedMessage_, 6, 4))}});
              break;
            }

            case 1: {
              this->call_homeassistant_service(
                  "light.turn_on",
                  {{"entity_id", light}, {"brightness", to_string(getIntFromVector(this->receivedMessage_, 6, 3))}});
              break;
            }
          }
          break;
        }

        case 5: {
          std::string light = this->get_connected_light_from_communication_id_(ID);

          switch (getIntFromVector(this->receivedMessage_, 5, 1)) {
            case 0: {
              this->call_homeassistant_service("script.esphome_changer_de_couleur",
                                               {{"lumiere", light},
                                                {"r", to_string(getIntFromVector(this->receivedMessage_, 6, 3))},
                                                {"g", to_string(getIntFromVector(this->receivedMessage_, 9, 3))},
                                                {"b", to_string(getIntFromVector(this->receivedMessage_, 12, 3))}});
              break;
            }

            case 1: {
              this->call_homeassistant_service(
                  "light.turn_on",
                  {{"entity_id", light}, {"kelvin", to_string(getIntFromVector(this->receivedMessage_, 6, 4))}});
              break;
            }

            case 2: {
              this->call_homeassistant_service(
                  "light.turn_on",
                  {{"entity_id", light}, {"brightness", to_string(getIntFromVector(this->receivedMessage_, 6, 3))}});
              break;
            }
          }
          break;
        }
      }

      break;
    }

    case 1: {
      int ID = getIntFromVector(this->receivedMessage_, 1, 2);

      switch (getIntFromVector(this->receivedMessage_, 3, 2)) {
        case 1: {
          switch_::Switch *switch_ = this->get_switch_from_communication_id_(ID);

          if (switch_ != nullptr) {
            switch_->publish_state(getIntFromVector(this->receivedMessage_, 5, 1));

            break;
          }

          alarm_control_panel::AlarmControlPanel *alarm = this->get_alarm_from_communication_id_(ID);

          if (alarm != nullptr) {
            if (getIntFromVector(this->receivedMessage_, 5, 1) == 0)
              alarm->publish_state(alarm_control_panel::ACP_STATE_DISARMED);

            else if (getIntFromVector(this->receivedMessage_, 5, 1) == 1)
              alarm->publish_state(alarm_control_panel::ACP_STATE_ARMED_AWAY);

            break;
          }

          media_player::MediaPlayer *television = this->get_television_from_communication_id(ID);

          if (television != nullptr) {
            if (getIntFromVector(this->receivedMessage_, 5, 1) == 0) {
              television->state = media_player::MEDIA_PLAYER_STATE_NONE;
              television->publish_state();
            }

            else if (getIntFromVector(this->receivedMessage_, 5, 1) == 1) {
              television->state = media_player::MEDIA_PLAYER_STATE_IDLE;
              television->publish_state();
            }
          }

          break;
        }

        case 3: {
          alarm_control_panel::AlarmControlPanel *alarm = this->get_alarm_from_communication_id_(ID);

          if (alarm == nullptr)
            break;

          if (getIntFromVector(this->receivedMessage_, 5, 1) == 0)
            alarm->publish_state(alarm_control_panel::ACP_STATE_ARMED_AWAY);

          else if (getIntFromVector(this->receivedMessage_, 5, 1) == 1)
            alarm->publish_state(alarm_control_panel::ACP_STATE_TRIGGERED);

          break;
        }

        case 4: {
          ConnectedBedroomMediaPlayer *television =
              dynamic_cast<ConnectedBedroomMediaPlayer *>(this->get_television_from_communication_id(ID));

          if (television == nullptr)
            break;

          switch (getIntFromVector(this->receivedMessage_, 5, 1)) {
            case 0: {
              int volume = map(long(getIntFromVector(this->receivedMessage_, 6, 2)), 0.0l, 25.0, 0.0, 1.0);

              television->volume = volume;
              television->publish_state();
              break;
            }

            case 1: {
              television->muted_ = true;
              television->publish_state();
              break;
            }
          }

          case 2: {
            television->muted_ = false;
            television->publish_state();
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

    case 2: {
      std::string message;
      for (int i = 1; i < this->receivedMessage_.size(); i++)
        message.push_back(this->receivedMessage_[i]);

      this->call_homeassistant_service(
          "script.emettre_un_message",
          {{"message", message}, {"enceinte", "media_player.reveil_google_cast_de_la_chambre_de_louis"}});

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

  switch (this->get_type_from_connected_light_communication_id(id)) {
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

  switch (this->get_type_from_connected_light_communication_id(id)) {
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
  ESP_LOGCONFIG(TAG, "Connected bedroom");

  ESP_LOGCONFIG(TAG, "Analog sensors");
  for (auto sens : this->analog_sensors_) {
    ESP_LOGCONFIG(TAG, "Communication id: %d", sens.first);
    LOG_SENSOR(TAG, "", sens.second);
  }

  ESP_LOGCONFIG(TAG, "Binary sensors");
  for (auto sens : this->binary_sensors_) {
    ESP_LOGCONFIG(TAG, "Communication id: %d", sens.first);
    LOG_BINARY_SENSOR(TAG, "", sens.second);
  }

  ESP_LOGCONFIG(TAG, "Switches");
  for (auto sens : this->switches_) {
    ESP_LOGCONFIG(TAG, "Communication id: %d", sens.first);
    LOG_SWITCH(TAG, "", sens.second);
  }

  ESP_LOGCONFIG(TAG, "Alarms");
  for (auto sens : this->alarms_) {
    ESP_LOGCONFIG(TAG, "Communication id: %d", sens.first);
  }

  ESP_LOGCONFIG(TAG, "Connected lights");
  for (auto light : this->connected_lights_) {
    ESP_LOGCONFIG(TAG, "Entity id: %s", std::get<1>(light).c_str());
    ESP_LOGCONFIG(TAG, "Communication id: %d", std::get<0>(light));
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

void ConnectedBedroom::add_alarm(int communication_id, alarm_control_panel::AlarmControlPanel *alarm) {
  this->alarms_.push_back(std::make_pair(communication_id, alarm));
}

void ConnectedBedroom::add_television(int communication_id, media_player::MediaPlayer *media_player_) {
  this->televisions_.push_back(std::make_pair(communication_id, media_player_));
}

void ConnectedBedroom::add_connected_light(int communication_id, std::string entity_id, ConnectedLightTypes type) {
  this->connected_lights_.push_back(std::make_tuple(communication_id, entity_id, type));
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
  auto it = std::find_if(alarms_.begin(), alarms_.end(),
                         [communication_id](const std::pair<int, alarm_control_panel::AlarmControlPanel *> &element) {
                           return element.first == communication_id;
                         });

  if (it != alarms_.end()) {
    return it->second;
  } else {
    return nullptr;
  }
}

media_player::MediaPlayer *ConnectedBedroom::get_television_from_communication_id(int communication_id) const {
  auto it = std::find_if(televisions_.begin(), televisions_.end(),
                         [communication_id](const std::pair<int, media_player::MediaPlayer *> &element) {
                           return element.first == communication_id;
                         });

  if (it != televisions_.end()) {
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

ConnectedLightTypes ConnectedBedroom::get_type_from_connected_light_communication_id(int communication_id) const {
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

void ConnectedBedroomAlarmControlPanel::dump_config() {
  ESP_LOGCONFIG(TAG, "ConnectedBedroomAlarmControlPanel");
  ESP_LOGCONFIG(TAG, "  Current State: %s", LOG_STR_ARG(alarm_control_panel_state_to_string(this->current_state_)));
  ESP_LOGCONFIG(TAG, "  Number of Codes: %u", this->codes_.size());
  if (!this->codes_.empty())
    ESP_LOGCONFIG(TAG, "  Requires Code To Arm: %s", YESNO(this->get_requires_code_to_arm()));
  ESP_LOGCONFIG(TAG, "  Supported Features: %" PRIu32, this->get_supported_features());
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

void ConnectedBedroomMediaPlayer::dump_config() {
  ESP_LOGCONFIG(TAG, "ConnectedBedroomMediaPlayer");
  ESP_LOGCONFIG(TAG, "  Current State: %s", LOG_STR_ARG(media_player::media_player_state_to_string(this->state)));
  ESP_LOGCONFIG(TAG, "  Volume: %u", this->volume);
}

void ConnectedBedroomMediaPlayer::register_device() { this->parent_->add_television(this->communication_id_, this); }

bool ConnectedBedroomMediaPlayer::is_muted() const { return muted_; }

media_player::MediaPlayerTraits ConnectedBedroomMediaPlayer::get_traits() {
  auto traits = media_player::MediaPlayerTraits();
  traits.set_supports_pause(false);
  return traits;
}

void ConnectedBedroomMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  this->parent_->write('0');
  this->parent_->write_str(addZeros(this->communication_id_, 2).c_str());

  /*if (call.get_volume().has_value()) {
    this->parent_->write('0');
    this->parent_->write('3');

    if (this->volume > call.get_volume().value())
      this->parent_->write('0');

    else if (this->volume < call.get_volume().value())
      this->parent_->write('1');
  }*/

  if (call.get_command().has_value()) {
    switch (call.get_command().value()) {
      case media_player::MEDIA_PLAYER_COMMAND_MUTE: {
        this->parent_->write('0');
        this->parent_->write('3');
        this->parent_->write('2');
        break;
      }

      case media_player::MEDIA_PLAYER_COMMAND_UNMUTE: {
        this->parent_->write('0');
        this->parent_->write('3');
        this->parent_->write('3');
        break;
      }

      case media_player::MEDIA_PLAYER_COMMAND_PLAY: {
        this->parent_->write('0');
        this->parent_->write('0');
        this->parent_->write('1');
        break;
      }

      case media_player::MEDIA_PLAYER_COMMAND_STOP: {
        this->parent_->write('0');
        this->parent_->write('0');
        this->parent_->write('0');
        break;
      }

      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_UP: {
        this->parent_->write('0');
        this->parent_->write('3');
        this->parent_->write('1');
        break;
      }

      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_DOWN: {
        this->parent_->write('0');
        this->parent_->write('3');
        this->parent_->write('0');
        break;
      }
    }
  }

  this->parent_->write('\n');
}

}  // namespace connected_bedroom
}  // namespace esphome

// Se baser sur tuya light
// Comment update le state ?