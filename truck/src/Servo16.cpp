#include "Servo16.h"

Servo::Servo()
{
  this->pin = -1;
  this->channel = -1;
  this->min = DEFAULT_uS_LOW;
  this->max = DEFAULT_uS_HIGH;
  this->hertz = 50;
  this->resolution = 14;
  this->angle_range = DEFAULT_ANGLE_RANGE;
}

void Servo::attach(int pin, int channel, int min, int max)
{
  this->pin = pin;
  this->channel = channel;

  // min/max checks
  if (min < MIN_PULSE_WIDTH) // ensure pulse width is valid
    min = MIN_PULSE_WIDTH;
  if (max > MAX_PULSE_WIDTH)
    max = MAX_PULSE_WIDTH;
  this->min = min; // store this value in uS
  this->max = max; // store this value in uS

  ledcSetup(channel, hertz, resolution);
  ledcAttachPin(pin, channel);
}

void Servo::writeMicroseconds(float value)
{
  if (value < min)
    value = min;
  if (value > max)
    value = max;
  value = usToTicks(value);
  ledcWrite(channel, value);
}

void Servo::write(float value)
{
  value = fmap(value, 0, angle_range, min, max);
  writeMicroseconds(value);
}

void Servo::setPeriodHertz(int hertz)
{
  this->hertz = hertz;
}

void Servo::setAngleRange(int angle_range)
{
  this->angle_range = angle_range;
}

int Servo::usToTicks(float us)
{
  return (int)(us * hertz * (pow(2, resolution)) / 1000000);
}

float Servo::fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
