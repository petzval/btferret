#!/usr/bin/python3
import btfpy
import datetime

# ********** Bluetooth LE time server **********
# From https://github.com/petzval/btferret
#   Version 20 or later
#   Build btfpy.so module - instructions in README file
#
# Download
#   timserver.py    this code
#   timserver.txt
# 
# Edit timserver.txt to set ADDRESS=
# to the address of the local device
# that runs this code
#
# Run
#   sudo python3 timserver.py
#
# Connect from phone/tablet/PC to "Time" device
#
# Client can read time characteristic or enable
# notifications that are sent one per second
#
# This code sets an unchanging random address.
# If connection is unreliable try changing the address.
#
# ********************************    

name = "BTF"
node = 0
timeindex = -1
conflag = 0
notflag = 0
curtime = [0,0,0,0,0,0,0,0,0,0]

def lecallback(clientnode,op,cticn):
  global timeindex
  global conflag
  global notflag
  global curtime
  global node
  
  if(op == btfpy.LE_CONNECT):
    print("Connected OK")
    conflag = 1
  elif(op == btfpy.LE_NOTIFY_ENABLE): 
    print("Notifications enabled")
    notflag = 1
  elif(op == btfpy.LE_NOTIFY_DISABLE):
    print("Notifications disabled")
    notflag = 0
  elif(op == btfpy.LE_TIMER and notflag != 0):
    getcurtime()  
    # send notification if client has enabled  
    btfpy.Write_ctic(node,timeindex,curtime,0)
  elif(op == btfpy.LE_READ and cticn == timeindex and notflag == 0):
    getcurtime()   # read time to curtime[]
    btfpy.Write_ctic(node,timeindex,curtime,0)
  elif(op == btfpy.LE_DISCONNECT):
    return(btfpy.SERVER_EXIT)
  return(btfpy.SERVER_CONTINUE)
 

def getcurtime():
  global curtime

  dt = datetime.datetime.now()

  curtime[0] = dt.year & 0xFF
  curtime[1] = (dt.year >> 8) & 0xFF
  curtime[2] = dt.month
  curtime[3] = dt.day
  curtime[4] = dt.hour
  curtime[5] = dt.minute
  curtime[6] = dt.second
  curtime[7] = datetime.date(dt.year,dt.month,dt.day).isoweekday()
  curtime[8] = 0  # 1/256 second
  curtime[9] = 1  # adjust reason bits  0=manual 1=external reference
                  #      2=change time zone  3=change daylight saving               
# end getcurtime


############ START ###########
   
if(btfpy.Init_blue("timserver.txt") == 0):
  exit(0)

if(btfpy.Localnode() != 1):
  print("ERROR - Edit timserver.txt to set ADDRESS = " + btfpy.Device_address(btfpy.Localnode()))
  exit(0)
      
node = btfpy.Localnode()    

# Write data to local characteristics
getcurtime()  # read time to curtime
uuid = [0x2A,0x2B]
timeindex = btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid)
btfpy.Write_ctic(node,timeindex,curtime,0)
                 # local time information
locinfo = [0,0]  # time zone in 15min steps  -48 to 56
                 # daylight savings time bits
                 # 0=standard  2=+0.5h  4=+1h   8=+2h                               
uuid = [0x2A,0x0F]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),locinfo,0)

  # reference time information
  # [0] time source 0=unknown 1=network 2=GPS 3=radio
  #     4=manual 5=atomic clock 6=cellular network
  # [1] time accuracy 255=unknown
  #     1-253 drift in 1/8 second steps
  # [2] days since update 255=greater than 254
  # [3] hours since update 0-23  255=greater than 23                                    
refinfo = [0,255,255,255]
uuid = [0x2A,0x14]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),refinfo,0)

uuid[0] = [0x2A,0x29] # manufacturer name
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),name,0)

  # advertise time service 1805 when set_le_random_address() is called
uuid = [0x18,0x05]
btfpy.Uuid_advert(uuid)
                      
  # Set unchanging random address by hard-coding a fixed value.
  # If connection produces an "Attempting Classic connection"
  # error then choose a different address.
  # If set_le_random_address() is not called, the system will set a
  # new and different random address every time this code is run.  
 
  # Choose the following 6 numbers
  # 2 hi bits of first number must be 1
randadd = [0xD3,0x56,0xD6,0x74,0x33,0x03]
btfpy.Set_le_random_address(randadd)

btfpy.Set_le_wait(20000)  # Allow 20 seconds for connection to complete
                                       
btfpy.Le_server(lecallback,10)   # 1 second timer
  
btfpy.Close_all()
