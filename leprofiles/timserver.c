#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "btlib.h" 


/********** Bluetooth LE time server **********
Download from https://github.com/petzval/btferret
  btlib.c    Version 20 or later
  btlib.h    Version 20 or later
  timserver.c    this code
  timserver.txt
 
Edit timserver.txt to set ADDRESS=
to the address of the local device
 that runs this code

Compile
  gcc timserver.c btlib.c -o timserver

Run
   sudo ./timserver

Client can read time characteristic or enable 
notifications that are sent one per second

Connect from phone/tablet/PC to "Time" device
This code sets an unchanging random address.
If connection is unreliable try changing the address.

********************************/    

int lecallback(int clientnode,int op,int cticn);
void getcurtime(unsigned char *ct);

unsigned char curtime[10]; 
unsigned char locinfo[2];
unsigned char refinfo[4];
unsigned char name[4] = { 'B','T','F',0 };


int timeindex;
 
int main()
  {
  unsigned char uuid[2],randadd[6];
   
  if(init_blue("timserver.txt") == 0)
    return(0);
    
  if(localnode() != 1)
    {
    printf("ERROR - Edit timserver.txt to set ADDRESS = %s\n",device_address(localnode()));
    return(0);
    }

  // Write data to local characteristics
  
  getcurtime(curtime);  // read time to curtime[]
  uuid[0] = 0x2A;
  uuid[1] = 0x2B;
  timeindex = find_ctic_index(localnode(),UUID_2,uuid);
  write_ctic(localnode(),timeindex,curtime,10);

                  // local time information
  locinfo[0] = 0; // time zone in 15min steps  -48 to 56
  locinfo[1] = 0; // daylight savings time bits
                  // 0=standard  2=+0.5h  4=+1h   8=+2h                               
  uuid[0] = 0x2A;
  uuid[1] = 0x0F;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),locinfo,2);
  
                   // reference time information
  refinfo[0] = 0;  // time source 0=unknown 1=network 2=GPS 3=radio
                   // 4=manual 5=atomic clock 6=cellular network
  refinfo[1] = 255;  // time accuracy 255=unknown
                     // 1-253 drift in 1/8 second steps
  refinfo[2] = 255;  // days since update 255=greater than 254
  refinfo[3] = 255;  // hours since update 0-23  255=greater than 23                                    
  uuid[0] = 0x2A;
  uuid[1] = 0x14;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),refinfo,4);
               
  uuid[0] = 0x2A;   // manufacturer name
  uuid[1] = 0x29;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),name,4);

    // advertise current time service 1805 when set_le_random_address() is called
  uuid[0] = 0x18;
  uuid[1] = 0x05;
  uuid_advert(uuid);
  
  // Set unchanging random address by hard-coding a fixed value.
  // Choose a unique address so the client does
  // not get confused by multiple identities for a given address.
 
  // Choose the following 6 numbers
  randadd[0] = 0xD3;  // 2 hi bits must be 1
  randadd[1] = 0x56;
  randadd[2] = 0xD6;
  randadd[3] = 0x74;
  randadd[4] = 0x33;
  randadd[5] = 0x03;
  set_le_random_address(randadd);
       
  set_le_wait(20000);  // Allow 2 seconds for connection to complete
                                         
  // le_pair(localnode(),JUST_WORKS,0);  // If client requires security 

  le_server(lecallback,10);  // 1 second timer to send notifications
  
  close_all();
  return(1);
  }


int lecallback(int clientnode,int op,int cticn)
  {
  static int conflag = 0;
  static int notflag = 0;  
               
  if(op == LE_CONNECT)
    {
    printf("Connected OK\n");
    conflag = 1;
    }
  else if(op == LE_NOTIFY_ENABLE && cticn == timeindex)
    {
    printf("Notifications enabled\n");
    notflag = 1;
    }
  else if(op == LE_NOTIFY_DISABLE && cticn == timeindex)
    {
    printf("Notifications disabled\n");
    notflag = 0;
    }
  else if(op == LE_TIMER && notflag != 0)
    {
    getcurtime(curtime);  
    // send notification if client has enabled  
    write_ctic(localnode(),timeindex,curtime,10);
    }
  else if(op == LE_READ && cticn == timeindex && notflag == 0)
    {
    getcurtime(curtime);  // read time to curtime[]
    write_ctic(localnode(),timeindex,curtime,10);
    }
  else if(op == LE_DISCONNECT)
    return(SERVER_EXIT);
  return(SERVER_CONTINUE);
  }

void getcurtime(unsigned char *ct)
  {
  time_t tim;
  struct tm tm;  
  int year,day;
   
  time(&tim);
  localtime_r(&tim,&tm);
  
  year = tm.tm_year + 1900;
  day = tm.tm_wday;
  if(day == 0)
   day = 7;
  ct[0] = (unsigned char)(year & 0xFF);
  ct[1] = (unsigned char)((year >> 8) & 0xFF);
  ct[2] = (unsigned char)(tm.tm_mon + 1);  // month 1=jan
  ct[3] = (unsigned char)tm.tm_mday;  // day 1-31
  ct[4] = (unsigned char)(tm.tm_hour+1);  // hour 0-23
  ct[5] = (unsigned char)tm.tm_min;   // min 0-59
  ct[6] = (unsigned char)tm.tm_sec;   // sec 0-59
  ct[7] = (unsigned char)day;         // day of week 1=mon 7=sun
  ct[8] = 0;  // 1/256 second
  ct[9] = 1;  // adjust reason bits  0=manual 1=external reference
              // 2=change time zone  3=change daylight saving               
  }
