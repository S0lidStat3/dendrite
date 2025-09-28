# Python Visualization (Prototype. No really, Very prototype.)

Matplotlib-based direction tracker that parses the serial output scanner debug firmware onto a radar like interface to debug tracking and calibration. There's some filtering and smoothing built into the tracking as well to reduce jitter and reject baring changes that are too large.

This is a highly unfinished script, but it works. I was working fast and needed functionality. It worked good enough for what I needed at the time. :)

The UI provides Start/Stop, an RSSI threshold slider, and allow/block lists. The radar plot uses the default
panel angle mapping (N=45°, E=135°, S=225°, W=315°). You may have to modify this if yours is different. When I used this, I had all 4 antennas placed orthangonally and spaced about one wavelength apart.

Feel free to play with it!