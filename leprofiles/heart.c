#include <stdio.h>
#include <stdlib.h>
#include "btlib.h"  

/********** Bluetooth LE heart monitor **********
Download from https://github.com/petzval/btferret
  btlib.c    Version 20 or later
  btlib.h    Version 20 or later

Download
   heart.c
   heart.txt
 
Edit heart.txt to set ADDRESS=
to the address of the local device
that runs this code

Compile
  gcc heart.c btlib.c -o heart

Run
  sudo ./heart

Connect from phone/tablet/PC to "Heart" device

Simulates a slowly changing average heart rate
with a +/-25ms random variation in the intervals   

This code sets an unchanging random address.
If connection is unreliable try changing the address.

**************/



int lecallback(int clientnode,int op,int cticn);

unsigned char sens[1] = { 0x01 };  // sensor location 1=Chest
                                   // 0=Other 1=Chest 2=Wrist 3=Finger
                                   // 4=Hand  5=Ear lobe 6=Foot
unsigned char heart[64] = { 0,0,0,0,0,0,0,0 };
unsigned char name[4] = { 'B','T','F',0 };


int notifyindex,controlindex;
 
int main()
  {
  unsigned char uuid[2],randadd[6];
   
  if(init_blue("heart.txt") == 0)
    return(0);
    
  if(localnode() != 1)
    {
    printf("ERROR - Edit heart.txt to set ADDRESS = %s\n",device_address(localnode()));
    return(0);
    }

  // Write data to local characteristics
  uuid[0] = 0x2A;
  uuid[1] = 0x5D;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),sens,1);

  uuid[0] = 0x2A;
  uuid[1] = 0x37;
  notifyindex = find_ctic_index(localnode(),UUID_2,uuid);
  write_ctic(localnode(),notifyindex,heart,2);

  uuid[0] = 0x2A;
  uuid[1] = 0x29;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),name,4);

  uuid[0] = 0x2A;
  uuid[1] = 0x39;
  controlindex = find_ctic_index(localnode(),UUID_2,uuid);

  // advertise heart service 180D when set_le_random_address() is called
  uuid[0] = 0x18;
  uuid[1] = 0x0D;
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
  randadd[5] = 0x01;
  set_le_random_address(randadd);
       
  set_le_wait(2000);  // Allow 2 seconds for connection to complete
                                         
  //  le_pair(localnode(),JUST_WORKS,0);  // If client requires security   

  le_server(lecallback,10);  // 1 second timer to send notifications
 
  close_all();
  return(1);
  }


int lecallback(int clientnode,int op,int cticn)
  {
  int n,hn,rr,randel;
  unsigned int tim;
  double ran;
  unsigned char buf[16],reply[16];
  static int rate = 100;
  static unsigned int beatlist[8];
  static int flag = 0;  
  static int beat = 1000;
  static int delbeat = -50;
             
  if(op == LE_CONNECT)
    {
    flag = 1;
    printf("Connected OK. Sending notifications\n");
    // set up simulated data with an array of beat times
    beatlist[0] = (unsigned int)time_ms() + beat;
    beat += delbeat;
    for(n = 1 ; n < 8 ; ++n)
      {
      ran = (double)rand()/RAND_MAX;
      randel = (int)((ran * 100.0) - 25.0);
      beatlist[n] = beatlist[n-1] + beat + randel;
      beat += delbeat;
      }   
    }
  if(op == LE_TIMER && flag != 0)
    {
    /*** send notification
    heart[0] = flags  // bit 0
                      //   xxxxxxx0 = 1-byte rate follows
                      //   xxxxxxx1 = 2-byte rate follows
                      // bits 1/2
                      //   xxxxx00x = no sensor contact info
                      //   xxxxx10x = sensor not in contact
                      //   xxxxx11x = sensor in contact
                      // bit 4
                      //   xxxx0xxx = no rr interval data
                      //   xxxx1xxx = variable number of 2-byte rr intervals follow
    heart[1] = rate bpm
      // followed by a variable number (maybe 0)
      // of 2-byte intervals (1/1024 s count)
    heart[2][3] = 1st interval
    heart[4][5] = 2nd interval
    ...
    
    This code simulates a slowly changing average rate with
    a +/-25ms random variation in the intervals   
    *******/
    
    // set average rate
    rate = (60*4096)/ (beatlist[4] - beatlist[0]);
    heart[0] = 0; 
    heart[1] = rate;  // beats per min 

    // set up simulated interval data
    // add interval data as required
    tim = (unsigned int)time_ms();
    hn = 0;
    while(tim > beatlist[1])
      {
      // add a RR interval entry
      heart[0] |= 16;  // flags set RR data bit
      // rr = interval between beats in 1/1024 second count
      rr = (int)(beatlist[1] - beatlist[0]);
      // add 2-byte RR data
      heart[hn+2] = rr & 0xFF;          // RR lo byte
      heart[hn+3] = (rr >> 8) & 0xFF;   // RR hi byte
      hn += 2;  // add two bytes to notification data send
     
      for(n = 0 ; n < 7 ; ++n)
        beatlist[n] = beatlist[n+1];
      ran = (double)rand()/RAND_MAX;
      randel = (int)((ran * 50.0) - 25.0);
      beatlist[7] = beatlist[6] + beat + randel;
      beat += delbeat;
      if(beat > 1200)
        delbeat = -50;
      if(beat < 400)
        delbeat = 50;
      }
    // end set up simulated data
      
    // send notification by writing to local characteristic  
    write_ctic(localnode(),notifyindex,heart,2+hn);
    }
  if(op == LE_WRITE)
    {
    if(cticn == controlindex)
      {
      read_ctic(localnode(),cticn,buf,16);
      printf("Control point opcode %02X received\n",buf[0]);
      // opcode 1=zero expended energy
      // Do nothing
      }
    }
  if(op == LE_DISCONNECT)
    return(SERVER_EXIT);
  return(SERVER_CONTINUE);
  }

