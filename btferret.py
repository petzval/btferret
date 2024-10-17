#!/usr/bin/python3

###### VERSION 19 ######
### btfpy.so must be built with btlib.c/btlib.h/btfpython.c version 19 or later via:###
###
### apt-get install python3-setuptools
### apt-get install python3-dev
### python3 btfpy.py build
###
### btfpy.py code:
###   import os
###   from setuptools import setup, Extension
###   print("Removing any existing module")
###   os.system('rm build/lib*/btfpy*.so')
###   module1 = Extension(name='btfpy',sources=['btlib.c'],extra_compile_args=['-D BTFPYTHON'])
###   ret = setup(name = 'BtfpyPackage',
###               version = '1.0',
###               description = 'Bluetooth interface',
###               ext_modules = [module1])
###   print("Copying module to btfpy.so")
###   os.system('cp build/lib*/btfpy*.so btfpy.so')

import btfpy
import os
     
endchar = 10  # termination character (10 = line feed)
lesecurity = 2
crctable = []
gdestdir = ""
gnblock = 400
count = 0
connected_node = 0
file_length = 0
file = None

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


def inputnode(mask,meshflag):
  count = 0  
  flag = 0
  if((mask & btfpy.BTYPE_CL) != 0):
    print("CLASSIC servers",end = '')
    flag = 1
  
  if((mask & btfpy.BTYPE_LE) != 0):
    print("  LE servers",end = '')
    flag = 1
 
  if((mask & btfpy.BTYPE_ME) != 0):
    print("  NODE servers",end = '')
  print()      
  if((mask & btfpy.BTYPE_CONNECTED) != 0):
    print("Connected only")
  if((mask & btfpy.BTYPE_DISCONNECTED) != 0):
    print("Disconnected only")
   
  n = btfpy.Device_info(mask | btfpy.BTYPE_SHORT)
  if(n == 0):
    print("None")
    
  count = count + n
       
  if(meshflag == 1):
    print(" 0 - All mesh servers (not connected node servers)")
  elif(meshflag == 2):
    print(" 0 - Any device")
  elif(count == 0):
    return(-1)  
                
  flag = 0
  while flag == 0:
    flag = 0
    node = inputint("Input node")
    if(node < 0):
      return(-1)  # cancel
    if(meshflag != 0 and node == 0):
      flag = 1
    elif((btfpy.Device_type(node) and mask) != 0):
      flag = 1
    else:
      print("Invalid node")
    
  return(node)
  
def inputchan(node):
    
  chan = 0
   
  print("  0 = Use device file CHANNEL= number or Reconnect")
  print("  1 = Input channel number")
  print("  2 = Read services to choose channel")
  flag = 1
  while(flag != 0):
    flag = 0
    n = inputint("Input option 0/1/2")
    if(n < 0):
      return(-1)
    if(n > 2):
      flag = 1
      
  if(n == 0):
    return(chan)
  
  if(n == 2):
    # read server's SDP database to find available channels
    if(btfpy.List_channels(node,btfpy.LIST_SHORT) <= 0):     
      print("Failed to find RFCOMM channels")
      return(-1)
    
  chan = inputint("Input RFCOMM channel number")
  if(chan < 0):
    return(-1)
  return(chan)


def sendstring(node,coms):
  global endchar
  # send string coms 
  clen = len(coms) 
  if(clen > 999):
    print("String too long")
    return
     
  if(clen == 0):
    print("Send endchar only to " + btfpy.Device_name(node))
  else:    
    print("Send to " + btfpy.Device_name(node) + " = " + coms)

  dat = coms + chr(endchar) 
  retval = btfpy.Write_node(node,dat,0)    # 0 = send all dat
   
  # return value is number of bytes written     

  return(retval)


def printifascii(buf,newline):
  for n in range(len(buf)):
    if((buf[n] < 32 or buf[n] > 126) and buf[n] != 0 and buf[n] != 10 and buf[n] != 13):
      if newline != 0:
        print()
      return(0)  # non-ascii char
  xbuf = buf.rstrip(b'\r\n')
  print("\"" + xbuf.decode() + "\"",end = '')
  if newline != 0:
    print()
  return(1)

def printhex(buf,newline):
  for n in range(len(buf)):
    print('{0:02X}'.format(buf[n]),end = ' ')
  if newline != 0: 
    print()  
  return


def clientread(node):
  global endchar
  
  buf = btfpy.Read_node_endchar(node,endchar,btfpy.EXIT_TIMEOUT,3000)
  if(len(buf) != 0):
    print("Reply from " + btfpy.Device_name(node) + " =",end = ' ')   
    if printifascii(buf,1) == 0:
      btfpy.Print_data(buf)
  else:
    print("No reply from " + btfpy.Device_name(node))
  return


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


def send_file(node,filename,destdirx,nblockx):
  # convert any str to bytes
  
  if isinstance(filename,str) == True:
    filename_by = filename.encode()
  else:
    filename_by = filename
    
  if isinstance(destdirx,str) == True:
    destdir_by = destdirx.encode()
  else:
    destdir_by = destdirx
    
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
    dfile = destdir + filename   # no directory
      
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

  btfpy.Read_node_clear(node)
   
  print("Sending "+filename.decode()+" to "+dfile.decode()+" Length="+str(flen)+" Block size="+str(nblock))
  
  # Send file command
  buf = "F".encode() + dfile + b'\n'
  btfpy.Write_node(node,buf,0)   # 0 = send all buf = len(buf)
   
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
  
  btfpy.Write_node(node,info,0)  # 0 = send len(info) 
  
  buf = btfpy.Read_node_count(node,1,btfpy.EXIT_TIMEOUT,3000)
  if len(buf) == 0 or buf[0] != 10:
    print("Not seen ack")
    f.close()
    return
    
  # start sending data      
  progflag = 0  # no print progress
  if(flen > 5000):
    # every 10 packets 
    progflag = 1
    print("Progress",end = '',flush=True)   
    
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

      buf = btfpy.Read_node_count(node,1,btfpy.EXIT_TIMEOUT,3000)
      if len(buf) == 0 or buf[0] != 10:
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
              
        buf = temps + bytes(tempsx)
        nsent = btfpy.Write_node(node,buf,0)  # 0  = send len(buf)
        if nsent != len(buf):
          getout = 1

        ackflag = 1 
        packn = packn + 1
        if(progflag != 0 and (packn % 10) == 0):
          print(".",end = '',flush=True) 
     
        # end getout == 0      
      # end ack
    # end block loop
    
  f.close()
    
  if progflag != 0:
    print()
    
  if(getout != 0):
    # error may have left data in buffer
    btfpy.Read_node_count(node,1024,btfpy.EXIT_TIMEOUT,5000)
    if(getout == 1):
      print("Timed out")
    else:
      print("File read error")   
  else:
    print("CRC=" + '{0:04X}'.format((crchi << 8)+crclo))
    # expect reply
    reply = btfpy.Read_node_endchar(node,10,btfpy.EXIT_TIMEOUT,5000)  
    print("Reply = " + reply.decode())      

  return
  # end send_file
  
