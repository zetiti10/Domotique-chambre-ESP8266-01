import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, binary_sensor, switch, alarm_control_panel, button
from esphome.const import CONF_ID, CONF_SWITCHES, CONF_ENTITY_ID

CODEOWNERS = ["@zetiti10"]

MULTI_CONF = True
DEPENDENCIES = ['uart']
AUTO_LOAD = ['sensor', 'binary_sensor', 'switch', 'alarm_control_panel', 'button']

connected_bedroom_ns = cg.esphome_ns.namespace('connected_bedroom')

ConnectedBedroom = connected_bedroom_ns.class_('ConnectedBedroom', cg.Component, uart.UARTDevice)
ConnectedBedroomDevice = connected_bedroom_ns.class_('ConnectedBedroomDevice')
ConnectedBedroomSwitch = connected_bedroom_ns.class_('ConnectedBedroomSwitch', switch.Switch, cg.Component, ConnectedBedroomDevice)
ConnectedBedroomAlarmControlPanel = connected_bedroom_ns.class_('ConnectedBedroomAlarmControlPanel', alarm_control_panel.AlarmControlPanel, cg.Component, ConnectedBedroomDevice)
TelevisionState = connected_bedroom_ns.class_('TelevisionState', switch.Switch)
TelevisionMuted = connected_bedroom_ns.class_('TelevisionMuted', switch.Switch)
TelevisionVolumeUp = connected_bedroom_ns.class_('TelevisionVolumeUp', button.Button)
TelevisionVolumeDown = connected_bedroom_ns.class_('TelevisionVolumeDown', button.Button)
ConnectedBedroomTelevision = connected_bedroom_ns.class_('TelevisionState', cg.Component, ConnectedBedroomDevice)
ConnectedLightTypes = connected_bedroom_ns.enum("ConnectedLightsType")

CONF_ANALOG_SENSORS = "analog_sensors"
CONF_BINARY_SENSORS = "binary_sensors"
CONF_ALARMS = "alarms"
CONF_CODES = "codes"
CONF_TELEVISIONS = "televisions"
CONF_STATE_SWITCH = "state_switch"
CONF_MUTE_SWITCH = "mute_switch"
CONF_VOLUME_UP_BUTTON = "volume_up_button"
CONF_VOLUME_DOWN_BUTTON = "volume_down_button"
CONF_VOLUME_STATE = "volume_state"
CONF_CONNECTED_LIGHTS = "connected_lights"
CONF_CONNECTED_LIGHT_TYPE = "type"
CONF_COMMUNICATION_ID = "communication_id"

ENUM_CONNECTED_LIGHT_TYPES = {
    "BINARY_CONNECTED_LIGHT": ConnectedLightTypes.BINARY_CONNECTED_LIGHT,
    "TEMPERATURE_VARIABLE_CONNECTED_LIGHT": ConnectedLightTypes.TEMPERATURE_VARIABLE_CONNECTED_LIGHT,
    "COLOR_VARIABLE_CONNECTED_LIGHT": ConnectedLightTypes.COLOR_VARIABLE_CONNECTED_LIGHT,
}


