#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <HardwareSerial.h>

// Pinagem

// UART - RPi zero W
#define RXpld 16
#define TXpld 17

// I2C - TC, EPS e ADCS
#define SDAitc 21
#define SCLitc 22

// VSPI - LoRa SX1276 TT&C 
#define SCKv 18
#define MISOv 19
#define MOSIv 23
#define CSv 5
#define RSTv 14
#define DIO0v 4

// HSPI - SD card OBC
#define SCKh 15
#define MISOh 35 // Input-only
#define MOSIh 12
#define CSh 13

// PWM - SimpleFOC
#define PWMu 25
#define PWMv 26
#define PWMw 27
#define EN 32

// Burn wire - Gatilho
#define BURN 33