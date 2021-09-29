#include <SoftwareSerial.h>  
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
//#include <ESP8266HTTPClient.h>
//#include <HttpClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Vector.h>
//#include <ArduinoSTL.h>
//#include <StandardCplusplus.h>
//#include <vector>

SoftwareSerial zigbeeSerial(D2, D3); // RX | TX  
ESP8266WiFiMulti wifiMulti;
ESP8266WebServer httpServer(80);
//HTTPClient httpClient;
StaticJsonDocument<2048> docIpt;
WiFiClient wifiClient;
//HttpClient httpClient(wifiClient);
WebSocketsClient webSocket;

IPAddress IP(192,168,55,2);
IPAddress GATEWAY(192,168,55,1);
IPAddress SUBNET(255,255,255,0);

const char* AP_SSID="Bus_Station";
const char* AP_PASSWORD="Bus_Station";
const int AP_CHANNEL=6;
const char* WS_HOST = "192.168.137.1";
const int WS_PORT = 8888;

char* WIFI_STATIONS[][2]={
  {"FSSH-HP450G3-DD 0667","88888888"},
  /*{"LIB","7463150600"},
  {"LIB1","7463150600"},
  {"Othersgt","3218850000"},
  {"Galaxy A71DB58","weli30201220"},
  {"TP-LINK_114C","09809246"},*/
};

const int kNetworkTimeout = 30*1000;
const int kNetworkDelay = 1000;

const int pinIn1 = D5;
const int pinIn2 = D6;
const int pinOut1 = D7;
const int pinOut2 = D8;

bool LED1State = false;
bool LED2State = false;

bool Button1State = false;
bool Button2State = false;
bool Button1OldState = false;
bool Button2OldState = false;

String Bus1 = "KHH28_0";
String Bus2 = "KHH829_0";

String busData;
const char* StationUID="KHH00162";

String storage_users[50];
Vector<String> users(storage_users);

StaticJsonDocument<2048> docBooks;
JsonObject objBooks = docBooks.to<JsonObject>();

void setup(){
  // Serial
  Serial.begin(74880);
  Serial.println("");
  Serial.println("Serial:");
  Serial.println("Serial Started...");
  // zigbeeSerial
  zigbeeSerial.begin(38400);
  Serial.println("zigbeeSerial Started...");

  // Network
  Serial.println("Network:");
  WiFi.mode(WIFI_AP_STA);
  // SoftAP
  WiFi.softAPConfig(IP, GATEWAY, SUBNET);        // declared as: bool softAPConfig (IPAddress local_ip, IPAddress gateway, IPAddress subnet)
  //WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL);  // network name, network password, wifi channel
  Serial.println("softAP Started...");
  Serial.print("softAP IP address: ");
  Serial.println(WiFi.softAPIP());
  // Station
  for(int i=0;i<sizeof(WIFI_STATIONS)/8;i++){
    wifiMulti.addAP(WIFI_STATIONS[i][0], WIFI_STATIONS[i][1]);
    Serial.print("WifiMulti Add SSID: ");
    Serial.println(WIFI_STATIONS[i][0]);
  }
  Serial.print("Looking for WiFi ");
  int wifiMultiTimes = 0;  
  while((wifiMulti.run() != WL_CONNECTED) && wifiMultiTimes<20) {
    Serial.print(".");
    delay(500);
    wifiMultiTimes++;
  }
  Serial.println("");
  if(wifiMulti.run() == WL_CONNECTED){
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("station IP address: ");
    Serial.println(WiFi.localIP());
  }else{
    Serial.println("WiFi Failed to Connect.");
  } 
  //WiFi.printDiag(Serial);

  // Filesystem
  Serial.println("Filesystem:");
  if(LittleFS.begin()){
    Serial.println("LittleFS Started...");
  } else {
    Serial.println("LittleFS Failed to Start.");
  }

  // HttpServer
  //httpServer.on("/", handleRoot);
  httpServer.on("/getBusList", handleGetBusList);
  httpServer.on("/register", handleRegister);
  httpServer.on("/getBooks", handleGetBooks);
  httpServer.on("/newBooks", handleNewBooks);
  httpServer.onNotFound(handleUserRequest);
  httpServer.begin();
  Serial.println("HttpServer Started...");

  // WebSocketsClient
  webSocket.begin(WS_HOST, WS_PORT, "/", "");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);

  // pinMode
  pinMode(D5, INPUT_PULLUP);
  pinMode(D6, INPUT_PULLUP);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);

  // init
}  

