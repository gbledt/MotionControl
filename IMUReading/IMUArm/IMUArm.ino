// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#include "Wire.h"

// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
//#include <Servo.h>

#include "MPU6050_6Axis_MotionApps20.h"
//#include "MPU6050.h" // not necessary if using MotionApps include file

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for SparkFun breakout and InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 mpu;

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
//uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
float grav[3];
float yawOffset = 0;

const int pinOut = 11;
const int pinIn = 13;
int reset = 0;
//Servo motorYaw;
//Servo motorPitch;

//Servo elbow;
//Servo wrist;
//Servo twist;

void setup() {
    // Begin I2C communication
    Wire.begin();
    pinMode(pinOut, OUTPUT);
    pinMode(pinIn, INPUT);
    digitalWrite(pinOut, HIGH);

    // Initialize serial communication
    Serial.begin(38400);

    // NOTE: 8MHz or slower host processors, like the Teensy @ 3.3v or Ardunio
    // Pro Mini running at 3.3v, cannot handle this baud rate reliably due to
    // the baud timing being too misaligned with processor ticks. You must use
    // 38400 or slower in these cases, or use some kind of external separate
    // crystal solution for the UART timer.

    // initialize device
    //Serial.println(F("Initializing I2C devices..."));
    mpu.initialize();

    // verify connection
    //Serial.println(F("Testing device connections..."));
    //Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

    // load and configure the DMP
    //Serial.println(F("Initializing DMP..."));
    devStatus = mpu.dmpInitialize();
    
    // make sure it worked (returns 0 if so)
    if (devStatus == 0) {
        // turn on the DMP, now that it's ready
        //Serial.println(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);

        // set our DMP Ready flag so the main loop() function knows it's okay to use it
        dmpReady = true;

        // get expected DMP packet size for later comparison
        packetSize = mpu.dmpGetFIFOPacketSize();
    } else {
        // ERROR!
    }
    //motorYaw.attach(10);
    //motorPitch.attach(9);
    //elbow.attach(6);
    //wrist.attach(5);
    //twist.attach(3);
    //twist.write(105);
    //elbow.write(150);
    //wrist.write(30);
}

void loop() {
  // if programming failed, don't try to do anything
  if (!dmpReady) return;

  // resets length
  while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

  // get current FIFO count
  fifoCount = mpu.getFIFOCount();

  // check for overflow (this should never happen unless our code is too inefficient)
  if (fifoCount >= 1024) {
      // reset so we can continue cleanly
      mpu.resetFIFO();
  } 
  // wait for correct available data length, should be a VERY short wait
  while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

  // read a packet from FIFO
  mpu.getFIFOBytes(fifoBuffer, packetSize);
       
  // track FIFO count here in case there is > 1 packet available
  // (this lets us immediately read more without waiting for an interrupt)
  fifoCount -= packetSize;

  // Display quaternion values in easy matrix form: w x y z
  mpu.dmpGetQuaternion(&q, fifoBuffer);
            
  // Conversion from quaternions back to yaw, pitch, roll
  if (q.w >= 2) q.w = -4 + q.w;
  if (q.x >= 2) q.x = -4 + q.x;
  if (q.y >= 2) q.y = -4 + q.y;
  if (q.z >= 2) q.z = -4 + q.z;
  grav[0] = 2 * (q.x*q.z - q.w*q.y);
  grav[1] = 2 * (q.w*q.x + q.y*q.z);
  grav[2] = q.w*q.w - q.x*q.x - q.y*q.y + q.z*q.z;  
  ypr[0] = atan2(2*q.x*q.y - 2*q.w*q.z, 2*q.w*q.w + 2*q.x*q.x - 1);
  ypr[1] = atan(grav[0] / sqrt(grav[1]*grav[1] + grav[2]*grav[2]));
  ypr[2] = atan(grav[1] / sqrt(grav[0]*grav[0] + grav[2]*grav[2]));
  
  int buttonState = digitalRead(pinIn);
  
  if (buttonState == HIGH) {
    yawOffset = ypr[0]* 180/M_PI;
  }
  
  
  float yaw = ypr[0]* 180/M_PI - yawOffset;
  //Serial.print("ypr\t");
  // Creates the communication protocol
  Serial.print("$");
  Serial.print(constrain(map(yaw, -90, 90, 0, 180),0,180));Serial.print("#");
  Serial.print(constrain(map(ypr[1]* 180/M_PI, -90, 90, 0, 180),0,180));Serial.print("%");
  Serial.print(constrain(map(ypr[2]* 180/M_PI, -90, 90, -20, 160),0,180));Serial.println("&");
  
  
  //motorYaw.write(constrain(map((ypr[0]* 180/M_PI),-90,90,0,175),0,175));    
  //motorPitch.write(constrain(map((ypr[1]* 180/M_PI),-90,90,0,175),0,175));    
}
