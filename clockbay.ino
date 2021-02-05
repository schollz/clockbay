// 1. INSTALL U8glib from sketch -> include library -> manage libraries
// include the graphics library and initialize
#include "U8glib.h"
U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE); // Just for 0.91”(128*32)
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

// state0000000000000000000000000000
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
bool started = false;
float lastBpm = 0;
float lastDivision = 0;
float lastTenDiffs = 0;
int lastBpmDiffIndex = 0;

void setup() {
  pinMode(output_clock_digital_pin, OUTPUT);

  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255, 255, 255);
  }

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
  bool updateScreen = false;

  // get how to do current pulse
  int setPulseOld = setPulse;
  setPulse = map(analogRead(input_setPulse), 0, 1023, 5, 0);
  if (setPulseOld != setPulse) {
    updateScreen = true;
  }

  // read what the current voltage is set to
  // by measuring the max value over time
  float new_voltage = (float)analogRead(input_setVoltage) / 1023.0 * 5.0;
  if (new_voltage > 0.3 && (new_voltage > setVoltage || currentTime - lastVoltageSet > 1000)) {
    if (abs(new_voltage - setVoltage) > 0.2) {
      updateScreen = true;
    }
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
        if (lastBpm > 0) {
          lastTenDiffs = lastTenDiffs + result.bpm - lastBpm;
          lastBpmDiffIndex = lastBpmDiffIndex + 1;
          if (lastBpmDiffIndex == 10) {
            if (abs(lastTenDiffs) > 2) {
              Serial.print("lastTenDiffs: ");
              Serial.println(lastTenDiffs);
              reset_timer();
            }
            lastTenDiffs = 0;
            lastBpmDiffIndex = 0;
          }
        }
        lastBpm = result.bpm;
        lastDivision = result.divisions;


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
      } else  {
        inDoublePulse = false;
        Serial.println("pulse off");
        digitalWrite(13, LOW);
        digitalWrite(output_clock_digital_pin, LOW);
        u8g.firstPage();
        do {
          draw();
        } while ( u8g.nextPage() );
      }
    }
  }

  //  // stop main pulse
  //  if (currentTime - lastPulseStart > 0 && currentTime - lastPulseStart < durationBetweenLastPulses * 1 / 4 && currentTime - lastPulseStart > lastPulseDuration / 5 && inDoublePulse) {
  //    inDoublePulse = false;
  //    Serial.println("pulse off");
  //    digitalWrite(13, LOW);
  //    digitalWrite(output_clock_digital_pin, HIGH);
  //  }
  //
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

  if ((lastPulseStart - currentTime > 1500 & lastPulseStart - currentTime < 2000) || !started || (lastPulseStart - currentTime > 1500 & updateScreen)) {
    // update the screen
    if (!updateScreen) {
      reset_timer();
    }
    u8g.firstPage();
    do {
      draw();
    } while ( u8g.nextPage() );
    started = true;
  }
}


void draw(void) {
  u8g.setFont(u8g_font_unifont);
  int bpmWhole = (int)lastBpm;
  int bpmDecimal = (int)(lastBpm * 10) - 10 * bpmWhole;
  int vWhole = (int)setVoltage;
  int vDecimal = (int)(setVoltage * 10) - 10 * vWhole;
  u8g.setPrintPos(0, 11);
  if (started) {
    u8g.print("bpm " + String(bpmWhole) + "." + String(bpmDecimal) + " x" + String((int)lastDivision + 1));
  } else {
    u8g.print("no input clk");
  }
  u8g.setPrintPos(0, 28);
  u8g.print("out " + String(vWhole) + "." + String(vDecimal) + "v  ");
  if (setPulse == 0) {
    u8g.print("off");
  } else if (setPulse == 1) {
    u8g.print("x4");
  } else if (setPulse == 2) {
    u8g.print("x2");
  } else if (setPulse == 3) {
    u8g.print("x1");
  } else if (setPulse == 4) {
    u8g.print("/2");
  } else if (setPulse == 5) {
    u8g.print("/4");
  }
}
