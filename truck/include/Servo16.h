#ifndef SERVO16_H
#define SERVO16_H

#include "Arduino.h"

#define DEFAULT_uS_LOW 1080
#define DEFAULT_uS_HIGH 1920
#define MIN_PULSE_WIDTH 500  // the shortest pulse sent to a servo
#define MAX_PULSE_WIDTH 2500 // the longest pulse sent to a servo
#define DEFAULT_ANGLE_RANGE 180

class Servo
{
public:
  Servo();
  int pin, channel, min, max, hertz, resolution, angle_range;
  void attach(int pin, int channel, int min, int max);
  void writeMicroseconds(float value);
  void write(float value);
  void setPeriodHertz(int hertz);
  void setAngleRange(int angle_range);

private:
  int usToTicks(float us);
  float fmap(float x, float in_min, float in_max, float out_min, float out_max);
};

#endif
