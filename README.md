# MSP430-Adafruit-Bluefruit-LE-SPI-Friend

<p>This repo demonstrates how to setup a two-way interface between an MSP430FR5969 Launchpad and the Adafruit Bluefruit Connect app using Adafruit's Bluefruit LE SPI Friend module: 
<p>https://www.adafruit.com/product/2633

<p>The MSP430 configures the BLE module to advertise as "LED Demo". Select UART after connecting to it with the Adafruit app. The app is used to switch the red LED on P4.6 on and off by sending single lower-case characters <b>t</b> or <b>f</b>, respectively. Any other characters generate an input error. The Launchpad will acknowledge a change in LED state by sending a short message to the Bluefruit app. 

<p>Communication between the MSP430 and BLE module is via SPI using eUSCI_B0. The BLE module is configured using the UART-AT command set encoded with the SDEP protocol. This is implemented with a wrapper script, where an acknowledgement from the module is returned automatically. Separate write and read functions send and receive data to/from the app via BLE wireless. Details can be found in the Adafruit documentation linked above.
  
<p>The following Launchpad pins are used:
  <br>P1.0 Green LED
 <br>P1.2 SPI CS for BLE
 <br>P1.6 MOSI to MOSI
 <br>P1.7 MISO to MISO
 <br>P2.2 SCLK for SPI
 <br>P3.4 RST on BLE (optional, use with caution)
 <br>P4.6 Red LED
  
<p><b>Important:</b> The IRQ interrupt pin on the SPI Friend is <b>not</b> used as it is a highly unreliable indicator of the state of the module. Instead, the module is polled using Timer_A sourced to ACLK at 32768 Hz with LFXT referenced to the onboard crystal. The timer idles in LPM3. It is critical to introduce suitable delays between sequential operations in the BLE module. These delays were determined by experimentation.
  
<p> Requires nofloat printf support in Code Composer. 3.3V for the BLE module can be supplied
 by the Launchpad. Modifications will be needed for the other MSP430 Launchpad models, but the code is thoroughly documented to make this transparent. This repo contains the MSP430 firmware only; Adafruit maintains the firmware for their module. 
  
