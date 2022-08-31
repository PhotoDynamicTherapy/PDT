# PDT: TO COMPILE AND LOAD THE PROGRAM #
########################################
ARDUINO IDE SETTINGS FOR PDT:

1st. Install the libraries below (Sketch Menu, Add Library, Add .zip library)
# Note - search web browsers for the libraries below:
## Adafruit_NeoPixel-1.10.4
## Adafruit_Sensor_Lab-0.7.1
## Adafruit_TSL2561-1.0.3
## Adafruit_Unified_Sensor-1.1.5
## LiquidCrystal_I2C

2nd. Choose ESP32 Dev Module board (Tools Menu)

3rd. After connecting the MiniUSB cable to the ESP32, disconnect the red wire connected to the 3V3 on the board.

4th. Copy the source code provided as PTD.ino into a new file on Arduino

5th. To compile the source code and load (button Load) it will be necessary to press the BOOT button located next to the connection with the MiniUSB cable on the ESP32 board.
When compiling, errors may occur, such as requesting Libraries as in the example below (Tools Menu, Library Manager):
Search the website below:
https://github.com/arduino/Arduino/wiki/Unofficial-list-of-3rd-party-boards-support-urls
 
Copy the address: https://arduino.esp8266.com/stable/package_esp8266com_index.json
Paste into the Library Manager field and click on Close

Another error identified is forgetting to disconnect the red wire connected to the 3V3 on the ESP32 board:
https://docs.espressif.com/projects/esptool/en/latest/esp32/troubleshooting.html

6th. To test, reconnect the red wire connected to 3V3 on the ESP32 board.
