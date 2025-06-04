/******* BLUETOOTH INTERFACE **********
REQUIRES
  btlib.c/h  Version 23.1 
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
#include <stdlib.h>
#include <string.h>
#include "btlib.h"

#define BTFWINDOWS

int btfdespatch(char c);
void printhelp(void);
void settings(void);
int clientread(int node);
int sendstring(int node,char *comd);
int sendgetfile(void);
int sendfilex(int node,char *opcode,char *filename,char *destdir,int blocksize,int termchar);
int sendfileobex(int node,char *fname);
int receivefile(char *fname,int clientnode);
int receivefilex(char *fname,int clientnode);
int filelength(FILE *file);
int inputnode(char *pr,int typemask);
int inputchan(int node,int *channel,int *method);
void readservices(void);
void readuuid(void);
int clientconnect(void);
int meshsend(void);
int clientsend(int fun);
int server(void);
int universal_callback(int clientnode,int operation,int cticn,unsigned char *buf,int nread);
int node_classic_callback(int clientnode,unsigned char *buf,int nread);
int le_callback(int clientnode,int operation,int cticn);
int mesh_callback(int clientnode,unsigned char *buf,int nread);
int notify_callback(int lenode,int cticn,unsigned char *buf,int nread);
int obex_callback(int node,unsigned char *data,int len);
void localdisconnect(void);
void readnotify(void);
void readle(void);
void writele(void);
void notifyle();
void readnotify(void);
void regserial(void);
unsigned short crccalc(unsigned short crc,unsigned char *buf,int len);
int inputlist(char *buf,int len);
int clientsecurity(int *passkey);
int serversecurity(int *passkey);
void readlehandles();
void prcancel(void);
int printhex(unsigned char *s,int len);
void print(char *s);


#ifdef BTFWINDOWS
int input_string(char *prompt,char *buf,int len,char *curval);
int input_filename(char *prompt,char *buf,int len,int rwflag,char *curval);
int input_integer(char *prompt,int *curval);
int input_select(char *prompt,char *select);
int input_radio(char *prompt,char *select);
int print_status(char *s);
int dongtype(void);
char pathsep = '\\';
#else
int inputint(char *ps);
void getstring(char *prompt,char *s,int len);
char pathsep = '/';
#endif

char endchar = 10;  // terminate char for read/write 
char *prompt = NULL;
char *selectlist = NULL;
char *temps = NULL;
char noyes[16] = {"0 = No\n1 = Yes\n"};

#ifndef BTFWINDOWS
int main(int argc,char *argv[])
  { 
  int ret;
  char cmds[64];
  
  if(init_blue("devices.txt") == 0)      
    return(0);     

  printf("h = help\n");
  do
    {
    getstring("> ",cmds,64);
    ret = btfdespatch(cmds[0]);
    }
  while(ret != 0);
  return(0);
  }
#endif


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
        
    case 'q':
      print("Disconnecting everything and exit..\n");
      close_all();
      return(0);
        
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
  print("  s Become a listening server   f File transfer (send or get)\n");
  print("  v Read node services          y Read specified UUID service\n");
  print("  o Save screen output to file  g Register custom serial UUID\n");  
  print("  j LE notify/indicate on/off   R Read LE notifications\n");                     
  print("  m Mesh transmit on            n Mesh transmit off\n");                     
  print("  u Clear input buffer         [] Scroll screen back/forward\n"); 
  print("  l Read LE handles             q Quit\n"); 
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
    
    sprintf(temps,"Reply from %s: ",device_name(node));
    print(temps);
    print((char*)buf);
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
  char *pr,coms[256];
  
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
                 
  
  coms[0] = 0;
  coms[1] = 0;
  if(cmd == 't')
    {
    sprintf(prompt,"Will add endchar = %d\n",endchar);
    strcat(prompt,"Input string to send");
#ifdef BTFWINDOWS
    input_string(prompt,coms,128,NULL);
#else 
    printf("%s\n",prompt);    
    getstring("? ",coms,128);
#endif
    } 
  else if(cmd == 'p')
    {
    coms[0] = 0;  // empty string - will add endchar
    print("Ping server\n");
    }
  else if(cmd == 'D')
    {
    coms[0] = 'D';
    print("Tell server to disconnect\n");
    }
  else
    {
    print("Unknown command\n");
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
  int serverflag,clinode,keyflag,inkey,timeds,passkey,pairflags,addr,lesecurity;
     // choose this random address (2 hi bits of 1st byte must be 1)
  static unsigned char randadd[6] = {  0xD9,0x56,0xDB,0x38,0x32,0xA2 };
  static unsigned char locadd[6] = { 0,0,0,0,0,0 };
  
  pairflags = 0;
  passkey = 0;
  timeds = 0;
  
  strcpy(prompt,"SERVER\n");   
  strcpy(selectlist,"  0 = Node server\n  1 = Classic server\n  2 = LE server\n");
  strcat(selectlist,"  3 = Universal server (Classic and LE)\n");
  strcat(selectlist,"  4 = Mesh server\n  5 = OBEX server (receive file from Windows, Android...)\n");
#ifdef BTFWINDOWS
  strcat(prompt,"Input server type\n");
  serverflag = input_radio(prompt,selectlist);
#else
  strcat(prompt,selectlist);  
  strcat(prompt,"Input server type 0-5");
  serverflag = inputint(prompt);
#endif
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
#ifdef BTFWINDOWS
    strcat(prompt,"Enable security options\n");
    lesecurity = input_select(prompt,selectlist);
#else       
    strcat(prompt,"Enable security options 0=No 1=Yes");
    lesecurity = inputint(prompt);
#endif
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
#ifdef BTFWINDOWS
      strcat(prompt,"Input identity");
      addr = input_radio(prompt,selectlist);
#else
      strcat(prompt,selectlist);
      strcat(prompt,"Input 0/1");      
      addr = inputint(prompt);
#endif      
      if(addr < 0)
        return(0);
      }
    else
      addr = 0;
        
    strcpy(prompt,"TIMER\nInput TIMER interval in deci (0.1) seconds\n   0 = No TIMER calls\n");
    strcat(prompt,"  10 = One second interval\n  50 = Five second interval etc...\nTimer interval");
#ifdef BTFWINDOWS
    timeds = input_integer(prompt,&timeds);
#else   
    timeds = inputint(prompt);
#endif

    if(timeds < 0)
      return(0);
    
    if(serverflag == 2)
      {
      strcpy(prompt,"KEY PRESSES\n");
#ifdef BTFWINDOWS
      strcpy(selectlist,noyes);
      strcat(prompt,"Send key presses to KEYPRESS callback\n");
      keyflag = input_select(prompt,selectlist);
#else 
      strcat(prompt,"Send key presses to KEYPRESS callback 0=No 1=Yes");
      keyflag = inputint(prompt);  
#endif      
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
#ifdef BTFWINDOWS
      strcat(prompt,"Client's security requirement");
      inkey = input_radio(prompt,selectlist);
#else
      strcat(prompt,selectlist);
      strcat(prompt,"Client's security requirement");      
      inkey = inputint(prompt);
#endif      

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
    else if(serverflag == 5)
      {
      print("\nServer will listen on channel 2 and UUID = 1105\n");
      classic_server(clinode,obex_callback,PACKET_ENDCHAR,keyflag);
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
#ifdef BTFWINDOWS
  strcat(prompt,"Select option");
  key = input_radio(prompt,selectlist);
#else
  strcat(prompt,selectlist);
  strcat(prompt,"Input option");      
  key = inputint(prompt);
#endif 
  
  if(key <  0)
     return(-1);
  
  if(key == 1)
    {
#ifdef BTFWINDOWS
    *passkey = input_integer("Fixed 6-digit passkey",NULL);        
#else
    *passkey = inputint("Fixed 6-digit passkey");        
#endif
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
#ifdef BTFWINDOWS
  strcat(prompt,"Enable authentication");
  auth = input_select(prompt,selectlist);
#else
  strcat(prompt,"Enable authentication 0=No 1=Yes");    
  auth = inputint(prompt);
#endif

  if(auth <  0)
     return(-1);

  strcpy(prompt,"PASSKEY SECURITY\n");
  strcpy(selectlist,"  0 = No - Ask for Just Works\n");
  strcat(selectlist,"  1 = Yes - Accept Passkey\n");
#ifdef BTFWINDOWS
  strcat(prompt,"Accept Passkey security if the client asks");
  jwflag = input_radio(prompt,selectlist);
#else
  strcat(prompt,selectlist);
  strcat(prompt,"Accept Passkey security if the client asks");
  jwflag = inputint(prompt);
#endif 
  
  if(jwflag < 0)
    return(-1);   

  /******* uncomment for secure connect option 
  strcpy(prompt,"SECURE CONNECTION\n");
  strcpy(selectlist,"  0 = No - Legacy pairing\n");
  strcat(selectlist,"  1 = Yes - Secure Connection pairing\n");
#ifdef BTFWINDOWS
  strcat(prompt,"Accept secure connection if the client asks");
  scflag = input_radio(prompt,selectlist);
#else
  strcat(prompt,selectlist);
  strcat(prompt,"Accept secure connection if the client asks");   
  scflag = inputint(prompt);
#endif

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
  sprintf(temps,"Mesh packet from %s\n",device_name(clientnode));
  print(temps);
  printhex(buf,nread);
  
  if(buf[0] == 'D')
    {
    print("Disconnect\n");
    return(SERVER_EXIT);  // server exits
    }
    
  return(SERVER_CONTINUE);  // server loops for another packet
  }    
    
int le_callback(int clientnode,int operation,int cticn)
  {
  int nread;
  unsigned char dat[256]; 
     
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
    nread = read_ctic(localnode(),cticn,dat,sizeof(dat));
    sprintf(temps,"  %s has written local characteristic %s =",device_name(clientnode),ctic_name(localnode(),cticn));
    print(temps);
    printhex(dat,nread);
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
  unsigned char dat[256]; 
  int n,k,flag,nreadle;
  char firstc,*s,files[256];
  static char destdir[256] = {""};
  static int nblock = 400;

  
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
    nreadle = read_ctic(localnode(),cticn,dat,sizeof(dat));
    sprintf(temps,"  %s has written local characteristic %s =",device_name(clientnode),ctic_name(localnode(),cticn));
    print(temps);
    printhex(dat,nreadle);
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
    else if(firstc == 'F')
      {  // receive file      
      // command = Ffilename  endchar stripped 
      receivefile((char*)&buf[1],clientnode);
      }
    else if(firstc == 'X' && nread > 1)
      {  // destination directory for get file
      s = (char*)(buf+1);
      sprintf(temps,"Destination directory for GET file = %s\n",s);
      print(temps);
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
        sprintf(temps,"Block size for GET file = %d\n",nblock);
        print(temps);
        }
      }
    else if(firstc == 'G' && nread > 1)
      { // get file request with file name
      s =  (char*)(buf+1);
      n = 0;
      while(s[n] != 0 && n < nread && n < 255)
        {
        files[n] = s[n];
        ++n;
        }
      files[n] = 0;
      sprintf(temps,"GET file %s\n",files);
      print(temps);
      sendfilex(clientnode,"F",files,destdir,nblock,endchar);
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
  char firstc,*s,files[256];
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
  else if(firstc == 'F')
    {  // receive file      
    // command = Ffilename  endchar stripped 
    receivefile((char*)&buf[1],clientnode);
    }
  else if(firstc == 'X' && nread > 1)
    {  // destination directory for get file
    s = (char*)(buf+1);
    sprintf(temps,"Destination directory for GET file = %s\n",s);
    print(temps);
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
      sprintf(temps,"Block size for GET file = %d\n",nblock);
      print(temps);
      }
    }
  else if(firstc == 'G' && nread > 1)
    { // get file request with file name
    s =  (char*)(buf+1);
    n = 0;
    while(s[n] != 0 && n < nread && n < 255)
      {
      files[n] = s[n];
      ++n;
      }
    files[n] = 0;
    sprintf(temps,"GET file %s\n",files);
    print(temps);
    sendfilex(clientnode,"F",files,destdir,nblock,endchar);
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
  char buf[64];
    
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
#ifdef BTFWINDOWS
    strcat(prompt,"Listening type of remote btferret device");
    contype = input_radio(prompt,selectlist);
#else    
    strcat(prompt,selectlist);
    strcat(prompt,"Input listening type of remote btferret device");
    contype = inputint(prompt);
#endif    
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
#ifdef BTFWINDOWS
    strcpy(selectlist,noyes);
    strcat(prompt,"Enable security options");
    lesecurity = input_radio(prompt,selectlist);
#else
    strcat(prompt,"Enable security options 0=No 1=Yes");
    lesecurity = inputint(prompt);
#endif   
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
    sprintf(buf,"  Current value=%d  Default value=750\n",curwait);
    strcat(prompt,buf);
    strcat(prompt,"Time in ms");
#ifdef BTFWINDOWS
    wait = input_integer(prompt,&curwait);   
#else
    wait = inputint(prompt);
#endif
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
#ifdef BTFWINDOWS
      pairwait = input_integer(prompt,&pairwait);
#else
      pairwait = inputint(prompt);
#endif
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
#ifdef BTFWINDOWS
  strcat(prompt,"Bond");
  bond = input_radio(prompt,selectlist);
#else
  strcat(prompt,selectlist);
  strcat(prompt,"Bond");
  bond = inputint(prompt);
#endif

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
#ifdef BTFWINDOWS
  strcat(prompt,"Select pair option");  
  pair = input_radio(prompt,selectlist);
#else
  strcat(prompt,selectlist);
  strcat(prompt,"Input pair option");  
  pair = inputint(prompt);
#endif
  
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
#ifdef BTFWINDOWS
    strcat(prompt,"Select option");
    keydev = input_radio(prompt,selectlist);
#else
    strcat(prompt,selectlist);
    strcat(prompt,"Enter 0/1");
    keydev = inputint(prompt);
#endif
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
#ifdef BTFWINDOWS
    *passkey = input_integer("Fixed 6-digit passkey",NULL);        
#else
    *passkey = inputint("Fixed 6-digit passkey");        
#endif
    
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
#ifdef BTFWINDOWS
  strcat(prompt,"Accept secure connection if the client asks");
  scflag = input_radio(prompt,selectlist);
#else
  strcat(prompt,selectlist);
  strcat(prompt,"Accept secure connection if the client asks");   
  scflag = inputint(prompt);
#endif
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
#ifdef BTFWINDOWS
  strcat(prompt,"Select option"); 
  valn = input_radio(prompt,selectlist);
#else  
  strcat(prompt,selectlist);
  strcat(prompt,"Input option"); 
  valn = inputint(prompt);
#endif
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
#ifdef BTFWINDOWS
  timeout = input_integer(prompt,&timeout);
#else     
  timeout = inputint(prompt);
#endif

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
  
void readuuid()
  {
  int num,node,op,ret,flag;
  char suuid[64];
  unsigned char *uuid;
   
  strcpy(prompt,"Find services that contain a specified UUID\n");
  strcpy(selectlist,"  0 = List services\n");
  strcpy(selectlist,"  1 = Find LE characteristic index\n");
  strcpy(selectlist,"  2 = Find Classic RFCOMM channel\n");
#ifdef BTFWINDOWS
  strcat(prompt,"Select option");
  op = input_radio(prompt,selectlist);
#else
  strcat(prompt,selectlist);
  strcat(prompt,"Input 0-2"); 
  op = inputint(prompt);
#endif
  if(op < 0 || op > 2)
    {
    print("Invalid entry\n");
    return;
    }
    
  node = inputnode("Find services",BTYPE_CL | BTYPE_LE | BTYPE_ME);
  if(node < 0)
    return;
  
  if(op == 0)    
    strcpy(prompt,"Input 2-byte UUID in hex e.g. 0100");
  else
    strcpy(prompt,"Input 2 or 16-byte UUID in hex e.g. 0100");

#ifdef BTFWINDOWS
  input_string(prompt,suuid,64,NULL);
#else    
  printf("%s  (x = cancel)\n",prompt);
  getstring("? ",suuid,64);
#endif
   
  if(suuid[0] == 'x')
    return;  
     
  uuid = strtohex(suuid,&num);
    
  if(op == 0)
    {
    if(num != 2)
      {
      print("Not 2-byte\n");
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
    print("Not 2/16 byte\n");
    return;
    }
    
  if(op == 1)  
    {
    ret = find_ctic_index(node,flag,uuid);
    if(ret < 0)
       print("UUID not found\n");
    else
      {
      sprintf(temps,"Characteristic index = %d\n",ret);
      print(temps);
      }
    }
  else if(op == 2)
    { 
    ret = find_channel(node,flag,uuid);
    if(ret < 0)
      print("Failed to read services\n");
    else if(ret == 0)
      print("UUID not found\n");
    else
      {
      sprintf(temps,"RFCOMM channel = %d\n",ret);
      print(temps);
      }
    }
       
  }
    
 
/************* SEND/RECEIVE FILE *****************/


