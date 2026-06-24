/********************************************************************************
 * cdc_uart.h — CDC-ACM <-> UART2 (FPGA link) bridge.
 *
 * Adapted from the WCH SimulateCDC example with two changes:
 *   1. CDC data moved from EP2 to EP4 (EP2/EP3 belong to the MSC function in
 *      the composite device).
 *   2. The OWNERSHIP_TOGGLE_CHAR byte ('t') in the host->FPGA direction is
 *      consumed and toggles SD ownership instead of being forwarded.
 *
 * UART2 = PA2 (RX) / PA3 (TX), the board's FPGA link. The FPGA currently
 * echoes everything. Default 115200 8N1; CDC SET_LINE_CODING re-baudss it.
 *******************************************************************************/
#ifndef CDC_UART_H_
#define CDC_UART_H_

#include "CH56x_common.h"

/* CDC line coding as last set by the host (defaults from bridge_config.h). */
typedef struct __PACKED
{
    UINT32 dwDTERate;
    UINT8  bCharFormat;
    UINT8  bParityType;
    UINT8  bDataBits;
} CDC_LINE_CODING;

extern CDC_LINE_CODING cdc_line_coding;

/* EP4 DMA buffers (single 1024-byte SS bulk packet each). */
extern __attribute__((aligned(16))) UINT8 endp4Txbuff[1024] __attribute__((section(".DMADATA")));
extern __attribute__((aligned(16))) UINT8 endp4Rxbuff[1024] __attribute__((section(".DMADATA")));

/* Shared with the EP4 callbacks in CH56x_usb30.c. */
extern volatile UINT16 USBByteCount;        /* bytes pending in endp4Rxbuff   */
extern volatile UINT16 USBBufOutPoint;
extern volatile UINT8  UploadPoint4_Busy;   /* EP4 IN transfer in flight      */
extern volatile UINT8  DownloadPoint4_Busy; /* EP4 OUT armed, awaiting host   */

void CDC_Uart_Init(UINT32 baudrate);        /* UART2 + GPIO + RX interrupt    */
void TMR2_TimerInit1(void);                 /* 10us tick for RX flush timeout */
void CDC_Uart_Deal(void);                   /* main-loop pump, both ways      */
void CDC_Variable_Clear(void);

#endif /* CDC_UART_H_ */
