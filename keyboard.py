#!/usr/bin/python3
import btfpy

# ********** Bluetooth keyboard **********
# From https://github.com/petzval/btferret
#   Build btfpy.so module - instructions in README file
#
# Download
#   keyboard.py
#   keyboard.txt
# 
# Edit keyboard.txt to set ADDRESS=
# to the address of the local device
# that runs this code
#
# Run
#   sudo python3 keyboard.py
#
# Connect from phone/tablet/PC to "HID" device
#
# All keystrokes go to connecting device
# F10 sends "Hello" plus Enter
# ESC stops the server 
#
# To add a battery level service:
# uncomment all battery level labelled
# code in keyboard.txt and here.
# F10 will then also send a battery level notification
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

#/*********  keyboard.txt DEVICES file ******
# DEVICE = My Pi   TYPE=Mesh  node=1  ADDRESS = DC:A6:32:04:DB:56
#  PRIMARY_SERVICE = 1800
#    LECHAR=Device Name   SIZE=4   Permit=02 UUID=2A00  
#    LECHAR=Appearance    SIZE=2   Permit=02 UUID=2A01  
#  PRIMARY_SERVICE = 180A
#    LECHAR= PnP ID       SIZE=7 Permit=02 UUID=2A50  
#  PRIMARY_SERVICE = 1812
#    LECHAR=Protocol Mode   SIZE=1  Permit=06  UUID=2A4E  
#    LECHAR=HID Info        SIZE=4  Permit=02  UUID=2A4A  
#    LECHAR=HID Ctl Point   SIZE=8  Permit=04  UUID=2A4C  
#    LECHAR=Report Map      SIZE=47 Permit=02  UUID=2A4B  
#    LECHAR=Report1         SIZE=8  Permit=92  UUID=2A4D  
#        ; Report1 must have Report ID = 1 
#        ;   0x85, 0x01 in Report Map
#        ; uuid = [0x2A,0x4D]
#        ; index = btfpy.Find_ctic_index(btfpy.Localnode(),btfpy.UUID_2,uuid)
#        ; Send data: btfpy.Write_ctic(btfpy.Localnode(),index,data,0)
#
# ;  *** Optional battery level ***
# ;  PRIMARY_SERVICE = 180F
# ;    LECHAR=Battery Level   SIZE=1 Permit=12  UUID=2A19   
#
# ********

# **** KEYBOARD REPORT MAP *****
# 0x05, 0x01 Usage Page (Generic Desktop)
# 0x09, 0x06 Usage (Keyboard)
# 0xa1, 0x01 Collection (Application)
# 0x85, 0x01 Report ID = 1
# 0x05, 0x07 Usage Page (Keyboard)
# 0x19, 0xe0 Usage Minimum (Keyboard LeftControl)
# 0x29, 0xe7 Usage Maximum (Keyboard Right GUI)
# 0x15, 0x00 Logical Minimum (0)
# 0x25, 0x01 Logical Maximum (1)
# 0x75, 0x01 Report Size (1)  
# 0x95, 0x08 Report Count (8)
# 0x81, 0x02 Input (Data, Variable, Absolute) Modifier byte
# 0x95, 0x01 Report Count (1)
# 0x75, 0x08 Report Size (8)
# 0x81, 0x01 Input (Constant) Reserved byte
# 0x95, 0x06 Report Count (6)
# 0x75, 0x08 Report Size (8)
# 0x15, 0x00 Logical Minimum (0)
# 0x25, 0x65 Logical Maximum (101)
# 0x05, 0x07 Usage Page (Key Codes)
# 0x19, 0x00 Usage Minimum (Reserved (no event indicated))
# 0x29, 0x65 Usage Maximum (Keyboard Application)
# 0x81, 0x00 Input (Data,Array) Key arrays (6 bytes)
# 0xc0 End Collection
#*******************

    # NOTE the size of reportmap (47 in this case) must appear in keyboard.txt as follows:
    #   LECHAR=Report Map      SIZE=47 Permit=02  UUID=2A4B  
reportmap = [0x05,0x01,0x09,0x06,0xA1,0x01,0x85,0x01,0x05,0x07,0x19,0xE0,0x29,0xE7,0x15,0x00,\
             0x25,0x01,0x75,0x01,0x95,0x08,0x81,0x02,0x95,0x01,0x75,0x08,0x81,0x01,0x95,0x06,\
             0x75,0x08,0x15,0x00,0x25,0x65,0x05,0x07,0x19,0x00,0x29,0x65,0x81,0x00,0xC0]

    # NOTE the size of report (8 in this case) must appear in keyboard.txt as follows:
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

def lecallback(clientnode,op,cticn):
    
  if(op == btfpy.LE_CONNECT):
    print("Connected OK. Key presses sent to client. ESC stops server")
    print("F10 sends Hello plus Enter")
 
  if(op == btfpy.LE_KEYPRESS):
    # cticn = ASCII code of key OR btferret custom code
    if(cticn == 23):
      # 23 = btferret custom code for F10
      # Send "Hello" plus Enter
      # Must use ord() to send ASCII value from a string
      hello = "Hello\n"
      for n in range(len(hello)):
        send_key(ord(hello[n]))
          
      #**** battery level ****
      #  if(battery[0] > 0):
      #    battery[0] = battery[0] - 1
      #  uuid = [0x2A,0x19]
      #  btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),battery,1)
      #*******************
    else:  
      send_key(cticn)      
 
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

############ START ###########
   
if(btfpy.Init_blue("keyboard.txt") == 0):
  exit(0)

if(btfpy.Localnode() != 1):
  print("ERROR - Edit keyboard.txt to set ADDRESS = " + btfpy.Device_address(btfpy.Localnode()))
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
   
  #**** battery level *****
  # uuid = [0x2A,0x19]
  # btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),battery,1) 
  #************************     
                          
  # Set unchanging random address by hard-coding a fixed value.
  # If connection produces an "Attempting Classic connection"
  # error then choose a different address.
  # If set_le_random_address() is not called, the system will set a
  # new and different random address every time this code is run.  
 
  # Choose the following 6 numbers
  # 2 hi bits of first number must be 1
randadd = [0xD3,0x56,0xDB,0x15,0x32,0xA0]
btfpy.Set_le_random_address(randadd)
     
btfpy.Keys_to_callback(btfpy.KEY_ON,0)   # enable LE_KEYPRESS calls in lecallback
                                         # 0 = GB keyboard  
btfpy.Set_le_wait(20000)  # Allow 20 seconds for connection to complete
                                         
btfpy.Le_pair(btfpy.Localnode(),btfpy.JUST_WORKS,0)  # Easiest option, but if client requires
                                                     # passkey security - remove this command  

btfpy.Le_server(lecallback,0)
  
btfpy.Close_all()
