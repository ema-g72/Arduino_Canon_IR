/*
 * Generates the Infra Red pulses to remote control a Canon camera (RC-6).
 * Runs on ATtiny85 microcontroller:
 *
 * 1) Connect an IR LED to pin 6 (PB1).
 * 2) Connect a switch open-GND to pin 2 (PB3) for the shooting mode selection (immediate or delayed).
 * 3) Connect a notification LED to pin 3 (PB4).
 *
 * Note: The Arduino function "digitalWrite" is too slow and "delayMicroseconds" does not give the expected values
 * when the processor runs at 1 MHz.
 * 
 * 13/01/2023 - First release
 */

// This macro sets the CPU frequency and shall be defined before util/delay.h.
#define F_CPU 1000000
#include <avr/sleep.h>
#include <avr/io.h>
#include <util/delay.h>

// Infrared LED
#define IR_TX_LED PB1 // PB4 on Pin 6 of Attiny85
#define MODE_SELECTOR PB3 // PB3 on Pin 2 of Attiny85
#define NOTIFICATION_LED PB4 // PB4 on Pin3 of Attiny85

// Delay between two burst of pulses for the instant shot.
#define INSTANT_SHOT_MSEC 7.330
// Delay between two burst of pulses for the delayed (2 secs) shot.
#define DELAYED_SHOT_MSEC 5.360
// Duration of a pulse in a burst: 16 usec.
#define PULSE_DURATION_USEC 16

// Global pulse counter.
volatile byte gPulseCounter;

void setup() 
{   
   // Set IR led pin (OC1A) as output.
   DDRB |= _BV(IR_TX_LED);
   // Set notification LED pin as output.
   DDRB |= _BV(NOTIFICATION_LED);

   // Enable pull-up for input pin.
   PORTB |= _BV(MODE_SELECTOR);
   
   // Clear registers.
   TCNT1 = 0;
   TCCR1 = 0;
   
   // Reset the prescaler of Timer1
   GTCCR |= _BV(PSR1);
   // Clear timer on compare match: timer is reset to 0 in a cycle after a match with OCR1C.   
   TCCR1 |= _BV(CTC1); 
   // Set the compare registers, the output frequency is (CLK/PRESCALER)/OCR1A/2
   OCR1C = PULSE_DURATION_USEC-1;
   OCR1A = OCR1C;

   // Timer/Counter interrupt mask: interrupt enabled in compare match A.
   TIMSK = _BV(OCIE1A);
   // Enable interrupts.
   sei();

   // Reduce power consumpition disabling the ADC and Analog comparator.
   ADCSRA &= ~_BV(ADEN);
   ACSR &= ~_BV(ACD);
}

ISR(TIMER1_COMPA_vect)
{
   gPulseCounter++;
   if( gPulseCounter == 14 )
   {
      // Stop timer.
      TCCR1 &= ~_BV(CS10); 
      TCCR1 &= ~_BV(COM1A0); 
      PORTB &= ~_BV(IR_TX_LED);   
      TCNT1 = 0;
      gPulseCounter = 0;  
   }
}

void start_pulse_train()
{
   // Comparator A output mode: toggle the OC1A output line when timer match compare register A. 
   TCCR1 |= _BV(COM1A0); 
   // Start timer, lock select bits: prescaler set to CK/1
   TCCR1 |= (1 << CS10);   
}

void flash_led()
{
  PORTB |= _BV(NOTIFICATION_LED);   
  _delay_ms(30);
  PORTB &= ~_BV(NOTIFICATION_LED);     
}

void loop() 
{
   // Read digital input pin for mode selection.
   byte mode = (PINB & _BV(MODE_SELECTOR));

   // Start first IR burst.
   start_pulse_train();

   // Wait according to shot mode.
   if( mode )
   {
      _delay_ms(INSTANT_SHOT_MSEC);
   }
   else
   {
      _delay_ms(DELAYED_SHOT_MSEC);   
   }

   // Start second IR burst.
   start_pulse_train();
   
   // Flash the notification LED.
   flash_led();
   
   set_sleep_mode(SLEEP_MODE_PWR_DOWN);
   sleep_mode();
}