CONFIG_SCHEMA = uart.UART_DEVICE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ConnectedBedroom),
        cv.Optional(CONF_ANALOG_SENSORS): cv.ensure_list(
            sensor.SENSOR_SCHEMA.extend(
                {
                    cv.Required(CONF_COMMUNICATION_ID): cv.positive_int,
                }
            )
        ),
        cv.Optional(CONF_BINARY_SENSORS): cv.ensure_list(
            binary_sensor.BINARY_SENSOR_SCHEMA.extend(
                {
                    cv.Required(CONF_COMMUNICATION_ID): cv.positive_int,
                }
            )
        ),
        cv.Optional(CONF_SWITCHES): cv.ensure_list(
            switch.SWITCH_SCHEMA.extend(
                {
                    cv.GenerateID(): cv.declare_id(ConnectedBedroomSwitch),
                    cv.Required(CONF_COMMUNICATION_ID): cv.positive_int,
                }
            )
        ),
        cv.Optional(CONF_ALARMS): cv.ensure_list(
            alarm_control_panel.ALARM_CONTROL_PANEL_SCHEMA.extend(
                {
                    cv.GenerateID(): cv.declare_id(ConnectedBedroomAlarmControlPanel),
                    cv.Optional(CONF_CODES): cv.ensure_list(cv.string_strict),
                    cv.Required(CONF_COMMUNICATION_ID): cv.positive_int,
                }
            )
        ),
        cv.Optional(CONF_TELEVISIONS): cv.ensure_list(
            {
                cv.GenerateID(): cv.declare_id(ConnectedBedroomTelevision),
                cv.Required(CONF_COMMUNICATION_ID): cv.positive_int,
                cv.Required(CONF_STATE_SWITCH): switch.SWITCH_SCHEMA.extend(
                    {
                        cv.GenerateID(): cv.declare_id(TelevisionState),
                    }),
                cv.Required(CONF_MUTE_SWITCH): switch.SWITCH_SCHEMA.extend(
                    {
                        cv.GenerateID(): cv.declare_id(TelevisionMuted),
                    }),
                cv.Required(CONF_VOLUME_UP_BUTTON) : button.BUTTON_SCHEMA.extend(
                    {
                        cv.GenerateID(): cv.declare_id(TelevisionVolumeUp),
                    }),
                cv.Required(CONF_VOLUME_DOWN_BUTTON) : button.BUTTON_SCHEMA.extend(
                    {
                        cv.GenerateID(): cv.declare_id(TelevisionVolumeDown),
                    }),
                cv.Required(CONF_VOLUME_STATE) : sensor.SENSOR_SCHEMA,
            }
        ),
        cv.Optional(CONF_CONNECTED_LIGHTS): cv.ensure_list(
            {
                cv.Required(CONF_COMMUNICATION_ID): cv.positive_int,
                cv.Required(CONF_ENTITY_ID): cv.string,
                cv.Required(CONF_CONNECTED_LIGHT_TYPE): cv.enum(
                    ENUM_CONNECTED_LIGHT_TYPES, upper=True
                )
            }
        )
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

    for conf in config[CONF_ALARMS]:
        alarm_var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(alarm_var, conf)
        await alarm_control_panel.register_alarm_control_panel(alarm_var, conf)
        communication_id = conf[CONF_COMMUNICATION_ID]
        cg.add(alarm_var.set_communication_id(communication_id))
        cg.add(alarm_var.set_parent(var))
        if CONF_CODES in conf:
            for acode in conf[CONF_CODES]:
                cg.add(alarm_var.add_code(acode))

    for conf in config[CONF_TELEVISIONS]:
        television_var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(television_var, conf)
        communication_id = conf[CONF_COMMUNICATION_ID]
        cg.add(television_var.set_communication_id(communication_id))
        cg.add(television_var.set_parent(var))
        state_switch = await switch.new_switch(conf[CONF_STATE_SWITCH])
        cg.add(state_switch.set_parent(television_var))
        mute_switch = await switch.new_switch(conf[CONF_MUTE_SWITCH])
        cg.add(mute_switch.set_parent(television_var))
        volume_up_button = await switch.new_switch(conf[CONF_VOLUME_UP_BUTTON])
        cg.add(volume_up_button.set_parent(television_var))
        volume_down_button = await switch.new_switch(conf[CONF_VOLUME_DOWN_BUTTON])
        cg.add(volume_down_button.set_parent(television_var))
        await sensor.new_sensor(conf[CONF_VOLUME_STATE])

    for conf in config[CONF_CONNECTED_LIGHTS]:
        cg.add(var.add_connected_light(conf[CONF_COMMUNICATION_ID], conf[CONF_ENTITY_ID], conf[CONF_CONNECTED_LIGHT_TYPE]))
