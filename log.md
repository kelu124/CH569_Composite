# DMESG 1

```
[130229.158054] usb 1-1: new full-speed USB device number 33 using xhci_hcd
[130229.284921] usb 1-1: New USB device found, idVendor=4348, idProduct=55e0, bcdDevice=27.00
[130229.284966] usb 1-1: New USB device strings: Mfr=0, Product=0, SerialNumber=0
[130233.559027] usb 1-1: USB disconnect, device number 33
[130233.860072] usb 2-1: new SuperSpeed USB device number 2 using xhci_hcd
[130233.872329] usb 2-1: LPM exit latency is zeroed, disabling LPM.
[130233.872656] usb 2-1: New USB device found, idVendor=1a86, idProduct=fe2c, bcdDevice= 0.01
[130233.872668] usb 2-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[130233.872675] usb 2-1: Product: SD UART Bridge USB3
[130233.872680] usb 2-1: Manufacturer: WCH
[130233.872685] usb 2-1: SerialNumber: 0123456789
[130233.879573] cdc_acm 2-1:1.1: ttyACM0: USB ACM device
[130233.881618] cdc_acm 2-1:1.3: ttyACM1: USB ACM device
[130233.930042] usb-storage 2-1:1.0: USB Mass Storage device detected
[130233.930721] scsi host3: usb-storage 2-1:1.0
[130233.932441] usbcore: registered new interface driver usb-storage
[130233.935971] usbcore: registered new interface driver uas
[130234.960030] scsi 3:0:0:0: Direct-Access EMMC USB3.0 Disk WCH PQ: 0 ANSI: 2
[130234.961276] sd 3:0:0:0: Attached scsi generic sg1 type 0
[130280.335791] xhci_hcd 0000:00:14.0: Timeout while waiting for setup device command
[130285.967008] xhci_hcd 0000:00:14.0: Timeout while waiting for setup device command
[130286.174760] usb 2-1: device not accepting address 2, error -62
[130287.759060] sd 3:0:0:0: [sdb] 15523840 512-byte logical blocks: (7.95 GB/7.40 GiB)
[130287.759553] usb 2-1: USB disconnect, device number 2
[130287.768742] sd 3:0:0:0: [sdb] Write Protect is off
[130287.768750] sd 3:0:0:0: [sdb] Mode Sense: 00 00 00 00
[130287.768763] sd 3:0:0:0: [sdb] Asking for cache data failed
[130287.768769] sd 3:0:0:0: [sdb] Assuming drive cache: write through
[130287.769155] sd 3:0:0:0: [sdb] Attached SCSI removable disk
[130287.806409] usb usb2-port1: config error
[130291.862859] usb usb2-port1: Cannot enable. Maybe the USB cable is bad?
[130291.862905] usb usb2-port1: config error
[130292.718027] usb 1-1: new high-speed USB device number 39 using xhci_hcd
[130295.918801] usb usb2-port1: Cannot enable. Maybe the USB cable is bad?
[130295.918838] usb usb2-port1: config error
[130296.672784] usb usb2-port1: config error
[130296.689013] usb 1-1: new high-speed USB device number 43 using xhci_hcd
```

# DMESG 2

```
[141225.500115] scsi 3:0:0:0: Direct-Access     EMMC     USB3.0      Disk  WCH PQ: 0 ANSI: 2
[141225.501250] sd 3:0:0:0: Attached scsi generic sg1 type 0
[141255.649566] usb 2-1: reset SuperSpeed USB device number 24 using xhci_hcd
[141255.667341] usb 2-1: LPM exit latency is zeroed, disabling LPM.
[141255.678563] cdc_acm 2-1:1.1: ttyACM0: USB ACM device
[141255.680808] cdc_acm 2-1:1.3: ttyACM1: USB ACM device
[141255.680981] sd 3:0:0:0: [sdb] 15523840 512-byte logical blocks: (7.95 GB/7.40 GiB)
[141255.832828] usb 2-1: reset SuperSpeed USB device number 24 using xhci_hcd
[141255.849930] usb 2-1: LPM exit latency is zeroed, disabling LPM.
[141255.860593] cdc_acm 2-1:1.1: ttyACM0: USB ACM device
[141255.862688] cdc_acm 2-1:1.3: ttyACM1: USB ACM device
[141255.994065] usb 2-1: reset SuperSpeed USB device number 24 using xhci_hcd
[141256.011283] usb 2-1: LPM exit latency is zeroed, disabling LPM.
```