void loop(){
  Button1State = !digitalRead(D5);
  Button2State = !digitalRead(D6);
  httpServer.handleClient();
  if ((WiFi.status() == WL_CONNECTED)) {
    webSocket.loop();
  }
  LED1State = Button1State == Button1OldState ? LED1State : Button1State ? !LED1State : LED1State;
  LED2State = Button2State == Button2OldState ? LED2State : Button2State ? !LED2State : LED2State;
  Button1OldState = Button1State;
  Button2OldState = Button2State;
  digitalWrite(pinOut1, LED1State);
  digitalWrite(pinOut2, LED2State);
  
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
    file.close();
  }else{
    httpServer.send(404, "text/plain", "404: Not found");
  }
}

void handleGetBusList(){
  Serial.println("/getBusList");
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.send(200, "application/json", busData);
}

void handleRegister(){
  Serial.println("/register");
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  Serial.print("count: ");
  Serial.println(users.size());
  String hash=httpServer.arg("hash");
  for(int i=0;i<users.size();i++){
    Serial.print(users[i]);
    Serial.print(" ");
    if(hash==users[i]){
      Serial.print("!");
      httpServer.send(200, "text/plain", "HASH-EXIST");
      return;
    }
  }
  users.push_back(hash);
  httpServer.send(200, "text/plain", hash);
}

void handleGetBooks(){
  Serial.println("/getBooks");
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  String user=httpServer.arg("user");
  httpServer.send(200, "text/plain", objBooks[user].as<String>());
}

void handleNewBooks(){
  Serial.println("/newBooks");
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  String user=httpServer.arg("user");
  String bus=httpServer.arg("bus");
  objBooks[user]=bus;
  httpServer.send(200, "text/plain", "OK");
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
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type){
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      webSocket.sendTXT(StationUID);
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      busData=(char*)payload;
      sendToBus();
      break;
    case WStype_PING:
        Serial.printf("[WSc] get ping\n");
        break;
    case WStype_PONG:
        Serial.printf("[WSc] get pong\n");
        break;
  }
}

void sendToBus(){
  deserializeJson(docIpt, busData);
  const size_t CAPACITY = JSON_OBJECT_SIZE(64);
  StaticJsonDocument<CAPACITY> docOpt;
  JsonObject objOpt = docOpt.to<JsonObject>();

  const size_t CAPACITY2 = JSON_OBJECT_SIZE(16);
  StaticJsonDocument<CAPACITY2> docTmp;
  JsonObject objTmp = docTmp.to<JsonObject>();

  for (JsonPair pairBooks : objBooks) {
    //Serial.println(pairBooks.value().as<char*>());
    //objTmp[pairBooks.value().as<char*>()]=true;
    objTmp[pairBooks.value().as<char*>()]=true;
  }

  for (JsonObject elemIpt : docIpt.as<JsonArray>()) {
    String key=elemIpt["RouteUID"].as<String>()+"_"+elemIpt["Direction"].as<String>();
    bool come=elemIpt["EstimateTime"].as<int>()<180;
    Serial.println(elemIpt["RouteUID"].as<String>()+" "+come);
    bool books=false;
    if(objTmp[elemIpt["RouteUID"].as<String>()]){
      books=objTmp[elemIpt["RouteUID"].as<String>()];
    }
    objOpt[key] = (int)(come && (books || (key==Bus1 && LED1State) || (key==Bus2 && LED2State)));
    /*Serial.println(value);
    Serial.println(key==Bus1);
    Serial.println(LED1State);
    Serial.println(key==Bus2);
    Serial.println(LED2State);
    Serial.println((value && ((key==Bus1 && LED1State) || (key==Bus2 && LED2State))));*/
    Serial.println(elemIpt["RouteUID"].as<String>()+" "+objOpt[key].as<String>());
  }
  serializeJson(docOpt, zigbeeSerial);
  zigbeeSerial.println();
}