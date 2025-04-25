/************** BTFDONGLE ********

Version 21.1

For Pi Zero 2W or Pi4 set up as a serial USB device
and connected to a PC so it looks like a COM port.

COMPILE
  gcc btfdongle.c -o btfdongle
  
Must be run with sudo or from root

Communicates with Windows code running
on the PC:

https://github.com/petzval/btferret/windows

***********************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>

#define VERSION 21

#define BTPROTO_HCI 1
//#define HCIDEVUP _IOW('H',201,int)
#define HCIDEVUP 0x400448C9
//#define HCIDEVDOWN _IOW('H',202,int)
#define HCIDEVDOWN 0x400448CA
//#define HCISETSCAN _IOW('H',221,int)
#define HCISETSCAN 0x400448DD

int sendreply(int opcode,int id,int *ndat,int ndatlen,unsigned char *dat,int datlen,int datflag);
int readcmd(void);
int readn(unsigned char *buf,int len);
int writen(unsigned char *buf,int len);
int cmdhandle(int opcode,int id,unsigned char *dat,int datlen);
void getints(unsigned char *dat,int *ndat,int ndatlen);
int init_bluetooth(void);
int bluezdown(void);
int hcisock(void);
int sendpack(unsigned char *dat,int len);
int readpack(unsigned char *buf,int toms);
int readbytes(unsigned char *dat,int *len,int wantlen,int toms);
unsigned long long timems(void);
int statusok(unsigned char *cmd);
int setupcrypt(int flag);
int aes_cmac(unsigned char *key,unsigned char *msg,int msglen,unsigned char *res);
int calce(unsigned char *key,unsigned char *in,unsigned char *out);


unsigned char btreset[10] = {0x01,0x03,0x0C,0};
unsigned char lemask[20] = { 0x01,0x01,0x20,0x08,0xFF,0x05,0,0,0,0,0,0 };   // BF 05 for no data len event
unsigned char scanip[10] = {1,0x1A,0x0C,1,3};  // len 5   I/P scans [8] 0=off  3=I/P scans
unsigned char eventmask[20] =  { 1,1,0x0C,8,0xFF,0xFF,0xFB,0xFF,0x07,0xF8,0xBF,0x3D };  // default  FF FF DF = disable PIN request
unsigned char setcto[16] =  { 0x01,0x16,0x0C,0x02,0xA0,0x3F };
unsigned char setpto[16] =  { 0x01,0x18,0x0C,0x02,0x00,0x40 };

struct globdata
  {
  int hci;
  int btflag;
  int stfail;
  unsigned char *dongdat;
  unsigned char *packdongle;
  unsigned char *pack;
  int donglefd;
  int cryptfd;
  int cryptfd1;
  unsigned long long lastsend;
  };
struct globdata gpar;
   
  
int main()
  {
  int flag;
  struct termios tty;

  gpar.hci = -1;
  gpar.btflag = 3;
  gpar.stfail = 0;
  gpar.lastsend = 0;
  gpar.dongdat = malloc(8192);
  gpar.packdongle = malloc(8200);
  gpar.cryptfd = 0;
  gpar.cryptfd1 = 0;
  
  if(gpar.dongdat == NULL || gpar.packdongle == NULL)
    {
    printf("Alloc errror\n");
    return(0);
    }
  gpar.pack = gpar.packdongle + 5;
  gpar.donglefd = open("/dev/ttyGS0",O_RDWR | O_NONBLOCK);
  if(gpar.donglefd < 0)
    {
    printf("COM open fail\n");
    return(0);
    }
  else
    printf("COM open\n");
  
  tcflush(gpar.donglefd,TCIOFLUSH);
  flag = 0;
  if(tcgetattr(gpar.donglefd,&tty) == 0)
    {   
    cfmakeraw(&tty);    
    cfsetispeed(&tty,B115200);
    cfsetospeed(&tty,B115200);

    if(tcsetattr(gpar.donglefd,TCSANOW,&tty) == 0)
      flag = 1;
    }
  
  if(flag == 0)
    printf("Set COM failed\n");
  else
    gpar.btflag = init_bluetooth();
     
  readcmd();
  close(gpar.donglefd);
  return(0);
  }
  

int readcmd()
  {
  int getout,id,opcode,dlen,nread,ib0;
  unsigned char head[8];
      
  getout = 0;
  do
    {
    nread = read(gpar.donglefd,head,1);
    
    if(nread == 1)
      {
      ib0 = head[0];
      if(ib0 == 0xF7 || ib0 == 0xF8)
        {
        readn(head+1,4);
        opcode = head[1];
        id = head[2];
        dlen = (head[4] << 8) + head[3];
        
        // ib0 == 0xF8 Echo
              
        readn(gpar.dongdat,dlen);  
                  
        if(ib0 == 0xF7)
          getout = cmdhandle(opcode,id,gpar.dongdat,dlen);
        }
      // else not F7/F8
      }
    }
  while(getout == 0);   
  
  sleep(1);
      
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
    nr = read(gpar.donglefd,buf+nread,ntogo);
    if(nr > 0)
      {
      ntogo -= nr;
      nread += nr;
      }
    }
  while(ntogo > 0);
  return(nread);
  }



int cmdhandle(int opcode,int id,unsigned char *dat,int datlen)
  {
  int rflag,dplen,retval;
  int ndat[8],ndatlen;
  unsigned char *dp;
  unsigned char crypt[16];
   
   
  // printf("Handle op %d id %d len %d\n",opcode,id,datlen);
  ndatlen = 0;
  rflag = 0;  // reply one integer = retval
  retval = 0;
  switch(opcode)
    {
    case 1:   // ping
      printf("Ping\n");
      ndat[0] = VERSION;
      ndat[1] = gpar.btflag;
      ndat[2] = gpar.stfail;
      ndatlen = 3;
      dp = NULL;
      dplen = 0;
      rflag = 3;
      break;
      
    case 2:    
      printf("Halt dongle - power off\n");
      system("halt");
      break;  
    case 3:
      printf("Exit btfdongle code\n");
      return(1);  // exit readcmd in main()
      break;
 
    case 5:  // send packet dat = len[4],packet[len]
      getints(dat,ndat,1);
      retval = sendpack(dat+4,ndat[0]);  // return retval[4]
      break;
    case 6:  // read packet dat = timeout[4]
      getints(dat,ndat,1);  // timeout
      retval = readpack(gpar.pack,ndat[0]);  // +5 for header
      if(retval > 0)
        {         
        dp = gpar.packdongle;  
        dplen = retval;  // packet len
        rflag = 2;  // dplen packet data at gpar.pack = gpar.packdongle + 5
        }
      else
        rflag = 1;  
      // else return retval[4]
      break;
    case 7:   // calce dat = key[16],res[16]
      ndat[0] = calce(dat,dat+16,crypt);
      ndatlen = 1;
      dp = crypt;
      dplen = 16;
      rflag = 3;  // return ret[4],out[16]
      break;
    case 8:  // aes_cmac  dat = msglen[4],key[16],msg[msglen]
      getints(dat,ndat,1);
      ndat[0] = aes_cmac(dat+4,dat+20,ndat[0],crypt);
      ndatlen = 1;
      dp = crypt;
      dplen = 16;
      rflag = 3;  // return ret[4],out[16]
      break;        
    default:  
      printf("Unknown opcode %d\n",opcode);
    }

  if(rflag == 0)    
    sendreply(opcode,id,&retval,1,NULL,0,0);  // single int = retval
  else if(rflag == 1)    
    sendreply(opcode,id,NULL,0,NULL,0,0);  // void
  else if(rflag == 2)
    sendreply(opcode,id,NULL,0,dp,dplen,1);  // use dp for header and ints 
  else if(rflag == 3)
    sendreply(opcode,id,ndat,ndatlen,dp,dplen,0);  // two sends header + dp if dplen large
  return(0);
  }


void getints(unsigned char *dat,int *ndat,int ndatlen)
  {
  int n;
  unsigned char *cp;
  long *lp;
  
  cp = dat;
  for(n = 0 ; n < ndatlen ; ++n)
    {
    lp = (long*)cp;
    ndat[n] = (int)(*lp);
    cp += 4;
    }
  }

int sendreply(int opcode,int id,int *ndat,int ndatlen,unsigned char *dat,int datlen,int datflag)
  {
  int n,dn,flag,nsend;
  unsigned char *cmd,loccmd[40];
  long *sp;
  unsigned char *cp;
  
    
  if(datflag == 0)
    cmd = loccmd;
  else
    cmd = dat;   // must be room left for header and ints (9 for 1 int)
    
  dn = 0;
  if(ndatlen > 0 && ndat != NULL)
    {
    cp = cmd+5;
    for(n = 0 ; n < ndatlen && n < 5 ; ++n)
      {
      sp = (long*)cp;
      *sp = (long)(ndat[n]);
      cp += 4;
      dn += 4;
      }
    }
 
  flag = 0;
  if(datflag != 0)
    {
    dn += datlen;
    nsend = dn+5;  // header and data from dat
    }
  else if((ndatlen*4)+datlen < 30 || dat == NULL)
    {
    if(datlen > 0 && dat != NULL)
      {
      for(n = 0 ; n < datlen ; ++n)
        {
        cmd[dn+5] = dat[n];
        ++dn;
        }
      }
    nsend = dn+5;  // header and data from loccmd
    } 
  else
    {
    nsend = dn+5;  // header + ints only from loccmd
    dn += datlen;  // data sent 2nd from dat
    flag = 1;
    }
  
  cmd[0] = 0xF8;
  cmd[1] = opcode;
  cmd[2] = id;
  cmd[3] = dn & 0xFF;
  cmd[4] = (dn >> 8) & 0xFF;
 
  writen(cmd,nsend);

  if(flag != 0)
    writen(dat,datlen);

  return(1);
  }

int writen(unsigned char *dat,int datlen)
  {
  int wn,ntogo,nwrit;
 
  if(datlen == 0)
    return(0);
     
  ntogo = datlen;
  nwrit = 0;
  do
    {
    wn = write(gpar.donglefd,dat+nwrit,ntogo);
    if(wn > 0)
      {
      ntogo -= wn;
      nwrit += wn;
      }
    }
  while(ntogo > 0);
  return(nwrit);
  }
  

int init_bluetooth()
  {
  int n,ret;
  
  if(gpar.hci > 0)
    return(0);  // already done
    
  bluezdown(); 
  if(hcisock() == 0)
    return(0);

  printf("Trying to unblock bluetooth with rfkill\n");
  usleep(500000);
  n = system("rfkill unblock bluetooth");
  if(n == 0)
    n = 0;
  usleep(500000);
  bluezdown();
  ret = hcisock();
  if(ret == 0)
    return(0);
    
  printf("No root permission or Bluetooth hci0 is off or crashed\n"); 
  printf("Must run with root permission via sudo as follows:\n"); 
  printf("  sudo ./btfdongle\n");
  return(ret);
  }


int hcisock()
  {
  int dd;
  //struct sockaddr_hci sa;
  unsigned char sa[6];
   
  if(gpar.hci > 0)
    return(0);
     
  printf("Open HCI user socket\n");

         // AF_BLUETOOTH=31
  printf("Open BTPROTO socket\n");
  dd = socket(31, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, BTPROTO_HCI);

  if(dd < 0)
    {
    printf("Socket open error\n");
    return(1);
    }
 
  printf("Bind to Bluetooth hci0 user channel\n");

  sa[0] = 31;  // hci_family = AF_BLUETOOTH
  sa[1] = 0;
  sa[2] = 0;   // hci_dev = hci0/1/2...
  sa[3] = 0;   // dev hi 
  sa[4] = 1;   // hci_channel = HCI_CHANNEL_USER
  sa[5] = 0;
   
  if(bind(dd,(struct sockaddr *)sa,sizeof(sa)) < 0)
    {
    printf("Bind failed\n");
    close(dd);
    return(2);
    }

  gpar.hci = dd;    
  gpar.stfail = 0;
    
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
    
  printf("Bluetooth OK\n");
  return(0);
  }

int statusok(unsigned char *cmd)
  {
  int ret;
  unsigned char *buf;
  
  buf = gpar.pack;    
  do
    {
    ret = readpack(gpar.pack,100);
    if(ret > 3 && buf[4] == cmd[1] && buf[5] == cmd[2])
      {
      if(buf[6] == 0)
        return(1);  // status OK
      ret = 0;
      }
    }
  while(ret > 0);
  printf("FAILED OGF %02X OCF %02X\n",cmd[2] >> 2,cmd[1]);
  ++gpar.stfail;
  return(0);
  }

int bluezdown()
  {
  int dd,retval;
  
  printf("Bluez down\n");

  retval = 0;
  
  dd = socket(31, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, BTPROTO_HCI);

  if(dd >= 0)
    {
    if(ioctl(dd,HCIDEVDOWN,0) >= 0)  // hci0
      retval = 1;
    close(dd); 
    }            
 
  usleep(500000);  
  return(retval);
  }






int sendpack(unsigned char *buf,int len)
  {
  int ntogo,nwrit;
  unsigned long long timstart;
  unsigned char *cmd;
  
  ntogo = len;  
  timstart = timems();  
  cmd = buf;
   
  do
    {
    nwrit = write(gpar.hci,cmd,ntogo);

    if(nwrit > 0)
      {
      ntogo -= nwrit;
      cmd += nwrit;
      }
    else
      usleep(1000);
        
    if(ntogo != 0 && (int)(timems() - timstart) > 5000)   // 5 sec timeout
      {
      printf("Send CMD timeout - may need to reboot\n");
      return(0);
      }
    }
  while(ntogo != 0);
  gpar.lastsend = timems();
  return(1);    
  }


int readbytes(unsigned char *dat,int *len,int wantlen,int toms)
  {
  int rlen,loclen;
  struct pollfd pfd[1];
  unsigned long long tim0;
  
     
  loclen = *len;
  if(loclen >= wantlen)
    return(1);

  tim0 = timems();
  do
    {  
    pfd[0].fd = gpar.hci;
    pfd[0].events = POLLIN;
    if(poll(pfd,1,1) != 0 && (pfd[0].events & POLLIN) != 0)
      {
      rlen = read(gpar.hci,dat+loclen,8190-loclen);
      if(rlen > 0)
        {
        loclen += rlen;
        *len = loclen;
        }
      }
    if(loclen >= wantlen)
      return(1);
    }
  while((int)(timems() - tim0) < toms);
  
  return(0);
  }   


int readpack(unsigned char *buf,int toms)
  {
  int n,sn,nx,ny,dn,dlen,nread,xnread,totlen,thislen,handle,errflag;
  unsigned char b0,xb0;
  static int blen = 0;
  static int packlen = 0;
  
  for(n = packlen ; n < blen ; ++n)
    buf[n-packlen] = buf[n];
  blen -= packlen;  // >0 if store
  packlen = 0;  
  handle = 0;
  totlen = 0;
      
  do  // dunp orphan loop
    {
    if(readbytes(buf,&blen,1,toms) == 0)
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
      printf("Invalid packet\n");
      blen = 0;
      packlen = 0;
      return(0);  // fatal
      }
   
    if(readbytes(buf,&blen,nread,1000) == 0)
      {
      printf("Timed out waiting\n");
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
        printf("Orphan extra data\n");
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
     
  if(readbytes(buf,&blen,packlen,1000) == 0)
    {
    printf("Timed out waiting\n");
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
      if(readbytes(buf,&blen,sn+1,1000) != 0)
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
  
        if(xnread != 0 && readbytes(buf,&blen,sn+xnread,1000) != 0)
          {
          // packet size dn        
          if(xb0 == 1)
            dn = buf[sn+3] + 4; 
          else if(xb0 == 4)
            dn = buf[sn+2] + 3;
          else if(xb0 == 2)
            dn = buf[sn+3] + (buf[sn+4] << 8) + 5;

          if(readbytes(buf,&blen,sn+dn,1000) != 0)
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
        printf("Missing extra data\n");
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

unsigned long long timems()
  {
  unsigned long long dt;
  struct timespec ts;
  static time_t tim0;
  static int xflag = 0;  // force reset on first call
 
  clock_gettime(CLOCK_MONOTONIC_RAW,&ts);

  if(xflag == 0)
    {
    srand((unsigned int)ts.tv_nsec);
    tim0 = ts.tv_sec;
    xflag = 1;
    return(0);  // zero time on first call
    }   
    
  dt = ts.tv_sec - tim0;   
    
  return((dt*(unsigned long long)1000) + (unsigned long long)(ts.tv_nsec/1e6));   
  }



int calce(unsigned char *key,unsigned char *in,unsigned char *out)
  {
  struct cmsghdr *cmsg;
  struct msghdr msg;
  struct iovec iov; 
  unsigned long alg_op;
  char cbuf[CMSG_SPACE(sizeof(alg_op))];
  unsigned char tmpkey[16],tmpin[16],tmpout[16];
  int n,fd,len;

  if(gpar.cryptfd == 0)
    {
    if(setupcrypt(0) == 0)
      return(0);
    }
  
  for(n = 0 ; n < 16 ; ++n)
    tmpkey[15-n] = key[n];

  if(setsockopt(gpar.cryptfd,279,1,tmpkey,16) < 0)
    {   // SOL_ALG,ALG_SET_KEY
    printf("Calce sock opt fail\n");
    return(0);
    }
  fd = accept(gpar.cryptfd,NULL,0);
  if(fd < 0)
    {
    printf("Calce accept error\n");
    return(0);
    }

  for(n = 0 ; n < 16 ; ++n)
    tmpin[15-n] = in[n];
 
  alg_op = 1; // ALG_OP_ENCRYT
  memset(cbuf,0,sizeof(cbuf));
  memset(&msg,0,sizeof(msg));
  msg.msg_control = cbuf;
  msg.msg_controllen = sizeof(cbuf);
  
  cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = 279;  // SOL_ALG
  cmsg->cmsg_type = 3;     // ALG_SET_OP
  cmsg->cmsg_len = CMSG_LEN(sizeof(alg_op));
  memcpy(CMSG_DATA(cmsg), &alg_op, sizeof(alg_op));
  iov.iov_base = (void *)tmpin;
  iov.iov_len = 16;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  len = sendmsg(fd, &msg, 0);
  if (len < 0)
    {
    printf("Calce send fail\n");
    close(fd);
    return(0);
    }
  len = read(fd, tmpout, 16);
  if (len < 0)
    {
    printf("Calce read fail\n");
    close(fd);
    return(0);
    }

  for(n = 0 ; n < 16 ; ++n)
    out[15-n] = tmpout[n];

  close(fd);
  return(1);    
  }  


int aes_cmac(unsigned char *key,unsigned char *msg,int msglen,unsigned char *res)
  {
  int n,len,fd;
  unsigned char tmpkey[16],msg_msb[80],out[80];

  if(gpar.cryptfd1 == 0)
    {
    if(setupcrypt(1) == 0)
      return(0);
    }
    
  if(msglen > 80)
    return(0);

  for(n = 0 ; n < 16 ; ++n)
    tmpkey[15-n] = key[n];  

  if(setsockopt(gpar.cryptfd1,279,1,tmpkey,16) < 0)
    {   // SOL_ALG,ALG_SET_KEY
    printf("AES sock opt fail\n");
    return(0);
    }
  fd = accept(gpar.cryptfd1,NULL,0);
  if(fd < 0)
    {
    printf("AES accept error\n");
    return(0);
    }
  
  for(n = 0 ; n < msglen ; ++n)
    msg_msb[msglen-1-n] = msg[n];

  len = send(fd,msg_msb,msglen,0);
  if (len < 0)
    {
    close(fd);
    return(0);
    }

  len = read(fd, out, 16);
  if (len < 0)
    {
    close(fd);
    return(0);
    }

  for(n = 0 ; n < 16 ; ++n)
    res[15-n] = out[n];  
  close(fd);
  return(1);
  }


int setupcrypt(int flag)
  {
  //struct sockaddr_alg sa;
  int n,fd;
  unsigned char sa[88];
  
  fd = socket(38,SOCK_SEQPACKET | SOCK_CLOEXEC,0);
  if(fd < 0)
    {
    printf("Crypto sock error\n");
    return(0);
    }
    
  for(n = 0 ; n < 88 ; ++n)
    sa[n] = 0;  
  
  if(flag == 0)
    {
    strcpy((char*)(sa+2),"skcipher");
    strcpy((char*)(sa+24),"ecb(aes)");
    }
  else
    {
    strcpy((char*)(sa+2),"hash");
    strcpy((char*)(sa+24),"cmac(aes)");
    }
    
  if(bind(fd,(struct sockaddr*)sa,88) < 0)
    {
    printf("Crypto bind error\n");
    close(fd);
    return(0);
    }  
   
  if(flag == 0)
    gpar.cryptfd = fd;
  else
    gpar.cryptfd1 = fd;
  return(1);  
  }