######### SEND FILE OBEX #######
  
def sendfileobex(node,filname):
  connect = [0x80,0x00,0x07,0x10,0x00,0x01,0x90]
  disconnect = [0x81,0x00,0x03]   

  # convert to bytes object
  if isinstance(filname,str) == True:
    filename = filname.encode()
  else:
    filename = filname

  # strip this machine directory from filename 
  sgs = filename.split(b'/')   # linux directory
  if(len(sgs) > 1):
    fname = sgs[len(sgs)-1]      
  else:
    fname = filename
  
  print("Sending " + filename.decode() + " to " + fname.decode())

  # open file
  try:
    file = open(filename,'rb')
  except:
    print("File open error")
    return(0) 

  # find file length
  file.seek(0,os.SEEK_END)
  flen = file.tell()
  file.seek(0)
   
  ntogo = flen 
  nlen = len(fname)

  nblock = 400
  connect[5] = (nblock >> 8) & 0xFF
  connect[6] = nblock & 0xFF
  send = [0 for n in range(nblock)] 

  # OBEX connect
  btfpy.Write_node(node,connect,0)
  
  # wait for Success reply 0x0A
  inbuf = btfpy.Read_node_endchar(node,btfpy.PACKET_ENDCHAR,btfpy.EXIT_TIMEOUT,5000)
  if(len(inbuf) == 0 or inbuf[0] != 0xA0):
    print("OBEX Connect failed")
    file.close()
    return(0) 
  elif((inbuf[1] << 8) + inbuf[2] >= 7):
    n = (inbuf[5] << 8) + inbuf[6]
    if(n < nblock):
      nblock = n  # reduce chunk size
    
  send[3] = 0x01
  n = 2*nlen + 5
  send[4] = (n >> 8) & 0xFF
  send[5] = n & 0xFF
  k = 6
  for n in range(nlen):
    send[k] = 0
    send[k+1] = fname[n]
    k = k + 2
     
  send[k] = 0
  send[k+1] = 0
  k = k + 2

  send[k] = 0xC3
  send[k+1] = (flen >> 24) & 0xFF
  send[k+2] = (flen >> 16) & 0xFF
  send[k+3] = (flen >> 8) & 0xFF
  send[k+4] = flen & 0xFF
  k = k + 5
  err = 0
  # loop to send data chunks
  while(ntogo > 0 and err == 0):  
    if(ntogo <= nblock - 3 - k):
      send[k] = 0x49
      send[0] = 0x82
      ndat = ntogo + 3
    else:
      send[k] = 0x48
      send[0] = 0x02
      ndat = nblock - k
    send[k+1] = (ndat >> 8) & 0xFF
    send[k+2] = ndat & 0xFF
    k = k + 3
    ndat = ndat - 3

    try:
      temps = file.read(ndat)
    except:
      err = 1

    if err == 0:
      for n in range(ndat):
        send[k+n] = temps[n]
      ntogo = ntogo - ndat
      k = k + ndat
      send[1] = (k >> 8) & 0xFF
      send[2] = k & 0xFF
      btfpy.Write_node(node,send,k)  # send k bytes
      # wait for Success reply 0x0A
      inbuf = btfpy.Read_node_endchar(node,btfpy.PACKET_ENDCHAR,btfpy.EXIT_TIMEOUT,5000)
      if(len(inbuf) == 0 or (inbuf[0] != 0xA0 and inbuf[0] != 0x90)):
        print("Send failed")
        err = 1
   
    k = 3
    # end chunk loop

  file.close()
  
  btfpy.Write_node(node,disconnect,0)
  # wait for Success reply 0x0A
  inbuf = btfpy.Read_node_endchar(node,btfpy.PACKET_ENDCHAR,btfpy.EXIT_TIMEOUT,5000)
  if(len(inbuf) == 0 or inbuf[0] != 0xA0):
    print("OBEX Disconnect failed")
  
  return(1)
  # end sendfileobex    
  
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
  btfpy.Write_node(node,cmd,0)
  cmd = "Y".encode() + str(nblockx).encode() + b'\n'
  btfpy.Write_node(node,cmd,0)
  cmd = "G".encode() + filename_by + b'\n' 
  btfpy.Write_node(node,cmd,0)
  
  # wait for sending file command F + filename from node
  
  cmd = btfpy.Read_node_endchar(node,10,btfpy.EXIT_TIMEOUT,5000)
  
  # got cmd from connected device
  dlen = len(cmd)
  if dlen > 2 and cmd[0] == ord('F'):
    if(cmd[dlen-1] == endchar):
      temps = cmd[1:dlen-1]
    else:
      temps = cmd[1:] 
    receive_file(node,temps)
  else:
    print("Not seen sending file command from connected device")  
            
  return
   
  
############# RECEIVE FILE ##############  
# node = socket
# fname = file name to store on this device (bytes object)

def receive_file(node,fname):

  print("Receiving file " + fname.decode())
  
  # read 7 bytes of file info
  dat = btfpy.Read_node_count(node,7,btfpy.EXIT_TIMEOUT,3000)
  if len(dat) != 7:
    print("No file data")
    return(0)
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

  # ack    
  btfpy.Write_node(node,b'\n',1)
  
  crchi = 0
  crclo = 0  
  crc = 0xFFFF
  ntogo = flen+2
  getout = 0
  while (getout == 0 and ntogo > 0):
    if ntogo < nblock:
      nblock = ntogo
    dat = btfpy.Read_node_count(node,nblock,btfpy.EXIT_TIMEOUT,3000)
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
            
          nsent = btfpy.Write_node(node,b'\n',1)
          if nsent != 1:
            getout = 1

        # end getout = 0                        
      # end got nread dat
    # end getout loop  
  
  
  f.close()
  retval = 0
  if (getout != 0 or crc != 0):
    btfpy.Read_node_count(node,1024,btfpy.EXIT_TIMEOUT,1000)
    if getout == 1:
      replys = "Timed out"
    elif getout == 2:
      replys = "File write error"
    else:
      replys = "CRC error"
  else:
    replys = "Received OK. CRC=" + '{0:04X}'.format((crchi << 8)+crclo)
    retval = 1
    
  print("Sending " + replys)
  replys = replys + "\n"

  buf = replys.encode()
  btfpy.Write_node(node,buf,0)  # 0 = send len(buf) bytes
  return(retval)  
  # end receive_file


