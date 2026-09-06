#pragma once

#include <memory>
#include <string>
#include <utility>

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/pid/pid_climate.h"
#include "esphome/components/pid/pid_autotuner.h"

namespace esphome::pid_autotune {

static const char *const TAG = "pid_autotune";

namespace internal {

// --- 1. Access Bypass Trick for PIDClimate (which is marked 'final') ---
struct AutotunerTag {
  using type = std::unique_ptr<pid::PIDAutotuner> pid::PIDClimate::*;
};

template<typename Tag> struct MemberStorage {
  static typename Tag::type ptr;
};

template<typename Tag> typename Tag::type MemberStorage<Tag>::ptr;

template<typename Tag, typename Tag::type P> struct MemberExtractor {
  struct Filler {
    Filler() { MemberStorage<Tag>::ptr = P; }
  };
  static Filler filler;
};

template<typename Tag, typename Tag::type P> typename MemberExtractor<Tag, P>::Filler MemberExtractor<Tag, P>::filler;

template class MemberExtractor<AutotunerTag, &pid::PIDClimate::autotuner_>;

// --- 2. Access Bypass Trick for PIDAutotuner (not final) ---
class AutotunerInspector : public pid::PIDAutotuner {
 public:
  static bool is_amplitude_convergent(pid::PIDAutotuner *tuner) {
    auto amp_ptr = &AutotunerInspector::amplitude_detector_;
    auto &amp = tuner->*amp_ptr;
    return amp.is_amplitude_convergent();
  }

  static bool is_frequency_symmetrical(pid::PIDAutotuner *tuner) {
    auto freq_ptr = &AutotunerInspector::frequency_detector_;
    auto &freq = tuner->*freq_ptr;
    return freq.is_increase_decrease_symmetrical();
  }

  static bool is_successful(pid::PIDAutotuner *tuner) {
    return is_frequency_symmetrical(tuner) && is_amplitude_convergent(tuner);
  }

  static uint32_t get_phase_count(pid::PIDAutotuner *tuner) {
    auto relay_ptr = &AutotunerInspector::relay_function_;
    auto &relay = tuner->*relay_ptr;
    return relay.phase_count;
  }

  static pid::PIDAutotuner::PIDResult calculate_pid(pid::PIDAutotuner *tuner, float kp_factor, float ki_factor,
                                                    float kd_factor) {
    auto calc_ptr = &AutotunerInspector::calculate_pid_;
    return (tuner->*calc_ptr)(kp_factor, ki_factor, kd_factor);
  }
};

}  // namespace internal

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

  void dump_config() override {
    ESP_LOGCONFIG(TAG, "PID Autotune Text Sensors:");
    LOG_UPDATE_INTERVAL(this);
    if (this->status_sensor_ != nullptr) {
      LOG_TEXT_SENSOR("  ", "Status", this->status_sensor_);
    }
    if (this->error_message_sensor_ != nullptr) {
      LOG_TEXT_SENSOR("  ", "Error Message", this->error_message_sensor_);
    }
  }

  void update() override {
    if (this->climate_ == nullptr) {
      this->mark_failed();
      return;
    }

    std::string current_status = "Off";
    std::string current_error = "None";

    auto ptr_to_member = internal::MemberStorage<internal::AutotunerTag>::ptr;
    auto &autotuner_ptr = this->climate_->*ptr_to_member;
    pid::PIDAutotuner *autotuner = autotuner_ptr.get();

    if (autotuner != nullptr) {
      if (autotuner->is_finished()) {
        if (internal::AutotunerInspector::is_successful(autotuner)) {
          current_status = "Finished";
        } else {
          current_status = "Failed";
          if (!internal::AutotunerInspector::is_amplitude_convergent(autotuner)) {
            current_error = "Amplitude not convergent";
          } else if (!internal::AutotunerInspector::is_frequency_symmetrical(autotuner)) {
            current_error = "Frequency not symmetrical";
          }
        }
      } else {
        current_status = "Running";
      }
    }

    if (this->status_sensor_ != nullptr && current_status != this->last_status_) {
      this->status_sensor_->publish_state(current_status);
      this->last_status_ = current_status;
    }

    if (this->error_message_sensor_ != nullptr && current_error != this->last_error_) {
      this->error_message_sensor_->publish_state(current_error);
      this->last_error_ = current_error;
    }
  }
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

