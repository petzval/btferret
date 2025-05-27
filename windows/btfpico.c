#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tusb.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// version 23

#define VERSION 23

// btstack is not used, but if this causes compile to fail
// uncomment the following define USE_BTSTACK,
// and uncomment the four related instructions in CMakeLists.txt
// #define USE_BTSTACK


int readcmd(void);
int readn(unsigned char *buf,int len);
int writen(unsigned char *buf,int len);
int sendpack(unsigned char *dat,int len);
int readbytes(int *len,int wantlen,int toms);
int readpack(int toms);
int init_bluetooth(void);
int statusok(unsigned char *cmd);
#ifndef USE_BTSTACK
  /**** btstack replacement functions ****/ 
void cyw43_bluetooth_hci_process(void);
bool btstack_cyw43_init(async_context_t *context);
bool btstack_cyw43_deinit(async_context_t *context);
#endif

unsigned char btreset[10] = {0,0,0,0x01,0x03,0x0C,0};
unsigned char lemask[20] = { 0,0,0,0x01,0x01,0x20,0x08,0xFF,0x05,0,0,0,0,0,0 };
unsigned char scanip[10] = { 0,0,0,1,0x1A,0x0C,1,3};
unsigned char eventmask[20] =  { 0,0,0,1,1,0x0C,8,0xFF,0xFF,0xFB,0xFF,0x07,0xF8,0xBF,0x3D };
unsigned char setcto[16] =  {0,0,0,0x01,0x16,0x0C,0x02,0xA0,0x3F };
unsigned char setpto[16] =  {0,0,0,0x01,0x18,0x0C,0x02,0x00,0x40 };

unsigned char xbuf[2048];
unsigned char dongdat[8192];
unsigned char packdongle[8192];
unsigned char *pack;
unsigned char outbuf[20];
int btflag;
int stfail;

int main()
  {
  int n,tocount;
  int id,opcode,dlen,nread,ib0;
  unsigned int ndat;
  unsigned char head[8];  
  
  stdio_init_all();
  if(cyw43_arch_init())
    return(0);

  cyw43_init(&cyw43_state);

  btflag = 0;  // OK
  stfail = 0;
  pack = packdongle + 5;

  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);  
  sleep_ms(200);
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);
   
  cyw43_bluetooth_hci_init();

  init_bluetooth();

  while(1)
    {
    nread = 0;
    if(tud_cdc_available())
      { 
      head[0] = tud_cdc_read_char();   
      nread = 1;
      }     
    
    if(nread > 0)
      {
      ib0 = head[0];
      if(ib0 == 0xF7 || ib0 == 0xF8)
        {
        readn(head+1,4);
        opcode = head[1];
        id = head[2];
        dlen = (head[4] << 8) + head[3];
        
        if(dlen > 0)
          readn(dongdat+3,dlen);  

        if(ib0 == 0xF7)
          {        
          if(opcode == 5 || opcode == 1)
            {
            for(n = 5 ; n < 20 ; ++n)
              outbuf[n] = 0; 
            outbuf[0] = 0xF8;
            outbuf[1] = opcode;
            outbuf[2] = id;
            }


          if(opcode == 5)
            {
            dongdat[0] = 0;
            dongdat[1] = 0;
            dongdat[2] = 0;
            tocount = 0;
            do
              {
              n = cyw43_bluetooth_hci_write(dongdat,dlen + 3);
              if(n != 0)
                {
                sleep_ms(1);
                ++tocount;
                }
              }       
            while(n != 0 && tocount < 5000);   
            if(n == 0)
              n = 1;  // OK
            else 
              n = 0;    
            outbuf[3] = 4;
            outbuf[5] = (unsigned char)n;
            writen(outbuf,9);            
            }
          else if(opcode == 6)
            {
            ndat = dongdat[3] + (dongdat[4] << 8) + (dongdat[5] << 16) + (dongdat[6] << 24);
            nread = readpack(ndat);
            packdongle[0] = 0xF8;
            packdongle[1] = opcode;
            packdongle[2] = id;
            packdongle[3] = (unsigned char)(nread & 0xFF);
            packdongle[4] = (unsigned char)((nread >> 8) & 0xFF);
            writen(packdongle,nread+5);
            }
          else if(opcode == 1)
            {                
            outbuf[3] = 12;
            outbuf[5] = (unsigned char)(VERSION & 0xFF);
            outbuf[6] = (unsigned char)((VERSION >> 8) & 0xFF);
            outbuf[7] = 1;  // type = pico2
            outbuf[9] = btflag;
            outbuf[13] = stfail;
            writen(outbuf,17);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);  
            sleep_ms(200);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);           
            }
          }
        }
      // else not F7/F8
      }
    }
    
  return(0);
  }


