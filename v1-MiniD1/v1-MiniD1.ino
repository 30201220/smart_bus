#include <SoftwareSerial.h>  
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define LED_PIN D5
#define BUTTON_PIN D6

SoftwareSerial zigbeeSerial(D3, D2); // TX | RX  
ESP8266WiFiMulti wifiMulti;
ESP8266WebServer httpServer(80);

IPAddress IP(192,168,4,1);
IPAddress GATEWAY(192,168,4,1);
IPAddress SUBNET(255,255,255,0);

char* AP_SSID = "Bus";
char* AP_PASSWORD = "Bus";
int AP_CHANNEL = 2;
int buttonState = 0;
int LEDState = 0;

String BusNumber;

const byte numChars = 1000;
char receivedChars[numChars];
boolean newData = false;

boolean LEDOUTPUT = false;

StaticJsonDocument<200> docIpt;

void setup(){
  pinMode(LED_PIN,OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  Serial.begin(74880);
  //read configuration from FS json
  Serial.println("mounting FS...");
  // zigbeeSerial
  zigbeeSerial.begin(38400);
  Serial.println("zigbeeSerial Started...");
  // Network
  Serial.println("Network:");
  WiFi.mode(WIFI_AP);
  // softAP
  WiFi.softAPConfig(IP, GATEWAY, SUBNET);        // declared as: bool softAPConfig (IPAddress local_ip, IPAddress gateway, IPAddress subnet)
  WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL);  // network name, network password, wifi channel
  Serial.println("softAP Started...");
  Serial.print("softAP IP address: ");
  Serial.println(WiFi.softAPIP());
  // Filesystem
  Serial.println("Filesystem:");
  /*if(LittleFS.begin()){
    Serial.println("LittleFS Started...");
  } else {
    Serial.println("LittleFS Failed to Start.");
  }*/
  // HttpServer
  httpServer.on("/LED-Control", handleRoot);
  httpServer.onNotFound(handleUserRequest);
  httpServer.begin();
  Serial.println("HttpServer Started...");
  
  //data
  if (LittleFS.begin())
  {
    Serial.println("mounted file system");
    if (LittleFS.exists("/web/test.txt"))
    {
      Serial.println("reading config file");
      // 開啟文件
      File configFile = LittleFS.open("/web/test.txt", "r");
      if (configFile)
      {
        Serial.println("opened config file");
        // 文件內容大小
        int s = configFile.size();
        Serial.printf("Size=%d\r\n", s);

        // 讀取文件內容
        BusNumber = configFile.readString();
        Serial.println(BusNumber);

        configFile.close();
      }
      else{
        Serial.println("opened config file failed");
      }
    }
  }
  else
  {
    Serial.println("failed to mount FS");
  }
  //end read
  
}

void loop(){
  buttonState = digitalRead(BUTTON_PIN);
  httpServer.handleClient();
  recvWithStartEndMarkers();
  showNewData();
  digitalWrite(LED_PIN, LEDOUTPUT ? HIGH : LOW);
  
 
  
  
  /*String json = zigbeeSerial.readString();
  StaticJsonDocument<200> doc;
  deserializeJson(doc, json);
  serializeJson(doc, Serial);
 // const char* world = doc["KHH28_0"];
  Serial.println(json);
  //Serial.println(doc["KHH28_0"]);*/
}

void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '{';
  char endMarker = '}';
  char rc;

  while (zigbeeSerial.available() > 0 && newData == false) {
    rc = zigbeeSerial.read();

    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
      else {
        receivedChars[ndx] = '}';
        ndx++;
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (rc == startMarker) {
      recvInProgress = true;
      receivedChars[ndx] = '{';
      ndx++;
    }
  }
}

void showNewData() {
  if (newData == true) {
    Serial.println(receivedChars);
    newData = false;
    deserializeJson(docIpt, receivedChars);
    //serializeJsonPretty(docIpt, Serial);
    //const char* world = docIpt["KHH28_0"];
    //JsonObject object = docIpt.as<JsonObject>();
    //const char* world = object["KHH28_0"];
    LEDOUTPUT = (bool)docIpt[BusNumber];
    Serial.println((bool)docIpt[BusNumber].as<int>());
  }
}

void handleUserRequest(){
  String reqUri = httpServer.uri();
  Serial.print("reqUri: ");
  Serial.print(reqUri);
  if(reqUri=="/"){
    reqUri="/index.html";
  }else if(reqUri.endsWith("/")) {
    reqUri = reqUri.substring(0,reqUri.length()-1);
  }
  reqUri="/web"+reqUri;
  Serial.print(" => ");
  Serial.println(reqUri);
  
  if (LittleFS.exists(reqUri)){
    File file = LittleFS.open(reqUri, "r");
    httpServer.streamFile(file, getContentType(reqUri));
    file.close();                             // 返回true
  }else{
    httpServer.send(404, "text/plain", "404: Not found");
  }
}

void handleRoot(){
  BusNumber = httpServer.arg("busLine");
  Serial.print(BusNumber);
  httpServer.sendHeader("Location", "/index.html");       
  httpServer.send(303);
}

String getContentType(String filename){
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".json")) return "text/json";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

String readLineZigbee(){
  String ss="";
  char c;
  while((c = zigbeeSerial.read()) !='\n'){
    if(c!=255)
      ss += c;
  }  
  return ss;
}
