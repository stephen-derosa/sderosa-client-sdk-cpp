# Overview
turn this into a git repo asap

## PTLiveKit
`PTLiveKit` is a LiveKit participant executable for a real pan/tilt robot. It
owns the pan/tilt servo controller (`PanTiltController`) and gyro
(`L3G4200D_IMU`), publishes robot state, and accepts remote velocity commands.

### DataTrack namespace
- Publishes:
  - `gyro.state`
  - `pan.state`
  - `tilt.state`
- Subscribes:
  - `control_cmd` (from the current controller identity only)

### Message formats
- `control_cmd` (JSON, subscriber input):
  - `{"pan_vel": <rad_per_sec>, "tilt_vel": <rad_per_sec>}`
  - Velocities are in `rad/s` and converted internally to servo `steps/s`.
- `gyro.state` (JSON, publisher output):
  - `{"gyro_x_dps": <double>, "gyro_y_dps": <double>, "gyro_z_dps": <double>, "angle_x_deg": <double>, "angle_y_deg": <double>, "angle_z_deg": <double>, "valid": <bool>}`
- `pan.state` / `tilt.state` (JSON, publisher output):
  - `{"servo": "pan|tilt", "motor_id": <int>, "position_ticks": <int>, "speed_steps_s": <int>, "load_pwm": <int>, "voltage_01v": <int>, "temperature_celsius": <int>, "moving": <int>, "current_milliamps": <int>, "valid": <bool>}`

### Acquiring control
Control is explicitly gated by RPC:
- Call RPC method `acquire_control` to claim control when no controller is set.
- If another controller is active, the request is rejected and returns the
  current controller identity.
- The active controller can unset/release control via `acquire_control`, which
  clears the `control_cmd` subscription.

# todo: bring in examples/servo_control

# BOM

# Setup and Tests
`setup/` has tools for configuring your servos, validating your IMU

## Motor Setup

### Motor Controller
`ls /dev/ttyUSB*`
`ls /dev/ttyACM*`


### Motors
- servo id
- zero position
- min/max angles
- current (milli-amps) limits

## IMU Setup
schematic
VCC
GND
...
...

### Find your i2c
where n is the bus that the device is on
- `sudo i2cdetect -y -r n`
- specify as imu constructor