int readn(unsigned char *buf,int len)
  {
  int nr,nread,ntogo;

  if(len == 0)
    return(0);

  nread = 0;
  ntogo = len;
  do
    {
    nr = 0;
    
    if(tud_cdc_available())
      {
      buf[nread] = tud_cdc_read_char();
      nr = 1;
      } 
    if(nr > 0)
      {
      ntogo -= nr;
      nread += nr;
      }
    }
  while(ntogo > 0);
  return(nread);
  }

int writen(unsigned char *dat,int datlen)
  {
  int n,wn,ntogo,nwrit,avail,nsend;

  if(datlen == 0)
    return(0);
    
  ntogo = datlen;
  nwrit = 0;
  do
    {
    wn = 0;
    avail = tud_cdc_write_available();
    if(ntogo <= avail)
      nsend = ntogo;
    else 
      nsend = avail;
    if(nsend > 0)
        wn = (int)tud_cdc_write(dat+nwrit,nsend);
    
    tud_task();
    tud_cdc_write_flush();
       
    if(wn > 0)
      {
      ntogo -= wn;
      nwrit += wn;
      }
    }
  while(ntogo > 0);
  return(nwrit);
  }

  int sendpack(unsigned char *dat,int len)
    {
    int n,tocount;

    tocount = 0;
    do
      {
      n = cyw43_bluetooth_hci_write(dat,len + 3);                
      if(n != 0)
        {
        sleep_ms(1);
        ++tocount;
        }
      }       
    while(n != 0 && tocount < 5000);
    if(n == 0)
      return(1);
    return(0);   
    }


  int readbytes(int *len,int wantlen,int toms)
    {
    int ret,loclen,tocount;
    uint32_t rlen;
        
    loclen = *len;
    if(loclen >= wantlen)
      return(1);

    tocount = 0;
    do
      {
      ret = cyw43_bluetooth_hci_read(xbuf,2048,&rlen);
      if (ret != 0 || rlen <= 3)
        rlen = 0;
      else
        {
        rlen -= 3;
        memcpy(pack+loclen,&xbuf[3],rlen);    
        }
 
      if(rlen > 0)
        {
        loclen += rlen;
        *len = loclen;
        }
      if(loclen >= wantlen)
        return(1);       
      sleep_ms(1);
      ++tocount;
      }
    while(tocount < toms);

    return(0);
    }   
  
  
  int readpack(int toms)
    {
    int n,sn,nx,ny,dn,dlen,nread,xnread,totlen,thislen,handle,errflag;
    unsigned char *buf,b0,xb0;
    static int blen = 0;
    static int packlen = 0;
    
    buf = pack;

    for(n = packlen ; n < blen ; ++n)
      buf[n-packlen] = buf[n];
    blen -= packlen;  // >0 if store
    packlen = 0;  
    handle = 0;
    totlen = 0;
        
    do  // dunp orphan loop
      {
      if(readbytes(&blen,1,toms) == 0)
        {
        // normal time out no bytes from store or new read
        return(0);
        } 
     
      b0 = buf[0];
      if(b0 == 1)
        nread = 4;
      else if(b0 == 2)
        nread = 7;
      else if(b0 == 4)
        nread = 3;
      else
        {
        //  Invalid packet
        blen = 0;
        packlen = 0;
        return(0);  // fatal
        }
     
      if(readbytes(&blen,nread,toms) == 0)
        {
        //  Timed out waiting
        blen = 0;
        packlen = 0;
        return(0);  // fatal
        }
         
      if(b0 == 1)
        packlen = buf[3] + 4;
      else if(b0 == 4) 
        packlen = buf[2] + 3;
      else if(b0 == 2)
        {
        thislen = buf[3] + (buf[4] << 8);
        packlen = thislen + 5;
        thislen -= 4;
        totlen = buf[5] + (buf[6] << 8);
        handle = (buf[1] + (buf[2] << 8)) & 0xFFF;
        if((buf[2] & 0x30) == 0x10)
          {
          // Orphan extra data
          // remove packet
          for(n = packlen ; n < blen ; ++n)
            buf[n-packlen] = buf[n];
          blen -= packlen;
          packlen = 0; // loop for next  
          }
        else
          {  // temp change totlen to this packet only
          n = packlen - 9;  // mew totlen
          buf[5] = n & 0xFF;
          buf[6] = (n >> 8) & 0xFF;
          }       
        }         
      }
    while(packlen == 0);
       
    if(readbytes(&blen,packlen,toms) == 0)
      {
      // Timed out waiting
      blen = 0;
      packlen = 0;
      return(0);  // fatal
      }
       
    // Check extra data needed
    sn = packlen;
    while(b0 == 2 && packlen > 0 && totlen > thislen)
      {
      // find extra data handle with 10 flag
      // sn = start next packet        
      nx = -1;
      do
        {
        errflag = 1;
        // need 1+ sn
        if(readbytes(&blen,sn+1,toms) != 0)
          {  
          xb0 = buf[sn];
          if(xb0 == 1)
            xnread = 4;
          else if(xb0 == 2)
            xnread = 5;
          else if(xb0 == 4)
            xnread = 3;
          else
            xnread = 0;  // missing
    
          if(xnread != 0 && readbytes(&blen,sn+xnread,toms) != 0)
            {
            // packet size dn        
            if(xb0 == 1)
              dn = buf[sn+3] + 4; 
            else if(xb0 == 4)
              dn = buf[sn+2] + 3;
            else if(xb0 == 2)
              dn = buf[sn+3] + (buf[sn+4] << 8) + 5;
  
            if(readbytes(&blen,sn+dn,toms) != 0)
              {
              if(xb0 == 2 && ((buf[sn+1] + (buf[sn+2] << 8)) & 0xFFF) == handle && (buf[sn+2] & 0x30) == 0x10)
                {
                nx = sn;
                ny = nx + 5;
                dlen = dn - 5; // extra data size ny to sn
                }
              sn += dn;
              errflag = 0;
              }
            }
          }
        if(errflag != 0)
          {
          //  Missing extra data
          blen = sn;  // sn is previous value OK
          return(packlen);  // so data missing but legal data in packlen
          } 
        }
      while(nx < 0);
      
      // add nx data
      // shift up dlen
      for(n = blen-1 ; n >= packlen ; --n)
        buf[n+dlen] = buf[n];
      blen += dlen;
      sn += dlen;
      nx += dlen;
      ny += dlen;
      // copy extra data
      for(n = 0 ; n < dlen ; ++n)
        buf[packlen+n] = buf[ny+n];
      packlen += dlen;
      // shift remainder to nix extra
      dn = blen - sn;
      for(n = 0 ; n < dn ; ++n)
        buf[nx+n] = buf[sn+n];
      dn = sn - nx;
      blen -= dn;
      sn -= dn;
      // update sizes so legal     
      thislen += dlen;
      n = packlen - 5;  // new thislen
      buf[3] = n & 0xFF;
      buf[4] = (n >> 8) & 0xFF;
      n = packlen - 9;  // mew totlen
      buf[5] = n & 0xFF;
      buf[6] = (n >> 8) & 0xFF;       
      }  // emd extra data search
        
    return(packlen);
    }

