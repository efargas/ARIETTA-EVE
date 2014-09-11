ARIETTA-EVE
===========

FTDI EVE application examples with an enhanced HAL using ARIETTA as target.

The ARIETTA G25 boards from ACME SYSTEMS (www.acmesystems.it) are based on ATMEL AT91SAM9G25 processors running Linux.
If you want to use them with the FTDI EVE processor FT800, you have to enhance the HAL to support the ARIETTA SPI port.

This source code implements a simple HAL so the orginal application examples can be used unmodified.

A short demonstration video can be watched on https://www.youtube.com/watch?v=AMdwIcSCg3Q
The ARIETTA is combined with a small daughter board built by myself implementing an USB port, housing the WiFi module for better radio performance and includes an audio codec with headphone plug.

The EVE board is connected to the two rows of IO signals comming from the ARIETTA on this board. 
