// debug.h

#ifndef _DEBUG_h
#define _DEBUG_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include "HardwareSerial.h"

class debug
{
public:
	debug();
	~debug();
	void print(String s);
	void print();

private:

};

debug::debug()
{
	Serial.begin(115200);
}

debug::~debug()
{
}

#endif

