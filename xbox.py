#!/usr/bin/python3
import btfpy

class itemdata:
  def __init__(self,flag,cticn,dirn,offset,numbytes,size,shift,bitmask,min,max,value,desc):
    self.flag = flag
    self.cticn = cticn
    self.dirn = dirn
    self.offset = offset
    self.numbytes = numbytes
    self.size = size
    self.shift = shift
    self.bitmask = bitmask
    self.min = min
    self.max = max
    self.value = value
    self.desc = desc

ios = [ "in ","out" ]
inouts = ["Invalid","Input","Output","Input and output"]
outdat = []
stopflag = 0
firstflag = 0
  
def mycode_handler(jn):
  global stopflag
  global rep1

  # Input jn has changed to newvalue
  
  newvalue = rep1[jn].value

  if(jn == 1):
    # L stick X
    pass
  elif(jn == 2):
    # L stick Y 
    pass
  elif(jn == 3):
    # R stick X
    pass
  elif(jn == 4):
    # R stick Y
    pass
  elif(jn == 5):
    # L trigger 
    pass
  elif(jn == 6):
    # R trigger
    pass
  elif(jn == 7):
    # D pad
    pass
  elif(jn == 8):
    # A button
    pass
  elif(jn == 9):
    # B button
    pass
  elif(jn == 10):
    # Unknown
    pass
  elif(jn == 11):
    # X button
    pass
  elif(jn == 12):
    # Y button
    pass
  elif(jn == 13):
    # Unknown
    pass
  elif(jn == 14):
    # L bumper
    pass
  elif(jn == 15):
    # R bumper
    pass
  elif(jn == 16):
    # Unknown
    pass
  elif(jn == 17):
    # Unknown
    pass
  elif(jn == 18):
    # View button
    pass
  elif(jn == 19):
    # Menu button
    pass
  elif(jn == 20):     
    # Xbox button
    if(newvalue == 1):
      stopflag = 1
  elif(jn == 21):
    # Unknown
    pass
  elif(jn == 22):
    # Unknown
    pass
  elif(jn == 23):
    # Share button
    pass
    
  return(0) 
  
def mycode_callback(node,cticn,dat,datlen):
  global rep1
  global firstflag
   
  if(cticn != rep1[0].cticn):
    return(0) 

  jn = 1
  while(rep1[jn].flag != 0):
    dn = rep1[jn].offset
    val = 0
    for n in range(rep1[jn].numbytes-1,-1,-1):
      val = (val << 8) + dat[dn+n]
    val = val >> rep1[jn].shift
    val = val & rep1[jn].bitmask
    if(firstflag == 0):
      rep1[jn].value = val
    elif(rep1[jn].value != val):
      rep1[jn].value = val
      mycode_handler(jn)
    jn = jn + 1 
  firstflag = 1
  return(0)
  # end mycode_callback


def monitor_callback(node,cticn,dat,datlen):
  global stopflag
  global rep1
  global firstflag
  
  if(cticn != rep1[0].cticn):
    return(0) 

  jn = 1
  while(rep1[jn].flag != 0):
    dn = rep1[jn].offset
    val = 0
    for n in range(rep1[jn].numbytes-1,-1,-1):
      val = (val << 8) + dat[dn+n]
    val = val >> rep1[jn].shift
    val = val & rep1[jn].bitmask
    if(firstflag == 0):
      rep1[jn].value = val
    elif(rep1[jn].value != val):
      rep1[jn].value = val
      if(jn == 1 or jn == 2):
        print("1,2 L stick = %d,%d" % (rep1[1].value,rep1[2].value))       
      elif(jn == 3 or jn == 4):
        print("3,4 R stick = %d,%d" % (rep1[3].value,rep1[4].value))       
      else:
        if(jn <= 31):
          print("%d %s = %d" % (jn,rep1[jn].desc,val))
        else:
          print("%d Unknown = %d" % (jn,val))
    jn = jn + 1 
    

  if(rep1[20].value != 0):
    stopflag = 1
  firstflag = 1   
  return(0)
  

def sendoutput(node):
  global rep2
  global outdat
  
  btfpy.Write_ctic(node,rep2[0].cticn,outdat,rep2[0].numbytes)
  
      
def setoutput(jn,val):
  global rep2
  global output
  
  if(jn < 0 or jn > 31 or rep2[jn].flag == 0):
    return(0)
  
  xval = val << rep2[jn].shift  
  bitmask = rep2[jn].bitmask << rep2[jn].shift
  off = rep2[jn].offset
  for n in range(rep2[jn].numbytes):
    msk = bitmask & 0xFF
    byt = xval & msk
    outdat[off] = outdat[off] & (~msk) & 0xFF
    outdat[off] = outdat[off] | byt
    xval = xval >> 8
    bitmask = bitmask >> 8
    off = off+1   
  return(0)
 
  
