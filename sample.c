/****** SAMPLE mesh network procedure *********
COMPILE 
  gcc sample.c btlib.c -o sample
EDIT
  sampledev.txt to list all network devices
RUN
  1. With sampledev.txt in the same directory
  2. This same code on all three Mesh Pis
  3. Start Mesh Pis 2 and 3 first, then start 1
*********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "btlib.h"

int hubnode(void);
int meshserver(int clientnode,unsigned char *inbuf,int count);
int nodeserver(int clientnode,unsigned char *inbuf,int count);

int main()
  {
  if(init_blue("sampledev.txt") == 0)
    return(0);
        
  if(localnode() == 1)
    {
    // this device is the hub controller
    hubnode();
    }
  else
    {
    // this device is not the hub
    // set up as a mesh server running meshserver callback
    mesh_server(meshserver);
    }

  printf("Finished - disconnecting...\n");
  
  sleep(3);  // wait for any pending disconnects to complete 
      
  close_all();
  
  return(0);
  }


int hubnode()
  {
  int len,channel;
  unsigned char buf[8],name[64];

  // send mesh packet to all mesh devices = 2 10 4 
  // node 3 is running meshserver callback which ignores this packet
  // node 2 is running meshserver callback which
  // interprets this as follows:
 
  printf("Broadcast mesh packet asking node 2 to become a node server\n");
  buf[0] = 2;   // this packet for node 2 mesh server
  buf[1] = 10;  // node server should use this termination char
  buf[2] = 4;   // become a node server using above term char
                // note mesh packets do not use termination characters
                // and operate via byte counts 

  if(write_mesh(buf,3) != 3)
    printf("Write mesh error\n");

    // For write_mesh, the packet will not be sent immediately
    // because the advertising repeat rate is only about 3 per second.
    // Always allow for this possible delay.
    // Do not call another write_mesh until you are sure this one has
    // been sent - by adding a delay for example. 
     
  sleep(2);  // 2 second delay
   
  // node 2 should now be listening for a connection from this device 
  // connect to it as a node server
  
  printf("Connect to node 2 as a node server\n");
  
  if(connect_node(2,CHANNEL_NODE,0) == 0)  // 3rd parameter ignored
    printf("Connect node error\n");
     
  // send node packet to node 2 = 7 5 10
  // node 2 is now running nodeserver callback which
  // interprets this as follows:
  
  printf("Send command to node 2 to read name of LE node 7\n");  
  buf[0] = 5;   // read name of LE device command
  buf[1] = 7;   // LE node to read
  buf[2] = 10;  // termination char expected by node server
  if(write_node(2,buf,3) != 3)
    printf("Write node error\n");
     
  // read reply from node 2 that should be the name of LE node 5
  // plus a 10 termination character added by nodeserver()
  // followed by a zero added by read_node_endchar()
  // 8 second time out.
   
  printf("Wait for reply from node 2 with name\n");
  
  len = read_node_endchar(2,name,sizeof(name),10,EXIT_TIMEOUT,8000); 
  if(read_error() != 0)
    printf("Read name from node 2 error\n");
  else
    {   
    printf("Got name from node 2 = %s\n",name);
     
    printf("Connect to classic server node 4 to find standard UUID serial channel\n"); 
   
    channel = find_channel(4,UUID_16,strtohex("0000-1101-0000-1000-8000-00805F9B34FB",NULL));
       
       // for a Windows COM port, use the standard 2-byte UUID instead:
       // channel = find_channel(4,UUID_2,strtohex("1101",NULL));
   
    if(channel <= 0)
      printf("Failed to find channel\n");
    else
      { 
      // connect to classic server node 4 on found channel
      printf("Connecting on found channel = %d\n",channel);
  
      if(connect_node(4,CHANNEL_NEW,channel) == 0)
        printf("Connect failed\n");
      else
        {
        // send name to node 4 with its termination char 10
        printf("Send name to node 4\n");
  
        if(write_node(4,name,len) != len)
          printf("Failed\n");
    
        // disconnect classic server which is not programmed to
        // initiate disconnection, so initiate disconection here
  
        sleep(2); // give server 2s before disconnecting  
   
        printf("Disconnect node 4\n");
  
        disconnect_node(4);
        }
      }
    }
       
  // server-initiated disconnection    
  // node 2 is still a node server
  // send node packet to node 2 = 'D'
  // node 2 is programmed in nodeserver() below to
  // interpret this as an instruction to
  // initiate disconnection and return to being a mesh server
  
  printf("Send disconnect as node server command to node 2\n");
  
  buf[0] = 'D';  // disconnect command
  buf[1] = 10;   // termination char expected by node server
  if(write_node(2,buf,2) != 2)
    printf("Write node error\n");
  
  // wait for node 2 to initiate disconnection
  // if not done within timeout of 3s - force a disconnect 
  
  wait_for_disconnect(2,3000);

  // send mesh packet = 'D'
  // all mesh nodes (2 and 3) running meshserver callback
  // interpret this as a command to stop being mesh servers
  // and their programs will exit
  
  printf("Broadcast disconnect to all mesh servers - 2 and 3\n");
  
  buf[0] = 'D';
  write_mesh(buf,1);
  sleep(1);   // allow time for mesh packet to transmit before exit                          
  
  // return and exit program
  return(1);
  }


int meshserver(int clientnode,unsigned char *inbuf,int count)
  {
  if(inbuf[0] == 'D')
    {
    printf("Got mesh server exit command\n");
    return(SERVER_EXIT);  // exit command
    }
  
  if(inbuf[0] == localnode())  
    {   // command is for this node
    printf("Got mesh packet for this device\n");
    if(inbuf[2] == 4)
      { 
      printf("Become a node server\n");
      // become a node server and listen for clientnode to connect
      // use a termination character for reads and writes = inbuf[1] = 10
      node_server(clientnode,nodeserver,inbuf[1]);
      // while node_server is running it is using nodeserver()
      // on return - has disconnected as node server and is now a mesh server again  
      }
    else
      printf("No action\n"); 
    }
  else
    printf("Mesh packet not for this device\n");
  
  return(SERVER_CONTINUE);  // wait for next mesh packet
  }
  

int nodeserver(int clientnode,unsigned char *inbuf,int count)
  {
  int len,lenode,index;
  unsigned char name[64];
  
  if(inbuf[0] == 'D')      // disconnect command
    {
    printf("Got node server disconnect command\n");
    return(SERVER_EXIT);   // return to meshserver
    }

  printf("Got node packet\n");    
  if(inbuf[0] == 5)      // read LE name command
    {   
    lenode = inbuf[1];   // node of LE device
    printf("Read name of LE node %d command\n",lenode);

     // connect LE device
    if(connect_node(lenode,CHANNEL_LE,0) == 0)
      printf("Failed to connect to LE device\n");
    else
      { 
      // connected OK      
      // read lenode name characteristic 
    
      // find index of characteristic with UUID=2A00
      // which is the standard UUID for the device name     
      find_ctics(lenode);  // read characteristic info
      index = find_ctic_index(lenode,UUID_2,strtohex("2A00",NULL));
     
      if(index < 0)
        {
        printf("Failed to find index\n");
        len = 0;
        }
      else
        len = read_ctic(lenode,index,name,sizeof(name));
             
      if(len == 0)
        {
        printf("Failed to read name\n");
        sprintf((char*)name,"Fail");  // send this as name
        len = strlen((char*)name);
        }
      else
        printf("Read name = %s\n",name);
                      
                        // send reply even if failed
                        // len = number of chars in name - may include term 0
       if(len > 0 && name[len-1] == 0)
         --len;         // len = index of term 0
       name[len] = 10;  // add termination char that client expects
       ++len;           // include termination char in count to send
   
      printf("Sending name to client\n");
      
      // send name to hub node client
      if(write_node(clientnode,name,len) != len)
        printf("Failed to send reply\n");

      printf("Disconnect LE device\n");
      
          // disconnct LE device
      disconnect_node(lenode);    
      }
    }
  else
    printf("No action\n");
    
  return(SERVER_CONTINUE); // wait for next node packet
  }


