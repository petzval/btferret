#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btlib.h"

// VERSION 23

/***** functions in picostack.c for USB serial monitor control ****/
/***** for inputpin() and readkey() **/
#define ENABLE_READKEY 1
#define ENABLE_INPUTPIN 2
void set_usb_flags(int flags);
int read_usb_endchar(unsigned char *buf,int len,unsigned char endchar,int toms);
/********/

/********** DEVICES list ************
  Each line must end with \n\
************************************/

char *devices = { 
";*** This LOCAL device ***                                      \n\
DEVICE = Picostack  type=mesh  node=1  address=local             \n\
PRIMARY_SERVICE=1800                                             \n\
  LECHAR=Device name  SIZE=16 PERMIT=02 UUID=2A00     ;index 0   \n\
PRIMARY_SERVICE=112233445566778899AABBCCDDEEFF00                 \n\
   LECHAR = Data   SIZE=8  PERMIT=06 UUID=ABCD        ;index 1   \n\
   LECHAR = LED control  SIZE=1 PERMIT=04 UUID=CDEF   ;index 2   \n\
                                                                 \n\
; **** OTHER DEVICES                                                               \n\
DEVICE = Windows PC    TYPE=CLASSIC  NODE=4   address=00:1A:7D:DA:71:13            \n\
DEVICE = LE device     TYPE=LE      NODE=7   ADDRESS = 00:1E:C0:2D:17:7C           \n\
DEVICE = Android phone TYPE=CLASSIC node=10  ADDRESS = 98:80:EE:75:4D:EB           \n\
DEVICE = HC-05 TYPE=CLASSIC node=9 PIN=1234 channel=1 ADDRESS=98:D3:32:31:59:84    \n\
DEVICE = My Other Pi   TYPE=mesh NODE=2  ADDRESS = DC:A6:32:04:DB:56               \n\
   ; LE characteristics when acting as an LE server                                \n\
   ; Specify UUIDs and let system allocate handles                                 \n\
   PRIMARY_SERVICE = 1800                                                          \n\
     LECHAR = Device Name  PERMIT=02 SIZE=16 UUID=2A00   ; index 0                 \n\
   PRIMARY_SERVICE = 1801                                                          \n\
     LECHAR = Service changed PERMIT=20 SIZE=4 UUID=2A05 ; index 1                 \n\
   PRIMARY_SERVICE = 112233445566778899AABBCCDDEEFF00                              \n\
     LECHAR = Control  PERMIT=06 SIZE=8 UUID=ABCD        ; index 2                 \n\
     LECHAR = Info     PERMIT=06 SIZE=1 UUID=CDEF        ; index 3                 \n\
                                                                                   \n\
DEVICE = Pictail LE    TYPE=LE      NODE=12   ADDRESS = 00:1E:C0:3A:24:1C          \n\
   ; Characteristics of the LE server device                                       \n\
   ; Specify UUID, but find_citcs() must be called to find handle                  \n\
   LECHAR = Device Name  PERMIT=02 SIZE=16 UUID=2A00   ; index 0                   \n\
   ; If handles are known and specifed, find_ctics() is not needed                 \n\
   LECHAR=Test    HANDLE=000B PERMIT=06 SIZE=1         ; index 1                   \n\
   LECHAR=Control HANDLE=000D PERMIT=06 SIZE=8         ; index 2                   \n\
                                                                                   \n\
DEVICE = Galaxy  TYPE=LE NODE=5 ADDRESS = MATCH_NAME                               \n\
       ; use MATCH_NAME for devices with a known name and an                       \n\
       ; unknown address (e.g. a random address LE device)                         \n\
       ; Run a classic_scan or le_scan to find the address                         \n\
       ; This example will look for a device name starting Galaxy                  \n\
                                                                                   \n\
DEVICE = nRF52840  TYPE=LE NODE=8  ADDRESS=CD:01:87:91:DF:39  RANDOM=UNCHANGED     \n\
  LECHAR=Test    HANDLE=0014 PERMIT=0A SIZE=2                                      \n\
      ; use RANDOM=UNCHANGED for random address LE devices that                    \n\
      ; do not change their address (no scan required)                             \n\
"};


/**** functions in this code */
int btfdespatch(char c);
void printhelp(void);
void settings(void);
int clientread(int node);
int sendstring(int node,char *comd);
int inputnode(char *pr,int typemask);
int inputchan(int node,int *channel,int *method);
void readservices(void);
int clientconnect(void);
int meshsend(void);
int clientsend(int fun);
int server(void);
int universal_callback(int clientnode,int operation,int cticn,unsigned char *buf,int nread);
int node_classic_callback(int clientnode,unsigned char *buf,int nread);
int le_callback(int clientnode,int operation,int cticn);
int mesh_callback(int clientnode,unsigned char *buf,int nread);
int notify_callback(int lenode,int cticn,unsigned char *buf,int nread);
void localdisconnect(void);
void readnotify(void);
void readle(void);
void writele(void);
void notifyle();
void readnotify(void);
int clientsecurity(int *passkey);
int serversecurity(int *passkey);
void readlehandles();
void prcancel(void);
void print(char *s);
int inputint(char *ps);
void getstring(char *prompt,char *s,int len);
/********************/

