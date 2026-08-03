<p align="center">
  <img src="util-images/project-poster.png" width="1000">
</p>


## Overview

The project splits responsibilities across two processors connected by USB:

- **ESP32-S3:** Configures the OV2640 image sensor and sends image data to the RaspberryPi 
- **Raspberry Pi:** Serves the web UI, runs facial recognition and object detection on the images received



## ESP32 Firmware

### Camera Configuration

The camera sensor is configured at boot using the `camera_config_t` struct from `esp_camera.h`:

- Pin definitions for I2C (SCCB) control lines and clock inputs
- **Frame size**, **pixel format**, and **grab mode** settings


During development, adjusting the frame size and pixel format allowed us to address latency issues while maintaining the image quality required for facial recognition. We ultimately sent low quality JPEG compressed image data over serial, significantly reducing latency when capturing images.

### Serial Communication

The firmware's most important task is identfying and responding to a user's input:

| Input | Response |
|---|---|
| **Serial Port** | The MCU will read incoming serial data and respond to image requests made by the RaspberryPi. Grabbing a pointer to the most recently captured frame buffer and writing the image data to the serial port. |
| **GPIO Pin 8** | When a user pushes the physical button on our PCB, the ESP32 writes `"Take_Photo"` to the Raspberry Pi, signaling it to respond with `"TRIGGER\n"` — which then triggers the image capture process described above. | 

---

## Raspberry Pi

The Raspberry Pi does the heavy lifting in this project. By integrating the Dlib an OpenCV libraries, the RaspberryPi extracts data necessary to maintain an autonomous digital inventory from the images captured by the OV2640. The facial recognition, object detection, and web server all run together on a single **main thread** while a second **background thread** is dedicated to handling `"Take_Photo"` requests from the ESP32.

### 1. Main Thread

The main thread serves the web application via Flask, handling routing and HTTP requests. When an image arrives from the ESP32:

- Using DLIB, detect face locations and extract facial embeddings, comparing them against known users for identification
- Run a Roboflow object detection model to identify groceries in the frame
- Using OpenCV, draw a rectangle around every face identified and list each user
- Push the image to every client with the root path open for display

**Creating new users** is also done in this thread. A client's POST request triggers image capture and face location/embedding extraction. Using this, store the new face under `static/user_faces` for future recognition, and create of a new directory for that user's future images.

### 2. Background Thread

This thread ultimately polls the serial port for the `"Take_Photo"` string from the ESP32. When the string arrives, this thread responds by writing `"TRIGGER"` back to the ESP32, prompting it to send the captured JPEG.

```python
def listen_for_trigger():
    global ser
    while True:
        try:
            if ser.in_waiting > 0:
                # important to protect the shared resource (serial port)
                with serial_lock:
                    ser.timeout = 0.5
                    line = ser.readline().decode('utf-8').rstrip()
                if line == "Take_Photo":
                    print("received image from the PCB")
                    perform_capture()
        except Exception as e:
            print(f"Listener error: {e}")
```

The `perform_capture()` function will uncompress the image data, allowing my application to have a full BGR pixel representation of the image as a NumPy array. Using this my application flips the array to obtain RGB, then runs facial recognition/object detection, saving the image to the digital inventory. Once the inventory is updated, the annotated image is pushed to clients so they can see the results.

### 3. Serial Port Synchronization

Both the main thread (issuing triggers from the web UI) and the background thread (listening for ESP32-initiated captures) access the same global `Serial` object. This created multiple unhandled critical sections around reads/writes to the port.

Using a lock around all serial port accesses removed all undeterministic behavior caused by race conditions
