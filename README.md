# ESPHome PID Autotune Status

An ESPHome custom component that exposes the PID Climate autotune status, error messages, and phase progress as sensors in Home Assistant. All sensors automatically appear under the Diagnostic category with appropriate icons. It now also includes a dedicated Switch component to seamlessly start, configure, and safely abort the tuning process directly from your dashboard.

## Usage

You don't need to download any files manually. Just add the repository directly to your ESPHome configuration using the `external_components` block.

```yaml
external_components:
  - source: github://denpavibes/esphome-pid-autotune-status

climate:
  - platform: pid
    id: my_pid_climate
    # ... rest of your climate config

switch:
  - platform: pid_autotune
    name: "PID Autotune Switch"
    climate_id: my_pid_climate
    # Optional parameters (defaults shown)
    noiseband: 0.25
    positive_output: 1.0
    negative_output: -1.0
    update_interval: 5s

text_sensor:
  - platform: pid_autotune
    climate_id: my_pid_climate
    status:
      id: autotune_status
      name: "PID Autotune Status"
    error_message:
      id: autotune_error
      name: "PID Autotune Error"
    update_interval: 5s

sensor:
  - platform: pid_autotune
    climate_id: my_pid_climate
    phase:
      id: autotune_phase
      name: "PID Autotune Phase"
    kp:
      id: autotune_kp
      name: "PID Autotune Kp"
    ki:
      id: autotune_ki
      name: "PID Autotune Ki"
    kd:
      id: autotune_kd
      name: "PID Autotune Kd"
    update_interval: 5s
```

### Switch: Autotune Toggle
The switch allows you to control the autotune process interactively:
* **Turning On:** Creates a new autotuner configured with your provided parameters (`noiseband`, `positive_output`, `negative_output`) and starts the tuning process.
* **Turning Off:** Immediately aborts an active autotune by destroying the autotuner object in memory.
* **Auto-Sync:** The switch automatically toggles itself to the Off position when the autotune finishes or fails, keeping your Home Assistant dashboard perfectly synchronized with the hardware state.

#### Switch Configuration Variables:
* **climate_id** (*Optional*, ID): The ID of the PID climate controller to tune.
* **noiseband** (*Optional*, float): The amplitude of noise on the sensor. The autotuner will only trigger a relay state change when the measurement exceeds this value. Defaults to `0.25`.
* **positive_output** (*Optional*, float): The output value to apply for the heating phase during tuning. Defaults to `1.0`.
* **negative_output** (*Optional*, float): The output value to apply for the cooling phase during tuning. Defaults to `-1.0`.
* **update_interval** (*Optional*, Time): The interval to check the background process state to keep the switch UI in sync. Defaults to `5s`.

### Text Sensor: Status & Error Message
Both `status` and `error_message` text sensors are optional, but at least one must be configured.

#### Status (`status`)
The status sensor will report one of the following states to Home Assistant:
* **Off**: Autotune is not running.
* **Running**: Autotune is currently in progress.
* **Finished**: Autotune completed successfully (data was convergent and symmetrical).
* **Failed**: Autotune finished, but failed to reach amplitude convergence or symmetry.

#### Error Message (`error_message`)
Provides diagnostic error information if autotuning fails:
* **None**: No error detected (autotune is Off, Running, or completed successfully).
* **Amplitude not convergent**: Autotune finished, but could not reliably determine oscillation amplitude.
* **Frequency not symmetrical**: Autotune finished, but oscillation frequency was not symmetrical.

### Numeric Sensors: Phase Counter, Kp, Ki, Kd
All sensors under this platform (`phase`, `kp`, `ki`, `kd`) are optional, but at least one must be configured.

* **Phase (`phase`)**: Tracks the `phase_count` during the autotuning process.
  * Returns **0** when the autotuner is Off, Finished, or Failed.
  * Increments sequentially during the **Running** state as the relay function oscillates.
* **Kp, Ki, Kd (`kp`, `ki`, `kd`)**:
  * While the autotuner is **Off** or **Running**, these sensors report **0.0**.
  * When autotune completes, these values automatically update to the calculated **"No Overshoot PID"** rule parameters (`kp_factor = 0.2`, `ki_factor = 0.4`, `kd_factor = 0.0625`).

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