char pathsep = '/';
char endchar = 10;  // terminate char for read/write 
char *prompt = NULL;
char *selectlist = NULL;
char *temps = NULL;
char noyes[16] = {"0 = No\n1 = Yes\n"};
char tempc[256];
unsigned char tempu[1024];
unsigned char sbuff[1100]; 

/***** mycode called from picostack.c on program start ****/

void mycode(void);

void mycode()
  { 
  int ret;

  sleep_ms(3000);  // 3s delay for USB monitor
  
  set_usb_flags(ENABLE_READKEY | ENABLE_INPUTPIN);

  if(init_blue(devices) == 0)      
    return;     

  printf("Send h as Text with LF line ending = help\n");
  while(1)
    {
    getstring("> ",tempc,8);
    btfdespatch(tempc[0]);
    }
  }


int btfdespatch(char cmd)
  {
  if(prompt == NULL)
    prompt = malloc(4096);
  if(selectlist == NULL)
    selectlist = malloc(4096);
  if(temps == NULL)
    temps = malloc(2048);
           
  switch(cmd)
    {
    case 'h':
      printhelp();
      break;

    case 'k':
      settings();
      break;
          
    case 'a':
      classic_scan();
      break;
      
    case 'b':
      le_scan();
      break;
      
    case 'i':
      device_info(BTYPE_CL | BTYPE_LO | BTYPE_LE | BTYPE_ME);
      break;
             
    case 'c':  
      clientconnect();
      break;
              
    case 't':
    case 'D':
    case 'p':
      clientsend(cmd);
      break;
         
    case 'T':
      meshsend();   
      break;
                 
    case 'd':
      localdisconnect();    
      break;
        
    case 's':
      server();
      break;

    case 'v':
      readservices();
      break;
       
    case 'r':
      readle();
      break;

    case 'w':
      writele();
        break;
  
    case 'j':
      notifyle();
        break;
             
    case 'R':
      readnotify();
        break;
        
    case 'm':
      mesh_on();
      print("Mesh on\n");
      break;
        
    case 'n':
      mesh_off();
      print("Mesh off\n");
      break;

    case 'u':
      print("Clear input buffer\n");
      read_all_clear();
      break;

    case 'l':
      readlehandles();
      break;
       
    default:
      print("Unknown command\n");
      break;  
    }     
  return(1);
  }


  
void printhelp()
  {
  print("\n  HELP\n");
  print("  a Scan for Classic devices    i Print device info\n");
  print("  b Scan for LE/Mesh devices    k Settings\n");   
  print("  c Connect to a node           p Ping server\n");
  print("  t Send string to server       T Send string to mesh\n");
  print("  r Read LE characteristic      w Write LE characteristic\n");
  print("  d Disconnect                  D Tell server to disconnect\n");  
  print("  s Become a listening server   l Read LE handles\n");
  print("  v Read node services          u Clear input buffer\n");
  print("  j LE notify/indicate on/off   R Read LE notifications\n");                     
  print("  m Mesh transmit on            n Mesh transmit off\n");                     
  }


/*********** CLIENT READ/SEND **********/

int clientread(int node)
  {
  int n,gotn;
     
   // read to endchar 3s time out
  gotn = read_node_endchar(node,tempu,1024,endchar,EXIT_TIMEOUT,3000);

  if(gotn > 0)
    {
    for(n = 0 ; n < gotn && n < 1023 ; ++n)
      { 
      if(n == gotn-1 && tempu[n] == endchar)
        tempu[n] = 0;  // wipe terminate char          
      else if(tempu[n] < 32 || tempu[n] > 127)
        tempu[n] = '.';            
      }
    // read has set buf[gotn] = 0
    
    sprintf(temps,"Reply from %s: ",device_name(node));
    print(temps);
    print((char*)tempu);
    print("\n");
    return(1);
    }
  sprintf(temps,"No reply from %s\n",device_name(node));
  print(temps);
  return(0);
  }


int clientsend(int cmd)
  {
  int flag,node;
  char *pr;
  
  if(cmd == 'p')
    pr = "Ping server";
  else if(cmd == 'D')
    pr = "Disconnect";
  else
    pr = "Send string";
    
  flag = BTYPE_CL | BTYPE_ME | BTYPE_CONNECTED;
  if(cmd == 'D')
    flag |= BTYPE_SERME;  // all mesh servers option
  
  node = inputnode(pr,flag);   // only connected classic/mesh
        
  if(node < 0)
    {
    prcancel();
    return(0);
    }
    
  if(node == 0)   // mesh packet to all mesh servers
    {
    write_mesh((unsigned char*)"D",1);
    return(1);
    }  
    
    
  if(device_connected(node) == NO_CONN)
    {
    print("Not connected\n");
    return(0);
    }  
  
  if(cmd == 'p' || cmd == 'D')
    print("This command only works if connected to a btferret server\n");
                 
  
  tempc[0] = 0;
  tempc[1] = 0;
  if(cmd == 't')
    {
    sprintf(prompt,"Will add endchar = %d\n",endchar);
    strcat(prompt,"Input string to send");
    printf("%s\n",prompt);    
    getstring("? ",tempc,128);
    } 
  else if(cmd == 'p')
    {
    tempc[0] = 0;  // empty string - will add endchar
    print("Ping server\n");
    }
  else if(cmd == 'D')
    {
    tempc[0] = 'D';
    print("Tell server to disconnect\n");
    }
  else
    {
    print("Unknown command\n");
    return(0);
    }
      
  if(sendstring(node,tempc) != 0)   
    {   // read reply from connected node - not mesh
    clientread(node);
    }
          
  if((device_type(node) == BTYPE_CL || device_type(node) == BTYPE_ME) && cmd == 'D') 
    wait_for_disconnect(node,5000);  // wait for classic server to initiate and complete disconnect
                                     // 5 sec time out

  return(1);
  }

