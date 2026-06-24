/********************************************************************************
 * bridge_config.h — single place for every board/behaviour tunable of the
 * CH569 SD/UART USB3 bridge.
 *
 * Board facts (from USB3_SSP.kicad_sch):
 *   - SD card    : SDIO 4-bit on the CH569's fixed EMMC pins
 *                  (CLK=PB14, CMD=PB16, DAT0..3=PB17..PB20), behind the
 *                  FPGA-controlled MUX.
 *   - PB10       : SD-ownership request line, 10K pull-down (R6).
 *                  High = CH569 asks FPGA to release the card.
 *                  Low  = FPGA may claim the card.
 *   - UART2      : PA2 (RX, from FPGA "UART_TX") / PA3 (TX, to FPGA "UART_RX").
 *                  This is the CDC pass-through link. FPGA echoes for now.
 *   - UART1      : PA7/PA8 — debug log output (not routed to FPGA).
 *******************************************************************************/
#ifndef BRIDGE_CONFIG_H_
#define BRIDGE_CONFIG_H_

/* USB identity (SuperSpeed composite). 0x1A86 = WCH; PID picked to avoid the
 * stock example PIDs (FE10 MSC / FE0C CDC) so cached host drivers don't clash. */
#define BRIDGE_USB_VID_L            0x86
#define BRIDGE_USB_VID_H            0x1A
#define BRIDGE_USB_PID_L            0x2C
#define BRIDGE_USB_PID_H            0xFE

/* Byte on the CDC port that toggles SD ownership (consumed, not forwarded to
 * the FPGA). Client-requested bring-up shortcut. */
#define OWNERSHIP_TOGGLE_CHAR       't'

/* Out-of-band alternative to the toggle byte: vendor control request on EP0.
 *   bmRequestType 0x40, bRequest 0x50, wValue 0=release / 1=claim / 2=toggle
 *   bmRequestType 0xC0, bRequest 0x51 -> 1 status byte:
 *       bit0 = CH569 owns the card (PB10 high), bit1 = card mounted/ready    */
#define BRIDGE_REQ_SD_OWNER         0x50
#define BRIDGE_REQ_SD_STATUS        0x51

/* in bridge_config.h, near the other tunables */
#define MSC_EMMC_GUARD   2000000UL

/* 1 = claim the SD card automatically at power-up (CH569 drives PB10 high and
 * mounts the card immediately, so the drive appears with files without the host
 * pressing 't'). Set to 1 for this bring-up test, where the CH569 owns the card
 * by default. Set back to 0 for the production handover model, where the FPGA
 * owns the card at boot (PB10 idles low through R6) and the host claims it. */
#define CLAIM_ON_BOOT               1

/* Delay after raising PB10 before touching the SD bus. There is no ACK line
 * from the FPGA (assert-and-proceed, per client), so this must cover the
 * FPGA's flush + MUX switch time. */
#define PB10_SETTLE_MS              100

/* Attempts of the full SD init (each attempt internally tries all 8 EMMC IO
 * timing modes, as in the WCH SD example). */
#define SD_MOUNT_RETRIES            3

/* Default UART2 (FPGA link) baud rate; a CDC SET_LINE_CODING from the host
 * overrides it at run time. 8N1, no flow control. */
#define FPGA_UART_DEFAULT_BAUD      115200

/* Debug log UART1 baud rate (PA8 = TX, PA7 = RX). */
#define DEBUG_UART_BAUD             115200

/* 1 = also expose the debug log as a SECOND, read-only USB CDC port (a second
 * /dev/ttyACMx), so the log can be read over USB without wiring UART1. The
 * first CDC port stays the FPGA-UART bridge. */
#define DEBUG_OVER_USB              1

/* Status LEDs on PB22/PB23/PB24 (LEDs on the WCH eval board):
 *   PB22 = CH569 owns the SD card (PB10 high)
 *   PB23 = SD card mounted / ready
 *   PB24 = USB3 SuperSpeed link up
 * Set ACTIVE_HIGH to 0 if the board's LEDs are wired active-low (pin low = lit).
 * The WCH eval board wires VIO->1K->LED->pin, i.e. active-low, so 0 here means
 * "all three LEDs lit = everything good" (owns card + ready + USB3 link up). */
#define STATUS_LEDS                 1
#define STATUS_LED_ACTIVE_HIGH      0
#define LED_OWN_PIN                 GPIO_Pin_22
#define LED_READY_PIN               GPIO_Pin_23
#define LED_LINK_PIN                GPIO_Pin_24

#endif /* BRIDGE_CONFIG_H_ */
