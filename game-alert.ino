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
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int countSsids = WiFi.scanNetworks();
  
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
    routeHttpRequest(server.available(), countSsids);
    dnsServer.processNextRequest();
  }
}

void routeHttpRequest(WiFiClient client, int countSsids) {
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        String requestLine = client.readStringUntil('\r');
        if (requestLine.startsWith("GET ")) {
          handleCaptivePortal(client, countSsids);
        }
        // handle other routes in ELSE statements
        break;
      }
    }
    delay(500);
    client.stop();
  }
}

void handleCaptivePortal(WiFiClient client, int countSsids) {
  while (client.connected()) {
    if (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
      if (line.length() == 1 && line[0] == '\n') {
        // serve up the page, blink one LED like a cursor until a response is received
        Serial.println(renderPortal(countSsids));
        client.println(renderPortal(countSsids));
        break;
      }
    }
  }
}

String renderPortal(int countSsids) {
  String options = String("");
  for (int i = 0; i < countSsids; i++) {
    Serial.println(WiFi.SSID(i));
    options.concat("<option>" + WiFi.SSID(i) + "</option>");
  }
  String content =
  String("<!DOCTYPE html>\r\n") +
         "<html>" +
         "<head>" +
         "<title>WiFi Credentials</title>" +
         "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">" +
         "<style>" +
         "select,input,button {" +
         "  width: 100%;" +
         "  margin-bottom: 16px;" +
         "  font-size: 20px;" +
         "}" +
         "</style>" +
         "</head>" +
         "<body>" +
         "<h1>Enter Your WiFi Credentials</h1>" +
         "<form method=\"post\" action=\"/\">" +
         "<select name=\"s\">" +
         options +
         "</select>" +
         "<input type=\"password\" name=\"p\">" +
         "<button type=\"submit\">Connect</button>" +
         "</form>" +
         "</body>" +
         "</html>";
  return String("HTTP/1.1 200 OK\r\n") +
         "Content-Type: text/html\r\n" +
         "Content-Length: " + content.length() + "\r\n" +
         "Connection: close\r\n" +
         "\r\n" +
         content;
}
