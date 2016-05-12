//
// main-bike-sensor.c
//
// communication with ADXL343 accelerometer over I2C
// ADC conversion for two piezo films
// Serial communication over FTDI cable or HM-10 BLE module
//
// Neil Gershenfeld 11/8/15
// (c) Massachusetts Institute of Technology 2015
//
// Adapted by Caroline Jaffe, 5/11/16
//
// This work may be reproduced, modified, distributed,
// performed, and displayed for any purpose. Copyright is
// retained and must be preserved. The work is provided
// as is; no warranty is provided, and users accept all 
// liability.
//

#include <avr/io.h>
#include <util/delay.h>

#define output(directions,pin) (directions |= pin) // set port direction for output
#define input(directions,pin) (directions &= (~pin)) // set port direction for input
#define set(port,pin) (port |= pin) // set port pin
#define clear(port,pin) (port &= (~pin)) // clear port pin
#define pin_test(pins,pin) (pins & pin) // test for port pin
#define bit_test(byte,bit) (byte & (1 << bit)) // test for bit set
#define bit_delay_time 102 // bit delay for 9600 with overhead
#define bit_delay() _delay_us(bit_delay_time) // RS232 bit delay
#define half_bit_delay() _delay_us(bit_delay_time/2) // RS232 half bit delay
#define char_delay() _delay_ms(10) // char delay

#define serial_port PORTA
#define serial_direction DDRA
#define serial_pin_out (1 << PA3)

#define I2C_slave_address 0x53 // ADXL345 alt address
#define I2C_delay() _delay_us(5)
#define SCL_pin (1 << PB2)
#define SCL_pins PINB
#define SCL_port PORTB
#define SCL_direction DDRB
#define SDA_pin (1 << PA7)
#define SDA_pins PINA
#define SDA_port PORTA
#define SDA_direction DDRA

void SCL_write(char bit) {
   //
   // write SCL bit
   //
   if (bit == 0) {
      output(SCL_direction,SCL_pin);
      clear(SCL_port,SCL_pin);
      }
   else {
      input(SCL_direction,SCL_pin);
      while (pin_test(SCL_pins,SCL_pin) == 0); // check for clock stretching
      }
   }

void SDA_write(char bit) {
   //
   // write SDA bit
   //
   if (bit == 0) {
      output(SDA_direction,SDA_pin);
      clear(SDA_port,SDA_pin);
      }
   else
      input(SDA_direction,SDA_pin);
   }

void I2C_init() {
   //
   // initialize I2C lines
   //
   SDA_write(1);
   SCL_write(1);
   } 

char I2C_master_write_byte(unsigned char byte) {
   //
   // master write I2C byte
   //
   unsigned char bit;
   //
   // loop over bits
   //
   for (bit = 0; bit < 8; ++bit) {
      if ((byte & 0x80) == 0)
         SDA_write(0);
      else
         SDA_write(1);
      SCL_write(1);
      I2C_delay();
      SCL_write(0);
      I2C_delay();
      byte <<= 1;
      }
   //
   // check for ACK
   //
   SDA_write(1);
   SCL_write(1);
   I2C_delay();
   if (pin_test(SDA_pins,SDA_pin) != 0) {
      //
      // no ACK, return 1
      //
      return 1;	
      }
   //
   // yes ACK, return 0
   //
   SCL_write(0);
   I2C_delay();
   return 0;
   }	

char I2C_master_write(unsigned char* data, unsigned char nbytes, unsigned char slave_address) {
   //
   // I2C master write
   //
   unsigned char index,ret,slave_address_write;
   //
   // send start
   //
   SDA_write(0);
   I2C_delay();
   SCL_write(0);	
   I2C_delay();
   //
   // send slave address
   //
   slave_address_write = slave_address << 1;
   if (I2C_master_write_byte(slave_address_write) != 0)
      //
      // no ACK, return 1
      //
      return 1;
   //
   // loop over bytes
   //
   for (index = 0; index < nbytes; ++index) {
      ret = I2C_master_write_byte(data[index]);
      if (ret != 0)
         //
         // no ACK, return 1
         //
         break;
      //
      // yes ACK, continue
      //	
      }
   //
   // send stop
   //
   SCL_write(1);
   I2C_delay();
   SDA_write(1);
   I2C_delay();
   return ret;
   }

