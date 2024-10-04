#include <stdio.h>
#include <stdlib.h>
#include "btlib.h" 

/********* OBEX push server
Receives OBEX NAME and DATA
Saves DATA in file called NAME in the obex_server directory

Start this server, then on the remote device:
  Windows: Settings / Devices / Send or receive files via Bluetooth / Send files
             selcet No Authentication, or pair this receiving device first
  Android: Share / via Bluetooth
***********/

int callback(int node,unsigned char *data,int len);

int main()
  {
  int security,keyflag;
  
  if(init_blue("devices.txt") == 0)
    return(0);
  
  printf("IF FAILS - Experiment with security = 0/1/2/3\n");
  security = 0;

  keyflag = KEY_ON | PASSKEY_LOCAL;
  if(security == 1)
    keyflag = KEY_OFF | PASSKEY_LOCAL;  
  else if(security == 2)
    keyflag = KEY_OFF | PASSKEY_OFF;
  else if(security == 3)
    keyflag = KEY_ON | PASSKEY_OFF;    
  
  printf("Waiting to receive OBEX file. You may need to pair this device first\n");
  classic_server(ANY_DEVICE,callback,PACKET_ENDCHAR,keyflag);
 
  close_all();
  }
  
  
int callback(int node,unsigned char *data,int len)
  {
  int n,j,k,ilen,length,hi,datan,datalen;
  char filename[256]; 
  static int count = 0;
  static int connected_node = 0;
  static int file_length;
  static FILE *stream = NULL;
  static unsigned char connect_success[7] = {0xA0,0x00,0x07,0x10,0x00,0x01,0x90};
  static unsigned char connect_fail[7] = {0xC3,0x00,0x03};
  static unsigned char success[3] = {0xA0,0x00,0x03};
  static unsigned char continue_reply[3] = {0x90,0x00,0x03};
   
  datalen = 0;  // no data
  
  // data[0] = opcode (section 3.4 in OBEX15.pdf)
  
  if(data[0] == 0x80)
    {  // Connect 
    if(connected_node == 0)
      {  // not connected to another device
      printf("OBEX connect\n");
      write_node(node,connect_success,7);    
      connected_node = node;
      count = 0;  // data bytes received
      if(stream != NULL)
        fclose(stream);
      stream = NULL;
      }
    else
      { // already connected to another device - refuse
      printf("Node %d trying to OBEX connect - refuse\n",node);
      write_node(node,connect_fail,3);
      }
    return(SERVER_CONTINUE);
    }
      
  if(node != connected_node)
    {
    printf("Node %d not OBEX connected\n",node);
    return(SERVER_CONTINUE);
    }  
  
  if((data[0] & 0x7F) == 0x02)
    { // 0x02 or 0x82 Put
    length = (data[1] << 8) + data[2];  // should = len
    n = 3;  // 1st header item
    while(n < length && n < len)
      {
      hi = data[n];  // header item identifier (section 2.1 in OBEX15.pdf)
      if((hi & 0xC0) == 0x80)
        {  // 1-byte value
        if(hi == 0x97)
          {  // Single Response Mode
          printf("SRM not programmed\n");
          write_node(node,connect_fail,3);
          connected_node = 0;
          return(SERVER_EXIT);
          }
        /*** other header identifiers here 0x93 0x94... ***/
        n += 2;  // next item
        }
      else if((hi & 0xC0) == 0xC0)
        {  // 4-byte value
        if(hi == 0xC3)
          {  // Count       
          file_length = (data[n+1] << 24) + (data[n+2] << 16) + (data[n+3] << 8) + data[n+4];
          printf("File length = %d\n",file_length);
          }
        /**** other header identifiers here 0xC4 0xCB... ****/
        n += 5;  // next item
        }
      else
        {  // 2 length bytes 
        ilen = (data[n+1] << 8) + data[n+2];  // item length 
        j = n+3;    // start data  
        if(ilen == 0)
          n = length;  // error exit loop
        if(hi == 0x01)
          {  // unicode file name
          ++j; // skip unicode 0
          k = 0;
          while(j < n+ilen && j < len)
            {
            filename[k] = data[j];
            ++k;
            j += 2;
            }
          filename[k] = 0;
          printf("File name = %s\n",filename);
          stream = fopen(filename,"wb");
          if(stream == NULL)
            printf("File open error\n"); 
          }
        else if(hi == 0x48 || hi == 0x49)
          { // data chunk
          datan = j;           // index 
          datalen = ilen-3;    // length
          }
        /*********         
        Other header identifiers here
        0x42 = Type
        0x47 = HTTP
        0x44 = Time
        ...
        *********/
        
        n += ilen;  // next item  
        }
      }     
    }     
  else if(data[0] == 0x81)
    {  // Disconnect
    printf("OBEX disconnect\n");
    connected_node = 0;
    if(stream != NULL)
      { 
      fclose(stream);
      stream = NULL;
      }
    write_node(node,success,3);  
    return(SERVER_EXIT);
    } 
  else
    printf("GOT opcode %02X - no action\n",data[0]);
 
  // Write data chunk to file
  if(datalen != 0 && stream != NULL)
    {   
    count += datalen;  
    for(n = 0 ; n < datalen ; ++n)
      fputc(data[n+datan],stream); 
    if(data[0] == 0x82)
      {  // last chunk - finished
      if(count != file_length)
        printf("Expected %d bytes. Got %d\n",file_length,count);
      fclose(stream);
      stream = NULL;
      printf("File saved\n");
      }     
    }
  
  // Send response Continue or Success (section 3.2.1 in OBEX15.pdf)
  if(data[0] == 0x02)  // Put - not last chunk 
    write_node(node,continue_reply,3);  
  else      
    write_node(node,success,3);  

  return(SERVER_CONTINUE);
  }
