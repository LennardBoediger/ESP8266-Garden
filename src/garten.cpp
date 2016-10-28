#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <TimeLib.h>
#include <NtpClientLib.h>
#include <Wifi.h>

#define ROM_START_H   0   //hardcoded ROM address definiton
#define ROM_START_MIN 1
#define ROM_DURATION 2
#define WATER_RELAI 12
#define WATER_TANK_SENSOR 13
#define WATER_TANK_LED 14

ESP8266WebServer server(80);
enum error_handler{
valid,
invalid_character,
invalid_sum
};

//to do: struct uint 8
//setting variables
uint8_t start_h;
uint8_t start_min;
uint8_t duration;

//service variables
uint8_t watering_flag=0;
uint8_t water_tank_empty;
unsigned long previousMillis = 0;
unsigned long currentMillis;//to do remove ??

void handelWaterTank() {
	water_tank_empty = !(digitalRead(WATER_TANK_SENSOR));
	if (water_tank_empty == 1) {
		digitalWrite(WATER_TANK_LED, 0);
	}
	else{
		digitalWrite(WATER_TANK_LED, 1);
	}
}
void handelWater() {
	if ((currentMillis-previousMillis >= duration*1000 && watering_flag==1) || water_tank_empty == 1) {
		digitalWrite(WATER_RELAI, 0);
		watering_flag = 0;
		Serial.println("water");
	}
	if (hour() == start_h && minute() == start_min && (timeStatus() == timeSet || timeStatus() == timeNeedsSync) && water_tank_empty == 0){
		if (second() == 0) { //only once per minute
			watering_flag = 1;
			previousMillis = millis();
			digitalWrite(WATER_RELAI, 1);
		}
	}
}

void onSTAGotIP() {
//	Serial.printf("Got IP: %s\r\n", ipInfo.ip.toString().c_str());
	NTP.begin("fritz.box", 1, true);
	NTP.setInterval(60);
}

void onSTADisconnected(WiFiEventStationModeDisconnected event_info) {
	Serial.printf("Disconnected from SSID: %s\n", event_info.ssid.c_str());
	Serial.printf("Reason: %d\n", event_info.reason);
}

int handleStart_time(String stmp) {
	//to do: def emu for return
  if(isdigit(stmp[0]) && isdigit(stmp[1]) && stmp[2]==':' && isdigit(stmp[3]) && isdigit(stmp[4])){
    stmp[5]='\n';
    if(atoi(&stmp[0])<=23 && atoi(&stmp[3])<=59){
      start_h = atoi(&stmp[0]);
      start_min = atoi(&stmp[3]);
      EEPROM.begin(512);
      EEPROM.write(ROM_START_H, start_h);
      EEPROM.write(ROM_START_MIN, start_min);
      EEPROM.end();
      return valid; //0
    } else {
      return invalid_character; //1
    }
  } else {
    return invalid_sum; //2
  }
}

int handleDuration(String dtmp) {
	int i = 0;
	while (dtmp[i]!='\n' && i >= 2) {
		i++;
		if (isdigit(dtmp[i])) {
			if (atoi(&dtmp[0])>=0 && atoi(&dtmp[0])<=255) {
				duration = atoi(&dtmp[0]);
				EEPROM.begin(512);
				EEPROM.write(ROM_DURATION, duration);
				EEPROM.end();
				return valid;
			}
			else{
				return invalid_sum;
			}
		}
		else{
			return invalid_character;
		}
	}
}

