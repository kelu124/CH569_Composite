/********************************************************************************
 * cdc_uart.c — CDC-ACM <-> UART2 (FPGA link) bridge on endpoint 4.
 * Adapted from WCH SimulateCDC/User/CDC/cdc.c; see cdc_uart.h for the deltas.
 *******************************************************************************/
#include "CH56x_common.h"
#include "CH56xusb30_LIB.h"
#include "CH56x_usb30.h"
#include "cdc_uart.h"
#include "bridge_config.h"
#include "ownership.h"

/* Global define */
#define UART_REV_LEN  1024                  /* uart receive ring size         */
#define UART_TIMEOUT  1000                  /* x10us: flush partial RX data   */

/* Global Variable */
__attribute__((aligned(16))) UINT8 endp4Txbuff[1024] __attribute__((section(".DMADATA")));
__attribute__((aligned(16))) UINT8 endp4Rxbuff[1024] __attribute__((section(".DMADATA")));
__attribute__((aligned(16))) static UINT8 Receive_Uart_Buf[UART_REV_LEN] __attribute__((section(".DMADATA")));

CDC_LINE_CODING cdc_line_coding = { FPGA_UART_DEFAULT_BAUD, 0, 0, 8 };

volatile UINT16 Uart_Input_Point = 0;       /* ring write pointer             */
volatile UINT16 Uart_Output_Point = 0;      /* ring read pointer              */
volatile UINT16 UartByteCount = 0;          /* bytes pending in the ring      */
volatile UINT16 USBByteCount = 0;           /* bytes received from USB EP4    */
volatile UINT16 USBBufOutPoint = 0;
volatile UINT8  UploadPoint4_Busy = 0;
volatile UINT8  DownloadPoint4_Busy = 0;
volatile UINT16 Uart_Timecount = 0;
volatile UINT16 Uart_Sendlenth = 0;

/* Function declaration.
 * Plain __attribute__((interrupt)): upstream GCC ignores MounRiver's
 * "WCH-Interrupt-fast" variant, which would leave these as ordinary
 * functions and crash on the first interrupt. */
void TMR2_IRQHandler(void) __attribute__((interrupt));
void UART2_IRQHandler(void) __attribute__((interrupt));

/*******************************************************************************
 * @fn        CDC_Uart_Init
 *
 * @brief     UART2 (FPGA link, PA2=RX / PA3=TX) init. Re-entered on every CDC
 *            SET_LINE_CODING with the host's baud rate.
 ******************************************************************************/
void CDC_Uart_Init(UINT32 baudrate)
{
    UINT32 x;
    UINT32 t = FREQ_SYS;

    x = 10 * t * 2 / 16 / baudrate;
    x = (x + 5) / 10;
    R8_UART2_DIV = 1;
    R16_UART2_DL = x;
    R8_UART2_FCR = RB_FCR_FIFO_TRIG | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;
    R8_UART2_LCR = RB_LCR_WORD_SZ;          /* 8N1 */
    R8_UART2_IER = RB_IER_TXD_EN;

    GPIOA_SetBits(GPIO_Pin_3);
    GPIOA_ModeCfg(GPIO_Pin_2, GPIO_ModeIN_PU_NSMT);
    GPIOA_ModeCfg(GPIO_Pin_3, GPIO_Slowascent_PP_8mA);
    UART2_ByteTrigCfg(UART_7BYTE_TRIG);
    UART2_INTCfg(ENABLE, RB_IER_RECV_RDY | RB_IER_LINE_STAT);
    PFIC_EnableIRQ(UART2_IRQn);

    cdc_line_coding.dwDTERate = baudrate;
}

/*******************************************************************************
 * @fn        TMR2_TimerInit1
 *
 * @brief     10us tick used to flush partially-filled UART RX data to USB.
 ******************************************************************************/
void TMR2_TimerInit1(void)
{
    R32_TMR2_CNT_END = FREQ_SYS / 100000;
    R8_TMR2_CTRL_MOD = RB_TMR_ALL_CLEAR;
    R8_TMR2_CTRL_MOD = RB_TMR_COUNT_EN;
    R8_TMR2_INTER_EN |= 0x01;
    PFIC_EnableIRQ(TMR2_IRQn);
}

void TMR2_IRQHandler(void)
{
    if(R8_TMR2_INT_FLAG & 0x01)
    {
        R8_TMR2_INT_FLAG = 0x01;
        Uart_Timecount++;
    }
}

/*******************************************************************************
 * @fn        U30_CDC_UartRx_Deal
 *
 * @brief     UART(FPGA) -> USB host: push ring-buffer bytes out of EP4 IN.
 ******************************************************************************/
