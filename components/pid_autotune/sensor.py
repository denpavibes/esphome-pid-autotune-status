import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, sensor
from esphome.components.const import CONF_CLIMATE_ID
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC, STATE_CLASS_MEASUREMENT
from esphome.types import ConfigType

DEPENDENCIES = ["climate"]

pid_ns = cg.esphome_ns.namespace("pid")
PIDClimate = pid_ns.class_("PIDClimate", climate.Climate)

pid_autotune_ns = cg.esphome_ns.namespace("pid_autotune")
PIDAutotuneSensor = pid_autotune_ns.class_(
    "PIDAutotuneSensor", sensor.Sensor, cg.PollingComponent
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        PIDAutotuneSensor,
        icon="mdi:counter",
        accuracy_decimals=0,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.GenerateID(CONF_CLIMATE_ID): cv.use_id(PIDClimate),
        }
    )
    .extend(cv.polling_component_schema("5s"))
)


async def to_code(config: ConfigType) -> None:
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    climate_ = await cg.get_variable(config[CONF_CLIMATE_ID])
    cg.add(var.set_climate(climate_))
