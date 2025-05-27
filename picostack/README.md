Pico Bluetooth Stack
==========================

*Version 23*

## Contents
- [1 Introduction](#1-introduction)
- [2 Setup](#2-setup)
    - [2.1 File list](#2-1-file-list) 
    - [2.2 Examples](#2-2-examples)
    - [2.3 Visual Studio Code Procedure](#2-3-visual-studio-code-procedure)
        - [2.3.1 Simple LE server example](#2-3-1-simple-le-server-example)      
        - [2.3.2 Full feature btferret example](#2-3-2-full-feature-btferret-example)
- [3 Pico code](#3-pico-code)
    - [3.1 Where to put your code](#3-1-where-to-put-your-code)    
    - [3.2 Screen prints](#3-2-screen-prints)
    - [3.3 Read key](#3-3-read-key)
    - [3.4 Input PIN](#3-4-input-pin)         
    - [3.5 FLASH usage](#3-5-flash-usage)

## 1 Introduction

The btlib library, and any C code written for Linux can be run on a Pico W,
and replaces btstack.
There are instructions here for setting up the Pico using Visual Studio Code.
Code can be developed on VSC, but because the same code runs on a Pi under Linux,
it can also be developed
and debugged on a Pi and then copied over to a Pico source file.



## 2 Setup

### 2-1 File list


```
There are two sample codes:

In the github/picostack folder

  mycodepico.c
  CMakeLists.txt
  picostack.c
  btlibp.c
  btlib.h
   
In the github/picostack/btferret folder

  btfcodepico.c
  CMakeLists.txt
    (you will also need picostack.c,
     btlibp.c and btlib.h from the picostack folder
     to compile this example)

NOTE in case you modify btlib for Linux and Pico
  btlibp.c is the Linux btlib.c with #define PICOSTACK uncommented
```

### 2-2 Examples

There are two sample codes as a starting point:

```
1. mycodepico.c and its CMakeLists.txt
      A simple LE server example with three characteristics:
        1. Device name (UUID:2A00) = "Picostack"
        2. Read/write data (UUID:ABCD) = "Hello"
        3. Writeable data (UUID:CDEF):  0 = LED off
                                        1 = LED on
                                        2 = LED flash
      This will run with the Pico stand-alone and no USB PC serial monitor.
      Once this is working, the small amount of code can be removed to
      provide a starting-point template for your own code.                          

2. btfcodepico.c and its CMakeLists.txt
      A stripped-down version of the Linux btferret.c code that implements many
      functions such as scans, Classic/LE clients and servers, read/write
      data to a connected device, Bluetooth traffic monitor, etc..
      The Pico must have a USB PC connection with the PC running a serial terminal
      because the code requires input/output. When prompted by a print to the 
      terminal, the appropriate response must be sent from the terminal
      with a line feed termination.
      This is not really a practical setup in itself, but allows experimentation
      with the basic functions and may provide suitable starting code for your
      own application.  

Both codes need picostack.c, btlibp.c and btlib.h.
``` 

## 2-3 Visual Studio Code Procedure

This procedure uses Visual Studio Code on a PC to compile and download code to the Pico 2W. Here are full
instructions if using VSC for the first time. There may be long delays at various stages as stuff is
downloaded. These instructions are for a Pico 2W, but a Pico W will also work.

### 2-3-1 Simple LE server example

This runs as an LE server that does not need a serial monitor connection from the PC, but
a serial monitor will display printed information.

```
1. Create a folder called "mycodepico" in a location that you choose.
2. Copy five files from the github/picostack folder to the mycodepico folder:
       mycodepico.c
       CMakeLists.txt
       picostack.c
       btlibp.c
       btlib.h 
3. If the board is not a Pico 2W, edit the CMakeLists.txt line:
        set(PICO_BOARD pico2_w)
4. Download Visual Studio Code.
5. Start Visual Studio Code. 
6. Select View/Extensions. Search for Raspberry Pi Pico. Install extension.
7. Click the Pico icon (probably in the vertical section on the
   left of the screen). It looks like a chip and reports
   "Raspberry Pi Pico Project" when the mouse is hovered over it.
8. Select "Import project" and set:
       Location = Your mycodepico folder 
9. At the bottom right of the screen, check that the board is
   correct. If not, it can be changed by clicking on Board:
       Board = pico 2w
       Use RISC V = no
10. From the bottom of the screen, click Compile..
11. Plug the Pico into the PC while pressing the BOOTSEL button. A new
    disk drive called something like "RP2350" should appear on the PC.          
12. From the bottom of the screen, click Run. If Run does not work,
    you can do it yourself with Explorer. Just copy the executable
    file (picostack/build/btfpico.uf2) to the disk.       
13. The LED on the Pico should flash briefly which shows that the code
    is running.
14. The Pico is waiting to be connected by an LE client such as the
    Linux btferret running on a Pi.
    Writing a single byte to UUID=CDEF does the following:
       0 = LED off
       1 = LED on
       2 = LED flash
    If running btferret on a Pi, connect to the Pico as follows:
      Enter b for an LE scan. It should find the Pico called "Picostack"
      Enter c to connect to "Picostack"
        Choose LE server
        Enable security = No
        Connection time = 0
      Enter v to read the Pico services
      Enter r to read one of the services
      Enter w to write 
        Choose UUID = CDEF
        Write one byte 0/1/2 for LED off/on/flash
      Enter d to disconnect.
      The Pico will wait for another connection (and can
      be connected by multiple clients simultaneously).             
``` 

### 2-3-2 Full-feature btferret example

This requires a serial monitor connection from the PC to run.

```
1. Create a folder called "btfcodepico" in a location that you choose.
2. Copy two files from the github/picostack/btferret folder to the
   btfcodepico folder:
       btfcodepico.c
       CMakeLists.txt
3. Copy three files from the github/picostack folder to the
   btfcodepico folder:
       picostack.c
       btlibp.c
       btlib.h     
4. Follow steps 3-13 in the previous section.
5. The Pico is waiting for input from a USB connected serial terminal.
6. Click SERIAL MONITOR
7. Select the Port for the Pico (something like "COM12 - USB Serial Device")
8. Set Line ending = LF line feed
9. Click Start Monitoring
10. The Pico has waited 3 seconds before starting its code, but if the 
    above steps were too slow, the initial prints to the monitor will
    be missed. In any case, it is waiting for a single character command
    (followed by a line feed termination) from the monitor. Enter h in 
    the send box at the bottom of the screen. Set Send as Text. Click
    the right-pointing arrow on the right of the box to send. This should 
    send h which will prompt a help listing print to the monitor.
11. Call other functions by sending the appropriate single character
    command as listed by help. (e.g. Send b for an LE scan which will
    probably find a number of nearby phones). If prompted for input,
    enter the input in the send box and send. Send k and choose option 2
    verbose to see the Bluetooth traffic monitor. 
12. To automatically start the USB monitor the next time the Pico runs,
    click the Toggle Automatic Reconnection icon. Probably third to the right
    of the Start Monitoring button - looks like a pair of connectors. Now
    when you click SERIAL MONITOR immediately after Run, the initial prints
    will appear.
```

     
## 3 Pico code

Code written for Linux can be simply pasted into mycodepico.c with some minor changes
as described in the following sections. Full documentation in the main btferret README.

### 3-1 Where to put your code

Your code goes into mycodepico.c, with one major difference from Linux code.
In Linux code init\_blue() is passed the name of
a text file containing devices data. For Pico code, init\_blue() is passed a hard-coded
text string that contains the devices data. The following sections describe other
differences from Linux. Insert your code into this empty mycodepico.c template:


```
#include <stdio.h>
#include <stdlib.h>
#include "btlib.h"

void mycode(void);

/********** DEVICES list ************
  Each line must end with \n\
************************************/
char *devices = {
"DEVICE = Picostack type=mesh node=1 address=local \n\
    ; your remote devices here                     \n\
"};
 
void mycode()
  { 
  if(init_blue(devices) == 0)
    return;

  while(1) 
    {
    // your code here
    }
  }
```
         

### 3-2 Screen prints

The C printf function works and sends output to a USB serial monitor if connected. 


### 3-3 Read key

When a server is running on Linux, pressing x or ESC stops the server. Also, an LE server may have
specified keys\_to\_callback which sends key strokes to the LE callback. These are 
handled on a Pico by the readkey() function in picostack.c, and the options are as follows:

```
1. Default behaviour is to return no key press.
2. Re-code readkey() yourself to return a key.
3. Call set_usb_flags(ENABLE_READKEY) and connect the Pico to a
   serial monitor. In this case, a key press can be sent from
   the serial monitor. See example in btfcodepico.c. 
```


### 3-4 Input PIN

When connecting a device with security, a PIN or passkey input is sometimes 
required. This is handled by the inputpin() function in picostack.c, and the
options are as follows:

```
1. For LE devices call le_pair with options which avoid passkey
   entry (e.g. Just Works, fixed passkey).
2. For Classic devices which require a pin, avoid input by specifying
   PIN= in the devices data (see DEVICE=HC05 example in btfcodepico.c). 
3. Default behaviour is to return "0000".
4. Re-code inputpin() yourself to return a pin.
5. Call set_usb_flags(ENABLE_INPUTPIN) and connect the Pico to a serial monitor.
   In this case, a prompt will be printed on the monitor, and a pin must
   be returned by a send from the monitor. See example in btfcodepico.c.
```

### 3-5 FLASH usage

Picostack uses a part of flash memory to store pairing data.


```
A flash sector is 4k

1. Top 4 sectors (16k) = left alone
   Change this by setting #define KEYFREE at the top of picostack.c
   
2. Below this, 16 sectors (64k) are used for storage.
   Change this by setting #define KEYSECTORS at the top of picostack.c
   The value can be anything from 1 upwards. 
```

The storage uses a scheme that greatly reduces stress on the flash memory.
Multiple erase/writes will eventually cause the flash memory to fail. The 
pairing data (which may only be a few bytes) is first written to page one (256 bytes)
of the available memory. Next time, it is written to page two, then page three, etc..
Only when all the pages (total 64k) have been used is the flash memory
erased, and the storage cycles back to page one. So if KEYSECTORS is small, memory
use is small, but wear is larger. A large KEYSECTORS uses a lot of memory and
reduces wear.



