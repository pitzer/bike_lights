#pragma once

#include <Arduino.h>

constexpr uint8_t pinMomentarySwitch = 22;

// Debounced momentary button API.
// Call setup() once during startup, then call loop() each main loop cycle.
namespace Button {
	// Initializes the button input pin and internal debouncing state.
	void setup(void);
	// Polls the input and updates debounced state/edge flags.
	void loop(void);

	// Returns true while the button is currently pressed.
	bool isPressed(void);
	// Returns true only on the loop iteration where a press is detected.
	bool wasPressed(void);
	// Returns true only on the loop iteration where a release is detected.
	bool wasReleased(void);
}
