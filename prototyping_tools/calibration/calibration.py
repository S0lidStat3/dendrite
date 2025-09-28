import serial
import serial.tools.list_ports
import time
import csv
from datetime import datetime
import os
import threading
import queue
import re

# === CONFIGURATION ===
LOG_DURATION = 5  # seconds per angle
ANGLE_STEP = 10   # degrees between each measurement
OUTPUT_DIR = 'calibration_runs'
BAUD_RATE = 115200
DIRECTION_LABELS = ["North", "East", "South", "West"]
DIRECTION_MATCH = {"NORTH": "North", "EAST": "East", "SOUTH": "South", "WEST": "West"}
SYNC_TOLERANCE_MS = 100  # max allowable timestamp offset between scanners after reboot

DISTANCE_LABELS = {
    '20in': '20 inches',
    '60in': '60 inches',
    '158in': '158 inches',
    '394in': '394 inches'
}

def find_scanner_ports(timeout=5):
    print("🔍 Scanning serial ports for BLE scanner nodes...")
    ports = list(serial.tools.list_ports.comports())
    label_map = {}
    matched_labels = set()

    for port in ports:
        try:
            ser = serial.Serial(port.device, BAUD_RATE, timeout=1)
            ser.flushInput()
            ser.write(b"\n")  # nudge prompt
            found = False
            start_time = time.time()

            while time.time() - start_time < timeout and not found:
                line = ser.readline().decode(errors='ignore').strip()
                if line:
                    for key in DIRECTION_MATCH:
                        if key in line and DIRECTION_MATCH[key] not in matched_labels:
                            direction = DIRECTION_MATCH[key]
                            label_map[direction] = port.device
                            matched_labels.add(direction)
                            print(f"✅ Found {direction} on {port.device}")
                            found = True
                            break
            ser.close()
        except Exception as e:
            print(f"⚠️ Could not read from {port.device}: {e}")

    if len(label_map) < 4:
        print("⚠️ Warning: Not all scanners found. Proceeding with available ones.")
    return label_map

def reboot_and_sync_scanners(scanner_ports):
    print("🔀 Sending REBOOT command to all scanners...")
    reboot_times = {}
    pattern = re.compile(r"Starting BLE scan\.\.\. \[(\w+)] \| (\d+)")

    serial_objects = {}
    for direction, port in scanner_ports.items():
        try:
            ser = serial.Serial(port, BAUD_RATE, timeout=1)
            serial_objects[direction] = ser
        except Exception as e:
            print(f"❌ Failed to open {direction} ({port}): {e}")

    for direction, ser in serial_objects.items():
        try:
            ser.write(b"REBOOT\n")
        except Exception as e:
            print(f"❌ Failed to send REBOOT to {direction}: {e}")

    time.sleep(3)

    print("⏱️ Verifying reboot timestamps...")
    for direction, ser in serial_objects.items():
        try:
            start_time = time.time()
            while time.time() - start_time < 3:
                line = ser.readline().decode(errors='ignore').strip()
                match = pattern.search(line)
                if match:
                    label = DIRECTION_MATCH.get(match.group(1).upper())
                    millis = int(match.group(2))
                    reboot_times[label] = millis
                    print(f"🛁 {label} rebooted at t = {millis} ms")
                    break
        except Exception as e:
            print(f"❌ Failed to read reboot from {direction}: {e}")
        finally:
            ser.close()

    if len(reboot_times) != len(scanner_ports):
        print("⚠️ Warning: Some scanners did not report a reboot timestamp.")

    timestamps = list(reboot_times.values())
    if timestamps:
        min_t = min(timestamps)
        max_t = max(timestamps)
        delta = max_t - min_t
        if delta > SYNC_TOLERANCE_MS:
            print(f"⚠️ Timestamps out of sync by {delta} ms")
        else:
            print(f"✅ All scanners synchronized within {delta} ms")
    else:
        print("⚠️ Could not verify sync timestamps.")

