import btfpy


def notify_callback(node,cticn,dat,datlen):
  print("Reply from HM10")
  btfpy.Print_data(dat)
  # end notify_callback

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
      elif s[0] == '[':
        btfpy.Scroll_back()
      elif s[0] == ']':
        btfpy.Scroll_forward()
      else:
        print("Not a number")
    else:
      flag = 1
    # end while flag != 0  
 
  return(val)
  # end inputint


##### START #####
  
if(btfpy.Init_blue("devices.txt") == 0):
  exit(0)
    
print("Scanning for HM10")
btfpy.Le_scan()

print("Enter [ ] to scroll device list")
node = inputint("Enter node of HM10 (should report UUID=FFE0)")
if(node < 0):
  exit(0)

if(btfpy.Connect_node(node,btfpy.CHANNEL_LE,0) == 0):
  print("Connect failed")
  exit(0)
  
  # read services
btfpy.Find_ctics(node)
  # find data characteristic UUID=FFE1
cticn = btfpy.Find_ctic_index(node,btfpy.UUID_2,[0xFF,0xE1])
if(cticn > 0):
  print("Found data characteristic FFE1 index")  
  print("Enabling notifications")
  btfpy.Notify_ctic(node,cticn,btfpy.NOTIFY_ENABLE,notify_callback)
  print("Sending Hello")
  btfpy.Write_ctic(node,cticn,"Hello\n",0)
  print("Waiting 30s for a reply from HM10")
  print("Send an ASCII string reply from the HM10")
  btfpy.Read_notify(30000)
  print("30s wait for reply timed out")
else:
  print("Data characteristic FFE1 not found")
   
print("Disconnecting HM10")
btfpy.Disconnect_node(node)
btfpy.Close_all()

