#include "LPC17xx.h"
#include "PIN_LPC17xx.h"
#include "Driver_USART.h"
#include <stdio.h>
#include <string.h>

void UART_write_byte(uint8_t data) {
  while (!(LPC_UART0->LSR & (1 << 5)))
    ;                    // Wait for THR to be empty
  LPC_UART0->THR = data; // Send data
}

void UART_write_string(const char *str) {
  while (*str) {
    UART_write_byte(*str++);
  }
}

uint8_t UART_read_byte(void) {
  while (!(LPC_UART0->LSR & (1 << 0)))
    ;                    // Wait for data
  return LPC_UART0->RBR; // Read data
}

void Init_UART0_reg(void) {
  LPC_SC->PCONP |= (1 << 3); // Power up UART0

  LPC_SC->PCLKSEL0 &= ~(0x03 << 6); // Clear PCLK_UART0
  LPC_SC->PCLKSEL0 |= (0x00 << 6);  // Set PCLK_UART0

  LPC_UART0->LCR = (1 << 7);           // Enable DLAB
  LPC_UART0->DLM = 0x00;               // Set baud rate high
  LPC_UART0->DLL = 0x6C;               // Set baud rate low
  LPC_UART0->FDR = (0x02 << 4) | 0x01; // MULVAL | DIVADDVAL
  LPC_UART0->LCR = 0x03; // 8 bits, no parity, 1 stop bit, disable DLAB
  LPC_UART0->FCR = 0x07; // Enable and reset TX/RX FIFO

  // Clear P0.2 and P0.3 function
  LPC_PINCON->PINSEL0 &= (~(0x03 << 4) & ~(0x03 << 6));
  // Set P0.2, P0.3 to TXD0, RXD0
  LPC_PINCON->PINSEL0 |= ((0x01 << 4) | (0x01 << 6));

  // Set P0.2, P0.3 to pull-up mode
  LPC_PINCON->PINMODE0 &= (~(0x03 << 4) & ~(0x03 << 6)); // optional
  // Set P0.2, P0.3 to normal mode
  LPC_PINCON->PINMODE_OD0 &= (~(0x00 << 2) & ~(0x00 << 3)); // optional

  // PIN_Configure(0, 2, 0x01, 0x00, 0); // Configure P0.2 as TXD0
  // PIN_Configure(0, 3, 0x01, 0x00, 0); // Configure P0.3 as RXD0
}

extern ARM_DRIVER_USART Driver_USART3;
static ARM_DRIVER_USART * USARTdrv = &Driver_USART3;

volatile char rdata;

void myUSART_callback(uint32_t event)
{
  if(event & ARM_USART_EVENT_RECEIVE_COMPLETE) {
	USARTdrv->Send((const char*)&rdata, 1);
	USARTdrv->Send("\r\n", 2);
  }
}

void Init_UART3_lib(void) {
	USARTdrv->Initialize(myUSART_callback);
    /*Power up the USART peripheral */
    USARTdrv->PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 4800 Bits/sec */
    USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
                      ARM_USART_DATA_BITS_8 |
                      ARM_USART_PARITY_NONE |
                      ARM_USART_STOP_BITS_1 |
                      ARM_USART_FLOW_CONTROL_NONE, 9600);
     
    /* Enable Receiver and Transmitter lines */
    USARTdrv->Control (ARM_USART_CONTROL_TX, 1);
    USARTdrv->Control (ARM_USART_CONTROL_RX, 1);
}

int main() {
  
  SystemInit();
  Init_UART0_reg();
  Init_UART3_lib();
  
  char buff[32];
  sprintf(buff, "%d", SystemCoreClock);
	
	const char* tdata = "USART CMSIS Hello\r\n";
	  USARTdrv->Send(tdata, strlen(tdata));
  for (;;) {
	  /*
	  uint8_t byte = UART_read_byte() + 1;
	  UART_write_byte(byte);
	  UART_write_byte(byte);
	  UART_write_byte(byte);
	  UART_write_string("\r\n");
	  */
	  
	  
	  
	  //while(!USARTdrv->GetStatus().rx_busy)
		  //UART_write_byte('B');
	  
	  USARTdrv->Receive((void*)&rdata, 1);
  }
}
