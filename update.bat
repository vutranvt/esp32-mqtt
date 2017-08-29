copy /Y C:\esp32\esp32-mqtt\build\esp32-audio-code-idf.bin C:\xampp\htdocs\version1.0.bin
mosquitto_pub -t hue/device1/update -m "update"