  void dump_config() override {
    ESP_LOGCONFIG(TAG, "PID Autotune Sensors:");
    LOG_UPDATE_INTERVAL(this);
    if (this->phase_sensor_ != nullptr) {
      LOG_SENSOR("  ", "Phase", this->phase_sensor_);
    }
    if (this->kp_sensor_ != nullptr) {
      LOG_SENSOR("  ", "Kp", this->kp_sensor_);
    }
    if (this->ki_sensor_ != nullptr) {
      LOG_SENSOR("  ", "Ki", this->ki_sensor_);
    }
    if (this->kd_sensor_ != nullptr) {
      LOG_SENSOR("  ", "Kd", this->kd_sensor_);
    }
  }

  void update() override {
    if (this->climate_ == nullptr) {
      this->mark_failed();
      return;
    }

    auto ptr_to_member = internal::MemberStorage<internal::AutotunerTag>::ptr;
    auto &autotuner_ptr = this->climate_->*ptr_to_member;
    pid::PIDAutotuner *autotuner = autotuner_ptr.get();

    if (this->phase_sensor_ != nullptr) {
      float current_phase = 0.0f;  // Default to 0 when off/finished
      if (autotuner != nullptr && !autotuner->is_finished()) {
        current_phase = (float) internal::AutotunerInspector::get_phase_count(autotuner);
      }
      if (!this->phase_sensor_->has_state() || this->phase_sensor_->get_state() != current_phase) {
        this->phase_sensor_->publish_state(current_phase);
      }
    }

    if (this->kp_sensor_ != nullptr || this->ki_sensor_ != nullptr || this->kd_sensor_ != nullptr) {
      float kp = 0.0f;
      float ki = 0.0f;
      float kd = 0.0f;

      if (autotuner != nullptr && autotuner->is_finished()) {
        auto pid_res = internal::AutotunerInspector::calculate_pid(autotuner, 0.2f, 0.4f, 0.0625f);
        kp = pid_res.kp;
        ki = pid_res.ki;
        kd = pid_res.kd;
      } else {
        kp = this->climate_->get_kp();
        ki = this->climate_->get_ki();
        kd = this->climate_->get_kd();
      }

      if (this->kp_sensor_ != nullptr && (!this->kp_sensor_->has_state() || this->kp_sensor_->get_state() != kp)) {
        this->kp_sensor_->publish_state(kp);
      }
      if (this->ki_sensor_ != nullptr && (!this->ki_sensor_->has_state() || this->ki_sensor_->get_state() != ki)) {
        this->ki_sensor_->publish_state(ki);
      }
      if (this->kd_sensor_ != nullptr && (!this->kd_sensor_->has_state() || this->kd_sensor_->get_state() != kd)) {
        this->kd_sensor_->publish_state(kd);
      }
    }
  }
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

  void dump_config() override {
    LOG_SWITCH("", "PID Autotune Switch", this);
    LOG_UPDATE_INTERVAL(this);
    ESP_LOGCONFIG(TAG, "  Noiseband: %.2f", this->noiseband_);
    ESP_LOGCONFIG(TAG, "  Positive Output: %.2f", this->positive_output_);
    ESP_LOGCONFIG(TAG, "  Negative Output: %.2f", this->negative_output_);
  }

  void write_state(bool state) override {
    if (this->climate_ == nullptr) {
      this->mark_failed();
      return;
    }

    if (state) {
      auto tuner = std::make_unique<pid::PIDAutotuner>();
      tuner->set_noiseband(this->noiseband_);
      tuner->set_output_positive(this->positive_output_);
      tuner->set_output_negative(this->negative_output_);
      this->climate_->start_autotune(std::move(tuner));
    } else {
      // Abort autotune by destroying the object in memory
      auto ptr_to_member = internal::MemberStorage<internal::AutotunerTag>::ptr;
      (this->climate_->*ptr_to_member).reset();
    }

    this->publish_state(state);
  }

  void update() override {
    if (this->climate_ == nullptr) {
      this->mark_failed();
      return;
    }

    auto ptr_to_member = internal::MemberStorage<internal::AutotunerTag>::ptr;
    auto &autotuner_ptr = this->climate_->*ptr_to_member;
    pid::PIDAutotuner *autotuner = autotuner_ptr.get();

    // The switch should be ON if the autotuner exists and hasn't finished
    bool is_running = (autotuner != nullptr && !autotuner->is_finished());

    if (this->state != is_running) {
      this->publish_state(is_running);
    }
  }
};

}  // namespace esphome::pid_autotune
