#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define CONF_ID "30cl"
#define CONF_TYPE "rel4"
#define CONF_VERSION 1

struct ConfHeader {
	char id[5];
	char type[5];
	byte version;
};

struct ConfData {
	char ssid[32];
	char pass[32];
	char brokerHost[15];
	char brokerUser[32];
	char brokerPass[32];
	char brokerSub[32];
};

int OUTPUTS[] = {D5, D6, D7, D2};

WiFiClient espClient;
PubSubClient client(espClient);
ConfData config;
String clientId = "ESP8266Client-";

void loadConfig() {
	ConfHeader header;
	EEPROM.get(0, header);

	if (strcmp(header.id, CONF_ID) != 0 || strcmp(header.type, CONF_TYPE) != 0) {
		for (int i = 0 ; i < EEPROM.length() ; i++) {
			EEPROM.write(i, 0);
		}
		return;
	}

	switch (header.version) {
		case 1:
			// Last case, do nothing
			break;
	}

	EEPROM.get(sizeof(header), config);
}

void saveConfig() {
	ConfHeader header = {
		CONF_ID,
		CONF_TYPE,
		CONF_VERSION
	};
	EEPROM.put(0, header);
	EEPROM.put(sizeof(header), config);
	EEPROM.commit();
}

void setLights(bool value) {
	setLights(value, value, value, value);
}
void setLights(bool a, bool b, bool c, bool d) {
	digitalWrite(OUTPUTS[0], a);
	digitalWrite(OUTPUTS[1], b);
	digitalWrite(OUTPUTS[2], c);
	digitalWrite(OUTPUTS[3], d);
}

void readSerialCommands() {
	while (Serial.available()) {
		char c = Serial.read();
		if (c == '?') {
			Serial.println("#30cl-command;v=0.1;tv=0.1");
			Serial.print("#ip;"); Serial.println(WiFi.localIP());
			Serial.print('#ESPcid;'); Serial.println(String(ESP.getChipId(), HEX));
			Serial.print("#wifi;");
			switch (WiFi.status()) {
				case WL_CONNECTED: Serial.println("WL_CONNECTED"); break;
				case WL_NO_SHIELD: Serial.println("WL_NO_SHIELD"); break;
				case WL_IDLE_STATUS: Serial.println("WL_IDLE_STATUS"); break;
				case WL_NO_SSID_AVAIL: Serial.println("WL_NO_SSID_AVAIL"); break;
				case WL_SCAN_COMPLETED: Serial.println("WL_SCAN_COMPLETED"); break;
				case WL_CONNECT_FAILED: Serial.println("WL_CONNECT_FAILED"); break;
				case WL_CONNECTION_LOST: Serial.println("WL_CONNECTION_LOST"); break;
				case WL_DISCONNECTED: Serial.println("WL_DISCONNECTED"); break;
				default: Serial.println(""); break;
			}
			Serial.print("#pubsub;");
			switch (client.state()) {
				case MQTT_CONNECTION_TIMEOUT: Serial.println("CONNECTION_TIMEOUT;"); break;
				case MQTT_CONNECTION_LOST: Serial.println("CONNECTION_LOST"); break;
				case MQTT_CONNECT_FAILED: Serial.println("CONNECT_FAILED"); break;
				case MQTT_DISCONNECTED: Serial.println("DISCONNECTED"); break;
				case MQTT_CONNECTED: Serial.println("CONNECTED"); break;
				case MQTT_CONNECT_BAD_PROTOCOL: Serial.println("CONNECT_BAD_PROTOCOL"); break;
				case MQTT_CONNECT_BAD_CLIENT_ID: Serial.println("CONNECT_BAD_CLIENT_ID"); break;
				case MQTT_CONNECT_UNAVAILABLE: Serial.println("CONNECT_UNAVAILABLE"); break;
				case MQTT_CONNECT_BAD_CREDENTIALS: Serial.println("CONNECT_BAD_CREDENTIALS"); break;
				case MQTT_CONNECT_UNAUTHORIZED: Serial.println("CONNECT_UNAUTHORIZED"); break;
				default: Serial.println(""); break;
			}
			Serial.print("#clientId;"); Serial.println(clientId);
			Serial.println("c1;c;32;WiFi SSID");
			Serial.println("c2;c;32;WiFi Password");
			Serial.println("c3;c;15;MQTT broker host IP");
			Serial.println("c4;c;15;MQTT broker username");
			Serial.println("c5;c;15;MQTT broker password");
			Serial.println("c6;c;15;MQTT broker subject");
		} else if (c == '<') {
			switch (Serial.read()) {
				case '1': Serial.print('>'); Serial.println(config.ssid); break;
				case '2': Serial.print('>'); Serial.println(config.pass); break;
				case '3': Serial.print('>'); Serial.println(config.brokerHost); break;
				case '4': Serial.print('>'); Serial.println(config.brokerUser); break;
				case '5': Serial.print('>'); Serial.println(config.brokerPass); break;
				case '6': Serial.print('>'); Serial.println(config.brokerSub); break;
				default: Serial.println('$'); break;
			}
		} else if (c == '>') {
			switch (Serial.read()) {
				case '1':
					Serial.readStringUntil('\n').toCharArray(config.ssid, 32); 
					wifiReconnect();
					break;
				case '2':
					Serial.readStringUntil('\n').toCharArray(config.pass, 32); 
					wifiReconnect();
					break;
				case '3':
					Serial.readStringUntil('\n').toCharArray(config.brokerHost, 15);
					mqttConnect();
					break;
				case '4':
					Serial.readStringUntil('\n').toCharArray(config.brokerUser, 32); 
					mqttConnect();
					break;
				case '5':
					Serial.readStringUntil('\n').toCharArray(config.brokerPass, 32); 
					mqttConnect();
					break;
				case '6':
					Serial.readStringUntil('\n').toCharArray(config.brokerSub, 32); 
					mqttConnect();
					break;
					
				default: Serial.println('$'); break;
			}

		} else if (c == '@') {
			saveConfig();
		}
	}
}

