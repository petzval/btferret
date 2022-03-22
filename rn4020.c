#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "btlib.h"

int motorcontrol(int node,int dirn,int speed);

int main()
  {
  if(init_blue("rn4020.txt") == 0)
    return(0);

   // connect to RN4020 - just need node=2
  if(connect_node(2,0,0) == 0)
    return(0);
       
   // RN4020 node = 2
   // Direction = 0  forward
   // Speed = 20 
  motorcontrol(2,0,20);
  sleep(4);
    //  Speed = 30
  motorcontrol(2,0,30);
  sleep(4);
    //  Speed = 0
  motorcontrol(2,0,0);
  sleep(1);
    
    // disconnect RN4020
  disconnect_node(2);
  
  close_all();
  
  return(0);
  }

/*********** MOTOR CONTROL ************
 node = node of RN4020
 dirn = direction  0=forward   1=back
 speed = 0 to 64
**********************************/       

int motorcontrol(int node,int dirn,int speed)
  {
  int ctl;
  unsigned char dat[16];    
   
  if(speed < 0 || speed > 64)
    {
    printf("Invalid speed\n");
    return(0);
    }
 
  if(speed == 0)         
    {
    dat[0] = 0;   // data byte - ignored
      // write to Stop characteristic index 1
    write_ctic(node,1,dat,0);    // calls ?FUNC2 sets enable hi, 
                                 // dirn lo, and stops PWM signal
    return(1);
    }
    
  if(dirn == 0)
    ctl = 0;     // FORWARD   enable lo,  dirn lo
  else
    ctl = 2;     // BACKWARD  enable lo,  dirn hi

    // construct 8 byte ascii string for Control data = PWMhi,PWMlo,ctl
  sprintf(dat,"%02X,%02X,%02X",speed,64-speed,ctl);
    // write to Control charcteristic index 0 calls ?FUNC1
    // last parameter can be 0 because rn4020.txt has specified size=1
  write_ctic(node,0,dat,0);
       
  return(1);    
  }
