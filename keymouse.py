#!/usr/bin/python3
import btfpy
import os

# ********** Bluetooth keyboard and mouse **********
# From https://github.com/petzval/btferret
#   Version 20 or later
#   Build btfpy.so module - instructions in README file
#
# Download
#   keymouse.py
#   keymouse.txt
# 
# Edit keymouse.txt to set ADDRESS=
# to the address of the local device
# that runs this code
#
# Run
#   sudo python3 keymouse.py
#
# Connect from phone/tablet/PC to "HID" device
#
# All keystrokes go to client
# F10 sends "Hello" plus Enter
# ESC stops the server 
#
# Mouse movements and button clicks go to client
#
# Note: This code uses the lowest level of security.
# Do not use it if you need high security.
#
# Non-GB keyboards
# Even if the keyboard of this device is non-GB
# it must be specified as "gb" in the boot info as follows:
#
# Edit /etc/default/keyboard to include the line:
# XKBLAYOUT="gb"
#
# It is the receiving device that decides which
# characters correspond to which keys. See discussion
# in the HID Devices section of the documentation.
#
# This code sets an unchanging random address.
# If connection is unreliable try changing the address.
#
# See HID Devices section in documentation for 
# more infomation.
#
# ********************************    

    # NOTE the size of reportmap (99 in this case) must appear in keymouse.txt as follows:
    #   LECHAR=Report Map      SIZE=99 Permit=02  UUID=2A4B  
reportmap = [0x05,0x01,0x09,0x06,0xA1,0x01,0x85,0x01,0x05,0x07,0x19,0xE0,0x29,0xE7,0x15,0x00,\
             0x25,0x01,0x75,0x01,0x95,0x08,0x81,0x02,0x95,0x01,0x75,0x08,0x81,0x01,0x95,0x06,\
             0x75,0x08,0x15,0x00,0x25,0x65,0x05,0x07,0x19,0x00,0x29,0x65,0x81,0x00,0xC0,\
             0x05,0x01,0x09,0x02,0xA1,0x01,0x85,0x02,0x09,0x01,0xA1,0x00,0x05,0x09,0x19,0x01,\
             0x29,0x03,0x15,0x00,0x25,0x01,0x95,0x03,0x75,0x01,0x81,0x02,0x95,0x01,0x75,0x05,\
             0x81,0x01,0x05,0x01,0x09,0x30,0x09,0x31,0x15,0x81,0x25,0x7F,0x75,0x08,0x95,0x02,\
             0x81,0x06,0xC0,0xC0]

    # NOTE the size of report (8 in this case) must appear in keymouse.txt as follows:
    #   LECHAR=Report1         SIZE=8  Permit=92  UUID=2A4D  
report = [0,0,0,0,0,0,0,0]

name = "HID"
appear = [0xC1,0x03]  # 03C1 = keyboard icon appears on connecting device 
pnpinfo = [0x02,0x6B,0x1D,0x46,0x02,0x37,0x05]
protocolmode = [0x01]
hidinfo = [0x01,0x11,0x00,0x02]
battery = [100] 
reportindex = -1
node = 0
fd = None

def lecallback(clientnode,op,cticn):
  global fd
      
  if(op == btfpy.LE_CONNECT):
    fd = open('/dev/input/mouse0','rb')
    if(fd == None):
      print("Connected OK. Keys to client. Fail to open /dev/input/mouse0. ESC stops server")
    else: 
      print("Connected OK. Keys/Mouse sent to client. ESC stops server")
      os.set_blocking(fd.fileno(),False)
    print("F10 sends Hello plus Enter")
 
  if(op == btfpy.LE_KEYPRESS):
    # cticn = ASCII code of key OR btferret custom code
    send_key(cticn)      
 
  if(fd != None and op == btfpy.LE_TIMER):   
    buf = fd.read(3)
    if(buf != None and len(buf) == 3):
      send_mouse(buf[1],-buf[2],buf[0])      
      # OR if Y is reversed  send_mouse(buf[1],buf[2],buf[0])

  if(op == btfpy.LE_DISCONNECT):
    return(btfpy.SERVER_EXIT)
  return(btfpy.SERVER_CONTINUE)
 

