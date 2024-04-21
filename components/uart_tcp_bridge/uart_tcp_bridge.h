#ifndef _UART_BRIDGE_H_
#define _UART_BRIDGE_H_

#define UART_BRIDGE_PORT     1234
#define UART_BRIDGE_BAUDRATE 74880

void uart_bridge_init();
void uart_bridge_task();
void uart_bridge_close();


#endif