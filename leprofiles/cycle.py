#!/usr/bin/python3
import btfpy

# ********** Bluetooth LE Cycle speed and cadence server **********
# From https://github.com/petzval/btferret
#   Version 20 or later
#   Build btfpy.so module - instructions in README file
#
# Download
#   cycle.py    this code
#   cycle.txt
# 
# Edit cycle.txt to set ADDRESS=
# to the address of the local device
# that runs this code
#
# Run
#   sudo python3 cycle.py
#
# Connect from phone/tablet/PC to "Cycle" device
#
# Notifications of cadence and speed are sent once per second
# Constant cadence = 60 per min
# Constant wheel rotations = 60 per min
#
# This code sets an unchanging random address.
# If connection is unreliable try changing the address.
#
# ********************************    

name = "BTF"
node = 0
cycleindex = -1
conflag = 0
notflag = 0
feat = [3,0]   # 3 = wheel and crank data in notification
sens = [0]         # sensor location 0=Other
                   # 1=top of shoe 2=in shoe 3=hip 4=front wheel 5=left crank
                   # 6=right crank 7=left pedal 8=right pedal 9=front hub
                   # 10=rear dropout 11=chainstay 12=rear wheel 13=rear hub
                   # 14=chest 15=spider 16=chain ring                                    
cycle = [0x03,0,0,0,0,0,0,0,0,0,0]  # 3=wheel and crank data
conflag = 0
notflag = 0
crev = 0
lastct = 0
wrev = 0
lastwt = 0
reply = [0,0,0]

def lecallback(clientnode,op,cticn):
  global cycleindex
  global conflag
  global notflag
  global node
  global lastwt
  global lastct
  global crev
  global wrev
  global reply
  
  if(op == btfpy.LE_CONNECT):
    print("Connected OK")
    conflag = 1
    crev = 0
    wrev = 0
    lastct = 0
    lastwt = 0   
  elif(op == btfpy.LE_NOTIFY_ENABLE): 
    print("Notifications enabled")
    notflag = 1
  elif(op == btfpy.LE_NOTIFY_DISABLE):
    print("Notifications disabled")
    notflag = 0
  elif(op == btfpy.LE_TIMER and notflag != 0):
    # send notification 11 bytes
    # [0] = flags     00000011 = wheel and crank data
    # [1][2][3][4] = cumulative count of wheel rotations (lo byte first)
    # [5][6] = time of last wheel event (1/1024 s units)
    # [7][8] = cumulative count of crank rotations
    # [9][10] = time of last crank event
    
    wrev = wrev + 1   # one more wheel rev
    lastwt = lastwt + 1024  # in 1 second
    crev = crev + 1   # one more crank rev
    lastct = lastct + 1024  # in 1 second             
    # wrev=cumulative wheel rotations  lastwt=last wheel event time
    # crev=cumulative crank rotations  lastct=last crank event time           
    cycle[0] = 3 # wheel and crank data
    cycle[1] = wrev & 0xFF
    cycle[2] = (wrev >> 8) & 0xFF
    cycle[3] = (wrev >> 16) & 0xFF
    cycle[4] = (wrev >> 24) & 0xFF
    cycle[5] = lastwt & 0xFF
    cycle[6] = (lastwt >> 8) & 0xFF

    cycle[7] = crev & 0xFF
    cycle[8] = (crev >> 8) & 0xFF
    cycle[9] = lastct & 0xFF
    cycle[10] = (lastct >> 8) & 0xFF
    
    # send 11-byte notification    
    btfpy.Write_ctic(node,cycleindex,cycle,0)
  elif(op == btfpy.LE_WRITE and cticn == controlindex):
    # Control point
    buf = btfpy.Read_ctic(node,cticn)
    print("Contol point received opcode = " + str(buf[0]))
      # opcode 1=set cumulative wheel rotations = buf[1]-[4]
      #        2=calibration
      #        3=update sensor location = buf[1]
      #        4=request sensor location
      # Do nothing but send OK reply
    reply[0] = 0x10  # response opcode
    reply[1] = buf[0]  # request opcode
    reply[2] = 1  # 1=success
                    # 2=opcode not supported
                    # 3=invalid parameter
                    # 4=response failed
      # reply[3]... data if required
      # send reply as indication
    btfpy.Write_ctic(node,controlindex,reply,0)
  elif(op == btfpy.LE_DISCONNECT):
    return(btfpy.SERVER_EXIT)
  return(btfpy.SERVER_CONTINUE)
 

############ START ###########
   
if(btfpy.Init_blue("cycle.txt") == 0):
  exit(0)

if(btfpy.Localnode() != 1):
  print("ERROR - Edit cycle.txt to set ADDRESS = " + btfpy.Device_address(btfpy.Localnode()))
  exit(0)
      
node = btfpy.Localnode()    

# Write data to local characteristics

uuid = [0x2A,0x5C]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),feat,0) 

uuid = [0x2A,0x5B]
cycleindex = btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid)
btfpy.Write_ctic(node,cycleindex,cycle,0)

uuid = [0x2A,0x5D]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),sens,0)

uuid = [0x2A,0x29]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),name,0)

uuid = [0x2A,0x55]
controlindex = btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid)

  # advertise cycling speed and cadence service 1816
  # when set_le_random_address() is called
uuid = [0x18,0x16]
btfpy.Uuid_advert(uuid)
                      
  # Set unchanging random address by hard-coding a fixed value.
  # If connection produces an "Attempting Classic connection"
  # error then choose a different address.
  # If set_le_random_address() is not called, the system will set a
  # new and different random address every time this code is run.  
 
  # Choose the following 6 numbers
  # 2 hi bits of first number must be 1
randadd = [0xD3,0x56,0xD6,0x74,0x33,0x02]
btfpy.Set_le_random_address(randadd)

btfpy.Set_le_wait(20000)  # Allow 20 seconds for connection to complete

btfpy.Le_server(lecallback,10)   # 1 second timer
  
btfpy.Close_all()
