# CH569 USB3 Bridge — SD Card (Mass Storage) + FPGA UART (Virtual COM)

Firmware for the **USB3_SSP** board (WCH **CH569W**). It enumerates on the USB-C
port as a **composite USB 3.0 SuperSpeed device** with two functions:

| Function | What the host sees | Backed by |
|---|---|---|
| USB Mass Storage (BOT/SCSI) | a removable drive | the microSD card, served as raw 512-byte sectors via the CH569 SD/EMMC controller |
| CDC-ACM #1 (bridge) | a serial/COM port | **UART2** (PA2/PA3), the board's UART link to the FPGA |
| CDC-ACM #2 (debug, read-only) | a second COM port | the firmware's own debug log (mirrors UART1) — `DEBUG_OVER_USB` |

The host OS owns the FAT32 filesystem — the firmware is filesystem-agnostic and
just moves sectors.

**Two COM ports appear.** By interface order, the first (lower-numbered, e.g.
Linux `ttyACM0`) is the **FPGA-UART bridge**; the second (`ttyACM1`) is the
**read-only debug log** — open it in any terminal to watch boot/SD/ownership
messages without wiring UART1. Bytes sent *to* the debug port are ignored.

**Status LEDs** (`STATUS_LEDS`, on the WCH eval board's PB22/23/24):
PB22 = CH569 owns the SD card · PB23 = card mounted/ready · PB24 = USB3 link up.
Set `STATUS_LED_ACTIVE_HIGH` to 0 if your LEDs are wired active-low.

**Debug serial pins** (always available, independent of the USB debug port):
**UART1 — PA8 = TX, PA7 = RX, 115200 8N1**.

## SD-card ownership (the MUX / PB10 protocol)

The SD card sits behind a physical MUX controlled by the FPGA. Only one side
(FPGA or CH569) is ever connected to the card:

- **PB10 high** → CH569 asks the FPGA to release the card (then waits
  `PB10_SETTLE_MS`, re-initialises the card, and reports "medium present" to
  the host).
- **PB10 low** → FPGA may claim the card; the host sees "no medium".
- There is **no ACK line** from the FPGA: assert-and-proceed, with the SD init
  retried (`SD_MOUNT_RETRIES` × 8 IO timing modes).

At power-up the FPGA owns the card (PB10 has a 10K pull-down). Ownership flips by:

1. **The `t` byte on the COM port** — consumed (not forwarded to the FPGA),
   toggles ownership. Bring-up shortcut as requested. Caveats: a binary stream
   containing `0x74` will also trigger it, and the byte will *not* come back in
   the FPGA echo.
2. **Vendor control request** (robust, recommended for real use):
   - `bmRequestType=0x40, bRequest=0x50, wValue = 0(release) / 1(claim) / 2(toggle)`
   - `bmRequestType=0xC0, bRequest=0x51` → 1 status byte: bit0 = CH569 owns,
     bit1 = card mounted.
   - e.g. Linux: `usbtool`/`pyusb` one-liner in `docs/TESTING.md`.

When ownership changes, the firmware raises a SCSI **UNIT ATTENTION (media
changed)** and reports **medium not present** while the FPGA holds the card, so
the host re-reads the filesystem instead of trusting a stale cache.

## Build

### Docker (recommended — no local toolchain needed)

```sh
docker compose run --rm build            # builds build/ch569_sd_uart_bridge.{elf,bin,hex}
docker compose run --rm build make clean
docker compose run --rm build make check # static USB-descriptor self-check
```

A GitHub Actions workflow (`.github/workflows/build.yml`) runs the same build +
descriptor check on every push and uploads the `.bin`/`.hex` as artifacts, if
this folder is hosted in a GitHub repo.

### Native

Needs xPack `riscv-none-elf-gcc` (12.x) + GNU make:

```sh
make            # or: make PREFIX=riscv64-unknown-elf- ARCH="-march=rv32imac -mabi=ilp32"
```

## Flash

Via WCH **WCHISPTool** (Windows) or [wchisp](https://github.com/ch32-rs/wchisp) over USB:

1. Hold the **BOOT** button while plugging in / resetting the board — the chip
   enumerates as the WCH bootloader.
2. Load `build/ch569_sd_uart_bridge.hex` (WCHISPTool),
   `wchisp flash build/ch569_sd_uart_bridge.bin`, or
   `wch-ch56x-isp -vr -f=build/ch569_sd_uart_bridge.bin`.
3. Reset. Debug log appears on **UART1 (PA7/PA8) at 115200 8N1**.

The flashed image is ~21 KB (`.bin`). The DMA ring buffers are RAM-only
(linker `.DMADATA (NOLOAD)`), so they are not part of the flash image.

> **At boot the drive shows "no medium"** — this is correct: the FPGA owns the
> SD card until the host claims it. Open the COM port and press `t` (or send the
> vendor claim request) to mount it. See "SD-card ownership" above.

## Configuration

Everything tunable lives in `src/bridge_config.h`: VID/PID, the `t` toggle
byte, `CLAIM_ON_BOOT`, PB10 settle time, SD retry count, default UART baud.

## Host setup note (Linux)

On Linux, **ModemManager** probes any CDC-ACM port on plug-in (cycling baud
rates, sending AT strings). Since this device exposes two ACM ports, tell MM to
ignore it so it doesn't poke the bridge:

```
# /etc/udev/rules.d/99-ch569-nomm.rules
ACTION=="add|change", SUBSYSTEMS=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="fe2c", ENV{ID_MM_DEVICE_IGNORE}="1"
```
`sudo udevadm control --reload-rules`, then replug. (Forwarded bytes are now
harmless anyway — ownership is control-request only — but this stops the noise.)

## Known limitations (by design, for this bring-up)

- **USB2 fallback is drive-only.** If the link trains at high-speed instead of
  SuperSpeed (USB2 cable/port), the device enumerates as MSC only — the COM
  port exists only on USB3. (The vendor USB2 stack has no CDC function;
  extending it is possible but out of bring-up scope.)
- **COM port pauses during large file transfers.** The MSC engine masks the
  USB interrupt while it streams sectors (vendor design). UART2 *receive* is
  interrupt-driven and buffered (1 KB ring), so no FPGA bytes are lost; they
  are delivered when the transfer completes.
- The CDC notification endpoint is declared but idle (no serial-state events),
  matching WCH's own CDC example. Hosts tolerate this.
- `_close/_fstat/_isatty/...` linker notes are expected (`--specs=nosys.specs`;
  only `_write` is used, for the debug printf).

## Layout

```
src/                  firmware
  Main.c              init + main loop (MSC pump / CDC pump / ownership poll)
  CH56x_usb30.c/.h    unified composite USB3 layer (descriptors, EP0, callbacks)
  CH56x_usb20.c/.h    USB2 fallback (vendor MSC stack, MSC-only)
  SW_UDISK.c/.h       MSC/SCSI engine (vendor + media-change UNIT ATTENTION)
  SD.c/.h             SD-card command helpers (vendor, verbatim)
  sd_storage.c/.h     SD init: CMD8/ACMD41 SDXC flow + adaptive IO timing
  cdc_uart.c/.h       CDC <-> UART2 bridge + 't' ownership toggle
  ownership.c/.h      PB10 claim/release state machine
  bridge_config.h     all tunables
sdk/                  vendored WCH CH569 EVT pieces (drivers, startup, ld, USB3 lib)
tools/                check_descriptors.py (static descriptor validation)
docs/TESTING.md       step-by-step bench test procedure
```

Endpoint map (SuperSpeed): EP1-IN = CDC notify · EP2-IN/EP3-OUT = MSC bulk ·
EP4-IN/OUT = CDC data. Interfaces: IF0 = MSC, IAD(IF1 = CDC comm, IF2 = CDC data).

## Provenance

Built on WCH's official CH569 EVT examples
([github.com/openwch/ch569](https://github.com/openwch/ch569)): `MSC_U-Disk`
(USB3 MSC), `SimulateCDC` (USB3 CDC), `SD` (SD-card init), plus the stock
peripheral drivers and the precompiled `libCH56x_USB30_device_lib.a` USB3
device library. The USB enumeration layer is a hand merge of the two USB3
examples (they define identical symbols and cannot co-link as shipped).
