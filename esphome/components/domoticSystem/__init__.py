# File witten with help from ChatGPT...

import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import uart, switch
from esphome.const import CONF_ID

CONF_LED_CUBE = 'led_cube'
CONF_DEVICE_ID = 'device_id'

led_cube_schema = cv.Schema({
    cv.GenerateID(): cv.declare_id(switch.Switch),
    cv.Required(CONF_DEVICE_ID): cv.int_,
}).extend(cv.COMPONENT_SCHEMA)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_LED_CUBE): led_cube_schema,
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    sw = await switch.new_switch(config[CONF_LED_CUBE])
    cg.add(sw.setID(config[CONF_LED_CUBE][CONF_DEVICE_ID]))
    cg.add(var.setLEDCube(sw))