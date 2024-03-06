
/******* BLUETOOTH INTERFACE **********
REQUIRES
  btlib.c/h  Version 13 
  devices.txt
COMPILE
  gcc btferret.c btlib.c -o btferret
RUN
  sudo ./btferret
EDIT
  devices.txt to list all devices in the network
  and set their ADDRESS= entries 
**************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "btlib.h"  


void btlink(void);
void printhelp(void);
void settings(void);
int clientread(int node);
int sendstring(int node,char *comd);
int inputint(char *ps);
int sendgetfile(void);
int sendfilex(int node,char *opcode,char *filename,char *destdir,int blocksize,int termchar);
int receivefile(char *fname,int clientnode);
int receivefilex(char *fname,int clientnode);
int filelength(FILE *file);
int inputnode(int typemask,int meshflag);
int inputchan(int node,int *channel,int *method);
void readservices(void);
void readuuid(void);
int clientconnect(void);
int meshsend(void);
int clientsend(int fun);
int server(void);
int node_classic_callback(int clientnode,unsigned char *buf,int nread);
int le_callback(int clientnode,int operation,int cticn);
int mesh_callback(int clientnode,unsigned char *buf,int nread);
int notify_callback(int lenode,int cticn,unsigned char *buf,int nread);
void localdisconnect(void);
void readnotify(void);
void readle(void);
void writele(void);
void notifyle();
void getstring(char *prompt,char *s,int len);
void readnotify(void);
void regserial(void);
unsigned short crccalc(unsigned short crc,unsigned char *buf,int len);
int inputlist(char *buf,int len);

char endchar = 10;  // terminate char for read/write 


int main(int argc,char *argv[])
  { 
  if(init_blue("devices.txt") == 0)      
    return(0);     

  btlink();

  printf("Disconnecting everything and exit..\n");
  
  close_all();
  
  return(0);
  } 


void btlink()
  {
  int cmd;
  int retval;
  char cmds[64];
    

  do
    {
    // wait for command char
    getstring("> ",cmds,64);
        
    cmd = cmds[0];
    
    switch(cmd)
      {                              
      case 'q':
        return;
        break;

      default:
        retval=0;
          le_scan();
          sleep(2);

        while (retval!=1){
          retval=clientconnect();
          sleep(2);
        }

        notifyle();
        sleep(2);

        readle();
        sleep(2);

        disconnect_node(1);
        sleep(5);
        printf("Continue...\n");
        break;
      }
    }
  while(1); 
  
  }

/*********** CONNECT/DISCONNECT *********/

int clientconnect()
  {
  int node,channel,method,retval,contype,wait;
    
     // only disconnected devices
  //node = inputnode(BTYPE_CL | BTYPE_LE | BTYPE_DISCONNECTED | BTYPE_ME,0);
  node = 1;

  if(node < 0)
    {
    printf("Cancelled\n");
    return(0);
    }
   
  channel = 0;
  method = 0;
  
  if(device_type(node) == BTYPE_ME)
    contype = inputint("\n  0 = Node server\n  1 = Classic server\n  2 = LE server\nInput listening type of remote mesh device");
  else
    contype = 0;
   
  if(contype < 0)
    {
    printf("Cancelled\n");
    return(0);
    }

  if(device_type(node) == BTYPE_CL)
    {  // classic server needs method = CHANNEL_STORED
       // or method = CHANNEL_NEW and channel  
    if(inputchan(node,&channel,&method) < 0)
      {
      printf("Cancelled\n");
      return(0);
      }   
    }
  else if(device_type(node) == BTYPE_ME)
    {  // mesh device can be listening as Classic, LE or node server  
    if(contype == 0)
      method = CHANNEL_NODE;  // node server
    else if(contype == 1)
      {  // classic server
      channel = 1;  // mesh classic listens on channel 1
      method = CHANNEL_NEW;
      }
    else if(contype == 2)
      method = CHANNEL_LE;  // LE server
    else
      {
      printf("Invalid listening type\n");
      return(0);
      }
    }
  else if(device_type(node) == BTYPE_LE)
    {
    method = CHANNEL_LE;
    printf("\nCONNECTION COMPLETE TIME\n");
    printf("  After connecting, some LE servers need more time to complete\n");
    printf("  the process or they will disconnect. Zero may work, otherwise\n");
    printf("  find the shortest time that will prevent disconnection.\n");
    printf("  Current value=%d\n",set_le_wait(READ_WAIT)); 
    }
  else
    {  
    printf("Invalid device type\n");
    return(0);
    }
  
  retval = connect_node(node,method,channel);
  return(retval);    
  }


