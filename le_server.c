#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btlib.h"

int callback(int clientnode,int operation,int ctic_index);

int main()
  {
  unsigned char *mydata = {"Hello world"};
    // random address - 2 hi bits of [0] must be 1
  static unsigned char randadd[6] = {  0xD3,0x56,0xDB,0x24,0x32,0xA0 };

  
  if(init_blue("devices.txt") == 0)
    return(0);

  printf("\nThe local device must be the first entry in devices.txt\n");
  printf("(MyPi) that defines the LE characteristics\n");  
    // Set My data (index 1) value  
  write_ctic(localnode(),1,mydata,strlen(mydata));    

  /********* CONNECTIOM/PAIRING problems ********/
  
  // If you have connection problems - especially from
  // Android/iOS/Windows, uncomment these three instructions
  // to use a random address. This creates a new identity
  // for the server, with a different Bluetooth address.
  // Choose 6 bytes for randadd[] = random address
  
  //set_le_random_address(randadd);
  //set_le_wait(5000);   // wait 5 seconds for connection/pairing                                         
  //le_pair(localnode(),JUST_WORKS,0);  // Easiest option, but if client requires
                                      // passkey security - remove this command  
  
  /******** end CONNECTION problems *******/


  le_server(callback,0);  // timerds=0
  close_all();
  return(1);
  } 

int callback(int clientnode,int operation,int cticn)
  {
  if(operation == LE_CONNECT)
    {
    // clientnode has just connected
    printf("Connected\n");
    }
  else if(operation == LE_READ)
    {
    // clientnode has just read local characteristic index cticn
    }
  else if(operation == LE_WRITE)
    {
    // clientnode has just written local characteristic index cticn
    }
  else if(operation == LE_DISCONNECT)
    {
    // clientnode has just disconnected
    // uncomment next line to stop LE server when client disconnects
    // return(SERVER_EXIT);
    // otherwise LE server will continue and wait for another connection
    // or operations from other clients that are still connected
    printf("Disconnected\n");
    return(SERVER_EXIT);
    }
  else if(operation == LE_TIMER)
    {
    // The server timer calls here every timerds deci-seconds 
    // clientnode and cticn are invalid
    // This is called by the server not a client
    }
  else if(operation == LE_KEYPRESS)
    {
    // Only active if keys_to_callback(KEY_ON,0) has been called before le_server()
    // clientnode is invalid
    // cticn = key code
    //       = ASCII code of key (e.g. a=97) OR
    //         btferret custom code for other keys such as Enter, Home, PgUp
    //         Full list in keys_to_callback() section
    }
    
  return(SERVER_CONTINUE);
  }