int init_bluetooth()
  {
  sendpack(btreset,4);
  statusok(btreset);
    
  sendpack(eventmask,12);
  statusok(eventmask);
   
  sendpack(lemask,12);
  statusok(lemask);
   
  sendpack(scanip,5);
  statusok(scanip);
    
  sendpack(setcto,6);
  statusok(setcto);
    
  sendpack(setpto,6);
  statusok(setpto);
      
  return(0);
  }
  
int statusok(unsigned char *cmd)
    {
    int ret;
    unsigned char *buf;
    
    do
      {
      ret = readpack(250);
      // hci packet at cmd[3]
      if(ret > 3 && pack[4] == cmd[4] && pack[5] == cmd[5])
        {
        if(pack[6] == 0)
          return(1);  // status OK
        ret = 0;
        }
      }
    while(ret > 0);
     // printf("FAILED OGF %02X OCF %02X\n",cmd[2] >> 2,cmd[1]);
    ++stfail;
    return(0);
    }  

/******* Replacement functions 
These functions are needed for cyw43, and would be provided by btstack
if it was linked. So we must provide replacements.
******************************/     
#ifndef USE_BTSTACK
void cyw43_bluetooth_hci_process(void)
  {
  return;
  }
bool btstack_cyw43_init(async_context_t *context)
  {
  return(true);
  };
bool btstack_cyw43_deinit(async_context_t *context)
  {
  return(true);
  } 
#endif

 