void localdisconnect()
  {
  int node=1;
  
  printf("\nUse D instead to disconnect btferret node and mesh servers\n");
     
      // only connected devices
  //node = inputnode(BTYPE_CL | BTYPE_LE | BTYPE_ME | BTYPE_CONNECTED,0);
  if(node < 0)
    {
    printf("Cancelled\n");
    return;
    }   

  disconnect_node(node);
  }

void readnotify()
  {
  int timeout;
     
  timeout = inputint("\nRead notifications\nInput time out in s");
  if(timeout < 0)
    {
    printf("Cancelled\n");
    return;
    }

  printf("Reading... (x = stop)\n"); 
  read_notify(timeout*1000);
   
  printf("Read notifications finished\n");
  return;
  }
  
  
/*********** READ SERVICES *************/

  
void readservices()
  {
  int node;
  
  printf("\nRead services\n");
  
  node = inputnode(BTYPE_CL | BTYPE_LE | BTYPE_ME | BTYPE_LO, 0);
  if(node < 0)
    return;

  if(device_type(node) != BTYPE_CL)  // pure classic devices cannot have LE characteristics
    find_ctics(node);
  if(device_type(node) != BTYPE_LE)  // pure LE devices cannot have classic serial channels
    list_channels(node,LIST_FULL);
  
  return;
  }
  
  
void readuuid()
  {
  int num,node,op,ret,flag;
  char suuid[64];
  unsigned char *uuid;
   
  printf("\nFind services that contain a specified UUID\n");
  printf("  0 = List services\n");
  printf("  1 = Find LE characteristic index\n");
  printf("  2 = Find Classic RFCOMM channel\n"); 
  op = inputint("Input 0-2");
  if(op < 0 || op > 2)
    {
    printf("Invalid entry\n");
    return;
    }
    
  node = inputnode(BTYPE_CL | BTYPE_LE | BTYPE_ME,0);
  if(node < 0)
    return;
  
  if(op == 0)    
    printf("Input 2-byte UUID in hex e.g. 0100  (x = cancel)\n");
  else
    printf("Input 2 or 16-byte UUID in hex e.g. 0100  (x = cancel)\n");
    
  getstring("? ",suuid,64);
   
  if(suuid[0] == 'x')
    return;  
     
  uuid = strtohex(suuid,&num);
    
  if(op == 0)
    {
    if(num != 2)
      {
      printf("Not 2-byte\n");
      return;
      }  
    // uuid must be 2-byte hi first     
    list_uuid(node,uuid);
    return;
    }
   
  if(num == 2)
    flag = UUID_2;
  else if(num == 16)
    flag = UUID_16;
  else
    {
    printf("Not 2/16 byte\n");
    return;
    }
    
  if(op == 1)  
    {
    ret = find_ctic_index(node,flag,uuid);
    if(ret < 0)
       printf("UUID not found\n");
    else
      printf("Characteristic index = %d\n",ret);
    }
  else if(op == 2)
    { 
    ret = find_channel(node,flag,uuid);
    if(ret < 0)
      printf("Failed to read services\n");
    else if(ret == 0)
      printf("UUID not found\n");
    else
      printf("RFCOMM channel = %d\n",ret);
    }
       
  }

/********** READ/WRITE LE CHARACTERISTIC ****************/

