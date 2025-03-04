/********** Bluetooth HID keyboard and mouse **********

Download from https://github.com/petzval/btferret
  btlib.c    Version 20 or later
  btlib.h    Version 20 or later
  keymouse.c  this code
  keymouse.txt
   
Edit keymouse.txt to set ADDRESS=
to the address of the local device
that runs this code

Compile
  gcc keymouse.c btlib.c -o keymouse
  
Run
  sudo ./keymouse

Connect from phone/tablet/PC to "HID" device

Keystrokes and mouse data sent to client
F10 sends "Hello" plus Enter

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
int send_key(int key);
int send_mouse(char x,char y,char but);

unsigned char reportmap[99] = {0x05,0x01,0x09,0x06,0xA1,0x01,0x85,0x01,0x05,0x07,0x19,0xE0,0x29,0xE7,0x15,0x00,
                               0x25,0x01,0x75,0x01,0x95,0x08,0x81,0x02,0x95,0x01,0x75,0x08,0x81,0x01,0x95,0x06,
                               0x75,0x08,0x15,0x00,0x25,0x65,0x05,0x07,0x19,0x00,0x29,0x65,0x81,0x00,0xC0,
                               0x05,0x01,0x09,0x02,0xA1,0x01,0x85,0x02,0x09,0x01,0xA1,0x00,0x05,0x09,0x19,0x01,
                               0x29,0x03,0x15,0x00,0x25,0x01,0x95,0x03,0x75,0x01,0x81,0x02,0x95,0x01,0x75,0x05,
                               0x81,0x01,0x05,0x01,0x09,0x30,0x09,0x31,0x15,0x81,0x25,0x7F,0x75,0x08,0x95,0x02,
                               0x81,0x06,0xC0,0xC0};

unsigned char report[8] = {0,0,0,0,0,0,0,0};
unsigned char *name = "HID"; 
unsigned char appear[2] = {0xC1,0x03};  // 03C1 = keyboard icon appears on connecting device 
unsigned char pnpinfo[7] = {0x02,0x6B,0x1D,0x46,0x02,0x37,0x05};
unsigned char protocolmode[1] = {0x01};
unsigned char hidinfo[4] = {0x01,0x11,0x00,0x02};
int fd = -1;

int main()
  {
  unsigned char uuid[2],randadd[6];
   
  if(init_blue("keymouse.txt") == 0)
    return(0);
    
  if(localnode() != 1)
    {
    printf("ERROR - Edit keymouse.txt to set ADDRESS = %s\n",device_address(localnode()));
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
  // Choose the following 6 numbers
  randadd[0] = 0xD3;  // 2 hi bits must be 1
  randadd[1] = 0x56;
  randadd[2] = 0xD6;
  randadd[3] = 0x74;
  randadd[4] = 0x33;
  randadd[5] = 0x06;
  set_le_random_address(randadd);
     
  keys_to_callback(KEY_ON,0);  // enable LE_KEYPRESS calls in lecallback
                               // 0 = GB keyboard  
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
      printf("Connected OK. Keys and mouse sent to client. ESC stops server\n");
    else
      printf("Connected OK. Keys sent to client. Failed to open /dev/input/mouse0. ESC stops\n");
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
  if(op == LE_KEYPRESS)
    {  // cticn = ASCII code of key OR btferret custom code
    send_key(cticn);      
    }
  if(op == LE_DISCONNECT)
    return(SERVER_EXIT);
  return(SERVER_CONTINUE);
  }

/*********** SEND KEY *****************

send_key(key code);

key code = ASCII code of character (e.g a=97) OR one of the
           following btferret custom codes:

1 = Pause     8 = Backspace  17 = F4     24 = F11
2 = Insert    9 = Tab        18 = F5     25 = F12
3 = Del      10 = Enter      19 = F6     27 = Esc
4 = Home     11 = Pound (^3) 20 = F7     28 = Right arrow
5 = End      14 = F1         21 = F8     29 = Left arrow
6 = PgUp     15 = F2         22 = F9     30 = Down arrow
7 = PgDn     16 = F3         23 = F10    31 = Up arrow 

ASCII codes                    'a' = 97               (valid range 32-126)
CTRL add 128 (0x80)         CTRL a = 'a' + 128 = 225  (valid range 225-255)
Left ALT add 256 (0x100)     ALT a = 'a' + 256 = 353  (valid range 257-382)
Right ALT add 384 (0x180)  AltGr a = 'a' + 384 = 481  (valid range 481-516)

SHIFT F1-F8 codes SHIFT F1 = 471  (valid range 471-478) 

Note CTRL i = same as Tab  CTRL m = same as Enter  
Some ALT keys generate ASCII codes

To send k: send_key('k')  
To send F1: send_key(14)
To send CTRL b:  send_key(226) same as send_key('b' | 0x80)
To send AltGr t: send_key(500) same as send_key('t' | 0x180)

These key codes are also listed in the
keys_to_callback() section in documentation

Modifier bits, hex values:
01=Left CTL  02=Left Shift  04=Left Alt  08=Left GUI
10=Right CTL 20=Right Shift 40=Right Alt 80=Right GUI
 
**************************************/

int send_key(int key)
  {
  int n,hidcode;
  unsigned char buf[8];
  static int reportindex = -1;

  // convert btferret code (key) to HID code  
  hidcode = hid_key_code(key);
  if(hidcode == 0)
    return(0);

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
 
  for(n = 0 ; n < 8 ; ++n)
    buf[n] = 0;
        
  // send key press to Report1
  buf[0] = (hidcode >> 8) & 0xFF;  // modifier
  buf[2] = hidcode & 0xFF;         // key code
  write_ctic(localnode(),reportindex,buf,0);
  // send no key pressed - all zero
  buf[0] = 0;
  buf[2] = 0;
  write_ctic(localnode(),reportindex,buf,0); 
  return(1);
  }

/*********** SEND MOUSE *********
x,y = mouse movement -127 to 127

but  1 = Left button click
     2 = Right button click
     4 = Middle button click
  
  For a single click these button presses are followed
  by a buf[0]=0 send which indicates button up.      
********************************/

int send_mouse(char x,char y,char but)
  {
  unsigned char buf[3];
  static int reportindex = -1;
  
  if(reportindex < 0)
    {  // look up Report 1 index
    buf[0] = 0x2A;
    buf[1] = 0x4D;
    reportindex = find_ctic_index(localnode(),UUID_2,buf);
    if(reportindex < 0)
      {
      printf("Failed to find Report characteristic\n");
      return(0);
      }
    ++reportindex;  // Mouse is Report 2
    }     
 
  buf[0] = but;
  buf[1] = x;
  buf[2] = y;
        
  // send to Report2
  write_ctic(localnode(),reportindex,buf,0);

  return(1);
  }
