
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
  char cmds[64];
  
  printf("h = help\n");
    
  do
    {
    // wait for command char
    getstring("> ",cmds,64);
        
    cmd = cmds[0];
    
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
             
      case 'f':
        sendgetfile();
        break;

      case '[':
        scroll_back();
        break;
        
      case ']':
        scroll_forward();
        break;
        
      case 'o':
        output_file("btout.txt");
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

      case 'y':
        readuuid();
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
             
      case 'g':
        regserial();
        break;

      case 'R':
        readnotify();
        break;
        
      case 'm':
        mesh_on();
        printf("Mesh on\n");
        break;
        
      case 'n':
        mesh_off();
        printf("Mesh off\n");
        break;
                                
      case 'q':
        return;
        break;

      
      default:
        printf("Unknown command\n");
        break;       
      }
    }
  while(1); 
  
  }
  
void printhelp()
  {
  printf("\n  HELP\n");
  printf("  a Scan for Classic devices    i Print device info\n");
  printf("  b Scan for LE/Mesh devices    k Settings\n");   
  printf("  c Connect to a node           p Ping server\n");
  printf("  t Send string to server       T Send string to mesh\n");
  printf("  r Read LE characteristic      w Write LE characteristic\n");
  printf("  d Disconnect                  D Tell server to disconnect\n");  
  printf("  s Become a listening server   f File transfer (send or get)\n");
  printf("  v Read node services          y Read specified UUID service\n");
  printf("  o Save screen output to file  g Register custom serial UUID\n");  
  printf("  j LE notify/indicate on/off   R Read LE notifications\n");                     
  printf("  m Mesh transmit on            n Mesh transmit off\n");                     
  printf("  q Quit                        [] Scroll screen back/forward\n"); 
  }


/*********** CLIENT READ/SEND **********/

int clientread(int node)
  {
  int n,gotn;
  unsigned char buf[1024];
  
   // read to endchar 3s time out
  gotn = read_node_endchar(node,buf,1024,endchar,EXIT_TIMEOUT,3000);

  if(gotn > 0)
    {
    for(n = 0 ; n < gotn && n < 1023 ; ++n)
      { 
      if(n == gotn-1 && buf[n] == endchar)
        buf[n] = 0;  // wipe terminate char          
      else if(buf[n] < 32 || buf[n] > 127)
        buf[n] = '.';            
      }
    // read has set buf[gotn] = 0
    printf("Reply from %s: %s\n",device_name(node),buf);
    return(1);
    }
  printf("No reply from %s\n",device_name(node));
  return(0);
  }


