import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, text_sensor
from esphome.components.const import CONF_CLIMATE_ID
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC
from esphome.types import ConfigType

DEPENDENCIES = ["climate"]

pid_ns = cg.esphome_ns.namespace("pid")
PIDClimate = pid_ns.class_("PIDClimate", climate.Climate)

pid_autotune_ns = cg.esphome_ns.namespace("pid_autotune")
PIDAutotuneTextSensor = pid_autotune_ns.class_(
    "PIDAutotuneTextSensor", text_sensor.TextSensor, cg.PollingComponent
)

CONFIG_SCHEMA = (
    text_sensor.text_sensor_schema(
        PIDAutotuneTextSensor,
        icon="mdi:thermometer-auto",
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    )
    .extend(
        {
            cv.GenerateID(CONF_CLIMATE_ID): cv.use_id(PIDClimate),
        }
    )
    .extend(cv.polling_component_schema("5s"))
)


async def to_code(config: ConfigType) -> None:
    var = await text_sensor.new_text_sensor(config)
    await cg.register_component(var, config)

    climate_ = await cg.get_variable(config[CONF_CLIMATE_ID])
    cg.add(var.set_climate(climate_))