#*********** SEND KEY *****************
# key = ASCII code of character (e.g a=97) OR one of the
#            following btferret custom codes:
#
# 1 = Pause     8 = Backspace  17 = F4     24 = F11
# 2 = Insert    9 = Tab        18 = F5     25 = F12
# 3 = Del      10 = Enter      19 = F6     27 = Esc
# 4 = Home     11 = Pound (^3) 20 = F7     28 = Right arrow
# 5 = End      14 = F1         21 = F8     29 = Left arrow
# 6 = PgUp     15 = F2         22 = F9     30 = Down arrow
# 7 = PgDn     16 = F3         23 = F10    31 = Up arrow 
#
# ASCII codes                    'a' = 97               (valid range 32-126)
# CTRL add 128 (0x80)         CTRL a = 'a' + 128 = 225  (valid range 225-255)
# Left ALT add 256 (0x100)     ALT a = 'a' + 256 = 353  (valid range 257-382)
# Right ALT add 384 (0x180)  AltGr a = 'a' + 384 = 481  (valid range 481-516)
#
# SHIFT F1-F8 codes SHIFT F1 = 471  (valid range 471-478) 
#
# Note CTRL i = same as Tab  CTRL m = same as Enter  
# Some ALT keys generate ASCII codes
#
# To send k: send_key('k')  
# To send F1: send_key(14)
# To send CTRL b:  send_key(226) same as send_key('b' | 0x80)
# To send AltGr t: send_key(500) same as send_key('t' | 0x180)
#
# These key codes are also listed in the
# keys_to_callback() section in documentation
#
# Modifier bits, hex values:
# 01=Left CTL  02=Left Shift  04=Left Alt  08=Left GUI
# 10=Right CTL 20=Right Shift 40=Right Alt 80=Right GUI
# 
#*************************************

def send_key(key):
  global reportindex
  global node
   
  # convert btferret code (key) to HID code  
  hidcode = btfpy.Hid_key_code(key)
  if(hidcode == 0):
    return

  buf = [0,0,0,0,0,0,0,0] 
        
  # send key press to Report1
  buf[0] = (hidcode >> 8) & 0xFF  # modifier
  buf[2] = hidcode & 0xFF         # key code
  btfpy.Write_ctic(node,reportindex,buf,0)
  # send no key pressed - all zero
  buf[0] = 0
  buf[2] = 0
  btfpy.Write_ctic(node,reportindex,buf,0) 
  return

#*********** SEND MOUSE *****************
# x,y = mouse movement -127 to 127
#
# but  1 = Left button click
#      2 = Right button click
#      4 = Middle button click
#  
#  For a single click these button presses are followed
#  by a buf[0]=0 send which indicates button up.
#      
#********************************/

def send_mouse(x,y,but):
  global reportindex
  global node

  # convert signed xy to signed byte
  if(x < 0):
    ux = x + 256
  else:
    ux = x
    
  if(y < 0):
    uy = y + 256
  else:
    uy = y
           
  # send to Report2
  btfpy.Write_ctic(node,reportindex+1,[but,ux,uy],0)
 
  return


############ START ###########
   
if(btfpy.Init_blue("keymouse.txt") == 0):
  exit(0)

if(btfpy.Localnode() != 1):
  print("ERROR - Edit keymouse.txt to set ADDRESS = " + btfpy.Device_address(btfpy.Localnode()))
  exit(0)
      
node = btfpy.Localnode()    

# look up Report1 index
uuid = [0x2A,0x4D]
reportindex = btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid)
if(reportindex < 0):
  print("Failed to find Report characteristic")
  exit(0)

  # Write data to local characteristics  node=local node
uuid = [0x2A,0x00]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),name,0) 

uuid = [0x2A,0x01]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),appear,0) 

uuid = [0x2A,0x4E]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),protocolmode,0)

uuid = [0x2A,0x4A]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),hidinfo,0)

uuid = [0x2A,0x4B]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),reportmap,0)

uuid = [0x2A,0x4D]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),report,0)

uuid = [0x2A,0x50]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),pnpinfo,0)
   
                         
  # Set unchanging random address by hard-coding a fixed value.
  # If connection produces an "Attempting Classic connection"
  # error then choose a different address.
  # If set_le_random_address() is not called, the system will set a
  # new and different random address every time this code is run.  
 
  # Choose the following 6 numbers
  # 2 hi bits of first number must be 1
randadd = [0xD3,0x56,0xD6,0x74,0x33,0x06]
btfpy.Set_le_random_address(randadd)
     
btfpy.Keys_to_callback(btfpy.KEY_ON,0)   # enable LE_KEYPRESS calls in lecallback
                                         # 0 = GB keyboard  
btfpy.Set_le_wait(20000)  # Allow 20 seconds for connection to complete
                                         
btfpy.Le_pair(btfpy.Localnode(),btfpy.JUST_WORKS,0)  # Easiest option, but if client requires
                                                     # passkey security - remove this command  

btfpy.Set_flags(btfpy.FAST_TIMER,btfpy.FLAG_ON)
btfpy.Le_server(lecallback,20)  # 20ms fast timer
 
if fd != None:
  fd.close()
   
btfpy.Close_all()