def inputint(ps):
  flag = 1
  val = -1
  while flag != 0:
    print(ps + "  (x=cancel)")
    s = input("? ")
    flag = 0
    if(len(s) > 0):
      for n in range(len(s)):
        if(s[n] < '0' or s[n] > '9'):
          flag = 1
      if flag == 0:
        val = int(s)
      elif s[0] == 'x':
        val = -1
        flag = 0
      else:
        print("Not a number")
    else:
      flag = 1
    # end while flag != 0  
 
  return(val)


def search():
 
  btfpy.Le_scan()   
  node = 0
  n = 1000
  print("DEVICES")
  count = 0
  while(btfpy.Device_type(n) != 0):
    count = count+1
    s = btfpy.Le_advert(n)
    len = s[0]
    print("  Node = %d Address = %s " % (n,btfpy.Device_address(n)),end="") 
    i = 1
    flag = 0
    while(i < len-1 and flag == 0):
      if((s[i+1] == 0x19 and s[i+2] == 0xC4 and s[i+3] == 0x03) or
         (s[i+1] == 0x03 and s[i+2] == 0x12 and s[i+3] == 0x18)):
        print("Gamepad",end="")
        flag = 1
        if(node == 0):
          node = n
      i = i + s[i]+1
    print()
    n = n + 1 
  return(count,node)
  
 
  
def printmap(mapindex): 
  rep = replist[mapindex]
  print("         Report %d %s" % (mapindex+1,inouts[rep[0].dirn]))
  print("         Bits  Offset  Nbytes  Shift   Min      Max")
  n = 1
  while(rep[n].flag != 0):
    print("%2d  %s   %2d    %2d      %2d      %2d  %5d      %5d   %s" % (n,ios[rep[n].dirn],
    rep[n].size,rep[n].offset,rep[n].numbytes,rep[n].shift,rep[n].min,rep[n].max,rep[n].desc))
    n = n+1
    
  return   

def autoconnect():
  global stopflag
      
  if(btfpy.Device_address(2) == "00:00:00:00:00:00"):
    searchflag = 1  # scan to search for unknown xbox
  else:
    searchflag = 0  # known xbox address set in devices[]
        
  repairnode = 0  # failed re-pair  
  pret = 0
  while(pret != 1):
    if(searchflag == 0):
      node = 2  # address set in devices
    elif(pret == 0):
      # scan to search
      node = 0
      while(node == 0):
        (count,node) = search()
      # Found 
            
    print("Connect to node %d" % node)
    btfpy.Set_le_wait(2000)
    ret = btfpy.Connect_node(node,btfpy.CHANNEL_LE,0)
    if(ret == 0):
      # Connect failed
      pret = 0
    else: 
      # Connect OK
      btfpy.Set_le_wait(10000)
      pflag = btfpy.Device_paired(node)
      if(pflag == 0 or pret == 2 or node == repairnode):
        # New bond
        print("New bond - press pair button for rapid flash")
        pret = btfpy.Le_pair(node,btfpy.JUST_WORKS | btfpy.BOND_NEW | btfpy.IRKEY_ON,0)
      else:
        # Re-bond
        print("Re-pair")
        pret = btfpy.Le_pair(node,btfpy.BOND_REPAIR,0)  
        if(pret == 0):
          repairnode = node
          pret = 2
          # RePair failed - loop for new bond
      if(pret != 1):
        btfpy.Disconnect_node(node)
    if(pret != 1):
      btfpy.Sleep_ms(3000)
    # end pret != 1
  
    
  btfpy.Set_le_interval_update(node,6,12)
  btfpy.Mesh_off()
  btfpy.Find_ctics(node)  
  uuid = [0x2A,0x4D]
  repn = btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid) # index of Report1
  uuid = [0x2A,0x4B]
  mapn = btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid) # index of Report Map
  if(repn < 0 or mapn < 0):
    print("Bad connection/pairing or not an Xbox - may need a new pairing")
  else:
    # connection/pair OK
    if(searchflag != 0):
      print("Set Xbox address in xbox.txt file")
      print("   ADDRESS = %s" % btfpy.Device_address(node))
      print("This will allow easy re-connection next time")
     
    rep1[0].cticn = repn
    rep2[0].cticn = repn+1
    print("Connected OK")  
    print("Only programmed with one action = Xbox button to exit")
    btfpy.Notify_ctic(node,repn,btfpy.NOTIFY_ENABLE,mycode_callback)
    stopflag = 0
    while(stopflag == 0 and btfpy.Device_connected(node) != 0):
      btfpy.Read_notify(10)
    btfpy.Notify_ctic(node,repn,btfpy.NOTIFY_DISABLE,0)
    btfpy.Disconnect_node(node)
    print("Xbox button pressed")
  return(node)
  

