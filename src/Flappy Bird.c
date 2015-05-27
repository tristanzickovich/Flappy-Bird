/*
 * Flappy_Bird.c
 *
 * Created: 5/23/2015 1:20:26 PM
 *  Author: Tristan
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include "C:\Users\Tristan\Documents\Atmel Studio\6.2\includes\io.c"
#include "C:\Users\Tristan\Documents\Atmel Studio\6.2\includes\bit.h"
#include <stdio.h>
// TimerISR() sets this to 1. C programmer should clear to 0.
volatile unsigned char TimerFlag = 0;
// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks
void TimerOn() {
	// AVR timer/counter controller register TCCR1
	// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s
	TCCR1B = 0x0B;
	// AVR output compare register OCR1A.
	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	OCR1A = 125;
	// AVR timer interrupt mask register
	// bit1: OCIE1A -- enables compare match interrupt
	TIMSK1 = 0x02;
	//Initialize avr counter
	TCNT1=0;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds
	_avr_timer_cntcurr = _avr_timer_M;
	//Enable global interrupts: 0x80: 1000000
	SREG |= 0x80;
}
void TimerOff() {
	// bit3bit1bit0=000: timer off
	TCCR1B = 0x00;
}
void TimerISR() {
	TimerFlag = 1;
}
// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1
	// (every 1 ms per TimerOn settings)
	// Count down to 0 rather than up to TOP (results in a more efficient comparison)
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0) {
		// Call the ISR that the user uses
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}
// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}
unsigned short newb = 0x01;
//--------Find GCD function -------------------------------
unsigned long int findGCD (unsigned long int a,
unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
		b = c;
	}
	return 0;
}
//--------End find GCD function ---------------------------
//--------Task scheduler data structure--------------------
// Struct for Tasks represent a running process in our
// simple real-time operating system.
/*Tasks should have members that include: state, period, a
measurement of elapsed time, and a function pointer.*/
typedef struct _task {
	//Task's current state, period, and the time elapsed
	// since the last tick
	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	//Task tick function
	int (*TickFct)(int);
} task;
//--------End Task scheduler data structure----------------
//--------Shared Variables---------------------------------
unsigned char SM1_output[] = {"     Flappy Bird!     Press A to Play, B for highscore"};
unsigned char message_size = 54;
unsigned char cdarr[] = {0x4F, 0x5B, 0x06, 0x3F};
unsigned char ct_cd = 0, ct_cd_index = 0;
unsigned char highscore1 = 0, highscore2 = 0;
unsigned char score1 = 0, score2 = 0;
unsigned char startgame = 0, playinggame=0, dead=0;
//--------End Shared Variables-----------------------------
//--------User defined FSMs--------------------------------
void handleMessage(){
	unsigned char tmp = SM1_output[0], cindex = 0;
	for(cindex = 1; cindex < message_size; ++cindex){
		SM1_output[cindex-1] = SM1_output[cindex];
	}
	SM1_output[message_size - 1] = tmp;
	unsigned char cpos = 1;
	LCD_ClearScreen();
	//LCD_DisplayString(1, SM1_output);
	for(cindex = 0; cindex < 15; ++cindex){
		LCD_Cursor(cpos);
		LCD_WriteData(SM1_output[cindex]);
		++cpos;
	}
}

enum SM1_States {SM1_display, SM1_hold, SM1_highscore, SM1_hold2, SM1_CountDown, SM1_GamePlay, SM1_newhighscore};
int Menu(int state) {
	unsigned char hsmessage[15] = {"Highscore: "};
	switch(state){
		case SM1_display:
			if(GetBit(PINA, 2)&&GetBit(PINA, 3))
				state = SM1_display;
			else if(GetBit(PINA, 2)&&!GetBit(PINA, 3)){
				state = SM1_hold;
				LCD_ClearScreen();
				LCD_DisplayString(1, hsmessage);
				LCD_Cursor(12);
				if(highscore1>0){
					LCD_WriteData(highscore1 + '0');
					LCD_Cursor(13);
					LCD_WriteData(highscore2 + '0');
				}
				else
					LCD_WriteData(highscore2 + '0');
			}
			else if(!GetBit(PINA, 2)&&GetBit(PINA, 3)){
				state = SM1_CountDown;
				ct_cd = 0;
				ct_cd_index = 0;
				LCD_ClearScreen();
				LCD_DisplayString(1, "Score: ");
				//finish score code here FIXME  or add to gameplay function
			}
			break;
		case SM1_hold:
			if(!GetBit(PINA, 3))
				state = SM1_hold;
			else if(GetBit(PINA, 3)){
				state = SM1_highscore;
			}
			break;
		case SM1_highscore:
			if(GetBit(PINA, 3))
				state = SM1_highscore;
			else if(!GetBit(PINA, 3))
				state = SM1_hold2;
			break;
		case SM1_hold2:
			if(!GetBit(PINA, 3))
				state = SM1_hold2;
			else if(GetBit(PINA, 3))
				state = SM1_display;
			break;
		case SM1_CountDown:
			if(ct_cd < 20)
				state = SM1_CountDown;
			else if (ct_cd > 19){
				playinggame = 1;
				state = SM1_GamePlay;
				PORTB = 0x00;
			}
			break;
		case SM1_GamePlay:
			if(playinggame){
				state = SM1_GamePlay;
			}
			else if(!playinggame){
				ct_cd = 0;
				if(score1>highscore1){
					highscore1 = score1;
					highscore2 = score2;
					state = SM1_newhighscore;
					LCD_ClearScreen();
					LCD_DisplayString(1, "New High Score!");
				}
				else if(score1==highscore1){
					if(score2>highscore2){
						highscore1 = score1;
						highscore2 = score2;
						state = SM1_newhighscore;
						LCD_ClearScreen();
						LCD_DisplayString(1, "New High Score!");
					}
					else
						state = SM1_display;
				}
				else
					state = SM1_display;
			}
			break;
		case SM1_newhighscore:
			if(ct_cd < 20)
				state = SM1_newhighscore;
			else
				state = SM1_display;
			break;
		default:
			state = SM1_display;
			break;
	}
	switch(state){
		case SM1_display:
			handleMessage();
			break;
		case SM1_CountDown:
			if(ct_cd%5==0){
				PORTB = cdarr[ct_cd_index];
				++ct_cd_index;
			}
			++ct_cd;
			break;
		case SM1_newhighscore:
			++ct_cd;
			break;
		default:
			break;
	}
	return state;
}

