# Calibration Logger

Interactive logger for distance-labeled sweeps. It discovers the four scanner nodes by reading their scan headers, reboots/synchronizes them, and saves both averaged and raw CSVs.

You must use the debug scanner fimrware for this tool.

Feel free to customize this script to your need.

You likely don't need as much calibration data as I collected. I initially planned on using this to train a small NN to do calibration but ended up doing a calibration bias table for simplicity and for fear of overfitting the NN to a calm RF environment. 

Once you've collected calibration data, check out the calibration steps in the docs. You can also give the bias table from my usermod and your calibration logger output to an LLM to generate a bias table fit to your data.

It's best to calibrate in the type of environment you plan on using it in.

## Run
```bash
python calibration.py
```
Follow the prompts (choose distance, rotate target, press Enter to log each angle).

Outputs are written under `calibration_runs/` as:
- `<distance>_run_<timestamp>.csv` (averaged)
- `<distance>_run_<timestamp>_raw.csv` (raw lines per scanner)