######## START #######

rep1 = []
rep1.append(itemdata(1,0,1,0,16,0,0,0,0,0,0," "))
rep1.append(itemdata(1,0,0,0,2,16,0,65535,0,65535,0,"L stick X"))
rep1.append(itemdata(1,0,0,2,2,16,0,65535,0,65535,0,"L stick Y"))
rep1.append(itemdata(1,0,0,4,2,16,0,65535,0,65535,0,"R stick X"))
rep1.append(itemdata(1,0,0,6,2,16,0,65535,0,65535,0,"R stick Y"))
rep1.append(itemdata(1,0,0,8,2,10,0,1023,0,1023,0,"L trig"))
rep1.append(itemdata(1,0,0,10,2,10,0,1023,0,1023,0,"R trig"))
rep1.append(itemdata(1,0,0,12,1,4,0,15,1,8,0,"D pad"))
rep1.append(itemdata(1,0,0,13,1,1,0,1,0,1,0,"A but"))
rep1.append(itemdata(1,0,0,13,1,1,1,1,0,1,0,"B but"))
rep1.append(itemdata(1,0,0,13,1,1,2,1,0,1,0,"Unknown"))
rep1.append(itemdata(1,0,0,13,1,1,3,1,0,1,0,"X but"))
rep1.append(itemdata(1,0,0,13,1,1,4,1,0,1,0,"Y but"))
rep1.append(itemdata(1,0,0,13,1,1,5,1,0,1,0,"Unknown"))
rep1.append(itemdata(1,0,0,13,1,1,6,1,0,1,0,"L bumper"))
rep1.append(itemdata(1,0,0,13,1,1,7,1,0,1,0,"R bumper"))
rep1.append(itemdata(1,0,0,14,1,1,0,1,0,1,0,"Unknown"))
rep1.append(itemdata(1,0,0,14,1,1,1,1,0,1,0,"Unknown"))
rep1.append(itemdata(1,0,0,14,1,1,2,1,0,1,0,"View but"))
rep1.append(itemdata(1,0,0,14,1,1,3,1,0,1,0,"Menu but"))
rep1.append(itemdata(1,0,0,14,1,1,4,1,0,1,0,"Xbox but"))
rep1.append(itemdata(1,0,0,14,1,1,5,1,0,1,0,"L stk but"))
rep1.append(itemdata(1,0,0,14,1,1,6,1,0,1,0,"R stk but"))
rep1.append(itemdata(1,0,0,15,1,1,0,1,0,1,0,"Share but"))
rep1.append(itemdata(0,0,0,0,0,0,0,0,0,0,0," "))

rep2 = []
rep2.append(itemdata(3,0,2,1,8,0,0,0,0,0,0," "))
rep2.append(itemdata(1,0,1,0,1,4,0,15,0,1,0,"DC enable actuators"))
rep2.append(itemdata(1,0,1,1,1,8,0,255,0,100,0,"Unknown"))
rep2.append(itemdata(1,0,1,2,1,8,0,255,0,100,0,"Unknown"))
rep2.append(itemdata(1,0,1,3,1,8,0,255,0,100,0,"Unknown"))
rep2.append(itemdata(1,0,1,4,1,8,0,255,0,100,0,"Magnitude"))
rep2.append(itemdata(1,0,1,5,1,8,0,255,0,255,0,"Duration"))
rep2.append(itemdata(1,0,1,6,1,8,0,255,0,255,0,"Start delay"))
rep2.append(itemdata(1,0,1,7,1,8,0,255,0,255,0,"Unknown"))
rep2.append(itemdata(0,0,0,0,0,0,0,0,0,0,0," "))

replist = [rep1,rep2]

if(btfpy.Init_blue("xbox.txt") == 0):
  exit(0)

outdat = [0 for n in range(64)]

oknode = 0

  #********* AUTO CONNECT ************
  #  Uncomment the following two instructions
  #  to connect automatically and run mycode_callback
  #  with inputs sent to in mycode_handler
  #*************************************

#autoconnect()
#exit(0)

 #******* END AUTO CONNECT **********
   
   
