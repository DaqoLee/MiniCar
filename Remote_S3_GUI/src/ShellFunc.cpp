#include "ShellFunc.h"
#include <Arduino.h>
#include <SimpleSerialShell.h>
#include "main.h"
/*
pressure
MKSspeed 2 53

MKSpos 1 -3200 28

57 4 50 50
57 4 -50 50
------------------

STSspeed 1 6000
ZDTspeed 2 25

57 4 50 50
57 4 -50 50
ZDT 1 -3200 10

STSspeed 1 0
ZDTspeed 2 0

*/


int STScmd(int argc, char **argv) {
  if (argc != 4) {
    shell.println("bad argument count");
    return -1;
  }

  auto id = atoi(argv[1]);
  auto pos = atoi(argv[2]);
  auto vel = atoi(argv[3]);

  // uint8_t dir = 0;
  // if (pos < 0) {
  //   dir = 1;
  //   pos = -pos;
  // }

  shell.print("calling ");
  shell.print(id);
  shell.print(" with ");
  shell.print(pos);
  shell.print(" and ");
  shell.println(vel);

  // sms_sts.WritePosEx(id, pos, vel, 200);//舵机(ID1)以最高速度V=2400步/秒，加速度A=50(50*100步/秒^2)，运行至P1=4095位置

  return 0;
}

int setMaxonSpeed(int argc, char **argv) {
  if (argc == 3) {
    // auto id = atoi(argv[1]);
    // auto num = atoi(argv[2]);
    // if (id == 1)
    // {
    //   runVelocity(0x101,num);
    // }
    // else if (id == 2)
    // {
    //   runVelocity(0x102,num);
    // }
  }
  return 0;
}

int Servo_Ctrl(int argc, char **argv) {

  return 0;
}

int setPID(int argc, char **argv) {

  if (argc == 7)
  {
    // posPID.setPID(atof(argv[1]),atof(argv[2]),atof(argv[3]));
    // velPID.setPID(atof(argv[4]),atof(argv[5]),atof(argv[6]));
  }
  
  return 0;
}

// with Serial pointer motor.setTorque(10);
void shellInit(void) {


 shell.addCommand(F("STS <id> <pos> <vel>"), STScmd);
 shell.addCommand(F("maxon  <vel>"), setMaxonSpeed);
 shell.addCommand(F("Servo  <id> <vel>"), Servo_Ctrl);
 shell.addCommand(F("setPID  <po> <io> <do> <pi> <ii> <di>"), setPID);
}