/************ SERVER FUNCTIONS *************/

int server()
  {
  int serverflag,clinode,keyflag,inkey,timeds,passkey,pairflags,addr,lesecurity;
     // choose this random address (2 hi bits of 1st byte must be 1)
  static unsigned char randadd[6] = {  0xD9,0x56,0xDB,0x38,0x32,0xA2 };
  static unsigned char locadd[6] = { 0,0,0,0,0,0 };
  
  pairflags = 0;
  passkey = 0;
  timeds = 0;
  
  strcpy(prompt,"SERVER\n");   
  strcpy(selectlist,"  0 = Node server\n  1 = Classic server\n  2 = LE server\n");
  strcat(selectlist,"  3 = Universal server (Classic and LE)\n  4 = Mesh server\n");
  strcat(prompt,selectlist);  
  strcat(prompt,"Input server type 0-5");
  serverflag = inputint(prompt);

  if(serverflag < 0 || serverflag > 5)
    return(0);
  if(serverflag == 4)   // Mesh
    mesh_server(mesh_callback);
  else if(serverflag == 2 || serverflag == 3)
    {   // LE
    strcpy(prompt,"\nLE PAIRING and SECURITY\n");
    strcat(prompt,"  Most LE servers do not need security\n");
    strcat(prompt,"  so this option is not necessary\n");
    strcpy(selectlist,noyes);
    strcat(prompt,"Enable security options 0=No 1=Yes");
    lesecurity = inputint(prompt);
    if(lesecurity < 0)
      return(0);
    if(lesecurity > 1)
      lesecurity = 1;
  
    if(lesecurity == 1)
      {
      pairflags = serversecurity(&passkey);
      if(pairflags < 0)
        return(0);
      }
    
    if(serverflag == 2)
      {  
      strcpy(prompt,"DEVICE IDENTITY\n");
      strcat(prompt,"Some clients may fail to connect/pair if the Local address is used\n");
      strcat(prompt,"Random will set up a new LE address and identity for this device\n");
      strcpy(selectlist,"  0 = Local address - normal\n");
      strcat(selectlist,"  1 = Random address\n");
      strcat(prompt,selectlist);
      strcat(prompt,"Input 0/1");      
      addr = inputint(prompt);
    
      if(addr < 0)
        return(0);
      }
    else
      addr = 0;
        
    strcpy(prompt,"TIMER\nInput TIMER interval in deci (0.1) seconds\n   0 = No TIMER calls\n");
    strcat(prompt,"  10 = One second interval\n  50 = Five second interval etc...\nTimer interval");  
    timeds = inputint(prompt);

    if(timeds < 0)
      return(0);
    
    if(serverflag == 2)
      {
      strcpy(prompt,"KEY PRESSES\n");
      strcat(prompt,"Send key presses to KEYPRESS callback 0=No 1=Yes");
      keyflag = inputint(prompt);  
      if(keyflag < 0)
        return(0);
      }
    else
      keyflag = 0;
        
    if(addr == 0)  // Local address
      set_le_random_address(locadd);
    else  // Random address identity
      set_le_random_address(randadd);
                    
        
    if(keyflag == 0)
      keys_to_callback(KEY_OFF,0);
    else
      keys_to_callback(KEY_ON,0);  

    le_pair(localnode(),pairflags,passkey);  
    set_le_wait(5000);   // wait 5 seconds for connection/pairing                                         
    
    if(serverflag == 2)     
      le_server(le_callback,timeds);
    }
  else if(serverflag == 0)
    {  // node    
    clinode = inputnode("Input node of client that will connect",BTYPE_ME);  
    if(clinode < 0)
      {
      prcancel();
      return(0);
      }
    node_server(clinode,node_classic_callback,endchar);     
    }
     
  if(serverflag == 1 || serverflag == 5 || serverflag == 3)
    {  // classic
    if(serverflag == 3)
      clinode = 0;
    else  
      {
      clinode = inputnode("Input node of client that will connect",BTYPE_ME | BTYPE_CL | BTYPE_ANY);  
      if(clinode < 0)
        {
        prcancel();
        return(0);
        }
      }
      
    if(clinode != 0 && device_type(clinode) == BTYPE_ME)
      keyflag = KEY_OFF | PASSKEY_OFF;
    else
      {
      strcpy(prompt,"Classic security  (0,1,3 to pair or connect Android/Windows.. clients)\n");
      strcpy(selectlist,"  0 = Use link key, print passkey here, remote may ask to confirm\n");
      strcat(selectlist,"  1 = No link key,  print passkey here (forces re-pair if pairing fails)\n");
      strcat(selectlist,"  2 = No keys  (connecting client is another btferret device)\n");
      strcat(selectlist,"  3 = Use link key, no passkey\n");
      strcat(selectlist,"  4 = Use link key, remote prints passkey, enter it here if asked\n");
      strcat(prompt,selectlist);
      strcat(prompt,"Client's security requirement");      
      inkey = inputint(prompt);  

      if(inkey < 0 || inkey > 4)
        {
        prcancel();
        return(0);
        }      
      else if(inkey == 0)
        keyflag = KEY_ON | PASSKEY_LOCAL;  // local prints passkey - confirm on client
      else if(inkey == 1)
        keyflag = KEY_OFF | PASSKEY_LOCAL;
      else if(inkey == 2)
        keyflag = KEY_OFF | PASSKEY_OFF;
      else if(inkey == 3)
        keyflag = KEY_ON | PASSKEY_OFF;    
      else if(inkey == 4)
        keyflag = KEY_ON | PASSKEY_REMOTE;  // client prints passkey - enter on local
      }
                 
    if(clinode == 0)
      clinode = ANY_DEVICE;
    if(serverflag == 1)
      {
      print("\nServer will listen on channel 1 and any of the following UUIDs\n");
      print("  Standard serial 2-byte 1101\n");
      print("  Standard serial 16-byte\n");
      print("  Custom serial set via register serial\n");
      classic_server(clinode,node_classic_callback,endchar,keyflag);
      }
    }   
      
  if(serverflag == 3)
    universal_server(universal_callback,endchar,keyflag,timeds);
  return(0);
  }  
 

