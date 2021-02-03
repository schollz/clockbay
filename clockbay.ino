// pin for clock input
int inputClock = A3; 
// pins for clock outputs
int outputClock[] = { 2, 3, 4}; 
int outputClockCount = 3;

// state
bool inputClockOn = false;
unsigned long startClockTime = 0;
float numberOfPulses = 0;

void setup() {
  for (int i = 0; i < outputClockCount; i++) {
    pinMode(outputClock[i], OUTPUT);
  }
  Serial.begin(9600);
}

float calculateBPM(float pulses, float totalMS) {
  float result;
  result = (pulses * 60) / (totalMS / 1000);
  while (result > 300) {
    result = result / 2;
  }
  return result;
}

void loop() {
  bool inputClockOn_old = inputClockOn;
  inputClockOn = (analogRead(inputClock) > 200); // > 1 volts
  if (inputClockOn != inputClockOn_old) {
    if (inputClockOn) {
      if (startClockTime == 0) {
        startClockTime = millis();
      } else {
        numberOfPulses = numberOfPulses + 1;
        // do bpm calculation and show it!
        float bpm = calculateBPM(numberOfPulses, (float)(millis() - startClockTime));
        Serial.print("bpm = ");
        Serial.println(bpm);
      }
    }
    for (int i = 0; i < outputClockCount; i++) {
      if (inputClockOn) {
        digitalWrite(outputClock[i], HIGH);
      } else {
        digitalWrite(outputClock[i], LOW);
      }
    }
  }
}