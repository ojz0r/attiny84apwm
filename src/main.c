// PROGRAM: SPI slave
/*********************************************************************
Includes
*********************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>

/*********************************************************************
Defines
*********************************************************************/
/* PASCALIFY */
#define begin {		     		// Pascalify
#define end }			      	// Pascalify
#define then {		    		// Pascalify
#define AND &&		    		// Pascalify
#define OR ||			      	// Pascalify
#define TRUE 1			    	// TRUE = 1
#define FALSE 0				    // FALSE = 0
#define NOT(VAR) (~VAR)		// Invert, 1 := 0

/* MACROS */
#define SET(BANK, PIN)	BANK |= (1 << PIN)	// Set output high
#define RST(BANK, PIN)	BANK &= ~(1 << PIN)	// Set output low
#define DI(BANK, PIN)	BANK & (1 << PIN)	    // Read input pin

/* DEFINE IO NAMES */
#define MOSI PINA6
#define SCL  PINA4
#define CS   PINB0

/*********************************************************************
Global variables
*********************************************************************/
volatile uint16_t receive = 0;		// Receive word
volatile uint16_t value = 0;		  // Copied receive value
volatile uint16_t old_value = 0;	// Previous value
volatile uint8_t bits = 0;		  	// Bits counter for SPI
volatile uint8_t sreg = 0;	  		// Temp byte for SREG copy

/*********************************************************************
SPI PWM slave bit bang interrupts
*********************************************************************/
/* RECIEVE: Interrupt on SCL (PINA4) */
ISR (PCINT0_vect) begin
	if NOT(DI(PINB, CS)) then
		if (bits < 16) then
			receive <<= 1;
			if (DI(PINA, MOSI)) then
				receive |= 1;
			end
			bits++;
		end
	end
end // interrupt

/* Set PWM out value when CS is released (16-bits in bits) */
ISR (PCINT1_vect) begin
	if (DI(PINB, CS)) then
		if (bits >= 16) then
			value = receive;
			receive = 0;
			bits = 0;
		end
	end
end // interrupt

/* Update PWM value on counter overflow, only if there is a new value */
ISR (TIM1_OVF_vect) begin
	if (value != old_value) then
		PWM(value);
		old_value = value;
	end
end // interrupt

/*********************************************************************
Functions
*********************************************************************/
/* Set PWM value, disable/enable interrupts while writing */
void PWM(uint16_t VALUE) begin
	sreg = SREG;
	cli();
	OCR1B = VALUE;
	SREG = sreg;
	sei();
end

/*********************************************************************
ATTINY84A

  -----o-----
--|VCC   GND|--
--|PB0   PA0|--
--|PB1   PA1|--
--|PB3   PA2|--
--|PB2   PA3|--
--|PA7   PA4|--
--|PA6   PA5|--
  -----------

Setup profiles
*********************************************************************/
void clrReg(void) begin
	DDRA   = 0x0;			// Set all low before config
	DDRB   = 0x0;			// Set all low before config
	PORTA  = 0x0;			// Port A all bits low
	PORTB  = 0x0;			// Port B all bits low
end

void spiSetup(void) begin
/* SET I/O DIRECTIONS */
	DDRA  &= ~(1 << MOSI);	// Set PINA6 as input, MOSI
	DDRA  &= ~(1 << SCL);	  // Set PINA4 as input, SCL
	DDRB  &= ~(1 << CS);	  // Set PINB0 as input, CS
	PORTB |= (1 << CS);	  	// Set CS as internal pull-up
end

void pwmSetup(void) begin
	DDRA   |= (1 << PINA5);			        		// Set PINA5 as output
	TCCR1B |= (1 << CS10);			        		// Set clock prescaler
	TCCR1A |= (1 << WGM11);			        		// Set to 'Fast PWM' mode
	TCCR1B |= (1 << WGM13) | (1 << WGM12);	// Set to 'Fast PWM' mode
	TCCR1A |= (1 << COM1B1);		        		// Clear OC1B (PINA5) output on compare match, upwards counting
	ICR1    = 0x3FFF;					            	// Set max counter value (TOP)
	TIMSK1 |= (1 << TOIE1);		        			// Set interrupt on overflow (TOIE1)
end

void interruptSetup(void) begin
	PCMSK0 |= (1 << PCINT4);						              		// Set MCLK (PINA4) as interrupt mask
	PCMSK1 |= (1 << PCINT8);						              		// Set CS (PINB0) as interrupt mask
	GIMSK  |= (1 << INT0) | (1 << PCIE1) | (1 << PCIE0);	// Enable external interrupt (INT0) and (PCIE0)+(PCIE1) detection setting
	MCUCR  |= (1 << ISC01) | (1 << ISC00);		      			// ISC01 + ISC00 = Rising edge interrupt
end

/*********************************************************************
Main: Setup
*********************************************************************/
int main(void) begin
/* SETUP */
	clrReg();		    	// Clear registers, fresh start
	pwmSetup();	  		// PWM and analog input setup
	spiSetup();	  		// SPI pin profile
	interruptSetup();	// Interrupt MCLK
	sei();			    	// Enable interrupts globally
	PWM(0);

/* LOCAL VARIABLES */

/*********************************************************************
Main: Loop
*********************************************************************/
	while(TRUE) begin
	/* Nothing here, everything executes in interrupts */
	end // while loop

/*********************************************************************
End program
*********************************************************************/
	return 0;
end // main
