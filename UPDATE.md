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

