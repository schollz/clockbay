// 1. INSTALL U8glib from sketch -> include library -> manage libraries
// include the graphics library and initialize
#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE | U8G_I2C_OPT_DEV_0);
// connections:
// 3.3V  Vdd
// Gnd Gnd
// Analog 4  SDA
// Analog 5  SCK

// read the beat using A0
int input_clock_analog_pin = A0;
// subdivide the beat based on which digital pin you output from
// 0 = send every pulse
// 1 = send every other pulse
// 2 = send every 2 pulses
// 3 = send every 3 pulses
int output_clock_digital_pin[] = {7, 8, 9, 10, 11};

// state
bool inputClockOn = false;
unsigned long startClockTime = 0;
float numberOfPulses = 0;
unsigned long lastPulseStart = 0;
unsigned long lastPulseDuration = 0;
unsigned long durationBetweenLastPulses = 0;
bool inDoublePulse = false;

void setup() {
  // set subdivided output clocks
  for (int i = 0; i < (sizeof(output_clock_digital_pin) / sizeof(output_clock_digital_pin[0])); i++) {
    pinMode(output_clock_digital_pin[i], OUTPUT);
  }

  // set font
  u8g.setFont(u8g_font_unifont);
  Serial.begin(9600);
}

struct bpmResult {
  float bpm;
  float divisions;
};

struct bpmResult calculateBPM(float pulses, float totalMS) {
  struct bpmResult result;
  result.bpm = (pulses * 60) / (totalMS / 1000);
  result.divisions = 0;
  while (result.bpm > 160) {
    result.bpm = result.bpm / 2;
    result.divisions = result.divisions + 1;
  }
  return result;
}

void reset_timer() {
  numberOfPulses = 0;
  startClockTime = 0;
  inputClockOn = false;
}

void loop() {
  bool inputClockOn_old = inputClockOn;
  int val = analogRead(input_clock_analog_pin);
  inputClockOn = (val > 50); // > 0.25 volts
  //  if (val > 0) {
  //    Serial.println(val);
  //  }
  if (inputClockOn != inputClockOn_old) {
    if (inputClockOn) {
      if (lastPulseStart > 0) {
        durationBetweenLastPulses = millis() - lastPulseStart;
      Serial.print("durationBetweenLastPulses: ");
      Serial.println(durationBetweenLastPulses);
      }
      lastPulseStart = millis();
      if (startClockTime == 0) {
        startClockTime = lastPulseStart;
      } else {
        numberOfPulses = numberOfPulses + 1;
        // do bpm calculation and show it!
        struct bpmResult result = calculateBPM(numberOfPulses, (float)(lastPulseStart - startClockTime));
        Serial.print("bpm = ");
        Serial.print(result.bpm);
        Serial.print(", divisions = ");
        Serial.println(result.divisions);
        u8g.drawStr(20, 40, "Hello World."); // TODO show bpm
      }
    } else {
      lastPulseDuration =millis() - lastPulseStart;
    }
    // write to the clock for each subdivision
    if (inputClockOn) {
      digitalWrite(13, HIGH);
      digitalWrite(output_clock_digital_pin[0], HIGH);
      for (int i = 1; i < (sizeof(output_clock_digital_pin) / sizeof(output_clock_digital_pin[0])); i++) {
        if ((int)numberOfPulses % (i + 1 ) == 0) {
          digitalWrite(output_clock_digital_pin[i], HIGH);
        }
      }
    } else {
      digitalWrite(13, LOW);
      digitalWrite(output_clock_digital_pin[0], HIGH);
      for (int i = 1; i < (sizeof(output_clock_digital_pin) / sizeof(output_clock_digital_pin[0])); i++) {
        digitalWrite(output_clock_digital_pin[i], LOW);
      }
    }
  }

  // write the double pulse time
  if (durationBetweenLastPulses > 0) {
    if (inDoublePulse == false) {
      if (millis() - lastPulseStart > durationBetweenLastPulses / 2 & millis() - lastPulseStart < durationBetweenLastPulses / 2 + lastPulseDuration/2) {
        inDoublePulse = true;
        digitalWrite(output_clock_digital_pin[0], HIGH);
      }
    } else {
      if (millis() - lastPulseStart > durationBetweenLastPulses / 2 + lastPulseDuration / 2) {
        inDoublePulse = false;
        digitalWrite(output_clock_digital_pin[0], LOW);
      }
    }
  }
}
