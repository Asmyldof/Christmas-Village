Christmas-Village
=================
A two cheap microcontroller project to control which of ten channels is on or off based on a true-random generator (with guaranteed no bias), while also controlling the general brightness either manually or from a light sensor.
The remainder of this ReadMe will be written as a chronological logbook, to show the speed at which the development was done.

How a missing connector inspired me to make a present in stead buying one:
=================

Early December 2013:
-----
I glue some LED strips into the crockery Christmas Village houses my parents bought and supplied father with some pre-fab connectors and cables. That year the houses were lit at a fraction of the cost of high-watage incandescent bulbs from the supplier. (Where the LEDstrip uses less than 1/10th the power).

Early January 2014:
-----
The houses get packed into wraps and boxes.

December 19th 2014:
-----
Father WhatsApps to ask about the connector set-up, because the old one seems to have fallen out of the package at some point. We call and agree that I will get a new connector to go from one single power supply to ten houses the next week, for the main holidays (as they were busy upto Dec 22nd)

December 20th 2014:
-----
I start designing a PCB to control the houses with the shortest-path-available method, still considering reusability, and thus splitting the dimmer and the 10 channel on/off controller into two separate, relatively cheap Atmel AVRs.
The plan:
* First and foremost: Use only items from my direct stock (My stock is large, very large even, but not limitless), so it can be built the same weekend.
* Get an ATTiny10 to do the dimming and keeping an eye on the light/dark situation
* Get an ATTiny4313 to control the channels on/off, including a true-random generator without bias (more on this later)
* Add a FM noise source for very-high-speed random generation (for future plans) (see below why that's not in there anymore)

December 21st 2014 (late night):
-----
With the hardware fully designed it's time to start the etching

December 22nd 2014 (afternoon):
-----
With the hardware fully built it's time to test the noise generation from the FM antenna. The FM antenna proves to be unreliable and insensitive in the small box that has been chosen, so a lower-frequency noise source is taken: The good old Zener Diode.

December 22nd 2014 (early evening):
-----
The ATTiny10 software is ready for testing, but a paid projct takes temporary precedence.

December 22nd 2014 (late evening):
-----
The ATTiny10 is working and installed, time to go bring what is done as a present to the parental home and continue programming there later.

December 22nd/23rd 2014 (midnight and later):
-----
Development of the random control is well underway (all chips were already mounted before ATTiny10 testing started).

December 23rd 2014 (approx 5:30 AM):
-----
Random control of the houses works, time to decouple the programmer and go to bed, allowing the random behaviour as a surprise for the folks that'll wake up soon.

December 23rd (5:30 AM) through december 26th:
-----
Break (the houses are automatically dimmed by the sensor and the light sensor sucessfully influences the number of houses on - more on during the day time than during the night time - and the random behaviour works, that was the target for christmas.
(However, some work had already been done for other functionality, including patterns and EEPROM re-configurable settings, which should later be read/write-able through the serial interface (quick after thought in the hardware design).

December 27th 2014:
-----
Continuation of the development: Adding Serial interface, interleaved with shopping, cleaning and such.
(Note, however, that the hardware is not available at this point, so all code is written and tested on paper only, undoubtedly leaving bugs to be discovered when the hardware is back again for a while.)

December 28th 2014 (2:30 AM):
-----
Serial command set almost completely implemented, before the remaining command get implemented the default eeprom_() routines from AVR-GCC need to be switched over to interrupt driven and flag-bocked, or some routine splitting needs to be done to get the blocking eeprom read/writes in the non-interrupt space inside main(). Else the "ReadPattern" and "WritePattern" commands will hold up too many interrupts.

December 28th 2014 (14:00 to 16:30):
-----
A small window of hardware testing, allowing debugging of the USART code and changing some variables into GPIOR, to decrease code size. Basic USART system working, not all commands fully tested yet.
