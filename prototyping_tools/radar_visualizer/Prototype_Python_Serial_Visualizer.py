import serial
import time
import threading
import re
import math
import matplotlib.pyplot as plt
from serial.tools import list_ports
from collections import deque, defaultdict
from matplotlib.widgets import Button, Slider, TextBox

# ==== CONFIGURATION ====
BAUDRATE = 115200
SCAN_DURATION = 1               # seconds, should match ESP scan duration
SMOOTHING_ALPHA = 0.6           # EMA smoothing factor (higher = more responsive)
MEDIAN_WINDOW = 3               # sliding-window median size (smaller = less lag)
OUTLIER_THRESHOLD_DEG = 90      # ignore jumps > this (larger = fewer rejections)
HISTORY_LENGTH = 20             # number of past positions per MAC to keep

# Actual antenna positions in inches (from top-down view)
ANTENNA_POSITIONS = {
    'NORTH': (0, 5),
    'EAST':  (5, 0),
    'SOUTH': (0, -5),
    'WEST':  (-5, 0),
}

# Regex to capture MAC, RSSI, optional Name
read_pattern = re.compile(
    r"MAC:\s*([0-9A-Fa-f:]+)\s*\|\s*RSSI:\s*(-?\d+)(?:\s*\|\s*Name:\s*(.*))?"
)

# ==== SMOOTHING UTILITIES ====
def wrap_angle(a):
    return (a + math.pi) % (2*math.pi) - math.pi

def circular_median(buf):
    xs = [math.cos(a) for a in buf]
    ys = [math.sin(a) for a in buf]
    return math.atan2(sum(ys)/len(ys), sum(xs)/len(xs))

# ==== GLOBAL STATE ====
data_queue = deque(maxlen=5000)
smoothed_angles = {}
angle_buffer = defaultdict(lambda: deque(maxlen=MEDIAN_WINDOW))
angle_history = defaultdict(lambda: deque(maxlen=HISTORY_LENGTH))
mac_names = {}

running = True
rssi_threshold = -80
allow_list = []
block_list = []

# ==== SERIAL & DEVICE FUNCTIONS ====
def detect_ports(timeout=2):
    detected = {}
    for p in list_ports.comports():
        port = p.device
        try:
            ser = serial.Serial(port, BAUDRATE, timeout=timeout)
            ser.reset_input_buffer()
            start = time.time()
            while time.time() - start < timeout:
                line = ser.readline().decode(errors='ignore')
                if 'Starting BLE scan' in line:
                    m = re.search(r"\[(\w+)\]", line)
                    if m and m.group(1) in ANTENNA_POSITIONS:
                        detected[m.group(1)] = ser
                    break
            else:
                ser.close()
        except Exception:
            continue
    return detected

def reboot_devices(serials):
    for ser in serials.values():
        try:
            ser.write(b'REBOOT\n')
        except:
            pass
    time.sleep(2)

def read_loop(direction, ser, queue):
    while True:
        line = ser.readline().decode(errors='ignore').strip()
        m = read_pattern.search(line)
        if m:
            mac = m.group(1).upper()
            rssi = int(m.group(2))
            name = m.group(3) or ''
            mac_names[mac] = name
            queue.append((direction, mac, rssi))

# ==== DIRECTION ESTIMATION WITH ANTENNA POSITION OFFSETS ====
def estimate_direction(readings):
    x = y = total_weight = 0.0
    for direction, rssi in readings.items():
        if direction not in ANTENNA_POSITIONS:
            continue
        weight = 10 ** (rssi / 20)
        dx, dy = ANTENNA_POSITIONS[direction]
        x += dx * weight
        y += dy * weight
        total_weight += weight
    if total_weight == 0:
        return 0
    centroid_x = x / total_weight
    centroid_y = y / total_weight
    return math.atan2(centroid_y, centroid_x)

# ==== GUI CALLBACKS ====
def on_start(event):
    global running
    running = True

def on_stop(event):
    global running
    running = False

