import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, text_sensor
from esphome.components.const import CONF_CLIMATE_ID
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_UPDATE_INTERVAL,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from esphome.types import ConfigType

DEPENDENCIES = ["climate"]

CONF_STATUS = "status"
CONF_ERROR_MESSAGE = "error_message"

pid_ns = cg.esphome_ns.namespace("pid")
PIDClimate = pid_ns.class_("PIDClimate", climate.Climate)

pid_autotune_ns = cg.esphome_ns.namespace("pid_autotune")
PIDAutotuneTextSensorComponent = pid_autotune_ns.class_(
    "PIDAutotuneTextSensorComponent", cg.PollingComponent
)


def _validate_legacy(config):
    if not isinstance(config, dict):
        return config
    if (
        CONF_STATUS not in config
        and CONF_ERROR_MESSAGE not in config
        and CONF_NAME in config
    ):
        config = config.copy()
        status_conf = {}
        platform_keys = {CONF_CLIMATE_ID, CONF_UPDATE_INTERVAL}
        for key in list(config.keys()):
            if key not in platform_keys:
                status_conf[key] = config.pop(key)
        config[CONF_STATUS] = status_conf
    return config


CONFIG_SCHEMA = cv.All(
    _validate_legacy,
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PIDAutotuneTextSensorComponent),
            cv.GenerateID(CONF_CLIMATE_ID): cv.use_id(PIDClimate),
            cv.Optional(CONF_STATUS): text_sensor.text_sensor_schema(
                icon="mdi:thermometer-auto",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_ERROR_MESSAGE): text_sensor.text_sensor_schema(
                icon="mdi:alert-circle-outline",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    ).extend(cv.polling_component_schema("5s")),
    cv.has_at_least_one_key(CONF_STATUS, CONF_ERROR_MESSAGE),
)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    climate_ = await cg.get_variable(config[CONF_CLIMATE_ID])
    cg.add(var.set_climate(climate_))

    if status_config := config.get(CONF_STATUS):
        sens = await text_sensor.new_text_sensor(status_config)
        cg.add(var.set_status_sensor(sens))

    if error_config := config.get(CONF_ERROR_MESSAGE):
        sens = await text_sensor.new_text_sensor(error_config)
        cg.add(var.set_error_message_sensor(sens))
