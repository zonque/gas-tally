# Gas Tally

A little project to keep track of gas usage and log it to [InfluxDB](https://www.influxdata.com/) for vizualisation.

It's built with a D1 mini board, featuring an ESP8266 and an HMC5883 magnet sensor chip, connected via I2C.

## Monitoring

Gas meters such as the the one pictured below have a small magnet in the last (rightmost) ring which
can be detected using a magnet sensor.

![meter](https://raw.githubusercontent.com/zonque/gas-tally/main/images/meter.jpg)

## Hardware wiring

A D1 mini module (ESP8266 based) was used and connected to an HMC5883 breakout board. Other ESP8266 derivates might also work.
Connect the I2C lines (SDA and SCL) and supply the HMC5883 board with +3.3v and GND. The DRDY pin can be left unconnected.

![wiring schematics](https://github.com/zonque/gas-tally/blob/main/images/wiring.png)

The sensor is then mounted underneath the gas meter. Duct tape is your friend :)

![installed](https://github.com/zonque/gas-tally/blob/main/images/installed.jpg)

## Firmware details

The firmware will do the following upon startup:

* Initialize the HMC5883
* Connect to the configured Wifi
* Set the local time using NTP
* Connect to InfluxDB

It will then periodically (once per second) read out the sensor values and print them to the serial console.

The Z axis of the magnet sensor is further evaluated, assuming that the sensor is mounted flat on the bottom of the gas meter.

For the purposes of debouncing, its value will have to drop below the configured `MAGNET_THRESHOLD_LOW` value,
and then rise above `MAGNET_THRESHOLD_HIGH` again.

This event signifies a full turnaround of the measured ring. A data point is now logged to InfluxDB with a fixed value as
configured in `DATA_POINT_VALUE`.

## Compiling

You will need to have [PlatformIO](https://platformio.org/) set up in order to satisfy the dependency libraries and compile the firmware.

Copy `include/config.h.template` to `include/config.h` and add your configuration to the variables documented in that file.

## Influx

Use InfluxDB queries to make use of the data. Note that the data represents multiple data points with the same value each,
so you need to select the `sum` aggregation to plot meaningful graphs.

For instance, use the following query:

```
from(bucket: "gas-tally")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["_measurement"] == "usage")
  |> filter(fn: (r) => r["_field"] == "m3")
  |> filter(fn: (r) => r["sensor-id"] == "sensor-1")
  |> aggregateWindow(every: 5m, fn: sum, createEmpty: true)
  |> yield(name: "sum")
```

Here's an example of the plot that resulted from the query above:

![influx graph example](https://github.com/zonque/gas-tally/blob/main/images/influx.png)


## License

MIT