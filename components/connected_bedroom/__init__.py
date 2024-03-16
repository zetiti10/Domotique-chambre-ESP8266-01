import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import uart, sensor, binary_sensor, switch, light
from esphome.const import CONF_ID, CONF_SWITCHES, CONF_OUTPUT_ID

CODEOWNERS = ["@zetiti10"]

MULTI_CONF = True
DEPENDENCIES = ['uart']
AUTO_LOAD = ['sensor', 'binary_sensor', 'switch', 'light']

connected_bedroom_ns = cg.esphome_ns.namespace('connected_bedroom')

ConnectedBedroom = connected_bedroom_ns.class_('ConnectedBedroom', cg.Component, uart.UARTDevice)
ConnectedBedroomDevice = connected_bedroom_ns.class_('ConnectedBedroomDevice')
ConnectedBedroomSwitch = connected_bedroom_ns.class_('ConnectedBedroomSwitch', switch.Switch, cg.Component, ConnectedBedroomDevice)
ConnectedBedroomRGBLEDStrip = connected_bedroom_ns.class_('ConnectedBedroomRGBLEDStrip', light.LightOutput, cg.Component, ConnectedBedroomDevice)

CONF_ANALOG_SENSORS = "analog_sensors"
CONF_BINARY_SENSORS = "binary_sensors"
CONF_RGB_LED_STRIPS = "RGB_LED_strips"
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
                    cv.GenerateID(): cv.declare_id(ConnectedBedroomSwitch),
                    cv.Required(CONF_COMMUNICATION_ID): cv.positive_int,
                }
            )
        ),
        cv.Required(CONF_RGB_LED_STRIPS): cv.ensure_list(
            light.RGB_LIGHT_SCHEMA.extend(
                {
                    cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(ConnectedBedroomRGBLEDStrip),
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
        cg.add(var.add_binary_sensor(communication_id, binary_sensor_))

    for conf in config[CONF_SWITCHES]:
        switch_ = await switch.new_switch(conf)
        communication_id = conf[CONF_COMMUNICATION_ID]
        cg.add(switch_.set_communication_id(communication_id))
        cg.add(switch_.set_parent(var))
        
    for conf in config[CONF_RGB_LED_STRIPS]:
        var2 = cg.new_Pvariable(conf[CONF_OUTPUT_ID])
        await light.register_light(var2, conf)
        communication_id = conf[CONF_COMMUNICATION_ID]
        cg.add(var2.set_communication_id(communication_id))
        cg.add(var2.set_parent(var))
        cg.add(var2.set_light_state_object(conf[CONF_ID]))
