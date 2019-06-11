/**
 * @file      UART2.h
 * @brief     Interrupt driven serial I/O on UART0.
 * @details   Runs on LM4F120/TM4C123.
 * Uses UART0 to implement bidirectional data transfer to and from a
 * computer running PuTTy.
 * Interrupts, semaphores and FIFOs  * are used.
 * @version   V1.0
 * @author    Valvano
 * @copyright Copyright 2017 by Jonathan W. Valvano, valvano@mail.utexas.edu,
 * @warning   AS-IS
 * @note      For more information see  http://users.ece.utexas.edu/~valvano/
 * @date      March 9, 2017

 ******************************************************************************/

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2017

 Copyright 2017 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// U0Rx (VCP receive) connected to PA0
// U0Tx (VCP transmit) connected to PA1
/**
 * \brief standard ASCII symbols
 */
#define CR    0x0D
#define LF    0x0A
#define BS    0x08
#define ESC   0x1B
#define SP    0x20
#define DEL   0x7F
#define UP    0x41
#define DOWN  0x42
#define LEFT  0x43
#define RIGHT 0x44

#define BLACK   0
#define RED     1
#define GREEN   2
#define YELLOW  3
#define BLUE    4
#define MAGENTA 5
#define CYAN    6
#define WHITE   7
#define RESET   8


#define MAX_CMD_LEN 50
#define MAX_WORD_LEN 20
#define MAX_CMD_MEM 10


/**
 * @details  Initialize the UART using interrupts
 * @note 115,200 baud rate (assuming 80 MHz clock)<br>
 8 bit word length, no parity bits, one stop bit, FIFOs enabled
 * @param  none
 * @return none
 * @brief  Initialize the UART.
 */
void UART_Init(void);

/**
 * @details  Wait for new serial port input, interrupt driven
 * @param  none
 * @return ASCII code for key typed
 * @brief  UART input.
 */
char UART_InChar(void);


/**
 * @details  Send serial port output, interrupt driven
 * @param  data is an 8-bit ASCII character to be transferred
 * @return none
 * @brief  8-bit serial output.
 */
void UART_OutChar(char data);
void UART_OutChar2(char data);


/**
 * @details  Output String (NULL termination), interrupt driven
 * @param  pt pointer to a NULL-terminated string to be transferred
 * @return none
 * @brief  String serial output.
 */
void UART_OutString(char *pt);


/**
 * @details  Wait for new serial port input, interrupt driven
 * @note UART_InUDec accepts ASCII input in unsigned decimal format<br>
  and converts to a 32-bit unsigned number<br>
  valid range is 0 to 4294967295 (2^32-1)<br>
  If you enter a number above 4294967295, it will return an incorrect value.<br>
  Backspace will remove last digit typed.
 * @param  none
 * @return 32-bit unsigned number
 * @brief  UART input unsigned number.
 */
unsigned long UART_InUDec(void);

/**
 * @details  Output a 32-bit number in unsigned decimal format
 * @param  n 32-bit number to be transferred
 * @note Variable format 1-10 digits with no space before or after
 * @return none
 * @brief  Number serial output.
 */
 void UART_OutUDec(unsigned long n);

/**
 * @details  Output a 32-bit number in unsigned decimal format
 * @param  n 32-bit number to be transferred, 0 to 999
 * @note Fixed format 3 digits with space after
 * @return none
 * @brief  Number serial output.
 */
 void UART_OutUDec3(unsigned long n);


/**
 * @details  Output a 32-bit number in unsigned decimal format
 * @param  n 32-bit number to be transferred, 0 to 99999
 * @note Fixed format 5 digits with space after
 * @return none
 * @brief  Number serial output.
 */
void UART_OutUDec5(unsigned long n);

/**
 * @details  Output a 32-bit number in signed decimal format
 * @param  n 32-bit number to be transferred
 * @note Variable format 1-10 digits with no space before or after
 * @return none
 * @brief  Number serial output.
 */
void UART_OutSDec(long n);

/**
 * @details  Wait for new serial port input, interrupt driven
 * @note UART_InUHex accepts ASCII input in unsigned hex format<br>
  No '$' or '0x' should be entered, just the 1 to 8 hex digits<br>
  It will convert lower case a-f to uppercase A-F<br>
  and converts to a 16 bit unsigned number<br>
  value range is 0 to FFFFFFFF<br>
  If you enter a number above FFFFFFFF, it will return an incorrect value<br>
  Backspace will remove last digit typed.
 * @param  none
 * @return 32-bit unsigned number
 * @brief  UART input unsigned number.
 */
unsigned long UART_InUHex(void);


/**
 * @details  Output a 32-bit number in unsigned hexadecimal format
 * @param  n 32-bit number to be transferred
 * @note Variable format 1-8 digits with no space before or after
 * @return none
 * @brief  Number serial output.
 */
void UART_OutUHex(unsigned long number);


/**
 * @details  Accepts ASCII characters from the serial port
 * @note Accept input until <enter> is typed<br>
 or until max length of the string is reached.<br>
 It echoes each character as it is inputted.<br>
 If a backspace is inputted, the string is modified and the backspace is echoed<br>
 Terminates the string with a null character<br>
 -- Modified by Agustinus Darmawan + Mingjie Qiu --
 * @param  bufPt pointer to empty buffer,
 * @param  max size of buffer
 * @return none
 * @brief  UART input ASCII string.
 */
void UART_InString(char *bufPt, unsigned short max);

//------------UART_InCMD------------
// Accepts ASCII characters from the serial port
//    and adds them to a string until <enter> is typed
//    or until max length of the string is reached.
// It echoes each character as it is inputted.
// If a backspace is inputted, the string is modified
//    and the backspace is echoed
// If an up or down arrow is inputted, it loads previous
//    commands from memory
// terminates the string with a null character
// uses busy-waiting synchronization on RDRF
// Input: pointer to empty buffer
// Output: Null terminated string
// -- Modified by Agustinus Darmawan + Mingjie Qiu --
void UART_InCMD(char *bufPt);

/**
 * @details  Output a 32-bit number in 0.01 fixed-point format
 * @param  number 32-bit signed number to be transferred, -99999 to +99999
 * @note Fixed format, always 7 characters <br>
  12345 to " 123.45"  <br>
  -22100 to "-221.00"<br>
  -102 to "  -1.02" <br>
     31 to "   0.31" <br>
  error     " ***.**" <br>
 * @return none
 * @brief  Fixed-point output.
 */
void UART_Fix2(long number);

void UART_SetColor(char color);
void UART_OutError(char *string);
void UART_OutStringColor(char *string, char color);
