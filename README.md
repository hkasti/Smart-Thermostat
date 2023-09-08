# Smart Thermostat
This project is an arduino project that helps control your house cooler and heater devices using a phone application.
## Hardware
The application connects to esp32 via wifi. This project also includes pieces like esp32, DHT11, PIR sensor, OLED lcd, 5v Relays as cooler and heater. If no motion is detected from the PIR sensor then automatically the cooler and heater stop after a few minutes. If the user needs the room temperature to be a specific temperature, they can set it on the app and the cooler and heater will be adujsted accordingly. The information is exchanged via a MQTT server. Replace the wifi user and pass with your own and also update the mqtt server information or simply comment the lines.

![madar](https://github.com/hkasti/smart-thermostat/assets/45814369/9cf54f3d-9c6d-4415-a36d-77e43e117ef9)

## Software
A simple app is implemented in Kotlin. In the app, the user can set a specific temperature. The app also shows the percentage of humidity, current temperature, if there's any movement, and whether the heater or cooler is on.
