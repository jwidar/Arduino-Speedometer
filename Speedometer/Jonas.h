#ifndef PullupInput_H
#define PullupInput_H

#define SerialDebug(X,Y) \
 ( \
	(Serial.print(X)), (Serial.print(Y)), (Serial.println()), (void)0  \
 )

#define M_2PI 6.28318530717958647693

#define MILLION_F 1000000.0
#define MILLION 1000000

struct PullupInput {
	int Pin;
	bool High;
	PullupInput(int pin) {
		Pin = pin;
		pinMode(pin, INPUT_PULLUP);
		read();
	}

	// Den här metoden gör så att man kan betrakta en instans av klassen som en bool.
	// I stället för if(myInput.High) kan man skriva if(myInput)
	operator bool&() { return High; }

	void read() {
		High = !digitalRead(Pin);
	}
};

class Debouncer {
	bool state;
	bool oldValue;
	long timestamp;
	long timeout;
public:
	Debouncer(long timeoutMicros) {
		timestamp = micros();
		timeout = timeoutMicros;
		state = oldValue = false;
	}

	void operator = (const bool & value) {
		if (value != state) {
			if (value != oldValue)
				timestamp = micros();

			if ((micros() - timestamp) > timeout) {
				state = value;
			}
		}

		oldValue = value;
	}

	operator bool() { return state; }
};

struct PressHoldButton {
	int state;
	bool oldValue;
	long timestamp;
	long timeout;

	static const int E_NONE = 0;
	static 	const int E_PRESSED = 1;
	static const int E_HOLD = 2;

	PressHoldButton(long clickTimeout) {
		timestamp = millis();
		timeout = clickTimeout;
		state = E_NONE;
	}

	void operator = (const bool & value) {

		switch (state)
		{
		case E_NONE:
			if (value != oldValue) {
				if (value) {
					timestamp = millis();
				}
				else {
					state = E_PRESSED;
				}
			}
			else if (value && (millis() - timestamp) > timeout) {
				state = E_HOLD;
			}
			break;

		case E_PRESSED:
			// We get here upon next iteration after the state is set to E_PRESSED, and should not linger here.
			state = E_NONE;
			break;

		case E_HOLD:
			if (!value) {
				state = E_NONE;
			}
			break;
		}

		/*if ((millis() - timestamp) > timeout) {
			state = value;
		}*/

		oldValue = value;
	}

	operator int() { return state; }
};


template <class TVal> struct AnalogInput {
	int Pin;
	TVal Value;
private:
	TVal _low;
	TVal _span;
public:
	AnalogInput(int pin, TVal low, TVal high) {
		Pin = pin;
		_low = low;
		_span = high - low;
	}

	operator TVal&() { return Value; }

	void read() {
		Value = _low + (TVal)((float)analogRead(Pin) / (float)1023 * (float)_span);
	}
};

//struct DigitalInput {
//	int Pin;
//	bool High;
//	DigitalInput(int pin) {
//		Pin = pin;
//		pinMode(pin, INPUT);
//		read();
//	}
//
//	// Den här metoden gör så att man kan betrakta en instans av klassen som en bool.
//	// I stället för if(myInput.High) kan man skriva if(myInput)
//	operator bool&() { return High; }
//
//	void read() {
//		High = digitalRead(Pin);
//	}
//};

struct DigitalOutput {
	int Pin;
	DigitalOutput(int pin, bool initialState) {
		Pin = pin;
		pinMode(pin, OUTPUT);
		*this = initialState;
	}

	// Den här metoden gör så att man kan betrakta en instans av klassen som en bool.
	// I stället för myOutput.set(true); kan man skriva myOutput = true;
	void operator = (const bool & value) {
		digitalWrite(Pin, value ? HIGH : LOW);
	}

	operator bool() { return digitalRead(Pin); }

};

struct PwmOutput {
	int Pin;
	int Value;
	PwmOutput(int pin, int initialValue) {
		Pin = pin;
		pinMode(pin, OUTPUT);
		*this = initialValue;
	}

	// Den här metoden gör så att man kan betrakta en instans av klassen som en bool.
	// I stället för myOutput.set(true); kan man skriva myOutput = true;
	void operator = (const int & value) {
		Value = value;
		analogWrite(Pin, value);
	}

	operator int() { return Value; }

};

#endif