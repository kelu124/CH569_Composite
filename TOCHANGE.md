# CH569 SD/UART USB3 Bridge — Debug Changelog & Patch Spec

**Audience:** a coding agent that will edit the firmware files directly.
**Goal:** apply the confirmed fixes from a long debugging session, and remove the
debug-over-USB CDC port (we now use the physical UART1 TX/RX only).

**Context.** This is a CH569 USB3 *composite* device (USB Mass Storage on the SD/eMMC
card behind an FPGA-controlled MUX + CDC-ACM bridge to UART2). It was assembled from
the WCH CH56x EVT examples (MSC_U-Disk, SimulateCDC, SD). The root theme of nearly
every bug: **single-function example code whose assumptions break when the USB
controller is shared across functions and the host probes the ports.**

Apply the steps in order. Each step lists the file, the location, the change, the code,
and why. Do not skip the rationale — several changes have non-obvious invariants.

---

## Conventions used below

- `MSC_EMMC_GUARD` is a finite loop-iteration cap that converts unbounded hardware
  waits into clean failures. It must be visible to every file that does EMMC waits
  (`SW_UDISK.c`, `SD.c`, `sd_storage.c`).
- "Bounded wait" pattern, used everywhere we replace a `while(1){ CheckCMDComp }`:
  ```c
  { UINT32 g = MSC_EMMC_GUARD;
    while( (sta = CheckCMDComp(&TF_EMMCParam)) == CMD_NULL )
        if( --g == 0 ) { sta = CMD_FAILED; break; } }
  ```

---

## Step 0 — Define MSC_EMMC_GUARD in one shared place

**File:** `bridge_config.h`
**Change:** add the guard constant (single source of truth).

```c
/* Bounded spin guard for EMMC command/data completion waits. Finite so an
 * unanswered card fails the mount/read instead of hanging the device forever.
 * Iteration count, not time: at 80 MHz this is ~tens of ms for a tight loop.
 * The ACMD41/SDReadOCR power-up poll has its own multi-second retry budget. */
#define MSC_EMMC_GUARD   2000000UL
```

**Then:** remove any duplicate `#define MSC_EMMC_GUARD` from `SW_UDISK.c`.
**Verify:** `SD.c` and `sd_storage.c` reach this define — add `#include "bridge_config.h"`
at the top of each if not already pulled in transitively.

**Why:** the guard was previously defined only in `SW_UDISK.c`, causing
`'MSC_EMMC_GUARD' undeclared` build errors in `SD.c`.

---

## Step 1 — Stop masking the USB3 interrupt during MSC transfers  *(root cause #1)*

**File:** `SW_UDISK.c`
**Functions:** `UDISK_Up_OnePack`, `UDISK_Down_OnePack`

**Change:** in the SuperSpeed branch (`Link_Sta != LINK_STA_1`), **delete** every
`PFIC_DisableIRQ(USBSS_IRQn)` / `PFIC_EnableIRQ(USBSS_IRQn)` pair and the long
busy-wait pipeline between them. Replace with a **single bounded chunk** per call,
driven by the existing `UDISK_InPackflag` / `EP2_IN_Callback` / `UDISK_In_EP_Deal`
scaffolding (the same mechanism INQUIRY/READ-CAPACITY already use successfully).

The `Link_Sta == LINK_STA_1` (USB2 fallback, MSC-only) branch can keep its existing
behaviour for now — it is not part of the composite-interleave problem.

Final `UDISK_Up_OnePack` (SuperSpeed branch) reads one chunk and arms EP2:

