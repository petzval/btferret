import btfpy

if btfpy.Init_blue("devices.txt") == 0:
  exit(0)
  
print("Node 4 must be TYPE=CLASSIC in devices.txt")  
  # find standard serial channel
channel = btfpy.Find_channel(4,btfpy.UUID_16,btfpy.Strtohex("00001101-0000-1000-8000-00805F9B34FB"))  
btfpy.Connect_node(4,btfpy.CHANNEL_NEW,channel)
print("Send Hello world to client (assumes line end character = Line feed)")
btfpy.Write_node(4,"Hello world\n",0)
btfpy.Disconnect_node(4)
    
btfpy.Close_all()