# lsusb


```
Bus 002 Device 024: ID 1a86:fe2c QinHeng Electronics SD UART Bridge USB3 
Device Descriptor:
  bLength                18
  bDescriptorType         1
  bcdUSB               3.00
  bDeviceClass          239 Miscellaneous Device
  bDeviceSubClass         2 
  bDeviceProtocol         1 Interface Association
  bMaxPacketSize0         9
  idVendor           0x1a86 QinHeng Electronics
  idProduct          0xfe2c 
  bcdDevice            0.01
  iManufacturer           1 WCH
  iProduct                2 SD UART Bridge USB3 
  iSerial                 3 0123456789
  bNumConfigurations      1
  Configuration Descriptor:
    bLength                 9
    bDescriptorType         2
    wTotalLength       0x00d4
    bNumInterfaces          5
    bConfigurationValue     1
    iConfiguration          0 
    bmAttributes         0x80
      (Bus Powered)
    MaxPower              800mA
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        0
      bAlternateSetting       0
      bNumEndpoints           2
      bInterfaceClass         8 Mass Storage
      bInterfaceSubClass      6 SCSI
      bInterfaceProtocol     80 Bulk-Only
      iInterface              0 
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x82  EP 2 IN
        bmAttributes            2
          Transfer Type            Bulk
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0400  1x 1024 bytes
        bInterval               0
        bMaxBurst               0
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x03  EP 3 OUT
        bmAttributes            2
          Transfer Type            Bulk
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0400  1x 1024 bytes
        bInterval               0
        bMaxBurst               3
    Interface Association:
      bLength                 8
      bDescriptorType        11
      bFirstInterface         1
      bInterfaceCount         2
      bFunctionClass          2 Communications
      bFunctionSubClass       2 Abstract (modem)
      bFunctionProtocol       1 AT-commands (v.25ter)
      iFunction               0 
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        1
      bAlternateSetting       0
      bNumEndpoints           1
      bInterfaceClass         2 Communications
      bInterfaceSubClass      2 Abstract (modem)
      bInterfaceProtocol      1 AT-commands (v.25ter)
      iInterface              0 
      CDC Header:
        bcdCDC               1.10
      CDC Call Management:
        bmCapabilities       0x00
        bDataInterface          2
      CDC ACM:
        bmCapabilities       0x02
          line coding and serial state
      CDC Union:
        bMasterInterface        1
        bSlaveInterface         2 
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x81  EP 1 IN
        bmAttributes            3
          Transfer Type            Interrupt
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0400  1x 1024 bytes
        bInterval               8
        bMaxBurst               0
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        2
      bAlternateSetting       0
      bNumEndpoints           2
      bInterfaceClass        10 CDC Data
      bInterfaceSubClass      0 
      bInterfaceProtocol      0 
      iInterface              0 
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x84  EP 4 IN
        bmAttributes            2
          Transfer Type            Bulk
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0400  1x 1024 bytes
        bInterval               0
        bMaxBurst               0
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x04  EP 4 OUT
        bmAttributes            2
          Transfer Type            Bulk
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0400  1x 1024 bytes
        bInterval               0
        bMaxBurst               0
    Interface Association:
      bLength                 8
      bDescriptorType        11
      bFirstInterface         3
      bInterfaceCount         2
      bFunctionClass          2 Communications
      bFunctionSubClass       2 Abstract (modem)
      bFunctionProtocol       1 AT-commands (v.25ter)
      iFunction               0 
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        3
      bAlternateSetting       0
      bNumEndpoints           1
      bInterfaceClass         2 Communications
      bInterfaceSubClass      2 Abstract (modem)
      bInterfaceProtocol      1 AT-commands (v.25ter)
      iInterface              0 
      CDC Header:
        bcdCDC               1.10
      CDC Call Management:
        bmCapabilities       0x00
        bDataInterface          4
      CDC ACM:
        bmCapabilities       0x02
          line coding and serial state
      CDC Union:
        bMasterInterface        3
        bSlaveInterface         4 
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x85  EP 5 IN
        bmAttributes            3
          Transfer Type            Interrupt
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0400  1x 1024 bytes
        bInterval               8
        bMaxBurst               0
    Interface Descriptor:
      bLength                 9
      bDescriptorType         4
      bInterfaceNumber        4
      bAlternateSetting       0
      bNumEndpoints           2
      bInterfaceClass        10 CDC Data
      bInterfaceSubClass      0 
      bInterfaceProtocol      0 
      iInterface              0 
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x86  EP 6 IN
        bmAttributes            2
          Transfer Type            Bulk
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0400  1x 1024 bytes
        bInterval               0
        bMaxBurst               0
      Endpoint Descriptor:
        bLength                 7
        bDescriptorType         5
        bEndpointAddress     0x06  EP 6 OUT
        bmAttributes            2
          Transfer Type            Bulk
          Synch Type               None
          Usage Type               Data
        wMaxPacketSize     0x0400  1x 1024 bytes
        bInterval               0
        bMaxBurst               0
Binary Object Store Descriptor:
  bLength                 5
  bDescriptorType        15
  wTotalLength       0x0016
  bNumDeviceCaps          2
  USB 2.0 Extension Device Capability:
    bLength                 7
    bDescriptorType        16
    bDevCapabilityType      2
    bmAttributes   0x00000006
      BESL Link Power Management (LPM) Supported
  SuperSpeed USB Device Capability:
    bLength                10
    bDescriptorType        16
    bDevCapabilityType      3
    bmAttributes         0x00
    wSpeedsSupported   0x000e
      Device can operate at Full Speed (12Mbps)
      Device can operate at High Speed (480Mbps)
      Device can operate at SuperSpeed (5Gbps)
    bFunctionalitySupport   1
      Lowest fully-functional device speed is Full Speed (12Mbps)
    bU1DevExitLat           0 micro seconds
    bU2DevExitLat           0 micro seconds
Device Status:     0x0001
  Self Powered
```