```c
void UDISK_Up_OnePack( void )
{
    UINT16 nsec;
    UINT32 chunk;

    if( Link_Sta == LINK_STA_1 ) { /* existing USB2 branch unchanged */ return; }

    if( UDISK_Transfer_DataLen == 0 ) {            /* done: CSW handled by EP2 cb */
        Udisk_Transfer_Status &= ~DEF_UDISK_BLUCK_UP_FLAG;
        return;
    }

    chunk = (UDISK_Transfer_DataLen < MSC_CHUNK_BYTES) ? UDISK_Transfer_DataLen : MSC_CHUNK_BYTES;
    nsec  = chunk / 512;

    if( msc_read_chunk( UDisk_In_Buf, UDISK_Cur_Sec_Lba, nsec ) != OP_SUCCESS ) {
        UDISK_CMD_Deal_Status( 0x04, 0x11, 0x01 );   /* MEDIUM ERROR, unrec read */
        UDISK_CMD_Deal_Fail();                        /* STALL EP2, clears UP flag */
        return;
    }

    UDISK_Cur_Sec_Lba      += nsec;
    UDISK_Transfer_DataLen -= chunk;
    if( UDISK_Transfer_DataLen == 0 )                /* last data packet: */
        Udisk_Transfer_Status &= ~DEF_UDISK_BLUCK_UP_FLAG;  /* next EP2 cb -> CSW */

    USB30_IN_ClearIT( ENDP_2 );
    USBSS->UEP2_TX_DMA = (UINT32)(UINT8 *)UDisk_In_Buf;
    USB30_IN_Set( ENDP_2, ENABLE, ACK, DEF_ENDP2_IN_BURST_LEVEL, chunk );
    USB30_Send_ERDY( ENDP_2 | IN, DEF_ENDP2_IN_BURST_LEVEL );
}
```

Final `UDISK_Down_OnePack` (SuperSpeed branch) consumes one EP3 packet, writes it,
re-arms or finishes:

```c
void UDISK_Down_OnePack( void )
{
    UINT16 nsec, i, len;

    if( Link_Sta == LINK_STA_1 ) { /* existing USB2 branch unchanged */ return; }

    len  = UDISK_Out_Pack_Len;     /* bytes the host just delivered to UDisk_Out_Buf */
    nsec = len / 512;

    for( i = 0; i < nsec; i++ ) {
        if( msc_write_chunk( UDisk_Out_Buf + i*512, UDISK_Cur_Sec_Lba + i, 1 ) != OP_SUCCESS ) {
            UDISK_CMD_Deal_Status( 0x03, 0x0C, 0x00 );   /* WRITE ERROR */
            UDISK_CMD_Deal_Fail();                        /* STALL EP3, clears DOWN */
            return;
        }
    }

    UDISK_Cur_Sec_Lba += nsec;
    UDISK_Transfer_DataLen = (UDISK_Transfer_DataLen >= len) ? (UDISK_Transfer_DataLen - len) : 0;

    if( UDISK_Transfer_DataLen == 0 ) {
        Udisk_Transfer_Status &= ~DEF_UDISK_BLUCK_DOWN_FLAG;
        UDISK_Up_CSW();
    }
    /* else: UDISK_onePack_Deal re-ACKs EP3 for the next data packet (Step 1c) */
}
```

**Why:** EP0 (enumeration + CDC class requests) is serviced **only** by the USBSS ISR.
The original code disabled that ISR for the whole multi-sector transfer and busy-waited,
so every control transfer timed out at 5 s. Harmless in the MSC-only example; fatal in a
composite device the host interleaves control/CDC traffic with. The diagnostic signature
was 5-second `ENOENT` completions on every EP0 control transfer. Keeping the ISR live
between single chunks fixes it.

### Step 1a — New globals

**File:** `SW_UDISK.c` (near the other globals, ~line 154)

```c
volatile uint16_t UDISK_Out_Pack_Len = 0;   /* bytes in UDisk_Out_Buf this pass */

/* Sectors moved per USB packet. 1 sector = simplest/proven; raise later only with
 * matching burst + multi-block DMA handling. Must be <= UDISKSIZE/512. */
#define MSC_CHUNK_SECTORS   1
#define MSC_CHUNK_BYTES     (MSC_CHUNK_SECTORS * 512)
```

**File:** `SW_UDISK.h` — add next to the other `extern` decls:
```c
extern volatile uint16_t UDISK_Out_Pack_Len;
```

### Step 1b — Bounded EMMC chunk helpers (CMD17 read / CMD25 write)

**File:** `SW_UDISK.c`, placed just above `UDISK_Up_OnePack`.