while(1):
  ret = 0
  while(ret == 0):
    if(oknode != 0):
      print("  0 = Exit")
      print("  1 = Reconnect")
      flag = inputint("Option 0/1")
      if(flag <= 0):
        exit(0)
      newflag = 0
      node = oknode  
    elif(btfpy.Device_address(2) == "00:00:00:00:00:00"):
      print("\nXbox controller address not set in xbox.txt file")  
      print("  0 = Exit and edit xbox.txt to set known address")
      print("  1 = Scan to find Xbox waiting to pair - press")
      print("      Xbox pair button for 3 seconds for rapid flash")
      flag = inputint("Option 0/1")
      if(flag <= 0):
        exit(0)
      (count,node) = search() 
      if(count == 0):
        print("No devices found")
        exit(0)
      else:
        if(node != 0):
          print("Found Gamepad node = %d" % (node))
        node = inputint("Input node number")
      if(node < 0):
        exit(0)
      newflag = 1
      flag = 0
    else:
      print("\nXbox address = %s in xbox.txt file" % (btfpy.Device_address(2)))
      print("  0 = Exit")
      print("  1 = New pairing (press pair button for rapid flash)")
      print("  2 = Reconnect to previously paired device")
      flag = inputint("Option 0/1/2")
      if(flag <= 0):
        exit(0)
      newflag = 0
      node = 2
  
    btfpy.Set_le_wait(2000)         
    ret = btfpy.Connect_node(node,btfpy.CHANNEL_LE,0)
    if(ret == 0):
      print("Connect failed")
    # end while(ret==0}


    # connected
       
  btfpy.Set_le_wait(10000)
  if(flag == 1):
    pret = btfpy.Le_pair(node,btfpy.JUST_WORKS | btfpy.BOND_NEW | btfpy.IRKEY_ON,0)
  else:
    pret = btfpy.Le_pair(node,btfpy.BOND_REPAIR,0)
  
  if(pret == 0):
    print("Pairing failed - may need a new pairing")
  else:
    # paired 
    btfpy.Set_le_interval_update(node,6,12)
    btfpy.Mesh_off()
    btfpy.Find_ctics(node)  
    uuid = [0x2A,0x4D]
    repn = btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid) # index of Report1
    uuid = [0x2A,0x4B]
    mapn = btfpy.Find_ctic_index(node,btfpy.UUID_2,uuid) # index of Report Map
    if(repn < 0 or mapn < 0):
      print("Bad connection/pairing or not an Xbox - may need a new pairing")
    else:
      # connection/pair OK
      rep1[0].cticn = repn
      rep2[0].cticn = repn+1
      oknode = node
      flag = 0
      while(flag != 5):
        if(newflag == 1):
          print("\nEdit xbox.txt to set Xbox ADDRESS = %s" % (btfpy.Device_address(node)))
          print("This will allow easy re-connection next time")
          newflag = 2
           
        print("\nOPERATION")
        print("  0 = Print Report1")
        print("  1 = Print Report2")
        print("  2 = Monitor inputs and print incoming data")
        print("  3 = Run mycode")
        print("  4 = Rumble")
        print("  5 = Disconnect and exit")
        flag = inputint("Enter option")  
        if(flag == 0):
          printmap(0)
        elif(flag == 1):
          printmap(1) 
        elif(flag == 2):
          print("**** MONITOR **** Operate controls - Xbox button to exit")
          btfpy.Notify_ctic(node,repn,btfpy.NOTIFY_ENABLE,monitor_callback)
          stopflag = 0
          while(stopflag == 0 and btfpy.Device_connected(node) != 0):
            btfpy.Read_notify(10)
          btfpy.Notify_ctic(node,repn,btfpy.NOTIFY_DISABLE,0)
          print("Xbox button pressed")
        elif(flag == 3):
          print("**** Mycode ****")
          print("Put your code in mycode_handle()")
          print("Only programmed with one action = Xbox button to exit")
          btfpy.Notify_ctic(node,repn,btfpy.NOTIFY_ENABLE,mycode_callback)
          stopflag = 0
          while(stopflag == 0 and btfpy.Device_connected(node) != 0):
            btfpy.Read_notify(10)
          btfpy.Notify_ctic(node,repn,btfpy.NOTIFY_DISABLE,0)
          print("Xbox button pressed")
        elif(flag == 4):
          printmap(1)
          print("Above is the Report for outputs")
          print("This now assumes that the Rumble items are")
          print("1=Activate 5=Magnitude 6=Duration 7=Start delay")
          val = inputint("Magnitude")
          setoutput(1,1)
          setoutput(5,val)
          val = inputint("Duration")
          setoutput(6,val)
          val = inputint("Start delay")
          setoutput(7,val)
          sendoutput(node)
        #end while(flag != 5)
      # end len > 0
    # end pret
  btfpy.Disconnect_node(node)
  # end while(1)
  
