#include <msp430.h> 
#include <stdio.h>
#include <stdint.h>

/* Demonstrates two-way BLE communication between MSP430FR5969 Launchpad
 and the Adafruit Bluefruit Connect app using their Bluefruit SPI Friend module.
 The module advertises as "LED Demo". Select UART after connecting.
 Switch the red LED on P4.6 of MSP430FR5969 Launchpad on and off
 by sending single characters t or f. Any other characters generate
 an input error. MSP430 will acknowledge change in LED state by sending
 a short message to the Bluefruit app. SPI on eUSCI_B0. Timer_A sourced
 by ACLK using LFXT with crystal. 3.3V for the BLE module can be supplied
 by Launchpad.  Requires nofloat printf support in CCS.

 Launchpad pins are as follows:
 P1.0 Green LED
 P1.2 SPI CS for BLE
 P1.6 MOSI to MOSI on Bluefruit LE SPI friend.
 P1.7 MISO to MISO on Bluefruit LE SPI friend.
 P2.2 SCLK for SPI
 P3.4 RST on BLE (optional)
 P4.6 Red LED

 The IRQ line of the Adafruit SPI Friend is not used!

The following definitions are for the BLE.h library */
# define LP_DELAY 164 //5 ms with 32.768 kHz ACLK
# define PACKET_DELAY 197 //6 ms with ACLK
# define CS_DELAY 100 //Delay after asserting CS on BLE (100 us with 1 MHz MCLK)
# define RESET_TIME 655 //20 ms with ACLK
# define RESET P3OUT &= ~BIT4;
# define RESET_CLEAR P3OUT |= BIT4;
# define ENABLE_SPI  P1OUT &= ~BIT2;
# define RELEASE_SPI  P1OUT |= BIT2;
# define BYTES 20
#include "BLE.h"

//Allocate memory for messages
char  BLE_status[BYTES], BAUD_1[BYTES], BAUD_2[BYTES], NAME_1[BYTES], NAME_2[BYTES], led[BYTES];
char  ON[BYTES], OFF[BYTES], R_ERROR[BYTES], T_ERROR[BYTES], N_ERROR[BYTES];

void SetTimer(void);
void SetClock(void);
void SetPins(void);
void SetSPI(void);
void Messages(void);

const uint16_t period = 32768; // Polling period = 1 second

void main(void) {

        WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer
        SetPins();
        SetClock();
        SetTimer();
        SetSPI();
        Messages();
        _BIS_SR(GIE); //Enable global interrupts.

       //Set the baud rate of BLE module
       TA0CCR0 = LP_DELAY;
       LPM3;
       Wrap(BAUD_1, sizeof BAUD_1);
       TA0CCR0 = LP_DELAY;
       LPM3;
       Wrap(BAUD_2, sizeof BAUD_2);

       //Set the BLE module name to "LED Demo"
       TA0CCR0 = LP_DELAY;
       LPM3;
       Wrap(NAME_1, sizeof NAME_1);
       TA0CCR0 = LP_DELAY;
       LPM3;
       Wrap(NAME_2, sizeof NAME_2);

       // Option to turn on/off the MODE LED on Adafruit board
        TA0CCR0 = LP_DELAY;
        LPM3;
        Wrap(led, sizeof led);

    //   TA0CCR0 = LP_DELAY;
    //   LPM3;
    //   ResetBLE(); //Hardware reset the BLE module

    //Main polling loop follows
    while(1)
    {
        Wrap_error = 0;
        //Check BLE connection status using AT wrapper syntax
        Wrap(BLE_status,sizeof BLE_status);
        /* User enters a control character during the following long delay.  Must temporally separate
         the BLE connection query from data entry because the input characters would be lost on MISO.  */
         TA0CCR0 = period; // Polling period of main loop
         LPM3;      //Wait in low power mode
         //Timeout. Turn on green LED. Check if BLE connected.
         P1OUT |= BIT0;
         if(Wrap_error==1) ; //Garbage response to connection query; assume app not connected. Poll again
         else if (WrapRX[3] != 0x07) ; //Payload should be 7 bytes; assume app not connected. Poll again
         else if (WrapRX[4] == '0') ;//Good response from BLE, but app not connected. Poll again
         else if(WrapRX[4] == '1')  //BLE connected to app; check for single character input
         {
             Read_flag = 0; //Reset BLE SPI error flag
             //Check to see if there is data from phone app on BLE SPI
             ReadBLE();
             TA0CCR0 = LP_DELAY;
             LPM3;
             if((Read_flag==1)||(RXBuffer[3]==0)) ; //Nothing to read
             else if (RXBuffer[3]!=2)
                 {
                     WriteBLE(R_ERROR,sizeof R_ERROR); //RX Error text string
                 }
             else if (RXBuffer[4]!='t' && RXBuffer[4]!='f') //Looking for t or f only
                {
                    WriteBLE(N_ERROR,sizeof N_ERROR); //RX Error text string
                }
             else if (RXBuffer[4]=='t') //Turn on red LED
                 {
                    P4OUT |= BIT6;
                    WriteBLE(ON,sizeof ON);
                 }
             else if (RXBuffer[4]=='f') //Turn off red LED
                 {
                    P4OUT &= ~BIT6;
                    WriteBLE(OFF,sizeof OFF);
                 }
             else ; //Should not get here. Iterate the loop
         }
         else ;
         P1OUT &= ~BIT0; //Polling loop done. Turn off green LED
    }
}

