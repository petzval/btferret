/********** Bluetooth mouse **********
Download from https://github.com/petzval/btferret
  mouse.c  this code
  mouse.txt
  btlib.c    Version 20 or later
  btlib.h    Version 20 or later
 
Edit mouse.txt to set ADDRESS=
to the address of the local device
that runs this code

Compile
  gcc mouse.c btlib.c -o mouse
  
Run
  sudo ./mouse

Connect from phone/tablet/PC to "HID" device

Mouse data from /dev/input/mouse0 sent to client

This code sets an unchanging random address.
If connection is unreliable try changing the address.

See HID Devices section in documentation for 
more infomation.

*********************************/    

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include "btlib.h"  

int lecallback(int clientnode,int op,int cticn);
int send_mouse(char x,char y,char but);

/*********  mouse.txt DEVICES file ******
DEVICE = My Pi   TYPE=Mesh  node=1  ADDRESS = DC:A6:32:04:DB:56
  PRIMARY_SERVICE = 1800
    LECHAR=Device Name   SIZE=4   Permit=02 UUID=2A00  
    LECHAR=Appearance    SIZE=2   Permit=02 UUID=2A01  
  PRIMARY_SERVICE = 180A
    LECHAR= PnP ID       SIZE=7 Permit=02 UUID=2A50  
  PRIMARY_SERVICE = 1812
    LECHAR=Protocol Mode   SIZE=1  Permit=06  UUID=2A4E  
    LECHAR=HID Info        SIZE=4  Permit=02  UUID=2A4A  
    LECHAR=HID Ctl Point   SIZE=8  Permit=04  UUID=2A4C  
    LECHAR=Report Map      SIZE=52 Permit=02  UUID=2A4B  
    LECHAR=Report1         SIZE=3  Permit=92  UUID=2A4D  
        ; Report1 must have Report ID = 1 
        ;   0x85, 0x01 in Report Map
        ; unsigned char uuid[2]={0x2A,0x4D};
        ; index = find_ctic_index(localnode(),UUID_2,uuid);
        ; Send data: write_ctic(localnode(),index,data,0);
********/


/**** MOUSE REPORT MAP *****

From Appendix E.10 in the following:

https://www.usb.org/sites/default/files/documents/hid1_11.pdf

Report ID = 1 has been added (0x85,0x01)

*******************/

    // NOTE the size of reportmap (52 in this case) must appear in mouse.txt as follows:
    //   LECHAR=Report Map      SIZE=52 Permit=02  UUID=2A4B  
unsigned char reportmap[52] = {0x05,0x01,0x09,0x02,0xA1,0x01,0x85,0x01,0x09,0x01,0xA1,0x00,0x05,0x09,0x19,0x01,
                               0x29,0x03,0x15,0x00,0x25,0x01,0x95,0x03,0x75,0x01,0x81,0x02,0x95,0x01,0x75,0x05,
                               0x81,0x01,0x05,0x01,0x09,0x30,0x09,0x31,0x15,0x81,0x25,0x7F,0x75,0x08,0x95,0x02,
                               0x81,0x06,0xC0,0xC0};

    // NOTE the size of report (3 in this case) must appear in mouse.txt as follows:
    //   LECHAR=Report1         SIZE=3  Permit=92  UUID=2A4D  
unsigned char report[3] = {0,0,0};

unsigned char *name = "HID"; 
unsigned char appear[2] = {0xC2,0x03};  // 03C2 = mouse icon appears on connecting device 
unsigned char pnpinfo[7] = {0x02,0x6B,0x1D,0x46,0x02,0x37,0x05};
unsigned char protocolmode[1] = {0x01};
unsigned char hidinfo[4] = {0x01,0x11,0x00,0x02};
int fd = -1;

int main()
  {
  unsigned char uuid[2],randadd[6];
   
  if(init_blue("mouse.txt") == 0)
    return(0);
    
  if(localnode() != 1)
    {
    printf("ERROR - Edit mouse.txt to set ADDRESS = %s\n",device_address(localnode()));
    return(0);
    }

  // Write data to local characteristics
  uuid[0] = 0x2A;
  uuid[1] = 0x00;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),name,3); 

  uuid[0] = 0x2A;
  uuid[1] = 0x01;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),appear,0);

  uuid[0] = 0x2A;
  uuid[1] = 0x4E;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),protocolmode,0);

  uuid[0] = 0x2A;
  uuid[1] = 0x4A;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),hidinfo,0);

  uuid[0] = 0x2A;
  uuid[1] = 0x4B;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),reportmap,0);

  uuid[0] = 0x2A;
  uuid[1] = 0x4D;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),report,0);

  uuid[0] = 0x2A;
  uuid[1] = 0x50;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),pnpinfo,0);
                            
  // Set unchanging random address by hard-coding a fixed value.
  // If connection produces an "Attempting Classic connection"
  // error then choose a different address.
  // If set_le_random_address() is not called, the system will set a
  // new and different random address every time this code is run.  
 
  // Choose the following 6 numbers
  randadd[0] = 0xD3;  // 2 hi bits must be 1
  randadd[1] = 0x56;
  randadd[2] = 0xD6;
  randadd[3] = 0x74;
  randadd[4] = 0x33;
  randadd[5] = 0x05;
  set_le_random_address(randadd);
     
  set_le_wait(20000);  // Allow 20 seconds for connection to complete
                                         
  le_pair(localnode(),JUST_WORKS,0);  // Easiest option, but if client requires
                                      // passkey security - remove this command  
  set_flags(FAST_TIMER,FLAG_ON);
  le_server(lecallback,20);  // 20ms FAST_TIMER
  
  if(fd > 0)
    close(fd);
    
  close_all();
  return(1);
  }


int lecallback(int clientnode,int op,int cticn)
  {
  int n,nread;
  unsigned char buf[3];
          
  if(op == LE_CONNECT)
    {
    fd = open("/dev/input/mouse0",O_NONBLOCK);
    if(fd > 0)
      printf("Connected OK. Mouse sent to client. x stops server\n");
    else
      printf("Connected OK. Failed to open /dev/input/mouse0. x stops server\n");
    }
  if(fd > 0 && op == LE_TIMER)
    {
    nread = read(fd,buf,3);
    if(nread == 3)
      {
      send_mouse(buf[1],(char)(-buf[2]),buf[0]);
      // or if Y is reversed:  send_mouse(buf[1],buf[2],buf[0]);
      }
    }
  if(op == LE_DISCONNECT)
    return(SERVER_EXIT);
  return(SERVER_CONTINUE);
  }

/*********** SEND MOUSE *********
x,y = mouse movement -127 to 127

but  1 = Left button down
     2 = Right button down
     4 = Middle button down
  
  For a single click, these button presses are followed
  by a buf[0]=0 send which indicates button up.
        
********************************/

int send_mouse(char x,char y,char but)
  {
  unsigned char buf[3];
  static int reportindex = -1;
  
  if(reportindex < 0)
    {  // look up Report1 index
    buf[0] = 0x2A;
    buf[1] = 0x4D;
    reportindex = find_ctic_index(localnode(),UUID_2,buf);
    if(reportindex < 0)
      {
      printf("Failed to find Report characteristic\n");
      return(0);
      }
    }     
 
  buf[0] = but;
  buf[1] = x;
  buf[2] = y;
        
  // send to Report1
  write_ctic(localnode(),reportindex,buf,0);

  return(1);
  }

