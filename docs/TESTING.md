# Bench Test Procedure — CH569 SD/UART USB3 Bridge

Step-by-step validation on the real board. Each step names the expected
outcome; if a step fails, capture the **UART1 debug log (PA7/PA8, 115200 8N1)**
and the step number — that is enough for a firmware-side diagnosis.

## 0. Setup

- microSD (64 GB, FAT32, with a few files the FPGA wrote) in its slot.
- FPGA gateware loaded: UART echo running; PB10 release logic active
  (PB10 high → FPGA flushes + flips `MUX_SEL` to the CH569; PB10 low → FPGA
  may reclaim).
- Serial dongle on UART1 (PA7=RX of the dongle side: board TX) for the log.
- Board USB-C → a **USB 3.x port** on the host (blue / SS-marked).

> **This build claims the SD card at boot** (`CLAIM_ON_BOOT 1`): the CH569 owns
> the card by default, so the drive should appear mounted without pressing `t`.
> Two COM ports appear: `ttyACM0` = FPGA-UART bridge, `ttyACM1` = read-only
> debug log (open it to watch the messages below over USB instead of UART1).
> Status LEDs: PB22 = owns SD, PB23 = card ready, PB24 = USB3 link up.

## 1. Flash + boot

1. Hold BOOT, plug in, flash `build/ch569_sd_uart_bridge.hex` (WCHISPTool) or
   `wchisp flash build/ch569_sd_uart_bridge.bin`.
2. Reset. Expected debug log:
   ```
   CH569 SD/UART USB3 bridge (80 MHz)
   USB init done
   ```

## 2. Enumeration (composite)

- **Windows**: Device Manager shows, simultaneously:
  - *Disk drives* → a removable disk (no medium yet — that's correct: the
    FPGA owns the card at boot), and
  - *Ports (COM & LPT)* → "USB Serial Device (COMx)".
- **Linux**: `lsusb -d 1a86:fe2c -v` shows `bcdUSB 3.00`,
  `bDeviceClass 239 Miscellaneous`, 3 interfaces (08/06/50, 02/02/01, 0A);
  `dmesg` shows `usb-storage` + `cdc_acm` binding; a `/dev/sdX` (no medium)
  and `/dev/ttyACM0` appear.

FAIL here → note whether it enumerated at SuperSpeed (`bcdUSB 3.00` /
`5000M` in `lsusb -t`) or fell back to USB2 (drive only, no COM port —
check the cable/port are USB3).

## 3. UART echo through the COM port

1. Open the COM port (PuTTY / `minicom -D /dev/ttyACM0`) at **115200**.
2. Type characters **other than `t`** — each should echo back (FPGA echo).
3. Change the terminal baud (e.g. 9600) — debug log prints
   `CDC: line coding 9600 baud`; echo still works if the FPGA side matches
   (FPGA echo is baud-agnostic only if its UART is fixed at 115200 — keep
   115200 unless the gateware is rebauded).

FAIL → check PA2/PA3 wiring direction; debug log shows `uart2 line error:xx`
on framing problems.

## 4. Claim the SD card ('t' toggle)

1. In the open COM session press **`t`** (it will *not* echo — it is consumed).
2. Expected debug log:
   ```
   OWN: claiming SD card (PB10 high)
   SD: init OK (io mode N), type 2, XXXXXXX sectors
   OWN: card ready, XXXXXXX sectors (61057 MB)
   ```
3. Host: the removable drive now reports media; the FAT32 volume mounts and
   the FPGA's files are visible.

FAIL at `SD: init failed in all IO modes` → PB10/MUX timing: confirm the FPGA
released the card; try raising `PB10_SETTLE_MS`.

## 5. File I/O

1. Copy a ~100 MB file host → drive; verify (`fc /b` on Windows,
   `sha256sum` on Linux) after a remount.
2. Copy files drive → host, compare with what the FPGA wrote.
3. While a large copy runs, typed COM-port characters may stall and burst
   afterwards — expected (documented limitation), nothing should be lost
   from the FPGA→host direction.

## 6. Release / re-claim cycle

1. Eject/unmount the volume on the host (flushes the FAT).
2. Press `t` — log: `OWN: releasing SD card (PB10 low)`; host shows
   "no medium" again; FPGA may use the card.
3. Let the FPGA write a new file, press `t` again: drive remounts and the
   new file is visible (the UNIT ATTENTION forces a re-read — no stale
   directory listing).

## 7. Out-of-band ownership control (optional, replaces 't')

Linux (pyusb):
```python
import usb.core
dev = usb.core.find(idVendor=0x1A86, idProduct=0xFE2C)
dev.ctrl_transfer(0x40, 0x50, 2, 0)            # toggle (0=release, 1=claim)
print(dev.ctrl_transfer(0xC0, 0x51, 0, 0, 1))  # status: bit0 owns, bit1 ready
```

## 8. Reboot persistence

Power-cycle the board with the host attached: it should re-enumerate cleanly
and return to the boot state (FPGA owns card, COM port live).

---

### What to send back if anything fails
1. The step number and what differed.
2. Full UART1 debug log from reset.
3. `lsusb -v -d 1a86:fe2c` output (Linux) or USBView/USBTreeView dump (Windows).
