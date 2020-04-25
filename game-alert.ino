#include <DNSServer.h>

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
#include <ESP8266WiFi.h>

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
  promptCredentials();
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

IPAddress local_IP(192,168,0,1);
IPAddress gateway(192,168,0,2);
IPAddress subnet(255,255,255,0);

void promptCredentials() {
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP("game-alert-device", "extremelymoist", 3, false, 1);


  while (WiFi.softAPgetStationNum() == 0) {
    Serial.println("No connections yet");
    // TODO sequentially blink LEDs while waiting for connection
    delay(100);
  }
  Serial.println("Station connected");

  // Now that a connection has been made, change the blink style

  WiFiServer server(80);
  server.begin();
  
  DNSServer dnsServer;
  dnsServer.start(53, "*", local_IP);

  while (true) {
    WiFiClient client = server.available();
    if (client) {
      Serial.println("Got client");
      while (client.connected()) {
        if (client.available()) {
          String line = client.readStringUntil('\r');
          Serial.print(line);

          if (line.length() == 1 && line[0] == '\n') {
            // serve up the page, blink one LED like a cursor until a response is received
            Serial.println("HTTP/1.1 200 OK\r\nContent-Type:text/html\r\nContent-length:6\r\nConnection:close\r\n\r\nHello!");
            client.println("HTTP/1.1 200 OK\r\nContent-Type:text/html\r\nContent-length:6\r\nConnection:close\r\n\r\nHello!");
            break;
          }
        }
      }
      delay(1);
      client.stop();
    }
    dnsServer.processNextRequest();
  }
}
