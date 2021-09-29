#include <SoftwareSerial.h>  
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

SoftwareSerial zigbeeSerial(D3, D2); // RX | TX  
ESP8266WebServer httpServer(80);

IPAddress IP(192,168,62,1);
IPAddress GATEWAY(192,168,62,1);
IPAddress SUBNET(255,255,255,0);

String AP_SSID="Bus";
String AP_PASSWORD="Bus";
int AP_CHANNEL=8;

char* str="";

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
  WiFi.mode(WIFI_STA);
  // softAP
  WiFi.softAPConfig(IP, GATEWAY, SUBNET);        // declared as: bool softAPConfig (IPAddress local_ip, IPAddress gateway, IPAddress subnet)
  WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL);  // network name, network password, wifi channel
  Serial.println("softAP Started...");
  Serial.print("softAP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Filesystem
  Serial.println("Filesystem:");
  if(LittleFS.begin()){
    Serial.println("LittleFS Started...");
  } else {
    Serial.println("LittleFS Failed to Start.");
  }

  // HttpServer
  //httpServer.on("/", handleRoot);
  httpServer.onNotFound(handleUserRequest);
  httpServer.begin();
  Serial.println("HttpServer Started...");
  
}  
void loop(){  
  httpServer.handleClient();
  
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

String readLineZigbee(){
  String ss="";
  char c;
  while((c = zigbeeSerial.read()) !='\n'){
    if(c!=255)
      ss += c;
  }  
  return ss;
}
