#include <Arduino.h>
#include <Controller.h>
#include <Button.h>

namespace Button {

	static const uint32_t debounceMs = 30;

	static bool stablePressed = false;
	static bool candidatePressed = false;
	static bool edgePressed = false;
	static bool edgeReleased = false;
	static uint32_t lastTransitionMs = 0;

	static bool readPressedRaw()
	{
		// Momentary switch wired to 3.3 V with internal pull-down enabled.
		return digitalRead(pinMomentarySwitch) == HIGH;
	}

	void setup(void)
	{
		pinMode(pinMomentarySwitch, INPUT_PULLDOWN);

		bool pressed = readPressedRaw();
		stablePressed = pressed;
		candidatePressed = pressed;
		lastTransitionMs = millis();
		edgePressed = false;
		edgeReleased = false;

		Serial.printf("Button setup on pin %d, initial state: %s\n",
					  pinMomentarySwitch,
					  stablePressed ? "pressed" : "released");
	}

	void loop(void)
	{
		edgePressed = false;
		edgeReleased = false;

		uint32_t now = millis();
		bool rawPressed = readPressedRaw();

		if (rawPressed != candidatePressed)
		{
			candidatePressed = rawPressed;
			lastTransitionMs = now;
			return;
		}

		if (candidatePressed == stablePressed)
		{
			return;
		}

		if ((now - lastTransitionMs) < debounceMs)
		{
			return;
		}

		stablePressed = candidatePressed;
		if (stablePressed)
		{
			edgePressed = true;
			Serial.println("Button pressed");
		}
		else
		{
			edgeReleased = true;
			Serial.println("Button released");
		}
	}

	bool isPressed(void)
	{
		return stablePressed;
	}

	bool wasPressed(void)
	{
		return edgePressed;
	}

	bool wasReleased(void)
	{
		return edgeReleased;
	}

}