int sendgetfile()
  {
  int n,j,servernode,maxblock,xblock,retval,sorg,count;
  char ec,fname[256],ddir[256],list[4096];
  static char ddirsav[256] = {""};
  static int nblock = 0;
#ifdef BTFWINDOWS
  char *xdir;
#endif 
 
  strcpy(prompt,"File transfer\n");
  strcpy(selectlist,"  0 = SEND file\n  1 = GET file\n  2 = SEND multiple files\n");
  strcat(selectlist,"  3 = SEND file to OBEX server (Windows, Android..)\n  4 = RECEIVE file from OBEX client\n"); 
#ifdef BTFWINDOWS
  strcat(prompt,"Select option");
  sorg = input_radio(prompt,selectlist);
#else
  strcat(prompt,selectlist);
  strcat(prompt,"Input 0-4"); 
  sorg = inputint(prompt);
#endif
       
  if(sorg == 4)
    {
    print("Start an OBEX server via s. Send the file from the remote device\n");
    return(0);
    }     
       
  if(sorg == 0)
    print("\nSEND file\n");
  else if(sorg == 1)
    print("\nGET file\n");
  else if(sorg == 2)
    print("\nSEND multiple\n");
  else if(sorg == 3)
    print("\nSEND to OBEX\n");
  else
    {
    prcancel();
    return(0);
    }
     
  servernode = inputnode("File transfer",BTYPE_CONNECTED | BTYPE_ME | BTYPE_CL);       
  if(servernode < 0)
    {
    prcancel();
    return(0);
    }

  maxblock = 1000;        
  if(device_connected(servernode) == NO_CONN)
    {
    print("Not connected\n");
    return(0);
    }
  else if(sorg == 3)
    print("*** NOTE *** Must be connected to an OBEX server RFCOMM channel\n");
  else if(device_type(servernode) == BTYPE_CL)
    print("*** NOTE *** Server must understand btferret transfer protocol\n");
  else if(device_connected(servernode) == NODE_CONN)
    maxblock = 400;  // node connect max block size 
  
  if(nblock < 64 || nblock > maxblock)
    nblock = maxblock;

  if(sorg == 2)
    {
    count = inputlist(list,4096);
    if(count < 1)
      {
      prcancel();
      return(0);
      }
    }
  else
    {
    strcpy(prompt,"Enter file name e.g.  /home/pi/doc.txt");

#ifdef BTFWINDOWS
    input_filename(prompt,fname,256,0,NULL);
#else
    printf("%s  (x = cancel)\n",prompt);
    getstring("? ",fname,256);
#endif
        
    if((fname[0] == 'x' && fname[1] == 0) || fname[0] == ' ')
      {
      prcancel();
      return(0);
      }
    }
  
  if(sorg != 3)
    {     
#ifdef BTFWINDOWS
    if(ddirsav[0] != 0)
      xdir = ddirsav;
    else 
      xdir = NULL;
    input_string("Destination directory",ddir,256,xdir);
#else
    printf("Destination directory\n");
    if(ddirsav[0] != 0)
      printf("Current = %s  ( /=none, r=retain, x=cancel)\n",ddirsav);
    else
      printf("e.g /home/pi/ OR C:\\rat\\  ( /=none, x=cancel)\n");     
    getstring("? ",ddir,256);
#endif

    if(ddir[0] == 'x' && ddir[1] == 0)
      {
      prcancel();
      return(0);
      } 

    if(ddirsav[0] != 0 && ddir[0] == 'r' && ddir[1] == 0)
      strcpy(ddir,ddirsav);
    else  
      strcpy(ddirsav,ddir);

    ec = ddir[strlen(ddir) - 1];
    if(ec != '/' && ec != '\\')
      {
      print("Directory must end with / or \\\n");
      return(0);
      }
  
    if(ddir[0] == '/' && (ddir[1] == 0 || ddir[1] == ' '))
      {
      print("None - will save to server's btferret directory\n");
      ddir[0] = 0;
      }

    if(ddirsav[0] != 0 && ddir[0] == 'r' && ddir[1] == 0)
      strcpy(ddir,ddirsav);
    else  
      strcpy(ddirsav,ddir);
    
    
    sprintf(prompt,"BLOCK SIZE = %d bytes\n",nblock);
    strcat(prompt,"  Data is transmitted in blocks of this size\n");
#ifdef BTFWINDOWS
    if(dongtype() != 0)
      xblock = 200;
    else
      {
      sprintf(temps,"  Retain or enter new value 64-%d",maxblock);
      strcat(prompt,temps);
      xblock = input_integer(prompt,&nblock);
      }   
#else    
    sprintf(temps,"  Enter x to keep, or enter new value 64-%d",maxblock);
    strcat(prompt,temps);
    xblock = inputint(prompt);
#endif
    
    if(xblock >= 64 && xblock < maxblock)
      {
      nblock = xblock;         
      sprintf(temps,"Block size changed to %d\n",nblock);
      print(temps);
      }
    else
      print("Block size not changed\n");
    }
    
  // server must use the same opcode (F)
  if(sorg == 2)
    {
    sprintf(temps,"Sending %d files\n",count);
    print(temps);
    j = 0;
    for(n = 0 ; n < count ; ++n)
      {
      sprintf(temps,"%d. %s\n",n+1,list+j);
      print(temps);
      retval = sendfilex(servernode,"F",list+j,ddir,nblock,endchar);
      while(list[j] != 0)
        ++j;
      ++j;
      } 
    }
  else if(sorg == 0) 
    retval = sendfilex(servernode,"F",fname,ddir,nblock,endchar);
  else if(sorg == 3)
    retval = sendfileobex(servernode,fname);
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
        print("Timed out\n");
        retval = 0;
        }
      }
    else
      {
      print("Server did not send file\n");
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
  unsigned char tempbuf[1024],buf[8];
  unsigned short crc;
  char *fname;
 
  crchi = 0;
  crclo = 0;
 
  fn = 0;  // after last / start of file name 
  n = 0;
  while(filename[n] > 32 && n < 1022)
    {  // strip non alpha
    if(filename[n] == pathsep)  
      fn = n+1;  // start of file name
    ++n;
    }
    
  fname = filename+fn;
  
  sprintf(temps,"Sending file %s\n",filename);
  print(temps);
       
  type = device_type(node);
  if(!(type == BTYPE_ME || type == BTYPE_CL) || device_connected(node) == NO_CONN)
    {
    print("Invalid device type or not connected\n");
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
    print("No file name or opcode\n");
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
        print("Destination directory must end with / or \\\n");
        return(0);
        }
      }
    }  
                 
  stream = fopen(filename,"rb");
  if(stream == NULL)
    {
    print("File open error\n");
    return(0);
    }
 

  // clear all packets from input buffer
  read_node_clear(node);

  strcpy((char*)tempbuf,opcode);   // send file command
  strcat((char*)tempbuf,destdir);
  strcat((char*)tempbuf,fname);  // add dir:file name
  flen = strlen((char*)tempbuf);
  tempbuf[flen] = termchar;
  ++flen;      
  // send command  Ffilespec + termchar
  if(write_node(node,tempbuf,flen) != flen)
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
    print("Transmit fail\n");
    fclose(stream);
    return(0);
    }

       // wait for single ack byte
  gotn = read_node_count(node,buf,1,EXIT_TIMEOUT,5000);             
  if(gotn != 1 || buf[0] != 10)
    {
    print("Not seen acknowledge from receiver\n");
    fclose(stream);
    return(0);
    }
    
  progflag = 0;  // no print progress
  if(flen > 5000)
    {  // every 20 packets 
    progflag = 1;
#ifdef BTFWINDOWS
    print_status("Progress 0%");
#else 
    print("Progress");
    fflush(stdout);
#endif
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
                // wait for single ack byte 3s time out
      gotn = read_node_count(node,buf,1,EXIT_TIMEOUT,3000);                
       
      if(gotn != 1 || buf[0] != 10)
        {
        getout = 1;
        print("Not seen acknowledge from receiver\n");
        print("Transmission may be slow for unknown reason\n");
        print("Try reducing block size\n");
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
        if(fread(tempbuf,1,ndat,stream) != ndat)
          getout = 2;
        else
          { 
          crc = crccalc(crc,tempbuf,ndat);
          ntogo -= ndat;
          }
        }  
     
      if(getout == 0)
        {   
        if(ncrc == 2)
          {
          tempbuf[ndat] = (unsigned short)((crc >> 8) & 255);
          crchi = tempbuf[ndat];
          tempbuf[ndat+1] = (unsigned short)(crc & 255);
          crclo = tempbuf[ndat+1];
          ntogo -= 2;
          }
        else if(ncrc == 1)
          {
          if(ndat == 0)
            {
            tempbuf[0] = (unsigned short)(crc & 255);
            crclo = tempbuf[0];
            }
          else
            {
            tempbuf[ndat] = (unsigned short)((crc >> 8) & 255);
            crchi = tempbuf[ndat];
            }
          --ntogo;
          }     

        // send nblk bytes
       
        if(write_node(node,tempbuf,nblk) != nblk)
          getout = 1;
        
       
        ackflag = 1; 
        ++packn;
        if(progflag != 0 && (packn % 20) == 0)
          {
#ifdef BTFWINDOWS
          sprintf(tempbuf,"Progress %d%%",(100*(flen+2-ntogo))/(flen+2));
          print_status(tempbuf);    
#else
          print(".");
          if((packn % 1000) == 0)
            print("\n");
          fflush(stdout);
#endif
          } 
        }      
      } // end retval==1
    }   // end block loop
  while(getout == 0 && ntogo > 0);
  
 
  if(progflag != 0)