void handleRoot() {
  String message = "<html><head><meta charset=\"UTF-8\"><title>ESP8266-Garden</title><link rel\"stylesheet\" href=\"css/bootstrap.min.css\"><link rel\"stylesheet\" href=\"style.css\"><style> ";
  //message += "input[type=\"submit\"] { color: rgb(255, 255, 255); cursor: pointer; display: block; height: 36px; min-height: auto; min-width: 104px; text-overflow: ellipsis; vertical-align: middle; white-space: nowrap; width: 120px; will-change: margin, width, background-color, color; word-break: break-word; column-rule-color: rgb(255, 255, 255); perspective-origin: 60px 18px; transform-origin: 60px 18px; background: rgb(38, 142, 223) none repeat scroll 0% 0% / auto padding-box border-box; border: 0px none rgb(255, 255, 255); font: normal normal normal normal 16px / normal \"Source Sans Pro\", Arial, sans-serif; margin: 9.6px 4.8px; outline: rgb(255, 255, 255) none 0px; overflow: hidden; padding: 0px 16px; transition: margin 0.4s cubic-bezier(0.165, 0.84, 0.44, 1) 0s, width 0.4s cubic-bezier(0.165, 0.84, 0.44, 1) 0s, background-color 0.1s ease 0s, color 0.1s ease 0s; }";
  //message += "input[type=\"text\"] { height: 24px; vertical-align: middle; width: 228px; word-break: break-word; perspective-origin: 114px 12px; transform-origin: 114px 12px; font: normal normal normal normal 16px / normal \"Source Sans Pro\", Arial, sans-serif; margin: 1px 0px;padding: 0px; }";
  message += "text { box-sizing: border-box; color: rgb(63, 70, 76); display: inline-block; height: 20px; text-align: left; vertical-align: middle; width: 142.375px; word-break: break-word; column-rule-color: rgb(63, 70, 76); perspective-origin: 71.1875px 10px; transform-origin: 71.1875px 10px; border: 0px none rgb(63, 70, 76); font: normal normal normal normal 16px / 20px \"Source Sans Pro\", Arial, sans-serif; margin: 1px 6px 1px 4px; outline: rgb(63, 70, 76) none 0px; }";
  message += " </style></head><body>";
  message += "<form action='/' method='POST'>";
	message += "<em>";
	message += NTP.getTimeDateString();
	message += "</em>";
  message += "<label for='Duration'>Duration:</label><input type='text' name='Duration' value=";
	message += String(duration);
	message += ">";
	String errmsg = "";
  if (server.hasArg("Duration")) {
    String dtmp = server.arg("Duration");
		switch(handleDuration(dtmp)) {
			case 0:
				message += "<em style=\"color: green;\">";
		    message += "Duration was set to ";
		    message += dtmp;
		    message += "</em>";
				break;
			case 1:
			errmsg += "<strong style=\"color: red;\">";
			errmsg += dtmp;
			errmsg += "Duration is a Number";
			errmsg += "</strong>";
			break;
			case 2:
			errmsg += "<strong style=\"color: red;\">";
			errmsg += dtmp;
			errmsg += "Duration must be smaller than 256";
			errmsg += "</strong>";
			break;

		}
	}

  errmsg = "";
  if (server.hasArg("starttime")) {
    String stmp = server.arg("starttime");
    switch(handleStart_time(stmp)) {
      case 0:
        errmsg += "<em style=\"color: green;\">";
        errmsg += "Starttime was set to ";
        errmsg += stmp;
        errmsg += "</em>";
        break;
      case 1:
        errmsg += "<strong style=\"color: red;\">";
        errmsg += stmp;
        errmsg += "is not a valid time. use HH:MM";
        errmsg += "</strong>";
        break;
      case 2:
        errmsg += "<strong style=\"color: red;\">";
        errmsg += "use HH:MM";
        errmsg += "</strong>";
        break;
    }
  }

  message += "<br/><label for='starttime'>Starttime:</label><input type='time' name='starttime' value='";
  if (start_h < 10) message += "0";
  message += String(start_h);
  message += ':';
  if (start_min < 10) message += "0";
  message += String(start_min);
  message += "'>";

  message += errmsg;

  message += "<br/><input type='submit' class= 'btn btn-primary' value='senden'></form>";
  message += "</body></html>";

  server.send(200, "text/html", message);
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void){
	pinMode(WATER_RELAI, OUTPUT);
	pinMode(WATER_TANK_SENSOR,INPUT);
	pinMode(WATER_TANK_LED,OUTPUT);
	digitalWrite(WATER_RELAI, 0);
	digitalWrite(WATER_TANK_LED, 1);
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.begin(SSID, PASSWORD);
  Serial.println("");

  // Read EPROM Time config
  EEPROM.begin(512);
  start_h   = EEPROM.read(ROM_START_H);
  start_min = EEPROM.read(ROM_START_MIN);
	duration = 	EEPROM.read(ROM_DURATION);
  EEPROM.end();

  // Wait for connection
 while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  //NTP  setup
  NTP.onNTPSyncEvent([](NTPSyncEvent_t ntpEvent) {
		if (ntpEvent) {
			Serial.print("Time Sync error: ");
			if (ntpEvent == noResponse)
				Serial.println("NTP server not reachable");
			else if (ntpEvent == invalidAddress)
				Serial.println("Invalid NTP server address");
		}
		else {
			Serial.print("Got NTP time: ");
			Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
		}
	});
	//start time sync
	onSTAGotIP();
}

void loop(void){
	currentMillis = millis();
  server.handleClient();
	handelWaterTank();
	handelWater();
}
