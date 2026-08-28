import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, switch
from esphome.components.const import CONF_CLIMATE_ID
from esphome.const import ENTITY_CATEGORY_CONFIG
from esphome.types import ConfigType

DEPENDENCIES = ["climate"]

pid_ns = cg.esphome_ns.namespace("pid")
PIDClimate = pid_ns.class_("PIDClimate", climate.Climate)

pid_autotune_ns = cg.esphome_ns.namespace("pid_autotune")
PIDAutotuneSwitch = pid_autotune_ns.class_(
    "PIDAutotuneSwitch", switch.Switch, cg.PollingComponent
)

CONF_NOISEBAND = "noiseband"
CONF_POSITIVE_OUTPUT = "positive_output"
CONF_NEGATIVE_OUTPUT = "negative_output"

CONFIG_SCHEMA = (
    switch.switch_schema(
        PIDAutotuneSwitch,
        icon="mdi:tune",
        entity_category=ENTITY_CATEGORY_CONFIG,
    )
    .extend(
        {
            cv.GenerateID(CONF_CLIMATE_ID): cv.use_id(PIDClimate),
            cv.Optional(CONF_NOISEBAND, default=0.25): cv.float_,
            cv.Optional(CONF_POSITIVE_OUTPUT, default=1.0): cv.float_,
            cv.Optional(CONF_NEGATIVE_OUTPUT, default=-1.0): cv.float_,
        }
    )
    .extend(cv.polling_component_schema("5s"))
)


async def to_code(config: ConfigType) -> None:
    var = await switch.new_switch(config)
    await cg.register_component(var, config)

    climate_ = await cg.get_variable(config[CONF_CLIMATE_ID])
    cg.add(var.set_climate(climate_))
    cg.add(var.set_noiseband(config[CONF_NOISEBAND]))
    cg.add(var.set_positive_output(config[CONF_POSITIVE_OUTPUT]))
    cg.add(var.set_negative_output(config[CONF_NEGATIVE_OUTPUT]))
