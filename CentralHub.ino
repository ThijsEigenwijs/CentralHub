#include "ESP8266WiFi.h"
#include "WiFiClient.h"
#include "ESP8266WebServer.h"
#include "credentials.h"	//This file isn't included in the git

#ifndef credentials
const char *ssid = "your ssid";
const char *pass = "your pass";

const char *ap_ssid = "CH";
const char *ap_pass = "defaultPass";
#endif


//Webserver stuff
ESP8266WebServer server(80);


void setup()
{

	Serial.begin(115200);
	Serial.println("CH - Booting up.... Please wait");


	Serial.println("CH - Loading configurations...");
	// Get the last configs out of EEPROM

	Serial.println("CH - Loading settings");
	// Set the settings

	Serial.println("Wifi - Initialise");
	//Initialse the wifi here
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
	
	Serial.print("Wifi - Status: ");
	// Indicate if there is a connection
	Serial.print("Connected");

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
	Serial.println("Hue - Connecting to the hue");

	Serial.println("Hue - Reading information");
	// Reading out the settings

	Serial.println("Hue - Setting up the rooms");
	// Getting the rooms offline

	Serial.println("Hue - Settings up the lights");
	// Getting the lights offline

	Serial.println("WP - Setting up the web envoirment");
	server.on("/", handleRoot);
	Serial.println("WP - Done");
	Serial.println("WP - {Homepage}");

	Serial.println("CH - Done!");

}

void loop()
{

	/* add main program code here */

}

void handleRoot() {
	server.send(200, "text/html", "<h1>You are connected</h1>");
}
