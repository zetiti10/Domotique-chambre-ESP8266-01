#ifdef USE_ARDUINO

#include "domoticSystem.h"
#include "esphome/core/log.h"

namespace esphome {
namespace DomoticSystem {

static const char *const TAG = "système de domotique";
static const int UART_WAITING_TIME = 10;

void DomoticSystemDevice::setID(int ID) { ID_ = ID; }

String DomoticSystemDevice::addZeros(int number, int length) {
  String result = String(number);
  while (result.length() < (unsigned int) length) {
    result = "0" + result;
  }
  return result;
}

int DomoticSystemDevice::getIntFromString(String &string, int position, int lenght) {
  int result = 0;

  for (int i = 0; i < lenght; i++)
    result += (string.charAt(position + i) - '0') * pow(10, ((lenght - i) - 1));

  return result;
}

void DomoticSystemSwitch::write_state(bool state) {
  this->write_byte(0);
  this->write_str(this->addZeros(ID_, 2).c_str());
  this->write_byte(0);
  this->write_byte(0);
  this->write_byte(state);
  this->write_byte('\n');

  // Pas de retour d'état ici : mise à jour dynamique.
}

void DomoticSystem::setLEDCube(DomoticSystemSwitch *LEDCube) { LEDCube_ = LEDCube; }

void DomoticSystem::setup() {
  unsigned long initialTime = millis();

  while (!this->available()) {
    if ((millis() - initialTime) >= 10000) {
      ESP_LOGE(TAG, "Impossible d'établir la communication avec l'Arduino Méga.");
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
    ESP_LOGW(TAG, "Message inconnu reçu : %s", receivedMessage);

  ESP_LOGD(TAG, "Initialisation terminée avec succès !");
}

void DomoticSystem::loop() {
  if (!this->available())
    return;

  String receivedMessage;
  while (this->available() > 0) {
    char letter = this->read();

    if (letter == '\n')
      break;

    receivedMessage += letter;
  }

  int ID = getIntFromString(receivedMessage, 1, 2);

  switch (getIntFromString(receivedMessage, 0, 1)) {
    case 1: {
      switch (getIntFromString(receivedMessage, 3, 2))
      {
        case 1:
        {
          if (ID == 2)
            LEDCube_->publish_state(getIntFromString(receivedMessage, 5, 1));
          break;
        }
      }
      break;
    }
  }
}

}  // namespace DomoticSystem
}  // namespace esphome

#endif  // USE_ARDUINO
