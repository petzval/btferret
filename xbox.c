#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btlib.h" 

struct itemdata
  {
  unsigned char flag;
  unsigned char cticn;    // characteristic index of report rep[0] only
  unsigned char dirn;     // 0=input 1=output
  unsigned char offset;   // into notifictaion data packet
  unsigned char numbytes; // rep[0]=bytes in report
  unsigned char size;     // number bits in numbytes at offset
  unsigned char shift;    // right shift data
  int bitmask;            // after shift
  int min;
  int max;
  unsigned int value;
  char *desc;
  };
  
struct itemdata rep1[25] = {
{1,0,1,0,16,0,0,0,0,0,0,""},
{1,0,0,0,2,16,0,65535,0,65535,0,"L stick X"},
{1,0,0,2,2,16,0,65535,0,65535,0,"L stick Y"},
{1,0,0,4,2,16,0,65535,0,65535,0,"R stick X"},
{1,0,0,6,2,16,0,65535,0,65535,0,"R stick Y"},
{1,0,0,8,2,10,0,1023,0,1023,0,"L trig"},
{1,0,0,10,2,10,0,1023,0,1023,0,"R trig"},
{1,0,0,12,1,4,0,15,1,8,0,"D pad"},
{1,0,0,13,1,1,0,1,0,1,0,"A but"},
{1,0,0,13,1,1,1,1,0,1,0,"B but"},
{1,0,0,13,1,1,2,1,0,1,0,"Unknown"},
{1,0,0,13,1,1,3,1,0,1,0,"X but"},
{1,0,0,13,1,1,4,1,0,1,0,"Y but"},
{1,0,0,13,1,1,5,1,0,1,0,"Unknown"},
{1,0,0,13,1,1,6,1,0,1,0,"L bumper"},
{1,0,0,13,1,1,7,1,0,1,0,"R bumper"},
{1,0,0,14,1,1,0,1,0,1,0,"Unknown"},
{1,0,0,14,1,1,1,1,0,1,0,"Unknown"},
{1,0,0,14,1,1,2,1,0,1,0,"View but"},
{1,0,0,14,1,1,3,1,0,1,0,"Menu but"},
{1,0,0,14,1,1,4,1,0,1,0,"Xbox but"},
{1,0,0,14,1,1,5,1,0,1,0,"L stk but"},
{1,0,0,14,1,1,6,1,0,1,0,"R stk but"},
{1,0,0,15,1,1,0,1,0,1,0,"Share but"},
{0,0,0,0,0,0,0,0,0,0,0,""}
};

struct itemdata rep2[10] = {
{3,0,2,1,8,0,0,0,0,0,0,""},
{1,0,1,0,1,4,0,15,0,1,0,"DC enable actuators"},
{1,0,1,1,1,8,0,255,0,100,0,"Unknown"},
{1,0,1,2,1,8,0,255,0,100,0,"Unknown"},
{1,0,1,3,1,8,0,255,0,100,0,"Unknown"},
{1,0,1,4,1,8,0,255,0,100,0,"Magnitude"},
{1,0,1,5,1,8,0,255,0,255,0,"Duration"},
{1,0,1,6,1,8,0,255,0,255,0,"Start delay"},
{1,0,1,7,1,8,0,255,0,255,0,"Unknown"},
{0,0,0,0,0,0,0,0,0,0,0,""}
};

struct itemdata *replist[2] = { rep1,rep2 };

int readnot(unsigned char *dat,int datlen);
int monitor_callback(int node,int cticn,unsigned char *dat,int datlen);
int mycode_callback(int node,int cticn,unsigned char *dat,int datlen);
int mycode_handler(int in);
void printmap(int mapindex);
void getstring(char *prompt,char *s,int len);    
int inputint(char *ps);
int search(int *node);
int setoutput(int in,unsigned int val);
void sendoutput(int node);
int autoconnect(void);

int stopflag;
unsigned char outdat[64];