static void U30_CDC_UartRx_Deal(void)
{
    if(!UploadPoint4_Busy)
    {
        Uart_Sendlenth = UartByteCount;
        if(Uart_Sendlenth > 0)
        {
            if((Uart_Sendlenth >= (UART_REV_LEN / 2) && DownloadPoint4_Busy == 0) || Uart_Timecount > UART_TIMEOUT)
            {
                if(Uart_Output_Point + Uart_Sendlenth > UART_REV_LEN)
                    Uart_Sendlenth = UART_REV_LEN - Uart_Output_Point;

                if(Uart_Sendlenth > (UART_REV_LEN / 2))
                    Uart_Sendlenth = (UART_REV_LEN / 2);

                UartByteCount -= Uart_Sendlenth;
                memcpy(endp4Txbuff, &Receive_Uart_Buf[Uart_Output_Point], Uart_Sendlenth);
                Uart_Output_Point += Uart_Sendlenth;
                if(Uart_Output_Point >= UART_REV_LEN)
                    Uart_Output_Point = 0;

                UploadPoint4_Busy = 1;
                USB30_IN_ClearIT(ENDP_4);
                USB30_IN_Set(ENDP_4, ENABLE, ACK, 1, Uart_Sendlenth);
                USB30_Send_ERDY(ENDP_4 | IN, 1);

                Uart_Timecount = 0;
            }
        }
    }
}

/*******************************************************************************
 * @fn        U30_CDC_UartTx_Deal
 *
 * @brief     USB host -> UART(FPGA): drain EP4 OUT data into UART2, one byte
 *            per pass. OWNERSHIP_TOGGLE_CHAR is consumed and toggles PB10
 *            instead of being forwarded (client-requested bring-up control).
 ******************************************************************************/
static void U30_CDC_UartTx_Deal(void)
{
    if(USBByteCount)
    {
        UINT8 ch = endp4Rxbuff[USBBufOutPoint++];
        USBByteCount--;
        UART2_SendString(&ch, 1);          /* forward all bytes; no magic 't' */
    }

    if(DownloadPoint4_Busy == 0)
    {
        if(USBByteCount == 0 && UploadPoint4_Busy == 0)
        {
            USBBufOutPoint = 0;
            DownloadPoint4_Busy = 1;
            USBSS->UEP4_RX_DMA = (UINT32)(UINT8 *)endp4Rxbuff;
            USB30_OUT_ClearIT(ENDP_4);
            USB30_OUT_Set(ENDP_4, ACK, 1);
            USB30_Send_ERDY(ENDP_4 | OUT, 1);
        }
    }
}

/*******************************************************************************
 * @fn        CDC_Uart_Deal
 *
 * @brief     Main-loop pump. Only active on the USB3 link: the USB2 fallback
 *            is MSC-only (no CDC function), see README.
 ******************************************************************************/
void CDC_Uart_Deal(void)
{
    if(Link_Sta != LINK_STA_1)
    {
        U30_CDC_UartTx_Deal();
        U30_CDC_UartRx_Deal();
    }
}

void CDC_Variable_Clear(void)
{
    Uart_Input_Point = 0;
    Uart_Output_Point = 0;
    UartByteCount = 0;
    USBByteCount = 0;
    USBBufOutPoint = 0;
    UploadPoint4_Busy = 0;
    DownloadPoint4_Busy = 0;
    Uart_Timecount = 0;
    Uart_Sendlenth = 0;
}

/*******************************************************************************
 * @fn        UART2_IRQHandler
 *
 * @brief     FPGA-link RX into the ring buffer. Interrupt-driven, so bytes
 *            keep arriving safely even while an MSC bulk transfer is running
 *            with the USB interrupt masked.
 ******************************************************************************/
void UART2_IRQHandler(void)
{
    UINT8 i, rec_length;
    UINT8 rec[8] = {0};

    switch(UART2_GetITFlag())
    {
        case UART_II_LINE_STAT:
            PRINT("uart2 line error:%x\n", R8_UART2_LSR);
            break;

        case UART_II_RECV_RDY:              /* FIFO reached trigger level */
            for(rec_length = 0; rec_length < 7; rec_length++)
            {
                Receive_Uart_Buf[Uart_Input_Point++] = UART2_RecvByte();
                if(Uart_Input_Point >= UART_REV_LEN)
                    Uart_Input_Point = 0;
            }
            UartByteCount += rec_length;
            Uart_Timecount = 0;
            break;

        case UART_II_RECV_TOUT:             /* partial data, RX timeout */
            rec_length = UART2_RecvString(rec);
            for(i = 0; i < rec_length; i++)
            {
                Receive_Uart_Buf[Uart_Input_Point++] = rec[i];
                if(Uart_Input_Point >= UART_REV_LEN)
                    Uart_Input_Point = 0;
            }
            UartByteCount += i;
            Uart_Timecount = 0;
            break;

        case UART_II_THR_EMPTY:
            break;

        default:
            break;
    }
}