# Callback for Classic or Node servers

def classic_node_callback(clientnode,dat,datlen):
  global endchar
  global gdestdir
  global gnblock
   
  dlen = len(dat)
  if(dlen > 0):
    firstc = dat[0]  
  else:
    firstc = 0

  print("Received from " + btfpy.Device_name(clientnode) + " =",end = ' ')
  if printifascii(dat,1) == 0:
    btfpy.Print_data(dat)
      
  # check if received string is a known command 
  # and send reply to client
          
  if(firstc == endchar or firstc == ord('p')):  # ord() converts character to ascii integer
    print("Ping")
    sendstring(clientnode,"OK")  # OK reply
  elif(firstc == ord('D')):
    print("Disconnect")
    sendstring(clientnode,"Server disconnecting")
    return(btfpy.SERVER_EXIT) # stop server and initiate disconnection
  elif(firstc == ord('F')):
    # receive file      
    # command = Ffilename  endchar stripped
    if(dlen > 2):
      if(dat[dlen-1] == endchar):
        temps = dat[1:dlen-1]
      else:
        temps = dat[1:]
      receive_file(clientnode,temps)
    else:
      sendstring(clientnode,"SEND file - no filename")
  elif(firstc == ord('X')):
    # destination directory for get file
    if(dat[dlen-1] == endchar):
      gdestdir = dat[1:dlen-1]
    else:
      gdestdir = dat[1:] 
    print("Destination directory for GET file = " + gdestdir.decode())
  elif(firstc == ord('Y')):
    # nblock for get file
    if(dlen > 2):
      gnblock = 0
      n = 1
      while(n < dlen and dat[n] >= ord('0') and dat[n] <= ord('9')):
        gnblock = (gnblock*10) + (dat[n] - ord('0'))
        n = n + 1
    else:
      gnblock = 400
    print("Block size for GET file = " + str(gnblock))
  elif(firstc == ord('G')):
     # get file request with file name
    if(dlen > 2):
      if(dat[dlen-1] == endchar):
        temps = dat[1:dlen-1]
      else:
        temps = dat[1:] 
      print("GET file " + temps.decode())
      send_file(clientnode,temps,gdestdir,gnblock)
    else:
      sendstring(clientnode,"GET file - no filename")
  else:
    print("No action")
    sendstring(clientnode,"Unknown command - no action")    
        
  return(btfpy.SERVER_CONTINUE)  # loop for another packet
  # end node_classic_callback


# Callback for Universal servers

def universal_callback(clientnode,operation,cticn,dat,datlen):
  global endchar
  global gdestdir
  global gnblock
    
  name = btfpy.Device_name(clientnode)
  if(operation == btfpy.LE_CONNECT):
    print(name + " has connected")
  elif(operation == btfpy.LE_READ):
    print(name + " has read local characteristic " + btfpy.Ctic_name(btfpy.Localnode(),cticn))
  elif(operation == btfpy.LE_WRITE):
    # read local characteristic that client has just written
    dat = btfpy.Read_ctic(btfpy.Localnode(),cticn)
    print(name + " has written local characteristic " + btfpy.Ctic_name(btfpy.Localnode(),cticn))
    btfpy.Print_data(dat)    
  elif(operation == btfpy.LE_DISCONNECT):
    print(name + " has disconnected")
  elif(operation == btfpy.CLASSIC_DATA):
    dlen = len(dat)
    if(dlen > 0):
      firstc = dat[0]  
    else:
      firstc = 0

    print("Received from " + name + " =",end = ' ')
    if printifascii(dat,1) == 0:
      btfpy.Print_data(dat)
      
    # check if received string is a known command 
    # and send reply to client
          
    if(firstc == endchar or firstc == ord('p')):  # ord() converts character to ascii integer
      print("Ping")
      sendstring(clientnode,"OK")  # OK reply
    elif(firstc == ord('D')):
      print("Disconnect")
      sendstring(clientnode,"Server disconnecting")
      btfpy.Disconnect_node(clientnode); # disconnect
    elif(firstc == ord('F')):
      # receive file      
      # command = Ffilename  endchar stripped
      if(dlen > 2):
        if(dat[dlen-1] == endchar):
          temps = dat[1:dlen-1]
        else:
          temps = dat[1:]
        receive_file(clientnode,temps)
      else:
        sendstring(clientnode,"SEND file - no filename")
    elif(firstc == ord('X')):
      # destination directory for get file
      if(dat[dlen-1] == endchar):
        gdestdir = dat[1:dlen-1]
      else:
        gdestdir = dat[1:] 
      print("Destination directory for GET file = " + gdestdir.decode())
    elif(firstc == ord('Y')):
      # nblock for get file
      if(dlen > 2):
        gnblock = 0
        n = 1
        while(n < dlen and dat[n] >= ord('0') and dat[n] <= ord('9')):
          gnblock = (gnblock*10) + (dat[n] - ord('0'))
          n = n + 1
      else:
        gnblock = 400
      print("Block size for GET file = " + str(gnblock))
    elif(firstc == ord('G')):
       # get file request with file name
      if(dlen > 2):
        if(dat[dlen-1] == endchar):
          temps = dat[1:dlen-1]
        else:
          temps = dat[1:] 
        print("GET file " + temps.decode())
        send_file(clientnode,temps,gdestdir,gnblock)
      else:
        sendstring(clientnode,"GET file - no filename")
    else:
      print("No action")
      sendstring(clientnode,"Unknown command - no action")    
    # end CLASSIC_DATA
  elif(operation == btfpy.SERVER_TIMER):
    print("Timer")
    # end TIMER    
  return(btfpy.SERVER_CONTINUE)  # loop for another packet
  # end universal_callback