# Claude PArt 1

Your log actually tells a fairly clear story, and the good news is that the part you were probably worried about — the composite descriptor — is *not* your problem.

## What the log shows

The device enumerates and all three functions bind successfully:

- `2-1:1.0` → USB Mass Storage (eMMC, detected as a 7.95 GB SCSI removable disk)
- `2-1:1.1` → `ttyACM0`
- `2-1:1.3` → `ttyACM1`

So the IAD/interface layout (interface 0 = MSC, 1+2 = CDC-ACM #0, 3+4 = CDC-ACM #1) is being accepted by the host. `cdc_acm` and `usb-storage` both attach. Your composite logic is essentially working.

The failure is a **SuperSpeed (USB3) link collapse**, not a descriptor or composite problem. The tells, in order:

1. `LPM exit latency is zeroed, disabling LPM` — the device's BOS descriptor reports U1/U2 exit latencies of 0, which is invalid. The kernel only enables USB3 link PM if those exit latencies are set in the BOS descriptor, so it's flagging your descriptor as malformed.
2. ~45 seconds of working operation, then `Timeout while waiting for setup device command`, `device not accepting address 2, error -62` (that's `-ETIME`), `config error`, and `Cannot enable. Maybe the USB cable is bad?` — this is the classic signature of a SuperSpeed link that drops into Recovery/SS.Inactive and can't retrain.
3. The device then falls back to high-speed re-enumeration on bus `1-1` (the USB2 side of the same connector) and keeps cycling (device numbers 39, 43…).

The ~45 s "works then dies" pattern points at one of two things: link power management kicking in after the link goes idle, or the eMMC/SCSI path stalling USB servicing long enough that the host times out (`Asking for cache data failed` and the very delayed READ CAPACITY suggest the mass-storage handler is slow/blocking).

## Most likely root causes

**USB3 link power management (U1/U2).** The zeroed exit latency is a strong hint your firmware's U1/U2 handling is wrong. This is a well-trodden failure mode — others with unstable USB3 peripherals have found that simply disabling low-power link states made the link stable. One Infineon SuperSpeed peripheral case was fixed specifically by disabling low power mode. Note that many SuperSpeed devices simply reject U1/U2 entry by responding LXU (link reject) rather than supporting it, which is a perfectly valid strategy for a device that doesn't implement it correctly.

**Blocking eMMC access starving the USB controller.** If your BOT (Bulk-Only Transport) handler busy-waits on the eMMC read/write, it can starve EP0 control servicing and the bulk endpoints, producing exactly the setup-command timeout and `-62` you see.

**SuperSpeed PHY power / signal integrity.** The CH569's SS PHY is sensitive; the literal "Maybe the USB cable is bad" is also literal advice.

## What to explore (cheap host-side tests first)

- **Force USB2 and see if it's rock-solid.** Plug into a USB2 port or USB2 hub. If high-speed is stable and only SuperSpeed dies, you've narrowed it to the SS link/PHY/firmware, which is most of the battle.
- **Disable USB3 LPM and retest.** The per-port `.../power/usb3_hardware_lpm_u1` and `usb3_hardware_lpm_u2` files control U1/U2; the `usb3_lpm_permit` attribute takes values "0" (neither), "u1", "u2", or "u1_u2" and works both before and after enumeration. Set it to `0` and see if the link stays up. If it does, your fix is in firmware (BOS descriptor + U1/U2 handling).
- **Dump the descriptors:** `sudo lsusb -v -d 1a86:fe2c`. Check the BOS / SS USB Device Capability descriptor's U1/U2 exit-latency fields (the kernel already told you they're zero), and the SuperSpeed Endpoint Companion descriptors (`bMaxBurst`, `bmAttributes`). A burst value the firmware doesn't actually honor will cause link errors.
- **Capture the failing exchange:** load `usbmon` (Wireshark) and enable xHCI debug: `echo 'module xhci_hcd =p' > /sys/kernel/debug/dynamic_debug/control`. That shows you precisely which command times out before the disconnect.
- **Swap cable and use a rear motherboard port**, not a front-panel header or hub.

## Firmware-side

- **Fix the BOS descriptor:** put non-zero, realistic U1/U2 device exit latencies in the SS USB Device Capability descriptor — or, more robustly, have the device decline device-initiated U1/U2 entirely (reject with LXU) so the host never parks the link in a state your PHY can't exit.
- **Make the eMMC/MSC path non-blocking** so a long flash transaction never stalls EP0 or the CDC endpoints.
- **Confirm it's not a firmware watchdog reset** — the device reappears with a fresh address, which can mean it's re-attaching rather than just losing the link.
- **Check the SS PHY analog supply decoupling and reference clock**, since brownout or an out-of-spec clock under combined eMMC + USB3 load will collapse SuperSpeed retraining.

One concrete reference worth studying: **HydraUSB3** is an open-source CH569 USB3 firmware/dev-kit. It implements a bare-metal device stack in bulk mode with burst for USB3 SuperSpeed and reaches >330 MB/s to a PC host, so its descriptor set and SS link bring-up are a good known-good comparison for yours.

If you can paste the output of `lsusb -v` for the device (the BOS and endpoint companion sections especially), I can point at the specific descriptor fields that are likely off.

# Claude part 2


```
This is very useful — and it changes my read of the problem a bit, so let me correct course on one thing.

## The descriptor is structurally fine

The composite layout is valid: Misc/IAD device class (239/2/1), two IADs each grouping a CDC control + data interface, MSC on interface 0. `bMaxPacketSize0 = 9` is correct for SuperSpeed (2⁹ = 512). All bulk endpoints are 1024-byte as required at 5 Gbps. The host accepted all of it, which matches the clean enumeration in your log. So nothing here is "malformed enough to be rejected."

But there are several real defects, and one of them lines up with *when* your link actually dies.

## What I'd revise from before

The BOS descriptor confirms `bU1DevExitLat = 0` and `bU2DevExitLat = 0`, and `bmAttributes = 0x00` (no LTM). That's exactly what produced the `LPM exit latency is zeroed, disabling LPM` message — **and it means the host already turned U1/U2 hardware LPM off.** So link power management is *not* the live cause of your mid-session collapse, the way I leaned earlier. The host isn't parking your link in U1/U2 at all. You should still fix those fields (put realistic values, e.g. U1 ≤ 10 µs, U2 ≤ 2047 µs) for correctness, but it won't be what's killing the link.

## The defect that correlates with the failure

Look at the mass-storage bulk endpoints' burst settings:

- EP `0x82` (Bulk **IN**, device→host, i.e. disk **reads**): `bMaxBurst = 0`
- EP `0x03` (Bulk **OUT**, host→device, disk writes): `bMaxBurst = 3`

That asymmetry is a red flag. It strongly suggests the descriptor table was hand-edited and the IN/OUT companions don't reflect a single coherent configuration. Now line that up with your dmesg timeline: enumeration and driver binding (almost no bulk traffic) succeed, and the link only collapses ~45 s later — precisely during the window where the SCSI layer is hammering the **read** path (`Asking for cache data failed`, then a long stall before `[sdb] … 7.40 GiB` finally prints). The endpoint that's exercised there is the burst-0 Bulk-IN.

The classic mechanism: if the firmware's *actual* hardware burst configuration for an endpoint disagrees with what the descriptor advertises, the device emits more (or fewer) data packets per burst than the host's xHCI scheduler expects. At 5 Gbps that desyncs the link, it drops into Recovery, fails to retrain, and you get exactly your signature — `Timeout while waiting for setup device command`, `error -62`, `config error`, `Cannot enable`, then USB2 fallback re-enumeration.

So my prime suspect is now: **the MSC bulk-IN (read) path — either a descriptor/hardware burst mismatch, or the eMMC read stalling the controller mid-burst.**

## Other descriptor issues worth cleaning up

- **Power flags are inconsistent.** Config `bmAttributes = 0x80` says bus-powered, but `Device Status = 0x0001` says self-powered, and you advertise `MaxPower 800 mA`. Pick one and be consistent. If it's genuinely bus-powered at 800 mA, a VBUS sag when the eMMC and SS PHY spike together is a plausible secondary trigger for link loss.
- **CDC notification endpoints are oversized.** EP `0x81`/`0x85` are interrupt-IN with `wMaxPacketSize = 1024` and `bInterval = 8`. A CDC ACM serial-state notification is normally 8–16 bytes. 1024 is legal but reserves a lot of periodic bandwidth for two endpoints you barely use. Not a crash cause; just wasteful.

## What I'd do next, in order

1. **Make the MSC burst settings symmetric and verify firmware honors them.** Set both EP `0x82` and EP `0x03` to the same `bMaxBurst`, and confirm the value in the descriptor exactly matches the burst configuration register you program for that endpoint in firmware. This is the cheapest high-probability fix.

2. **Test reads with bursting disabled end-to-end.** Set `bMaxBurst = 0` on the MSC endpoints *and* configure the hardware to burst 0. If the link stops dying under read load, you've confirmed it's a burst-handling problem and can then re-introduce bursting carefully.

3. **Check whether the eMMC read blocks the SS controller.** If your BOT read handler busy-waits on eMMC before feeding the bulk-IN endpoint, the controller can stall mid-burst. Make sure endpoint servicing isn't blocked by long flash transactions.

4. **Confirm it's an I/O-triggered link drop, not idle.** Quick test: enumerate but *don't* mount or read `/dev/sdb`. Just open `ttyACM0` and push UART traffic for a couple of minutes. If the link survives indefinitely with no storage I/O, that isolates the fault to the MSC path rather than the SS PHY in general.

5. **Then fix the cosmetic-but-real descriptor bugs:** non-zero U1/U2 exit latencies, consistent power flags, smaller notification endpoints.

If you can capture a `usbmon`/Wireshark trace of the moment it dies — specifically the last successful bulk-IN burst and the first link-recovery event — that would confirm the burst-mismatch theory directly. Want me to sketch what a healthy vs. failing bulk-IN burst should look like in that trace so you know what you're looking for?
```

```
sudo modprobe usbmon
sudo modprobe usbmon
ls /sys/kernel/debug/usb/usbmon/      # confirms it's live; needs debugfs mounted
```




## Claude 3 abotu USBMON

A pcapng is a binary capture — the right tool is `tshark`. Let me check it's there and inspect the file.No tshark, and installing Wireshark here is heavy. I'll parse the pcapng directly — first let me confirm the link type so I decode the per-packet headers correctly.Link type 220 = Linux usbmon (mmapped), 2189 packets. Let me decode the usbmon headers and summarize.This capture rewrites the diagnosis — and it clears the MSC burst theory. Let me match submit/complete pairs by URB id to be rigorous about what's actually stalling.Your `-104` was a symptom, not the cause — and this capture finally shows the actual failure. I need to walk back the MSC-burst theory: the trace doesn't support it. The problem is on **EP0, the control endpoint**, and it's a firmware servicing stall.

## What the trace proves

I matched every submit to its completion by URB id and measured the duration. Here's the spine of it (device 29 = your CH569):

```
 t_submit  dur(s)  xfer  ep  status      request
   0.000    5.035  CTRL  0   ENOENT      GET_DESCRIPTOR (Device)
   4.524    5.119  CTRL  0   ENOENT      SET_CONTROL_LINE_STATE  if=1
   5.036    5.120  CTRL  0   ENOENT      GET_DESCRIPTOR (Config)
   9.643    5.120  CTRL  0   ENOENT      SET_LINE_CODING         if=1
  23.998    5.101  CTRL  0   ENOENT      SET_CONTROL_LINE_STATE  if=1
  29.100    5.119  CTRL  0   ENOENT      SET_CONTROL_LINE_STATE  if=3
```

**Every single control transfer runs for ~5.0–5.1 seconds and is then killed. Not one completes.** That 5-second figure is exactly the kernel's control-message timeout (`USB_CTRL_GET/SET_TIMEOUT`). So the device is accepting the SETUP at the link layer but never completing the transfer — EP0 just goes silent and the host gives up five seconds later, over and over.

Two things this kills off immediately:

- **It is not a SuperSpeed link / PHY / LPM problem.** The link stays up the whole time; the host keeps successfully *issuing* SETUPs. A PHY collapse wouldn't let you cleanly time out six control transfers in a row.
- **It is not the MSC read-burst mismatch I flamed about earlier.** Even a plain `GET_DESCRIPTOR(Device)` hangs here. The fault is independent of which request it is, which means it's not in any one class handler — it's EP0 servicing itself.

And it's not request-specific: a standard descriptor read and the CDC class requests fail identically.

## The cascade that produced your -104

Once EP0 wedges, here's the teardown (all downstream symptoms):

- The queued CDC bulk-IN reads (`ep4 IN`, 2048 B each) and the notification endpoints (`ep1`, `ep5 IN`) sit unanswered, then come back `ESHUTDOWN`/`ENOENT` when the host resets the device.
- The one MSC read (`ep2 IN`) and several `ep6 OUT` writes get asynchronously unlinked → **`ECONNRESET` (-104)**.

So `-104` never came from the wire. It's what the host stamps on URBs it cancels when it tears the device down after the control timeouts. The earlier `error -62` and "device not accepting address" in dmesg are the same reset escalating.

One more detail worth noting: two 1-byte CDC bulk-OUT writes *did* complete OK (`t=14.764`, dur 0.000) right in the middle of this. So the bulk path momentarily works while EP0 is dead. That's the fingerprint of a **single-threaded USB service path where one endpoint's handler blocks** — bulk gets a slice in, then something stalls the loop and EP0 (which needs servicing every transfer) starves.

## What to actually investigate now

The question is no longer "why does SuperSpeed drop" — it's **"what blocks the firmware's USB service path so EP0 stops completing transfers."** Concretely:

1. **Put a scope on a GPIO toggled at the top of your USB interrupt handler (or main USB poll loop).** Open `/dev/ttyACM0` and watch. If the toggling stops, your service routine is hanging — find the blocking call. If it keeps toggling but EP0 still never completes, it's a control-FSM logic bug (next item).

2. **Audit the EP0 control state machine for the status stage.** After handling `SET_LINE_CODING` / `SET_CONTROL_LINE_STATE`, does the firmware arm EP0 for the zero-length status ACK and re-arm EP0-RX afterward? A missing or mis-armed status stage is the canonical "control transfers hang forever" bug. Since `GET_DESCRIPTOR` also hangs, also check that nothing leaves EP0's transfer-length/DMA registers in a stuck state.

3. **Bisect by removing functions.** Build a **CDC-only** image (no MSC/eMMC) and open the ports. If EP0 stays healthy, the eMMC/MSC handler is blocking the shared USB service (e.g., a busy-wait on a flash operation inside or behind the USB ISR). If it still wedges, the fault is in the CDC/control path itself.

4. **Check for a fault/watchdog.** Confirm the firmware isn't taking a hard fault around the moment the host drives traffic — a half-alive USB core (link up, no transfers completing) is consistent with the MCU spinning in a fault handler or a stuck loop.

If you grab a scope trace of that ISR heartbeat against the moment you `open()` the port, that single observation will tell you whether you're chasing a hang (item 1/3) or a control-FSM bug (item 2) — and I can help you read it.