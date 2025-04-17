#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btlib.h" 

int notify_callback(int node,int cticn,unsigned char *dat,int datlen);
void getstring(char *prompt,char *s,int len);
int inputint(char *ps);

int main()
  {
  int node,cticn;
  unsigned char uuid[2];
  char *buf;
  
  if(init_blue("devices.txt") == 0)
    return(0);
    
  printf("\nScanning for HM10\n");
  le_scan();
  printf("\nEnter [ ] to scroll device list\n");
  node = inputint("Enter node of HM10 (should report UUID=FFE0)");
  if(node < 0)
    return(0);
  if(connect_node(node,CHANNEL_LE,0) == 0)
    {
    printf("Connect failed\n");
    return(0);
    }
    // read services
  find_ctics(node);
    // find data characteristic UUID=FFE1
  uuid[0] = 0xFF;
  uuid[1] = 0xE1;
  cticn = find_ctic_index(node,UUID_2,uuid);
  if(cticn > 0)
    {
    printf("Found data characteristic index = %d\n",cticn);
    printf("Enabling notifications\n");
    notify_ctic(node,cticn,NOTIFY_ENABLE,notify_callback);
    printf("Sending Hello\n");
    buf = "Hello\n";
    write_ctic(node,cticn,buf,strlen(buf));
    printf("Waiting 30s for a reply from HM10\n");
    printf("Send an ASCII string reply from the HM10\n");
    read_notify(30000);
    printf("\n30s wait for reply timed out\n");
    }
  else
    printf("Data characteristic FFE1 not found\n");
   
  printf("Disconnecting HM10\n");
  disconnect_node(node);
  close_all();
  return(0);
  }  
  
int notify_callback(int node,int cticn,unsigned char *dat,int datlen)
  {
  int n;

  // print hex
  for(n = 0 ; n < datlen ; ++n)
    printf(" %02X",dat[n]);
  printf("\n");
  // print ASCII - dat may not be zero terminated 
  for(n = 0 ; n < datlen ; ++n)
    {
    if(dat[n] >= 32 || dat[n] == 10 || dat[n] == 13)
      printf("%c",dat[n]);
    else
      printf(".");
    }
  printf("\n");
  }  

void getstring(char *prompt,char *s,int len)
  {
  int n;
  
  do
    {
    printf("%s",prompt);
    fgets(s,len,stdin);
    }
  while(s[0] == 10);

  n = 0;
  while(s[n] != 10 && s[n] != 13 && s[n] != 0)
    ++n;
  s[n] = 0;
  }  
     
/***** INPUT INTEGER ********/     
     
int inputint(char *ps)
  {
  int n,flag;
  char s[128];
 
  do
    {
    printf("%s  (x=cancel)\n",ps);
  
    getstring("? ",s,128);
       
    flag = 0;
    for(n = 0 ; s[n] != 0 ; ++n)
      {
      if(s[n] < '0' || s[n] > '9')
        flag = 1;
      }
    if(flag == 0)
      n = atoi(s);
    else if(s[0] == 'x')
      {
      n = -1;
      flag = 0;
      }
    else if(s[0] == '[')
      scroll_back();
    else if(s[0] == ']')
      scroll_forward();
    else
      printf("Not a number\n");
    }
  while(flag != 0);
  return(n);
  } 
