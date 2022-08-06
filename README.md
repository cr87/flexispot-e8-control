# flexispot-e8-control

ESP8266-based controller to raise and lower the Flexispot (Sanodesk) E8 standing desk based on a configurable schedule. 

## Status
Currently somewhat limited, my use case is mainly to raise the desk for 10-15 minutes every hour on the hour during work days, so the code is pretty tailored towards that.

## TODO
- Implement DST handling to NTP client (needs to be set manually at the moment)
- Anything to do with the webserver, doesn't really offer a lot of functionality (or robustness) right now
- General code improvements

Huge thanks to these sources for not having to reverse engineer the comms protocol:
[https://www.mikrocontroller.net/topic/493524]
[https://github.com/iMicknl/LoctekMotion_IoT]