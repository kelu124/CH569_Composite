/********************************************************************************
 * Main.c — CH569 USB3 bridge: SD card (USB Mass Storage) + FPGA UART (CDC-ACM)
 *
 * Composite USB 3.0 device for the USB3_SSP board:
 *   - The microSD card (behind the FPGA-controlled MUX) is exposed to the USB
 *     host as a removable drive, served as raw sectors over MSC/BOT. The host
 *     owns the FAT32 filesystem; this firmware is filesystem-agnostic.
 *   - UART2 (PA2/PA3, the FPGA link) is exposed as a CDC-ACM virtual COM port.
 *
 * SD ownership: PB10 high asks the FPGA to release the card; low gives it
 * back (see ownership.c). Toggle via the 't' byte on the CDC port or the
 * vendor control request (bridge_config.h). At boot the FPGA owns the card;
 * the host sees "no medium" until ownership is claimed.
 *
 * On a USB2-only link the device falls back to MSC-only (no COM port) — the
 * vendor USB2 stack has no CDC function; see README "USB2 fallback".
 *
 * Derived from the WCH CH569 EVT examples (MSC_U-Disk, SimulateCDC, SD).
 *******************************************************************************/
#include "CH56x_common.h"
#include "CH56x_usb30.h"
#include "CH56x_usb20.h"
#include "SW_UDISK.h"
#include "cdc_uart.h"
#include "ownership.h"
#include "bridge_config.h"
#if DEBUG_OVER_USB
#include "debug_cdc.h"
#endif

#if STATUS_LEDS
/* Drive a status LED to reflect a boolean state, honouring the board's
 * active level (bridge_config.h). */
static void led_set(UINT32 pin, UINT8 on)
{
#if STATUS_LED_ACTIVE_HIGH
    if(on) GPIOB_SetBits(pin); else GPIOB_ResetBits(pin);
#else
    if(on) GPIOB_ResetBits(pin); else GPIOB_SetBits(pin);
#endif
}

static void status_leds_init(void)
{
    GPIOB_ModeCfg(LED_OWN_PIN | LED_READY_PIN | LED_LINK_PIN, GPIO_Slowascent_PP_8mA);
    led_set(LED_OWN_PIN, 0);
    led_set(LED_READY_PIN, 0);
    led_set(LED_LINK_PIN, 0);
}

static void status_leds_update(void)
{
    led_set(LED_OWN_PIN,   own_ch569_owns);
    led_set(LED_READY_PIN, own_card_ready);
    led_set(LED_LINK_PIN,  Link_Sta == LINK_STA_3);   /* USB3 SuperSpeed up */
}
#endif

/*******************************************************************************
 * @fn        DebugInit
 *
 * @brief     UART1 (PA7=RX/PA8=TX) debug log output.
 ******************************************************************************/
void DebugInit(UINT32 baudrate)
{
    UINT32 x;
    UINT32 t = FREQ_SYS;

    x = 10 * t * 2 / 16 / baudrate;
    x = (x + 5) / 10;
    R8_UART1_DIV = 1;
    R16_UART1_DL = x;
    R8_UART1_FCR = RB_FCR_FIFO_TRIG | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;
    R8_UART1_LCR = RB_LCR_WORD_SZ;
    R8_UART1_IER = RB_IER_TXD_EN;
    R32_PA_SMT |= (1 << 8) | (1 << 7);
    R32_PA_DIR |= (1 << 8);
}

/*********************************************************************
 * @fn      main
 *********************************************************************/
int main(void)
{
    SystemInit(FREQ_SYS);
    Delay_Init(FREQ_SYS);

    DebugInit(DEBUG_UART_BAUD);
    PRINT("\nCH569 SD/UART USB3 bridge (%d MHz)\n", FREQ_SYS / 1000000);

    /* SD ownership line: FPGA owns the card at reset (PB10 low). */
    ownership_init();

#if STATUS_LEDS
    status_leds_init();         /* PB22/23/24 LEDs: own / ready / USB3 link */
#endif

    /* FPGA UART link (CDC bridge) + RX flush timer */
    CDC_Uart_Init(FPGA_UART_DEFAULT_BAUD);
    TMR2_TimerInit1();

    /* The disk starts "not present": no EMMC access happens until a claim. */
    Udisk_Status &= ~DEF_UDISK_EN_FLAG;

    /* USB: try SuperSpeed first; TMR0 times out into the USB2 fallback. */
    R32_USB_CONTROL = 0;
    PFIC_EnableIRQ(USBSS_IRQn);
    PFIC_EnableIRQ(LINK_IRQn);
    PFIC_EnableIRQ(TMR0_IRQn);
    R8_TMR0_INTER_EN = RB_TMR_IE_CYC_END;
    TMR0_TimerInit(67000000);
    USB30D_init(ENABLE);
    PRINT("USB init done\n");

#if CLAIM_ON_BOOT
    ownership_request(OWN_REQ_CLAIM);
#endif

    while(1)
    {
        UDISK_onePack_Deal();   /* MSC multi-sector transfers (EMMC <-> USB) */
        CDC_Uart_Deal();        /* CDC <-> UART2 pump + 't' ownership toggle */
        ownership_poll();       /* executes pending PB10 claim/release       */
#if DEBUG_OVER_USB
        debug_cdc_poll();       /* stream debug log out the 2nd CDC port     */
#endif
#if STATUS_LEDS
        status_leds_update();   /* refresh PB22/23/24 status LEDs            */
#endif
    }
}
