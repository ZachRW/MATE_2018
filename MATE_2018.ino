#include <PS4USB.h>
#include <SendOnlySoftwareSerial.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

USB Usb;
PS4USB PS4(&Usb);

SendOnlySoftwareSerial front(2);
SendOnlySoftwareSerial back(3);
SendOnlySoftwareSerial vert(4);
float frontLeft, frontRight, backLeft, backRight, vertPower, clawPower;
float leftX, leftY, rightX, l2, r2;

void setup() {
  Serial.begin(9600);
  front.begin(9600);
  back.begin(9600);
  vert.begin(9600);

#if !defined(__MIPSEL__)
  while (!Serial);
#endif
  Usb.Init();

  Serial.println("Start");
}

void loop() {
  Usb.Task();

  //Map inputs
  leftX  = mapFloat(PS4.getAnalogHat(LeftHatX), 0, 255, -1, 1);
  leftY  = mapFloat(PS4.getAnalogHat(LeftHatY), 0, 255, -1, 1);
  rightX = mapFloat(PS4.getAnalogHat(RightHatX), 0, 255, -1, 1);
  l2     = mapFloat(PS4.getAnalogButton(L2), 0, 255, 0, 1);
  r2     = mapFloat(PS4.getAnalogButton(R2), 0, 255, 0, 1);

  //Deadzones
  if (fabs(leftX) < .1) {
    leftX = 0;
  }
  if (fabs(leftY) < .1) {
    leftY = 0;
  }
  if (fabs(rightX) < .1) {
    rightX = 0;
  }
  if (l2 < .1) {
    l2 = 0;
  }
  if (r2 < .1) {
    r2 = 0;
  }

  //Sets horizontal motor power
  frontLeft  = motorPower(true, true);
  frontRight = motorPower(false, true);
  backLeft   = motorPower(true, false);
  backRight  = motorPower(false, false);

  //Sets vertical motor power
  if (l2 != 0) {
    vertPower = -l2;
  } else {
    vertPower = r2;
  }

  //Sets claw power
  if (PS4.getButtonPress(TRIANGLE) || PS4.getButtonPress(CIRCLE)) {
    clawPower = .75;
  } else if (PS4.getButtonPress(SQUARE) || PS4.getButtonPress(CROSS)) {
    clawPower = -.75;
  } else {
    clawPower = 0;
  }

  //Writes the motor power with the sabertooth controllers
  front.write(mapPower(-frontLeft, false));
  front.write(mapPower(-frontRight, true));
  back.write(mapPower(-backLeft, false));
  back.write(mapPower(backRight, true));
  vert.write(mapPower(-vertPower, false));
  vert.write(mapPower(clawPower, true));
}

float motorPower(bool right, bool forward) {
  int rightSign = right ? 1 : -1;
  int forwardSign = forward ? 1 : -1;
  int xReflection = rightSign * copysign(1, leftX);
  int yReflection = forwardSign * copysign(1, -leftY);

  float result = sqrt(sq(leftX) + sq(leftY));

  float absLeftX = fabs(leftX);
  float absLeftY = fabs(leftY);
  if (xReflection * yReflection == -1 && absLeftX + absLeftY != 0) {
    result *= (absLeftX - absLeftY) / (absLeftX + absLeftY);
  }

  return xReflection * result + rightSign * forwardSign * rightX;
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (float) (x - in_min) * (out_max - out_min) / (float) (in_max - in_min) + out_min;
}

byte mapPower(float motorPower, bool second) {
  motorPower = constrain(motorPower, -1, 1);
  if (second) {
    byte result = mapFloat(motorPower, -1, 1, 128, 255);
    if (abs(result - 191) < 3) {
      result = 191;
    }
    return result;
  } else {
    byte result = mapFloat(motorPower, -1, 1, 1, 127);
    if (abs(result - 64) < 3) {
      result = 64;
    }
    return result;
  }
}
