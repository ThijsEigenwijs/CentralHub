#include "ESP8266WiFi.h"
#include "WiFiClient.h"
#include "ESP8266WebServer.h"


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

	Serial.print("Wifi - Status: ");
	// Indicate if there is a connection

	Serial.println("AP - Creating Network");
	// Create the access point so we can connect the sensors and other stuff

	Serial.print("AP - Created: ");
	// Display the name

	//Some logic to see if its configured
	Serial.println("Hue - Connecting to the hue");

	Serial.println("Hue - Reading information");
	// Reading out the settings

	Serial.println("Hue - Setting up the rooms");
	// Getting the rooms offline

	Serial.println("Hue - Settings up the lights");
	// Getting the lights offline

	Serial.println("WP - Setting up the web envoirment");

	Serial.println("WP - Done");
	Serial.println("WP - {Homepage}");

	Serial.println("CH - Done!");

}

void loop()
{

	/* add main program code here */

}
