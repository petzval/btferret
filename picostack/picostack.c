#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tusb.h>
#include <hardware/flash.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// VERSION 23

    // KEYSECTORS
    // FLASH memory size in sectors used for key/pair storage
    // A larger number reduces flash wear
    // A smaller number leaves more room for code
    // 16 = 64k
#define KEYSECTORS 16
    // KEYFREE
    // This number of sectors at the top of flash memory are left alone
#define KEYFREE 4
    // WRITEBASE
    // Position of storage
    // Leaves KEYFREE sectors at top of flash memory free for other use
    // Uses KEYSECTORS below that            
#define WRITEBASE PICO_FLASH_SIZE_BYTES - ((KEYSECTORS+KEYFREE)*FLASH_SECTOR_SIZE)
#define READBASE  XIP_BASE + WRITEBASE
#define NUMPAGE (KEYSECTORS*FLASH_SECTOR_SIZE)/FLASH_PAGE_SIZE

    // Enable "keyboard" input from a USB serial monitor
    // for inputpin() and readkey()
#define ENABLE_READKEY 1
#define ENABLE_INPUTPIN 2
int usbflags = 0;

void  set_usb_flags(int flags);
int readkeyfile(unsigned char **table,int *count);
void writekeyfile(unsigned char *table,int count);
unsigned char *newkeytable(int count);
void freekeytable(unsigned char **table);
void clear_pair_store(void);
int read_usb_endchar(unsigned char *buf,int len,unsigned char endchar,int toms);

char btferrets[32] = {"btferret key and pairing storage"};
unsigned char pagebuf[FLASH_PAGE_SIZE];

void mycode(void);
int sendpack(unsigned char* buf, int len);
int readpack(unsigned char* buf, int toms);
void printn(char* buf, int len);
int closehci(void);
void inputpin(char* prompt, char* sbuf);
int setkeymode(int setflag);
int readkey(void);
int getdatfile(char *s);
void serverexit(int flag);
void scroll_back(void);
void scroll_forward(void);
unsigned long long time_ms(void);
int readbytes(int *len,int wantlen,int *toms);

 /**** btstack replacement functions ****/ 
void cyw43_bluetooth_hci_process(void);
bool btstack_cyw43_init(async_context_t *context);
bool btstack_cyw43_deinit(async_context_t *context);


unsigned char xbuf[2048];
unsigned char dongdat[8192];
unsigned char packdongle[8192];
unsigned char *pack = packdongle + 5;
unsigned char outbuf[20];


int main(int argc,char *argv[])
  { 
  stdio_init_all();
  if(cyw43_arch_init())
    return(0);

  cyw43_init(&cyw43_state);
  cyw43_bluetooth_hci_init();

  // flash LED
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);  
  sleep_ms(200);
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, false);  

  mycode();
  return(0);
  }

void inputpin(char* prompt, char* sbuf)
  {
  /************************************** 
     REPLACE this with your own programming
     if PIN/Passkey input is needed.
     Return as ASCII string in sbuf
  ****************************************/    
  if((usbflags & ENABLE_INPUTPIN) != 0) 
    {  // input from USB serial monitor
    printf("%s ?\n",prompt);
    read_usb_endchar(sbuf,64,10,0);
    }
  else
    {        
    printf("PIN/Passkey requested - returning default 0000\n");
    strcpy(sbuf,"0000");  // default
    }
  return;
  }

int readkey()
  {
  int nread;
  unsigned char buf[8];

  if((usbflags & ENABLE_READKEY) != 0)
    {  // Input from USB serial monitor
    nread = read_usb_endchar(buf,8,10,1);
    if(nread == 0)
      return(0);
    return(buf[0]);
    }
  return(0);
  }

int read_usb_endchar(unsigned char *buf,int len,unsigned char endchar,int toms)
  {
  int nr,nread,ntogo,xtoms;

  if(len == 0)
    return(0);

  nread = 0;
  xtoms = toms;
  ntogo = len;
  do
    {
    nr = 0;
    
    if(tud_cdc_available())
      {
      buf[nread] = tud_cdc_read_char();
      if(buf[nread] == endchar)
        return(nread);
      nr = 1;
      } 
    if(nr > 0)
      {
      ntogo -= nr;
      if(nread < len-1)
        nread += nr;
      }
    if(ntogo > 0)
      {
      sleep_ms(1);
      if(toms != 0)
        --xtoms;
      }
    }
  while(toms == 0 || (ntogo > 0 && xtoms > 0));
  return(nread);
  }


