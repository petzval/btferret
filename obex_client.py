import btfpy

#***************
# Send OBEX protocol data to node 4 which must be an OBEX push server
#
# Windows: Settings/Devices/Send or receive files via Bluetooth/Receive files
# Android: Enabling Bluetooth will normally make the device an OBEX server
#
# Sends NAME = hello.txt  DATA = Hello
# The receiver will nomally save DATA in a file called NAME
#
# For fully-programmed file send code see sendfileobex() in btferret.py
#***************

if btfpy.Init_blue("devices.txt") == 0:
  exit(0)
  
print("Node 4 must be TYPE=CLASSIC in devices.txt with a UUID = 1105 service")  
  # find OBEX channel - try 2 and 16 byte
channel = btfpy.Find_channel(4,btfpy.UUID_2,btfpy.Strtohex("1105"))
if(channel <= 0):
  channel = btfpy.Find_channel(4,btfpy.UUID_16,btfpy.Strtohex("00001105-0000-1000-8000-00805F9B34FB"))
if(channel <= 0):
  print("OBEX seervice not found")
  btfpy.Close_all()
  exit(0)   
  
btfpy.Connect_node(4,btfpy.CHANNEL_NEW,channel)
print("Send hello.txt conatining Hello")

connect = [0x80,0x00,0x07,0x10,0x00,0x01,0x90]
btfpy.Write_node(4,connect,0)
  # wait for Success reply 0x0A
inbuf = btfpy.Read_node_endchar(4,btfpy.PACKET_ENDCHAR,btfpy.EXIT_TIMEOUT,3000)
if(len(inbuf) == 0 or inbuf[0] != 0xA0):
  print("OBEX Connect failed")

send = [0x82,0x00,0x27,\
        0x01,0x00,0x17,0,104,0,101,0,108,0,108,0,111,0,46,0,116,0,120,0,116,0,0,\
        0xC3,0,0,0,5,\
        0x49,0,8,72,101,108,108,111]
btfpy.Write_node(4,send,0)
inbuf = btfpy.Read_node_endchar(4,btfpy.PACKET_ENDCHAR,btfpy.EXIT_TIMEOUT,3000)
if(len(inbuf) == 0 or inbuf[0] != 0xA0):
  print("Send failed")

disconnect = [0x81,0x00,0x03]   
btfpy.Write_node(4,disconnect,0)
inbuf = btfpy.Read_node_endchar(4,btfpy.PACKET_ENDCHAR,btfpy.EXIT_TIMEOUT,3000)
if(len(inbuf) == 0 or inbuf[0] != 0xA0):
  print("OBEX Disconnect failed")
   
btfpy.Disconnect_node(4)
btfpy.Close_all()