int main()
  {
  int n,flag,newflag,len,repn,mapn,node,ret,val,count,pret,oknode;
  unsigned char uuid[2],buf[1024];
    
  if(init_blue("xbox.txt") == 0)
    return(0);

  for(n = 0 ; n < 64 ; ++n)
    outdat[n] = 0;
  oknode = 0;
 
  /********* AUTO CONNECT ************
    Uncomment the following two instructions
    to connect automatically and run mycode_callback
    with inputs sent to in mycode_handler
  *************************************/  
     
  // autoconnect();
  // return(0);
  
  /********* END AUTO CONNECT ******/
 
 
  while(1)
    {
    do
      { 
      if(oknode != 0)
        {
        printf("  0 = Exit\n");
        printf("  1 = Reconnect\n");
        flag = inputint("Option 0/1");
        if(flag <= 0)
          {
          close_all();
          return(0);
          }
        newflag = 0;
        node = oknode;
        }  
      else if(strcmp(device_address(2),"00:00:00:00:00:00") == 0)
        {
        printf("\nXbox controller address not set in xbox.txt file\n");  
        printf("  0 = Exit and edit xbox.txt to set known address\n");
        printf("  1 = Scan to find Xbox waiting to pair - press\n");
        printf("      Xbox pair button for 3 seconds for rapid flash\n");
        flag = inputint("Option 0/1");
        if(flag <= 0)
          return(0);
        count = search(&node);
        if(count == 0)
          {
          printf("No devices found\n");
          return(0);
          }
        else
          {
          if(node != 0)
            printf("Found Gamepad node = %d\n",node);
          node = inputint("Input node number");
          }
        if(node < 0)
          return(0);
        newflag = 1;
        flag = 0;
        }
      else
        { 
        printf("\nXbox address = %s in xbox.txt file\n",device_address(2));
        printf("  0 = Exit\n");
        printf("  1 = New pairing (press pair button for rapid flash)\n");
        printf("  2 = Reconnect to previously paired device\n");
        flag = inputint("Option 0/1/2");
        if(flag <= 0)
          return(0);
        newflag = 0;
        node = 2;
        }
  
      set_le_wait(2000);         
      ret = connect_node(node,CHANNEL_LE,0);
      if(ret == 0)
        printf("Connect failed\n");
      }
    while(ret == 0);
    // connected
       
    set_le_wait(10000);
    if(flag == 1)
      pret = le_pair(node,JUST_WORKS | BOND_NEW | IRKEY_ON,0);
    else
      pret = le_pair(node,BOND_REPAIR,0);
  
    if(pret == 0)
      printf("Pairing failed - may need a new pairing\n");
    else
      { // paired 
      set_le_interval_update(node,6,12);
      mesh_off();
      find_ctics(node);  
      uuid[0] = 0x2A;
      uuid[1] = 0x4D;
      repn = find_ctic_index(node,UUID_2,uuid); // index of Report1
      uuid[0] = 0x2A;
      uuid[1] = 0x4B;
      mapn = find_ctic_index(node,UUID_2,uuid);   // index of Report Map
      if(repn < 0 || mapn < 0)
        printf("Bad connection/pairing or not an Xbox - may need a new pairing\n");
      else
        {  // connection/pair OK
        rep1[0].cticn = repn;
        rep2[0].cticn = repn+1;
        oknode = node;
        do
          {
          if(newflag == 1)
            {
            printf("\nSet Xbox address in xbox.txt file\n");
            printf("   ADDRESS = %s\n",device_address(node));
            printf("This will allow easy re-connection next time\n");
            newflag = 2;
            } 
           
          printf("\nOPERATION\n");
          printf("  0 = Print Report1\n");
          printf("  1 = Print Report2\n");
          printf("  2 = Monitor inputs and print incoming data\n");
          printf("  3 = Run mycode\n");
          printf("  4 = Rumble\n");
          printf("  5 = Disconnect and exit\n");
          flag = inputint("Enter option");  
          if(flag == 0)
            printmap(0);
          else if(flag == 1)
            printmap(1); 
          else if(flag == 2)
            {
            printf("**** MONITOR **** Operate controls - Xbox button to exit\n");
            notify_ctic(node,repn,NOTIFY_ENABLE,monitor_callback);
            stopflag = 0;
            do
              read_notify(10);
            while(stopflag == 0 && device_connected(node) != 0);
            notify_ctic(node,repn,NOTIFY_DISABLE,NULL);
            printf("Xbox button pressed\n");
            }
          else if(flag == 3)
            {
            printf("**** Mycode ****\n");
            printf("Put your code in mycode_handle()\n");
            printf("Only programmed with one action = Xbox button to exit\n");
            notify_ctic(node,repn,NOTIFY_ENABLE,mycode_callback);
            stopflag = 0;
            do
              read_notify(10);
            while(stopflag == 0 && device_connected(node) != 0);
            notify_ctic(node,repn,NOTIFY_DISABLE,NULL);
            printf("Xbox button pressed\n");
            }
          else if(flag == 4)
            {
            printmap(1);
            printf("Above is the Report for outputs\n");
            printf("This now assumes that the Rumble items are\n");
            printf("1=Activate 5=Magnitude 6=Duration 7=Start delay\n");
            val = inputint("Magnitude");
            setoutput(1,1);
            setoutput(5,val);
            val = inputint("Duration");
            setoutput(6,val);
            val = inputint("Start delay");
            setoutput(7,val);
            sendoutput(node);
            }
          }
        while(flag != 5);
        }  // end len > 0
      } // end pret
    disconnect_node(node);
    } // end while
  
  return(0);
  }

