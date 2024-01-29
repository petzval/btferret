/****** SAMPLE mesh network procedure *********
REQUIRES
  btlib.c version 13
  btlib.h
  bluedot.txt 
COMPILE 
  gcc bluedot.c btlib.c -o bluedot
EDIT
  bluedot.txt to list the local device running this code
  and bluedot devices that may connect
  and set their ADDRESS=
RUN
  sudo ./bluedot
*********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "btlib.h" 


int bdotserver(int clientnode,unsigned char *inbuf,int count);
void help(void);

int main()
  {
  int node;
  char buf[16];
  
  if(init_blue("bluedot.txt") == 0)
    return(0);

  do
    {
    printf("\nEnter node number of Blue Dot device\n");
    printf("Or 0 for any device (h=help q=exit) : ");
    fgets(buf,16,stdin);  
    if(buf[0] == 'q')
      {
      printf("Exit...\n");
      close_all();
      return(0);
      }
    else if(buf[0] == 'h')
      help();
    else
      {
      node = atoi(buf);
      if(node == 0)
        node = ANY_DEVICE;
      else
        {     
        printf("\nYou have chosen %s as the Blue Dot device\n\n",device_name(node));  
        if(device_type(node) != BTYPE_CL) 
          printf("Not a classic device\n");
        }
      }
    }
  while(!(node == ANY_DEVICE || device_type(node) == BTYPE_CL));
  
  do
    {  
    printf("\n  s - Server - wait for Blue Dot device to pair or connect\n");
    printf("  h - Help\n");
    printf("  q - Exit\n");
    printf("Enter one of the above commands : ");
    fgets(buf,16,stdin);
    if(buf[0] == 's') 
      classic_server(node,bdotserver,10,KEY_ON | PASSKEY_LOCAL);
    else if(buf[0] == 'S')
      classic_server(node,bdotserver,10,KEY_OFF | PASSKEY_LOCAL);
    else if(buf[0] == 'h')
      help();       
    }
  while(buf[0] != 'q');
    
  printf("Exit\n");
  
  close_all();
  
  return(0);
  }



int bdotserver(int clientnode,unsigned char *buf,int count)
  {
  int n,col,row,op;
  double x,y;
  char *cmd;
  static char *opname[3] = { "release","press","move" };  
   
  op = atoi((char*)buf);
  if(op >= 0 && op <= 2)
    {
    n = 0;
    while(buf[n] != ',' && n < count)
      ++n;
    ++n;
    col = atoi((char*)buf+n);
    while(buf[n] != ',' && n < count)
      ++n;
    ++n;
    row = atoi((char*)buf+n);
    while(buf[n] != ',' && n < count)
      ++n;
    ++n;
    x = atof((char*)buf+n);
    while(buf[n] != ',' && n < count)
      ++n;
    ++n;
    y = atof((char*)buf+n);
    printf("Op=%d Button %s Col=%d Row=%d x=%.4g y=%.4g\n",op,opname[op],col,row,x,y);
    }
  else if(op == 3)
    printf("Protocol %s\n",buf);
  else
    {
    printf("Unknown operation\n");
    return(SERVER_CONTINUE);
    } 
    
  if(op == 0)
    {
    // button releae code here
    // col,row,x,y set
    }
  else if(op == 1)
    {
    // button press code here
    // col,row,x,y set
    }
  else if(op == 2)
    {
    // button move code here
    // col,row,x,y set
    }
  else if(op == 3)
    {
    // initial connection code here
    
    // e.g. send config command to display two buttons
    cmd = "4,#0000ffff,0,0,1,1,2\n";
    write_node(clientnode,(unsigned char*)cmd,strlen(cmd));
    
    // e.g. send button config command to make bottom colour red
    cmd = "5,#ff0000ff,0,0,1,0,1\n";
    write_node(clientnode,(unsigned char*)cmd,strlen(cmd));
    }    
   
  return(SERVER_CONTINUE); // wait for next node packet
  }


void help()
  {
  printf("\nEdit bluedot.txt to set the addresses of this device and\n");
  printf("the Blue Dot device. There is an option to accept a connection\n");
  printf("from any Blue Dot device. Find the address of the Android Blue Dot\n");
  printf("device by turning Bluetooth on, then Settings/About/Status\n\n");
  printf("If this device has been previously paired with the Blue Dot device\n");
  printf("using other software, unpair it from the Blue Dot device first.\n\n");
  printf("Pair by entering s here, then on the Blue Dot device:\n");
  printf("  Settings/Bluetooth, tap this device. Tap OK within 10 seconds if\n");
  printf("  asked to confirm a passkey. If asked for a PIN, enter 0000.\n");
  printf("  Wait for Blue Dot device to pair and disconnect. This device\n");
  printf("  will then wait for a connection from Blue Dot as below.\n\n");
  printf("If already paired, enter s here. This device will wait for a\n"); 
  printf("connection from Blue Dot. Start Blue Dot and tap this device.\n");
  printf("Tapping the bluedot buttons will trigger thc callback code here\n\n");
  }
