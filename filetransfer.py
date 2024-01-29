#!/usr/bin/python3

####### FILE TRANSFER client/server for btferret #########
# Call via:
#   python3 filetransfer.py
#
# On Linux, ensure device is visible by setting scan parameters first:
#   hciconfig hci0 piscan
#   python3 filetransfer.py
#
# This code acts as a client or server to connect with
# a remote device running btferret. The client side of the connection
# can then send a file to the server, or get a file from the server.
# It should work on any machine that is Python+Bluetooth capable
# Windows requires Python 3.9 or later.
# btferret should act as a classic server or classic client
# and must use 10 (line feed, \n) as the termination character (endchar)
# This code running on two machines will also exchange files
###############################

import socket
import time
import os


crctable = []


############ CALC CRC ###########

def calccrc(crc,dat,dlen):  
  global crctable
     
  if(len(crctable) == 0):
    # calc crctable[256] on first entry
    for j in range(256):
      cwd = (j << 8) & 0xFFFF
      for i in range(8):
        if((cwd & 0x8000) == 0):       
          cwd = (cwd << 1) & 0xFFFF
        else:
          cwd = ((cwd << 1) & 0xFFFF) ^ 0x1021
        # end i loop  
      crctable.append(cwd)
      # end j loop
    # end table calc
  
  cwd = crc 
  
  for j in range(dlen):  
    cwd = crctable[(dat[j] ^ ((cwd >> 8) & 0xFF)) & 0xFF] ^ ((cwd & 0xFF) << 8)
    cwd = cwd & 0xFFFF  
 
  return(cwd)   
  # end calccrc
  
  
########## SEND - STR and BYTES versions ############


def send_file(node,filename,destdir,nblockx):
  # convert any str to bytes
  
  if isinstance(filename,str) == True:
    filename_by = filename.encode()
  else:
    filename_by = filename
    
  if isinstance(destdir,str) == True:
    destdir_by = destdir.encode()
  else:
    destdir_by = destdir
    
  # send_file must have bytes objects
            
  send_file_by(node,filename_by,destdir_by,nblockx)
  return

######### SEND_FILE_BY #############
# node = socket
# filename = file name on this machine - must be bytes object
# destdir = destination directory on other machine - must be a bytes object and end with / or \
# nblockx = number of bytes in each block sent - integer max 400

