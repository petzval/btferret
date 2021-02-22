
/******* BLUETOOTH INTERFACE **********
COMPILE
gcc bterret.c btlib.c -o btferret
EDIT
devices.txt to list all devices in the network 
**************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "btlib.h"

void btlink(void);
void printhelp(void);
void settings(void);
int clientread(int node);
int sendstring(int node,char *comd,char endchar);
int inputint(char *ps);
int sendfile(void);
int receivefile(char *fname,int clientnode);
int filelength(FILE *file);
int inputnode(int typemask,int meshflag);
int inputchan(int node,int *channel,int *method);
void readservices(void);
void readuuid(void);
int clientconnect(void);
int meshsend(void);
int clientsend(int fun);
int server(void);
int node_callback(int clientnode,char *buf,int nread);
int mesh_callback(int clientnode,char *buf,int nread);
int nodeserver(void);
void localdisconnect(void);
void readle(void);
void writele(void);
void getstring(char *prompt,char *s,int len);


char termchar = 10;  // terminate char for string sent from client
char repchar = 10;   // terminate char for reply sent from server

 
int main(int argc,char *argv[])
  { 
  if(init_blue("devices.txt") == 0)
    return(0);     

  btlink();

  printf("Disconnectng everything and exit..\n");
  
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
        sendfile();
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
  printf("  a Scan for Classic devices       i Print device info\n");
  printf("  b Scan for LE/Mesh devices       k Settings\n");   
  printf("  c Connect to a node              p Ping node server\n");
  printf("  t Send string to node server     T Send string to mesh\n");
  printf("  r Read LE characteristic         w Write LE characteristic\n");
  printf("  d Disconnect                     D Tell server to disconnect\n");  
  printf("  s Become a listening server      f Send file to node server\n");
  printf("  v Read node services             y Read node UUID matches\n");
  printf("  o Save screen output to file    [] Scroll screen back/forward\n"); 
  printf("  m Mesh transmit on               n Mesh transmit off\n");                     
  printf("  h Print this help                q Quit \n");
  }


/*********** CLIENT READ/SEND **********/

