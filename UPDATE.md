### Version 2

1. A Mesh Pi can now be set up as a Classic server and accept connections from another
Mesh Pi or Windows/Android/.. Classic clients such as Bluetooth terminals. The Pi-Pi Classic connection has faster
transfer speeds than a Node connection. See documentation sections 3.4 and 3.7, and
functions: classic\_server() and register\_serial().

2. LE notifications are now supported. See functions: notify\_ctic() to enable/disable notifications,
and read\_notify() to read them. 

### Version 3

1. A Mesh Pi can now be set up as an LE server via the btferret command s or the btlib function le_server().
See documentation sections 3.6 and 4.2.17.

2. New passkey options for classic_server(). A classic connection now tries different
link key options automatically, and a remote device (e.g. Android) might now ask for confirmation that a passkey
has appeared on the Pi display. 

3. Some LE servers would disconnect immediately after connection because they needed more time
to complete the process. A connection waiting time has been added, and can be changed via set\_le\_wait().

4. The device_connected() function now returns the type of connection rather than just 0 or 1.

5. Sample code for a Blue Dot server has been included. Blue Dot is an
Android app that provides an easy way to control a Pi from a phone. See documentation section 3.10.