def obex_callback(node,data,plen):
  global count
  global connected_node
  global file_length
  global file
  
  connect_success = [0xA0,0x00,0x07,0x10,0x00,0x01,0x90]
  fail = [0xC3,0x00,0x03]
  success = [0xA0,0x00,0x03]
  continue_reply = [0x90,0x00,0x03]
  
  datalen = 0 # no data
  
  # data[0] = opcode (section 3.4 in OBEX15.pdf)
  
  if(data[0] == 0x80):
    # Connect 
    if(connected_node == 0):
      # not connected to another device
      print("OBEX connect")
      btfpy.Write_node(node,connect_success,0)    
      connected_node = node
      count = 0  # data bytes received  
      if(file != None):
        file.close()
        file = None
    else:
      # already connected to another device - refuse
      print("Node " + str(node) + " trying to OBEX connect - refuse")
      btfpy.Write_node(node,fail,0)
    return(btfpy.SERVER_CONTINUE)
 
  if(node != connected_node):
    print("Node " + str(node) + " not OBEX connected")
    return(btfpy.SERVER_CONTINUE)
     
  
  if((data[0] & 0x7F) == 0x02):
    # 0x02 or 0x82 Put
    length = (data[1] << 8) + data[2] # should = plen
    n = 3  # 1st header item
    while(n < length and n < plen):
      hi = data[n] # header item identifier (section 2.1 in OBEX15.pdf)
      if((hi & 0xC0) == 0x80):
        # 1-byte value
        if(hi == 0x97):
          # Single Response Mode
          print("SRM not programmed")
          btfpy.Write_node(node,fail,0)
          connected_node = 0
          return(btfpy.SERVER_CONTINUE)  
        # other header identifiers here 0x93 0x94... 
        n = n + 2  # next item
      elif((hi & 0xC0) == 0xC0):
        # 4-byte value
        if(hi == 0xC3):
          # Count       
          file_length = (data[n+1] << 24) + (data[n+2] << 16) + (data[n+3] << 8) + data[n+4]
          print("File length = " + str(file_length))
        # other header identifiers here 0xC4 0xCB... 
        n = n + 5 # next item
      else:
        # 2 length bytes 
        ilen = (data[n+1] << 8) + data[n+2] # item length 
        j = n+3 # start data  
        if(ilen == 0):
          n = length  # error exit loop
        if(hi == 0x01):
          # unicode file name
          j = j + 1 # skip unicode 0
          k = 0
          filename = ""
          while(j < n+ilen and j < plen):
            if(data[j] != 0): 
              filename = filename + chr(data[j])
            k = k + 1
            j = j + 2
          print("File name = " + filename)
          
          # open file
          try:
            file = open(filename,'wb')
          except:
            file = None
            print("File open error")
            return(btfpy.SERVER_CONTINUE) 
          
        elif(hi == 0x48 or hi == 0x49):
          # data chunk
          datan = j         # index 
          datalen = ilen-3  # length
                 
        # Other header identifiers here
        # 0x42 = Type
        # 0x47 = HTTP
        # 0x44 = Time
        
        n = n + ilen  # next item     
         
  elif(data[0] == 0x81):
    # Disconnect
    print("OBEX disconnect")
    connected_node = 0
    if(file != None):
      file.close()
      file = None
     
    btfpy.Write_node(node,success,0)  
    return(btfpy.SERVER_CONTINUE)
  else:
    print("GOT opcode " + str(data[0]) + " - no action")
 
  # Write data chunk to file
  if(datalen != 0 and file != None):
    count = count + datalen 
    file.write(data[datan:datan+datalen]) 
    if(data[0] == 0x82):
      # last chunk - finished
      if(count != file_length):
        print("Expected " + str(file_length) + " bytes. Got " + str(count))
      file.close()
      file = None
      print("File saved") 
  
  # Send response Continue or Success (section 3.2.1 in OBEX15.pdf)
  if(data[0] == 0x02):  # Put - not last chunk 
    btfpy.Write_node(node,continue_reply,0)  
  else:      
    btfpy.Write_node(node,success,0) 

  return(btfpy.SERVER_CONTINUE)
  # end obex_callback  

def mesh_callback(clientnode,dat,datlen):

  print("Mesh packet from " + btfpy.Device_name(clientnode))
  btfpy.Print_data(dat)
   
  if dat[0] == ord('D'):  # ord() converts character to ascii integer
    print("Disconnect")
    return(btfpy.SERVER_EXIT) # server exits
        
  return(btfpy.SERVER_CONTINUE)  # server loops for another packet


def le_callback(clientnode,operation,cticn):
  name = btfpy.Device_name(clientnode)
  if(operation == btfpy.LE_CONNECT):
    print(name + " has connected")
  elif(operation == btfpy.LE_READ):
    print(name + " has read local characteristic " + btfpy.Ctic_name(btfpy.Localnode(),cticn))
  elif(operation == btfpy.LE_WRITE):
    # read local characteristic that client has just written
    dat = btfpy.Read_ctic(btfpy.Localnode(),cticn)
    print(name + " has written local characteristic " + btfpy.Ctic_name(btfpy.Localnode(),cticn))
    btfpy.Print_data(dat)    
  elif(operation == btfpy.LE_DISCONNECT):
    print(name + " has disconnected")
    # uncomment next line to stop LE server when client disconnects
    # return(btfpy.SERVER_EXIT)
    # otherwise LE server will continue and wait for another connection
    # or opeeration from clients that are still connected
  elif(operation == btfpy.LE_TIMER):
    print("Timer")
  elif(operation == btfpy.LE_KEYPRESS):
    print("Key code = " + str(cticn))
        
  return(btfpy.SERVER_CONTINUE)
  # end le_callback
  
def notify_callback(node,cticn,dat,datlen):
  # LE device lenode has sent notification
  print("Notification from " + btfpy.Device_name(node))
  print("Characteristic = " + btfpy.Ctic_name(node,cticn))
  btfpy.Print_data(dat)
  return  
  
def printhelp():
  print("  HELP")
  print("  a Scan for Classic devices    i Print device info")
  print("  b Scan for LE/Mesh devices    k Settings")   
  print("  c Connect to a node           p Ping server")
  print("  t Send string to server       T Send string to mesh")
  print("  r Read LE characteristic      w Write LE characteristic")
  print("  d Disconnect                  D Tell server to disconnect")  
  print("  s Become a listening server   f File transfer (send or get)")
  print("  v Read node services          y Read specified UUID service")
  print("  o Save screen output to file  g Register custom serial UUID")  
  print("  j LE notify/indicate on/off   R Read LE notifications")                     
  print("  m Mesh transmit on            n Mesh transmit off")
  print("  u Clear input buffer         [] Scroll screen back/forward") 
  print("  l Read LE handles             q Quit") 
  return
   
