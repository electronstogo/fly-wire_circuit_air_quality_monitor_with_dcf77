# air_quality_monitor_prj_4


Flywire circuit that shows different measurement values to rate the indoor air quality (relative humidity, temperature, pressure, VOC index from Sensirion). In addition a DCF77 receiver supports a RTC module with the precise time after start.
To see what is meant, best way is to look at the images :).

This circuit uses a 1.3 inch OLED display to show the following values:
-----------------------------------------------------------------------

- VOC index from Sensirion
- Relative humidity [%]
- Temperature [°C]
- Pressure [mbar]
- Time [hh:mm:ss]
- Week day
- Date [dd.mm.yyyy]


Components in use:
------------------

- Sensirion SGP40 sensor
- Bosch BME280 sensor
- DS3231 RTC modul
- 1.3 inch OLED modul
- Arduino Nano
- 1 x electronic switch (to switch between 2 possible views on the display).
- DCF77 reiceiver + electrolytic capacitor to stabilize the voltage supply for open collector circuit.
- Wires with different profiles
