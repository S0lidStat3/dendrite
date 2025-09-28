# Calibration Guide

This was trained with three distance bins (60 in, 158 in, 394 in (Why? Meters converted to inches and rounded.)), doing ~5 sweeps per distance. The production usermod uses a single collapsed bias table `calibration[4][36]` applied at runtime.

## Collect data (development mode)
1. Flash the debug_serial scanner firmware to your C3s.
2. Connect all four boards via USB and run your calibration logger (see `tools/calibration_logger/`).
3. For each distance (e.g., `60in`), perform 5 complete 360° sweeps. (You can get away with less, but the more data you have, the more accurate your bias table will be.) The logger will save files like:
   - `60in_run_1.csv`, `60in_run_2.csv`, ...
4. Repeat for `158in` and `394in`.

## CSV shape (typical)
Each row corresponds to a device observation in a frame:
```
ts_ms, frame_id, mac, rssi_W, rssi_S, rssi_E, rssi_N, present_W, present_S, present_E, present_N, distance_label, run_id
```

## Derive the bias table (outline)
1. **Bucket by heading** (10° buckets > 36 buckets around the circle). If heading is not directly measured, infer via vectoring: build `(vx, vy)` from per‑antenna RSSI and use `atan2`.
2. For each **antenna × bucket**, compute the **mean offset** to minimize systematic error; this becomes `calibration[ant][bucket]`.
3. Export a `float calibration[4][36]` array and embed it in the usermod.

## Notes
- RSSI is noisy. Temporal smoothing and outlier rejection help.
- If you change antennas, cables, or mount geometry, re‑calibrate.
- Active vs passive scan can impact visibility and timing; V1 stayed passive for simplicity and stability, and a foolish hope that it would pick up more devices.
- If you don't understand how to calculate the bias table, you can give the example table in the usermod and your calibration output to an LLM to help you out. It's smart to remove any outliers across sweeps in the data before building your calibration table.