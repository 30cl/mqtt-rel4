# mqtt-rel4
MQTT 

This code is written and only tested on the WEMOS D1, an arduino clone with ESP8266 wifi.

A subject can be configured via the serial interface. When the arduino receives a single character, `1` or `0`, all 4 
outputs react correspondingly.

Alternatifly 4 characters of `1`'s and `0`'s can be send, to change the state of each output individually. 

The connected outputs are: `D5`, `D6`, `D7` and `D2`.

## LED Indication

100ms on, 500ms off => Searching for WiFi connection

300ms on, 300ms off => Connecting to the MQTT broker

## Serial protocol

Connect the serial at 115200BAUD.

Send `?` to get some basic information about the device settings. Including a menu with options. Lines starting with `#` is information
provided by the system. This can be the Chip ID or the currnet IP address.

Options that can be configured, start with a `c`, followed by a number. To change for example the MQTT subject send the following command:

```
>6home/hall/lights/4
```

To store the current configuration to the EEPROM, send a single character `@`
