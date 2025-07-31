#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btlib.h"   
#include "pico/cyw43_arch.h"

/********* XBOX *************
  Continuously attempts connection to DEVICE=Xbox node=2 if the ADDRESS is set
      e.g. ADDRESS=AC:8E:BD:7A:48:0D

  If ADDRESS is not set (all zero) - continuously scans for an advertising
  Gamepad device and attempts connection if one is found.
  
  When connected: The following are programmed in mycode_handler();
    1. Pressing the A button flashes the LED
    2. Pressing the Xbox button rumbles the controller and disconnects.
       The code then loops back to search for another connection.  
*******************************/

/*********** Set ADDRESS= of DEVICE=Xbox if known **********/

char *devices = { "DEVICE=Local node=1 type=mesh ADDRESS=local\n\
DEVICE=Xbox  node=2 type=LE   ADDRESS=00:00:00:00:00:00" };

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
int mycode_callback(int node,int cticn,unsigned char *dat,int datlen);
int mycode_handler(int node,int in);
int search(int *node);
int setoutput(int in,unsigned int val);
void sendoutput(int node);
int autoconnect(void);
void rumble(int node,int magnitude,int duration,int startdelay);

int stopflag,repairnode;
unsigned char outdat[64];
unsigned char buf[1024];

int mycode()
  {
  int n,node,repn,mapn,len;
  unsigned char uuid[2];
  
  if(init_blue(devices) == 0)
    return(0);
  
  for(n = 0 ; n < 64 ; ++n)
    outdat[n] = 0;
  repairnode = 0;
  
  while(1)
    {
    node = autoconnect();
  
    set_le_interval_update(node,6,12);
    mesh_off();
    find_ctics(node);
  
    uuid[0] = 0x2A;
    uuid[1] = 0x4D;
    repn = find_ctic_index(node,UUID_2,uuid); // index of Report1
    uuid[0] = 0x2A;
    uuid[1] = 0x4B;
    mapn = find_ctic_index(node,UUID_2,uuid);   // index of Report Map
    if(repn > 0 && mapn > 0)
      {
      rep1[0].cticn = repn;
      rep2[0].cticn = repn+1;
      if(node == repairnode)
        repairnode = 0;
      notify_ctic(node,repn,NOTIFY_ENABLE,mycode_callback);

      stopflag = 0;
      do
        read_notify(10);
      while(stopflag == 0 && device_connected(node) != 0);
      notify_ctic(node,repn,NOTIFY_DISABLE,NULL);
      }
    disconnect_node(node);
    }
  }

int autoconnect()
  {
  int ret,node,count,pflag,pret,searchflag;
  
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
  return(node);
  }
  
  
int mycode_callback(int node,int cticn,unsigned char *dat,int datlen)
  {
  int n,in,dn;
  unsigned int val;
  static int firstflag = 0;
  
  if(cticn != rep1[0].cticn)
    return(0); 

  in = 1;
  while(rep1[in].flag != 0)
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
      mycode_handler(node,in);
      }
    ++in; 
    }
    
  firstflag = 1;  
  return(0);
  }  

int mycode_handler(int node,int in)
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
      if(newvalue == 1)
        {  // flash LED
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);  
        sleep_ms(100);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);  
        }
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
        {
        stopflag = 1;
        rumble(node,50,20,0);
        }
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

void rumble(int node,int magnitude,int duration,int startdelay)
  {
  setoutput(1,1);   // activate
  setoutput(5,magnitude); 
  setoutput(6,duration); 
  setoutput(7,startdelay); 
  sendoutput(node);
  }
 
int search(int *node)
  {
  int n,i,len,count;
  unsigned char *s;
 
  le_scan();   
  *node = 0;
  n = 1000;
  count = 0;
  while(device_type(n) != 0)
    {
    ++count;
    s = le_advert(n);
    len = s[0];
    i = 1;
    while(i < len-1)
      {  // look for Gamepad appearance=03C4 or HID service UUID=1812
      if(s[i+1] == 0x19 && s[i+2] == 0xC4 && s[i+3] == 0x03)
        {  // Gamepad
        *node = n;
        return(count);
        }
      else if(*node == 0 && (s[i+1] == 0x03 && s[i+2] == 0x12 && s[i+3] == 0x18))
        {   // HID
        *node = n;
        } 
      i += s[i]+1;
      }
    ++n; 
    }
  return(count);
  }
  
void sendoutput(int node)
  {
  write_ctic(node,rep2[0].cticn,outdat,rep2[0].numbytes);
  }
    
int setoutput(int in,unsigned int val)
  {
  unsigned int xval,bitmask,msk,byt;
  int n,off;
  
  if(in < 0 || in > 31 || rep2[in].flag == 0)
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
   
