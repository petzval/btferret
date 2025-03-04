#!/usr/bin/python3
import btfpy
import random
import math

# ********** Bluetooth LE heart monitor **********
# From https://github.com/petzval/btferret
#   Version 20 or later
#   Build btfpy.so module - instructions in README file
#
# Download
#   heart.py
#   heart.txt
# 
# Edit heart.txt to set ADDRESS=
# to the address of the local device
# that runs this code
#
# Run
#   sudo python3 heart.py
#
# Connect from phone/tablet/PC to "Heart" device
#
# Simulates a slowly changing average heart rate
# with a +/-25ms random variation in the intervals   
#
# This code sets an unchanging random address.
# If connection is unreliable try changing the address.
#
# ********************************    


sens = [0x01]  # sensor location 1=Chest
               # 0=Other 1=Chest 2=Wrist 3=Finger
               # 4=Hand  5=Ear lobe 6=Foot
heart = [0,0,0,0,0,0,0,0]
name = "BTF"
node = 0
notifyindex = -1
controlindex = 0
conflag = 0
rate = 100
beatlist = [0,0,0,0,0,0,0,0]
beat = 1000
delbeat = -50

def lecallback(clientnode,op,cticn):
  global notifyindex
  global controlindex
  global conflag
  global rate
  global beatlist  
  global flag  
  global beat
  global delbeat
  global node
      
  if(op == btfpy.LE_CONNECT):
    print("Connected OK. Data sent to client every second. ESC stops server")
    conflag = 1
    # set up simulated data with an array of beat times
    beatlist[0] = btfpy.Time_ms() + beat
    beat = beat + delbeat
    for n in range(1,8):
      ran = random.random()
      randel = (ran * 100.0) - 25.0
      beatlist[n] = beatlist[n-1] + beat + randel
      beat = beat + delbeat
         
    
  if(op == btfpy.LE_TIMER and conflag != 0):
    # send notification
    # heart[0] = flags   bit 0
    #                      xxxxxxx0 = 1-byte rate follows
    #                      xxxxxxx1 = 2-byte rate follows
    #                    bits 1/2
    #                      xxxxx00x = no sensor contact info
    #                      xxxxx10x = sensor not in contact
    #                      xxxxx11x = sensor in contact
    #                    bit 4
    #                      xxxx0xxx = no rr interval data
    #                      xxxx1xxx = variable number of 2-byte rr intervals follow
    # heart[1] = rate bpm
    #  followed by a variable number (maybe 0)
    #  of 2-byte intervals (1/1024 s count)
    # heart[2][3] = 1st interval
    # heart[4][5] = 2nd interval
    # ...
    # This code simulates a slowly changing average rate with
    # a +/-25ms random variation in the intervals   
        
    # set average rate
    ratex = (60*4096)/(beatlist[4] - beatlist[0])
    rate = math.floor(ratex)
    heart[0] = 0 
    heart[1] = rate  # beats per min 

    # set up simulated interval data
    # add interval data as required
    tim = btfpy.Time_ms()
    hn = 0
    while(tim > beatlist[1]):
      # add a RR interval entry
      heart[0] = heart[0] | 16  # flags set RR data bit
      # rr = interval between beats in 1/1024 second count
      rr = beatlist[1] - beatlist[0]
      # add 2-byte RR data
      heart[hn+2] = math.floor(rr) & 0xFF          # RR lo byte
      heart[hn+3] = (math.floor(rr) >> 8) & 0xFF   # RR hi byte
      hn = hn + 2  # add two bytes to notification data send
     
      for n in range(7):
        beatlist[n] = beatlist[n+1]
      ran = random.random()
      randel = (ran * 50.0) - 25.0
      beatlist[7] = beatlist[6] + beat + randel
      beat = beat + delbeat
      if(beat > 1200):
        delbeat = delbeat-50
      if(beat < 400):
        delbeat = 50
      
    # end set up simulated data
      
    # send notification by writing to local node 
    btfpy.Write_ctic(node,notifyindex,heart,2+hn)
 
  if(op == btfpy.LE_WRITE and cticn == controlindex):
    buf = btfpy.Read_ctic(node,cticn)
    print("Control point opcode received = " + str(buf[0]))
    # opcode 1=zero expended energy
    # Do nothing
   
  if(op == btfpy.LE_DISCONNECT):
    return(btfpy.SERVER_EXIT)
  return(btfpy.SERVER_CONTINUE)
 

############ START ###########
   
if(btfpy.Init_blue("heart.txt") == 0):
  exit(0)

if(btfpy.Localnode() != 1):
  print("ERROR - Edit heart.txt to set ADDRESS = " + btfpy.Device_address(btfpy.Localnode()))
  exit(0)
      
node = btfpy.Localnode()    

# Write data to local characteristics
uuid = [0x2A,0x5D]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),sens,0)

uuid = [0x2A,0x37]
notifyindex = btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid)
if(notifyindex < 0):
  print("Failed to find data characteristic")
  exit(0)
btfpy.Write_ctic(node,notifyindex,heart,0);

uuid = [0x2A,0x29]
btfpy.Write_ctic(node,btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid),name,0)

uuid = [0x2A,0x39]
controlindex = btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid)

# advertise heart service 180D when set_le_random_address() is called
uuid = [0x18,0x0D]
btfpy.Uuid_advert(uuid)
                      
  # Set unchanging random address by hard-coding a fixed value.
  # If connection produces an "Attempting Classic connection"
  # error then choose a different address.
  # If set_le_random_address() is not called, the system will set a
  # new and different random address every time this code is run.  
 
  # Choose the following 6 numbers
  # 2 hi bits of first number must be 1
randadd = [0xD3,0x56,0xD6,0x74,0x33,0x01]
btfpy.Set_le_random_address(randadd)

btfpy.Set_le_wait(20000)  # Allow 20 seconds for connection to complete
                                         
btfpy.Le_server(lecallback,10)   # 1 second notifications
  
btfpy.Close_all()
