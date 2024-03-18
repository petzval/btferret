#****** BLUEDOT server *********
# REQUIRES
#   btfpy.so     btfpy module
#   bluedot.txt  devices file 
# EDIT
#   bluedot.txt to list the local device running this code
#   and bluedot devices that may connect
#   and set their ADDRESS=
# RUN
#   sudo python3 bluedot.py
# *********************************************/

import btfpy

  
def bdotserver(clientnode,data,count):
  # btfpy.Print_data(data)   # Hex dump of s

  s = data.decode()
  k = 0
  while(s[k] != ','):
    k = k + 1
  op = int(s[0:k])
   
  if(op >= 0 and op <= 2):
    n = k + 1 
    k = n
    while(s[k] != ','):
      k = k + 1  
    col = int(s[n:k])

    n = k + 1
    k = n
    while(s[k] != ','):
      k = k + 1
    row = int(s[n:k])

    n = k + 1
    k = n
    while(s[k] != ','):
      k = k + 1
    x = float(s[n:k])
    y = float(s[k+1:])
    
    if(op == 0):
      print("Op= " + str(op) + " Release  Col=" + str(col) + " Row=" + str(row) + " x=" + str(x) + " y=" + str(y))
    elif(op == 1):
      print("Op= " + str(op) + " Press  Col=" + str(col) + " Row=" + str(row) + " x=" + str(x) + " y=" + str(y))
    elif(op == 2):
      print("Op= " + str(op) + " Move  Col=" + str(col) + " Row=" + str(row) + " x=" + str(x) + " y=" + str(y))

  elif(op == 3):
    print("Protocol " + s)
  else:
    print("Unknown operation")
    return(btfpy.SERVER_CONTINUE)
 
    
  if(op == 0):
     # button releae code here
     # col,row,x,y set
     pass
  elif(op == 1):
     # button press code here
     # col,row,x,y set
     pass 
  elif(op == 2):
    # button move code here
    # col,row,x,y set
    pass
  elif(op == 3):
    # initial connection code here
    # e.g. send config command to display two buttons
    cmd = "4,#0000ffff,0,0,1,1,2\n"
    btfpy.Write_node(clientnode,cmd,0)
    
    
    # e.g. send button config command to make bottom colour red
    cmd = "5,#ff0000ff,0,0,1,0,1\n"
    btfpy.Write_node(clientnode,cmd,0)
    pass
   
  return(btfpy.SERVER_CONTINUE)  # wait for next node packet



def help():
  print("Edit bluedot.txt to set the addresses of this device and")
  print("the Blue Dot device. There is an option to accept a connection")
  print("from any Blue Dot device. Find the address of the Android Blue Dot")
  print("device by turning Bluetooth on, then Settings/About/Status")
  print("If this device has been previously paired with the Blue Dot device")
  print("using other software, unpair it from the Blue Dot device first.")
  print("Pair by entering s here, then on the Blue Dot device:")
  print("  Settings/Bluetooth, tap this device. Tap OK within 10 seconds if")
  print("  asked to confirm a passkey. If asked for a PIN, enter 0000.")
  print("  Wait for Blue Dot device to pair and disconnect. This device")
  print("  will then wait for a connection from Blue Dot as below.")
  print("If already paired, enter s here. This device will wait for a") 
  print("connection from Blue Dot. Start Blue Dot and tap this device.")
  print("If re-connection requires a k press on the Pi to change the key")
  print("option, enter S instead of s here to start the server.")
  print("Tapping the bluedot buttons will trigger thc callback code here")
 

######### START ######
 
if(btfpy.Init_blue("bluedot.txt") == 0):
  exit(0)
 
flag = 0
while(flag == 0):
  print("Enter node number of Blue Dot device")
  print("Or 0 for any device (h=help q=exit)")
  s = input("? ")  
  if(s[0] == 'q'):
    print("Exit...")
    exit(0)    
  elif(s[0] == 'h'):
    help()
  else:
    nflag = 0
    for n in range(len(s)):
      if(ord(s[n]) < ord('0') or ord(s[n]) > ord('9')):
        nflag = 1
    if nflag == 0:
      node = int(s)
      flag = 1
      if(node == 0):
        print("You have chosen ANY DEVICE as the Blue Dot device")
        node = btfpy.ANY_DEVICE
      else:
        print("You have chosen " +  btfpy.Device_name(node) + " as the Blue Dot device")  
        if(btfpy.Device_type(node) != btfpy.BTYPE_CL):
          flag = 0 
          print("Error - Unknown or not a classic device")
    else:
      print("Not a number")
       
flag = 0
while(flag == 0): 
  print("s or S - Server. Wait for Blue Dot device to pair or connect")
  print("h - Help")
  print("q - Exit")
  print("Enter one of the above commands")
  s = input("? ")
  if(s[0] == 's'): 
    btfpy.Classic_server(node,bdotserver,10,btfpy.KEY_ON | btfpy.PASSKEY_LOCAL)
  elif(s[0] == 'S'):
    btfpy.Classic_server(node,bdotserver,10,btfpy.KEY_OFF | btfpy.PASSKEY_LOCAL)
  elif(s[0] == 'h'):
    help()       
  elif(s[0] == 'q'):
    flag = 1
    
print("Exit")
  
btfpy.Close_all()