def settings():
 
  valn = inputint("PRINT options\n  0 = None\n  1 = Normal\n  2 = Verbose - all HCI traffic\nInput one of the above options")
  if(valn >= 0):
    if(valn == 0):
      valn = btfpy.PRINT_NONE
    elif(valn == 1):
      valn = btfpy.PRINT_NORMAL  
    elif(valn == 2):
      valn = btfpy.PRINT_VERBOSE
    else:
      print("Invalid option")
  
    if(valn < 3):
      btfpy.Set_print_flag(valn)
  return
  # end settings
    
def sendgetfile():

  
  print("FILE TRANSFER\n  0 = SEND file\n  1 = GET file")
  print("  2 = SEND file to OBEX server (Windows/Android...)\n  3 = RECEIVE file from OBEX client")
  flag = inputint("Input one of the above options")

  if flag == 0 or flag == 2:
    print("SEND",end = ' ')
  elif flag == 1:
    print("GET",end = ' ')
  elif flag == 3:
    print("Start an OBEX server via s. Send the file from the remote device")
    return
  else:
    print("Invalid option")
    return
  
  if flag == 2:
    print("file - must be connected to an OBEX protocol server")
  else:
    print("file - must be connected to a btferret transfer protocol server")
   
  servernode = inputnode(btfpy.BTYPE_CONNECTED | btfpy.BTYPE_ME | btfpy.BTYPE_CL,0)       
  if(servernode < 0):
    print("Cancelled")
    return(0)
    
  maxblock = 1000        
  if(btfpy.Device_connected(servernode) == btfpy.NO_CONN):
    print("Not connected")
    return(0)  
  elif(btfpy.Device_type(servernode) == btfpy.BTYPE_CL):
    print("*** NOTE *** Server must be programmed like btferret")
  elif(btfpy.Device_connected(servernode) == btfpy.NODE_CONN):
    maxblock = 400  # node connect max block size 
  
  nblock = maxblock

  print("Enter file name e.g.  /home/pi/doc.txt  (x = cancel)") 
  fname = input("? ")
  
  if(len(fname) == 0 or (len(fname) == 1 and fname[0] == 'x')):
    print("Cancelled")
    return(0)
 
  if(flag != 2):
    print("Input destination directory - must end with / or \\")
    print("  e.g. /home/pi/ OR C:\\mydir\\   (/ = none  x = cancel)")
    ddir = input("? ")

    slen = len(ddir)
    if(slen > 0):
      ec = ddir[slen - 1]
      if(ec != '/' and ec != '\\'):
        print("Directory must end with / or \\")
        return    

    if(len(ddir) == 0 or (len(ddir) == 1 and ddir[0] == 'x')):
      print("Cancelled")
      return(0)
  
    if(len(ddir) == 0 or (len(ddir) == 1 and ddir[0] == '/')):
      print("None - will save to server's btferret directory")
      ddir = "" 
    
    print("BLOCK SIZE = " + str(nblock) + " bytes")
    print("  Data is transmitted in blocks of this size")
    print("  Enter x to keep, or enter new value 64-" + str(maxblock))
    xblock = inputint("Block size")
    if(xblock >= 64 and xblock < maxblock):
      nblock = xblock         
      print("Block size changed to " + str(nblock))
    else:
      print("Block size not changed")
    # end dir/block input for non-OBEX
    
  if flag == 0:
    send_file(servernode,fname,ddir,nblock)
  elif flag == 2:
    sendfileobex(servernode,fname)
  else:
    get_file(servernode,fname,ddir,nblock) 
      
  return

def clientsecurity():
 
  flaglook = [0,btfpy.JUST_WORKS,btfpy.PASSKEY_FIXED,btfpy.PASSKEY_RANDOM]
  devlook = [btfpy.PASSKEY_LOCAL,btfpy.PASSKEY_REMOTE]
  bondlook = [0,btfpy.BOND_NEW,btfpy.BOND_REPAIR] 
  
  flags = 0 
  pair = 0
  keydev = 0
 
  print("\nBONDING  (save pairing info)")
  print("  0 = Do not bond - OK for most servers")
  print("  1 = New bond")
  print("  2 = Re-pair with a previously bonded device")
  bond = inputint("Bond")
  if(bond < 0):
    return(-1,0)
  if(bond > 2):
    print("Invalid option")
    return(-1,0)
    
  flags = bondlook[bond]
  if(bond == 2):
    return(flags,0)
  
  print("\nPAIRING and SECURITY")
  print("  0 = Do not pair - OK for most servers")
  print("  1 = Pair - Just Works")
  print("  2 = Pair - Fixed Passkey")
  print("  3 = Pair - Random Passkey")
  pair = inputint("Input pair option")
  if(pair <  0):
     return(-1,0)
 
  if(pair > 3):
    pair = 0
    print("Invalid option")
    return(-1,0)
    
  if(bond == 1 and pair == 0):
    print("Must pair for New Bond")
    return(-1,0)

  if(pair == 0):
    return(0,0)
     
  if(pair == 2 or pair == 3):
    print("\nPASSKEY chosen by")
    print("  0 = This local device")
    print("  1 = Remote server")
    keydev = inputint("Enter 0/1")
    if(keydev < 0):
      return(-1,0)
    if(keydev > 1):
      print("Invalid option")
      return(-1,0)
        
  if(pair == 2):
    passkey = inputint("Fixed 6-digit passkey")        
    if(passkey < 0):
      return(-1,0)
    if(passkey > 999999):
      print("Too big")
      return(-1,0)
  else:
    passkey = 0
  
  flags = flags | flaglook[pair] | devlook[keydev] 
    
  return(flags,passkey)
   
       
