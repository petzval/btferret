#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "btlib.h"

int motorcontrol(int node,int dirn,int speed);

int main()
  {
  if(init_blue("rn4020.txt") == 0)
    return(0);

   // connect to RN4020 - LE only needs first parameter node=2
  if(connect_node(2,0,0) == 0)
    return(0);
       
   // RN4020 node = 2
   // Direction = 0  forward
   // Speed = 16 
  motorcontrol(2,0,16);
  sleep(4);
    //  Speed = 24
  motorcontrol(2,0,24);
  sleep(4);
    //  Speed = 0
  motorcontrol(2,0,0);
  
    // disconnect RN4020
  disconnect_node(2);
  
  close_all();
  
  return(0);
  }

/*********** MOTOR CONTROL **************
 node = node of RN4020
 dirn = direction  0=forward   1=back
 speed = 0 to 32
**********************************/       

int motorcontrol(int node,int dirn,int speed)
  {
  unsigned char dat;    
   
  if(speed < 0 || speed > 32)
    {
    printf("Invalid speed\n");
    return(0);
    }
 
   // decide Control characteristic (script will send to dirn and enable pins)
  if(speed == 0)         
    dat = 4;            // STOP      enable hi,  dirn lo
  else if(dirn == 0)
    dat = 0;            // FORWARD   enable lo,  dirn lo
  else
    dat = 2;            // BACKWARD  enable lo,  dirn hi

    // write Control ctic index 1
    // the ctic index is determined by the order of listing in rn4020.txt
    // last parameter can be 0 because rn4020.txt has specified the size=1
  write_ctic(node,1,&dat,0);
  
    // write PWMhi ctic index 2 
  dat = (unsigned char)speed;
  write_ctic(node,2,&dat,0);
  
    // write PWMlo ctic index 3   
  dat = (unsigned char)(32-speed);
  write_ctic(node,3,&dat,0);
  
    // write ALERT=2 ctic index 0 to trigger @ALERTH script  
  dat = 2;          
  write_ctic(node,0,&dat,0); 
     
  return(1);    
  }