#pragma vector=TIMER0_A0_VECTOR
 __interrupt void TIMERA (void) //The name of this interrupt is arbitrary
{
    LPM3_EXIT;
}

void SetPins(void)
  {
   /*   P1.0 Green LED
        P1.2 CS for BLE
        P1.6 MOSI connects to MOSI on Bluefruit LE SPI friend.
        P1.7 MISO connects to MISO on Bluefruit LE SPI friend.
        P2.2 SPI clock
        P3.4 RST on BLE
        P4.6 Red LED */
        PM5CTL0 &= ~LOCKLPM5; //Unlocks GPIO pins at power-up
        P1DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5;
        P1SEL1 |= BIT6 + BIT7; //MOSI and MISO
        P1OUT &= ~BIT0; //Green LED off
        RELEASE_SPI //Make sure BLE module not enabled at start
        P2DIR |= BIT0 + BIT1 + BIT3 + BIT4 + BIT5 + BIT6 + BIT7;
        P2SEL1 |= BIT2; //SPI Clock
        P3DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5 + BIT6 + BIT7;
        P4DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5 + BIT6 + BIT7;
        P4OUT &= ~BIT6; //Red LED off
        PJSEL0 = BIT4 | BIT5; // XT1 for LFXT
  }

 void SetClock(void) //Set ACLK to LFXT at 32768 Hz; uses crystal
 {
        CSCTL0 = CSKEY; //Password to unlock the clock registers
        CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;
        CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1; //Set frequency dividers to 1
}

 void SetTimer(void)
 {
     //Enable the timer interrupt, MC_1 to count up to TA0CCR0; Timer A set to ACLK
     TA0CCTL0 = CCIE;
     TA0CTL |= MC_1 + TASSEL_1;
 }

void SetSPI(void)
   {
     // Configure the eUSCI_B0 module for 3-pin SPI at 1 MHz
     UCB0CTLW0 |= UCSWRST;
     // Use SMCLK at 1 MHz;
     UCB0CTLW0 |= UCSSEL__SMCLK + UCMODE_0 + UCMST + UCSYNC + UCMSB + UCCKPH;
     //UCB0BR0 |= 0x02; //Divide SMCLK by 2 to clock at 500 kHz
     UCB0BR0 |= 0x01; //1 MHz SPI clock
     UCB0BR1 |= 0x00;
     UCB0CTLW0 &= ~UCSWRST;
   }

void Messages(void)
    {
        //Following string is used to check if BLE connected to phone; formatted with SDEP wrapper
        sprintf(BLE_status,"%c%c%c%c%s",0x10,0x00,0x0A,0x0D,"AT+GAPGETCONN");

        sprintf(led,"%c%c%c%c%s",0x10,0x00,0x0A,0x0E,"AT+HWMODELED=0"); //Default is 1

        //Following two string are used to adjust UART baud rate; formatted with SDEP wrapper
        sprintf(BAUD_1,"%c%c%c%c%s",0x10,0x00,0x0A,0x8C,"AT+BAUDRATE="); //Bit 7 of payload size is also set
         //sprintf(BAUD_2,"%c%c%c%c%s",0x10,0x00,0x0A,0x04,"9600");
        // sprintf(BAUD_2,"%c%c%c%c%s",0x10,0x00,0x0A,0x04,"2400");
        sprintf(BAUD_2,"%c%c%c%c%s",0x10,0x00,0x0A,0x06,"115200");

        //Change name of BLE module; SDEP wrapper
        sprintf(NAME_1,"%c%c%c%c%s",0x10,0x00,0x0A,0x8E,"AT+GAPDEVNAME="); //Bit 7 of payload size is also set
        sprintf(NAME_2,"%c%c%c%c%s",0x10,0x00,0x0A,0x08,"LED Demo");

        //Messages sent to app
        sprintf(ON,"%c%c%c%c%s",0x10,0x01,0x0A,0x07,"LED on\n");
        sprintf(OFF,"%c%c%c%c%s",0x10,0x01,0x0A,0x08,"LED off\n");
        sprintf(R_ERROR,"%c%c%c%c%s",0x10,0x01,0x0A,0x0C,"Input Error\n");
        sprintf(N_ERROR,"%c%c%c%c%s",0x10,0x01,0x0A,0x0D,"Enter t or f\n");
        sprintf(T_ERROR,"%c%c%c%c%s",0x10,0x01,0x0A,0x09,"TX Error\n");
    }
