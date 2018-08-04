#include "ESP8266WiFi.h"
#include "WiFiClient.h"
#include "ESP8266WebServer.h"
#include <ArduinoJson.h>
#include "FS.h"
#include <ESPHue.h>
#include "credentials.h"	//This file isn't included in the git
#include "structs.h"

#ifndef credentials
const char *ssid = "your ssid";
const char *pass = "your pass";

const char *ap_ssid = "CH";
const char *ap_pass = "defaultPass";
#endif

_rgb rgb;

//Command Parser
#define MAX_COMMAND_LENGTH 80
#define MAX_NUM_ARGUMENTS 8
#define UART_BAUDRATE 9600
int cmd_ind = 0;
char cmd_buf[MAX_COMMAND_LENGTH + 1] = "         /0";
char *argv[MAX_NUM_ARGUMENTS];
unsigned int argc = 0;
volatile int new_command = 0;

//Configurations
//Config file
File configFile;
_configFile cf;

//Config Lights
File lights;

//Config Rooms
File rooms;

//Webserver stuff
ESP8266WebServer wp(80);	//Webportal

//Node connector
#define MAX_SRV_CLIENTS 10
WiFiServer ssh(22);	//Textline interface for nodes
WiFiClient serverClients[MAX_SRV_CLIENTS];

//Hue settings
WiFiClient chue;
ESPHue hue = ESPHue(chue, "uDCpx3W4OhcdipQTiPIFYgXvIizP5yMYUf6Y5E7a", "192.168.0.52", 80);


void setup()
{

	Serial.begin(115200);
	Serial.println("CH - Booting up.... Please wait");


	Serial.println("CH - Loading configurations...");
	// Get the last configs
	SPIFFS.begin();
	configFile = SPIFFS.open("/config.json", "r");
	if (!configFile) {
		Serial.println("CH - No configuration file found");
	}
	else {
		size_t size = configFile.size();
		if (size > 1024) {
			Serial.println("CH - Configuration file is too big");
		}
		else {

			std::unique_ptr<char[]> buf(new char[size]);
			configFile.readBytes(buf.get(), size);
			StaticJsonBuffer<200> jsonBuffer;
			JsonObject& json = jsonBuffer.parseObject(buf.get());

			if (!json.success()) {
				Serial.println("CH - Couldn't parse the config file");
			}
			else {
				Serial.println("CH - Loading settings");
				cf.empty = false;
				cf.api = json["api"];
				cf.ip = json["ip"];

				Serial.print("\tAPI key: ");
				Serial.println(cf.api);
				Serial.print("\tHue IP: ");
				Serial.println(cf.ip);
			}
		}
	}

	Serial.println("Wifi - Initialise");
	WiFi.mode(WIFI_AP_STA);

	Serial.print("Wifi - Connecting to: ");
	Serial.print(ssid);
	Serial.print(" ");

	WiFi.begin(ssid, pass);

	while (WiFi.status() != WL_CONNECTED) {
		//This is indeed somekind of bootloop, to prevent it from rebooting we delay it
		delay(150);
		Serial.print(".");
	}

	Serial.print("\nWifi - Status: ");
	// Indicate if there is a connection
	Serial.println("Connected");

	//Print some debug info
	Serial.print("\tIP: ");
	Serial.println(WiFi.localIP());
	Serial.print("\tGateway: ");
	Serial.println(WiFi.gatewayIP());

	Serial.print("AP - Creating Network");
	// Create the access point so we can connect the sensors and other stuff
	WiFi.softAP(ap_ssid, ap_ssid);

	Serial.print("AP - Created: ");
	Serial.println(ap_ssid);
	// Display the name
	Serial.print("\tIP: ");
	Serial.println(WiFi.softAPIP());


	//Some logic to see if its configured
	if (!cf.empty) {
		Serial.println("Hue - Connecting to the hue");
		// Connect to the hue



		Serial.println("Hue - Reading information");
		// Reading out the settings


		Serial.println("Hue - Setting up the rooms");
		// Getting the rooms offline
		//Serial.println(hue.getGroupInfo(0));

		Serial.println("Hue - Settings up the lights");
		// Getting the lights offline
		//Serial.println(hue.getLightInfo(1));

	}

	Serial.println("WP - Setting up the web envoirment");
	wp.on("/", wp_handleRoot);
	wp.on("/config", wp_configPage);
	wp.begin();

	Serial.println("SSH - Setting up the node connection");
	ssh.begin();
	ssh.setNoDelay(true);

	Serial.println("CH - Done!");

}

void loop()
{

	wp.handleClient();

	ssh_handleClient();

}

void wp_handleRoot() {
	wp.send(200, "text/html", "Nothing yet to see...<br>Thijs");
}