def on_threshold(val):
    global rssi_threshold
    rssi_threshold = slider_thresh.val

def on_allow(text):
    global allow_list
    allow_list = [m.strip().upper() for m in text.split(',') if m.strip()]

def on_block(text):
    global block_list
    block_list = [m.strip().upper() for m in text.split(',') if m.strip()]

# ==== MAIN LOOP ====
def main():
    global slider_thresh, text_allow, text_block

    serials = detect_ports()
    if not serials:
        print("No ESP devices detected. Check connections.")
        return
    print("Detected:", list(serials.keys()))
    reboot_devices(serials)
    for direction, ser in serials.items():
        threading.Thread(target=read_loop, args=(direction, ser, data_queue), daemon=True).start()

    plt.ion()
    fig = plt.figure(figsize=(10, 8))
    gs = fig.add_gridspec(3, 3, height_ratios=[4, 0.2, 1], hspace=0.4, width_ratios=[1,1,1])
    ax = fig.add_subplot(gs[0, :], projection='polar')
    ax_table = fig.add_subplot(gs[2, :])
    ax_table.axis('off')

    btn_start = Button(fig.add_subplot(gs[1,0]), 'Start')
    btn_stop  = Button(fig.add_subplot(gs[1,1]), 'Stop')
    slider_thresh = Slider(fig.add_subplot(gs[1,2]), 'RSSI Thresh', -100, 0, valinit=rssi_threshold)
    text_allow  = TextBox(fig.add_axes([0.1, 0.07, 0.35, 0.05]), 'Allow-list')
    text_block  = TextBox(fig.add_axes([0.55, 0.07, 0.35, 0.05]), 'Block-list')

    btn_start.on_clicked(on_start)
    btn_stop.on_clicked(on_stop)
    slider_thresh.on_changed(on_threshold)
    text_allow.on_submit(on_allow)
    text_block.on_submit(on_block)

    try:
        while True:
            if not running:
                plt.pause(0.1)
                continue

            latest = defaultdict(dict)
            for d, mac, rssi in list(data_queue):
                latest[mac][d] = rssi

            ax.clear()
            table_data = []
            for mac, readings in latest.items():
                if (allow_list and mac not in allow_list) or (mac in block_list):
                    continue
                if max(readings.values()) < rssi_threshold or len(readings) < 2:
                    continue

                raw_theta = estimate_direction(readings)
                buf = angle_buffer[mac]
                buf.append(raw_theta)
                med_theta = circular_median(buf)
                prev = smoothed_angles.get(mac, med_theta)
                if abs(math.degrees(wrap_angle(med_theta - prev))) > OUTLIER_THRESHOLD_DEG:
                    med_theta = prev

                smooth = SMOOTHING_ALPHA * med_theta + (1 - SMOOTHING_ALPHA) * prev
                smoothed_angles[mac] = smooth
                angle_history[mac].append(smooth)

                hist = list(angle_history[mac])
                for i in range(len(hist)-1):
                    ax.plot([hist[i], hist[i+1]], [1,1], alpha=(i+1)/len(hist))
                ax.scatter(smooth, 1, s=80)

                table_data.append([mac, mac_names.get(mac, ''), f"{max(readings.values())}", f"{math.degrees(smooth):.0f}°"])

            ax.set_rticks([])
            ax.set_title('BLE Direction Tracker (Antenna-Aware)')

            ax_table.clear()
            ax_table.axis('off')
            if table_data:
                tbl = ax_table.table(cellText=table_data,
                                     colLabels=['MAC','Name','RSSI','Angle'],
                                     cellLoc='center', loc='center')
                tbl.auto_set_font_size(False)
                tbl.set_fontsize(10)
                tbl.scale(1,1.5)

            plt.draw()
            plt.pause(SCAN_DURATION)

    except KeyboardInterrupt:
        print("Exiting...")
    finally:
        for ser in serials.values():
            ser.close()

if __name__ == '__main__':
    main()