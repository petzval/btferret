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

### Version 4

1. Support for BeetleIN server. BeetleIN is an Android app that is programmed from a Pi via Bluetooth LE in
Python or C,
and allows an Android phone/tablet to be used as a touch screen input/output device. See separate repository
[github.com/petzval/beetlein](https://github.com/petzval/beetlein) for full documentation. The C interface is based on btlib.c.

2. Bug fix: LE characteristics with notify permission 10 or indicate permission 20 were not recognised.
Characteristics with indicate permission now handled correctly.
 
### Version 5

1. A new devices.txt file option ADDRESS=MATCH\_NAME. Some LE servers have a random address, so it is
not possible to set the address in the devices file. If MATCH\_NAME has been specified, the name is used
to identify a device and a scan must be run
to find the address before connection. During a scan the remote device name is compared to the DEVICE=Name specified in the
devices file, and if they match, the address is allocated to that device. See sections 3.3 (devices file) and 3.6 (LE client) in the documentation.

### Version 6

1. A further addition to the LE random address issue from Version 5. Some LE devices have a random address,
but do not change it. In this case, the address can be listed in the devices file, but it must be flagged as
random by adding RANDOM=UNCHANGED. No scan is needed. See sections 3.3 (devices file) and
3.6 (LE client) in the documentation.

### Version 7

1. The limit on LE characteristic size (for server and client) has been increased from 20 to 245.

2. Bug fix: The LE server did not respond correctly to some attribute information requests from some LE clients
(e.g. nRF Connect Desktop could not see characteristic UUIDs), and did not respond to connection parameter change requests,
which could cause a disconnection.

### Version 8

1. Bug fix. LE characteristic search and size setting did not work for some devices. The limit on size has been reduced
from 245 to 244 to deal with this. 

### Version 9

1. A new option for classic\_server (ANY\_DEVICE) that accepts a connection from any device rather than a specified node.
2. Primary service UUIDs can now be specified in a devices.txt file for LE servers.
3. File transfer - btferret can now get a file from a server as well as send.
4. File transfer. The distribution now includes a filetransfer.py file. This is a Python program
that should run on any Python/Bluetooth capable device, and is compatible with btferret's file 
transfer protocol. It can act as a classic client or server for btferret.
5. Bug fix. Under some circumstances, a Windows COM port would disconnect unexpectedly.

### Version 10
1. New functions: set\_le\_interval(), set\_le\_interval\_update(), and set\_le\_interval\_server().
Set the LE connection interval which controls the speed of multiple read/writes.
2. Bug fix. Rapid writes of large LE characteristics could sometimes crash the system.

### Version 11
1. LE server: 2-byte primary services in devices.txt, allows standard LE characteristics set-up.

### Version 12

There are no new features, but the following changes have been made. 

1. When run on Ubuntu, the code may have failed to initialise, maybe with an "Unable to read local board address"
or "No root permission or hci0 is not operational" message. A reset has been added to the initialisation
process that should fix this problem. The code has been tested on a PC running Ubuntu 22.04.3 LTS.

2. When acting as a server, the CPU is no longer 100% occupied waiting for client packets.

3. The compilation would throw up multiple signed/unsigned char warnings. They were harmless, but they have
now been eliminated by re-defining the relevant function parameters from char to unsigned char.

4. When doing a classic scan, some Bluetooth adapters did not filter out repeats, resulting in multiple FOUND
reports for a single device. Now fixed.

### Version 13

1. HID (Human Interface Device) support. An LE server can act an HID device such as a keyboard, mouse,
or joystick. Code is included (keyboard.c and keyboard.txt) that makes the Pi a 
Bluetooth keyboard for a tablet/phone/PC. See documentation section: HID Devices.

2. New function: set\_le\_random\_address(). Used by HID device to specify a random address that connecting
devices will see as an LE-only device.

3. New function: keys\_to\_callback(). Key presses are sent to the LE server callback routine.
This is used by the HID keyboard, but can also be used to locally trigger an
action by an LE server. So there are now two ways of doing this: the timer (LE\_TIMER) and a key press
(LE\_KEYPRESS). Note that the key that stops the server changes from x to ESC when this is active.

4. New function: hid\_key\_code(). Used by the HID keyboard.

5. The LE server no longer inserts empty 1800 and 1801 primary services if they do not appear
   in the devices file. Fixes a bug - when
   the devices file only specified one primary service: 1800 with characteristics.

### Version 14

1. Python interface. The library can now be compiled as a Python module. Instructions for Python
   coding have been added to the documentation.
   
2. New function le\_pair(). Implements pairing and bonding from an LE client, and authentication
   for an LE server.
   
3. Revised HID code keyboard.c to make use of le\_pair.

4. The Python capability has made it necessary to change the way pairing information is
   stored. Any existing pairing information generated with previous versions will be lost.
   
### Version 15

1. Secure Connection pairing added to le\_pair options.

2. An amusing, but probably useless program to turn a Pi into a Bluetooth mouse. Connect from a PC/tablet and
   the Pi's arrow keys move the cursor. Function keys F1/2/3 execute left/middle/right button clicks.
   See mouse.c/py. An example of HID programming.

### Version 16

1. A new option for classic\_server() and read\_node\_endchar(). These functions require a termination
   character (endchar) that signals the end of the packet. Setting endchar = PACKET\_ENDCHAR will read one
   Bluetooth packet whatever the last character is. Use this to read one packet when there is no defined
   termination character.
   
2. OBEX (Object Exchange) protocol support. This is the protocol used when most devices (Windows, Android..)
   transfer files or data. When set up as a Classic server, btferret advertises serial services as before, but
   also that it is an OBEX push server. A Windows/Android device will recognise this and transfer a file
   using the OBEX protocol. The btferret receiver must be programmed with this protocol.
   The files obex\_server.c/py and btferret.c/py do this and will receive files via the OBEX protocol.
   The obex\_client.c/py files send data to a listening Windows/Android OBEX server.
   The btferret.c/py file transfer (f) and server (s) functions now include OBEX exchange as an option.
   
3. An option in set\_flags to enable or disable the OBEX capability. Enabled by default.   