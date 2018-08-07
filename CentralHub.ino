#include "ESP8266WiFi.h"
#include "WiFiClient.h"
#include "ESP8266WebServer.h"
#include <ArduinoJson.h>
#include "FS.h"
#include <ESPHue.h>
#include "credentials.h"	//This file isn't included in the git
#include "structs.h"

int lastConnection = -1;

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
	pinMode(D2, OUTPUT);
	digitalWrite(D2, HIGH);
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

	Serial.print("AP - Creating Network");
	// Create the access point so we can connect the sensors and other stuff
	WiFi.softAP(ap_ssid, ap_ssid);

	Serial.print("AP - Created: ");
	Serial.println(ap_ssid);
	// Display the name
	Serial.print("\tIP: ");
	Serial.println(WiFi.softAPIP());


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
	digitalWrite(D2, LOW);

	//Print some debug info
	Serial.print("\tIP: ");
	Serial.println(WiFi.localIP());
	Serial.print("\tGateway: ");
	Serial.println(WiFi.gatewayIP());


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
				Serial.print("New client: "); Serial.print(i); Serial.println();
				lastConnection = i;
				printlnSSH("Welcome at the Central Hub Command Line!");
				break;
			}
		}
	}

	for (int i = 0; i < MAX_SRV_CLIENTS; i++) {

		if (serverClients[i] && serverClients[i].connected()) {
			if (serverClients[i].available()) {
				lastConnection = i;
				//get data from the telnet client and push it to the UART
				String data = "";
				while (serverClients[i].available()) {

					//Create the command buffer
					cmd_buf[cmd_ind] = (char)serverClients[i].read();
					Serial.print(cmd_buf[cmd_ind]);
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
	//Serial.print("Argc: ");
	//Serial.print(argc);
	//Serial.println("x");

	//for (int i = 0; i < argc; i++)
	//{
	//	Serial.print(i);
	//	Serial.print(" - ");
	//	Serial.println(argv[i]);
	//}

	if (strcmp(argv[0], "allOff") == 0) {
		//Serial.println("Commando Uit!");
		printlnSSH("Turning Off");
		//hue.setGroup(0, hue.OFF, 255, rgb.brightness, rgb.r, rgb.g, rgb.b);
		hue.setGroupPower(0, hue.OFF);
	}
	else if (strcmp(argv[0], "allOn") == 0) {
		//Serial.println("Commando Aan!");
		printlnSSH("Turning On");
		hue.setGroupPower(0, hue.ON);
	}
	else if (strcmp(argv[0], "on") == 0) {
		if (argc != 1) {
			for (int i = 1; i < argc; i++)
			{
				setLightPower(1, atoi(argv[i]));
			}
		}
	}
	else if (strcmp(argv[0], "off") == 0) {
		if (argc != 1) {
			for (int i = 1; i < argc; i++)
			{
				setLightPower(0, atoi(argv[i]));
			}
		}
	}
	else if (strcmp(argv[0], "set") == 0) {
		if (argc == 1) {
			showRgbValues();
		}
		else if (argc == 4) {
			printlnSSH("Set the colors");
			setRGB(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), rgb.brightness);
		}
		else if (argc == 5) {
			printlnSSH("Set the colors");
			setColorLight(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
		}
	}
	else if (strcmp(argv[0], "show") == 0) {
		if (argc == 2)
			if (strcmp(argv[1], "white") == 0) {
				printlnSSH("Showing White!");
				hue.setGroupWhite(0, hue.ON, 255, rgb.brightness, rgb.ct);
			}
			else {
				printlnSSH("Showing Color!");
				setColor(rgb);
			}
		else if (argc == 3) {
			printlnSSH("Only 1 light");
			if (strcmp(argv[1], "white") == 0) {
				printlnSSH("Showing White!");
				hue.setLightWhite(atoi(argv[2]), hue.ON, 255, rgb.brightness, rgb.ct);
			}
			else {
				printlnSSH("Showing Color!");
				setColor(rgb, atoi(argv[2]));
			}
		}
		else if (argc == 1) {
			showRgbValues();
		}
			
	}
	else if (strcmp(argv[0], "white") == 0) {
		if (argc == 2) {
			printlnSSH("Set White");
			rgb.ct = atoi(argv[1]);
		}
		else if (argc == 3) {
			printlnSSH("Set White Light");
			hue.setLightWhite(atoi(argv[2]), hue.ON, 255, rgb.brightness, atoi(argv[1]));
		}
		else if (argc == 1) {
			printSSH("CT Value: ");
			printlnSSH(rgb.ct);
		}
	}
	else if (strcmp(argv[0], "brightness") == 0) {
		if (argc == 2) {
			printlnSSH("Set brightness");
			rgb.brightness = atoi(argv[1]);
		}
		else if (argc == 1) {
			printSSH("Brightness: ");
			printlnSSH(rgb.brightness);
		}
	}
	else if (strcmp(argv[0], "help") == 0) {
		printlnSSH("Help page for the Central Hub\n");
		printlnSSH("Turn 1 or more lights on:");
		printlnSSH("on {lamp id} ...\n");
		printlnSSH("Turn 1 or more lights off:");
		printlnSSH("off {lamp id} ...\n");
		printlnSSH("Turn all on or off");
		printlnSSH("allOn / allOff\n");
		printlnSSH("Set the colors");
		printlnSSH("set {r} {g} {b}\nset {r} {g} {b} {light}\n");
		printlnSSH("show color");
		printlnSSH("show color {light}");
		printlnSSH("show white {light}\n");
		printlnSSH("Set the white light");
		printlnSSH("white {ct}\nwhite {ct} {light}\n");
		printlnSSH("Set brightness");
		printlnSSH("brightness {bright}");
		printlnSSH("Help - This page");


	}
}

void setColor(_rgb rgb) {
	hue.setGroup(0, hue.ON, 255, rgb.brightness, rgb.r, rgb.g, rgb.b);
	return;
}

void setColor(_rgb rgb, int light) {
	hue.setLight(light, hue.ON, 255, rgb.brightness, rgb.r, rgb.g, rgb.b);
	return;
}

void setColorLight(int r, int g, int b, int light) {
	hue.setLight(light, hue.ON, 255, rgb.brightness, r, g, b);
	return;
}

void setRGB(int r, int g, int b, int brightness) {
	rgb.r = r;
	rgb.g = g;
	rgb.b = b;
	rgb.brightness = brightness;
	Serial.println("=======\nNew Colors\n======");
	Serial.print("r: ");
	Serial.println(rgb.r);

	Serial.print("g: ");
	Serial.println(rgb.g);

	Serial.print("b: ");
	Serial.println(rgb.b);

	return;
}

void printlnSSH(char* s) {
		serverClients[lastConnection].print(s);
		serverClients[lastConnection].print("\n");
}

void printlnSSH(int i) {
	serverClients[lastConnection].print(i);
	serverClients[lastConnection].print("\n");
}

void printSSH(int i) {
	serverClients[lastConnection].print(i);
}

void printSSH(char* s) {
		serverClients[lastConnection].print(s);
	
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
	content += "\",\"Connectivity\":{\"Live Connections\":\"";
	content += liveConnections();
	content += "\"}}";
	wp.send(200, "application/json", content);
}

void setLightPower(bool state, int light) {
	hue.setLightPower(light, state);
}

void showRgbValues() {
	printSSH("Colors:\nRed: ");
	printlnSSH(rgb.r);
	printSSH("Green: ");
	printlnSSH(rgb.g);
	printSSH("Blue: ");
	printlnSSH(rgb.b);
	printSSH("\nBrightness: ");
	printlnSSH(rgb.brightness);
	printSSH("CT Value: ");
	printlnSSH(rgb.ct);
	return;
}

int liveConnections() {
	int connections = MAX_SRV_CLIENTS;
	for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
		if (!serverClients[i] || !serverClients[i].connected())
			connections--;
	}
	return connections;
}