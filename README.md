# Kamstrup Multical 21 water meter in Home Assistant (Wireless MBus)

<img align="right" height="250" src="images/kamstrup_multical21.png">

### Features
 * Configuration ready for Home Assistan with MQTT.
 * Support for AES-128 decryption (with vaild key).
 * CRC Check of recived data.
 * Wireless reading of data.
 * Easy to build and configure.


### Parts 
Use these arfilliate links to support me!\
[CC1101 Module](https://s.click.aliexpress.com/e/_oDW0qJ2) \
[ESP32-C3 Super Mini](https://s.click.aliexpress.com/e/_c3HOPvoX) \
Some cables

### Wiring

| CC1101 | ESP32-C3 Super Mini |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| CSN | 7 |
| MOSI| 6 |
| MISO| 5 |
| SCK | 4 |
| GD0 | 10 |
| GD2 | Not Connected |


<img height="300" src="images/esp32_c3_mini.jpg"> <img height="300" src="images/esp32_c3_mini_pinout.jpg"> 

### Build and Upload Firmware
* Make sure you have a decryption key for your meter (you need to ask your water service provider for it).
* Read the serial number on the meter (typically S/N: XXXXXXXX/A/20, the serial number is the XXXXXXXXX part).
* Rename credentials_template.h to credentials.h and add your details.
* Compile and upload:
  - You need [VS Code](https://code.visualstudio.com/) and the [PIO Plugin](https://platformio.org/)
  - Open the project folder with the platformio.ini file (File -> Open Folder...), connect the ESP32 via USB then build and upload with Ctrl+Alt+U.

### Home Assistant

Setup [MQTT](https://www.home-assistant.io/integrations/mqtt/) if you don't already have it.

Add this to configuration.yaml
```
mqtt:
  sensor:
    - name: "Water Meter Usage"
      state_topic: "watermeter/0/sensor/mydatajson"
      unit_of_measurement: "m³"
      value_template: "{{ value_json.CurrentValue }}"
      device_class: water
      state_class: total_increasing
      availability:
        - topic: "watermeter/0/online"
          payload_available: "True"
          payload_not_available: "False"
    - name: "Water Meter Month Start Value"
      state_topic: "watermeter/0/sensor/mydatajson"
      unit_of_measurement: "m³"
      value_template: "{{ value_json.MonthStartValue }}"
      device_class: water
      state_class: total_increasing
    - name: "Water Meter Room Temperature"
      state_topic: "watermeter/0/sensor/mydatajson"
      value_template: "{{ value_json.RoomTemp }}"
      unit_of_measurement: "°C"
    - name: "Water Meter Water Temperature"
      state_topic: "watermeter/0/sensor/mydatajson"
      value_template: "{{ value_json.WaterTemp }}"
      unit_of_measurement: "°C"
```

Donation using [Ko-Fi](https://ko-fi.com/patriksretrotech) or [PayPal](https://www.paypal.com/donate/?business=UCTJFD6L7UYFL&no_recurring=0&item_name=Please+support+me%21&currency_code=SEK) are highly appreciated!

This is a based on [chester4444/esp-multical21](https://github.com/chester4444/esp-multical21).
Thanks to chester4444for his effort.