```c
/* Bounded single-sector reads (CMD17). No USBSS_IRQn masking. */
static UINT8 msc_read_chunk( UINT8 *buf, UINT32 lba, UINT16 nsec )
{
    UINT16 i;
    UINT32 guard;

    for( i = 0; i < nsec; i++ )
    {
        guard = MSC_EMMC_GUARD;
        while( !(R32_EMMC_STATUS & (1<<17)) )                 /* controller ready */
            if( --guard == 0 ) return OP_FAILED;

        TF_EMMCParam.EMMCOpErr = 0;
        R32_EMMC_DMA_BEG1  = (UINT32)(buf + i*512);
        R32_EMMC_TRAN_MODE = 0;                                /* single block, like SD.c */
        R32_EMMC_BLOCK_CFG = (TF_EMMCParam.EMMCSecSize) << 16 | 1;

        EMMCSendCmd( lba + i, RB_EMMC_CKIDX | RB_EMMC_CKCRC | RESP_TYPE_48 | EMMC_CMD17 );

        guard = MSC_EMMC_GUARD;
        while( !(R16_EMMC_INT_FG & RB_EMMC_IF_TRANDONE) )
            if( TF_EMMCParam.EMMCOpErr || --guard == 0 ) { R16_EMMC_INT_FG = 0xffff; return OP_FAILED; }
        R16_EMMC_INT_FG = RB_EMMC_IF_CMDDONE | RB_EMMC_IF_TRANDONE;
    }
    return OP_SUCCESS;
}

/* Bounded single-sector write (CMD25 + CMD12), mirrors SD.c SDCardWriteONESec. */
static UINT8 msc_write_chunk( UINT8 *buf, UINT32 lba, UINT16 nsec )
{
    UINT16 i;
    UINT32 guard;
    UINT8  s;

    for( i = 0; i < nsec; i++ )
    {
        guard = MSC_EMMC_GUARD;
        while( !(R32_EMMC_STATUS & (1<<17)) )
            if( --guard == 0 ) return OP_FAILED;

        TF_EMMCParam.EMMCOpErr = 0;
        EMMCSendCmd( lba + i, RB_EMMC_CKIDX | RB_EMMC_CKCRC | RESP_TYPE_48 | EMMC_CMD25 );
        guard = MSC_EMMC_GUARD;
        while( (s = CheckCMDComp(&TF_EMMCParam)) == CMD_NULL )
            if( --guard == 0 ) return OP_FAILED;
        if( s == CMD_FAILED ) return OP_FAILED;

        R32_EMMC_TRAN_MODE = RB_EMMC_DMA_DIR | (1<<6);
        R32_EMMC_DMA_BEG1  = (UINT32)(buf + i*512);
        R32_EMMC_BLOCK_CFG = (TF_EMMCParam.EMMCSecSize) << 16 | 1;

        guard = MSC_EMMC_GUARD;
        while( !(R16_EMMC_INT_FG & RB_EMMC_IF_TRANDONE) )
            if( TF_EMMCParam.EMMCOpErr || --guard == 0 ) return OP_FAILED;
        R16_EMMC_INT_FG = RB_EMMC_IF_TRANDONE | RB_EMMC_IF_CMDDONE;

        R32_EMMC_TRAN_MODE = 0;
        EMMCSendCmd( 0, RB_EMMC_CKIDX | RB_EMMC_CKCRC | RESP_TYPE_R1b | EMMC_CMD12 );
        guard = MSC_EMMC_GUARD;
        while( CheckCMDComp(&TF_EMMCParam) == CMD_NULL )
            if( --guard == 0 ) return OP_FAILED;
        R16_EMMC_INT_FG = RB_EMMC_IF_CMDDONE;
    }
    return OP_SUCCESS;
}
```