void readle()
  {
  int n,node,cticn,ascflag,datlen;
  unsigned char dat[256];
  
  printf("\nRead an LE characteristic\n");
  
  //node = inputnode(BTYPE_LE | BTYPE_ME | BTYPE_CONNECTED | BTYPE_LO,0);   // only connected LE devices
  node = 1;
  if(node < 0)
    {
    printf("Cancelled\n");
    return;
    }
  
  if(list_ctics(node,LIST_SHORT | CTIC_R) == 0)
    {
    printf("No readable characteristics. Read services to find\n");
    return;
    }
      
  //cticn = inputint("Input ctic index");
  cticn = 0;
  if(cticn < 0)
    return;
       
  datlen = read_ctic(node,cticn,dat,sizeof(dat));
  
  if(datlen == 0)
    return;     // fail or no data
  
  printf("%s %s = ",device_name(node),ctic_name(node,cticn));
  
  ascflag = 0;
  if(datlen > 2)     
    ascflag = 1;
        
  for(n = 0 ; n < datlen ; ++n)
    {
    printf("%02X ",dat[n]);
    if(!(n == datlen-1 && dat[n] == 0) && (dat[n] < 32 || dat[n] > 126))
      ascflag = 0;  // not an ascii string
    }
        
  if(ascflag == 0)
    printf("\n");
  else
    printf("= \"%s\"\n",dat);  // print ascii - readctic has added term 0
                               // at dat[datlen] 
  }

void writele()
  {
  int node,cticn,size;
  char buf[256];
  unsigned char *val;
  
  printf("\nWrite an LE characteristic\n");
  
  node = inputnode(BTYPE_LE | BTYPE_ME | BTYPE_CONNECTED | BTYPE_LO,0);   // only connected LE devices
  if(node < 0)
    {
    printf("Cancelled\n");
    return;
    }
  
  if(list_ctics(node,LIST_SHORT | CTIC_W) == 0)
    {
    printf("No writeable characteristics\n");
    return;
    }
      
  cticn = inputint("Input ctic index");
  if(cticn < 0)
    return;    
   
  printf("Input data bytes in hex e.g. 5A 43 01\n");
  getstring("? ",buf,128);

  val = strtohex(buf,&size);
 
  write_ctic(node,cticn,val,size);
   
  }


void notifyle()
  {
  int node,cticn,flag;
  char *s;

     
  printf("\nEnable/Disable LE characteristic notify/indicate\n"); 
  
  //node = inputnode(BTYPE_LE | BTYPE_ME | BTYPE_CONNECTED,0);  // only connected LE devices
  node = 1;
  if(node < 0)
    {
    printf("Cancelled\n");
    return;
    }
  
  if(list_ctics(node,LIST_SHORT | CTIC_NOTIFY) == 0)
    {
    printf("No characteristics with notify/indicate permission\n");
    return;
    }
       
  //cticn = inputint("Input ctic index");
  cticn = 0;
   if(cticn < 0)
    return; 

  if(ctic_ok(node,cticn) == 0)
    {
    printf("Invalid index\n");
    return;
    }

  //flag = inputint("0=Disable 1=Enable");
  flag = 1;
  if(flag < 0)
    return;
    
  if(flag == 1)
    {
    flag = NOTIFY_ENABLE;
    s = "enabled";
    }
  else
    {
    flag = NOTIFY_DISABLE;
    s = "disabled";
    }

  if(notify_ctic(node,cticn,flag,notify_callback) == 0)
    s = "failed";
  
  printf("%s %s %s\n",device_name(node),ctic_name(node,cticn),s);
 
  return;
  }   


int notify_callback(int lenode,int cticn,unsigned char *buf,int nread)
  {
  int n;
  
  // LE device has sent notification or indication enabled by notify_ctic()
  
  printf("%s %s notify =",device_name(lenode),ctic_name(lenode,cticn));
  for(n = 0 ; n < nread ; ++n)
    printf(" %02X",buf[n]);
  printf("\n");
  return(0);
  }


