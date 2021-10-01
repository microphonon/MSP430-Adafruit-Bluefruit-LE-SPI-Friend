/*
 Library for communicating with Adafruit SPI Friend via SPI
 using the eUSCI_B module of the MSP430FR5969. Definitions set
 in the main program.
 */

#ifndef BLE_H_
#define BLE_H_

char read[] = {0x10,0x02,0x0A,0x00};
char buffer, ReadBuffer[4], RXBuffer[BYTES], WrapRX[BYTES];
volatile uint8_t Write_flag, Read_flag, Wrap_error;
int16_t i, Rcount=0, Wcount=0;

void ResetBLE(void) //Hardware reset of BLE module; use with caution
{
	 RESET //Reset pin low
	 TA0CCR0 = RESET_TIME; // Hold RST low for 20 ms in LPM3
	 LPM3;
	 RESET_CLEAR //Reset pin high
}

void WriteBLE(char *str1, int16_t count) //Writes data bytes to BLE
{
 	 	for (i=0; i < 4; ++i) //Initialize read buffer
 	 	{
 	 		ReadBuffer[i] = 0xFA;
 	 	}
	 	UCB0CTLW0 &= ~UCSWRST; //Start USCI for SPI
	 	ENABLE_SPI //Pull CS line low
	 	__delay_cycles(CS_DELAY); //Required
            	for (i=0; i < count; ++i)
	 	{
	 		while (!(UCB0IFG & UCTXIFG)); //Check if it is OK to write
	 		UCB0TXBUF = str1[i]; //Load data into transmit buffer
	 		while (!(UCB0IFG & UCRXIFG)); //Wait until complete RX byte is received
	 		buffer = UCB0RXBUF; //Read buffer to clear RX flag. Data is not used
	 	}
	 	RELEASE_SPI //Pull CS line high
	 	/* The BLE module acknowledges a write command with the 4 byte string 0x20,0x01,0x0A,0x00.
	 	Wait ~6 ms in this setup for bytes to arrive on SPI.  */
	 	TA0CCR0 = PACKET_DELAY;
	 	LPM3;
	 	ENABLE_SPI  //Re-assert CS line
	 	__delay_cycles(CS_DELAY); //100 us

	 	for (i=0; i < 4; ++i)
	 	{
	 		while (!(UCB0IFG & UCTXIFG));
	 		UCB0TXBUF = 0xAA; //Load dummy data into transmit buffer
	 		while (!(UCB0IFG & UCRXIFG));
	 		ReadBuffer[i] = UCB0RXBUF;
	 	}
	 	RELEASE_SPI //Pull CS line high
	 	UCB0CTLW0 |= UCSWRST; //Stop USCI module

	 	if ((ReadBuffer[0] == 0x20)&&(ReadBuffer[1] == 0x01)) Write_flag = 0; //Looking for first two bytes
	 	else Write_flag = 1;
	 	if ((ReadBuffer[2] == 0x0A)&&(ReadBuffer[3] == 0x00)) Write_flag = 0; //Looking for last two bytes
	 	else Write_flag = 1;
}