#ifdef BTFWINDOWS
    print_status("BT_ferret running");
#else
    print("\n");
#endif  
  fclose(stream);

  if(getout != 0)
    {  // error may have left ack in buffer
    read_node_count(node,tempbuf,1024,EXIT_TIMEOUT,5000);
    if(getout == 1)
      print("Timed out\n");
    else
      print("File read error\n");
    return(0);
    }   
  else
    {
    sprintf(temps,"Sent OK. Length=%d CRC=%04X\n",flen,(crchi << 8) + crclo);
    print(temps);
    // expect reply from receiving device
    gotn = read_node_endchar(node,tempbuf,1024,termchar,EXIT_TIMEOUT,5000);
    if(gotn == 0)
      {
      sprintf(temps,"No reply from %s\n",device_name(node));
      print(temps);
      }
    else
      {
      --gotn;
      while(gotn >= 0)
        {  // strip trailing endchar and any cr/lf
        if(tempbuf[gotn] < 32 || tempbuf[gotn] == termchar)
          tempbuf[gotn] = 0;
        else
          gotn = 0;
        --gotn;
        }
      sprintf(temps,"Reply from %s: %s\n",device_name(node),tempbuf);
      print(temps);
      }
    }
      
  return(1);
  }

int sendfileobex(int node,char *filename)
  {
  int n,k,fn,flen,nlen,ndat,ntogo,nblock,len,err;
  unsigned char inbuf[64],send[512];
  static unsigned char connect[7] = {0x80,0x00,0x07,0x10,0x00,0x01,0x90};
  static unsigned char disconnect[3] = {0x81,0x00,0x03};   
  char *fname,temps[128];
  FILE *stream;

  fn = 0;  // after last / start of file name 
  n = 0;
  while(filename[n] > 32 && n < 1022)
    {   // strip non alpha
    if(filename[n] == pathsep)
      fn = n+1;  // start of file name
    ++n;
    }
    
  fname = filename+fn;
  
  sprintf(temps,"Sending file %s\n",filename);
  print(temps);
  
  stream = fopen(filename,"rb");
  if(stream == NULL)
    {
    print("File open error\n");
    return(0);
    }
  flen = filelength(stream);
  ntogo = flen; 
  nlen = strlen(fname);

  nblock = 400;
  connect[5] = (nblock >> 8) & 0xFF;
  connect[6] = nblock & 0xFF;

  // OBEX connect
  write_node(node,connect,7);
  inbuf[0] = 0;
  // wait for Success reply 0x0A
  len = read_node_endchar(node,inbuf,64,PACKET_ENDCHAR,EXIT_TIMEOUT,5000);
  if(len == 0 || inbuf[0] != 0xA0)
    {
    print("OBEX Connect failed\n");
    fclose(stream);
    return(0);
    }
  else if((inbuf[1] << 8) + inbuf[2] >= 7)
    {
    n = (inbuf[5] << 8) + inbuf[6];
    if(n < nblock)
      nblock = n;
    }
 
  send[3] = 0x01;
  n = 2*nlen + 5;
  send[4] = (n >> 8) & 0xFF;
  send[5] = n & 0xFF;
  k = 6;
  for(n = 0 ; n < nlen ; ++n)
    {
    send[k] = 0;
    send[k+1] = fname[n];
    k += 2;
    } 
  send[k] = 0;
  send[k+1] = 0;
  k += 2;

  send[k] = 0xC3;
  send[k+1] = (flen >> 24) & 0xFF;
  send[k+2] = (flen >> 16) & 0xFF;
  send[k+3] = (flen >> 8) & 0xFF;
  send[k+4] = flen & 0xFF;
  k += 5;
  err = 0;
  do
    { 
    if(ntogo <= nblock - 3 - k)
      {
      send[k] = 0x49;
      send[0] = 0x82;
      ndat = ntogo + 3;
      }
    else
      {
      send[k] = 0x48;
      send[0] = 0x02;
      ndat = nblock - k;
      } 
    send[k+1] = (ndat >> 8) & 0xFF;
    send[k+2] = ndat & 0xFF;
    k += 3;
    ndat -= 3;
    if(fread(send+k,1,ndat,stream) != ndat)
      {
      print("File read error\n");
      err = 1;
      }
    else
      {
      ntogo -= ndat;
      k += ndat;
      send[1] = (k >> 8) & 0xFF;
      send[2] = k & 0xFF;
      write_node(node,send,k);
      inbuf[0] = 0;
      len = read_node_endchar(node,inbuf,64,PACKET_ENDCHAR,EXIT_TIMEOUT,5000);
      if(len == 0 || (inbuf[0] != 0xA0 && inbuf[0] != 0x90))
        {
        print("Send failed\n");
        err = 1;
        }
      }  
    k = 3;
    }
  while(ntogo > 0 && err == 0);  

  fclose(stream);
  
  write_node(node,disconnect,3);
  inbuf[0] = 0;
  len = read_node_endchar(node,inbuf,64,PACKET_ENDCHAR,EXIT_TIMEOUT,5000);
  if(len == 0 || inbuf[0] != 0xA0)
    print("OBEX Disconnect failed\n");

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
  unsigned char lens[8],tempbuf[1024],chksum;
  unsigned short crc;

  if(fname[0] == '\0')
    {
    print("No file name\n");
    return(0);
    }
 
  sprintf(temps,"Receive file %s\n",fname);  
  print(temps);
  
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
    print("Checksum error\n");
    return(0);
    }
    
  sprintf(temps,"File length = %d\n",len);
  print(temps);
  
  stream = fopen(fname,"wb");
  if(stream == NULL)
    {
    print("File open error\n");
    return(0);
    }

  tempbuf[0] = 10;
  if(write_node(clientnode,tempbuf,1) != 1)  // send one endchar ack byte
    {
    print("Timed out\n");
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

    // read nblock bytes 3s time out
    nread = read_node_count(clientnode,tempbuf,nblock,EXIT_TIMEOUT,3000);  
     
    if(nread == nblock)   // got nblock chars
      {
      if(nread == 1)
        {
        crchi = crclo;
        crclo = tempbuf[0];
        }
      else
        {
        crchi = tempbuf[nread-2];
        crclo = tempbuf[nread-1];
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
        if((int)fwrite(tempbuf,1,ndat,stream) != ndat)
          getout = 2;
        else
          ntogo -= ndat;
        }  
    
      if(getout == 0)
        {  
        ntogo -= ncrc;
      
        crc = crccalc(crc,tempbuf,nblock);
               
        if(ntogo != 0)
          {  // ack send not last block
          tempbuf[0] = 10;
          if(write_node(clientnode,tempbuf,1) != 1)  // send one endchar ack byte
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
    read_node_count(clientnode,tempbuf,1024,EXIT_TIMEOUT,1000);
    if(getout == 1)
      print("Timed out\n");
    else if(getout == 2)
      print("File write error\n");
    else
      print("CRC error\n"); 
    return(0);
    }
       
  sprintf(temps,"Received OK. CRC=%04X\n",(crchi << 8) + crclo);
  print(temps);
  return(1);    
  }  


int obex_callback(int node,unsigned char *data,int len)
  {
  int n,j,k,ilen,length,hi,datan,datalen;
  char temps[280],filename[256]; 
  static int count = 0;
  static int connected_node = 0;
  static int file_length;
  static FILE *stream = NULL;
  static unsigned char connect_success[7] = {0xA0,0x00,0x07,0x10,0x00,0x01,0x90};
  static unsigned char connect_fail[7] = {0xC3,0x00,0x03};
  static unsigned char success[3] = {0xA0,0x00,0x03};
  static unsigned char continue_reply[3] = {0x90,0x00,0x03};
   
  datalen = 0;  // no data
  
  // data[0] = opcode (section 3.4 in OBEX15.pdf)
  
  if(data[0] == 0x80)
    {  // Connect 
    if(connected_node == 0)
      {  // not connected to another device
      print("OBEX connect\n");
      write_node(node,connect_success,7);    
      connected_node = node;
      count = 0;  // data bytes received
      if(stream != NULL)
        fclose(stream);
      stream = NULL;
      }
    else
      { // already connected to another device - refuse
      sprintf(temps,"Node %d trying to OBEX connect - refuse\n",node);
      print(temps);
      write_node(node,connect_fail,3);
      }
    return(SERVER_CONTINUE);
    }
      
  if(node != connected_node)
    {
    sprintf(temps,"Node %d not OBEX connected\n",node);
    print(temps);
    return(SERVER_CONTINUE);
    }  
  
  if((data[0] & 0x7F) == 0x02)
    { // 0x02 or 0x82 Put
    length = (data[1] << 8) + data[2];  // should = len
    n = 3;  // 1st header item
    while(n < length && n < len)
      {
      hi = data[n];  // header item identifier (section 2.1 in OBEX15.pdf)
      if((hi & 0xC0) == 0x80)
        {  // 1-byte value
        if(hi == 0x97)
          {  // Single Response Mode
          print("SRM not programmed\n");
          write_node(node,connect_fail,3);
          connected_node = 0;
          return(SERVER_CONTINUE);
          }
        /*** other header identifiers here 0x93 0x94... ***/
        n += 2;  // next item
        }
      else if((hi & 0xC0) == 0xC0)
        {  // 4-byte value
        if(hi == 0xC3)
          {  // Count       
          file_length = (data[n+1] << 24) + (data[n+2] << 16) + (data[n+3] << 8) + data[n+4];
          sprintf(temps,"File length = %d\n",file_length);
          print(temps);
          }
        /**** other header identifiers here 0xC4 0xCB... ****/
        n += 5;  // next item
        }
      else
        {  // 2 length bytes 
        ilen = (data[n+1] << 8) + data[n+2];  // item length 
        j = n+3;    // start data  
        if(ilen == 0)
          n = length;  // error exit loop
        if(hi == 0x01)
          {  // unicode file name
          ++j; // skip unicode 0
          k = 0;
          while(j < n+ilen && j < len)
            {
            filename[k] = data[j];
            ++k;
            j += 2;
            }
          filename[k] = 0;
          sprintf(temps,"File name = %s\n",filename);
          print(temps);
          stream = fopen(filename,"wb");
          if(stream == NULL)
            print("File open error\n"); 
          }
        else if(hi == 0x48 || hi == 0x49)
          { // data chunk
          datan = j;           // index 
          datalen = ilen-3;    // length
          }
        /*********         
        Other header identifiers here
        0x42 = Type
        0x47 = HTTP
        0x44 = Time
        ...
        *********/
        
        n += ilen;  // next item  
        }
      }     
    }     
  else if(data[0] == 0x81)
    {  // Disconnect
    print("OBEX disconnect\n");
    connected_node = 0;
    if(stream != NULL)
      { 
      fclose(stream);
      stream = NULL;
      }
    write_node(node,success,3);  
    return(SERVER_CONTINUE);
    } 
  else
    {
    sprintf(temps,"GOT opcode %02X - no action\n",data[0]);
    print(temps);
    }
  // Write data chunk to file
  if(datalen != 0 && stream != NULL)
    {   
    count += datalen;  
    for(n = 0 ; n < datalen ; ++n)
      fputc(data[n+datan],stream); 
    if(data[0] == 0x82)
      {  // last chunk - finished
      if(count != file_length)
        {
        sprintf(temps,"Expected %d bytes. Got %d\n",file_length,count);
        print(temps);
        }
      fclose(stream);
      stream = NULL;
      print("File saved\n");
      }     
    }
  
  // Send response Continue or Success (section 3.2.1 in OBEX15.pdf)
  if(data[0] == 0x02)  // Put - not last chunk 
    write_node(node,continue_reply,3);  
  else      
    write_node(node,success,3);  

  return(SERVER_CONTINUE);
  }

  
 
/************** SEND ASCII STRING **************************
comd = zero terminated ascii string - not binary data
add endchar to string
************************************************/

int sendstring(int node,char *comd)
  {
  int clen,retval;
  char *s;
  unsigned char sbuff[1100]; 
                
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
  char coms[64];
  
  strcpy(prompt,"Broadcast a string to all mesh devices\n");
  strcat(prompt,"Input string (max 25 chars)");

#ifdef BTFWINDOWS
    input_string(prompt,coms,64,NULL);
#else
    printf("%s\n",prompt);
    getstring("? ",coms,64);
#endif

  len = strlen(coms); 
  if(len > 25)
    {
    print("String too long\n");
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
 
  
  node = inputnode("Read an LE characteristic",BTYPE_LE | BTYPE_ME | BTYPE_CONNECTED | BTYPE_LO);   // only connected LE devices
  if(node < 0)
    {
    prcancel();
    return;
    }
  
#ifdef BTFWINDOWS
  n = list_ctics_ex(node,CTIC_R,selectlist,4096);
#else     
  n = list_ctics(node,LIST_SHORT | CTIC_R);
#endif
  
  if(n == 0)
    {
    print("No readable characteristics. Read services to find\n");
    return;
    }

#ifdef BTFWINDOWS
  if(n < 8) 
    cticn = input_radio("Select characteristic",selectlist);
  else
    cticn = input_select("Select characteristic",selectlist);
#else      
  cticn = inputint("Input ctic index");
#endif
 
  if(cticn < 0)
    return;
       
  datlen = read_ctic(node,cticn,dat,sizeof(dat));
  
  if(datlen == 0)
    return;     // fail or no data
  
  sprintf(temps,"%s %s = ",device_name(node),ctic_name(node,cticn));
  print(temps);
  ascflag = printhex(dat,datlen);
          
  if(ascflag != 0)
    {
    sprintf(temps,"ASCII = \"%s\"\n",dat);  // print ascii - readctic has added term 0
                                            // at dat[datlen]
    print(temps);
    } 
  }

void writele()
  {
  int n,node,cticn,size;
  char buf[256];
  unsigned char *val;
  
  node = inputnode("Write an LE characteristic",BTYPE_LE | BTYPE_ME | BTYPE_CONNECTED | BTYPE_LO);   // only connected LE devices
  if(node < 0)
    {
    prcancel();
    return;
    }
  

#ifdef BTFWINDOWS
  n = list_ctics_ex(node,CTIC_W,selectlist,4096);
#else     
  n = list_ctics(node,LIST_SHORT | CTIC_W);
#endif
  
  if(n == 0)
    {
    print("No writeable characteristics. Read services to find\n");
    return;
    }

#ifdef BTFWINDOWS
  if(n < 8)
    cticn = input_radio("Select characteristic",selectlist); 
  else
    cticn = input_select("Select characteristic",selectlist);
#else      
  cticn = inputint("Input ctic index");
#endif

  if(cticn < 0)
    return;    
   
  strcpy(prompt,"Input data bytes in hex e.g. 5A 43 01");

#ifdef BTFWINDOWS
  input_string(prompt,buf,128,NULL);
#else
  printf("%s  (x=cancel)\n",prompt);
  getstring("? ",buf,128);
#endif

  if(buf[0] == 'x' && buf[1] == 0)
    return;
        
  val = strtohex(buf,&size);
 
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
  
  
#ifdef BTFWINDOWS
  flag = list_ctics_ex(node,CTIC_NOTIFY,selectlist,4096);
#else     
  flag = list_ctics(node,LIST_SHORT | CTIC_NOTIFY);
#endif
  
  if(flag == 0)
    {
    print("No notify/indicate characteristics. Read services to find\n");
    return;
    }

#ifdef BTFWINDOWS
  if(flag < 8)
    cticn = input_radio("Select characteristic",selectlist); 
  else
    cticn = input_select("Select characteristic",selectlist);
#else      
  cticn = inputint("Input ctic index");
#endif

  
  if(cticn < 0)
    return; 

  if(ctic_ok(node,cticn) == 0)
    {
    print("Invalid index\n");
    return;
    }

#ifdef BTFWINDOWS
  flag = input_radio("Action","0 = Disable\n1 = Enable");
#else  
  flag = inputint("0=Disable 1=Enable");
#endif

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
  // LE device has sent notification or indication enabled by notify_ctic()  
  sprintf(temps,"%s %s notify =",device_name(lenode),ctic_name(lenode,cticn));
  print(temps);
  printhex(buf,nread);  
  return(0);
  }

void regserial()
  {
  int n;
  char name[128],uuid[64];
  unsigned char *newcustom;
  
  strcpy(prompt,"Register a custom serial service\n");
  strcat(prompt,"Input 16-byte UUID e.g. 0011-2233-44556677-8899AABBCCDDEEFF");

#ifdef BTFWINDOWS
  input_string(prompt,uuid,64,NULL);
#else
  printf("%s  (x = cancel)\n",prompt);
  getstring("? ",uuid,64);
#endif
  
  if(uuid[0] == 'x')
    return;

  newcustom = strtohex(uuid,&n);
   
  if(n != 16)
    {
    print("UUID must be 16 bytes\n");
    return;
    }     
    
  strcpy(prompt,"Input service name");
#ifdef BTFWINDOWS
  input_string(prompt,name,128,NULL);
#else
  printf("%s  (x = cancel)\n",prompt);
  getstring("? ",name,128);
#endif
  
  if(name[0] == 'x')
    return;
    
  register_serial(newcustom,name);
  print("Done\n");
  }

/******** Input file list ********/


int inputlist(char *s,int len)
  {
  int n,i,getout,count;
  char last[128];
  
  n = 0;
  i = 0;
  count = 0;
  getout = 0;
  last[0] = 0;
  
#ifdef BTFWINDOWS  
  strcpy(prompt,"Input next file name   e=end list\n");
  strcat(prompt,"e.g. /home/pi/doc.txt or e to end list\n");
#else
  printf("Input list of files  e=end list  x=cancel\n");
  printf("One file name for each ? prompt\n");
  printf("e.g. /home/pi/doc.txt or e to end list\n");
  if(last[i] == 0)
    i = 0;
#endif

  do
    {
#ifdef BTFWINDOWS
    input_filename(prompt,s+n,len-n,0,last);
    i = 0;
    while(s[n+i] != 0 && i < 127)
      {
      last[i] = s[n+i];
      ++i;
      }
    last[i] = 0;
    // strip file name
    while(last[i] != 92 && i > 0)
      --i;
    last[i+1] = 0;
#else    
    getstring("? ",s+n,len-n);
#endif
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
      print("Too many\n");
      getout = 1;
      }
    }
  while(getout == 0);
  return(count);
  }  

/********** USER INPUT FUNCTIONS *******/
 
int inputnode(char *pr,int mask)
  {
  int n,node,flag;


#ifdef BTFWINDOWS
  strcpy(prompt,pr);
  strcat(prompt,"\nAVAILABLE DEVICES\nSelect node\n");
  n = device_info_ex(mask,selectlist,4096);
#else
  printf("%s\nAVAILABLE DEVICES\n",pr);
  n = device_info(mask | BTYPE_SHORT);
#endif

  if(n == 0)
    {
    print("No suitable devices available\n");
    return(-1);
    }    

  do
    {
    flag = 0;
#ifdef BTFWINDOWS
    if(n < 8)    
      node = input_radio(prompt,selectlist);
    else
      node = input_select(prompt,selectlist);
#else
    node = inputint("Input node");
#endif
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
#ifdef BTFWINDOWS
  strcat(prompt,"Select option");
  n = input_radio(prompt,selectlist);  
#else
  strcat(prompt,selectlist);
  strcat(prompt,"Input option 0-2");
  n = inputint(prompt);
#endif
  
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
#ifdef BTFWINDOWS
    if(list_channels_ex(node,selectlist,4096) <= 0)
      {
      print("Failed to find RFCOMM channels\n");
      return(-1);
      }
    *channel = input_radio("Select RFCOMM channel",selectlist);
    if(*channel < 0)
      return(-1);
    return(1);   
#else
    if(list_channels(node,LIST_SHORT) <= 0)     
      {
      print("Failed to find RFCOMM channels\n");
      return(-1);
      }
#endif
    }
    
  strcpy(prompt,"Input RFCOMM channel number");
#ifdef BTFWINDOWS
  *channel = input_integer(prompt,NULL);
#else    
  *channel = inputint(prompt);
#endif

  if(*channel < 0)
    return(-1);
    
  return(1);
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
  
void prcancel()
  {
  print("Cancelled\n");
  }

int printhex(unsigned char *s,int len)
  {
  int n,ascflag;
  char buf[8];
  
  if(len == 0)
    {
    print("\n");
    return(0);
    }  
  
  ascflag = 1;
  n = 0;
  temps[0] = 0;
  while(n < len)
    {
    sprintf(buf," %02X",s[n]);
    strcat(temps,buf);
    if(((n+1)%16) == 0)
      {
      strcat(temps,"\n");
      print(temps);
      temps[0] = 0;
      }
    if(s[n] < 32 || s[n] > 126)
      ascflag = 0;
    ++n;
    }
  if(temps[0] != 0)
    {
    strcat(temps,"\n");
    print(temps);
    }
  return(ascflag);
  }


#ifndef BTFWINDOWS

void print(char *s)
  {
  printf("%s",s);
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
  
#endif
