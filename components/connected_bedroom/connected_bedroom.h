#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/button/button.h"
#include "esphome/components/alarm_control_panel/alarm_control_panel.h"
#include "esphome/components/number/number.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_effect.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/api/custom_api_device.h"

namespace esphome {
namespace connected_bedroom {

/// @brief Types de périphériques disponibles à la connexion au système de domotique, pour pouvoir les contrôler depuis
/// celui-ci.
enum ConnectedDeviceTypes {
  BINARY_CONNECTED_DEVICE,
  TEMPERATURE_VARIABLE_CONNECTED_LIGHT,
  COLOR_VARIABLE_CONNECTED_LIGHT
};

class ConnectedBedroomTelevision;
class ConnectedBedroomRGBLEDStrip;

/// @brief Classe de gestion de la communication entre l'Arduino Mega et Home Assistant.
class ConnectedBedroom : public Component, public uart::UARTDevice, public api::CustomAPIDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  // Méthodes permettant d'enregistrer les périphériques utilisés à l'initialisation.
  void add_analog_sensor(int communication_id, sensor::Sensor *analog_sensor);
  void add_binary_sensor(int communication_id, binary_sensor::BinarySensor *binary_sensor);
  void add_switch(int communication_id, switch_::Switch *switch_);
  void add_alarm(int communication_id, alarm_control_panel::AlarmControlPanel *alarm);
  void add_alarm_missile_launcher_base_number(int communication_id, number::Number *number);
  void add_alarm_missile_launcher_angle_number(int communication_id, number::Number *number);
  void add_alarm_missile_launcher_launch_button(int communication_id, button::Button *button);
  void add_alarm_missile_launcher_available_missiles_sensor(int communication_id, sensor::Sensor *sensor);
  void add_television(int communication_id, ConnectedBedroomTelevision *television);
  void add_connected_device(int communication_id, std::string entity_id, ConnectedDeviceTypes type);
  void add_RGB_LED_strip(int communication_id, ConnectedBedroomRGBLEDStrip *light);

 protected:
  void process_message_();

  void send_message_to_Arduino_(std::string title, std::string message);

  // Méthodes permettant d'envoyer une mise à jour de l'état d'un périphériques connecté depuis Home Assistant.
  void update_connected_device_state_(std::string entity_id, std::string state);
  void update_connected_light_brightness_(std::string entity_id, std::string state);
  void update_connected_light_temperature_(std::string entity_id, std::string state);
  void update_connected_light_color_(std::string entity_id, std::string state);

  // Méthodes permettant de récupérer des périphériques à partir de leur identifiant unique de communication, et
  // inversement (et autres).
  sensor::Sensor *get_analog_sensor_from_communication_id_(int communication_id) const;
  binary_sensor::BinarySensor *get_binary_sensor_from_communication_id_(int communication_id) const;
  switch_::Switch *get_switch_from_communication_id_(int communication_id) const;
  alarm_control_panel::AlarmControlPanel *get_alarm_from_communication_id_(int communication_id) const;
  number::Number *get_missile_launcher_base_number_from_communication_id_(int communication_id) const;
  number::Number *get_missile_launcher_angle_number_from_communication_id_(int communication_id) const;
  button::Button *get_missile_launcher_launch_button_from_communication_id_(int communication_id) const;
  sensor::Sensor *get_missile_launcher_available_missiles_sensor_from_communication_id_(int communication_id) const;
  ConnectedBedroomTelevision *get_television_from_communication_id_(int communication_id) const;
  ConnectedBedroomRGBLEDStrip *get_RGB_LED_strip_from_communication_id(int communication_id) const;
  std::string get_connected_device_from_communication_id_(int communication_id) const;
  int get_communication_id_from_connected_light_entity_id_(std::string entity_id) const;
  ConnectedDeviceTypes get_type_from_connected_light_communication_id_(int communication_id) const;

  // Attribut de stockage du message en cours de réception de l'Arduino Mega.
  std::vector<uint8_t> receivedMessage_;

  bool synchronized_{false};

  // Attributs de stockage des périphériques utilisés dans la communication.
  std::vector<std::pair<int, sensor::Sensor *>> analog_sensors_;
  std::vector<std::pair<int, binary_sensor::BinarySensor *>> binary_sensors_;
  std::vector<std::pair<int, switch_::Switch *>> switches_;
  std::vector<std::tuple<int, alarm_control_panel::AlarmControlPanel *, number::Number *, number::Number *,
                         button::Button *, sensor::Sensor *>>
      alarms_;
  std::vector<std::pair<int, ConnectedBedroomTelevision *>> televisions_;
  std::vector<std::pair<int, ConnectedBedroomRGBLEDStrip *>> RGB_LED_strips_;
  std::vector<std::tuple<int, std::string, ConnectedDeviceTypes>> connected_lights_;

  friend class light::LightState;
};

/// @brief Classe abstraite représentant un périphérique du système de domotique.
class ConnectedBedroomDevice {
 public:
  void set_communication_id(int communication_id);
  void set_parent(ConnectedBedroom *parent);
  ConnectedBedroom *get_parent() const;

  virtual void register_device() = 0;

 protected:
  ConnectedBedroom *parent_;
  int communication_id_;
};

/// @brief Classe représentant un périphérique "basique" du système de domotique : apparaît comme une entité "switch".
class ConnectedBedroomSwitch : public Component, public switch_::Switch, public ConnectedBedroomDevice {
 public:
  void register_device() override;

