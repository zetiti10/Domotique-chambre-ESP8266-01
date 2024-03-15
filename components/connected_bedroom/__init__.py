import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import uart, sensor, binary_sensor, switch
from esphome.const import CONF_ID, CONF_SWITCHES

CODEOWNERS = ["@zetiti10"]

MULTI_CONF = True
DEPENDENCIES = ['uart']
AUTO_LOAD = ['sensor', 'binary_sensor', 'switch']

serial_ns = cg.esphome_ns.namespace('connected_bedroom')

ConnectedBedroom = serial_ns.class_('ConnectedBedroom', cg.Component, uart.UARTDevice)
ConnectedBedroomSwitch = serial_ns.class_('ConnectedBedroomSwitch', switch.Switch, cg.Component)

CONF_ANALOG_SENSORS = "analog_sensors"
CONF_BINARY_SENSORS = "binary_sensors"
CONF_COMMUNICATION_ID = "communication_id"


CONFIG_SCHEMA = uart.UART_DEVICE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ConnectedBedroom),
        cv.Required(CONF_ANALOG_SENSORS): cv.ensure_list(
            sensor.SENSOR_SCHEMA.extend(
                {
                    cv.Required(CONF_COMMUNICATION_ID): cv.positive_int,
                }
            )
        ),
        cv.Required(CONF_BINARY_SENSORS): cv.ensure_list(
            binary_sensor.BINARY_SENSOR_SCHEMA.extend(
                {
                    cv.Required(CONF_COMMUNICATION_ID): cv.positive_int,
                }
            )
        ),
        cv.Required(CONF_SWITCHES): cv.ensure_list(
            switch.SWITCH_SCHEMA.extend(
                {
                    cv.Required(CONF_COMMUNICATION_ID): cv.positive_int,
                }
            )
        ),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    for conf in config[CONF_ANALOG_SENSORS]:
        analog_sensor = await sensor.new_sensor(conf)
        communication_id = conf[CONF_COMMUNICATION_ID]
        cg.add(var.add_analog_sensor(communication_id, analog_sensor))

    for conf in config[CONF_BINARY_SENSORS]:
        binary_sensor_ = await binary_sensor.new_binary_sensor(conf)
        communication_id = conf[CONF_COMMUNICATION_ID]
        cg.add(var.add_binay_sensor(communication_id, binary_sensor_))

    for conf in config[CONF_SWITCHES]:
        switch_ = await switch.new_switch(conf)
        communication_id = conf[CONF_COMMUNICATION_ID]
        cg.add(switch_.set_communication_id(communication_id))
        cg.add(switch_.set_parent(var))
