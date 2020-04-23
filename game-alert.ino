/*
  This example demonstrates all of the needed on-device IO.

  It send signals to the shift-register to toggle the LEDs,
  receive interrupts from the button, and communicate via
  serial, all without any of these three tasks interfering
  with each-other.  Pressing the button will emit a 1 to
  the serial monitor.

  Board: Generic ESP8266 Module
  Upload Speed: 115200
*/

// Must cache interrupt handler in IRAM.  See:
// https://forum.arduino.cc/index.php?topic=616264.msg4296914#msg4296914
void ICACHE_RAM_ATTR onPressed ();

int pushButton = 2;
int dataPin = 3;
int clockPin = 0;

void setup() {
  Serial.begin(115200);
  pinMode(pushButton, INPUT_PULLUP);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(pushButton), onPressed, FALLING);
}

void loop() {
  digitalWrite(clockPin, HIGH);
  digitalWrite(clockPin, LOW);
  delay(1000);
  digitalWrite(clockPin, HIGH);
  digitalWrite(clockPin, LOW);
  delay(1000);
  digitalWrite(dataPin, HIGH);
  digitalWrite(clockPin, HIGH);
  digitalWrite(clockPin, LOW);
  delay(1000);
  digitalWrite(dataPin, LOW);
}
int lastPress = 0;
void onPressed() {
  // Re-enable other interrupts before finishing this function
  // see: https://arduino.stackexchange.com/a/68770/57971
  interrupts();

  if (debounce(&lastPress, 500)) {
    Serial.println(1);
  }
}

bool debounce(int *lastInvoke, int threshold) {
  int currentInvoke = millis();
  if (currentInvoke - *lastInvoke > threshold) {
    *lastInvoke = currentInvoke;
    return true;
  } else {
    return false;
  }
}