int autoconnect()
  {
  int ret,node,count,pflag,pret,searchflag;
  int mapn,repn,repairnode;
  unsigned char uuid[2];
    
  if(strcmp(device_address(2),"00:00:00:00:00:00") == 0)
    searchflag = 1;  // scan to search for unknown xbox
  else
    searchflag = 0;  // known xbox address set in devices[]
        
  repairnode = 0;  // failed re-pair  
  pret = 0;
  do
    {
    if(searchflag == 0)
      node = 2;  // address set in devices
    else if(pret == 0)
      {          // scan to search
      do
        { 
        count = search(&node);
        }
      while(node == 0);
      // Found 
      }
       
    printf("Connect to node %d\n",node);
    set_le_wait(2000);
    ret = connect_node(node,CHANNEL_LE,0);
    if(ret == 0)
      {
      // Connect failed
      pret = 0;
      }
    else
      {   
      // Connect OK
      set_le_wait(10000);
      pflag = device_paired(node);
      if(pflag == 0 || pret == 2 || node == repairnode)
        {
        // New bond
        printf("New bond - press pair button for rapid flash\n");
        pret = le_pair(node,JUST_WORKS | BOND_NEW | IRKEY_ON,0);
        }
      else
        {
        // Re-bond
        printf("Re-pair\n");
        pret = le_pair(node,BOND_REPAIR,0);  
        if(pret == 0)
          {
          repairnode = node;
          pret = 2;
          // RePair failed - loop for new bond
          }
        }
      if(pret != 1)
        disconnect_node(node);
      }
    if(pret != 1)
      sleep_ms(3000);
    }  
  while(pret != 1);
  
    
  set_le_interval_update(node,6,12);
  mesh_off();
  find_ctics(node);  
  uuid[0] = 0x2A;
  uuid[1] = 0x4D;
  repn = find_ctic_index(node,UUID_2,uuid); // index of Report1
  uuid[0] = 0x2A;
  uuid[1] = 0x4B;
  mapn = find_ctic_index(node,UUID_2,uuid);   // index of Report Map
  if(repn < 0 || mapn < 0)
    printf("Bad connection/pairing or not an Xbox - may need a new pairing\n");
  else
    {  // connection/pair OK
    if(searchflag != 0)
      {
      printf("\nSet Xbox address in xbox.txt file\n");
      printf("   ADDRESS = %s\n",device_address(node));
      printf("This will allow easy re-connection next time\n");
      }

    rep1[0].cticn = repn;
    rep2[0].cticn = repn+1;
    printf("Connected OK\n");  
    printf("Only programmed with one action = Xbox button to exit\n");
    notify_ctic(node,repn,NOTIFY_ENABLE,mycode_callback);
    stopflag = 0;
    do
      read_notify(10);
    while(stopflag == 0 && device_connected(node) != 0);
    notify_ctic(node,repn,NOTIFY_DISABLE,NULL);
    disconnect_node(node);
    printf("Xbox button pressed\n");
    }
  return(node);
  }

