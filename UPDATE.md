### Version 2

1. A Mesh Pi can now be set up as a Classic server and accept connections from another
Mesh Pi or Windows/Android/.. Classic clients such as Bluetooth terminals. The Pi-Pi Classic connection has faster
transfer speeds than a Node connection. See documentation sections 3.4 and 3.6, and
functions: classic\_server() and register\_serial().

2. LE notifications are now supported. See functions: notify\_ctic() to enable/disable notifications,
and read\_notify() to read them. 