void ssh_handleClient() {
	if (ssh.hasClient()) {
		for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
			//find free/disconnected spot
			if (!serverClients[i] || !serverClients[i].connected()) {
				if (serverClients[i]) serverClients[i].stop();
				serverClients[i] = ssh.available();
				Serial.print("New client: "); Serial.print(i);

				break;
			}
		}
	}

	for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
		//		serverClients[i].write("\r\n-----------------------------------\r\n,", 41);
		//		serverClients[i].write("This connection is not ready for usage\r\n", 41);
		//		serverClients[i].write("-----------------------------------\r\n\r\n,", 41);
		//		serverClients[i].stop();

		if (serverClients[i] && serverClients[i].connected()) {
			if (serverClients[i].available()) {
				//get data from the telnet client and push it to the UART
				String data = "";
				while (serverClients[i].available()) {

					//Create the command buffer
					cmd_buf[cmd_ind] = (char)serverClients[i].read();
					if (cmd_buf[cmd_ind] == '\n') {
						cmd_buf[cmd_ind - 1] = '\0';
						argCreator();
					}
					else {
						cmd_ind++;
					}

				}
			}
		}

	}
}

void argCreator() {

	char *argtemp = NULL;
	cmd_buf[cmd_ind] = NULL;
	argc = 0;

	if ((argtemp = strtok(cmd_buf, " ")) != NULL) // Check if string contains arguments
	{
		argv[argc++] = argtemp; // save pointer to argument argc
		while (argc < MAX_NUM_ARGUMENTS && (argtemp = strtok(NULL, " ")) != NULL)
		{
			argv[argc++] = argtemp;

		}
	}


	argProcessor();
	cmd_ind = 0;



}


void argProcessor() {
	Serial.print("Argc: ");
	Serial.print(argc);
	Serial.println("x");

	for (int i = 0; i < argc; i++)
	{
		Serial.print(i);
		Serial.print(" - ");
		Serial.println(argv[i]);
	}

	if (strcmp(argv[0], "uit") == 0) {
		Serial.println("Commando Uit!");
		hue.setGroup(0, hue.OFF, 255, 255, 255, 255, 255);
	}
	else if (strcmp(argv[0], "aan") == 0) {
		Serial.println("Commando Aan!");
		hue.setGroup(0, hue.ON, 255, 255, 255, 255, 255);
	}
	else if (strcmp(argv[0], "set") == 0) {
		if (argc == 4) {
			setRGB(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), rgb.brightness);
		}
		else if (argc == 5) {
			setRGB(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
		}
		else {
			printSSH("set {r} {g} {b} \n set {r} {g} {b} {brightness}");
		}
	}
	else if (strcmp(argv[0], "show") == 0) {
		hue.setGroupWhite(0, hue.ON, 255, rgb.brightness, rgb.ct);
		setColor(rgb);
	}
	else if (strcmp(argv[0], "white") == 0) {
		if (argc == 2) {
			hue.setGroupWhite(0, hue.ON, 255, rgb.brightness, atoi(argv[1]) );
			rgb.ct = atoi(argv[1]);
		}
	}
	else if (strcmp(argv[0], "brightness") == 0) {
		if (argc == 2)
			rgb.brightness = atoi(argv[1]);
	}
}

void setColor(int r, int g, int b, int brightness) {
	hue.setGroup(0, hue.ON, 255, rgb.brightness, rgb.r, rgb.g, rgb.b);
	rgb.r = r;
	rgb.g = g;
	rgb.b = b;
	rgb.brightness = brightness;
	return;
}

void setColor(_rgb rgb) {
	hue.setGroup(0, hue.ON, 255, rgb.brightness, rgb.r, rgb.g, rgb.b);
	return;
}

void setRGB(int r, int g, int b, int brightness) {
	rgb.r = r;
	rgb.g = g;
	rgb.b = b;
	rgb.brightness = brightness;

	Serial.print("r: ");
	Serial.println(rgb.r);

	Serial.print("g: ");
	Serial.println(rgb.g);

	Serial.print("b: ");
	Serial.println(rgb.b);

	return;
}

void printSSH(char* s) {
	for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
		serverClients[i].write(s, sizeof(s) + 1);
	}
}

void wp_configPage() {
	String content = "";
	content += "{\"RGB\":{\"Red\":\"";
	content += rgb.r;
	content += "\",\"Green\":\"";
	content += rgb.g;
	content += "\",\"Blue\":\"";
	content += rgb.b;
	content += "\"},\"Brightness\":\"";
	content += rgb.brightness;
	content += "\",\"White\":\"";
	content += rgb.ct;
  content += "\"}";
	wp.send(200, "application/json", content);
}
