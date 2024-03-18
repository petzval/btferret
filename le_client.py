import btfpy

if btfpy.Init_blue("devices.txt") == 0:
  exit(0)
  
print("Node 7 must be TYPE=LE in devices.txt")  
btfpy.Connect_node(7,btfpy.CHANNEL_LE,0)
btfpy.Find_ctics(7)  # Read services
   # Device name UUID = 2A00
index = btfpy.Find_ctic_index(7,btfpy.UUID_2,[0x2A,0x00])
   # Read device name 
name = btfpy.Read_ctic(7,index)
print("Device name = " + name.decode())
btfpy.Disconnect_node(7)
    
btfpy.Close_all()