int mycode_callback(int node,int cticn,unsigned char *dat,int datlen)
  {
  int n,in,dn;
  unsigned int val;
  static int firstflag = 0;
  
  if(cticn != rep1[0].cticn)
    return(0); 

  for(in = 1 ; rep1[in].flag != 0 ; ++in)
    {
    dn = rep1[in].offset;
    val = 0;
    for(n = rep1[in].numbytes-1 ; n >= 0 ; --n)
      val = (val << 8) + dat[dn+n];
    val >>= rep1[in].shift;
    val &= rep1[in].bitmask;
    if(firstflag == 0)
      rep1[in].value = val;
    else if(rep1[in].value != val)
      {
      rep1[in].value = val;
      mycode_handler(in);
      }
    }
  firstflag = 1;  
  return(0);
  }  

int mycode_handler(int in)
  {
  int newvalue;
  
  // Input in has changed to newvalue
  
  newvalue = rep1[in].value;
  switch(in)
    {
    case 1:
      // L stick X
      break;
    case 2:
      // L stick Y 
      break;
    case 3:
      // R stick X
      break;
    case 4:
      // R stick Y
      break;
    case 5:
      // L trigger 
      break;
    case 6:
      // R trigger
      break;
    case 7:
      // D pad
      break;
    case 8:
      // A button
      break;
    case 9:
      // B button
      break;
    case 10:
      // Unknown
      break;
    case 11:
      // X button
      break;
    case 12:
      // Y button
      break;
    case 13:
      // Unknown
      break;
    case 14:
      // L bumper
      break;
    case 15:
      // R bumper
      break;
    case 16:
      // Unknown
      break;
    case 17:
      // Unknown
      break;
    case 18:
      // View button
      break;
    case 19:
      // Menu button
      break;
    case 20:     
      // Xbox button
      if(newvalue == 1)
        stopflag = 1;
      break;
    case 21:
      // Unknown
      break;
    case 22:
      // Unknown
      break;
    case 23:
      // Share button
      break;
    default:
      break;
    }
  
  
  return(0);  
  }


int monitor_callback(int node,int cticn,unsigned char *dat,int datlen)
  {
  int n,in,dn;
  unsigned int val;
  static int firstflag = 0;
  
  if(cticn != rep1[0].cticn)
    return(0); 
  
  for(in = 1 ; rep1[in].flag != 0 ; ++in)
    {
    dn = rep1[in].offset;
    val = 0;
    for(n = rep1[in].numbytes-1 ; n >= 0 ; --n)
      val = (val << 8) + dat[dn+n];
    val >>= rep1[in].shift;
    val &= rep1[in].bitmask;
    if(firstflag == 0)
      rep1[in].value = val;
    else if(rep1[in].value != val)
      {
      rep1[in].value = val;
      if(in == 1 || in == 2)
        printf("1,2 L stick = %d,%d\n",rep1[1].value,rep1[2].value);       
      else if(in == 3 || in == 4)
        printf("3,4 R stick = %d,%d\n",rep1[3].value,rep1[4].value);       
      else
        printf("%d %s = %d\n",in,rep1[in].desc,val);
      }
    }

  if(rep1[20].value != 0)
    stopflag = 1;
  firstflag = 1;  
  return(0);
  }  
  
