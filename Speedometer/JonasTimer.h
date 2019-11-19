#ifndef JonasTimer_H
#define JonasTimer_H


int Timer_preset;
bool Timer_Running = false;
void Timer_OnTick();

void  Timer_CalculatePreset(int frequency) {
	Timer_preset = 65536 - 16000000 / 256 / frequency;
}

void Timer_Stop() {
	noInterrupts();           // disable all interrupts

	TCCR1A = 0;
	TCCR1B = 0;
	Timer_Running = false;
	interrupts();             // enable all interrupts
}

void Timer_Start(int frequency) {
	// initialize timer1 
	noInterrupts();           // disable all interrupts
	TCCR1A = 0;
	TCCR1B = 0;

	// Set timer1_counter to the correct value for our interrupt interval
	Timer_CalculatePreset(frequency);

	TCNT1 = Timer_preset;   // preload timer
	TCCR1B |= (1 << CS12);    // 256 prescaler 
	TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
	Timer_Running = true;
	interrupts();             // enable all interrupts
}


ISR(TIMER1_OVF_vect)        // interrupt service routine 
{
	TCNT1 = Timer_preset;   // preload timer to fire again.
	Timer_OnTick();
}

#endif