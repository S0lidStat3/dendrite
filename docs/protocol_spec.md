# Protocol Spec - Production (I2C) + Debug (Serial)


This document defines two protocol formats used by the 4 ESP32‑C3 scanner nodes on the hardhat and consumed by downstream tools:

- **Production format (I²C)** - compact binary payload for the WLED usermod on the ESP32‑S3 master.
- **Debug format (Serial)** - line‑oriented text for the calibration logger and the Python visualization tool.

The scanner firmware logic is otherwise the same (scan params, filtering), only the transport/encoding differs.

---

## Orientation & Timing (applies to both)
- **Direction identity**  
  In debug, a *direction label* (e.g. `[NORTH]`) is printed in the header for human/tools.  
  In production, the *I²C address* encodes direction (see table below).
- **Timestamp**  
  Debug headers include milliseconds since boot. Production format does not transmit a timestamp.
- **Loop cadence**  
Each scan cycle gathers results for ~1 s (configurable), then emits a frame/buffer.

> **Default tool mapping (for visualization):** the hardhat front lies between the boards labeled "`NORTH`" and "`EAST`". Parsers that render angles use: `NORTH`=45°, `EAST`=135°, `SOUTH`=225°, `WEST`=315°. You can rotate in post if you re‑mount antennas later.

---

## Production Format - I2C (for ESP32-S3 usermod)

### Bus & addressing
- **Role:** Scanner node acts as an I²C slave and the S3 master polls it.
- **Speed:** 100 kHz.
- **Pins (scanner default):** `SDA=GPIO6`, `SCL=GPIO7` (can be changed in firmware).
- **8‑bit I²C address encodes direction:**

| Direction | Address |
|---|---|
| WEST  | `0x10` |
| SOUTH | `0x11` |
| EAST  | `0x12` |
| NORTH | `0x13` |

### Commands (master > scanner)
- `0x01` **READ_COUNT** > scanner will return **1 byte**: `count` (0..4).
- `0x02` **READ_DATA** > scanner will return **count × 7 bytes** (see record layout).

*(If an unknown command is used, the scanner replies with a single zero byte.)*

### Record layout (scanner > master)
For each detected/allowed device (up to `MAX_DEVICES=4` per buffer):

```
[ MAC0 ][ MAC1 ][ MAC2 ][ MAC3 ][ MAC4 ][ MAC5 ][ RSSI ]
   6 bytes (raw BLE addr)                1 byte (signed dBm)
```

- **MAC6:** six raw bytes copied from `BLEAdvertisedDevice.getAddress().getNative()`.  
  When converting to a printable string, reverse/format as required by your BLE stack to match `XX:XX:XX:XX:XX:XX`.
- **RSSI:** signed 8‑bit integer in dBm (`int8_t` when decoded on the master).

The full response buffer is therefore:

```
count (1 byte)  +  count × [MAC6 (6 bytes) | RSSI (1 byte)]
```

### Typical polling sequence (master)
1. Write `0x01` then read 1 byte > `count`.
2. If `count > 0`: Write `0x02` then read (count × 7) bytes.
3. Repeat at ~10-20 Hz (scanner refreshes its buffer every ~50 ms after each 1 s scan).

### Filtering behavior (production)
Only devices that pass the firmware filter are packed:
- **Allowed OUIs:** `ALLOWED_OUIS[]` (prefix match on the first 3 bytes, "`XX:XX:XX`").
- **Allowed full MACs:** `ALLOWED_MACS[]` (exact match "`XX:XX:XX:XX:XX:XX`").

A device is included if it matches either list. Maximum of 4 records per buffer.

---

## Debug Format - Serial (for the calibration logger and python visualization tools)

### Header
```
Starting BLE scan... [<DIRECTION>] | <millis>
```
Example:
```
Starting BLE scan... [NORTH] | 298
```

### Device lines (0 to N per frame, duplicates may appear across frames)
```
MAC: <XX:XX:XX:XX:XX:XX> | RSSI: <-NN>
```

### Footer
```
Scan complete
```

### Serial command (optional)
- `REBOOT` (ASCII line) then node restarts. Tools may use the near‑zero `<millis>` after reboot to sanity‑check multi‑node alignment during calibration.

### Filtering behavior (debug)
- Debug firmware supports OUI filtering (`OUIS[]`) and can be toggled at compile time (e.g., `FILTER_BY_OUI`).  
- When disabled, all discovered devices are printed.

---

## Scan parameters (both builds)
- **Active scan:** disabled (passive). This was done in attempt to detect devices that do not support active scanning.
- **Window/interval:** `0x50 / 0x50` (~31 ms each).
- **Scan duration per loop:** ~1 s.

---

## Example (debug, one frame)

```
Starting BLE scan... [WEST] | 120734
MAC: AA:BB:CC:11:22:33 | RSSI: -63
MAC: BB:CC:DD:22:33:44 | RSSI: -79
Scan complete
```

## Example (production, I²C bytes)
If `count = 2` and the two devices are:

- `AA:BB:CC:11:22:33` with RSSI `-56` (0xC8 as int8)
- `BB:CC:DD:22:33:44` with RSSI `-79` (0xB1 as int8)

The readout would be:

```
[ 0x02 ]  // READ_DATA
0x02,  AA BB CC 11 22 33, C8, BB CC DD 22 33 44, B1
```
