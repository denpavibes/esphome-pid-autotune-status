import logging

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, sensor
from esphome.components.const import CONF_CLIMATE_ID
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_UPDATE_INTERVAL,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
)
from esphome.types import ConfigType

_LOGGER = logging.getLogger(__name__)

DEPENDENCIES = ["climate"]

CONF_PHASE = "phase"
CONF_KP = "kp"
CONF_KI = "ki"
CONF_KD = "kd"
CONF_RULES = "rules"

pid_ns = cg.esphome_ns.namespace("pid")
PIDClimate = pid_ns.class_("PIDClimate", climate.Climate)

pid_autotune_ns = cg.esphome_ns.namespace("pid_autotune")
PIDAutotuneSensorComponent = pid_autotune_ns.class_(
    "PIDAutotuneSensorComponent", cg.PollingComponent
)
PIDRule = pid_autotune_ns.enum("PIDRule")

PID_RULES = {
    "ZIEGLER_NICHOLS_PID": PIDRule.ZIEGLER_NICHOLS_PID,
    "ZIEGLER_NICHOLS_PI": PIDRule.ZIEGLER_NICHOLS_PI,
    "PESSEN_INTEGRAL_PID": PIDRule.PESSEN_INTEGRAL_PID,
    "SOME_OVERSHOOT_PID": PIDRule.SOME_OVERSHOOT_PID,
    "NO_OVERSHOOT_PID": PIDRule.NO_OVERSHOOT_PID,
}


def _validate_legacy(config):
    if not isinstance(config, dict):
        return config
    if (
        CONF_PHASE not in config
        and CONF_KP not in config
        and CONF_KI not in config
        and CONF_KD not in config
        and CONF_NAME in config
    ):
        config = config.copy()
        phase_conf = {}
        platform_keys = {CONF_CLIMATE_ID, CONF_UPDATE_INTERVAL}
        for key in list(config.keys()):
            if key not in platform_keys:
                phase_conf[key] = config.pop(key)
        config[CONF_PHASE] = phase_conf
    return config


def _validate_rules(config):
    has_pid_sensors = any(k in config for k in (CONF_KP, CONF_KI, CONF_KD))
    if CONF_RULES in config and not has_pid_sensors:
        _LOGGER.warning(
            "'rules' is configured but none of 'kp', 'ki', or 'kd' are present — "
            "'rules' will have no effect."
        )
    return config


CONFIG_SCHEMA = cv.All(
    _validate_legacy,
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PIDAutotuneSensorComponent),
            cv.GenerateID(CONF_CLIMATE_ID): cv.use_id(PIDClimate),
            cv.Optional(CONF_RULES, default="ZIEGLER_NICHOLS_PID"): cv.enum(
                PID_RULES, upper=True
            ),
            cv.Optional(CONF_PHASE): sensor.sensor_schema(
                icon="mdi:counter",
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_KP): sensor.sensor_schema(
                icon="mdi:chart-line",
                accuracy_decimals=5,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_KI): sensor.sensor_schema(
                icon="mdi:chart-line",
                accuracy_decimals=5,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_KD): sensor.sensor_schema(
                icon="mdi:chart-line",
                accuracy_decimals=5,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
        }
    ).extend(cv.polling_component_schema("5s")),
    cv.has_at_least_one_key(CONF_PHASE, CONF_KP, CONF_KI, CONF_KD),
    _validate_rules,
)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    climate_ = await cg.get_variable(config[CONF_CLIMATE_ID])
    cg.add(var.set_climate(climate_))

    cg.add(var.set_rules(config[CONF_RULES]))

    if phase_config := config.get(CONF_PHASE):
        sens = await sensor.new_sensor(phase_config)
        cg.add(var.set_phase_sensor(sens))

    if kp_config := config.get(CONF_KP):
        sens = await sensor.new_sensor(kp_config)
        cg.add(var.set_kp_sensor(sens))

    if ki_config := config.get(CONF_KI):
        sens = await sensor.new_sensor(ki_config)
        cg.add(var.set_ki_sensor(sens))

    if kd_config := config.get(CONF_KD):
        sens = await sensor.new_sensor(kd_config)
        cg.add(var.set_kd_sensor(sens))
