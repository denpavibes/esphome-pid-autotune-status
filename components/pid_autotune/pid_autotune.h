#pragma once

#include <string>

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/pid/pid_climate.h"

namespace esphome::pid_autotune {

// --- Text Sensor: Status & Error Message ---
class PIDAutotuneTextSensorComponent : public PollingComponent {
 protected:
  pid::PIDClimate *climate_{nullptr};
  text_sensor::TextSensor *status_sensor_{nullptr};
  text_sensor::TextSensor *error_message_sensor_{nullptr};
  std::string last_status_{""};
  std::string last_error_{""};

 public:
  void set_climate(pid::PIDClimate *climate) { climate_ = climate; }
  void set_status_sensor(text_sensor::TextSensor *status_sensor) { status_sensor_ = status_sensor; }
  void set_error_message_sensor(text_sensor::TextSensor *error_message_sensor) {
    error_message_sensor_ = error_message_sensor;
  }

  void dump_config() override;
  void update() override;
};

using PIDAutotuneTextSensor = PIDAutotuneTextSensorComponent;

// --- Numeric Sensors: Phase Counter, Kp, Ki, Kd ---
class PIDAutotuneSensorComponent : public PollingComponent {
 protected:
  pid::PIDClimate *climate_{nullptr};
  sensor::Sensor *phase_sensor_{nullptr};
  sensor::Sensor *kp_sensor_{nullptr};
  sensor::Sensor *ki_sensor_{nullptr};
  sensor::Sensor *kd_sensor_{nullptr};

 public:
  void set_climate(pid::PIDClimate *climate) { climate_ = climate; }
  void set_phase_sensor(sensor::Sensor *phase_sensor) { phase_sensor_ = phase_sensor; }
  void set_kp_sensor(sensor::Sensor *kp_sensor) { kp_sensor_ = kp_sensor; }
  void set_ki_sensor(sensor::Sensor *ki_sensor) { ki_sensor_ = ki_sensor; }
  void set_kd_sensor(sensor::Sensor *kd_sensor) { kd_sensor_ = kd_sensor; }

  void dump_config() override;
  void update() override;
};

using PIDAutotuneSensor = PIDAutotuneSensorComponent;

// --- Switch: Autotune Toggle ---
class PIDAutotuneSwitch : public switch_::Switch, public PollingComponent {
 protected:
  pid::PIDClimate *climate_{nullptr};
  float noiseband_{0.25f};
  float positive_output_{1.0f};
  float negative_output_{-1.0f};

 public:
  void set_climate(pid::PIDClimate *climate) { climate_ = climate; }
  void set_noiseband(float noiseband) { noiseband_ = noiseband; }
  void set_positive_output(float positive_output) { positive_output_ = positive_output; }
  void set_negative_output(float negative_output) { negative_output_ = negative_output; }

  void dump_config() override;
  void write_state(bool state) override;
  void update() override;
};

}  // namespace esphome::pid_autotune
