import btfpy

# OBEX server
# Receives files sent from Windows/Android... via OBEX protocol
# Receives OBEX NAME and DATA
# Saves DATA in file called NAME in the obex_server directory
#
# Start this server, then on the remote device:
#   Windows: Settings / Devices / Send or receive files via Bluetooth / Send files
#              select No Authentication, or pair this receiving device first
#  Android: Share / via Bluetooth

count = 0
connected_node = 0
file_length = 0
file = None

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
          return(btfpy.SERVER_EXIT)  
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
            return(btfpy.SERVER_EXIT) 
    
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
    return(btfpy.SERVER_EXIT)
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

##### START ######
  
if btfpy.Init_blue("devices.txt") == 0:
  exit(0)

print("IF FAILS - Experiment with security = 0/1/2/3")
security = 0

keyflag = btfpy.KEY_ON | btfpy.PASSKEY_LOCAL
if security == 1:
  keyflag = btfpy.KEY_OFF | btfpy.PASSKEY_LOCAL
elif security == 2:
  keyflag = btfpy.KEY_OFF | btfpy.PASSKEY_OFF
elif security == 3:
  keyflag = btfpy.KEY_ON | btfpy.PASSKEY_OFF    

print("Waiting to receive OBEX file. You may need to pair this device first")
btfpy.Classic_server(btfpy.ANY_DEVICE,obex_callback,btfpy.PACKET_ENDCHAR,keyflag)
btfpy.Close_all()
  
