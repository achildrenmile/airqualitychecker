# AirQualityChecker
Measure the air quality with the help of BME680 in order to find out when it's time to ventilate the room.

![Good](good.jpeg)

# Thanks to
Many thanks to Rui Santos for providing the base idea https://randomnerdtutorials.com/esp32-bme680-sensor-arduino/, to G6EJD for the air quality score - https://github.com/G6EJD/BME680-Example/blob/master/ESP32_bme680_CC_demo_03.ino and to the AirSniffer project https://www.stall.biz/project/der-airsniffer-schlechte-luft-kann-man-messen.

# Current Version
This version creates a small web server which can be accessed in order to get the temperature, humidity, air pressure, gas resistance as well as the air quality. In addition, a RGB led can be used in order to visualize the current state of the room air quality.

# One word to the gas sensors
One must also point out the limits of the sensors used here: In particular, gas sensors are more qualitative sensors, with which one can very well show deterioration compared to fresh air also quantitatively, but the absolute values are to be “treated with caution”. Actually you should calibrate these sensors with fresh air as well as with a calibration gas, but you can't pay for that in the hobby area. In addition, this calibration would have to be repeated regularly because these sensors age or change over time. For the qualitative assessment of the indoor air quality and the derivation of ventilation recommendations in the smart home, these sensors are definitely sufficient. You just have to be aware of these limited properties!

# What is planned next
1) MQTT integration in order to sent the values additionally to a broker in order to persist the data etc.
2) add color configuration to web interface

