#include "OS.h"
#include "wifi.h"
#include "UART.h"
#include "UART5.h"
#include "Interpreter.h"

extern char paramBuffer[IT_MAX_PARAM_N][IT_MAX_CMD_LEN];
char at_buffer[500];

// tcb_t* esp8266;

void WiFi_Init(void) {
	cmd_t *cmd;

	UART5_Init();

	// Add Interpreter Commands
	cmd = IT_AddCommand("wifi", 0, "", &wifi, "print wifi information", 128, 3);
	IT_AddFlag(cmd, 'l', 0, "", &wifi_l, "list available APs", 512, 3);
	IT_AddFlag(cmd, 'c', 2, "[ssid][pass]", &wifi_c, "connect to AP", 512, 3);
	// IT_AddFlag(cmd, 'd', 0, "", &wifi_d, "disconnect from AP", 128, 3);
	// cmd = IT_AddCommand("ping", 0, "", &ping, "", 128, 3);

	// UART5_OutString("AT+RST\r\n");
	// UART5_OutString("AT+RST\r\n");
	// UART5_InResponse(at_buffer, 500);
}

// Interpreter commands
// Print wifi information
void wifi(void) {
	IT_Init();
	UART5_OutString("AT+CIPSTATUS\r\n");
	UART5_InResponse(at_buffer, 500);
	UART_OutString(at_buffer);
	IT_Kill();
}

// List available APs
void wifi_l(void) {
	IT_Init();
	UART_OutString("\n\r  Nearby Access Points:\r\n");

	UART5_OutString("AT+CWLAP\r\n");

	UART5_InResponse(at_buffer, 500);
	UART_OutString(at_buffer);

	UART_OutString("\r\n ");
	IT_Kill();
}

// Connect to AP
void wifi_c(void) {
	IT_Init();
  IT_GetBuffer(paramBuffer);
	UART_OutString("\r\n  Attempting connection to:\r\n");
	UART_OutString("    SSID: ");
	UART_OutString(paramBuffer[0]);
	UART_OutString("\r\n    PASS: ");
	UART_OutString(paramBuffer[1]);
	UART_OutString("\r\n");

	UART5_OutString("AT+CWMODE=1\r\n");
	UART5_InResponse(at_buffer, 500);
	UART_OutString(at_buffer);

	UART5_OutString("AT+CWJAP=\"");
	UART5_OutString(paramBuffer[0]);
	UART5_OutString("\",\"");
	UART5_OutString(paramBuffer[1]);
	UART5_OutString("\"\r\n");

	UART5_InResponse(at_buffer, 500);
	UART_OutString(at_buffer);

	UART5_OutString("AT+CIPSTATUS\r\n");
	UART5_InResponse(at_buffer, 500);
	UART_OutString(at_buffer);


	IT_Kill();
}