def parse_rssi_line(line, direction):
    if line.startswith("MAC:") and "RSSI:" in line:
        try:
            rssi_str = line.split("RSSI:")[-1].strip()
            return direction, int(rssi_str)
        except ValueError:
            return None, None
    return None, None

def scan_worker(direction, port, duration, result_queue, raw_writer, distance_label, angle):
    rssi_list = []
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        start_time = time.time()
        while time.time() - start_time < duration:
            line = ser.readline().decode(errors='ignore').strip()
            if not line:
                continue
            timestamp = datetime.now().isoformat()
            raw_writer.writerow([timestamp, distance_label, angle, direction, line])
            label, rssi = parse_rssi_line(line, direction)
            if label == direction and isinstance(rssi, int):
                rssi_list.append(rssi)
        ser.close()
    except Exception as e:
        print(f"❌ Error reading from {port} ({direction}): {e}")
    result_queue.put((direction, rssi_list))

def get_new_filenames(distance_label):
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    base = os.path.join(OUTPUT_DIR, f"{distance_label}_run_{timestamp}")
    return base + ".csv", base + "_raw.csv"

def run_calibration(distance_label, scanner_ports):
    avg_filename, raw_filename = get_new_filenames(distance_label)
    print(f"📁 Saving averaged data to: {avg_filename}")
    print(f"📁 Saving raw scan log to: {raw_filename}")

    reboot_and_sync_scanners(scanner_ports)

    with open(avg_filename, 'w', newline='') as avg_file, open(raw_filename, 'w', newline='') as raw_file:
        avg_writer = csv.writer(avg_file)
        raw_writer = csv.writer(raw_file)

        avg_writer.writerow(["timestamp", "distance", "angle", "North", "East", "South", "West"])
        raw_writer.writerow(["timestamp", "distance", "angle", "direction", "raw_line"])

        angle = 0
        while angle < 360:
            input(f"\n➡️  Ready to log at {angle}° for distance '{DISTANCE_LABELS[distance_label]}'? Press [Enter] when beacon is in place...")
            result_queue = queue.Queue()
            threads = []

            for direction, port in scanner_ports.items():
                t = threading.Thread(target=scan_worker, args=(
                    direction, port, LOG_DURATION, result_queue, raw_writer, distance_label, angle))
                t.start()
                threads.append(t)

            for t in threads:
                t.join()

            results = {label: [] for label in DIRECTION_LABELS}
            while not result_queue.empty():
                direction, rssi_values = result_queue.get()
                results[direction] = rssi_values

            averaged = {
                k: sum(v)/len(v) if v else -100
                for k, v in results.items()
            }

            print("📈 Averaged RSSI:")
            for label in DIRECTION_LABELS:
                print(f"  {label}: {averaged[label]:.2f}")

            avg_writer.writerow([
                datetime.now().isoformat(),
                DISTANCE_LABELS[distance_label],
                angle,
                averaged["North"],
                averaged["East"],
                averaged["South"],
                averaged["West"]
            ])

            print(f"✅ Logged {angle}° entry.")
            angle += ANGLE_STEP

    print(f"\n🎉 Calibration sweep complete.")
    print(f"  ➔ Averaged data: {avg_filename}")
    print(f"  ➔ Raw scan data: {raw_filename}")

def main():
    print("=== BLE Direction Calibration Logger ===")
    scanner_ports = find_scanner_ports()
    if not scanner_ports:
        print("❌ No scanner nodes found. Exiting.")
        return

    while True:
        print("Available distances:")
        for key, label in DISTANCE_LABELS.items():
            print(f"  - {key} => {label}")
        distance_label = input("Enter distance key for this sweep: ").strip().lower()
        if distance_label not in DISTANCE_LABELS:
            print("❌ Invalid label. Please choose one from the list above.")
            continue

        run_calibration(distance_label, scanner_ports)

        again = input("\n🔁 Do you want to perform another sweep? (y/n): ").strip().lower()
        if again != 'y':
            break

    print("👋 Done! You can now process your calibration data.")

if __name__ == "__main__":
    main()