#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>

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

#define API_HOST "http://192.168.86.38:8000"

// 0 Jonah
// 1 Jesse
// 2 Jake
// 3 Nick
#define PERSON_INDEX '0'

// Must cache interrupt handler in IRAM.  See:
// https://forum.arduino.cc/index.php?topic=616264.msg4296914#msg4296914
void ICACHE_RAM_ATTR onPressed ();

int pushButton = 2;
int dataPin = 3;
int clockPin = 0;
int CONNECT_TIMEOUT_MS = 15000;
char PERSON_STATUS = '0';


Ticker animation;

void setup() {
  // Resets WiFi info
  //WiFi.begin("foo", "bar");
  
  Serial.begin(115200);
  pinMode(pushButton, INPUT_PULLUP);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(pushButton), onPressed, FALLING);

  WiFi.mode(WIFI_STA);
  WiFi.begin();
  while (true) {
    int start = millis();
    while (true) {
      if (WiFi.status() == WL_CONNECTED) {
        // Connected to network, end setup and begin loop
        return;
      }
      if (millis() - start > CONNECT_TIMEOUT_MS) {
        // Timed out, prompt for password again
        break;
      }
      delay(100);
    }
    promptCredentials();
  }
}

bool toggleFlag = false;

void loop() {
  delay(1000);
  pullStatusUpdate();
  delay(1000);
  /*
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
  */
  pushStatusUpdate();
}
int lastPress = 0;
void onPressed() {
  // Re-enable other interrupts before finishing this function
  // see: https://arduino.stackexchange.com/a/68770/57971
  interrupts();

  if (debounce(&lastPress, 500)) {

    // TODO maybe move this net code out of interrupt function?
    if (PERSON_STATUS == '1') {
      PERSON_STATUS = '0';
    } else {
      PERSON_STATUS = '1';
    }
    toggleFlag = true;
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

void pushStatusUpdate() {
  // wait for WiFi connection
  if (toggleFlag) {
    toggleFlag = false;
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;
      http.begin(client, API_HOST);
      char body[] = {PERSON_INDEX, '=', PERSON_STATUS, '\0'};
      http.POST(body);
      http.end();
    }
  }
}

void pullStatusUpdate() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, API_HOST);
    http.GET();
    String payload = http.getString();
    Serial.println(payload);
    /*
     * Example body payload:
     * 
     * ```
     * 83972834
     * 0010
     * ```
     * 
     * First line is the time to wait until pinging again, second line is status.
     */

    int secondsUntilNextPing = getSecondsUntilNextPing(payload);
    int l = payload.length() - 1;// subtract one to trim \n
    char statuses[4] = {
      payload.charAt(l - 4),
      payload.charAt(l - 3),
      payload.charAt(l - 2),
      payload.charAt(l - 1),
    };
    renderStatus(statuses);
    Serial.println(secondsUntilNextPing);
    http.end();
  }
}

void renderStatus(char statuses[4]) {
  for (int i=0; i < 4; i++) {
    digitalWrite(dataPin, (statuses[i] == '1') ? HIGH : LOW);
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);
  }
}

int getSecondsUntilNextPing(String payload) {
  int secondsUntilNextPing;
  for (int i=0; i < payload.length(); i++) {
    if (payload.charAt(i) == '\n') {
      secondsUntilNextPing = payload.substring(0, i).toInt();
    }
  }
  return secondsUntilNextPing;
}

IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

char data = 0b11101110;
void yieldAnimationFrame() {
  data = (data << 1) | (data >> 7);
  digitalWrite(clockPin, HIGH);
  digitalWrite(clockPin, LOW);
  digitalWrite(dataPin, (data & 1) ? HIGH : LOW);
}

/**
 * Sets up an AP serving DNS and HTTP to create a captive portal.  Fields requests on
 * the access point until a completed Wi-Fi credentials form is submitted, at which
 * point it returns.
 */
void promptCredentials() {
  animation.attach(0.5, yieldAnimationFrame);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int countSsids = WiFi.scanNetworks();
  
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP("game-alert-device", "extremelymoist", 3, false, 1);


  while (WiFi.softAPgetStationNum() == 0) {
    delay(50);
  }

  WiFiServer server(80);
  server.begin();
  
  DNSServer dnsServer;
  dnsServer.start(53, "*", local_IP);

  while (true) {
    dnsServer.processNextRequest();
    bool gotCredentials = routeHttpRequest(server.available(), countSsids);
    if (gotCredentials) {
      animation.detach();
      return;
    }
  }
}

/**
 * Handles any pending HTTP requests.  If the request was a completed credentials
 * form, `true` is returned.  Otherwise `false`.
 */
bool routeHttpRequest(WiFiClient client, int countSsids) {
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        String requestLine = client.readStringUntil('\r');
        if (requestLine.startsWith("GET ")) {
          handleCaptivePortal(client, countSsids);
        } else if (requestLine.startsWith("POST ")) {
          handleCredentials(client);
          return true;
        }
        // handle other routes in ELSE statements
        break;
      }
    }
  }
  return false;
}

/**
 * Handles a POST request, and parses for SSID and password fields.  Those values are
 * saved to EEPROM so that the WiFi library can remember them.
 */
void handleCredentials(WiFiClient client) {
  bool isBody = false;
  while (client.connected()) {
    if (client.available()) {
      String line = client.readStringUntil('\r');
      if (isBody) {
        client.println(String("HTTP/1.1 200 OK\r\n") +
         "Content-Type: text/plain\r\n" +
         "Content-Length: 4\r\n" +
         "Connection: close\r\n" +
         "\r\n" +
         "Done");
         
        client.stop();
        String sParam = line.substring(0, line.indexOf('&'));
        String pParam = line.substring(line.indexOf('&') + 1);
        String sEncodedValue = sParam.substring(sParam.indexOf('=') + 1);
        String pEncodedValue = pParam.substring(pParam.indexOf('=') + 1);
        String ssid = urldecode(sEncodedValue);
        String password = urldecode(pEncodedValue);

        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(500);

        // Save credentials to EEPROM
        WiFi.begin(ssid, password);
        break;
      }
      if (line.length() == 1 && line[0] == '\n') {
        isBody = true;
      }
    }
  }
}

/**
 * Handles all GET requests.  Responds to the client with an HTML page containing
 * the credentials form.
 */
void handleCaptivePortal(WiFiClient client, int countSsids) {
  while (client.connected()) {
    if (client.available()) {
      String line = client.readStringUntil('\r');
      if (line.length() == 1 && line[0] == '\n') {
        // serve up the page, blink one LED like a cursor until a response is received
        client.println(renderPortal(countSsids));
        delay(500);
        client.stop();
        break;
      }
    }
  }
}

/**
 * Renders the HTML for the credentials form, including a list of SSIDs in a select menu.
 */
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
         "button:disabled {" +
         "  background: #aaa;" +
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
         "<button type=\"submit\" onClick=\"setTimeout(() => {this.disabled = true;},1);\">Connect</button>" +
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
 * Populates an array with SSID indexes, sorted by signal strength.  Brute force sort.
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

/**
 * @see https://circuits4you.com/2019/03/21/esp8266-url-encode-decode-example/
 */
String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

/**
 * @see https://circuits4you.com/2019/03/21/esp8266-url-encode-decode-example/
 */
unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}
