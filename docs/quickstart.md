# Quickstart (Production On‑Hat)

1. **Flash the four ESP32‑C3 scanners (production I²C‑slave)**
   - Make sure you set your desired OUI and MAC filters correctly.
   - Also make sure you set your SDA and SCL pins correctly.
   - `firmware/firmware\esp32-c3-scanner/production_i2c_slave/production_i2c_scanner.ino`
   - Set addresses: **0x10=W**, **0x11=S**, **0x12=E**, **0x13=N**.
2. **Flash ESP32‑S3 with WLED + usermod**
   - Clone WLED from its repo and extract the `firmware/esp32-s3-wled-usermod/WLED` folder on top of it. This will extract the usermod and the config files. 
   - Wire I²C: **SDA=4**, **SCL=5**, clock **100 kHz**. Change if your pins are different.
   - Build and flash with visual studio and PlatformIO.
3. **Configure the LED ring segment in WLED**
   - Segment length = X. (Length of LED Strip) **Front** = X (Front-Center LED, counted from the start of the strip.)
4. **Power up & Import presets in WLED.**
   - `presets/detected.json` > assign **ID 1** (Device detected).
   - `presets/idle.json` > assign **ID 2** (idle animation).
5. **Run**
   - The S3 polls devices, applies calibration, computes bearing, and draws a white 5‑pixel marker over the **detected** preset. Idle preset shows otherwise.

> For development/calibration instead, use the **debug_serial** scanner firmware and see `Calibration & Debugging` in the README.
