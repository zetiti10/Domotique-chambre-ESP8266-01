import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import uart
from esphome.components import sensor
from esphome.const import CONF_ID, CONF_SENSORS

CODEOWNERS = ["@zetiti10"]

DEPENDENCIES = ['uart']

CONF_COMMUNICATION_ID = "communication_id"

serial_ns = cg.esphome_ns.namespace('serial')

ConnectedBedroom = serial_ns.class_('ConnectedBedroom', cg.Component, sensor.Sensor, uart.UARTDevice)


CONFIG_SCHEMA = uart.UART_DEVICE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ConnectedBedroom),
        cv.Required(CONF_SENSORS): cv.ensure_list(
            sensor.SENSOR_SCHEMA.extend(
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
    for conf in config[CONF_SENSORS]:
        sens = await sensor.new_sensor(conf)
        communication_id = conf[CONF_COMMUNICATION_ID]
        cg.add(var.add_sensor(communication_id, sens))