def clientconnect():
  global lesecurity
    
  node = inputnode(btfpy.BTYPE_CL | btfpy.BTYPE_LE | btfpy.BTYPE_DISCONNECTED | btfpy.BTYPE_ME,0)

  if(node < 0):
    print("Cancelled")
    return(0)

  if(btfpy.Device_type(node) == btfpy.BTYPE_ME):
    print("0 = Node server")
    print("1 = Classic server")
    print("2 = LE server")
    contype = inputint("Input listening type of remote mesh device")
  else:
    contype = 0
   
  if(contype < 0):
    print("Cancelled")
    return(0)
   
  chan = 0
  method = 0
  pairflags = 0
  
  if(btfpy.Device_type(node) == btfpy.BTYPE_CL):
    # classic server needs method = CHANNEL_STORED
    # or method = CHANNEL_NEW and channel
    # input chan and set method
    
    chan = inputchan(node)
    if(chan < 0):
      print("Cancelled")
      return(0)
    if(chan == 0):
      method = btfpy.CHANNEL_STORED # in devices file or reconnect
    else:
      method = btfpy.CHANNEL_NEW   
       
  elif (btfpy.Device_type(node) == btfpy.BTYPE_ME):
    # mesh device can be listening as Classic, LE or node server  
    if(contype == 0):
      method = btfpy.CHANNEL_NODE # node server
    elif(contype == 1):
      # classic server
      chan = 1 # mesh classic listens on channel 1
      method = btfpy.CHANNEL_NEW
    elif(contype == 2):
      method = btfpy.CHANNEL_LE  # LE server
    else:
      print("Invalid listening type")
      return(0)
     
  elif(btfpy.Device_type(node) == btfpy.BTYPE_LE):
    method = btfpy.CHANNEL_LE
  else:
    print("Invalid device type")
    return(0)

  if(method == btfpy.CHANNEL_LE):

    if(lesecurity == 2):
      print("\nPAIRING and SECURITY")
      print("  Most LE servers do not need security")
      print("  so this option is not necessary")
      lesecurity = inputint("Enable security options 0=No 1=Yes")
      if(lesecurity < 0):
        return(0)
      if(lesecurity > 1):
        lesecurity = 1
  
    if(lesecurity == 1):
      (pairflags,passkey) = clientsecurity()      
      if(pairflags < 0):
        return(0)
    
    curval = btfpy.Set_le_wait(btfpy.READ_WAIT)
    print("\nCONNECTION COMPLETE TIME")
    print("  After connecting, some LE servers need more time to complete")
    print("  the process or they will disconnect. Zero may work, otherwise")
    print("  find the shortest time that will prevent disconnection.")
    print("  Current value= " + str(curval) + " (x=cancel to keep)  Default value=750")
    wait = inputint("Time in ms")
    if(wait < 0):
      return(0)
    if(wait >= 0):
      btfpy.Set_le_wait(wait)   
    if(pairflags != 0):
      print("\nPAIRING COMPLETE TIME")
      print("This might include the time to enter a passkey")
      print("or pairing may fail with a timeout")
      pairwait = inputint("Time in ms")
      if(pairwait < 0):
        return(0)
         
  retval = btfpy.Connect_node(node,method,chan)

  if(pairflags != 0):
    btfpy.Set_le_wait(pairwait)
    btfpy.Le_pair(node,pairflags,passkey)
    
  return(retval)    

                  
def clientsend(cmd):
  if(cmd == 'D'):
    flag = 1  # all mesh servers option
  else:
    flag = 0
  
  # only connected classic/mesh
  node = inputnode(btfpy.BTYPE_CL | btfpy.BTYPE_ME | btfpy.BTYPE_CONNECTED,flag) 
        
  if(node <= 0):
    print("Cancelled")
    return
  
    
  if(btfpy.Device_connected(node) == btfpy.NO_CONN):
    print("Not connected")
    return
     
  
  if(cmd == 'p' or cmd == 'D'):
    print("This command only works if connected to a btferret server")
                 
      
  if(cmd == 't'):
    print("Current termination character (endchar) = " + str(endchar))
    coms = input("Input string to send: ")
  elif(cmd == 'p'):
    coms = "" # empty string - sendstring will add endchar
    print("Ping server")   
  elif(cmd == 'D'):
    coms = 'D'
    print("Tell server to disconnect")
  else:
    print("Unknown command")
    return
         
  if(sendstring(node,coms) != 0):   
    # read reply from connected node - not mesh
    clientread(node)
             
  if((btfpy.Device_type(node) == btfpy.BTYPE_CL or btfpy.Device_type(node) == btfpy.BTYPE_ME) and cmd == 'D'): 
    btfpy.Wait_for_disconnect(node,5000)  # wait for classic server to initiate and complete disconnect
                                          # 5 sec time out
 
  return 
  
def meshsend():
  print("Broadcast a string to all mesh devices")
  coms = input("Input string (max 25 chars): ")
   
  if(len(coms) > 25):
    print("String too long")
    return
       
  btfpy.Write_mesh(coms,len(coms))
  return
     
def localdisconnect():

  print("Use D instead to disconnect btferret node and mesh servers")    
     # only connected devices
  node = inputnode(btfpy.BTYPE_CL | btfpy.BTYPE_LE | btfpy.BTYPE_ME | btfpy.BTYPE_CONNECTED,0)
  if(node < 0):
    print("Cancelled")
    return
      
  btfpy.Disconnect_node(node)
  return
     

def serversecurity():
  
  flags = 0 
  auth = 0
  key = 0
  
  print("\nPAIRING and SECURITY")
  print("  0 = Random Passkey or Just Works or None")
  print("  1 = Fixed Passkey")
  key = inputint("Input option")
  if(key <  0):
     return(-1,0)
               
  if(key == 1):
    passkey = inputint("Fixed 6-digit passkey")        
    if(passkey < 0):
      return(-1,0)
  else:
    passkey = 0

  print("\nAUTHENTICATION")
  print("  If authentication is enabled, the client must connect")
  print("  with passkey security or this server will not")
  print("  allow characteristic reads/writes")
  auth = inputint("Authentication 0=No 1=Yes")
  if(auth <  0):
     return(-1,0)

  print("\nPASSKEY SECURITY")
  print("  0 = No - Ask for Just Works")
  print("  1 = Yes - Accept Passkey")
  jwflag = inputint("Accept Passkey security if the client asks")
  if(jwflag < 0):
    return(-1,0)

  if(key == 1):
    flags = btfpy.PASSKEY_FIXED
   
  if(auth == 1):  
    flags = flags | btfpy.AUTHENTICATION_ON
     
  if(jwflag == 0):
    flags = flags | btfpy.JUST_WORKS

  return(flags,passkey)
    