void transmit_data(unsigned char data, unsigned char data2) { 
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTC = 0x18;
		// set SER = next bit of data to be sent.
		PORTC |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTC |= 0x02;
	}
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTC = 0x18;
		// set SER = next bit of data to be sent.
		PORTC |= ((data2 >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTC |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTC |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTC = 0x00;
}

unsigned char gamestage[] = {0xF9, 0xF3, 0xE7, 0xCF, 0x9F};

enum SM2_States {sm2_wait, sm2_display, sm2_gameover, sm2_scores} state;
int Matrix_Tick(int state) {
	// === Local Variables ===
	static unsigned char column_val = 0x00;  // sets the pattern displayed on columns
	static unsigned char column_sel = 0xFC; // grounds column to display
	static unsigned char column_bird = 0x7F; //displays bird on far left
	static unsigned char column_bird_pattern = 0x08;
	static unsigned long scrollcount = 0, fallcount = 0;
	static unsigned short fallperiod = 2999, fallperiodnew=169, raiseperiod = 499;
	static unsigned char raisebird = 0;
	static unsigned char buttonreadct = 0;
	// === Transitions ===
	switch (state) {
		case sm2_wait:
			if(!playinggame)
				state = sm2_wait;
			else if(playinggame)
				state = sm2_display;
			break;
		case sm2_display: 
			if(!dead)
				state = sm2_display;
			else if(dead){
				state = sm2_gameover;
				LCD_ClearScreen();
				LCD_DisplayString(1, "Game Over!");
				ct_cd = 0;
			}
			break;
		case sm2_gameover:
			if(ct_cd<20)
				state = sm2_gameover;
			else if (ct_cd>19){
				state = sm2_scores;
				ct_cd = 0;
				//display score and highscore.
			}
			break;
		case sm2_scores:
			if(ct_cd<20)
				state = sm2_gameover;
			else if(ct_cd>19){
				playinggame = 0;
				state = sm2_wait;
			}
			break;
		default: 
			state = sm2_wait;
			break;
	}

	// === Actions ===
	switch (state) {
		case sm2_display:
			//displays bird col and position
			if(!GetBit(PINA, 2) && buttonreadct > 99){
				 raisebird = 1;
				 buttonreadct = 0;
			}
			if(raisebird){
				if(column_bird_pattern > 0x01)
					column_bird_pattern = (column_bird_pattern >> 1);
				fallperiod = raiseperiod;
				raisebird = 0;
			}
			else if(fallcount>fallperiod){
				fallperiod = fallperiodnew;
				fallcount = 0;
				if(column_bird_pattern < 0x80)
					column_bird_pattern = (column_bird_pattern << 1);
			}
			transmit_data(column_bird_pattern, column_bird);
			//display for scrolling pipes
			// if far left column was last to display (grounded)
			if(scrollcount > 299){
				scrollcount = 0;
				if (column_sel == 0x7F){
					column_sel = 0xFE; // resets display column to far right column
					column_val = gamestage[rand()%5];
				}
				//set up 2 pipes side by side if first column
				else if (column_sel == 0xFE)
					column_sel = (column_sel << 1);
				//Shift displayed column one to the right
				else
					column_sel = (column_sel << 1) | 0x01;
			}
			transmit_data(column_val, column_sel);
			
			++scrollcount;
			++fallcount;
			++buttonreadct;
			break;
		case sm2_gameover:
			++ct_cd;
			break;
		case sm2_scores:
			++ct_cd;
			break;
		default: break;
	}
	//transmit_data(column_val, column_sel);
	return state;
}

enum SM3_States {sm3_control};
int MoveBird_Tick(int state){
	//add code for controlling bird and keeping score and tracking life
}


int main()
{
	DDRA = 0x03; PORTA = 0xFC;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	// . . . etc
	// Period for the tasks
	LCD_init();
	unsigned long int SMTick1_calc = 200;
	unsigned long int SMTick2_calc = 1;
	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMTick1_calc, SMTick2_calc);
	//Greatest common divisor for all tasks
	// or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;
	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;
	unsigned long int SMTick2_period = SMTick2_calc/GCD;
	//Declare an array of tasks
	static task task1, task2;
	task *tasks[] = { &task1, &task2};
	const unsigned short numTasks =
	sizeof(tasks)/sizeof(task*);
	// Task 1
	task1.state = SM1_display;
	task1.period = SMTick1_period;
	task1.elapsedTime = SMTick1_period;
	task1.TickFct = &Menu;
	// Task 2
	task2.state = sm2_wait;
	task2.period = SMTick2_period;
	task2.elapsedTime = SMTick2_period;
	task2.TickFct = &Matrix_Tick;

	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();
	// Scheduler for-loop iterator
	unsigned short i;
	while(1) {
		// Scheduler code
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime ==
			tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state =
				tasks[i]->TickFct(tasks[i]->state);
				// Reset elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}
	//Error: Program should not exit!
	return 0;
}