btferret/btlib Bluetooth Interface
==================================

*Version 1 Feb 2021*

## Contents
- [1 Introduction](#1-introduction)
- [2 File list and compile](#2-file-list-and-compile)
- [3 Interface](#3-interface)
    - [3.1 Networks](#3:1-networks)
    - [3.2 btferret](#3.2-btferret)
    - [3.3 Windows/Android/HC-05 Classic devices](#3.3-windows/android/hc-05-classic-devices)   
    - [3.4 LE devices](#3.4-le-devices) 
    - [3.5 Node client/server connection](#3.5-node-client/server-connection) 
    - [3.6 Broadcast to all mesh servers](#3.6-broadcast-to-all-mesh-servers) 
    - [3.7 sample.c](#3.7-sample.c)
- [4 btlib Library](#4-btlib-library) 
    - [4.1 Function list](#4.1-function-list)
    - [4.2 Functions](#4.2-functions)    
        - [4.2.1 classic\_scan](#4:2:1-classic\_scan)
        - [4.2.2 close\_all](#4.2.2-close\_all)
        - [4.2.3 connect\_node](#4.2.3-connect\_node)
        - [4.2.4 ctic\_name](#4.2.4-ctic\_name)
        - [4.2.5 ctic\_ok](#4.2.5-ctic\_ok)
        - [4.2.6 device\_connected](#4.2.6-device\_connected)
        - [4.2.7 device\_info](#4.2.7-device\_info)
        - [4.2.8 device\_name](#4.2.8-device\_name)
        - [4.2.9 device\_type](#4.2.9-device\_type)
        - [4.2.10 disconnect\_node](#4.2.10-disconnect\_node)
        - [4.2.11 find\_channel](#4.2.11-find\_channel)
        - [4.2.12 find\_ctics](#4.2.12-find\_ctics)
        - [4.2.13 find\_ctic\_index](#4.2.13-find\_ctic\_index)
        - [4.2.14 init\_blue](#4.2.14-init\_blue)
        - [4.2.15 le\_scan](#4.2.15-le\_scan)
        - [4.2.16 list\_channels](#4.2.16-list\_channels)
        - [4.2.17 list\_ctics](#4.2.17-list\_ctics)
        - [4.2.18 list\_uuid](#4.2.18-list\_uuid)       
        - [4.2.19 localnode](#4.2.19-localnode)
        - [4.2.20 mesh\_on](#4.2.20-mesh\_on)
        - [4.2.21 mesh\_off](#4.2.21-mesh\_off)
        - [4.2.22 mesh\_server](#4.2.22-mesh\_server)
        - [4.2.23 node\_server](#4.2.23-node\_server)
        - [4.2.24 output\_file](#4.2.24-output\_file)
        - [4.2.25 read\_ctic](#4.2.25-read\_ctic)
        - [4.2.26 read\_error](#4.2.26-read\_error)
        - [4.2.27 read\_mesh](#4.2.27-read\_mesh)
        - [4.2.28 read\_node\_count](#4.2.28-read\_node\_count)
        - [4.2.29 read\_node/all\_endchar](#4.2.29-read\_node/all\_endchar)
        - [4.2.30 read\_node/all\_clear](#4.2.30-read\_node/all\_clear)
        - [4.2.31 scroll\_back/forward](#4.2.31-scroll\_back/forward)
        - [4.2.32 set\_print\_flag](#4.2.32-set\_print\_flag)
        - [4.2.33 strtohex](#4.2.33-strtohex)
        - [4.2.34 wait\_for\_disconnect](#4.2.34-wait\_for\_disconnect)
        - [4.2.35 write\_ctic](#4.2.35-write\_ctic)
        - [4.2.36 write\_mesh](#4.2.36-write\_mesh)
        - [4.2.37 write\_node](#4.2.37-write\_node) 
- [5 Reference](#5-reference)
    - [5.1 What gives with UUIDs?](#5.1-what-gives-with-uuids?)
    - [5.2 Packet formats](#5.2-packet-formats)
        - [5.2.1 Starting 01 HCI Commands](#5.2.1-starting-01-hci-commands)
        - [5.2.2 Starting 04 HCI Events](#5.2.2-starting-04-hci-events)
        - [5.2.3 Starting 02 Channel 0001](#5.2.3-starting-02-channel-0001)
        - [5.2.4 Starting 02 Channel 0004](#5.2.4-starting-02-channel-0004)
        - [5.2.5 Starting 02 Channel 0040+](#5.2.5-starting-02-channel-0040+)
    - [5.3 Procedures](#5.3-procedures)
        - [5.3.1 HCI socket read/write packets](#5.3.1-hci-socket-read/write-packets)
        - [5.3.2 Classic procedures](#5.3.2-classic-procedures) 
        - [5.3.3 Classic connect with PIN code](#5.3.3-classic-connect-with-pin-code)
        - [5.3.4 Classic disconnect initiated by server](#5.3.4-classic-disconnect-initiated-by-server)
        - [5.3.5 Classic scan](#5.3.5-classic-scan)
        - [5.3.6 SDP database operations](#5.3.6-sdp-database-operations)
        - [5.3.7 LE Procedures](#5.3.7-le-procedures)
        - [5.3.8 Read LE services](#5.3.8-read-le-services)
        - [5.3.9 LE scan](#5.3.9-le-scan)
    - [5.4 Server Code](#5.4-server-code)
        - [5.4.1 Raspberry Pi bluez server](#5.4.1-raspberry-pi-bluez-server)
        - [5.4.2 Windows COM port](#5.4.2-windows-com-port)
        - [5.4.3 Windows Sockets](#5.4.3-windows-sockets)
        - [5.4.4 Android](#5.4.4-android)
- [6 Documentation References](#6-documentation-references)

## 1 Introduction

This is a Bluetooth interface that has been developed for Raspberry Pis.

A Pi running this interface can connect simultaneously to multiple Classic and LE servers,
and also to a mesh network of other Pis running the same software.

There is a library of functions ([btlib](#4-btlib-library)) and a sample program
([btferret](#3.2-btferret)) that implements most of
the interface features such as connecting to other devices, operating as a client or server,
exchanging data (including a file
transfer routine) and display of information - it is a bit like a super bluetoothctl that won't drive
you mad. It has a verbose print mode that displays the HCI Bluetooth traffic with details of
the packets that are exchanged.

Also included is the code for a simple [mesh network example](#3.7-sample.c).

In the [reference](#5-reference) section there is a detailed description
of the HCI Bluetooth interface, the packet formats and how they are constructed,
and the sequence of instructions
needed to establish connections and exchange data. This information
is difficult to extract in a coherent form from the Bluetooth specification documents.

There is a [server code](#5.4-server-code) section that is a brief guide to writing code
for other machines to act as servers for btferret/btlib.

This interface is programmed at the HCI level and bypasses higher-level bluez functions,
so it does not use the Pi's Bluetooth service, which can be stopped.

## 2 File list and compile

## DOWNLOAD

```
btferret.c
btlib.c
btlib.h
devices.txt
```

## COMPILE

```
gcc btferret.c btlib.c -o btferret

Run with devices.txt in the same directory
```

No additional libraries or installs are required, the code is self-contained.
It does not use higher-level bluez functions, so if desired the Pi's Bluetooth service can be
stopped as follows:

```
service bluetooth stop

Check result via:

service bluetooth status
```

## CODE

To customise btferret.c for your devices, an essential first step is to
edit the devices.txt file to list all the devices in the
network (see the [init_blue](#4.2.2-init\_blue) documentation).

To write your own code using the [btlib](#4-btlib-library)
functions, start from scratch or modify the btferret.c or sample.c examples.
Compile and link to the library functions in btlib.c.

```
gcc mycode.c btlib.c -o mycode
```

## 3 Interface  
   

## 3:1 Networks

A Mesh Pi running btferret/btlib can operate as a client or as two types of server: mesh or node.

Any Mesh Pi can connect to multiple Classic and LE servers as a client.

A node server is in a one-to-one connection
with a client Mesh Pi and only exchanges packets with that device. These packets are sent once.

A mesh server receives all mesh packets that
any other Mesh Pi broadcasts. A broadcasting device sends its packet repeatedly - not just once.
A receiving device will read each packet once and ignore subsequent repeats.

Don't expect too much of the speeds here. The mesh packet repeat rate may only be around once per
second, btferret's file transfer speed to another Mesh Pi is about 2000 bytes/s, but can be more like
50,000 bytes/s to a suitably programmed Classic server.

The following diagram shows these connections
with the principal btlib functions that apply in each case.

![picture](image1.png)

## 3.2 btferret

The btferret.c program implements the basic interface functions: scan for active
devices, operate as
a client or a server, connect to Classic/LE/Mesh Pis, exchange data, file
transfer, broadcast mesh packets,
read/write LE characteristics, read service information from Classic and LE servers,
and print detailed information about the HCI Bluetooth traffic.

```
a - Scan for Classic Bluetooth devices
b - Scan for LE and Mesh Pi Bluetooth devices
i - Print device information
v - Read services (Classic serial channels or LE characteristics)
c - Connect to a classic/LE/Mesh node (can be multiple)
d - Disconnect a node (use D instead for btferret node and mesh servers)
t - Transmit ascii string to a connected node
T - Transmit (broadcast) ascii string to mesh
r - Read characteristic from LE device
w - Write characteristic to LE device
k - Settings (inc. verbose print option to print HCI traffic)
[] - Scroll screen output back/forward
o - Save all recent screen output to a text file
s - Become a mesh or node server and listen for remote client
y - List services that contain a specified UUID
h - Print help

```

These commands only work when connected to another Mesh Pi running
btferret set up as a node server (via s).

```
p - Ping a node server for OK reply
f - Send a file to a node server
D - Tell a node server (or all mesh servers) to disconnect
```

The following sections describe how to use these commands to 
establish connections and exchange data.


## 3.3 Windows/Android/HC-05 Classic devices

The btferret/btlib code can connect to a Windows/Android/HC-05 Classic server. This might be a
Bluetooth terminal program set up as a server (via a "Make discoverable" option for example). It
is not possible to connect from a Bluetooth terminal program as a client to btferret/btlib as a server,
so commands in the terminal program such as "connect" will not work. Guidance on writing server
code for other machines is in the [server code](#5.4-server-code) section.

With the server waiting
for a connection, btferret must first find
the [RFCOMM channel](#5.1-what-gives-with-uuids?) on which
the server is listening.

```
devices.txt 
DEVICE=Windows PC  type=classic node=4 address=00:1A:7D:DA:71:13
DEVICE = HC-05 TYPE=CLASSIC NODE=6 PIN=1234 CHANNEL=1 ADDRESS=98:D3:32:31:59:84


btferret commands

i - Print device info. If server is already in device info via devices.txt
    the following step is not necessary
a - Scan for classic devices if not already set in devices.txt.
    This should find the listening server device.
c - Connect. Enter the node number of the listening device.
    Select the "Read services" option. The server's serial channels will be
    displayed, and the appropriate one will probably have one of the two
    following UUIDs, or may be described as something like "rfcomm serial port".    
       1101
       00001101-0000-1000-8000-00805F9B34FB
    Enter the RFCOMM channel number. The device should connect.
t - Send string. Enter the node number of the device.
    Enter a string, for example: hello
    Enter the termination character - almost certainly line feed 10.
    The hello message should appear on the Bluetooth terminal screen.
    Because the terminal program is not set up to reply, btferret will
    report "No reply".
d - Disconnect. Enter the device node number.
```

The same procedure is programmed via btlib functions as follows:

```
devices.txt 
DEVICE=Windows PC  type=classic node=4 address=00:1A:7D:DA:71:13
DEVICE = HC-05 TYPE=CLASSIC NODE=6 PIN=1234 CHANNEL=1 ADDRESS=98:D3:32:31:59:84


int channel,len;
char buf[16],inbuf[64];

   // The serial channel might be known. For example an HC-05 has just one
   // channel which can be specified in devices.txt via CHANNEL=1 as above
connect_node(6,CHANNEL_STORED,0);

   // find the serial channel of node 4 if not known  
channel = find_channel(4,UUID_2,strtohex("1101",NULL));
OR
channel = find_channel(4,UUID_16,strtohex("00001101-0000-1000-8000-00805F9B34FB",NULL));
   // if channel > 0 then the serial channel has been found 
connect_node(4,CHANNEL_NEW,channel);
   
strcpy(buf,"hello");
len = strlen(buf);
         // replace term zero with termination char 10 for server
buf[len] = 10;
++len;   // len will now be 6

write_node(4,buf,len);  // send len chars to node 4

   // if the server is expected to reply:
   // wait for 1 second for a reply with a 10 termination char
read_node_endchar(4,inbuf,sizeof(inbuf),10,EXIT_TIMEOUT,1000);

disconnect_node(4);
    // OR if the server is programmed to initiate disconnection
    // see the discussion in disconnect_node() section 4.2.10 
```

## 3.4 LE devices

Connect to an LE device and read/write characteristics as follows:

```
devices.txt
DEVICE = Pictail  TYPE=LE  NODE=7   ADDRESS = 00:1E:C0:2D:17:7C
  LECHAR=Test    HANDLE=001C PERMIT=0A SIZE=2     ; index 0
  LECHAR=Name    UUID=2A00                        ; index 1


btferret commands

i - Print device info. If the LE device is already in device info via
    devices.txt, the following step is not necessary.
b - Scan for LE devices if not already set in devices.txt
c - Connect. Enter node number of LE device.
    If the device's characteristics are already known by device info
    the following step is not necessary.
v - Read services. The characteristic info will be read from the device
r - Read a characteristic - enter its characteristic index from the list
w - Write a characteristic
d - Disconnect

```
LE read/writes are programmed via btlib funtions as follows:

```
devices.txt
DEVICE = Pictail  TYPE=LE  NODE=7   ADDRESS = 00:1E:C0:2D:17:7C
  LECHAR=Name    UUID=2A00                        ; index 0
  LECHAR=Test    HANDLE=001C PERMIT=0A SIZE=2     ; index 1
 


char buf[32];

connect_node(7,0,0);     // 2nd/3rd parameters 0,0 not needed for LE devices

read_ctic(7,1,buf,sizeof(buf));   // read Test (index 1) from node 7
                                  // buf[0] and buf[1] will have the data
read_ctic(7,0,buf,sizeof(buf));   // read Name (index 0)
printf("Name = %s\n",buf);

         // write 12 34 to Test (index 1)
buf[0] = 0x12;
buf[1] = 0x34;
write_ctic(7,1,buf,0);  // device info knows the size of Test is 2
                        // so set the last parameter count=0

disconnect_node(7);

```

## 3.5 Node client/server connection

Two Pis connected as a client/server pair. There must be two Pis
listed as MESH type in devices.txt.


```
devices.txt
DEVICE = Mesh Pi 1  TYPE=mesh node=1 ADDRESS = B8:27:EB:F1:50:C3
DEVICE = Mesh Pi 2  TYPE=mesh node=2 ADDRESS = DC:A6:32:04:DB:56

btferret commands

NODE SERVER
Set up one Pi as a node server:

i - Print device info. The client that will connect must be listed.
    If not, add it to the devices.txt info.
s - Select node server. Enter node number of client that will connect.
    The device will now report that it is waiting for a connection
    from the specified client node.
    At any time, pressing the x key will stop the server.
    
NODE CLIENT
On another Pi, connect as a client

i - Print device info. The node server Pi must be listed.
c - Connect. Enter node number of the listening node server Pi.
    The devices should both report a connection
p - Ping. Enter server node number. It should reply "OK" as 
    programmed in btferret's node_callback routine.
t - Send string. Enter string which will appear on the server.
f - Send file to server.
D - Disconnect. A 'D' command that the server has been programmed
    via node_callback in btferret to initiate disconnection.
```

The same procedure is programmed via btlib functions as follows:

```
devices.txt
DEVICE = Mesh Pi 1  TYPE=mesh node=1 ADDRESS = B8:27:EB:F1:50:C3
DEVICE = Mesh Pi 2  TYPE=mesh node=2 ADDRESS = DC:A6:32:04:DB:56

NODE SERVER - node 1
Set up node 1 as a server listening for node 2 to connect
Specify 10 as end char for packets sent by client
 
node_server(2,node_callback,10)

This node_callback routine receives packets sent by the client.

int node_callback(int clientnode,char *data,int datlen)
  {
  char buf[4];
  
  // data[] has datlen bytes from clientnode
  
  printf("Node packet from %s\n",device_name(clientnode));
  if(data[0] == 'p')
    {   // pinged - send OK reply
    buf[0] = 'O';
    buf[1] = 'K';
    buf[2] = 10;   // end char expected by client
    write_node(clientnode,buf,3);  // send 3-byte reply
    }
  else if(data[0] == 'D')  // 'D' programmed as exit command   
    return(SERVER_EXIT);   // server initiates disconnection
  
  return(SERVER_CONTINUE);  // wait for another packet
  }  



NODE CLIENT - node 2
Connect as a client to the node 1 server

char outbuf[4],inbuf[64];

connect_node(1,0,0);  // connect to node 1

   // Ping node 1 server
outbuf[0] = 'p';
outbuf[1] = 10;           // end char expected by server
write_node(1,outbuf,2);   // send 2-byte packet to node 1
   // wait for reply from node 1 with end char 10. Time out = 2 seconds
read_node_endchar(1,inbuf,sizeof(inbuf),10,EXIT_TIMEOUT,2000);
printf("Reply %s\n",inbuf);
    // Tell server to initiate disconnection
outbuf[0] = 'D';
outbuf[1] = 10;
write_node(1,outbuf,2);
    // no reply programmed  
    // wait for disconnection procedure initiated by server
wait_for_disconnect();

```


## 3.6 Broadcast to all mesh servers

There must be at least two Pis listed as MESH type in devices.txt. 

```
devices.txt
DEVICE = Mesh Pi 1  TYPE=MESH node=1 ADDRESS = B8:27:EB:F1:50:C3
DEVICE = Mesh Pi 2  TYPE=MESH node=2 ADDRESS = DC:A6:32:04:DB:56

btferret commands

MESH SERVERS
Set up one or more mesh devices as mesh servers to receive all broadcast packets.

i - Print device info. Any broadcasting device must be on the list
    or its packets will be ignored as a security risk.
    If not on the list - add to the devices.txt info.
s - Select mesh server. The device will report that it is waiting for packets.
    At any time, pressing the x key will stop the server.     

MESH BROADCASTER
On another mesh device, send a mesh packet

T - Enter string. It will appear on all listening mesh servers
D - Disconnect. Tell all mesh servers to stop.
```

The same procedure is programmed via btlib funtions as follows:

```
devices.txt
DEVICE = Mesh Pi 1  TYPE=mesh node=1 ADDRESS = B8:27:EB:F1:50:C3
DEVICE = Mesh Pi 2  TYPE=mesh node=2 ADDRESS = DC:A6:32:04:DB:56


MESH SERVER

mesh_server(mesh_callback);

This mesh callback routine receives all broadcast packets from
devices on the devices.txt list.

int mesh_callback(int clientnode,char *data,int datlen)
  {
  int n;
    
  printf("Mesh packet from %s\n",device_name(clientnode));
    // print mesh packet in hex format
  for(n = 0 ; n < datlen ; ++n)
    printf(" %02X",data[n]);
  printf("\n");
  if(data[0] == 'D')   // 'D' programmed as exit command
    return(SERVER_EXIT);
  return(SERVER_CONTINUE);  // wait for another packet
  } 
 

MESH BROADCASTER

char buf[4];

buf[0] = 0x12;
buf[1] = 0x34;
   // broadcast two-byte mesh packet
write_mesh(buf,2);
   // broadcast 'D' disconnect command
write_mesh('D',1);
```

## 3.7 sample.c

The sample.c code is an illustration of a procedure using mesh, node and
Classic connections on the following mesh network.

![picture](image2.png)

The Classic server might be a Windows PC with Bluetooth set up as an
incoming [COM port](#5.4.2-windows-com-port) that presents as a UUID=1101 channel.
All three mesh Pis run the same sample.c code that executes the following sequence:

```
sampledev.txt:
DEVICE = Pictail  TYPE=LE  NODE=7   ADDRESS = 00:1E:C0:2D:17:7C
  LECHAR=Test    HANDLE=001C PERMIT=0A SIZE=2     ; index 0
  LECHAR=Name    UUID=2A00                        ; index 1
DEVICE = Windows PC  type = classic node = 4 address=00:1A:7D:DA:71:13
DEVICE = Mesh Pi 1  TYPE=mesh node=1 ADDRESS = B8:27:EB:F1:50:C3
DEVICE = Mesh Pi 2  TYPE=mesh node=2 ADDRESS = DC:A6:32:04:DB:56
DEVICE = Mesh Pi 3  TYPE=mesh node=3 ADDRESS = 00:15:83:EF:24:3D

1. Nodes 2 and 3 set up as mesh servers
2. Node 1 broadcasts a mesh packet telling 2 to become a node server
3. Node 3 receives and ignores the command
4. Node 2 receives and becomes a node server waiting for 1 to connect
5. Node 1 connects to node 2
6. Node 1 sends a node packet to 2 requesting the name of node 7
7. Node 2 connects to the LE server node 7 and reads its name
8. Node 2 sends the name to node 1
9. Node 2 disconnects node 7
10. Node 1 connects to the classic server node 4
11. Node 1 sends the name to node 4
12. Node 1 disconnects node 4
13. Node 1 sends node packet to node 2 telling it disconnect as a 
           node server and revert to a mesh server
14. Node 1 broadcasts a mesh packet telling all mesh servers (2,3) to close
15. Nodes 2 and 3 mesh servers close and exit their programs
16. Node 1 exits
```

None of these mesh and node packet commands are part of the btlib interface - they are all
custom programmed in the sample.c callback functions.
The sample.c code is extensively commented and no further description will be
given here.

```
DOWNLOAD

sample.c
sampledev.txt

COMPILE

gcc sample.c btlib.c -o sample

Run with sampledev.txt in the same directory
```


## 4 btlib Library 

These library functions are in btlib.c/btlib.h.       

## 4.1 Function List


[init\_blue](#4.2.14-init\_blue) - Initialize must be called on program start<br/>
[close\_all](#4.2.2-close\_all) - Close all connections on program end<br/>
[device\_info](#4.2.7-device\_info) - Print device information<br/>
[classic\_scan](#4.2.1-classic\_scan) - Scan for classic devices<br/>
[le\_scan](#4.2.15-le\_scan) - Scan for LE devices<br/>
[localnode](#4.2.19-localnode) - Return node number of local device<br/>
[list\_channels](#4.2.16-list\_channels) - List serial data channels of a Classic device<br/>
[list\_ctics](#4.2.17-list\_ctics) - List characteristics of an LE device<br/>
[list\_uuid](#4.2.18-list\_uuid) - List node services that contain a specified UUID<br/>
[connect\_node](#4.2.3-connect\_node) - Connect to a server node as a client<br/>
[node\_server](#4.2.23-node\_server) - Become a node server. Listen for connection<br/>
[mesh\_server](#4.2.22-mesh\_server) - Listen for broadcast mesh packets<br/>
[find\_channel](#4.2.11-find\_channel) - Find RFCOMM serial channel of Classic device<br/>
[find\_ctics](#4.2.12-find\_ctics) - Read all characteristic info from LE device<br/>
[find\_ctic\_index](#4.2.13-find\_ctic\_index) - Find characteristic index of UUID<br/>
[write\_ctic](#4.2.35-write\_ctic) - Write characteristic to an LE device<br/>
[read\_ctic](#4.2.25-read\_ctic) - Read characteristic from an LE device<br/>
[write\_node](#4.2.37-write\_node) - Write serial data to connected node device<br/>
[write\_mesh](#4.2.36-write\_mesh) - Start broadcasting a packet to all mesh devices<br/>
[read\_mesh](#4.2.27-read\_mesh) - Read next packet from all broadcasting mesh devices<br/> 
[read\_node\_count](#4.2.28-read\_node\_count) - Read a specified byte count from specified node<br/>
[read\_node/all\_endchar](#4.2.29-read\_node/all\_endchar) - Read from specified or all nodes until end char received<br/>
[read\_node/all\_clear](#4.2.30-read\_node/all\_clear) - Clear data in input buffer<br/>
[read\_error](#4.2.-read\_error) - Return error code of last read<br/>
[device\_type](#4.2.9-device\_type) - Return device type (Classic/LE/Mesh/Local)<br/>
[device\_name](#4.2.8-device\_name) - Return device name string<br/>
[device\_connected](#4.2.6-device\_connected) - Return device connection state<br>
[ctic\_ok](#4.2.5-cti\c_ok) - Return LE characteristic valid flag<br/>
[ctic\_name](#4.2.4-ctic\_name) - Return LE characteristic name string<br/>
[disconnect\_node](#4.2.10-disconnect\_node) - Disconnect initiated by client<br/>
[wait\_for\_disconnect](#4.2.34-wait\_for\_disconnect) - Wait for disconnect initiated by server<br/>
[scroll\_back/forward](#4.2.31-scroll\_back/forward) - Scroll screen back/forward<br/>
[set\_print\_flag](#4.2.32-set\_print\_flag) - Set screen print mode (none/normal/verbose)<br/>
[output\_file](#4.2.24-output\_file) - Save all recent screen output to a file<br/>
[strtohex](#4.2.33-strtohex) - Convert ascii string to array of hex values<br/>
[mesh\_on](#4.2.20-mesh\_on) - Turn mesh transmission on<br/>
[mesh\_off](#4.2.21-mesh\_off) - Turn mesh transmission off<br/>


## 4.2 Functions

## 4:2:1 classic\_scan

```
void classic_scan(void)
```

Scan for Classic Bluetooth devices. If a device is not already on the device information list, 
it is added and will be listed via [device\_info](#4.2.7-device\_info)
 . 


## 4.2.2 close\_all

```
void close_all(void)
```

Close all connections to remote devices and the local Bluetooth adapter. Call this on
program termination.
 

## 4.2.3 connect\_node

```
int connect_node(int node,int flag,int channel)
```

Connect to a node and set up a serial channel.

PARAMETERS

```
node = Node number

For LE servers and Pi Mesh nodes, channel and flag are ignored
For Classic servers, channel and flag are required
  
flag = one of the following
    CHANNEL_NEW       Use channel in parameters
    CHANNEL_STORED    Reconnect using previous/stored channel
    ignored if device is not a Classic server
    
channel = RFCOMM channel
          ignored if flag = CHANNEL_STORED
          ignored if device is not a Classic server        
    
```

The RFCOMM channel must be the serial channel on which the remote device (acting as a
server) is listening (see [UUIDs](#5.1-what-gives-with-uuids?)).
Remote device channels can be found
by calling [find\_channel](#4.2.11-find\_channel) or [list\_channels](#4.2.16-list\_channels).
Use
CHANNEL\_NEW to specify the channel in the parameters. CHANNEL\_STORED uses the channel stored
in device information that can be set in two ways:

1. With an e.g. "CHANNEL=1" entry in the [init\_blue](#4.2.14-init\_blue) devices.txt infomation.

2. A CHANNEL\_NEW connection will store the channel number, so CHANNEL\_STORED can be used for
all subsequent reconnections.

Some devices have fixed RFCOMM channels which are permanent and known, while others allocate
them as needed and can only be found by reading the remote device services at connection time.

RETURN

```
0 = Fail
1 = OK
```

SAMPLE CODE

```
If device information in devices.txt for init_blue() is:

DEVICE = Windows PC  TYPE=classic NODE=7 address=00:1A:7D:DA:71:13
DEVICE = HC-05 TYPE=CLASSIC NODE=6 PIN=1234 CHANNEL=1 ADDRESS=98:D3:32:31:59:84
DEVICE = Pictail  TYPE=LE NODE=3  ADDRESS = 00:1E:C0:2D:17:7C
DEVICE = My other Pi  TYPE=MESH NODE=9 ADDRESS = B8:27:EB:F1:50:C3


int channel;

connect_node(3,0,0)   // connect to Pictail LE server node 3
connect_node(9,0,0)   // connect to My other Pi Mesh device node 9
                      // listening as a node server
                      
connect_node(6,CHANNEL_STORED,0); // connect to HC-05 classic server node 6
                                  // via channel 1 specified in devices.txt                       

connect_node(7,CHANNEL_NEW,4)  // connect to Windows PC Classic server node 7 
                               // listening on RFCOMM serial channel 4
                               // will store channel 4 in device info
connect_node(7,CHANNEL_STORED,0)  // reconnect to Windows PC classic server node 7
                                  // using stored channel 4 set by the previous
                                  // CHANNEL_NEW connection  
                                  
      // find Classic server channel number from its 2 or 16-byte UUID               
channel = find_channel(7,UUID_2,strtohex("1101",NULL));
OR
channel = find_channel(7,UUID_16,strtohex("FCF05AFD-67D8-4F41-83F5-7BEE22C03CDB",NULL));
connect_node(7,CHANNEL_NEW,channel)  // connect to Windows PC node 7 on found channel 

```


## 4.2.4 ctic\_name

```
char *ctic_name(int node,int cticn)
```

Return an ascii string containing the name of LE characteristic with index cticn.


PARAMETERS

```
node = Node number
cticn = Characteristic index
```

RETURN

```
Pointer to a zero-terminated ascii string containing characteristic name
(or "Invalid node","Invald characteristic","Nameless")
```

See [device\_type](#4.2.9-device\_type) for sample code.


## 4.2.5 ctic\_ok

```
int ctic_ok(int node,int cticn)
```

Return 1 if node and cticn are the indices of a valid characteristic entry
in the device information. 

PARAMETERS

```
node = Node number
cticn = Characteristic index
```

RETURN

```
0 = Invalid node or characteristic
1 = Valid node and characteristic
```

See [device\_type](#4.2.9-device\_type) for sample code.


## 4.2.6 device\_connected

```
int device_connected(int node)
```

Return the connection state of a node.

PARAMETERS

```
node = Node number
```

RETURN

```
0 = Not connected
1 = Connected
```


## 4.2.7 device\_info

```
int device_info(int mask)
```

PARAMETERS

```
mask = OR'ed combination of the following flags:

BTYPE_LO  include local device
BTYPE_ME  include Pi Mesh devices
BTYPE_CL  include classic servers
BTYPE_LE  include LE servers

BTYPE_CONNECTED     exclude disconnected devices
BTYPE_DISCONNECTED  exclude connected devices

BTYPE_SHORT  short list with device names only

```

SAMPLE CODE

```
device_info(BTYPE_LO | BTYPE_ME | BTYPE_CL | BTYPE_LE)
                    // full list all device info
device_info(BTYPE_CL | BTYPE_CONNECTED)
                    // full list connected classic servers
device_info(BTYPE_LE | BTYPE_DISCONNECTED | BTYPE_SHORT)
                    // short list disconnected LE servers
```
                   

## 4.2.8 device\_name

```
char *device_name(int node)
```

PARAMETERS

```
node = Node number
```

RETURN

```
pointer to a zero terminated string containing device name or
"Invalid node"
```

See [device\_type](#4.2.9-device\_type) for sample code.


## 4.2.9 device\_type

```
int device_type(int node)
```

Return type of a device.

PARAMETERS

```
node = Node number
```

RETURN

```
0        = Invalid node number
BTYPE_CL = Classic server
BTYPE_LE = LE server
BTYPE_ME = Pi Mesh device
BTYPE_LO = Local device 0
```

SAMPLE CODE

Use [device\_type](#4.2.9-device\_type) and [ctic\_ok](#4.2.5-ctic\_ok)
to check devices and characteristics
in the device information and list their names with [device\_name](#4.2.8-device\_name)
and [ctic\_name](#4.2.4-ctic\_name).

```
int k;

printf("Node 7 name = %s\n",device_name(7));
if(device_type(7) == BTYPE_LE)
  {
  for(k = 0 ; ctic_ok(7,k) != 0 ; ++k)
    printf("  Characteristic %d = %s\n",k,ctic_name(7,k));
  }       
```

## 4.2.10 disconnect\_node

```
int disconnect_node(int node)
```

Client-initiated disconnect a node. There are two ways to disconnect:
client-initiated and server-initiated. This disconnect_node() function
is client-initiated and the client simply disconnects without telling the server.
If the server is set up
to recognise this (e.g. an HC-05 or LE server), then use this method. See
below for an explanation of a server-initiated disconnection that should be used
for node connections to other Mesh Pis.


PARAMETERS

```
node = Node number
```

RETURN

```
0 = Fail
1 = OK
```

SAMPLE CODE


```
disconnect_node(3);   // disconnect node 3
```

Some servers may not respond to a client disconnect and
may continue to listen for data packets from the client without realising that it has 
disconnected.
The solution is to send the server a message that it interprets as an instruction
to disconnect. The server then initiates
the disconnection and the client must wait for a disconnection sequence from the server to 
complete the process gracefully - and [wait\_for\_disconnect](#4.2.34-wait\_for\_disconnect)
does this. In this way both devices agree
to disconnect. For an example, see the node_callback() code in btferret.c or
[node client/server connection](#3.5-node-client/server-connection).

```
   // Send a serial data message to node 3 that it interprets as
   // a disconnect instruction - in this case a single 'D' character

char buf[4];

buf[0] = 'D';  
write_node(3,buf,1);   
   // Server has been programmed to initiate disconnection
   // in response to this message.
   // Wait 3s for the disconnect procedure packets sent back 
   // by the server and complete the disconnection properly
wait_for_disconnect(3,3000);
   // If the function times out, the local device
   // initiates and completes the disconnection.
   // So disconnection is guaranteed at this point. 
```


## 4.2.11 find\_channel

```
int find_channel(int node,int flag,char *uuid)
```

Returns the RFCOMM channel number of a specified
[UUID](#5.1-what-gives-with-uuids?). Use the RFCOMM channel to
connect to a Classic server via [connect\_node](#4.2.3-connect\_node).

PARAMETERS

```
node = Node number
flag = One of the following: 
  UUID_2   Two-byte UUID
  UUID_16  16-byte UUID  
uuid = 2 or 16-byte array containing UUID 
```

RETURN

```
-1 = Failed to read channel info
 0 = Read channel info OK but no UUID match
>0 = RFCOMM channel
```

SAMPLE CODE

```

Find RFCOMM channel of node 7's serial data channel with UUID = 1101

int channel;
char uuid[2];

uuid[0] = 0x11;
uuid[1] = 0x01;
channel = find_channel(7,UUID_2,uuid);

The strtohex() function can be used to generate the uuid array from an ascii string.

channel = find_channel(7,UUID_2,strtohex("1101",NULL));
channel = find_channel(7,UUID_16,strtohex("FCF05AFD-67D8-4F41-83F5-7BEE22C03CDB",NULL));

if(channel > 0)
  connect_node(7,CHANNEL_NEW,channel)  // connect to Classic server node 7 on found channel 
```

## 4.2.12 find\_ctics

```
int find_ctics(int node)
```

Read the characteristic information from a connected LE server and save it
in the device info. This is not necessary if the characteristic
information has been set via devices.txt in [init\_blue](#4.2.14-init\_blue).

PARAMETERS

```
node = Node number
```

RETURN

```
 -1 = Fail to read characteristic info
>=0 = Number of characteristics found
```

SAMPLE CODE

```
Find all characteristics of LE server node 5 and save in device info

find_citcs(5);
```

## 4.2.13 find\_ctic\_index

```
int find_ctic_index(int node,int flag,char *uuid)
```

Returns the characteristic index of a specified UUID for an LE server. This
searches the device information for the UUID. It does not read information from
the LE device itself. If the characteristic is not in the device information (most 
conveniently via devices.txt in [init\_blue](#4.2.14-init\_blue)),
call [find\_ctics](#4.2.12-find\_ctics) which reads all
available characteristics from the LE device into the device information.
This function will then succeed.
Use the characteristic index in [read\_ctic](#4.2.25-read\_ctic)
and [write\_ctic](#4.2.35-write\_ctic).


PARAMETERS

```
node = Node number of connected LE server
flag = One of the following: 
  UUID_2   Two-byte UUID
  UUID_16  16-byte UUID  
uuid = 2 or 16-byte array containing UUID 
```

RETURN

```
 -1 = Fail
>=0 = Characteristic index
```

SAMPLE CODE

```

Find index of LE server node 7's characteristic with UUID = 2A00 and read it.

int cticn;
char uuid[2],data[64];

uuid[0] = 0x2A;
uuid[1] = 0x00;
cticn = find_ctic_index(7,UUID_2,uuid);
read_ctic(7,cticn,data,sizeof(data));

The strtohex() function can be used to generate the uuid array from an ascii string.

cticn = find_ctic_index(7,UUID_2,strtohex("2A00",NULL));
cticn = find_ctic_index(7,UUID_16,strtohex("FCF05AFD-67D8-4F41-83F5-7BEE22C03CDB",NULL));

```

## 4.2.14 init\_blue

```
int init_blue(char *filename)
```

Initialise the btlib.c library. Must be called on program start. It reads a text
file with information about devices in the network (btferret assumes devices.txt).
The list should include the local device itself. All other Pis in the network
should be MESH type, while other servers will be CLASSIC or LE.

PARAMETERS

```
filename = text file containing pre-set device information
```

RETURN

```
0 = Fail - program must terminate
1 = OK
```

SAMPLE CODE

```
if(init_blue("/home/pi/mydevices.txt") == 0)
  return(0);
  
txt file example:

  ; semi colon is comment character - everything beyond ignored
  ; This file must list all network devices
DEVICE = My Pi        TYPE=MESH NODE=1 ADDRESS=B6:15:EB:F5:50:53
DEVICE = My other Pi  TYPE=MESH NODE=2 ADDRESS=B8:27:EB:F1:50:C3
DEVICE = Pictail  TYPE=LE NODE=5 ADDRESS = 00:1E:C0:2D:17:7C
  LECHAR=Alert   HANDLE=000B PERMIT=06 size=1   ; characteristic index 0
  LECHAR=Control handle=000E PERMIT=06 SIZE=1   ;                index 1
  LECHAR=Test    HANDLE=0014 PERMIT=0A SIZE=2   ;                index 2
  LECHAR=Name    UUID=2A00                      ;                index 3  
DEVICE = Windows PC  TYPE=classic node=10 address=00:1A:7D:DA:71:13
DEVICE = HC-05 TYPE=CLASSIC NODE=7 PIN=1234 CHANNEL=1 ADDRESS = 98:D3:32:31:59:84
DEVICE = Android phone TYPE=CLASSIC NODE=4 ADDRESS = 4C:4E:03:83:CE:B9
```

Text in the devices text file can be upper or lower case and can include spaces.

```
DEVICE = Device name of your choice (don't use any of the key words
                  such as TYPE/NODE/ADDRESS.. or chaos will follow) 
TYPE   = MESH     all Pis running btferret/btlib 
         CLASSIC  classic servers such as Windows/Android/HC05
         LE       LE servers
 
NODE = 4                      Node number in decimal of your choice
ADDRESS = 00:1E:C0:2D:17:7C   6-byte Bluetooth address
CHANNEL = 4                   RFCOMM channel for CLASSIC servers (optional)
PIN = 1234                    PIN code for CLASSIC servers (optional)
```

LE DEVICE entries can be followed by any number of characteristic entries as follows:

```
LECHAR = Characteristic name of your choice
HANDLE = 000B     2-byte handle in hex
PERMIT = 06       Permissions in hex
SIZE = 1          Number of bytes in decimal. Only used for writes - not needed for reads
UUID = 2A00       Not needed if HANDLE specified
```

PERMIT permissions should be one of the following bit combinations:

```
02 = Read
04 = Write no acknowledge
08 = Write with acknowledge
06 = Read/Write no ack
0A = Read/Write ack
```

If PERMIT is not known, find_ctics will
find it and save it in the device information. 

If HANDLE is not known, the UUID can be specified instead.

```
LECHAR = Characteristic name
UUID = 11223344-5566-7788-99AABBCCDDEEFF00
```

With this minimum information, when the characteristic is read via
read_ctic, the code will find the HANDLE and SIZE and save them in the device information.

Additional devices can be found and added to the device information 
via [classic\_scan](#4.2.1-classic\_scan)
or [le\_scan](#4.2.15-le\_scan).
Characteristics are found via [find\_ctics](#4.2.12-find\_ctics).


## 4.2.15 le\_scan

```
void le_scan(void)
```

Scan for active LE devices. If a device is not in the device information, it is
added. (Except if it is transmitting mesh packets and is not in the devices.txt list,
it is ignored for security reasons.) If the device's characteristics are 
unknown, they can be found via [find\_ctics](#4.2.12-find\_ctics),
and they will also be added to the device
information. 

## 4.2.16 list\_channels

```
int list_channels(int node,int flag)
```

List RFCOMM serial channels of a Classic server.

PARAMETERS

```
node = Node number
flag = One of the following
   LIST_SHORT   Short list - names only   
   LIST_FULL    Full information
```

RETURN

```
 -1 = Fail to read channel info
>=0 = Number of channels found
```

SAMPLE CODE

```
list_channels(7,LIST_SHORT);  // list of node 7 RFCOMM serial channel names
list_channels(5,LIST_FULL);   // full info about node 5 serial channels
```


## 4.2.17 list\_ctics

```
int list_ctics(int node,int flag)
```
List characteristics known by device info. If device info
does not have the required knowledge, call [find\_ctics](#4.2.12-find\_ctics)
to read from the 
device itself.

PARAMETERS

```
node = Node number
flag = One of the following
   LIST_FULL            Full information, all characteristics
   LIST_SHORT           Short list of all characteristics - names only   
   LIST_SHORT | CTIC_R  Short list - readable only
   LIST_SHORT | CTIC_W  Short list - writeable only 
```

RETURN

```
 -1 = Fail to read characteristic info
>=0 = Number of characteristics found
```

SAMPLE CODE

```
list_citcs(4,LIST_SHORT);           // list all characteristic names of node 4
list_citcs(4,LIST_SHORT | CTIC_R);  // list readable characteristic names of node 4
list_citcs(4,LIST_SHORT | CTIC_W);  // list writeable characteristic names of node 4

list_ctics(3,LIST_FULL);   // full characteristic info of node 3
                           // known by device info
```
                           
## 4.2.18 list\_uuid

```
int list_uuid(int node,char *uuid)
```

List information about a node's services that contain a
specified 2-byte [UUID](#5.1-what-gives-with-uuids?).

PARAMETERS

```
node = Node number
uuid = 2-byte array containing UUID
```

RETURN

```
0 = Fail
1 = OK
```

SAMPLE CODE

```
List services of node 5 that contain the UUID = 0100

char uuid[2];

uuid[0] = 0x01;
uuid[1] = 0x00;
list_uuid(5,uuid);

The strtohex function can be used to generate the uuid array from an ascii string.

list_uuid(5,strtohex("0100",NULL));

```

## 4.2.19 localnode

```
int localnode(void)
```

Return the node number of the local device - should be set via devices.txt
data in [init\_blue](#4.2.14-init\_blue).

SAMPLE CODE

```
printf("Local device node number = %d\n",localnode());
```


## 4.2.20 mesh\_on

```
void mesh_on(void)
```

Turn on mesh transmission. The local device will continuously send the last
mesh packet set via write\_mesh. Mesh transmission is automatically enabled
by calling [write\_mesh](#4.2.36-write\_mesh), or [read\_mesh](#4.2.27-read\_mesh),
or [mesh\_server](#4.2.22-mesh\_server), or [node\_server](#4.2.23-node\_server),
so it is usually not necessary to call mesh\_on explicitly. Mesh must be on for
another mesh device to connect.


## 4.2.21 mesh\_off

```
void mesh_off(void)
```

Turn off mesh transmission. The local device will stop continuously sending
the last mesh packet set via write_mesh. The purpose is to reduce the load on the 
system when mesh functions are no longer needed, or to make the device invisible.

## 4.2.22 mesh\_server

```
void mesh_server(int (*callback)())
```

Sets up the local device as a mesh server which spends all its time listening
for mesh packets from all other mesh devices, sent via [write\_mesh](#4.2.36-write\_mesh).
The packets are limited to a maximum size of
25 bytes. When a packet is received, it is despatched to the callback
function. The callback function returns a flag telling mesh\_server to continue
or exit. The mesh\_server function only completes when it receives this exit return, or the
x key is pressed.


PARAMETERS

```
callback() = Callback function that deals with the received mesh packet
```

The callback function is defined as follows:

```
int callback(int clientnode,char *data,int datlen)

clientnode = Node number of the device that sent the packet
data = array of packet data bytes
datlen = number of bytes in data[] - max 25
```

SAMPLE CODE

This a minimal mesh server callback that simply prints a message, and exits
when the first data byte is an ascii 'D'. It can also be stopped by
pressing the x key. See btferret.c or sample.c for similar examples.

```
mesh_server(mesh_callback)


int mesh_callback(int clientnode,char *data,int datlen)
  {
  printf("Mesh packet from %s\n",device_name(clientnode));
  if(data[0] == 'D')     // 'D' programmed as exit command
    return(SERVER_EXIT);
  return(SERVER_CONTINUE);
  }  
```

## 4.2.23 node\_server

```
int node_server(int clientnode,int (*callback)(),char endchar)
```

Sets up the local device as a node server that waits for 
a specified client (clientnode) to connect, then spends all its time listening
for node packets sent from that client via [write\_node](#4.2.37-write\_node).
The packets must have the specified
termination character (endchar), and are limited to a maximum size of 400 bytes.
When a packet is received,
it is despatched to the callback
function. The callback function returns a flag telling node\_server to continue
or exit. The node\_server function only completes when it receives this exit return or the
x key is pressed.
When operating as a node\_server, there is a 1:1 connection between the client and
this server - the other mesh devices do not participate. 

PARAMETERS

```
clientnode = Node number of the client that will send packets
callback() = Callback function that deals with the received node packet
endchar = termination character of packets sent by the client
```

RETURN

```
0 = Fail
1 = OK exit via callback returning SERVER_EXIT 
```

The callback function is defined as follows:

```
int callback(int clientnode,char *data,int datlen)

clientnode = Node number of the device that sent the packet
data = array of packet data bytes
datlen = number of bytes in data[] - max 400
```

SAMPLE CODE

This a minimal node server callback that simply prints a message, and exits
when the first data byte is an ascii 'D'. It can also be stopped by
pressing the x key. See btferret.c or sample.c for other examples.

```
   // listen for packets from node 4 with termination character 10
node_server(4,node_callback,10);


int node_callback(int clientnode,char *data,int datlen)
  {
  printf("Node packet from %s\n",device_name(clientnode));
  if(data[0] == 'D')      // 'D' programmed as exit command
    return(SERVER_EXIT);
  return(SERVER_CONTINUE);
  }  
```

## 4.2.24 output\_file

```
int output_file(char *filemame)
```

Save all recent screen output to a file. Screen prints performed by btlib.c functions
(that might have scrolled off the top of the screen) are saved in a buffer.
This function writes the entire buffer to a file.

PARAMETERS

```
filename = Name of output file
```

RETURN

```
0 = Fail - probably fail to open file
1 = OK
```

SAMPLE CODE

```
output_file("/home/pi/output.txt");
```


## 4.2.25 read\_ctic

```
int read_ctic(int node,int cticn,unsigned char *inbuf,int bufsize)
```

Read a characteristic value from a connected LE device.

PARAMETERS

```
node = Node number of connected LE device
buf = char buffer to receive data
bufsize = size of buf
```

RETURN

```
Number of bytes read

inbuf[] = read data
          a termination zero that is not part of the read data
          is added so inbuf[Number read] = 0 
          
The read_error() function returns one of the following:

  0 = OK - no error
  ERROR_FATAL = some other error 
  
One possible fatal error is that the characteristic has been read OK
but the LE device has not set it and Number of bytes read = 0      
          
```


SAMPLE CODE

```
If devices.txt information for init_blue() is as follows:

DEVICE = Pictail TYPE=LE NODE=4 ADDRESS = 00:1E:C0:2D:17:7C
  LECHAR=Alert HANDLE=000B PERMIT=06 SIZE=1               ; index 0
  LECHAR=Test  HANDLE=0014 PERMIT=0A SIZE=2               ; index 1
  LECHAR=My data UUID=11223344-5566-7788-99AABBCCDDEEFF00 ; index 2 


char data[32];
int nread,cticn;

  // connect to LE device Pictail node 4
connect_node(4,0,0); 

  // read Pictail (node 4) characteristic Test (index 1)

nread = read_ctic(4,1,data,sizeof(data));

  // nread should be 2 with the result in data[0] and data[1]
 
  // read Pictail (node 4) characteristic My data
  // find index from UUID which will be 2
cticn = find_ctic_index(4,UUID_16,strtohex("11223344-5566-7788-99AABBCCDDEEFF00",NULL));
nread = read_ctic(4,cticn),data,sizeof(data));
   
   // disconnect Pictail
disconnect_node(4);
```

## 4.2.26 read\_error

```
int read_error(void)
```

Return error state following a read via
[read\_node/all\_endchar](#4.2.29-read\_node/all\_endchar), or
[read\_node\_count](#4.2.28-read\_node\_count), or
[read\_ctic](#4.2.25-read\_ctic).

RETURN

```
  0 = OK - no error
  ERROR_TIMEOUT = timed out
  ERROR_KEY = x key press
  ERROR_FATAL = some other error such as device not connected
```


## 4.2.27 read\_mesh

```
int read_mesh(int *node,char *inbuf,int bufsize,int exitflag,int timeoutms)
```

Reads a mesh packet sent by any other trasmitting mesh device. The maximum size
of a mesh packet is 25 bytes. Mesh reads do not look for a termination character,
they read the full byte count in the packet. The most convenient way to wait for
mesh packets as a server is to use [mesh\_server](#4.2.22-mesh\_server).

PARAMETERS

```
*node = Pointer to integer that will receive sending node number
inbuf = Input buffer that will receive data
bufsize = size of inbuf[]
exitflag = Flag specifies how return is forced if the read fails
           and must be one of the following: 
  EXIT_TIMEOUT  = Exits after timeoutms ms
  EXIT_KEY      = Exits when x key is pressed  
timeoutms = Time out in milliseconds when exitflag = EXIT_TIMEOUT
            Ignored when exitflag = EXIT_KEY
```

RETURN

```
Number of bytes in packet
inbuf[] = received data
          a terminating zero that is not part of the data is added
          so inbuf[Number of bytes] = 0
*node = Node number of sending node
        Zero on failure
        
The read_error() function returns one of the following:

  0 = OK - no error
  ERROR_TIMEOUT = timed out
  ERROR_KEY = x key press
  ERROR_FATAL = some other error       
        
```        

SAMPLE CODE

```
int nread,node;
char inbuf[32];

    // wait 5 seconds for mesh packet
nread = read_mesh(&node,inbuf,sizeof(inbuf),EXIT_TIMEOUT,5000);
if(read_error() == 0)
  printf("Mesh packet from %s\n",device_name(node));
  

do
  {    // wait for a mesh packet or x key press
  nread = read_mesh(&node,inbuf,sizeof(inbuf),EXIT_KEY,0);
  if(nread != 0)
    printf("Got mesh packet\n");
  }
while(read_error() == 0);

```


## 4.2.28 read\_node\_count

```
int read_node_count(int node,char *inbuf,int count,int exitflag,int timeoutms)
```

Read node packet from a specified connected node until a specified 
number of bytes (count) is received (sent via [write\_node](#4.2.37-write\_node)).
If no such packet is received, the
function terminates via a time out or x key press. The [read\_error](#4.2.26-read\_error)
function returns
the error state. Node packets have a maximum size of 400 bytes.

PARAMETERS

```
node = Node number of the sending device
inbuf = Buffer to receive read bytes 
count = Number of bytes expected
exitflag = Flag specifies how return is forced if the read fails
           and must be one of the following: 
  EXIT_TIMEOUT  = Exits after timeoutms ms
  EXIT_KEY      = Exits when x key is pressed  
timeoutms = Time out in milliseconds when exitflag = EXIT_TIMEOUT
            Ignored when exitflag = EXIT_KEY
```

RETURN

```
Number of bytes read

inbuf[] = Received data

  Number of bytes read = count if read succeeds.
  A termination zero that is not part of the
  received data is also added - so inbuf[Number of bytes read] = 0
  On failure, there may be bytes in the buffer.
  
The read_error() function returns one of the following:

  0 = OK - no error
  ERROR_TIMEOUT = timed out
  ERROR_KEY = x key press
  ERROR_FATAL = some other error such as device not connected

```

SAMPLE CODE

```
int nread;
char buff[64];

  // Read 8 bytes from node 3. Time out after 1 second
nread = read_node_count(3,buff,8,EXIT_TIMEOUT,1000);
  // Read 8 bytes from node 6, If not done, exit on x key press
nread = read_node_count(6,buff,8,EXIT_KEY,0);
  // check
if(read_error() != 0)
  printf("Read failed\n");

```

## 4.2.29 read\_node/all\_endchar

```
int read_node_endchar(int node,char *inbuf,int bufsize,char endchar,int exitflag,int timeoutms)
int read_all_endchar(int *node,char *inbuf,int bufsize,char endchar,int exitflag,int timeoutms)
```

Read node packet from a specified connected node, or all connected nodes, until a specified 
termination character (endchar) is received (sent via [write\_node](#4.2.37-write\_node)).
If no such packet is received, the function terminates via a time out or x key press.
The [read\_error](#4.2.26-read\_error) function returns
the error state. The read\_all\_node function listens to all connected nodes and
returns the first packet received. Node packets have a maximum size of 400 bytes.
The most convenient way to wait for node packets as a server is to
use [node\_server](#4.2.23-node\_server).

PARAMETERS

```
For read_node
  node = Node number of the sending device
For read_all
  *node = Pointer to integer that receives the
          node number of the sending device
inbuf = Buffer to receive read bytes 
bufsize = Size of inbuf[]
endchar = Terminate character (usually 10)
exitflag = Flag specifies how return is forced if the read fails
           and must be one of the following: 
  EXIT_TIMEOUT  = Exits after timeoutms ms
  EXIT_KEY      = Exits when x key is pressed  
timeoutms = Time out in milliseconds when exitflag = EXIT_TIMEOUT
            Ignored when exitflag = EXIT_KEY
```

RETURN

```
Number of bytes read

inbuf[] = Received data
  
  If Number of bytes read = n then inbuf[n-1] = endchar if read succeeds.
  A termination zero that is not part of the
  received data is also added - so inbuf[n] = 0
  On failure, there may be bytes in the buffer.
  
*node  For read_all_endchar() the sending node is returned in *node
       Zero on failure
  
The read_error() function returns one of the following:

  0 = OK - no error
  ERROR_TIMEOUT = timed out
  ERROR_KEY = x key press
  ERROR_FATAL = some other error such as device not connected
                or buffer size exceeded

```

SAMPLE CODE

```
int nread,node;
char buff[64];

  // Read bytes from node 3 until termination char 10 received
  // Time out after 1 second
nread = read_node_endchar(3,buff,sizeof(buff),10,EXIT_TIMEOUT,1000);

  // Read bytes from node 6 until termination char 10 received
  // If not done, exit on x key press
nread = read_node_endchar(6,buff,sizeof(buff),10,EXIT_KEY,0);

  // Poll all nodes for a packet. When the first packet arrives
  // keep reading it until termination char 10 received.
  // Return the node number of the sending device in node
  // Time out after 1 second
nread = read_all_endchar(&node,buff,sizeof(buff),10,EXIT_TIMEOUT,1000);

  // check
if(read_error() != 0)
  printf("Read failed\n");

```

## 4.2.30 read\_node/all\_clear

```
void read_node_clear(int node)
void read_all_clear(void)
```

Clears the read input buffer of all queued node packets - from a specified 
node, or all nodes.

PARAMETERS

```
node = Node number - clear all packets from this node
```

SAMPLE CODE

```
read_node_clear(6);   // clear all packets from node 6 
                      // from the input buffer
read_all_clear();     // clear all node packets
                      // from the input buffer
```
 


## 4.2.31 scroll\_back/forward

```
void scroll_back(void)
void scroll_forward(void)
```

Screen prints performed by btlib.c funtions are saved in a buffer. These functions
scroll the screen backwards and forwards through this buffer. Screen prints can
be controlled via [set\_print\_flag](#4.2.32-set\_print\_flag).
The buffer can be saved to a file via [output\_file](#4.2.24-output\_file).

SAMPLE CODE

```
scroll_back();      // scroll screen back through print buffer
scroll_forward();   // scroll screen forwards through print buffer
```

## 4.2.32 set\_print\_flag

```
int set_print_flag(int flag)
```

The print flag controls how btlib.c functions print to the screen. The verbose
mode prints all Bluetooth HCI traffic with details of how the packets are
constructed, the replies expected and explanations of what is going on. This will often
scroll off the top of the screen, but can be seen via
[scroll\_back/forward](#4.2.31-scroll\_back/forward), or the
square bracket keys in btferret, or saved to a file via [output\_file](#4.2.24-output\_file).

PARAMETERS

```
flag = One of the following:
   PRINT_NONE       No prints to screen
   PRINT_NORMAL     Normal prints to screen
   PRINT_VERBOSE    Verbose prints
```

RETURN

```
The previous value of flag which can be
used to re-set the flag after a temporary
change.
```

SAMPLE CODE

```
int savflag;

savflag = set_print_flag(PRINT_VERBOSE);

 // verbose prints here
 
set_print_flag(savflag);  // restore original setting
```

## 4.2.33 strtohex

```
char *strtohex(char *s,int *nbytes)
```

Convert an ascii string in hex format to an array of byte values. This can
be used to specify a UUID in [find\_ctic\_index](#4.2.13-find\_ctic\_index)
and [find\_channel](#4.2.11-find\_channel).

PARAMETERS

```
s      = ascii string (see below for formats)
*nbytes = pointer to integer to receive number of bytes in array
         (NULL if not required) 
         
Valid formats for s (strtohex returns dat and *nbytes):

  112233AABB   nbytes=5 dat[0]=0x11 dat[1]=0x22 etc. 
  12233        nbytes=3 dat[0]=0x01 dat[1]=0x22 dat[3]=0x33
  0000-1000-ABcdEF  nbytes=7 dat[0]=0x00 .. dat[6]=0xEF
  D 22 AA B 0    nbytes=5 dat[0]=0x0D ... dat[4]=0
  1,2,3,4f,de,A  nbytes=6 dat[0]=0x01 ... dat[5]=0x0A
        
```

RETURN

```
pointer to array of bytes
*nbytes = number of bytes in array (if nbytes != NULL)
```


SAMPLE CODE

```
int n,count,channel;
char *dat;

dat = strtohex("1122334455",&count);
for(n = 0 ; n < count ; ++n)
  printf("%02X ",dat[n]);
printf("\n");
  
   // find channel of node 5 serial channel with UUID = 1101
channel = find_channel(5,strtohex("1101",NULL));

```

## 4.2.34 wait\_for\_disconnect

```
int wait_for_disconnect(int node,int timout)
```

Wait for server to initiate disconnection. See explanation in
[disconnect\_node](#4.2.10-disconnect\_node).


PARAMETERS

```
node = Node number of server that is expected to initiate disconnection
timout = Time out in ms. If the function times out, the local device
         initiates the disconnection.
```

RETURN

```
0 = Fail
1 = OK
```

## 4.2.35 write\_ctic

```
int write_ctic(int node,int cticn,unsigned char *outbuf,int count)
```

Write characteristic data to a connected LE device.

PRAMETERS

```
node = Node number of connected LE device
cticn = Characteristic index in device information
outbuf = char array of data to be written
count = 0   Use the characteristic size known by device information.
        >0  Use this byte count and override device info size 
```

RETURN

```
Number of bytes written
```

SAMPLE CODE

```
If devices.txt information for init_blue() is as follows:

DEVICE = Pictail TYPE=LE NODE=4 ADDRESS = 00:1E:C0:2D:17:7C
  LECHAR=Alert HANDLE=000B PERMIT=06 SIZE=1                 ; index 0
  LECHAR=Test  HANDLE=0014 PERMIT=0A SIZE=2                 ; index 1
  LECHAR=My data UUID = 11223344-5566-7788-99AABBCCDDEEFF00 ; index 2 


int cticn;
char data[8];

  // connect to LE device Pictail node 4
connect_node(4,0,0); 

  // write two bytes 12 34 to Pictail (node 4) 
  // characteristic Test (index 1) - size 2 known by device info
  // so set last parameter count=0
data[0] = 0x12;
data[1] = 0x34;
write_ctic(4,1,data,0);
 
  // write one byte 56 to characteristic My data in Pictail (node 4)
  // Size unknown by device info so set last parameter
  // (count=1) in write_ctic to write one byte.
  // If not known, find index (which is 2) from UUID 

data[0] = 0x56;
cticn = find_ctic_index(4,UUID_16,strtohex("11223344-5566-7788-99AABBCCDDEEFF00",NULL));
write_ctic(4,cticn,data,1);
   
   // disconnect Pictail
disconnect_node(4);
```

## 4.2.36 write\_mesh

```
int write_mesh(char *outbuf,int count)
```

Broadcast a mesh packet. This packet will be transmitted repeatedly until another
write_mesh changes the data, or mesh transmission is turned off via
[mesh\_off](#4.2.21-mesh\_off).
All other mesh devices can read the
packet via [read\_mesh](#4.2.27-read\_mesh).
Other mesh devices running a [mesh\_server](#4.2.22-mesh\_server) will read the 
packet and pass it to their callback function. The maximum size of a mesh packet is
25 bytes. Mesh reads do not look for a termination character, they read the full
byte count of the packet.


## ***** WARNING ****

Mesh packets are transmitted publically with no encryption and can be read by
any Bluetooth device in the vicinity, so they are NOT SECURE.

PARAMETERS

```
outbuf = char array with packet data
count = number of bytes (max 25)
```

RETURN

```
Number of bytes written
```

SAMPLE CODE

```
char data[4];


data[0] = 0x00;
data[1] = 0x11;
data[2] = 0x22;

  // start transmitting a 3-byte mesh packet
  
mesh_write(data,3);
```


## 4.2.37 write\_node

```
int write_node(int node,unsigned char *outbuf,int count)
```

Write a node packet to the specified connected classic server
or mesh device connected as a node server. The remote node
must have been connected via [connect\_node](#4.2.3-connect\_node). A node server reads
the packet via the various read_node functions.

PARAMETERS

```
node = Node number of connected node
outbuf = char array of data
count = number of bytes to transmit
        If the node is a classic server, the
        maximun count is 1000.
        If the node is a mesh device connected
        as a node server, the maximum count is 400
```

RETURN

```
Number of bytes written
```

SAMPLE CODE

```
char buf[4];

buf[0] = 0x00;
buf[1] = 0x11;
buf[2] = 0x22;
    
  // connect node 7 which is a mesh device
connect_node(7,0,0);
  // send 3 bytes to node 7
write_node(7,buf,3);

  // connect to node 5 which is a classic server
  // on RFCOMM channel 4
connect_node(5,CHANNEL_NEW,4);
  // send 3 bytes to node 5
write_node(5,buf,3);

```                  


## 5 Reference

## 5.1 What gives with UUIDs?

When making a serial data connection to a remote Classic server, the server will listen on a
particular RFCOMM channel (a small number like 1,2,3..). The client must
try to connect on that channel. If this RFCOMM channel number is known, then the UUID
is irrelevant - it is not needed to make the connection.
Each serial RFCOMM channel on the server has an associated UUID. This can be a
2-byte or a 16-byte number, and can be anything - it is just a number that is a free choice.
There are two standard UUIDs for serial channels, but any other number can be registered
as a serial channel UUID.

```
Standard 2-byte Serial Port
   UUID = 1101
Standard 16-byte Serial Port
   UUID = 00001101-0000-1000-8000-00805F9B34FB
   (note the 3rd/4th bytes are the 2-byte UUID 1101)
My custom serial
   UUID = FCF05AFD-67D8-4F41-83F5-7BEE22C03CDB
```

The connection between the UUID and the associated RFCOMM channel is contained in
the SDP database on the server. If the client does not know the RFCOMM channel, but
does know the UUID, it must first read the server's SDP database and search it for 
the UUID entry to find the RFCOMM channel. Some devices have fixed RFCOMM channel
numbers that do not change, but others allocate them as needed and they can only be
found at connection time via the UUID.
The SDP database of the client making the connection is irrelevant. Here is a typical SDP database
record for a serial channel. (decode info v3,pB,3)

```
 **** RECORD 3 ****
    aid = 0000
        Handle 00 01 00 40 
    aid = 0001
      UUID FC F0 5A FD 67 D8 4F 41 83 F5 7B EE 22 C0 3C DB 
    aid = 0004
        UUID 01 00 L2CAP
        UUID 00 03 RFCOMM
        RFCOMM channel = 6
    aid = 0005
      UUID 10 02 
    aid = 0100
        My custom serial
    aid = 0101
        Comment
```

The UUID is in the aid=1 entry. The aid=4 entry identifies this record as an RFCOMM channel.

A channel can be specified in the device information via the devices.txt
file passed to [init\_blue](#4.2.14-init\_blue). 
BTlib also has functions for reading a remote device's SDP
database [list\_channels](#4.2.16-list\_channels),
finding the RFCOMM channel of a specified UUID [find\_channel](#4.2.11-find\_channel).
Connecting to a classic device [connect\_node](#4.2.3-connect\_node)
needs the RFCOMM channel number.
To see full SDP database records as above, set verbose print mode
during [list\_channels](#4.2.16-list\_channels) or use [list\_uuid](#4.2.18-list\_uuid).      

The situation with LE devices is similar. LE devices hold characteristics that are values 
that can be read and sometimes written (e.g. Heart rate). Each characteristic is identified
by a handle that is a 2-byte hex number like 000A,0010,0012... If the handle is known then
the UUID is irrelevant - everything can be done by knowing the handle.
Each characteristic has an associated UUID that can be 2-byte or 16-byte. Again, if the
client knows the UUID, it can read information from the LE device to find the associated handle.
There are some standard 2-byte UUIDs that identify the type of characteristic;

```
2A00 = Device Name
2A19 = Battery Level
2A37 = Heart Rate
...etc.
```
The full list can be found in a pdf document called "16-bit UUID Numbers Document"
[here](https://www.bluetooth.com/specifications/assigned-numbers/). 

A handle or UUID can be pre-set in the device information via the devices.txt file
passed to [init\_blue](#4.2.14-init\_blue).
 
BTlib also has functions for reading an LE device's characteristic/UUID list
[find\_ctics](#4.2.12-find\_ctics) which
adds the list to the device information, listing it
[list\_ctics](#4.2.17-list\_ctics), and subsequently
[find\_ctic\_index](#4.2.13-find\_ctic\_index)
to find the characteristic
index of a specified UUID in the device information. Functions to
read and write characteristics
[read\_ctic](#4.2.25-read\_ctic),
[write\_ctic](#4.2.35-write\_ctic)
need this index. 

 
## 5.2 Packet formats

These are the formats of packets exchanged at the HCI level.

## 5.2.1 Starting 01 HCI Commands

HCI commands to read information, connect to a remote device, etc..
Reference Vol 2 Part E Section 7

```
Example packet

01 06 04 03 40 00 13

[0] = 01 HCI command packet
[1] = 06 OCF
[2] = 04 OGF*4
[3] = 03 number of bytes starting from [4]
[4]-[6] = 3 parameter bytes as defined for this OGF/OCF

OGF and OCF are command opcodes in [1][2] as follows:

        [2]       [1]
       0   4     0    6
bits 0000 0100 0000 0110    

OGF = top 6 bits 000001 = 1
OCF = bottom 10 bits 0000000110 = 6

OGF OGF*4   Reference Vol 2 Part E Section
1    04     7.1 Link control
2    08     7.2 Link policy
3    0C     7.3 Control
4    10     7.4 Info  
5    14     7.5 Status
8    20     7.8 LE

Each of these OGF reference sections is a list of OCF commands.
```

## 5.2.2 Starting 04 HCI Events

Mostly replies from HCI commands. Reference Vol 2 Part E Section 7.7

```
Example packet

04 05 04 00 40 00 16

[0] = 04 HCI event packet
[1] = 05 Event opcode
[2] = 04 Number of bytes starting from [3]
[3]-[6] = 4 bytes of parameters as defined for this Event

The reference section 7.7 is a list of Event opcodes.
```

## 5.2.3 Starting 02 Channel 0001

Request/reply exchanges during connection establishment.
Reference Vol 3 Part A Section 4

```
Example packet

02 40 00 0A 00 06 00 01 00 0A 06 02 00 01 02

[0] = 02 L2CAP packet
[1][2] = 0040 Handle of connection (low 12 bits)
[3][4] = 000A Number of bytes starting from [5]
[5][6] = 0006 Number of bytes starting from [9]
         (but see multiple packets description in 5.2.4)
[7][8] = 0001 Channel 0001
[9]  = 0A Opcode
[10] = 06 ID
[11][12] = 0002 Number of bytes starting from [13]
[13]-[16] = 2 bytes of parameters 

These packets come in request/reply pairs. A device sends
a Request_Opcode:ID where the ID is a free choice. It
expects a reply: Reply_Opcode:ID with the same ID.
These are the relevant opcodes.

Request Reply Purpose
  02     03   connect
  04     05   config
  06     07   disconnect
  0A     0B   info
  0C     0D   channel
  
So an opcode:ID = 0A:04 expects a reply 0B:04 
```

## 5.2.4 Starting 02 Channel 0004

LE packets. Reference Vol 3 Part F Section 3.4

```
Example packet

02 40 00 08 00 04 00 04 00 52 12 00 05

[0] = 02 L2CAP packet
[1][2] = 0040 Handle of connection (low 12 bits)
[3][4] = 0008 Number of bytes starting from [5]
[5][6] = 0004 Number of bytes starting from [9]
         (but see multiple packets description next)
[7][8] = 0004 Channel 0004 LE
[9] = 52 Opcode
[10]-[12] = 3 parameter bytes 

The relevant opcodes for LE commands are
0A  Read characteristic request
0B  Read response
52  Write characteristic command - no ack
12  Write characteristic request - ack
13  Write ack response
```

This packet format is also used for data packets sent via
[write\_node](#4.2.37-write\_node) to a connected mesh device acting as a node
server (it is an LE connection and the data is sent as LE packets). 
In this case there is no opcode - just data as shown in
the multiple packets example next.

Sometimes there is too much data to fit in one packet, and it is
then split over mutliple packets as follows:

```
First packet
02 41 20 1B 00 3B 00 04 00 48 65 72 65 20 69 73 
20 61 20 6C 6F 6E 67 20 6D 65 73 73 61 67 65 20 

[3][4] = 001B Number of bytes in this packet from [5]
[5][6] = 003B Total number of data bytes in all packets from [9]
[9]... = Data

The data in this packet runs from [9] to [4+001B] = 0017 bytes
The total data expected is 003B so need an extra 0024 bytes

Second packet
02 41 10 1B 00 74 6F 20 69 6C 6C 75 73 74 72 61 
74 65 20 6D 75 6C 74 69 70 6C 65 20 64 61 74 61 

[3][4] = 001B Number of bytes in this packet from [5]
[5]..  = Data

The data in this packet runs from [5] to [4+001B] = 001B bytes
Needed another 0024, so now need an extra 0009 bytes

Third packet
02 41 10 09 00 20 70 61 63 6B 65 74 73 0A 

[3][4] = 0009 Number of bytes in this packet from [5]
[5]..  = Data

Here are the last 0009 bytes from [5] to [4+0009]

```

## 5.2.5 Starting 02 Channel 0040+

These packets are used by serial RFCOMM connections to a Classic server.
Reference [RFCOMM](https://www.bluetooth.com/specifications/protocol-specifications/) section here.

```
Example packet

02 0C 00 09 00 05 00 40 00 09 EF 03 11 40

[0] = 02 L2CAP packet
[1][2] = 000C Handle of connection (low 12 bits)
[3][4] = 0009 Number of bytes starting from [5]
[5][6] = 0005 Number of bytes starting from [9]
         (but see multiple packets description in 5.2.4)
[7][8] = 0040 Channel >= 0040
[9]  = 09  Address of CONTROL or RFCOMM channel
[10] = EF  Control (opcode)
[11] = 03  Length first byte (Number of data bytes = 1)
[12] = 11  One data byte
[13] = 40  FCS check byte

There may be one or two Length bytes. If the low bit
of the first byte is 1 (as here) there is only one
Length byte and the number of data bytes is the high 7
bits (1 here). If the low bit is zero, there are two
length bytes and the number of bytes is the high 7 bits
of the first, plus the second << 7. The number of data bytes
does not include the final error check FCS.
```

## 5.3 Procedures

These procedure setions are largely as produced by the verbose print mode
with added explanations.

## 5.3.1 HCI socket - read/write packets

Bluetooth packets are sent and received through an HCI socket opened
as follows.

```
void hcisock()
  {
  int dd;
  struct sockaddr_hci sa;
  struct hci_filter flt;
  struct hci_dev_req dr;
    
  // turn bluez off if it is running
 
  dd = socket(31,SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK,BTPROTO_HCI);
  ioctl(dd,HCIDEVDOWN,0);   // hci0
  close(dd);                // finished with bluez
   
  // open HCI socket 
  
  dd = socket(AF_BLUETOOTH,SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK,BTPROTO_HCI);

  flt.type_mask = 0x16;    
  flt.event_mask[0] = 0xFFFFFFFF;
  flt.event_mask[1] = 0xFFFFFFFF;
  flt.opcode = 0;
    
  setsockopt(dd,SOL_HCI,HCI_FILTER,&flt,sizeof(flt));
 
  sa.hci_family = AF_BLUETOOTH;
  sa.hci_dev = 0;   // hci0
  sa.hci_channel = HCI_CHANNEL_USER    
  
  bind(dd,(struct sockaddr *)&sa,sizeof(sa));
       
  dr.dev_id = 0;  // hci0
  dr.dev_opt = SCAN_PAGE | SCAN_INQUIRY;
  ioctl(dd,HCISETSCAN,(unsigned long)&dr);
  
   // dd is needed for read/write/close
 
   // Read/write HCI packets via:

  read(dd,inbuf,sizeof(inbuf));
  write(dd,outbuf,count);
  
   // Close 
 
  close(dd);
  }  
  
In the following sections, the packets are shown as follows.

< indicates a packet sent to the Bluetooth adapter
The full packet (01 01 0C 08...) is listed on the lines starting 0000
which is the hex address of the packet data. The Set addresses such
as [4] refer to offsets in the packet data.

  Set [4].. board address reversed 56..DC
< HCI OGF=01 OCF=05
      0000  01 05 04 0D 56 DB 04 32 - A6 DC 18 CC 02 00 00 00 
      0010  01 

> indicates a packet received from the Bluetooth adapter

> Event 0E = 01 01 0C 00
      0000  04 0E 04 01 01 0C 00 

```

## 5.3.2 Classic procedures 

This connects to a classic device server, sets up a serial channel, exchanges
a few serial data packets with the server and disconnects. It includes
[HCI Commands](#5.2.1-starting-01-hci-commands),
[HCI Events](#5.2.2-starting-04-hci-events),
[L2CAP 0001 packets](#5.2.3-starting-02-channel-0001), and
[L2CAP 0040+ packets](#5.2.5-starting-02-channel-0040+).
The disconnection here is initiated by the client, but there
may be reasons for letting the server initiate - see 
[disconnect\_node](#4.2.10-disconnect\_node), and
[Classic disconnect initiated by server](#5.3.4-classic-disconnect-initiated-by-server).

```
Connect to Pi4 (DC:A6:32:04:DB:56) on RFCOMM channel 2
An HCI socket must be open - see separate section

Set event mask
< HCI OGF=03 OCF=01
      0000  01 01 0C 08 FF FF FB FF - 07 F8 BF 3D 
> Event 0E = 01 01 0C 00
      0000  04 0E 04 01 01 0C 00 

Set simple pair mode on
< HCI OGF=03 OCF=56
      0000  01 56 0C 01 01 
> Event 0E = 01 56 0C 00
      0000  04 0E 04 01 56 0C 00 

Open classic connection to DC...56
  Set [4].. board address reversed 56..DC
< HCI OGF=01 OCF=05
      0000  01 05 04 0D 56 DB 04 32 - A6 DC 18 CC 02 00 00 00 
      0010  01 
> Event 0F = 00 01 05 04
      0000  04 0F 04 00 01 05 04 
> Event 12 = 00 56 DB 04 32 A6 DC 01
      0000  04 12 08 00 56 DB 04 32 - A6 DC 01 
> Event 03 = 00 0C 00 56 DB 04 32 A6 DC 01...
      0000  04 03 0B 00 0C 00 56 DB - 04 32 A6 DC 01 00 
GOT Open OK (Event 03) with handle = 000C     
> Event 1B = 0C 00 05
      0000  04 1B 03 0C 00 05 
> L2CAP 0001 Opcode:id = 0A:01  Must send reply 0B:01
      0000  02 0C 20 0A 00 06 00 01 - 00 0A 01 02 00 02 00 
GOT ask info (opcode 0A) type 2

SEND info reply type 2
  Set [1][2] handle 0C 00
  Set [10] id 01
< L2CAP 0001 Opcode:id = 0B:01   is reply to 0A:01
      0000  02 0C 00 10 00 0C 00 01 - 00 0B 01 08 00 02 00 00 
      0010  00 B8 02 00 00 
> L2CAP 0001 Opcode:id = 0A:02  Must send reply 0B:02
      0000  02 0C 20 0A 00 06 00 01 - 00 0A 02 02 00 03 00 
GOT ask info (opcode 0A) type 3

SEND info reply type 3
  Set [1][2] handle 0C 00
  Set [10] id 02
< L2CAP 0001 Opcode:id = 0B:02   is reply to 0A:02
      0000  02 0C 00 14 00 10 00 01 - 00 0B 02 0C 00 03 00 00 
      0010  00 86 00 00 00 00 00 00 - 00 
> Event 13 = 01 0C 00 02 00
      0000  04 13 05 01 0C 00 02 00 

SEND Authentication request
  Set [4][5] handle 0C 00
< HCI OGF=01 OCF=11
      0000  01 11 04 02 0C 00 
> Event 0F = 00 01 11 04
      0000  04 0F 04 00 01 11 04 
> Event 17 = 56 DB 04 32 A6 DC
      0000  04 17 06 56 DB 04 32 A6 - DC 
GOT link request (Event 17)

SEND link request neg reply
  Set [4].. board address reversed 56..DC
< HCI OGF=01 OCF=0C
      0000  01 0C 04 06 56 DB 04 32 - A6 DC 
> Event 0E = 01 0C 04 00 56 DB 04 32 A6 DC
      0000  04 0E 0A 01 0C 04 00 56 - DB 04 32 A6 DC 
> Event 31 = 56 DB 04 32 A6 DC
      0000  04 31 06 56 DB 04 32 A6 - DC 
NO PIN code request
GOT IO capability request (Event 31)
Event 31 does not require a PIN

SEND IO capability request reply
  Set [4].. board address reversed 56..DC
< HCI OGF=01 OCF=2B
      0000  01 2B 04 09 56 DB 04 32 - A6 DC 03 00 00 
> Event 0E = 01 2B 04 00 56 DB 04 32 A6 DC
      0000  04 0E 0A 01 2B 04 00 56 - DB 04 32 A6 DC 
> Event 32 = 56 DB 04 32 A6 DC 03 00 00
      0000  04 32 09 56 DB 04 32 A6 - DC 03 00 00 
> Event 33 = 56 DB 04 32 A6 DC 1E 22 0A 00
      0000  04 33 0A 56 DB 04 32 A6 - DC 1E 22 0A 00 
GOT confirmation request (Event 33)

SEND confirmation request reply
  Set [4].. board address reversed 56..DC
< HCI OGF=01 OCF=2C
      0000  01 2C 04 06 56 DB 04 32 - A6 DC 
> Event 0E = 01 2C 04 00 56 DB 04 32 A6 DC
      0000  04 0E 0A 01 2C 04 00 56 - DB 04 32 A6 DC 
> Event 36 = 00 56 DB 04 32 A6 DC
      0000  04 36 07 00 56 DB 04 32 - A6 DC 
> Event 18 = 56 DB 04 32 A6 DC C1 C7 B4 A1...
      0000  04 18 17 56 DB 04 32 A6 - DC C1 C7 B4 A1 8D 94 23 
      0010  D3 A4 A2 98 BF 25 20 D0 - 28 04 
> Event 06 = 00 0C 00
      0000  04 06 03 00 0C 00 
GOT pairing complete (Event 36)
GOT Authentication/pair OK (Event 06)

SEND encrypt
  Set [4][5] handle 0C 00
< HCI OGF=01 OCF=13
      0000  01 13 04 03 0C 00 01 
> Event 0F = 00 01 13 04
      0000  04 0F 04 00 01 13 04 
> Event 08 = 00 0C 00 01
      0000  04 08 04 00 0C 00 01 
GOT Encrypt OK (Event 08)

***** Opened OK *****  

(At this point the SDP database may be read. See 
Read SDP database section that starts at this point)

Create a serial link by opening three channels:

1. L2CAP psm 3 channel
2. CONTROL channel
3. RFCOMM channel

SEND connect L2CAP channel psm 3
  Choose local channel 0042 (Must be >= 0x0040). Choose id 3
  Set [13] psm 03
  Set [1][2] handle 0C 00
  Set [10] id 03 your choice
  Set [15][16] local L2CAP channel 0042 (your choice >= 0040)
< L2CAP 0001 Opcode:id = 02:03   Expects reply 03:03
      0000  02 0C 00 0C 00 08 00 01 - 00 02 03 04 00 03 00 42 
      0010  00 
> L2CAP 0001 Opcode:id = 03:03  is reply from 02:03
      0000  02 0C 20 10 00 0C 00 01 - 00 03 03 08 00 40 00 42 
      0010  00 00 00 00 00 
> L2CAP 0001 Opcode:id = 04:03  Must send reply 05:03
      0000  02 0C 20 1B 00 17 00 01 - 00 04 03 13 00 42 00 00 
      0010  00 01 02 F5 03 04 09 00 - 00 00 00 00 00 00 00 00 
 (might get a result=1 reply pending at this point
  wait for a result=0 OK reply. See Read SDP database
  section for an example)
GOT connect OK reply with remote channel
  Remote channel for following sends = 0040

SEND L2 config request. Choose id 04
  Set [1][2] handle 0C 00
  Set [10] id 04
  Set [13][14] remote L2CAP channel 40 00
< L2CAP 0001 Opcode:id = 04:04   Expects reply 05:04
      0000  02 0C 00 1B 00 17 00 01 - 00 04 04 13 00 40 00 00 
      0010  00 01 02 F5 03 04 09 00 - 00 00 00 00 00 00 00 00 
> Event 13 = 01 0C 00 02 00
      0000  04 13 05 01 0C 00 02 00 
> L2CAP 0001 Opcode:id = 05:04  is reply from 04:04
      0000  02 0C 20 12 00 0E 00 01 - 00 05 04 0A 00 42 00 00 
      0010  00 00 00 01 02 F5 03 
GOT L2 config reply
GOT L2 request config - id 03

SEND L2 config reply with received id
  Set [1][2] handle 0C 00
  Set [10] id 03
  Set [13][14] remote L2CAP channel 40 00
< L2CAP 0001 Opcode:id = 05:03   is reply to 04:03
      0000  02 0C 00 12 00 0E 00 01 - 00 05 03 0A 00 40 00 00 
      0010  00 00 00 01 02 F5 03 
Connect L2CAP psm 3 now done OK

SEND Open CONTROL channel 0
  Command address = 03
  Reply address = 01
  DLCI = 0
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
  Set [12] FCS=1C calculated for 3 bytes from [9]
< CONTROL Address:Opcode = 03:3F
      0000  02 0C 00 08 00 04 00 40 - 00 03 3F 01 1C 
> Event 13 = 01 0C 00 02 00
      0000  04 13 05 01 0C 00 02 00 
> CONTROL Address:Opcode 03:73
      0000  02 0C 20 08 00 04 00 42 - 00 03 73 01 D7 
GOT UA reply (03 73)

Open RFCOMM channel 02
  These values are needed by subsequent commands:
   - Command address (channel<<3+3) = 13
   - Reply address   (channel<<3+1) = 11
   - DLCI (channel<<1) = 04
SEND PN CMD to set RFCOMM channel and parameter negotiation
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
  Set [22] FCS=70 calculated for 2 bytes from [9]
  Frame size at [18] = 0400
  Set [14] DLCI 04
< CONTROL Address:Opcode = 03:EF
      0000  02 0C 00 12 00 0E 00 40 - 00 03 EF 15 83 11 04 F0 
      0010  07 00 00 04 00 07 70 
> CONTROL Address:Opcode 01:EF
      0000  02 0C 20 12 00 0E 00 42 - 00 01 EF 15 81 11 04 E0 
      0010  07 00 F0 03 00 07 AA 
GOT PN RSP reply (01 EF 81)
CONTROL channel OK

SEND Open RFCOMM channel 02
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
  Set [9] RFCOMM address 13
  Set [12] FCS=96 calculated for 3 bytes from [9]
< RFCOMM Address:Opcode = 13:3F
      0000  02 0C 00 08 00 04 00 40 - 00 13 3F 01 96 
> Event 13 = 01 0C 00 02 00
      0000  04 13 05 01 0C 00 02 00 
> RFCOMM Address:Opcode 13:73
      0000  02 0C 20 08 00 04 00 42 - 00 13 73 01 5D 
> CONTROL Address:Opcode 01:EF
      0000  02 0C 20 0C 00 08 00 42 - 00 01 EF 09 E3 05 13 8D 
      0010  AA 
GOT UA reply (13 73)

SEND MSC CMD to set modem status
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
  Set [16] FCS=70 calculated for 2 bytes from [9]
  Set [14] RFCOMM address 13
< CONTROL Address:Opcode = 03:EF
      0000  02 0C 00 0C 00 08 00 40 - 00 03 EF 09 E3 05 13 8D 
      0010  70 

SEND MSC RSP
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
  Set [16] FCS=70 calculated for 2 bytes from [9]
  Set [14] RFCOMM address 13
< CONTROL Address:Opcode = 03:EF
      0000  02 0C 00 0C 00 08 00 40 - 00 03 EF 09 E1 05 13 8D 
      0010  70 
> CONTROL Address:Opcode 01:EF
      0000  02 0C 20 0C 00 08 00 42 - 00 01 EF 09 E1 05 13 8D 
      0010  AA 
> Event 13 = 01 0C 00 02 00
      0000  04 13 05 01 0C 00 02 00 
> RFCOMM Address:Opcode 11:FF
      0000  02 0C 20 09 00 05 00 42 - 00 11 FF 01 21 A3 
RFCOMM channel OK

**** Connected OK **** - the server will now accept the connection

Serial RFCOMM channel is now open for read/write

At any time the server may try to read our local SDP database
by opening an L2CAP psm 1 channel as follows.
This is none of its business so fob it off.

> L2CAP 0001 Opcode:id = 02:04  Must send reply 03:04
      0000  02 0C 20 0C 00 08 00 01 - 00 02 04 04 00 01 00 41 
      0010  00 
GOT connect request 02:04 to read SDP
SEND Fob it off with a No Resources reply
  Set [1][2] handle 0C 00
  Set [10] id 04
< L2CAP 0001 Opcode:id = 03:04   is reply to 02:04
      0000  02 0C 00 10 00 0C 00 01 - 00 03 04 08 00 00 00 00 
      0010  00 04 00 00 00 
      
The client now gives the server a specified number of credits.
Each time the server sends a serial packet it uses up a credit.
The client must keep track of the credits the server has left.
When they run low they must be topped up again.

SEND Top up server's credits by 100 (0x64)
  Set [12] credits = 64 
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
  Set [9] RFCOMM address 13
  Set [13] FCS=79 calculated for 2 bytes from [9]
< RFCOMM Address:Opcode = 13:FF
      0000  02 0C 00 09 00 05 00 40 - 00 13 FF 01 64 79 
      
Some serial data exchanges follow.      
      
Send a single byte (0A) to server.      
The server has been programmed to treat OA as a termination byte
and to respond to a single 0A byte with an "OK" reply       
      
SEND DATA 1 bytes =  0A
  Set [11] one length byte = 03 (length*2 + 1 to indicate 1 byte)
  Set [3].. packet lengths 09 00 05 00
  Set [12].. 1 data bytes 0A..
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
  Set [9] RFCOMM address 13
  Set [13] FCS=65 calculated for 2 bytes from [9]
< RFCOMM Address:Opcode = 13:EF  Send serial data
      0000  02 0C 00 09 00 05 00 40 - 00 13 EF 03 0A 65 
> Event 13 = 01 0C 00 01 00
      0000  04 13 05 01 0C 00 01 00 
      
Reply comes back with an 0A termination

> RFCOMM Address:Opcode 11:EF  received serial data
      0000  02 0C 20 0B 00 07 00 42 - 00 11 EF 07 4F 4B 0A BF 
      TEXT  OK
The number of bytes (3) is at [11] and follows the same rules as
the length byte in the SEND DATA packet (also see long message below).
Has used a credit. Number remaining = 99

Some servers send data with an FF opcode instead of EF,
and a slightly different format. Here is the same OK reply
      
Reply comes back with an FF opcode and an extra byte 01

> RFCOMM Address:Opcode 11:FF received serial data
      0000  02 0C 20 0C 00 08 00 42 - 00 11 FF 07 01 4F 4B 0A 
      0010  A3 
      TEXT  OK
  Has used a credit. Number remaining = 98

If there are more than 127 data bytes, two length bytes
are needed as follows. The same rule applies to the received
data packets. Packets must be smaller than the frame size 
set via PN CMD above.

SEND DATA 131 bytes = Here is a long message...
  Set [11][12] two length bytes = 06 01 (length*2) (length >> 7)
  Set [3].. packet lengths 8C 00 88 00
  Set [13].. 131 data bytes 48..
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
  Set [9] RFCOMM address 13
  Set [144] FCS=65 calculated for 2 bytes from [9]
< RFCOMM Address:Opcode = 13:EF  Send serial data
      0000  02 0C 00 8C 00 88 00 40 - 00 13 EF 06 01 48 65 72 
      0010  65 20 69 73 20 61 20 6C - 6F 6E 67 20 6D 65 73 73 
      0020  61 67 65 20 74 6F 20 69 - 6C 6C 75 73 74 72 61 74 
      0030  65 20 64 61 74 61 20 73 - 65 6E 64 73 20 6F 66 20 
      0040  6D 6F 72 65 20 74 68 61 - 6E 20 31 32 37 20 63 68 
      0050  61 72 61 63 74 65 72 73 - 20 74 68 61 74 20 72 65 
      0060  71 75 69 72 65 20 74 77 - 6F 20 6C 65 6E 67 74 68 
      0070  20 62 79 74 65 73 20 69 - 6E 20 74 68 65 20 62 6C 
      0080  75 65 74 6F 6F 74 68 20 - 70 61 63 6B 65 74 21 0A 
      0090  65 

Send a "hello" ascii data message (plus termination byte 0A)
The server has been programmed to reply "hello to you"

SEND DATA 6 bytes = hello 0A
  Set [11] one length byte = 0D (length*2 + 1 to indicate 1 byte)
  Set [3].. packet lengths 0E 00 0A 00
  Set [12].. 6 data bytes 68..
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
  Set [9] RFCOMM address 13
  Set [18] FCS=65 calculated for 2 bytes from [9]
< RFCOMM Address:Opcode = 13:EF  Send serial data
      0000  02 0C 00 0E 00 0A 00 40 - 00 13 EF 0D 68 65 6C 6C 
      0010  6F 0A 65 
> Event 13 = 01 0C 00 02 00
      0000  04 13 05 01 0C 00 02 00 

Reply comes back with an 0A termination

> RFCOMM Address:Opcode 11:EF   received serial data
      0000  02 0C 20 15 00 11 00 42 - 00 11 EF 1B 68 65 6C 6C 
      0010  6F 20 74 6F 20 79 6F 75 - 0A BF 
      TEXT  hello to you
  Has used a credit. Number remaining = 97

Disconnect initiated by client

(The server may or may not recognise the disconnection and may
continue to wait for serial input. To allow for this, disconnect
can also be initiated by the server - see separate section)

Close all the open channels in reverse order.

SEND Close RFCOMM request (opcode 53)
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
  Set [9] RFCOMM address 13
  Set [12] FCS=77 calculated for 3 bytes from [9]
< RFCOMM Address:Opcode = 13:53
      0000  02 0C 00 08 00 04 00 40 - 00 13 53 01 77 
> Event 13 = 01 0C 00 01 00
      0000  04 13 05 01 0C 00 01 00 
> RFCOMM Address:Opcode 13:73
      0000  02 0C 20 08 00 04 00 42 - 00 13 73 01 5D 
GOT UA reply (opcode 73)
RFCOMM channel closed

SEND Close CONTROL request (opcode 53)
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
  Set [12] FCS=FD calculated for 3 bytes from [9]
< CONTROL Address:Opcode = 03:53
      0000  02 0C 00 08 00 04 00 40 - 00 03 53 01 FD 
> Event 13 = 01 0C 00 02 00
      0000  04 13 05 01 0C 00 02 00 
> CONTROL Address:Opcode 03:73
      0000  02 0C 20 08 00 04 00 42 - 00 03 73 01 D7 
> L2CAP 0001 Opcode:id = 06:05  Must send reply 07:05
      0000  02 0C 20 0C 00 08 00 01 - 00 06 05 04 00 42 00 40 
      0010  00 
GOT UA reply (opcode 73)
CONTROL channel closed (note the L2CAP disconnect request)

SEND disconnect L2CAP channel. Choose id 08
  Set [1][2] handle 0C 00
  Set [10] id 08
  Set [13][14] remote L2CAP channel 40 00
  Set [15][16] local L2CAP channel 42 00
< L2CAP 0001 Opcode:id = 06:08   Expects reply 07:08
      0000  02 0C 00 0C 00 08 00 01 - 00 06 08 04 00 40 00 42 
      0010  00 
> L2CAP 0001 Opcode:id = 07:08  is reply from 06:08
      0000  02 0C 20 0C 00 08 00 01 - 00 07 08 04 00 40 00 42 
      0010  00 
GOT L2CAP disconnect request - id 05

SEND L2CAP disconnect reply with received id 05
  Set [1][2] handle 0C 00
  Set [10] id 05
  Set [13][14] local L2CAP channel 42 00
  Set [15][16] remote L2CAP channel 40 00
< L2CAP 0001 Opcode:id = 07:05   is reply to 06:05
      0000  02 0C 00 0C 00 08 00 01 - 00 07 05 04 00 42 00 40 
      0010  00 
L2CAP channel closed      
      
SEND Close connection
  Set [4][5] handle 0C 00
< HCI OGF=01 OCF=06
      0000  01 06 04 03 0C 00 13 
> Event 0F = 00 01 06 04
      0000  04 0F 04 00 01 06 04 
> Event 13 = 01 0C 00 02 00
      0000  04 13 05 01 0C 00 02 00 
> Event 05 = 00 0C 00 16
      0000  04 05 04 00 0C 00 16 
GOT Disconnected OK (Event 05)

```

## 5.3.3 Classic connect with PIN code

This is the procedure to connect to an HC-05 module on its single
RFCOMM channel 1 and a PIN code of 1234.

```
devices.txt info for init_blue():

  DEVICE = HC-05 TYPE=CLASSIC NODE=6 PIN=1234 CHANNEL=1 ADDRESS=98:D3:32:31:59:84

connect via:

  connect_node(6,CHANNEL_STORED,0);

Connecting to HC-05 on channel 1
Set event mask
< HCI OGF=03 OCF=01
      0000  01 01 0C 08 FF FF FB FF - 07 F8 BF 3D 
> Event 0E = 01 01 0C 00
      0000  04 0E 04 01 01 0C 00 
STATUS OK

Set simple pair mode on
< HCI OGF=03 OCF=56
      0000  01 56 0C 01 01 
> Event 0E = 01 56 0C 00
      0000  04 0E 04 01 56 0C 00 
STATUS OK

Open classic connection to 98:D3:32:31:59:84
  Set [4].. board address reversed 84..98
< HCI OGF=01 OCF=05
      0000  01 05 04 0D 84 59 31 32 - D3 98 18 CC 02 00 00 00 
      0010  01 
> Event 0F = 00 01 05 04
      0000  04 0F 04 00 01 05 04 
GOT Open OK (Event 03) with handle = 000C
> Event 03 = 00 0C 00 84 59 31 32 D3 98 01...
      0000  04 03 0B 00 0C 00 84 59 - 31 32 D3 98 01 00 
> Event 20 = 84 59 31 32 D3 98 01
      0000  04 20 07 84 59 31 32 D3 - 98 01 
> Event 1B = 0C 00 05
      0000  04 1B 03 0C 00 05 
      
SEND Authentication request
  Set [4][5] handle 0C 00
< HCI OGF=01 OCF=11
      0000  01 11 04 02 0C 00 
> Event 0F = 00 01 11 04
      0000  04 0F 04 00 01 11 04 
> Event 17 = 84 59 31 32 D3 98
      0000  04 17 06 84 59 31 32 D3 - 98 
GOT link request (Event 17)

SEND link request neg reply
  Set [4].. board address reversed 84..98
< HCI OGF=01 OCF=0C
      0000  01 0C 04 06 84 59 31 32 - D3 98 
> Event 0E = 01 0C 04 00 84 59 31 32 D3 98
      0000  04 0E 0A 01 0C 04 00 84 - 59 31 32 D3 98 
> Event 16 = 84 59 31 32 D3 98
      0000  04 16 06 84 59 31 32 D3 - 98 
GOT PIN request (Event 16)

Using PIN=1234 from device info
SEND PIN code
  Set [11] PIN = 1234 ascii
  Set [4].. board address reversed 84..98
< HCI OGF=01 OCF=0D
      0000  01 0D 04 17 84 59 31 32 - D3 98 04 31 32 33 34 00 
      0010  00 00 00 00 00 00 00 00 - 00 00 00 
> Event 0E = 01 0D 04 00 84 59 31 32 D3 98
      0000  04 0E 0A 01 0D 04 00 84 - 59 31 32 D3 98 
> Event 18 = 84 59 31 32 D3 98 F7 E7 28 68...
      0000  04 18 17 84 59 31 32 D3 - 98 F7 E7 28 68 1B 3A BE 
      0010  03 B3 D9 8B 6B 2F FC 4D - 1A 00 
> Event 06 = 00 0C 00
      0000  04 06 03 00 0C 00 
GOT key (Event 18)
GOT Authentication/pair OK (Event 06)

SEND encrypt
  Set [4][5] handle 0C 00
< HCI OGF=01 OCF=13
      0000  01 13 04 03 0C 00 01 
> Event 0F = 00 01 13 04
      0000  04 0F 04 00 01 13 04 
> Event 08 = 00 0C 00 01
      0000  04 08 04 00 0C 00 01 
GOT Encrypt OK (Event 08)

***** Opened OK *****  

Continues from here as in the previous
Classic procedures section...

```

## 5.3.4 Classic disconnect initiated by server

```
Server initiates the disconnection by sending an opcode 53 packet
to close the RFCOMM channel.

> RFCOMM Address:Opcode 11:53   close request. Must send 11:73 UA reply
      0000  02 0C 20 08 00 04 00 42 - 00 11 53 01 16 
Remote device Pi4 has initiated disconnection
GOT Disconnect RFCOMM channel request (opcode 53)
SEND UA reply (opcode 73)
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
  Set [9] CONTROL address 11
  Set [12] FCS=3C calculated for 3 bytes from [9]
< RFCOMM Address:Opcode = 11:73
      0000  02 0C 00 08 00 04 00 40 - 00 11 73 01 3C 
> Event 13 = 01 0C 00 02 00
      0000  04 13 05 01 0C 00 02 00 
> CONTROL Address:Opcode 01:53   close request. Must send 01:73 UA reply
      0000  02 0C 20 08 00 04 00 42 - 00 01 53 01 9C 
GOT Disconnect CONTROL channel 0 request (opcode 53)
SEND UA reply (opcode 73)
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
  Set [12] FCS=B6 calculated for 3 bytes from [9]
< CONTROL Address:Opcode = 01:73
      0000  02 0C 00 08 00 04 00 40 - 00 01 73 01 B6 
> L2CAP 0001 Opcode:id = 06:05  Must send reply 07:05
      0000  02 0C 20 0C 00 08 00 01 - 00 06 05 04 00 42 00 40 
      0010  00 
GOT L2CAP disconnect request - 06:05
SEND L2CAP disconnect request - 06:09
  Set [1][2] handle 0C 00
  Set [10] id 09
  Set [13][14] remote L2CAP channel 40 00
  Set [15][16] local L2CAP channel 42 00
< L2CAP 0001 Opcode:id = 06:09   Expects reply 07:09
      0000  02 0C 00 0C 00 08 00 01 - 00 06 09 04 00 40 00 42 
      0010  00 
> Event 13 = 01 0C 00 02 00
      0000  04 13 05 01 0C 00 02 00 
> L2CAP 0001 Opcode:id = 07:09  is reply from 06:09
      0000  02 0C 20 0C 00 08 00 01 - 00 07 09 04 00 40 00 42 
      0010  00 
SEND L2CAP disconnect reply with received id 05
  Set [1][2] handle 0C 00
  Set [10] id 05
  Set [13][14] local L2CAP channel 42 00
  Set [15][16] remote L2CAP channel 40 00
< L2CAP 0001 Opcode:id = 07:05   is reply to 06:05
      0000  02 0C 00 0C 00 08 00 01 - 00 07 05 04 00 42 00 40 
      0010  00 
GOT L2CAP disconnect reply

SEND Close connection
  Set [4][5] handle 0C 00
< HCI OGF=01 OCF=06
      0000  01 06 04 03 0C 00 13 
> Event 0F = 00 01 06 04
      0000  04 0F 04 00 01 06 04 
> Event 13 = 01 0C 00 01 00
      0000  04 13 05 01 0C 00 01 00 
> Event 05 = 00 0C 00 16
      0000  04 05 04 00 0C 00 16 
GOT Disconnected OK (Event 05)
Device Pi4 disconnected


```

## 5.3.5 Classic scan

```
Scanning for Classic devices - 10 seconds

SEND classic scan command
  Set [7] = 08 count of 1.28s timeout intervals
  so timeout = 1.28 x 8 = 10.24s 
< HCI OGF=01 OCF=01
      0000  01 01 04 05 33 8B 9E 08 - 00 
> Event 0F = 00 01 01 04
      0000  04 0F 04 00 01 01 04
      
An 02 Event will be returned for each device found, with its
board address.
       
> Event 02 = 01 84 59 31 32 D3 98 01 00 00...
      0000  04 02 0F 01 84 59 31 32 - D3 98 01 00 00 00 1F 00 
      0010  31 73 
> Event 02 = 01 56 DB 04 32 A6 DC 01 00 00...
      0000  04 02 0F 01 56 DB 04 32 - A6 DC 01 00 00 00 00 00 
      0010  32 6A 
> Event 01 = 00
      0000  04 01 01 00 
1 FOUND 98 D3 32 31 59 84 
   Known device 6 = HC-05
2 FOUND DC A6 32 04 DB 56 
   Known device 4 = Pi4 classic
Found 2 devices

```

## 5.3.6 SDP database operations

The following procedure shows how to read a remote device's SDP database which
can then be searched for RFCOMM serial channels. The btlib procedure is more 
efficient because it reads UUID 0003, which returns fewer results.
This example shows how extra bytes and continue bytes are handled when there
is too much returned data to fit in one packet.
The details of decoding the SDP data will not be described here because the documentation
at Vol 3,Part B,Section 3 is clear enough.
This is followed by a guide to reading the local device's database via the standard
bluez mechanism, and registering a new serial service.

```
Read SDP database of Windows PC

Open procedure as in Classic procedures section
will not be repeated here. Assume we have got
to the Opened OK point in that procedure. 

**** Opened OK ****

Open an L2CAP psm 1 channel

SEND connect L2CAP channel psm 1
  Choose local channel 0044 (Must be >= 0x0040). Choose id 3
  Set [13] psm 01
  Set [1][2] handle 0C 00
  Set [10] id 03
  Set [15][16] local L2CAP channel 44 00
< L2CAP 0001 Opcode:id = 02:03   Expects reply 03:03
      0000  02 0C 00 0C 00 08 00 01 - 00 02 03 04 00 01 00 44 
      0010  00 
> L2CAP 0001 Opcode:id = 03:03  is reply from 02:03
      0000  02 0C 20 10 00 0C 00 01 - 00 03 03 08 00 40 00 44 
      0010  00 01 00 00 00 
> L2CAP 0001 Opcode:id = 03:03  is reply from 02:03
      0000  02 0C 20 10 00 0C 00 01 - 00 03 03 08 00 40 00 44 
      0010  00 00 00 00 00 
> L2CAP 0001 Opcode:id = 04:04  Must send reply 05:04
      0000  02 0C 20 10 00 0C 00 01 - 00 04 04 08 00 44 00 00 
      0010  00 01 02 00 04 
GOT result=1 pending reply. Wait for result=0 reply
GOT connect OK reply with remote channel
  Remote channel for following sends = 0040

SEND L2 config request. Choose id 04
  Set [1][2] handle 0C 00
  Set [10] id 04
  Set [13][14] remote L2CAP channel 40 00
< L2CAP 0001 Opcode:id = 04:04   Expects reply 05:04
      0000  02 0C 00 1B 00 17 00 01 - 00 04 04 13 00 40 00 00 
      0010  00 01 02 F5 03 04 09 00 - 00 00 00 00 00 00 00 00 
> Event 13 = 01 0C 00 02 00
      0000  04 13 05 01 0C 00 02 00 
> L2CAP 0001 Opcode:id = 05:04  is reply from 04:04
      0000  02 0C 20 19 00 15 00 01 - 00 05 04 11 00 44 00 00 
      0010  00 00 00 04 09 00 00 00 - 00 00 00 00 00 00 
GOT L2 config reply
GOT L2 request config - id 04
SEND L2 config reply with received id
  Set [1][2] handle 0C 00
  Set [10] id 04
  Set [13][14] remote L2CAP channel 40 00
< L2CAP 0001 Opcode:id = 05:04   is reply to 04:04
      0000  02 0C 00 12 00 0E 00 01 - 00 05 04 0A 00 40 00 00 
      0010  00 00 00 01 02 F5 03 
Connect L2CAP psm 1 now done OK

Read the SDP database

Because there is too much data to fit in one packet, it comes
back in three packets that illustrate two mechanisms: extra
bytes and continue bytes.

SEND SSA request for UUID = 0100, all aid
  Set [17][18] UUID 0100
  Set [26][27] last aid FF FF
  Set [3] [5] packet lengths
  Set [11] id 01
  Set [13] length 0F
  Set [28] continue bytes count = 0
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
< RFCOMM Address:Opcode = 06:00
      0000  02 0C 00 18 00 14 00 40 - 00 06 00 01 00 0F 35 03 
      0010  19 01 00 FF FF 35 05 0A - 00 00 FF FF 00 
> Event 13 = 01 0C 00 02 00
      0000  04 13 05 01 0C 00 02 00 
> CONTROL Address:Opcode 07:00
      0000  02 0C 20 A7 02 F5 03 44 - 00 07 00 01 03 F0 03 E5 
      0010  36 04 AF 35 98 09 00 00 - 0A 00 00 00 00 09 00 01 
      0020  35 03 19 10 00 09 00 04 - 35 0D 35 06 19 01 00 09 
      0030  00 01 35 03 19 00 01 09 - 00 05 35 03 19 10 02 09 
      0040  00 06 35 09 09 65 6E 09 - 00 6A 09 01 00 09 01 00 
      0050  25 12 53 65 72 76 69 63 - 65 20 44 69 73 63 6F 76 
      0060  65 72 79 00 09 01 01 25 - 25 50 75 62 6C 69 73 68 
      0070  65 73 20 73 65 72 76 69 - 63 65 73 20 74 6F 20 72 
      0080  65 6D 6F 74 65 20 64 65 - 76 69 63 65 73 00 09 01 
      0090  02 25 0A 4D 69 63 72 6F - 73 6F 66 74 00 09 02 00 
      00A0  35 03 09 01 00 09 02 01 - 0A 00 00 00 0A 35 95 09 
      00B0  00 00 0A 00 01 00 00 09 - 00 01 35 03 19 12 00 09 
      00C0  00 04 35 0D 35 06 19 01 - 00 09 00 01 35 03 19 00 
      00D0  01 09 00 05 35 03 19 10 - 02 09 00 06 35 09 09 65 
      00E0  6E 09 00 6A 09 01 00 09 - 01 00 25 18 44 65 76 69 
      00F0  63 65 20 49 44 20 53 65 - 72 76 69 63 65 20 52 65 
      0100  63 6F 72 64 09 01 01 25 - 18 44 65 76 69 63 65 20 
      0110  49 44 20 53 65 72 76 69 - 63 65 20 52 65 63 6F 72 
      0120  64 09 02 00 09 01 03 09 - 02 01 09 00 06 09 02 02 
      0130  09 00 01 09 02 03 09 0A - 00 09 02 04 28 01 09 02 
      0140  05 09 00 01 35 9D 09 00 - 00 0A 00 01 00 01 09 00 
      0150  01 35 03 19 11 15 09 00 - 04 35 1B 35 06 19 01 00 
      0160  09 00 0F 35 11 19 00 0F - 09 01 00 35 09 09 08 00 
      0170  09 86 DD 09 08 06 09 00 - 05 35 03 19 10 02 09 00 
      0180  06 35 09 09 65 6E 09 00 - 6A 09 01 00 09 00 09 35 
      0190  08 35 06 19 11 15 09 01 - 00 09 01 00 25 1D 50 65 
      01A0  72 73 6F 6E 61 6C 20 41 - 64 20 48 6F 63 20 55 73 
      01B0  65 72 20 53 65 72 76 69 - 63 65 00 09 01 01 25 1D 
      01C0  50 65 72 73 6F 6E 61 6C - 20 41 64 20 48 6F 63 20 
      01D0  55 73 65 72 20 53 65 72 - 76 69 63 65 00 09 03 0A 
      01E0  09 00 00 35 5F 09 00 00 - 0A 00 01 00 02 09 00 01 
      01F0  35 03 19 11 0A 09 00 04 - 35 10 35 06 19 01 00 09 
      0200  00 19 35 06 19 00 19 09 - 01 03 09 00 05 35 03 19 
      0210  10 02 09 00 06 35 09 09 - 65 6E 09 00 6A 09 01 00 
      0220  09 00 09 35 08 35 06 19 - 11 0D 09 01 03 09 01 00 
      0230  25 0C 41 75 64 69 6F 20 - 53 6F 75 72 63 65 09 03 
      0240  11 09 00 01 35 8C 09 00 - 00 0A 00 01 00 03 09 00 
      0250  01 35 03 19 11 0C 09 00 - 04 35 10 35 06 19 01 00 
      0260  09 00 17 35 06 19 00 17 - 09 01 04 09 00 05 35 03 
      0270  19 10 02 09 00 06 35 09 - 09 65 6E 09 00 6A 09 01 
      0280  00 09 00 09 35 08 35 06 - 19 11 0E 09 01 06 09 00 
      0290  0D 35 12 35 10 35 06 19 - 01 00 09 00 1B 35 06 19 
      02A0  00 17 09 01 04 09 01 00 - 25 22 41 75 
      
  First packet   
  Byte counts:
    [3][4] = 02A7 from [5] to end of this packet
    [5][6] = 03F5 from [9] to end of continue bytes (inc next packet)
    [12][13] = 03F0 from [14] to end of continue bytes (inc next packet)
    [14][15] = 03E5 from [16] to end of SDP data (inc next packet)
  Data:
        [16] = 36 (first SDP data byte)
    [0x02AB] = 75 (last SDP data byte in this packet)
    
  This calculation using the above byte counts reveals if there
  are missing bytes that will be sent in an Extra bytes packet:  
         
    Missing number = 03F5 - 02A7 + 4 = 0152 (338 decimal)        
      
Need an extra 0152 bytes - here they come:

> Extra bytes - add to previous data
      0000  02 0C 10 52 01 64 69 6F - 20 56 69 64 65 6F 20 52 
      0010  65 6D 6F 74 65 20 43 6F - 6E 74 72 6F 6C 20 50 72 
      0020  6F 66 69 6C 65 09 03 11 - 09 00 11 35 5A 09 00 00 
      0030  0A 00 01 00 04 09 00 01 - 35 06 19 11 0E 19 11 0F 
      0040  09 00 04 35 10 35 06 19 - 01 00 09 00 17 35 06 19 
      0050  00 17 09 01 04 09 00 05 - 35 03 19 10 02 09 00 09 
      0060  35 08 35 06 19 11 0E 09 - 01 06 09 00 0D 35 12 35 
      0070  10 35 06 19 01 00 09 00 - 1B 35 06 19 00 17 09 01 
      0080  04 09 03 11 09 00 02 35 - 5D 09 00 00 0A 00 01 00 
      0090  05 09 00 01 35 03 19 11 - 0B 09 00 04 35 10 35 06 
      00A0  19 01 00 09 00 19 35 06 - 19 00 19 09 01 03 09 00 
      00B0  05 35 03 19 10 02 09 00 - 06 35 09 09 65 6E 09 00 
      00C0  6A 09 01 00 09 00 09 35 - 08 35 06 19 11 0D 09 01 
      00D0  03 09 01 00 25 0A 41 75 - 64 69 6F 20 53 69 6E 6B 
      00E0  09 03 11 09 00 02 35 44 - 09 00 00 0A 00 01 00 06 
      00F0  09 00 01 35 06 19 11 1F - 19 12 03 09 00 04 35 0C 
      0100  35 03 19 01 00 35 05 19 - 00 03 08 01 09 00 05 35 
      0110  03 19 10 02 09 00 09 35 - 08 35 06 19 11 1E 09 01 
      0120  07 09 03 01 08 01 09 03 - 11 09 00 27 35 3F 09 00 
      0130  00 0A 00 01 00 07 09 00 - 01 35 06 19 11 1E 19 12 
      0140  03 09 00 04 35 0C 35 03 - 19 01 00 35 05 19 08 20 
      0150  7A AC AB 0F E7 FF FF 
           
   The offset of the last SDP data byte is calculated from the
   previous packet byte counts as follows:
      
   03E5 - 02A7 + 000F = 014D   
   
   The offset of the continue byte count is one more = 014E
      
   Second packet
   Byte count
     [3][4] = 0152 expected number of extra bytes missing from previous packet
   Data:
          [5] = 64 (first extra SDP data byte to be added to previous data)
     [0x014D] = 19 (last SDP data byte in this packet)
     [0x014E] = 08 (continue byte count) 20 7A AC AB 0F E7 FF FF (continue bytes) 
           
GOT SSA reply

Last packet has ended with 8 continue bytes
SEND another SSA request with these bytes:
  Set [28]..  count and continue bytes..
    08 20 7A AC AB 0F E7 FF FF
  Set [3] [5] packet lengths
  Set [11] id 02
  Set [13] length 17
  Set [1][2] handle 0C 00
  Set [7][8] remote L2CAP channel 40 00
< RFCOMM Address:Opcode = 06:00
      0000  02 0C 00 20 00 1C 00 40 - 00 06 00 02 00 17 35 03 
      0010  19 01 00 FF FF 35 05 0A - 00 00 FF FF 08 20 7A AC 
      0020  AB 0F E7 FF FF 
> CONTROL Address:Opcode 07:00
      0000  02 0C 20 D9 00 D5 00 44 - 00 07 00 02 00 D0 00 CD 
      0010  00 03 08 02 09 00 05 35 - 03 19 10 02 09 00 09 35 
      0020  08 35 06 19 11 1E 09 01 - 07 09 03 11 09 00 37 35 
      0030  52 09 00 00 0A 00 01 00 - 08 09 00 01 35 11 1C C7 
      0040  F9 47 13 89 1E 49 6A A0 - E7 98 3A 09 46 12 6E 09 
      0050  00 04 35 0C 35 03 19 01 - 00 35 05 19 00 03 08 03 
      0060  09 00 05 35 03 19 10 02 - 09 01 00 25 16 43 44 50 
      0070  20 50 72 6F 78 69 6D 61 - 6C 20 54 72 61 6E 73 70 
      0080  6F 72 74 35 58 09 00 00 - 0A 00 01 00 09 09 00 01 
      0090  35 11 1C FC F0 5A FD 67 - D8 4F 41 83 F5 7B EE 22 
      00A0  C0 3C DB 09 00 04 35 0C - 35 03 19 01 00 35 05 19 
      00B0  00 03 08 04 09 00 05 35 - 03 19 10 02 09 01 00 25 
      00C0  10 4D 79 20 63 75 73 74 - 6F 6D 20 73 65 72 69 61 
      00D0  6C 09 01 01 25 07 43 6F - 6D 6D 65 6E 74 00 
GOT SSA reply

   Third packet
   Byte counts:
       [3][4] = 00D9 from [5] to end of this packet
       [5][6] = 00D5 from [9] to end of continue bytes
     [12][13] = 00D0 from [14] to end of continue bytes
     [14][15] = 00CD from [16] to end of SDP data
   Data:
         [16] = 00 (first SDP data byte to be added to previous data)
     [0x00DC] = 74 (last SDP data byte)
     [0x00DD] = 00 (number of continue bytes) 

No extra bytes
No continue bytes - done

The SDP data from the three packets can now be assembled.
Close the L2CAP channel and then decode the data.

SEND disconnect L2CAP channel. Choose id 08
  Set [1][2] handle 0C 00
  Set [10] id 08
  Set [13][14] remote L2CAP channel 40 00
  Set [15][16] local L2CAP channel 44 00
< L2CAP 0001 Opcode:id = 06:08   Expects reply 07:08
      0000  02 0C 00 0C 00 08 00 01 - 00 06 08 04 00 40 00 44 
      0010  00 
> Event 13 = 01 0C 00 02 00
      0000  04 13 05 01 0C 00 02 00 
> L2CAP 0001 Opcode:id = 07:08  is reply from 06:08
      0000  02 0C 20 0C 00 08 00 01 - 00 07 08 04 00 40 00 44 
      0010  00 

Disconnect from Windows PC - will not be repeated here

Decode SDP data 1202 bytes start 36 04 AF end 65 6E 74

 **** RECORD 0 ****
    aid = 0000
        Handle 00 00 00 00 
    aid = 0001
      UUID 10 00 SrvrDiscServID
    aid = 0004
        UUID 01 00 L2CAP
        00 01 
        UUID 00 01 SDP
    aid = 0005
      UUID 10 02 
    aid = 0006
      65 6E 
      00 6A 
      01 00 
    aid = 0100
        Service Discovery 
    aid = 0101
        Publishes services to remote devices 
    aid = 0102
        Microsoft 
    aid = 0200
      01 00 
    aid = 0201
        00 00 00 0A 
 **** RECORD 1 ****
    aid = 0000
        Handle 00 01 00 00 
    aid = 0001
      UUID 12 00 PnPInformation
    aid = 0004
        UUID 01 00 L2CAP
        00 01 
        UUID 00 01 SDP
    aid = 0005
      UUID 10 02 
    aid = 0006
      65 6E 
      00 6A 
      01 00 
    aid = 0100
        Device ID Service Record
    aid = 0101
        Device ID Service Record
    aid = 0200
        01 03 
    aid = 0201
        00 06 
    aid = 0202
        00 01 
    aid = 0203
        0A 00 
    aid = 0204
        01 
    aid = 0205
        00 01 
 **** RECORD 2 ****
    aid = 0000
        Handle 00 01 00 01 
    aid = 0001
      UUID 11 15 PANU
    aid = 0004
        UUID 01 00 L2CAP
        00 0F 
        UUID 00 0F BNEP
        01 00 
          08 00 
          86 DD 
          08 06 
    aid = 0005
      UUID 10 02 
    aid = 0006
      65 6E 
      00 6A 
      01 00 
    aid = 0009
        UUID 11 15 PANU
        01 00 
    aid = 0100
        Personal Ad Hoc User Service 
    aid = 0101
        Personal Ad Hoc User Service 
    aid = 030A
        00 00 
 **** RECORD 3 ****
    aid = 0000
        Handle 00 01 00 02 
    aid = 0001
      UUID 11 0A AudioSource
    aid = 0004
        UUID 01 00 L2CAP
        00 19 
        UUID 00 19 AVDTP
        01 03 
    aid = 0005
      UUID 10 02 
    aid = 0006
      65 6E 
      00 6A 
      01 00 
    aid = 0009
        UUID 11 0D AdvancedAudioDist
        01 03 
    aid = 0100
        Audio Source
    aid = 0311
        00 01 
 **** RECORD 4 ****
    aid = 0000
        Handle 00 01 00 03 
    aid = 0001
      UUID 11 0C A/V_RemoteCtrlTarget
    aid = 0004
        UUID 01 00 L2CAP
        00 17 
        UUID 00 17 AVCTP
        01 04 
    aid = 0005
      UUID 10 02 
    aid = 0006
      65 6E 
      00 6A 
      01 00 
    aid = 0009
        UUID 11 0E A/V_RemoteControl
        01 06 
    aid = 000D
          UUID 01 00 L2CAP
          00 1B 
          UUID 00 17 AVCTP
          01 04 
    aid = 0100
        Audio Video Remote Control Profile
    aid = 0311
        00 11 
 **** RECORD 5 ****
    aid = 0000
        Handle 00 01 00 04 
    aid = 0001
      UUID 11 0E A/V_RemoteControl
      UUID 11 0F A/V_RemoteCtrlController
    aid = 0004
        UUID 01 00 L2CAP
        00 17 
        UUID 00 17 AVCTP
        01 04 
    aid = 0005
      UUID 10 02 
    aid = 0009
        UUID 11 0E A/V_RemoteControl
        01 06 
    aid = 000D
          UUID 01 00 L2CAP
          00 1B 
          UUID 00 17 AVCTP
          01 04 
    aid = 0311
        00 02 
 **** RECORD 6 ****
    aid = 0000
        Handle 00 01 00 05 
    aid = 0001
      UUID 11 0B AudioSink
    aid = 0004
        UUID 01 00 L2CAP
        00 19 
        UUID 00 19 AVDTP
        01 03 
    aid = 0005
      UUID 10 02 
    aid = 0006
      65 6E 
      00 6A 
      01 00 
    aid = 0009
        UUID 11 0D AdvancedAudioDist
        01 03 
    aid = 0100
        Audio Sink
    aid = 0311
        00 02 
 **** RECORD 7 ****
    aid = 0000
        Handle 00 01 00 06 
    aid = 0001
      UUID 11 1F HandsfreeAudioGateway
      UUID 12 03 GenericAudio
    aid = 0004
        UUID 01 00 L2CAP
        UUID 00 03 RFCOMM
        RFCOMM channel = 1
    aid = 0005
      UUID 10 02 
    aid = 0009
        UUID 11 1E Handsfree
        01 07 
    aid = 0301
        01 
    aid = 0311
        00 27 
 **** RECORD 8 ****
    aid = 0000
        Handle 00 01 00 07 
    aid = 0001
      UUID 11 1E Handsfree
      UUID 12 03 GenericAudio
    aid = 0004
        UUID 01 00 L2CAP
        UUID 00 03 RFCOMM
        RFCOMM channel = 2
    aid = 0005
      UUID 10 02 
    aid = 0009
        UUID 11 1E Handsfree
        01 07 
    aid = 0311
        00 37 
 **** RECORD 9 ****
    aid = 0000
        Handle 00 01 00 08 
    aid = 0001
      UUID C7 F9 47 13 89 1E 49 6A A0 E7 98 3A 09 46 12 6E 
    aid = 0004
        UUID 01 00 L2CAP
        UUID 00 03 RFCOMM
        RFCOMM channel = 3
    aid = 0005
      UUID 10 02 
    aid = 0100
        CDP Proximal Transport
 **** RECORD 10 ****
    aid = 0000
        Handle 00 01 00 09 
    aid = 0001
      UUID FC F0 5A FD 67 D8 4F 41 83 F5 7B EE 22 C0 3C DB 
    aid = 0004
        UUID 01 00 L2CAP
        UUID 00 03 RFCOMM
        RFCOMM channel = 4
    aid = 0005
      UUID 10 02 
    aid = 0100
        My custom serial
    aid = 0101
        Comment

```

Reading the local SDP database is similar, but bluez is required. Because it uses
bluez, this code cannot co-exist with btferret/btlib. A local
file socket must be opened as follows:

```
To enable access to bluez the following will be necessary:

MODIFY service file:

sudo nano /lib/systemd/system/bluetooth.service
Add -C to the ExecStart line so it reads:
ExecStart=/usr.....bluetoothd -C
reboot or restart the bluetooth service to implement the change

If access to /var/run/sdp is denied, change its permissions or
run the code with root priviledges from root or with sudo.


int sock;
struct sockaddr_un sax;
     
sock = socket(PF_UNIX,SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK,0);   
sax.sun_family = AF_UNIX;
   // SDP_UNIX_PATH is probably  /var/run/sdp
strcpy(sax.sun_path,"/var/run/sdp");      
connect(sock,(struct sockaddr *)&sax,sizeof(sax));    
  // send packets via:  
write(sock,packet,packetsize);
  // read packets via:
read(sock,buf,sizeof(buf);
 
*** NOTE packets do not have the 9-byte HCI header *** 

So an SSA request packet starts with the opcode 06 and the SSA reply
starts with the opcode 07

To disconnect:

close(sock);

```

Here is code to register two new serial services in the local SDP
database, and then read the database to see the result. Note that
the 9-byte HCI headers are absent. 

```

Open local SDP database as above

We have previously determined that channels 1 and 2 are unused.

Register 2-byte UUID=1101 Serial Port as channel 1
  Set [4]  41 + name length 0B
  Set [7]  38 + name length 0B
  Set [14] UUID 1101
  Set [32] channel 01
  Set [45] name length 0B
  Set [46].. name "Serial Port"
< SDP 75
      0000  75 00 01 00 34 01 35 31 - 09 00 01 35 03 19 11 01 
      0010  09 00 04 35 0C 35 03 19 - 01 00 35 05 19 00 03 08 
      0020  01 09 00 05 35 03 19 10 - 02 09 01 00 25 0B 53 65 
      0030  72 69 61 6C 20 50 6F 72 - 74 
> SDP 76
      0000  76 00 01 00 04 00 01 00 - 01
      
    // error reply   01 00 01 00 02 00 03 = error 0003  v3,pB,4.4.1
    // OK reply      76 00 01 00 04 00 01 00 01 = SDP handle 00010001                 
   
Done OK

Register 16-byte UUID My Custom serial as channel 2
  Set [4]  55 + name length 10
  Set [7]  52 + name length 10
  Set [14]..[29] UUID = FC F0 5A FD 67 D8 4F 41 83 F5 7B EE 22 C0 3C DB 
  Set [46] channel 02
  Set [59] name length 10
  Set [60].. name "My Custom serial"
< SDP 75
      0000  75 00 01 00 47 01 35 44 - 09 00 01 35 11 1C FC F0 
      0010  5A FD 67 D8 4F 41 83 F5 - 7B EE 22 C0 3C DB 09 00 
      0020  04 35 0C 35 03 19 01 00 - 35 05 19 00 03 08 02 09 
      0030  00 05 35 03 19 10 02 09 - 01 00 25 10 4D 79 20 43 
      0040  75 73 74 6F 6D 20 73 65 - 72 69 61 6C 
> SDP 76
      0000  76 00 01 00 04 00 01 00 - 02 
         
Done OK

Read SDP database of Local
Find all UUID = 0003
  Set [8][9] UUID
  Set [17][18] last aid FF FF
  Set [2] id 01
  Set [4] length 0F
  Set [19] continue bytes count = 0
SEND SSA request all aid
< SDP 06
      0000  06 00 01 00 0F 35 03 19 - 00 03 FF FF 35 05 0A 00 
      0010  00 FF FF 00 
> SDP 07
      0000  07 00 01 00 8E 00 8B 35 - 89 35 39 09 00 00 0A 00 
      0010  01 00 01 09 00 01 35 03 - 19 11 01 09 00 04 35 0C 
      0020  35 03 19 01 00 35 05 19 - 00 03 08 01 09 00 05 35 
      0030  03 19 10 02 09 01 00 25 - 0B 53 65 72 69 61 6C 20 
      0040  50 6F 72 74 35 4C 09 00 - 00 0A 00 01 00 02 09 00 
      0050  01 35 11 1C FC F0 5A FD - 67 D8 4F 41 83 F5 7B EE 
      0060  22 C0 3C DB 09 00 04 35 - 0C 35 03 19 01 00 35 05 
      0070  19 00 03 08 02 09 00 05 - 35 03 19 10 02 09 01 00 
      0080  25 10 4D 79 20 43 75 73 - 74 6F 6D 20 73 65 72 69 
      0090  61 6C 00 
GOT SSA reply
No continue bytes - done

Disconnect Local SDP as above

Decode SDP info 139 bytes start 35 89 35 end 69 61 6C

 **** RECORD 0 ****
    aid = 0000
        Handle 00 01 00 01 
    aid = 0001
      UUID 11 01 SerialPort
    aid = 0004
        UUID 01 00 L2CAP
        UUID 00 03 RFCOMM
        RFCOMM channel = 1
    aid = 0005
      UUID 10 02 
    aid = 0100
        Serial Port
 **** RECORD 1 ****
    aid = 0000
        Handle 00 01 00 02 
    aid = 0001
      UUID FC F0 5A FD 67 D8 4F 41 83 F5 7B EE 22 C0 3C DB 
    aid = 0004
        UUID 01 00 L2CAP
        UUID 00 03 RFCOMM
        RFCOMM channel = 2
    aid = 0005
      UUID 10 02 
    aid = 0100
        My Custom serial

``` 

## 5.3.7 LE Procedures

This connects to an LE device, writes a 1-byte characteristic with
no acknowledge, writes a 2-byte characteristic with acknowledge, reads
the 2-byte characteristic back, and disconnects. The commands use the 
[L2CAP 0004](#5.2.4-starting-02-channel-0004)  LE packet format. The connect opcode
is documented at v2,pE,7.8.12, the Event 3E reply at 7.7.65.1. The read 
opcodes are documented at v3,pF,3.4.4.3 and 3.4.4.4. The write
opcodes at v3,pF,3.4.5. A characteristic with no
acknowledge permission must be sent a write command 3.4.5.3, and one with
acknowledge permission a write request 3.4.5.1. The close opcode at v2,pE,7.1.6.  

```

SEND LE connect to 00:1E:C0:2D:17:7C
  Set [10].. board address reversed 7C..00
< HCI OGF=08 OCF=0D
      0000  01 0D 20 19 60 00 60 00 - 00 00 7C 17 2D C0 1E 00 
      0010  00 18 00 28 00 00 00 11 - 01 00 00 00 00 
> Event 0F = 00 01 0D 20
      0000  04 0F 04 00 01 0D 20 
> Event 3E = 01 00 40 00 00 00 7C 17 2D C0...
      0000  04 3E 13 01 00 40 00 00 - 00 7C 17 2D C0 1E 00 27 
      0010  00 00 00 11 01 00 
Connect OK
Handle = 0040

Write (no acknowledge) one byte (12) to characteristic handle 0018
SEND write LE characteristic
  Set [3][4] [5][6] packet lengths
  Set [9] opcode 52
  Set [10][11] characteristic handle 0018
  Set [12].. 01 data bytes
  Set [1][2] handle 40 00
< L2CAP 0004 Opcode = 52
      0000  02 40 00 08 00 04 00 04 - 00 52 18 00 12 

Write (with acknowledge) two bytes (34 56) to characteristic handle 001C
Replies with opcode 13 acknowledge
SEND write LE characteristic
  Set [3][4] [5][6] packet lengths
  Set [9] opcode 12
  Set [10][11] characteristic handle 001C
  Set [12].. 02 data bytes
  Set [1][2] handle 40 00
< L2CAP 0004 Opcode = 12
      0000  02 40 00 09 00 05 00 04 - 00 12 1C 00 34 56 
> Event 13 = 01 40 00 01 00
      0000  04 13 05 01 40 00 01 00 
> L2CAP 0004 Opcode = 13
      0000  02 40 20 05 00 01 00 04 - 00 13 
> Event 13 = 01 40 00 01 00
      0000  04 13 05 01 40 00 01 00 
      
Read characteristic handle 001C - returns two bytes 34 56
  Set [10][11] handle 001C
  Set [1][2] handle 40 00
< L2CAP 0004 Opcode = 0A
      0000  02 40 00 07 00 03 00 04 - 00 0A 1C 00 
> L2CAP 0004 Opcode = 0B
      0000  02 40 20 07 00 03 00 04 - 00 0B 34 56    
      
SEND Close connection
  Set [4][5] handle 40 00
< HCI OGF=01 OCF=06
      0000  01 06 04 03 40 00 13 
> Event 13 = 01 40 00 01 00
      0000  04 13 05 01 40 00 01 00 
> Event 0F = 00 01 06 04
      0000  04 0F 04 00 01 06 04 
> Event 05 = 00 40 00 22
      0000  04 05 04 00 40 00 22 
GOT Disconnected OK (Event 05)

```

## 5.3.8 Read LE services

This reads a list of characteristic UUIDs and handles from an
LE device. The read UUID opcode is documented at v3,pF,3.4.4.1 and 
the reply at v3,pF,3.4.4.2.
There is a list of standard 2-byte UUIDs in a pdf document
called "16-bit UUID Numbers Document"
[here](https://www.bluetooth.com/specifications/assigned-numbers/). 


```

SEND LE connect to 00:1E:C0:2D:17:7C
  Set [10].. board address reversed 7C..00
< HCI OGF=08 OCF=0D
      0000  01 0D 20 19 60 00 60 00 - 00 00 7C 17 2D C0 1E 00 
      0010  00 18 00 28 00 00 00 11 - 01 00 00 00 00 
> Event 0F = 00 01 0D 20
      0000  04 0F 04 00 01 0D 20 
> Event 3E = 01 00 40 00 00 00 7C 17 2D C0...
      0000  04 3E 13 01 00 40 00 00 - 00 7C 17 2D C0 1E 00 27 
      0010  00 00 00 11 01 00 
Connect OK
Has returned handle = 0040

Search for UUID=2803 services. This returns a list of characteristic
handles and UUIDs. Start search at handle 0001 and repeat until
no more are returned. 

Read services from LE device
SEND read UUID (opcode 08)
  Set [10][11] starting characteristic handle 0001
  Set [14][15] UUID 2803
  Set [1][2] handle 40 00
< L2CAP 0004 Opcode = 08
      0000  02 40 00 0B 00 07 00 04 - 00 08 01 00 FF FF 03 28 
> L2CAP 0004 Opcode = 09
      0000  02 40 20 1B 00 17 00 04 - 00 09 07 02 00 0A 03 00 
      0010  00 2A 04 00 02 05 00 01 - 2A 06 00 02 07 00 04 2A 
  
This has returned the following list of 2-byte UUID characteristics
       
Handle Permit  Value   UUID
               Handle
0002    0A     0003    2A00 (standard UUID = Device name)
0004    02     0005    2A01
0006    02     0007    2A04  

To read/write the characteristic - use the Value Handle
      
Keep going from the next handle 0008 following already found 0007      
      
SEND read UUID (opcode 08)     
  Set [10][11] starting characteristic handle 0008
  Set [14][15] UUID 2803
  Set [1][2] handle 40 00
< L2CAP 0004 Opcode = 08
      0000  02 40 00 0B 00 07 00 04 - 00 08 08 00 FF FF 03 28 
> Event 13 = 01 40 00 01 00
      0000  04 13 05 01 40 00 01 00 
> L2CAP 0004 Opcode = 09
      0000  02 40 20 1B 00 17 00 04 - 00 09 07 0A 00 10 0B 00 
      0010  37 2A 0D 00 02 0E 00 38 - 2A 0F 00 08 10 00 39 2A 

This has returned three more 2-byte UUIDs
      
SEND read UUID (opcode 08)     
  Set [10][11] starting characteristic handle 0011
  Set [14][15] UUID 2803
  Set [1][2] handle 40 00
< L2CAP 0004 Opcode = 08
      0000  02 40 00 0B 00 07 00 04 - 00 08 11 00 FF FF 03 28 
> Event 13 = 01 40 00 02 00
      0000  04 13 05 01 40 00 02 00 
> L2CAP 0004 Opcode = 09
      0000  02 40 20 0D 00 09 00 04 - 00 09 07 12 00 04 13 00 
      0010  06 2A 
      
This has returned one more 2-byte UUID
      
SEND read UUID (opcode 08)     
  Set [10][11] starting characteristic handle 0014
  Set [14][15] UUID 2803
  Set [1][2] handle 40 00
< L2CAP 0004 Opcode = 08
      0000  02 40 00 0B 00 07 00 04 - 00 08 14 00 FF FF 03 28 
> L2CAP 0004 Opcode = 09
      0000  02 40 20 1B 00 17 00 04 - 00 09 15 15 00 06 16 00 
      0010  01 FF EE DD CC BB AA 99 - 88 77 66 55 44 33 22 11 
 
This has returned one value handle (0016) with permit 06 (r/w no ack)
and a 16-byte UUID 112233445566778899AABBCCDDEEFF01
This will be a user-defined private characteristic 
      
SEND read UUID (opcode 08)      
  Set [10][11] starting characteristic handle 0017
  Set [14][15] UUID 2803
  Set [1][2] handle 40 00
< L2CAP 0004 Opcode = 08
      0000  02 40 00 0B 00 07 00 04 - 00 08 17 00 FF FF 03 28 
> L2CAP 0004 Opcode = 01
   Error 0A = Attribute Not Found - have reached end of characteristics
      0000  02 40 20 09 00 05 00 04 - 00 01 08 20 00 0A 

SEND Close connection
  Set [4][5] handle 40 00
< HCI OGF=01 OCF=06
      0000  01 06 04 03 40 00 13 
> Event 13 = 01 40 00 01 00
      0000  04 13 05 01 40 00 01 00 
> Event 0F = 00 01 06 04
      0000  04 0F 04 00 01 06 04 
> Event 05 = 00 40 00 16
      0000  04 05 04 00 40 00 16 
GOT Disconnected OK (Event 05)

```

## 5.3.9 LE scan

This searches for active LE devices and reads their advertising
data. Information on decoding the advertising data can be found at
Vol 2,Part E,Section 7.7.65.2 and Vol 3,Part C,Section 11. Also
[Data type codes](https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile/)
and
[Manufacturer codes](https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers/).

```
Enable LE scan, let it run for maybe 10 seconds, then disable.
It will return an Event 3E reply for each LE device found.

SEND Enable LE scan ([4]=01) with repeat filter on ([5]=01)
< HCI OGF=08 OCF=0C
      0000  01 0C 20 02 01 01 
> Event 0E = 01 0C 20 00
      0000  04 0E 04 01 0C 20 00 

The repeat filter should stop multiple replies from the same device, but
it does not work on some Bluetooth adapters, and multiple replies will arrive.

Replies come back

> Event 3E = 02 01 00 00 7C 17 2D C0 1E 00...
      0000  04 3E 29 02 01 00 00 7C - 17 2D C0 1E 00 1D 02 01 
      0010  06 11 07 00 FF EE DD CC - BB AA 99 88 77 66 55 44 
      0020  33 22 11 07 09 52 4E 31 - 37 37 43 A8 
      
> Event 3E = 02 01 00 01 D8 E2 59 6E 94 49...
      0000  04 3E 1E 02 01 00 01 D8 - E2 59 6E 94 49 12 02 01 
      0010  1A 02 0A 0C 0B FF 4C 00 - 10 06 03 1A 79 89 1C BF 
      0020  A0      

SEND Disable LE scan
< HCI OGF=08 OCF=0C
      0000  01 0C 20 02 00 01 
      
Decode the advertising data returned by each device.

1 FOUND 00:1E:C0:2D:17:7C 
    Flags = 06
    All 16-byte UUIDs = 00 FF EE DD CC BB AA 99 88 77 66 55 44 33 22 11
    Name = RN177C
   
2 FOUND 49:94:6E:59:E2:D8 
    Flags = 1A
    Tx Power Level = 0C
    Apple = 4C 00 10 06 03 1A 79 89 1C BF
    No name 

```

## 5.4 Server Code

The following sections are intended as a brief guide to writing server
code for other machines from scratch. The listings are not fully workable
code. They are mostly code fragments, with no error checking, that give an
indication of how to get started.  

## 5.4.1 Raspberry Pi bluez server

Because it uses bluez, this code cannot co-exist with btferret.

```
To enable access to bluez the following will be necessary:

MODIFY service file:

sudo nano /lib/systemd/system/bluetooth.service
Add -C to the ExecStart line so it reads:
ExecStart=/usr.....bluetoothd -C
reboot or restart the bluetooth service to implement the change

If access to /var/run/sdp is denied, change its permissions or
run the code with root priviledges from root or with sudo.

   // libbluetooth-dev must be installed
#include <bluetooth/bluetooth.h>

  struct sockaddr_rc locaddr,remaddr;
  socklen_t opt; 
  int n,serversock,serialfd,flags,channel;
  char *c;
  
     // set up server to listen on an RFCOMM channel
     // which you choose and must be a serial service
     // in the local SDP database
 
  channel = 4;  // RFCOMM channel = 4 for example    
     
  serversock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

  locaddr.rc_family = AF_BLUETOOTH;
  c = (char *)locaddr.rc_bdaddr;
  for(n = 0 ; n < 6 ; ++n)
    c[n] = 0;    // all zero = BDADDR_ANY
  locaddr.rc_channel = channel;
  
  bind(serversock,(struct sockaddr *)&locaddr, sizeof(locaddr));
   
  listen(serversock,50);
  
    // unblock socket
  flags = fcntl(serversock,F_GETFL);
  fcntl(serversock,F_SETFL,flags | O_NONBLOCK);
  
  opt = sizeof(remaddr);
 
  do
    {   // wait for client to connect
    serialfd = accept(serversock,(struct sockaddr *)&remaddr,&opt);
    }
  while(serialfd <= 0 && time out or stop listen test);
  
    // unblock socket
     
  flags = fcntl(serialfd,F_GETFL);
  fcntl(serialfd,F_SETFL,flags | O_NONBLOCK);
  
    // serialfd is now open for serial data exchange with client
  
    // read/write serial data
    
  read(serialfd,data,count);
  write(serialfd,data,count);
  
  
   // disconnect
   
  close(serialfd);
  close(serversock);
  
```

## 5.4.2 Windows COM port

Probably the easiest way to program a Windows server because the
operating system does the work and presents the connection as a
COM port, which must first be set up as follows:

Start/Settings/Devices/Bluetooth ON

Start/Settings/Devices/More Bluetooth Options/COM Ports/Add..

Select Incoming/OK 

It should then report the COM port number e.g. COM3.
The number is needed for the following code.

The server must be listening via the CreateFile function for the
serial channel to be visible to clients. It will be reported
as UUID 1101 "COM 3". The client must read the services to
find the RFCOMM channel (which is not the same as the COM number).

```

  HANDLE hCom;
  COMMTIMEOUTS cto;
  int comport;
  static char serport[8] = {"COMx"};

     // set up server with incoming COM port = comport
  
  comport = 3;    // for example COM3
         
     // replace x in serport[] with COM port number

  serport[3] = (char)(comport + '0');
  
    // CreateFile waits for a client to connect

  hCom = CreateFile( serport,
    GENERIC_READ | GENERIC_WRITE,
    0,    // exclusive-access 
    NULL, // no security attributes 
    OPEN_EXISTING,  
    0,     // 0=not overlapped I/O  or FILE_FLAG_OVERLAPPED
    NULL   // hTemplate 
    );

   // set time outs

  cto.ReadIntervalTimeout = 5; 
  cto.ReadTotalTimeoutMultiplier=5; 
  cto.ReadTotalTimeoutConstant=5; 
  cto.WriteTotalTimeoutMultiplier=5; 
  cto.WriteTotalTimeoutConstant=5; 

  SetCommTimeouts(hCom,&cto);
  
    // hCom is now an open connection to client
    
    // Read/write serial data
    
  ReadFile(hCom,buf,count,&nread,NULL); 
  WriteFile(hCom,buf,count,&nwrit,NULL);  
  
      // disconnect   
  CloseHandle(hCom); 
  
```

## 5.4.3 Windows Sockets

This code sets up a Bluetooth socket directly rather than going via a 
COM port, and registers a custom 16-byte UUID serial channel. The
RFCOMM channel number cannot be specified, and is set by the WSASetService
function, so may be different each time. Consequently the client must read
the services to find the RFCOMM channel number. 

```
#include <ws2bth.h>    
#include <winsock2.h>  

  struct WSAData wsadata;
  WSAQUERYSET wsaq;
  CSADDR_INFO csinfo;
  GUID uuid;
  SOCKADDR_BTH server; 
  SOCKET LocalSocket;
  SOCKET ClientSocket;
  u_long arg;
  int adlen;

    // Start WSA once only   
  WSAStartup((WORD)(2 + (2 << 8)), &wsadata);

    // Set up server and listen for client to connect
    
  LocalSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);

  server.addressFamily = AF_BTH;
  server.port = BT_PORT_ANY;
  server.btAddr = ADDR_ANY;  

  bind(LocalSocket,(struct sockaddr *) &server,sizeof(SOCKADDR_BTH)); 

  adlen = sizeof(SOCKADDR_BTH);
  getsockname(LocalSocket,(struct sockaddr *)&server,&adlen);

  csinfo.LocalAddr.iSockaddrLength = sizeof( SOCKADDR_BTH );
  csinfo.LocalAddr.lpSockaddr = (LPSOCKADDR)&server;
  csinfo.RemoteAddr.iSockaddrLength = sizeof( SOCKADDR_BTH );
  csinfo.RemoteAddr.lpSockaddr = (LPSOCKADDR)&server;
  csinfo.iSocketType = SOCK_STREAM;
  csinfo.iProtocol = BTHPROTO_RFCOMM;

  wsaq.dwSize = sizeof(WSAQUERYSET);
  wsaq.lpServiceClassId = &uuid;  

   // register UUID = FCF05AFD-67D8-4F41-83F57BEE22C03CDB
   // WSASetService chooses the RFCOMM channel   

  uuid.Data1 = 0xFCF05AFD;
  uuid.Data2 = 0x67D8;
  uuid.Data3 = 0x4F41;
  uuid.Data4[0] = 0x83;
  uuid.Data4[1] = 0xF5;
  uuid.Data4[2] = 0x7B;
  uuid.Data4[3] = 0xEE; 
  uuid.Data4[4] = 0x22;
  uuid.Data4[5] = 0xC0;
  uuid.Data4[6] = 0x3C;
  uuid.Data4[7] = 0xDB;

  wsaq.lpszServiceInstanceName = "My custom serial";
  wsaq.lpszComment = "Comment";
  wsaq.dwNameSpace = NS_BTH;
  wsaq.dwNumberOfCsAddrs = 1; 
  wsaq.lpcsaBuffer = &csinfo; 
  wsaq.lpVersion = NULL;
  wsaq.lpNSProviderId = NULL;
  wsaq.lpszContext = NULL;
  wsaq.dwNumberOfProtocols = 0;
  wsaq.lpafpProtocols = NULL;
  wsaq.lpszQueryString = NULL;
  wsaq.dwOutputFlags = 0;
  wsaq.lpBlob = NULL;
    
  WSASetService(&wsaq, RNRSERVICE_REGISTER, 0);

  listen(LocalSocket,SOMAXCONN);

  arg = 1;
  ioctlsocket(LocalSocket,FIONBIO, &arg);  // non-blocking

  do
    {  // loop waiting for client to connect
    ClientSocket = accept(LocalSocket,NULL, NULL);
    }
  while(ClientSocket == INVALID_SOCKET  && time out or stop listen test);
 
  arg = 1;
  ioctlsocket(ClientSocket,FIONBIO, &arg);  // non-blocking

   // ClientSocket is now an open serial channel to the client

   // read/write serial data
   
  recv(ClientSocket,buf,count,0);   
  send(ClientSocket,buf,count,0);

   // close connection
   
  closesocket(ClientSocket);
  closesocket(LocalSocket);

```

## 5.4.4 Android

This code sets up a custom 16-byte UUID serial channel.

```

AndroidManifest.xml 

  <uses-permission android:name="android.permission.BLUETOOTH" />
  <uses-permission android:name="android.permission.BLUETOOTH_ADMIN" />
  <uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />


MainActivity.java

  import android.bluetooth.BluetoothAdapter;
  import android.bluetooth.BluetoothDevice;
  import android.bluetooth.BluetoothServerSocket;
  import android.bluetooth.BluetoothSocket;
  import android.content.BroadcastReceiver;
  import java.io.File;
  import java.io.IOException;
  import java.io.InputStream;
  import java.io.OutputStream;
  import java.io.FileOutputStream;
  import java.util.Set;
  import java.util.UUID;
  import java.io.InputStreamReader;
  import java.io.OutputStreamWriter;

  BluetoothAdapter mBluetoothAdapter;
  BluetoothServerSocket mmServerSocket;
  BluetoothSocket mmSocket;
  InputStream instream;
  OutputStream outstream;  

     // initialise on program start

  mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
  if(!mBluetoothAdapter.isEnabled())
    {
    Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
    startActivity(enableBtIntent);
    }   

    // set up server

    // make discoverable for 300 seconds
  Intent discoverableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_DISCOVERABLE);
  discoverableIntent.putExtra(BluetoothAdapter.EXTRA_DISCOVERABLE_DURATION, 300);
  startActivity(discoverableIntent); 
  
     // register UUID = FCF05AFD-67D8-4F41-83F57BEE22C03CDB
     // the function chooses the RFCOMM channel

  mmServerSocket = mBluetoothAdapter.listenUsingInsecureRfcommWithServiceRecord(
           "My custom serial", UUID.fromString("FCF05AFD-67D8-4F41-83F57BEE22C03CDB"));

    // wait for client to connect - repeat until mmSocket != null 
  mmSocket = mmServerSocket.accept();  

  instream = mmSocket.getInputStream();
  outstream = mmSocket.getOutputStream();  

     // instream/outstream are now open connections to client

     // read/write serial data

  if(instream.available() > 0)
    data = instream.read();

  outstream.write(data);

     // disconnect

  instream.close();
  outstream.close();
  mmSocket.close();
  mmServerSocket.close(); 

```

## 6 Documentation References

References of the type v2,pE,7.7.14 (Vol 2 Part E Section 7.7.14) refer to the pdf 
document in the CS (Core Specification 5.2) section at
[Bluetooth Core Specification](https://www.bluetooth.com/specifications/specs/)

See the [RFCOMM](https://www.bluetooth.com/specifications/protocol-specifications/) section here for
bewildering details of serial connections.

HCI commands Vol 2, Part E, Section 7

L2CAP 0001 commands Vol 3, Part A, Section 4

L2CAP 0040+ commands RFCOMM reference above, but good luck making sense of it.

LE commands Vol 3, Part F, Section 3.4
   
LE characteristic UUIDs in a pdf document called "16-bit UUID Numbers Document"
[here](https://www.bluetooth.com/specifications/assigned-numbers/). 

SDP database decode Vol 3,Part B,Section 3

LE advertising decode Vol 2,Part E,Section 7.7.65.2 and Vol 3,Part C,Section 11. Also
[Data type codes](https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile/)
and
[Manufacturer codes](https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers/).