def send_file_by(node,filename,destdir,nblockx):

  # strip this machine directory from filename and construct
  # dfile = full destination filename on other machine
  sgs = filename.split(b'/')   # try linux directory
  if(len(sgs) > 1):
    dfile = destdir + sgs[len(sgs)-1]      
  else:
    sgs = filename.split(b'\\')  # try Windows directory
    if(len(sgs) > 1):
      dfile = destdir + sgs[len(sgs)-1]
    else:
      dfile = filename   # no directory
      
  # check nblockx    
  if nblockx < 64 or nblockx > 400:
    nblock = 400
  else:
    nblock = nblockx
    
  # open file
  try:
    f = open(filename,'rb')
  except:
    print("File open error")
    return 
    
  # find file length
  f.seek(0,os.SEEK_END)
  flen = f.tell()
  f.seek(0)
 
  print("Sending "+filename.decode()+" to "+dfile.decode()+" Length="+str(flen)+" Block size="+str(nblock))
  
  # Send file command
  node.send("F".encode() + dfile + b'\n')
   
  # Send file info 7 bytes
  info = [flen & 0xFF]
  info.append((flen>>8) & 0xFF)
  info.append((flen>>16) & 0xFF)
  info.append((flen>>24) & 0xFF)
  info.append(nblock & 0xFF)
  info.append((nblock >> 8) & 0xFF)
  # calc checksum b6
  b6 = info[0]
  for n in range(1,6):
    b6 = (b6 + info[n]) & 0xFF
  info.append(b6)   
  node.send(bytes(info))
  
  # wait for ack
  try:
    buf = node.recv(1)
  except:
    print("Not seen ack")
    f.close()
    return
  
  if buf[0] != 10:
    print("Not seen ack")
    f.close()
    return
    
  # start sending data      
  progflag = 0  # no print progress
  if(flen > 5000):
    # every 10 packets 
    progflag = 1
    print("Progress",end = '')   
    
  crc = 0xFFFF
  ntogo = flen+2    
  getout = 0
  packn = 0  
  ackflag = 0
  nblk = nblock  

  # data sent in blocks of size nblk on each while loop
  while(getout == 0 and ntogo > 0):
    if(ntogo < nblock):
      nblk = ntogo  # last block size
      
    if(ackflag != 0):
      # just sent nblk       
      # wait for ack = 10 from receiving device
      # single char read + time out
      # wait for single ack byte 3s time out
      try:
        buf = node.recv(1)          
      except:
        print("Not seen ack")
        getout = 1
        
      if getout == 0:
        if buf[0] != 10:
          print("Not seen ack")
          getout = 1   
                           
    if(getout == 0):   
      if(ntogo <= 2):
        ncrc = ntogo
        ndat = 0
      else:
        ncrc = nblk - ntogo + 2
        if(ncrc < 0):
          ncrc = 0
        ndat = nblk - ncrc
        if(ndat < 0):
          ndat = 0     
     
      temps = b''
      if(ndat > 0):
        try:
          temps = f.read(ndat)
        except:
          getout = 2
        
        if(getout == 0):
          crc = calccrc(crc,temps,ndat)
          ntogo = ntogo - ndat
     
      if(getout == 0):
        tempsx = []
        if(ncrc == 2):
          tempsx.append((crc >> 8) & 255)
          crchi = tempsx[0]
          tempsx.append(crc & 255)
          crclo = tempsx[1]
          ntogo = ntogo - 2
        elif(ncrc == 1):
          if(ndat == 0):
            tempsx.append(crc & 255)
            crclo = tempsx[0]
          else:
            tempsx.append((crc >> 8) & 255)
            crchi = tempsx[0]
          ntogo = ntogo - 1
    
        # send nblk bytes + crc if end of data
       
        try:
          node.send(temps + bytes(tempsx))
        except:
          getout = 1
       
        ackflag = 1 
        packn = packn + 1
        if(progflag != 0 and (packn % 10) == 0):
          print(".",end = '') 
     
        # end getout == 0      
      # end ack
    # end block loop
    
  f.close()
    
  if progflag != 0:
    print()
    
  if(getout != 0):
    # error may have left data in buffer
    try:
      node.recv(1024)
    except:
      pass
    if(getout == 1):
      print("Timed out")
    else:
      print("File read error")   
  else:
    print("CRC=" + '{0:04X}'.format((crchi << 8)+crclo))
    # expect reply
    reply = b''
    dat = b''
    while len(dat) == 0:
      try:
        dat = node.recv(1)
      except:
        dat = b'\n'
      if(len(dat) == 1 and dat[0] != 10):
        reply = reply + dat      
        dat = b''
      # loop until endchar=10
    print("Reply = " + reply.decode())      

  return
  # end send_file
  
############ GET FILE ##################  

def get_file(node,filename,destdir,nblockx):
  # convert any str to bytes
  
  if isinstance(filename,str) == True:
    filename_by = filename.encode()
  else:
    filename_by = filename
    
  if isinstance(destdir,str) == True:
    destdir_by = destdir.encode()
  else:
    destdir_by = destdir

  cmd = "X".encode() + destdir_by + b'\n'
  node.send(cmd)
  cmd = "Y".encode() + str(nblockx).encode() + b'\n'
  node.send(cmd)
  cmd = "G".encode() + filename_by + b'\n' 
  node.send(cmd) 
  
  # wait for sending file command F + filename from node
  cmd = b''
  dat = b''
  while len(dat) == 0:
    try:
      dat = node.recv(1)
    except TimeoutError:
      print("Timed out")
      return
    except:
      return
    if (len(dat) == 1 and dat[0] != 10):
      cmd = cmd + dat      
      dat = b''
    # loop until endchar=10
      
  if len(cmd) == 0:
    cmd = b'\n' 
    
  # got cmd from connected device
  retval = 0
  if cmd[0] == ord('F') and len(cmd) > 1:
    if receive_file(node,cmd[1:]) != 0:
      retval = 1
  
  if retval == 0:
    print("Not seen sending file command from connected device")  
            
  return
   
  
############# RECEIVE FILE ##############  
# node = socket
# fname = file name to store on this device (bytes object)