void I2C_master_read_byte(unsigned char* data, unsigned char index, unsigned char nbytes) {
   //
   // master read I2C byte
   //
   unsigned char byte,bit;
   SDA_write(1);
   byte = 0;
   //
   // loop over bits
   //
   for (bit = 0; bit < 8; ++bit)  {
      SCL_write(1);
      I2C_delay();
      if (pin_test(SDA_pins,SDA_pin) != 0)
         byte |= (1 << (7-bit));
      SCL_write(0);
      I2C_delay();
      }
   data[index] = byte;
   if (index < (nbytes-1)) {
      //
      // not done, send ACK
      //
      SDA_write(0);
      SCL_write(1);
      I2C_delay();
      SCL_write(0);
      SDA_write(1);
      I2C_delay();
      }
   else {
      //
      // done, send NACK
      //
      SDA_write(1);
      SCL_write(1);
      I2C_delay();
      SCL_write(0);
      I2C_delay();
      }
   }	

char I2C_master_read(unsigned char* data, unsigned char nbytes, unsigned char slave_address) {
   //
   // I2C master read
   //
   unsigned char index,slave_address_read;
   //
   // send start
   //
   SDA_write(0);
   I2C_delay();
   SCL_write(0);	
   I2C_delay();
   //
   // send slave address
   //
   slave_address_read = (slave_address << 1) + 1;
   if (I2C_master_write_byte(slave_address_read) == 1)
      //
      // no ACK, return 1
      //
      return 1;
   //
   // loop over bytes
   //
   for (index = 0; index < nbytes; ++index)
      I2C_master_read_byte(data,index,nbytes);
   //
   // send stop
   //
   SCL_write(1);
   I2C_delay();
   SDA_write(1);
   I2C_delay();
   return 0;
   }	

void put_char(volatile unsigned char *port, unsigned char pin, char txchar) {
   //
   // send character in txchar on port pin
   //    assumes line driver (inverts bits)
   //
   // start bit
   //
   clear(*port,pin);
   bit_delay();
   //
   // unrolled loop to write data bits
   //
   if bit_test(txchar,0)
      set(*port,pin);
   else
      clear(*port,pin);
   bit_delay();
   if bit_test(txchar,1)
      set(*port,pin);
   else
      clear(*port,pin);
   bit_delay();
   if bit_test(txchar,2)
      set(*port,pin);
   else
      clear(*port,pin);
   bit_delay();
   if bit_test(txchar,3)
      set(*port,pin);
   else
      clear(*port,pin);
   bit_delay();
   if bit_test(txchar,4)
      set(*port,pin);
   else
      clear(*port,pin);
   bit_delay();
   if bit_test(txchar,5)
      set(*port,pin);
   else
      clear(*port,pin);
   bit_delay();
   if bit_test(txchar,6)
      set(*port,pin);
   else
      clear(*port,pin);
   bit_delay();
   if bit_test(txchar,7)
      set(*port,pin);
   else
      clear(*port,pin);
   bit_delay();
   //
   // stop bit
   //
   set(*port,pin);
   bit_delay();
   //
   // char delay
   //
   bit_delay();
   }

void put_string(volatile unsigned char *port, unsigned char pin, char *str) {
   //
   // print a null-terminated string
   //
   static int index;
   index = 0;
   do {
      put_char(port, pin, str[index]);
      ++index;
      } while (str[index] != 0);
   }