int clientsend(int cmd)
  {
  int flag,node;
  char coms[256];
  
  if(cmd == 'D')
    flag = 1;  // all mesh servers option
  else
    flag = 0;
  
  node = inputnode(BTYPE_CL | BTYPE_ME | BTYPE_CONNECTED,flag);   // only connected classic/mesh
        
  if(node < 0)
    {
    printf("Cancelled\n");
    return(0);
    }
    
  if(node == 0)   // mesh packet to all mesh servers
    {
    write_mesh((unsigned char*)"D",1);
    return(1);
    }  
    
    
  if(device_connected(node) == NO_CONN)
    {
    printf("Not connected\n");
    return(0);
    }  
  
  if(cmd == 'p' || cmd == 'D')
    printf("This command only works if connected to a btferret server\n");
                 
  
  coms[0] = 0;
  coms[1] = 0;
  if(cmd == 't')
    { 
    printf("Will add endchar = %d\n",endchar); 
    getstring("Input string to send: ",coms,128);
    } 
  else if(cmd == 'p')
    {
    coms[0] = 0;  // empty string - will add endchar
    printf("Ping server\n");
    }
  else if(cmd == 'D')
    {
    coms[0] = 'D';
    printf("Tell server to disconnect\n");
    }
  else
    {
    printf("Unknown command\n");
    return(0);
    }
      
  if(sendstring(node,coms) != 0)   
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
  int serverflag,clinode,keyflag,inkey,timeds;
   
  printf("\n  0 = node server\n  1 = classic server\n  2 = LE server\n  3 = mesh server\n");
  serverflag = inputint("Input server type 0/1/2/3");
  if(serverflag < 0)
    return(0);
  if(serverflag == 3)   // Mesh
    mesh_server(mesh_callback);
  else if(serverflag == 2)
    {   // LE
    printf("Input LE_TIMER interval in deci (0.1) seconds\n   0 = No LE_TIMER calls\n  10 = One second interval\n  50 = Five second interval etc...\n");
    timeds = inputint("Timer interval");
    if(timeds < 0)
      return(0);
    keyflag = inputint("Send key presses to LE_KEYPRESS callback 0=No 1=Yes");  
    if(keyflag < 0)
      return(0);
    if(keyflag == 0)
      keys_to_callback(KEY_OFF,0);
    else
      keys_to_callback(KEY_ON,0);  
    le_server(le_callback,timeds);
    }
  else if(serverflag == 0 || serverflag == 1)
    {  // node or classic
    printf("\nInput node of client that will connect\n");
   
    if(serverflag == 0)
      clinode = inputnode(BTYPE_ME | BTYPE_CL,0);  
    else
      clinode = inputnode(BTYPE_ME | BTYPE_CL,2);  
    if(clinode < 0)
      {
      printf("Cancelled\n");
      return(0);
      }
    if(serverflag == 0)   // node
      node_server(clinode,node_classic_callback,endchar);
    else
      {  // classic
      if(clinode != 0 && device_type(clinode) == BTYPE_ME)
        keyflag = KEY_OFF | PASSKEY_OFF;
      else
        {
        printf("\nClient's security  (0 or 1 to pair or connect Android/Windows.. clients)\n");
        printf("  0 = Use link key, print passkey here, remote may ask to confirm\n");
        printf("  1 = No link key,  print passkey here (forces re-pair if pairing fails)\n");
        printf("  2 = No keys  (connecting client is another mesh Pi)\n");
        printf("  3 = Use link key, no passkey\n");
        printf("  4 = Use link key, remote prints passkey, enter it here if asked\n");
        inkey = inputint("\nClient's security requirement");
   
        if(inkey < 0 || inkey > 4)
          {
          printf("Cancelled or invalid entry\n");
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
                 
      printf("\nServer will listen on channel 1 and any of the following UUIDs\n");
      printf("  Standard serial 2-byte 1101\n");
      printf("  Standard serial 16-byte\n");
      printf("  Custom serial set via register serial\n");
      if(clinode == 0)
        clinode = ANY_DEVICE;
      classic_server(clinode,node_classic_callback,endchar,keyflag);
      }
    }   
   else
    printf("Invalid type\n");
  return(0);
  }  
 
int mesh_callback(int clientnode,unsigned char *buf,int nread)
  {
  int n;
  
  printf("Mesh packet from %s\n",device_name(clientnode));
  for(n = 0 ; n < nread ; ++n)
    printf(" %02X",buf[n]);
  printf("\n");
  
  if(buf[0] == 'D')
    {
    printf("Disconnect\n");
    return(SERVER_EXIT);  // server exits
    }
    
  return(SERVER_CONTINUE);  // server loops for another packet
  }    
    
int le_callback(int clientnode,int operation,int cticn)
  {
  int n,nread;
  unsigned char dat[256]; 
    
  if(operation == LE_CONNECT)
    printf("  %s has connected\n",device_name(clientnode));
  else if(operation == LE_READ)
    printf("  %s has read local characteristic %s\n",device_name(clientnode),ctic_name(localnode(),cticn));
  else if(operation == LE_WRITE)
    {
    // read local characteristic that client has just written
    nread = read_ctic(localnode(),cticn,dat,sizeof(dat));
    printf("  %s has written local characteristic %s =",device_name(clientnode),ctic_name(localnode(),cticn));
    for(n = 0 ; n < nread ; ++n)
      printf(" %02X",dat[n]);
    printf("\n");
    }
  else if(operation == LE_DISCONNECT)
    {
    printf("  %s has disconnected - waiting for another connection\n",device_name(clientnode));
    // uncomment next line to stop LE server when client disconnects
    // return(SERVER_EXIT);
    // otherwise LE server will continue and wait for another connection
    // or opeeration from clients that are still connected
    }
  else if(operation == LE_TIMER)
    {
    printf("  Timer\n");
    }
  else if(operation == LE_KEYPRESS)
    {   // cticn is key code
    printf("   Key code = %d\n",cticn);
    }    
  return(SERVER_CONTINUE);
  }


int node_classic_callback(int clientnode,unsigned char *buf,int nread)
  {
  int n,k,flag;
  char firstc,*s,temps[256];
  static char destdir[256] = {""};
  static int nblock = 400;
      
  if(buf[0] == endchar || (nread == 2 && buf[0] != 0 && buf[1] == endchar) || 
         (nread == 3 && buf[0] != 0 && buf[1] <= 13 && buf[2] == endchar) ||
         buf[0] == 'F' || buf[0] == 'X' || buf[0] == 'Y' || buf[0] == 'G')
    firstc = buf[0];  // is a single char or FXYG file transfer
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
    printf("Recvd from %s: %s\n",device_name(clientnode),buf);  
    }
      
  // check if received string is a known command 
  // and send reply to client
          
  if(firstc == endchar || firstc == 'p')
    {    
    printf("Ping\n");
    sendstring(clientnode,"OK");  // REPLY 2=add endchar   
    }
  else if(firstc == 'D')
    {
    printf("Disconnect\n");
    sendstring(clientnode,"Server disconnecting");
    }
  else if(firstc == 'F')
    {  // receive file      
    // command = Ffilename  endchar stripped 
    receivefile((char*)&buf[1],clientnode);
    }
  else if(firstc == 'X' && nread > 1)
    {  // destination directory for get file
    s = (char*)(buf+1);
    printf("Destination directory for GET file = %s\n",s);
    n = 0;
    while(s[n] != 0 && n < nread && n < 255)
      {
      destdir[n] = s[n];
      ++n;
      }
    destdir[n] = 0;
    }
  else if(firstc == 'Y' && nread > 1)
    {  // nblock for get file
    k = 0;
    s = (char*)(buf+1);
    flag = 0;
    n = 0;
    while(s[n] != 0 && n < nread)
      {
      if(s[n] >= '0' && s[n] <= '9')
        k = (k*10) + (s[n] - '0');
      else
        flag = 1;  // error
      ++n;
      }
    if(flag == 0)
      {
      nblock = k; 
      printf("Block size for GET file = %d\n",nblock);
      }
    }
  else if(firstc == 'G' && nread > 1)
    { // get file request with file name
    s =  (char*)(buf+1);
    n = 0;
    while(s[n] != 0 && n < nread && n < 255)
      {
      temps[n] = s[n];
      ++n;
      }
    temps[n] = 0;
    printf("GET file %s\n",temps);
    sendfilex(clientnode,"F",temps,destdir,nblock,endchar);
    }
  else
    {
    printf("No action\n");
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
    
     // only disconnected devices
  node = inputnode(BTYPE_CL | BTYPE_LE | BTYPE_DISCONNECTED | BTYPE_ME,0);

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
    printf("  Current value=%d  (x=cancel to keep)  Default value=750\n",set_le_wait(READ_WAIT));
    wait = inputint("Time in ms");
    if(wait >= 0)
      set_le_wait(wait);   
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
  int node;
  
  printf("\nUse D instead to disconnect btferret node and mesh servers\n");
     
      // only connected devices
  node = inputnode(BTYPE_CL | BTYPE_LE | BTYPE_ME | BTYPE_CONNECTED,0);
  if(node < 0)
    {
    printf("Cancelled\n");
    return;
    }   

  disconnect_node(node);
  }


/************* SETTINGS ************/


void settings()
  {
  int valn;
  
  valn = inputint("PRINT options\n  0 = None\n  1 = Normal\n  2 = Verbose - all HCI traffic\nInput one of the above options");
  if(valn >= 0)
    {
    if(valn == 0)
      valn = PRINT_NONE;
    else if(valn == 1)
      valn = PRINT_NORMAL;  
    else if(valn == 2)
      valn = PRINT_VERBOSE;  
    else
      printf("Invalid option\n");
  
    if(valn < 3)
      set_print_flag(valn);
    }
  else
    printf("Invalid option\n");
  }
  
    
  
void xsettings()
  { 
  int valn;

          
  valn = inputint("PRINT options\n  0 = None\n  1 = Normal\n  2 = Verbose - all HCI traffic\nInput one of the above options");
  if(valn >= 0)
    {
    if(valn == 0)
      valn = PRINT_NONE;
    else if(valn == 1)
      valn = PRINT_NORMAL;  
    else if(valn == 2)
      valn = PRINT_VERBOSE;
    else
      printf("Invalid option\n");
  
    if(valn < 3)
      set_print_flag(valn);
    }
    
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
  
  node = inputnode(BTYPE_CL | BTYPE_LE | BTYPE_ME | BTYPE_LO,0);
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
    
 
/************* SEND/RECEIVE FILE *****************/


int sendgetfile()
  {
  int n,j,servernode,maxblock,xblock,retval,sorg,count;
  char ec,fname[256],ddir[256],list[512];
  char temps[256];
  static char ddirsav[256] = {""};
  static int nblock = 0;
 
  printf("File transfer\n  0 = SEND file\n  1 = GET file\n  2 = SEND multiple files\n"); 
  sorg = inputint("Input 0/1/2");
       
  if(sorg == 0)
    printf("\nSEND file\n");
  else if(sorg == 1)
    printf("\nGET file\n");
  else if(sorg == 2)
    printf("\nSEND multiple\n");
  else
    {
    printf("Cancelled\n");
    return(0);
    }
     
  servernode = inputnode(BTYPE_CONNECTED | BTYPE_ME | BTYPE_CL,0);       
  if(servernode < 0)
    {
    printf("Cancelled\n");
    return(0);
    }

  maxblock = 1000;        
  if(device_connected(servernode) == NO_CONN)
    {
    printf("Not connected\n");
    return(0);
    }
  else if(device_type(servernode) == BTYPE_CL)
    printf("*** NOTE *** Server must be programmed like file_server in btlib.c\n");
  else if(device_connected(servernode) == NODE_CONN)
    maxblock = 400;  // node connect max block size 
  
  if(nblock < 64 || nblock > maxblock)
    nblock = maxblock;

  if(sorg == 2)
    {
    count = inputlist(list,512);
    if(count < 1)
      {
      printf("Cancelled\n");
      return(0);
      }
    }
  else
    {
    printf("Enter file name e.g.  /home/pi/doc.txt  (x=cancel)\n");
    getstring("? ",fname,256);
        
    if((fname[0] == 'x' && fname[1] == 0) || fname[0] == ' ')
      {
      printf("Cancelled\n");
      return(0);
      }
    }

  if(ddirsav[0] != 0)
    {
    printf("Existing destination directory = %s\n",ddirsav);    
    printf("Enter destination directory  e.g.  /home/pi/  ( /=none, r=retain, x=cancel)\n");
    }
  else
    printf("Enter destination directory  e.g.  /home/pi/  ( /=none, x=cancel)\n");
    
  getstring("? ",ddir,256);

  if(ddir[0] == 'x' && ddir[1] == 0)
    {
    printf("Cancelled\n");
    return(0);
    } 

  if(ddirsav[0] != 0 && ddir[0] == 'r' && ddir[1] == 0)
    strcpy(ddir,ddirsav);
  else  
    strcpy(ddirsav,ddir);

  ec = ddir[strlen(ddir) - 1];
  if(ec != '/' && ec != '\\')
    {
    printf("Directory must end with / or \\\n");
    return(0);
    }
  
  if(ddir[0] == '/' && (ddir[1] == 0 || ddir[1] == ' '))
    {
    printf("None - will save to server's btferret directory\n");
    ddir[0] = 0;
    }

  if(ddirsav[0] != 0 && ddir[0] == 'r' && ddir[1] == 0)
    strcpy(ddir,ddirsav);
  else  
    strcpy(ddirsav,ddir);
    
  printf("BLOCK SIZE = %d bytes\n",nblock);
  printf("  Data is transmitted in blocks of this size\n");
  printf("  Enter x to keep, or enter new value 64-%d\n",maxblock);
  xblock = inputint("Block size");
  if(xblock >= 64 && xblock < maxblock)
    {
    nblock = xblock;         
    printf("Block size changed to %d\n",nblock);
    }
  else
    printf("Block size not changed\n");
 
  // server must use the same opcode (F)
  if(sorg == 2)
    {
    printf("Sending %d files\n",count);
    j = 0;
    for(n = 0 ; n < count ; ++n)
      {
      printf("%d. %s\n",n+1,list+j);
      retval = sendfilex(servernode,"F",list+j,ddir,nblock,endchar);
      while(list[j] != 0)
        ++j;
      ++j;
      } 
    }
  else if(sorg == 0) 
    retval = sendfilex(servernode,"F",fname,ddir,nblock,endchar);
  else
    {  
    // get file - send destination directory
    temps[0] = 'X';
    temps[1] = 0;
    strcat(temps,ddir);
    sendstring(servernode,temps);
    // send nblock
    temps[0] = 'Y';
    sprintf(temps+1,"%d",nblock);
    sendstring(servernode,temps);
    // send GET file command
    temps[0] = 'G';
    temps[1] = 0;
    strcat(temps,fname);
    sendstring(servernode,temps);
    // server sends file
    retval = read_node_endchar(servernode,(unsigned char*)temps,256,endchar,EXIT_TIMEOUT,5000);
    if(retval != 0 && temps[0] == 'F')
      {
      // strip endchar
      if(temps[retval-1] == endchar)
        {
        temps[retval-1] = 0;
        retval = receivefile(temps+1,servernode);
        }
      else
        {
        printf("Timed out\n");
        retval = 0;
        }
      }
    else
      {
      printf("Server did not send file\n");
      retval = 0;
      }
    }
  return(retval);
  }



int sendfilex(int node,char *opcode,char *filename,char *destdir,int blocksize,int termchar)
  {
  FILE *stream;
  int flen,n,fn,ntogo,type,ndat,ncrc,ackflag,nblk,crchi,crclo;
  int getout,gotn,progflag,packn,maxblock,nblock;
  unsigned char temps[1024],buf[8];
  unsigned short crc;
  char *fname;
 
  crchi = 0;
  crclo = 0;
 
  fn = 0;  // after last / start of file name 
  n = 0;
  while(filename[n] > 32 && n < 1022)
    {  // strip non alpha
    if(filename[n] == '/')
      fn = n+1;  // start of file name
    ++n;
    }
    
  fname = filename+fn;
  
  printf("Sending file %s\n",filename);
       
  type = device_type(node);
  if(!(type == BTYPE_ME || type == BTYPE_CL) || device_connected(node) == NO_CONN)
    {
    printf("Invalid device type or not connected\n");
    return(0);
    }

  maxblock = 1000;        
  if(type == BTYPE_CL)
    maxblock = 1000;
  else if(device_connected(node) == NODE_CONN)
    maxblock = 400;  // node connect max block size 
  
  nblock = blocksize;
  if(nblock < 64 || nblock > maxblock)
    nblock = maxblock;
       
  
  if(fname[0] == 0 || opcode[0] == 0)
    {
    printf("No file name or opcode\n");
    return(0);
    } 
    
  if(destdir[0] != 0)
    {   
    n = 0;
    while(destdir[n] != 0)
      ++n;   
   
    if(n != 0)
      {
      if(!(destdir[n-1] == '/' || destdir[n-1] == '\\')) 
        {  // backslash allowed for Classic/Windows
        printf("Destination directory must end with / or \\\n");
        return(0);
        }
      }
    }  
                 
  stream = fopen(filename,"r");
  if(stream == NULL)
    {
    printf("File open error\n");
    return(0);
    }
 

  // clear all packets from input buffer
  read_node_clear(node);

  strcpy((char*)temps,opcode);   // send file command
  strcat((char*)temps,destdir);
  strcat((char*)temps,fname);  // add dir:file name
  flen = strlen((char*)temps);
  temps[flen] = termchar;
  ++flen;      
  // send command  Ffilespec + termchar
  if(write_node(node,temps,flen) != flen)
    {   
    fclose(stream);
    return(0);
    }

  // send 4 length bytes 2 block size bytes and checksum byte
  flen = filelength(stream);
  buf[0] = flen & 255;
  buf[1] = (flen >> 8) & 255;
  buf[2] = (flen >> 16) & 255;
  buf[3] = (flen >> 24) & 255;
  buf[4] = nblock & 255;
  buf[5] = (nblock >> 8) & 255;
  buf[6] = buf[0];
  for(n = 1 ; n < 6 ; ++n)
    buf[6] += buf[n];
      
  if(write_node(node,buf,7) != 7)
    {
    printf("Transmit fail\n");
    fclose(stream);
    return(0);
    }

       // wait for single ack byte
  gotn = read_node_count(node,buf,1,EXIT_TIMEOUT,5000);             
  if(gotn != 1 || buf[0] != 10)
    {
    printf("Not seen acknowledge from receiver\n");
    fclose(stream);
    return(0);
    }
    
  progflag = 0;  // no print progress
  if(flen > 5000)
    {  // every 10 packets 
    progflag = 1;
    printf("Progress");
    fflush(stdout);
    }
    
 
  crc = 0xFFFF;
     
  ntogo = flen+2;  // do loop counter  
  getout = 0;
  packn = 0;   // number of packets sent
  ackflag = 0;
  nblk = nblock;

  do
    {
    if(ntogo < nblock)
      nblk = ntogo;  // last block size
      
    if(ackflag != 0)
      {  // just sent nblk       
         //  wait for ack = 10 from receiving device
         //  single char read + time out
      buf[0] = 0;
                // wait for single ack byte 5s time out
      gotn = read_node_count(node,buf,1,EXIT_TIMEOUT,5000);                
       
      if(gotn != 1 || buf[0] != 10)
        {
        getout = 1;
        printf("Not seen acknowledge from receiver\n");
        printf("Transmission may be slow for unknown reason\n");
        printf("Try reducing block size\n");
        }
      }
      
    if(getout == 0)  // ack wait may have timed out
      {
      if(ntogo <= 2)
        {
        ncrc = ntogo;
        ndat = 0;
        }
      else
        {  
        ncrc = nblk - ntogo + 2;
        if(ncrc < 0)
          ncrc = 0;
        ndat = nblk - ncrc;
        if(ndat < 0)
          ndat = 0;
        }              
    
      if(ndat > 0)
        {
        if(fread(temps,1,ndat,stream) != ndat)
          getout = 2;
        else
          { 
          crc = crccalc(crc,temps,ndat);
          ntogo -= ndat;
          }
        }  
     
      if(getout == 0)
        {   
        if(ncrc == 2)
          {
          temps[ndat] = (unsigned short)((crc >> 8) & 255);
          crchi = temps[ndat];
          temps[ndat+1] = (unsigned short)(crc & 255);
          crclo = temps[ndat+1];
          ntogo -= 2;
          }
        else if(ncrc == 1)
          {
          if(ndat == 0)
            {
            temps[0] = (unsigned short)(crc & 255);
            crclo = temps[0];
            }
          else
            {
            temps[ndat] = (unsigned short)((crc >> 8) & 255);
            crchi = temps[ndat];
            }
          --ntogo;
          }     

        // send nblk bytes
       
        if(write_node(node,temps,nblk) != nblk)
          getout = 1;
        
       
        ackflag = 1; 
        ++packn;
        if(progflag != 0 && packn % 10 == 0)
          {
          printf(".");
          fflush(stdout);
          } 
        }      
      } // end retval==1
    }   // end block loop
  while(getout == 0 && ntogo > 0);
  
 
  if(progflag != 0)
    printf("\n");  
  fclose(stream);

  if(getout != 0)
    {  // error may have left ack in buffer
    read_node_clear(node);
    if(getout == 1)
      printf("Timed out\n");
    else
      printf("File read error\n");
    return(0);
    }   
  else
    {
    printf("Sent OK. Length=%d CRC=%04X\n",flen,(crchi << 8) + crclo);
    // expect reply from receiving device
    gotn = read_node_endchar(node,temps,1024,termchar,EXIT_TIMEOUT,5000);
    if(gotn == 0)
      printf("No reply from %s\n",device_name(node));
    else
      {
      --gotn;
      while(gotn >= 0)
        {  // strip trailing endchar and any cr/lf
        if(temps[gotn] < 32 || temps[gotn] == termchar)
          temps[gotn] = 0;
        else
          gotn = 0;
        --gotn;
        }
      printf("Reply from %s: %s\n",device_name(node),temps);
      }
    }
      
  return(1);
  }




/******* FILE LENGTH ********/

int filelength(FILE *file)
  {
  int len;
  
  fseek(file,0,SEEK_END);
  len = ftell(file);
  fseek(file,0,SEEK_SET);
  return(len);
  }

/********** RECEIVE FILE **************
by server
have read Ffname command from client
fname = file name to save on this device 
next 7 bytes =  4 length bytes  2 block size bytes  chksum
followed by length bytes
followed by 2 crc bytes
***************************/

 
int receivefile(char *fname,int clientnode)
  {
   if(receivefilex(fname,clientnode) == 0)
    {
    sendstring(clientnode,"Fail");
    return(0);
    }
  sendstring(clientnode,"OK");
  return(1);
  }
  
int receivefilex(char *fname,int clientnode)
  {
  FILE *stream;
  int n,ntogo,nblock,len,nread,getout,ndat,ncrc,crchi,crclo;
  unsigned char lens[8],temps[1024],chksum;
  unsigned short crc;

  if(fname[0] == '\0')
    {
    printf("No file name\n");
    return(0);
    }
 
  printf("Receive file %s\n",fname);  

  if(read_node_count(clientnode,lens,7,EXIT_TIMEOUT,5000) != 7)   // read 7 bytes 3 s timeout
    return(0);       
  
    // file length   
  len = (int)lens[0] + ((int)lens[1] << 8) + ((int)lens[2] << 16) + ((int)lens[3] << 24);
    // packet block size
  nblock = (int)lens[4] + ((int)lens[5] << 8);
  
  chksum = lens[0];
  for(n = 1 ; n < 6 ; ++n)
    chksum += lens[n];
  if(lens[6] != chksum)
    {
    printf("Checksum error\n");
    return(0);
    }
    
  printf("File length = %d\n",len);

  stream = fopen(fname,"w");
  if(stream == NULL)
    {
    printf("File open error\n");
    return(0);
    }

  temps[0] = 10;
  if(write_node(clientnode,temps,1) != 1)  // send one endchar ack byte
    {
    printf("Timed out\n");
    return(0);
    }

  crc = 0xFFFF;
  crchi = 0xFF;
  crclo = 0xFF;
  
  ntogo = len+2;

  getout = 0;
  do
    {
    if(ntogo < nblock)
      nblock = ntogo;
       // read nblock with time out

    // read nblock bytes 5s time out
    nread = read_node_count(clientnode,temps,nblock,EXIT_TIMEOUT,5000);  
     
    if(nread == nblock)   // got nblock chars
      {
      if(nread == 1)
        {
        crchi = crclo;
        crclo = temps[0];
        }
      else
        {
        crchi = temps[nread-2];
        crclo = temps[nread-1];
        }
      if(ntogo <= 2)
        {
        ncrc = ntogo;
        ndat = 0;
        }
      else
        {  
        ncrc = nblock - ntogo + 2;
        if(ncrc < 0)
          ncrc = 0;
        ndat = nblock - ncrc;
        if(ndat < 0)
          ndat = 0;
        }              
    
      if(ndat > 0)
        {
        if((int)fwrite(temps,1,ndat,stream) != ndat)
          getout = 2;
        else
          ntogo -= ndat;
        }  
    
      if(getout == 0)
        {  
        ntogo -= ncrc;
      
        crc = crccalc(crc,temps,nblock);
               
        if(ntogo != 0)
          {  // ack send not last block
          temps[0] = 10;
          if(write_node(clientnode,temps,1) != 1)  // send one endchar ack byte
            getout = 1; // error
          }
        }
                      
      }   // end got block
    else
      getout = 1;   // error    
    }
  while(ntogo > 0 && getout == 0);  // plus 2 CRC bytes
  
  fclose(stream);

  if(getout != 0 || crc != 0)
    {   // error may have left data in buffer
    read_node_clear(clientnode);
    if(getout == 1)
      printf("Timed out\n");
    else if(getout == 2)
      printf("File write error\n");
    else
      printf("CRC error\n"); 
    return(0);
    }
       
  printf("Received OK. CRC=%04X\n",(crchi << 8) + crclo);
  return(1);    
  }  
  
 
/************** SEND ASCII STRING **************************
comd = zero terminated ascii string - not binary data
add endchar to string
************************************************/

int sendstring(int node,char *comd)
  {
  int clen,retval;
  char *s;
  unsigned char sbuff[1024]; 
                
  clen = strlen(comd); 
  if(clen > 999)
    {
    printf("String too long\n");
    return(0);
    }
   
  if(clen == 0)
    s = "(endchar only)";
  else
    s = comd;
     
  printf("Sending to %s: %s\n",device_name(node),s);

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
  char coms[64];
  
  printf("\nBroadcast a string to all mesh devices\n");
  getstring("Input string (max 25 chars): ",coms,64);
 
  len = strlen(coms); 
  if(len > 25)
    {
    printf("String too long\n");
    return(0);
    }
    
  write_mesh((unsigned char*)coms,len);
  return(1);
  }

/********** READ/WRITE LE CHARACTERISTIC ****************/

void readle()
  {
  int n,node,cticn,ascflag,datlen;
  unsigned char dat[256];
  
  printf("\nRead an LE characteristic\n");
  
  node = inputnode(BTYPE_LE | BTYPE_ME | BTYPE_CONNECTED | BTYPE_LO,0);   // only connected LE devices
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
      
  cticn = inputint("Input ctic index");
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
  
  node = inputnode(BTYPE_LE | BTYPE_ME | BTYPE_CONNECTED,0);  // only connected LE devices
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
       
  cticn = inputint("Input ctic index");
  if(cticn < 0)
    return; 

  if(ctic_ok(node,cticn) == 0)
    {
    printf("Invalid index\n");
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

/******** Input file list ********/


int inputlist(char *s,int len)
  {
  int n,getout,count;
  
  n = 0;
  count = 0;
  getout = 0;
  printf("Input list of files e=end list x=cancel\n");
  printf("One file name for each ? prompt\n");
  printf("e.g. /home/pi/doc.txt or e to end list\n");
  do
    {
    getstring("? ",s+n,len-n);
    if(s[n] == 'x' && s[n+1] == 0)
      return(-1);
    if(s[n] == 'e' && s[n+1] == 0)
      getout = 1;
    else
      ++count;   
    while(s[n] != 0 && n < len-2)
      ++n;
    ++n;
    s[n] = 0;
    if(getout == 0 && n > len-32)
      {
      printf("Too many\n");
      getout = 1;
      }
    }
  while(getout == 0);
  return(count);
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
