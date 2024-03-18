#include <stdio.h>
#include <stdlib.h>
#include "btlib.h"

int main()
  {
  int index;
  unsigned char buf[64],uuid[2];
  
  if(init_blue("devices.txt") == 0)
    return(0);
  
  printf("Node 7 must be TYPE=LE in devices.txt\n");  
  connect_node(7,CHANNEL_LE,0);
  find_ctics(7);  // Read services
   // Device name UUID = 2A00
  uuid[0] = 0x2A;
  uuid[1] = 0x00;
  index = find_ctic_index(7,UUID_2,uuid);
   // Read device name 
  read_ctic(7,index,buf,sizeof(buf));
  printf("Device name = %s\n",buf);
  disconnect_node(7); 
  close_all();
  }