int serversecurity(int *passkey)
  {
  int flags,key,auth,jwflag,scflag;
 
  flags = 0; 
  auth = 0;
  key = 0;
  jwflag = 0;
  scflag = 0;
  *passkey = 0;

  strcpy(prompt,"PAIRING and SECURITY\n");
  strcpy(selectlist,"  0 = Random passkey or Just Works or None\n");
  strcat(selectlist,"  1 = Fixed Passkey set by this device\n");
  strcat(prompt,selectlist);
  strcat(prompt,"Input option");      
  key = inputint(prompt);
  
  if(key <  0)
     return(-1);
  
  if(key == 1)
    {
    *passkey = inputint("Fixed 6-digit passkey");
    if(*passkey < 0)
      {
      *passkey = 0;
      return(-1);
      }
    }
  else
    *passkey = 0;

  strcpy(prompt,"AUTHENTICATION\n");
  strcat(prompt,"  If authentication is enabled, the client must connect\n");
  strcat(prompt,"  with passkey security or this server will not\n");
  strcat(prompt,"  allow characteristic reads/writes\n");
  strcpy(selectlist,noyes);
  strcat(prompt,"Enable authentication 0=No 1=Yes");    
  auth = inputint(prompt);

  if(auth <  0)
     return(-1);

  strcpy(prompt,"PASSKEY SECURITY\n");
  strcpy(selectlist,"  0 = No - Ask for Just Works\n");
  strcat(selectlist,"  1 = Yes - Accept Passkey\n");
  strcat(prompt,selectlist);
  strcat(prompt,"Accept Passkey security if the client asks");
  jwflag = inputint(prompt);
  
  if(jwflag < 0)
    return(-1);   

  /******* uncomment for secure connect option 
  strcpy(prompt,"SECURE CONNECTION\n");
  strcpy(selectlist,"  0 = No - Legacy pairing\n");
  strcat(selectlist,"  1 = Yes - Secure Connection pairing\n");
  strcat(prompt,selectlist);
  strcat(prompt,"Accept secure connection if the client asks");   
  scflag = inputint(prompt);

  if(scflag < 0)
    return(-1);   
  ******/
  
  if(key == 1)
    flags = PASSKEY_FIXED;
  if(auth == 1)
    flags |= AUTHENTICATION_ON; 
  if(jwflag == 0)
    flags |= JUST_WORKS;
  if(scflag == 1)
    flags |= SECURE_CONNECT;
        
  return(flags);
  }  



int mesh_callback(int clientnode,unsigned char *buf,int nread)
  {
  int n;
  char byts[8];
  
  sprintf(temps,"Mesh packet from %s\n",device_name(clientnode));
  print(temps);
  temps[0] = 0;
  for(n = 0 ; n < nread ; ++n)
    {
    sprintf(byts," %02X",buf[n]);
    strcat(temps,byts);
    }
    
  strcat(temps,"\n");
  print(temps);
  
  if(buf[0] == 'D')
    {
    print("Disconnect\n");
    return(SERVER_EXIT);  // server exits
    }
    
  return(SERVER_CONTINUE);  // server loops for another packet
  }    
    
int le_callback(int clientnode,int operation,int cticn)
  {
  int n,nread;
      
  if(operation == LE_CONNECT)
    {
    sprintf(temps,"  %s has connected\n",device_name(clientnode));
    print(temps);
    }
  else if(operation == LE_READ)
    {
    sprintf(temps,"  %s has read local characteristic %s\n",device_name(clientnode),ctic_name(localnode(),cticn));
    print(temps);
    }
  else if(operation == LE_WRITE)
    {
    // read local characteristic that client has just written
    nread = read_ctic(localnode(),cticn,tempu,sizeof(tempu));
    sprintf(temps,"  %s has written local characteristic %s =",device_name(clientnode),ctic_name(localnode(),cticn));
    print(temps);
    for(n = 0 ; n < nread ; ++n)
      {
      sprintf(temps," %02X",tempu[n]);
      print(temps);
      }
    print("\n");
    }
  else if(operation == LE_DISCONNECT)
    {
    sprintf(temps,"  %s has disconnected - waiting for another connection\n",device_name(clientnode));
    print(temps);
    // uncomment next line to stop LE server when client disconnects
    // return(SERVER_EXIT);
    // otherwise LE server will continue and wait for another connection
    // or opeeration from clients that are still connected
    }
  else if(operation == LE_TIMER)
    {
    print("  Timer\n");
    }
  else if(operation == LE_KEYPRESS)
    {   // cticn is key code
    sprintf(temps,"   Key code = %d\n",cticn);
    print(temps);
    }    
  return(SERVER_CONTINUE);
  }