void setADC1(){
      ADMUX = (0 << REFS1) | (0 << REFS0) // Vcc ref
         | (0 << MUX5) | (0 << MUX4) | (0 << MUX3) | (0 << MUX2) | (0 << MUX1) | (0 << MUX0); // ADC0
}

void setADC2(){
      ADMUX = (0 << REFS1) | (0 << REFS0) // Vcc ref
         | (0 << MUX5) | (0 << MUX4) | (0 << MUX3) | (0 << MUX2) | (0 << MUX1) | (1 << MUX0); // ADC1
}

void read1(){
   setADC1();
    //
    // initiate conversion
    //
    ADCSRA |= (1 << ADSC);
    //
    // wait for completion
    //
    while (ADCSRA & (1 << ADSC))
       ;
}

void read2(){
   setADC2();
    //
    // initiate conversion
    //
    ADCSRA |= (1 << ADSC);
    //
    // wait for completion
    //
    while (ADCSRA & (1 << ADSC))
       ;
}

int main(void) {
   //
   // main
   //
   unsigned char data[6];
   unsigned char ret;
   static char chr;
   //
   // set clock divider to /1
   //
   CLKPR = (1 << CLKPCE);
   CLKPR = (0 << CLKPS3) | (0 << CLKPS2) | (0 << CLKPS1) | (0 << CLKPS0);
   //
   // initialize output pins
   //
   set(serial_port, serial_pin_out);
   output(serial_direction, serial_pin_out);
   // init A/D
   ADCSRA = (1 << ADEN) // enable
      | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // prescaler /128
   //
   // main loop
   //
   I2C_init();
   data[0] = 0x2D; // POWER_CTL register
   data[1] = 8; // turn on measure bit
   ret = I2C_master_write(data,2,I2C_slave_address);
   while (1) {
      /*
      //
      // read device ID
      //
      data[0] = 0x00; // device ID register
      ret = I2C_master_write(data,1,I2C_slave_address);
      ret = I2C_master_read(data,1,I2C_slave_address);
      put_char(&serial_port,serial_pin_out,data[0]);
      */
      //
      // send ADC0 framing
      //
      put_char(&serial_port,serial_pin_out,1);
      put_char(&serial_port,serial_pin_out,2);
      put_char(&serial_port,serial_pin_out,3);
      put_char(&serial_port,serial_pin_out,4);
      read1();
      //
      // send result
      //
      chr = ADCL;
      put_char(&serial_port, serial_pin_out, chr);
      char_delay();
      chr = ADCH;
      put_char(&serial_port, serial_pin_out, chr);
      char_delay();
      //
      // send ADC2 framing
      //
      put_char(&serial_port,serial_pin_out,5);
      put_char(&serial_port,serial_pin_out,6);
      put_char(&serial_port,serial_pin_out,7);
      put_char(&serial_port,serial_pin_out,8);
      read2();
      //
      // send result
      //
      chr = ADCL;
      put_char(&serial_port, serial_pin_out, chr);
      char_delay();
      chr = ADCH;
      put_char(&serial_port, serial_pin_out, chr);
      char_delay();
      //
      // send accelerometer framing
      //
      put_char(&serial_port,serial_pin_out,9);
      put_char(&serial_port,serial_pin_out,10);
      put_char(&serial_port,serial_pin_out,11);
      put_char(&serial_port,serial_pin_out,12);
      //
      // read and send data
      //
      data[0] = 0x32; // X0 register
      ret = I2C_master_write(data,1,I2C_slave_address);
      ret = I2C_master_read(data,6,I2C_slave_address);
      put_char(&serial_port,serial_pin_out,data[0]);
      put_char(&serial_port,serial_pin_out,data[1]);
      put_char(&serial_port,serial_pin_out,data[2]);
      put_char(&serial_port,serial_pin_out,data[3]);
      put_char(&serial_port,serial_pin_out,data[4]);
      put_char(&serial_port,serial_pin_out,data[5]);
      }
   }