**Why CMD17, not CMD18:** CMD18 (read-multiple) with a block count of 1 never raised
`TRANDONE` on this controller and hung the read. CMD17 (read-single-block) is the proven
single-sector primitive (matches `SD.c`'s `SDCardReadOneSec`). Multi-sector CMD18 is a
later optimization that additionally requires reloading `R32_EMMC_DMA_BEG1` on every
`RB_EMMC_IF_BKGAP` and terminating with CMD12 — do not use it until single-sector is solid.

### Step 1c — Re-ACK EP3 only while a write is in progress

**File:** `SW_UDISK.c`, `UDISK_onePack_Deal`

```c
    if( UDISK_OutPackflag == 1 )
    {
        UDISK_OutPackflag = 0;
        UDISK_Down_OnePack();

        if( Udisk_Transfer_Status & DEF_UDISK_BLUCK_DOWN_FLAG )   /* more data expected */
        {
            if( Link_Sta == LINK_STA_1 )
                R8_UEP3_RX_CTRL = (R8_UEP3_RX_CTRL & ~RB_UEP_RRES_MASK) | UEP_R_RES_ACK;
            else {
                USB30_OUT_Set(ENDP_3, ACK, DEF_ENDP3_OUT_BURST_LEVEL);
                USB30_Send_ERDY(ENDP_3 | OUT, DEF_ENDP3_OUT_BURST_LEVEL);
            }
        }
    }
```

**Why:** otherwise the device requests another EP3 data packet after the final CSW.

### Step 1d — Stash the received length for the write path

**File:** `CH56x_usb30.c`, `EP3_OUT_Callback`, immediately after `USB30_OUT_Status(...)`:

```c
    USB30_OUT_Status(ENDP_3, &nump, &len, NULL);
    UDISK_Out_Pack_Len = len;            /* hand the length to UDISK_Down_OnePack */
```

`EP2_IN_Callback` needs **no change** — it already re-drives the chain correctly.

---

## Step 2 — Fix the EP3-OUT SuperSpeed burst descriptor  *(reset loop)*

**File:** `CH56x_usb30.c`, config descriptor, the MSC EP3-OUT companion (was ~line 95).

**Change:** `bMaxBurst` `0x03 → 0x00`.

```c
/* MSC EP3-OUT endpoint companion */
0x06, 0x30, 0x00, 0x00, 0x00, 0x00,   /* burst 0 (=1 pkt), matches DEF_ENDP3_OUT_BURST_LEVEL */
```

**Why:** the companion advertised a burst of 3 (leftover stock-example value) while the
firmware armed EP3 at burst level 1. The first MSC CBW on EP3-OUT then failed at the
SuperSpeed transaction level (`EPROTO`), and usb-storage reset the device in a ~150 ms
loop. **Invariant:** descriptor `bMaxBurst`, `DEF_ENDPx_*_BURST_LEVEL`, and the
`USB30_*_Set(...)` burst args must always agree. After Step 8 (kill debug CDC), re-verify
every companion `bMaxBurst` in the active (3-interface) descriptor is `0x00`, except where
runtime intentionally bursts more.

---

## Step 3 — Make the EMMC error interrupt latch and clear

**File:** `SW_UDISK.c`, `EMMC_IRQHandler`

```c
void EMMC_IRQHandler(void)
{
    UINT16 fg = R16_EMMC_INT_FG;
    if(fg) {
        TF_EMMCParam.EMMCOpErr = fg;   /* latch so polled read/write loops can see it */
        R16_EMMC_INT_FG = fg;          /* CLEAR — without this the IRQ re-fires forever */
    }
}
```

**Why:** the original handler printed the flag but never cleared it → interrupt storm,
and `EMMCOpErr` was never set, so the polled EMMC loops could not detect failure and
hung. Latching `EMMCOpErr` is what lets the bounded helpers in Step 1b/Step 4 exit on error.

---

## Step 4 — Bound every EMMC wait in the SD init code

**Files:** `SD.c` and `sd_storage.c`

Replace **every** `while(1){ sta = CheckCMDComp(&TF_EMMCParam); if(sta!=CMD_NULL) break; }`
and every `while(1){ if(R16_EMMC_INT_FG & ...) break; }` with the bounded pattern:

```c
{ UINT32 g = MSC_EMMC_GUARD;
  while( (sta = CheckCMDComp(&TF_EMMCParam)) == CMD_NULL )
      if( --g == 0 ) { sta = CMD_FAILED; break; } }
```

Specific locations (search and convert all of them):
- `sd_storage.c` → `sd_card_init`: the **CMD8** `while(1)` loop (this one hung the boot).
- `SD.c` → `SDReadOCR`: both inner CMD55 and CMD41 `while(1)` waits. **Important:** keep
  the outer `for(i=0;i<100;i++)` retry so ACMD41 power-up still gets its ~2 s budget, but
  ensure it ultimately `return OP_FAILED;` if never ready (see Step 7 for the open issue).
- `SD.c` → `SDSetRCA`, `SDReadCSD`, `SDSetBusWidth`, `SD_ReadSCR`: all `while(1)` waits.
- `SD.c` → `SDCardReadOneSec`, `SDCardWriteONESec`: the `while(!(...))` / `while(1)` data waits.

**Why:** unbounded waits turned any unanswered SD command into a full device hang
(no USB, dead debug port). Bounding them converts a hang into a clean
"SD: init failed in all IO modes", keeping USB alive and the failure observable.

---

## Step 5 — Remove the `'t'` ownership-toggle data byte  *(footgun)*

**File:** `cdc_uart.c`, `U30_CDC_UartTx_Deal`

```c
static void U30_CDC_UartTx_Deal(void)
{
    if(USBByteCount)
    {
        UINT8 ch = endp4Rxbuff[USBBufOutPoint++];
        USBByteCount--;
        UART2_SendString(&ch, 1);          /* forward ALL bytes; no magic 't' */
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
```

**Also:**
- `bridge_config.h`: remove (or comment out) `#define OWNERSHIP_TOGGLE_CHAR 't'`.
- `cdc_uart.h`: update the header comment that documents the `'t'` behaviour.
- Ownership now changes **only** via the EP0 vendor request `BRIDGE_REQ_SD_OWNER`
  (already implemented in `USB30_NonStandardReq`). Leave that path intact.

**Why:** any host that opens the port (ModemManager, a terminal, a script) sends probe
bytes; a byte equal to `'t'` triggered `ownership_release()` → `sd_storage_unmount()`
while the disk was mounted, yanking the medium out from under usb-storage and causing
resets. A magic data byte must never tear down a live mass-storage device.

---

## Step 6 — (Recommended) Fix the BOS U1/U2 exit latencies

**File:** `CH56x_usb30.c`, BOS descriptor, SuperSpeed USB Device Capability.

Set non-zero `bU1DevExitLat` / `bU2DevExitLat` (e.g. U1 ≤ 0x0A, U2 ≤ 0x07FF) instead of 0.

**Why:** the host logs `LPM exit latency is zeroed, disabling LPM`. Cosmetic today
(LPM ends up disabled either way) but the zeroed values are an invalid descriptor.

---

## Step 7 — KILL the debug-over-USB CDC port (use physical UART1 only)

We rely on the physical UART1 (PA8 = TX, PA7 = RX) for `PRINT` output. The second USB CDC
"debug log" port (EP5/EP6, the 5-interface composite) is removed.

**7a. `bridge_config.h`**
```c
#define DEBUG_OVER_USB   0
```
This flips the existing `#if DEBUG_OVER_USB` guards: config descriptor drops from 212→128
bytes, interfaces 5→3, EP5/EP6 init and `debug_cdc_poll()` compile out.

**7b. `Main.c`** — remove the debug-CDC poll from the main loop and its include:
```c
/* delete: #include "debug_cdc.h"      (it is already #if DEBUG_OVER_USB guarded) */
while(1)
{
    UDISK_onePack_Deal();
    CDC_Uart_Deal();
    ownership_poll();
    /* debug_cdc_poll();  -- REMOVED: debug log goes out physical UART1 now */
#if STATUS_LEDS
    status_leds_update();
#endif
}
```
(If `debug_cdc_poll()` is already inside an `#if DEBUG_OVER_USB` block, setting the macro
to 0 removes it; just confirm no unguarded call remains.)

**7c. Verify the 3-interface descriptor is correct.**
With `DEBUG_OVER_USB == 0`, `SIZE_CONFIG_DESC == 128` and `BRIDGE_NUM_INTERFACES == 0x03`.
Confirm `SS_ConfigDescriptor` in `CH56x_usb30.c` has a complete, consistent 3-interface
variant: IF0 MSC (EP2-IN/EP3-OUT), IAD + IF1/IF2 CDC bridge (EP1 notify, EP4-IN/EP4-OUT),
`bNumInterfaces = 3`, correct `wTotalLength`, and **every endpoint companion `bMaxBurst`
consistent with its runtime burst** (see Step 2). The device should now expose **one**
`ttyACM` (the bridge) plus the mass-storage disk.

**7d. Keep `PRINT` on physical UART1.**
`DebugInit()` in `Main.c` (UART1, PA8 TX) is unconditional — leave it. `PRINT`/`printf`
output goes to the physical pin. The `debug_mirror` ring hook in `debug_cdc.c` is now
dead code (its file compiles out under `#if DEBUG_OVER_USB`); leave the files in place but
unused, or delete `debug_cdc.c`/`debug_cdc.h` and remove references if you want a clean tree.

**7e. (Optional, debug hardening) Make PRINT survive tight loops.**
The physical UART corrupts (bit-flips, spliced lines) when `PRINT` is called rapidly and
re-entered from a USB/CDC interrupt. For noisy debug paths, drain TX after each line:
```c
/* after a debug PRINT in a tight loop */
while( !(R8_UART1_LSR & RB_LSR_TX_ALL_EMPTY) );
```
and emit at most one line per retry (not per inner command).

**Why:** the USB debug port is drained by the main loop, so it goes dead exactly when the
main loop hangs — useless for the failures we were chasing. Physical UART1 prints
synchronously and survives hangs. Dropping the second CDC function also simplifies the
descriptor and removes a whole probed endpoint pair.

---

## Step 8 — OPEN ISSUE (do not mark resolved): SD card init hangs on ACMD41

**Status:** unresolved as of this changelog. The mount loops on **ACMD41**; the card never
reports the OCR ready bit (bit 31). With Step 4 applied the device stays alive, but the
mount still fails.

**Leading hypothesis:** *warm-card state.* The card stays powered through the MUX across
resets/reflashes, so it is not in the idle state ACMD41 requires, and CMD0
(`EMMCResetIdle`) may not be returning it to idle. Supported by the fact that the card
**did** enumerate at full 7.95 GB capacity in an earlier run — the hardware and ACMD41
sequence are capable of working.

**Agent: implement the diagnostic, do not guess a fix:**

In `SD.c` `SDReadOCR`, capture status/response and report **once after** the retry loop
(in-loop printing corrupts the UART):
```c
UINT32 last_rsp3 = 0; UINT8 last_sta = 0xFF;
for( i = 0; i < 100; i++ ) {
    /* CMD55 (bounded) ; CMD41 (bounded) */
    last_sta = sta; last_rsp3 = R32_EMMC_RESPONSE3;
    if( sta == CMD_SUCCESS && (last_rsp3 & (1u<<31)) ) break;   /* ready */
    mDelaymS(20);
}
PRINT("\r\n@@@ A41 i=%ld sta=%d rsp3=%08lx @@@\r\n", i, last_sta, last_rsp3);
while( !(R8_UART1_LSR & RB_LSR_TX_ALL_EMPTY) );
if( i >= 100 ) return OP_FAILED;     /* ensure the loop actually gives up */
```
Interpretation for the human: `sta` never `CMD_SUCCESS` → CMD41 not completing (card not
answering OCR). `sta==CMD_SUCCESS` but `rsp3` bit 31 clear → card busy/never finishes
power-up. Bit 31 set but loop continues → ready-check logic bug.

**Confirmation test for the human (not code):** fully power-cycle the board (unplugged
~10 s so the card rail discharges) and boot cold. If it mounts cold but hangs on ACMD41
after warm reset/reflash, the warm-card theory is confirmed and the fix is to make CMD0
reliably return the card to idle before ACMD41 (or detect "already initialized" and skip
OCR negotiation).

---

## Acceptance checks (run after Steps 0–7)

1. **Builds clean** — no `MSC_EMMC_GUARD` undeclared, no missing-symbol errors.
2. **Enumerates** — host shows one `ttyACM` + one mass-storage disk; dmesg has no
   `reset SuperSpeed` loop and no 5 s control-transfer timeouts.
3. **EP3 CBW** — usbmon shows the first MSC CBW on EP3-OUT completing `OK` (no `EPROTO`).
4. **Control + I/O interleave** — opening the `ttyACM` while reading the disk does not
   stall EP0; control transfers complete in <1 s.
5. **No hang on SD failure** — if the card is absent/unready, UART1 prints
   `SD: init failed in all IO modes` and the device stays enumerated (does not freeze).
6. **`'t'` is inert** — sending the byte `t` to the bridge port forwards it to UART2 and
   does **not** change SD ownership.
7. **Debug** — `PRINT` output appears on physical UART1 (PA8); no second USB CDC port.

Once Step 8's ACMD41 issue is resolved, READ10/WRITE10 should complete and the disk
should mount on the host.

---

## One-line summary of the journey (for context)

USB-IRQ masking starved EP0 → EP3 burst descriptor mismatch reset loop → EMMC IRQ
not cleared + CMD18-count-1 + unbounded waits broke reads → `'t'` byte unmounted a live
disk under host probing → (open) SD ACMD41 fails to bring up a warm card. Fixes 1–7 are
confirmed; fix 8 is the active thread.
