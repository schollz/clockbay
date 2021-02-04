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
int input_setPulse = A0;
int input_setVoltage = A1;
int input_clock_analog_pin = A2;
int output_clock_digital_pin = 7;

// state
// setPulse
// 0 = no pulse
// 1 = pulse 4x incoming
// 2 = pulse 2x incoming
// 3 = pulse at every incoming
// 4 = pulse every other
// 5 = pulse every other other
int setPulse = 3;
float setVoltage = 1;
bool inputClockOn = false;
unsigned long startClockTime = 0;
float numberOfPulses = 0;
unsigned long lastVoltageSet = 0;
unsigned long lastPulseStart = 0;
unsigned long lastPulseDuration = 0;
unsigned long durationBetweenLastPulses = 0;
bool inDoublePulse = false;

void setup() {
  pinMode(output_clock_digital_pin, OUTPUT);

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
  // read potentiometer states
  int val = 0;
  unsigned long currentTime = millis();

  // get how to do current pulse
  setPulse = map(analogRead(input_setPulse), 0, 1023, 0, 5);

  // read what the current voltage is set to
  // by measuring the max value over time
  float new_voltage = (float)analogRead(input_setVoltage) / 1023.0 * 5.0;
  if (new_voltage > 0 && (new_voltage > setVoltage || currentTime - lastVoltageSet > 1000)) {
    setVoltage = new_voltage;
    lastVoltageSet = currentTime;
  }

  bool inputClockOn_old = inputClockOn;
  val = analogRead(input_clock_analog_pin);
  inputClockOn = (val > 50); // > 0.25 volts
  if (inputClockOn != inputClockOn_old) {
    if (inputClockOn) {
      if (lastPulseStart > 0) {
        durationBetweenLastPulses = currentTime - lastPulseStart;
      }
      lastPulseStart = currentTime;
      if (startClockTime == 0) {
        startClockTime = lastPulseStart;
      } else {
        numberOfPulses = numberOfPulses + 1;
        // do bpm calculation and show it!
        struct bpmResult result = calculateBPM(numberOfPulses, (float)(lastPulseStart - startClockTime));
        Serial.print("bpm = ");
        Serial.print(result.bpm);
        Serial.print(", setPulse = ");
        Serial.print(setPulse);
        Serial.print(", setVoltage = ");
        Serial.print(setVoltage);
        Serial.print(", lastPulseDuration = ");
        Serial.print(lastPulseDuration);
        Serial.print(", durationBetweenLastPulses = ");
        Serial.print(durationBetweenLastPulses);
        Serial.print(", divisions = ");
        Serial.println(result.divisions);
        //        u8g.drawStr(20, 40, "Hello World."); // TODO show bpm
      }
    } else {
      lastPulseDuration = currentTime - lastPulseStart;
    }
    if (setPulse > 0) {
      // write to the clock for each subdivision
      bool on = false;
      if (setPulse > 0 && setPulse <= 3) {
        on = true;
      } else if (setPulse == 4 && (int)numberOfPulses % 2 == 0) {
        on = true;
      } else if (setPulse == 5 && (int)numberOfPulses % 4 == 0) {
        on = true;
      }
      if (inputClockOn && on) {
        Serial.println("pulse on");
        digitalWrite(13, HIGH);
        digitalWrite(output_clock_digital_pin, HIGH);
        inDoublePulse = true;
      } else if (inDoublePulse) {
        inDoublePulse = false;
        Serial.println("pulse off");
        digitalWrite(13, LOW);
        digitalWrite(output_clock_digital_pin, HIGH);
      }
    }
  }

  // stop main pulse
  if (currentTime - lastPulseStart > 0 && currentTime - lastPulseStart < durationBetweenLastPulses * 1 / 4 && currentTime - lastPulseStart > lastPulseDuration / 5 && inDoublePulse) {
    inDoublePulse = false;
    Serial.println("pulse off");
    digitalWrite(13, LOW);
    digitalWrite(output_clock_digital_pin, HIGH);
  }

  //  // write the double pulse time
  if (durationBetweenLastPulses > 0 && setPulse > 0 && setPulse < 3) {
    int iStart = 2;
    int iEnd = 2;
    if (setPulse == 1) {
      iStart = 1;
      iEnd = 3;
    }
    if (inDoublePulse == false) {
      for (int i = iStart; i <= iEnd; i++) {
        if (currentTime - lastPulseStart >= durationBetweenLastPulses * (i + 1) / 4) {
          continue;
        }
        if (currentTime - lastPulseStart > durationBetweenLastPulses * i / 4 & currentTime - lastPulseStart < durationBetweenLastPulses * i / 4 + lastPulseDuration / 5) {
          inDoublePulse = true;
          Serial.print("pulse");
          Serial.print(i);
          Serial.println(" on");
          digitalWrite(13, HIGH);
          digitalWrite(output_clock_digital_pin, HIGH);
        }
      }
    } else {
      for (int i = iStart; i <= iEnd; i++) {
        if (currentTime - lastPulseStart >= durationBetweenLastPulses * (i + 1) / 4) {
          continue;
        }
        if (currentTime - lastPulseStart > durationBetweenLastPulses * i / 4 + lastPulseDuration / 5) {
          inDoublePulse = false;
          Serial.print("pulse");
          Serial.print(i);
          Serial.println(" off");
          digitalWrite(13, LOW);
          digitalWrite(output_clock_digital_pin, LOW);
        }
      }
    }
  }
}