int clientread(int node)
  {
  int n,gotn;
  char buf[1024];
  
   // read to repchar 3s time out
  gotn = read_node_endchar(node,buf,1024,repchar,EXIT_TIMEOUT,3000);

  if(gotn > 0)
    {
    for(n = 0 ; n < gotn && n < 1023 ; ++n)
      { 
      if(n == gotn-1 && buf[n] == repchar)
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
  int n,flag,node;
  char savtc,savrc,coms[256];
  
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
    write_mesh("D",1);
    return(1);
    }  
    
    
  if(device_connected(node) == 0)
    {
    printf("Not connected\n");
    return(0);
    }  
  
  if(cmd == 'p' || cmd == 'D')
    printf("This command only works if connected to a btferret server\n");
                 
  savtc = termchar;
  savrc = repchar;
  
  coms[0] = 0;
  coms[1] = 0;
  if(cmd == 't')
    {  
    getstring("Input string to send: ",coms,128);
    n = inputint("Input terminate char used by server (probably 10)");
    if(n < 0)
      return(0);
        
    termchar = n;
    repchar = n;  // assume reply term is same as send
    } 
  else if(cmd == 'p')
    {
    coms[0] = 0;  // empty string - will add termchar
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
      
  if(sendstring(node,coms,termchar) != 0)   
    {   // read reply from connected node - not mesh
    clientread(node);
    }
          
  if((device_type(node) == BTYPE_CL || device_type(node) == BTYPE_ME) && cmd == 'D') 
    wait_for_disconnect(node,5000);  // wait for classic server to initiate and complete disconnect
                                     // 5 sec time out
  termchar = savtc; 
  repchar = savrc; 
  return(1);
  }

/************ SERVER FUNCTIONS *************/

int server()
  {
  int serverflag;
  
  printf("\n  0 = mesh server\n  1 = node server\n");
  serverflag = inputint("Input server type 0/1/2");
  if(serverflag < 0)
    return(0);
  if(serverflag == 0)
    mesh_server(mesh_callback);
  else if(serverflag == 1)
    nodeserver();
  else
    printf("Invalid type\n");
  return(0);
  }  
    
int nodeserver()
  {
  int clinode,retval;
   
  printf("\nInput node of mesh client that will connect\n");
  clinode = inputnode(BTYPE_ME,0);  
  if(clinode < 0)
    {
    printf("Cancelled\n");
    return(0);
    }
  
  retval = node_server(clinode,node_callback,termchar);
    
  return(1);
  }    
    
 
int mesh_callback(int clientnode,char *buf,int nread)
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
    


int node_callback(int clientnode,char *buf,int nread)
  {
  int n,k;
  char firstc;
   
  if(buf[0] == termchar || (buf[0] != 0 && buf[1] == termchar) || buf[0] == 'F')
    firstc = buf[0];  // is a single char or F receive file 
  else
    firstc = 0;       // more than one char - not a single char command  
 
  if(nread != 0)
    {   
    for(k = 0 ; k < nread ; ++k)
      {   // strip non-ascii chars for print
      if(k == nread-1 && buf[k] == termchar)
        buf[k] = 0;  // wipe termchar
      else if(buf[k] < 32 || buf[k] > 126)
        buf[k] = '.';
      }   
    printf("Recvd from %s: %s\n",device_name(clientnode),buf);  
    }
      
  // check if received string is a known command 
  // and send reply to client
          
  if(firstc == termchar)
    {
    printf("Ping\n");
    sendstring(clientnode,"OK",repchar);  // REPLY 2=add repchar   
    }
  else if(firstc == 'D')
    {
    printf("Disconnect\n");
    sendstring(clientnode,"Server disconnecting",repchar);
    }
  else if(firstc == 'F')
    {  // receive file      
    // command = Ffilename  termchar stripped 
    if(receivefile(&buf[1],clientnode) == 0)
      sendstring(clientnode,"Fail",repchar);
    else 
      sendstring(clientnode,"OK",repchar);
    }
  else
    {
    printf("No action\n");
    sendstring(clientnode,"Unknown command - no action",repchar);    
    }
    
  if(firstc == 'D')
    return(SERVER_EXIT);   // stop node server and initiate disconnection
 
  return(SERVER_CONTINUE);    // loop for another packet
  }


/*********** CONNECT/DISCONNECT *********/

int clientconnect()
  {
  int node,channel,method,retval;
    
     // only disconnected devices
  node = inputnode(BTYPE_CL | BTYPE_LE | BTYPE_DISCONNECTED | BTYPE_ME,0);

  if(node < 0)
    {
    printf("Cancelled\n");
    return(0);
    }
   
  if(device_type(node) == BTYPE_CL)
    {  // classic server needs channel and method  
    if(inputchan(node,&channel,&method) < 0)
      {
      printf("Cancelled\n");
      return(0);
      }   
    }
  else
    {  // LE and Mesh devices do not use channel/method
    channel = 0;
    method = 0;
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
  int setn,valn;

  setn = 0;
  if(setn == 0)
    {           
    valn = inputint("PRINT options\n  0 = None\n  1 = Normal\n  2 = Verbose - all HCI traffic\nInput one of the above options");
    if(valn < 0)
      return;
    if(valn == 0)
      valn = PRINT_NONE;
    else if(valn == 1)
      valn = PRINT_NORMAL;
    else if(valn == 2)
      valn = PRINT_VERBOSE;
    else
      {
      printf("Invalid option\n");
      return;
      }
    set_print_flag(valn);
    }
  }


  
/*********** READ SERVICES *************/

  
void readservices()
  {
  int node;
  
  printf("\nRead services\n");
  
  node = inputnode(BTYPE_CL | BTYPE_LE,0);
  if(node < 0)
    return;
 
  if(device_type(node) == BTYPE_LE)
    find_ctics(node);
  else  // CLASSIC
    list_channels(node,LIST_FULL);
  }
  
  
void readuuid()
  {
  int num,node;
  char suuid[32],uuid[2],*val;

  printf("\nList services that contain a specified UUID\n");
  
  node = inputnode(BTYPE_CL | BTYPE_LE,0);
  if(node < 0)
    return;
       
  printf("Input 2-byte UUID in hex e.g. 0100  (x = cancel)\n");

  getstring("? ",suuid,32);
   
  if(suuid[0] == 'x')
    return;  
 
  val = strtohex(suuid,&num);
  
  if(num == 1)
    {
    uuid[0] = 0;  // hi
    uuid[1] = val[0];  // lo
    }
  else if(num == 2)
    {
    uuid[0] = val[0];
    uuid[1] = val[1];
    }
  else
    {
    printf("Not 2-byte\n");
    return;
    }
    
      // uuid must be 2-byte hi first     
  list_uuid(node,uuid);  
  }
    
 
/************* SEND/RECEIVE FILE *****************/

int sendfile()
  {
  FILE *stream;
  int len,n,bn,k,key,nblock,ntogo,fn,type;
  int getout,gotn,progflag,packn,servernode;
  unsigned char temps[1024],fname[256],ddir[256],buf[8];
  unsigned short crc,bwd;
  char c; 
 
 
    // input target device to receive file - will read replies from it

  printf("\nSend file - only works if connected to a btferret node server\n");
   
  servernode = inputnode(BTYPE_CONNECTED | BTYPE_ME | BTYPE_CL,0);       
  if(servernode < 0)
    {
    printf("Cancelled\n");
    return(0);
    }
   
  type = device_type(servernode);
  if(type != BTYPE_ME && type != BTYPE_CL)
    {
    printf("Invalid device type\n");
    return(0);
    }

  if(type == BTYPE_CL)
    {
    nblock = 1000;
    printf("*** NOTE *** Classic server must be programmed like receivefile() in btferret.c\n");
    }  
  else
    nblock = 400;  // node connect max block size
         
    // clear all packets from input buffer
  read_all_clear();

  printf("Enter file name e.g.  /home/pi/doc.txt  (x = cancel)\n");
  
  getstring("? ",fname,256);
  
  if((fname[0] == 'x' && fname[1] == 0) || fname[0] == ' ')
    {
    printf("Cancelled\n");
    return(0);
    }

  // find end of directory - last /
  
  fn = 0;  // no directory
  for(n = 0 ; fname[n] != 0 && n < 255 ; ++n)
    {
    if(fname[n] == '/')
      fn = n+1;  // start of file name
    }
 
  if(fname[fn] == 0)
    {
    printf("No file name\n");
    return(0);
    } 
     
  printf("Enter destination directory  e.g.  /home/pi/  ( / = none, x = cancel)\n");
  getstring("? ",ddir,256);
  
  if(ddir[0] == 'x' && ddir[1] == 0)
    {
    printf("Cancelled\n");
    return(0);
    } 
    
  if(ddir[0] == '/' && ddir[1] == 0)
    ddir[0] = 0;  // no directory
  else
    {   
    n = 0;
    while(ddir[n] != 0)
      ++n;   
   
    if(n != 0)
      {
      if(!(ddir[n-1] == '/' || ddir[n-1] == '\\')) 
        {  // backslash allowed for Classic/Windows
        printf("Directory must end with /\n");
        return(0);
        }
      }
    }  
    
          
  stream = fopen(fname,"r");
  if(stream == NULL)
    {
    printf("File open error\n");
    return(0);
    }
           
  len = filelength(stream);
  
  temps[0] = 'F';   // send file command
  temps[1] = '\0';
  strcat(temps,ddir);
  strcat(temps,fname+fn);  // add dir:file name
       
  // send command  Ffilespec + termchar
     
  if(sendstring(servernode,temps,termchar) == 0) 
    {   
    fclose(stream);
    return(0);
    }

  // send 4 length bytes 2 block size bytes and checksum byte

  buf[0] = len & 255;
  buf[1] = (len >> 8) & 255;
  buf[2] = (len >> 16) & 255;
  buf[3] = (len >> 24) & 255;
  buf[4] = nblock & 255;
  buf[5] = (nblock >> 8) & 255;
  buf[6] = buf[0];
  for(n = 1 ; n < 6 ; ++n)
    buf[6] += buf[n];
      
  if(write_node(servernode,buf,7) != 7)
    {
    printf("Timed out\n");
    fclose(stream);
    return(0);
    }

       // wait for single ack byte
  gotn = read_node_count(servernode,buf,1,EXIT_TIMEOUT,5000);             
  if(gotn != 1 || buf[0] != repchar)
    {
    printf("Not seen ack\n");
    fclose(stream);
    return(0);
    }
    
  progflag = 0;  // no print progress
  if(len > 5000)
    {  // every 10 packets 
    progflag = 1;
    printf("Progress..");
    fflush(stdout);
    }

  crc = 0xFFFF;
    
  ntogo = len+2;  // do loop counter  
  n = 0;          // char count
  getout = 0;
  packn = 0;   // number of packets sent
  do
    {
    if(ntogo < nblock)
      nblock = ntogo;  // last block size
      
    if(n != 0)
      {  // just sent nblock       
         //  wait for ack = repchar from receiving device
         //  single char read + time out
      buf[0] = 0;
                // wait for single ack byte
      gotn = read_node_count(servernode,buf,1,EXIT_TIMEOUT,5000);                
      if(gotn != 1 || buf[0] != repchar)
        {
        getout = 1;
        printf("Not seen ack\n");
        }
      }
      
    if(getout == 0)  // ack wait may have timed out
      {
      for(bn = 0 ; bn < nblock ; ++bn)  // fill inbuff with next block
        {
        if(ntogo > 2)  // not last 2 crc
          {  
          bwd = (unsigned short)fgetc(stream);
          crc = (bwd << 8)^crc;
          for(k = 0 ; k < 8 ; ++k)
            {
            if( (crc & (unsigned short)32768) == 0)
              crc = crc << 1;
            else
              {
              crc = crc << 1;
              crc ^= 0x1021;
              }
            }
          }
        else if(ntogo == 2)
          bwd = (unsigned short)((crc >> 8) & 255);
        else
          bwd = (unsigned short)(crc & 255);
        
        temps[bn] = (unsigned char)bwd;
        ++n;   
        --ntogo;
        }   // end bn loop fill inbuff[nblock] 
  
      // send nblock bytes
         
      if(write_node(servernode,temps,nblock) != nblock)
        getout = 1;
      
      ++packn;
      if(progflag != 0 && packn % 10 == 0)
        {
        printf("."); 
        fflush(stdout);
        }
        
      //  1=OK else timed out 
      } // end retval==1
    }   // end block loop
  while(getout == 0 && ntogo != 0);
  
  if(progflag != 0)
    printf("\n");
  fclose(stream);

  if(getout != 0)
    {  // error may have left ack in buffer
    read_all_clear();
    printf("Timed out\n");
    return(0);
    }   
  else
    clientread(servernode);  // expect reply from receiving device
      
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
  FILE *stream;
  int n,k,len,key,ntogo,nblock,bn,bcount,nread,getout;
  unsigned char c,lens[8],temps[1024],chksum;
  unsigned short crc,bwd;


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

  if(write_node(clientnode,&repchar,1) != 1)  // send one repchar ack byte
    {
    printf("Timed out\n");
    return(0);
    }

  crc = 0xFFFF;

  bcount = 1;  // block number for info print
      
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
      for(bn = 0 ; bn < nblock ; ++bn)
        {
        if(ntogo > 2)        // not last 2 crc bytes
          fputc(temps[bn],stream);
        --ntogo;
          
        bwd = temps[bn];
        crc = (bwd << 8)^crc;
        for(k = 0 ; k < 8 ; ++k)
          {
          if( (crc & (unsigned short)32768) == 0)
            crc = crc << 1;
          else
            {
            crc = crc << 1;
            crc ^= 0x1021;
            }
          }  // end k loop
        }  // end bn chars loop
         
         
      if(ntogo != 0)
        {  // ack send not last block
        if(write_node(clientnode,&repchar,1) != 1)  // send one repchar ack byte
          getout = 1; // error
        }              
      }   // end got block
    else
      getout = 1;   // error    
    }
  while(ntogo > 0 && getout == 0);  // plus 2 CRC bytes
  
  fclose(stream);

  if(getout != 0)
    {   // error may have left data in buffer
    read_all_clear();
    printf("Timed out\n");
    return(0);
    }
    
  if(crc !=  0)
    {  
    printf("CRC error\n");
    return(0);
    }
    
  printf("OK\n");
  return(1);    
  }  
  
 
/************** SEND ASCII STRING **************************
comd = zero terminated ascii string - not binary data
add endchar to string
************************************************/

int sendstring(int node,char *comd,char endchar)
  {
  int n,clen,retval;
  char *s,sbuff[1024]; 
                
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
  strcpy(sbuff,comd);
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
    
  write_mesh(coms,len);
  return(1);
  }

/********** READ/WRITE LE CHARACTERISTIC ****************/

void readle()
  {
  int n,k,xn,node,cticn,chand,ascflag,len,maxlen,datlen;
  char dat[64];
  
  printf("\nRead an LE characteristic\n");
  
  node = inputnode(BTYPE_LE | BTYPE_CONNECTED,0);   // only connected LE devices
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
  if(datlen > 4)     
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
  char buf[128];
  char *val;
  
  printf("\nWrite an LE characteristic\n");
  
  node = inputnode(BTYPE_LE | BTYPE_CONNECTED,0);   // only connected LE devices
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


/********** USER INPUT FUNCTIONS *******/
 
int inputnode(int mask,int meshflag)
  {
  int n,count,type,node,numbdevs,flag;

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
    printf("Mesh node servers");
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
       
  if(meshflag != 0)
    printf(" 0  All mesh servers (not connected node servers)\n");
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
  int n,k,flag;
  struct clasdata *cp;


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
    printf(prompt);
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