def server():
  global endchar
  global lesecurity
  
  pairflags = 0
  passkey = 0
  
  print("  0 = Node server\n  1 = Classic server\n  2 = LE server")
  print("  3 = Universal server (Classic and LE)")
  print("  4 = Mesh server\n  5 = OBEX server (receive file from Windows/Android...)")
  serverflag = inputint("Input server type 0-5")
  if(serverflag < 0 or serverflag > 5):
    return(0)
  if(serverflag == 4):  # Mesh
    btfpy.Mesh_server(mesh_callback)
  elif(serverflag == 2 or serverflag == 3):  # LE
      
    if(lesecurity == 2):
      print("\nPAIRING and SECURITY")
      print("  Most LE servers do not need security")
      print("  so this option is not necessary")
      lesecurity = inputint("Enable security options 0=No 1=Yes")
      if(lesecurity < 0):
        return(0)
      if(lesecurity > 1):
        lesecurity = 1
  
    if(lesecurity == 1):
      (pairflags,passkey) = serversecurity()
      if(pairflags < 0):
        return(0)
    
    if(serverflag == 2):
      print("\nDEVICE IDENTITY")
      print("Some clients may fail to connect/pair if the Local address is used")
      print("Random will set up a new LE address and identity for this device")
      print("  0 = Local Bluetooth address")
      print("  1 = Random Bluetooth address")
      addr = inputint("Input 0/1")
      if(addr < 0):
        return(0)
    else:
      addr = 0
          
    print("\nInput TIMER interval in deci (0.1) seconds")
    print("   0 = No TIMER calls\n  10 = One second interval\n  50 = Five second interval etc...")
    timeds = inputint("Timer interval")
    if(timeds < 0):
      return(0)
      
    if(serverflag == 2):
      keyflag = inputint("\nSend key presses to KEYPRESS callback 0=No 1=Yes")
      if(keyflag < 0):
        return(0)
    else:
      keyflag = 0
      
    if(addr == 0):
      # Local address
      btfpy.Set_le_random_address([0,0,0,0,0,0])
    else:
      # Random address identity - choose 6-byte address (2 hi bits of 1st byte must be 1)
      btfpy.Set_le_random_address([0xD1,0x58,0xD3,0x24,0x32,0xA7])
    
    if(keyflag == 0):
      btfpy.Keys_to_callback(btfpy.KEY_OFF,0)
    else:
      btfpy.Keys_to_callback(btfpy.KEY_ON,0)
      
    btfpy.Le_pair(btfpy.Localnode(),pairflags,passkey)
    btfpy.Set_le_wait(5000)   # wait 5 seconds for connection/pairing
    if(serverflag == 2):
      btfpy.Le_server(le_callback,timeds)
  elif(serverflag == 0):  # node
    print("\nInput node of client that will connect")
    clinode = inputnode(btfpy.BTYPE_ME,0)  
    if(clinode < 0):
      print("Cancelled")
      return(0)
    btfpy.Node_server(clinode,classic_node_callback,endchar)
  elif(serverflag == 1 or serverflag == 5 or serverflag == 3):  # classic
    if(serverflag == 3):
      clinode = 0
    else:
      print("\nInput node of client that will connect")
      clinode = inputnode(btfpy.BTYPE_ME | btfpy.BTYPE_CL,2)        
   
    if(clinode != 0 and btfpy.Device_type(clinode) == btfpy.BTYPE_ME):
      keyflag = btfpy.KEY_OFF | btfpy.PASSKEY_OFF
    else:
      print("\nClassic security  (0,1,3 to pair or connect Android/Windows.. clients)")
      print("  0 = Use link key, print passkey here, remote may ask to confirm")
      print("  1 = No link key,  print passkey here (forces re-pair if pairing fails)")
      print("  2 = No keys  (connecting client is another mesh Pi)")
      print("  3 = Use link key, no passkey")
      print("  4 = Use link key, remote prints passkey, enter it here if asked")
      inkey = inputint("Client's security requirement")
   
      if(inkey < 0 or inkey > 4):
        print("Cancelled or invalid entry")
        return(0)            
      elif(inkey == 0):
        keyflag = btfpy.KEY_ON | btfpy.PASSKEY_LOCAL  # local prints passkey - confirm on client
      elif(inkey == 1):
        keyflag = btfpy.KEY_OFF | btfpy.PASSKEY_LOCAL
      elif(inkey == 2):
        keyflag = btfpy.KEY_OFF | btfpy.PASSKEY_OFF
      elif(inkey == 3):
        keyflag = btfpy.KEY_ON | btfpy.PASSKEY_OFF    
      elif(inkey == 4):
        keyflag = btfpy.KEY_ON | btfpy.PASSKEY_REMOTE # client prints passkey - enter on loca  
    
    if(clinode == 0):
      clinode = btfpy.ANY_DEVICE
    if(serverflag == 1):             
      print("Server will listen on channel 1 and any of the following UUIDs")
      print("  Standard serial 2-byte 1101")
      print("  Standard serial 16-byte")
      print("  Custom serial set via register serial")
      btfpy.Classic_server(clinode,classic_node_callback,endchar,keyflag)
    elif(serverflag == 5):
      print("Server will listen on channel 2 and UUID = 1105")
      print("You may need to pair this device first")
      btfpy.Classic_server(clinode,obex_callback,btfpy.PACKET_ENDCHAR,keyflag) 
            
  if(serverflag == 3):
    btfpy.Universal_server(universal_callback,endchar,keyflag,timeds)
  return(0)


   
def readservices():
  print("Read services")
  
  node = inputnode(btfpy.BTYPE_CL | btfpy.BTYPE_LE | btfpy.BTYPE_ME | btfpy.BTYPE_LO,0)
  if(node < 0):
    return

  if(btfpy.Device_type(node) != btfpy.BTYPE_CL):  # pure classic devices cannot have LE characteristics
    btfpy.Find_ctics(node)
  if(btfpy.Device_type(node) != btfpy.BTYPE_LE):  # pure LE devices cannot have classic serial channels
    btfpy.List_channels(node,btfpy.LIST_FULL)
  
  return


def readlehandles():
  print("Read LE handles")
  
  node = inputnode(btfpy.BTYPE_CONNECTED | btfpy.BTYPE_LE | btfpy.BTYPE_ME,0)
  if(node < 0):
    return
  btfpy.Le_handles(node,0)
  return

  
def readuuid():
  print("\nFind services that contain a specified UUID")
  print("  0 = List services")
  print("  1 = Find LE characteristic index")
  print("  2 = Find Classic RFCOMM channel") 
  op = inputint("Input 0/1/2")
  if(op < 0 or op > 2):
    print("Invalid entry")
    return
        
  node = inputnode(btfpy.BTYPE_CL | btfpy.BTYPE_LE | btfpy.BTYPE_ME,0)
  if(node < 0):
    return
  
  if(op == 0):    
    print("Input 2-byte UUID in hex e.g. 0100  (x = cancel)")
  else:
    print("Input 2 or 16-byte UUID in hex e.g. 0100  (x=cancel)")
    
  suuid = input("? ")
   
  if(len(suuid) == 0 or suuid[0] == 'x'):
    return  
     
  dat = btfpy.Strtohex(suuid)
  num = len(dat) 
      
  if(op == 0):
    if(num != 2):
      print("Not 2-byte")
      return
    # uuid must be 2-byte hi first     
    btfpy.List_uuid(node,dat)
    return
       
  if(num == 2):
    flag = btfpy.UUID_2
  elif(num == 16):
    flag = btfpy.UUID_16
  else:
    print("Not 2/16 byte")
    return
        
  if(op == 1):  
    ret = btfpy.Find_ctic_index(node,flag,dat)
    if(ret < 0):
       print("UUID not found")
    else:
      print("Characteristic index = " + str(ret))
  elif(op == 2):
    ret = btfpy.Find_channel(node,flag,dat)
    if(ret < 0):
      print("Failed to read services")
    elif(ret == 0):
      print("UUID not found")
    else:
      print("RFCOMM channel = " + str(ret))
  return     
  
     
