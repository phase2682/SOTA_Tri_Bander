/*
 * File:   main.c
 * Author: Dan Koellen AI6XG
 *
 * Creation started on February 9, 2023, 5:18 PM
 */

/*
*  Project:                      Relay proto                      *
*                                                                             *
*  Description: This file contains C code for a PIC16F688 using xc8 version
*  compiler.
*  This is the interface to select the proper latching relay configuration  
 *  for 40, 20 and 15 meters on the QRPkits Tribander.  the band will be 
 * selected by successive button pushes to step through the bands
 * Two outputs will drive transistors that will be open collector to 
 * ground the appropriate band selecting resistor
 * 
 * Latching relays are being used instead of the monostable relays of the
 * original design.  A pulsed output will drive transistors that will set or 
 * reset the latching relays to the Tribander main board.
 * 
 * RA0, RA1 and RA3 are reserved for ICSP

	In debug mode:
	

	in non- debug 				
				
				

				
				

				

				
*  Versions:                                                                  *
*                                                                             *
*  MAJ.MIN - AUTHOR - CHANGE DESCRIPTION                                      *
*                                                                             *
*   1.0    - 
*
*   2.0    -                                                                  *
******************************************************************************/



#include <xc.h>
#include "configBits.h"
#include <stdio.h>
#include <stdlib.h>

#define band_set	RA5                 //set input to advance band 
#define _15_mtr     RC2                 //15 meters band set output
#define _20_mtr     RC1                 //20 meters band set output

#define R12s        RA2                 //relays 1 and 2 set output
#define R12r        RC0                 //relays 1 and 2 reset output
#define R34s        RC4                 //relays 3 and 4 set output
#define R34r        RC5                 //relays 3 and 4 reset output
#define relay_pulse_dur 3               //relay pulse duration mSecs output
                            
#define timer_reset_value 	9			//reset value for timer0
#define cal_out     RA4                 //cal output F = 1/(2*timer_events) output

//function declaration for clearing flag after duration mSecs
unsigned char 	DurationClear(unsigned char start, unsigned char duration, unsigned char reset, unsigned char events);

unsigned char 	timer_events = 0;			//number of timer0 overflows
unsigned char 	previous_timer_event =0;	//previous timer_events

unsigned char 	band = 0;            	//0->40 mtrs, 1->20 mtrs, 2->15 mtrs
unsigned char 	previous_band = 3;		//forces band <> previous_band at startup
unsigned int 	state = 0;				//debounce state holder

unsigned char   R12s_time = 0;          //time that R12 is set
unsigned char   R12r_time = 0;          //time that R12 is reset
unsigned char   R34s_time = 0;          //time that R34 is set
unsigned char   R34r_time = 0;          //time that R34 is reset



void main(void) {

PORTA = PORTC= 0;						//Clear PortA and C
TRISA = 0b0100000;						//Ports A5 input, A0,1,2,3,4 outputs,
TRISC = 0; 								//All PortC is output
CMCON0 = 0b111;                         //Comparator Off
CMCON1 = 0b10;
ANSEL = 0b00000000;						//All digital 

//Interrupt setup
GIE = 1;								//global interrupt enable
INTF = 0;								//clear the external interrupt flag
INTE = 0;								//disable0/enable1 external interrupt on RA2

//setup Timer0
OPTION_REGbits.PSA = 0;					//prescaler for T0
OPTION_REGbits.PS = 0b001;              //prescaler set to 1:4
T0CS = 0;								//Fosc/4 timer
T0IE = 1;                               //T0 interrupt enabled 

while(1==1){
   
//millisecond tasks
if(timer_events != previous_timer_event)	//test state every tmr0 interrupt
	{
	//debounce  band_set -> increment band for each press
        state = ((state << 1) | (!band_set) | 0xE000);	//must be clear after 12 loops
		if (state == 0xF000)                            //true if band_set clear
			{
			band++;							//increment band to next band
			if (band > 2) band = 0;         //go back to initial band
            
        }
             cal_out ^= 1;                  //toggle cal_out
        
	previous_timer_event = timer_events;

    if (band != previous_band)              //test if the band has changed
    {
        if (band == 0) 					//40 meters
        {
            _15_mtr = 0;                //band set
            _20_mtr = 0;                //band set

            R12r = 1;                   //reset relays 1 and 2
            R34r = 1;                   //reset relays 3 and 4
            R12r_time = R34r_time = timer_events;
                                        //time relay pulse starts

        }								//end 40 meters if

        if (band == 1)					//20 meters	
            {

            _15_mtr = 0;                //band set
            _20_mtr = 1;                //band set

            R12r = 1;                   //reset relays 1 and 2
            R34s = 1;                   //set relays 3 and 4
            R12r_time = R34s_time = timer_events;
                                        //time relay pulse starts

            }                           //end 20 meters if

        if (band  == 2)					//15 meters
            {

            _15_mtr = 1;                //band set
            _20_mtr = 0;                //band set

            R12s = 1;                   //set relays 1 and 2
     
            R12s_time = timer_events;   //time relay pulse starts


            }							//end 15 meters if
        previous_band = band;
        }                               //end previous_band if
    
    }                                   //end timer_events if

//clear relay pulse after relay_pulse_dur duration
 R12r &= DurationClear(R12r_time, relay_pulse_dur, 255, timer_events ) ;
 R34r &= DurationClear(R34r_time, relay_pulse_dur, 255, timer_events ) ;
 R12s &= DurationClear(R12s_time, relay_pulse_dur, 255, timer_events ) ;
 R34s &= DurationClear(R34s_time, relay_pulse_dur, 255, timer_events ) ;
    
}                                   //end while(1==1)
    
    
}                                  //end main(void)

/**********************************************************************************************
		Function Name: 	unsigned char DurationClear(unsigned char start, unsigned char duration, 
 *                                                  unsigned char reset, unsigned char events)
		Return Value:	flag = 0
		Parmeters:		start is when flag was set, duration is mSecs flag to be set for,
 *                      reset is number of timer_events when reset or overflowed (usigned char-->255), 
 *                      events is timer_events count
		Description:	returns 0 after duration mSecs, otherwise 1
	
	************************************************************************************************/
unsigned char DurationClear(unsigned char start, unsigned char duration, unsigned char reset, unsigned char events)
{
unsigned char flag = 1;
unsigned char time_now1 = events;						//current total of timer overflows
unsigned char delta = reset - duration;                 //region where overflow will occur during duration
if (start >= delta & time_now1 >= delta)
                                        ;				//do nothing until time_now1 is overflowed	
if (start >= delta & time_now1 < delta) 				//adapt for overflow reset
	{
	if ((time_now1 - start + reset)>=duration) flag = 0;	//clear flag at the end of duration
	}													//end if
if (start < delta)                                      //no adapting outside of overflow region
	{
	if (time_now1 - start >= duration) flag = 0;			//clear  flag
	}													//end if
return flag;
}														//end function




void __interrupt()
isr(void)
{
		
	if (T0IF)
		{
			timer_events++;						//increment number of T0 overflow
			TMR0 = timer_reset_value;			//reload starting value in T0
			T0IF = 0;							//clear interrupt flag
		}										//end T0IF if
}