int search(int *node)
  {
  int n,i,len,flag,count;
  unsigned char *s;
 
  le_scan();   
  *node = 0;
  n = 1000;
  printf("DEVICES\n");
  count = 0;
  while(device_type(n) != 0)
    {
    ++count;
    s = le_advert(n);
    len = s[0];
    printf("  Node = %d Address = %s ",n,device_address(n)); 
    i = 1;
    flag = 0;
    while(i < len-1 && flag == 0)
      {
      if((s[i+1] == 0x19 && s[i+2] == 0xC4 && s[i+3] == 0x03) ||
         (s[i+1] == 0x03 && s[i+2] == 0x12 && s[i+3] == 0x18))
        {        
        printf("Gamepad");
        flag = 1;
        if(*node == 0)
          *node = n;
        } 
      i += s[i]+1;
      }
    printf("\n");
    ++n; 
    }
  return(count);
  }

  
void printmap(int mapindex)
  {
  int n;
  struct itemdata *rep;
  static char *ios[2] = { "in ","out" };
  static char *inouts[4] = {
  "Invalid",
  "Input",
  "Output",
  "Input and output" };
  
  if(mapindex < 0 || mapindex > 1)
    {
    printf("Invalid map number\n");
    return;
    }
  rep = replist[mapindex];
  printf("         Report %d %s\n",mapindex+1,inouts[rep[0].dirn]);
  printf("         Bits  Offset  Nbytes  Shift   Min      Max\n");
  for(n = 1 ; rep[n].flag != 0 ; ++n)
    {
    printf("%2d  %s   %2d    %2d      %2d      %2d  %5d      %5d   %s\n",n,ios[rep[n].dirn],
            rep[n].size,rep[n].offset,rep[n].numbytes,rep[n].shift,rep[n].min,rep[n].max,rep[n].desc);
    }
  }   

void sendoutput(int node)
  {
  write_ctic(node,rep2[0].cticn,outdat,rep2[0].numbytes);
  } 
    
int setoutput(int in,unsigned int val)
  {
  unsigned int xval,bitmask,msk,byt;
  int n,off;

  if(in < 1 || in > 8)
    return(0);
  
  xval = val << rep2[in].shift;  
  bitmask = rep2[in].bitmask << rep2[in].shift;
  off = rep2[in].offset;
  for(n = 0 ; n < rep2[in].numbytes ; ++n)
    {
    msk = bitmask & 0xFF;
    byt = xval & msk;
    outdat[off] &= (~msk) & 0xFF;
    outdat[off] |= byt;
    xval >>= 8;
    bitmask >>= 8;
    ++off;
    }    
  return(0);
  }   
   
/***** INPUT INTEGER ********/     
int inputint(char *ps)
  {
  int n,flag;
  char s[128];
 
  do
    {
    printf("%s  (x=cancel)\n",ps);
  
    getstring("? ",s,128);
       
    flag = 0;
    for(n = 0 ; s[n] != 0 ; ++n)
      {
      if(s[n] < '0' || s[n] > '9')
        flag = 1;
      }
    if(flag == 0)
      n = atoi(s);
    else if(s[0] == 'x')
      {
      n = -1;
      flag = 0;
      }
    else if(s[0] == '[')
      scroll_back();
    else if(s[0] == ']')
      scroll_forward();
    else
      printf("Not a number\n");
    }
  while(flag != 0);
  return(n);
  } 



/******** GET STRING *******
from keyboard input
return in s with terminate 0
len = length of s
***********************/

void getstring(char *prompt,char *s,int len)
  {
  int n;
  
  do
    {
    printf("%s",prompt);
    fgets(s,len,stdin);
    }
  while(s[0] == 10);

  n = 0;
  while(s[n] != 10 && s[n] != 13 && s[n] != 0)
    ++n;
  s[n] = 0;
  }  
