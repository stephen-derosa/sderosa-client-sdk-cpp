/*
 * Copyright 2026 LiveKit
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <array>
#include <cstdlib>
#include <string>
#include <vector>
#include <unistd.h>

#include "SCServo.h"
#include "pan_tilt_controller.h"

namespace {
constexpr useconds_t kHalfSecondUs = 500000;
constexpr double kHalfPi = 1.57079632679489661923;

bool isValidServoId(const int id) {
  return id >= 0 && id <= 253;
}

std::vector<u8> scanServoIds(SMS_STS &sm_st) {
  std::vector<u8> found_ids;
  for (int id = 0; id <= 253; ++id) {
    const int ping_id = sm_st.Ping(static_cast<u8>(id));
    if (ping_id == id) {
      found_ids.push_back(static_cast<u8>(id));
      LK_LOG_INFO("[custom_calibration] Found servo ID {}", id);
    }
  }
  return found_ids;
}
} // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    LK_LOG_ERROR("Usage: ./custom_calibration <serial-port> [motor-id-1] [motor-id-2] [--scan-only] [--set-zero]");
    LK_LOG_ERROR("Example: ./custom_calibration /dev/ttyUSB0 1 2");
    LK_LOG_ERROR("Example: ./custom_calibration /dev/ttyUSB0 --scan-only");
    LK_LOG_ERROR("Example: ./custom_calibration /dev/ttyUSB0 1 2 --set-zero");
    return 1;
  }

  const char *serial_port = argv[1];
  int motor_id_1 = 1;
  int motor_id_2 = 2;
  bool motor_1_provided = false;
  bool motor_2_provided = false;
  bool scan_only = false;
  bool set_zero = false;

  for (int i = 2; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--scan-only") {
      scan_only = true;
      continue;
    }
    if (arg == "--set-zero") {
      set_zero = true;
      continue;
    }
    if (!motor_1_provided) {
      motor_id_1 = std::atoi(argv[i]);
      motor_1_provided = true;
      continue;
    }
    if (!motor_2_provided) {
      motor_id_2 = std::atoi(argv[i]);
      motor_2_provided = true;
      continue;
    }
    LK_LOG_ERROR("Unexpected argument: {}", arg);
    return 1;
  }

  if (motor_1_provided && !isValidServoId(motor_id_1)) {
    LK_LOG_ERROR("Invalid motor-id-1. Valid range is 0-253.");
    return 1;
  }
  if (motor_2_provided && !isValidServoId(motor_id_2)) {
    LK_LOG_ERROR("Invalid motor-id-2. Valid range is 0-253.");
    return 1;
  }
  if (motor_1_provided && motor_2_provided && motor_id_1 == motor_id_2) {
    LK_LOG_ERROR("Motor IDs must be unique.");
    return 1;
  }
  if (motor_1_provided != motor_2_provided) {
    LK_LOG_ERROR("Provide both motor IDs, or omit both to auto-detect.");
    return 1;
  }

  if (scan_only || set_zero || !motor_1_provided) {
    SMS_STS scanner;
    LK_LOG_INFO("[custom_calibration] Opening {} @ {} baud", serial_port,
                PanTiltController::kDefaultBaud);
    if (!scanner.begin(PanTiltController::kDefaultBaud, serial_port)) {
      LK_LOG_ERROR("[custom_calibration] Failed to init SMS/STS bus.");
      return 1;
    }

    LK_LOG_INFO("[custom_calibration] Scanning IDs 0-253...");
    const std::vector<u8> found_ids = scanServoIds(scanner);
    scanner.end();

    if (found_ids.empty()) {
      LK_LOG_ERROR("[custom_calibration] No servos found at baud {}.",
                   PanTiltController::kDefaultBaud);
      return 1;
    }
    if (scan_only) {
      return 0;
    }
    if (found_ids.size() < 2) {
      LK_LOG_ERROR("[custom_calibration] Found only one servo. Connect two servos or pass IDs manually.");
      return 1;
    }

    motor_id_1 = static_cast<int>(found_ids[0]);
    motor_id_2 = static_cast<int>(found_ids[1]);
    LK_LOG_INFO("[custom_calibration] Auto-selected IDs: {} and {}", motor_id_1,
                motor_id_2);
  }

  std::array<u8, PanTiltController::kMotorCount> ids = {
      static_cast<u8>(motor_id_1), static_cast<u8>(motor_id_2)};
  PanTiltController controller(serial_port, ids);
  if (!controller.initialize(set_zero)) {
    return 1;
  }
  usleep(kHalfSecondUs);
  if (set_zero)
  {
    LK_LOG_INFO("[custom_calibration] set zero position");
    return 0;
  }

  LK_LOG_INFO("[custom_calibration] Running sequence on motor index 1 (ID {})",
              motor_id_1);
  if (!controller.setMotorAngleRelative(0, -kHalfPi)) {
    return 1;
  }
  usleep(kHalfSecondUs);
  if (!controller.setMotorAngleRelative(0, kHalfPi)) {
    return 1;
  }
  usleep(kHalfSecondUs * 4);

  LK_LOG_INFO("[custom_calibration] Running sequence on motor index 2 (ID {})",
              motor_id_2);
  if (!controller.setMotorAngleRelative(1, -kHalfPi)) {
    return 1;
  }
  usleep(kHalfSecondUs);
  if (!controller.setMotorAngleRelative(1, kHalfPi)) {
    return 1;
  }
  usleep(kHalfSecondUs);

  LK_LOG_INFO("[custom_calibration] SECOND SEQUENCE\n*********\n");

  controller.homeMotors();

  auto home_states = controller.pollState();
  while(home_states[0].moving != 0 || home_states[1].moving != 0) {
    home_states = controller.pollState();
    LK_LOG_INFO("[custom_calibration] Sleeping until home is complete: {} {}", home_states[0].moving, home_states[1].moving);
    usleep(10 * 1000);
  }

  LK_LOG_WARN("[custom_calibration] homing is complete!");

  LK_LOG_INFO("[custom_calibration] Calling blocking absolute move for motor index 1 (ID {}), position={}",
              motor_id_1, home_states[0].position_ticks);
  if (!controller.setMotorAngleBlocking(0, -kHalfPi)) {
    return 1;
  }
  LK_LOG_INFO("[custom_calibration] Calling blocking absolute move for motor index 1 (ID {}), position={}",
              motor_id_1, home_states[0].position_ticks);
  if (!controller.setMotorAngleBlocking(0, kHalfPi)) {
    return 1;
  }
  LK_LOG_INFO("[custom_calibration] Calling blocking absolute move for motor index 2 (ID {}), position={}",
              motor_id_2, home_states[1].position_ticks);
  if (!controller.setMotorAngleBlocking(1, kHalfPi)) {
    return 1;
  }
  LK_LOG_INFO("[custom_calibration] Calling blocking absolute move for motor index 2 (ID {}), position={}",
              motor_id_2, home_states[1].position_ticks);
  if (!controller.setMotorAngleBlocking(1, -kHalfPi)) {
    return 1;
  }
  LK_LOG_INFO("[custom_calibration] complete");

  LK_LOG_INFO("[custom_calibration] THIRD SEQUENCE\n*********\n");
  controller.homeMotors();

  // Run the same as sequence 2 but using a speed of 3400

  // Re-home for clean start
  controller.homeMotors();

  auto home_states_speed = controller.pollState();
  while (home_states_speed[0].moving != 0 || home_states_speed[1].moving != 0) {
    home_states_speed = controller.pollState();
    LK_LOG_INFO("[custom_calibration] (speed=3400) Sleeping until home is complete: {} {}", home_states_speed[0].moving, home_states_speed[1].moving);
    usleep(10 * 1000);
  }

  LK_LOG_WARN("[custom_calibration] (speed=3400) homing is complete!");

  // Set speed to 3400 for both motors
  // controller.sms_sts_.WriteSpeed(static_cast<u8>(motor_id_1), 3400, controller.kDefaultMoveAcc);
  // controller.sms_sts_.WriteSpeed(static_cast<u8>(motor_id_2), 3400, controller.kDefaultMoveAcc);

  const int speed = 3400;

  LK_LOG_INFO("[custom_calibration] (speed=3400) Calling blocking absolute move for motor index 1 (ID {}), position={}",
              motor_id_1, home_states_speed[0].position_ticks);
  if (!controller.setMotorAngleBlocking(0, -kHalfPi, speed)) {
    return 1;
  }
  LK_LOG_INFO("[custom_calibration] (speed=3400) Calling blocking absolute move for motor index 1 (ID {}), position={}",
              motor_id_1, home_states_speed[0].position_ticks);
  if (!controller.setMotorAngleBlocking(0, kHalfPi, speed)) {
    return 1;
  }
  LK_LOG_INFO("[custom_calibration] (speed=3400) Calling blocking absolute move for motor index 2 (ID {}), position={}",
              motor_id_2, home_states_speed[1].position_ticks);
  if (!controller.setMotorAngleBlocking(1, kHalfPi, speed)) {
    return 1;
  }
  LK_LOG_INFO("[custom_calibration] (speed=3400) Calling blocking absolute move for motor index 2 (ID {}), position={}",
              motor_id_2, home_states_speed[1].position_ticks);
  if (!controller.setMotorAngleBlocking(1, -kHalfPi, speed)) {
    return 1;
  }
  LK_LOG_INFO("[custom_calibration] (speed=3400) sequence complete");

  LK_LOG_INFO("[custom_calibration] FOURTH SEQUENCE\n*********\n");

  LK_LOG_INFO("[custom_calibration] Pan motor velocity +1700 for 3 seconds");
  if (!controller.setVelocity(0, 1700)) {
    return 1;
  }
  using namespace std::chrono;
  auto print_start_time = steady_clock::now();
  while (duration_cast<seconds>(steady_clock::now() - print_start_time).count() < 5) {
    auto state = controller.pollState()[0];
    LK_LOG_INFO("[custom_calibration] [PAN] ticks={}, current(milliamps)={}, speed={}, time={:.2f}s",
                state.position_ticks,
                state.current_milliamps,
                state.speed,
                duration_cast<milliseconds>(steady_clock::now() - print_start_time).count() / 1000.0
    );
    usleep(100 * 1000);
  }

  LK_LOG_INFO("[custom_calibration] Reversing pan motor velocity to -1700");
  if (!controller.setVelocity(0, -1700)) {
    return 1;
  }
  print_start_time = steady_clock::now();
  while (duration_cast<seconds>(steady_clock::now() - print_start_time).count() < 5) {
    auto state = controller.pollState()[0];
    LK_LOG_INFO("[custom_calibration] [PAN] ticks={}, current(milliamps)={}, speed={}, time={:.2f}s",
                state.position_ticks,
                state.current_milliamps,
                state.speed,
                duration_cast<milliseconds>(steady_clock::now() - print_start_time).count() / 1000.0
    );
    usleep(100 * 1000);
  }
  const auto states = controller.pollState();
  for (const auto &state : states) {
    if (!state.valid) {
      LK_LOG_ERROR("[custom_calibration] State is invalid for ID {}", state.motor_id);
      return 1;
    }
    LK_LOG_INFO("[custom_calibration] State is valid for ID {}: position={}, speed={}, load={}, voltage={}, temperature={}, moving={}, current={}", state.motor_id, state.position_ticks, state.speed, state.load_pwm, state.voltage_01v, state.temperature_celsius, state.moving, state.current_milliamps);
  }
  return 0;
}