void wifiReconnect() {
	Serial.println(config.ssid);
	WiFi.begin(config.ssid, config.pass);
}

bool mqttConnect() {
	if (client.connected()) {
		return true;
	}

	if (!config.brokerHost) {
		return false;
	}
	client.setServer(config.brokerHost, 1883);

	if (!config.brokerUser || !config.brokerPass || !config.brokerSub) {
		return false;
	}

	if (!client.connect(clientId.c_str(), config.brokerUser, config.brokerPass)) {
		return false;
	}

	client.subscribe(config.brokerSub);

	return true;
}

void blinkBuildinLed(int onTime) {
	digitalWrite(BUILTIN_LED, LOW);
	delay(100);
	digitalWrite(BUILTIN_LED, HIGH);
	delay(600 - onTime);
}

void callback(char* topic, byte* payload, unsigned int length) {
	if (length == 4) {
		setLights(
			(char)payload[0] == '1',
			(char)payload[1] == '1',
			(char)payload[2] == '1',
			(char)payload[3] == '1'
		);
	} else if (length == 1) {
		setLights((char)payload[0] == '1');
	}
}

void setup() {
	Serial.begin(115200);
	EEPROM.begin(512);
	WiFi.mode(WIFI_STA);

	loadConfig();

	pinMode(BUILTIN_LED, OUTPUT);
	digitalWrite(BUILTIN_LED, HIGH);

    clientId += String(random(0xffff), HEX);

	wifiReconnect();
	mqttConnect();

	for (int i = 0; i < 4; i++) {
		digitalWrite(OUTPUTS[i], HIGH);
		pinMode(OUTPUTS[i], OUTPUT);
	}

	client.setCallback(callback);
}

void loop() {
	readSerialCommands();

	// show some indication that we are not connected
	if (WiFi.status() != WL_CONNECTED) {
		blinkBuildinLed(100);
		return;
	}

	if (!mqttConnect()) {
		blinkBuildinLed(300);
		client.loop();
		return;
	}

	client.loop();

	// From this point, we are connected to the access point :)
	// 

	delay(10);
}
