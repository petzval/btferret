#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btlib.h"

int main()
  {
  int channel;
  unsigned char *message = {"Hello world\n"};
   
  if(init_blue("devices.txt") == 0)
    return(0);
  
  printf("Node 4 must be TYPE=CLASSIC in devices.txt\n");  
    // find standard serial channel
  channel = find_channel(4,UUID_16,strtohex("00001101-0000-1000-8000-00805F9B34FB",NULL));  
  connect_node(4,CHANNEL_NEW,channel);
  printf("Send Hello world to client (assumes line end character = Line feed)\n");
  write_node(4,message,strlen(message));
  disconnect_node(4);
  close_all();
  }