int universal_callback(int clientnode,int operation,int cticn,unsigned char *buf,int nread)
  {
  int n,k,flag,nreadle;
  char firstc,*s;
  
  if(operation == LE_CONNECT)
    {
    sprintf(temps,"  %s has connected\n",device_name(clientnode));
    print(temps);
    }
  else if(operation == LE_READ)
    {
    sprintf(temps,"  %s has read local characteristic %s\n",device_name(clientnode),ctic_name(localnode(),cticn));
    print(temps);
    }
  else if(operation == LE_WRITE)
    {
    // read local characteristic that client has just written
    nreadle = read_ctic(localnode(),cticn,tempu,sizeof(tempu));
    sprintf(temps,"  %s has written local characteristic %s =",device_name(clientnode),ctic_name(localnode(),cticn));
    print(temps);
    for(n = 0 ; n < nreadle ; ++n)
      {
      sprintf(temps," %02X",tempu[n]);
      print(temps);
      }
    print("\n");
    }
  else if(operation == LE_DISCONNECT)
    {
    sprintf(temps,"  %s has disconnected - waiting for another connection\n",device_name(clientnode));
    print(temps);
    // uncomment next line to stop LE server when client disconnects
    // return(SERVER_EXIT);
    // otherwise LE server will continue and wait for another connection
    // or opeeration from clients that are still connected
    }
  else if(operation == SERVER_TIMER)
    {
    print("  Timer\n");
    }      
  else if(operation == CLASSIC_DATA)
    {
    if(buf[0] == endchar || (nread == 2 && buf[0] != 0 && buf[1] == endchar) || 
           (nread == 3 && buf[0] != 0 && buf[1] <= 13 && buf[2] == endchar))
      firstc = buf[0];  // is a single char
    else
      firstc = 0;       // more than one char - not a single char command  
 
    if(nread != 0)
      {   
      for(k = 0 ; k < nread ; ++k)
        {   // strip non-ascii chars for print
        if(k == nread-1 && buf[k] == endchar)
          buf[k] = 0;  // wipe endchar
        else if(buf[k] < 32 || buf[k] > 126)
          buf[k] = '.';
        }   
      sprintf(temps,"Recvd from %s: ",device_name(clientnode));
      print(temps);
      print((char*)buf);
      print("\n");  
      }
      
    // check if received string is a known command 
    // and send reply to client
          
    if(firstc == endchar || firstc == 'p')
      {    
      print("Ping\n");
      sendstring(clientnode,"OK");  // REPLY 2=add endchar   
      } 
    else if(firstc == 'D')
      {
      print("Disconnect\n");
      sendstring(clientnode,"Server disconnecting");
      }
    else
      {
      print("No action\n");
      sendstring(clientnode,"Unknown command - no action");    
      }
    
    if(firstc == 'D') 
      disconnect_node(clientnode);  
    }
  return(SERVER_CONTINUE);    // loop for another packet
  }


int node_classic_callback(int clientnode,unsigned char *buf,int nread)
  {
  int n,k,flag;
  char firstc,*s;
      
  if(buf[0] == endchar || (nread == 2 && buf[0] != 0 && buf[1] == endchar) || 
         (nread == 3 && buf[0] != 0 && buf[1] <= 13 && buf[2] == endchar) )
    firstc = buf[0];  // is a single char
  else
    firstc = 0;       // more than one char - not a single char command  
 
  if(nread != 0)
    {   
    for(k = 0 ; k < nread ; ++k)
      {   // strip non-ascii chars for print
      if(k == nread-1 && buf[k] == endchar)
        buf[k] = 0;  // wipe endchar
      else if(buf[k] < 32 || buf[k] > 126)
        buf[k] = '.';
      }   
    sprintf(temps,"Recvd from %s: %s\n",device_name(clientnode),buf); 
    print(temps); 
    }
      
  // check if received string is a known command 
  // and send reply to client
          
  if(firstc == endchar || firstc == 'p')
    {    
    print("Ping\n");
    sendstring(clientnode,"OK");  // REPLY 2=add endchar   
    }
  else if(firstc == 'D')
    {
    print("Disconnect\n");
    sendstring(clientnode,"Server disconnecting");
    }
  else
    {
    print("No action\n");
    sendstring(clientnode,"Unknown command - no action");    
    }
    
  if(firstc == 'D')
    return(SERVER_EXIT);   // stop node server and initiate disconnection
 
  return(SERVER_CONTINUE);    // loop for another packet
  }


/*********** CONNECT/DISCONNECT *********/

