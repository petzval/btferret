#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btlib.h"

int callback(int node,unsigned char *data,int len);

int main()
  {
  if(init_blue("devices.txt") == 0)
    return(0);
  
  classic_server(ANY_DEVICE,callback,10,KEY_ON | PASSKEY_LOCAL);
  close_all();
  }
  
  
int callback(int node,unsigned char *data,int len)
  {
  static unsigned char *xmessage = {"Hello world\n"};
  static unsigned char *message = {"Send Hello to exit\n"};
  
  printf("Received: %s",data);
  if(data[0] == 'H')
    {
    write_node(node,xmessage,strlen(xmessage));
    printf("Disconnecting...\n");
    return(SERVER_EXIT);
    }
  write_node(node,message,strlen(message));
  return(SERVER_CONTINUE);
  }
