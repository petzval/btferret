/********** Bluetooth keyboard **********
Download from https://github.com/petzval/btferret
  keyboard.c
  keyboard.txt
  btlib.c    Version 13 or later
  btlib.h    Version 13 or later

Edit keyboard.txt to set ADDRESS=
to the address of the local device
that runs this code

Compile
  gcc keyboard.c btlib.c -o keyboard
  
Run
  sudo ./keyboard

Connect from phone/tablet/PC to "HID" device

All keystrokes go to connecting device
F10 sends "Hello" plus Enter
ESC stops the server 

To add a battery level service:
uncomment all battery level labelled
code in keyboard.txt and here.
F10 will then also send a battery level notification

Note: This code uses the lowest level of security.
Do not use it if you need high security.

Non-GB keyboards
Even if the keyboard of this device is non-GB
it must be specified as "gb" in the boot info as follows:

Edit /etc/default/keyboard to include the line:
XKBLAYOUT="gb"

It is the receiving device that decides which
characters correspond to which keys. See discussion
in the HID Devices section of the documentation.

This code sets an unchanging random address.
If connection is unreliable try changing the address.

See HID Devices section in documentation for 
more infomation.

*********************************/    

#include <stdio.h>
#include <stdlib.h>
#include "btlib.h"

int lecallback(int clientnode,int op,int cticn);
int send_key(int key);

void getstring(char *prompt,char *s,int len);  //################
int inputint(char *ps);

/*********  keyboard.txt DEVICES file ******
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
    LECHAR=Report Map      SIZE=47 Permit=02  UUID=2A4B  
    LECHAR=Report1         SIZE=8  Permit=92  UUID=2A4D  
        ; Report1 must have Report ID = 1 
        ;   0x85, 0x01 in Report Map
        ; unsigned char uuid[2]={0x2A,0x4D};
        ; index = find_ctic_index(localnode(),UUID_2,uuid);
        ; Send data: write_ctic(localnode(),index,data,0);

;  *** Optional battery level ***
;  PRIMARY_SERVICE = 180F
;    LECHAR=Battery Level   SIZE=1 Permit=12  UUID=2A19   

********/


/**** KEYBOARD REPORT MAP *****
0x05, 0x01 Usage Page (Generic Desktop)
0x09, 0x06 Usage (Keyboard)
0xa1, 0x01 Collection (Application)
0x85, 0x01 Report ID = 1
0x05, 0x07 Usage Page (Keyboard)
0x19, 0xe0 Usage Minimum (Keyboard LeftControl)
0x29, 0xe7 Usage Maximum (Keyboard Right GUI)
0x15, 0x00 Logical Minimum (0)
0x25, 0x01 Logical Maximum (1)
0x75, 0x01 Report Size (1)  
0x95, 0x08 Report Count (8)
0x81, 0x02 Input (Data, Variable, Absolute) Modifier byte
0x95, 0x01 Report Count (1)
0x75, 0x08 Report Size (8)
0x81, 0x01 Input (Constant) Reserved byte
0x95, 0x06 Report Count (6)
0x75, 0x08 Report Size (8)
0x15, 0x00 Logical Minimum (0)
0x25, 0x65 Logical Maximum (101)
0x05, 0x07 Usage Page (Key Codes)
0x19, 0x00 Usage Minimum (Reserved (no event indicated))
0x29, 0x65 Usage Maximum (Keyboard Application)
0x81, 0x00 Input (Data,Array) Key arrays (6 bytes)
0xc0 End Collection
*******************/

    // NOTE the size of reportmap (47 in this case) must appear in keyboard.txt as follows:
    //   LECHAR=Report Map      SIZE=47 Permit=02  UUID=2A4B  
unsigned char reportmap[47] = {0x05,0x01,0x09,0x06,0xA1,0x01,0x85,0x01,0x05,0x07,0x19,0xE0,0x29,0xE7,0x15,0x00,
                               0x25,0x01,0x75,0x01,0x95,0x08,0x81,0x02,0x95,0x01,0x75,0x08,0x81,0x01,0x95,0x06,
                               0x75,0x08,0x15,0x00,0x25,0x65,0x05,0x07,0x19,0x00,0x29,0x65,0x81,0x00,0xC0};

    // NOTE the size of report (8 in this case) must appear in keyboard.txt as follows:
    //   LECHAR=Report1         SIZE=8  Permit=92  UUID=2A4D  
unsigned char report[8] = {0,0,0,0,0,0,0,0};

unsigned char name[4] = { 'H','I','D',0};
unsigned char appear[2] = {0xC1,0x03};  // 03C1 = keyboard icon appears on connecting device 
unsigned char pnpinfo[7] = {0x02,0x6B,0x1D,0x46,0x02,0x37,0x05};
unsigned char protocolmode[1] = {0x01};
unsigned char hidinfo[4] = {0x01,0x11,0x00,0x02};
unsigned char battery[1] = {100}; 

int main()
  {
  unsigned char uuid[2],randadd[6];
   
  if(init_blue("keyboard.txt") == 0)
    return(0);
    
  // Write data to local characteristics
  uuid[0] = 0x2A;
  uuid[1] = 0x00;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),name,0); 

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
   
  /**** battery level ******/
  //  uuid[0] = 0x2A;
  //  uuid[1] = 0x19;
  //  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),battery,1); 
  /************************/     
                          
  // Set unchanging random address by hard-coding a fixed value.
  // If connection produces an "Attempting Classic connection"
  // error then choose a different address.
  // If set_le_random_address() is not called, the system will set a
  // new and different random address every time this code is run.  
 
  // Choose the following 6 numbers
  randadd[0] = 0xD3;  // 2 hi bits must be 1
  randadd[1] = 0x56;
  randadd[2] = 0xDB;
  randadd[3] = 0x04;
  randadd[4] = 0x32;
  randadd[5] = 0xA0;
  set_le_random_address(randadd);
     
  keys_to_callback(KEY_ON,0);  // enable LE_KEYPRESS calls in lecallback
                               // 0 = GB keyboard  
  set_le_wait(2000);  // Allow 2 seconds for connection to complete
                      // If connect fails, try increasing this value
  le_server(lecallback,0);
  
  close_all();
  return(1);
  }


int lecallback(int clientnode,int op,int cticn)
  {
  int n;
  static char hello[8] = { "Hello\n"};  // \n = Enter
        
  if(op == LE_CONNECT)
    {
    printf("Connected OK. Key presses sent to client. ESC stops server\n");
    printf("F10 sends Hello plus Enter\n");
    }
  if(op == LE_KEYPRESS)
    {  // cticn = ASCII code of key OR btferret custom code
    if(cticn == 23)
      {    // 23 = btferret custom code for F10
           // Send "Hello" plus Enter
      for(n = 0 ; hello[n] != 0 ; ++n)
        send_key(hello[n]);
        
      /**** battery level ****/
      //  if(battery[0] > 0)
      //    --battery[0];
      //  uuid[0] = 0x2A;
      //  uuid[1] = 0x19;
      //  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),battery,1);
      /*******************/
      }
    else  
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
    else
      printf("Not a number\n");
    }
  while(flag != 0);
  return(n);
  } 