int clientconnect()
  {
  int node,channel,method,retval,contype,wait;
  int pairflags,passkey,pairwait,curwait,lesecurity;
      
     // only disconnected devices
  node = inputnode("Connect to a server",BTYPE_CL | BTYPE_LE | BTYPE_DISCONNECTED | BTYPE_ME);

  if(node < 0)
    {
    prcancel();
    return(0);
    }
   
  channel = 0;
  method = 0;
  passkey = 0;
  pairflags = 0;
  pairwait = 2000;
  wait = 0;
   
  if(device_type(node) == BTYPE_ME)
    {
    strcpy(prompt,"TYPE OF btferret SERVER\n");
    strcpy(selectlist,"  0 = Node server\n");
    strcat(selectlist,"  1 = Classic server\n");
    strcat(selectlist,"  2 = LE server\n");
    strcat(prompt,selectlist);
    strcat(prompt,"Input listening type of remote btferret device");
    contype = inputint(prompt); 
    }
  else
    contype = 0;
   
  if(contype < 0)
    {
    prcancel();
    return(0);
    }

  if(device_type(node) == BTYPE_CL)
    {  // classic server needs method = CHANNEL_STORED
       // or method = CHANNEL_NEW and channel  
    if(inputchan(node,&channel,&method) < 0)
      {
      prcancel();
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
      print("Invalid listening type\n");
      return(0);
      }
    }
  else if(device_type(node) == BTYPE_LE)
    method = CHANNEL_LE;   
  else
    {  
    print("Invalid device type\n");
    return(0);
    }
    
  if(method == CHANNEL_LE)
    {
    strcpy(prompt,"PAIRING and SECURITY\n");
    strcat(prompt,"  Most LE servers do not need security\n");
    strcat(prompt,"  so this option is not necessary\n");
    strcat(prompt,"Enable security options 0=No 1=Yes");
    lesecurity = inputint(prompt);

    if(lesecurity < 0)
      return(0);
    if(lesecurity > 1)
      lesecurity = 1;
  
    if(lesecurity == 1)
      {
      pairflags = clientsecurity(&passkey);
      if(pairflags < 0)
        return(0);
      }       
     
    curwait = set_le_wait(READ_WAIT);  
    strcpy(prompt,"CONNECTION COMPLETE TIME\n");
    strcat(prompt,"  After connecting, some LE servers need more time to complete\n");
    strcat(prompt,"  the process or they will disconnect. Zero may work, otherwise\n");
    strcat(prompt,"  find the shortest time that will prevent disconnection.\n");
    sprintf(tempc,"  Current value=%d  Default value=750\n",curwait);
    strcat(prompt,tempc);
    strcat(prompt,"Time in ms");
    wait = inputint(prompt);

    if(wait < 0)
      return(0);
    if(wait >= 0)
      set_le_wait(wait);
    if(pairflags != 0)
      {
      strcpy(prompt,"PAIRING COMPLETE TIME\n");
      strcat(prompt,"This might include the time to enter a passkey\n");
      strcat(prompt,"or pairing will fail with a timeout\n");
      strcat(prompt,"Time in ms");
      pairwait = inputint(prompt);
      if(pairwait < 0)
        return(0);
      }   
    }
  
  retval = connect_node(node,method,channel);

    
  if(pairflags != 0)
    {
    set_le_wait(pairwait);
    le_pair(node,pairflags,passkey);
    }
    
  return(retval);    
  }


int clientsecurity(int *passkey)
  {
  int flags,pair,keydev,bond,scflag;
  int bondlook[3] = { 0,BOND_NEW,BOND_REPAIR }; 
  int flaglook[4] = { 0,JUST_WORKS,PASSKEY_FIXED,PASSKEY_RANDOM };
  int devlook[2] = { PASSKEY_LOCAL,PASSKEY_REMOTE };
   
  flags = 0; 
  pair = 0;
  keydev = 0;
  scflag = 0;
  *passkey = 0;
  
  strcpy(prompt,"BONDING  (save pairing info)\n");
  strcpy(selectlist,"  0 = Do not bond - OK for most servers\n");
  strcat(selectlist,"  1 = New bond\n");
  strcat(selectlist,"  2 = Re-Pair with a previously bonded device\n");
  strcat(prompt,selectlist);
  strcat(prompt,"Bond");
  bond = inputint(prompt);

  if(bond < 0)
    return(-1);
  if(bond > 2)
    {
    print("Invalid option\n");
    return(-1);
    }
  flags |= bondlook[bond];
  if(bond == 2)
    return(flags);
   
  strcpy(prompt,"PAIRING and SECURITY\n");
  strcpy(selectlist,"  0 = Do not pair - OK for most servers\n");
  strcat(selectlist,"  1 = Pair - Just Works\n");
  strcat(selectlist,"  2 = Pair - Fixed Passkey\n");
  strcat(selectlist,"  3 = Pair - Random Passkey\n");
  strcat(prompt,selectlist);
  strcat(prompt,"Input pair option");  
  pair = inputint(prompt);
  
  if(pair <  0)
     return(-1);
   
  if(bond == 1 && pair == 0)
    {
    print("Must pair for new bond\n");
    return(-1);
    }
  
  if(pair > 3)
    {
    print("Invalid option\n");
    return(-1);
    }  
  
  if(pair == 0)
    return(0);
    
  if(pair == 2 || pair == 3)
    {  
    strcpy(prompt,"PASSKEY chosen by\n");
    strcpy(selectlist,"  0 = This local device\n");
    strcat(selectlist,"  1 = Remote server\n");
    strcat(prompt,selectlist);
    strcat(prompt,"Enter 0/1");
    keydev = inputint(prompt);

    if(keydev < 0)
      return(-1);
    if(keydev > 1)
      {
      print("Invalid option\n");
      return(-1);
      }
    }
     
  if(pair == 2)
    {
    *passkey = inputint("Fixed 6-digit passkey");
    
    if(*passkey < 0)
      {
      *passkey = 0;
      return(-1);
      }
    if(*passkey > 999999)
      {
      *passkey = 0;
      print("Too big\n");
      return(-1);
      }
    }
  else
    *passkey = 0;

  flags |= flaglook[pair] | devlook[keydev]; 

  /******* uncomment for secure connect option 
  strcpy(prompt,"SECURE CONNECTION\n");
  strcpy(selectlist,"  0 = No - Legacy pairing\n");
  strcat(selectlist,"  1 = Yes - Secure Connection pairing\n");
  strcat(prompt,selectlist);
  strcat(prompt,"Accept secure connection if the client asks");   
  scflag = inputint(prompt);
  *******/
 
  if(scflag != 0)
    flags |= SECURE_CONNECT;
 
    
  return(flags);
  }  