def readle():

  print("Read an LE characteristic")
  # only connected LE devices
  node = inputnode(btfpy.BTYPE_LE | btfpy.BTYPE_ME | btfpy.BTYPE_CONNECTED | btfpy.BTYPE_LO,0)  
  if(node < 0):
    print("Cancelled")
    return
      
  if(btfpy.List_ctics(node,btfpy.LIST_SHORT | btfpy.CTIC_R) == 0):
    print("No readable characteristics. Read services to find")
    return
         
  cticn = inputint("Input ctic index")
  if(cticn < 0):
    return
       
  dat = btfpy.Read_ctic(node,cticn)
  print(btfpy.Device_name(node) + "  " + btfpy.Ctic_name(node,cticn) + " =")
  btfpy.Print_data(dat)
  return

  
def writele():
 
  print("Write an LE characteristic")
  
  node = inputnode(btfpy.BTYPE_LE | btfpy.BTYPE_ME | btfpy.BTYPE_CONNECTED | btfpy.BTYPE_LO,0)  
  if(node < 0):
    print("Cancelled")
    return
 
  
  if(btfpy.List_ctics(node,btfpy.LIST_SHORT | btfpy.CTIC_W) == 0):
    print("No writeable characteristics")
    return
         
  cticn = inputint("Input ctic index")
  if(cticn < 0):
    return   
   
  print("Input data bytes in hex e.g. 5A 43 01")
  buf = input("? ")
  if(len(buf) == 0):
    return
    
  val = btfpy.Strtohex(buf)

  if(len(val) > 0):
    btfpy.Write_ctic(node,cticn,val,len(val))
  else:
    print("No data")
  return
  
def notifyle():

  print("Enable/Disable LE characteristic notify/indicate") 
  
  # only connected LE devices
  node = inputnode(btfpy.BTYPE_LE | btfpy.BTYPE_ME | btfpy.BTYPE_CONNECTED,0) 
  if(node < 0):
    print("Cancelled")
    return
     
  if(btfpy.List_ctics(node,btfpy.LIST_SHORT | btfpy.CTIC_NOTIFY) == 0):
    print("No characteristics with notify/indicate permission")
    return
           
  cticn = inputint("Input ctic index")
  if(cticn < 0):
    return 

  if(btfpy.Ctic_ok(node,cticn) == 0):
    print("Invalid index")
    return
   
  flag = inputint("0=Disable 1=Enable")
  if(flag < 0):
    return
    
  if(flag == 1):
    flag = btfpy.NOTIFY_ENABLE
    s = " enabled"
  else:
    flag = btfpy.NOTIFY_DISABLE
    s = " disabled"
   
  if(btfpy.Notify_ctic(node,cticn,flag,notify_callback) == 0):
    s = " failed"
    
  print(btfpy.Device_name(node) + " " + btfpy.Ctic_name(node,cticn) + s)
  return           
     
def regserial():
  print("\nRegister a custom serial service")
  
  print("Input 16-byte UUID e.g. 0011-2233-44556677-8899AABBCCDDEEFF (x=cancel)")
  uuid = input("? ")
  
  if(len(uuid) == 0 or uuid[0] == 'x'):
    return

  print("Input service name (x=cancel)")
  name = input("? ") 

  if(len(name) == 0 or (len(name) == 1 and name[0] == 'x')):
    return
    
  btfpy.Register_serial(btfpy.Strtohex(uuid),name)
  print("Done")
  return
  
def readnotify():
  print("Read notifications (must be enabled via j)")
  tos = inputint("Time out in seconds")
  if tos < 0:
    return
 
  print("Reading notifications... (x=stop)")
  btfpy.Read_notify(tos*1000)
  print("Read notifications finished")   
  return 
 
#*************** START ****************************

if btfpy.Init_blue("devices.txt") == 0:
  s = "q"
else:
  s = "a"
  print("h = help")
  
while s[0] != 'q':
  flag = 0
  while flag == 0:
    s = input("> ")
    if(len(s) > 0):
      flag = 1

  if s[0] == 'h':
    printhelp() 
  elif s[0] == 'k':    
    settings()         
  elif s[0] == 'a':    
    btfpy.Classic_scan()    
  elif s[0] == 'b':    
    btfpy.Le_scan()    
  elif s[0] == 'i':    
    btfpy.Device_info(btfpy.BTYPE_CL | btfpy.BTYPE_LO | btfpy.BTYPE_LE | btfpy.BTYPE_ME)
  elif s[0] == 'f':    
    sendgetfile()
  elif s[0] == '[':    
    btfpy.Scroll_back()      
  elif s[0] == ']':    
    btfpy.Scroll_forward()
  elif s[0] == 'o':    
    btfpy.Output_file("btout.txt")
  elif s[0] == 'c':    
    clientconnect()               
  elif s[0] == 't' or s[0] == 'D' or s[0] == 'p':    
    clientsend(s[0]) 
  elif s[0] == 'T':    
    meshsend()   
  elif s[0] == 'd':    
    localdisconnect()
  elif s[0] == 's':    
    server()
  elif s[0] == 'v':    
    readservices()
  elif s[0] == 'y':    
    readuuid()     
  elif s[0] == 'r':    
    readle()
  elif s[0] == 'w':    
    writele()
  elif s[0] == 'j':    
    notifyle()           
  elif s[0] == 'g':    
    regserial()
  elif s[0] == 'R':    
    readnotify() 
  elif s[0] == 'u':
    print("Clear input buffer")
    btfpy.Read_all_clear()
  elif s[0] == 'm':    
    btfpy.Mesh_on()
    print("Mesh on")       
  elif s[0] == 'n':    
    btfpy.Mesh_off()
    print("Mesh off")
  elif s[0] == 'l':
    readlehandles()
  elif s[0] != 'q':      
    print("Unknown command")
  # end
     
btfpy.Close_all()

