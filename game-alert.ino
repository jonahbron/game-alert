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
  int sortedIndexes[countSsids];
  sortedSsidIndexes(countSsids, sortedIndexes);
  String options = String("");
  for (int i = 0; i < countSsids; i++) {
    options.concat("<option>" + WiFi.SSID(sortedIndexes[i]) + "</option>");
  }

  String content =
  String("<!DOCTYPE html>\r\n") +
         "<html>" +
         "<head>" +
         "<title>Game Alert Wi-Fi</title>" +
         "<meta name=\"viewport\" content=\"width=device-width,height=device-height,initial-scale=1.0\">" +
         "<style>" +
         "html {" +
         "  padding: 0;" +
         "  margin: 0;" +
         "  overflow: hidden;" +
         "}" +
         "body {" +
         "  height: 100vh;" +
         "  width: 100vw;" +
         "  overflow: hidden;" +
         "  box-sizing: border-box;" +
         "  padding: 16px;" +
         "  margin: 0;" +
         "  position: relative;" +
         "}" +
         "select,input,button {" +
         "  font-size: 20px;" +
         "  line-height: 36px;" +
         "  height: 36px;" +
         "  color: #444;" +
         "  outline: none;" +
         "  padding: 0 4px;" +
         "  box-sizing: border-box;" +
         "}" +
         "select,input {" +
         "  width: 100%;" +
         "  background: white;" +
         "  border: none;" +
         "  border-bottom: solid 2px #888;" +
         "  margin-bottom: 16px;" +
         "}" +
         "button {" +
         "  background: white;" +
         "  display: block;" +
         "  border: none;" +
         "  box-shadow: 0 3px 1px -2px rgba(0,0,0,.2),0 2px 2px 0 rgba(0,0,0,.14),0 1px 5px 0 rgba(0,0,0,.12);" +
         "  position: absolute;" +
         "  margin: 0 16px 16px;" +
         "  width: calc(100vw - 32px);" +
         "  left: 0px;" +
         "  bottom: 0px;" +
         "}" +
         "button:active {" +
         "  box-shadow: 0 5px 5px -3px rgba(0,0,0,.2),0 8px 10px 1px rgba(0,0,0,.14),0 3px 14px 2px rgba(0,0,0,.12)" +
         "}" +
         "@keyframes blink {" +
         "  0% {opacity: 1;}" +
         "  12.5% {opacity: 0.3;}" +
         "  25% {opacity: 0.3;}" +
         "  37.5% {opacity: 1;}" +
         "}" +
         ".yellow-c,.green-c, .blue-c, .red-c {" +
         "  display: inline-block;" +
         "  width: 32px;" +
         "  height: 32px;" +
         "  border-radius: 16px;" +
         "  margin: 16px 16px 16px 0;" +
         "  animation-name: blink;" +
         "  animation-duration: 2s;" +
         "  animation-iteration-count: infinite;" +
         "}" +
         ".yellow-c {" +
         "  background-color: yellow;" +
         "  animation-delay: 0s;" +
         "}" +
         ".green-c {" +
         "  background-color: green;" +
         "  animation-delay: 0.5s;" +
         "}" +
         ".blue-c {" +
         "  background-color: blue;" +
         "  animation-delay: 1s;" +
         "}" +
         ".red-c {" +
         "  background-color: red;" +
         "  animation-delay: 1.5s;" +
         "}" +
         "</style>" +
         "</head>" +
         "<body>" +
         "<span class=\"yellow-c\"></span>" +
         "<span class=\"green-c\"></span>" +
         "<span class=\"blue-c\"></span>" +
         "<span class=\"red-c\"></span>" +
         "<h1>Hey there!</h1>" +
         "<h2>I need your Wi-Fi password.</h2>" +
         "<form method=\"post\" action=\"/\">" +
         "<select name=\"s\" placeholder=\"SSID\">" +
         options +
         "</select>" +
         "<input placeholder=\"Password\" type=\"password\" name=\"p\">" +
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

/**
 * Populates an array with SSID indexes, sorted by signal strength.
 */
void sortedSsidIndexes(int countSsids, int indexes[]) {
  bool taken[countSsids];
  for (int i = 0; i < countSsids; i++) {
    taken[i] = false;
  }
  for (int i = 0; i < countSsids; i++) {
    int greatestStrength = -2147483648;
    int strongestJ = 0;
    for (int j = 0; j < countSsids; j++) {
      if (!taken[j] && WiFi.RSSI(j) > greatestStrength) {
        greatestStrength = WiFi.RSSI(j);
        strongestJ = j;
      }
    }
    indexes[i] = strongestJ;
    taken[strongestJ] = true;
  }
}
