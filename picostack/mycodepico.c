#include <stdio.h>
#include <stdlib.h>
#include "btlib.h"


  // include for LED on/off
#include "pico/cyw43_arch.h"
  // callback for LE server
int lecallback(int node,int op,int cticn);

/********** DEVICES list ************
  Each line must end with \n\
  
  The LE server example has three characteritics;
     UUID = 2A00  Device name
     UUID = ABCD  Data  (can be read and written)
     UUID = CDEF  LED control (1 byte writeable)
                  Write 0 = LED off
                        1 = LED on
                        2 = LED flash 
  
************************************/

char *devices = { 
"DEVICE = Picostack type=mesh  node=1  address=local          \n\
PRIMARY_SERVICE=1800                                          \n\
  LECHAR=Device name    SIZE=16 PERMIT=02 UUID=2A00  ;index 0 \n\
PRIMARY_SERVICE=112233445566778899AABBCCDDEEFF00              \n\
   LECHAR = Data        SIZE=8  PERMIT=06 UUID=ABCD  ;index 1 \n\
   LECHAR = LED control SIZE=1  PERMIT=04 UUID=CDEF  ;index 2 \n\
"};
                                                                 

void mycode(void);

void mycode()
  {
  if(init_blue(devices) == 0)
    return;
    
  while(1)
    {
    le_server(lecallback,10);  // 10 = 1 second timer 
    }
  }

int lecallback(int node,int op,int cticn)
  {
  static unsigned char flashstate = 0;
  static unsigned char ledstate = 0;
  static int writeflag = 0;  // 1=Data written by client
 
  if(op == LE_CONNECT)
    printf("Connected\n");
  else if(op == LE_READ && cticn == 1)
    {
    // Client is about to read Data (index 1)
    // Set up data to send if not written by client
    if(writeflag == 0)
      write_ctic(localnode(),1,"Hello",5);
    }
  else if(op == LE_WRITE && cticn == 1)
    writeflag = 1;
  else if(op == LE_WRITE && cticn == 2)
    {
    // Client has written LED control (index 2)
    read_ctic(localnode(),2,&ledstate,1);  // read value to ledstate
    if(ledstate == 2)
      {
      flashstate = 1;
      ledstate = 1;
      }
    else
      flashstate = 0;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN,ledstate);   
    }
  else if(op == LE_TIMER && flashstate == 1)
    {
    ledstate = 1 - ledstate;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN,ledstate);       
    }
  
  // IF op == LE_DISCONNECT server keeps running and waits for another connecton
  
  return(SERVER_CONTINUE);
  }

