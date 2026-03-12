/**
 * Copyright 2026 LiveKit, Inc.
 *
 * @file SetLimits.cpp
 * @brief Set persistent SMS/STS EEPROM position limits.
 *
 * Usage:
 *   ./SetLimits <serial_port> <id> <min_angle_deg> <max_angle_deg>
 *
 * Example:
 *   ./SetLimits /dev/ttyUSB0 1 -45.5 90.0
 */

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

#include "SCServo.h"

namespace {

constexpr int kMinPosition = 0;
constexpr int kMaxPosition = 4095;  // SMS/STS position range
constexpr int kMinServoId = 0;
constexpr int kMaxServoId = 253;    // 254 is broadcast and unsafe for EEPROM writes
constexpr double kMinAngleDeg = -180.0;
constexpr double kMaxAngleDeg = 180.0;
constexpr double kCenterTick = 2047.5;
constexpr double kTicksPerDegree = static_cast<double>(kMaxPosition) / 360.0;

bool ParseInt(const char* arg, int& out) {
  if (arg == nullptr) {
    return false;
  }

  char* end = nullptr;
  errno = 0;
  const long value = std::strtol(arg, &end, 10);
  if (errno != 0 || end == arg || *end != '\0') {
    return false;
  }
  if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
    return false;
  }

  out = static_cast<int>(value);
  return true;
}

bool ParseDouble(const char* arg, double& out) {
  if (arg == nullptr) {
    return false;
  }

  char* end = nullptr;
  errno = 0;
  const double value = std::strtod(arg, &end);
  if (errno != 0 || end == arg || *end != '\0') {
    return false;
  }
  if (!std::isfinite(value)) {
    return false;
  }

  out = value;
  return true;
}

int AngleDegToTicks(double angle_deg) {
  const double raw_ticks = kCenterTick + (angle_deg * kTicksPerDegree);
  return static_cast<int>(std::lround(raw_ticks));
}

void PrintUsage(const char* program_name) {
  std::cout << "Usage: " << program_name << " <serial_port> <id> <min_angle_deg> <max_angle_deg>" << std::endl;
  std::cout << "  id         : 0-253" << std::endl;
  std::cout << "  min_angle  : -180.0 to 180.0 (double)" << std::endl;
  std::cout << "  max_angle  : -180.0 to 180.0 (double)" << std::endl;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 5) {
    PrintUsage(argv[0]);
    return 1;
  }

  int id = -1;
  double min_angle_deg = 0.0;
  double max_angle_deg = 0.0;
  if (!ParseInt(argv[2], id) || !ParseDouble(argv[3], min_angle_deg) || !ParseDouble(argv[4], max_angle_deg)) {
    std::cout << "Invalid numeric argument." << std::endl;
    PrintUsage(argv[0]);
    return 1;
  }

  if (id < kMinServoId || id > kMaxServoId) {
    std::cout << "Invalid servo ID " << id << ". Expected " << kMinServoId << "-" << kMaxServoId << "." << std::endl;
    return 1;
  }
  if (min_angle_deg < kMinAngleDeg || min_angle_deg > kMaxAngleDeg) {
    std::cout << "Invalid min angle " << min_angle_deg << ". Expected " << kMinAngleDeg << " to " << kMaxAngleDeg << " degrees." << std::endl;
    return 1;
  }
  if (max_angle_deg < kMinAngleDeg || max_angle_deg > kMaxAngleDeg) {
    std::cout << "Invalid max angle " << max_angle_deg << ". Expected " << kMinAngleDeg << " to " << kMaxAngleDeg << " degrees." << std::endl;
    return 1;
  }
  if (min_angle_deg > max_angle_deg) {
    std::cout << "Invalid limits: min_angle must be <= max_angle." << std::endl;
    return 1;
  }

  const int min_ticks = AngleDegToTicks(min_angle_deg);
  const int max_ticks = AngleDegToTicks(max_angle_deg);
  if (min_ticks < kMinPosition || min_ticks > kMaxPosition || max_ticks < kMinPosition || max_ticks > kMaxPosition) {
    std::cout << "Converted ticks out of range. Computed [" << min_ticks << ", " << max_ticks << "]." << std::endl;
    return 1;
  }

  SMS_STS servo;
  constexpr int kBaudRate = 1000000;
  if (!servo.begin(kBaudRate, argv[1])) {
    std::cout << "Failed to initialize SMS/STS on port " << argv[1] << std::endl;
    return 1;
  }

  std::cout << "Programming servo ID " << id << " with limits [" << min_angle_deg << ", " << max_angle_deg
            << "] deg -> [" << min_ticks << ", " << max_ticks << "] ticks" << std::endl;

  if (!servo.unLockEeprom(static_cast<u8>(id))) {
    std::cout << "Failed to unlock EEPROM." << std::endl;
    servo.end();
    return 1;
  }

  const bool wrote_min = servo.writeWord(static_cast<u8>(id), SMS_STS_MIN_ANGLE_LIMIT_L, static_cast<u16>(min_ticks));
  const bool wrote_max = servo.writeWord(static_cast<u8>(id), SMS_STS_MAX_ANGLE_LIMIT_L, static_cast<u16>(max_ticks));
  const bool locked = servo.LockEeprom(static_cast<u8>(id));
  if (!wrote_min || !wrote_max || !locked) {
    servo.end();
    std::cout << "Failed to write and lock EEPROM limits." << std::endl;
    return 1;
  }

  int out_of_range_ticks = 0;
  bool has_out_of_range_target = true;
  if (max_ticks < kMaxPosition) {
    out_of_range_ticks = max_ticks + 200;
  } else if (min_ticks > kMinPosition) {
    out_of_range_ticks = min_ticks - 200;
  } else {
    has_out_of_range_target = false;
  }

  const auto us_per_s = 1000000;


  if (has_out_of_range_target) {
    if (!servo.EnableTorque(static_cast<u8>(id), 1)) {
      servo.end();
      std::cout << "EEPROM limits updated, but failed to enable torque for out-of-range test." << std::endl;
      return 1;
    }

    // Home to PoseEx 0, wait 1 second
    std::cout << "Moving servo to home position (tick 0)..." << std::endl;
    if (!servo.WritePosEx(static_cast<u8>(id), 2048, 500, 50)) {
      servo.end();
      std::cout << "Failed to move servo to home position before out-of-range test." << std::endl;
      return 1;
    }
    usleep(us_per_s * 5);

    std::cout << "Attempting out-of-range move to tick " << out_of_range_ticks
              << " (limits are [" << min_ticks << ", " << max_ticks << "])." << std::endl;
    const bool move_sent = servo.WritePosEx(static_cast<u8>(id), static_cast<s16>(out_of_range_ticks), 500, 50);
    if (!move_sent) {
      servo.end();
      std::cout << "EEPROM limits updated, but failed to send out-of-range move command." << std::endl;
      return 1;
    }
  } else {
    std::cout << "EEPROM limits updated. Skipping out-of-range move test because configured limits already span full range."
              << std::endl;
  }

  usleep(5 * us_per_s);

  servo.end();
  std::cout << "EEPROM limits updated successfully." << std::endl;
  return 0;
}
