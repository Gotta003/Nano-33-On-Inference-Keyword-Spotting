# Nano-33-On-Inference-Keyword-Spotting
AI scalable Embedded application that recognizes gestures using an Arduino Nano 33 BLE with an external Omnidirectional Microphone Module MEMS Digital PDM. The code should work for Arduino Nano 33 BLE Sense, too, however the data given in input since they were sampled via this external module can be slightly different, leading to inaccuracy. It contains 1MB Flash CPU, For complete datasheet, pinout and schematics, check out this url: [Arduino Nano 33 BLE](https://docs.arduino.cc/hardware/nano-33-ble/).

The project was run on Linux Ubuntu 24.04 LTS with a machine with a dedicated NVIDIA GeForce RTX 5060, but the project can work using CPU too. The objective of the project was to recognize 4 classes (clap, tap, snap, silence [collapse class]) with a model that fits the device.

The pipeline followed to perform this was:
1. Environment Setup
2. Data Collection
3. MFCC Conversion using CMSIS and Training
4. Deployment on Device

The project is scalable with other audio signals and via the colab file these can be easily added, but requires careful management on deployment code with the addition of that label and modifying the number of classes.

# Table of Contents
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [IDE Setup](#ide-setup)
  - [Visual Studio Code](#visual-studio-code)
  - [Arduino IDE](#arduino-ide)
  - [Virtual Environment](#virtual-environment)
  - [Hardware Setup](#hardware-setup)
- [Project Running](#project-running)
  - [Data Acquisition](#data-acquisition)
  - [Features Extraction and Training](#features-extraction-and-training)
  - [Deployment](#deployment)
- [Contacts](#contacts)

# Project Structure
```
Nano-33-On-Inference-Keyword-Spotting
|- dataset                  # Folder containing data collected in reconstructed .wav (only numerical, not full audible format)
|- audio_capture            # Folder with code to verify if the microphone is working
|- deployment               # Folder with code to perform deployment on MCU
|- sound_model.tflite       # TFLite model generated for deployment
|- data_collection.ipynb    # Collection + Training colab code
|- packages_checker.py      # Python code packages checker
|- requirements.txt         # Requirements list for virtual environment
|- setup.sh                 # Code for setup the virtual environment
```

Note that those .wav files were not
[Go to Table of Contents](#table-of-contents)

---

# Requirements
- Arduino Nano 33 BLE Rev2 (or Arduino Nano 33 Sense BLE Rev2)
- Cable compatible
- If with non-Sense, Omnidirectional Microphone Module MEMS Digital PDM MP34DT01
- If with non-Sense, a 100 Ohm resistor to clear the clock signal
- Arduino IDE ([Download](https://support.arduino.cc/hc/en-us/articles/360019833020-Download-and-install-Arduino-IDE)) or Deploy on Arduino Cloud
- Visual Studio Code ([Download](https://code.visualstudio.com/download))
  
[Go to Table of Contents](#table-of-contents)

---

# Setup
After downloading these development IDE are the following and setuping the virtual environment for python running with local GPU, however this second part can be also done on Google Colab importing there .ipynb file.
## Visual Studio Code
Download the following extensions in the Marketplace in the application:
- Python
- Jupyter

## Arduino IDE
(The project was run on local machine, on Arduino Cloud all libraries should be already installed)
Download the following board support from Boards Manager:
- Arduino Mbed OS Nano Boards by Arduino

After the board, are required some extra libraries from library manager:
- Arduino_CMSIS-DSP by Arduino (PDM and audio processing core features)
- ArduTFLite by Chirale (Neural Network Support Library)

To verify the correctness of the library of the board, plug in the computer the MCU and go to "Tools > Board > Arduino Mbed OS > Arduino Nano 33 BLE. If the label above becomes bold it means the device is detected. Some computers may recognize the device, but not have the permission to exchange the data between the computer and that and this requires extra management. 

irst of all run this bash command that will open the permission manager:
```bash
sudo nano /etc/udev/rules.d/99-arduino.rules
```
In the environment copy and paste this:
```bash
SUBSYSTEMS=="usb", ATTRS{idVendor}=="2341", ATTRS{idProduct}=="005a", MODE="0666">
SUBSYSTEMS=="usb", ATTRS{idVendor}=="2341", ATTRS{idProduct}=="805a", MODE="0666">
```
Then perform "Ctrl + Shift + O" To Write and "Ctrl + Shift + X" to close.
To end and saving permanently these changes to always take effect run this bash:
```bash
sudo udevadm control --reload-rules && sudo udevadm trigger
```

## Virtual Environment 

This is required only if you want to create a kernel for run .ipynb file on local machine for simplicity use Google Colab.
First of all, navigate through this folder downloaded on the computer. When in the location run:
```bash
chmod +x run.sh
./run.sh
```
This will trigger the setup.sh script and it will perform the following things:
- If on a linux machine trying to install xxd library which facilitate the convertion from .tflite file to C header
- Check the python3 and pip3 installation
- Create a virtual environment in the locatin in which the bash command above was run in and activates it
- Install manually tensorflow correct version if for CPU or GPU and then all libraries in requirements.txt
- Checks if all packages were installed correctly

Then go to Visual Studio and open the .ipynb file and pressing button combination "Ctrl + Shift + P" and typing the bar that pops up "Select Interpreter" enables to enter interpreter path. To gather the correct one, run the bash code in the terminal use before, copy and paste the output and add at the end "/.tf-env/bin/python"
```bash
pwd
```

[Go to Table of Contents](#table-of-contents)

---

## Hardware Setup

![[hardware_setup.png]]