void regserial()
  {
  int n;
  char name[128],uuid[64];
  unsigned char *newcustom;
  
  printf("\nRegister a custom serial service\n");
  
  printf("Input 16-byte UUID e.g. 0011-2233-44556677-8899AABBCCDDEEFF (x = cancel)\n");
  getstring("? ",uuid,64);
  
  if(uuid[0] == 'x')
    return;

  newcustom = strtohex(uuid,&n);
   
  if(n != 16)
    {
    printf("UUID must be 16 bytes\n");
    return;
    }     
    
  printf("Input service name (x = cancel)\n");
  getstring("? ",name,128); 

  if(name[0] == 'x')
    return;
    
  register_serial(newcustom,name);
  printf("Done\n");
  }

/********** USER INPUT FUNCTIONS *******/
 
int inputnode(int mask,int meshflag)
  {
  int n,count,node,flag;

  printf("\n");

  count = 0; 
   
  flag = 0;
  if((mask & BTYPE_CL) != 0)
    {
    printf("CLASSIC servers");
    flag = 1;
    }
  if((mask & BTYPE_LE) != 0)
    {
    if(flag != 0)
      printf(" + ");
    printf("LE servers");
    flag = 1;
    }
  if((mask & BTYPE_ME) != 0)
    {
    if(flag != 0)
      printf(" + ");
    printf("NODE servers");
    }
    
  if((mask & BTYPE_CONNECTED) != 0)
    printf(" - connected only");
  if((mask & BTYPE_DISCONNECTED) != 0)
    printf(" - disconnected only");
  printf("\n");
  
  n = device_info(mask | BTYPE_SHORT);
  if(n == 0)
    printf("  None\n");
    
  count += n;
       
  if(meshflag == 1)
    printf(" 0 - All mesh servers (not connected node servers)\n");
  else if(meshflag == 2)
    printf(" 0 - Any device\n");  
  else if(count == 0)
    return(-1);  
                
  do
    {
    flag = 0;
    node = inputint("Input node");
    if(node < 0)
      return(-1);   // cancel
    if(meshflag != 0 && node == 0)
      flag = 1;
    else if((device_type(node) & mask) != 0)
      flag = 1;
    else
      printf("Invalid node\n");
    }
  while(flag == 0);

  return(node);
  }


/******* INPUT CHANNEL *******/

int inputchan(int node,int *channel,int *method) 
  {
  int n,flag;

  printf("\n  0 = Reconnect/Use stored channel number\n");
  printf("  1 = Input channel number\n  2 = Read services to choose channel\n");
  do
    {
    flag = 0;
    n = inputint("Input option 0/1/2");
    if(n < 0)
      return(-1);
    if(n > 2)
      flag = 1;
    }
  while(flag != 0);
  
  if(n == 0)
    {
    *channel = 0;  
    *method = CHANNEL_STORED;  // reconnect
    return(1);
    }
    
  *method = CHANNEL_NEW;   
  
  if(n == 2)
    {
    if(list_channels(node,LIST_SHORT) <= 0)     
      {
      printf("Failed to find RFCOMM channels\n");
      return(-1);
      }
    }
  
  *channel = inputint("Input RFCOMM channel number");
  if(*channel < 0)
    return(-1);
    
  return(1);
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


unsigned short crccalc(unsigned short crc,unsigned char *buf,int len)
  {
  static unsigned short table[256];
  static int init = 0;
  unsigned short i,j,cwd;
     
  if(init == 0)
    {
    init = 1;
    for(j = 0 ; j < 256 ; ++j)
      {
      cwd = j << 8;
      for(i = 0 ; i < 8 ; ++i)
        {
        if((cwd & 0x8000) == 0)       
          cwd <<= 1;
        else
          {
          cwd <<= 1;
          cwd ^= 0x1021;
          }
        }
      table[j] = cwd;
      }
    }
  
  cwd = crc;  
   
  for(j = 0 ; j < len ; ++j)
    cwd = table[buf[j] ^ ((cwd >> 8) & 0xFF)] ^ ((cwd & 0xFF) << 8);
    
  return(cwd);
      
  }
