# Firmware for Managed IoT Modules (ESP32, ESP-IDF)

This repository contains the **firmware for ESP32-based IoT modules** developed as part of a master's thesis.  
The firmware is written in **C** using the **ESP-IDF framework** and is designed to be managed by the server application (Blazor Server) from the main system.

## Project Overview
This firmware implements the functionality required for **communication and integration between IoT modules and the server system**.  
Each ESP32 device connects to the server, exchanges data, and can be remotely managed.

### Features
- **Wi-Fi connectivity** (configuration and connection to a network)  
- **Bidirectional communication** with the server via **HTTP/JSON key-value messages**  
- **OTA (Over-the-Air) firmware updates** controlled by the server  
- **Remote execution and management of user-selected programs** assigned by the server  
- **Basic device management and monitoring**  

## Technologies
- ESP32 microcontroller  
- ESP-IDF framework (C)  
- HTTP communication with the server using JSON key-value data