def receive_file(node,fname):

  print("Receiving file " + fname.decode())
  
  # read 7 bytes of file info
  dat = node.recv(7)
  flen = dat[0] + (dat[1] << 8) + (dat[2] << 16) + (dat[3] << 24)
  # packet block size
  nblock = dat[4] + (dat[5] << 8)
  print("Length " + str(flen) + " Block size " + str(nblock))
    
  chksum = 0
  for n in range(6):
    chksum = (chksum + dat[n]) & 0xFF
  if(dat[6] != chksum):
    print("Checksum error - invalid file info")
    return(0)
         
  try:
    f = open(fname,'wb')
  except:
    print("File open error")
    return(0)
    
  node.send(b'\n')  # ack byte

  crchi = 0
  crclo = 0  
  crc = 0xFFFF
  ntogo = flen+2
  getout = 0
  while (getout == 0 and ntogo > 0):
    if ntogo < nblock:
      nblock = ntogo
    dat = node.recv(nblock)
    nread = len(dat)
    if nread != nblock:
      getout = 1
    else:
      if nread == 1:
        crchi = crclo
        crclo = dat[0]
      else:
        crchi = dat[nread-2]
        crclo = dat[nread-1]
        
      if(ntogo <= 2):
        ncrc = ntogo
        ndat = 0
      else:
        ncrc = nblock - ntogo + 2
        if(ncrc < 0):
          ncrc = 0
        ndat = nblock - ncrc
        if(ndat < 0):
          ndat = 0        
    
      if(ndat > 0):
        if ndat == len(dat):
          nwrit = f.write(dat)
        else:
          nwrit = f.write(dat[0:ndat])
        if nwrit != ndat:            
          getout = 2  # write error
        else:
          ntogo = ntogo - ndat
        
      if(getout == 0):
        ntogo = ntogo - ncrc   
        crc = calccrc(crc,dat,nblock)
               
        if(ntogo != 0):
          # ack send not last block
          if node.send(b'\n') != 1:
            getout = 1 # send error
        # end getout = 0                        
      # end got nread dat
    # end getout loop  
  
  
  f.close()
  retval = 0
  if (getout != 0 or crc != 0):
    node.recv(1024)  #clear input buffer
    if getout == 1:
      replys = "Timed out"
    elif getout == 2:
      replys = "File write error"
    else:
      replys = "CRC error"
  else:
    replys = "Received OK. CRC=" + '{0:04X}'.format((crchi << 8)+crclo)
    retval = 1
    
  print(replys)
  replys = replys + "\n"
  node.send(replys.encode())
  return(retval)  
  # end receive_file
  
  

############ CLIENT #######################

def client(node):

  flag = 0
  while flag == 0:
    print("Input address of remote device e.g. DC:A6:32:04:DB:56")
    node_address = input("? ")
 
    # For convenience, addresses can be hard coded here
    # In this example, entering 'a' for the address
    # sets it to the coded value 
    if len(node_address) == 1 and node_address[0] == 'a':
      node_address = "DC:A6:32:04:DB:56"
      flag = 1 
    elif (len(node_address) == 17 and node_address[2] == ':' and node_address[5] == ':' and
          node_address[8] == ':' and node_address[11] == ':' and node_address[14] == ':'):
      flag = 1
    else:
      print("Invalid address")
    # loop until valid address  
    
  # btferret connects on all RFCOMM channels, so any number will work
  rfcomm_channel = 16
  print("Trying to connect to " + node_address + " on channel " + str(rfcomm_channel) + "...")
  try:
    node.connect((node_address,rfcomm_channel))
  except:
    node.close()
    print("Failed. On Linux, both devices must be visible:")
    print("  hciconfig hci0 piscan")
    print("  python3 filetransfer.py")
    quit()
    
  ######### CONNECTED AS CLIENT ###########
  
  node.settimeout(3.0)    
  print("Connected OK") 

  inp = "a"
  while(inp[0] != 'x'):
    print("Input one of the following options")
    print("  s = Send file to btferret device")
    print("  g = Get file from btferret device")
    print("  p = Ping server")
    print("  x = Disconnect and exit")
    inp = input("? ")

    if inp[0] == 's' or inp[0] == 'g':
      print("Input filename")
      print("  e.g. C:\\mydir\\test.txt OR /home/pi/test.txt")
      filname = input("? ") 
      print("Input destination directory - must end with / or \\")
      print("  e.g. /home/pi/ OR C:\\mydir\\")
      ddir = input("? ")
      slen = len(ddir)
      flag = 0
      if(slen > 0):
        ec = ddir[slen - 1]
        if(ec != '/' and ec != '\\'):
          print("Directory must end with / or \\")
          flag = 1    
      if(slen == 1 and (ddir[0] == '/' or ddir[0] == '\\')):
        ddir = ""
      if flag == 0:  
        if inp[0] == 's':
          # send file from this machine to server
          # Block size = 400 bytes
          send_file(node,filname,ddir,400)   
        elif inp[0] == 'g': 
          # get file from server to this machine
          get_file(node,filname,ddir,400)


    if inp[0] == 'p':
      # send p for Ping 
      node.send(b'p\n')      
      # expect reply
      reply = b''
      dat = b''
      while len(dat) == 0:
        try:
          dat = node.recv(1)
        except:
          dat = b'\n'
        if(len(dat) == 1 and dat[0] != 10):
          reply = reply + dat      
          dat = b''
        # loop until endchar=10
      print("Reply = " + reply.decode()) 
     
    # end when input = x
    
  # send disconnect instruction
  node.send(b'D\n')
  print("Disconnecting...")
  return
  