void localdisconnect()
  {
  int node;
  
  print("\nUse D instead to disconnect btferret node and mesh servers\n");
     
      // only connected devices
  node = inputnode("Disconnect",BTYPE_CL | BTYPE_LE | BTYPE_ME | BTYPE_CONNECTED);
  if(node < 0)
    {
    prcancel();
    return;
    }   

  disconnect_node(node);
  }


/************* SETTINGS ************/


void settings()
  {
  int valn;

  strcpy(prompt,"PRINT options\n");
  strcpy(selectlist,"  1 = Normal\n");
  strcat(selectlist,"  2 = Verbose - all HCI traffic\n");
  strcat(selectlist,"  0 = None\n");
  strcat(prompt,selectlist);
  strcat(prompt,"Input option"); 
  valn = inputint(prompt);

  if(valn >= 0)
    {
    if(valn == 0)
      valn = PRINT_NONE;
    else if(valn == 1)
      valn = PRINT_NORMAL;  
    else if(valn == 2)
      valn = PRINT_VERBOSE;  
    else
      print("Invalid option\n");
  
    if(valn < 3)
      set_print_flag(valn);
    }
  else
    prcancel();
  }
  
  

void readnotify()
  {
  int timeout;
  
  timeout = 10;
  strcpy(prompt,"Read notifications\nInput time out in s");    
  timeout = inputint(prompt);

  if(timeout < 0)
    {
    prcancel();
    return;
    }

  print("Reading... (x = stop)\n"); 
  read_notify(timeout*1000);
   
  print("Read notifications finished\n");
  return;
  }
  
  
/*********** READ SERVICES *************/

  
void readservices()
  {
  int node;
  
  node = inputnode("Read services",BTYPE_CL | BTYPE_LE | BTYPE_ME | BTYPE_LO);
  if(node < 0)
    return;

  if(device_type(node) != BTYPE_CL)  // pure classic devices cannot have LE characteristics
    find_ctics(node);
  if(device_type(node) != BTYPE_LE)  // pure LE devices cannot have classic serial channels
    list_channels(node,LIST_FULL);
  
  return;
  }
  
void readlehandles()
  {
  int node;
  
  node = inputnode("Read LE handles",BTYPE_CONNECTED | BTYPE_LE | BTYPE_ME);
  if(node < 0)
    return;
  le_handles(node,0);
  }  
  

/************** SEND ASCII STRING **************************
comd = zero terminated ascii string - not binary data
add endchar to string
************************************************/

int sendstring(int node,char *comd)
  {
  int clen,retval;
  char *s;

                
  clen = strlen(comd); 
  if(clen > 999)
    {
    print("String too long\n");
    return(0);
    }
   
  if(clen == 0)
    s = "(endchar only)";
  else
    s = comd;
     
  sprintf((char*)sbuff,"Sending to %s: %s\n",device_name(node),s);
  print((char*)sbuff);
  // add endchar   
  strcpy((char*)sbuff,comd);
  sbuff[clen] = endchar;
  ++clen;
     
  retval = write_node(node,sbuff,clen);
      
  if(retval == clen)
    return(1);

  return(0);
  }

 
/********* MESH BROADCAST **********/

int meshsend()
  {
  int len;
  
  strcpy(prompt,"Broadcast a string to all mesh devices\n");
  strcat(prompt,"Input string (max 25 chars)");

  printf("%s\n",prompt);
  getstring("? ",tempc,64);

  len = strlen(tempc); 
  if(len > 25)
    {
    print("String too long\n");
    return(0);
    }
    
  write_mesh((unsigned char*)tempc,len);
  return(1);
  }

/********** READ/WRITE LE CHARACTERISTIC ****************/