 protected:
  void write_state(bool state) override;
};

/// @brief Classe représentant une alarme du système de domotique : apparaît comme une entité "alarm_control_panel".
class ConnectedBedroomAlarmControlPanel : public Component,
                                          public alarm_control_panel::AlarmControlPanel,
                                          public ConnectedBedroomDevice {
 public:
  void register_device() override;

  uint32_t get_supported_features() const override;
  bool get_requires_code() const override;
  bool get_requires_code_to_arm() const override;
  void add_code(const std::string &code);

 protected:
  virtual void control(const alarm_control_panel::AlarmControlPanelCall &call) override;

  std::vector<std::string> codes_;
};

/// @brief Classe représentant le contrôle de la base d'un lance-missile du système de domotique : apparaît comme une
/// entité "number".
class ConnectedBedroomMissileLauncherBaseNumber : public Component,
                                                  public number::Number,
                                                  public ConnectedBedroomDevice {
 public:
  void register_device() override;

 protected:
  void control(float value) override;
};

/// @brief Classe représentant le contrôle de l'angle d'un lance-missile du système de domotique : apparaît comme une
/// entité "number".
class ConnectedBedroomMissileLauncherAngleNumber : public Component,
                                                   public number::Number,
                                                   public ConnectedBedroomDevice {
 public:
  void register_device() override;

 protected:
  void control(float value) override;
};

/// @brief Classe représentant le bouton de tir d'un missile d'un lance-missile du système de domotique : apparaît comme
/// une entité "button".
class ConnectedBedroomMissileLauncherLaunchButton : public Component,
                                                    public button::Button,
                                                    public ConnectedBedroomDevice {
 public:
  void register_device() override;

 protected:
  void press_action() override;
};

/// @brief Classe abstraite commune aux classes utilisées pour représenter une télévision du système de domotique.
class TelevisionComponent {
 public:
  void set_parent(ConnectedBedroomTelevision *parent);

  virtual void register_component() = 0;

 protected:
  ConnectedBedroomTelevision *parent_;
};

/// @brief Classe représentant le contrôle de l'état (allumer / éteindre) d'une télévision du système de domotique :
/// apparaît comme une entité "switch".
class TelevisionState : public switch_::Switch, public TelevisionComponent {
 public:
  void register_component() override;

 protected:
  void write_state(bool state) override;
};

/// @brief Classe représentant le contrôle de l'état de la sourdinne (allumer / éteindre) d'une télévision du système de
/// domotique : apparaît comme une entité "switch".
class TelevisionMuted : public switch_::Switch, public TelevisionComponent {
 public:
  void register_component() override;

 protected:
  void write_state(bool state) override;
};

/// @brief Classe représentant le bouton d'augmentation du volume d'une télévision du système de domotique : apparaît
/// comme une entité "button".
class TelevisionVolumeUp : public button::Button, public TelevisionComponent {
 public:
  void register_component() override;

 protected:
  void press_action() override;
};

/// @brief Classe représentant le bouton de diminution du volume d'une télévision du système de domotique : apparaît
/// comme une entité "button".
class TelevisionVolumeDown : public button::Button, public TelevisionComponent {
 public:
  void register_component() override;

 protected:
  void press_action() override;
};

/// @brief Classe représentant une télévision du système de domotique. Celle-ci intègre les différentes classes de
/// contrôle de la télévision.
class ConnectedBedroomTelevision : public Component, public ConnectedBedroomDevice {
 public:
  void register_device() override;

  void setVolumeSensor(sensor::Sensor *sens);

  TelevisionState *state;
  TelevisionMuted *muted;
  TelevisionVolumeUp *volume_up;
  TelevisionVolumeDown *volume_down;
  sensor::Sensor *volume;

 protected:
  friend class TelevisionState;
  friend class TelevisionMuted;
  friend class TelevisionVolumeUp;
  friend class TelevisionVolumeDown;
};

/// @brief Classe représentant un ruban de DEL RVB du système de domotique : apparaît comme une entité "light".
class ConnectedBedroomRGBLEDStrip : public Component, public light::LightOutput, public ConnectedBedroomDevice {
 public:
  void register_device() override;

  light::LightTraits get_traits() override;

  void setup_state(light::LightState *state) override;
  void write_state(light::LightState *state) override;

  void block_next_write();

  light::LightState *state{nullptr};

 protected:
  bool block_next_write_{false};
  bool previous_state_{false};
};

/// @brief Classe représentant le mode arc-en-ciel d'un ruban de DEL RVB du système de domotique.
class ConnectedBedroomRGBLEDStripRainbowEffect : public light::LightEffect {
 public:
  explicit ConnectedBedroomRGBLEDStripRainbowEffect(const std::string &name);

  void apply() override;
};

/// @brief Classe représentant le mode son-réaction d'un ruban de DEL RVB du système de domotique.
class ConnectedBedroomRGBLEDStripSoundreactEffect : public light::LightEffect {
 public:
  explicit ConnectedBedroomRGBLEDStripSoundreactEffect(const std::string &name);

  void apply() override;
};

/// @brief Classe représentant le mode de l'alarme d'un ruban de DEL RVB du système de domotique.
class ConnectedBedroomRGBLEDStripAlarmEffect : public light::LightEffect {
 public:
  explicit ConnectedBedroomRGBLEDStripAlarmEffect(const std::string &name);

  void apply() override;
};

}  // namespace connected_bedroom
}  // namespace esphome