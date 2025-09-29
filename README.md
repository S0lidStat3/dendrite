# Dendrite - a BLE direction finding hardhat

<img src="/pics/dendrite.jpeg" alt="Dendrite" style="width: 70%;">

## What is this?
Dendrite is a Bluetooth direction finding hardhat that is capable of performing signal strength based direction estimation of BLE devices. Its main software is a WLED usermod. When it detects a device it will switch WLED presets and overlay 5 white LEDs in the direction of each device on top of the WLED animation.


## Why build this?
This was built as a visual learning tool to highlight an interesting implementation of BLE in some Axon devices such as Bodycameras and Tasers. I have intentionally left the OUI filters blank for the published version of this project. However, these three OUIs are publicly accessible: https://standards-oui.ieee.org/

- Visual learning is one of the best ways to teach, inspire, spark curiosity, and spread awareness. So, the hardhat was built to showcase BLE direction finding with relatively low cost hardware in a visual and exciting way.
- The hope is to encourage curiosity while also raising awareness of BLE’s risks when implemented in devices designed for critical, high-risk environments like those often found in security and emergency response roles, such as wearable tech, communication tools, and vehicle systems. 
- This was built purely as a novelty item and visual learning tool. Please do not use this for anything other than educational purposes. I do not condone, encourage, or promote using this in the field for any reason.

## Is it finished? Why did you do X?
- No, it is not a finished or polished project. I'll eventually make a smaller, more polished version of this.
- This was built last minute for a conference over about ~30 hours. 
When I started this project, I had little skill in the areas needed for this project, and learned a lot along the way. There is a lot I would do differently in a second version.
Most of the choices made reflect the easiest path forward at the time. There is a lot to be improved.  To give you an idea of how last minute this was, I finished it the day before the conference in a hotel room.
Currently the WLED mod only tracks one device at a time.

## What are the LEDs on top?
It's an LED Mohawk that was configured as a seperate LED segment and plays different animations.

## Disclaimer

This project is provided for educational and research purposes only. It is not a safety, security, or surveillance device and is not intended for real-world or field use.

By using this project you agree to:

- Comply with all applicable laws and policies.

- Avoid collecting or storing data about people or devices without consent.

- Use at your own risk. No warranty is provided. The authors are not liable for any damages, misuse, injury, or legal violations.

- Prohibited uses include anything outside of pure educational purposes, any activity that infringes privacy and other rights or violates laws.

### Safety notes: 
- This is a hobby build with batteries and high-brightness LEDs; risk of heat, glare, or seizure in photosensitive individuals. The author makes no guarantees to the safety of this device.

## Files & Usage
See the "Docs" folder.

[Wiring Diagram](/docs/wiring_diagram.png)

<img src="/docs/wiring_diagram.png" alt="Wiring Diagram" style="width: 70%;">

 


See also: [Protocol Spec](docs/protocol_spec.md) - [Hardware & Wiring](docs/hardware_wiring.md) - [Quickstart](docs/quickstart.md) - [Troubleshooting](docs/troubleshooting.md) - [Calibration Guide](docs/calibration_guide.md)


**Presets:** `presets/detected.json` (ID 1) and `presets/idle.json` (ID 2)


## Inspiriation

This project was inspired by some awesome creations from the hardhat brigade and this talk from Defcon 31 "DEF CON 31 - Snoop On To Them, As They Snoop On To Us - Alan Meekins":
https://www.youtube.com/watch?v=cO1JSzAdPM8

## License: CC-BY-NC-SA 4.0
In general: Hack it, remix it, show it off. If you use it, give me a shoutout. Please keep it open if you decide to share changes. Don't use this to make money. Don't use this with ill intent.
