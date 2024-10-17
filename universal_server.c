#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btlib.h"


int universal_callback(int clientnode,int operation,int cticn,unsigned char *buf,int nread);

int main()
  {
  char *mydata = {"Hello world"};

  if(init_blue("devices.txt") == 0)
    return(0);

  printf("\nThe local device must be the first entry in devices.txt\n");
  printf("(MyPi) that defines the LE characteristics\n");  
    // Set My data LE characteristic (index 1) value  
  write_ctic(localnode(),1,(unsigned char*)mydata,strlen(mydata));    
  
  universal_server(universal_callback,10,KEY_ON | PASSKEY_LOCAL,0);  // 0 = no timer
  close_all();
  }

int universal_callback(int clientnode,int operation,int cticn,unsigned char *buf,int nread)
  {
  unsigned char dat[256]; 
  int n,nreadle;
  static char reply[16] = "Hello World\n";
  
  if(operation == LE_CONNECT)
    printf("  LE %s has connected\n",device_name(clientnode));
  else if(operation == LE_READ)
    printf("  LE %s has read local characteristic %s\n",device_name(clientnode),ctic_name(localnode(),cticn));
  else if(operation == LE_WRITE)
    {
    // read local characteristic that client has just written
    nreadle = read_ctic(localnode(),cticn,dat,sizeof(dat));
    printf("  LE %s has written local characteristic %s =",device_name(clientnode),ctic_name(localnode(),cticn));
    for(n = 0 ; n < nreadle ; ++n)
      printf(" %02X",dat[n]);
    printf("\n");
    }
  else if(operation == LE_DISCONNECT)
    {
    printf("  LE %s has disconnected - waiting for another connection\n",device_name(clientnode));
    // return(SERVER_EXIT);  // to exit server
    }
  else if(operation == SERVER_TIMER)
    {
    printf("  Timer\n");
    }      
  else if(operation == CLASSIC_DATA)
    {
    printf("Classic data from %s: %s",device_name(clientnode),buf);
    write_node(clientnode,(unsigned char*)reply,strlen(reply));  // send Hello World reply
    }
    
  return(SERVER_CONTINUE);    // loop for another packet
  }
