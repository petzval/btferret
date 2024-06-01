#include <stdio.h>
#include <stdlib.h>
#include "btlib.h" 


/***************
Send OBEX protocol data to node 4 which must be an OBEX push server

Windows: Settings/Devices/Send or receive files via Bluetooth/Receive files
Android: Enabling Bluetooth will normally make the device an OBEX server

Sends NAME = hello.txt  DATA = Hello
The receiver will nomally save DATA in a file called NAME

For fully-programmed file send code see sendfileobex() in btferret.c

************/

int main()
  {
  int channel,len;
  unsigned char inbuf[64];
  unsigned char connect[7] = {0x80,0x00,0x07,0x10,0x00,0x01,0x90};
  unsigned char send[39] = {0x82,0x00,0x27,
                            0x01,0x00,0x17,0,'h',0,'e',0,'l',0,'l',0,'o',0,'.',0,'t',0,'x',0,'t',0,0,
                            0xC3,0,0,0,5,
                            0x49,0,8,'H','e','l','l','o'};
  unsigned char disconnect[3] = {0x81,0x00,0x03};   
  
  
  if(init_blue("devices.txt") == 0)
    return(0);
  
  printf("Node 4 must be TYPE=CLASSIC in devices.txt with 1105 OBEX service\n");  
    // find OBEX push server channel
  channel = find_channel(4,UUID_2,strtohex("1105",NULL));
  if(channel <= 0)
    channel = find_channel(4,UUID_16,strtohex("00001105-0000-1000-8000-00805F9B34FB",NULL));
  if(channel <= 0)
    {
    printf("OBEX seervice not found\n");
    close_all();
    return(0);
    }    
    
  connect_node(4,CHANNEL_NEW,channel);
  
  printf("Send hello.txt containing Hello\n");
 
  write_node(4,connect,7);
  inbuf[0] = 0;
  // wait for Success reply 0x0A
  len = read_node_endchar(4,inbuf,64,PACKET_ENDCHAR,EXIT_TIMEOUT,5000);
  if(len == 0 || inbuf[0] != 0xA0)
    printf("OBEX Connect failed\n");

  write_node(4,send,39);
  inbuf[0] = 0;
  len = read_node_endchar(4,inbuf,64,PACKET_ENDCHAR,EXIT_TIMEOUT,5000);
  if(len == 0 || inbuf[0] != 0xA0)
    printf("Send failed\n");

  write_node(4,disconnect,3);
  inbuf[0] = 0;
  len = read_node_endchar(4,inbuf,64,PACKET_ENDCHAR,EXIT_TIMEOUT,5000);
  if(len == 0 || inbuf[0] != 0xA0)
    printf("OBEX Disconnect failed\n");
   
  disconnect_node(4);
  close_all();
  }
