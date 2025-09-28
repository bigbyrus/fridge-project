# Smart Fridge Face & Item Recognition System

This project implements a **smart fridge** system that uses facial recognition and image capture to identify users, and keep track of their groceries. It integrates a Raspberry Pi, ESP32 camera module, DLIB facial recognition, OpenCV image processing, and a Flask web server.

## Features

- **Facial recognition** using DLIB
- **ESP32 camera** captures photos and sends to RaspberryPi
- **Flask web server** for user interface and image serving
- **Automated image capture** triggered over serial (`"Take_Photo"` command)
- **User-specific image storage** with timestamped file names

---

## System Overview

**Hardware:**
- **Raspberry Pi** – runs the main Python application, Flask server, and facial recognition program
- **ESP32 Camera (OV2640 module)** – captures images and sends them to Raspberry Pi over serial
- **External pushbutton** – physical trigger to take a picture

**Software:**
- **Python 3**
- **Flask** – web framework
- **DLIB** – face detection and recognition
- **OpenCV (cv2)** – image processing
- **NumPy** – image and face embedding data handling
- **PIL** – image formatting and Base64 conversion
- **Arduino** – ESP32 firmware

---

## Boot Process

1. **ESP32 connection** – Raspberry Pi connects to ESP32 over serial
2. **Initialize known faces** – Load LeBron image as a dummy known user, compute and store its 128-dimension face embedding
3. **Start Flask server** – Serves web UI
4. **Start background serial listener** – Thread listens for `"Take_Photo"` trigger and calls `capture()` when received

---

## Core Functionality

### `load_faces_and_encodings(directory)`
- Scans a directory for images containing faces
- Uses DLIB to detect face locations and generate embeddings
- Adds new encodings to `known_face_encodings` and usernames to `known_face_names`

---

## Image Capture Flow

### `capture()`
1. **Take photo** – calls `take_photo()`
2. **Convert BGR → RGB** – prepares image for DLIB
3. **Recognize and save** – calls `recognize_n_save(image)`
4. **Reformat image** – calls `reformat_image()` to Base64 encode the processed image
5. **Return JSON response** – sends Base64 image string to client

### `take_photo()`
- **Raspberry Pi mode** – Requests an image from ESP32 over serial (`read_image_from_serial()`)

### `read_image_from_serial(ser)`
- Instructs ESP32 to capture a frame
- Reads first 4 bytes to determine image size (little endian)
- Reads image data and converts it to a NumPy array for OpenCV decoding

### `reformat_image(image)`
- Converts NumPy image to PIL Image
- Saves to an in-memory buffer as JPEG
- Base64 encodes JPEG bytes into a string

---

## Face Recognition

### `recognize_n_save(image)`
- Resizes image for faster processing
- Converts to RGB and extracts face embeddings
- **If recognized**:
  - Logs timestamp
  - Saves image to that user’s folder
  - Draws bounding box and label
- **If unknown**:
  - Saves image to `unknown` folder
  - Labels face as `"???"`

---

## ESP32 Firmware

- **Camera initialization** – configures pins, clock frequency, frame size, and quality.
- **Serial protocol**:
  1. Receives trigger from Raspberry Pi.
  2. Captures image via `camera_fb_t` struct.
  3. Sends image size (4 bytes, little endian) followed by raw JPEG bytes.