void readle()
  {
  int n,node,cticn,ascflag,datlen;
 
  node = inputnode("Read an LE characteristic",BTYPE_LE | BTYPE_ME | BTYPE_CONNECTED | BTYPE_LO);   // only connected LE devices
  if(node < 0)
    {
    prcancel();
    return;
    }
   
  n = list_ctics(node,LIST_SHORT | CTIC_R);
  
  if(n == 0)
    {
    print("No readable characteristics. Read services to find\n");
    return;
    }

  cticn = inputint("Input ctic index");
 
  if(cticn < 0)
    return;
       
  datlen = read_ctic(node,cticn,tempu,sizeof(tempu));
  
  if(datlen == 0)
    return;     // fail or no data
  
  sprintf(temps,"%s %s = ",device_name(node),ctic_name(node,cticn));
  print(temps);
  
  ascflag = 0;
  if(datlen > 2)     
    ascflag = 1;
        
  for(n = 0 ; n < datlen ; ++n)
    {
    sprintf(temps,"%02X ",tempu[n]);
    print(temps);
    if(!(n == datlen-1 && tempu[n] == 0) && (tempu[n] < 32 || tempu[n] > 126))
      ascflag = 0;  // not an ascii string
    }
        
  if(ascflag == 0)
    print("\n");
  else
    {
    sprintf(temps,"= \"%s\"\n",tempu);  // print ascii - readctic has added term 0
                                      // at dat[datlen]
    print(temps);
    } 
  }

void writele()
  {
  int n,node,cticn,size;
  unsigned char *val;
  
  node = inputnode("Write an LE characteristic",BTYPE_LE | BTYPE_ME | BTYPE_CONNECTED | BTYPE_LO);   // only connected LE devices
  if(node < 0)
    {
    prcancel();
    return;
    }
  
  n = list_ctics(node,LIST_SHORT | CTIC_W);
  
  if(n == 0)
    {
    print("No writeable characteristics. Read services to find\n");
    return;
    }
   
  cticn = inputint("Input ctic index");

  if(cticn < 0)
    return;    
   
  strcpy(prompt,"Input data bytes in hex e.g. 5A 43 01");

  printf("%s  (x=cancel)\n",prompt);
  getstring("? ",tempc,128);

  if(tempc[0] == 'x' && tempc[1] == 0)
    return;
        
  val = strtohex(tempc,&size);
 
  write_ctic(node,cticn,val,size);
   
  }


void notifyle()
  {
  int node,cticn,flag;
  char *s;

  node = inputnode("Enable/Disable LE characteristic notify/indicate",BTYPE_LE | BTYPE_ME | BTYPE_CONNECTED);
  if(node < 0)
    {
    prcancel();
    return;
    }
  
  flag = list_ctics(node,LIST_SHORT | CTIC_NOTIFY);
  
  if(flag == 0)
    {
    print("No notify/indicate characteristics. Read services to find\n");
    return;
    }
  
  cticn = inputint("Input ctic index");
  
  if(cticn < 0)
    return; 

  if(ctic_ok(node,cticn) == 0)
    {
    print("Invalid index\n");
    return;
    }

  flag = inputint("0=Disable 1=Enable");

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
  
  sprintf(temps,"%s %s %s\n",device_name(node),ctic_name(node,cticn),s);
  print(temps);
  
  return;
  }   


int notify_callback(int lenode,int cticn,unsigned char *buf,int nread)
  {
  int n;
   
  // LE device has sent notification or indication enabled by notify_ctic()
  
  sprintf(temps,"%s %s notify =",device_name(lenode),ctic_name(lenode,cticn));
  print(temps);
  for(n = 0 ; n < nread ; ++n)
    {
    sprintf(temps," %02X",buf[n]);
    print(temps);
    }
  print("\n");
  return(0);
  }

/********** USER INPUT FUNCTIONS *******/
 
int inputnode(char *pr,int mask)
  {
  int n,node,flag;

  printf("%s\nAVAILABLE DEVICES\n",pr);
  n = device_info(mask | BTYPE_SHORT);

  if(n == 0)
    {
    print("No suitable devices available\n");
    return(-1);
    }    

  do
    {
    flag = 0;

    node = inputint("Input node");

    if(node < 0)
      return(-1);   // cancel
    if(node == 0 && (mask & (BTYPE_ANY | BTYPE_SERME)) != 0)
      flag = 1;
    else if((device_type(node) & mask) != 0)
      flag = 1;
    else
      print("Invalid node\n");
    }
  while(flag == 0);

  return(node);
  }


/******* INPUT CHANNEL *******/

int inputchan(int node,int *channel,int *method) 
  {
  int n;

  strcpy(prompt,"RFCOMM channel\n");
  strcpy(selectlist,"  0 = Reconnect/Use stored channel number\n");
  strcat(selectlist,"  1 = Input channel number\n  2 = Read services to choose channel\n");

  strcat(prompt,selectlist);
  strcat(prompt,"Input option 0-2");
  n = inputint(prompt);
  
  if(n < 0)
    return(-1);
  
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
      print("Failed to find RFCOMM channels\n");
      return(-1);
      }
    }
    
  strcpy(prompt,"Input RFCOMM channel number");
   
  *channel = inputint(prompt);

  if(*channel < 0)
    return(-1);
    
  return(1);
  }
    

void prcancel()
  {
  print("Cancelled\n");
  }



void print(char *s)
  {
  printf("%s",s);
  }

/***** INPUT INTEGER ********/     
     
int inputint(char *ps)
  {
  int n,flag;
  char s[32];
 
  do
    {
    printf("%s  (x=cancel)\n",ps);
  
    getstring("? ",s,32);
       
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
    read_usb_endchar(s,len,10,0);
    }
  while(s[0] == 10);

  n = 0;
  while(s[n] != 10 && s[n] != 13 && s[n] != 0)
    ++n;
  s[n] = 0;
  }  
  
