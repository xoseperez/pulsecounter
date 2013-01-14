# pulsecounter

Arduino sketch that monitors pulses from a home power meter and 
reports them every minute to a server via an XBee link.

## Requirements

This sketch requires a ATMEGA328 with the Arduino booloader.
The Arduino sleeps most of the time and only awakes when en external event happens.
One interrupt pin is connected to a voltage divider with a photocell to monitor pulses of
light from a home power meter LED. When it goes HIGH the Arduino awakes, increments the counter and goes back to sleep.
The second is attached to the ON_SLEEP pin of an XBee so the Arduino awakes when the XBee awakes asserting the pin LOW.
This happens every minute. The Arduino then sends the average power consumption and the battery voltage (every 10 transmissions).
The XBee must be loaded with the END DEVICE or ROUTER AT firmware.

See the schematic to learn how to connect the different components.