void ReadBLE(void) //Reads BYTES from BLE
{
	 	for (i=0; i < BYTES; i++) //Initialize read buffer
	 	{
	 		RXBuffer[i] = 0x00;
	 	 }
	 	UCB0CTLW0 &= ~UCSWRST; //Start USCI for SPI
	 	ENABLE_SPI  //Pull CS line low
	 	__delay_cycles(CS_DELAY); //Required delay after CS asserted
	 	for (i=0; i < 4; i++)
	 	{
	 		while (!(UCB0IFG & UCTXIFG)); //Check if it is OK to write
	 		UCB0TXBUF = read[i]; //Load 0x10,0x02,0x0A,0x00 into transmit buffer
	 		while (!(UCB0IFG & UCRXIFG)); //Wait until complete RX byte is received
	 		buffer = UCB0RXBUF; //Read buffer to clear RX flag. Data is not used
	 	}
	 	RELEASE_SPI //Pull CS line high
	 	/* The BLE module acknowledges the above read command with the 4 byte string 0x20,0x02,0x0A,[num strings],[bytes]
	 	Wait ~6 ms in this setup for bytes to arrive on SPI.  */
	 	TA0CCR0 = PACKET_DELAY;
	 	LPM3;
	 	ENABLE_SPI  //Re-assert CS line
	 	__delay_cycles(CS_DELAY); //100 us
	 	for (i=0; i < BYTES; i++)
	 	{
	 		while (!(UCB0IFG & UCTXIFG));
	 		UCB0TXBUF = 0xAA; //Load dummy data into transmit buffer
	 		while (!(UCB0IFG & UCRXIFG));
	 		RXBuffer[i] = UCB0RXBUF;
	 		if (i==1) //First two bytes have arrived
	 		{
	 			if ((RXBuffer[0]==0x20)&&(RXBuffer[1]==0x02)) ;
	 			else //Problem on BLE SPI
	 			{
	 			 	Read_flag=1;
	 			 	break;
	 			}
	 		}
	 		else if (i==3) //4th byte indicates size of data string
	 		{
	 			Rcount = (int16_t)RXBuffer[3]; //Number of data packets
	 			if (RXBuffer[3] == 0x00) break;
	 		}
	 		else if (i == (Rcount+3)) break; //Run loop until all bytes read
			else ; //Should never get here
	 	}
	 	RELEASE_SPI //Pull CS line high
	 	UCB0CTLW0 |= UCSWRST; //Stop USCI module
}

void Wrap(char *str1, int16_t count)
/* Writes wrapped message to BLE. Does not need a separate read function. Response comes back on MISO
  after a 4 ms delay.  Payload stored in WrapRX[] */
{
 		for (i=0; i < BYTES; i++) //Initialize read buffer
 	 	{
 	 		WrapRX[i] = 0xAA;
 	 	}
	 	UCB0CTLW0 &= ~UCSWRST; //Start USCI for SPI
	 	ENABLE_SPI  //Pull CS line low
	 	__delay_cycles(CS_DELAY); //Required delay after CS asserted
	 	for (i=0; i < count; i++)
	 	{
	 		while (!(UCB0IFG & UCTXIFG)); //Check if it is OK to write MOSI bytes
	 		UCB0TXBUF = str1[i]; //Load data into transmit buffer
	 		while (!(UCB0IFG & UCRXIFG)); //Wait until complete RX byte is received
	 		buffer = UCB0RXBUF; //Read buffer to clear RX flag. Data is not used
	 	}
	 	RELEASE_SPI //Pull CS line high
	 /* The BLE module acknowledges the above command with the 4 byte string:
	 	0x20,0x00,0x0A,[bytes in payload], [payload].
	 	Wait ~6 ms in this setup for MISO bytes to arrive on SPI.  */
	 	TA0CCR0 = PACKET_DELAY;
	 	LPM3;
	 	ENABLE_SPI  //Re-assert CS line
	 	__delay_cycles(CS_DELAY); //100 us
	 	for (i=0; i < BYTES; i++)
	 	{
	 		while (!(UCB0IFG & UCTXIFG));
	 		UCB0TXBUF = 0xAA; //Load dummy data into transmit buffer
	 		while (!(UCB0IFG & UCRXIFG));
	 		WrapRX[i] = UCB0RXBUF;
	 		if (i == 1) //First two bytes have arrived
	 		{
	 			if ((WrapRX[0] == 0x20)&&(WrapRX[1] == 0x00)) ;
	 			else //Problem on BLE SPI
	 			{
	 			 	Wrap_error = 1;
	 			 	break;
	 			 }
	 		}
	 		else if (i==3) //This byte indicates size of payload
	 		{
	 			Wcount = (int16_t)WrapRX[3]; //Number of data packets
	 			if (WrapRX[3] == 0x00) break; //Empty payload
	 		}
	 		else if (i == (Wcount+3)) break; //Run loop until all bytes read
	 		else ;
	 	}
	 	RELEASE_SPI //Pull CS line high
	 	UCB0CTLW0 |= UCSWRST; //Stop USCI module
}

#endif /* BLE_H_ */
