#include <stdio.h>
#include <stdlib.h>
#include "btlib.h" 

/********** Bluetooth LE Cycle speed and cadence server **********
From https://github.com/petzval/btferret
  Version 20 or later

Download
  cycle.c    this code
  cycle.txt
  btlib.c
  btlib.h
   
Edit cycle.txt to set ADDRESS=
to the address of the local device
that runs this code

Compile
  gcc cycle.c btlib.c -o cycle

Run
  sudo ./cycle

Connect from phone/tablet/PC to "Cycle" device

Notifications of cadence and speed are sent once per second
Constant cadence = 60 per min
Constant wheel rotations = 60 per min

This code sets an unchanging random address.
If connection is unreliable try changing the address.
********************************/    

int lecallback(int clientnode,int op,int cticn);

unsigned char feat[4] = { 0x03,0,0,0 };   // wheel and crank data in notification
unsigned char sens[1] = { 0 };         // sensor location 0=Other
                                       // 1=top of shoe 2=in shoe 3=hip 4=front wheel 5=left crank
                                       // 6=right crank 7=left pedal 8=right pedal 9=front hub
                                       // 10=rear dropout 11=chainstay 12=rear wheel 13=rear hub
                                       // 14=chest 15=spider 16=chain ring                                    
unsigned char cycle[16] = { 0x30,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };  // 3=wheel and crank data
unsigned char name[4] = { 'B','T','F',0 };

int cycleindex,controlindex;

int main()
  {
  unsigned char uuid[2],randadd[6];
   
  if(init_blue("cycle.txt") == 0)
    return(0);
    
  if(localnode() != 1)
    {
    printf("ERROR - Edit cycle.txt to set ADDRESS = %s\n",device_address(localnode()));
    return(0);
    }

  // Write data to local characteristics
  uuid[0] = 0x2A;
  uuid[1] = 0x5C;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),feat,2); 

  uuid[0] = 0x2A;
  uuid[1] = 0x5B;
  cycleindex = find_ctic_index(localnode(),UUID_2,uuid);
  write_ctic(localnode(),cycleindex,cycle,11);

  uuid[0] = 0x2A;
  uuid[1] = 0x5D;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),sens,1);

  uuid[0] = 0x2A;
  uuid[1] = 0x29;
  write_ctic(localnode(),find_ctic_index(localnode(),UUID_2,uuid),name,4);

  uuid[0] = 0x2A;
  uuid[1] = 0x55;
  controlindex = find_ctic_index(localnode(),UUID_2,uuid);

    // advertise cycling speed and cadence service 1816
    // when set_le_random_address() is called
  uuid[0] = 0x18;
  uuid[1] = 0x16;
  uuid_advert(uuid);

  // Set unchanging random address by hard-coding a fixed value.
  // Choose a unique address so the client does not get
  // confused by multiple identities for a given address.
 
  // Choose the following 6 numbers
  randadd[0] = 0xD3;  // 2 hi bits must be 1
  randadd[1] = 0x56;
  randadd[2] = 0xD6;
  randadd[3] = 0x74;
  randadd[4] = 0x33;
  randadd[5] = 0x02;
  set_le_random_address(randadd);
       
  set_le_wait(2000);  // Allow 2 seconds for connection to complete
                                         
  le_server(lecallback,10);   // 1 seond timer to send notification 
  
  close_all();
  return(1);
  }


int lecallback(int clientnode,int op,int cticn)
  {
  int n,k;
  unsigned int tim;
  unsigned char buf[16],reply[16];
  static int conflag = 0;
  static int notflag = 0;
  static int lastwt;
  static int lastct;
  static int crev;
  static int wrev;
          
  if(op == LE_CONNECT)
    {
    conflag = 1;
    crev = 0;
    wrev = 0;
    lastwt = 0;
    lastct = 0;
    printf("Connected OK\n");
    }
  else if(op == LE_NOTIFY_ENABLE && cticn == cycleindex)
    {
    printf("Notifications enabled\n");
    notflag = 1;
    }
  else if(op == LE_NOTIFY_DISABLE && cticn == cycleindex)
    {
    printf("Notifications disabled\n");
    notflag = 0;
    }
  else if(op == LE_TIMER && notflag != 0)
    {
    /********
    send notification 11 bytes
    [0] = flags     00000011 = wheel and crank data
    [1][2][3][4] = cumulative count of wheel rotations (lo byte first)
    [5][6] = time of last wheel event (1/1024 s units)
    [7][8] = cumulative count of crank rotations
    [9][10] = time of last crank event
    *******/
     
    ++wrev;   // one more wheel rev
    lastwt += 1024;  // in 1 second
    ++crev;       // one more crank rev
    lastct += 1024;  // in 1 second

    // wrev=cumulative wheel rotations  lastwt=last wheel event time
    // crev=cumulative crank rotations  lastct=last crank event time           
    cycle[0] = 3;  // wheel and crank data
    cycle[1] = wrev & 0xFF;
    cycle[2] = (wrev >> 8) & 0xFF;
    cycle[3] = (wrev >> 16) & 0xFF;
    cycle[4] = (wrev >> 24) & 0xFF;
    cycle[5] = lastwt & 0xFF;
    cycle[6] = (lastwt >> 8) & 0xFF;
    cycle[7] = crev & 0xFF;
    cycle[8] = (crev >> 8) & 0xFF;
    cycle[9] = lastct & 0xFF;
    cycle[10] = (lastct >> 8) & 0xFF;
    
    // send 11-byte notification    
    write_ctic(localnode(),cycleindex,cycle,11);
    }
  else if(op == LE_WRITE)
    {
    if(cticn == controlindex)
      {  // Control point
      read_ctic(localnode(),cticn,buf,16);
      printf("Contol point received opcode %02X\n",buf[0]);
      // opcode 1=set cumulative wheel rotations = buf[1]-[4]
      //        2=calibration
      //        3=update sensor location = buf[1]
      //        4=request sensor location
      // Do nothing but send OK reply
      reply[0] = 0x10;  // response opcode
      reply[1] = buf[0];  // request opcode
      reply[2] = 1;  // 1=success
                     // 2=opcode not supported
                     // 3=invalid parameter
                     // 4=response failed
      // reply[3]... data if required
      // send reply as indication
      write_ctic(localnode(),controlindex,reply,3);
      }
    }
  else if(op == LE_DISCONNECT)
    return(SERVER_EXIT);
    
  return(SERVER_CONTINUE);
  }

