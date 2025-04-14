# BLE Drone Remote ID to Meshtastic with Mesh-Mapper API 📡

Minimal **BLE-based** Drone Remote ID Scanner  
This project is a minimal scanner for BLE-based Drone Remote ID based on modified firmware supporting **OpenDroneID**. It runs on an **ESP32‑C6** (tested with the **Xiao ESP32‑C6** board) and sends parsed messages over a custom UART to a serial mesh network.

<img src="eye.png" alt="eye" style="width:50%; height:25%;">

---

## Features 🌟

- **BLE Monitoring:** Listens to BLE advertisements to capture Drone Remote ID packets in real time.
- **Protocol Support:** Decodes messages in **OpenDroneID** format.
- **Mesh Integration:** Uses UART to send compact, formatted messages to a mesh network.
- **Real-Time Mapping:** Provides a web-based interface built with the Mesh-Mapper API that:
  - Displays drone and pilot positions on a map using Leaflet and OpenStreetMap tiles.
  - Tracks movement paths automatically with unique color markers (derived from device MAC addresses).
  - Offers intuitive controls such as alias management, marker locking, and color customization.
- **Stale Data Management:** Automatically removes markers and paths if no new data is received within 5 minutes.
- **Logging & Export:** Saves each detection to a CSV file and continuously updates a KML file for offline analysis.
- **Serial Port Selection:** Presents a user-friendly interface to select the correct USB serial port for the ESP32 connection.

---

## How It Works 🔍

1. **ESP32 Firmware:**
   - **Initialization:**  
     - Configures USB Serial (115200 baud) for JSON output and Serial1 for mesh messaging.
     - Initializes the BLE stack and starts scanning for BLE advertisements.
   - **Data Capture & Parsing:**  
     - Listens for BLE advertisements and decodes Drone Remote ID packets using registered callbacks.
     - Formats the data into a minimal JSON payload including:
       - `mac`: The device MAC address.
       - `rssi`: Signal strength.
       - `drone_lat`, `drone_long`, `drone_altitude`: Drone’s GPS data.
       - `pilot_lat`, `pilot_long`: Pilot’s location data.
       - `basic_id`: A unique identifier or Remote ID.
   - **Data Transmission:**  
     - Sends the JSON payload over USB Serial to a computer running the Flask API.
     - Sends compact, formatted messages via UART (mesh messages) to integrate with the mesh network.

2. **Flask API & Mapping Interface:**
   - **Serial Port Management:**  
     - On start, prompts the user to select the USB serial port where the ESP32 is connected.
   - **Data Handling & Logging:**  
     - Receives and parses JSON data from the ESP32.
     - Remaps keys for consistency and logs each detection to a CSV file with a timestamped filename.
     - Continuously regenerates a KML file to visualize drone and pilot trajectories.
   - **Real-Time Map Visualization:**  
     - The web-based mapping interface polls the API regularly to update marker positions.
     - Displays markers for drones (🛸) and pilots (👤) and dynamically draws movement paths.
     - Incorporates user-friendly controls for locking onto specific markers, setting aliases, and adjusting colors.
   - **Mesh-Mapper Integration:**  
     - The mapping program, Mesh-Mapper, unifies real-time locations with historical data and interactive controls to enhance user experience.

---

## How to Connect and Map 🚀

1. **Connect Your ESP32:**
   - Flash the provided firmware onto your **ESP32‑C6 (such as the Xiao ESP32‑C6)**.
   - Connect the ESP32 to your computer via USB.

2. **Start the Flask API:**
   - Run the Python Flask API script.
   - Open your web browser to view the interactive map and control panel.

3. **Select Your Serial Port:**
   - The web interface will prompt you to select the correct USB serial port (corresponding to your ESP32 connection).
   - Click "Select Port" to continue.

4. **View the Map:**
   - After port selection, the map displays:
     - Real-time markers for drones and pilots.
     - Continuously updated movement paths.
     - Options to lock onto devices and adjust marker settings.
   - The interface refreshes frequently to ensure live updates.
   - Markers and paths are removed automatically if no valid data is received for more than 5 minutes.

---

## API Endpoints & Usage 🚀

The Flask API provides several endpoints:

- **GET `/api/detections`:**  
  Retrieves current detection data in JSON format for updating the map.

- **POST `/api/detections`:**  
  Accepts new detection data (from the ESP32 or for testing) and logs it.

- **GET `/api/detections_history`:**  
  Provides historical detection data in GeoJSON format for mapping.

- **GET `/api/aliases`:**  
  Returns device alias mappings stored on the server.

- **POST `/api/set_alias`:**  
  Allows setting a custom alias for a given device (by MAC address).

- **POST `/api/clear_alias/<mac>`:**  
  Clears a previously set alias for a device.

- **GET `/api/serial_status`:**  
  Indicates whether the USB serial connection is active.

- **GET `/api/paths`:**  
  Retrieves saved drone and pilot paths for persistent mapping.

---

## Drone Remote ID Firmware (ESP32) Overview 🛠️

The ESP32 firmware is the heart of the BLE scanning operation:
- **BLE Scanning:**  
  Continuously scans for BLE advertisements to detect Drone Remote ID packets.
- **Data Parsing:**  
  Decodes Remote ID messages using registered BLE callbacks and processes the OpenDroneID payload.
- **Message Transmission:**  
  - **USB JSON Output:** Sends a minimal JSON payload (containing fields like `mac`, `rssi`, GPS coordinates, and `basic_id`) over USB to the Flask API.
  - **Mesh Messaging via UART:** Sends compact, human-readable messages to a mesh network, facilitating additional integration or display options.
- **Dual Transmission Modes:**  
  - **Standard JSON Transmission:** For regular updates.
  - **Fast JSON Transmission:** For high-frequency detections, ensuring data is as real-time as possible.

---

## Installation & Setup 🚀

1. **Clone the Repository:**

   ```bash
   git clone https://github.com/yourusername/drone-remote-id-scanner.git
   cd drone-remote-id-scanner
   ```

2. **Upload the ESP32 Firmware:**
   - Open the firmware folder.
   - Build and flash the ESP32 BLE code to your device using your preferred IDE (such as PlatformIO) or command-line tools.

3. **Run the Flask API:**
   - Install the required Python dependencies:
     ```bash
     pip install -r requirements.txt
     ```
   - Run the API script:
     ```bash
     python app.py
     ```
   - The API will start and open in your default web browser.

4. **Start Scanning:**
   - Connect your ESP32 via USB.
   - Select the correct serial port from the web interface.
   - Watch as drone and pilot detections appear in real-time on the interactive map.

---

## Acknowledgments

Thanks to Cemaxacutor, Luke Switzer, and other contributors for their underlying code and support.

---

## Order a PCB for this Project

Support the project by ordering a custom PCB:
<a href="https://www.tindie.com/stores/colonel_panic/?ref=offsite_badges&utm_source=sellers_colonel_panic&utm_medium=badges&utm_campaign=badge_large">
    <img src="https://d2ss6ovg47m0r5.cloudfront.net/badges/tindie-larges.png" alt="I sell on Tindie" width="200" height="104">
</a>