################# SERVER ##########
  
def server(dd):

  rfcomm_channel = 16  # change this if another program is using channel 16
  try:
    dd.bind(("00:00:00:00:00:00",rfcomm_channel))
  except:
    print("No Bluetooth, or channel " + str(rfcomm_channel) + " in use")
    print("Turn Bluetooth off/on or edit code to use a different rfcomm_channel")
    dd.close()
    quit()
  dd.listen()
  print("Waiting for remote device to connect on RFCOMM channel " + str(rfcomm_channel) + " endchar=10/line feed")
  dd.settimeout(3.0)
  flag = 0
  while flag == 0:
    flag = 1
    try:
      (node,add) = dd.accept()
    except TimeoutError:
      flag = 0
    except OSError:
      flag = 0
    except:
      dd.close()
      quit()    
 
  ####### CONNECTED AS SERVER ######### 
    
  node.settimeout(3.0)
  print("Connected OK to " + str(add))
  print("Waiting for instructions (endchar=10/line feed)...")
  
  getdestdir = ""
  getnblock = 400  
  
  getout = 0
  while(getout == 0):
    cmd = b''
    dat = b''
    # get cmd from client ending with 10/line feed
    while len(dat) == 0:
      try:
        dat = node.recv(1)
      except TimeoutError:
        dat = b''
      except OSError:
        dat = b''
      except:
        node.close()
        dd.close()
        quit()
      if (len(dat) == 1 and dat[0] != 10):
        cmd = cmd + dat      
        dat = b''
      # loop until endchar=10
      
    if len(cmd) == 0:
      cmd = b'\n' 
    
    # got cmd from client  
    
    print("Got command " + cmd.decode()) 
    if cmd[0] == ord('D'):                 # disconnect
      node.send(b"Disconnecting\n")
      getout = 1
    elif (cmd[0] == ord('p') or cmd[0] == 10):  # ping
      print("Ping")
      node.send(b"OK\n")
    elif cmd[0] == ord('F'):      # client is sending file 
      receive_file(node,cmd[1:])
    elif cmd[0] == ord('X'):      # set destination directory
      getdestdir = cmd[1:]
      print("GET destination directory = " + getdestdir.decode())
    elif cmd[0] == ord('Y'):      # set block size
      k = 0
      flag = 0
      for n in range(1,len(cmd)):
        if cmd[n] >= ord('0') and cmd[n] <= ord('9'):
          k = (k*10) + (cmd[n] - ord('0'))
        else:
          flag = 1
      if flag == 0:
        getnblock = k 
        print("GET block size = " + str(getnblock))
    elif cmd[0] == ord('G'):      # client is requesting file send
      send_file_by(node,cmd[1:],getdestdir,getnblock)               
    else:
      print("Invalid command")
      node.send(b"Invalid command\n")
    
    # end getout loop  
  print("Disconnecting...")
  node.close()            
  return
  # end server
  
  
########## START ##############


try:
  fd = socket.socket(socket.AF_BLUETOOTH,socket.SOCK_STREAM,socket.BTPROTO_RFCOMM)
except:
  print("No Bluetooth")
  quit()
  
print("File transfer to/from a device running btferret")
print("Input s or c for this device")
print("  s = Server (for btferret to connect as a classic client)")
print("  c = Client (for btferret listening as a classic server)")
sorc = input("? ")
if sorc[0] == 's':
  server(fd)
elif sorc[0] == 'c':
  client(fd)

fd.close()   
print("Exit")  
 
  
         
