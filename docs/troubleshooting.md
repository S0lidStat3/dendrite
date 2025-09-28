# Troubleshooting

- **No white overlay appears**
  - Ensure your WLED segment length is **exactly 41**. The usermod only draws on that length. You can modify this if needed.
  - Confirm presets are assigned: **ID 1 = detected**, **ID 2 = idle**.
- **Overlay appears but angle is wrong / mirrored**
  - Verify I²C **address > direction** mapping: `0x10=W, 0x11=S, 0x12=E, 0x13=N`.
  - Check that the front index is set to your front-center LED (count LEDs starting from the beginning of the strip)
- **No devices detected**
  - Temporarily disable/loosen OUI and full‑MAC filters in the scanner firmware.
  - Confirm BLE is scanning passively and that the target is advertising.
- **Boot issues (S3)**
  - Double‑check the board env/partitions and flash mode for your S3.
  - You may need to modify platformio_override.ini if you're using a different board.
  - Ensure I²C is initialized safely; if watchdog resets occur, delay bus activity until WLED is up.
- **Power brownouts or PD bank sleeps**
  - Reduce LED brightness, or use a USB-C PD trigger with sufficient current. Add a small idle load if the bank sleeps.
- **Runtime drops to idle too quickly**
  - Adjust presence hysteresis in the usermod (`PRESENCE_HYSTERESIS_MS=3000`, `SWITCH_MIN_INTERVAL_MS=5000` by default).