int sendpack(unsigned char *dat,int len)
    {
    int n,tocount;
    unsigned char buf[1024];

    for(n = 0 ; n < len ; ++n)
      buf[n+3] = dat[n];
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;

    tocount = 0;
    do
      {
      n = cyw43_bluetooth_hci_write(buf,len + 3);                
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


int readbytes(int *len,int wantlen,int *toms)
    {
    int ret,loclen;
    uint32_t rlen;
        
    loclen = *len;
    if(loclen >= wantlen)
      return(1);

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
      --*toms;
      }
    while(*toms > 0);

    return(0);
    }   
  
  
int readpack(unsigned char *outbuf,int toms)
    {
    int n,sn,nx,ny,dn,dlen,nread,xnread,totlen,thislen,handle,errflag,xtoms;
    unsigned char *buf,b0,xb0;
    static int blen = 0;
    static int packlen = 0;
    
    buf = pack;
    xtoms = toms;

    for(n = packlen ; n < blen ; ++n)
      buf[n-packlen] = buf[n];
    blen -= packlen;  // >0 if store
    packlen = 0;  
    handle = 0;
    totlen = 0;
        
    do  // dunp orphan loop
      {
      if(readbytes(&blen,1,&xtoms) == 0)
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
     
      if(readbytes(&blen,nread,&xtoms) == 0)
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
       
    if(readbytes(&blen,packlen,&xtoms) == 0)
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
        if(readbytes(&blen,sn+1,&xtoms) != 0)
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
    
          if(xnread != 0 && readbytes(&blen,sn+xnread,&xtoms) != 0)
            {
            // packet size dn        
            if(xb0 == 1)
              dn = buf[sn+3] + 4; 
            else if(xb0 == 4)
              dn = buf[sn+2] + 3;
            else if(xb0 == 2)
              dn = buf[sn+3] + (buf[sn+4] << 8) + 5;
  
            if(readbytes(&blen,sn+dn,&xtoms) != 0)
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
          for(n = 0 ; n < packlen ; ++n)
            outbuf[n] = buf[n];          
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
        
    for(n = 0 ; n < packlen ; ++n)
      outbuf[n] = buf[n];
    return(packlen);
    }

void printn(char* dat, int datlen)
    {
    unsigned char sav;

    sav = dat[datlen];
    dat[datlen] = 0;
    printf(dat);
    dat[datlen] = sav;
    return;
    }

int closehci()
  {
  return(1);
  }

unsigned long long time_ms()
  {
  unsigned long long tms;
  tms = time_us_64() / 1000;
  return(tms);
  }
  
int setkeymode(int setflag)
  {
  return(0);
  }  
int getdatfile(char *s)
  {
  strcpy(s,"FLASH");
  return(0);
  }
void serverexit(int flag)
  {
  return;
  }
void scroll_back()
  {
  return;
  }
void scroll_forward()
  {
  return;
  }
void set_usb_flags(int flags)
  {
  usbflags = flags;
  }
int readkeyfile(unsigned char **table,int *count)
  {
  uint32_t ints;
  int n,k,flag,pagen,lastpagen,flen,retval,alloclen;
  unsigned char *p,len,lastlen,*atable;

  *count = 0;
  *table = NULL;

  p = (unsigned char*)READBASE;
  flag = 0;
  for(n = 0 ; n < 32 && flag == 0; ++n)
    {
    if(*(p+n) != btferrets[n])
      flag = 1;
    }

  if(flag != 0)
    clear_pair_store();

  // pagen=1 1st file
  lastlen = *(p+FLASH_PAGE_SIZE);
  if(lastlen == 0xFF)
    {  // no data in 1st file
    *count = 0;
    return(0);
    }
  // find last non-FF
  pagen = 1;
  do
    {
    len = *(p+(FLASH_PAGE_SIZE*pagen));  // 22 byte entries
    if(len != 0xFF)
      {  // skip to next file
      lastpagen = pagen;
      lastlen = len;
      flen = 22*len + 1;
      pagen += flen / FLASH_PAGE_SIZE;
      if((flen % FLASH_PAGE_SIZE) != 0)
        ++pagen;  
      }
    }
  while(len != 0xFF && pagen < NUMPAGE);

  retval = 0;  
  atable = newkeytable(lastlen);  // with atable-1 for count write
  if(atable != NULL)
    {
    k = len*22;   // read k bytes      
    p += (lastpagen*FLASH_PAGE_SIZE)+1;   // skip len byte
    for(n = 0 ; n < k ; ++n)  // skip len byte
      atable[n] = *(p+n);
    *count = lastlen;
    *table = atable;
    retval = 1;
    }

  return(retval);
  }  

void writekeyfile(unsigned char *table,int count)
  {
  uint32_t ints;
  int n,k,flag,pagen,flen,datlen,writen,needpage;
  unsigned char *p,len,*atable;

  if(table == NULL || count == 0)
    return;
   
  p = (unsigned char*)READBASE;
  flag = 0;
  for(n = 0 ; n < 32 && flag == 0; ++n)
    {
    if(*(p+n) != btferrets[n])
      flag = 1;
    }

  datlen = count*22 + 1;
  needpage = datlen / FLASH_PAGE_SIZE;
  if((datlen % FLASH_PAGE_SIZE) != 0)
      ++needpage;

  if(flag == 0)
    {  
    // find first FF with room for needpage
    pagen = 1;
    do
      {
      len = *(p+(FLASH_PAGE_SIZE*pagen));  // 22 byte entries
      if(len != 0xFF)
        {  // skip to next file
        flen = 22*len + 1;
        pagen += flen / FLASH_PAGE_SIZE;
        if((flen % FLASH_PAGE_SIZE) != 0)
          ++pagen;  
        }
      }
    while(len != 0xFF && pagen+needpage <= NUMPAGE);
    if(pagen+needpage > NUMPAGE)
      flag = 1;   // full
    }

  if(flag != 0)
    {
    // not init or full
    clear_pair_store();
    pagen = 1;
    }
  
  atable = table-1;  // -1 was allocated by newkeytable
  atable[0] = (unsigned char)count;
  // write n*page size

  // atable was allocated n*page size include -1 count
  
  writen = needpage*FLASH_PAGE_SIZE;
  for(n = datlen ; n < writen ; ++n)
    atable[n] = 0xFF;
  
  ints = save_and_disable_interrupts();
  flash_range_program(WRITEBASE+(pagen*FLASH_PAGE_SIZE),atable,writen);
  restore_interrupts(ints);
  }

void clear_pair_store()
  {
  int n;
  uint32_t ints;

  for(n = 0 ; n < 32 ; ++n)
    pagebuf[n] = btferrets[n];
  for(n = 32 ; n < FLASH_PAGE_SIZE ; ++n)
    pagebuf[n] = 0xFF;

  ints = save_and_disable_interrupts();
  flash_range_erase(WRITEBASE,KEYSECTORS*FLASH_SECTOR_SIZE);
  flash_range_program(WRITEBASE,pagebuf,FLASH_PAGE_SIZE);
  restore_interrupts(ints);
  }

void freekeytable(unsigned char **table)
  {
  if(*table != NULL)
    free(*table-1);
  *table = NULL;
  }

unsigned char *newkeytable(int count)
  {
  int alloclen,n;
  unsigned char *table;

  alloclen = count*22 + 1;
  n = alloclen / FLASH_PAGE_SIZE;
  if((alloclen % FLASH_PAGE_SIZE) != 0)
    ++n;
  
  if(n >= NUMPAGE)
    {
    printf("KEYSECTORS too small\n");
    return(NULL);
    }

  table = (unsigned char*)malloc(n*FLASH_PAGE_SIZE);
  if(table == NULL)
    return(NULL);
  return(table+1);
  }


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