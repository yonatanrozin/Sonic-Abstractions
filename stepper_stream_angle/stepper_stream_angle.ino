#define PIN_EN 3
#define PIN_DIR 4
#define PIN_STEP 5

#define motorStepsPerRev 1600

int motorPosition = 0;

void setup() {
  Serial.begin(9600);
  
  // put your setup code here, to run once:
  pinMode(PIN_EN, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_STEP, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(PIN_EN, LOW);
  digitalWrite(PIN_DIR, HIGH);
  digitalWrite(PIN_STEP, LOW);

}

void loop() {
  if (!Serial.available()) return;
  if (Serial.read() == 100) {
    digitalWrite(LED_BUILTIN, HIGH);
    while (motorPosition != motorStepsPerRev) {
      for (int i = 0; i < 2; i++) {
        digitalWrite(PIN_STEP, HIGH);
        delay(3);
        digitalWrite(PIN_STEP, LOW);
        delay(3);
      }
      motorPosition += 2;
      byte motorOrientationByte = (float)motorPosition/(float)motorStepsPerRev * 255.0;
      Serial.write(motorOrientationByte);
    }
    motorPosition = 0;
  }
  
}
