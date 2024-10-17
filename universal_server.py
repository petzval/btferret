import btfpy

def callback(clientnode,operation,cticn,data,datlen):

  if(operation == btfpy.LE_CONNECT):
    # clientnode has just connected
    print("LE Connected")
  elif(operation == btfpy.LE_READ):
    # clientnode has just read local characteristic index cticn
    pass
  elif(operation == btfpy.LE_WRITE):
    # clientnode has just written local characteristic index cticn
    pass
  elif(operation == btfpy.LE_DISCONNECT):
    # clientnode has just disconnected
    print("LE Disconnected")
    # return(btfpy.SERVER_EXIT)  # to exit server
  elif(operation == btfpy.SERVER_TIMER):
    # The server timer calls here every timerds deci-seconds 
    # clientnode,cticn,data,datlen are invalid
    # This is called by the server not a client    
    pass
  elif(operation == btfpy.CLASSIC_DATA):
    # clientnode has sent data, length datlen
    print("Classic data received: " + data.decode())
    btfpy.Write_node(clientnode,"Hello World\n",0)  # send Hello World reply
    
  return(btfpy.SERVER_CONTINUE)  

######## START #######

if btfpy.Init_blue("devices.txt") == 0:
  exit(0)

print()
print("The local device must be the first entry in devices.txt")
print("(My Pi) that defines the LE characteristics")  

  # Set My data LE characteristic (index 1) value  
btfpy.Write_ctic(btfpy.Localnode(),1,"Hello world",0)    

btfpy.Universal_server(callback,10,btfpy.KEY_ON | btfpy.PASSKEY_LOCAL,0)   # timerds=0
    
btfpy.Close_all()
