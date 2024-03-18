import btfpy

def callback(node,data,len):
  print("Received: " + data.decode())
  if data[0] == ord("H"):
    btfpy.Write_node(node,"Hello world\n",0)
    print("Disconnecting...")
    return(btfpy.SERVER_EXIT)
  else:
    btfpy.Write_node(node,"Send Hello to exit\n",0)
    return(btfpy.SERVER_CONTINUE)
  
if btfpy.Init_blue("devices.txt") == 0:
  exit(0)

print("IF FAILS - Experiment with security = 0/1/2/3")
security = 3

keyflag = btfpy.KEY_ON | btfpy.PASSKEY_LOCAL
if security == 1:
  keyflag = btfpy.KEY_OFF | btfpy.PASSKEY_LOCAL
elif security == 2:
  keyflag = btfpy.KEY_OFF | btfpy.PASSKEY_OFF
elif security == 3:
  keyflag = btfpy.KEY_ON | btfpy.PASSKEY_OFF    

btfpy.Classic_server(btfpy.ANY_DEVICE,callback,10,keyflag)
btfpy.Close_all()
  
