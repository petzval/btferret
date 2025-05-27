/********* Version 23 *********/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btlib.h"

// #define BTFWINDOWS
#define PICOSTACK

#if (!defined(BTFWINDOWS) && !defined(PICOSTACK))                       
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#endif



#ifdef BTFPYTHON
  #include "btfpython.c"          
#endif

/********** BLUETOOTH defines ********/

#define BTPROTO_HCI 1
#define BTPROTO_RFCOMM 3
#define BTPROTO_L2CAP 0
#define SOL_HCI 0
#define HCI_FILTER 2
#define SCAN_PAGE 2
#define SCAN_INQUIRY 1
//#define HCIDEVUP _IOW('H',201,int)
#define HCIDEVUP 0x400448C9
//#define HCIDEVDOWN _IOW('H',202,int)
#define HCIDEVDOWN 0x400448CA
//#define HCISETSCAN _IOW('H',221,int)
#define HCISETSCAN 0x400448DD

/************** END BLUETOOTH DEFINES ********/


#define VERSION 23
   // max possible NUMDEVS = 1024 
#define NUMDEVS 1024
#define NAMELEN 34
#define TOPUPCREDIT 200
  // ATT_MTU data cannot be more than 244 
#define LEDATLEN 244
  // Target packet transmission time in microseconds
#define TRANSMITUS 2048 


struct psdata
  {
  int handle;
  int eog;
  int uuidtype;
  unsigned char uuid[16];
  };
  
struct psdata pserv[32]; 

struct cticdata 
  {
  int type;    // CTIC_UNUSED=allocated but unused CTIC_ACTIVE=used CTIC_END=terminate
  int cticn;   // index
  int size;    // number of bytes 0=unknown
  int origsize;  // local - from devices file
  int perm;    // permissions 0=unknown 02=read 04=write no ack 08=write ack
  int notify;   // 1=enable notifications
  int psnx;
  int lasthandle;  // in primary service
  char name[NAMELEN];  // name of characteristic - your choice
  int chandle;    // characteristic handle 0=unknown 
  int uuidtype;   // 0=unknown 2=2-byte 16=16-byte
  int iflag;      // 1=have read info from remote device
  int reportflag;  // HID Report count
  int reportid;    // Report ID (85 XX) in Report Map
  unsigned char value[LEDATLEN];
  unsigned char uuid[16];
  int (*callback)();
  struct cticdata *nextctic;  // next in chain
#ifdef BTFPYTHON
  PyObject *pycallback;
#endif  
  };

#define CTIC_UNUSED 0
#define CTIC_ACTIVE 1
#define CTIC_END    2
struct cticdata cticnull = {CTIC_END};  // terminate ctic chain 

struct devdata
  {
  int type;                   // 0=not used    BTYPE_  values   
  char name[NAMELEN];         // name of LE device - your choice
  int matchname;              // 1=use name match to find baddr 
  unsigned char baddr[6];     // board address hi byte first
  unsigned int rfchan;        // RFCOMMM serial channel
  int node;                   // of device
  int conflag;                // 0=disconnected see below for values
  int leaddtype;              // LE board address type 
                              // bit 0 clr=public  set=random
                              // bit 1 clr=on initblue() list  set=found by scan  
  int lecflag;                // 1 = BTYPE_ME LE client connect 
  unsigned int meshindex;     // mesh message index
  unsigned char scid[4];      // L2CAP 0040+ scid channelof local
  unsigned char dcid[4];      // L2CAP 0040+ dcid channel of remote
  unsigned char psm;          // 1 or 3
  unsigned int id;
  int method;                 // connection method BLUEZ/HCI
  int credits;                // serial data credits
  int dhandle[2];             // handle returned by connect  [0]=lo [1]=hi
  int linkflag;
  int setdatlen;
  int foundflag;
  int leinterval;
  unsigned char linkey[16];
  char pincode[16]; 
  unsigned char advert[64];
  unsigned char keyk;
  unsigned char pairk;    
  unsigned int cryptoflag; 
  unsigned char pres[7];
  unsigned char preq[7];
  unsigned char rrand[16];
  unsigned char irand[16];
  unsigned char confirm[16];
  unsigned char divrand[16];
  unsigned char pubkeyx[32];
  unsigned char pubkeyy[32];
  unsigned char dhkey[32];
  int pkround;
  int lepasskey;
  int lepairflags;
   
  struct cticdata *ctic;      // first ctic in chain
  };

 
  // devdata conflag values
#define CON_HCI 1
         // BT connected via HCI device handle dhandle[]
#define CON_PSM3 (1 << 1)
         // L2CAP channel psm3 open dcid[0]
#define CON_PSM1 (1 << 2)
         // L2CAP channel psm1 open dcid[2]
#define CON_RF (1 << 3)
         // RFCOMM channel open rfchan
#define CON_LE (1 << 4)
         // LE device connected via HCI handle dhandle[]
#define CON_MESH (1 << 5)
         // LE connected to mesh device         
#define CON_SERVER (1 << 6)
         // RFCOMM classic server
#define CON_CH0 (1 << 7)
        // CONTROL channel 0
#define CON_LX (1 << 8)
        // LE server
        
#define CON_PSM1X (1 << 29)  // disconnect requested
#define CON_PSM3X (1 << 30)

#define LE_SERV (1 << 16)
#define CL_SERV (1 << 17)
         
  // dev[0].dhandle[] index for server/sdp
#define SERIALFD 0
#define SERVERSOCK 1  
#define SDPFD 0
       
   // readserial flags

#define EXIT_CLEAR    4   
#define EXIT_COUNT    8
#define EXIT_CHAR     16
  
  // instack pop flags
#define INS_NOPOP 0
#define INS_POP   0xFF
#define INS_FREE  0xFE
#define INS_LOCK  0xFD
#define INS_IMMED 0x80

#define METHOD_HCI   0
  // bluez connect - disabled in cl_connect
#define METHOD_BLUEZ 1

   // services() flags

#define SRVC_SHORTLIST   1
#define SRVC_LONGLIST    2
#define SRVC_FINDUUID2   3
#define SRVC_FINDUUID16  4
#define SRVC_READCTICS   5
#define SRVC_FREECHANNEL 6
#define SRVC_UUID        7

   // mesh transmit state

#define MESH_OFF 0
#define MESH_W   1
#define MESH_R   2
#define MESH_RW  3
#define MESH_STATUS 4

   // read/write

#define TO_MESH      0x010000
#define FROM_MESH    0x020000
#define FROM_MESHCON 0x040000
#define FROM_CLCON   0x080000
#define FROM_ALLCON  (FROM_MESHCON | FROM_CLCON)

  // link key

// KEY_OFF 0
// PASSKEY_OFF 0
// AUTHENTICATION_OFF
// KEY_ON  1
// PASSKEY_LOCAL 2
// PASSKEY_REMOTE 4
// PASSKEY_FIXED 8
// PASSKEY_RANDOM 16
// JUST_WORKS 32
// BOND_NEW 64
// BOND_REPAIR 128
// AUTHENTICATION_ON 256
#define KEY_FILE (1 << 10)
#define KEY_NEW  (1 << 11)
#define KEY_SENT (1 << 12)
#define PAIR_FILE (1 << 13)
#define PAIR_NEW  (1 << 14)

#define BTYPE_XALL 0
#define DIRN_FOR  0
#define DIRN_REV  1

// cryptoflag
#define CRY_NEW 1
#define CRY_SC (1 << 1)
#define CRY_PKEY (1 << 2)
#define CRY_BOND (1 << 3)
#define CRY_DONE (1 << 4)
#define CRY_AUTHOK (1 << 5)
#define CRY_POKNOTEN (1 << 6)

#define DISPLAY_ONLY 0
#define DISPLAY_YN 1
#define KEY_ONLY 2
#define NO_INOUT 3
#define KEY_DISPLAY 4
             
#define SERVDAT 128
struct servicedata 
  {
  int channel;  // 0=not used  RFCOMM channel for Classic  1 for LE  
  int handle;      // LE handle 
  int perm;        // LE permission
  int uuidtype;    // 2 or 16 length of uuid
  unsigned char uuid[16];
  char data[64];  // Channel name or LE Value from reading handle
  };

struct globpar 
  {
  int printflag;       // PRINT_NONE PRINT_NORMAL PRINT_VERBOSE
  int screenoff;       // printflag+8
  int debug;
  int serveractive;    // 1=is server
  int blockflag;       
  int hci;             // file desc for hci/acl commands sendhci/readhci
  int devid;           // hciX number
  int lecap;           // LE capable
  int sccap;           // Secure connect keys
  int timout;          // long time out for replies ms
  int toshort;         // short time out replies
  int cmdcount;        // command stack pointer
  int meshflag;        // 1=R 2=W
  int readerror;       // 0=none 1=time out 2=key press
  int keyboard;        // 0=GB  
  int exitchar;
  int ledatlenflag;    // 0=no data length change
  int lebufsize;
  int leintervalmin;
  int leintervalmax;
  int leclientwait;    // delay when connect as LE client
                       // to see any requests from server
  unsigned char rand[8];
  unsigned char buf[2048];
  char datfile[256];
  unsigned long long lastsend;
  int (*lecallback)();
              // screen print buffer 
  int prtp;   // current end of buffer
  int dump;   // dump destination always same
  int dumpn;  // dump char count - value never used
  int *prtv;  // points to prtp or dump
  int *prtvn; // points to prtp or dumpn
  int prtp0;  // start next print 
  int prtw;   // wrap index
  int prts;   // start of circular buffer
  int prte;   // end of print for scroll
  char *s;    // print buffer
  unsigned char *ssareply;  // SSA reply packet
  unsigned char *sdp;
  unsigned char randbadd[6];
  int dhkeydev;

  int randomfd;
  int hidflag; // 01=HID 10=User add 
  int settings;
  int keytocb;  // keys to callback     
  int maxpage;
  int notifynode;  // 0=all
  };

struct globpar gpar;

struct devdata *dev[NUMDEVS];  // allocated as needed by devalloc()

   // incoming stack for readhci()
   // each entry  type,length lo,length hi,device,data
   // length is number data bytes
   // total length of each entry = INSHEADSIZE + length
   
#define INSTACKSIZE 65536
#define INSHEADSIZE 4
unsigned char *instack; // allocated INSTACKSIZE in initblue()
unsigned char *insdat;  // instack+INSHEADSIZE
unsigned char *insdatn; // instack+INSHEADSIZE+findhci return 

struct sdpdata
  {
  int level;
  int record;
  int aid;
  int handle;  // aid=0 value
  int uuidtype;  // 1=not a target uuid  2=1101  3=16 byte uuid
  int numchan;   // number of RFCOMM channels found
  int nameflag;   // look for service name
  struct servicedata *serv;
  int servlen;  // length of serv[] array
  int chdn;     // serv[] index
  unsigned char *uuid;
  };

int meshpacket(unsigned char *buf);
void clscanx(void);
int lescanx(void);
int clconnect(int ndevice,int channel,int method);
int leservices(int ndevice,int flags,unsigned char *uuid);
int leservicesx(int ndevice,int flag,unsigned char *uuid,struct servicedata *serv);
int clservices(int ndevice,int flags,unsigned char *uuid,char *buf,int len);
int clservicesx(int ndevice,int flags,unsigned char *uuid,char *listbuf,int listlen,unsigned char *sdat,struct servicedata *serv);
void printlocalchannels(void);
int printchannels(int ndevice,int flags,struct servicedata *serv,int servlen,char *buf,int len);
int printctics0(int devicen,int flags,char *buf,int len);
int printctics1(int devicen);
int savectic(int devicen,struct servicedata *serv,int servlen);
int finduuidtext(int uuid);
void hexdump(unsigned char *buf, int len);
void printascii(unsigned char *s,int len);
unsigned char calcfcs(unsigned char *s,int count);
int pushins(long long int typebit,int ndevice,int len,unsigned char *s);
void popins(void);
int decodesdp(unsigned char *sin,int len,struct servicedata *serv,int servlen);
int decodedes(unsigned char *sin,int len,struct sdpdata *sdpp);
int openremotesdp(int ndevice);
int clconnect0(int ndevice);
int clconnectxx(int ndevice);
void clearins(int ndevice);
int connectpsm(int psm,int channel,int ndevice);
int disconnectl2(int ndevice,int psm);
void setcredits(int ndevice);
int readrf(int *ndevice,unsigned char *inbuff,int count,char endchar,int flag,int timeoutms);
int readhci(int ndevice,long long int mustflag,long long int lookflag,int timout,int toshort);
int leconnect(int ndevice);
int openremotesdpx(int ndevice);
int sconnectx(int ndevice);
int strcomp(char *s,char *t,int len);
unsigned int strinstr(char *s,char *t);
int hexchar(char c);
int entrylen(unsigned int *ind,int in);
int devalloc(unsigned char *badd,int type);
struct cticdata *cticalloc(int dn); 
struct cticdata *ctic(int ndevice,int n);
int devok(int ndevice);
int devokp(int ndevice);
int devlist(char *buf,int len,int mask);
void flushprint();
int devn(int node);
int devnp(int node);
int devnfrombadd(unsigned char *badd,int type,int dirn);
int devnfromhandle(unsigned char *hand);
char *baddstr(unsigned char *badd,int dirn);
int disconnectdev(int ndevice);
void meshreadon(void);
void meshreadoff(void);
int statusok(int flag,unsigned char *md);
int readline(FILE *stream,char *devs,char *s);
int newnode(void);
int classicserverx(int clientnode);
void immediate(long long lookflag);
int bincmp(unsigned char *s,unsigned char *t,int count,int dirn);
void waitdis(int ndevice,unsigned int timout);
int writecticx(int node,int cticn,unsigned char *data,int count,int notflag,int (*callback)());
void replysdp(int ndevice,int in,unsigned char *uuid,char *name);
int addaid(unsigned char *sdp,unsigned char *aid,int *rn,int aidj,int aidk,int aidn);
void swapk(int k0,int k1);
void rwlinkey(int rwflag,int ndevice,unsigned char *addr);
int localctics();
int leserver(int ndevice,int count,unsigned char *dat,int insn);
int nextctichandle(int start,int end,int *handle,int flag);
char *cticerrs(struct cticdata * cp);
void addname(int flag);
int setlelen(int ndevice,int len,int flag);
void checkpairflags(int *flags);
int check_init(int flag);
int sendhci(unsigned char *cmd,int ndevice);
int findhci(long long int type,int devicen,int popflag);
void printins(void);
int sconnect(int ndevice);
int readserial(int *ndevice,unsigned char *inbuff,int count,char endchar,int exitflags,int timeoutms);
unsigned char *strtohexx(char *s,int slen,int *num);
void save_pico_info();
void readleatt(int node,int handle);
void printval(unsigned char *s,int len,unsigned char *t);
int splitcmd(unsigned char *s,int plen,int ndevice);
int splitwrite(unsigned char *cmd,int len,int ndevice);
int stuuid(unsigned char *s);  

int calcc1(unsigned char *key,unsigned char *r,unsigned char *preq,unsigned char *pres,unsigned char iat,unsigned char rat,
            unsigned char *ia,unsigned char *ra,unsigned char *res);
int calcs1(unsigned char *key,unsigned char *r1,unsigned char *r2,unsigned char *out);
int calcf4(unsigned char *u, unsigned char *v,
           unsigned char *key, unsigned char z, unsigned char *res);
int calcf5(unsigned char *w, unsigned char *n1,
           unsigned char *n2, unsigned char *a1, unsigned char *a2,
           unsigned char *mackey, unsigned char *ltk);
int calcf6(unsigned char* w, unsigned char* n1,
           unsigned char* n2, unsigned char* r, unsigned char* io_cap,
           unsigned char* a1, unsigned char* a2, unsigned char* res);
int calcg2(unsigned char* u, unsigned char* v,
           unsigned char* x, unsigned char* y);
int calce(unsigned char *key,unsigned char *in,unsigned char *out,int flipflag);
int aes_cmac(unsigned char *key,unsigned char *msg,int msglen,unsigned char *res);
void bxor(unsigned char *a,unsigned char *b,unsigned char *c);
void bleft(unsigned char *a,unsigned char *b);
void getrand(unsigned char *s,int len);

// PICOSTACK replaced
int readkeyfile(unsigned char **table,int *count);
void writekeyfile(unsigned char *table,int count);
unsigned char *newkeytable(int count);
void freekeytable(unsigned char **table);
// Windows replaced
int sendpack(unsigned char *buf,int len);
int readpack(unsigned char *buf,int toms);
void printn(char *buf,int len);
int inithci(void);
int closehci(void);
void inputpin(char *prompt,char *sbuf);
void serverexit(int flag);
int setkeymode(int setflag);
int readkey(void);
int checkfilename(char *funs,char *s);
int getdatfile(char *s);
// in btlib.h
// void scroll_back(void);
// void scroll_forward(void);
// unsigned long long time_ms(void);
// void sleep_ms(int ms);
// Windows lost
int readbytes(unsigned char *dat,int *len,int wantlen,int toms);
int bluezdown(void);
int hcisock(void);
// Windows from btfw
int packet_size(int size);


/***************** Received PACKET TYPES for readhci() and findhci() *****************/
                              // 1 still available
#define IN_DATA     ((long long int)1 << 1)  // UIH or LE data 
#define IN_CLHAND   ((long long int)1 << 2)   // classic open reply
#define IN_ATTDAT   ((long long int)1 << 3)   // 02 channel 4 ATT e.g. LE characteristic value
#define IN_LESCAN    ((long long int)1 << 4)  // LE scan reply
#define IN_LINKREQ   ((long long int)1 << 5)  // HCI event 17 link key req
#define IN_IOCAPREQ  ((long long int)1 << 6)  // HCI event 31 io cap
#define IN_ACOMP     ((long long int)1 << 7)  // HCI event 06 authentication complete
#define IN_PCOMP     ((long long int)1 << 8)   // HCI event 36 simple pair complete
#define IN_CONFREQ   ((long long int)1 << 9)   // HCI event 33 confirmation request
#define IN_L2ASKCT   ((long long int)1 << 10)  // L2CAP channel 0001 opcode 02 connect request
#define IN_L2REPCT   ((long long int)1 << 11)  // L2CAP channel 0001 opcode 03 connect reply with result=0
#define IN_L2ASKCF   ((long long int)1 << 12)  // L2CAP channel 0001 opcode 04 config request
#define IN_L2REPCF   ((long long int)1 << 13)  // L2CAP channel 0001 opcode 05 config reply
#define IN_L2ASKDIS   ((long long int)1 << 14)  // L2CAP channel 0001 opcode 06 disconnect request
#define IN_L2REPDIS   ((long long int)1 << 15)  // L2CAP channel 0001 opcode 07 disconnect reply
#define IN_L2ASKINFO   ((long long int)1 << 16)  // L2CAP channel 0001 opcode 0A info request
#define IN_L2REPINFO   ((long long int)1 << 17)  // L2CAP channel 0001 opcode 0B info reply
#define IN_L2REPALL  (IN_L2REPCT | IN_L2REPCF | IN_L2REPDIS | IN_L2REPINFO)
#define IN_ENCR     ((long long int)1 << 18)  // Encrypt change with status = 0 
#define IN_RFUA     ((long long int)1 << 19)  // rfcomm UA reply
#define IN_PNRSP    ((long long int)1 << 20)  // PN RSP
#define IN_LEHAND   ((long long int)1 << 21)  // LE open reply
#define IN_DISCH    ((long long int)1 << 22)  // disconnect CONTROL/RFCOMM channel
#define IN_DISCOK   ((long long int)1 << 23)  // disconnect done
#define IN_STATOK   ((long long int)1 << 24)  // event 0E with first param status=0
#define IN_SSAREQ   ((long long int)1 << 25)  // SSA request 06 on psm 1
#define IN_SSAREP   ((long long int)1 << 26)  // SSA response 07
#define IN_CSCAN    ((long long int)1 << 27)  // classic scan result Event 02/22/2F
#define IN_CNAME    ((long long int)1 << 28)  // read classic name following scan
#define IN_KEY      ((long long int)1 << 29)  // link key notification
#define IN_BADD     ((long long int)1 << 30)  // read local board address
#define IN_INQCOMP ((long long int)1 << 31)  // Event 01 inquiry complete
#define IN_CONREQ  ((long long int)1 << 32)  
#define IN_CONCHAN ((long long int)1 << 33)  
#define IN_RFCHAN  ((long long int)1 << 34)  
#define IN_MSC     ((long long int)1 << 35)
#define IN_AUTOEND ((long long int)1 << 36)
#define IN_PINREQ  ((long long int)1 << 37)  // HCI event 16 pin code
#define IN_NOTIFY  ((long long int)1 << 38)
#define IN_ECHO    ((long long int)1 << 39)
#define IN_IOCAPRESP ((long long int)1 << 40)  // HCI event 32
#define IN_PAIRED    ((long long int)1 << 41)  // HCI event 36
#define IN_LECMD  ((long long int)1 << 42)    // LE server operation 
#define IN_PASSREQ  ((long long int)1 << 43)    // HCI event 34 
#define IN_PARAMREQ ((long long int)1 << 44)    // HCI event 3E/6 
#define IN_DATLEN   ((long long int)1 << 45)    // HCI event 3E/7 
#define IN_CONUP5   ((long long int)1 << 46)    // channel 5 connection paramters 
#define IN_LEACK    ((long long int)1 << 47)    // write ctic response 
#define IN_CRYPTO   ((long long int)1 << 48) 
#define IN_IMMED  ((long long int)1 << 63)

/***************** END Received PACKET TYPES *************/

#define AUTO_RF  1
#define AUTO_DIS 2
#define AUTO_MSC 3
#define AUTO_PAIROK 4
#define AUTO_PAIRFAIL 5
#define AUTO_PAIRREQ 6
#define AUTO_NOTIFY 7

/*********************** sendhci() PACKETS sent to the Bluetooth socket **********************/        
 
// When calling sendhci(hcicommand,ndevice)  hcicommand is a char array with the following format:

#define PAKHEADSIZE 4
         // PAKHEADSIZE = number of header bytes before packet
         // header = first four bytes [0]-[3]
         //    [0] = length of packet lo byte
         //    [1] = length of packet hi byte
         //    [2] = S2_ flags
         //    [3] = S3_ flags
         // [4].. packet - length bytes long

// The S2_ and S3_ flags specify how sendhci() modifies the packet by, for example, setting the board address
// [1].. are offsets from start of packet at [4]
  
#define S2_HAND 1           // dev[]->dhandle to [1][2] for 02 packets or [4][5] for 01 HCI cmd packets
#define S2_BADD (1 << 1)    // dev[]->bdaddr board address to [4] or [10] if LE open
#define S2_ID   (1 << 2)    // dev[]->xd->id  L2CAP channel 0001 id to [10] 
#define S2_DCIDC (1 << 3)   // dev[]->xd->dcid L2CAP channel 0040+ to [7][8] 
#define S2_ADD   (1 << 4)   // dev[]->rfchan modified RFCOMM address to [9] 
#define S2_FCS2  (1 << 5)   // set fcs at last byte  2 byte calc 
#define S2_FCS3  (1 << 6)   //                       3 byte calc
#define S2_SDP   (1 << 7)   // SDP operation

#define S3_DCID1 1          // dev[]->xd->dcid  L2CAP channel 0040+ to [13][14] remote device
#define S3_SCID2 (1 << 1)   //            scid                         [15][16] local device
#define S3_SCID1 (1 << 2)   //            scid                         [13][14] local device
#define S3_DCID2 (1 << 3)   //            dcid                         [15][16] remote device
#define S3_DLCIPN (1 << 4)  // dev[]->rfchan modified RFCOMM address to [14]  PN CMD
#define S3_ADDMSC (1 << 5)  // dev[]->rfchan modified RFCOMM address to [14]  MSC CMD/RSP
#define S3_ADDX   (1 << 6)  // dev[]->rfchan modified RFCOMM address to [9]   UA reply  

         
// hcicommand packets - send all the following via for example:  sendhci(bluclose,ndevice)       
unsigned char locsup[8] = { 4,0,0,0,0x01,0x02,0x10,0x00 };
unsigned char lerand[8] = { 4,0,0,0,0x01,0x18,0x20,0x00 };
unsigned char lemask[20] = { 12,0,0,0,0x01,0x01,0x20,0x08,0xFF,0x05,0,0,0,0,0,0 };   // BF 05 for no data len event
unsigned char lesetscan[16] = { 11,0,0,0,0x01,0x0B,0x20,0x07,0x01,0x10,0x00,0x10,0x00,0x00,0x02 };
unsigned char locbadd[10] = {4,0,0,0,0x01,0x09,0x10,0};
unsigned char btreset[10] = {4,0,0,0,0x01,0x03,0x0C,0};
unsigned char wln[8] = { 252,0,0,0,1,0x13,0x0C,0xF8 };  // copied to [256]
unsigned char lesuggest[16] = { 8,0,0,0,1,0x24,0x20,4,0xF8,0,TRANSMITUS & 0xFF,(TRANSMITUS >> 8) & 0xFF};
unsigned char leopen[40] = {29,0,S2_BADD,0,
  1,0x0D,0x20,0x19,0x60,0,0x60,0,0,0,0x7C,0x17,0x2D,0xC0,0x1E,0,0,0x18,0,0x28,0,0x00,0,0x11,0x01,0,0,0,0}; // len 29
unsigned char leconnup[32] = {21,0,S2_HAND,0,2,0x40,0,0x10,0,0x0C,0,0x05,0,0x12,0x03,0x08,0x00,0x06,0x00,0x06,0x00,0,0,0xF4,0x01};
unsigned char leupdate[32] = {18,0,S2_HAND,0,1,0x13,0x20,0x0E,0,0,0x18,0,0x28,0,0,0,0xF4,0x01,0,0,0,0}; // len 18
unsigned char lerrf[20] = {6,0,0,0,1,0x16,0x20,0x02,0,0}; // len 6
unsigned char authto[16] = {8,0,S2_HAND,0,1,0x7C,0x0C,0x04,0,0,0xB8,0x0B};
unsigned char lerepair[40] = {32,0,S2_HAND,0,1,0x19,0x20,0x1C,0,0,0,0,0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                                 // LE open [10]... board address     [23][24] = timeout x 10ms
unsigned char lecancel[8] = {4,0,0,0,0x01,0x0E,0x20,0};
unsigned char leconnreply[20] = {15,0,S2_HAND,0,2,0x40,0,0x0A,0,0x06,0,0x05,0,0x13,0x01,2,0,0,0 };

unsigned char bluclosex[16] = {7,0,0,0,1,6,4,3,0x40,0,0x13};  // len 7
unsigned char bluclose[16] = {7,0,S2_HAND,0,1,6,4,3,0x40,0,0x13};  // len 7
                                 // Classic and LE disconnect  device handle=[4][5]   
unsigned char lenotify[LEDATLEN+20] = {13,0,S2_HAND,0,2,0x40,0,8,0,4,0,4,0,0x1B,0x0B,0,0};  // len 13 if 1 byte
unsigned char lewrite[LEDATLEN+20] = {13,0,S2_HAND,0,2,0x40,0,8,0,4,0,4,0,0x52,0x0B,0,0};  // len 13 if 1 byte
              //  [1][2]=device handle  [9]=opcode  [10][11]=characteristic handle  [12]=data - up to LEDATLEN bytes
              //  [3]=size+7 [5]=size+3
unsigned char leread[20] = {12,0,S2_HAND,0,2,0x40,0,7,0,3,0,4,0,0x0A,0x12,0};  // len 12 
   
              //  [1][2]=device handle  [9]=0A read req [10][11]=handle of characteristic
unsigned char leindack[16] = { 10,0,S2_HAND,0,2,0x40,0x00,5,0,1,0,4,0,0x1E };   // ack indicate

unsigned char leread04[20] = {14,0,S2_HAND,0,2,0x40,0,9,0,5,0,4,0,0x04,0x01,0,0x01,0x00};     
unsigned char lereaduuid2[32] = {16,0,S2_HAND,0,2,0x40,0,11,0,7,0,4,0,0x08,0x01,0,0xFF,0xFF,0x03,0x28};   
unsigned char lereaduuid16[40] = {30,0,S2_HAND,0,2,0x40,0,25,0,21,0,4,0,0x08,0x01,0,0xFF,0xFF,
0x00,0xFF,0xEE,0xDD,0xCC,0xBB,0xAA,0x99,0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11};   
unsigned char leconf[20] = {10,0,S2_HAND,0,2,0x40,0,5,0,1,0,4,0,0x1E};  // len 12 

unsigned char mtuset[20] = {12,0,S2_HAND,0,2,0x40,0,7,0,3,0,4,0,0x02,0x19,0x00};  // len 14
unsigned char datlenset[16] = { 10,0,S2_HAND,0,1,0x22,0x20,0x06,0x40,0x00,0xFB,0x00,TRANSMITUS & 0xFF,(TRANSMITUS >> 8) & 0xFF};
unsigned char paramnegreply[16] = { 7,0,S2_HAND,0,1,0x21,0x20,0x03,0x40,0x00,0x3B };
unsigned char paramset[24] =   { 18,0,S2_HAND,0,1,0x13,0x20,0x0E,0x40,0x00,0x06,0,0x06,0,0,0,0xF4,0x01,0,0,0,0 };
unsigned char paramreply[24] = { 18,0,S2_HAND,0,1,0x20,0x20,0x0E,0x40,0x00,0x06,0,0x06,0,0,0,0xF4,0x01,0,0,0,0 };
               //  packet size = len+9      [3][4] = len + 4  [5][6] = len  [9] = data 
unsigned char attdata[1024] = { 0,0,S2_HAND,0,0x02,0,0,0,0,0,0,0x04,0x00 };

unsigned char lescanon[10] = {6,0,0,0,1,0x0C,0x20,2,1,0};  // scan for LE devices on  duplicate filter off
unsigned char lescanonf[10] = {6,0,0,0,1,0x0C,0x20,2,1,1};  // scan for LE devices on  duplicate filter on
unsigned char lescanoff[10] = {6,0,0,0,1,0x0C,0x20,2,0,0};  // scan for LE devices off 
       // n data = 8   [PAKHEADSIZE+11] = board
unsigned char leadvert[40] = { 36,0,0,0,0x01,0x08,0x20,0x20,0x0F,0x08,0xFF,0x34,0x12,
0x00,0x00,0xC0,0xDE,0x99,0x05,0x08,0x61,0x62,0x63,0x64,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

unsigned char leadvertx[40] = { 36,0,0,0,0x01,0x08,0x20,0x20,0x09,0x02,0x01,0x06,
0x05,0x08,0x61,0x62,0x63,0x64,0x00,0x00,0x00,0x00,0x0,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

//iBeacon advert UUID = 112233445566778899AABBCCDDEEFF00 Major=1234 Minor=5678
//unsigned char ibadvert[40] = { 36,0,0,0,0x01,0x08,0x20,0x20,
//0x1E,0x02,0x01,0x06,0x1A,0xFF,0x4C,0x00,0x02,0x15,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,
//0xEE,0xFF,0x00,0x12,0x34,0x56,0x78,0x00,0x00 };

   // keyboard appearance bytes = C1 03
unsigned char hidadvert[40] = { 36,0,0,0,0x01,0x08,0x20,0x20,0x10,0x02,0x01,0x06,0x03,0x19,
0xC1,0x03,
0x04,0x08,0x48,0x49,0x44,0x03,0x02,0x12,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00 };

unsigned char lerandadd[16] = { 10,0,0,0,0x01,0x05,0x20,6,0x11,0x22,0x33,0x44,0xAA,0xDC };

     // set advertising parmeters  [8]=type 0=connectable 3=non connectable, unidirected adv
                                                      //        min int   max int    x0.625ms 0800 = 1.28s  0200=320ms                               
unsigned char leadparam[32] =   { 19,0,0,0,0x01,0x06,0x20,0x0F,0x00,0x02,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x00 };
unsigned char leadparamx[32] =   { 19,0,0,0,0x01,0x06,0x20,0x0F,0x00,0x02,0x00,0x02,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x00 };

        // le advert  [4] 0=disable  1=enable
unsigned char leadvon[16] = { 5,0,0,0,0x01,0x0A,0x20,0x01,0x01 };
unsigned char leadvoff[16] = { 5,0,0,0,0x01,0x0A,0x20,0x01,0x00 };

       // classic scan ogf=1 (04)  ocf = 1  LAP=9e8b33 general discovery  time=8x1.28=10 secs num resp=0 unlimited
unsigned char cscan[16] = {9,0,0,0,1,0x01,0x04,5,0x33,0x8B,0x9E,8,0};
unsigned char cname[20] = {14,0,S2_BADD,0,1,0x19,0x04,0x0A,0x11,0x22,0x33,0x44,0x55,0x66,2,0,0,0};
                  // [4] 0=off 1=on
unsigned char scanr[10] = {4,0,0,0,1,0x19,0x0C,0};  // len 4 read I/P scan settings 
unsigned char scanx[10] = {5,0,0,0,1,0x1A,0x0C,1,0};  // len 5   I/P scans [8] 0=off  3=I/P scans
unsigned char scanip[10] = {5,0,0,0,1,0x1A,0x0C,1,3};  // len 5   I/P scans [8] 0=off  3=I/P scans
unsigned char readspm[10] = {4,0,0,0,1,0x55,0x0C,0};     // read simple pairing 
unsigned char clrspm[10] = {5,0,0,0,1,0x56,0x0C,1,0};  // set simple pairing [4]  0=off 
unsigned char readauth[10] = {4,0,0,0,1,0x1F,0x0C,0};     // read authentication 
unsigned char setauth[10] = {5,0,0,0,1,0x20,0x0C,1,0};  // set authentication [4] 0=off 1=on
unsigned char readexinq[10] = {4,0,0,0,1,0x51,0x0C,0};     // read ex inq
unsigned char pinreply[40] = { 27,0,S2_BADD,0,1,0x0D,0x04,23,0x56,0xDB,0x04,0x32,0xA6,0xDC,4,'0','0','0','0',0,0,0,0,0,0,0,0,0,0,0,0 }; 
unsigned char readlocname[32] = { 4,0,0,0,1,0x14,0x0C,0 };
unsigned char readremname[32] = { 14,0,S2_BADD,0,1,0x19,4,0x0A,0x56,0xDB,0x04,0x32,0xA6,0xDC,2,0,0,0 };

  // [PAKHEADSIZE+10] = key
unsigned char linkey[32] = { 26,0,S2_BADD,0,0x01,0x0B,0x04,0x16,0x13,0x71,0xDA,0x7D,0x1A,0x00,
0x08,0x7A,0xC7,0xFB,0x8C,0x86,0xF3,0xCF,0x36,0xF4,0x0C,0xD8,0xDD,0xA2,0xF9,0xD3 }; 

unsigned char pincode[40] = {27,0,S2_BADD,0,1,0x0D,4,23,0x11,0x22,0x33,0x44,0x55,0x66,4,'1','2','3','4',0,0,0,0,0,0,0,0,0,0,0,0 };
unsigned char passkey[40] = {14,0,S2_BADD,0,1,0x2E,4,10,0x11,0x22,0x33,0x44,0x55,0x66,0x01,0x02,0x03,0x04 };

unsigned char sabm0[64] = { 13,0,S2_HAND | S2_DCIDC | S2_FCS3,0,0x02,0x0C,0x00,0x08,0x00,0x04,0x00,0x40,0x00,0x03,0x3F,0x01,0x1C};


unsigned char pncmd[64] = { 23,0,S2_HAND | S2_DCIDC | S2_FCS2,S3_DLCIPN,0x02,0x0C,0x00,0x12,0x00,0x0E,0x00,0x40,0x00,0x03,0xEF,0x15,0x83,0x11,0x02,0xF0,
0x07,0x00,0x00,0x04,0x00,0x07,0x70};
       // [PAKHEADSIZE+18][19] = frame size  0400
unsigned char pnreply[64] = { 23,0,S2_HAND | S2_DCIDC | S2_FCS2,S3_DLCIPN,0x02,0x0C,0x00,0x12,0x00,0x0E,0x00,0x40,0x00,0x01,0xEF,0x15,0x81,0x11,0x02,0xF0,
0x07,0x00,0x00,0x04,0x00,0x07,0x70};

unsigned char sabmx[64] = { 13,0,S2_HAND | S2_DCIDC | S2_ADD | S2_FCS3,0,0x02,0x0C,0x00,0x08,0x00,0x04,0x00,0x40,0x00,0x0B,0x3F,0x01,0x59};


unsigned char msccmdrspe[64] = { 17,0,S2_HAND | S2_DCIDC | S2_FCS2,S3_ADDMSC,0x02,0x0C,0x00,0x0C,0x00,0x08,0x00,0x40,0x00,
0x03,0xEF,0x09,0xE3,0x05,0x0B,0x8D,0x70};

unsigned char msccmdrsp9[64] = {23,0,S2_HAND | S2_DCIDC | S2_FCS2,S3_ADDMSC,0x02,0x0C,0x00,0x12,0x00,0x0E,0x00,0x41,
0x00,0x01,0xEF,0x15,0x93,0x11,0x0B,0x07,0x00,0x00,0x00,0x00,0x01,0x00,0xAA };


unsigned char msccmdsend[32] = { 17,0,S2_HAND | S2_DCIDC | S2_FCS2,S3_ADDMSC,0x02,0x0C,0x00,0x0C,0x00,0x08,0x00,0x40,0x00,0x03,0xEF,0x09,0xE3,0x05,0x0B,0x8D,
0x70};

unsigned char setcred[64] = { 14,0,S2_HAND | S2_DCIDC | S2_ADD | S2_FCS2,0,0x02,0x0C,0x00,0x09,0x00,0x05,0x00,0x40,0x00,0x0B,0xFF,0x01,TOPUPCREDIT,0x86};
unsigned char setcred0[64] = { 14,0,S2_HAND | S2_DCIDC | S2_FCS2,0,0x02,0x0C,0x00,0x09,0x00,0x05,0x00,0x40,0x00,0x09,0xFF,0x01,0x21,0x86};
                                // [PAKHEADSIZE+12] = number of credits = TOPUPCREDIT

unsigned char closereqrf[64] = { 13,0,S2_HAND | S2_DCIDC | S2_ADD | S2_FCS3,0,0x02,0x0C,0x00,0x08,0x00,0x04,0x00,0x40,0x00,0x09,0x53,0x01,0xF3};
unsigned char closereq0[64] = { 13,0,S2_HAND | S2_DCIDC | S2_FCS3,0,0x02,0x0C,0x00,0x08,0x00,0x04,0x00,0x40,0x00,0x03,0x53,0x01,0xF3};

unsigned char uareply[20] = { 13,0,S2_HAND | S2_DCIDC | S2_FCS3,0,0x02,0x0C,0x00,0x08,0x00,0x04,0x00,0x40,0x00,0x01,0x73,0x01,0xB6};

unsigned char blusend[2048] = { 14,0,S2_HAND | S2_DCIDC | S2_ADD | S2_FCS2,0,0x02,0x0C,0x00,0x09,0x00,0x05,0x00,0x40,0x00,0x0B,0xEF,0x03,0x21,0x9A};

unsigned char clopen[32] = { 17,0,S2_BADD,0,
                       0x01,0x05,0x04,0x0D,0x56,0xDB,0x04,0x32,0xA6,0xDC,0x18,0xCC,0x02,0x00,0x00,0x00,0x01};
                           // classic open [4].. board address
unsigned char clcancel[16] = {10,0,S2_BADD,0,0x01,0x08,0x04,6,0x11,0x22,0x33,0x44,0x55,0x66 };
 
unsigned char setspm[10] = {5,0,0,0,1,0x56,0x0C,1,1};  // set simple pairing [4]  1=on
unsigned char eventmask[20] =  { 12,0,0,0,1,1,0x0C,8,0xFF,0xFF,0xFB,0xFF,0x07,0xF8,0xBF,0x3D };  // default  FF FF DF = disable PIN request
unsigned char authreq[16] = {  6,0,S2_HAND,0,0x01,0x11,0x04,0x02,0x0C,0x00};
unsigned char linkreply[20] = { 10,0,S2_BADD,0,0x01,0x0C,0x04,0x06,0x56,0xDB,0x04,0x32,0xA6,0xDC};

unsigned char iocapreply[20] = { 13,0,S2_BADD,0,0x01,0x2B,0x04,0x09,0x56,0xDB,0x04,0x32,0xA6,0xDC,0x03,0x00,0x00};     
unsigned char confreply[16] = { 10,0,S2_BADD,0,0x01,0x2C,0x04,0x06,0x56,0xDB,0x04,0x32,0xA6,0xDC};
unsigned char encryptx[16] = {  7,0,S2_HAND,0,0x01,0x13,0x04,0x03,0x0C,0x00,0x01};

unsigned char psmreply[32] = { 21,0,S2_HAND | S2_ID,S3_SCID1 | S3_DCID2,0x02,0x0C,0x00,0x10,0x00,0x0C,0x00,0x01,0x00,0x03,0x03,0x08,0x00,0x04,0x00,0x42,0x00,0x00,0x00,0x00,0x00 };

unsigned char conpsm1[32] = { 17,0,S2_HAND | S2_ID,S3_SCID2,0x02,0x0C,0x00,0x0C,0x00,0x08,0x00,0x01,0x00,0x02,0x03,0x04,0x00,0x01,0x00,0x40,0x00};
             // psm = 1/3 at PAKHEADSIZE+ [13]
unsigned char figreply[40] = { 23,0,S2_HAND | S2_ID,S3_DCID1,0x02,0x0C,0x00,0x12,0x00,0x0E,0x00,0x01,0x00,0x05,0x03,0x0A,0x00,0x40,0x00,0x00,
0x00,0x00,0x00,0x01,0x02,0xF5,0x03};
unsigned char figreq[40] = { 32,0,S2_HAND | S2_ID,S3_DCID1,0x02,0x0C,0x00,0x1B,0x00,0x17,0x00,0x01,0x00,0x04,0x04,0x13,0x00,0x40,0x00,0x00,
0x00,0x01,0x02,0xF5,0x03,0x04,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    // SSA request to read SDP database
    // PAKHEADSIZE+[]
    // returns all records containing 2-byte UUID = [17][18] = 0003 so will find RFCOMM channels which have aid=4/UUID=0100/UUID=0003 
    // returns aids in range [24][25] = 0000  to [26][27] = FFFF so all aid
    // need aid=1 (registered UUID of service)  aid=4 (RFCOMM channel number)  aid=0100 (service name)
    // set [24][25] = 0001  [26][27] = 0004 to read aid = 1 to 4 only to find RFCOMM channels (but will miss service name aid = 0100)
        
unsigned char ssareq[128] = { 29,0,S2_HAND | S2_SDP,0,0x02,0x0C,0x00,0x18,0x00,0x14,0x00,0x41,0x00,0x06,0x00,0x00,0x00,0x0F,0x35,0x03,
0x19,0x00,0x03,0xFF,0xFF,0x35,0x05,0x0A,0x00,0x00,0xFF,0xFF,0x00};    // MAY ADD TO LENGTH leave [128] long all aid

unsigned char psmdisreq[32] = { 17,0,S2_HAND | S2_ID,S3_DCID1 | S3_SCID2,0x02,0x0C,0x00,0x0C,0x00,0x08,0x00,0x01,0x00,0x06,0x09,0x04,0x00,0x40,0x00,0x40,
0x00}; 

unsigned char psmdisreply[32] = { 17,0,S2_HAND | S2_ID,S3_SCID1 | S3_DCID2,0x02,0x0C,0x00,0x0C,0x00,0x08,0x00,0x01,0x00,0x07,0x08,0x04,0x00,0x40,0x00,0x40,
0x00};

unsigned char foboff[32] = { 21,0,S2_HAND | S2_ID,0,0x02,0x0C,0x00,0x10,0x00,0x0C,0x00,0x01,0x00,0x03,0x05,0x08,0x00,0x00,0x00,0x00,
0x00,0x02,0x00,0x00,0x00};
                    // [PAKHEADSIZE+17] = 02  psm not supp   scid/dcid ignored  OK reply was S3_SCID1 | S3_DCID2

unsigned char storekey[40] =  { 27,0,S2_BADD,0,1,0x11,0x0C,23,1,0x11,0x22,0x33,0x44,0x55,0x66,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }; 
                           // board address at PAKHEADSIZE +  [5]  key at [11-26]
unsigned char readstorekey[16] =  { 11,0,S2_BADD,0,1,0x0D,0x0C,7,0x11,0x22,0x33,0x44,0x55,0x66,0 }; 

   
     // inforeply()
unsigned char inforeply2[32] = { 21,0,S2_HAND | S2_ID,0,0x02,0x0C,0x00,0x10,0x00,0x0C,0x00,0x01,0x00,0x0B,0x01,0x08,0x00,0x02,0x00,0x00,
0x00,0xB8,0x02,0x00,0x00};
unsigned char inforeply3[32] = { 25,0,S2_HAND | S2_ID,0,0x02,0x0C,0x00,0x14,0x00,0x10,0x00,0x01,0x00,0x0B,0x02,0x0C,0x00,0x03,0x00,0x00,
0x00,0x86,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
unsigned char echoreq[20] = { 13,0,S2_HAND | S2_ID,0,0x02,0x0C,0x00,0x08,0x00,0x04,0x00,0x01,0x00,0x08,0x01,0x00,0x00 };
unsigned char echoreply[20] = { 13,0,S2_HAND | S2_ID,0,0x02,0x0C,0x00,0x08,0x00,0x04,0x00,0x01,0x00,0x09,0x01,0x00,0x00 };


unsigned char readcto[10] = { 4,0,0,0,0x01,0x15,0x0C,0x00 };
unsigned char setcto[16] =  { 6,0,0,0,0x01,0x16,0x0C,0x02,0xA0,0x3F };
unsigned char readpto[10] = { 4,0,0,0,0x01,0x17,0x0C,0x00 };
unsigned char setpto[16] =  { 6,0,0,0,0x01,0x18,0x0C,0x02,0x00,0x40 };


unsigned char lebufsz[8] = {4,0,0,0,0x01,0x02,0x20,0};

   // classic server
unsigned char conaccept[16] =  { 11,0,S2_BADD,0,0x01,0x09,0x04,0x07,0x11,0x22,0x33,0x44,0x55,0x66,0x00 };
unsigned char conreject[16] =  { 11,0,0,0,0x01,0x0A,0x04,0x07,0x11,0x22,0x33,0x44,0x55,0x66,0x0E };
unsigned char spcomp[20] =   { 10,0,S2_BADD,0,0x01,0x2C,0x04,0x06,0x11,0x22,0x33,0x44,0x55,0x66 };


unsigned char baseuuid[16] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0xFF};    
unsigned char standard[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5F,0x9B,0x34,0xFB};    
  

  // LE server
unsigned char lereadreply[LEDATLEN+20] = {11,0,S2_HAND,0,2,0x40,0,0x06,0,0x02,0,4,0,0x0B,0};  // length 10+number bytes

unsigned char le05reply[40]  = {15,0,S2_HAND,0,2,0x40,0,0x0A,0,0x06,0,4,0,0x05,0x01}; 
unsigned char le07reply[20]  = {14,0,S2_HAND,0,2,0x40,0,0x09,0,0x05,0,4,0,0x07,0x00,0x00,0x00,0x00}; 
 
unsigned char le09replyv[LEDATLEN+24] =  {32,0,S2_HAND,0,2,0x40,0,0x1B,0,0x17,0,4,0,0x09};

unsigned char leack[16] =  {10,0,S2_HAND,0,2,0x40,0,0x05,0,0x01,0,4,0,0x13};
unsigned char lemtu[16] =  {12,0,S2_HAND,0,2,0x40,0,0x07,0,0x03,0,4,0,0x03,23,0};  // MTU 23-512


unsigned char fob05[20]  = {15,0,S2_HAND,0,2,0x40,0,0x0A,0,0x06,0,4,0,0x05,0x01,0x01,0x00,0x00,0x28}; 
unsigned char fob09[20]  = {15,0,S2_HAND,0,2,0x40,0,0x0A,0,0x06,0,4,0,0x09,0x04,0x01,0x00,0x00,0x18}; 
unsigned char fob11[24]  = {17,0,S2_HAND,0,2,0x40,0,0x0C,0,0x08,0,4,0,0x11,0x06,0x01,0x00,0xFF,0xFF,0x00,0x18}; 
 
unsigned char lefail[20] =  {14,0,S2_HAND,0,2,0x40,0,9,0,5,0,4,0,0x01,0x08,0,0,0x0A};  

unsigned char custuuid[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
char custname[64] = "None";

unsigned char preq[20]  = {16,0,S2_HAND,0,2,0x40,0,0x0B,0,0x07,0,6,0,0x01,0x04,0x00,0x00,0x10,0x00,0x01}; 
unsigned char pres[20]  = {16,0,S2_HAND,0,2,0x40,0,0x0B,0,0x07,0,6,0,0x02,0x03,0x00,0x00,0x10,0x00,0x01}; 
unsigned char confirm[32] = {26,0,S2_HAND,0,2,0x40,0,0x15,0,0x11,0,6,0,0x03,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
unsigned char sendrand[32] =  {26,0,S2_HAND,0,2,0x40,0,0x15,0,0x11,0,6,0,0x04,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
unsigned char sendkey[32] = { 22,0,S2_HAND,0,1,0x1A,0x20,0x12,0x40,0x00,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
unsigned char negkey[16] = { 6,0,S2_HAND,0,1,0x1B,0x20,2,0,0 };
unsigned char ltkcrypt[40] = { 32,0,S2_HAND,0,1,0x19,0x20,0x1C,0x40,0x00,0,0,0,0,0,0,0,0,0,0,
                     1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
unsigned char sendltk[32] =  {26,0,S2_HAND,0,2,0x40,0,0x15,0,0x11,0,6,0,0x06,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
unsigned char sendci[32] =  {20,0,S2_HAND,0,2,0x40,0,0x0F,0,0x0B,0,6,0,0x07,1,2,3,4,5,6,7,8,9,10 };
unsigned char confail[16] =  {11,0,S2_HAND,0,2,0x40,0,0x06,0,0x02,0,6,0,0x05,4 };
unsigned char conup[32] = { 18,0,S2_HAND,0,1,0x13,0x20,14,0x40,0,0x30,0x00,0x30,0x00,0x02,0x00,0xC0,0x03,0x00,0x00,0x00,0x00 };
unsigned char keypair[16] = {4,0,0,0,1,0x25,0x20,0x00}; 
unsigned char dhkey[80] =   {68,0,0,0,1,0x26,0x20,0x40}; 
unsigned char sendxykeys[80] =  {74,0,S2_HAND,0,2,0x40,0,0x45,0,0x41,0,6,0,0x0C };
unsigned char dhcheck[32] =  {26,0,S2_HAND,0,2,0x40,0,0x15,0,0x11,0,6,0,0x0D };
unsigned char leservsecreq[16] = {11,0,S2_HAND,0,2,0x40,0,6,0,2,0,6,0,0x0B,0 };

/********************** END sendhci() PACKETS *********************/


/*************** PRINT BUFFER **********/

#if (defined(BTFWINDOWS) || defined(PICOSTACK))                       
#define PRBUFSZ 4092
#define PRLINESZ 256
#define PRPAGESZ 1024
#else
#define PRBUFSZ 65532
#define PRLINESZ 256
#define PRPAGESZ 8192
#endif

  // normal prints always go to print buffer
#define NPRINT gpar.prtp += sprintf(gpar.s+gpar.prtp,
  // verbose prints go to print buffer or dump
#define VPRINT *gpar.prtvn += sprintf(gpar.s+*gpar.prtv,

/*********** END PRINT BUFFER ************/


/*************** TEXT ************************/


/*********** 2-byte UUID codes *******************/

char uuidlist[11200] = {"\000\
\052\000Device Name\000\052\001Appearance\000\
\052\002Peripheral Privacy Flag\000\052\003Reconnection Address\000\
\052\004Pref Connection Parameters\000\052\005Service Changed\000\
\052\006Alert Level\000\052\007Tx Power Level\000\
\052\010Date Time\000\052\011Day of Week\000\
\052\012Day Date Time\000\052\014Exact Time 256\000\
\052\015DST Offset\000\052\016Time Zone\000\
\052\017Local Time Info\000\052\021Time with DST\000\
\052\022Time Accuracy\000\052\023Time Source\000\
\052\024Reference Time Info\000\052\026Time Update Control Point\000\
\052\027Time Update State\000\052\030Glucose Measurement\000\
\052\031Battery Level\000\052\034Temperature Measurement\000\
\052\035Temperature Type\000\052\036Intermediate Temperature\000\
\052\041Measurement Interval\000\052\042Boot Keybrd Input Report\000\
\052\043System ID\000\052\044Model Number String\000\
\052\045Serial Number String\000\052\046Firmware Revision String\000\
\052\047Hardware Revision String\000\052\050Software Revision String\000\
\052\051Manufacturer Name String\000\052\052IEEE 11073-20601 List\000\
\052\053Current Time\000\052\061Scan Refresh\000\
\052\062Boot Keybrd Output Report\000\052\063Boot Mouse Input Report\000\
\052\064Glucose Measure Context\000\052\065Blood Pressure Measure\000\
\052\066Intermediate Cuff Pressure\000\052\067Heart Rate Measure\000\
\052\070Body Sensor Location\000\052\071Heart Rate Control Point\000\
\052\077Alert Status\000\052\100Ringer Control Point\000\
\052\101Ringer Setting\000\052\102Alert Category ID Bit Mask\000\
\052\103Alert Category ID\000\052\104Alert Notify Control Point\000\
\052\105Unread Alert Status\000\052\106New Alert\000\
\052\107New Alert Category\000\052\110Unread Alert Category\000\
\052\111Blood Pressure Feature\000\052\112HID Information\000\
\052\113Report Map\000\052\114HID Control Point\000\
\052\115Report\000\052\116Protocol Mode\000\
\052\117Scan Interval Window\000\052\120PnP ID\000\
\052\121Glucose Feature\000\052\122Record Access Cntrl Point\000\
\052\123RSC Measurement\000\052\124RSC Feature\000\
\052\125SC Control Point\000\052\132Aggregate\000\
\052\133CSC Measurement\000\052\134CSC Feature\000\
\052\135Sensor Location\000\052\136PLX Spot-Check Measure\000\
\052\137PLX Continuous Measure\000\052\140PLX Features\000\
\052\143Cycling Power Measure\000\052\144Cycling Power Vector\000\
\052\145Cycling Power Feature\000\052\146Cycling Power Cntrl Point\000\
\052\147Location and Speed\000\052\150Navigation\000\
\052\151Position Quality\000\052\152LN Feature\000\
\052\153LN Control Point\000\052\154Elevation\000\
\052\155Pressure\000\052\156Temperature\000\
\052\157Humidity\000\052\160True Wind Speed\000\
\052\161True Wind Direction\000\052\162Apparent Wind Speed\000\
\052\163Apparent Wind Direction\000\052\164Gust Factor\000\
\052\165Pollen Concentration\000\052\166UV Index\000\
\052\167Irradiance\000\052\170Rainfall\000\
\052\171Wind Chill\000\052\172Heat Index\000\
\052\173Dew Point\000\052\175Descriptor Value Changed\000\
\052\176Aerobic Heartrate Low Limit\000\052\177Aerobic Threshold\000\
\052\200Age\000\052\201Anaerobic Heartrate Low Limit\000\
\052\202Anaerobic Heartrate Top Limit\000\052\203Anaerobic Threshold\000\
\052\204Aerobic Heartrate Top Limit\000\052\205Date of Birth\000\
\052\206Date of Threshold Assessment\000\052\207Email Address\000\
\052\210Fat Burn Heartrate Low Limit\000\052\211Fat Burn Heartrate Top Limit\000\
\052\212First Name\000\052\213Five Zone Heart Rate Limits\000\
\052\214Gender\000\052\215Heart Rate Max\000\
\052\216Height\000\052\217Hip Circumference\000\
\052\220Last Name\000\052\221Max Recommended Heartrate\000\
\052\222Resting Heart Rate\000\052\223Sport Type for Thresholds\000\
\052\224Three Zone Heartrate Limits\000\052\225Two Zone Heartrate Limits\000\
\052\226VO2 Max\000\052\227Waist Circumference\000\
\052\230Weight\000\052\231Database Change Increment\000\
\052\232User Index\000\052\233Body Composition Feature\000\
\052\234Body Composition Measure\000\052\235Weight Measurement\000\
\052\236Weight Scale Feature\000\052\237User Control Point\000\
\052\240Magnetic Flux Density 2D\000\052\241Magnetic Flux Density 3D\000\
\052\242Language\000\052\243Barometric Pressure Trend\000\
\052\244Bond Management Cntrl Pt\000\052\245Bond Management Feature\000\
\052\246Central Address Resolution\000\052\247CGM Measurement\000\
\052\250CGM Feature\000\052\251CGM Status\000\
\052\252CGM Session Start Time\000\052\253CGM Session Run Time\000\
\052\254CGM Specific Ops Cntrl Pt\000\052\255Indoor Positioning Config\000\
\052\256Latitude\000\052\257Longitude\000\
\052\260Local North Coordinate\000\052\261Local East Coordinate\000\
\052\262Floor Number\000\052\263Altitude\000\
\052\264Uncertainty\000\052\265Location Name\000\
\052\266URI\000\052\267HTTP Headers\000\
\052\270HTTP Status Code\000\052\271HTTP Entity Body\000\
\052\272HTTP Control Point\000\052\273HTTPS Security\000\
\052\274TDS Control Point\000\052\275OTS Feature\000\
\052\276object name\000\052\277object type\000\
\052\300object size\000\052\301object first created\000\
\052\302object last modified\000\052\303object ID\000\
\052\304object properties\000\052\305object actioncontrol point\000\
\052\306object list control point\000\052\307object list filter\000\
\052\310object changed\000\052\311Resolvable Private Address\000\
\052\312Unspecified\000\052\313Directory Listing\000\
\052\314Fitness Machine Feature\000\052\315Treadmill Data\000\
\052\316Cross Trainer Data\000\052\317Step Climber Data\000\
\052\320Stair Climber Data\000\052\321Rower Data\000\
\052\322Indoor Bike Data\000\052\323Training Status\000\
\052\324Supported Speed Range\000\052\325Supported Inclination Range\000\
\052\326Supported Resistance Range\000\052\327Supported Heart Rate Range\000\
\052\330Supported Power Range\000\052\331Fitness Machine Cntrl Point\000\
\052\332Fitness Machine Status\000\052\333Mesh Provisioning Data In\000\
\052\334Mesh Provisioning Data Out\000\052\335Mesh Proxy Data In\000\
\052\336Mesh Proxy Data Out\000\052\340Average Current\000\
\052\341Average Voltage\000\052\342Boolean\000\
\052\343Chromatic Distance Planckian\000\052\344Chromaticity Coordinates\000\
\052\345Chromaticity CCT+Duv Values\000\052\346Chromaticity Tolerance\000\
\052\347CIE Color Rendering Index\000\052\350Coefficient\000\
\052\351Correlated Color Temperature\000\052\352Count 16\000\
\052\353Count 24\000\052\354Country Code\000\
\052\355Date UTC\000\052\356Electric Current\000\
\052\357Electric Current Range\000\052\360Electric Current Spec\000\
\052\361Electric Current Statistics\000\052\362Energy\000\
\052\363Energy in a Period Of Day\000\052\364Event Statistics\000\
\052\365Fixed String 16\000\052\366Fixed String 24\000\
\052\367Fixed String 36\000\052\370Fixed String 8\000\
\052\371Generic Level\000\052\372Global Trade Item Number\000\
\052\373Illuminance\000\052\374Luminous Efficacy\000\
\052\375Luminous Energy\000\052\376Luminous Exposure\000\
\052\377Luminous Flux\000\053\000Luminous Flux Range\000\
\053\001Luminous Intensity\000\053\002Mass Flow\000\
\053\003Perceived Lightness\000\053\004Percentage 8\000\
\053\005Power\000\053\006Power Specification\000\
\053\007Rel Runtime in Current Range\000\053\010Rel Runtime in Generic Range\000\
\053\011Rel Value in Voltage Range\000\053\012Rel Value Illuminance Range\000\
\053\013Rel Value in Period Of Day\000\053\014Rel Value Temperature Range\000\
\053\015Temperature 8\000\053\016Temperature 8 in Period/Day\000\
\053\017Temperature 8 Statistics\000\053\020Temperature Range\000\
\053\021Temperature Statistics\000\053\022Time Decihour 8\000\
\053\023Time Exponential 8\000\053\024Time Hour 24\000\
\053\025Time Millisecond 24\000\053\026Time Second 16\000\
\053\027Time Second 8\000\053\030Voltage\000\
\053\031Voltage Specification\000\053\032Voltage Statistics\000\
\053\033Volume Flow\000\053\034Chromaticity Coordinate\000\
\053\035RC Feature\000\053\036RC Settings\000\
\053\037Reconnect Config Cntrl Pt\000\053\040IDD Status Changed\000\
\053\041IDD Status\000\053\042IDD Annunciation Status\000\
\053\043IDD Features\000\053\044IDD Status Reader Cntrl Pt\000\
\053\045IDD Command Cntrl Pt\000\053\046IDD Command Data\000\
\053\047IDD Record Access Cntrl Pt\000\053\050IDD History Data\000\
\053\051Client Supported Features\000\053\052Database Hash\000\
\053\053BSS Control Point\000\053\054BSS Response\000\
\053\055Emergency ID\000\053\056Emergency Text\000\
\053\072Server Supported Features\000\000\001SDP\000\
\000\002UDP\000\000\003RFCOMM\000\
\000\004TCP\000\000\005TCS-BIN\000\
\000\006TCS-AT\000\000\007ATT\000\
\000\010OBEX\000\000\011IP\000\
\000\012FTP\000\000\014HTTP\000\
\000\016WSP\000\000\017BNEP\000\
\000\020UPNP\000\000\021HIDP\000\
\000\022HardcopyCtrlChannel\000\000\024HardcopyDataChannel\000\
\000\026HardcopyNotification\000\000\027AVCTP\000\
\000\031AVDTP\000\000\033CMTP\000\
\000\036MCAPControlChannel\000\000\037MCAPDataChannel\000\
\001\000L2CAP\000\020\000SrvrDiscServID\000\
\020\001BrwseGrpDescServID\000\021\001SerialPort\000\
\021\002LANAccessUsingPPP\000\021\003DialupNetworking\000\
\021\004IrMCSync\000\021\005OBEXObjectPush\000\
\021\006OBEXFileTransfer\000\021\007IrMCSyncCommand\000\
\021\010Headset\000\021\011CordlessTelephony\000\
\021\012AudioSource\000\021\013AudioSink\000\
\021\014A/V_RemoteCtrlTarget\000\021\015AdvancedAudioDist\000\
\021\016A/V_RemoteControl\000\021\017A/V_RemoteCtrlController\000\
\021\020Intercom\000\021\021Fax\000\
\021\022Headset Audio Gateway\000\021\023WAP\000\
\021\024WAP_CLIENT\000\021\025PANU\000\
\021\026NAP\000\021\027GN\000\
\021\030DirectPrinting\000\021\031ReferencePrinting\000\
\021\032Basic Imaging Profile\000\021\033ImagingResponder\000\
\021\034ImagingAutomaticArchive\000\021\035ImagingReferencedObjects\000\
\021\036Handsfree\000\021\037HandsfreeAudioGateway\000\
\021\040DirectPrintRefObjServ\000\021\041ReflectedUI\000\
\021\042BasicPrinting\000\021\043PrintingStatus\000\
\021\044HumanInterfaceDevServ\000\021\045HardcopyCableReplace\000\
\021\046HCR_Print\000\021\047HCR_Scan\000\
\021\050Common_ISDN_Access\000\021\055SIM_Access\000\
\021\056Phonebook Access PCE\000\021\057Phonebook Access PSE\000\
\021\060Phonebook Access\000\021\061Headset HS\000\
\021\062Message Access Server\000\021\063Message Notify Server\000\
\021\064Message Access Profile\000\021\065GNSS\000\
\021\066GNSS_Server\000\021\0673D Display\000\
\021\0703D Glasses\000\021\0713D Synchronization\000\
\021\072MPS Profile UUID\000\021\073MPS SC UUID\000\
\021\074CTN Access Service\000\021\075CTN Notify Service\000\
\021\076CTN Profile\000\022\000PnPInformation\000\
\022\001GenericNetworking\000\022\002GenericFileTransfer\000\
\022\003GenericAudio\000\022\004GenericTelephony\000\
\022\005UPNP_Service\000\022\006UPNP_IP_Service\000\
\023\000ESDP_UPNP_IP_PAN\000\023\001ESDP_UPNP_IP_LAP\000\
\023\002ESDP_UPNP_L2CAP\000\023\003VideoSource\000\
\023\004VideoSink\000\023\005VideoDistribution\000\
\024\000HDP\000\024\001HDP Source\000\
\024\002HDP Sink\000\030\000Generic Access\000\
\030\001Generic Attribute\000\030\002Immediate Alert\000\
\030\003Link Loss\000\030\004Tx Power\000\
\030\005Current Time\000\030\006Reference Time Update\000\
\030\007Next DST Change\000\030\010Glucose\000\
\030\011Health Thermometer\000\030\012Device Information\000\
\030\015Heart Rate\000\030\016Phone Alert Status\000\
\030\017Battery\000\030\020Blood Pressure\000\
\030\021Alert Notification\000\030\022Human Interface Device\000\
\030\023Scan Parameters\000\030\024Running Speed + Cadence\000\
\030\025Automation IO\000\030\026Cycling Speed + Cadence\000\
\030\030Cycling Power\000\030\031Location and Navigation\000\
\030\032Environmental Sensing\000\030\033Body Composition\000\
\030\034User Data\000\030\035Weight Scale\000\
\030\036Bond Management\000\030\037Continuous Glucose Monitor\000\
\030\040Internet Protocol Support\000\030\041Indoor Positioning\000\
\030\042Pulse Oximeter\000\030\043HTTP Proxy\000\
\030\044Transport Discovery\000\030\045Object Transfer\000\
\030\046Fitness Machine\000\030\047Mesh Provisioning\000\
\030\050Mesh Proxy\000\030\051Reconnection Config\000\
\030\072Insulin Delivery\000\030\073Binary Sensor\000\
\030\074Emergency Config\000\047\000unitless\000\
\047\001length (m)\000\047\002mass (kg)\000\
\047\003time (s)\000\047\004electric current (amp)\000\
\047\005Temperature (kelvin)\000\047\006amount of substance (mole)\000\
\047\007luminous intensity (candela)\000\047\020area (m2)\000\
\047\021volume (m3)\000\047\022velocity (m/s)\000\
\047\023acceleration (m/s2)\000\047\024wavenumber (1/m)\000\
\047\025density (kg/m3)\000\047\026surface density (kg/m2)\000\
\047\027specific volume (m3/kg)\000\047\030current density (amp/m2)\000\
\047\031magnetic field (amp/m)\000\047\032amount conc (mole/m3)\000\
\047\033mass conc (kg/m3)\000\047\034luminance (candela/m2)\000\
\047\035refractive index\000\047\036relative permeability\000\
\047\040plane angle (radian)\000\047\041solid angle (steradian)\000\
\047\042frequency (hertz)\000\047\043force (newton)\000\
\047\044pressure (pascal)\000\047\045energy (joule)\000\
\047\046power (watt)\000\047\047electric charge (coulomb)\000\
\047\050potential diff (volt)\000\047\051capacitance (farad)\000\
\047\052resistance (ohm)\000\047\053conductance (siemens)\000\
\047\054magnetic flux (weber)\000\047\055magnetic flux (tesla)\000\
\047\056inductance (henry)\000\047\057Temperature (deg C)\000\
\047\060luminous flux (lumen)\000\047\061illuminance (lux)\000\
\047\062radio activity (becquerel)\000\047\063absorbed dose (gray)\000\
\047\064dose equivalent (sievert)\000\047\065catalytic activity (katal)\000\
\047\100dynamic viscosity (pascal s)\000\047\101moment of force (Nm)\000\
\047\102surface tension (N/m)\000\047\103angular velocity (radian/s)\000\
\047\104angular accel (radian/s2)\000\047\105heat flux density (W/m2)\000\
\047\106heat capacity (J/kelvin)\000\047\107specific heat cap (J/kg K)\000\
\047\110specific energy (J/kg)\000\047\111thermal conduct (W/m K)\000\
\047\112energy density (J/m3)\000\047\113field strength (V/m)\000\
\047\114charge density (C/m3)\000\047\115surface charge dens (C/m2)\000\
\047\116electric flux dens (C/m2)\000\047\117permittivity (F/m)\000\
\047\120permeability (H/m)\000\047\121molar energy (J/mole)\000\
\047\122molar entropy (J/mole K)\000\047\123exposure (C/kg)\000\
\047\124absorbed dose rate (gray/s)\000\047\125radiant intensity (W/ster)\000\
\047\126radiance (W/m2 ster)\000\047\127catalytic conc (katal/m3)\000\
\047\140time (minute)\000\047\141time (hour)\000\
\047\142time (day)\000\047\143plane angle (degree)\000\
\047\144plane angle (minute)\000\047\145plane angle (second)\000\
\047\146area (hectare)\000\047\147volume (litre)\000\
\047\150mass (tonne)\000\047\200pressure (bar)\000\
\047\201pressure (mm of mercury)\000\047\202length (ngstr m)\000\
\047\203length (nautical mile)\000\047\204area (barn)\000\
\047\205velocity (knot)\000\047\206log radio quantity (neper)\000\
\047\207log radio quantity (bel)\000\047\240length (yard)\000\
\047\241length (parsec)\000\047\242length (inch)\000\
\047\243length (foot)\000\047\244length (mile)\000\
\047\245pressure pound-force/inch2\000\047\246velocity (km/hour)\000\
\047\247velocity (mile/hour)\000\047\250ang velocity (rev/min)\000\
\047\251energy (g calorie)\000\047\252energy (kg calorie)\000\
\047\253energy (kW hour)\000\047\254Temperature (deg F)\000\
\047\255percentage\000\047\256per mille\000\
\047\257period (beats/min)\000\047\260electric charge (amp hours)\000\
\047\261mass density (mg/decilitre)\000\047\262mass dens (millimole/litre)\000\
\047\263time (year)\000\047\264time (month)\000\
\047\265concentration (count/m3)\000\047\266irradiance (W/m2)\000\
\047\267milliliter (per kg/min)\000\047\270mass (pound)\000\
\047\271metabolic equivalent\000\047\272step (per min)\000\
\047\274stroke (per min)\000\047\275pace (km/min)\000\
\047\276luminous efficacy (lumen/W)\000\047\277luminous energy (lumen hour)\000\
\047\300luminous exposure (lux hour)\000\047\301mass flow (g/s)\000\
\047\302volume flow (litre/s)\000\047\303sound pressure (db)\000\
\047\304parts per million\000\047\305parts per billion\000\
\050\000Primary Service\000\050\001Secondary Service\000\
\050\002Include\000\050\003Characteristic\000\
\051\000Char'tic Extended Properties\000\051\001Char'tic User Description\000\
\051\002Client Char'tic Config\000\051\003Server Char'tic Config\000\
\051\004Char'tic Presentation Format\000\051\005Char'tic Aggregate Format\000\
\051\006Valid Range\000\051\007External Report Reference\000\
\051\010Report Reference\000\051\011Number of Digitals\000\
\051\012Value Trigger Setting\000\051\013Environment Sense Config\000\
\051\014Environment Sense Measure\000\051\015Envirmnt Sense Trig Setting\000\
\051\016Time Trigger Setting\000\051\017BR-EDR Transport Block Data\000\000\000" };

/**************** HCI error codes returned by Event 0F *****************/

char *error0f[70] = {
"",
"Unknown HCI Command",
"Unknown Connection Identifier",
"Hardware Failure",
"Page Timeout",
"Authentication Failure",
"PIN or Key Missing",
"Memory Capacity Exceeded",
"Connection Timeout",
"Connection Limit Exceeded",
"Synchronous Connection Limit To A Device Exceeded",
"Connection Already Exists",
"Command Disallowed",
"Connection Rejected due to Limited Resources",
"Connection Rejected Due To Security Reasons",
"Connection Rejected due to Unacceptable BD_ADDR",
"Connection Accept Timeout Exceeded",
"Unsupported Feature or Parameter Value",
"Invalid HCI Command Parameters",
"Remote User Terminated Connection",
"Remote Device Terminated Connection due to Low Resources",
"Remote Device Terminated Connection due to Power Off",
"Connection Terminated By Local Host",
"Repeated Attempts",
"Pairing Not Allowed",
"Unknown LMP PDU",
"Unsupported Remote/LMP Feature",
"SCO Offset Rejected",
"SCO Interval Rejected",
"SCO Air Mode Rejected",
"Invalid LMP/LL Parameters",
"Unspecified Error",
"Unsupported LMP/LL Parameter Value",
"Role Change Not Allowed",
"LMP/LL Response Timeout",
"LMP/LL Collision",
"LMP PDU Not Allowed",
"Encryption Mode Not Acceptable",
"Link Key cannot be Changed",
"Requested QoS Not Supported",
"Instant Passed",
"Pairing With Unit Key Not Supported",
"Different Transaction Collision",
"Reserved for future use",
"QoS Unacceptable Parameter",
"QoS Rejected",
"Channel Classification Not Supported",
"Insufficient Security",
"Parameter Out Of Mandatory Range",
"Reserved for future use",
"Role Switch Pending",
"Reserved for future use",
"Reserved Slot Violation",
"Role Switch Failed",
"Extended Inquiry Response Too Large",
"Secure Simple Pairing Not Supported By Host",
"Host Busy - Pairing",
"No Suitable Channel Found",
"Controller Busy",
"Unacceptable Connection Parameters",
"Advertising Timeout",
"Connection Terminated due to MIC Failure",
"Connection Failed / Synchronization Timeout",
"MAC Connection Failed",
"Coarse Clock Adjustment Rejected",
"Type0 Submap Not Defined",
"Unknown Advertising Identifier",
"Limit Reached",
"Operation Cancelled by Host",
"Packet Too Long"
};   // last = [69]


/*********** LE read/write chaaracteristic error codes ********/

char *errorle[20] = {
"",
"Invalid Handle",
"Read Not Permitted",
"Write Not Permitted",
"Invalid PDU",
"No Authentication (need passkey security)",
"Request Not Supported",
"Invalid Offset",
"Insufficient Authorization",
"Prepare Queue Full",
"Attribute Not Found",
"Attribute Not Long - no read blob",
"Insufficient Encryption Key Size",
"Invalid Attribute Value Length",
"Unlikely Error",
"Insufficient Encryption",
"Unsupported Group Type",
"Insufficient Resources",
"Database Out Of Sync",
"Value Not Allowed"
};  // last = [19]

char *errorcry[16] = {
"",
"Passkey entry failed",
"OOB not available",
"Authentication not possible",
"Confirm value fail",
"Pairing not supported",
"Key size",
"Command not supported",
"Unspecified",
"Repeated attempts",
"Invalid parameters",
"DHKey check fail",
"Numeric comparison failed",
"BR/EDR pair in progress",
"Cross transport not allowed",
"Key rejected"
};  // last = [15]


/******** LE advertising data type codes *****************/

char *adlist[48] = {
"Unknown",
"Flags",
"Some 2-byte UUIDs",
"All 2-byte UUIDs",
"Some 4-byte UUIDs",
"All 4-byte UUIDs",
"Some 16-byte UUIDs",
"All 16-byte UUIDs",
"Name",
"Name",
"Tx Power Level",
"Unknown",
"Unknown",
"Class of Device",
"Simple Pairing Hash",
"Simple Pairing Randomizer",
"Device ID",
"Security Manager Out of Band Flags",
"Slave Connect Interval Range",
"Unknown",
"2-byte Service Solicit UUIDs",
"16-byte Service Solicit UUIDs",
"Service Data",
"Public Target Address",
"Random Target Address",
"Appearance",
"Advertising Interval",
"LE Bluetooth Device Address",
"LE Role",
"Simple Pairing Hash",
"Simple Pairing Randomizer",
"32-bit Service Solicit UUIDs",
"Service Data 4-byte UUIDs",
"Service Data 16-byte UUIDs",
"LE Secure Connect Confirm Value",
"LE Secure Connect Random Value",
"URI Bluetooth",
"Indoor Positioning",
"Transport Discovery Data",
"LE Supported Features",
"Channel Map Update Indication",
"PB-ADV Mesh Profile",
"Mesh Message Mesh Profile",
"Mesh Beacon Mesh Profile",
"BIGInfo",
"Broadcast Code",  // 2D=[45]
"3D Info",         // 3D
"Manuf specific"   // FF
};


/************* END TEXT ********************/

  
/***************************** FUNCTIONS ************************/


int init_blue(char *filename)
  {
  return(init_blue_ex(filename,0));  // hci0
  }

int init_blue_ex(char *filename,int hcin)
  {
  int n,dn,k,sn,hn,i,len,flag,errflag,errcount,psnx;
  int clflag,leflag,readret,meshcount,reportflag,localflag;
  unsigned int ind[16];
  struct cticdata *cp;
  char s[256],buf[128],*es,*prs;
  unsigned char *data;
  FILE *stream;
  static char errs[16] = {"   ERROR **** "};
  static int initflag = 0;    

  
  check_init(1);
  
  if(checkfilename("init_blue",filename) == 0)
    return(0);
    
#if (!defined(BTFWINDOWS) && !defined(PICOSTACK))                       
  if(initflag != 0)
    {
    prs = "Init_blue called twice\n";
    printn(prs,strlen(prs));
    return(0);
    }
#endif    
 
  prs = "Initialising...\n";
  printn(prs,strlen(prs));
  // printf("Initialising...\n");


   // global 
  gpar.debug = 0;
  gpar.serveractive = 0; 
  gpar.timout = 1000;   // reply wait ms time out
  gpar.toshort = 5;    // ms
  gpar.cmdcount = 0;
  gpar.lecap = 0;
  gpar.lastsend = 0;
  gpar.meshflag = 0;    // le advertising off
  gpar.readerror = 0;
  gpar.ledatlenflag = 0;
  gpar.leclientwait = 750;
  gpar.leintervalmin = 0x18;
  gpar.leintervalmax = 0x28;  
  gpar.prtp = 0;   // current end of buffer
  gpar.dump = PRBUFSZ-PRLINESZ;  // dump destination
  gpar.dumpn = 0;  // char count
  gpar.prtp0 = 0;  // start next print 
  gpar.prtw = 0;   // wrap index
  gpar.prts = 0;   // start of circular buffer
  gpar.prte = 0;   // end of print for scroll
  gpar.screenoff = 0;
  gpar.printflag = PRINT_NORMAL;
  gpar.prtv = &gpar.dump;
  gpar.prtvn = &gpar.dumpn;
  gpar.dhkeydev = 0;
  gpar.randomfd = -1;
  gpar.hidflag = 0;
  gpar.settings = ENABLE_OBEX;
  gpar.keytocb = 0;
  gpar.keyboard = 0;
  gpar.exitchar = 'x';
  gpar.notifynode = 0;
  gpar.lecallback = NULL;
  getdatfile(gpar.datfile);
  for(n = 0 ; n < 8 ; ++n)
    gpar.rand[n] = s[n];        
  gpar.hci = -1;
 
  time_ms();
 
  if(initflag == 0)
    {
    gpar.s = (char*)calloc(PRBUFSZ+4,1);
    instack = (unsigned char*)malloc(INSTACKSIZE); 
  
    if(gpar.s == NULL || instack == NULL)
      {
      prs = "Memory allocate fail\n";
      printn(prs,strlen(prs));
      return(0);
      }
    // zero entries  n=first undefined
    for(k = 0 ; k < NUMDEVS ; ++k)
      dev[k] = NULL;
    }
  else
    {  // Windows restart
    for(k = 0 ; k < NUMDEVS ; ++k)
      {
      // free devdata
      if(dev[k] != NULL)
        {
        dev[k]->type = 0; 
        // free ctics
        cp = dev[k]->ctic;
        dn = 0;
        while(dn < 256 && cp->type != CTIC_END)
          {
          cp->type = CTIC_UNUSED;
          cp = cp->nextctic;
          ++dn;
          }
        }
      }
    }
    
  initflag = 1;
     
  for(n = 0 ; n < INSTACKSIZE ; ++n)
    instack[n] = INS_FREE;

  insdat = instack + INSHEADSIZE;     
  
  gpar.maxpage = 0;
  gpar.devid = hcin;  
  gpar.blockflag = 0;

  register_serial(strtohex("FCF05AFD-67D8-4F41-83F5-7BEE22C03CDB",NULL),"My custom serial");
  
  dn = devalloc(NULL,0);
  if(dn != 0)
    return(0);
     
  dev[0]->type = BTYPE_LO;
  dev[0]->meshindex = 1;  // first message index
  dev[0]->node = 0;   // node not found    
  strcpy(dev[0]->name,"not in devices.txt");
  
  clearins(0);   // initialise BT packet input stack 
   
  if(inithci() == 0)
    return(0);
     
  errcount = 0;
  meshcount = 0;
  gpar.lecap = 0;
  gpar.sccap = 0;
  flag = 0;
  VPRINT "Read local supported commands\n");
  sendhci(locsup,0);
  do
    {
    readhci(0,IN_STATOK,0,gpar.timout,gpar.toshort);
    n = findhci(IN_STATOK,0,INS_POP);
    if(n >= 0 && insdatn[1] == locsup[PAKHEADSIZE+1] && insdatn[2] == locsup[PAKHEADSIZE+2])
      {   // octet 0 = insdatn[4]
      flag = 1;
      if((insdatn[29] & 0xA2) == 0xA2 && (insdatn[30] & 0x3E) == 0x3E)
        gpar.lecap = 1;  // LE capable
      if((insdatn[38] & 0x06) == 0x06)
        gpar.sccap = 1;  // Secure con p256key+dhkey
      if((insdatn[31] & 0x80) != 0)
        {
        sendhci(lerand,0);
        statusok(0,lerand);
        }
      }
    }
  while(n >= 0 && flag == 0);
  
  srand(*((unsigned int*)(gpar.rand)));  
  VPRINT "Read local board address\n");
  flushprint();
  sendhci(locbadd,0);
  readhci(0,IN_BADD,0,gpar.timout,gpar.toshort);
  n = findhci(IN_BADD,0,INS_POP);
  if(n >= 0)
    {
    if(insdatn[3] == 0)  // status OK
      {  // got local board address
      data = dev[0]->baddr;
      for(k = 0 ; k < 6 ; ++k)
        data[5-k] = insdatn[4+k];
      leadvert[PAKHEADSIZE+7] =  (data[5] ^ data[4]) ^ data[3];
      leadvert[PAKHEADSIZE+8] = ((data[2] ^ data[1]) ^ data[0]) | 0xC0;   
      }
    popins();
    }
  else
    {
    NPRINT "Unable to read local board address\n");      
    NPRINT "Bluetooth may be locked - reboot\n");
    errcount = 1000;
    }

  if(gpar.lecap != 0)
    {
    // read LE buffer size for mesh node read/write
    gpar.lebufsize = 0; 
    sendhci(lebufsz,0);
    flag = 0;
    do
      {
      readhci(0,IN_STATOK,0,500,0);
      n = findhci(IN_STATOK,0,INS_POP);
      if(n >= 0 && insdatn[1] == lebufsz[PAKHEADSIZE+1] && insdatn[2] == lebufsz[PAKHEADSIZE+2])
        {
        flag = 1;
        if(insdatn[3] == 0)
          gpar.lebufsize = (insdatn[4] + (insdatn[5] << 8)) * insdatn[6];
        popins();
        }
      }
    while(n >= 0 && flag == 0);
    }
   
#ifdef PICOSTACK
  packet_size(220);
  stream = NULL;
  readret = readline(NULL,filename,s);
#else     
  stream = fopen(filename,"r");
  if(stream == NULL)
    {
    NPRINT "Unable to open devices info file %s\n",filename);
    readret = 0;
    }
  else
    readret = readline(stream,NULL,s);  // read first line
  
  NPRINT "Device data from %s file\n",filename);
  flushprint();
#endif
    
  for(n = 0 ; n < 32 ; ++n)
    {
    pserv[n].handle = -1;
    pserv[n].eog = 0xFFFF;
    }
  pserv[0].handle = 0;
  pserv[0].eog = 0;
  pserv[0].uuidtype = 16;
  for(i = 0 ; i < 16 ; ++i)
    pserv[0].uuid[i] = baseuuid[i];
  psnx = -1;    
  reportflag = 0;
  localflag = 0;
     
  while(readret > 0 && errcount == 0)
    {
    errflag = 0;
        

    ind[0] = strinstr(s,"DEVICE");
    ind[1] = strinstr(s,"TYPE");
    ind[2] = strinstr(s,"ADDRESS");
    ind[3] = strinstr(s,"PIN");
    ind[4] = strinstr(s,"LECHAR");
    ind[5] = strinstr(s,"HANDLE");
    ind[6] = strinstr(s,"UUID");
    ind[7] = strinstr(s,"SIZE");
    ind[8] = strinstr(s,"PERMIT");
    ind[9] = strinstr(s,"NODE");
    ind[10] = strinstr(s,"CHANNEL");
    ind[11] = strinstr(s,"RANDOM");
    ind[12] = strinstr(s,"PRIMARY_SERVICE");
    ind[13] = strlen(s) << 16;
    ind[14] = 0x80000000;  // terminate flag
      
    if(ind[4] != 0)
      NPRINT "  %s\n",s);
    else  
      NPRINT "%s\n",s);
    flushprint();
 
    flag = 0;   // type of line not determined
                // 1=device
                // 2=le characteristic

    cp = &cticnull;  // for safety
    
    clflag = ind[0] + ind[1] + ind[2] + ind[3] + ind[9] + ind[10] + ind[11];
    leflag = ind[4] + ind[5] + ind[6] + ind[7] + ind[8];
   
    es = NULL;
     
    if(ind[0] == 0 && ind[4] == 0 && ind[12] == 0)
      es = "Must start with DEVICE= or LECHAR= or PRIMARY_SERVICE=";
    else if(clflag != 0 && leflag != 0) 
      es = "DEVICE and LECHAR values on same line";  
    else if(leflag != 0 && dev[dn]->type == BTYPE_CL)
      es = "LECHAR not allowed with classic";     
    else if(ind[0] != 0 && ind[9] == 0)
      es = "Missing NODE";
    else if(clflag != 0)
      {
      if(ind[1] == 0 || ind[2] == 0)
        es = "Missing TYPE or ADDRESS";
      else
        {
        dn = devalloc(NULL,0);
        if(dn < 0)
          return(0);  // fatal alloc error
        flag = 1;
        }
      }
    else if(leflag != 0) 
      {
      // ind[5] == 0 && ind[6] == 0  no handle/UUID
      cp = cticalloc(dn); // return ctic pointer - may be to cticnull=failed
      if(cp->type != CTIC_UNUSED)
        return(0);   // fatal alloc error
      flag = 2;  // characteristic entry
      }
    else
      flag = 3;  // primary service
     
    if(es != NULL)
      {
      NPRINT "%s%s\n",errs,es);
      ++errcount;
      } 
        
      
    if(flag != 0)
      {
      if(ind[0] != 0)
        {    
        // DEVICE - copy name      
        len = entrylen(ind,0);
        i = 0;
        sn = ind[0] & 0xFFFF;
        while(i < NAMELEN-1 && i < len && s[sn+i] != 0)
          {
          dev[dn]->name[i] = s[sn+i];
          ++i;
          }
        dev[dn]->name[i] = 0;
        --i;
        while(i > 1 && dev[dn]->name[i] == ' ')
          {
          dev[dn]->name[i] = 0;
          --i;
          }
        psnx = -1;
        }  // end ind[0] DEVICE
        
      if(ind[1] != 0)
        {  // TYPE  
        sn = ind[1] & 0xFFFF;
        if(strcomp(s+sn,"CLASSIC",7) == 0)
          dev[dn]->type = BTYPE_CL;
        else if(strcomp(s+sn,"LE",2) == 0)   
          dev[dn]->type = BTYPE_LE;
        else if(strcomp(s+sn,"MESH",4) == 0)   
          dev[dn]->type = BTYPE_ME;
        else
          {
          NPRINT "%sTYPE must be CLASSIC/LE/MESH\n",errs);
          errflag = 1;
          }
        }  // end ind[1] TYPE
                      
      if(ind[2] != 0)
        {  // ADDRESS 
        len = entrylen(ind,2);
        sn = ind[2] & 0xFFFF;
        
        if(strcomp(s+sn,"MATCH_NAME",10) == 0)
          dev[dn]->matchname = 1;
        else if(strcomp(s+sn,"LOCAL",5) == 0)
          {
          localflag = 1;
          for(i = 0 ; i < 6 ; ++i)
            dev[dn]->baddr[i] = dev[0]->baddr[i];
          }
        else
          {
          dev[dn]->matchname = 0;
          data = strtohexx(s + sn,len,&hn);
          if(hn != 6)
            {
            NPRINT "%sDevice address must be 6 bytes\n",errs);
            errflag = 1;
            }  
          else
            {
            for(i = 0 ; i < 6 ; ++i)
              dev[dn]->baddr[i] = data[i];
            }
          }        
        }  // end ind[2] ADDRESS
      
      if(ind[3] != 0)
        {   // PIN 
        sn = ind[3] & 0xFFFF;
        i = 0;
        while(i < 15 && s[sn+i] != 0 && s[sn+i] != ' ')
          {
          dev[dn]->pincode[i] = s[sn+i];
          ++i;
          }     
        dev[dn]->pincode[i] = 0;   
        }  // end ind[3] PIN
        
      if(ind[4] != 0)
        {  // LECHAR - name
        len = entrylen(ind,4);
        sn = ind[4] & 0xFFFF;
        i = 0;
        while(i < NAMELEN-1 && i < len && s[sn+i] != 0)
          {
          cp->name[i] = s[sn+i];
          ++i;
          }
        cp->name[i] = 0;
        --i;
        while(i > 1 && cp->name[i] == ' ')
          {
          cp->name[i] = 0;
          --i;
          }
                  
        if(psnx < 0)
          psnx = 0;  // lechar before PS  
        cp->psnx = psnx;
        }  // end ind[4] LECHAR
        
        
      if(ind[5] != 0)
        { // HANDLE
        len = entrylen(ind,5);
        sn = ind[5] & 0xFFFF;
        data = strtohexx(s + sn,len,&hn);
        if(!(hn == 1 || hn == 2))
          {
          NPRINT "%sHANDLE must be 1 or 2 bytes\n",errs);
          errflag = 1;
          }
         else
          { 
          cp->chandle = 0;
          for(i = 0 ; i < hn ; ++i)
            cp->chandle = (cp->chandle << 8) + data[i];          
          }
        }  // end ind[5] HANDLE

      if(ind[6] != 0)
        {  // UUID 
        len = entrylen(ind,6);
        sn = ind[6] & 0xFFFF;
        data = strtohexx(s + sn,len,&hn);
        if(!(hn == 2 || hn == 16))
          {
          NPRINT "%sUUID must be 2 or 16 bytes\n",errs); 
          errflag = 1;
          }
        else
          {
          cp->uuidtype = hn;
          for(i = 0 ; i < hn ; ++i)
            cp->uuid[i] = data[i];
          if(hn == 2 && data[0] == 0x2A && data[1] == 0x4D)
            {
            ++reportflag;
            cp->reportflag = reportflag;
              // Report Reference descriptor ID for Report characteristic
              // reportflag=1 for 1st Report, 2 for 2nd...
              // Set Report ID (85 XX entry in Report Map)
            cp->reportid = reportflag;  // assumes Report ID = 1,2,3... 
            }
          }               
        }  // end ind[6] UUID
      
        
      if(ind[7] != 0)
        { // SIZE              
        len = entrylen(ind,7);
        sn = ind[7] & 0xFFFF;
        for(i = 0 ; i < len ; ++i)
          buf[i] = s[sn+i];
        buf[len] = 0;
        cp->size = atoi(buf);
        cp->origsize = cp->size;
        if(cp->size < 1 || cp->size > LEDATLEN)
          {
          NPRINT "%sSIZE must be 1-%d\n",errs,LEDATLEN);
          errflag = 1;
          }
        }  // end ind[7] SIZE
      else if(flag == 2)
        cp->size = 0;      


      if(ind[8] != 0)
        {  //PERMIT; 
        len = entrylen(ind,8);
        sn = ind[8] & 0xFFFF;
        data = strtohexx(s + sn,len,&hn);
        if(hn != 1)
          {
          NPRINT "%sPERMIT must be 1 hex byte\n",errs);
          errflag = 1;
          }
        else if((data[0] & 0x30) == 0x30)
          {
          NPRINT "%sPERMIT notify and indicate enabled\n",errs);
          errflag = 1;
          }
        else 
          cp->perm = data[0];               
        }  // end ind[8] PERMIT

      if(ind[9] != 0)
        {  //NODE 
        len = entrylen(ind,9);
        sn = ind[9] & 0xFFFF;       
        for(i = 0 ; i < len ; ++i)
          buf[i] = s[sn+i];
        buf[len] = 0;
        dev[dn]->node = atoi(buf);
          // check no repeat
        es = NULL;
        if(dev[dn]->node <= 0 || dev[dn]->node >= 0x10000)
          es = "Invalid NODE number";
        for(i = 0 ; i < dn-1 && es == NULL ; ++i)
          {
          if(dev[i]->node == dev[dn]->node)
            es = "Repeat NODE number";
          }
        if(es != NULL)
          { 
          NPRINT "%s%s %d\n",errs,es,dev[dn]->node);
          errflag = 1;
          }
        }  // end ind[9] NODE

      if(ind[10] != 0)
        {  //CHANNEL 
        len = entrylen(ind,10);
        sn = ind[10] & 0xFFFF;       
        for(i = 0 ; i < len ; ++i)
          buf[i] = s[sn+i];
        buf[len] = 0;
        dev[dn]->rfchan = atoi(buf);
        }  // end ind[10] CHANNEL


      if(ind[11] != 0)
        {  // RANDOM = UNCHANGED
        sn = ind[11] & 0xFFFF;
        if(strcomp(s+sn,"UNCHANGED",9) == 0)
          dev[dn]->leaddtype = 1;  // random
        else
          {
          NPRINT "%sExpecting RANDOM = UNCHANGED\n",errs);
          errflag = 1;
          }
        }
     
      if(dn == 0 && ind[12] != 0)
        {  // PRIMARY_SERVICE
        len = entrylen(ind,12);
        sn = ind[12] & 0xFFFF;
        data = strtohexx(s + sn,len,&hn);
        if(hn != 16 && hn != 2)
          {
          NPRINT "%sSERVICE UUID must be 2 or 16 bytes\n",errs); 
          errflag = 1;
          }
        else
          {
          if(hn == 2 && data[0] == 0x18 && data[1] == 0x12)
            {
            gpar.hidflag = 1;
            }
          if(errflag == 0 && psnx < 30)
            {
            if(psnx < 0)
              psnx = 0;
            else
             ++psnx;             
            pserv[psnx].handle = 0;
            pserv[psnx].uuidtype = hn;
            for(i = 0 ; i < hn ; ++i)
              pserv[psnx].uuid[i] = data[i];
            }
          else
            {
            NPRINT "%sToo many PRIMARY_SERVICEs\n",errs);
            errflag = 1;
            }           
          }                      
        }

      if(errflag != 0)
        ++errcount;

      // check if local device / mesh
      if(flag == 1)
        {
        if(devnfrombadd(dev[dn]->baddr,BTYPE_XALL,DIRN_FOR) == 0)
          {   // is device 0 board address local - move to dn=0
          if(dev[dn]->type == BTYPE_ME)
            {
            dev[0]->node = dev[dn]->node;
            strcpy(dev[0]->name,dev[dn]->name);
            }
          dev[dn]->type = 0;    // free dn
          dn = 0;
          }             // but not a fatal error
        else if(dev[dn]->type == BTYPE_ME)
          ++meshcount;
        }   
            
      if(flag == 1)
        {
        if(errflag != 0)
          dev[dn]->type = 0;  // free
        }
      else if(flag == 2 && cp->type != CTIC_END)
        {
        if(errflag == 0)
          cp->type = CTIC_ACTIVE;  // used
        else
          cp->type = CTIC_UNUSED;  // free
        }  
      }     
    flushprint();
    
    if(readret == 2)
      readret = 0;   // was last line - exit
    else
      readret = readline(stream,filename,s);  // read next line
    }  // end read file line loop
    
  if(stream != NULL)
    fclose(stream);
   
  if(errcount != 0)
    {
    closehci();
    prs = "\n************ initblue() FAILED ************\n";
    printn(prs,strlen(prs));
    return(0);
    }

  if(localflag != 0)
    {
    NPRINT "\nThis local device %s has been allocated NODE=%d\n",dev[0]->name,dev[0]->node);
    NPRINT "from the devices file. ADDRESS=%s\n",baddstr(dev[0]->baddr,0));
    }
  if(dev[0]->node == 0)
    {
    dev[0]->node = newnode();
    sprintf(buf,"Node %d",dev[0]->node);
    strcpy(dev[0]->name,buf);    
    NPRINT "\nThis local device has been allocated NODE = %d\n",dev[0]->node);
    NPRINT "It should be added to the %s file as follows:\n",filename);
    NPRINT "DEVICE=name (e.g. My Pi) TYPE=MESH  NODE=choose (e.g. 1)  ADDRESS=%s\n",baddstr(dev[0]->baddr,0));
    }
  else
    {  // write local name
    for(n = 0 ; n < 8 ; ++n)
      s[n] = wln[n];
    for(n = 8 ; n < 256 ; ++n)
      s[n] = 0;
    strcpy(s+8,dev[0]->name);
    sendhci((unsigned char*)s,0);
    }
      
  if(localctics() == 0)
    ++errcount;    

  flushprint();

  if(errcount != 0)
    {    
    prs = "\n************ initblue() FAILED ************\n";
    printn(prs,strlen(prs));
    closehci();            
    return(0);
    }

  rwlinkey(0,0,NULL);
  atexit(close_all);


  if(gpar.lecap == 0)
    {
    NPRINT "\n*** Bluetooth adapter is not LE capable ***\n");
    NPRINT "*** Mesh/LE functions will not work *******\n");
    }
  else
    {
    getrand(gpar.randbadd,6);
    gpar.randbadd[0] |=  0xC0;

    for(n = 0 ; n < 6 ; ++n)
      lerandadd[13-n] = gpar.randbadd[n];
      
    VPRINT "Set LE random address\n");
    sendhci(lerandadd,0);
    statusok(0,lerandadd);

  
    VPRINT "Set LE advertising parameters\n");
    if(gpar.hidflag == 0)
      {
      dev[0]->leaddtype = 0;
      sendhci(leadparam,0);
      statusok(0,leadparam);
      VPRINT "Set LE advertising data with device name\n");
      addname(0);
      sendhci(leadvert,0);  // reset mesh packet index = 0
      statusok(0,leadvert);
      }
    else
      {
      dev[0]->leaddtype = 1;
      sendhci(leadparamx,0);
      statusok(0,leadparamx);
      NPRINT "Advertise as LE HID device\n");
      sendhci(hidadvert,0);
      statusok(0,hidadvert);
      }
    if(gpar.sccap == 0)
      {
      NPRINT "Legacy LE pair only\n");
      }
    else
      {
      VPRINT "Get public keys\n");
      sendhci(keypair,0);
      readhci(0,IN_AUTOEND,0,2000,0);
      n = findhci(IN_AUTOEND,0,INS_POP);
      }
    }

  flushprint();
  check_init(2);
  return(1);     
  }

int check_init(int flag)
  {
  static int initdone = 0;
  static int printdone = 0;
  static int closedone = 0;
  char *prs;
  
  if(flag == 0)
    {
    if(initdone == 0 && printdone == 0)
      {
      prs = "*** ERROR *** init_blue not done\n";
      printn(prs,strlen(prs));
      printdone = 1;
      }
    return(initdone);
    }
  if(flag == 1)
    {
    initdone = 0;
    printdone = 0;
    closedone = 0;
    }
  else if(flag == 2)
    {
    initdone = 1;
    printdone = 0;
    closedone = 0;
    }
  else if(flag == 3)
    closedone = 1;
  else if(flag == 4 && initdone != 0 && closedone == 0)
    {
    close_all();
    closedone = 1;
    }
  else if(flag == 5)
    return(closedone);
  return(0);
  }

char *cticerrs(struct cticdata * cp)
  {
  static char errs[128];
    
  sprintf(errs,"\n  ERROR *** Local node %d LE characteristic %s\n      ",dev[0]->node,cp->name); 
  return(errs);
  }
  

int localctics()
  {
  int n,k,j,uuidn,handle,flag,min,max,psn;
  struct cticdata *cp,*cpx,*lastcp;
  char *errs;
  
  
  for(j = 0 ; devok(j) != 0 ; ++j)
    {
    //if(dev[j]->matchname != 0 && dev[j]->type == BTYPE_ME)
    //  {
    //  NPRINT "\n  ERROR *** %s Node %d - MATCH_NAME not allowed for TYPE=MESH\n",dev[j]->name,dev[j]->node);
    //  return(0);
    //  }
    
    
    for(n = 0 ; ctic(j,n)->type == CTIC_ACTIVE ; ++n)
      {
      cp = ctic(j,n);
      if(cp->chandle == 0 && cp->uuidtype == 0)
        {
        NPRINT "\n  ERROR *** %s Node %d LECHAR=%s - No handle or UUID\n",dev[j]->name,dev[j]->node,cp->name);
        return(0);
        }
      if(cp->size > LEDATLEN)
        {
        cp->size = LEDATLEN;
        cp->origsize = LEDATLEN;
        }
      }  
    }
    
  // find primary services
  psn = -1;
  lastcp = NULL;
  for(n = 0 ; ctic(0,n)->type == CTIC_ACTIVE ; ++n)
    {
    cp = ctic(0,n);
    if(cp->psnx != psn)
      {
      psn = cp->psnx;
      if(lastcp != NULL)
        {
        lastcp->psnx |= 0x20000;
        }
      cp->psnx |= 0x10000;
      }
    lastcp = cp;  
    if(psn > 2 && cp->chandle != 0)
      {
      NPRINT "%sDo not specify HANDLEs for these services\n",cticerrs(cp));
      NPRINT "      Specify UUIDs and let the system set the handles\n");
      return(0);
      }
    }  

  if(lastcp != NULL)
    {
    lastcp->psnx |= 0x40000;
    }
      
  // check handles
  handle = 4;  // 4
  for(n = 0 ; ctic(0,n)->type == CTIC_ACTIVE ; ++n)
    {
    cp = ctic(0,n);
    errs = cticerrs(cp);

    if(cp->chandle != 0)
      {
      if(cp->chandle < 5)
        {
        NPRINT "%shandle must be 5 or greater\n",errs);
        return(0);
        }
   
      if(cp->chandle > handle)
        handle = cp->chandle;
        
      for(j = 0 ; ctic(0,j)->type == CTIC_ACTIVE ; ++j)
        {
        cpx = ctic(0,j);
        if(cpx->chandle != 0 && j != n)
          {
          min = cpx->chandle-1;
          max = min + 2;
          if((cp->perm & 0x30) != 0)
            --min;
          if((cpx->perm & 0x30) != 0)
            ++max;          
          if(cp->chandle >= min && cp->chandle <= max)
            {
            NPRINT "%shandle %04X interferes with handles used by %s\n",errs,cp->chandle,cpx->name);
            return(0);
            }
          }
        }
      }
    }  

  uuidn = 0;
  
  
  for(n = 0 ; ctic(0,n)->type == CTIC_ACTIVE ; ++n)
    {
    cp = ctic(0,n);
    errs = cticerrs(cp);
   
    if(cp->chandle == 0)
      {  // allocate handle
      do
        {   // check duplicate handles
        ++handle;
        flag = 0;
        for(j = 0 ; ctic(0,j)->type == CTIC_ACTIVE && flag == 0 ; ++j)
          {
          cpx = ctic(0,j);
          if(cpx->chandle != 0 && j != n)
            {
            min = cpx->chandle-1;
            max = min + 2;
            if((cp->perm & 0x30) != 0)
              {
              --min;
              if(cp->reportflag != 0)
                min -= 2;
              }
            if((cpx->psnx & 0x10000) != 0)
              --min;  // PS
            if((cpx->perm & 0x30) != 0)
              {
              ++max;
              if(cpx->reportflag != 0)
                max += 2;
              }
            if((cp->psnx & 0x10000) != 0)
              ++max;          

            if(handle >= min && handle <= max)
              {
              flag = 1;  // fail
              handle = max;
              }
            }
          }
        }
      while(flag != 0);
    
      j = cp->psnx & 31;

      if(handle == 5 && j > 0)
        {  // initial empty PS
        for(k = 0 ; k < j ; ++k)
          {
          pserv[k].handle = k+1;
          pserv[k].eog = k+1;
          if(k > 1)
            ++handle;
          }
        }

      if((cp->psnx & 0x10000) != 0)
        {      
        for(k = 0 ; k < j ; ++k)
          {
          if(pserv[k].handle == 0)
            {
            pserv[k].handle = handle-2;
            pserv[k].eog = handle-2;
            ++handle;
            }
          }
        }

      cp->chandle = handle;
     
      if((cp->psnx & 0x10000) != 0)
        {
        pserv[j].handle = handle-2;
        pserv[j].eog = handle-2;
        }
      if((cp->psnx & 0x20000) != 0)
        {
        pserv[j].eog = handle;
        if((cp->perm & 0x30) != 0)
          {
          ++pserv[j].eog;
          if(cp->reportflag != 0)
            pserv[j].eog += 2;
          }
        } 
      }  

    if(cp->uuidtype == 0)
      {  // allocate UUID
      cp->uuidtype = 16;
      for(k = 0 ; k < 11 ; ++k)
        cp->uuid[k] = baseuuid[k];
      cp->uuid[15] = n & 0xFF;
      cp->uuid[14] = (n >> 8) & 0xFF;
      cp->uuid[13] = cp->chandle & 0xFF;
      cp->uuid[12] = (cp->chandle >> 8) & 0xFF;
      uuidn = 0xCC-1; 
      do
        {
        ++uuidn;
        cp->uuid[11] = uuidn & 0xFF;
        flag = 0;
        for(j = 0 ; ctic(0,j)->type == CTIC_ACTIVE && flag == 0 ; ++j)
          {
          cpx = ctic(0,j);
          if(j != n && cpx->uuidtype == 16)
            flag = bincmp(cpx->uuid,cp->uuid,16,DIRN_FOR);
          }
        }
      while(flag != 0 && uuidn < 0x1CC);
      }
    else if(cp->uuidtype == 16 && bincmp(cp->uuid,baseuuid,16,DIRN_FOR) != 0)
      {
      NPRINT "%sUUID not allowed\n",errs);
      return(0);
      }
    
    if(cp->size < 1 || cp->size > LEDATLEN)
      {
      NPRINT "%ssize must be 1-%d\n",errs,LEDATLEN);
      return(0);
      }


    if(cp->uuidtype == 2 && cp->uuid[0] == 0x2A && cp->uuid[1] == 0)
      { // device name
      j = 0;
      while(dev[0]->name[j] != 0 && j < LEDATLEN-1)
        {
        cp->value[j] = dev[0]->name[j];
        ++j;
        }
      for(k = j ; k < LEDATLEN ; ++k)
        cp->value[k] = 0;
      cp->size = j;
      }
     
    if(cp->uuidtype == 2 && cp->uuid[0] == 0x2A && cp->uuid[1] == 0x05)
      {
      NPRINT "Service changed 2A05 enabled\n");
      cp->notify = 1;  // enable service changed notify
      }
    }


  for(n = 0 ; ctic(0,n)->type == CTIC_ACTIVE ; ++n)
    {
    cp = ctic(0,n);
    if((cp->psnx & 0x40000) != 0)
      pserv[cp->psnx & 31].eog = 0xFFFF;
    }
    
  return(1);
  }


int entrylen(unsigned int *ind,int in)
  {
  int n,minlen,sn0,len;
  
  minlen = 8192;
  sn0 = ind[in] & 0xFFFF;  // index of end
  for(n = 0 ; (ind[n] & 0x80000000) == 0 ; ++n)
    {
    if(n != in && ind[n] != 0)
      {    // len = start of ind[n] - end of ind[in]
      len = ((ind[n] >> 16) & 0xFFFF) - sn0;
      if(len > 0 && len < minlen)
        minlen = len; 
      }   
    }
  
  return(minlen);
  } 


char *baddstr(unsigned char *badd,int dirn)
  {
  int n,k;
  static char s[20];
   
  for(n = 0 ; n < 6 ; ++n)
    {
    if(dirn == 0)
      k = n;
    else
      k = 5-n;
    sprintf(s+3*n,"%02X:",badd[k]);
    }
  s[17] = 0;
  return(s);
  }  

char *device_address(int node)
  {
  int ndevice;

  if(check_init(0) == 0)
    return("");
  
  if(node == 0)
    ndevice = 0;
  else
    ndevice = devnp(node);
  
  if(ndevice >= 0 && dev[ndevice]->matchname != 1)
    return(baddstr(dev[ndevice]->baddr,0));
  else
    return("00:00:00:00:00:00");
  }
  


int devnfrombadd(unsigned char *badd,int type,int dirn)
  {
  int n;
  
  for(n = 0 ; devok(n) != 0 ; ++n)
    { 
    if(dev[n]->matchname != 1)
      {   
      if(bincmp(badd,dev[n]->baddr,6,dirn) != 0 && (type == 0 || (type & dev[n]->type) != 0))
        return(n);
      }
    }
  return(-1);
  }
 
int devnfromhandle(unsigned char *hand)
  {
  int k;
  
  for(k = 1 ; devok(k) != 0 ; ++k)
    {
    if(dev[k]->conflag != 0 && (dev[k]->type & (BTYPE_CL | BTYPE_LE | BTYPE_ME)) != 0)
      {        // hi 4 bits of [1] = flags
      if(dev[k]->dhandle[0] == hand[0] && dev[k]->dhandle[1] == (hand[1] & 15))
        return(k);
      }
    }
  return(0);    
  }  
   
 
 
void mesh_on()
  {  // turn on LE advertising
  if(check_init(0) == 0)
    return;
  sendhci(leadvon,0);
  //statusok(0,leadvon);
  gpar.meshflag |= MESH_W;
  } 

void mesh_off()
  {  // turn off LE advertising
  if(check_init(0) == 0)
    return;
  sendhci(leadvoff,0);
  //statusok(0,leadvoff);
  gpar.meshflag &= ~MESH_W;
  } 

void meshreadon()
  {
  if((gpar.meshflag & MESH_R) != 0)
    return;
  sendhci(lescanon,0);
  //statusok(0,lescanon);
  gpar.meshflag |= MESH_R;
  }

void meshreadoff()
  {
  if((gpar.meshflag & MESH_R) == 0)
    return;
  sendhci(lescanoff,0);
  //statusok(0,lescanoff);
  gpar.meshflag &= ~MESH_R;
  }


int write_mesh(unsigned char *buf,int count)
  {
  int n;

  if(check_init(0) == 0)
    return(0);
  
  if((gpar.hidflag & 3) != 0)
    {
    NPRINT "Mesh does not work with random address/HID device\n");
    return(0);
    }
  
   
  if(count < 0 || count > 25)
    {
    NPRINT "More than 25 bytes of mesh data\n");
    flushprint();
    return(0);
    }

  mesh_on();           
   
  if(gpar.printflag == PRINT_VERBOSE)
    {
    VPRINT "SEND data to mesh\n");
    VPRINT "  Set [4][5] to byte counts\n");
    VPRINT "  Set [11].. to %d data bytes\n",count);
    }
      
   
  leadvert[PAKHEADSIZE+4] = count+6;
  leadvert[PAKHEADSIZE+5] = count+5;
  
  leadvert[PAKHEADSIZE+9] = dev[0]->meshindex & 0xFF;
  leadvert[PAKHEADSIZE+10] = (dev[0]->meshindex >> 8) & 0xFF;
  
  dev[0]->meshindex = (dev[0]->meshindex + 1) & 0xFFFF;
  if(dev[0]->meshindex == 0)
    ++dev[0]->meshindex;
    
  for(n = 0 ; n < count ; ++n)
    leadvert[PAKHEADSIZE+11+n] = buf[n];
    
  addname(0);     
  sendhci(leadvert,0);
  readhci(0,0,0,0,0);
  
  //if(statusok(0,leadvert) == 0)
  //  return(0);
  return(count);  
  }

void uuid_advert(unsigned char *uuid)
  {
  int n,nuuid;
  unsigned char *adv;

  if(check_init(0) == 0)
    return;
  
  nuuid = (gpar.hidflag >> 8) & 0xFF;

  adv = leadvertx + PAKHEADSIZE + 8;
  for(n = 0 ; n < nuuid ; ++n)
    {
    if(adv[2*n+2] == uuid[1] && adv[2*n+3] == uuid[0])
      return; 
    }
  if(nuuid > 11)
    {
    NPRINT "Too many UUID adverts\n");
    return;
    }
    
  adv[2*nuuid+2] = uuid[1];
  adv[2*nuuid+3] = uuid[0];
  ++nuuid;
  adv[0] = nuuid*2 + 1;
  adv[1] = 0x02;
  gpar.hidflag &= 0xFF;
  gpar.hidflag |= (nuuid << 8);
  addname(1);
  if((gpar.hidflag & 2) != 0)
    sendhci(leadvertx,0); 
  }


void addname(int flag)
  {   // add device name and pad zeoroes
  int maxm,k,padn,count,len,nuuid;
  unsigned char *leadv;


  if(flag == 0)
    {
    leadv = leadvert;
    count = leadv[PAKHEADSIZE+5] + 1;  // mesh data
    }
  else
    {  // random
    nuuid = (gpar.hidflag >> 8) & 0xFF;
    count = 3;
    if(nuuid > 0)
      count += 2*nuuid + 2;
    leadv = leadvertx;
    }
    
  maxm = 27 - count;
    
  if(maxm < 2)  // no room for name
    {
    padn = count + 5;
    leadv[PAKHEADSIZE+4] = count;
    }
  else
    {
    len = strlen(dev[0]->name);  
    if(len > maxm)
      len = maxm;
    leadv[PAKHEADSIZE+4] = count+len+2;
    leadv[PAKHEADSIZE+count+5] = len+1;
    leadv[PAKHEADSIZE+count+6] = 0x08; // name code
    for(k = 0 ; k < len ; ++k)
      leadv[PAKHEADSIZE+count+k+7] = dev[0]->name[k];
    padn = count + len + 7;
    }

   // pad with 0
  for(k = padn ; k <= 35 ; ++k)
    leadv[PAKHEADSIZE+k] = 0;
  }
 
 struct cticdata *ctic(int ndevice,int cticn)
  {
  int cn;
  struct cticdata *cp;
  
  if(cticn < 0 || devok(ndevice) == 0)
    return(&cticnull);  // type = CTIC_END
  cp = dev[ndevice]->ctic;
  cn = 0;
  while(cn < cticn && cp->type == CTIC_ACTIVE)
    {
    cp = cp->nextctic;
    ++cn;
    }
  return(cp);  
  }

 
struct cticdata *cticalloc(int ndevice)
  {
  int j;
  struct cticdata *cp,**cpp;   
   
  if(devokp(ndevice) == 0)
    return(&cticnull);
  
  cpp = &dev[ndevice]->ctic;   
  j = 0;
  while((*cpp)->type == CTIC_ACTIVE)
    {
    cpp = &(*cpp)->nextctic;
    ++j;
    if(j >= 255)
      {
      NPRINT "Out of characteristic storage\n");
      return(&cticnull);
      }
    }
    
  if((*cpp)->type == CTIC_END)
    {
    // new cticdata to replace cticnull
    cp = malloc(sizeof(struct cticdata));
    if(cp == NULL)
      {
      NPRINT "Alloc error\n");
      return(&cticnull);
      }
    *cpp = cp;
    cp->nextctic = &cticnull;  // with type = CTIC_END
    }
  else  // must be CTIC_UNUSED - already allocated
    cp = *cpp;    
     
  cp->type = CTIC_UNUSED;
  cp->cticn = j;
  cp->size = 0;
  cp->origsize = 0;   
  cp->perm = 0;
  cp->notify = 0;
  cp->chandle = 0;
  cp->uuidtype = 0;
  cp->reportflag = 0;
  cp->psnx = 0;
  cp->lasthandle = 0xFFFF;
  cp->iflag = 0;
  cp->callback = NULL;
#ifdef BTFPYTHON
  cp->pycallback = NULL;
#endif  
  for(j = 0 ; j < 16 ; ++j)
    cp->uuid[j] = 0;
  
  for(j = 0 ; j < LEDATLEN ; ++j)
    cp->value[j] = 0;
      
  return(cp);    
  }

int devalloc(unsigned char *badd,int type)
  {
  int j,dn;
  struct devdata *dp;
  static char lkey[16] = { 0x08,0x7A,0xC7,0xFB,0x8C,0x86,0xF3,0xCF,0x36,0xF4,0x0C,0xD8,0xDD,0xA2,0xF9,0xD3 }; 
    
  dn = 0;

  while(dn < NUMDEVS && dev[dn] != NULL && dev[dn]->type != 0)
    ++dn; 
        
  if(dn >= NUMDEVS)
    {
    NPRINT "Out of NUMDEVS device storage\n");
    return(-1);
    }

  if(dev[dn] == NULL)
    {
    dev[dn] = malloc(sizeof(struct devdata));
    if(dev[dn] == NULL)
      {
      NPRINT "Alloc error\n");
      return(-1);
      }
    dev[dn]->ctic = &cticnull;   
    }
  
  dp = dev[dn];  
  dp->conflag = 0;
  dp->meshindex = 0;
  dp->leaddtype = 0;
  dp->lecflag = 0;
  dp->rfchan = 0;
  dp->dhandle[0] = 0;
  dp->dhandle[1] = 0;
  dp->matchname = 0;
  dp->setdatlen = 20;
  dp->foundflag = 0;
  dp->leinterval = 0;
  dp->cryptoflag = 0;
  dp->lepasskey = 0;
  dp->lepairflags = 0;
  dp->keyk = 255;
  dp->pairk = 255;
    
    
  for(j = 0 ; j < 16 ; ++j)
    {
    dp->linkey[j] = lkey[j];
    dp->divrand[j] = 0;
    }
  for(j = 0 ; j < 64 ; ++j)
    dp->advert[j] = 0;
    
  dp->linkflag = 0;        
  dp->type = type;
  dev[dn]->name[0] = 0; 
  dp->node = 0;
  dev[dn]->pincode[0] = 0;
  dp->id = 1;
  dp->psm = 3;
  dp->credits = 0;
  dp->method = METHOD_HCI;
  if(badd == NULL)
    {
    for(j = 0 ; j < 6 ; ++j)
      dp->baddr[j] = 0;
    }
  else
    {
    for(j = 0 ; j < 6 ; ++j)
      dp->baddr[j] = badd[5-j];
    rwlinkey(0,dn,NULL);  // keys
    }
  return(dn);    
  }




/******************** LE SCAN ***********/
  
void le_scan()
  {
  if(check_init(0) == 0)
    return;
  lescanx();
  flushprint();
 
  } 
  
  
int lescanx()
  {    
  int n,j,k,i,tn,ndevice,count,repn,type,newcount,flag,nflag,meshflag;
  unsigned char *rp;
  struct devdata *dp;
  static char *fixran[2] = {"Fixed","Random"};
  static unsigned char advert[8] = { 0,0,0xC0,0xDE,0x99 };
  char *buf;
  //static double powa[5] = { 1,10,100,1000,10000 };
  //static double powb[10] = { 1,1.25,1.6,2.0,2.5,3.16,4,5,6.3,8.0 };
  
  
  NPRINT "Scanning for LE devices - 10 seconds..\n");
  flushprint();
 
 
  VPRINT "Enable LE scan\n");
  sendhci(lescanonf,0); 
  //if(statusok(0,lescanonf) == 0)
  //  {
  //  NPRINT "Scan on failed\n");
  //  return(0);
  //  }
    
  readhci(0,0,IN_LESCAN,10000,0);  // may be multiple scan replies                
  
  VPRINT "Disable LE scan\n");
  sendhci(lescanoff,0);
  // statusok(0,lescanoff);  

  for(n = 0 ; devok(n) != 0 ; ++n)
    dev[n]->foundflag = 0;   

  buf = (char*)gpar.buf;  
  count= 0;
  newcount = 0;
  do
    {
    n = findhci(IN_LESCAN,0,INS_LOCK);
    if(n < 0)   // finished - normal exit
      {     
      NPRINT "Found %d unknown devices\n",newcount);
      popins();
      return(1);
      }
    // insdat[n+1] = number of responses
    // data for each response starts at rp
    // first rp = insdat[n+2]   
    
    rp = insdat+n+2;
    for(repn = 0 ; repn < insdat[n+1] ; ++repn)
      {  // each response
      // rp[2]...[7]  board address

      ndevice = devnfrombadd(rp+2,BTYPE_LE | BTYPE_ME,DIRN_REV);
      
      if(ndevice < 0 || dev[ndevice]->foundflag == 0)      
        {  // not found  
        NPRINT "FOUND %s - %s\n",baddstr(rp+2,1),fixran[rp[1] & 1]);      
        flushprint();
      
        type = BTYPE_LE;  
        meshflag = 0;
        buf[0] = 0;  // for name             
        // find name in data at rp[9] length rp[8]
      
        j = 9;
        while(j-9 < rp[8])   // && dp->name[0] == 0)
          {  // each data entry
          // rp[j]=length type+data  rp[j+1]=type   rp[j+2]=data
          tn = rp[j+1];  // type
               // convert to valid adlist[] index
          if(tn == 0xFF)
            tn = 47;
          else if(tn == 0x3D)
            tn = 46;
          else if(tn > 0x2D)
            tn = 0;

          if(rp[j+1] == 0xFF && (rp[j+2] ==  ((rp[2] ^ rp[3]) ^ rp[4]) )  &&
                              (rp[j+3] == (((rp[5] ^ rp[6]) ^ rp[7]) | 0xC0) ) )
            {
            if(bincmp(rp+j+4,advert,5,DIRN_FOR) != 0)
              {
              type = BTYPE_ME;
              meshflag = 2;  // is a MESH devices
              }
            else
              meshflag = 1;  // might be a MESH device
            }
                        
          if(rp[j+1] == 8 || rp[j+1] == 9)  // found name 8=short 9=full
            {  // length rp[j]
            if(rp[j] > 1)
              {
              k = 0;
              while(k < rp[j]-1 && k < NAMELEN-1)   // to name[NAMELEN]
                {
                buf[k] = rp[j+2+k];
                ++k;
                }  
              buf[k] = 0;
              NPRINT "    Name = %s (Use this for MATCH_NAME)\n",buf);
              }
            }
          else
            {  // length rp[j]
            if(rp[j] > 1)
              {
              if(tn == 2 || tn == 3)
                {  // 2-byte UUIDs
                NPRINT "    %s\n",adlist[tn]);         
                k = 0;
                while(k < rp[j]-1)
                  {
                  if((k % 2) != 0)
                    {
                    if(rp[j+2+k] == 0xFE && rp[j+1+k] == 0xAA)
                      NPRINT "      FEAA Eddystone beacon\n");
                    else if(rp[j+2+k] == 0xFD && rp[j+1+k] == 0x6F)
                      NPRINT "      FD6F Contact tracing\n");
                    else
                      NPRINT "      %02X%02X %s\n",rp[j+2+k],rp[j+1+k],uuidlist+finduuidtext((rp[j+2+k] << 8) + rp[j+1+k]));                 
                    }
                  ++k;
                  }
                }
              else
                {
                if(rp[j] == 0x1A && rp[j+1] == 0xFF && rp[j+2] == 0x4C && rp[j+3] == 0x00)
                  {
                  NPRINT "    iBeacon UUID ");
                  for(k = 6 ; k < 22 ; ++k)
                    NPRINT "%02X",rp[j+k]);
                  NPRINT " Major %02X%02X Minor %02X%02X",rp[j+22],rp[j+23],rp[j+24],rp[j+25]);
                  }
                else
                  {
                  if(rp[j+1] == 0xFF && rp[j+2] == 0x4C && rp[j+3] == 0x00)
                    NPRINT "    Apple =");
                  else if(rp[j+1] == 0xFF && rp[j+2] == 0x06 && rp[j+3] == 0x00)
                    NPRINT "    Microsoft =");       
                  else if(rp[j+1] == 0xFF && rp[j+2] == 0x75 && rp[j+3] == 0x00)
                    NPRINT "    Samsung =");       
                  else       
                    NPRINT "    %s =",adlist[tn]);
                           
                  k = 0;
                  while(k < rp[j]-1)
                    {
                    if((tn == 6 || tn == 7) && k > 0 && (k & 0x0F) == 0)
                      NPRINT "\n       ");  // 16-byte UUIDs new line  
                    NPRINT " %02X",rp[j+2+k]);
                    ++k;
                    }
                  }
                NPRINT "\n");
                }                     
              }
            }  
            
          flushprint();   
          j += rp[j] + 1;  // next entry   
          }

      /******
      if(rp[j] > 0x7F)
        {  // estimate distance from rssi
        // rssi = rp[j];
        fac = (double)(256 - rp[j] - 69)/20.0;
        k = 0;
        i = 0;
        if(fac > 0)
          {
          k = (int)fac;
          if(k > 4)
            k = 4;
          i = (int)(fac * 10);
          i = i % 10;
          }    
        NPRINT "    Approx distance = %.0fm\n",powa[k]*powb[i]);
        }
      ******/   
        
        if((ndevice < 0 || (ndevice > 0 && dev[ndevice]->node >= 1000)) && buf[0] != 0)
          {  // no board address match or 1000+ from remote connect and have name in buf
             // look for name match or random address change
          nflag = 0;
          for(k = 1 ; nflag == 0 && devok(k) != 0 ; ++k)
            {
            if((dev[k]->type == BTYPE_LE || dev[k]->type == BTYPE_ME) && k != ndevice && dev[k]->node < 1000)
              {
              if((dev[k]->matchname & 1) != 0)
                {  // MATCH_NAME
                flag = 0;
                for(i = 0 ; i < (int)strlen(dev[k]->name) && flag == 0 ; ++i)
                  {
                  if(dev[k]->name[i] != buf[i])
                    flag = 1;
                  }
                if(flag == 0)
                  {
                  NPRINT "    Found via MATCH_NAME\n");
                  dev[k]->matchname |= 2;  // found address flag
                  dev[k]->leaddtype = rp[1] & 1;  // type public/random
                  dev[k]->leaddtype |= 2;         // found by scan
                  ndevice = k;
                  nflag = 1;
                  for(i = 0 ; i < 6 ; ++i)
                    dev[k]->baddr[i] = rp[7-i];          
                  }
                }            
              else if((dev[k]->leaddtype & 1) != 0 && (rp[1] & 1) != 0 && strcmp(buf,dev[k]->name) == 0)
                {
                NPRINT "    Random address changed\n");
                ndevice = k;
                // changed board address
                for(i = 0 ; i < 6 ; ++i)
                  dev[k]->baddr[i] = rp[7-i];
                }
              }
            }
          }
                  
        if(ndevice >= 0)
          {
          NPRINT "    Node=%d   Known device %s\n",dev[ndevice]->node,dev[ndevice]->name);
          if((dev[ndevice]->leaddtype & 1) != (rp[1] & 1))
            {
            NPRINT "    Changing fixed/random address type\n");
            if((dev[ndevice]->leaddtype & 2) == 0)
              NPRINT "    *** Fixed/Random info in devices file is wrong ***\n"); 
            dev[ndevice]->leaddtype = (rp[1] & 1) | (dev[ndevice]->leaddtype & 2);
            }
          if(dev[ndevice]->type == BTYPE_ME && dev[ndevice]->node >= 1000)
            NPRINT "    Add to devices.txt with node < 1000 to authorise mesh packets\n");
          if(dev[ndevice]->type != BTYPE_LE && dev[ndevice]->type != BTYPE_ME)
            NPRINT "    But not listed as LE or Mesh\n");
          }
        else
          {  // not already stored
          if(meshflag != 0)  // auto detect mesh device
            {
            if(meshflag == 2)
              NPRINT "                    MESH device\n");
            else
              NPRINT "                    May be a MESH device\n");
            NPRINT "                    To receive mesh packets from this device\n");
            NPRINT "                    add to devices.txt with node < 1000\n");
            }
          // find next free entry
          ndevice = devalloc(rp+2,type);
          if(ndevice < 0)  // failed
            {
            instack[n] = INS_POP;
            return(0);
            }
          ++newcount;
          dp = dev[ndevice];
          dp->leaddtype |= rp[1] & 1;  // type public/random
          dp->leaddtype |= 2;          // found by scan
          dp->node = newnode();
             
          if(buf[0] == 0)
            {  // no name
            k = devnfrombadd(dp->baddr,BTYPE_CL,DIRN_FOR);
            if(k > 0 && bincmp((unsigned char*)dev[k]->name,(unsigned char*)"Classic",7,DIRN_FOR) == 0)
              strcpy(buf,dev[k]->name);  // is also a classic with name 
            else
              {
              sprintf(buf," LE %s",baddstr(dp->baddr,0));
              // NPRINT "    No name so set = %s\n",buf);
              }
            }
          else
            {  // got name
            k = devnfrombadd(dp->baddr,BTYPE_CL,DIRN_FOR);
            if(k > 0 && bincmp((unsigned char*)dev[k]->name,(unsigned char*)"Classic",7,DIRN_FOR) != 0)
              strcpy(dev[k]->name,buf);  // is also a classic without name 
            }
          
          strcpy(dev[ndevice]->name,buf);
                      
          NPRINT "    Node=%d   New device %s\n",dev[ndevice]->node,dev[ndevice]->name);
          }
        if(ndevice >= 0)
          {
          dev[ndevice]->foundflag = 1;
          for(k = 0 ; k <= rp[8] && k < 64 ; ++k)
            dev[ndevice]->advert[k] = rp[8+k];
          if(dev[ndevice]->advert[0] > 63)
            dev[ndevice]->advert[0] = 63;
          }  
        }  // end not found       
      flushprint();          
      // next device entry is length 10+rp[8] away
      rp += rp[8] + 10;
      }  
    
    instack[n] = INS_POP;  
    flushprint();         
    ++count;
    }
  while(1); 
  }
  
unsigned char* le_advert(int node)
  {
  int ndevice;
  static unsigned char none[4] = { 0,0,0,0 };

  if(check_init(0) == 0)
    return(none);

  ndevice = devnp(node);
  if(ndevice < 0)
    return(none);
  return(dev[ndevice]->advert);
  }  
  

int newnode()
  {
  int node;
  
  for(node = 1000 ; node < 1256 ; ++node)
    {
    if(devn(node) < 0)
      return(node);
    }
    
  NPRINT "Failed to find free node number\n");
  flushprint();
  return(0);    
  }
  


int devokp(int ndevice)
  {
  if(devok(ndevice) == 0)
    {
    NPRINT "Invalid device\n");
    flushprint();
    return(0);
    }
  return(1);
  }
    
int devok(int ndevice)
  {
  if(ndevice < 0 || ndevice >= NUMDEVS || dev[ndevice] == NULL || dev[ndevice]->type == 0)
    return(0);   
  return(1);
  }


int devnp(int node)
  {
  int dn;
  
  dn = devn(node);
  if(dn < 0)
    {
    NPRINT "Invalid node\n");
    flushprint();
    }
  return(dn);
  }
   
int devn(int node)
  {
  int n;
  
  if(node <= 0 || node >= 0x10000)
    return(-1);  // invalid node
    
  for(n = 0 ; devok(n) != 0 ; ++n)
    {
    if(dev[n]->node == node)
      return(n);
    }
  return(-1);
  }


int ctic_ok(int node,int cticn)
  {   
  int ndevice;
 
  if(check_init(0) == 0)
    return(0);
  
  ndevice = devn(node);
  
  if(ndevice < 0 || ctic(ndevice,cticn)->type != CTIC_ACTIVE)
    return(0);   // checked ndevice
  return(1);
  }


/********** DEVICE NAME **************
input node
return pointer to device name string
***************************************/

char *device_name(int node)
  {
  int ndevice;
  
  if(check_init(0) == 0)
    return("");
   
  ndevice = devn(node);
    
  if(ndevice >= 0)
    {
    if(dev[ndevice]->name[0] == 0)
      return("Nameless");
    else
      return(dev[ndevice]->name);
    }
  return("Invalid device");
  }

/******** CHARACTERISTIC NAME *******
input node
      cticn = characteristic index
return pointer to characteristic name string
********************/

char *ctic_name(int node,int cticn)
  {
  int ndevice;
  struct cticdata *cp;

  if(check_init(0) == 0)
    return("");
  
  ndevice = devn(node);
  if(ndevice < 0)
    return("Invalid node");
    
  cp = ctic(ndevice,cticn);  // checks ndevice and cticn
  if(cp->type == CTIC_ACTIVE)
    {
    if(cp->name[0] == 0)
      return("Nameless");
    else
      return(cp->name);
    }
  return("Invalid characteristic");
  }


int device_connected(int node)
  {
  int ndevice;
  struct devdata *dp;
  
  if(check_init(0) == 0)
    return(0);
   
  ndevice = devnp(node);
  if(ndevice > 0)
    {
    dp = dev[ndevice];
    if((dp->conflag & CON_RF) != 0)
      return(CLASSIC_CONN);
    if((dp->conflag & (CON_LE | CON_LX)) != 0)
      return(LE_CONN);
    if((dp->conflag & CON_MESH) != 0)
      return(NODE_CONN);
    }    

  return(NO_CONN);
  }
   

/******** DEVICE TYPE ********
input node
return device type
           0 = invalid device number
    BTYPE_CL = classic
    BTYPE_LE = LE
    BTYPE_ME = Mesh
    BTYPE_LO = Local
********************************/    

int device_type(int node)
  {
  int ndevice;

  if(check_init(0) == 0)
    return(0);
  
  ndevice = devnp(node);
  
  if(ndevice < 0)
    return(0);
    
  return(dev[ndevice]->type);
  }

int exitchar()
  {
  if(check_init(0) == 0)
    return((int)'x');
  return(gpar.exitchar);
  }
    
int localnode()
  {
  if(check_init(0) == 0)
    return(0);  
  return(dev[0]->node); 
  }
 
int device_index(int node)
  {
  return(0);
  } 
  
/********* DEVICE INFO *********
mask = OR any combination of the following bit masks:
 BTYPE_CL = include classic devices
 BTYPE_LE = include LE devices
 BTYPE_LO = include Local device
 BTYPE_ME = include mesh devices
 BTYPE_CONNECTED = only list connected devices
 BTYPE_DISCONECTED = only list disconnected devices
 BTYPE_SHORT = only list node  number and name - no details

return number of devices listed
******************************/  
  
int device_info(int mask)
  {
  int n,flag,con,count;
  struct devdata *dp;
  char *s;
  
  if(check_init(0) == 0)
    return(0);

  if( (mask & BTYPE_SHORT) != 0)
    return(devlist(NULL,0,mask));

  NPRINT "\nnode                        btlib version %d\n",VERSION);
      
  count = 0;   // number printed
  
  for(n = 0 ; devok(n) != 0 ; ++n)
    {
    dp = dev[n];
    flag = 0;   // no print
    if( (dp->type & (mask & 0xFFFF)) != 0)
      flag = 1;  // type matches mask
    if( (mask & BTYPE_CONNECTED) != 0 && dp->conflag == 0 && n != 0)
      flag = 0;   // not connected
    if( (mask & BTYPE_DISCONNECTED) != 0 && dp->conflag != 0 && n != 0)
      flag = 0;   // not disconnected
      
          
    if(flag != 0)
      {
      ++count;
      
      if(n == 0 && dp->node != 0)
        NPRINT "%d  Local (%s) ",dp->node,dp->name);         
      else
        {
        NPRINT "%d  %s  ",dp->node,dp->name);

        if( (dp->type & BTYPE_CL) != 0)
          NPRINT "Classic");
        else if( (dp->type & BTYPE_LE) != 0)
          NPRINT "LE");  
        else if( (dp->type & BTYPE_ME) != 0)
          NPRINT "Mesh");
        }
   
      if(n == 0)
        {   
        if((gpar.meshflag & MESH_W) == 0)
          s = "off";
        else
          s = "on";
               
        NPRINT "  Mesh transmit %s",s);   
        }
      else
        {
        con = device_connected(dp->node);
       
        if(con == NO_CONN)
          NPRINT  "  Not connected");
        else
          {
          NPRINT "  Connected as ");
          if(con == CLASSIC_CONN)
            NPRINT "CLASSIC");
          else if(con == LE_CONN)
            NPRINT "LE  Interval = %d",dp->leinterval);    
          else if(con == NODE_CONN)
            NPRINT "NODE");
          }       
        }

     
      if(dp->matchname == 1)
        NPRINT "\n     Address via MATCH_NAME not found - run scan");
      else 
        {    
        NPRINT "\n      %s",baddstr(dp->baddr,0));
        if((dp->matchname & 2) != 0)
          NPRINT " via MATCH_NAME");
        }
      if(dp->type == BTYPE_LE && (dp->leaddtype & 1) != 0)
        NPRINT " Random - may change");
      
      if((dp->type & BTYPE_CL) != 0 && dp->pincode[0] != 0)
        NPRINT " PIN=%s",dp->pincode);    
    
      if((dp->type & BTYPE_CL) != 0 && dp->rfchan != 0)
        NPRINT " Channel=%d",dp->rfchan);
      
      NPRINT "\n");
    
      if(dp->type == BTYPE_ME && dp->node >= 1000)
        NPRINT "      Add to devices.txt with node < 1000 to authorise mesh packets\n");             
      flushprint();
      
      if(dp->type != BTYPE_CL)
        printctics1(n);
      
        
        
      }    // end details 
    flushprint();
    }    // end device loop
    
  return(count);
  }
  

int device_info_ex(int mask,char *buf,int len)
  {
  if(check_init(0) == 0)
    return(0);  
  return(devlist(buf,len,mask));
  }
  
int devlist(char *listbuf,int listlen,int mask)
  {
  int n,i,j,k,xn,ln,bn,count,flag,maxlen,node,listflag;
  short *vn;
  unsigned char *len;
  char *s,*buf;
  struct devdata *dp;

  vn = (short*)gpar.buf;  // 512      
  len = (unsigned char*)(gpar.buf+1024);
  buf = (char*)(gpar.buf+1536);
   
  if(listbuf == NULL || len <= 0)
    listflag = 0;
  else
    listflag = 1;
    
  ln = 0;   
  maxlen = 0;
  count = 0;
  if((mask & BTYPE_ANY) != 0)
    {
    vn[0] = -1;
    ++count;
    }
    
  for(n = 1 ; devok(n) != 0 ; ++n)
    {
    dp = dev[n];
    flag = 0;   // no print
    if( (dp->type & (mask & 0xFFFF)) != 0)
      flag = 1;  // type matches mask
    if( (mask & BTYPE_CONNECTED) != 0 && dp->conflag == 0 && n != 0)
      flag = 0;   // not connected
    if( (mask & BTYPE_DISCONNECTED) != 0 && dp->conflag != 0 && n != 0)
      flag = 0;   // not disconnected

    if(flag != 0)
      {
      vn[count] = n;
      if(count < 511)
        ++count;
      }
    }

  if((mask & BTYPE_LO) != 0)
    {
    vn[count] = 0;
    if(count < 511)
      ++count;
    }

  if((mask & BTYPE_SERME) != 0)
    {
    vn[count] = -2;
    if(count < 511)
      ++count;
    }
  
  if(count == 0)
    return(0);         
   
  flag = 0;        // one column
  xn = count;      // last vn index + 1
  buf[0] = 0;
  
 
  if(listflag == 0 && count > 5)
    {
    flag = 1;   // two column
    xn = (count+1)/2;    // last vn index + 1  of first column
         // find max length of first column       
    for(n = 0 ; n < xn ; ++n)
      {
      if(vn[n] == -1)
        len[n] = 11;  // Any device
      else if(vn[n] == -2)
        len[n] = 13;  // Mesh servers
      else
        {
        sprintf(buf,"%d",dev[vn[n]]->node);
        len[n] = strlen(buf);
        if(vn[n] == 0)
          len[n] += 5;  // Local
        else
          len[n] += strlen(dev[vn[n]]->name);
        }
      if(len[n] > maxlen)
        maxlen = len[n];
      }
    }
  
  for(n = 0 ; n < xn ; ++n)
    {
    i = vn[n];
    if(i == -1)
      {
      node = 0;
      s = "Any device";
      }
    else if(i == -2)
      {
      node = 0;
      s = "Mesh servers";
      }
    else
      {
      node = dev[i]->node;
      if(i == 0)
        s = "Local";
      else
        s = dev[i]->name;
      }

    if(listflag == 0)      
      NPRINT "  %d - %s",node,s);
    else
      {
      sprintf(buf,"%d - %s\n",node,s);
      bn = 0;
      while(buf[bn] != 0 && ln < listlen-1)
        {
        listbuf[ln] = buf[bn];
        ++bn;
        ++ln;
        }
      listbuf[ln] = 0;
      }
      
    if(flag != 0)
      {       
      j = n+xn;   // 2nd column vn[0 to count-1] index
      if(j < count)
        {
        j = vn[j];       
        k = len[n];
        while(k < maxlen+4)
          {
          NPRINT " ");
          ++k;
          }
        if(j == -2)
          {
          node = 0;
          s = "Mesh servers";
          }
        else
          {
          node = dev[j]->node;
          if(j == 0)
            s = "Local";
          else
            s = dev[j]->name;
          }
      
        NPRINT "%d - %s",node,s);
        }
      }    
    if(listflag == 0)     
      NPRINT "\n");
    } 
    
  flushprint();
  return(count);
  }  
  
int le_interval(int node)
  {
  int n;
  struct devdata *dp;

  if(check_init(0) == 0)
    return(0);
  
  n = devnp(node);
  if(n < 0)
    return(0);
  dp = dev[n];
  if(dp->conflag == CON_LE || dp->conflag == CON_LX || dp->conflag == CON_MESH)
    return(dp->leinterval);
  NPRINT "Error le_interval - not connected\n");
  return(0);
  }
  
/******** MESH SERVER **********/ 
  
void mesh_server(int(*callback)())
  {
  int nread,retval,clientnode,locdev;
  static unsigned char buf[32];
#ifdef BTFPYTHON
  PyObject *pycallback;
#endif

  if(check_init(0) == 0)
    return;

  if(gpar.serveractive != 0)
    {
    NPRINT "Cannot start a second server\n");
    return;
    }

#ifdef BTFPYTHON
  pycallback = py_mecallback;    
#endif

  gpar.serveractive = 1;
  mesh_on(); 
  meshpacket(NULL);  // enable unknown device message
   
  NPRINT "Mesh server listening (x = stop server)\n");
  flushprint();
  serverexit(1);
  meshreadon();
  retval = SERVER_CONTINUE;
  do
    {
    locdev = FROM_MESH;
    nread = readserial(&locdev,buf,32,0,EXIT_KEY,0);
    if(locdev == 0)
      clientnode = 0;
    else
      clientnode = dev[locdev]->node;  // known sender

    if(nread > 0)
      {
#ifdef BTFPYTHON
      if(pycallback != NULL)
        retval = py_mesh_callback(pycallback,clientnode,buf,nread);
#else
      if(callback != NULL)
        retval = (*callback)(clientnode,buf,nread);
#endif
      }
    else
      retval = SERVER_EXIT;
      
    flushprint();
    }
  while((retval & SERVER_CONTINUE) != 0 && read_error() != ERROR_KEY);
  serverexit(0);
  gpar.serveractive = 0;
  meshreadoff();
  if(read_error() == ERROR_KEY)
    NPRINT "Key press stop server\n");
  else if(read_error() == ERROR_FATAL)
    NPRINT "Fatal error stop server\n");
      
  flushprint();    
  }


/*********** LE SERVER ***********/


int le_server(int(*callback)(int clientnode,int operation,int cticn),int timerds)
  {
  int n,dn,key,ndevice,retval,oldkm,op,cticn,cbflag,loopt;
  unsigned long long tim0,timms;
  struct devdata *dp;
  unsigned char readret,*badd;
#ifdef BTFPYTHON
  PyObject *pycallback;
#endif

  if(check_init(0) == 0)
    return(0);

  if(gpar.serveractive != 0)
    {
    NPRINT "Cannot start a second server\n");
    return(0);
    }
     
  mesh_on();   
  oldkm = setkeymode(1); 
  dp = dev[0];
  
  NPRINT "Listening for LE clients to connect ");
  if(gpar.keytocb == 0)
    {
    gpar.exitchar = 'x';
    NPRINT "(x=stop server)\n");
    }
  else
    {
    gpar.exitchar = 27;
    NPRINT "(ESC=stop server)\n");
    }
    
  if((gpar.hidflag & 3) == 0)
    badd = dev[0]->baddr;
  else
    badd = gpar.randbadd;

  if((gpar.hidflag & 1) == 0)  
    NPRINT "Advertising as %s %s\n",baddstr(badd,0),dev[0]->name);
  else
    NPRINT "Advertising as %s HID device\n",baddstr(badd,0));
    
  flushprint();
  ndevice = 0;

#ifdef BTFPYTHON
  pycallback = py_lecallback;
#else
  gpar.lecallback = callback;
#endif
  
  gpar.serveractive = 1;

  tim0 = time_ms();
  timms = timerds;
  loopt = 0;
  if((gpar.settings & FAST_TIMER) == 0)
    {
    loopt = 10;
    timms *= 100;
    }
  
  serverexit(1);  
  retval = SERVER_CONTINUE;  
  do
    {
    readhci(LE_SERV,IN_LECMD,0,loopt,0);

    cbflag = 0;  // callback not called
          
    n = findhci(IN_LECMD,0,INS_POP);
    if(n >= 0)
      {   
      ndevice = instack[n+3];
      dp = dev[ndevice];  
      op = insdatn[0];
      cticn = insdatn[1];
      if(op == LE_READ)
        readret = insdatn[2];
         
      if(op == LE_DISCONNECT)
        VPRINT "%s has disconnected\n",dp->name);
      else if(op == LE_CONNECT)
        {
        if((gpar.hidflag & 1) != 0)
          {  // HID wait for AUTO_PAIROK
          NPRINT "Wait for pair...\n");  
          readhci(ndevice,IN_AUTOEND,0,gpar.leclientwait,0);
          n = findhci(IN_AUTOEND,ndevice,INS_POP);
          if(n >= 0)
            {
            if(insdat[n] == AUTO_PAIROK)
              NPRINT "PAIR OK\n");
            }
          else
            NPRINT "PAIR time out\n");
          }       

        VPRINT "%s has connected\n",dp->name);
        if((gpar.hidflag & 1) == 0)  
          {
          mesh_on();
          VPRINT "Set larger data length\n");  
          sendhci(datlenset,ndevice);
          }
        }   

      flushprint();
      popins();     

      if(op == LE_READ)
        retval = readret;  // from leserver callback
      else
        {        
#ifdef BTFPYTHON
        if(pycallback != NULL)
          retval = py_le_callback(pycallback,dp->node,op,cticn);
        cbflag = 1; 
#else
        if(callback != NULL)
          retval = callback(dp->node,op,cticn);
        cbflag = 1;
#endif
        }    
      
      if(op == LE_DISCONNECT)
        {  // clear all operations from this device
        do
          {
          n = findhci(IN_LECMD,ndevice,INS_POP);
          }
        while(n >= 0); 
      
        popins();
        }
      }   
      
    if(cbflag == 0 && timerds > 0)
      {
      if(time_ms() - tim0 >= timms)
        {
#ifdef BTFPYTHON
        if(pycallback != NULL)
          retval = py_le_callback(pycallback,localnode(),LE_TIMER,0);
#else
        if(callback != NULL)
          retval = callback(localnode(),LE_TIMER,0);
#endif
        tim0 = time_ms();
        }
      }
      
    flushprint();
    popins();
    key = readkey();
 
    if(key > 0 && gpar.keytocb != 0)  
      {
      if(key == 27)  
        key = 'x';
      else
        {
        VPRINT "Key code %d\n",key); 

#ifdef BTFPYTHON
        if(pycallback != NULL)
          retval = py_le_callback(pycallback,localnode(),LE_KEYPRESS,key);
#else
        if(callback != NULL)
          retval = callback(localnode(),LE_KEYPRESS,key);
#endif
        key = 0;
        }
      }  
    }
  while((retval & SERVER_CONTINUE) != 0 && key != 'x');
  serverexit(0);
  
  setkeymode(oldkm);
  gpar.serveractive = 0;
     
  if(key == 'x')
    NPRINT "Key press stop server...\n");

  if((retval & SERVER_EXIT_CONNECTED) == 0)
    {
    for(dn = 1 ; devok(dn) != 0  ; ++dn)
      {
      dp = dev[dn];
      if((dp->conflag & CON_LX) != 0)
        {
        NPRINT "Disconnecting %s\n",dp->name);
        disconnectdev(dn);
        do
          {
          n = findhci(IN_LECMD,dn,INS_POP);
          }
        while(n >= 0);
        }
      }
    }  

  flushprint();

  popins();    
  mesh_on();
    
  return(1);
  }  

int keys_to_callback(int flag,int keyboard)
  {
  if(check_init(0) == 0)
    return(0);
  
  if(flag == KEY_OFF)
    gpar.keytocb = 0;
  else if(flag == KEY_ON)
    gpar.keytocb = 1;
  else
    {
    NPRINT "keys_to_callback() invalid parameter\n");
    return(0);
    }
  gpar.keyboard = keyboard;
  return(1);
  }
  
  
/*********** NODE SERVER ***********/


int node_server(int clientnode,int (*callback)(int clientnode,unsigned char *buf,int count),char endchar)
  {
  int nread,key,ndevice,retval,oldkm;
  static unsigned char buf[1024];
  struct devdata *dp;
#ifdef BTFPYTHON
  PyObject *pycallback;
#endif
  
  if(check_init(0) == 0)
    return(0);
    
  if(gpar.serveractive != 0)
    {
    NPRINT "Cannot start a second server\n");
    return(0);
    }    

  ndevice = devn(clientnode);
  if(ndevice <= 0 || dev[ndevice]->type != BTYPE_ME)
    {
    NPRINT "Invalid client node\n");
    flushprint();
    return(0);
    }
    
  dp = dev[ndevice];

  dp->setdatlen = 20;

  mesh_on();   
  oldkm = setkeymode(1);
  serverexit(1);  
  while((dp->conflag & CON_MESH) == 0)
    {     
    NPRINT "Listening for %s to connect (x=cancel)\n",dp->name);
    flushprint();
    do
      {
      readhci(ndevice,0,IN_LEHAND,0,0);    
      key = readkey();
      }
    while((dp->conflag & CON_MESH) == 0 && key != 'x');
    
    if(key == 'x')
      {
      NPRINT "Cancelled\n");
      flushprint();
      setkeymode(oldkm);
      popins();
      serverexit(0);
      return(0);
      }
    } 
    
  popins();  
  setkeymode(oldkm);

#ifdef BTFPYTHON
  pycallback = py_callback;
#endif
  gpar.serveractive = 1;

  NPRINT "Connected OK\n");
  NPRINT "Waiting for data from %s (x = stop server)\n",dp->name);
  setlelen(ndevice,LEDATLEN,0); 
 
  retval = SERVER_CONTINUE;
  do
    {
    nread = read_node_endchar(clientnode,buf,1024,endchar,EXIT_KEY,0);
    if(nread > 0)
      {
#ifdef BTFPYTHON
      if(pycallback != NULL)
        retval = py_cmn_callback(pycallback,clientnode,buf,nread);
#else      
      if(callback != NULL)
        retval = (*callback)(clientnode,buf,nread);
#endif
      }
    else
      retval = SERVER_EXIT;  // key press or error
    }
  while((retval & SERVER_CONTINUE) != 0 && read_error() != ERROR_KEY);
  serverexit(0);
  gpar.serveractive = 0;
  
  if(read_error() == ERROR_KEY)
    NPRINT "Key press stop server...\n");
  else if(read_error() == ERROR_FATAL)
    NPRINT "Fatal error stop server...\n");
      
  flushprint();  
  sleep_ms(2000);    // allow time for any last reply sent by callback to transmit      
  
  if((retval & SERVER_EXIT_CONNECTED) == 0)
    disconnect_node(clientnode);  // sever initiated here
                                  // client should be running
                                  // wait_for_disconnect
  read_node_clear(clientnode);
        
  mesh_on();
  
  return(1);
  }  



int universal_server(int(*callback)(int,int,int,unsigned char*,int),char endchar,int keyflag,int timerds)
  {
  int n,dn,nread,key,ndevice,retval,loopt;
  int node,op,cticn;
  unsigned char readret;
  unsigned long long tim0,timms;
  struct devdata *dp;
  static unsigned char buf[1024];
#ifdef BTFPYTHON
  PyObject *pycallback;
#endif  

  if(check_init(0) == 0)
    return(0);
    
  if(gpar.serveractive != 0)
    {
    NPRINT "Cannot start a second server\n");
    flushprint();
    return(0);
    }
    
  if((gpar.hidflag & 3) != 0 || gpar.keytocb != 0)
    {
    NPRINT "No HID/random address/keys to callback with universal server\n");
    flushprint();
    return(0);
    } 
 
  VPRINT "Set simple pair mode on\n");
  sendhci(setspm,0);
    
#ifdef BTFPYTHON
  pycallback = py_lecallback;
#else
  gpar.lecallback = callback;
#endif

  gpar.serveractive = 2;    
  NPRINT "Waiting for any device to connect (x = stop server)\n");
  flushprint();
  
  mesh_on();
  tim0 = time_ms();
  timms = timerds;
  loopt = 0;
  if((gpar.settings & FAST_TIMER) == 0)
    {
    loopt = 10;
    timms *= 100;
    }
  serverexit(1);
  do
    {
    retval = SERVER_CONTINUE;
    key = 0;
    nread = read_all_endchar(&node,buf,1024,endchar,EXIT_KEY | EXIT_TIMEOUT,loopt);
    if(read_error() == ERROR_KEY)
      {
      key = 'x';
      retval = SERVER_EXIT;
      }           
    else if(nread > 0)
      {
      ndevice = devn(node);
      if(ndevice > 0)
        {
        dp = dev[ndevice];
#ifdef BTFPYTHON
        if(pycallback != NULL)
          retval = py_us_callback(pycallback,dp->node,CLASSIC_DATA,0,buf,nread);
#else      
        if(callback != NULL)
          retval = (*callback)(dp->node,CLASSIC_DATA,0,buf,nread);
#endif
        }
      }
      
    if((retval & SERVER_CONTINUE) != 0)
      {
      n = findhci(IN_CONREQ,0,INS_POP);
      if(n >= 0)
        {  // Classic connect
        ndevice = instack[n+3];
        dp = dev[ndevice];
        popins();
        
        dp->conflag = CON_SERVER;  
        dp->linkflag &= KEY_FILE | KEY_NEW;
        dp->linkflag |= keyflag & (KEY_ON | PASSKEY_LOCAL | PASSKEY_REMOTE);
                      
        retval = classicserverx(ndevice);
        mesh_on();
        popins();
        if((dp->conflag & CON_RF) != 0)
          NPRINT "%s has connected\n",dp->name);
        flushprint();          
        }
        
      n = findhci(IN_LECMD,0,INS_POP);
      if(n >= 0)
        {  // LE input
        ndevice = instack[n+3];
        dp = dev[ndevice];  
        op = insdatn[0];
        cticn = insdatn[1];
        if(op == LE_READ)
          readret = insdatn[2];
          
        if(op == LE_DISCONNECT)
          VPRINT "%s has disconnected\n",dp->name);
        else if(op == LE_CONNECT)
          {
          if((gpar.hidflag & 1) != 0)
            {  // HID wait for AUTO_PAIROK
            NPRINT "Wait for pair...\n");  
            readhci(ndevice,IN_AUTOEND,0,gpar.leclientwait,0);
            n = findhci(IN_AUTOEND,ndevice,INS_POP);
            if(n >= 0)
              {
              if(insdat[n] == AUTO_PAIROK)
                NPRINT "PAIR OK\n");
              }
            else
              NPRINT "PAIR time out\n");
            }       

          VPRINT "%s has connected\n",dp->name);
          if((gpar.hidflag & 1) == 0)  
            {
            mesh_on();
            VPRINT "Set larger data length\n");  
            sendhci(datlenset,ndevice);
            }
          }   

        flushprint();
        popins();     
        buf[0] = 0;
   
        if(op == LE_READ)
          retval = readret;  // from leserver callback
        else
          {
#ifdef BTFPYTHON
          if(pycallback != NULL)
            retval = py_us_callback(pycallback,dp->node,op,cticn,buf,0);
#else
          if(callback != NULL)
            retval = callback(dp->node,op,cticn,buf,0);
#endif
          } 
                   
        if(op == LE_DISCONNECT)
          {  // clear all operations from this device
          do
            {
            n = findhci(IN_LECMD,ndevice,INS_POP);
            }
          while(n >= 0); 
      
          popins();
          }
        }
      }
      
    if((retval & SERVER_CONTINUE) != 0 && timerds > 0)
      {
      if(time_ms() - tim0 >= timms)
        {
        buf[0] = 0;

#ifdef BTFPYTHON
        if(pycallback != NULL)
          retval = py_us_callback(pycallback,localnode(),SERVER_TIMER,0,buf,0);
#else
        if(callback != NULL)
          retval = callback(localnode(),SERVER_TIMER,0,buf,0);
#endif
        tim0 = time_ms();
        }
      }   
    }
  while((retval & SERVER_CONTINUE) != 0 && key != 'x');
  serverexit(0);
  gpar.serveractive = 0;    

    
  if(key == 'x')
    NPRINT "Key press - stopping server\n");
         
  flushprint();  
  sleep_ms(2000);    // allow time for any last reply sent by callback to transmit      

  if((retval & SERVER_EXIT_CONNECTED) == 0)
    {
    for(dn = 1 ; devok(dn) != 0  ; ++dn)
      {
      dp = dev[dn];
      if((dp->conflag & (CON_LX | CON_SERVER)) != 0)
        {
        NPRINT "Disconnecting %s\n",dp->name);
        if((dp->conflag & CON_SERVER) != 0)
          read_node_clear(dp->node);
        else
          {
          do
            {
            n = findhci(IN_LECMD,dn,INS_POP);
            }
          while(n >= 0);
          }
        disconnectdev(dn);
        }
      }
    }  
     
  return(1); 
  }


    
/********** CLASSIC SERVER ****************/



int classic_server(int clientnode,int (*callback)(int clientnode,unsigned char *buf,int count),char endchar,int keyflag)
  {
  int n,nread,key,ndevice,retval,oldkm,tryflag,keyflagx;
  char *s;
  struct devdata *dp;
  static unsigned char buf[1024];
#ifdef BTFPYTHON
  PyObject *pycallback;
#endif  

  if(check_init(0) == 0)
    return(0);
    
  if(gpar.serveractive != 0)
    {
    NPRINT "Cannot start a second server\n");
    flushprint();
    return(0);
    }
    
  if(clientnode == ANY_DEVICE)
    {
    ndevice = CL_SERV;
    dp = NULL;
    }
  else
    {  
    ndevice = devn(clientnode);
    if(ndevice <= 0 || !(dev[ndevice]->type == BTYPE_ME || dev[ndevice]->type == BTYPE_CL))
      {
      NPRINT "Invalid client node\n");
      flushprint();
      return(0);
      }
    
    dp = dev[ndevice];
   
    if(dp->conflag != 0)
      {
      NPRINT "Already connected\n");
      flushprint();
      return(0);
      }
    }
    

  VPRINT "Set simple pair mode on\n");
  sendhci(setspm,0);

  flushprint();
  
  oldkm = setkeymode(1);
  keyflagx = keyflag;
  tryflag = 0;
  serverexit(1);
  while(dp == NULL || (dp->conflag & CON_RF) == 0)
    {     
    if(tryflag == 0)
      {
      if(clientnode == ANY_DEVICE)
        s = "any device";
      else
        s = dp->name;
      NPRINT "Listening for %s to connect (x=cancel)\n",s);
      flushprint();
      }
    retval = 0;
    do
      {     
      readhci(ndevice,IN_CONREQ,0,750,0);
      n = findhci(IN_CONREQ,ndevice & 0xFF,INS_POP);
      if(n >= 0)
        {    
        if(ndevice == CL_SERV)
          {
          ndevice = instack[n+3];
          dp = dev[ndevice];
          }
        popins();
        
        dp->conflag = CON_SERVER;  
        dp->linkflag &= KEY_FILE | KEY_NEW;
        dp->linkflag |= keyflagx & (KEY_ON | PASSKEY_LOCAL | PASSKEY_REMOTE);
                        
        retval = classicserverx(ndevice);
        flushprint();
        popins();
        }
      
      if(tryflag != 0 && dp != NULL && (dp->conflag & CON_RF) == 0) 
        {
        tryflag = 0;
        NPRINT "  The remote device has connected and then disconnected.\n");
        NPRINT "  This may be OK - for example when pairing, which may have worked.\n"); 
        NPRINT "  But if pairing/connect failed, press k to change key option.\n");
        NPRINT "  If that fails, unpair on the remote device and try again. If pairing\n");
        NPRINT "  has worked, or if no further operation is expected, press x to stop\n");
        NPRINT "Listening for %s to connect (k=change key option x=stop server)\n",dp->name);
        flushprint();
        }
        
      key = readkey();

      }
    while((dp == NULL || (dp->conflag & CON_RF) == 0) && key != 'x' && key != 'k' && retval != 2);
    
    if(dp == NULL || (dp->conflag & CON_RF) == 0)
      {
      if(retval == 2)
        {
        if(dp != NULL)
          {
          if(dp->type == BTYPE_CL)
            tryflag = 1;
          dp->conflag = 0;
          NPRINT "%s disconnected\n",dp->name);
          flushprint();
          }
        }       
      else
        tryflag = 0;     

      if(key == 'k')
        {
        // flip KEY
        keyflagx ^= KEY_ON;
        if((keyflagx & KEY_ON) == 0)  
          NPRINT "Will not send a link key\n");
        else
          NPRINT "Will send a link key\n");
        flushprint();
        }

      if(key == 'x')
        {
        if(dp != NULL)
          dp->conflag = 0;
        if(key == 'x')
          NPRINT "Cancelled by x press\n");
        flushprint();
        setkeymode(oldkm);
        popins();
        serverexit(0);
        return(0);
        }
      }
    } 
  
  popins();  
  setkeymode(oldkm);

  if(dp == NULL)
    {
    NPRINT "Connect failed\n");
    serverexit(0);
    return(0);
    }
    
#ifdef BTFPYTHON
  pycallback = py_callback;
#endif  
  gpar.serveractive = 1;    
    
  NPRINT "Connected OK\n");
  NPRINT "Waiting for data from %s (x = stop server)\n",dp->name);
 
  retval = SERVER_CONTINUE;
  do
    {
    nread = read_node_endchar(dp->node,buf,1024,endchar,EXIT_KEY,0);
    if(nread > 0)
      {
#ifdef BTFPYTHON
      if(pycallback != NULL)
        retval = py_cmn_callback(pycallback,dp->node,buf,nread);
#else      
      if(callback != NULL)
        retval = (*callback)(dp->node,buf,nread);
#endif
      }
    else
      retval = SERVER_EXIT;
    }
  while((retval & SERVER_CONTINUE) != 0 && read_error() != ERROR_KEY);
 
  serverexit(0);
  gpar.serveractive = 0;    
  
  if(read_error() == ERROR_KEY)
    NPRINT "Key press - stopping server\n");
  else if(read_error() == ERROR_FATAL)
    NPRINT "Fatal error - stopping server\n");
  else if(read_error() == ERROR_DISCONNECT)
    NPRINT "%s has disconnected - stopping server\n",dp->name);
         
  flushprint();  
  sleep_ms(2000);    // allow time for any last reply sent by callback to transmit      

  if((retval & SERVER_EXIT_CONNECTED) == 0)
    disconnect_node(dp->node);  // sever initiated here
                                // client should be running
                                // wait_for_disconnect
  read_node_clear(dp->node);
       
  return(1);
  }  



int classicserverx(int ndevice)
  {
  int n;
  struct devdata *dp;
 
  dp = dev[ndevice];
 
    // accept with role=0 triggers Event 12 role changes
  VPRINT "GOT Connect request (Event 4)\n");
  VPRINT "SEND Accept connection\n");  
  sendhci(conaccept,ndevice);
  
  flushprint();
  
     // connect only if ndevice/IN_CLHAND
  readhci(ndevice,IN_CLHAND,0,gpar.timout,gpar.toshort);

  if((dp->conflag & CON_HCI) == 0)
    {
    VPRINT "Waiting for connect request\n");
    dp->conflag = 0;
    return(0);
    }
  
  flushprint();
  popins();  

  readhci(ndevice,IN_AUTOEND,0,5000,0); 
  n = findhci(IN_AUTOEND,ndevice,INS_POP);
  if(n < 0 || (n >= 0 && insdatn[0] == AUTO_DIS))
    {  // probably just paired or read SDP then disconnected   
    VPRINT "Waiting for connect RFCOMM request\n");
    disconnectdev(ndevice);
    return(2);
    }
   
  // expecting insdat[n] == AUTO_RF
 
  readhci(ndevice,IN_AUTOEND,0,100,0);
  n = findhci(IN_AUTOEND,ndevice,INS_POP);
 
  // expecing insdat[n] == AUTO_MSC

  dp->credits = 0;
  setcredits(ndevice);

  flushprint();
     
  return(1);
  }  
  


/*********** SDP DATABASE ********

All three serial services are on channel 1

 **** RECORD 0 ****  UUID 1200 device info
    aid = 0000
        Handle 00 01 00 00 
    aid = 0001
      UUID 12 00 PnPInformation
    aid = 0005
      UUID 10 02 
    aid = 0009
        UUID 12 00 PnPInformation
        01 03 
    aid = 0200
        01 03 
    aid = 0201
        1D 6B 
    aid = 0202
        02 46 
    aid = 0203
        05 2B 
    aid = 0204
        01 
    aid = 0205
        00 02 

 **** RECORD 1 **** Standard 2-byte serial 1101
    aid = 0000
        Handle 00 01 00 01 
    aid = 0001
      UUID 11 01 SerialPort
    aid = 0004
        UUID 01 00 L2CAP
        UUID 00 03 RFCOMM
        RFCOMM channel = 1
    aid = 0005
      UUID 10 02 
    aid = 0100
        Serial2

 **** RECORD 2 **** Standard 16-byte serial
    aid = 0000
        Handle 00 01 00 02 
    aid = 0001
      UUID 00 00 11 01 00 00 10 00 80 00 00 80 5F 9B 34 FB 
    aid = 0004
        UUID 01 00 L2CAP
        UUID 00 03 RFCOMM
        RFCOMM channel = 1
    aid = 0005
      UUID 10 02 
    aid = 0100
        Serial16

 **** RECORD 3 **** UUID and name set by register_serial()
    aid = 0000
        Handle 00 01 00 03 
    aid = 0001
      UUID FC F0 5A FD 67 D8 4F 41 83 F5 7B EE 22 C0 3C DB 
    aid = 0004
        UUID 01 00 L2CAP
        UUID 00 03 RFCOMM
        RFCOMM channel = 1
    aid = 0005
      UUID 10 02 
    aid = 0100
        My custom serial


 **** RECORD 4 ****
    aid = 0000
        Handle 00 01 00 04 
    aid = 0001
      UUID 11 05 OBEXObjectPush
    aid = 0004
        UUID 01 00 L2CAP
        UUID 00 03 RFCOMM
        RFCOMM channel = 9
        UUID 00 08 OBEX
    aid = 0005
      UUID 10 02 
    aid = 0009
        UUID 11 05 OBEXObjectPush
        01 02 
    aid = 0100
        OBEX server
    aid = 0303
      01 
      02 
      03 
      04 
      05 
      06 
      FF 


*****************************/

void register_serial(unsigned char *uuid,char *name)
  {
  if(uuid != NULL && name != NULL)
    replysdp(0,0,uuid,name);
  else
    NPRINT "NULL parameter\n");
  }
 
void replysdp(int ndevice,int in,unsigned char *uuid,char *name)
  {
  int n,aidlen,rn,uuidflag,aidj,aidk,ln,totlen;
  struct devdata *dp;
  unsigned char *ssarep,*des;
  unsigned char *sdpreply;
    
  static unsigned char uuid1200[5] = { 0x35,0x03,0x19,0x12,0x00 };
  static unsigned char uuid0003[5] = { 0x35,0x03,0x19,0x00,0x03 };
  //static char uuid1101[5] = { 0x35,0x03,0x19,0x11,0x01 };
  static unsigned char uuid0100[5] = { 0x35,0x03,0x19,0x01,0x00 };
  static unsigned char aidone[5] =   { 0x35,0x03,0x09 };
  static unsigned char aidrange[5] = { 0x35,0x05,0x0A };
   
static unsigned char ssaaidfail[32] = { 19,0,S2_HAND | S2_SDP,0,0x02,0x0B,0x20,0x0E,0x00,
0x0A,0x00,0x41,0x00,0x05,0x00,0x00,0x00,0x05,0x00,0x02,0x35,0x00,0x00 }; 
   // 03 reply with handle [21] = handle 
static unsigned char ssahandle[32] = { 23,0,S2_HAND | S2_SDP,0,0x02,0x0B,0x20,0x12,0x00,
0x0E,0x00,0x41,0x00,0x03,0x00,0x00,0x00,0x09,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00 }; 

static unsigned char ssahandle4[40] = { 35,0,S2_HAND | S2_SDP,0,0x02,0x0B,0x20,0x1E,0x00,
0x1A,0x00,0x41,0x00,0x03,0x00,0x00,0x00,0x15,0x00,0x04,0x00,
0x04,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x03,0x00,0x01,0x00,0x04,0x00 };
    
static unsigned char ssahandlefail[32] = { 19,0,S2_HAND | S2_SDP,0,0x02,0x0B,0x20,
0x0E,0x00,0x0A,0x00,0x41,0x00,0x03,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x00,0x00 }; 
  // handle = 00010000  1200
static unsigned char aid12_0[10] = { 8,0x09,0x00,0x00,0x0A,0x00,0x01,0x00,0x00 }; 
static unsigned char aid12_1[10] = { 8,0x09,0x00,0x01,0x35,0x03,0x19,0x12,0x00 };
static unsigned char aid12_5[10] = { 8,0x09,0x00,0x05,0x35,0x03,0x19,0x10,0x02 };
static unsigned char aid12_9[16] = { 13,0x09,0x00,0x09,0x35,0x08,0x35,0x06,0x19,0x12,0x00,0x09,0x01,0x03 };
static unsigned char aid12_200[8] = { 6,0x09,0x02,0x00,0x09,0x01,0x03 };
static unsigned char aid12_201[8] = { 6,0x09,0x02,0x01,0x09,0x1D,0x6B };
static unsigned char aid12_202[8] = { 6,0x09,0x02,0x02,0x09,0x02,0x46 };
static unsigned char aid12_203[8] = { 6,0x09,0x02,0x03,0x09,0x05,0x2B };
static unsigned char aid12_204[8] = { 5,0x09,0x02,0x04,0x28,0x01 };
static unsigned char aid12_205[8] = { 6,0x09,0x02,0x05,0x09,0x00,0x02 }; 
   // [8] = handle
static unsigned char aid0[10] = { 8,
0x09,0x00,0x00,0x0A,0x00,0x01,0x00,0x01 };
   // 2-byte stndard uuid 1101
static unsigned char aid1_2[10] = { 8,
0x09,0x00,0x01,0x35,0x03,0x19,0x11,0x01 };
   // 2-byte uuid 1105
static unsigned char aid1_4[10] = { 8,
0x09,0x00,0x01,0x35,0x03,0x19,0x11,0x05 };
   // 16-byte  uuid 1105 
static unsigned char obex16[20] = { 
0x35,0x11,0x1C,0x00,0x00,0x11,0x05,0x00,0x00,0x10,0x00,0x80,0x00,
0x00,0x80,0x5F,0x9B,0x34,0xFB };
   // 16-byte standard uuid 
static unsigned char aid1_16[24] = { 22,
0x09,0x00,0x01,0x35,0x11,0x1C,0x00,0x00,0x11,0x01,0x00,0x00,0x10,0x00,0x80,0x00,
0x00,0x80,0x5F,0x9B,0x34,0xFB };
   // 16-byte custom uuid
   // [7..] = uuid
static unsigned char aid1_c[24] = { 22,
0x09,0x00,0x01,0x35,0x11,0x1C,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,
0xBB,0xCC,0xDD,0xEE,0xFF,0x00 };
  // [17] = channel = 1 for handles 1/2/3
static unsigned char aid4[20] = { 17,
0x09,0x00,0x04,0x35,0x0C,0x35,0x03,0x19,0x01,0x00,0x35,0x05,0x19,0x00,0x03,0x08,0x01 };
  // [17] = channel = 1 for handle 4 
static unsigned char aid4_obex[24] = { 22,
0x09,0x00,0x04,0x35,0x11,0x35,0x03,0x19,0x01,0x00,0x35,0x05,0x19,0x00,0x03,0x08,0x02,0x35,0x03,0x19,0x00,0x08 };
static unsigned char aid5[10] = { 8,
0x09,0x00,0x05,0x35,0x03,0x19,0x10,0x02 };

static unsigned char aid9_obex[16] = { 13,
0x09,0x00,0x09,0x35,0x08,0x35,0x06,0x19,0x11,0x05,0x09,0x01,0x02 };
  // [0]   = len+5
  // [5]   = len
  // [6..] = custom name
static unsigned char aid0100_c[64] = { 9,
0x09,0x01,0x00,0x25,0x04,0x61,0x61,0x61,0x61 };
  // NAME = Serial2
static unsigned char aid0100_s2[16] = { 12,
0x09,0x01,0x00,0x25,0x07,0x53,0x65,0x72,0x69,0x61,0x6C,0x32 };
  // Serial16
static unsigned char aid0100_s16[16] = { 13,
0x09,0x01,0x00,0x25,0x08,0x53,0x65,0x72,0x69,0x61,0x6C,0x31,0x36 };
  // Object push
static unsigned char aid0100_obex[18] = { 16,
0x09,0x01,0x00,0x25,0x0B,0x4F,0x42,0x45,0x58,0x20,0x73,0x65,0x72,0x76,0x65,0x72 };
  // OBEX formats
static unsigned char aid0303_obex[20] = { 19,
0x09,0x03,0x03,0x35,0x0E,0x08,0x01,0x08,0x02,0x08,0x03,0x08,0x04,0x08,0x05,0x08,0x06,0x08,0xFF };
  // sdpreply
  // [0] = totlen+19 
  // PAKHEADSIZE+
  // [3][4] = totlen+14
  // [5][6] = totlen+10
  // [9] = 05 or 07
  // [10][11] = id
  // [13] = totlen+5
  // [15] = totlen+2
static unsigned char sdpreply0[24] = { 78,0,S2_HAND | S2_SDP,0,
0x02,0x0C,0x20,0x49,0x00,0x45,0x00,0x44,0x00,0x07,0x00,0x01,0x00,0x40,0x00,0x3D,0x35 }; 


  sdpreply = gpar.buf;
   
  if(uuid != NULL && name != NULL)
    {
    for(rn = 0 ; rn < 16 ; ++rn)
      {
      aid1_c[rn+7] = uuid[rn];
      custuuid[rn] = uuid[rn];
      }
    strcpy(custname,name);
    totlen = strlen(name);
    if(totlen > 40)
      totlen = 40;
    for(rn = 0 ; rn < totlen ; ++rn)
      aid0100_c[rn+6] = name[rn];
    aid0100_c[0] = totlen+5;
    aid0100_c[5] = totlen;
    return;
    }

  dp = dev[ndevice]; 
  dp->id = (insdat[in+1] << 8) + insdat[in+2];  // hi first
     
  VPRINT "GOT SDP data request\n");
  
  uuidflag = -1;
   
  if(insdat[in] == 0x02 || insdat[in] == 0x06)
    {  // uuid des
    des = insdat+in+5;  // search des from initial 35
    if(bincmp(des,aid1_2+4,5,DIRN_FOR) != 0)
      uuidflag = 1;   // 1101 2 records
    else if(bincmp(des,aid1_16+4,19,DIRN_FOR) != 0)
      uuidflag = 2;   // standard 16
    else if(bincmp(des,aid1_c+4,19,DIRN_FOR) != 0)
      uuidflag = 3;  // custom  
    else if((gpar.settings & ENABLE_OBEX) != 0 && bincmp(des,aid1_4+4,5,DIRN_FOR) != 0)
      uuidflag = 4;  // obex 2
    else if((gpar.settings & ENABLE_OBEX) != 0 && bincmp(des,obex16,19,DIRN_FOR) != 0)
      uuidflag = 4;  // obex 16  
    else if(bincmp(des,uuid0003,5,DIRN_FOR) != 0 ||     
            bincmp(des,uuid0100,5,DIRN_FOR) != 0 )
      uuidflag = 5;  // 4 records     
    else if(bincmp(des,uuid1200,5,DIRN_FOR) != 0)
      uuidflag = 0;   // 1200
    }
    
  aidj = -1;
  aidk = -1;
  
  if(insdat[in] == 0x04 || insdat[in] == 0x06)
    {  // aid des
    if(insdat[in] == 0x04)
      des = insdat+in+11;
    else
      {
      des = insdat+in+12;
      if(insdat[in+6] == 0x11)
        des += 14;  // 16-byte uuid
      } 
    aidj = (des[3] << 8)+des[4];      
    if(bincmp(des,aidone,3,DIRN_FOR) != 0)
      aidk = aidj;
    else if(bincmp(des,aidrange,3,DIRN_FOR) != 0)
      aidk = (des[5] << 8)+des[6];
   
        
    if(insdat[in] == 0x04)
      {  // handle specified  0/1/2/3/4
      uuidflag = insdat[in+8];
      }      
    }
 
                                       
  if(insdat[in] == 0x02)
    {  // handle request
    VPRINT "SEND handle that matches UUID\n");
    if(uuidflag < 0)
      {
      VPRINT "  No UUID match - send no data reply\n");
      ssarep = ssahandlefail;   
      }
    else if(uuidflag == 5)
      ssarep = ssahandle4;  // 1,2,3,4 
    else
      {  
      ssarep = ssahandle; 
      ssahandle[PAKHEADSIZE+21] = uuidflag;  // 00010000-03
      }
    }
  else
    {  // 04/06 aid request
    if(uuidflag < 0 || aidj < 0 || aidk < 0)
      {
      VPRINT "SEND no data reply\n");
      ssarep = ssaaidfail;
      }
    else  
      {
      VPRINT "SEND SDP reply\n");
      for(n = 0 ; n < 21 ; ++n)
        sdpreply[n] = sdpreply0[n];
        
      ssarep = sdpreply;
      rn = 16;
      totlen = 0;
      if(insdat[in] == 0x06)
        {
        rn += 3;
        sdpreply[PAKHEADSIZE+16] = 0x36;
        sdpreply[PAKHEADSIZE+17] = 0;    
        sdpreply[PAKHEADSIZE+18] = 0;    
        }
      else
        {
        sdpreply[PAKHEADSIZE+16] = 0x35;
        sdpreply[PAKHEADSIZE+17] = 0; 
        }
           
      if(uuidflag == 0)
        {  // 1200 only 
        sdpreply[PAKHEADSIZE+rn] = 0x35;
        ln = rn;
        rn += 2;
        //   aidlen = addaid(aid1200,&rn,0,2,1); // handle 0
                    
        aidlen = addaid(sdpreply,aid12_0,&rn,aidj,aidk,0);
        aidlen += addaid(sdpreply,aid12_1,&rn,aidj,aidk,1);
        aidlen += addaid(sdpreply,aid12_5,&rn,aidj,aidk,5);
        aidlen += addaid(sdpreply,aid12_9,&rn,aidj,aidk,9);
        aidlen += addaid(sdpreply,aid12_200,&rn,aidj,aidk,0x200);
        aidlen += addaid(sdpreply,aid12_201,&rn,aidj,aidk,0x201);
        aidlen += addaid(sdpreply,aid12_202,&rn,aidj,aidk,0x202);
        aidlen += addaid(sdpreply,aid12_203,&rn,aidj,aidk,0x203);
        aidlen += addaid(sdpreply,aid12_204,&rn,aidj,aidk,0x204);
        aidlen += addaid(sdpreply,aid12_205,&rn,aidj,aidk,0x205);
                  
        sdpreply[PAKHEADSIZE+ln+1] = aidlen;
        if(aidlen == 0)
          rn = ln;
        else
          totlen += aidlen+2;
        }
        
      if(uuidflag == 1 || uuidflag == 5)
        {  // standard 2-byte
        sdpreply[PAKHEADSIZE+rn] = 0x35;
        ln = rn;
        rn += 2;
        aid0[8] = 1;  // handle 1
        aidlen = addaid(sdpreply,aid0,&rn,aidj,aidk,0);
        aidlen += addaid(sdpreply,aid1_2,&rn,aidj,aidk,1);
           // aid4[17] = channel
        aidlen += addaid(sdpreply,aid4,&rn,aidj,aidk,4);
        aidlen += addaid(sdpreply,aid5,&rn,aidj,aidk,5);
        aidlen += addaid(sdpreply,aid0100_s2,&rn,aidj,aidk,0x0100);
        sdpreply[PAKHEADSIZE+ln+1] = aidlen;
        if(aidlen == 0)
          rn = ln;
        else
          totlen += aidlen+2;
        }
      if(uuidflag == 1 || uuidflag == 2 || uuidflag == 5)
        {  // standard 16-byte
        sdpreply[PAKHEADSIZE+rn] = 0x35;
        ln = rn;
        rn += 2;
        aid0[8] = 2;  // handle 2
        aidlen = addaid(sdpreply,aid0,&rn,aidj,aidk,0);
        aidlen += addaid(sdpreply,aid1_16,&rn,aidj,aidk,1);
           // aid4[17] = channel
        aidlen += addaid(sdpreply,aid4,&rn,aidj,aidk,4);
        aidlen += addaid(sdpreply,aid5,&rn,aidj,aidk,5);
        aidlen += addaid(sdpreply,aid0100_s16,&rn,aidj,aidk,0x0100);
        sdpreply[PAKHEADSIZE+ln+1] = aidlen;
        if(aidlen == 0)
          rn = ln;
        else
          totlen += aidlen+2;
        }
      if(uuidflag == 3 || uuidflag == 5)
        {  // custom 16-byte
        sdpreply[PAKHEADSIZE+rn] = 0x35;
        ln = rn;
        rn += 2;
        aid0[8] = 3;  // handle 3 
        aidlen = addaid(sdpreply,aid0,&rn,aidj,aidk,0);
        aidlen += addaid(sdpreply,aid1_c,&rn,aidj,aidk,1);
            // aid4[17] = channel
        aidlen += addaid(sdpreply,aid4,&rn,aidj,aidk,4);
        aidlen += addaid(sdpreply,aid5,&rn,aidj,aidk,5);
        aidlen += addaid(sdpreply,aid0100_c,&rn,aidj,aidk,0x0100);
        sdpreply[PAKHEADSIZE+ln+1] = aidlen;
        if(aidlen == 0)
          rn = ln;
        else
          totlen += aidlen+2;
        }  
      if((uuidflag == 4 || uuidflag == 5) && (gpar.settings & ENABLE_OBEX) != 0)
        {  // OBEX
        sdpreply[PAKHEADSIZE+rn] = 0x35;
        ln = rn;
        rn += 2;
        aid0[8] = 4;  // handle 4
        aidlen = addaid(sdpreply,aid0,&rn,aidj,aidk,0);
        aidlen += addaid(sdpreply,aid1_4,&rn,aidj,aidk,1);
           // aid4[17] = channel
        aidlen += addaid(sdpreply,aid4_obex,&rn,aidj,aidk,4);
        aidlen += addaid(sdpreply,aid5,&rn,aidj,aidk,5);
        aidlen += addaid(sdpreply,aid9_obex,&rn,aidj,aidk,9);
        aidlen += addaid(sdpreply,aid0100_obex,&rn,aidj,aidk,0x0100);
        aidlen += addaid(sdpreply,aid0303_obex,&rn,aidj,aidk,0x0303);
        sdpreply[PAKHEADSIZE+ln+1] = aidlen;
        if(aidlen == 0)
          rn = ln;
        else
          totlen += aidlen+2;
        }
      if(insdat[in] == 0x06)
        {
        sdpreply[PAKHEADSIZE+9] = 0x07;
        sdpreply[PAKHEADSIZE+17] = (totlen >> 8) & 0xFF;
        sdpreply[PAKHEADSIZE+18] = totlen & 0xFF;
        ++totlen;
        }
      else
        {
        sdpreply[PAKHEADSIZE+9] = 0x05;
        totlen = sdpreply[PAKHEADSIZE+17];
        }
               
      sdpreply[PAKHEADSIZE+rn] = 0;  // continue
        
      ln = totlen+2;
      sdpreply[PAKHEADSIZE+14] = (ln >> 8) & 0xFF;
      sdpreply[PAKHEADSIZE+15] = ln & 0xFF;
      ln = totlen + 5;
      sdpreply[PAKHEADSIZE+12] = (ln >> 8) & 0xFF;
      sdpreply[PAKHEADSIZE+13] = ln & 0xFF;
      ln = totlen+10;
      sdpreply[PAKHEADSIZE+5] = ln & 0xFF;
      sdpreply[PAKHEADSIZE+6] = (ln >> 8) & 0xFF;
      ln = totlen+14;
      sdpreply[PAKHEADSIZE+3] = ln & 0xFF;
      sdpreply[PAKHEADSIZE+4] = (ln >> 8) & 0xFF;
      totlen += 19;
      sdpreply[0] = totlen & 0xFF;
      sdpreply[1] = (totlen >> 8) & 0xFF;
      }
    }
     
  flushprint();
  sendhci(ssarep,ndevice);  
  }

int addaid(unsigned char *sdp,unsigned char *aid,int *rn,int aidj,int aidk,int aidn)
  {
  int n;
  
  if(aidn < aidj || aidn > aidk)
    return(0);
    
  for(n = 0 ; n < aid[0] ; ++n)
    sdp[PAKHEADSIZE+*rn+n] = aid[n+1];
  *rn += aid[0];
  return(aid[0]);
  }

  
int bincmp(unsigned char *s,unsigned char *t,int count,int dirn)
  {
  int n;
  
  if(dirn == 0)
    {
    for(n = 0 ; n < count ; ++n)
      {
      if(s[n] != t[n])
        return(0);
      }
    return(1);
    }

  // reverse
  for(n = 0 ; n < count ; ++n)
    {
    if(s[n] != t[count-1-n])
      return(0);
    }
  return(1);   
  }  

  
/***********  CONNECT LE DEVICE index ndevice *******************/

  
int leconnect(int ndevice)
  {
  struct devdata *dp;
  int flag;
  
     // ndevice checked
        
  dp = dev[ndevice];
  
     // if ndevice is BTYPE_LE - connect as LE client to LE server
     // if nedevce is BTYPE_ME - a Pi mesh device:
     //      dp->lecflag = 0  connect as node client to mesh device listening as node server
     //                    1  connect as LE client to mesh device listening as LE server
  
  if(dp->type == BTYPE_LE || (dp->type == BTYPE_ME && dp->lecflag != 0))
    {
    flag = 1;  // LE client
    leopen[PAKHEADSIZE+17] = gpar.leintervalmin & 0xFF;
    leopen[PAKHEADSIZE+18] = (gpar.leintervalmin >> 8) & 0xFF;
    leopen[PAKHEADSIZE+19] = gpar.leintervalmax & 0xFF;
    leopen[PAKHEADSIZE+20] = (gpar.leintervalmax >> 8) & 0xFF;
    }
   else
    {
    flag = 0;  // node client
    leopen[PAKHEADSIZE+17] = 6;
    leopen[PAKHEADSIZE+18] = 0;
    leopen[PAKHEADSIZE+19] = 6;
    leopen[PAKHEADSIZE+20] = 0;
    }
    
  mesh_off();
  
  VPRINT "SEND LE connect to %s\n",dp->name);

  dp->setdatlen = 20;
  dp->cryptoflag = 0;
     
  sendhci(leopen,ndevice);
      
  readhci(ndevice,IN_LEHAND,0,5000,gpar.toshort);   
  
  if(dp->conflag != 0)
    {    // IN_LEHAND not saved to stack
    if(flag == 0)
      NPRINT "Connect OK as NODE client\n");
    else
      NPRINT "Connect OK as LE client\n"); 

    VPRINT "Handle = %02X%02X\n",dp->dhandle[1],dp->dhandle[0]);

    setlelen(ndevice,LEDATLEN,flag);    
   
      // if(dev[ndevice]->type != BTYPE_ME && gpar.leclientwait > 0)
    if(gpar.leclientwait > 0)
      readhci(0,0,0,gpar.leclientwait,0);  // server may request attributes

    popins();
    flushprint(); 
    return(1);
    }      
  
  sendhci(lecancel,0);  // cancel open command
  // statusok(1,lecancel);         // may also return IN_LEHAND with fail status
  dp->lecflag = 0;
  
  NPRINT "Fail - no handle\n");
  if(dp->type == BTYPE_LE && (dp->leaddtype == 0 || (dp->leaddtype & 1) != 0))
    {
    if(dp->leaddtype == 0)   // on user list - may be random
      NPRINT "This device may have a random board address which changes\n");
    else                     // found by scan and is random
      NPRINT "This device has a random board address which may have changed\n");
   
    NPRINT "Scan for LE devices to find the current address\n");
    }
  flushprint();
  return(0);      
  }


int le_pair(int node,int flags,int passkey)
  {  
  int n,ndevice,scflag,sreqflag;
  struct devdata *dp;

  if(check_init(0) == 0)
    return(0);
   
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);
  dp = dev[ndevice];

  sreqflag = 0;
  if(dp->conflag == CON_LX)
    {  // server - request security  
    if(ndevice == 0 || dp->cryptoflag != 0)
      {
      NPRINT "le_pair ignored\n");
      flushprint();
      return(0);
      }
    VPRINT "Request security\n"); 
    sreqflag = 1;
    }
  
  if(sreqflag == 0 && ndevice != 0 && dp->conflag != CON_LE) 
    {  // client
    NPRINT "Pair %s - not connected as LE server\n",dp->name);
    flushprint();
    return(0);
    }

  if(sreqflag != 0)
    dp = dev[0];  // ndevice still remote
    
  dp->lepairflags = flags;
  dp->lepasskey = 0;
  if((flags & PASSKEY_FIXED) != 0)
    {
    if(passkey < 0 || passkey > 999999)
      {
      NPRINT "Invalid passkey 1-999999\n");
      flushprint();
      return(0);
      }
    dp->lepasskey = passkey;
    }
      
  if(ndevice == 0 || sreqflag != 0)  // server
    {
    dp->lepairflags &= AUTHENTICATION_ON | PASSKEY_FIXED | JUST_WORKS | SECURE_CONNECT;
    //######if(dp->lepairflags != flags)
    //  NPRINT "Server flag ignored in le_pair\n");
    if((dp->lepairflags & PASSKEY_FIXED) != 0)
      dp->lepasskey = passkey;
    if(sreqflag != 0)
      {  // ask client to initiate security
      leservsecreq[PAKHEADSIZE+10] = 0;
      if((flags & SECURE_CONNECT) != 0)
        leservsecreq[PAKHEADSIZE+10] |= 8;
      if((flags & (PASSKEY_FIXED | PASSKEY_RANDOM)) != 0)
        leservsecreq[PAKHEADSIZE+10] |= 4;
      if((flags & (BOND_NEW | BOND_REPAIR)) != 0)
        leservsecreq[PAKHEADSIZE+10] |= 1;
      sendhci(leservsecreq,ndevice);      
      }
    return(1);
    }
  
  if((dp->lepairflags & BOND_REPAIR) != 0)
    {
    if((dp->linkflag & (PAIR_NEW | PAIR_FILE)) == 0)
      {
      NPRINT "No bond information - cannot re-pair\n");
      flushprint();
      return(0);
      }
    if((dp->linkflag & PAIR_FILE) != 0)
      rwlinkey(4,ndevice,NULL); // used
    
    dp->lepairflags = BOND_REPAIR | PASSKEY_RANDOM | PASSKEY_REMOTE;    
    lerepair[PAKHEADSIZE+14] = dp->divrand[0];
    lerepair[PAKHEADSIZE+15] = dp->divrand[1];
    for(n = 0 ; n < 8 ; ++n)
      lerepair[PAKHEADSIZE+n+6] = dp->divrand[n+2];
        
    for(n = 0 ; n < 16 ; ++n)
      lerepair[PAKHEADSIZE+n+16] = dp->linkey[n];  

    sendhci(lerepair,ndevice);
    }
  else
    {  
    checkpairflags(&dp->lepairflags);
    
    preq[PAKHEADSIZE+10] = 3;  // No io - just works
    preq[PAKHEADSIZE+12] = 0;  // no bond, legacy
    preq[PAKHEADSIZE+15] = 1;  // link

    if((dp->lepairflags & (PASSKEY_RANDOM | PASSKEY_FIXED | PASSKEY_LOCAL | PASSKEY_REMOTE)) != 0)
      {
      preq[PAKHEADSIZE+12] = 0x04;  // passkey     
      preq[PAKHEADSIZE+10] = 0;     // D LOCAL
      if((dp->lepairflags & PASSKEY_REMOTE) != 0)
        preq[PAKHEADSIZE+10] = 4;  // D+K
      }

    if((dp->lepairflags & SECURE_CONNECT) != 0)
      {
      scflag = 0;
      for(n = 0 ; n < 32 && scflag == 0 ; ++n)
        scflag |= dev[0]->pubkeyx[n];
      if(scflag != 0)
        preq[PAKHEADSIZE+12] |= 8; // sc
      else 
        NPRINT "No keys - SECURE_CONNECT cancelled\n");
      }
      
    if((dp->lepairflags & (BOND_NEW | BOND_REPAIR)) != 0)
      preq[PAKHEADSIZE+12] |= 1; // bond 
      
    for(n = 0 ; n < 7 ; ++n)
      dp->preq[n] = preq[n+PAKHEADSIZE+9];          

    sendhci(preq,ndevice);
    } 

  readhci(ndevice,IN_AUTOEND,0,gpar.leclientwait,0);
  n = findhci(IN_AUTOEND,ndevice,INS_POP);
  popins();

  if(n >= 0 && insdat[n] == AUTO_PAIROK)
    {
    NPRINT "PAIR OK\n");
    flushprint();
    readhci(0,0,0,100,0);  // read codes 6/7
    return(1);
    }
  else if(n < 0)
    NPRINT "PAIR time out (try larger set_le_wait)\n");
  else
    NPRINT "PAIR FAIL\n");  
  flushprint();
  return(0);
  }

void checkpairflags(int *flags)
  {
  int n;

  if(*flags == 0)
    {
    *flags = JUST_WORKS;
    return;
    }
      
  if((*flags & JUST_WORKS) != 0)
    {
    *flags &= (JUST_WORKS | BOND_NEW | BOND_REPAIR | SECURE_CONNECT);
    return;
    }
         
  if((*flags & (PASSKEY_RANDOM | PASSKEY_FIXED | PASSKEY_LOCAL | PASSKEY_REMOTE)) != 0)
    {
    n = PASSKEY_RANDOM | PASSKEY_FIXED;
    if((*flags & n) == 0)
      {
      NPRINT "WARNING - No PASSKEY_ type\n");
      *flags |= PASSKEY_RANDOM;
      }
    else if((*flags & n) == n)
      {
      NPRINT "WARNING - Two PASSKEY_ types\n");
      *flags &= ~PASSKEY_FIXED;
      }
    
    n = PASSKEY_LOCAL | PASSKEY_REMOTE;
    if((*flags & n) == 0)
      {
      *flags |= PASSKEY_LOCAL;
      NPRINT "WARNING - No PASSKEY_ device\n");
      }
    else if((*flags & n) == n)
      {
      NPRINT "WARNING - Two PASSKEY_ devices\n");
      *flags &= ~PASSKEY_REMOTE;
      }
    }       
  }

int set_le_interval(int min,int max)
  {
  if(check_init(0) == 0)
    return(0); 
  if(min < 0x0006 || min > 0x0C80 || max < 0x0006 || max > 0x0C80 || min > max)
    {
    NPRINT "Invalid intervals\n");
    flushprint();
    return(0);
    } 
  gpar.leintervalmin = min;
  gpar.leintervalmax = max;
  return(1);
  }
  

int set_le_interval_update(int node,int min,int max)
  {
  int ndevice,cflag;

  if(check_init(0) == 0)
    return(0);
 
  if(min < 0x0006 || min > 0x0C80 || max < 0x0006 || max > 0x0C80 || min > max)
    {
    NPRINT "Invalid intervals\n");
    flushprint();
    return(0);
    } 
    
  ndevice = devn(node);
  if(ndevice <= 0)
    return(0);
    
  cflag = dev[ndevice]->conflag;
  if((cflag & (CON_LE | CON_LX | CON_MESH)) == 0)
    return(0);  
     
  // request interval     
  leupdate[PAKHEADSIZE+6] = min & 0xFF;
  leupdate[PAKHEADSIZE+7] = (min >> 8) & 0xFF;
  leupdate[PAKHEADSIZE+8] = max & 0xFF;
  leupdate[PAKHEADSIZE+9] = (max >> 8) & 0xFF;
  sendhci(leupdate,ndevice);
  readhci(0,0,0,250,0);
  return(1);
  }  


int set_le_interval_server(int node,int min,int max)
  {
  int ndevice,cflag;

  if(check_init(0) == 0)
    return(0);
 
  if(min < 0x0006 || min > 0x0C80 || max < 0x0006 || max > 0x0C80 || min > max)
    {
    NPRINT "Invalid intervals\n");
    flushprint();
    return(0);
    } 
    
  ndevice = devn(node);
  if(ndevice <= 0)
    return(0);
    
  cflag = dev[ndevice]->conflag;
  if((cflag & (CON_LE | CON_LX | CON_MESH)) == 0)
    return(0);  
     
  // request interval via chan 5     
  leconnup[PAKHEADSIZE+13] = min & 0xFF;
  leconnup[PAKHEADSIZE+14] = (min >> 8) & 0xFF;
  leconnup[PAKHEADSIZE+15] = max & 0xFF;
  leconnup[PAKHEADSIZE+16] = (max >> 8) & 0xFF;
  sendhci(leconnup,ndevice);
  readhci(0,0,0,250,0);
  return(1);
  }  
  

/*********** WAIT FOE DISCONNECT ***********
expecting remote server to initiate disconnection by sending
an IN__DISC packet
Wait for readhci() to deal with it
timout = time out in ms
If times out - forces disconnect from local device
*******************************************/

int wait_for_disconnect(int node,int timout)
  {
  int ndevice;
  unsigned long long timstart;

  if(check_init(0) == 0)
    return(0);
 
  flushprint();
    
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);

  timstart = time_ms();
  
  while(dev[ndevice]->conflag != 0 && time_ms() - timstart < (unsigned long long)timout)
    {
    readhci(0,0,0,25,0);  
    flushprint();
    }
   
    
  // conflag should be 0 - call disconnect to be sure  
    
  disconnectdev(ndevice);

  // conflag is 0
  flushprint();
  popins();
  return(1);
  }


int disconnect_node(int node)
  {
  int ndevice;

  if(check_init(0) == 0)
    return(0);
  
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);
    
  return(disconnectdev(ndevice));
  }

int disconnectdev(int ndevice)
  {
  int meshflag;
  struct devdata *dp;
  
  if(devok(ndevice) == 0)
    return(0);  
   
  dp = dev[ndevice];
 
  if(dp->conflag == 0)
    return(1);  // already disconnected         
    
  if(ndevice == 0)   // local 
    {
    return(1);
    }     
 
  if((dp->conflag & CON_MESH) != 0)
    meshflag = 1;
  else
    meshflag = 0;

  // HCI connected

       
  if((dp->conflag & CON_RF) != 0)  // RFCOMM
    {
    if((dp->conflag & CON_SERVER) == 0) 
      NPRINT "Remote device may think it is still connected\n");
    VPRINT "SEND Close RFCOMM request (opcode 53)\n");
    sendhci(closereqrf,ndevice);
    dp->conflag &= ~CON_RF;  
    readhci(ndevice,IN_RFUA,0,gpar.timout,gpar.toshort);
    if(findhci(IN_RFUA,ndevice,INS_POP) >= 0)
      VPRINT "GOT UA reply (opcode 73)\n");
    }
    
  if((dp->conflag & CON_CH0) != 0)  // CONTROL
    {
    VPRINT "SEND Close CONTROL request (opcode 53)\n");
    sendhci(closereq0,ndevice);
    dp->conflag &= ~CON_CH0;  
    readhci(ndevice,IN_RFUA,0,gpar.timout,gpar.toshort);
    if(findhci(IN_RFUA,ndevice,INS_POP) >= 0)
      VPRINT "GOT UA reply (opcode 73)\n");
    }
    
  if((dp->conflag & CON_PSM3) != 0)  
    disconnectl2(ndevice,3);
    
  if((dp->conflag & CON_PSM1) != 0)  
    disconnectl2(ndevice,1);
  
     
  if((dp->conflag & (CON_HCI | CON_LE | CON_LX | CON_MESH)) != 0)   // HCI   
    {
    VPRINT "SEND Close connection\n");
    sendhci(bluclose,ndevice);      

    waitdis(ndevice,5000);  // wait for dp->conflag=0 set by readhci
    
    
    if(dp->conflag == 0)
      VPRINT "GOT Disconnected OK (Event 05)\n");
    else
      VPRINT "No disconnect OK (Event 05)\n");
  
      
    if(meshflag != 0)
      mesh_on();
    }           
   
  popins();
  
  if(dp->conflag != 0 && !((dp->conflag & CON_RF) == 0 && (dp->conflag & CON_SERVER) != 0))    
    NPRINT "%s disconnected\n",dp->name);

  dp->conflag = 0;
  dp->lecflag = 0;
  dp->setdatlen = 20;
  
  flushprint();
  
  return(1);
  }
  



void waitdis(int ndevice,unsigned int timout)
  {
  unsigned long long timstart;
  struct devdata *dp;
 
  // wait for dp->conflag=0 set by readhci
  
  dp = dev[ndevice];
  timstart = time_ms();
  
  while(dp->conflag != 0 && time_ms() - timstart < (unsigned long long)timout)
      readhci(0,0,0,20,0);   // sets conflag=0 on event 05 
  }
  

/********** CLOSE SOCKETS ***********/

void close_all()
  {
  int n;

  if(check_init(0) == 0)
    return;
  
  if(check_init(5) != 0)
    return;
     
  if(gpar.hidflag != 0)
    sendhci(leadparam,0);  // nix random
    
  meshreadoff();   
  mesh_off();
         
  for(n = 0 ; devok(n) != 0 ; ++n)
    disconnectdev(n);  
    
  closehci();
  rwlinkey(1,0,NULL);  // write data
  rwlinkey(2,0,NULL);  // free table
  flushprint();
  check_init(3);
  // printins();            
  }  

  
/***********  WRITE CHARACTERISTIC *****************
device = device index in dev[ndevice]
cticn = characteristic index in dev[ndevice].ctic[cticn]
data = array of bytes to write - low byte first 
count=0 use stored size
*****************/

int write_ctic(int node,int cticn,unsigned char *data,int count)
  {
  if(check_init(0) == 0)
    return(0);  
  return(writecticx(node,cticn,data,count,0,NULL));
  }


int notify_ctic(int node,int cticn,int notifyflag,int (*callback)())
  {
  unsigned char data[2];

  if(check_init(0) == 0)
    return(0);
   
  if(!(notifyflag == NOTIFY_ENABLE || notifyflag == NOTIFY_DISABLE))
    {
    NPRINT "Invalid notify flag\n");
    return(0);
    }
   
  if(writecticx(node,cticn,data,2,notifyflag,callback) == 0)
    return(0);
  return(1);
  }
  

int writecticx(int node,int cticn,unsigned char *data,int count,int notflag,int (*callback)())
  {
  struct cticdata *cp;  // characteristic info structure
  struct devdata *dp;
  int n,k,devn,chandle,locsize,ndevice,flag;
  unsigned char *cmd;
  
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);

  dp = dev[ndevice];
  
  if(ndevice == 0)
    {  // local
    cp = ctic(0,cticn);
    if(cp->type != CTIC_ACTIVE)
      {
      NPRINT "Invalid local characteristic index in write_ctic\n");
      flushprint();
      return(0);
      }
      
    if(notflag != 0 && (cp->perm & 0x30) == 0)
      {
      NPRINT "Characteristic has no notify/indicate permission\n");
      flushprint();
      return(0);
      }

    if(count <= 0)
      locsize = cp->origsize;  // devices.txt
    else
      locsize = count;

    if(locsize > LEDATLEN)
      locsize = LEDATLEN;
  
    if(notflag == NOTIFY_ENABLE)
      cp->notify = 1;  // and send existing value - not data[]
    else if(notflag == NOTIFY_DISABLE)
      cp->notify = 0;
    else if(notflag == 0)
      {
      for(n = 0 ; n < locsize ; ++n)
        cp->value[n] = data[n];
      for(n = locsize ; n < LEDATLEN ; ++n)
        cp->value[n] = 0;  // pad zero
      cp->size = locsize;
      }
  
    flag = 0;  
    for(devn = 1 ; cp->notify != 0 && devok(devn) != 0 ; ++devn)
      {  // search all other devices for notify
      dp = dev[devn];
      if((dp->conflag & CON_LX) != 0 && (gpar.notifynode == 0 || gpar.notifynode == dp->node))
        {  // device devn is connected as LE client
           // send notification
        if(flag == 0)
          {  // first loop only
          flag = 1;
          cmd = lenotify + PAKHEADSIZE;         
     
          if((cp->perm & 0x10) != 0)
            {   
            VPRINT "Send %s notification to %s\n",cp->name,dp->name);
            cmd[9] = 0x1B;
            }
          else
            {   
            VPRINT "Send %s indication to %s\n",cp->name,dp->name);
            cmd[9] = 0x1D;
            }          
       
          for(k = 0 ; k < locsize ; ++k)     // set data
            cmd[12+k] = data[k];     // low byte first

          cmd[3] = (locsize+7) & 0xFF;
          cmd[4] = ((locsize+7) >> 8) & 0xFF;
  
          cmd[5] = (locsize+3) & 0xFF;
          cmd[6] = ((locsize+3) >> 8) & 0xFF;
  
          cmd[10] = (unsigned char)(cp->chandle & 0xFF);
          cmd[11] = (unsigned char)((cp->chandle >> 8) & 0xFF);
    
          lenotify[0] = (unsigned char)((12+locsize) & 0xFF);  // 13 for 1 byte
          lenotify[1] = (unsigned char)(((12+locsize) >> 8) & 0xFF);  // 13 for 1 byte
          }
        
        
        sendhci(lenotify,devn);
     
        if((cp->perm & 0x20) != 0)
          {  // wait for ack
          readhci(devn,IN_LEACK,0,gpar.timout,0);
          n = findhci(IN_LEACK,devn,INS_POP);  
          if(n >= 0) 
            {
            if(insdatn[0] == 0x01)
              {     
              k = insdatn[4]; 
              if(k > 19)
                k = 0;  // no errorle text
              NPRINT "  Error %d %s\n",k,errorle[k]);
              }
            popins();
            }
          else
            NPRINT "Indicate response not seen from %s\n",dev[devn]->name);
          }         
        }     
      }
    
    if(flag != 0) 
      readhci(0,0,0,0,0); 
    return(locsize);
    }



  if((dp->conflag & CON_LE) == 0)
    {
    NPRINT "%s not connected as LE server\n",dp->name);
    flushprint();
    return(0);
    }
          
  cp = ctic(ndevice,cticn);
  if(cp->type != CTIC_ACTIVE)
    {
    NPRINT "Invalid node or characteristic index in write_ctic\n");
    flushprint();
    return(0);
    }
  
  if(notflag == 0 && (cp->perm & 0x4C) == 0) 
    {
    if(cp->perm == 0)
      {
      NPRINT "Write permit not known. Guessing = no ack\n");
      NPRINT "Set in device info or Read services (find_ctics)\n");
      flushprint();
      }
    else
      {
      NPRINT "Not writeable\n");
      flushprint();
      return(0);
      }
    }
    
  if(cp->chandle == 0)
    {
    NPRINT "No handle. Read characteristic or Read services to find\n");
    NPRINT "Read services (find_ctics) if write permit is needed and not known\n");
    flushprint();
    return(0);
    }
    
  if(cp->origsize == 0 && count <= 0)
    {
    NPRINT "Must specify byte count\n");
    flushprint();
    return(0);
    }
    
  if(count <= 0)
    locsize = cp->origsize;  // known number of bytes in device info
  else
    locsize = count;
    
  if(locsize > LEDATLEN)
    {
    locsize = LEDATLEN;
    NPRINT "Warning - can only write %d bytes to %s\n",LEDATLEN,cp->name); 
    }
      
  chandle = cp->chandle;  // characteristic handle
  if(notflag == NOTIFY_ENABLE || notflag == NOTIFY_DISABLE)
    {
    if((cp->perm & 0x30) == 0)
      {
      NPRINT "Characteristic does not have notify/indicate permission\n");
      flushprint();
      return(0);
      }
       
    if(notflag == NOTIFY_ENABLE)
      {
      if((cp->perm & 0x20) != 0)
        VPRINT "Enable indicate for handle %04X\n",chandle);
      else
        VPRINT "Enable notify for handle %04X\n",chandle);
      data[0] = 1;  // enable
      cp->callback = callback;
#ifdef BTFPYTHON
      cp->pycallback = py_ncallback;
#endif  
      } 
    else
      {
      if((cp->perm & 0x20) != 0)
        VPRINT "Disable indicate for handle %04X\n",chandle);
      else
        VPRINT "Disable notify for handle %04X\n",chandle);
      data[0] = 0;  // disable
      cp->callback = NULL;
#ifdef BTFPYTHON
      cp->pycallback = NULL;
#endif  
      }
    data[1] = 0;
             
    if((cp->notify & 2) != 0)
      {
      VPRINT "Always enabled on remote device\n");
      flushprint();
      return(2);
      }
       
    ++chandle;   // next handle controls notify
    }

  cmd = lewrite + PAKHEADSIZE;         
                          // set characteristic handle
  
  cmd[10] = (unsigned char)(chandle & 0xFF);
  cmd[11] = (unsigned char)((chandle >> 8) & 0xFF);
  for(n = 0 ; n < locsize ; ++n)     // set data
    cmd[12+n] = data[n];     // low byte first

  cmd[3] = (locsize+7) & 0xFF;
  cmd[4] = ((locsize+7) >> 8) & 0xFF;
  
  cmd[5] = (locsize+3) & 0xFF;
  cmd[6] = ((locsize+3) >> 8) & 0xFF;
  
  lewrite[0] = (unsigned char)((12+locsize) & 0xFF);  // 13 for 1 byte
  lewrite[1] = (unsigned char)(((12+locsize) >> 8) & 0xFF); 
  
  if((cp->perm & 8) != 0 || notflag != 0)   // acknowledge 
    cmd[9] = 0x12;  // write request opcode
  else
    cmd[9] = 0x52;  // write command opcode

  if(gpar.printflag == PRINT_VERBOSE)
    {
    VPRINT "Write %s %s =",dp->name,cp->name);    
    for(n = 0 ; n < locsize ; ++n)
      VPRINT " %02X",cmd[n+12]);
    VPRINT "\n");
    VPRINT "SEND write LE characteristic\n");
    VPRINT "  Set [3][4] [5][6] packet lengths\n");
    VPRINT "  Set [9] opcode %02X\n",cmd[9]);
    VPRINT "  Set [10][11] characteristic handle %04X\n",chandle);
    VPRINT "  Set [12].. %02X data bytes\n",locsize);
    flushprint();
    }
         
     
  if(sendhci(lewrite,ndevice) == 0)  
    return(0);
    
  if(cmd[9] == 0x52)
    {   // no response or error reply
    readhci(0,0,0,0,0);  // peek to clear event 13 or notify maybe
    popins();
    flushprint();   
    return(count);  // OK
    }

  readhci(ndevice,IN_LEACK,0,gpar.timout,0);   
  n = findhci(IN_LEACK,ndevice,INS_POP);  
  if(n >= 0) 
    {
    if(insdatn[0] == 0x13)
      {   // write ack response
      popins();
      flushprint();
      return(count);   // OK 
      }
    if(insdatn[0] == 0x01)  // write ack error
      {
      k = insdatn[4]; 
      if(k > 19)
        k = 0;  // no errorle text
      NPRINT "  Error %d %s\n",k,errorle[k]);
      popins();
      flushprint();
      return(0); 
      }  
    }
  else
    NPRINT "Write response not seen\n");

  popins();
  flushprint();     
  return(count);
  }


/***********  READ CHARACTERISTIC *****************
ndevice = device index in dev[ndevice]
cticn =  characteristic index in dev[ndevice]->ctic[cticn]
data[] = array of bytes to receive data 
datlen = length of data[] 

If bit 16 of cticn is set (0x10000) then the low two bytes
of cticn are interpreted as the characteristic handle and
no use is made of dev[ndevice]->ctic[] 

return number of bytes n 
and data in data[0 to n-1]

Adds a terminating zero after n bytes
  data[n] = 0
So if the data is an ascii string wtihout a 
terminating zero, it is ready for use
  
*****************/

int read_ctic(int node,int cticn,unsigned char *data,int datlen)
  {
  struct cticdata *cp;  // characteristic info structure
  int n,k,k0,chandle,flag,retval,ndevice;
  unsigned char *cmd,*pack;
 
  if(check_init(0) == 0)
    return(0);

  for(n = 0 ; n < datlen ; ++n)
    data[n] = 0;

  gpar.readerror = ERROR_FATAL;
  
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);
  cp = ctic(0,0);
  chandle = 0;
   
    
  if(ndevice == 0)
    {  // local
    cp = ctic(0,cticn);
    if(cp->type != CTIC_ACTIVE)
      {
      NPRINT "Invalid local characteristic index in read_ctic\n");
      flushprint();
      return(0);
      }
    retval = 0;
    for(n = 0 ; n < cp->size && n < datlen ; ++n)
      {
      data[n] = cp->value[n];
      ++retval;
      }    
    if(retval < datlen)
      data[retval] = 0;
    return(retval);
    }

  if((dev[ndevice]->conflag & CON_LE) == 0)
    {
    NPRINT "%s not connected as LE server\n",dev[ndevice]->name);
    flushprint();
    return(0);
    }

  flag = 0;   // assume via handle
  pack = leread;
  cmd = leread + PAKHEADSIZE;
  data[0] = 0;  // no data return
  retval = 0;
     
  if((cticn & 0x10000) == 0)
    {  // use dev[]->ctic[cticn] info
    cp = ctic(ndevice,cticn);
    if(cp->type != CTIC_ACTIVE)
      {
      NPRINT "Invalid characteristic index in read_ctic\n");
      flushprint();
      return(0);
      }
    
    if(cp->chandle != 0)
      chandle = cp->chandle;  // characteristic handle      
    else if(cp->uuidtype != 0)
      {                       // UUID
      flag = cp->uuidtype;
      if(flag == 2)
        pack = lereaduuid2;
      else
        pack = lereaduuid16;
        
      cmd = pack + PAKHEADSIZE;
      for(n = 0 ; n < flag ; ++n)
        cmd[13+flag-n] = cp->uuid[n];   
      }
    else
      {
      NPRINT "No handle or UUID\n");
      flushprint();
      return(0);
      }  
    if(cp->perm != 0 && (cp->perm & 2) == 0)
      {
      NPRINT "Not readable\n");
      flushprint();
      return(0);
      }
    }
  else
    chandle = cticn & 0xFFFF;  // cticn = handle 
  
      
  VPRINT "Read LE characteristic\n");
           
  if(flag == 0)
    {                // set characteristic handle
    cmd[10] = (unsigned char)(chandle & 0xFF);
    cmd[11] = (unsigned char)((chandle >> 8) & 0xFF);                      
    VPRINT "  Set [10][11] handle %02X%02X\n",cmd[11],cmd[10]);
    }
  else
    {
    cmd[10] = 1;  // start handle = 1
    cmd[11] = 0;
    VPRINT "  Set [14].. UUID %02X..\n",cmd[14]);
    }
          
  // clear input buffer of any previous reads
  do
    {
    readhci(ndevice,0,IN_ATTDAT,0,0);
    n = findhci(IN_ATTDAT,ndevice,INS_POP);
    }
  while(n >= 0);
    
  sendhci(pack,ndevice);
       
  readhci(ndevice,IN_ATTDAT,0,5000,gpar.toshort);
      
  n = findhci(IN_ATTDAT,ndevice,INS_POP);  
  if(n >= 0) 
    {
    k0 = 0;    // error 
    if(insdatn[0] == 0x0B)  // handle read response opcode
      { 
      // instack[n] is type  [n+1][n+2]=number data bytes + 1
      // insdat[n]=0B [n+1]=data
      retval = instack[n+1] + (instack[n+2] << 8) - 1;  // number of data bytes
      k0 = 1;    // start of data
      }
    else if(insdatn[0] == 0x09)  // uuid read opcode
      {
      retval = insdatn[1] - 2;
      if((cticn & 0x10000) == 0)
        {
        if(cp->chandle == 0)   // uuid read has found handle
          cp->chandle = insdatn[2] + (insdatn[3] << 8);
        }  
      k0 = 4;  // data from [4]
      }
    else if(insdatn[0] == 0x01)
      {
      k = insdatn[4];  // error code
      if(k > 19)
        k = 0;    // no errorle text
      NPRINT "  Error %d %s\n",insdatn[4],errorle[k]); 
      } 
      
    if(k0 != 0)
      {  // read OK
      if(retval == 0)    // but no data
        NPRINT "Not set by LE device\n");
      else
        {
        gpar.readerror = 0;
        // set size in dev[] if previously unknown 
        if((cticn & 0x10000) == 0)
          {
          if(cp->size == 0)
            {
            if(retval > LEDATLEN)
              {
              NPRINT "Characteristic size larger than %d - truncated\n",LEDATLEN);
              retval = LEDATLEN;
              }
            cp->size = retval;
            }
          }
                
        if(retval > datlen-1)
          {
          NPRINT "  Need larger buffer for read_ctic\n");  
          retval = datlen-1;  // leave room for term 0
          }   
          
        for(k = 0 ; k < retval ; ++k)
          data[k] = insdatn[k+k0];         
        }     
      }
    }
  else
    {
    NPRINT "Read failed\n");
    }

  data[retval] = 0;
   
  popins();
  flushprint();
  return(retval);   
  }


/********* SEND HCI PACKET ********/


int sendhci(unsigned char *s,int ndevice)
  {
  int n,n0,len,chan,cmdflag;
  unsigned char *cmd;
  struct devdata *dp;

    
  cmd = s + PAKHEADSIZE;   // skip over header bytes to command 
  len = s[0] + (s[1] << 8);        // length of cmd from header
  dp = dev[0];
  
  cmdflag = 0;
    
  // s[2/3] = flags to set info in cmd string
 
 
  if((s[2] | s[3]) != 0)
    {  // will need board address, handle etc.  
    if(devokp(ndevice) == 0)
      return(0);
     
    dp = dev[ndevice];
          
    if((s[2] & ~S2_BADD) != 0)
      {   // not board address - need dhandle - must be connected  
      if(ndevice != 0 && dp->conflag == 0)
        {
        NPRINT "Not connected\n");
        return(0);
        }
      }
            
         
    if((s[2] & S2_HAND) != 0)  // device handle
      {
      cmdflag = 1;          
      if(cmd[0] == 2)
        n = 1;   // ACL
      else
        n = 4;   // HCI
      
      cmd[n] = dp->dhandle[0];
      cmd[n+1] = dp->dhandle[1];
      VPRINT "  Set [%d][%d] handle %02X %02X\n",n,n+1,dp->dhandle[0],dp->dhandle[1]);
      }

    if((s[2] & S2_BADD) != 0)  // board address  
      {  //  [4].. except LE open 0D 20
      if(cmd[1] == 0x11)
        n0 = 5;
      else if(cmd[1] == 0x0D && cmd[2] == 0x20)
        {      // LE open
        n0 = 10;
        cmd[9] = dp->leaddtype & 1;  // 0=public fixed  1=random 
        }
      else
        n0 = 4; 
        
      VPRINT "  Set [%d].. board address reversed %02X..%02X\n",n0,dp->baddr[5],dp->baddr[0]);  
      
      for(n = 0 ; n < 6 ; ++n)
        cmd[n+n0] = dp->baddr[5-n];
      }
  
    if(!((dp->type & BTYPE_LE) != 0))
      {    
      // following not done for LE - S2/3 flags not set for LE commands
     
      if(dp->psm == 1)
        n0 = 2;   // scid/dcid index 
      else
        n0 = 0;
             
      if((s[2] & S2_ID) != 0)
        {
        VPRINT "  Set [10] id %02X\n",dp->id);  
        cmd[10] = (unsigned char)(dp->id & 0xFF);;
        }

      if((s[2] & S2_DCIDC) != 0)
        {  // always psm 3
        cmd[7] = dp->dcid[0];  
        cmd[8] = dp->dcid[1];
        VPRINT "  Set [7][8] remote L2CAP channel %02X %02X\n",cmd[7],cmd[8]);
        }
    
      if((s[2] & S2_ADD) != 0)
        {
        cmd[9] = (dp->rfchan << 3);
        if((dp->conflag & CON_SERVER) != 0)
          cmd[9] += 1;  // server
        else
          cmd[9] += 3;  // client  
        VPRINT "  Set [9] RFCOMM address %02X\n",cmd[9]);
        }

      if((s[3] & S3_ADDX) != 0)
        {
        cmd[9] = (dp->rfchan << 3) + 1;   
        VPRINT "  Set [9] CONTROL address %02X\n",cmd[9]);
        } 

      if((s[2] & S2_FCS2) != 0)  
        calcfcs(s,2);    // calc and set last byte fcs
      if((s[2] & S2_FCS3) != 0)  
        calcfcs(s,3);    // calc and set last byte fcs
     
      if((s[3] & S3_DCID1) != 0)
        {
        cmd[13] = dp->dcid[n0];
        cmd[14] = dp->dcid[n0+1];
        VPRINT "  Set [13][14] remote L2CAP channel %02X %02X\n",cmd[13],cmd[14]);
        }

      if((s[3] & S3_SCID2) != 0)
        {
        cmd[15] = dp->scid[n0];
        cmd[16] = dp->scid[n0+1];
        VPRINT "  Set [15][16] local L2CAP channel %02X %02X\n",cmd[15],cmd[16]);
        }
    
      if((s[3] & S3_SCID1) != 0)
        {
        cmd[13] = dp->scid[n0];
        cmd[14] = dp->scid[n0+1];
        VPRINT "  Set [13][14] local L2CAP channel %02X %02X\n",cmd[13],cmd[14]);
        }

      if((s[3] & S3_DCID2) != 0)
        {
        cmd[15] = dp->dcid[n0];
        cmd[16] = dp->dcid[n0+1];
        VPRINT "  Set [15][16] remote L2CAP channel %02X %02X\n",cmd[15],cmd[16]);
        }
   
      if((s[3] & S3_DLCIPN) != 0)
        {
        cmd[14] = dp->rfchan << 1;
        VPRINT "  Frame size at [18] = %02X%02X\n",cmd[19],cmd[18]);
        VPRINT "  Set [14] DLCI %02X\n",cmd[14]);
        }
     
      if((s[2] & S2_SDP) != 0)
        {
        cmd[7] = dp->dcid[2];  // psm 1  
        cmd[8] = dp->dcid[3];  
        cmd[10] = (unsigned char)((dp->id >> 8) & 0xFF);  // id hi first
        cmd[11] = (unsigned char)(dp->id & 0xFF);   
        VPRINT "  Set [7][8] remote psm 1 channel\n");
        VPRINT "  Set [10][11] id %02X%02X\n",cmd[10],cmd[11]);   
        }
         
      if((s[3] & S3_ADDMSC) != 0)
        {
        cmd[14] = (dp->rfchan << 3) + 3;
        VPRINT "  Set [14] RFCOMM address %02X\n",cmd[14]);
        }
      }  // end classic clasdat xd sets 
    }  // end need device data         
         
  if(gpar.printflag == PRINT_VERBOSE)
    {
    if(cmd[0] == 2)
      {
      chan = cmd[7] + (cmd[8] << 8);  // channel
      if(chan == 1)
        {
        VPRINT "< CHANNEL 0001 Opcode:id = %02X:%02X   ",cmd[9],cmd[10]);
        if(cmd[9] == 2 || cmd[9] == 4 || cmd[9] == 6 || cmd[9] == 10)
          VPRINT "Expects reply %02X:%02X\n",cmd[9]+1,cmd[10]);
        else if(cmd[9] == 3 || cmd[9] == 5 || cmd[9] == 7  || cmd[9] == 11)
          VPRINT "is reply to %02X:%02X\n",cmd[9]-1,cmd[10]);
        else
         VPRINT "\n");              
        }
      else if(chan == 4)
        VPRINT "< CHANNEL 0004 Opcode = %02X\n",cmd[9]);       
      else if(chan >= 0x0040)
        {
        if((dp->conflag & CON_PSM1) != 0 && cmd[7] == dp->dcid[2])
          VPRINT "< SDP operation\n");
        else if(cmd[9] > 3)
          {
          VPRINT "< RFCOMM Address:Opcode = %02X:%02X",cmd[9],cmd[10]);
          if(cmd[10] == 0xEF)
            VPRINT "  Send serial data\n");
          else
            VPRINT "\n");
          }
        else
          VPRINT "< CONTROL Address:Opcode = %02X:%02X\n",cmd[9],cmd[10]);
        }
      else
        VPRINT "< CHANNEL %04X Opcode %02X\n",chan,cmd[9]);
      }
    else if(cmd[0] == 1)
      VPRINT "< HCI OGF=%02X OCF=%02X\n",(cmd[2] >> 2) & 0x3F,((cmd[2] << 8) & 0x300) + cmd[1]);
    else 
      VPRINT "< Unknown");
      
    hexdump(cmd,len);
    }  // end printflag
    
  if(packet_size(0) != 0 && (s == lereadreply || s == lewrite || s == lenotify ||
              s == le09replyv || s == attdata || s == blusend))
    {  // dongles
    if(splitcmd(s+PAKHEADSIZE,packet_size(0),ndevice) != 0)
      return(1);
    }
  
  else if(gpar.ledatlenflag == 0 && (s == lereadreply || s == lewrite || s == lenotify || s == le09replyv))
    {
    if(splitcmd(s+PAKHEADSIZE,32,ndevice) != 0)
      return(1);
    }  
  
  
  if(dp->conflag == CON_LE || dp->conflag == CON_LX || dp->conflag == CON_MESH)
    {
    n = (int)(dp->leinterval * 1.25) + 2;
    n0 = (int)(time_ms() - gpar.lastsend);
    if(n0 < n)
      sleep_ms(n-n0);
    } 
 
 
  n = sendpack(cmd,len);
  gpar.lastsend = time_ms();
  
  /*********
  ntogo = len;  // first header entry is length of cmd
  timstart = timems();  
   
  do
    {
    nwrit = write(gpar.hci,cmd,ntogo);

    if(nwrit > 0)
      {
      ntogo -= nwrit;
      cmd += nwrit;
      }
    else
      usleep(500);
        
    if(ntogo != 0 && timems() - timstart > (unsigned long long)5000)   // 5 sec timeout
      {
      NPRINT "Send CMD timeout - may need to reboot\n");
      return(0);
      }
    }
  while(ntogo != 0);
  gpar.lastsend = timems();
  *********/
      
  gpar.cmdcount += cmdflag;
  
  return(n);
  }

int packet_size(int size)
  {
  static int psize = 0;
  
  if(size >= 20)
    {
    if(size > 500)
      psize = 500;
    else
      psize = size;
    }
  return(psize);
  }     

int splitcmd(unsigned char *s,int plen,int ndevice)
  {
  int n,k,sn,tn,t20,len,numx,remx,kx;
  static unsigned char t[512];
  
  len = s[5] + (s[6] << 8);
  if(len <= plen-9)
    return(0);  

  for(n = 0 ; n < plen ; ++n)
    t[n] = s[n];
    
  t20 = t[2] & 0x0F;
  t[2] = t20;  // | 0x20;
  t[3] = plen-5;
  t[4] = 0;
  n = len-plen+9;
  numx = n/(plen-5);
  remx = n%(plen-5);
  if(remx > 0)
    ++numx;
  sn = plen;
  VPRINT "Split into %d packets\n",numx+1);
  splitwrite(t,plen,ndevice);
  for(n = 0 ; n < numx ; ++n)
    {
    ++gpar.cmdcount;
    t[0] = 0x02;
    t[1] = t[1];
    t[2] = t20 | 0x10;
    if(n == numx-1 && remx > 0)
      {
      t[3] = remx;
      kx = remx;
      }
    else
      {
      t[3] = plen-5;
      kx = plen-5;
      }
    t[4] = 0;
    tn = 5;
    for(k = 0 ; k < kx ; ++k)
      {
      t[tn] = s[sn];
      ++tn;
      ++sn;
      }
    splitwrite(t,kx+5,ndevice);
    }
  ++gpar.cmdcount;
  return(1);
  }

int splitwrite(unsigned char *cmd,int len,int ndevice)
  {
  int n,n0;
  struct devdata *dp;

  dp = dev[ndevice];
  if(dp->conflag == CON_LE || dp->conflag == CON_LX || dp->conflag == CON_MESH)
    {
    n = (int)(dp->leinterval * 1.25) + 10;
    n0 = (int)(time_ms() - gpar.lastsend);
    if(n0 < n)
      sleep_ms(n-n0);
    } 
  
  n = sendpack(cmd,len);
  gpar.lastsend = time_ms();
  return(n);
  }


/********* COMMAND STACK POINTER ********

Returns the APPROXIMATE number of commands waiting on the Bluetooth stack to be sent.
It is only approximate because each system sends Command Complete
events at a different rate, and some operations will not generate the event. 
So the stack may be empty and ready for more commands, but this function will return
a non-zero number. 
See Vol 4 Pt E  7.7.19

***********************************/

int cmd_stack_ptr()
  {
  if(check_init(0) == 0)
    return(0);
  readhci(0,0,0,0,0);
  return(gpar.cmdcount);
  }
  

/******* STATUS OK ********
look for standard status OK reply
flag=0 expect status reply only
     1 may be more replies to ditch

return 1 = seen status OK reply
       0 = not seen reply or status not OK
*********************/

     
int statusok(int flag,unsigned char *cmd)
  {
  int n,k,retval,repflag;
  
  retval = 0;
  repflag = 0;
  
  do
    {
    if(flag == 0)
      readhci(0,IN_STATOK,0,500,0);
    else
      readhci(0,IN_STATOK,0,500,500);
 
    n = findhci(IN_STATOK,0,INS_POP);
    if(n >= 0 && insdatn[1] == cmd[PAKHEADSIZE+1] && insdatn[2] == cmd[PAKHEADSIZE+2]) 
      {
      repflag = 1;
      if(insdatn[3] == 0)  // status = OK
        {      
        if(insdatn[1] == 0x18 && insdatn[2] == 0x20)
          {
          for(k = 0 ; k < 8 ; ++k)
            gpar.rand[k] = insdatn[k+4];
          }
        VPRINT "STATUS OK\n");
        retval = 1;
        }
      else if(!(insdatn[1] == 0x0A && insdatn[2] == 0x20))   // not mesh on/off
        VPRINT "STATUS failed OGF %02X OCF %02X\n",insdatn[2] >> 2,insdatn[1]);   
      }
    }
  while(n >= 0 && repflag == 0);
  
  if(repflag == 0)
    VPRINT "STATUS - no reply\n");
    
  flushprint();    
  
  popins();
  
  
  return(retval);
  }  


/************** READ HCI ***********************
Read incoming hci packets 
ndevice =  Expected sending device - but all available packets from all devices will be read
mustflag = Expected packet type (#defines of type IN_xxxxx) - will be stored on input stack
lookflag = Other packet types that will be stored on input stack if received
timout = Time out in ms spent waiting for mustflag packet 
         Usually set to global parameter gpar.timout
toshort = Time in ms spent waiting for more packets after mustflag received
          Usually set to global parameter gpar.toshort 

Waits for mustflag packet type. Times out after timout ms if not received.
Saves mustflag packet type on input stack
Also saves, but does not wait for, packets of lookflag type on input stack
All other packets will be discarded  
Once mustflag packet has been received, spends another toshort ms waiting for more packets.

Normal use:

1  sendhci(hcicmd,ndevice)
     Send command that expects a reply

2  readhci(ndevice,IN_RFUA,IN_L2ASKCF | IN_STATOK,gpar.timout,gpar.toshort);
     Wait gpar.timout ms for an IN_RFUA packet from ndevice and stores on input stack.
     If received, wait a further gpar.toshort ms for further packets
     If any packets of type IN_L2ASKCF or IN_STATOK are also received, store them on input stack
   
3  findhci(IN_RFUA,ndevice,INS_POP)
     returns index of first IN_RFUA packet from ndevice on input stack and marks for pop via popins()

4  findhci(IN_DATA,ndevice,INS_POP)
     returns index of first IN_DATA packet from ndevice on input stack and marks for pop via popins()


Other uses:
 
readhci(0,0,0,0,0)   peek to clear - read and discard any incoming packets
                     return immediately if none available (both time outs = 0)
                     
readhci(ndevice,0,IN_ATTDAT,0,0)  peek - return immediately if IN_ATTDAT not received from ndevice

readhci(0,0,0,3000,0)  Useful for code development - use as follows
                       set_print_flag(PRINT_VERBOSE)  for verbose prints
                       sendhci(hcicmd,ndevice)      send command
                       readhci(0,0,0,3000,0)        see all replies, 3s time out   
    


Also takes innediate action for some input packets via immediate()

GLOBAL PARAMTERS:

  gpar.printflag

RETURN
  1 = OK normal exit
  0 = error including time out if mustflag is set

****************************/

int readhci(int ndevice,long long int mustflag,long long int lookflag,int timout,int toshort)
  {
  unsigned char b0,*datp,*rsp,ledat[2];   
  int add,doneflag,crflag,disflag,lesflag,eflag;
  int gotn,k,j,n0,chan,xprintflag,devicen,stopverb,buf3sav;
  int retval,datlen,ascflag,clsflag,multiflag,seqflag;
  long long int locmustflag,gotflag;
  unsigned long long timstart,timx,timendms;
  struct devdata *dp,*condp;
  static long long int sflag;
  static int level = 0;   
  static unsigned char *buf = NULL;
  
  int toms,packlen;
  
  if(buf == NULL)
    buf = malloc(8192);

  condp = NULL;
  datp = NULL;
       
  //if(level > 0 && ndevice == 0 && mustflag == 0 && lookflag == 0 && timout == 0 && toshort == 0)
  //  return(0);
  
  if(ndevice == 0 && mustflag == 0 && lookflag == 0 && timout == 0 && toshort != 0)
    {
    seqflag = 1;
    timendms = toshort;
    }
  else
    {
    seqflag = 0;
    timendms = timout;
    }
    
  lesflag = ndevice & LE_SERV;
  clsflag = ndevice & CL_SERV;
  
  ndevice &= 0xFF;

  if(ndevice != 0 && (dev[ndevice]->conflag & CON_LX) != 0)
    lesflag = 1;   // mesh device is LE server
  if(gpar.serveractive == 2)
    {
    lesflag = 1;
    clsflag = 1;
    }
        
  if(level > 4)
    {
    NPRINT "ERROR - Callback has spawned too many nested operations\n"); 
    flushprint();
    return(0);
    }
         
  doneflag = 0;
    
  // check instack for mustflag data already read
  if(mustflag != 0 && findhci(mustflag,ndevice,INS_NOPOP) >= 0) 
    {
    locmustflag = 0;
    doneflag = 1;
    timendms = toshort;
    }       
  else
    locmustflag = mustflag;
  
  if(level == 0)
    sflag = lookflag | locmustflag; // packet search flags
  else
    sflag |= lookflag | locmustflag; // add to existing from previous level

  ++level;
  timstart = time_ms();  
  gotn = 0;   // number of reply
  devicen = 0; 
  xprintflag = 0;
  disflag = 0;

  do   
    {      
    if(locmustflag != 0 && findhci(mustflag,ndevice,INS_NOPOP) >= 0) 
      {
      locmustflag = 0;
      doneflag = 1;
      timendms = toshort;
      timstart = time_ms();
      }
       
    toms = (int)(timstart + (unsigned long long)timendms - time_ms());
    if(toms < 0)
      toms = 0;
    packlen = readpack(buf,toms);
       
    if(packlen <= 0)  
      {    // nothing read and timed out - normal exit route                         
      if(mustflag == 0 || doneflag != 0 || (disflag != 0 && ndevice == devicen) )
        retval = 1;  // OK normal exit or waiting for packet from disconnected device
      else
        {
        if(mustflag != IN_DATA && mustflag != IN_CONREQ && mustflag != IN_LEACK && lesflag == 0 && clsflag == 0)
          VPRINT "Timed out waiting for expected packet\n"); 
 
        retval = 0;
        }
      flushprint();
      popins();     
      --level;            

      return(retval);
      }

    if(doneflag != 0)
      {  // want quick exit but more coming - reset short TO
      // NPRINT "Reset TO 2\n");   
      timstart = time_ms();  // restart timer
      timendms = toshort;   // x 1024 ms to us     
      }

    gotflag = 0;  // no save to instack
    crflag = 0;   // no print credit decrement
    ascflag = 0;    // not serial data for ascii print
    disflag = 0;   // unexpected disconnect
    stopverb = 0;   // stop verbose le scan 
    chan = 0;
    xprintflag = 0;
    devicen = 0;      // sending device unknown
    buf3sav = -1;
    
    if(buf[0] == 4)    // HCI events
      {
      n0 = 0;  // offset of board address or handle for device identify
  
      if(buf[1] == 0x13)
        {
        // buf[3] = number of handles
        j = 6;
        for(k = 0 ; k < buf[3] ; ++k)
          {
          gpar.cmdcount -= buf[j] + (buf[j+1] << 8);
          j += 4;
          }
        }
      else if(buf[1] == 0x3E && buf[3] == 1) 
         {
         n0 = 9;  
         gotflag = IN_LEHAND;
         }
      else if(buf[1] == 0x3E && buf[3] == 2)
          {  
          stopverb = meshpacket(buf);  // 0 if got new mesh packet
          if( (sflag & IN_LESCAN) != 0)
            {
            gotflag = IN_LESCAN;
            stopverb = 0;
            }                        
          }
      else if(buf[1] == 0x3E && buf[3] == 3 && buf[4] == 0)
        {  // le interval update
        k = devnfromhandle(buf + 5);
        if(k != 0)
          dev[k]->leinterval = (buf[8] << 8) + buf[7];        
        }
      else if(buf[1] == 0x3E && buf[3] == 7)
        {
        n0 = 4 | 0x80;  // mtu change
        gotflag = IN_DATLEN | IN_IMMED;
        }
      else if(buf[1] == 0x3E && buf[3] == 6)
        {
        n0 = 4 | 0x80;  // param xchg req
        gotflag = IN_PARAMREQ | IN_IMMED;
        }
      else if(buf[1] == 0x3E && buf[3] == 5)
        {
        buf3sav = buf[3];
        buf[3] = 0xFF;
        n0 = 4 | 0x80;  
        gotflag = IN_CRYPTO | IN_IMMED;
        }
      else if(buf[1] == 0x3E && buf[3] == 8 && buf[4] == 0)
        {   // public XY  keys
        VPRINT "GOT local XY keys\n");
        for(k = 0 ; k < 32 ; ++k)
          {
          dev[0]->pubkeyx[k] = buf[k+5];
          dev[0]->pubkeyy[k] = buf[k+37];
          }
        gotflag = IN_AUTOEND;
        }     
      else if(buf[1] == 0x3E && buf[3] == 9 && buf[4] == 0 && gpar.dhkeydev != 0)
        {    // DHkey
        VPRINT "GOT DHkey\n");
        for(k = 0 ; k < 32 ; ++k)
          dev[gpar.dhkeydev]->dhkey[k] = buf[k+5];
        gpar.dhkeydev = 0;
        }
      else if(buf[1] == 8 || buf[1] == 0x59)
        {
        k = devnfromhandle(buf + 4);
        if(k != 0 && (dev[k]->conflag & CON_HCI) != 0)
          {  // classic
          if((sflag & IN_ENCR) != 0)
            {  
            if(buf[3] == 0 )  // status=0   
              {
              gotflag = IN_ENCR;
              n0 = 4 | 0x80;   // handle not board add
              }
            }  
          }
        else if(k != 0)
          {   // LE server crypto
          if(buf[3] == 0)
            {
            buf3sav = 0;
            buf[3] = 0xFE;
            }
          else
            {
            buf[6] = buf[3];
            buf3sav = buf[3];
            buf[3] = 0xFD;
            }
          n0 = 4 | 0x80;
          gotflag = IN_CRYPTO | IN_IMMED;
          }
        }         
      else if( (sflag & IN_BADD) != 0 && buf[1] == 0x0E && buf[4] == 0x09 && buf[5] == 0x10)   
        gotflag = IN_BADD;           // command complete with board address if buf[6]=0                   
      else if(buf[1] == 0x0E && (sflag & IN_STATOK) != 0)        
        gotflag = IN_STATOK;    // if buf[6] = 0 status OK         
      else if(buf[1] == 3)      //  (sflag & IN_CLHAND) != 0 && buf[3] == 0)  // status = 0   
        {  
        gotflag = IN_CLHAND;
        n0 = 6;
        } 
      else if(buf[1] == 0x04)
        {
        if((sflag & IN_CONREQ) != 0 || gpar.serveractive == 2)
          {
          gotflag = IN_CONREQ;
          n0 = 3;
          }
        else
          {
          NPRINT "%s is attempting a Classic connection\n",baddstr(buf+3,DIRN_REV));
          if((gpar.hidflag & 3) != 0)
            {
            NPRINT "This device is LE only - will fail for security reasons\n");
            NPRINT "Problem caused by client - its cache must be cleared or\n");
            NPRINT "call set_le_random_address() with a new value\n");
            }
          for(k = 0 ; k < 6 ; ++k)
            conreject[PAKHEADSIZE+k+4] = buf[3+k];
          sendhci(conreject,0);
          }
        }
      else if(buf[1] == 0x17) 
        {  
        gotflag = IN_LINKREQ | IN_IMMED;
        n0 = 3;
        }
      else if(buf[1] == 0x31)
        {    
        gotflag = IN_IOCAPREQ | IN_IMMED;
        n0 = 3;
        }
      else if(buf[1] == 0x32)
        {    
        gotflag = IN_IOCAPRESP | IN_IMMED;
        n0 = 3;
        }
      else if(buf[1] == 0x36)
        {
        gotflag = IN_PAIRED | IN_IMMED;
        n0 = 4;
        } 
      else if(buf[1] == 0x16)
        { 
        gotflag = IN_PINREQ | IN_IMMED;
        n0 = 3;
        }  
      else if(buf[1] == 0x34)
        { 
        gotflag = IN_PASSREQ | IN_IMMED;
        n0 = 3;
        }  
      else if( (sflag & IN_ACOMP) != 0 && buf[1] == 0x06)   // check status OK buf[3] == 0 after findtype   
        {
        gotflag = IN_ACOMP;
        n0 = 4 | 0x80;  // handle
        }
      else if( (sflag & IN_INQCOMP) != 0 && buf[1] == 0x01)   // scan inquiry complete       
        gotflag = IN_INQCOMP;     
      else if(buf[1] == 0x18)   // check status OK buf[3] == 0 after findtype   
        {
        gotflag = IN_KEY | IN_IMMED;
        n0 = 3;
        }
      else if( (sflag & IN_PCOMP) != 0 && buf[1] == 0x36 && buf[3] == 0)   
        {
        gotflag = IN_PCOMP;
        n0 = 4;
        }
      else if(buf[1] == 0x33 )
        {   
        gotflag = IN_CONFREQ | IN_IMMED;
        n0 = 3;
        }
      else if(buf[1] == 0x05 && buf[3] == 0 )  // STATUS=0    
        {
        disflag = 1;
        n0 = 4 | 0x80;  // handle not board add
        }
      else if( (sflag & IN_CSCAN) != 0 && (buf[1] == 0x02 || buf[1] == 0x22 || buf[1] == 0x2F) && buf[3] > 0 )  // number replies > 0    
        {
        gotflag = IN_CSCAN;
        n0 = 4;
        }
      else if( (sflag & IN_CNAME) != 0 && buf[1] == 0x07 && buf[3] == 0 )  // status 0
        {    
        gotflag = IN_CNAME;
        n0 = 4;         
        }
     
      if(buf[1] == 0x0E && buf[2] >= 4 && buf[4] == 0x22 && buf[5] == 0x20 && buf[6] == 0)
        {
        gpar.ledatlenflag = 1;  // data length change ok
        } 
       
      if(buf[1] == 0x0F && buf[3] == 0x0C)
        NPRINT "Command %02X %02X not allowed by adapter\n",buf[5],buf[6]);
   
      // find sending devicen  
       
      if((n0 & 0x80) != 0)
        {  // handle at n0
        n0 &= 0x7F;
        devicen = devnfromhandle(buf + n0);
        }
      else if(n0 != 0)
        {  // board address at n0 
        if(gotflag == IN_LEHAND)
          k = devnfrombadd(buf+n0,BTYPE_LE | BTYPE_ME,DIRN_REV);       
        else
          k = devnfrombadd(buf+n0,BTYPE_CL | BTYPE_ME,DIRN_REV);
          
        if(k > 0)
          devicen = k;         
        }   // end look for board address device
         
      if(gotflag == IN_LEHAND)
        {    
        gotflag = 0;  // no stack - conflag signals success
        if(buf[4] != 0)  // status error
          {
          k = buf[4];
          if(k > 69)
            k = 0;  // no error0f text
          NPRINT "%s\n",error0f[k]);
          }
        else   // LE connected
          {
          eflag = 1;  // error
          multiflag = 0;  // HID multi
          if((gpar.hidflag & 1) != 0 && (gpar.settings & HID_MULTI) == 0)
            {  // check for existing connection
            for(k = 1 ; multiflag == 0 && devok(k) != 0 ; ++k)
              {
              if(dev[k]->conflag == CON_LX)
                multiflag = 1;                
              } 
            }
          if((multiflag == 0 && ((sflag & IN_LEHAND) != 0 || (sflag & IN_LECMD) != 0)) || gpar.serveractive == 2)
            {  // is waiting for this connection
            if(lesflag != 0)
              {  // LE server accepts any client
              if(devicen == 0)
                {  // unknown     
                devicen = devalloc(buf+9,BTYPE_LE);
                if(devicen > 0)
                  {            
                  dp = dev[devicen];
                  dp->node = newnode();          
                  dp->leaddtype = buf[8] & 1;
                  strcpy(dev[devicen]->name,baddstr(dp->baddr,0));
                  }
                }                                 
              if(devicen != 0)
                {
                ledat[0] = LE_CONNECT;
                ledat[1] = (unsigned char)devicen;
                pushins(IN_LECMD,devicen,2,ledat);
                dev[devicen]->setdatlen = 20;
                dev[devicen]->cryptoflag = 0;
                eflag = 0;
                }
              }
            else if(devicen == ndevice)  // node server
              eflag = 0;
            }
                 
          if(eflag == 0)
            {               
            dp = dev[devicen];
            dp->dhandle[0] = buf[5];
            dp->dhandle[1] = buf[6];       
            dp->cryptoflag = 0;
            dp->leinterval = (buf[16] << 8) + buf[15];
             if(lesflag != 0)
              {
              dp->conflag = CON_LX;   // LE server
              VPRINT "Set MTU\n");
              mtuset[PAKHEADSIZE+10] = (unsigned char)(LEDATLEN & 0xFF);
              mtuset[PAKHEADSIZE+11] = (unsigned char)((LEDATLEN >> 8) & 0xFF);
              sendhci(mtuset,devicen);
              }       
            else if(dp->type == BTYPE_LE || (dp->type == BTYPE_ME && dp->lecflag != 0))
              dp->conflag = CON_LE;   // LE connected as LE
            else
              dp->conflag = CON_MESH;  // LE connected as mesh device
            doneflag = 1;  
            timstart = time_ms();
            timendms = toshort;
            }              
          else  // error disconnect
            {
            if(devicen == 0)
              NPRINT "Unknown device %s connected - rejected\n",baddstr(buf+n0,1));
            else
              NPRINT "Unwanted device %s connected - rejected\n",dev[devicen]->name);
              
            bluclosex[PAKHEADSIZE+4] = buf[5];
            bluclosex[PAKHEADSIZE+5] = buf[6];
            sendhci(bluclosex,0);
          //  if((gpar.meshflag & MESH_W) != 0)
          //    mesh_on();          
            }
          }
          
        if(((gpar.hidflag & 1) == 0 || (gpar.settings & HID_MULTI) != 0) && (gpar.meshflag & MESH_W) != 0)
          mesh_on();
        }  // end IN_LEHAND
      else if(gotflag == IN_CONREQ)
        {
        gotflag = 0;
        // want=ndevice got=devicen BADD=buf+n0 flag=clsflag
        if(devicen == 0)  
          {
          devicen = devalloc(buf+n0,BTYPE_CL);
          if(devicen > 0)
            {            
            dp = dev[devicen];
            dp->node = newnode();
            strcpy(dev[devicen]->name,baddstr(dp->baddr,0));
            } 
          }
        if((clsflag == 0 && devicen == ndevice) || (clsflag != 0 && devicen != 0))
          {
          ledat[0] = (unsigned char)devicen;
          pushins(IN_CONREQ,devicen,1,ledat);
          }
        else
          NPRINT "%s tried to connect - rejected\n",baddstr(buf+n0,1));
        }  
      else if(gotflag == IN_CLHAND)
        {
        gotflag = 0;  // no store on stack - conflag signals success
        if(buf[3] != 0)  // error code
          {
          k = buf[3];
          if(k > 69)
            k = 0;  // no error0f text
          NPRINT "%s\n",error0f[k]);
          }
        else   // connected
          {       
          if((devicen == ndevice) && (sflag & IN_CLHAND) != 0)
            {  // is waiting for this connection
            dp = dev[devicen];
            dp->dhandle[0] = buf[4];
            dp->dhandle[1] = buf[5];    
            dp->conflag |= CON_HCI;  
            VPRINT "GOT Open OK (Event 03) with handle = %02X%02X\n",buf[5],buf[4]);
            doneflag = 1;  
            timstart = time_ms();
            timendms = toshort;
            }
          else  // not waiting - disconnect
            {
            NPRINT "Unknown device %s connected - rejecting..\n",baddstr(buf+n0,1));
            bluclosex[PAKHEADSIZE+4] = buf[4];
            bluclosex[PAKHEADSIZE+5] = buf[5];
            sendhci(bluclosex,0);            
            }
          }
        }
      else if(gotflag == IN_CNAME)
        {        
        // find end of name buf[10] to term 0 force length to fit device name
        k = 0;
        while(buf[k+10] != 0 && k < NAMELEN-1)
          ++k;
        buf[k+10] = 0;
        pushins(gotflag,devicen,k+1,&buf[10]);
        } 
     
                      
      if(gotflag != 0 && gotflag != IN_CNAME)
        pushins(gotflag,devicen,buf[2],&buf[3]);
           
      if(disflag != 0 && devicen != 0)
        {
        dp = dev[devicen];   
            
        if(dp->conflag != 0 && !((dp->conflag & CON_RF) == 0 && (dp->conflag & CON_SERVER) != 0))    
          NPRINT "%s has disconnected\n",dev[devicen]->name);
        if((gpar.hidflag & 1) == 0 && (dp->conflag & CON_MESH) != 0)
          mesh_on();
        if(lesflag != 0 && (gpar.serveractive != 2 || (dp->conflag & CON_LX) != 0))
          {
          ledat[0] = LE_DISCONNECT;
          ledat[1] = 0;
          pushins(IN_LECMD,devicen,2,ledat);
          }
        dp->conflag = 0;
        dp->lecflag = 0;
        dp->setdatlen = 20;
        }                    
      }
    else if(buf[0] == 2)   // ACL
      {
      for(k = 1 ; devicen == 0 && devok(k) != 0 ; ++k)
        {
        // find sending device
        if(dev[k]->conflag != 0)
          {        // hi 4 bits of buf[2] = flags
          if(dev[k]->dhandle[0] == buf[1] && dev[k]->dhandle[1] == (buf[2] & 15))
            devicen = k;
          }
        }

      chan = (buf[7] + (buf[8]<<8));  // channel
        
      if(chan == 4)    // LE or mesh
        {
        if((dev[devicen]->conflag & CON_LE) != 0 && (buf[9] == 0x1B || buf[9] == 0x1D))
          {  // client notify
          gotflag = IN_NOTIFY | IN_IMMED;
          if(mustflag == IN_NOTIFY)
            locmustflag &= ~IN_NOTIFY;
          }
        else if((dev[devicen]->conflag & CON_LX) != 0)
          {  // server
          gotflag = IN_ATTDAT | IN_IMMED;
           
          // buf[9]==1  error=buf[13] return
                         
          if(buf[9] == 0x1E || ((sflag & IN_LEACK) != 0 && buf[9] == 0x01))
            {  // indicate ack 
            gotflag = IN_LEACK;
            }
          }
        else if(dev[devicen]->type == BTYPE_ME && (dev[devicen]->conflag & CON_LE) == 0)
          gotflag = IN_DATA;    // mesh data
        else
          {
          if(buf[9] != 3)  // not MTU ack
            gotflag = IN_ATTDAT;  // LE        
                         
          // buf[9] == 1  error buf[13]
          
          if(buf[9] == 0x13 || ((sflag & IN_LEACK) != 0 && buf[9] == 0x01))
            {  // writc ctic ack
            gotflag = IN_LEACK;
            }
                     
          if((dev[devicen]->conflag & CON_LE) != 0 && (buf[9] & 1) == 0 && buf[9] <= 0x20)
            {  // even opcode = request from server - let odd opcodes go to ATTDAT 
            gotflag = 0;  // ditch
            if(buf[9] == 0x02)
              {  // MTU exhange
              VPRINT "SEND MTU exchange reply\n");
              lemtu[PAKHEADSIZE+10] = (unsigned char)((LEDATLEN + 3) & 0xFF);
              lemtu[PAKHEADSIZE+11] = (unsigned char)(((LEDATLEN + 3) >> 8) & 0xFF);
              sendhci(lemtu,devicen);
              dev[devicen]->setdatlen = LEDATLEN;
              }
            else 
              {
              VPRINT "%s is requesting attributes - fob it off\n",dev[devicen]->name);
              if(buf[9] == 0x04 && buf[10] == 0x01 && buf[11] == 0x00) 
                 sendhci(fob05,devicen);
              else if((buf[9] == 0x08 || buf[9] == 0x10) &&
                          buf[10] == 0x01 && buf[11] == 0x00 && buf[14] == 0x00 && buf[15] == 0x28)
                {         
                if(buf[9] == 0x08)
                  sendhci(fob09,devicen);
                else
                  sendhci(fob11,devicen);           
                }
              else
                {
                lefail[PAKHEADSIZE+10] = buf[9];
                lefail[PAKHEADSIZE+11] = buf[10];
                lefail[PAKHEADSIZE+12] = buf[11];
                if(buf[9] == 0x04 || buf[9] == 0x06 || buf[9] == 0x08 || buf[9] == 0x10)
                  lefail[PAKHEADSIZE+13] = 0x0A;  // attribute not found
                else if(buf[9] == 0x0A || buf[9] == 0x12)
                  lefail[PAKHEADSIZE+13] = 0x01;  // invalid handle
                else
                  lefail[PAKHEADSIZE+13] = 0x06;  // req not supported
                 
                sendhci(lefail,devicen);  // error reply
                }
              }
            } 
          } 
        }   // end chan 4          
      else if(chan == 1)
        {
        if(buf[9] == 0x02)
           gotflag = IN_L2ASKCT | IN_IMMED;
        else if(buf[9] == 0x04)
          gotflag =  IN_L2ASKCF | IN_IMMED;
        else if(buf[9] == 0x06)
          gotflag = IN_L2ASKDIS | IN_IMMED;
        else if(buf[9] == 0x0A)
          gotflag = IN_L2ASKINFO | IN_IMMED;                                     
        else if(buf[9] == 0x08)
          gotflag = IN_ECHO | IN_IMMED;           
        else if((sflag & IN_L2REPCT) != 0 && buf[9] == 0x03 && buf[17] == 0 && buf[18] == 0)  // result=0 done   [17]=1 pending
          {
          gotflag =  IN_L2REPCT;
          if(devicen != 0)
            {
            dp = dev[devicen];
            if(buf[15] == 0x41 || buf[15] == 0x42)
              {
              k = 2;  // psm 1
              dp->conflag |= CON_PSM1;
              }
            else
              {
              k = 0;   // psm 3
              dp->conflag |= CON_PSM3;
              }
            dp->dcid[k] = buf[13];
            dp->dcid[k+1] = buf[14];
            }                
          }
        else if((sflag & IN_L2REPCF) != 0 && buf[9] == 0x05)
          gotflag = IN_L2REPCF;
        else if((sflag & IN_L2REPDIS) != 0 && buf[9] == 0x07)
          gotflag = IN_L2REPDIS;
        else if((sflag & IN_L2REPINFO) != 0 && buf[9] == 0x0B)
          gotflag = IN_L2REPINFO;
        }
      else if( chan >= 0x40)   // dynamic channel
        {
        add = buf[9] >> 3;  // 0=CONTROL else RFCOMM 
        if(buf[7] == 0x41 && (buf[9] == 6 || buf[9] == 4 || buf[9] == 2) )  // 41 = psm 1 channel
          gotflag = IN_SSAREQ | IN_IMMED; 
        if( (sflag & IN_SSAREP) != 0 && (buf[9] == 1 || buf[9] == 3 || buf[9] == 5 || buf[9] == 7) && buf[10] == 0)         
          gotflag = IN_SSAREP;
        else if( (sflag & IN_RFUA) != 0 && buf[10] == 0x73)
          gotflag = IN_RFUA;    
        else if(buf[10] == 0x3F)
          gotflag = IN_CONCHAN | IN_IMMED;
        else if(buf[10] == 0xEF && add == 0 && buf[12] == 0x83)
          gotflag = IN_RFCHAN | IN_IMMED;
        else if( (sflag & IN_PNRSP) != 0 &&  buf[10] == 0xEF && add == 0 && buf[12] == 0x81)
          gotflag = IN_PNRSP;    
        else if(buf[10] == 0xEF && buf[9] == 0x03 && (buf[12] == 0xE1 || buf[12] == 0xE3 || 
                     buf[12] == 0x93 || buf[12] == 0x91))
          {     // copy for reply
          gotflag = IN_MSC | IN_IMMED;
          datlen = buf[3] + (buf[4] << 8) + 2;
          if((buf[12] & 0xF0) == 0xE0)
            rsp = msccmdrspe;
          else
            rsp = msccmdrsp9;
          for(k = 0 ; k < datlen && k < 60 ; ++k)
            rsp[PAKHEADSIZE+k+3] = buf[k+3];  // same reply
          rsp[0] = datlen+3; 
          }
        else if( (buf[10] & 0xEF) == 0xEF && add != 0)   // EF or FF
          gotflag = IN_DATA;    
        else if(buf[10] == 0x53)
          gotflag = IN_DISCH | IN_IMMED;  // close CONTROL/RFCOMM            
        }
      else if(chan == 5)
        {
        if(buf[9] == 0x12)
          gotflag = IN_CONUP5 | IN_IMMED;
        }
      else if(chan == 6)
        {
        gotflag = IN_CRYPTO | IN_IMMED;
        }
            
         
      if(gotflag != 0)
        {        
        if(gotflag == IN_DATA)
          {            
          if(chan == 4)   // ATT LE node first data
            {
            datlen = buf[3] + (buf[4] << 8) - 4;
            datp = buf+9;
            pushins(gotflag,devicen,datlen,datp);
            xprintflag = 1;
            ascflag = 1;
            }
          else     // classic serial data first packet
            {
            datp = buf+12;        // data [12]
            k = 0;                // data shift
            datlen = buf[11] >> 1;  // length lo 7 bits
            if( (buf[11] & 1) == 0)
              {   // second length byte 
              datlen += (buf[12] << 7);
              ++k;  // data [13]
              }
            if(buf[10] == 0xFF)
              ++k;      // skip credit data [13] or [14] 
            datp += k;
            if(datlen != 0)   // length not zero    FF 01 credit ?
              {
              if(devicen != 0)
                {
                condp = dev[devicen];     
                --condp->credits;  // no extra inc
                crflag = 1;
                }
              if(datlen > 0)
                pushins(gotflag,devicen,datlen,datp);
              ascflag = 1;  // may print ascii with hexdump          
              }
            }
          }   // end IN_DATA 
        else
          {   // normal push
          pushins(gotflag,devicen,buf[3] + (buf[4] << 8) - 4,buf+9);
          }
        }  // end gotflag
      }  // end 02 packet   
  
    if(ndevice == 0 || (ndevice != 0 && devicen != 0 && ndevice == devicen))      
      locmustflag &= ~gotflag;  // zero bit - got this one - device must match
                               
    ++gotn;  // reply count  
       
                   
    if(gpar.printflag == PRINT_VERBOSE && stopverb == 0)
      {         // only print le scan reply gotflag == LE_SCAN or meshflag = 1
    //  igflag = 0;
      b0 = buf[0];
      if(buf3sav >= 0)
        buf[3] = (unsigned char)buf3sav;
        
      if(b0 == 2 && xprintflag != 0)
        {
        if(xprintflag == 1)
          VPRINT "> Node data - first packet\n");
        else
          VPRINT "> Extra bytes - add to previous data\n");
        }
      else if(b0 == 4)
        {
        VPRINT "> ");
        VPRINT "Event %02X =",buf[1]);
        for(k = 3 ; k < packlen && k < 13 ; ++k)
          VPRINT " %02X",buf[k]);
        if(k == packlen)
          VPRINT "\n");
        else
          VPRINT "...\n");
        if(buf[1] == 0x0F && buf[3] != 0)
          {
          k = buf[3];
          if(k > 69)
            k = 0;  // no error0f text
          VPRINT "*** FAIL *** CMD %02X %02X Error %02X %s\n",buf[5],buf[6],buf[3],error0f[k]);
          }
        if(buf[1] == 0x3E && buf[3] == 0x03)
          {
          if(buf[4] == 0)
            VPRINT "Interval changed to %02X%02X\n",buf[8],buf[7]);
          else
            VPRINT "Interval change failed\n");
          }       
        }
      else if(b0 == 2)
        {     
        if(chan == 1)
          {
          VPRINT "> CHANNEL 0001 Opcode:id = %02X:%02X",buf[9],buf[10]);
          if(buf[9] == 2 || buf[9] == 4 || buf[9] == 6 || buf[9] == 10)
            VPRINT "  Must send reply %02X:%02X\n",buf[9]+1,buf[10]);
          else if(buf[9] == 3 || buf[9] == 5 || buf[9] == 7  || buf[9] == 11)
            VPRINT "  is reply from %02X:%02X\n",buf[9]-1,buf[10]);
          else
           VPRINT "\n");
          }
        else if(chan == 4)
          {
          VPRINT "> CHANNEL 0004 Opcode = %02X\n",buf[9]);
          if(buf[9] == 1)  // error return
            {
            k = buf[13];
            if(k > 19)
              k = 0;
            VPRINT "   Error %02X = %s\n",buf[13],errorle[k]);
            }
          }  
        else if(chan >= 0x40)
          {
          if(buf[7] == 0x41 || buf[7] == 0x42)
            VPRINT "> SDP operation");
          else if((buf[9] >> 3) != 0)
            {
            VPRINT "> RFCOMM Address:Opcode %02X:%02X",buf[9],buf[10]);
            if(crflag != 0)       // not control channel
              VPRINT "   received serial data");   // serial data
            }   
          else
            {
            VPRINT "> CONTROL Address:Opcode %02X:%02X",buf[9],buf[10]);
            }  
          if( (buf[9] & 2) == 0 && buf[10] == 0x53)  // close from remote
            VPRINT "   close request. Must send %02X:73 UA reply\n",buf[9]);
          else
            VPRINT "\n");         
          }
        else if(chan != 0)
          {
          VPRINT "> CHANNEL %04X = ",chan);
          for(k = 9 ; k < packlen && k < 20 ; ++k)
            VPRINT " %02X",buf[k]);
          if(k == packlen)
            VPRINT "\n");
          else
            VPRINT "...\n");
          }
        }
      else
        VPRINT "> Unknown\n");
    
      hexdump(buf,packlen);
      if(ascflag != 0)   // serial read data - print if all ascii
        printascii(datp,datlen);      
      }   // end print
    

    // top up credits
    if(crflag != 0)
      {
      VPRINT "  Has used a credit. Number remaining = %d\n",condp->credits);  
      if(condp->credits < 8)
        setcredits(devicen);   // top up credits
      } 
 
    if(seqflag != 0)
      {  // restart short to
      timstart = time_ms();
      timendms = toshort;
      }   
       
    if((doneflag == 0 && mustflag != 0 && locmustflag == 0) ||
       (disflag != 0 && ndevice == devicen)  )
      {  // done - switch to short timeout
         // or waiting for packet from device that has disconnected unexpectedly 
      doneflag = 1;  
      timstart = time_ms();
      timendms = toshort;
      }
    flushprint(); 
    
       
    timx = time_ms();
    immediate(lookflag | locmustflag);
    timendms += time_ms() - timx;  // ignore time spent in immediate         
    }
  while(1);  
   
  return(1);
  } 


   
void immediate(long long lookflag)
  {
  int n,j,devicen,id,ch,psm,scflag,popflag;
  int bn,getout,cticn,chandle,cmpok;
  unsigned char *rsp,buf[16];
  char sbuf[64];
  long long int gotflag;
  struct devdata *dp;
  struct cticdata *cp;
  struct devdata *rp,*ip,*sp;  
  unsigned char rby,key[16],out[16],ia[7],ra[7],mackey[16];
    
  cp = NULL;
         
  while(1)
    {
    n = 0;
   
    do
      {
      while( (instack[n] & 0xC0) != INS_IMMED && instack[n] != INS_FREE)
        n += instack[n+1] + (instack[n+2] << 8) + INSHEADSIZE;  // next type
 
      if(instack[n] == INS_FREE)
        return;  // normal exit 
           
      if(instack[n+3] == 0)
        instack[n] = INS_POP;
      }
    while(instack[n+3] == 0);
 
    popflag = 0;   
    gotflag = (long long int)1 << (instack[n] & 0x3F);
    devicen = instack[n+3];
    dp = dev[devicen];
    
    
    if(gotflag == IN_ATTDAT && (dp->conflag & CON_LX) != 0)
      {  
      popflag = leserver(devicen,instack[n+1]+(instack[n+2] << 8),insdat+n,n);
      }
    else if(gotflag == IN_L2ASKCF)
      {
      id = insdat[n+1];
      VPRINT "GOT L2 request config - id %02X\n",id);
      j = insdat[n+4];
      if(j == 0x41 || j == 0x42)
        dp->psm = 1;
      else
        dp->psm = 3;
        
      if((dp->conflag & CON_SERVER) != 0)
        {  // server  (already done if client)
        dp->id = 5;
        VPRINT "SEND L2 config request. Choose id %02X\n",dp->id);
        flushprint();
        sendhci(figreq,devicen); 
        }
     
      dp->id = id;
      VPRINT "SEND L2 config reply\n");
      sendhci(figreply,devicen);
      // expect 05 reply
      }
    else if(gotflag == IN_L2ASKDIS)
      {
      id = insdat[n+1];

      j = insdat[n+4];
      if(j == 0x41 || j == 0x42)
        dp->psm = 1;
      else
        dp->psm = 3;

      VPRINT "GOT L2CAP disconnect psm %d request - id %02X\n",dp->psm,id);      
    
      if((dp->psm == 1 && (dp->conflag & CON_PSM1X) == 0)  ||
         (dp->psm == 3 && (dp->conflag & CON_PSM3X) == 0) )
        {  // disconnect request initiated by remote     
        dp->id = 11;        
        VPRINT "SEND same L2CAP disconnect request\n");
        sendhci(psmdisreq,devicen);
        }
      // else local has already sent psmdisreq
        
      VPRINT "SEND L2CAP disconnect reply\n");

      flushprint();
              
      dp->id = id;
      sendhci(psmdisreply,devicen);
      
      if(dp->psm == 1)
        dp->conflag &= ~(CON_PSM1 | CON_PSM1X);
      else
        dp->conflag &= ~(CON_RF | CON_CH0 | CON_PSM3 | CON_PSM3X);
      // expect 07 reply
      }
    else if(gotflag == IN_L2ASKINFO)           
      {
      j = insdat[n+4];
      dp->id = insdat[n+1];
      VPRINT "GOT ask info (opcode 0A) type %d\n",j);
      if(j == 2)
        {
        VPRINT "SEND info reply type 2\n");
        sendhci(inforeply2,devicen);
        }    
      else if(j == 3)
        {
        VPRINT "SEND info reply type 3\n");
        sendhci(inforeply3,devicen);
        } 
      }
    else if(gotflag == IN_ECHO)           
      {
      dp->id = insdat[n+1];
      VPRINT "GOT ask echo (opcode 08)\n");
      VPRINT "SEND echo reply\n");
      sendhci(echoreply,devicen);
      }
    else if(gotflag == IN_DISCH)
      {
      ch = insdat[n] >> 3;
      if(gpar.printflag == PRINT_VERBOSE)
        {   // should be RFCOMM channel or CONTROL 0 channel
        VPRINT "GOT Disconnect channel %d request (opcode 53)\n",ch);
        VPRINT "SEND UA reply (opcode 73)\n");
        VPRINT "  SET [9] Same address\n");
        }
      uareply[PAKHEADSIZE+9] = insdat[n];
      sendhci(uareply,devicen);  // UA reply to remote
      if(ch == 0) 
        dp->conflag &= ~CON_CH0;
      else
        dp->conflag &= ~CON_RF;
      }
    else if(gotflag == IN_CONCHAN)
      {
      ch = insdat[n] >> 3;        
      if(gpar.printflag == PRINT_VERBOSE)
        {
        VPRINT "GOT open channel %d (opcode 3F)\n",ch);
        VPRINT "SEND UA reply\n");
        VPRINT "  Set [9] Same address\n");
        }
      uareply[PAKHEADSIZE+9] = insdat[n];
      sendhci(uareply,devicen);  
      if(ch == 0) 
        dp->conflag |= CON_CH0;
      else
        {
        dp->conflag |= CON_RF;  // RFCOMM connect done 
        if((lookflag & IN_AUTOEND) != 0)
          {
          buf[0] = AUTO_RF;
          pushins(IN_AUTOEND,devicen,1,buf);
          }
        }     
      }         
    else if(gotflag == IN_RFCHAN)
      {
      dp->rfchan = insdat[n+5] >> 1;  // rfcomm channel
      VPRINT "%s is trying to connect on channel %d frame size %02X%02X\n",dp->name,dp->rfchan,insdat[n+10],insdat[n+9]);
 
      for(j = 0 ; j < 9 ; ++j)
        pnreply[PAKHEADSIZE+13+j] = insdat[n+4+j];
    
      VPRINT "SEND PN reply\n");
      
      sendhci(pnreply,devicen);   
      }
    else if(gotflag == IN_MSC)
      {
      
      j = insdat[n+3];
      VPRINT "GOT Type %02X\n",j);
      VPRINT "SEND reply\n");   
      if((j & 0xF0) == 0xE0)
        rsp = msccmdrspe;
      else
        rsp = msccmdrsp9;
      
      rsp[PAKHEADSIZE+9] = 0x01;  // reply
      rsp[PAKHEADSIZE+12] = j;  // E3 E1 93 91
      sendhci(rsp,devicen);
      if((lookflag & IN_AUTOEND) != 0 && j == 0xE1)
        {
        buf[0] = AUTO_MSC;
        pushins(IN_AUTOEND,devicen,1,buf);
        }
      }
    else if(gotflag == IN_L2ASKCT)
      {
      psm = insdat[n+4] + (insdat[n+5] << 8);
      dp->id = insdat[n+1];  // ID from request
      if((dp->conflag & CON_SERVER) == 0 ||
       !( (psm == 1 && (dp->conflag & CON_PSM1) == 0) ||
          (psm == 3 && (dp->conflag & CON_PSM3) == 0) ) )
        {  // only allow server psm 1/3  
        VPRINT "  GOT L2 connect request psm %04X. Fob it off\n",psm);
        sendhci(foboff,devicen);
        }
      else
        {
        VPRINT "GOT L2 connect request psm %d channel %02X%02X\n",insdat[n+4],insdat[n+7],insdat[n+6]);  
  
        if(psm == 1)   // psm 1 for SDP
          {
          dp->psm = 1;
          dp->scid[2] = 0x41;  // psm 1 for SSA request
          dp->scid[3] = 0x00; 
          dp->dcid[2] = insdat[n+6];
          dp->dcid[3] = insdat[n+7];
          }
        else  // psm 3 9 for RFCOMM
          {
          dp->psm = 3;
          dp->scid[0] = 0x43;
          dp->scid[1] = 0x00; 
          dp->dcid[0] = insdat[n+6];
          dp->dcid[1] = insdat[n+7];
          }
     
        VPRINT "SEND connect reply\n");
        sendhci(psmreply,devicen);

        if(dp->psm == 1)
          dp->conflag |= CON_PSM1;
        else
          dp->conflag |= CON_PSM3;
        } 
      }
    else if(gotflag == IN_SSAREQ)
      {
      replysdp(devicen,n,NULL,NULL);
      }
    else if(gotflag == IN_LINKREQ)
      {
      VPRINT "GOT link request (Event 17)\n"); 

      if((dp->linkflag & KEY_ON) == 0)
        {
        VPRINT "GOT 17 Send neg reply\n"); 
        VPRINT "SEND link request neg reply\n");  
        sendhci(linkreply,devicen);
        }
      else      
        {
        if((dp->linkflag & KEY_FILE) != 0)
          rwlinkey(4,devicen,NULL);  // used
        
        VPRINT "GOT 17 Send key\n"); 
        VPRINT "SEND link key\n"); 
        for(j = 0 ; j < 16 ; ++j)
          linkey[PAKHEADSIZE+j+10] = dp->linkey[j];
        sendhci(linkey,devicen);
        
        dp->linkflag |= KEY_SENT;
        }        
      }
    else if(gotflag == IN_KEY)
      {
      VPRINT "GOT link key (Event 18)\n");  
      for(j = 0 ; j < 16 ; ++j)
        dp->linkey[j] = insdat[n+j+6];
      dp->linkflag |= KEY_NEW;
      }  
    else if(gotflag == IN_IOCAPRESP)
      {
      VPRINT "GOT IO Cap reply (Event 32) I/O=%d Auth=%d\n",insdat[n+6],insdat[n+8]);  
      }
    else if(gotflag == IN_PAIRED)
      {
      VPRINT "GOT Event 36 status %d\n",insdat[n]); 
      if(insdat[n] == 0)
        VPRINT "Paired with %s\n",dp->name);
      else
        NPRINT "Failed to pair with %s\n",dp->name);
      }
    else if(gotflag == IN_IOCAPREQ)
      {
      if((dp->linkflag & PASSKEY_LOCAL) != 0)
        j = 1;  // display y/n prints passkey here PASSKEY_LOCAL 
      else if((dp->linkflag & PASSKEY_REMOTE) != 0)
        j = 2;  // remote prints passkey - keyboard enter here 
      else
        j = 3;   // no i/o 
      
      iocapreply[PAKHEADSIZE+10] = (char)j;  // io cap

      if(gpar.printflag == PRINT_VERBOSE) 
        {  
        VPRINT "GOT IO capability request (Event 31)\n");
        VPRINT "SEND IO capability reply %d\n",iocapreply[PAKHEADSIZE+10]);
        }   
          
      sendhci(iocapreply,devicen);   
      }
    else if(gotflag == IN_PINREQ)
      {      
      VPRINT "GOT PIN request (Event 16)\n");
      if(dp->pincode[0] == 0)
        {
        flushprint();
        dp->pincode[0] = 0;
        inputpin("Input PIN code (0000 if unknown)\n? ",dp->pincode);        
        j = 0;
        while(j < 15 && dp->pincode[j] != 0 && dp->pincode[j] != 10)
          ++j;
        dp->pincode[j] = 0;
        }
      else
        {
        NPRINT "Using PIN=%s from device info\n",dp->pincode);
        j = strlen(dp->pincode);
        }
        
      pincode[PAKHEADSIZE+10] = j;   
      strcpy((char*)pincode+PAKHEADSIZE+11,(char*)dp->pincode);
      VPRINT "SEND PIN code\n");
      VPRINT "  Set [10] PIN length\n");
      VPRINT "  Set [11] PIN = %s\n",dp->pincode);
     
      sendhci(pincode,devicen);
      }     
    else if(gotflag == IN_PASSREQ)
      {      
      VPRINT "GOT passkey request (Event 34)\n");
      flushprint();
      sbuf[0] = 0;
      if(dp->pincode[0] == 0)
        inputpin("Input passkey displayed on remote device\n? ",sbuf);      
      else
        {
        NPRINT "Using passkey = %s from devices file\n",dp->pincode);
        strcpy(sbuf,dp->pincode);
        }
        
      j = atoi(sbuf);
      passkey[PAKHEADSIZE+10] = j & 0xFF;
      passkey[PAKHEADSIZE+11] = (j >> 8) & 0xFF;
      passkey[PAKHEADSIZE+12] = (j >> 16) & 0xFF;
      passkey[PAKHEADSIZE+13] = (j >> 24) & 0xFF;
           
      VPRINT "SEND Passkey\n");
      VPRINT "  Set [10] = passkey\n");
      
      sendhci(passkey,devicen);
      }
       
           
    else if(gotflag == IN_CONFREQ)
      {
      VPRINT "GOT User confirm request with passkey (Event 33)\n");
      VPRINT "SEND User confirm reply\n");
   
      sprintf(sbuf,"Passkey = %d  Valid for 10 seconds\n",insdat[n+6] + (insdat[n+7] << 8) + (insdat[n+8] << 16));
      printn(sbuf,strlen(sbuf)); 
       
      flushprint();   
       
      sendhci(spcomp,devicen);
      }
    else if(gotflag == IN_NOTIFY)
      {  // LE notify (1B) or indicate (1D)
      if(insdat[n] == 0x1D)
        sendhci(leindack,devicen);  // acknowledge indicataion 1E          
       
      bn = instack[n+1] + (instack[n+2] << 8) - 3;
      chandle = insdat[n+1] + (insdat[n+2] << 8);
    
      getout = 0;
      for(j = 0 ; getout == 0 && j < 1024 ; ++j)
        {   // search cticn index for handle
        cp = ctic(devicen,j); 
        if(cp->type == CTIC_ACTIVE)
          {
          if(cp->chandle == chandle)
            {
            cticn = j;
            getout = 1;
            }
          }
        else
          getout = 2;  // fail to find handle match
        }   
       
      if(getout != 1)
        VPRINT "Unknown LE characteristic notified\n");
      else
        {
        if((cp->perm & 0x20) != 0)        
          VPRINT "%s %s indicate =",dp->name,cp->name);
        else
          VPRINT "%s %s notify =",dp->name,cp->name);
        for(j = 0 ; j < bn ; ++j)
          VPRINT " %02X",insdat[n+3+j]);
        VPRINT "\n");
#ifdef BTFPYTHON
        if(cp->pycallback != NULL)
          py_notify_callback(cp->pycallback,dp->node,cticn,insdat+n+3,bn);
#else  
        if(cp->callback != NULL)
          (*cp->callback)(dp->node,cticn,insdat+n+3,bn);
#endif
        }
      }
    else if(gotflag == IN_PARAMREQ)
      {
      VPRINT "Connection parameter change request\n");
      for(j = 0 ; j < 8 ; ++j)
        {
        paramreply[PAKHEADSIZE+j+6] = insdat[n+j+3];
        }
      sendhci(paramreply,devicen);                
      }
    else if(gotflag == IN_DATLEN)
      {
      VPRINT "%s data length change T %02X R %02X\n",dp->name,insdat[n+3],insdat[n+7]);
      if(insdat[n+3] < 0xF0)  // local tx
        setlelen(devicen,LEDATLEN,0);
      }
    else if(gotflag == IN_CONUP5)
      {
      leconnreply[PAKHEADSIZE+10] = insdat[n+1];  // id
      sendhci(leconnreply,devicen);
      }
    else if(gotflag == IN_CRYPTO && (dev[devicen]->conflag & CON_LX) != 0)
     {  // server
     rp = dev[0];
     ip = dev[devicen];
     sp = ip;   // save to remote devdata
     for(j = 0 ; j < 6 ; ++j)
       {
       ia[j] = ip->baddr[5-j];
       if((rp->leaddtype & 1) != 0)
         ra[j] = gpar.randbadd[5-j];
       else   
         ra[j] = rp->baddr[5-j];
       }
     ia[6] = ip->leaddtype & 1;
     ra[6] = rp->leaddtype & 1;     
     
     if(insdat[n] == 1)
       {
       VPRINT "GOT pair request - SEND response\n");
       pres[PAKHEADSIZE+10] = 3;  // no inout JustWorks      
       pres[PAKHEADSIZE+12] = 0;  // no passkey/bond
       pres[PAKHEADSIZE+15] = 0;  // no link key
 
       if((sp->linkflag & KEY_FILE) != 0)
         rwlinkey(4,devicen,NULL); // used

       sp->cryptoflag = CRY_NEW;
               
       if(insdat[n+1] == KEY_DISPLAY)
         pres[PAKHEADSIZE+10] = DISPLAY_ONLY;       
       else if(insdat[n+1] == DISPLAY_ONLY)
         pres[PAKHEADSIZE+10] = KEY_DISPLAY;       
       
       if((insdat[n+3] & 1) != 0)
         {
         pres[PAKHEADSIZE+12] = 1;   // bond
         sp->cryptoflag |= CRY_BOND;
         }
       if((insdat[n+3] & 4) != 0)
         {
         pres[PAKHEADSIZE+12] |= 4;  // passkey
         sp->cryptoflag |= CRY_PKEY;
         }
         
       if((insdat[n+3] & 8) != 0 && (rp->lepairflags & SECURE_CONNECT) != 0)
         {      
         // got public keys from OCF 25 call?   
         getout = 0;
         for(j = 0 ; j < 32 && getout == 0 ; ++j)
           getout |= rp->pubkeyx[j];
         if(getout != 0)
           {
           pres[PAKHEADSIZE+12] |= 8;  // sc
           sp->cryptoflag |= CRY_SC;
           }
         else
           NPRINT "No keys - SECURE_CONNECT cancelled\n");
         }
                  
       if((insdat[n+6] & 1) != 0)
         pres[PAKHEADSIZE+15] = 1;   // link

       if((rp->lepairflags & JUST_WORKS) != 0)
         {    
         pres[PAKHEADSIZE+10] = 3;   // no io                    
         pres[PAKHEADSIZE+12] &= 9;  // nix passkey
         sp->cryptoflag &= ~CRY_PKEY;
         }                    
         
       sendhci(pres,devicen);
       
       // save for confirm calc
       for(j = 0 ; j < 7 ; ++j)
         {
         sp->preq[j] = insdat[n+j];   // request
         sp->pres[j] = pres[j+PAKHEADSIZE+9];      
         }
  
       getrand(sp->rrand,16);

       if((rp->lepairflags & JUST_WORKS) != 0 || (sp->cryptoflag & CRY_PKEY) == 0)
         rp->lepasskey = 0;
       else if((sp->cryptoflag & CRY_PKEY) != 0 && sp->preq[1] == KEY_DISPLAY && (rp->lepairflags & PASSKEY_FIXED) == 0)
         {
         getrand(key,3);
         //key[0] = rand() & 0xFF;
         //key[1] = rand() & 0xFF;
         key[2] &= 0x0F;
         if(key[2] == 0x0F)
           key[2] = 0x0E;
         rp->lepasskey = (key[2] << 16) + (key[1] << 8) + key[0];
         flushprint();
         sprintf(sbuf,"Passkey = %06d\n",rp->lepasskey);
         printn(sbuf,strlen(sbuf));
         }
  
       sp->pkround = 0;  // sc passkey bit
       gpar.dhkeydev = 0; 
       }          
     else if(insdat[n] == 3)
       {
       if((sp->cryptoflag & CRY_SC) == 0)
         {  // legacy
         VPRINT "GOT I confirm - SEND R confirm\n");
         if((sp->cryptoflag & CRY_PKEY) != 0 && sp->preq[1] != KEY_DISPLAY)
           {
           if(sp->preq[1] != DISPLAY_ONLY)
              NPRINT "WARNING - Expected client to have a DisplayOnly agent\n");
           flushprint();
           sbuf[0] = 0;
           inputpin("Input passkey: ",sbuf);  
           rp->lepasskey = atoi(sbuf);
           }

         for(j = 0 ; j < 16 ; ++j)
           {
           sp->confirm[j] = insdat[n+j+1];  // I confirm
           key[j] = 0;   // Just Works
           }    

         if((sp->cryptoflag & CRY_PKEY) != 0)
           {
           j = rp->lepasskey;          
           key[0] = j & 0xFF;
           key[1] = (j >> 8) & 0xFF;
           key[2] = (j >> 16) & 0xFF;
           }
                                           
         if(calcc1(key,sp->rrand,sp->preq,sp->pres,ia[6],ra[6],ia,ra,confirm+PAKHEADSIZE+10) == 0)
           {
           NPRINT "No crypto\n");
           confail[PAKHEADSIZE+10] = 5;
           sendhci(confail,devicen);
           }
         else
           sendhci(confirm,devicen);
         }
       else if((sp->cryptoflag & CRY_PKEY) != 0)
         {  // sc passkey round
         VPRINT "Got I confirm. Send R confirm bit %d\n",sp->pkround);

         for(j = 0 ; j < 16 ; ++j)
           sp->confirm[j] = insdat[n+j+1];  // I confirm
  
         getrand(sp->rrand,16);
         rby = ((rp->lepasskey >> sp->pkround) & 1) | 0x80;
         if(calcf4(rp->pubkeyx,sp->pubkeyx,sp->rrand,rby,confirm+PAKHEADSIZE+10) == 0)
           {
           NPRINT "No crypto\n");
           confail[PAKHEADSIZE+10] = 5;
           sendhci(confail,devicen);
           }
         else        
           sendhci(confirm,devicen);
         }  
       }
     else if(insdat[n] == 0x0C)
       {
       if(gpar.sccap == 0)
         {
         NPRINT "Secure Connect not supported by Bluetooth adapter\n");
         confail[PAKHEADSIZE+10] = 11;  // DHkey check fail
         sendhci(confail,devicen);
         }
       else
         { 
         sp->cryptoflag |= CRY_SC;  // sc 
         VPRINT "Got remote XY keys. Calc DHkey. Send XY keys\n");

         if((sp->cryptoflag & CRY_PKEY) != 0 && sp->preq[1] != KEY_DISPLAY)
           {
           if(sp->preq[1] != DISPLAY_ONLY)
              NPRINT "WARNING - Expected client to have a DisplayOnly agent\n");
           flushprint();
           sbuf[0] = 0;   
           inputpin("Input passkey: ",sbuf);   
           rp->lepasskey = atoi(sbuf);
           }

         for(j = 0 ; j < 32 ; ++j)
           {
           sp->pubkeyx[j] = insdat[n+j+1];
           sp->pubkeyy[j] = insdat[n+j+33];
           dhkey[j+PAKHEADSIZE+4] = sp->pubkeyx[j];
           dhkey[j+PAKHEADSIZE+36] = sp->pubkeyy[j];
           sp->dhkey[j] = 0;
           }
         if(gpar.dhkeydev != 0)
           NPRINT "WARNING - Two devices connecting?\n");  
         gpar.dhkeydev = devicen;
         sendhci(dhkey,0); 
         sleep_ms(200);
         for(j = 0 ; j < 32 ; ++j)
           {
           sendxykeys[j+PAKHEADSIZE+10] = rp->pubkeyx[j];
           sendxykeys[j+PAKHEADSIZE+42] = rp->pubkeyy[j];
           }      
         sendhci(sendxykeys,devicen);
       
         if((sp->cryptoflag & CRY_PKEY) == 0)
           { // no passkey      
           VPRINT "Send confirm\n");
           calcf4(rp->pubkeyx,sp->pubkeyx,sp->rrand,0,confirm+PAKHEADSIZE+10);
           sendhci(confirm,devicen);
           }
         }  // end sccap=1
       }
     else if(insdat[n] == 4)
       {
       VPRINT "GOT I random\n");
       for(j = 0 ; j < 16 ; ++j)
         {
         sp->irand[j] = insdat[n+j+1];
         key[j] = 0;   // Just Works 
         }
         
       cmpok = 1;  
       if((sp->cryptoflag & CRY_SC) == 0)
         {  // legacy
         if((sp->cryptoflag & CRY_PKEY) != 0)
           {
           j = rp->lepasskey;          
           key[0] = j & 0xFF;
           key[1] = (j >> 8) & 0xFF;
           key[2] = (j >> 16) & 0xFF;
           }
               
         calcc1(key,sp->irand,sp->preq,sp->pres,ia[6],ra[6],ia,ra,out);
         cmpok = bincmp(sp->confirm,out,16,DIRN_FOR);
                  
         if(cmpok != 0)   
           VPRINT "I confirm OK\n");
         else
           {
           NPRINT "PAIR confirm fail\n");
           confail[PAKHEADSIZE+10] = 4;
           sendhci(confail,devicen);
           }
         }
       else
         {
         if((sp->cryptoflag & CRY_PKEY) != 0)
           {  // sc passkey round
           rby = (rp->lepasskey >> sp->pkround) & 1;
           rby |= 0x80;
           calcf4(sp->pubkeyx,rp->pubkeyx,sp->irand,rby,out);
           cmpok = bincmp(sp->confirm,out,16,DIRN_FOR);
           if(cmpok != 0)   
             VPRINT "I confirm OK\n");
           else
             {
             NPRINT "PAIR confirm fail\n");
             confail[PAKHEADSIZE+10] = 4;
             sendhci(confail,devicen);
             }         
           ++sp->pkround;
           }
         else
           {  // numeric comparison
           j = calcg2(sp->pubkeyx,rp->pubkeyx,sp->irand,sp->rrand);
           NPRINT "Numeric comparison passkey = %d\n",j);
           }
         }  
           
       if(cmpok != 0)
         {
         VPRINT "SEND R random\n");
         for(j = 0 ; j < 16 ; ++j)
           sendrand[j+PAKHEADSIZE+10] = sp->rrand[j];         
         sendhci(sendrand,devicen);
         } 
       else
         {
         NPRINT "PAIR confirm fail\n");
         confail[PAKHEADSIZE+10] = 4;
         sendhci(confail,devicen);
         }
       }
     else if(insdat[n] == 0x0D)
       {
       VPRINT "Got DHchk\n");
       for(j = 0 ; j < 16 ; ++j)
         key[j] = 0;
       if((sp->cryptoflag & CRY_PKEY) != 0)
         {
         j = rp->lepasskey;          
         key[0] = j & 0xFF;
         key[1] = (j >> 8) & 0xFF;
         key[2] = (j >> 16) & 0xFF;
         }
     
       getout = 0;
       for(j = 0 ; j < 32 && getout == 0 ; ++j)
         getout |= sp->dhkey[j];
       if(getout == 0)
         {
         NPRINT "No DHkey\n");
         confail[PAKHEADSIZE+10] = 11;
         sendhci(confail,devicen);
         }
       else
         {         
         calcf5(sp->dhkey,sp->irand,sp->rrand,ia,ra,mackey,sp->linkey);
         calcf6(mackey,sp->irand,sp->rrand,key,sp->preq+1,ia,ra,out);  
         if(bincmp(out,insdat+n+1,16,DIRN_FOR) == 0)
           {                  
           NPRINT "DH check fail\n");
           confail[PAKHEADSIZE+10] = 11;
           sendhci(confail,devicen);
           }
         else
           {
           VPRINT "Send DHchk\n");    
           calcf6(mackey,sp->rrand,sp->irand,key,sp->pres+1,ra,ia,dhcheck+PAKHEADSIZE+10);
           sendhci(dhcheck,devicen);
           }
         } 
       }
     else if(insdat[n] == 0xFF)
       {
       if((sp->linkflag & KEY_FILE) != 0)
         rwlinkey(4,devicen,NULL); // used

       if((sp->cryptoflag & CRY_NEW) == 0)
         {
         scflag = 0;
         for(j = 0 ; j < 10 && scflag == 0 ; ++j)
           scflag |= insdat[n+j+3];            

         if(scflag != 0 && (sp->linkflag & KEY_FILE) != 0)
           {  // legacy only - addr in CI
           // address may have changed - index at [n+3] = old address
           rwlinkey(0,devicen,insdat+n+3);
           rwlinkey(4,devicen,NULL);  // used
           }
           
         if((sp->linkflag & (KEY_NEW | KEY_FILE)) == 0)
           {
           NPRINT "PAIR fail - No bond info\n");       
           sendhci(negkey,devicen);
           }
         else
           {
           if(scflag == 0)
             sp->cryptoflag |= CRY_SC;  // divrand[0] set by NEW/FILE pair entry 
           else
             sp->divrand[0] = insdat[n+11]; // legacy CI ediv 0=was jw 1=was pk
           for(j = 0 ; j < 16 ; ++j)
             sendkey[j+PAKHEADSIZE+6] = sp->linkey[j];
           VPRINT "SEND key\n");
           sendhci(sendkey,devicen);
           }
         }
       else 
         {
         if((sp->cryptoflag & CRY_SC) == 0)
           {  // legacy
           VPRINT "STK encrypt\n");
           for(j = 0 ; j < 16 ; ++j)
             key[j] = 0;
  
           if((sp->cryptoflag & CRY_PKEY) != 0)
             {
             j = rp->lepasskey;          
             key[0] = j & 0xFF;
             key[1] = (j >> 8) & 0xFF;
             key[2] = (j >> 16) & 0xFF;
             }
                    
           calcs1(key,sp->rrand,sp->irand,sp->linkey);
           }
         else
           VPRINT "Send LTK\n");
           
         for(j = 0 ; j < 16 ; ++j)
           sendkey[j+PAKHEADSIZE+6] = sp->linkey[j];
         sendhci(sendkey,devicen);
         }
       }
     else if(insdat[n] == 0xFE)
       {
       if((sp->cryptoflag & CRY_NEW) == 0)
         { // reconnect
         VPRINT "Paired using bond info\n");
         for(j = 0 ; ctic(0,j)->type == CTIC_ACTIVE ; ++j)
           {   // HID Reports re-enable notify 
           cp = ctic(0,j);
           if(cp->uuidtype == 2 && (cp->perm & 0x30) != 0 && cp->uuid[0] == 0x2A && cp->uuid[1] == 0x4D)
              cp->notify = 1;
           }
         if((gpar.hidflag & 1) != 0)
           {  
           sp->cryptoflag |= CRY_AUTHOK;    
           buf[0] = AUTO_PAIROK; 
           pushins(IN_AUTOEND,devicen,1,buf);
           }
         else
           {
           if(sp->divrand[0] == 1)
             sp->cryptoflag |= CRY_AUTHOK | CRY_PKEY;   
           }
         }
       else
         {
         if((sp->cryptoflag & CRY_BOND) != 0)
           {  // bond
           VPRINT "BOND\n");          
           if((sp->cryptoflag & CRY_SC) == 0)
             { // legacy 
             getrand(sp->linkey,16);
             if(sp->linkey[15] == 0)
               sp->linkey[15] = 0xA5;  // avoid devrand signature
             
             for(j = 0 ; j < 16 ; ++j)
               sendltk[j+PAKHEADSIZE+10] = sp->linkey[j];
      
             sendhci(sendltk,devicen);  
                     
             for(j = 0 ; j < 6 ; ++j)
               sendci[j+PAKHEADSIZE+12] = sp->baddr[j];  // devrand
    
             if((sp->cryptoflag & CRY_PKEY) == 0)
               sendci[PAKHEADSIZE+10] = 0;  // ediv jw
             else
               sendci[PAKHEADSIZE+10] = 1;  // pk can auth on repair
 
             sendhci(sendci,devicen);
             }
           else
             {  // save pkeY FOR AUTH ON RECON 
             for(j = 0 ; j < 16 ; ++j)
               sp->divrand[j] = 0;
             if((sp->cryptoflag & CRY_PKEY) != 0)
               sp->divrand[0] = 1;
             else
               sp->divrand[0] = 0;
             
             sp->linkflag |= PAIR_NEW;
             }     
           sp->linkflag |= KEY_NEW;  // bond req saves LEkey
           save_pico_info();
           }                           
          
              
         if((gpar.hidflag & 1) != 0)
           sp->cryptoflag |= CRY_AUTHOK | CRY_POKNOTEN;   // auth + send AUTO_PAIROK when notify enabled in leserver 
         else
           {
           if((sp->cryptoflag & CRY_PKEY) != 0)  // passkey for auth
             sp->cryptoflag |= CRY_AUTHOK;
           else if((rp->lepairflags & AUTHENTICATION_ON) != 0)
             NPRINT "Must bond AUTHENTICATION_ON server with passkey\n");
           }
         }
       if((gpar.hidflag & 1) == 0)
         NPRINT "PAIR OK\n");
       // AUTO_PAIROK    
       }
     else if(insdat[n] == 0xFD || insdat[n] == 5)
       {
       if(insdat[n] == 5)
         {
         j = insdat[n+1];
         if(j > 15)
           j = 8;
         NPRINT "PAIR FAIL %s\n",errorcry[j]); // codes V3 pH 3.5.5
         }
       else
         {   
         j = insdat[n+3];
         if(j > 69)
           j = 0;  // no error0f text
         NPRINT "PAIR FAIL %s\n",error0f[j]);
         }
       if((gpar.hidflag & 1) != 0)
         {  
         buf[0] = AUTO_PAIRFAIL; 
         pushins(IN_AUTOEND,devicen,1,buf);
         }
       }
     }
    else if(gotflag == IN_CRYPTO && (dev[devicen]->conflag & CON_LX) == 0)
      {  // client
      ip = dev[0];
      rp = dev[devicen];
      sp = rp;
      for(j = 0 ; j < 6 ; ++j)
        {
        ia[j] = ip->baddr[5-j];
        ra[j] = rp->baddr[5-j];
        }       
     ia[6] = ip->leaddtype & 1;
     ra[6] = rp->leaddtype & 1;     

      
     if(insdat[n] == 2)
       {
       VPRINT "GOT pair response\n");
       // save for confirm calc
       for(j = 0 ; j < 7 ; ++j)
         sp->pres[j] = insdat[n+j];  // response 
         
       if((sp->lepairflags & BOND_NEW) != 0 && (sp->pres[3] & 3) == 0)                  
         NPRINT "Bonding refused\n");
    
       if((sp->linkflag & KEY_FILE) != 0)
         rwlinkey(4,devicen,NULL); // used

       sp->cryptoflag = CRY_NEW;
       j = sp->pres[3] & sp->preq[3];
       if((j & 1) != 0)
         sp->cryptoflag |= CRY_BOND;
       if((j & 4) != 0 || ( (sp->preq[3] & 4) != 0 &&
               ( (sp->preq[1] == DISPLAY_ONLY && sp->pres[1] == KEY_DISPLAY) ||
                 (sp->preq[1] == KEY_DISPLAY && sp->pres[1] == DISPLAY_ONLY) ) ))
         sp->cryptoflag |= CRY_PKEY;   // both mitm set or request mitm and correct agent pair
       if((j & 8) != 0)
         sp->cryptoflag |= CRY_SC;
      
       getrand(sp->irand,16);    

       for(j = 0 ; j < 16 ; ++j)
         key[j] = 0;   // Just Works

       sp->pkround = 0;
       gpar.dhkeydev = 0;  
  
       if((sp->lepairflags & PASSKEY_RANDOM) != 0)
         {
         if(sp->pres[1] == NO_INOUT)   
           {
           NPRINT "Server wants Just Works\n");
           sp->lepasskey = 0;
           sp->cryptoflag &= ~CRY_PKEY;
           }
         else if((sp->lepairflags & PASSKEY_REMOTE) != 0)
           { // passkey displayed on server
           if(sp->pres[1] != DISPLAY_ONLY)
             NPRINT "WARNING - Expected server to have a DisplayOnly agent\n");
           flushprint();
           sbuf[0] = 0;      
           inputpin("Input passkey: ",sbuf);
           sp->lepasskey = atoi(sbuf);
           }
         else if((sp->lepairflags & PASSKEY_LOCAL) != 0)
           {
           if(sp->pres[1] != KEY_DISPLAY)
             NPRINT "WARNING - Expected server to have KeyboardDisplay agent\n");
           getrand(key,3);
           key[2] &= 0x0F;
           if(key[2] == 0x0F)
             key[2] = 0x0E;
           sp->lepasskey = (key[2] << 16) + (key[1] << 8) + key[0];
           flushprint();
           sprintf(sbuf,"Passkey = %06d\n",sp->lepasskey);
           printn(sbuf,strlen(sbuf));
           }
         }
       
       if((sp->cryptoflag & CRY_SC) == 0)
         {  // legacy
         VPRINT "SEND I confirm\n");
         j = sp->lepasskey;          
         key[0] = j & 0xFF;
         key[1] = (j >> 8) & 0xFF;
         key[2] = (j >> 16) & 0xFF;    
    
         if(calcc1(key,sp->irand,sp->preq,sp->pres,ia[6],ra[6],ia,ra,confirm+PAKHEADSIZE+10) == 0)
           {
           NPRINT "No crypto\n");
           confail[PAKHEADSIZE+10] = 5;
           sendhci(confail,devicen);
           }
         else
           sendhci(confirm,devicen);
         }
       else
         {
         VPRINT "SEND public keys\n");
         for(j = 0 ; j < 32 ; ++j)
           {
           sendxykeys[j+PAKHEADSIZE+10] = ip->pubkeyx[j];
           sendxykeys[j+PAKHEADSIZE+42] = ip->pubkeyy[j];
           }      
         sendhci(sendxykeys,devicen);
         }       
       }
     else if(insdat[n] == 0x0C)
       {
       if(gpar.sccap == 0)
         {
         NPRINT "Secure Connect not supported by Bluetooth adapter\n");
         confail[PAKHEADSIZE+10] = 11;  // DHkey check fail
         sendhci(confail,devicen);
         }
       else
         {
         sp->cryptoflag |= CRY_SC; 
         VPRINT "Got remote XY keys. Calc DHkey\n");
         for(j = 0 ; j < 32 ; ++j)
           {
           sp->pubkeyx[j] = insdat[n+j+1];
           sp->pubkeyy[j] = insdat[n+j+33];
           dhkey[j+PAKHEADSIZE+4] = sp->pubkeyx[j];
           dhkey[j+PAKHEADSIZE+36] = sp->pubkeyy[j];
           sp->dhkey[j] = 0;
           }
         if(gpar.dhkeydev != 0)
           NPRINT "WARNING - Two devices connecting?\n");  
         gpar.dhkeydev = devicen;
         sendhci(dhkey,0); 
         sleep_ms(200);
         if((sp->cryptoflag & CRY_PKEY) != 0)
           {  // start 20 pkrounds
           VPRINT "Send 1st I confirm\n");
           sp->pkround = 0;
           getrand(sp->irand,16);    
           rby = (sp->lepasskey & 1) | 0x80;
           if(calcf4(ip->pubkeyx,sp->pubkeyx,sp->irand,rby,confirm+PAKHEADSIZE+10) == 0)
             {
             NPRINT "No crypto\n");
             confail[PAKHEADSIZE+10] = 5;
             sendhci(confail,devicen);
             }
           else
             sendhci(confirm,devicen);         
           }
         }  // end sccap=1        
       }
     else if(insdat[n] == 3)
       {
       VPRINT "GOT R confirm - SEND I rand\n");
       
       for(j = 0 ; j < 16 ; ++j)
         sp->confirm[j] = insdat[n+j+1];  // R confirm
     
       for(j = 0 ; j < 16 ; ++j)
         sendrand[PAKHEADSIZE+10+j] = sp->irand[j];         
       sendhci(sendrand,devicen);
       }
     else if(insdat[n] == 4)
       {
       VPRINT "GOT R random\n");
       for(j = 0 ; j < 16 ; ++j)
         {
         sp->rrand[j] = insdat[n+j+1];
         key[j] = 0;   // Just Works 
         }
       if((sp->cryptoflag & CRY_SC) == 0)
         {  // legacy         
         j = sp->lepasskey;          
         key[0] = j & 0xFF;
         key[1] = (j >> 8) & 0xFF;
         key[2] = (j >> 16) & 0xFF;    
         
         if(calcc1(key,sp->rrand,sp->preq,sp->pres,ia[6],ra[6],ia,ra,out) == 0)
           {
           NPRINT "No crypto\n");
           confail[PAKHEADSIZE+10] = 5;
           sendhci(confail,devicen);
           }
         else if(bincmp(sp->confirm,out,16,DIRN_FOR) != 0)
           {
           VPRINT "R confirm OK\n");
           VPRINT "SEND LTK encrypt\n");
           calcs1(key,sp->rrand,sp->irand,sp->linkey);
           for(j = 0 ; j < 16 ; ++j)
             ltkcrypt[j+PAKHEADSIZE+16] = sp->linkey[j];
           sendhci(ltkcrypt,devicen);
           }
         else
           {
           NPRINT "PAIR confirm fail\n");
           confail[PAKHEADSIZE+10] = 4;
           sendhci(confail,devicen);
           buf[0] = AUTO_PAIRFAIL;
           pushins(IN_AUTOEND,devicen,1,buf);
           }
         }
       else if((sp->cryptoflag & CRY_PKEY) != 0 && sp->pkround < 19)
         {  // loop for next round
         // check rC sp->confirm
         rby = ((sp->lepasskey >> sp->pkround) & 1) | 0x80;
         calcf4(sp->pubkeyx,ip->pubkeyx,sp->rrand,rby,out);
         if(bincmp(sp->confirm,out,16,DIRN_FOR) == 0)
           {
           NPRINT "R confirm round %d fail\n",sp->pkround+1);
           confail[PAKHEADSIZE+10] = 4;
           sendhci(confail,devicen);
           buf[0] = AUTO_PAIRFAIL;
           pushins(IN_AUTOEND,devicen,1,buf);          
           }
         else
           {
           ++sp->pkround;   
           VPRINT "Send I confirm round %d\n",sp->pkround+1);
           getrand(sp->irand,16);    
           rby = (sp->lepasskey >> sp->pkround) & 1;
           rby |= 0x80;
           calcf4(ip->pubkeyx,sp->pubkeyx,sp->irand,rby,confirm+PAKHEADSIZE+10);
           sendhci(confirm,devicen);
           }         
         }
       else
         {  // sc no PKEY or PKEY last round
         // check rC sp->confirm
         if((sp->cryptoflag & CRY_PKEY) == 0)
           rby = 0;
         else
           {         
           rby = (sp->lepasskey >> sp->pkround) & 1;
           rby |= 0x80;
           }
         calcf4(sp->pubkeyx,ip->pubkeyx,sp->rrand,rby,out);
         if(bincmp(sp->confirm,out,16,DIRN_FOR) == 0)
           {
           NPRINT "PAIR confirm fail\n");
           confail[PAKHEADSIZE+10] = 4;
           sendhci(confail,devicen);
           buf[0] = AUTO_PAIRFAIL;
           pushins(IN_AUTOEND,devicen,1,buf);          
           }
         for(j = 0 ; j < 16 ; ++j)
           key[j] = 0;
         if((sp->cryptoflag & CRY_PKEY) != 0)
           {
           j = sp->lepasskey;          
           key[0] = j & 0xFF;
           key[1] = (j >> 8) & 0xFF;
           key[2] = (j >> 16) & 0xFF;
           }
         else
           {
           j = calcg2(ip->pubkeyx,sp->pubkeyx,sp->irand,sp->rrand);
           NPRINT "Numeric comparison passkey = %d\n",j);
           }
         getout = 0;
         for(j = 0 ; j < 32 && getout == 0 ; ++j)
           getout |= sp->dhkey[j];
         if(getout == 0)
           {
           NPRINT "No DHkey\n");
           confail[PAKHEADSIZE+10] = 11;
           sendhci(confail,devicen);
           }
         else
           {         
           calcf5(sp->dhkey,sp->irand,sp->rrand,ia,ra,mackey,sp->linkey);   
           calcf6(mackey,sp->irand,sp->rrand,key,sp->preq+1,ia,ra,dhcheck+PAKHEADSIZE+10);
           VPRINT "Send I DHchk\n");
           sendhci(dhcheck,devicen);
           calcf6(mackey,sp->rrand,sp->irand,key,sp->pres+1,ra,ia,sp->confirm);  
           // expect sp->confirm = R DHchk
           }       
         }           
       }
     else if(insdat[n] == 0x0D)
       {
       VPRINT "Got R DHchk\n");
       for(j = 0 ; j < 16 ; ++j)
         key[j] = 0;
            
       getout = 0;
       for(j = 0 ; j < 32 && getout == 0 ; ++j)
         getout |= sp->dhkey[j];
       if(getout == 0)
         {
         NPRINT "No DHkey\n");
         confail[PAKHEADSIZE+10] = 11;
         sendhci(confail,devicen);
         }
       else
         { 
         if(bincmp(sp->confirm,insdat+n+1,16,DIRN_FOR) == 0)
           {                  
           NPRINT "DH check fail\n");
           confail[PAKHEADSIZE+10] = 11;
           sendhci(confail,devicen);
           }
         else
           {
           VPRINT "SEND LTK encrypt\n");
           for(j = 0 ; j < 16 ; ++j)
             ltkcrypt[j+PAKHEADSIZE+16] = sp->linkey[j];
           sendhci(ltkcrypt,devicen);
           }
         } 
       }
     else if((sp->cryptoflag & CRY_DONE) == 0 && (insdat[n] == 0x07 || insdat[n] == 0xFE))
       {
       if(insdat[n] == 0xFE && (sp->lepairflags & BOND_NEW) != 0 && (sp->cryptoflag & CRY_SC) != 0)
         {
         sp->linkflag |= KEY_NEW;
         save_pico_info();
         }    
       VPRINT "PAIR OK\n");
       buf[0] = AUTO_PAIROK;
       pushins(IN_AUTOEND,devicen,1,buf);
       sp->cryptoflag |= CRY_DONE;
       }
     else if((sp->cryptoflag & CRY_DONE) == 0 && (insdat[n] == 5 || insdat[n] == 0xFD))
       {
       if(insdat[n] == 5)
         {
         j = insdat[n+1];
         if(j > 15)
           j = 8;
         NPRINT "PAIR FAIL %s\n",errorcry[j]);  // codes V3 pH 3.5.5 
         }
       else
         {   
         j = insdat[n+3];
         if(j > 69)
           j = 0;  // no error0f text
         NPRINT "PAIR FAIL %s\n",error0f[j]);
         }
       buf[0] = AUTO_PAIRFAIL;
       pushins(IN_AUTOEND,devicen,1,buf);
       sp->cryptoflag |= CRY_DONE;
       }
       
     if(insdat[n] == 7)
       {
       for(j = 0 ; j < 10 ; ++j)
         sp->divrand[j] = insdat[n+j+1];
       if((sp->lepairflags & BOND_NEW) != 0)
         sp->linkflag |= PAIR_NEW;
       save_pico_info();  // 6 done?
       }  
     if(insdat[n] == 6)
       {
       for(j = 0 ; j < 16 ; ++j)
         sp->linkey[j] = insdat[n+j+1];
       if((sp->lepairflags & BOND_NEW) != 0)
         sp->linkflag |= KEY_NEW;  // saves LEkey
       }
     if(insdat[n] == 0x0B)
       NPRINT "Server is asking to pair - call le_pair\n");
     }  
    else 
      VPRINT "Unrecognised immediate\n");
    
    if(popflag == 0)                  
      instack[n] = INS_POP;
    }

 
    
  flushprint();
  }



/******* LE SERVER ************/

int leserver(int ndevice,int count,unsigned char *dat,int insn)
  {
  int n,dn,cticn,flag,notflag,handle,start,end,startx,psflag,eog;
  int size,uuidtype,aflag,xflag,acticn,ahandle,psn,datcount,popflag;
  unsigned char cmd[4],*s,*data,errcode;
  struct cticdata *cp;
  static unsigned char buf[32];
  
  VPRINT "GOT LE server opcode %02X from %s\n",dat[0],dev[ndevice]->name); 
  flushprint();

  cticn = 0;
  errcode = 0;  
  aflag = 0;   // 0A opcode for non-value
  notflag = 0;
  cmd[0] = 0;
  cmd[2] = 0; 
  acticn = 0;
  ahandle = 0;
  xflag = 0;   // stop error
  uuidtype = 0;
  size = 0;
  popflag = 0;
  // node = dev[0]->node;
  
  if(dat[0] == 0x52 || dat[0] == 0x12 || dat[0] == 0x0A)
    {  // read/write
    if((dev[0]->lepairflags & AUTHENTICATION_ON) != 0 &&
            (dev[ndevice]->cryptoflag & CRY_AUTHOK) == 0) 
      {
      lefail[PAKHEADSIZE+10] = dat[0];  // operation
      lefail[PAKHEADSIZE+11] = dat[1];
      lefail[PAKHEADSIZE+12] = dat[2];
      lefail[PAKHEADSIZE+13] = 5;
      NPRINT "No Authentication - must pair with passkey\n"); 
      sendhci(lefail,ndevice);
      flushprint();
      return(0);
      }


    xflag = 1;   // stop opcode not supported
    flag = 0;      
    handle = dat[1] + (dat[2] << 8);
    
    // look for PS
    psn = -1;
    for(n = 0 ; pserv[n].handle > 0 && psn < 0 && n < 32 ; ++n) 
      {
      if(pserv[n].handle == handle)
        psn = n;
      }
    if(psn >= 0)
      {  // PS
      if(dat[0] != 0x0A)
        {
        VPRINT "Write not permitted\n");
        errcode = 3;  // write not permit
        }
      else
        {
        size = pserv[psn].uuidtype;
        for(n = 0 ; n < size ; ++n)
          lereadreply[PAKHEADSIZE+10+n] = pserv[psn].uuid[size-1-n];
        VPRINT "SEND Primary service UUID\n");
        flag = 2;
        }    
      }
         
      // find cticn of handle
 
    for(cticn = 0 ; ctic(0,cticn)->type == CTIC_ACTIVE && flag == 0 && aflag == 0 && errcode == 0 ; ++cticn)
      {
      cp = ctic(0,cticn);
      if(handle == cp->chandle-1)
        {
        if(dat[0] == 0x0A)
          {
          aflag = 3;   // 0A read of INFO - treat as 08
          acticn = cticn;
          ahandle = handle;
          }
        else
          {
          VPRINT "Write not permitted\n");
          errcode = 3;  // write not permit
          }          
        }
      else if(cp->chandle == handle || ((cp->perm & 0x30) != 0 && handle == cp->chandle+1 ) ||
               (cp->reportflag != 0 && (handle == cp->chandle+2 || handle == cp->chandle+3)) )
        { 
        if(handle == cp->chandle+1)
          notflag = 1;  // notify
        else if(handle == cp->chandle+2)
          notflag = 2;  //  ctic extended props
        else if(handle == cp->chandle+3)
          notflag = 3;  // report ref

        if(dat[0] == 0x52 || dat[0] == 0x12)
          {  // write  12 ack  52 no ack
          if(notflag == 0)
            {
            if((cp->perm & 0x0C) == 0)
              {   // 8 ack  4 no ack
              VPRINT "Write not permitted\n");
              errcode = 3;  // write not permit
              }
            else
              {
              VPRINT "Received characteristic %s\n",cp->name);
                           
              datcount = count-3;
              data = dat+3;
              
              if(datcount > LEDATLEN)
                datcount = LEDATLEN;
  
              for(n = 0 ; n < datcount ; ++n)
                cp->value[n] = data[n];
              for(n = datcount ; n < LEDATLEN ; ++n)
                cp->value[n] = 0;
                                
              cp->size = datcount;  
              if(dat[0] == 0x12)
                {  // no check cp->perm & 8 write with ack
                VPRINT "Send acknowledgement\n");
                sendhci(leack,ndevice);  // send ack
                }
              cmd[0] = LE_WRITE;
              cmd[1] = cticn;
              flag = 1;
              }
            }
          else if(notflag == 1)   // notify descriptor
            {
            cp->notify = dat[3];
            if(cp->notify == 0)
              VPRINT "%s notify disable\n",cp->name);
            else
              {
              VPRINT "%s notify enable\n",cp->name);
              if((dev[ndevice]->cryptoflag & CRY_POKNOTEN) != 0 && cp->uuidtype == 2 && cp->uuid[0] == 0x2A && cp->uuid[1] == 0x4D)
                {
                buf[0] = AUTO_PAIROK;
                pushins(IN_AUTOEND,ndevice,1,buf);
                dev[ndevice]->cryptoflag &= ~CRY_POKNOTEN;
                }
              } 
            if(dat[0] == 0x12)
              {
              VPRINT "Send acknowledgement\n");
              sendhci(leack,ndevice);  // send ack
              }
            if(cp->notify == 0)
              cmd[0] = LE_NOTIFY_DISABLE;
            else  
              cmd[0] = LE_NOTIFY_ENABLE;
            cmd[1] = cticn;            
            flag = 1;
            }
          else
            {
            VPRINT "Write not permitted\n");
            errcode = 3;  // write not permit
            }
            
          }
        else
          {  // read
          if(notflag == 0)
            {
            if((cp->perm & 2) == 0)
              {
              VPRINT "Read not permitted\n");
              errcode = 2;  // read not permit
              }
            else
              {
              VPRINT "SEND characteristic %s\n",cp->name);
              instack[insn] = INS_POP;
              popflag = 1;
              // callback here rather than le_server - cmd[2] = retval     
#ifdef BTFPYTHON
              if(py_lecallback != NULL)
                cmd[2] = (unsigned char)py_le_callback(py_lecallback,dev[ndevice]->node,LE_READ,cticn);
#else
              if(gpar.lecallback != NULL)
                cmd[2] = (unsigned char)gpar.lecallback(dev[ndevice]->node,LE_READ,cticn);
#endif

              n = 0;
              while(n < cp->size && n < LEDATLEN)
                {
                lereadreply[PAKHEADSIZE+10+n] = cp->value[n];
                ++n;
                }
              size = n;
                          
              cmd[0] = LE_READ;
              cmd[1] = cticn;
              flag = 2;
              }
            }  
          else if(notflag == 1)   // notify descriptor
            {
            VPRINT "SEND notify status for characteristic %s\n",cp->name);
            size = 2;
            lereadreply[PAKHEADSIZE+10] = cp->notify & 1;
            lereadreply[PAKHEADSIZE+11] = 0;
            flag = 2;
            }
          else if(notflag == 2)           
            {  // ctic extended
            VPRINT "SEND extended props for %s\n",cp->name);
            size = 2;
            lereadreply[PAKHEADSIZE+10] = 0;  // 0x10;
            lereadreply[PAKHEADSIZE+11] = 0;
            flag = 2;
            }
          else if(notflag == 3)  
            {
            VPRINT "SEND report ref for %s\n",cp->name);
            size = 2;
            lereadreply[PAKHEADSIZE+10] = cp->reportid; 
            lereadreply[PAKHEADSIZE+11] = 1;
            flag = 2;
            }
            
          }            
        }  // VN handle
      }  // ctic loop
      
      
    if(flag == 2 && errcode == 0)
      {            
      lereadreply[0] = 10 + size;
      lereadreply[PAKHEADSIZE+3] = (unsigned char)((size+5) & 0xFF);
      lereadreply[PAKHEADSIZE+4] = (unsigned char)(((size+5) >> 8) & 0xFF);
      lereadreply[PAKHEADSIZE+5] = (unsigned char)((size+1) & 0xFF);
      lereadreply[PAKHEADSIZE+6] = (unsigned char)(((size+1) >> 8) & 0xFF);
      sendhci(lereadreply,ndevice);
      }
                
    if(errcode == 0 && (notflag == 0 || notflag == 1) && flag != 0 && cmd[0] != 0)
      pushins(IN_LECMD,ndevice,3,cmd);
   
        
    if(flag == 0 && aflag == 0)
      {
      NPRINT "%s trying to read/write invalid handle %04X\n",dev[ndevice]->name,handle);
      errcode = 1;
      }
      
    if(errcode != 0 && dat[0] == 0x52)
      errcode = 0;   // no error return for 52
    }
    
  if(dat[0] == 0x04)
    {
    flag = 0;
    start = dat[1]+(dat[2] << 8);
    end = dat[3]+(dat[4] << 8);
    VPRINT "Read attribute info for handles %04X to %04X\n",start,end);
    s = le05reply+PAKHEADSIZE;
    le05reply[0] = 15;
    s[3] = 0x0A;
    s[5] = 0x06;
    s[10] = 1;  // 2 byte    
    
    if(start >= 1)
      {    
      cticn = nextctichandle(start,end,&handle,1);
      
      if(cticn >= 0)
        {  // found handle = PS or one of 3 for cticn
        psflag = cticn >> 16;
        cticn &= 0xFFFF;
        if(psflag != 0)
          {
          s[13] = 0x00;  // 2800 
          s[14] = 0x28;
          flag = 1;
          }
        else
          {
          cp = ctic(0,cticn);          
          if(handle == cp->chandle-1)
            {
            s[13] = 0x03;  // 2803 
            s[14] = 0x28;
            flag = 1;
            }
          else if(handle == cp->chandle+1)
            {
            s[13] = 0x02;  // 2902 notify 
            s[14] = 0x29;
            flag = 1;
            }
          else if(handle == cp->chandle+2)
            {
            s[13] = 0x00;  // 2900 Extended 
            s[14] = 0x29;
            flag = 1;
            }
          else if(handle == cp->chandle+3)
            {
            s[13] = 0x08;  // 2908 Report ref 
            s[14] = 0x29;
            flag = 1;
            }
          else if(handle == cp->chandle)  // value uuid
            {
            if(cp->uuidtype == 16)
              {
              s[3] = 0x18;
              s[5] = 0x14;
              s[10] = 2;  // 16 byte
              le05reply[0] = 29;
              }
            for(n = 0 ; n < cp->uuidtype ; ++n)
              s[n+13] = cp->uuid[cp->uuidtype-n-1];
            flag = 1;
            } 
          }           
        s[11] = handle & 0xFF;  // handle
        s[12] = (handle >> 8) & 0xFF;    
        }
      }
         
    if(flag == 0)
      {
      VPRINT "Attribute not found\n");
      errcode = 0x0A;  // attrib not found
      }
    else
      {
      VPRINT "SEND reply opcode 05 for handle %02X%02X\n",s[12],s[11]);         
      sendhci(le05reply,ndevice); 
      }
    }
  else if(dat[0] == 0x06)
    {
    flag = 0;
    start = dat[1]+(dat[2] << 8);
    end = dat[3]+(dat[4] << 8);
    datcount = count-7;
    // assume handle = start
    s = le07reply+PAKHEADSIZE;

    do
      {
      cticn = nextctichandle(start,end,&handle,1);
      if(cticn >= 0)
        {  
        psflag = cticn >> 16;
        psn = cticn & 0xFFFF;
        if(psflag != 0)
          {
          if(dat[5] == 0x00 && dat[6] == 0x28 &&
              ( (datcount == 2 && pserv[psn].uuidtype == 2 && dat[7] == pserv[psn].uuid[1] && dat[8] == pserv[psn].uuid[0]) ||
                (datcount == 16 && pserv[psn].uuidtype == 16 && bincmp(pserv[psn].uuid,dat+7,16,DIRN_REV) != 0)))
            { 
            s[10] = handle & 0xFF;
            s[11] = (handle >> 8) & 0xFF;
            eog = pserv[psn].eog;
            s[12] = eog & 0xFF;
            s[13] = (eog >> 8) & 0xFF;
            flag = 1; 
            }  
          }
        else
          {
          cp = ctic(0,cticn);
          if(handle == cp->chandle-1 && dat[5] == 0x03 && dat[6] == 0x28 && datcount == cp->uuidtype+3)
            {
            buf[0] = cp->perm;
            buf[1] = cp->chandle & 0xFF;  // value handle
            buf[2] = cp->chandle >> 8;
            for(n = 0 ; n < cp->uuidtype ; ++n)
              buf[3+n] = cp->uuid[cp->uuidtype-n-1];
            if(bincmp(buf,dat+7,datcount,DIRN_FOR) != 0)
              {  // value or notify handle
              eog = cp->chandle;
              if((cp->perm & 0x30) != 0)
                ++eog; 
              flag = 1;
              }
            }
          else if(handle == cp->chandle && cp->uuidtype == 2 && dat[5] == cp->uuid[1] && dat[6] == cp->uuid[0]
                          && datcount == cp->size && bincmp(cp->value,dat+7,datcount,DIRN_FOR) != 0)
            {
            eog = pserv[cp->psnx & 31].eog;
            // eog = 0xFFFF;
            flag = 1;
            }
          else if(handle == cp->chandle+1 && dat[5] == 0x02 && dat[6] == 0x29 && datcount == 2 && dat[7] == cp->notify && dat[8] == 0)
            {
            eog = pserv[cp->psnx & 31].eog;
            // eog = 0xFFFF;
            flag = 1;
            }
          else if(handle == cp->chandle+2 && dat[5] == 0x00 && dat[6] == 0x29)
            {
            eog = pserv[cp->psnx & 31].eog;
            // eog = 0xFFFF;
            flag = 1;
            }
          else if(handle == cp->chandle+3 && dat[5] == 0x08 && dat[6] == 0x29)
            {
            eog = pserv[cp->psnx & 31].eog;
            // eog = 0xFFFF;
            flag = 1;
            }
          if(flag != 0)
            {
            s[10] = handle & 0xFF;
            s[11] = (handle >> 8) & 0xFF;
            s[12] = eog & 0xFF;
            s[13] = (eog >> 8) & 0xFF;
            }
          }
        start = handle;
        }
          
      ++start;
      }
    while(flag == 0 && start <= end && cticn >= 0);
     
    if(flag == 0)
      {
      VPRINT "Attribute not found\n");
      errcode = 0x0A;  // attrib not found
      }
    else
      {
      VPRINT "SEND reply opcode 07 for handle %02X%02X\n",s[11],s[10]);         
      sendhci(le07reply,ndevice); 
      }   
    }
  else if(dat[0] == 0x08 || dat[0] == 0x10 || aflag != 0)
    { 
    flag = 0;
    if(aflag != 0)
      {
      start = ahandle;
      end = ahandle;   // dat[1][2] have handle no more dat[]
      }
    else
      {
      start = dat[1]+(dat[2] << 8);
      end = dat[3]+(dat[4] << 8);
      uuidtype = count - 5;
      VPRINT "Read attribute info for handles %04X to %04X and\n  UUID ",start,end);
      for(n = 0 ; n < uuidtype ; ++n)
        VPRINT "%02X",dat[uuidtype+4-n]);
      VPRINT "\n");
      }
      
    s = le09replyv+PAKHEADSIZE;       
       
    if(dat[0] == 0x08 || aflag != 0)
      dn = 0;
    else
      dn = 2;   // insert end of group handle
    
    if(start >= 0)
      {  // characteristics 
      startx = start;
      do
        {
        if(aflag == 0)
          cticn = nextctichandle(startx,end,&handle,1);
        else
          {
          handle = ahandle;
          cticn = acticn;
          }
          
        if(cticn >= 0)
          {
          psflag = cticn >> 16;
          cticn &= 0xFFFF;
          if(psflag != 0)
            {
            if(uuidtype == 2 && dat[5] == 0x00 && dat[6] == 0x28)
              {   // PS
              size = pserv[cticn].uuidtype;
              if(dn != 0)
                {  // insert end of group handle  
                s[13] = pserv[cticn].eog & 0xFF;
                s[14] = (pserv[cticn].eog >> 8) & 0xFF;
                }
              for(n = 0 ; n < size ; ++n)
                s[size+12-n+dn] = pserv[cticn].uuid[n];
              flag = 1;
              }
            }
          else
            {  
            cp = ctic(0,cticn);
            if(aflag != 0)
              uuidtype = cp->uuidtype;
       
            if(handle == cp->chandle-1 && (aflag == 3 || (uuidtype == 2 && dat[5] == 0x03 && dat[6] == 0x28)))
              {   // info 2803
              size = cp->uuidtype + 3;  // beyond 09 len handlo handhi
              if(dn != 0)
                {  // end of group = value handle or +1 if notify
                n = cp->chandle;
                if((cp->perm & 0x30) != 0)
                  ++n; 
                s[13] = n & 0xFF;
                s[14] = n >> 8;
                }
              s[13+dn] = cp->perm;
              s[14+dn] = cp->chandle & 0xFF;  // value handle
              s[15+dn] = cp->chandle >> 8;
              for(n = 0 ; n < cp->uuidtype ; ++n)
                s[n+dn+16] = cp->uuid[cp->uuidtype-n-1];
              flag = 1;
              }
            else if(handle == cp->chandle+1 && aflag == 0 && uuidtype == 2 && dat[5] == 0x02 && dat[6] == 0x29)
              { // notify control 2902
              size = 2;
              if(dn != 0)
                {  // insert end of group handle  
                s[13] = pserv[cp->psnx & 0xFFFF].eog & 0xFF;
                s[14] = (pserv[cp->psnx & 0xFFFF].eog >> 8) & 0xFF;
                }
              s[13+dn] = cp->notify & 1; 
              s[14+dn] = 0;
              flag = 1;
              }
            else if(handle == cp->chandle+2 && aflag == 0 && uuidtype == 2 && dat[5] == 0x00 && dat[6] == 0x29)
              { // Extended 2900
              size = 2;
              if(dn != 0)
                {  // insert end of group handle  
                s[13] = pserv[cp->psnx & 0xFFFF].eog & 0xFF;
                s[14] = (pserv[cp->psnx & 0xFFFF].eog >> 8) & 0xFF;
                }
              s[13+dn] = 0;   // 0x10; 
              s[14+dn] = 0;
              flag = 1;
              }
            else if(handle == cp->chandle+3 && aflag == 0 && uuidtype == 2 && dat[5] == 0x08 && dat[6] == 0x29)
              { // Report ref 2908
              size = 2;
              if(dn != 0)
                {  // insert end of group handle  
                s[13] = pserv[cp->psnx & 0xFFFF].eog & 0xFF;
                s[14] = (pserv[cp->psnx & 0xFFFF].eog >> 8) & 0xFF;
                }
              s[13+dn] = cp->reportid; 
              s[14+dn] = 1;
              flag = 1;
              }
            else if(handle == cp->chandle && aflag == 0 && uuidtype == cp->uuidtype && bincmp(cp->uuid,dat+5,cp->uuidtype,DIRN_REV) != 0)  
              {   // value
              if((dev[0]->lepairflags & AUTHENTICATION_ON) != 0 &&
                                    (dev[ndevice]->cryptoflag & CRY_AUTHOK) == 0) 
                {
                errcode = 5;  // No Authentication
                flag = 2;
                }
              else
                {
                size = cp->size;
                if(dn != 0)
                  {  // insert end of group handle  
                  s[13] = pserv[cp->psnx & 0xFFFF].eog & 0xFF;
                  s[14] = (pserv[cp->psnx & 0xFFFF].eog >> 8) & 0xFF;
                  }
                if(size > LEDATLEN)
                  size = LEDATLEN;
                for(n = 0 ; n < size ; ++n)
                  s[13+n+dn] = cp->value[n];
                flag = 1; 
                }
              }
            }            
          startx = handle + 1;
          }
        }
      while(flag == 0 && aflag == 0 && cticn >= 0 && startx <= end);           
      }

    
    if(flag == 0)
      {
      VPRINT "Attribute/UUID not found\n");
      errcode = 0x0A; // att not found
      }
    else if(flag == 1)
      {
      if(aflag != 0)
        {
        s[9] = 0x0B;
        VPRINT "SEND reply for non-value handle %04X\n",handle);
        for(n = 0 ; n < size ; ++n)
          s[n+10] = s[n+13];
        size -= 3;
        }
      else if(dat[0] == 0x08)
        s[9] = 0x09;  // 08 reply
      else
        {
        s[9] = 0x11;   // 10 reply
        size += 2;
        }
   
      if(aflag == 0)
        {
        s[11] = handle & 0xFF;
        s[12] = handle >> 8;
        s[10] = size + 2;
        VPRINT "SEND reply opcode %02X for handle %02X%02X\n",s[9],s[12],s[11]);
        }
        
      s[5] = (unsigned char)((size + 4) & 0xFF);
      s[6] = (unsigned char)(((size + 4) >> 8) & 0xFF);
      s[3] = (unsigned char)((size + 8) & 0xFF);
      s[4] = (unsigned char)(((size + 8) >> 8) & 0xFF);
      le09replyv[0] = size + 13;
      
      sendhci(le09replyv,ndevice);      
      }  
    } 
  else if(dat[0] == 0x02)
    {  // MTU exhange
    VPRINT "SEND MTU exchange reply\n");
    lemtu[PAKHEADSIZE+10] = (unsigned char)((LEDATLEN + 3) & 0xFF);
    lemtu[PAKHEADSIZE+11] = (unsigned char)(((LEDATLEN + 3) >> 8) & 0xFF);
    sendhci(lemtu,ndevice);
    dev[ndevice]->setdatlen = LEDATLEN;
    }
  else if(dat[0] == 0x1E)
    {
    cmd[0] = 0x1E;
    cmd[1] = 0;
    pushins(IN_LEACK,ndevice,2,cmd);
    // indicate ack
    }  
  else if(xflag == 0)
    { // request not supported
    if(!(dat[0] == 0x03 || dat[0] == 0x1B || dat[0] == 0x1D || dat[0] == 0xD2 || dat[0] == 0x52))
      {
      VPRINT "Opcode not supported\n");
      errcode = 0x06;
      }
    // opcodes 1B 1D 1E D2 52 no error reply
    }
    
  if(errcode != 0)
    {
    lefail[PAKHEADSIZE+10] = dat[0];  // operation
    lefail[PAKHEADSIZE+11] = dat[1];
    lefail[PAKHEADSIZE+12] = dat[2];
    lefail[PAKHEADSIZE+13] = errcode;
    VPRINT "SEND error code %02X\n",errcode); 
    sendhci(lefail,ndevice);
    }
         
  flushprint();
  return(popflag);
  }


int nextctichandle(int start,int end,int *handle,int flag)
  {
  int n,cticn,minhandle,del,del0,notdel,minpshand,pshand,psn;
  struct cticdata *cp;

  psn = 0;  
  minpshand = 0;
  for(n = 0 ; minpshand == 0 && pserv[n].handle > 0 && n < 32 ; ++n)
    {
    pshand = pserv[n].handle;
    if(pshand >= start && pshand <= end)
      {
      minpshand = pshand;
      psn = n;
      }
    }
    
  if(flag != 0 && minpshand != 0 && minpshand == start)
    {
    *handle = minpshand;
    return(psn | 0x10000);
    }   
     
     
  *handle = 0;
  minhandle = 0xFFFF;
  cticn = -1;
  // find lowest handle between start and end
  for(n = 0 ; ctic(0,n)->type == CTIC_ACTIVE  ; ++n)
    {
    cp = ctic(0,n);
    notdel = 0;
    if((cp->perm & 0x30) != 0)
      {
      notdel = 1;  // include next handle notify control
      if(cp->reportflag != 0)
        notdel = 3;  // include Extended and Ref
      }
    del0 = -1;  // always include INFO
    for(del = del0 ; del <= notdel ; ++del)
      {
      if(cp->chandle+del >= start && cp->chandle+del <= end)
        {
        if(cp->chandle+del < minhandle)
          {
          minhandle = cp->chandle+del;
          cticn = n;
          *handle = minhandle;  // 4 possible handles for cticn
          }
        }
      }  
    }
    
  if(flag != 0 && minpshand != 0 && minpshand < minhandle)
    {
    *handle = minpshand;
    return(psn | 0x10000);
    } 
         
  return(cticn);
  }
  
  
int stuuid(unsigned char *s)  
  {
  int n;
  
  if(s[0] != 0 || s[1] != 0)
    return(0);
  for(n = 4 ; n < 16 ; ++n)
    {
    if(s[n] != standard[n])
      return(0);
    }
  return(1);
  }
  
void save_pico_info()
  {
#ifdef PICOSTACK
  rwlinkey(1,0,NULL);
#endif
  }  
  
void save_pair_info()
  {
  if(check_init(0) == 0)
    return; 
  readhci(0,0,0,250,0);  // codes 6/7
  rwlinkey(1,0,NULL);
  NPRINT "Pair info saved\n");
  flushprint();
  }   
 
void swapk(int k0,int k1)
  {
  int j;
  
  for(j = 0 ; devok(j) != 0 ; ++j)
    {
    if(dev[j]->keyk == k0)
      dev[j]->keyk = k1;
    if(dev[j]->pairk == k0)
      dev[j]->pairk = k1;
    } 
  }
   
void rwlinkey(int rwflag,int ndevice,unsigned char *addr)
  {
  int n,k,k1,k0,j,ntn,addcount,eflag,last0,last1,devn,delcount;
  unsigned char *badd,*key,*t0,*t1;
  struct devdata *dp;
  static int count = -1;
  static int changeflag = 0;
  unsigned char *newtable,*sav;
  static unsigned char zero[6] = {0,0,0,0,0,0};
  static unsigned char *table = NULL; 
  static unsigned char usedlist[256];
  
  sav = gpar.buf;
 
  if(rwflag == 0)
    {  // read   
    if(count < 0)
      {
      for(k = 0 ; k < 256 ; ++k)
        usedlist[k] = 0;
      count = 0;  // in file
      changeflag = 0;
      n = readkeyfile(&table,&count);      
      if(n == 0)
        {
        count = 0;
        VPRINT "Read %s failed\n",gpar.datfile);
        return;
        }
      }
    
     
    for(k = 0 ; k < count && table != NULL ; ++k)
      {
      badd = table + k*22;
      key = badd+6;
      if(addr == NULL)
        devn = devnfrombadd(badd,BTYPE_CL | BTYPE_LE | BTYPE_ME,DIRN_FOR);  
      else
        {
        devn = bincmp(badd,addr,6,DIRN_FOR);
        if(devn != 0)
          devn = ndevice;
        }
          
      if( (ndevice == 0 && devn > 0) || (ndevice > 0 && devn == ndevice) )
        {  // all on init (ndevice=0)  or ndevice only
        eflag = 0;
        dp = dev[devn];
        if(bincmp(zero,key+10,6,DIRN_FOR) != 0)
          {
          if((dp->linkflag & PAIR_FILE) == 0)
            {
            for(n = 0 ; n < 10 ; ++n)
              dp->divrand[n] = key[n];
            dp->linkflag |= PAIR_FILE;
            dp->pairk = k;
            }
          else
            eflag = 1;
          }
        else
          {          
          if((dp->linkflag & KEY_FILE) == 0)
            {
            for(n = 0 ; n < 16 ; ++n)
              dp->linkey[n] = key[n];
            dp->linkflag |= KEY_FILE;
            dp->keyk = k;
            }
          else
            eflag = 1;
          }
        if(eflag != 0)
          {
          for(n = 0 ; n < 6 ; ++n)
            badd[n] = 0;  // disable 2nd entry
          }     
        }
      }
    }
  else if(rwflag == 1)
    {  // write     
    // update table
    if(count > 0 && table != NULL) 
      {
      for(k = 0 ; k < count ; ++k)
        {
        badd = table + k*22;
        key = badd+6;
        n = devnfrombadd(badd,BTYPE_CL | BTYPE_LE | BTYPE_ME,DIRN_FOR);  
        if(n > 0)
          { 
          dp = dev[n];
          if(bincmp(zero,key+10,6,DIRN_FOR) != 0)
            {   
            if((dp->linkflag & PAIR_NEW) != 0)
              {
              for(n = 0 ; n < 16 ; ++n)
                key[n] = dp->divrand[n];
              dp->linkflag &= ~PAIR_NEW;
              dp->linkflag |= PAIR_FILE;
              usedlist[k] = 1;
              changeflag = 1;
              }
            }
          else if((dp->linkflag & KEY_NEW) != 0)
            {   // must be KEY_FILE also
            for(n = 0 ; n < 16 ; ++n)
              key[n] = dp->linkey[n];
            dp->linkflag &= ~KEY_NEW;
            dp->linkflag |= KEY_FILE;
            usedlist[k] = 1;
            changeflag = 1;
            }
          }
        }
      }
 
    // sort used to end
    if(count > 128)
      {
      do
        {
        last0 = -1;
        last1 = -1;
        for(n = count-1 ; n >= 0 ; --n)
          {
          if(last0 < 0 && usedlist[n] == 0)
            last0 = n;
          if(last0 > 0 && usedlist[n] != 0)
            last1 = n;
          }
        if(last1 >= 0)
          {
          flushprint();
          t1 = table + (22*last1);
          for(n = 0 ; n < 22 ; ++n)
            sav[n] = t1[n];
          swapk(last1,254);          
          k1 = last1;
          do
            {
            k0 = k1+1;
            flushprint();
            t1 = table + (22*k1);
            t0 = table + (22*k0);
            for(n = 0 ; n < 22 ; ++n)
              t1[n] = t0[n];
            usedlist[k1] = usedlist[k0];
            swapk(k0,k1);
            ++k1;
            }
          while(k0 < last0);
        
          t0 = table + (22*last0);
          for(n = 0 ; n < 22 ; ++n)
            t0[n] = sav[n];
          swapk(254,last0);
          }
        }
      while(last1 >= 0);
      } // end swaps
     
    // count NEW additions not in table
    addcount = 0;    
    for(n = 1 ; devok(n) != 0 ; ++n)
      {
      if((dev[n]->linkflag & KEY_NEW) != 0) 
        ++addcount;
      if((dev[n]->linkflag & PAIR_NEW) != 0)
        ++addcount;
      }
 
    if(changeflag == 0 && addcount == 0)   
      return;   // no changes
        
    if(count + addcount > 186)
      {  // limit = 4k
      NPRINT "%s file of paired devices is large\n",gpar.datfile);  
      delcount = count+addcount-186;
      NPRINT "Removing %d entries\n",delcount);
      }
    else
      delcount = 0;
      
    if(addcount != 0 || delcount != 0)
      {  // need new table
      count -= delcount;
      newtable = newkeytable(count+addcount);       
      if(newtable == NULL)
        return;
      
      if(count > 0 && table != NULL)
        {  
        ntn = 22*count;
        j = 22*delcount;
        for(n = 0 ; n < ntn ; ++n)
          newtable[n] = table[n+j];
        }
      else
        ntn = 0;
        
      freekeytable(&table);
      table = newtable;
      k = 0;
      for(n = 1 ; k < addcount && devok(n) != 0 ; ++n)
        {
        dp = dev[n];
        if((dp->linkflag & KEY_NEW) != 0)  
          {
          for(j = 0 ; j < 6 ; ++j)
            {
            table[ntn] = dp->baddr[j];
            ++ntn;
            }  
          for(j = 0 ; j < 16 ; ++j)
            {
            table[ntn] = dp->linkey[j];
            ++ntn;
            }  
          dp->linkflag &= ~KEY_NEW;
          dp->linkflag |= KEY_FILE;
          ++k;
          }
        if((dp->linkflag & PAIR_NEW) != 0) 
          {
          for(j = 0 ; j < 6 ; ++j)
            {
            table[ntn] = dp->baddr[j];
            ++ntn;
            }  
          for(j = 0 ; j < 16 ; ++j)
            {
            table[ntn] = dp->divrand[j];
            ++ntn;
            }  
          dp->linkflag &= ~PAIR_NEW;
          dp->linkflag |= PAIR_FILE;
          ++k;
          }
        }
      count += addcount;
      }  // end new table
      
    writekeyfile(table,count);
    changeflag = 0;
    for(k = 0 ; k < 256 ; ++k)
      usedlist[k] = 0;
    }
  else if(rwflag == 2)
    {  // close
    freekeytable(&table);
    count = -1;
    } 
  else if(rwflag == 3 && count > 0) 
    {
    for(k = 0 ; k < count && table != NULL ; ++k)
      {
      badd = table + k*22;
      n = devnfrombadd(badd,BTYPE_CL | BTYPE_LE | BTYPE_ME,DIRN_FOR);
      if(n >= 0)
        NPRINT "%d %s =",k,dev[n]->name);
      else
        NPRINT "%d Unknown =",k); 
      
      for(j = 0 ; j < 22 ; ++j)
        {
        NPRINT " %02X",badd[j]);
        if(j == 5)
          NPRINT " =");
        }
        if(n >= 0)  
      NPRINT " - F%02X K%d P%d",dev[n]->linkflag >> 10,dev[n]->keyk,dev[n]->pairk);
      NPRINT " - U%d\n",usedlist[k]);
      flushprint();
      }
    }
  else if(rwflag == 4)
    {  // mark used
    k = dev[ndevice]->keyk;
    if(k < 255)
      usedlist[k] = 1;
    k = dev[ndevice]->pairk;
    if(k < 255)
      usedlist[k] = 1;
    }       
    
  return;
  }  

  

void printascii(unsigned char *s,int len)
  {
  int n,flag;
  static char buf[64];
  
  if(gpar.printflag != PRINT_VERBOSE || len == 0)
    return;
     
  n = 0;
  while(n < 50 && n < len)
    {
    buf[n] = s[n];
    if(buf[n] == 10 || buf[n] == 13)
     buf[n] = '.';
    else if(buf[n] < 32 || buf[n] > 126)
      return;   // some non-ascii   
    ++n;
    }
  
  if(n == 0)
    return;
  
  buf[n] = 0;
  
  flag = 0;    // partial 
  if(n == len)
    flag = 1;  // all
  
  if(n == 0)
    return;      
    
  if(flag == 0)
    strcat(buf,"...");
 
  VPRINT "      TEXT  %s\n",buf); 
  }


int meshpacket(unsigned char *s)
  {
  int ndevice,repn,retval;
  unsigned int index;
  unsigned char *rp;
  struct devdata *dp;
  static int rejflag = 0;
  
  
  if(s == NULL)
    {
    rejflag = 0;  // reset reject message
    return(1);
    }
    
  if((gpar.meshflag & MESH_R) == 0)
    return(1);   // mesh read disabled
   
  // s = 04 3E len 02 nresp DATA
  // sf[4] = number of responses
  // data for each response starts at rp
  // first rp = s[5]
  
  retval = 1;
  
  rp = s + 5;
  for(repn = 0 ; repn < s[4] ; ++repn)
    {  // each response
    // rp[2-7] = board address
    // rp[9] = ndata + 5
    // rp[10] = FF IDlo IDhi INDEXlo INDEXhi 
    // rp[15] = data
         
    if(rp[10] == 0xFF && (rp[11] ==  ((rp[2] ^ rp[3]) ^ rp[4]) )  &&
                         (rp[12] == (((rp[5] ^ rp[6]) ^ rp[7])  | 0xC0) ) )
      {  // is a mesh packet
      index = rp[13] + (rp[14] << 8);
     
      // compare board address with known mesh devices
              
      ndevice = devnfrombadd(rp+2,BTYPE_ME,DIRN_REV);
      if(ndevice < 0 || dev[ndevice]->node >= 1000)
        {  // not known - reject
        if(rejflag == 0)
          {  // print only once 
          NPRINT "GOT mesh packet from unknown device %s - rejected for\n",baddstr(rp+2,1));
          NPRINT "    security. Add to devices.txt with node < 1000 to authorise\n");
          rejflag = 1;
          }
        return(1);
        }
      else  // known device ndevice
        dp = dev[ndevice];        

      if(index == 0) 
        { // first message reset and ignore
        dp->meshindex = 0;
        return(1);   // reset
        }

      if(dp->meshindex == index)
        return(1);   // already got this packet
    
         // got new packet index

      VPRINT "GOT mesh packet %d from %s\n",index,dev[ndevice]->name);  
  
      pushins(IN_DATA,ndevice,rp[9]-5,rp+15);
            
      dp->meshindex = index;  // last packet received
      retval = 0;
      }
      
    rp += 9 + rp[8];
    }
  return(retval);  
  }
  


/************ PUSH IN STACK ************
type 
length lo
length hi
device
data
return index of entry  -1=error
***********************************/

int pushins(long long int typebit,int devicen,int len,unsigned char *s)
  {
  int n,k,type,nret,xlen;
  long long int tyn;

  
  if(typebit == 0)
    return(-1);   // error - one but must be set
  
  // convert typebit to type=shift count
  type = 0;
  tyn = typebit;
  while((tyn & 1) == 0)
    {
    ++type;
    tyn >>= 1;
    }
 
  if((typebit & IN_IMMED) != 0)
    type |= INS_IMMED;
    
  // find free entry
  n = 0;
  while(instack[n] != INS_FREE)
    n += instack[n+1] + (instack[n+2] << 8) + INSHEADSIZE;  // next type
  if(n + len > INSTACKSIZE-16)
    {
    NPRINT "Serial buffer full - use read_all_clear()\n");
    return(-1);
    }
    
  xlen = len;
  if(typebit == IN_DATA)
    {  // add bookmark
    instack[n+4] = 0; 
    instack[n+5] = 0;
    xlen += 2;
    }    
      
  nret = n;  // start indec
  instack[n] = type;
  instack[n+1] = (xlen & 255);
  instack[n+2] = (xlen >> 8) & 255;
  instack[n+3] = devicen;
  n += INSHEADSIZE;
  if(typebit == IN_DATA)
    n += 2;
    
  for(k = 0 ; k < len ; ++k)
    {
    instack[n] = s[k];
    ++n;
    }  
  instack[n] = INS_FREE;  // next free

  return(nret);
  }

/******* POP IN STACK **********
pop all type = INS_POP
********************/

void popins()
  {
  int n,k,lastffn,lastn,lastwasff,count;
  
  
  // find last FF pop type
  do
    {
    lastn = -1;    // last active entry index    
    lastffn = -1;  // last pop entry index
    lastwasff = 0;
    count = 0;
    n = 0;
    while(instack[n] != INS_FREE)
      {
      if(instack[n] == INS_LOCK)
        return;
      if(instack[n] == INS_POP)
        {
        if(lastwasff == 0)
          {
          lastffn = n;
          lastwasff = 1;
          ++count;
          }
        }
      else
        {
        lastn = n;
        lastwasff = 0;
        }
      n += instack[n+1] + (instack[n+2] << 8) + INSHEADSIZE;  // next type
      }
      
    if(lastffn >= 0)  // must pop lastffn
      {     
      if(lastffn > lastn)  // is last one - can just mark as end
        instack[lastffn] = INS_FREE;
      else                 // is followed by active entry - must shift down
        {
        k = lastffn;
        while(instack[k] == INS_POP)
          k += instack[k+1] + (instack[k+2] << 8) + INSHEADSIZE;
                     // k is start of first active entry after lastffn
                     // n is last entry terminate 0 
        // move all entries from k and above down to lastffn by (k-lastffn)
         
        while(k <= n)  // n is final entry type 0 terminate
          {
          instack[lastffn] = instack[k];
          ++lastffn;
          ++k;
          }
        }
      }
    }
  while(lastffn > 0 && count > 1);    

  }


/*********** CLEAR STACK ********/

void clearins(int ndevice)
  {
  int n;
  
  if(ndevice == 0)
    {
    instack[0] = INS_FREE;
    return;
    }
    
  // nix ndevice entries only
  
  n = 0;
  while(instack[n] != INS_FREE)
    {
    if(instack[n+3] == ndevice)
      instack[n] = INS_POP;
    n += instack[n+1] + (instack[n+2] << 8) + INSHEADSIZE;  // next type
    }
  popins();  
  }


/*********** FIND TYPE in INSTACK ********

Find entry of packet type (IN_xxxx) in instack[]

devicen = 0  no device number check
devicen > 0  device number must match

popflag = INS_POP   mark for pop by popins
          INS_NOPOP no mark for pop

return index of entry
       -1 not found

****************************/

int findhci(long long int type,int devicen,int popflag)
  {
  int n;
 
  if(type == 0)
    return(-1);   // error - one bit must be set
    
  n = 0;
  while(instack[n] != INS_FREE)
    {
    if(instack[n] != INS_POP && 
         (type & ((long long int)1 << (instack[n] & 0x3F) )) != 0 &&
         (devicen == 0 || devicen == instack[n+3])  )
      {
      if(popflag == INS_POP)
        instack[n] = INS_POP; // mark for pop
      else if(popflag == INS_LOCK)
        instack[n] = INS_LOCK;
      insdatn = instack + n + INSHEADSIZE;
      return(n);   
      }
    n += instack[n+1] + (instack[n+2] << 8) + INSHEADSIZE;
    }  
    
  return(-1);
  }

/********** PRINT INSTACK ****************/

void printins()
  {
  int n,k,count;
  
  count= 1;
  n = 0;
  NPRINT "**INSTACK** CSP=%d DBG=%d\n",cmd_stack_ptr(),gpar.debug);
  flushprint();
  while(instack[n] != INS_FREE)
    {
    k = instack[n+1] + (instack[n+2] << 8);  // length of data
    NPRINT "%d Type %d Dev %d Len %d\n",count,instack[n],instack[n+3],k);
    hexdump(&insdat[n],k);
    n += k + INSHEADSIZE; // type lenlo lenhi data
    ++count;
    flushprint();
    }
  // rwlinkey(3,0,NULL);
  }



  
  
  
/******** OPEN REMOTE SDP *********
open an L2CAP connection to remote device
for read SDP database
*********************************/

int openremotesdp(int ndevice)
  {
  int retval;
  retval = openremotesdpx(ndevice);
  flushprint();
  return(retval);
  }
  
int openremotesdpx(int ndevice)
  {
  NPRINT "Connecting to %s to read classic serial services...\n",dev[ndevice]->name);

  if(clconnect0(ndevice) != 0)
    {  // psm 1 for SDP read
    flushprint();
    if(connectpsm(1,0x42,ndevice) != 0)
      {
      flushprint();
      return(1);  
      }
    disconnectdev(ndevice);
    }    
   
  NPRINT "Failed\n");
  flushprint();  
  return(0);
  }


/******** HEX DUMP - print buf in hex format, length = len ********/

void hexdump(unsigned char *buf, int len)
  {
  int i,i0,n;
 
  if(len <= 0)
    {
    VPRINT "No data\n");
    return;
    }
  
  i = 0;
  do
    {
    i0 = i;
    n = 0;
    VPRINT "      %04X  ",i0);
    do
      {
      if(n == 8)
        VPRINT "- ");
      VPRINT "%02X ",buf[i]);
      ++n;
      ++i;
      }
    while(n < 16 && i < len);
    VPRINT "\n"); 
    flushprint();
    }
  while(i < len); 
  }   



/********* CALC FCS ***********
s = command string with leading size at [0], string at [1]....[size]
[1] = 02  and channel = 0040 upwards  L2CAP 
calc fcs on count bytes: [9][10][11] = address,control
and put in fcs position at last byte = [len-1] 
*********************************/

unsigned char calcfcs(unsigned char *s,int count)
  {
  int n,len;
  unsigned char *cmd,fcs;
  static unsigned char crctable[256] = {
0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75, 0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69, 0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D, 0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51, 0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,
0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05, 0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19, 0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D, 0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21, 0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,
0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95, 0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89, 0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD, 0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1, 0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,
0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5, 0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9, 0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD, 0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1, 0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF};

  cmd = s + PAKHEADSIZE;
  fcs = 0xFF;
  len = s[0] + (s[1] << 8);  // length of cmd
  
    // two byte calc for address,control at cmd[9],[10]
  
  for (n = 0 ; n < count ; ++n)
    fcs = crctable[fcs ^ cmd[n+9]];
  fcs = ~fcs;  
  cmd[len-1] = fcs;  // last byte
  VPRINT "  Set [%d] FCS=%02X calculated for %d bytes from [9]\n",len-1,fcs,count); 
  return(fcs);     
  }


int hid_key_code(int key)
  {
  int retval;
  static unsigned short keylook[517] = {
  0x0000,0x0048,0x0049,0x004C,0x004A,0x004D,0x004B,0x004E,0x002A,0x002B,
  0x0028,0x0220,0x0235,0x0000,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
  0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0000,0x0029,0x004F,0x0050,
  0x0051,0x0052,0x002C,0x021E,0x021F,0x0032,0x0221,0x0222,0x0224,0x0034,
  0x0226,0x0227,0x0225,0x022E,0x0036,0x002D,0x0037,0x0038,0x0027,0x001E, 
  0x001F,0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0233,0x0033,
  0x0236,0x002E,0x0237,0x0238,0x0234,0x0204,0x0205,0x0206,0x0207,0x0208,
  0x0209,0x020A,0x020B,0x020C,0x020D,0x020E,0x020F,0x0210,0x0211,0x0212,
  0x0213,0x0214,0x0215,0x0216,0x0217,0x0218,0x0219,0x021A,0x021B,0x021C,
  0x021D,0x002F,0x0064,0x0030,0x0223,0x022D,0x0035,0x0004,0x0005,0x0006,
  0x0007,0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,0x0010,
  0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001A,
  0x001B,0x001C,0x001D,0x022F,0x0264,0x0230,0x0232,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0104,0x0105,0x0106,0x0107,0x0108,
  0x0109,0x010A,0x010B,0x010C,0x010D,0x010E,0x010F,0x0110,0x0111,0x0112,
  0x0113,0x0114,0x0115,0x0116,0x0117,0x0118,0x0119,0x011A,0x011B,0x011C,
  0x011D,0x0000,0x0164,0x0130,0x0135,0x012D,
  0x0000,0x0048,0x0049,0x004C,0x004A,0x004D,0x004B,0x004E,0x042A,0x042B,
  0x0428,0x0220,0x0000,0x0000,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
  0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0000,0x0429,0x004F,0x0050,
  0x0051,0x0052,0x042C,0x021E,0x021F,0x0432,0x0221,0x0222,0x0224,0x0434,
  0x0226,0x0227,0x0225,0x022E,0x0436,0x042D,0x0437,0x0438,0x0427,0x041E, 
  0x041F,0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0233,0x0433,
  0x0236,0x042E,0x0237,0x0238,0x0234,0x0204,0x0205,0x0206,0x0207,0x0208,
  0x0209,0x020A,0x020B,0x020C,0x020D,0x020E,0x020F,0x0210,0x0211,0x0212,
  0x0213,0x0214,0x0215,0x0216,0x0217,0x0218,0x0219,0x021A,0x021B,0x021C,
  0x021D,0x042F,0x0464,0x0430,0x0223,0x022D,0x0435,0x0404,0x0405,0x0406,
  0x0407,0x0408,0x0409,0x040A,0x040B,0x040C,0x040D,0x040E,0x040F,0x0410,
  0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,0x0418,0x0419,0x041A,
  0x041B,0x041C,0x041D,0x022F,0x0264,0x0230,0x0232,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000, 
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,  
  0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,  
  0x0000,0x023A,0x023B,0x023C,0x023D,0x023E,0x023F,0x0240,0x0241,0x0000,
  0x0000,0x4004,0x4005,0x4006,0x4007,0x4008,0x4009,0x400A,0x400B,0x400C,
  0x400D,0x400E,0x400F,0x4010,0x4011,0x4012,0x4013,0x4014,0x4015,0x4016,
  0x4017,0x4018,0x4019,0x401A,0x401B,0x401C,0x401D,0x401E,0x401F,0x4020,
  0x4021,0x4022,0x4023,0x4036,0x4037,0x4038,0x4064 
         }; 
  // last 516
  // use gpar.keyboard for non-GB table
  if(check_init(0) == 0)
    return(0);
  
  if(key < 1 || key > 516)
    retval = 0;
  else
    retval = keylook[key];

  VPRINT "HID key code %04X\n",retval);
  return(retval);
  }


/********** CLASSIC CONNECT ************
so ready for psm=1 control or psm=3 data channel request
***********************/

int clconnect0(int ndevice)
  {
  int retval,savto;

  clearins(0);  // clear input stack 
   
   
  if(devok(ndevice) == 0 || !(dev[ndevice]->type == BTYPE_CL || dev[ndevice]->type == BTYPE_ME))
    {
    NPRINT "Invalid or not classic device\n");
    return(0);
    }
    
  if(dev[ndevice]->conflag != 0) 
    {
    NPRINT "Already connected\n");
    return(0);
    }
 
  VPRINT "Set simple pair mode on\n");
  sendhci(setspm,0);
  // statusok(0,setspm);

  savto = gpar.timout;
  gpar.timout = 10000;  // 10s time out
                        // maybe android user permission/pair

  retval = clconnectxx(ndevice);
  gpar.timout = savto;
  
  if(retval == 0)
    { 
    VPRINT "DISCONNECT\n");
    disconnectdev(ndevice);  // sets conflags
    return(0);
    }
 
  VPRINT "HCI Connected OK\n");

  return(1);    
  }

int clconnectxx(int ndevice)
  {
  int n,tryflag;
  struct devdata *dp;
  
    
  dp = dev[ndevice];
  dp->setdatlen = 20;     
  dp->linkflag &= KEY_NEW | KEY_FILE;  // all off KEY_OFF  PASSKEY_OFF 
  if(dp->type == BTYPE_CL)
    {
    dp->linkflag |= PASSKEY_LOCAL;
    if((dp->linkflag & (KEY_NEW | KEY_FILE)) != 0)
      dp->linkflag |= KEY_ON;
    } 
  // else mesh pi  KEY_OFF PASSKEY_OFF  
         
  VPRINT "Open classic connection to %s\n",baddstr(dp->baddr,0));
      
  tryflag = 0;

  if(sendhci(clopen,ndevice) == 0)
    return(0); 
    
  readhci(ndevice,IN_CLHAND,0,10000,gpar.toshort);
         // sets conflag if OK - no store on stack
  if(dp->conflag == 0)
    {
    sendhci(clcancel,ndevice);  // cancel open command
    //statusok(1,clcancel);
    NPRINT "Open failed\n");
    return(0);
    }
    
  do
    {
    VPRINT "SEND Authentication request\n");
    sendhci(authreq,ndevice);
 
    readhci(ndevice,IN_ACOMP,0,gpar.timout,gpar.toshort);    
    flushprint();
    popins();
  
    n = findhci(IN_ACOMP,ndevice,INS_POP);
    if(n >= 0)
      {
      if(insdatn[0] != 0)  // want status = 0
        n = -1;
      }
  
    if(n < 0)
      {
      NPRINT "Authentication/PIN fail\n");
      if(tryflag == 0)
        {
        tryflag = 1;
        // flip KEY
        dp->linkflag ^= KEY_ON;
        if((dp->linkflag & KEY_ON) == 0)  
          NPRINT "Trying again with no link key..\n");
        else
          NPRINT "Trying again with link key..\n");
        }
      else
        {
        // Link key unknown or invalid;
        return(0);
        }
      }
    else 
      tryflag = 0;
    }
  while(tryflag != 0);     
    
  VPRINT "GOT Authentication/pair OK (Event 06)\n");
 
   
  VPRINT "SEND encrypt\n");    
  sendhci(encryptx,ndevice);
  readhci(ndevice,IN_ENCR,0,gpar.timout,gpar.toshort);
  if(findhci(IN_ENCR,ndevice,INS_POP) < 0)
    {
    NPRINT "Encrypt fail\n"); 
    return(0);
    }
    
  findhci(IN_ENCR,ndevice,INS_POP);  // strip 
   
    
  VPRINT "GOT Encrypt OK (Event 08)\n");

  popins();
  
 
  
  return(1);
  }
  


/************* SERVICES ******************/

int find_channel(int node,int flag,unsigned char *uuid)
  {
  int flags,retval,ndevice;

  if(check_init(0) == 0)
    return(-1);
  
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);
  
  retval = 0;
  
  if(flag == UUID_2)
    flags = SRVC_FINDUUID2;
  else if(flag == UUID_16)
    flags = SRVC_FINDUUID16;
  else
    {
    NPRINT "Flag error\n");
    flags = 0;
    }
  
  if(flags != 0)  
    retval = clservices(ndevice,flags,uuid,NULL,0);
  
  if(retval < 0)
    retval = 0;
    
  flushprint();
  return(retval);
  }

int list_channels(int node,int flag)
  {
  int flags,retval,ndevice;

  if(check_init(0) == 0)
    return(-1);
  
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);
  
  retval = 0;
  
  if((flag & 7) == LIST_FULL) 
    flags = SRVC_LONGLIST;
  else if((flag & 7) == LIST_SHORT)
    flags = SRVC_SHORTLIST;
  else
    {
    flags = 0;
    NPRINT "Flag error\n");
    }
 
  if(flags != 0)  
    retval = clservices(ndevice,flags,NULL,NULL,0);

  flushprint();
  return(retval);
  }

int list_channels_ex(int node,char *buf,int len)
  {
  int ndevice,retval;
 
  if(check_init(0) == 0)
    return(-1);
  
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);
  retval = clservices(ndevice,SRVC_SHORTLIST,NULL,buf,len);
  return(retval);
  }

 
/********** READ SERVICES *******/


int list_uuid(int node,unsigned char *uuid)
  {
  int retval,ndevice;

  if(check_init(0) == 0)
    return(0);
    
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);
         
  if(dev[ndevice]->type == BTYPE_LE || (dev[ndevice]->type == BTYPE_ME && (dev[ndevice]->conflag & CON_LE) != 0))
    retval = leservices(ndevice,SRVC_UUID,uuid);
  else if(dev[ndevice]->type == BTYPE_CL  || dev[ndevice]->type == BTYPE_ME)
    retval = clservices(ndevice,SRVC_UUID,uuid,NULL,0);
  else 
    retval = 0;
    
  if(retval < 0)
    retval = 0;
    
  flushprint();
  return(retval);
  }
  
/************
SHORTLIST/LONGLIST
   -1 = failed to read
 >= 0 = read OK and returned number of channels
FREECHANNEL
  > 0 = returned free channel
   -1 = failed to read - answer unknown
FINDUUID2/16
return -1 = failed to read channels - answer unknown
        0 = read channels OK and no match found
      > 0 = found channel returned
UUID
       -1 = failed to read or no data
        1 = OK  
****************/         
  
int clservices(int ndevice,int flags,unsigned char *uuid,char *listbuf,int listlen)
  {
  int ret;
  unsigned char *sdat;
  struct servicedata *serv;
  
  sdat = malloc(8192);
  serv = malloc(SERVDAT * sizeof(struct servicedata));
  if(serv == NULL)
    {
    NPRINT "Out of memory\n");
    return(0);
    }
  ret = clservicesx(ndevice,flags,uuid,listbuf,listlen,sdat,serv);
  free(serv);
  free(sdat);
  return(ret);
  }

int clservicesx(int ndevice,int flags,unsigned char *uuid,char *listbuf,int listlen,unsigned char *sdat,struct servicedata *serv)
  {
  int n,j,k,ncont,sn,savto,locndevice,type;
  int headsz,getout,savpf,flag;
  struct devdata *dp;
  unsigned char *cmd,*dat;
  // unsigned char sdat[8192];
  // struct servicedata serv[SERVDAT];
  static int freechan = 0;
   
  if(ndevice == 0)
    {
    printlocalchannels();
    return(3);
    }   
       
  serv[0].channel = 0;   // clear list
  
  // ndevice checked
  
  type = dev[ndevice]->type;
  if(!(type == BTYPE_CL || type == BTYPE_ME))
    {
    NPRINT "Not a classic server\n");
    return(-1);
    }
    
  dp = dev[ndevice];
  
  if(dp->matchname == 1)
    {  // must be 0 or 3
    NPRINT "Address via MATCH_NAME not found - run a classic scan\n");
    return(-1);
    }

  if(dp->conflag != 0)
    {
    if(dp->type == BTYPE_CL)
      NPRINT "%s must be disconnected and listening as a classic server to read serial channels\n",dp->name);
    return(-1);
    }

  if(flags == SRVC_FREECHANNEL)
    {
    if(freechan != 0)
      {  // stored from previous SDP read
      j = freechan;
      freechan= 0;
      return(j);
      }
    // no stored free channel - read sdp to find
    }

  if((flags == SRVC_UUID || flags == SRVC_FINDUUID2 || flags == SRVC_FINDUUID16) && uuid == NULL)
    {
    NPRINT "UUID not specified\n");
    return(-1);
    }

  clearins(0); 
   
  VPRINT "Reading SDP database of %s\n",dp->name);
   
  if(openremotesdp(ndevice) == 0)
   return(-1);
      
  locndevice = ndevice;
  headsz = 9;  // HCI header 02.. size

  savto = gpar.timout;  
  gpar.timout = 10000;  // 10 seconds
      
  if(flags == SRVC_UUID)
    {    // user specified UUID search
    ssareq[PAKHEADSIZE+17] = uuid[0];
    ssareq[PAKHEADSIZE+18] = uuid[1];
    }    
  else
    {   // 0003 for RFCOMM   
    ssareq[PAKHEADSIZE+17] = 0;
    ssareq[PAKHEADSIZE+18] = 0x03;    
    }
    
  if(gpar.printflag == PRINT_VERBOSE)
    {
    VPRINT "Find all UUID = %02X%02X\n",ssareq[PAKHEADSIZE+17],ssareq[PAKHEADSIZE+18]);          
    VPRINT "  Set [%d][%d] UUID\n",headsz+8,headsz+9);   
    }
    
  ssareq[PAKHEADSIZE+24] = 0;  // first aid = 0
  ssareq[PAKHEADSIZE+25] = 0;
        
  if(flags == SRVC_FINDUUID2 || flags == SRVC_FINDUUID16 || flags == SRVC_FREECHANNEL)
    {  // AID_4  aid 0 to 4 only 
    VPRINT "  Set [%d][%d] last aid 00 04\n",headsz+17,headsz+18);
    ssareq[PAKHEADSIZE+26] = 0;
    ssareq[PAKHEADSIZE+27] = 4;
    }
  else  
    {  // assume full search - all aid
    VPRINT "  Set [%d][%d] last aid FF FF\n",headsz+17,headsz+18);
    ssareq[PAKHEADSIZE+26] = 0xFF;
    ssareq[PAKHEADSIZE+27] = 0xFF;
    }
 
    
//  VPRINT "  Set [7][8] remote L2CAP channel\n");
//  ssareq[PAKHEADSIZE+7] = dp->dcid[2];  // psm 1  
//  ssareq[PAKHEADSIZE+8] = dp->dcid[3];  
     
   // assemble extra/continue replies in sdat[]
  sn = 0;   // sdat[] index   
  ncont = 0;  // 0=no continue
  dp->id = 1;
  do
    {
    ssareq[0] = 29 + ncont;
    cmd = ssareq + PAKHEADSIZE;
    if(headsz != 0)
      VPRINT "  Set [3] [5] packet lengths\n");
    cmd[3] = 24 + ncont;
    cmd[5] = 20 + ncont;
 
 //   VPRINT "  Set [%d] id %02X\n",headsz+2,id);
 //   cmd[11] = id;
 
    cmd[13] = 15 + ncont;   
    VPRINT "  Set [%d] length %02X\n",headsz+4,cmd[13]);
    if(ncont == 0)
      {
      VPRINT "  Set [%d] continue bytes count = 0\n",headsz+19);
      cmd[28] = 0;  // else filled in on previous reply
      }
    if(gpar.printflag == PRINT_VERBOSE)
      {
      if(flags == SRVC_FINDUUID2 || flags == SRVC_FINDUUID16 || flags == SRVC_FREECHANNEL)
        VPRINT "SEND SSA request aid 0 to 4\n");
      else
        VPRINT "SEND SSA request all aid\n");
      }
    sendhci(ssareq,locndevice);  // device 0 if bluez method
    readhci(locndevice,IN_SSAREP,0,gpar.timout,gpar.toshort);
    n = findhci(IN_SSAREP,0,INS_POP);
    if(n < 0)
      {
      VPRINT "No SSA reply\n");
      ncont = 0;
      }
    else
      {
      VPRINT "GOT SSA reply\n");
      dat = insdat + n; // reply starting at opcode 07
      if(dat[0] != 7)
        ncont= 0;  // error
      else
        {
        VPRINT "[%d]... = packet lengths  [%d].. = SDP data\n",headsz+3,headsz+7);
        k = ((dat[5] << 8) + dat[6]);  // length from dat[7] to dat[7+k-1] not inc continue count
                                       // number of continue bytes is next at dat[7+k]
        ncont = dat[7+k];                                
           // assemble service data in sdat[]
        if(sn != 0)
          VPRINT "Remove continue bytes and add to previous data\n");
        for(j = 0 ; j < k && sn < 8190 ; ++j)
          {
          sdat[sn] = dat[7+j];
          ++sn;
          }
                
        // ncont = ((dat[3] << 8) + dat[4]) - ((dat[5] << 8) + dat[6]) - 3;
        // ncont = continue bytes - 1
        // 0 if one contine byte - assume 0 no continue
        // NPRINT "Continue %d\n",ncont);
        
        if(ncont > 32)
          {
          VPRINT "**** Invalid ncont\n");
          ncont = 32;
          }
          
        if(ncont > 0)
          {  // copy ncont+1 continue bytes to end of ssareq command cmd[28..] 
          VPRINT "Last packet has ended with %d continue bytes\n",ncont);
          VPRINT "Need another SSA request with these bytes:\n");
            
          j = instack[n+1] + (instack[n+2] << 8);  // length of extra data dat[0] to [j-1]
                          // need last ncont+1 bytes back from dat[j-1] includes 1st byte = count then ncont data
                          // continue = ncont+1 bytes from dat[j-1-ncont] to dat[j-1]
          VPRINT "  Set [%d]..  count and continue bytes..\n  ",headsz+19);
          for(k = 0 ; k <= ncont ; ++k)
            {
            cmd[28+k] = dat[j-1-ncont+k];
            VPRINT " %02X",cmd[28+k]);
            }
          VPRINT "\n");
          ++dp->id; 
          }
        else
          VPRINT "No continue bytes - done\n");
        }
      popins();
      }
    flushprint();
    }
  while(ncont != 0);

  disconnectdev(ndevice);  // close sdp
  
  gpar.timout = savto;
  popins();
  
  if(sn > 0 && sdat[0] == 0x35 && sdat[1] == 0)
    {
    VPRINT "No SDP data\n");
    if(flags == SRVC_UUID)
      NPRINT "No matching SDP entries\n");
    return(-1);
    }
    
  if(sn > 0)
    {
    VPRINT "Decode SDP info %d bytes start %02X %02X %02X end %02X %02X %02X\n",sn,sdat[0],sdat[1],sdat[2],sdat[sn-3],sdat[sn-2],sdat[sn-1]);

    if(flags == SRVC_UUID)
      savpf = set_print_flag(PRINT_VERBOSE);  // verbose during decodesdp
    
    decodesdp(sdat,sn,serv,SERVDAT);
    flushprint();
    
    if(flags == SRVC_UUID)
      {
      set_print_flag(savpf);  // restore
      return(1);  // done OK
      }
    
    if(ndevice == 0)
      {  // local only - find free channel
      freechan = 0; 
      for(j = 1 ; j < 256 && freechan == 0 ; ++j)
        {  // try channel j
        flag = 0;
        for(k = 0 ; flag == 0 && serv[k].channel != 0 && k < SERVDAT ; ++k)
          {  
          if(serv[k].channel == j)
            flag = 1;    // not free 
          }
        if(flag == 0)
          freechan = j;  // found free channel
        }
      }
    
    if(flags == SRVC_FREECHANNEL)
      {
      j = freechan;  // may be 0
      freechan = 0;  // no store
      return(j);  
      }
         
    if(flags == SRVC_FINDUUID2 || flags == SRVC_FINDUUID16)
      {  // search channels for 16-byte or 2-byte RFCOMM UUID match 
      for(k = 0 ; serv[k].channel != 0 && k < SERVDAT ; ++k)
        {
        if( (flags == SRVC_FINDUUID16 && serv[k].uuidtype == 16) ||
            (flags == SRVC_FINDUUID2  && serv[k].uuidtype == 2)  )
          {
          getout = 0;
          for(j = 0 ; j < serv[k].uuidtype && getout == 0 ; ++j)
            {
            if(serv[k].uuid[j] != uuid[j])
              getout = 1;
            }
          if(getout == 0)
            return(serv[k].channel);   // found serial channel of UUID
          }
        }
      return(0);  // no UUID match   
      }
       
    if(flags == SRVC_SHORTLIST || flags == SRVC_LONGLIST)
      return(printchannels(ndevice,flags,serv,SERVDAT,listbuf,listlen));  // print RFCOMM channels   
    }
    
  flushprint(); 
  return(-1);  // no data
  }


int printchannels(int ndevice,int flags,struct servicedata *serv,int servlen,char *listbuf,int listlen)
  {
  int n,k,ln,bn,len,count,listflag;
  static char buf[64];
   
  if(listbuf == NULL || listlen <= 0)
    listflag = 0;
  else
    listflag = 1;
  
  ln = 0;
  count = 0;
   
  if(serv[0].channel == 0)
    {
    NPRINT "No services found\n");
    flushprint();
    return(0);
    }
   
  if(listflag == 0)
    NPRINT "\n%s RFCOMM serial channels\n",dev[ndevice]->name);  
  flushprint();
   
 
      
  for(n = 0 ; n < servlen && serv[n].channel != 0 ; ++n)
      {
      if(listflag == 0)
        NPRINT "  %d  %s\n",serv[n].channel,serv[n].data+1);  // data[0]=name length  data[1].. name
      else
        {
        sprintf(buf,"%d - %s\n",serv[n].channel,serv[n].data+1);
        bn = 0;
        while(buf[bn] != 0 && ln < listlen-1)
          {
          listbuf[ln] = buf[bn];
          ++bn;
          ++ln;
          }
        listbuf[ln] = 0;
        }
      if(flags == SRVC_LONGLIST)
        {     
        len = serv[n].uuidtype;  // 0 for free last entry
        if(len > 0)
          {
          ++count;
          NPRINT "    UUID = ");
          for(k = 0 ; k < len && k < 16 ; ++k)
           NPRINT "%02X",serv[n].uuid[k]);
            
          if(len == 2)
             NPRINT "  %s",uuidlist+finduuidtext((serv[n].uuid[0] << 8) + serv[n].uuid[1]));
           NPRINT "\n");

          if(ndevice == 0)   // classic SDP local info
            NPRINT "    Local SDP handle = %08X\n",serv[n].handle);
          }
        } 
      ++count;    
      flushprint();
      }
   
  return(count);   // classic done
  }


int find_ctics(int node)
  {
  int retval,ndevice;

  if(check_init(0) == 0)
    return(-1);
  
  ndevice = devnp(node);
  if(ndevice < 0)
    return(-1);
 
  retval = leservices(ndevice,SRVC_READCTICS,NULL);
  flushprint();
  return(retval);
  }


int list_ctics(int node,int flag)
  {
  int retval,ndevice;

  if(check_init(0) == 0)
    return(-1);
  
  ndevice = devnp(node);
  if(ndevice < 0)
    return(-1);
  
  retval = 0;
 
  if(!(dev[ndevice]->type == BTYPE_LE || dev[ndevice]->type == BTYPE_ME ||
                    (ndevice == 0 && ctic(0,0)->type == CTIC_ACTIVE) ) )
    {
    NPRINT "Not an LE device\n");  
    flushprint();
    return(-1);
    }  
  
  if((flag & 7) == LIST_FULL)
    retval = printctics1(ndevice);
  else if((flag & 7) == LIST_SHORT)
    retval = printctics0(ndevice,flag,NULL,0);  
     
  flushprint();
  return(retval);
  }

int list_ctics_ex(int node,int flag,char *buf,int len)
  {
  int retval,ndevice,locflag;
 
  if(check_init(0) == 0)
    return(-1);
  
  ndevice = devnp(node);
  if(ndevice < 0)
    return(-1);
  
  retval = 0;
 
  if(!(dev[ndevice]->type == BTYPE_LE || dev[ndevice]->type == BTYPE_ME ||
                    (ndevice == 0 && ctic(0,0)->type == CTIC_ACTIVE) ) )
    {
    NPRINT "Not an LE device\n");  
    flushprint();
    return(-1);
    }  
  
  locflag = flag & ~7;  // LIST_SHORT
  retval = printctics0(ndevice,locflag,buf,len);  
     
  flushprint();
  return(retval);
  }

void printlocalchannels()
  {
  int n;
  
  NPRINT "\n    Classic RFCOMM serial channels - all connect on channel 1\n");
  NPRINT "      Serial2  UUID=1101\n");
  NPRINT "      Serial16  UUID=00001101-0000-1000-8000-00805F9B34FB\n") ;
  NPRINT "      %s  UUID=",custname); 
  for(n = 0 ; n < 16 ; ++n)
    {
    NPRINT "%02X",custuuid[n]);
    if(n == 3 || n == 5 || n == 7 || n == 9)
      NPRINT "-");
    } 
  NPRINT "\n");
  NPRINT "\n    OBEX push server channel 2\n");
  NPRINT "      OBEX server  UUID=1105\n"); 
  }

int leservices(int ndevice,int flag,unsigned char *uuid)
  {
  int ret;
  struct servicedata *serv;

  serv = malloc(SERVDAT * sizeof(struct servicedata));
  if(serv == NULL)
    {
    NPRINT "Out of memory\n");
    return(0);
    }
  ret = leservicesx(ndevice,flag,uuid,serv);
  free(serv);
  return(ret);
  }

int leservicesx(int ndevice,int flag,unsigned char *uuid,struct servicedata *serv)
  {
  int n,n0,k,j,len,lasth,num,count,getout;
  int loop,chn,locuuid,cancelflag,failcount;
  unsigned char *cmd;
  struct cticdata *cp;
 
   // ndevice checked

  if(ndevice == 0)
    {
    count = printctics1(0);
    return(count);
    }


  if(!(dev[ndevice]->type == BTYPE_LE || dev[ndevice]->type == BTYPE_ME) )
    {
    NPRINT "Not an LE device\n");
    flushprint();
    return(-1);
    }     

  if(dev[ndevice]->conflag == 0)
    {
    NPRINT "%s must be connected as an LE server to read characteristics\n",dev[ndevice]->name);   
    return(-1);
    }

  if(flag == SRVC_UUID && uuid == NULL)
    {
    NPRINT "UUID not specified\n");
    return(-1);
    }
    
  serv[0].channel = 0;   // clear list
  chn = 0;              // serv[chn] index
  
  locuuid = 0;
  
  if(flag == SRVC_UUID)
    locuuid = (uuid[0] << 8) + uuid[1];   // user defined  0=all
  else        
    locuuid = 0x2803;   // SRVC_FIND = chaaracteristics
    
    
  lasth = 0; // last handle read
  cmd = lereaduuid2 + PAKHEADSIZE;
  count = 0; 
  getout = 0;
  loop = 0;
  failcount = 0;


  // clear input buffer 
  do
    {
    readhci(ndevice,0,IN_ATTDAT,0,0);
    n = findhci(IN_ATTDAT,ndevice,INS_POP);
    }
  while(n >= 0);
   
  NPRINT "Reading LE services from %s..",dev[ndevice]->name);
  if(gpar.printflag == PRINT_VERBOSE || flag != SRVC_READCTICS)
    NPRINT "\n");
    
  do 
    {           // go through handles 0001...  until get opcode=01 no more response 
    ++lasth;    // start search from next handle
    
    cmd[10] = lasth & 0xFF;
    cmd[11] = (lasth >> 8) & 0xFF;
    cmd[14] = locuuid & 0xFF;
    cmd[15] = (locuuid >> 8) & 0xFF;
    VPRINT "SEND read UUID (opcode 08)\n");
    VPRINT "  Set [10][11] starting ctic handle %02X%02X\n",cmd[11],cmd[10]);
    VPRINT "  Set [14][15] UUID %04X\n",locuuid);
    
    sendhci(lereaduuid2,ndevice);
    readhci(ndevice,IN_ATTDAT,0,gpar.timout,gpar.toshort);  
     
    n = findhci(IN_ATTDAT,ndevice,INS_POP);
      
    if(n >= 0)
      {
      if(insdatn[0] == 0x01)  // no more data - normal terminate
        getout = 1;
      else if(insdatn[0] == 0x1D)
        {  // LE device has sent handle value indication
        sendhci(leconf,ndevice);  // send confirmation
        }
      else if(insdatn[0] == 0x09)  // opcode for info request response
        {
        len = insdatn[1];  // of each entry in returned list
                           // number of entries        
        num = ( (instack[n+1] + (instack[n+2] << 8) ) - 2)/len;
             
        // read returned list of handle/handle data
        for(j = 0 ; j < num ; ++j)  
          {          
          n0 = j*len+2;  // handle of UUID match
          lasth = insdatn[n0] + (insdatn[n0+1] << 8);
          if(flag == SRVC_READCTICS)
            {    // UUID = 2803 characteristics      
                 // 2803 data = perm/handle/uuid of characteristic
            if(insdatn[n0+2] != 0)   // r/w permissions
              { 
              // handle of value                    
              serv[chn].handle = insdatn[n0+3] + (insdatn[n0+4] << 8);         
                  
              // check duplicate
              cancelflag = 0;
              for(k = 0 ; k < chn ; ++k)
                {
                if(serv[k].handle == serv[chn].handle)
                  {
                  cancelflag = 1;  // do not save
                  }
                }
              
             
              if(cancelflag == 0)
                { 
                serv[chn].perm = insdatn[n0+2];
                serv[chn].channel = 0x10000;  // new entry LE marker  not a channel
                // UUID of value
                if(len == 7)
                  {         
                  serv[chn].uuidtype = 2;   // 2 or 16
                  serv[chn].uuid[0] = insdatn[n0+6];
                  serv[chn].uuid[1] = insdatn[n0+5];
                  }
                else if(len == 21)
                  {
                  serv[chn].uuidtype = 16;  
                  for(k = 0 ; k < 16 ; ++k)
                    serv[chn].uuid[k] = insdatn[n0+20-k];
                  }
                else
                  {  // MTU too small for full UUID - read handle to find
                  serv[chn].uuidtype = 0;               
                  }

                // probably next handle up - so skip on next info request
                if(serv[chn].handle == lasth+1)
                  ++lasth;

                 // set data to UUID type if known - or UUID
               
                serv[chn].data[0] = 0;
                k = 0;
                if(serv[chn].uuidtype == 2)
                  k = finduuidtext((serv[chn].uuid[0] << 8) + serv[chn].uuid[1]);
                if(k == 0)
                  { 
                  if(serv[chn].uuidtype == 0)
                    strcpy(serv[chn].data,"Unrecognised config format");
                  else
                    {             
                    while(k < serv[chn].uuidtype)
                      {
                      sprintf(serv[chn].data + 2*k,"%02X",serv[chn].uuid[k]);
                      ++k;
                      }
                    sprintf(serv[chn].data + 2*k," UUID");
                    }
                  }
                else
                  strcpy(serv[chn].data,uuidlist+k);            
                }
              
                                                               
              if(chn >= SERVDAT-2)
                {
                VPRINT "Run out of service memory\n");  
                getout = 2;
                }        
              else if(cancelflag == 0)
                {
                ++chn;
                serv[chn].channel = 0;
                }

              } // end target permissions
            }  // end SRVC_FIND saving characteristics to chan
          else
            {  // not looking for 2803 characteristics - print data
            NPRINT "Handle %04X =",lasth);
            for(k = 0 ; k < len-2 ; ++k)
              NPRINT " %02X",insdatn[n0+2+k]);
            NPRINT "\n");               
            }
          ++count;   
          }  // end j loop read handle/uuid info from response
         
        }    // end opcode 09 got info response
      else   // unexpected opcode return
        {
        VPRINT "Read UUID failed\n");
        ++failcount;
        } 
      popins();  
      }   // end got ATTDAT reply
    else
      ++failcount;  // no ATTDAT reply - try again
      
    ++loop;       // to ensure no infinite loop  
    
    if(gpar.printflag != PRINT_VERBOSE && flag == SRVC_READCTICS)
      NPRINT ".");  // progress display
    flushprint();
    }
  while(n >= 0 && getout == 0 && failcount < 8 && loop < 100);

  NPRINT "\n");
  
  if(count == 0)
    NPRINT "None found\n");
 
  if(flag == SRVC_UUID)
    return(count);
       
  for(n = 0 ; ctic(ndevice,n)->type == CTIC_ACTIVE ; ++n)
    ctic(ndevice,n)->iflag = 0;
   
  savectic(ndevice,serv,SERVDAT);

  printctics1(ndevice);

  for(n = 0 ; ctic(ndevice,n)->type == CTIC_ACTIVE ; ++n)
    {
    cp = ctic(ndevice,n);
    if(cp->iflag == 0)
      NPRINT "WARNING does not have %s as listed in local devices.txt info\n",cp->name); 
    }

                
  return(count);  // LE done - return number found 
  }




/*********** FIND CHARACTERISTIC UUID *******
find n index dev[ndevice]->ctic[n] of characteristic
with 2 or 16 byte uuid in device information

flag = UUID_2 or UUID_16
uuid = array with uuid value - 2 or 16 bytes 

return n index of characteristic in device information
      -1 = fail
*************************************/


int find_ctic_index(int node,int flag,unsigned char *uuid)
  {
  int n,k,getout,ndevice;
  struct cticdata *cp;
  
  if(check_init(0) == 0)
    return(-1);
      
  for(n = 0 ; ctic_ok(node,n) != 0 ; ++n)
    {
    ndevice = devn(node);
    cp = ctic(ndevice,n);
    if( (flag == UUID_2  && cp->uuidtype == 2) || 
        (flag == UUID_16 && cp->uuidtype == 16) )
      {
      getout = 0;
      for(k = 0 ; k < cp->uuidtype && getout == 0 ; ++k)
        {
        if(uuid[k] != cp->uuid[k])
          getout = 1;
        }
      if(getout == 0)
        return(n);  // found uuid - return index
      }
    }
  return(-1);   // not found
  }


/**************** PRINT LE CHARACTERISTICS ********/
     



int printctics1(int ndevice)
  {
  int k,j,i,i0,pn,count,len,del,psnx;
  struct cticdata *cp;
  
  static char *permsn[16] = {" ? ","r  ","w  ","rw ","wa ","rwa"," ? "," ? ","n  ","rn ","wn ","rwn","wan","rwan","??n","??n"  };
  static char *permsi[16] = {" ? ","r  ","w  ","rw ","wa ","rwa"," ? "," ? ","i  ","ri ","wi ","rwi","wai","rwai","??i","??i"  };
  char **perms;
  char sizes[8];     

  count = 0;

  if(ctic(ndevice,0)->type != CTIC_ACTIVE)
    {
    if(dev[ndevice]->type == BTYPE_LE)
      NPRINT "     No characteristics - Read services to find\n");
    return(0);
    }

  // psn = 0;
  psnx = -1;  
    
  NPRINT "   ctic\n");
  NPRINT "   index       LE Characteristics\n");
  
  for(k = 0 ; ctic(ndevice,k)->type == CTIC_ACTIVE ; ++k)
    {
    cp = ctic(ndevice,k);
    if(cp->size == 0)
      sprintf(sizes,"?");
    else
      sprintf(sizes,"%d",cp->size);
    pn = (cp->perm >> 1) & 7;
    perms = permsn;   
    if((cp->perm & 0x30) != 0)
      {
      pn |= 8;  // notify or indicate
      if((cp->perm & 0x20) != 0)
        perms = permsi;  // indicate  
      }                   
    len = strlen(cp->name);                 
       
    if(ndevice == 0 && (cp->psnx & 0xFFFF) != psnx)
      {
      i0 = (cp->psnx & 0xFFFF);   
      for(i = i0 - 1 ; i > psnx && i >= 0 ; --i)
        i0 = i;
      for(i = i0 ; i < (cp->psnx & 0xFFFF) ; ++i)  
        NPRINT "        Empty Primary Service Handle=%04X\n",pserv[i].handle);
             
      psnx = cp->psnx & 0xFFFF;
      NPRINT "        PRIMARY SERVICE = ");  
      for(j = 0 ; j < pserv[psnx].uuidtype ; ++j)
        NPRINT "%02X",pserv[psnx].uuid[j]);
      NPRINT "\n");
      }                 

    NPRINT "     %d  %s",k,cp->name);  
     
    if(len < 20)
      del = 20 - len;
    else
      del = NAMELEN - len;
   
    for(j = 0 ; j < del ; ++j)
      NPRINT " ");
         
    NPRINT " %s byte Permit %02X %s ",sizes,cp->perm,perms[pn]);
    
    if(cp->chandle == 0)
      NPRINT "Handle ? ");
    else
      NPRINT "Handle=%04X ",cp->chandle);

    if(ndevice == 0 && cp->size < 8) 
      {
      NPRINT "Value=");
      for(j = 0 ; j < cp->size && j < LEDATLEN ; ++j)
        NPRINT "%02X ",cp->value[j]); 
      }
          
    if(cp->uuidtype == 0)
      NPRINT "UUID ?\n");
    else
      {
      if(cp->uuidtype == 16)
        NPRINT "\n            ");
      NPRINT "UUID=");      
      for(j = 0 ; j < cp->uuidtype ; ++j)
        NPRINT "%02X",cp->uuid[j]);
      NPRINT "\n");
      }

    if(ndevice == 0 && cp->size >= 8) 
      {
      NPRINT "            Value=");
      for(j = 0 ; j < cp->size && j < LEDATLEN ; ++j)
        NPRINT "%02X ",cp->value[j]);
      NPRINT "\n"); 
      }

    flushprint();
    }
  return(count);
  }




int printctics0(int devicen,int flags,char *listbuf,int listlen)
  {
  int n,j,jn,k,xn,ln,bn,len,maxlen,count,flag,delflag,perm,listflag;
  short *vn;   // list of vailid index
  struct cticdata *cp;
  char *buf;

  if(ctic(devicen,0)->type != CTIC_ACTIVE)
    return(0);

  vn = (short*)gpar.buf;  // 256
  buf = (char*)(gpar.buf+512); 
       
  maxlen = 0;  
  count = 0;
  ln = 0;
  if(listbuf == NULL || listlen <= 0)
    listflag = 0;
  else
    listflag = 1;
    
  for(n = 0 ; ctic(devicen,n)->type == CTIC_ACTIVE ; ++n)
    {
    perm = ctic(devicen,n)->perm;      
    if(  (devicen == 0) ||
         (flags & (CTIC_R | CTIC_W | CTIC_NOTIFY)) == 0 || (perm == 0 && (flags & CTIC_NOTIFY) == 0) ||
         ( (flags & CTIC_NOTIFY) != 0 && (perm & 0x30) != 0) ||
         ( (flags & CTIC_R)   != 0 && (perm & 0x02) != 0) ||
         ( (flags & CTIC_W)   != 0 && (perm & 0x4C) != 0)  ) 
      {
      vn[count] = n;   // save index of valid entry
      if(count < 255)
        ++count;          
      }
    }
  
  if(count == 0)
    return(0);
  
  flag = 0;        // one column
  xn = count;      // last vn index + 1
  if(listflag == 0 && count > 5)
    {
    flag = 1;   // two column
    xn = (count+1)/2;    // last vn index + 1  of first column
         // find max length of first column       
    for(n = 0 ; n < xn ; ++n)
      {
      len = strlen(ctic(devicen,vn[n])->name);
      if(len > maxlen)
        maxlen = len;
      }
    }
    
  delflag = 10;
  if(listflag == 0)
    NPRINT "ctic      LE characteristics\nindex\n"); 
  for(n = 0 ; n < xn ; ++n)
    {
    cp = ctic(devicen,vn[n]);
    if(listflag == 0)
      NPRINT " %d  %s",vn[n],cp->name);
    else
      {
      sprintf(buf,"%d - %s\n",vn[n],cp->name);
      bn = 0;
      while(buf[bn] != 0 && ln < listlen-1)
        {
        listbuf[ln] = buf[bn];
        ++bn;
        ++ln;
        }
      listbuf[ln] = 0;
      }


    if(flag != 0)
      {
      if(vn[n] >= delflag)
        {
        --maxlen;
        delflag *= 10;
        }

      j = n+xn;   // 2nd column vn[0 to count-1] index
      if(j < count)
        {
        jn = vn[j];      
        k = strlen(cp->name); // of first column
        
        while(k < maxlen + 4)
          {
          NPRINT " ");
          ++k;
          }
        NPRINT "%d  %s",jn,ctic(devicen,jn)->name);
        }
      }    
    if(listflag == 0) 
      {    
      NPRINT "\n");
      flushprint();
      }
    }   
   
  if(listflag == 0)
    { 
    NPRINT "\n");
    flushprint();
    }
  return(count);
  }




int finduuidtext(int uuid2)
  {
  int n,uuid;
  static int xn = 0;  // index of UUID=0001  jump past 2A/2B
  static int maxuuid = 0;
     
 
  if(xn == 0)
    {
    n = 1;
    while(!(uuidlist[n] == 0 && uuidlist[n+1] == 0) )
      {
      uuid = (uuidlist[n] << 8) + uuidlist[n+1];
      if(uuid == 1)
        xn = n;   // index 0001 start
      if(uuid > maxuuid)
        maxuuid = uuid;
      n += 2;
      while(uuidlist[n] != 0)
        ++n;
      ++n;    
      }
    }

  if(uuid2 > maxuuid)
    return(0);
 
  if(xn == 0)  // not found 0001 on first try
    xn = -1;   // no try again
   
  if(xn > 0 && ((uuid2 >> 8) & 0x2A) != 0x2A)
    n = xn;   // not 2A/2B 0001 start 
  else
    n = 1;
      
  while(!(uuidlist[n] == 0 && uuidlist[n+1] == 0) )
    {
    if( (uuidlist[n] << 8) + uuidlist[n+1] == uuid2 )
      return(n+2);   // index of text
    n += 2;
    while(uuidlist[n] != 0)
      ++n;
    ++n;  
    }
  return(0);  // points to term 0 string
  }
  



int savectic(int devicen,struct servicedata *serv,int servlen)
  {
  int n,handle,k,j; 
  struct cticdata *cp; 
  char *errs;
  static unsigned char buf[256];

  if(devokp(devicen) == 0 || serv[0].channel == 0)
    return(0);
   
  errs = "not correct in local devices.txt info";
  
    // look for existing entries in device info 

  for(n = 0 ; ctic(devicen,n)->type == CTIC_ACTIVE ; ++n)
    {
    cp = ctic(devicen,n);
    cp->iflag = 0;
          
    // look for handle match
    handle = cp->chandle;  // may be 0 not set
    for(k = 0 ; k < servlen && serv[k].channel != 0 && cp->iflag == 0 && handle != 0 ; ++k)
      {
      if(serv[k].channel == 0x10000 && serv[k].handle == handle)
        {
        cp->iflag = 1;               // exit k loop
        serv[k].channel = 0x20000;   // ditch      
        if(cp->perm != 0 && cp->perm != serv[k].perm)
          {
          cp->perm = 0;
          NPRINT "WARNING - %s permit %s\n",cp->name,errs);
          }
        if(cp->perm == 0)
          cp->perm = serv[k].perm;
          
        if(cp->uuidtype != 0 && (cp->uuidtype != serv[k].uuidtype || bincmp(cp->uuid,serv[k].uuid,cp->uuidtype,DIRN_FOR) == 0))
          {
          cp->uuidtype = 0;
          NPRINT "WARNING - %s UUID %s\n",cp->name,errs);
          }
        if(cp->uuidtype == 0)
          {
          cp->uuidtype = serv[k].uuidtype;
          for(j = 0 ; j < cp->uuidtype ; ++j)
            cp->uuid[j] = serv[k].uuid[j];
          }
        }
      }
   
    // look for UUID match
     
    for(k = 0 ; k < servlen && serv[k].channel != 0 && cp->iflag == 0 ; ++k)
      {
      if(serv[k].channel == 0x10000 && bincmp(cp->uuid,serv[k].uuid,cp->uuidtype,DIRN_FOR) != 0)
        {
        serv[k].channel = 0x20000;   // ditch
        cp->iflag = 1;               // exit k loop
        if(cp->perm != 0 && cp->perm != serv[k].perm)
          {
          cp->perm = 0;
          NPRINT "WARNING - %s permit %s\n",cp->name,errs);
          }
        if(cp->perm == 0)
          cp->perm = serv[k].perm;
          
        if(cp->chandle != 0 && cp->chandle != serv[k].handle)
          {
          cp->chandle = 0;
          NPRINT "WARNING - %s handle %s\n",cp->name,errs);
          }
        if(cp->chandle == 0)
          cp->chandle = serv[k].handle;        
        }
      }         
        
    }

  // add new entries to device info
         
  for(n = 0 ; n < servlen && serv[n].channel != 0 ; ++n)
    {  
    if(serv[n].channel == 0x10000 && serv[n].perm != 0)
      {
      cp = cticalloc(devicen);
      if(cp->type != CTIC_UNUSED)
        {
        flushprint();
        return(0);
        }      
        
      cp->type = CTIC_ACTIVE;  // valid entry  
      cp->chandle = serv[n].handle;
      cp->size = 0;  // unknown  
      cp->perm = serv[n].perm;
      cp->uuidtype = serv[n].uuidtype;
      cp->iflag = 1;
      for(j = 0 ; j < serv[n].uuidtype ; ++j)
        cp->uuid[j] = serv[n].uuid[j];
                  
      k = 0;
      while(serv[n].data[k] != 0 && k < NAMELEN-1)
        {   // to name[NAMELEN]
        cp->name[k] = serv[n].data[k];
        ++k;
        }
      cp->name[k] = 0;
        
            // strip trailing spaces  
      --k;    
      while(k > 1 && cp->name[k] == ' ')
        {
        cp->name[k] = 0;
        --k;
        }
      
      if((cp->perm & 0x30) != 0)
        {    
        // notify/indicate look for enable
        k = cp->chandle+1;
        leread04[14] = (unsigned char)(k & 0xFF);
        leread04[16] = leread04[14];  
        k >>= 8;
        leread04[15] = (unsigned char)(k & 0xFF);
        leread04[17] = leread04[15];
      
        sendhci(leread04,devicen);  
        readhci(devicen,IN_ATTDAT,0,gpar.timout,gpar.toshort);  
        k = findhci(IN_ATTDAT,devicen,INS_NOPOP);
        if(k < 0)
          cp->notify = 3;
        else
          {
          if(insdat[1] == 1 && insdat[5] == 0x29 && insdat[4] == 0x02)
            {
            VPRINT "Handle %04X has 2902 enable mechanism\n",cp->chandle); 
            cp->notify = 0;  // 2902 enable char
            }
          else
            {
            VPRINT "Handle %04X has no 2902 enable mechanism\n",cp->chandle); 
            cp->notify = 3;  // no enable char
            }
          }
        }
          // read value to find size
      if((cp->perm & 2) != 0) 
        {
        if(read_ctic(dev[devicen]->node,cp->cticn,buf,sizeof(buf)) == 0)
          NPRINT "  Error reading characteristic index %d\n",cp->cticn);
        } 
      } 
    }

     
  NPRINT "Characteristics saved to device info\n");
   
  flushprint();
  return(1);
  }


int setlelen(int ndevice,int len,int flag)
  {
  int xlen;  

  VPRINT "Set larger data length\n");  
  sendhci(datlenset,ndevice);

  if(len > LEDATLEN)
    xlen = LEDATLEN;
  else
    xlen = len;

  if(xlen > dev[ndevice]->setdatlen && flag != 0 && 
         !(dev[ndevice]->type == BTYPE_ME && (dev[ndevice]->conflag & CON_LE) == 0) )
    {  // not for node connect       
    VPRINT "Set MTU\n");  
    dev[ndevice]->setdatlen = xlen;
    xlen += 3;  
    mtuset[PAKHEADSIZE+10] = (unsigned char)(xlen & 0xFF);
    mtuset[PAKHEADSIZE+11] = (unsigned char)((xlen >> 8) & 0xFF);
    sendhci(mtuset,ndevice);
    }
          
  return(0);  
  }



/*********** CONNECT PSM ************
psm = 1  control for readservices
      3 data for read/write
*************************************/
      
int connectpsm(int psm,int channel,int ndevice)
  {
  int n,n0;
  struct devdata *dp;

 
  dp = dev[ndevice];
    
  dp->id = 3;
  dp->psm = psm;  
  if(psm == 1)
    n0 = 2;
  else
    n0 = 0;

  dp->scid[n0] = channel;
  dp->scid[n0+1] = 0;
      
  conpsm1[PAKHEADSIZE+13] = psm;
  
  if(gpar.printflag == PRINT_VERBOSE)
    {  
    VPRINT "SEND connect L2CAP channel psm %d\n",psm);   
    VPRINT "  Choose local channel %02X%02X (Must be >= 0x0040). Choose id %d\n",dp->scid[1],dp->scid[0],dp->id);   
    VPRINT "  Set [13] psm %02X\n",psm);
    }
    
  sendhci(conpsm1,ndevice);

  readhci(ndevice,IN_L2REPCT,0,gpar.timout,gpar.toshort);
  
  n = findhci(IN_L2REPCT,ndevice,INS_POP);
  if(n < 0)  
    {
    NPRINT "NO L2 connect reply\n");
    return(0);
    }
    
  if(gpar.printflag == PRINT_VERBOSE) 
    {
    VPRINT "GOT connect OK reply with remote channel\n");
    VPRINT "  Remote channel for following sends = %02X%02X\n",insdatn[5],insdatn[4]);
    }
    
     
  popins();
  
  dp->id = 4;
  dp->psm = psm;    
  VPRINT "SEND L2 config request. Choose id %02X\n",dp->id);
  sendhci(figreq,ndevice);
  readhci(ndevice,IN_L2REPCF,0,gpar.timout,gpar.toshort);
     
  n = findhci(IN_L2REPCF,ndevice,INS_POP);
  if(n < 0)
    {
    NPRINT "NO L2 config reply\n");
    return(0);
    }
    
  VPRINT "GOT L2 config reply\n");
 
        
  popins();


  VPRINT "Connect L2CAP psm %d now done OK\n",psm);

      
  flushprint();

  return(1);
  }


/***********DISCONNECT L2CAP initiated by local **********/

int disconnectl2(int ndevice,int psm)
  {
  struct devdata *dp;
  
  dp = dev[ndevice];
  
  if(psm == 3 && (dp->conflag & CON_PSM3) == 0)
    return(1);   // no L2CAP open
  if(psm == 1 && (dp->conflag & CON_PSM1) == 0)
    return(1);   // no L2CAP open

  if(psm == 1)
    dp->conflag |= CON_PSM1X;  // disconnect requested by local
  else                         // stop immediate() sending second psmdisreq  
    dp->conflag |= CON_PSM3X;
  
     // close l2cap channel
  dp->psm = psm;
  dp->id = 8;
  VPRINT "SEND disconnect L2CAP channel. Choose id %02X\n",dp->id);
  sendhci(psmdisreq,ndevice);
    
  // expect disconnect reply and maybe scid close request
  // X flags stop a repeat psmdisreq in readhci/immediate
    
  readhci(ndevice,IN_L2REPDIS,0,gpar.timout,gpar.toshort);
  if(findhci(IN_L2REPDIS,ndevice,INS_POP) < 0)
    VPRINT "Not seen L2CAP disconnect reply\n");
  else
    VPRINT "GOT L2CAP disconnect done reply\n");
        
  if(psm == 1)
    dp->conflag &= ~(CON_PSM1 | CON_PSM1X); 
  else
    dp->conflag &= ~(CON_RF | CON_CH0 | CON_PSM3 | CON_PSM3X);

  popins();
  flushprint();     
  return(1);
  }
  

  
/**************** SCAN FOR DEVICES *****************/

void classic_scan()
  {
  if(check_init(0) == 0)
    return;
  clscanx();
  flushprint();
  } 


void clscanx()
  {
  int n,k,j,repn,nrep,ndevice,count,newcount,flag;
  unsigned char *rp;
  struct devdata *dp;
  char *buf;
  
  buf = (char *)gpar.buf;
   
  NPRINT "Scanning for Classic devices - 10 seconds\n");

  for(n = 0 ; devok(n) != 0 ; ++n)
    dev[n]->foundflag = 0;
    
  sendhci(cscan,0);  // with 10 sec timeout
  readhci(0,IN_INQCOMP,IN_CSCAN,15000,gpar.toshort);
   
  if(findhci(IN_INQCOMP,0,INS_POP) < 0)
    NPRINT "Not seen inquiry complete\n");
    
  count = 0;
  newcount = 0;
  do
    {
    n = findhci(IN_CSCAN,0,INS_LOCK);
    if(n < 0)
      {
      NPRINT "Found %d unknown devices\n",newcount);    
      popins();
      return;
      }
                        // insdat[n] = number of replies - each 14 bytes
    rp = insdatn+1;  // start of first reply
    nrep = insdatn[0];
    for(repn = 0 ; repn < nrep ; ++repn)
      {
      flag = 0;  // no MATCH_NAME
      
      ndevice = devnfrombadd(rp,BTYPE_CL | BTYPE_ME,DIRN_REV);
      if(ndevice >= 0 && dev[ndevice]->foundflag == 0)
        {
        NPRINT "FOUND %s\n",baddstr(rp,1));      
        NPRINT "   Node=%d   Known device %s\n",dev[ndevice]->node,dev[ndevice]->name);
        if(dev[ndevice]->type != BTYPE_CL && dev[ndevice]->type != BTYPE_ME)
          NPRINT "   But not listed as Classic or Mesh\n"); 
        dev[ndevice]->foundflag = 1;
        ++count;
        }
      else if(ndevice < 0)
        {     
        NPRINT "FOUND %s\n",baddstr(rp,1));      
        ndevice = devalloc(rp,BTYPE_CL);
        if(ndevice < 0)
          {
          instack[n] = INS_POP;
          return;    
          }
        
        ++newcount;     
        dp = dev[ndevice];
        dp->node = newnode();          
        dp->foundflag = 1;
        NPRINT "   Trying to read name..\n");
        flushprint();        
        sendhci(cname,ndevice);      
        readhci(ndevice,IN_CNAME,0,8000,gpar.toshort);   
        j = findhci(IN_CNAME,ndevice,INS_POP);
        if(j < 0)
          {   // no name
          k = devnfrombadd(dp->baddr,BTYPE_LE,DIRN_FOR);
          if(k > 0 && bincmp((unsigned char*)dev[k]->name,(unsigned char*)" LE ",4,DIRN_FOR) == 0)
            strcpy(buf,dev[k]->name);  // is also LE with name
          else
            {
            sprintf(buf,"Classic %s",baddstr(dp->baddr,0));
            // NPRINT "   Unable to read name so set = %s\n",buf);
            }
          }
        else
          {  // got name
          k = 0;
          while(k < NAMELEN-1 && insdat[j+k] != 0)
            { 
            buf[k] = insdat[j+k];
            ++k;
            }
          buf[k] = 0;

            
          // MATCH_NAME?
          for(k = 1 ; flag == 0 && devok(k) != 0 ; ++k)
            {
            if(k != ndevice && (dev[k]->type == BTYPE_CL || dev[k]->type == BTYPE_ME))
              {
              if((dev[k]->matchname & 1) != 0)
                {  // MATCH_NAME
                flag = 1;
                for(j = 0 ; j < (int)strlen(dev[k]->name) && flag != 0 ; ++j)
                  {
                  if(dev[k]->name[j] != buf[j])
                    flag = 0;
                  }
                if(flag != 0)
                  {  
                  dp->type = 0;      // free ndevice
                  dp->foundflag = 0;
                  ndevice = k;       // swap to known device k
                  NPRINT "   Found via MATCH_NAME\n");
                  NPRINT "   Node=%d   Known device %s\n",dev[ndevice]->node,dev[ndevice]->name);
                  dev[k]->matchname |= 2;  // found address flag
                  dev[k]->foundflag = 1;
                  ndevice = k;
                  for(j = 0 ; j < 6 ; ++j)
                    dev[ndevice]->baddr[j] = rp[5-j];          
                  }
                } 
              }
            }           

          if(flag == 0)
            {
            k = devnfrombadd(dp->baddr,BTYPE_LE,DIRN_FOR);
            if(k > 0 && bincmp((unsigned char*)dev[k]->name,(unsigned char*)" LE ",4,DIRN_FOR) != 0)
              strcpy(dev[k]->name,buf);  // is also LE without name
            }
          }

        if(flag == 0)
          {
          strcpy(dev[ndevice]->name,buf);                
          NPRINT "   Node=%d   New device %s\n",dev[ndevice]->node,dev[ndevice]->name);
          }
        ++count;
        }
      rp += 14;
      }
    instack[n] = INS_POP;
    flushprint();
    }
  while(1); 
  }

/******** PRINT SDP DATA **************/

  
int decodesdp(unsigned char *sin,int len,struct servicedata *serv,int servlen)
  {
  struct sdpdata sdp;
        
  sdp.level = 0;
  sdp.record = 0;
  sdp.aid = -1;
  sdp.numchan = 0;   // number of RFCOMM channels found
  sdp.serv = serv;
  sdp.servlen = servlen;
  sdp.chdn = -1;   // sdp.serv[] index - inc before use
  sdp.serv[0].channel = 0;  // end mark
  sdp.nameflag = 0;  
      
  decodedes(sin,len,&sdp);
  
  flushprint();
 
  return(sdp.numchan);
  }
  
/******** DECODE DATA ELEMENT SEQUENCE 
re-entrant for mulitple des levels
******************************/  
  
int decodedes(unsigned char *sin,int len,struct sdpdata *sdpp)  //  int level,int aidflag,int record,unsigned int callaid)
  {
  int k,type,size,loop,textflag,ntogo,locaidflag,aidflip,loclevel,prflag,uuidn;
  unsigned int j,datlen,uuid;
  unsigned char *s;
  static int szlook[8] = {1,2,4,8,16,101,102,104};
   
  
  loclevel = sdpp->level;
  locaidflag = 0;  // set 1 if this is aid level
  aidflip = 0;     // aid/value alternate
  uuid = 0;
  
  loop = 0;
  s = sin;
  ntogo = len;

      
  do
    {
    if(s[0] == 0)
      return(-1);   // invalid=end  
    if(sdpp->aid < 0 && s[0] == 0x09)  // found first aid - this is aid level 
      {
      locaidflag = 1;
      sdpp->aid = 0;        // should find anyway
      sdpp->uuidtype = 0;   // for uuid search - target not found
      sdpp->uuid = NULL;
      sdpp->nameflag = 0;
      
      VPRINT " **** RECORD %d ****\n",sdpp->record);
      } 
       
    type = s[0] >> 3;
    if(type > 8)
      {
      NPRINT "Invalid des type %d\n",type);
      return(0);
      }
    size = s[0] & 7;
    ++s;
    --ntogo;
    uuidn = 0;   // uuidlist null string
    datlen = szlook[size];
    if(datlen > 100)
      {
      size = datlen-100;
      datlen = 0;
      for(k = 0 ; k < size ; ++k)
        {
        datlen = (datlen << 8) + s[0];
        ++s;
        --ntogo;
        }
      }
       
    // s = data  length=datlen
    if(type == 6 || type == 7)    // data element seq
      {  
      ++sdpp->level;
      flushprint();   
      k = decodedes(s,datlen,sdpp);
      flushprint();
      if(k < 0)
        return(-1);
      if(k != 0)   // level+1,(locaidflag | aidflag),locrecord,aid);
        { // returned from aid level - start new record
        ++sdpp->record;
        sdpp->aid = -1;
        }
      sdpp->level = loclevel;
      }
    else   // single value - not a des     
      {
      prflag = 1;   // print hex data
      uuidn = 0;    // uuidtext index
      if(gpar.printflag == PRINT_VERBOSE)
        {
        for(k = 0 ; k < loclevel ; ++k)
           VPRINT "  ");
        }
      if(locaidflag != 0)   // only at first level where 09
         {
         if(aidflip == 0)  // new aid
           { 
           sdpp->aid = (s[0] << 8) + s[1];
           VPRINT "aid = %04X",sdpp->aid);
           prflag = 0;
        //   if(sdpp->aid == 0)
        //     NPRINT "  ***** RECORD %d **** ",sdpp->record);
           }
         else  // aid value - single value or sequence
           {
           VPRINT "    ");
           if(sdpp->aid == 0)
             {
             sdpp->handle = (s[0] << 24) + (s[1] << 16) + (s[2] << 8) + s[3];
             VPRINT "Handle ");
             }
           }
         }

      if(type == 3)   // UUID
        {
        if(datlen == 2)
          {
          uuid = (s[0] << 8) + s[1];
          uuidn = finduuidtext(uuid);
          }
          
        if(sdpp->aid == 1)
          {
          if(sdpp->uuid == NULL)
            {  // for save to servicedata           
            sdpp->uuid = s;
            sdpp->uuidtype = datlen;       
            }
          }  
                       
        VPRINT "UUID ");
   
        }
      
      if(sdpp->aid == 4 && uuid == 0x0003 && type == 1 && datlen == 1)
        {   // rfcomm channel
        VPRINT "RFCOMM channel = %d",s[0]);
             
        ++sdpp->numchan;
                
        if(sdpp->uuidtype > 0 && sdpp->uuid != NULL)
          {
          if(sdpp->chdn < sdpp->servlen-2)
            {
            ++sdpp->chdn;
            sdpp->serv[sdpp->chdn+1].channel = 0;  // mark next as end
            }
          else
            NPRINT "Need more channel entries\n");
            
          sdpp->serv[sdpp->chdn].channel = s[0];
          sdpp->serv[sdpp->chdn].uuidtype = sdpp->uuidtype;
          sdpp->serv[sdpp->chdn].handle = sdpp->handle;

          for(k = 0 ; k < sdpp->uuidtype && k < 16 ; ++k)
            sdpp->serv[sdpp->chdn].uuid[k] = sdpp->uuid[k];
          strcpy(sdpp->serv[sdpp->chdn].data+1,"No name");                         // string to data[1]
          sdpp->serv[sdpp->chdn].data[0] = (char)strlen(sdpp->serv[sdpp->chdn].data+1);  // length of string to data[0]
          sdpp->nameflag = 1;   // look for aid = 0100 service name
          }

               
        prflag = 0;
        }
          
      if(sdpp->aid == 0x0100 && (type == 4 || type == 8) && sdpp->chdn >= 0 && sdpp->nameflag != 0)
        {    // aid = 0100 service name text
        j = 0;
        while(j < datlen && j < 62)   // data[64]
          {       
          sdpp->serv[sdpp->chdn].data[j+1] = s[j];  // name to data[1]...
          ++j;
          }
        sdpp->serv[sdpp->chdn].data[j+1] = 0;
        sdpp->serv[sdpp->chdn].data[0] = j;  // length to data[0]
       
        sdpp->nameflag = 0;
        }
        
      if(gpar.printflag == PRINT_VERBOSE)
        {   
        textflag = 0;
        if(type == 4 || type == 8)
          textflag = 1;
        for(j = 0 ; j < datlen && prflag != 0; ++j)
          {
          if(textflag == 0)
            {
            VPRINT "%02X ",s[j]);
            if( ((j+1) % 20) == 0)
              VPRINT "\n");
            }
          else
            {
            VPRINT "%c",s[j]);
            if( ((j+1) % 40) == 0)
              VPRINT "\n");
            }
          }

        VPRINT "%s\n",uuidlist+uuidn);
        }    // end print
      }    // end single value

         
    if(locaidflag != 0)
      aidflip = 1 - aidflip;
         
    s += datlen;
    ntogo -= datlen;
    ++loop;
    flushprint();
    }
  while(ntogo > 0 && loop < 256);
  return(locaidflag);  // 1 if this level is aid level
  }


/***************************** CLIENT SETUP ****************/



int connect_node(int node,int channelflag,int channel)
  {
  int ndevice,type,retval;

  if(check_init(0) == 0)
    return(0);
  
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);
  
  if(dev[ndevice]->conflag != 0)
    {
    NPRINT "Already connected\n");
    flushprint();
    return(0);
    }
  
  if(dev[ndevice]->matchname == 1)
    {  // must be 0 or 3
    NPRINT "Address via MATCH_NAME not found - run a scan\n");
    flushprint();
    return(0);
    }
    
  retval = 0;  
  type = dev[ndevice]->type;
  dev[ndevice]->lecflag = 0;  
  
  if(type == BTYPE_CL || (type == BTYPE_ME && (channelflag == CHANNEL_NEW || channelflag == CHANNEL_STORED)))
    retval = clconnect(ndevice,channel,channelflag);
  else if(type == BTYPE_LE || type == BTYPE_ME)
    {
    if(type == BTYPE_ME && channelflag == CHANNEL_LE)
      dev[ndevice]->lecflag = 1;  // remote mesh device listening as an LE server
    else if(type == BTYPE_LE && channelflag != CHANNEL_LE)
      NPRINT "NOTE - should use: connect_node(node,CHANNEL_LE,0)\n");   
    retval = leconnect(ndevice);
    }
  else
    NPRINT "Cannot connect to %s\n",dev[ndevice]->name);
      
  flushprint();
  return(retval);  
  }

/********** CLIENT CONNECT ********
method = CHANNEL_STORED   use stored rfchan 
                _NEW      use channel in parameters
          
**********************************/

int clconnect(int ndevice,int channel,int channelflag)
  {
  int retval;
  struct devdata *dp;
   
     // ndevice checked
    
  if(!(channelflag == CHANNEL_NEW || channelflag == CHANNEL_STORED))
    {
    NPRINT "Classic server must use CHANNEL_NEW or CHANNEL_STORED\n");
    return(0);
    }  
     
  dp = dev[ndevice];
  dp->method = METHOD_HCI; 
     
  if(channelflag == CHANNEL_STORED)
    {  // use previous rfchan
    if(dp->rfchan == 0)
      {
      NPRINT "No stored channel from device info or previous connect\n");
      return(0);
      }
    } 
  else 
    dp->rfchan = channel;
  
  if((dp->type & BTYPE_ME) != 0)
    NPRINT "Connecting to %s...\n",dp->name); 
  else
    NPRINT "Connecting to %s on channel %d...\n",dp->name,dp->rfchan);
         
  retval = clconnect0(ndevice);
  flushprint(); 
  if(retval != 0)
    retval = sconnect(ndevice);     
  if(retval == 0)
    disconnectdev(ndevice);
  else
    NPRINT "Connect OK\n");   
  return(retval);
  }

  
void setcredits(int ndevice)
  {
  struct devdata *dp;

     
  if(devok(ndevice) == 0)
    return;
    
  dp = dev[ndevice];
 
 
  dp->credits += TOPUPCREDIT;   
  setcred[12+PAKHEADSIZE] = TOPUPCREDIT;
  if(gpar.printflag == PRINT_VERBOSE)
    {
    VPRINT "SEND Top up credits\n");
    VPRINT "  Set [12] credits = %02X\n",dp->credits);
    }
  sendhci(setcred,ndevice);
  
  flushprint();
  }


int sconnect(int ndevice)
  {
  int retval;
  
  retval = sconnectx(ndevice);
  flushprint();
  return(retval);
  }


int sconnectx(int ndevice)
  {  
  struct devdata *dp;
  
  dp = dev[ndevice];
 
  if(connectpsm(3,0x44,ndevice) == 0)
    return(0);
        
  if(gpar.printflag == PRINT_VERBOSE)
    {
    VPRINT "SEND Open CONTROL channel 0\n");  //56
    VPRINT "  Command address = 03\n");
    VPRINT "  Reply address = 01\n");
    VPRINT "  DLCI = 0\n");
    }
 
  sendhci(sabm0,ndevice);
  readhci(ndevice,IN_RFUA,0,gpar.timout,gpar.toshort);
  
  if(findhci(IN_RFUA,ndevice,INS_POP) < 0)
    {
    NPRINT "No UA reply\n");
    return(0);
    }

  dp->conflag |= CON_CH0;
  
  popins();

       // need rfchan now

  if(gpar.printflag == PRINT_VERBOSE)
    {
    VPRINT "GOT UA reply (03 73)\n");   // 58 
    VPRINT "Open RFCOMM channel %02X obtained from remote SDP services database\n",dp->rfchan);  //61
    VPRINT "  Command address (channel<<3+3) = %02X\n",(dp->rfchan << 3) + 3);
    VPRINT "  Reply address   (channel<<3+1) = %02X\n",(dp->rfchan << 3) + 1);
    VPRINT "  DLCI (channelx2) = %02X\n",dp->rfchan << 1); 
    VPRINT "SEND PN CMD to set RFCOMM channel and parameter negotiation\n");    // 59
    }
 
    
  sendhci(pncmd,ndevice);
  readhci(ndevice,IN_PNRSP,0,gpar.timout,gpar.toshort);

  if(findhci(IN_PNRSP,ndevice,INS_POP) < 0)
    {
    NPRINT "No PN RSP reply - probably server not listening on channel %d\n",dp->rfchan);
    return(0);
    }
  
  VPRINT "GOT PN RSP reply (01 EF 81)\n");  // 60
    
  popins();

      
  VPRINT "SEND Open RFCOMM channel %02X\n",dp->rfchan);  //61
 
 
  sendhci(sabmx,ndevice);
  readhci(ndevice,IN_RFUA,0,gpar.timout,gpar.toshort);

  if(findhci(IN_RFUA,ndevice,INS_POP) < 0)
    {
    NPRINT "No UA reply\n");
    return(0);
    }

  VPRINT "GOT UA reply (%02X 73)\n",(dp->rfchan << 3)+3);   // 63 server accepts?
     
  popins();

  VPRINT "SEND MSC E3 to set modem status\n");   // 64
  msccmdsend[PAKHEADSIZE+9] = 0x03;
  msccmdsend[PAKHEADSIZE+12] = 0xE3;
  sendhci(msccmdsend,ndevice);
 // readhci(0,0,0,50,0);  // peek

  VPRINT "SEND MSC E1\n"); // 66 to receive data?
  msccmdsend[PAKHEADSIZE+12] = 0xE1;
  sendhci(msccmdsend,ndevice);
  readhci(0,0,0,25,0);  // peek
  
  dp->credits = 0;
  setcredits(ndevice);
  
      
  VPRINT "Serial RFCOMM channel is now open for read/write\n");

  popins();

  dp->conflag |= CON_RF;  // RFCOMM
    
  return(1);
  }


/************* WRITE **********/


int write_node(int node,unsigned char *outbuff,int count)
  {
  int n,xlength,datn,retval,flag,ndevice,pflag;
  unsigned char *bs,*dat;
  static char buf[128];
  char xbuf[8];

  if(check_init(0) == 0)
    return(0);

  if(count == 0)
    return(1);
      
  ndevice = devnp(node);  
  if(ndevice < 0)
    return(0);
 
  if((dev[ndevice]->conflag & (CON_MESH | CON_RF)) == 0)
    {
    NPRINT "%s not classic/node connected for write\n",dev[ndevice]->name);
    flushprint();
    return(0);
    }  
 
  n = 1000;
  if((dev[ndevice]->conflag & CON_MESH) != 0)
    n = 400;
  if(count > n)   
    {
    NPRINT "Write too many bytes (max %d)\n",n);
    flushprint();
    return(0);
    }  
    
  if((dev[ndevice]->conflag & CON_MESH) != 0)
    {
    //  packet size=len+9      [3][4]=len + 4  [5][6]=len  [9]=data 
    VPRINT "SEND %d bytes ATT data\n",count);
    attdata[0] = (count+9) & 0xFF;
    attdata[1] = ((count+9) >> 8) & 0xFF;
    attdata[PAKHEADSIZE+3] = (count+4) & 0xFF;
    attdata[PAKHEADSIZE+4] = ((count+4) >> 8) & 0xFF;
    attdata[PAKHEADSIZE+5] = count & 0xFF;
    attdata[PAKHEADSIZE+6] = (count >> 8) & 0xFF;
    dat = attdata + PAKHEADSIZE + 9;
    for(n = 0 ; n < count ; ++n)
      dat[n] = outbuff[n];
    sendhci(attdata,ndevice);
    readhci(0,0,0,0,0);     
    return(count);
    } 
      

  // is RFCOMM connected
     
  if(gpar.printflag == PRINT_VERBOSE)
    {
    if(outbuff[0] < 32 || outbuff[0] > 126)
      flag = 1;
    else
      flag = 0;
    for(n = 0 ; n < count-1 && n < 20 && flag == 0 ; ++n)
      {
      if(outbuff[n] < 32 || outbuff[n] > 126)
        flag = 1;  // binary
      }
               
    buf[0] = 0;
    n = 0;
    while(n < count && n < 20)
      {
      if(n == count-1 && flag == 0 && (outbuff[n] < 32 || outbuff[n] > 126))
        flag = 1;  // term char is non-ascii
        
      if(flag == 0)
        {
        buf[n] = outbuff[n];
        buf[n+1] = 0;
        }
      else
        {
        sprintf(xbuf," %02X",outbuff[n]);
        strcat(buf,xbuf);
        }
      ++n;
      }
     
        
    if(count > 20)
      strcat(buf,"...");
      
    VPRINT "SEND DATA %d bytes = %s\n",count,buf);
    }


    // send serial data over RFCOMM
  pflag = 0;  // 0=EF 1=FF                
  bs = blusend + PAKHEADSIZE; 
  xlength = count;  // assume 1 byte length
  
  if(count < 128)
    {  // 1 byte length
    bs[11] = (count << 1) + 1;  // bit 0 = 1
    datn = 12;  // for print
    VPRINT "  Set [11] one length byte = %02X (length*2 + 1 to indicate 1 byte)\n",bs[11]);   
    }
  else 
    {  // 2 byte length
    bs[11] = (count & 127) << 1;  // bit 0 = 0
    bs[12] = (count >> 7) & 255;
    datn = 13;
    ++xlength;  // one longer
    VPRINT "  Set [11][12] two length bytes = %02X %02X (length*2) (length >> 7)\n",bs[11],bs[12]);   
    }
  
  if(pflag == 0)
    bs[10] = 0xEF;
  else
    {
    bs[10] = 0xFF;
    bs[datn] = 1;  // credit
    ++datn;
    ++xlength;
    }    
    
  dat = bs + datn;
      
  blusend[0] = (xlength + 13) & 255;
  blusend[1] = ((xlength + 13) >> 8) & 255;
  bs[3] = (xlength+8) & 255;
  bs[4] = (xlength+8) >> 8;
 
  bs[5] = (xlength+4) & 255;
  bs[6] = (xlength+4) >> 8;
  VPRINT "  Set [3].. packet lengths %02X %02X %02X %02X\n",bs[3],bs[4],bs[5],bs[6]);
  for(n = 0 ; n < count ; ++n)
    dat[n] = outbuff[n];  
 
  VPRINT "  Set [%d].. %d data bytes %02X..\n",datn,count,dat[0]);
  flushprint();
  retval = sendhci(blusend,ndevice);
  readhci(0,0,0,0,0);
  flushprint();
  if(retval == 0)
    return(0);
  return(count);
  }


/*************** READ SERIAL DATA ***********/


int read_node_count(int node,unsigned char *buf,int count,int exitflag,int timeoutms)
  {
  int locnode;

  if(check_init(0) == 0)
    return(0);
  
  locnode = node;
  return(readserial(&locnode,buf,count,0,(exitflag & 3) | EXIT_COUNT,timeoutms));
  }

int read_node_endchar(int node,unsigned char *buf,int bufsize,char endchar,int exitflag,int timeoutms)
  {
  int locnode;

  if(check_init(0) == 0)
    return(0);
      
  locnode = node;    
  return(readserial(&locnode,buf,bufsize,endchar,(exitflag & 3) | EXIT_CHAR,timeoutms));
  }

int read_all_endchar(int *node,unsigned char *buf,int bufsize,char endchar,int exitflag,int timeoutms)
  {
  int locnode,retval;

  if(check_init(0) == 0)
    return(0);
  
  locnode = FROM_ALLCON;
  retval = readserial(&locnode,buf,bufsize,endchar,(exitflag & 3) | EXIT_CHAR,timeoutms);
   
  if(locnode == 0)
    *node = 0;
  else
    *node = dev[locnode]->node;  // known sender
    
  return(retval);
  }

int read_mesh(int *node,unsigned char *buf,int bufsize,int exitflag,int timeoutms)
  {
  int locnode,retval;

  if(check_init(0) == 0)
    return(0);
 
  meshreadon();
  locnode = FROM_MESH;
  retval = readserial(&locnode,buf,bufsize,0,(exitflag & 3),timeoutms);
  meshreadoff();
    
  if(locnode == 0)
    *node = 0;
  else
    *node = dev[locnode]->node;  // known sender
   
  return(retval);
  }

int read_error()
  {
  if(check_init(0) == 0)
    return(0); 
  return(gpar.readerror);
  }


void read_node_clear(int node)
  {
  int n,ndevice;
       
  if(check_init(0) == 0)
    return;
    
  ndevice = devnp(node);
  if(ndevice < 0)
    return;
  
  do
    {   
    readhci(ndevice,IN_DATA,0,10,0);   
    n = findhci(IN_DATA,ndevice,INS_POP);
    }
  while(n >= 0);
  
  popins();  
  return;
  }
  
   
void read_all_clear()
  {
  if(check_init(0) == 0)
    return;
  readhci(0,0,0,100,10);
  clearins(0);
  } 


void read_notify(int timeoutms)
  {  
  unsigned long long to,tim0;
  int kmsav,key,tox;
 
  if(check_init(0) == 0)
    return;

  if(gpar.serveractive != 0)
    return;  // not when server
    
  if(timeoutms < 0)
    to = 0;
  else
    to = timeoutms; 

  if(to <= 1000)
    {
    tox = (int)to;
    to = 0;
    }
  else
    tox = 50;
   
  kmsav = setkeymode(1);
   
  tim0 = time_ms();
  do
    {
    readhci(0,0,0,tox,0);
    key = readkey();
    }
  while(time_ms() - tim0 < to && key != 'x');
  
  setkeymode(kmsav);
  return;
  }



int readserial(int *node,unsigned char *inbuff,int count,char endchar,int flag,int timeoutms)
  {
  int n,nread,meshcon,clcon,onedevn,ndevice,oldkm;

  for(n = 0 ; n < count ; ++n)
    inbuff[n] = 0;
    
  gpar.readerror = ERROR_FATAL;  // fatal error

  if( (flag & (EXIT_TIMEOUT | EXIT_KEY)) == 0  || count == 0)
    {
    NPRINT "Time out/key flags or count zero in read\n");
    flushprint();
    *node = 0;
    return(0);
    }    

  if((*node & (FROM_ALLCON | FROM_MESH)) != 0)
    ndevice = *node;
  else
    {  // specified device
    ndevice = devnp(*node);
    if(ndevice < 0)
      {
      *node = 0;
      return(0);   // specified device invalid
      }
    }
   
  // ndevice is a valid device or FROM_MESH or  MESHCON | CLCON    
 
  meshcon = 0;  // count of mesh connected
  clcon = 0;    // count of classic connected
  onedevn = 0;
    
  if((ndevice & FROM_ALLCON) != 0)  
    {       // mesh or cl connected search
    for(n = 0 ; devok(n) != 0 ; ++n)
      { 
      if((ndevice & FROM_CLCON) != 0 && (dev[n]->conflag & CON_RF) != 0)
        {
        onedevn = n;
        ++clcon;
        }
      else if((ndevice & FROM_MESHCON) != 0 && (dev[n]->conflag & CON_MESH) != 0)
        {
        onedevn = n;
        ++meshcon;
        }
      }
      
    // if no or only one device   
      
    if(clcon + meshcon == 0)
      {
  //    NPRINT "No connected devices in read\n");
      *node = 0;
  //    flushprint();
  //    return(0);
      }       
    else if(clcon + meshcon == 1)
      ndevice = onedevn;  // is only one possible connected device
    }
  else if((ndevice & FROM_MESH) != 0)
    n = 0;  //  meshreadon();   // start scan - sets MESH_R              
  else if((dev[ndevice]->conflag & (CON_RF | CON_MESH)) == 0)
    {   // specified device
    NPRINT "%s not classic/node connected for read\n",dev[ndevice]->name);
    flushprint();
    *node = 0;
    return(0);
    }
 
 
  if((flag & EXIT_KEY) != 0)
    oldkm = setkeymode(1);   // unblock key read for readkey()   
  else
    oldkm = 2;
       
  nread = readrf(&ndevice,inbuff,count,endchar,flag,timeoutms);  
  if(ndevice > 0 && devok(ndevice) != 0)     
    *node = ndevice; // return device not node
  else
    *node = 0;  // failed
  
  // meshreadoff();
  
  if(oldkm != 2)  // key mode has changed 
    setkeymode(oldkm);  // restore
  
  flushprint();

  return(nread);
  }    
     
        

/*********** RECEIVE CHARS ************/
        
int readrf(int *ndevice,unsigned char *inbuff,int count,char endchar,int inflag,int timeoutms)
  {
  int n,k,len,getout,devicen,flag,gotn,ndev,meshflag,key;
  char lastchar;
  unsigned char *dat,*ecp;
  unsigned long long timstart;
     
  ecp = (unsigned char*)&endchar;
  meshflag = 0;
  if(*ndevice == FROM_MESH)
    {
    meshflag = 1;  // ignores EXIT_CHAR/EXIT_COUNT - returns first IN_DATA packet
    devicen = 0;   // try all until find one
    }
  else if((*ndevice & FROM_ALLCON) != 0)
    devicen = 0;  // try all until find one
  else
    devicen = *ndevice;  // specified device only
      
  gotn = 0;  // number read
  inbuff[0] = 0;
    
  flag = inflag;
  lastchar = 0;
  timstart = time_ms();

  do  // until exit from within loop
    {
    do   // until no more IN_DATA on stack
      {
      n = findhci(IN_DATA,devicen,INS_NOPOP); // no mark for pop until read all
      if(n >= 0)
        {   // data on instack - may be on first entry
        len = instack[n+1] + (instack[n+2] << 8) - 2;  // length of data
        dat = insdat+n+2;        // start of data
        if(devicen == 0)
          {
          ndev = instack[n+3];  // from device 
          
          if( (devok(ndev) != 0) &&
            (  ( ((*ndevice & FROM_MESH)    != 0) && (dev[ndev]->type == BTYPE_ME)        ) ||
              ( ((*ndevice & FROM_CLCON)   != 0) && (dev[ndev]->conflag & CON_RF ) != 0  ) ||
              ( ((*ndevice & FROM_MESHCON) != 0) && (dev[ndev]->conflag & CON_MESH ) != 0) ) )
            {  // read from this device
            devicen = ndev;
            *ndevice = devicen;      // return found device
            }
          }
          
        k = instack[n+4] + (instack[n+5] << 8);  // bookmark

        if(devok(devicen) == 0)
          VPRINT "GOT data from unknown device\n");
        else
          {                
          VPRINT "GOT data from %s\n",dev[devicen]->name);   
          }
                   
        getout = 0;
        do    // while still something on instack
          {
          inbuff[gotn] = dat[k];
          lastchar = dat[k];
          ++k;
          
              // update bookmark
          instack[n+4] = k & 0xFF;
          instack[n+5] = (k >> 8) & 0xFF;
          
          ++gotn;
          inbuff[gotn] = 0;  // terminate zero
              
          if(k == len) 
            {     // have read to end of data
            instack[n] = INS_POP;  // mark for pop
            getout = 1;  // exit read this IN_DATA
            }
         
          if((flag & EXIT_COUNT) == 0 && gotn >= count-1)
            {  // count is buffer size
            NPRINT "Read exceeded buffer size\n");
            popins();
            return(gotn);   // run out of memory
            }

          if( meshflag == 0 && 
             (  ( (flag & EXIT_CHAR) != 0 && lastchar == endchar && *ecp != PACKET_ENDCHAR ) ||
                ( (flag & EXIT_COUNT) != 0 && gotn == count) ) )
            {
            popins();
            gpar.readerror = 0;  // OK
            return(gotn);  // found endchar or got count or got mesh packet - normal exit - done OK
            }

          } 
        while(getout == 0);  // loop for next char

        if(meshflag != 0)
          {  // got one IN_DATA mesh packet
          popins();
          gpar.readerror = 0;  // OK
          return(gotn);
          }
        else if(gotn > 0 && (flag & EXIT_CHAR) != 0 && *ecp == PACKET_ENDCHAR) 
          {
          popins();
          gpar.readerror = 0;  // OK
          return(gotn);  // found endchar or got count or got mesh packet - normal exit - done OK
          }
        
        }  // end reading IN_DATA but need more         
      } 
    while(n >= 0);   // may be another IN_DATA
   
    // need more - must read a new IN_DATA
      
    // fail returns
       
    if(devicen != 0 && dev[devicen]->conflag == 0) 
      {
      VPRINT "READ disconnect exit\n"); 
      gpar.readerror = ERROR_DISCONNECT;
      return(gotn);
      }


    if( (flag & EXIT_TIMEOUT) != 0)
      {
      if(time_ms() - timstart > (unsigned long long)timeoutms)
        {
        if(gpar.serveractive != 2)
          VPRINT "Serial read time out\n");
        gpar.readerror = ERROR_TIMEOUT;
        return(gotn);
        }
      }
       
    if( (flag & EXIT_KEY) != 0)
      {
      key = readkey();
      if(key == 'x')
        {
        VPRINT "Serial read aborted by key press\n");
        gpar.readerror = ERROR_KEY;
        return(gotn);
        }
      }
    
    // look for a new IN_DATA
    
    readhci(devicen,IN_DATA,0,25,0);   

    if(gpar.serveractive == 2 && findhci(IN_LECMD,0,INS_NOPOP) >= 0)
      {  // universal server LE input
      popins();
      gpar.readerror = 0;  // OK
      return(gotn);  
      }

    flushprint();
              
    }  // loop to read IN_DATA or another readhci
  while(1);   // keep going 
  
  return(0);
  }
       
       
 
  
void set_le_random_address(unsigned char *add)
  {  
  int n,flag,zflag;

  if(check_init(0) == 0)
    return;

  if((gpar.meshflag & MESH_W) != 0)
    flag = 1;
  else
    flag = 0;
        
  zflag = 0;
  for(n = 0 ; n < 6 && zflag == 0 ; ++n)
    zflag |= add[n];
  if(zflag == 0 && (gpar.hidflag & 1) == 0 && (gpar.hidflag & 2) != 0)
    {  // turn off
    VPRINT "LE random address off\n");
    if(flag != 0)
      mesh_off();
    gpar.hidflag = 0;
    dev[0]->leaddtype = 0; 
    addname(0);
    sendhci(leadparam,0);
    sendhci(leadvert,0);
    if(flag != 0)
      mesh_on();
    }
  else if(zflag != 0)
    { 
    if(flag != 0)
      mesh_off();   
    for(n = 0 ; n < 6 ; ++n)
      gpar.randbadd[n] = add[n];  
    gpar.randbadd[0] |= 0xC0;

    for(n = 0 ; n < 6 ; ++n)
      lerandadd[13-n] = gpar.randbadd[n];
      
    VPRINT "Set LE random address\n");
    sendhci(lerandadd,0);

    gpar.hidflag |= 2;
    dev[0]->leaddtype = 1;
    sendhci(leadparamx,0);

    if((gpar.hidflag & 1) == 0)
      {
      addname(1);
      sendhci(leadvertx,0);
      }
    else
      sendhci(hidadvert,0); 

    if(flag != 0)
      mesh_on();
    }
   
  }

    
  
void set_flags(int flags,int onoff)
  {
  if(check_init(0) == 0)
    return; 
  if(onoff == FLAG_OFF)
    gpar.settings &= ~flags;
  else
    gpar.settings |= flags;
  } 
  
void set_notify_node(int node)
  {
  if(check_init(0) == 0)
    return;  
  gpar.notifynode = node;
  }   
  
int set_le_wait(int waitms)
  {
  if(check_init(0) == 0)
    return(0); 
  if(waitms >= 0)
    gpar.leclientwait = waitms;
  return(gpar.leclientwait); 
  }

int set_print_flag(int flag)
  {
  int oldflag,flagx;
  
  oldflag = gpar.printflag;
  
  flagx = flag & 7;
  if(!(flagx == PRINT_NONE || flagx == PRINT_NORMAL || flagx == PRINT_VERBOSE))
    return(oldflag);     
    
  gpar.printflag = flagx;
  gpar.screenoff = flag & 8;
  
  if(gpar.printflag == PRINT_VERBOSE)
    {   // verbose - include VPRINTs  
    gpar.prtv = &gpar.prtp;
    gpar.prtvn = &gpar.prtp;
    }
  else 
    {   // dump VPRINTs
    gpar.prtv = &gpar.dump;
    gpar.prtvn = &gpar.dumpn;
    }
  return(oldflag);
         
  }

int output_file(char *filename)
  {
  FILE *prstream;

  if(check_init(0) == 0)
    return(0);

  if(checkfilename("output_file",filename) == 0)
    return(0);
  
  prstream = fopen(filename,"wt");
  if(prstream == NULL)
    {
    NPRINT "Failed to open %s file\n",filename);
    flushprint();
    return(0);
    }
        
   // entire buffer from prts to prtp 
  
  if(gpar.prtp >= gpar.prts)  // does not wrap  
    fwrite(gpar.s+gpar.prts,1,gpar.prtp - gpar.prts,prstream);  
  else if(gpar.prtw >= gpar.prts)
    {
    fwrite(gpar.s+gpar.prts,1,gpar.prtw - gpar.prts,prstream);
    fwrite(gpar.s,1,gpar.prtp,prstream);
    }
    
  NPRINT "All screen output saved to %s file\n",filename);
  fclose(prstream);
  flushprint();
  return(1);
  }

  
void flushprint()
  {
  int n;

  if(gpar.prtp == gpar.prtp0)
    return;   // nothing added since last flush
 
  gpar.s[gpar.prtp] = 0;
      
 // printf("%d %d %d\n",gpar.printflag,gpar.prtp,gpar.prtp0);
    
  n = 0;  // page size
  if(gpar.printflag != PRINT_NONE)
    {      
    if(gpar.prtp > gpar.prtp0)  // does not wrap  
      {
      if(gpar.screenoff == 0)
        printn(gpar.s+gpar.prtp0,gpar.prtp - gpar.prtp0);
      n = gpar.prtp - gpar.prtp0;
      }
    else if(gpar.prtw > gpar.prtp0) // should be anyway 
      {
      if(gpar.screenoff == 0)
        printn(gpar.s+gpar.prtp0,gpar.prtw - gpar.prtp0);
      n = gpar.prtw - gpar.prtp0;
      if(gpar.screenoff == 0)
        printn(gpar.s,gpar.prtp);
      n += gpar.prtp;
      }
    }
    
  if(n > gpar.maxpage)
    gpar.maxpage = n;
     
  gpar.prtp0 = gpar.prtp;
  gpar.prte = gpar.prtp;
   
  if(gpar.prtp > PRBUFSZ-PRPAGESZ || (gpar.prtw != 0 && gpar.prtp >= gpar.prtw))
    {  // gone past limit or existing wrap - wrap to start of buffer - leave prte
    gpar.prtw = gpar.prtp;  // wrap
    gpar.prtp = 0;
    gpar.prtp0 = 0;
    gpar.s[0] = 0;
    }

  if(gpar.prtw == 0)
    return;    // not wrapped - prts still 0

  // move start of circular buffer to next line after prtp

  gpar.prts = gpar.prtp + 1;
  if(gpar.prts >= gpar.prtw)
    gpar.prts = 0;
          
  while(gpar.s[gpar.prts] != 10)
    {
    ++gpar.prts;
    if(gpar.prts == gpar.prtw)
      gpar.prts = 0;
    }
    
  ++gpar.prts;   // one past line end    
  if(gpar.prts == gpar.prtw)
    gpar.prts = 0;
  }

int strcomp(char *s,char *t,int len)
  {
  int n;
  char sc;
  
  for(n = 0 ; n < len ; ++n)
    {
    sc = s[n];
    if(sc >= 'a' && sc <= 'z')
      sc -= 32;
    if(sc != t[n])
      return(1);
    }   
  return(0);
  }  

/********* FIND STRING in STRING ******
find t (upper case) =  in s (case insensitive)
return index of match
  low word = s index of text following = 
  hi  word = s index of t
0 = not found

Example:
  t = "RAT"
  s = "abcrat = dog"
  returns  lo=9 (index of d)  hi=3 (index of r)
***********************************/

unsigned int strinstr(char *s,char *t)
  {
  unsigned int n,k,tlen,flag;
  char c;
  
  tlen = strlen(t);
  n = 0;
  do
    {
    flag = 0;
    while(s[n] != 0 && flag == 0)
      {
      c = s[n];
      if(c >= 'a' && c <= 'z')
        c &= 0xDF;  // to upper case   
      if(c == t[0])
        flag = 1;
      else
        ++n;
      }
      
    if(s[n] == 0)
      return(0);
    // found first char match at s[n]
    flag = 0;
    k = 1;
    while(k < tlen && flag == 0)
      {
      c = s[n+k];
      if(c >= 'a' && c <= 'z')
        c &= 0xDF;  // to upper case   
      if(c != t[k])
        flag = 1;
      else
        ++k;
      }
    
    if(flag == 0)  // found t match
      {
      // look for =
      while(s[n+k] != 0 && s[n+k] == ' ')
        ++k;
      if(s[n+k] == '=')
        {
        ++k;
        // skip spaces
        while(s[n+k] != 0 && s[n+k] == ' ')
          ++k;
        if(s[n+k] != 0)
          return(n+k + (n << 16));
        }
      }
    ++n;
    }
  while(1);  
  }


/********* CONVERT ASCII to HEX ARRAY ******
Input    s = ascii string
      slen = length of s  (e.g. strlen(s) if zero terminated) 
       val = array to receive converted bytes
       len = length of val[] array 

Valid formats for s

112233AABB       val[0]=0x11 val[1]=0x22 etc. return=5
12233            val[0]=0x01 val[1]=0x22 val[3]=0x33
0000-1000-0100-ABCDEF   spacers ignored but there must
                        be an even number of characters
                        (2 chars=1 byte) between spacers
11:22:AA:BB
11 22 AA BB
1 22 AA B 0 
1,2,3,A,de,4f

return number of values in val[] array

*************************/

unsigned char *strtohex(char *s,int *num)
  {
  return(strtohexx(s,strlen(s),num));
  }

unsigned char *strtohexx(char *s,int slen,int *num)
  {
  int n,j,j0,k,vn,flag,count,xcount,hxcount,errflag;
  unsigned char *val;
  static int del[4] = {0,'0','a'-10,'A'-10};
  static unsigned char val0[64];
  static unsigned char val1[64];
  static unsigned char val2[64];
  static unsigned char val3[64];
  static unsigned char *valx[4] = { val0,val1,val2,val3 };
  static int valn = 0;

  val = valx[valn];
  valn = (valn + 1) & 3;
  vn = 0;   // number of values in val[]
      
    // find first non-hex or non-white char
  k = 0;
  while(hexchar(s[k]) >= 0)
    ++k;   
   
  // s[k] is term char
      
  if(slen < k)
    k = slen;
    
  while( k > 0 && s[k-1] == ' ')
    --k;

  if(k == 0)
    {
    if(num != NULL)
      *num = vn;
    return(val);     
    }
    
  for(n = 0 ; n < 64 ; ++n)
    val[n] = 0;
   
  xcount = 0;    // separator chars inside hex chars
  count = 0;     // hex char count
  hxcount = 0;   // hex chars between separators
  errflag = 0;
  
  for(n = 0 ; n < k && errflag == 0 ; ++n)
    {
    j = hexchar(s[n]);
    if(j > 0)
      {
      ++count;
      ++hxcount;
      }
    else if(count != 0 && j == 0)
      ++xcount;
    
    if(j == 0)  // separator
      {   // odd number precedes separator
      if((hxcount % 2) != 0)
        errflag = 1;
      hxcount = 0;
      }      
    }

  if(xcount != 0 && (hxcount % 2) != 0)
    errflag = 1;   // odd number with separators
    
  if(errflag != 0)
    {
    NPRINT "Hex format error - odd number of digits\n");
    flushprint();
    if(num != NULL)
      *num = vn;
    return(val);
    }
     
  if( xcount == 0 && (count % 2) != 0)
    j0 = 1;  // no white and odd number of hex chars so first hex = one char
  else      
    j0 = 2;  

    
  n = 0;
  do
    {
    while(n < k && hexchar(s[n]) == 0)
      ++n;
  
    if(n >= k || vn == 64)
      {
      if(num != NULL)
        *num = vn;
      return(val);  // normal exit
      }
         
    val[vn] = 0;
    for(j = 0 ; j < j0 ; ++j)
      {
      flag = hexchar(s[n]);
      if(flag > 0)
        {    
        val[vn] = (val[vn] << 4) + s[n] - del[flag];     
        ++n;
        }
      }
      
    j0 = 2;   // 2-char beyond first hex
    if(vn < 64)
      ++vn;
    }
  while(1);    
  }




int hexchar(char c)
  {
  if(c == 0)
    return(-2);
  if(c >= '0' && c <= '9')
    return(1);
  if(c >= 'a' && c <= 'f')
    return(2);
  if(c >= 'A' && c <= 'F')
    return(3);
  if(c == ' ' || c == ':' || c == ',' || c == '-')
    return(0);
  return(-1);
  }

/******* READ LINE **********
strip lead spaces trail space cr/lf
strip trail cr/lf
skip over empty lines
; = comment
return
2 = end of file with good string
1 = good string
0 = end of file - no string
**************************/

int readline(FILE *stream,char *devs,char *s)
  {
  static int devn = 0;
  int n,c;
  
  n = 0;
  s[0] = 0;
  do
    {
    if(stream != NULL)    
      c = fgetc(stream);
    else
      {
      c = devs[devn];
      if(c == 0)
        c = EOF;
      ++devn;
      }
    if(c == ';')
      {
      do
        {
        if(stream != NULL)
          c = fgetc(stream);
        else
          {
          c = devs[devn];
          if(c == 0)
            c = EOF;
          ++devn;
          }
        }
      while(c != 10 && c != EOF);
      }
    if(c == EOF)
      {
      if(n == 0)
        return(0);
      while(s[n-1] == ' ' && n > 0)
        {
        s[n-1] = 0;
        --n;
        }
      return(2);
      }
    if(c == 10)
      {
      if(n != 0)
        {
        while(s[n-1] == ' ' && n > 0)
          {
          s[n-1] = 0;
          --n;
          }
        return(1);
        }
      // else n=0 empty line - loop for next
      }
    else if(c != 13)
      {
      if( !(n==0 && c==' ') )
        {
        s[n] = c;
        ++n;
        s[n] = 0;
        }    
      }
    }
  while(n < 254);

  return(0);
  }

void le_handles(int node,int lasthandle)
  {
  if(check_init(0) == 0)
    return;
  readleatt(node,lasthandle | 0x40000);
  }

void readleatt(int node,int xhandle)
  {
  int n,k,len,uuidtype,ndevice,h0,hx,handle,psflag,allflag,uuid,endff,bdlen,delh,lastflag,htype;
  char s[64],sx[8],*sp;
  unsigned char *bd,*bdat;
  
  static unsigned char opx2[32] = {16,0,S2_HAND,0,2,0x40,0,11,0,7,0,4,0,0x10,0x01,0,0x01,0x00,0x00,0x2A};     
  static unsigned char opx16[40] = {30,0,S2_HAND,0,2,0x40,0,25,0,21,0,4,0,0x10,0x01,0,0x01,0x00,
                                        0x00,0xFF,0xEE,0xDD,0xCC,0xBB,0xAA,0x99,0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11};   
  static unsigned char op4[32] = {14,0,S2_HAND,0,2,0x40,0,9,0,5,0,4,0,0x04,0x01,0,0x01,0x00};     
  static unsigned char op6[64] = {18,0,S2_HAND,0,2,0x40,0,0x0D,0,0x09,0,4,0,0x06,0x01,0,0x01,0x00,0x00,0x28,0x01,0x18};     
  static unsigned char opa[20] = {12,0,S2_HAND,0,2,0x40,0,7,0,3,0,4,0,0x0A,0x12,0};  
  //FILE *stream;  
  
  bdat = gpar.buf;
  ndevice = devn(node);
  if(ndevice <= 0 || (dev[ndevice]->conflag & (CON_LE | CON_MESH)) == 0)
    {
    NPRINT "Not an LE connected device\n");
    flushprint();
    return;
    }
     
  // set_le_interval_update(node,6,6);

  bd = NULL;
  read_all_clear();
  
  allflag = 0;
  endff = 0;
  hx = xhandle & 0xFFFF;
  h0 = hx;
  delh = 0;
  len = 0;
  
  
  if((xhandle & 0x40000) != 0)
    {
    if(hx == 0)
      {
      hx = 0x100;
      NPRINT "Searching to handle=255 only\n");
      }
    lastflag = 0;
    // vhand = 0;
    NPRINT "Handle    %s\n",dev[ndevice]->name);
    for(handle = 1 ; handle <= hx ; ++handle)
      { 
      uuidtype = 0;
      psflag = 0;
      htype = 0;
      op4[14] = handle & 0xFF;
      op4[15] = (handle >> 8) & 0xFF;
      op4[16] = handle & 0xFF;
      op4[17] = (handle >> 8) & 0xFF;
      sendhci(op4,ndevice);  
      readhci(ndevice,IN_ATTDAT,0,2000,0);      
      n = findhci(IN_ATTDAT,ndevice,INS_POP);
      if(lastflag != 0 && h0 == 0 && ((n >= 0 && insdat[n] == 0x01) || n < 0))
        {  
        flushprint();
        popins();
        return;
        }
      if(n >= 0 && insdat[n] == 0x05) 
        { 
        htype = 3;             
        if(insdat[n+1] == 1)
          {
          uuidtype = 2;
          if(insdat[n+5] == 0x28 && insdat[n+4] == 0x00)
            htype = 1;
          else if(insdat[n+5] == 0x28 && insdat[n+4] == 0x03)
            htype = 2;
          }
        else if(insdat[n+1] == 2)
          uuidtype = 16;
        NPRINT "%04X UUID ",handle);
        if(uuidtype == 2)
          {
          NPRINT "%02X%02X",insdat[n+5],insdat[n+4]);
          if(htype == 1)
            {
            NPRINT " Primary service\n");
            psflag = 1;
            }
          else if(htype == 2)
            NPRINT " Characteristic info\n");
          else if(insdat[n+5] == 0x29 && insdat[n+4] == 0x02)
            NPRINT " Notify enable\n");
          else
            NPRINT "\n");
          }  
        else if(uuidtype == 16)
          {
          for(k = 0 ; k < 16 ; ++k)
            NPRINT "%02X",insdat[n+19-k]);
          NPRINT "\n");
          }
        }
      flushprint();
      popins();

      if(htype == 1)
        {
        opx2[14] = handle & 0xFF;
        opx2[15] = (handle >> 8) & 0xFF; 
        opx2[16] = handle & 0xFF;
        opx2[17] = (handle >> 8) & 0xFF; 
        opx2[18] = insdat[n+4];
        opx2[19] = insdat[n+5];
        opx2[13] = 0x10;
        sendhci(opx2,ndevice);
        readhci(ndevice,IN_ATTDAT,0,2000,0);      
        n = findhci(IN_ATTDAT,ndevice,INS_POP);          
        if(n >= 0 && insdat[n] == 0x11)
          {
          len = instack[n+1]+(instack[n+2] << 8);
          NPRINT "     Value ");
          for(k = len-1 ; k >= 6 ; --k)
            NPRINT "%02X",insdat[n+k]);
          NPRINT "\n");
          NPRINT "     Last handle in service %02X%02X\n",insdat[n+5],insdat[n+4]);
          if(insdat[n+4] == 0xFF && insdat[n+5] == 0xFF)
            lastflag = 1;      
          }
        }   
      else if(htype != 0)
        {  
        opa[14] = handle & 0xFF;
        opa[15] = (handle >> 8) & 0xFF;
        sendhci(opa,ndevice);
        readhci(ndevice,IN_ATTDAT,0,2000,0);      
        n = findhci(IN_ATTDAT,ndevice,INS_POP);
        if(n >= 0 && insdat[n] == 0x0B)
          {
          if(htype == 2)
            {
            NPRINT "     Permit %02X  Value handle %02X%02X\n",insdat[n+1],insdat[n+3],insdat[n+2]);
            // vhand = insdat[n+2] + (insdat[n+3] << 8);
            }
          else if(htype == 3)
            {
            len = instack[n+1]+(instack[n+2] << 8);
            NPRINT "     Value");
            if(len <= 1)
              NPRINT " not set\n");
            else
              {
              for(k = 1 ; k < len ; ++k)
                {
                if(k > 1 && ((k-1) & 0x0F) == 0)
                  NPRINT "\n          ");
                NPRINT " %02X",insdat[n+k]);
                }
              NPRINT "\n");
              }
            // vhand = 0;
            }
          }
        else
          NPRINT "     Value not readable\n");
        }
      flushprint();
      popins();
    
      }
    return;
    }
  
   
  
  
  if((xhandle & 0x10000) != 0)
    {
    endff = 1;
    if(h0 > 1)
      delh = 1;
    }     
  else if((xhandle & 0x20000) != 0)
    {
    allflag = 1;
    h0 = 1;
    //stream = fopen("hidvals.txt","wb"); 
    }
    
  for(handle = h0 ; handle <= hx ; ++handle)
    { 
    psflag = 0;
    bd = NULL;
    bdlen = 0;
    
    opa[14] = handle;
    sendhci(opa,ndevice);
    readhci(ndevice,IN_ATTDAT,0,2000,0);      
    n = findhci(IN_ATTDAT,ndevice,INS_POP);
    if(n >= 0 && insdat[n] == 0x0B)
      {
      len = instack[n+1]+(instack[n+2] << 8);
      if(allflag == 0)
        {
        NPRINT "%02X size %d\n",insdat[n],len);
        printval(insdat+n+1,len-1,NULL);
        }
      for(k = 0 ; k < len && k < 256 ; ++k)
        bdat[k] = insdat[n+k];
      bd = bdat+1;
      bdlen = len-1;
      }    
 
    op4[14] = handle;
    op4[15] = 0;
    if(endff == 0)
      {
      op4[16] = handle;
      op4[17] = 0;
      }
    else
      {
      op4[16] = 0xFF;
      op4[17] = 0xFF;
      }
    sendhci(op4,ndevice);  
    readhci(ndevice,IN_ATTDAT,0,2000,0);      
    n = findhci(IN_ATTDAT,ndevice,INS_POP);
    if(n >= 0 && insdat[n] == 0x05)  // 1
      {   
      uuidtype = 0;
      if(insdat[n+1] == 1)
        uuidtype = 2;
      else if(insdat[n+1] == 2)
        uuidtype = 16;
      if(uuidtype != 0)  // 2
        {    
        if(allflag == 0)
          {
          if(uuidtype == 2)
            sprintf(s,"%02X%02X",insdat[n+5],insdat[n+4]);  
          else
            {
            s[0] = 0;
            for(k = 0 ; k < 16 ; ++k)
              {
              sprintf(sx,"%02X",insdat[n+19-k]);
              strcat(s,sx);
              }
            }
    
          NPRINT "%02X H=%02X%02X UUID=%s\n",insdat[n],insdat[n+3],insdat[n+2],s);
          }
        else
          {
          if(uuidtype == 2)
            {
            uuid = insdat[n+4] + (insdat[n+5] << 8);
            if(uuid == 0x2800)
              {
              sp = "PS ";
              psflag = 1;
              }
            else if(uuid == 0x2803)
              sp = "I ";
            else if(uuid == 0x2902)
              sp = "N ";
            else
              {
              sp = "V ";
              /***
              if(stream != NULL)
                {
                fprintf(stream,"val%d[%d] = {",handle,bdlen);
                for(k = 0 ; k < bdlen ; ++k)
                  fprintf(stream,"0x%02X,",bd[k]);
                fprintf(stream,"}\n");
                }  
              ***/  
              }
              
            NPRINT "%d %s %04X %s\n",handle,sp,uuid,uuidlist+finduuidtext(uuid));
            if(psflag != 0 && bd != NULL)
              {
              NPRINT "   ");
              for(k = len-2 ; k >= 0 ; --k)
                NPRINT "%02X",bd[k]);
              NPRINT "\n");
              psflag = 0;
              }
            }
          else
            {
            NPRINT "%d V  16-byte\n",handle);
            }
          }    
    
        if(uuidtype == 2)
          {
          op6[14] = handle;
          op6[15] = 0;
          if(endff == 0)
            {
            op6[16] = handle;
            op6[17] = 0;
            }
          else
            {
            op6[16] = 0xFF;
            op6[17] = 0xFF;
            }

          op6[18] = insdat[n+4];
          op6[19] = insdat[n+5];


          opx2[14] = handle - delh;
          opx2[15] = 0;
          if(endff == 0)
            {
            opx2[16] = handle;
            opx2[17] = 0;
            }
          else
            {
            opx2[16] = 0xFF;
            opx2[17] = 0xFF;
            }
       
          opx2[18] = insdat[n+4];
          opx2[19] = insdat[n+5];
   
          if(allflag == 0) 
            NPRINT "Specify %02X %02X\n",op6[18],op6[19]); 
    
          opx2[13] = 0x08;
          sendhci(opx2,ndevice);
          readhci(ndevice,IN_ATTDAT,0,2000,0);      
          n = findhci(IN_ATTDAT,ndevice,INS_POP);
          if(allflag == 0 || psflag != 0) 
            {
            if(n >= 0 && insdat[n] == 0x09)
              {
              if(allflag == 0)
                NPRINT "%02X H=%02X%02X\n",insdat[n],insdat[n+3],insdat[n+2]);
              printval(insdat+n+4,insdat[n+1]-2,bd);
              }
            }
           
          opx2[13] = 0x10;
          sendhci(opx2,ndevice);
          readhci(ndevice,IN_ATTDAT,0,2000,0);      
          n = findhci(IN_ATTDAT,ndevice,INS_POP);
          if(allflag == 0) 
            {
            if(n >= 0 && insdat[n] == 0x11)
              {
              NPRINT "%02X H=%02X%02X EOG=%02X%02X\n",insdat[n],insdat[n+3],insdat[n+2],insdat[n+5],insdat[n+4]);
              printval(insdat+n+6,insdat[n+1]-4,bd);
              }
            }
      
          //len = insdat[n+1]-4; // len of value
          if(bdlen == 0)
            NPRINT "No value for 06\n");
          else if(bdlen > 16)
            NPRINT "Val too long for 06\n");
          else
            {
            if(allflag == 0) 
              {
              NPRINT "Specify %02X %02X -",op6[18],op6[19]); 
              for(k = 0 ; k < bdlen ; ++k)
                {
                op6[20+k] = bd[k];
                NPRINT " %02X",bd[k]);
                } 
              NPRINT "\n");
              }
            op6[0] = 16+bdlen;
            op6[7] = 11+bdlen;
            op6[9] = 7+bdlen;
            sendhci(op6,ndevice);
            readhci(ndevice,IN_ATTDAT,0,2000,0);      
            n = findhci(IN_ATTDAT,ndevice,INS_POP);
            if(allflag == 0) 
              {
              if(n >= 0 && insdat[n] == 0x07)
                {
                NPRINT "%02X H=%02X%02X EOG=%02X%02X\n",insdat[n],insdat[n+2],insdat[n+1],insdat[n+4],insdat[n+3]);
                // printval(insdat+n+6,insdat[n+1]-4,bd);
                }
              }
            }
          }
        else if(uuidtype == 16)
          { 
          opx16[14] = handle - delh;
          opx16[15] = 0;
          if(endff == 0)
            {
            opx16[16] = handle;
            opx16[17] = 0;
            }
          else
            {
            opx16[16] = 0xFF;
            opx16[17] = 0xFF;
            }

          if(allflag == 0) 
            {
            NPRINT "Specify ");
            for(k = 0 ; k < 16 ; ++k)
              {
              opx16[k+18] = insdat[n+k+4];
              NPRINT " %02X",opx16[k+18]);
              }
            NPRINT "\n");
            }
          opx16[13] = 0x08;
          sendhci(opx16,ndevice);
          readhci(ndevice,IN_ATTDAT,0,2000,0);      
          n = findhci(IN_ATTDAT,ndevice,INS_POP);
          if(allflag == 0) 
            {
            if(n >= 0 && insdat[n] == 0x09)
              {
              NPRINT "%02X H=%02X%02X\n",insdat[n],insdat[n+3],insdat[n+2]);
              printval(insdat+n+4,insdat[n+1]-2,bd);
              }
            }
  
          opx16[13] = 0x10;
          sendhci(opx16,ndevice);
          readhci(ndevice,IN_ATTDAT,0,2000,0);      
          n = findhci(IN_ATTDAT,ndevice,INS_POP);
          if(allflag == 0) 
            {
            if(n >= 0 && insdat[n] == 0x11)
              {
              NPRINT "%02X H=%02X%02X EOG=%02X%02X\n",insdat[n],insdat[n+3],insdat[n+2],insdat[n+5],insdat[n+4]);
              printval(insdat+n+6,insdat[n+1]-4,bd);
              }
            }
          }
  
        flushprint();    
        popins();
        }  // 2
      }  // 1
    }  // handle

  /***
  if(stream != NULL) 
    fclose(stream);
  ***/
  }


void printval(unsigned char *s,int len,unsigned char *t)
  {
  int n,flag;
  
  NPRINT "   V =");
  for(n = 0 ; n < len && n < 20 ; ++n)
    NPRINT " %02X",s[n]);
  if(len > 20)
    NPRINT "...\n");
  else
    NPRINT "\n");

  if(t == NULL)
    return;
    
  flag = 0;
  for(n = 0 ; n < len && flag == 0 && n < 256 ; ++n)
    {
    if(s[n] != t[n])
      flag = 1;
    }
  if(flag == 0)
    NPRINT "   V match OK\n");
  else
    NPRINT "   V match FAIL\n");


  flushprint();
  }



int calcs1(unsigned char *key,unsigned char *r1,unsigned char *r2,unsigned char *out)
  {
  int n;
  unsigned char res[16];
  
  for(n = 0 ; n < 8 ; ++n)
    res[n] = r2[n];
  for(n = 0 ; n < 8 ; ++n)
    res[n+8] = r1[n];
    
  if(calce(key,res,out,1) == 0)
    return(0);
    
  return(1);
  }


int calcc1(unsigned char *key,unsigned char *r,unsigned char *preq,unsigned char *pres,unsigned char iat,unsigned char rat,
            unsigned char *ia,unsigned char *ra,unsigned char *res)
  {
  // key[16] r[16] preq[7] pres[7] ia[6] ra[6] res[16]
  int n;
  static unsigned char p1[16],p2[16],resa[16],resb[16];
  
  p1[0] = iat;
  p1[1] = rat;
  for(n = 0 ; n < 7 ; ++n)
    p1[n+2] = preq[n];
  for(n = 0 ; n < 7 ; ++n)
    p1[n+9] = pres[n];
    
  for(n = 0 ; n < 6 ; ++n)
    p2[n] = ra[n];
  for(n = 0 ; n < 6 ; ++n)
    p2[n+6] = ia[n];
  for(n = 0 ; n < 4 ; ++n)
    p2[n+12] = 0;
    
  for(n = 0 ; n < 16 ; ++n)
    resa[n] = r[n] ^ p1[n];
 
  if(calce(key,resa,resb,1) == 0)
    return(0);  
  
  for(n = 0 ; n < 16 ; ++n)
    resa[n] = resb[n] ^ p2[n];
  
  if(calce(key,resa,res,1) == 0)
    return(0);
  
  return(1);
  }

  
  


// u[32] v[32] key[16] res[16]
int calcf4(unsigned char *u, unsigned char *v,
           unsigned char *key, unsigned char z, unsigned char *res)
  {
  int n;
  static unsigned char msg[65];
 
  msg[0] = z;
  for(n = 0 ; n < 32 ; ++n)
    {
    msg[n+1] = v[n];
    msg[n+33] = u[n];
    }

  return(aes_cmac(key,msg,65,res));
  }



// w[32] n1[16] n2[16] a1[7] a2[7] mackey[16] ltk[16]
int calcf5(unsigned char *w, unsigned char *n1,
           unsigned char *n2, unsigned char *a1, unsigned char *a2,
           unsigned char *mackey, unsigned char *ltk)
  {
  static unsigned char btle[4] = { 0x65, 0x6c, 0x74, 0x62 };
  static unsigned char salt[16] = { 0xbe, 0x83, 0x60, 0x5a, 0xdb, 0x0b, 0x37, 0x60,
                                    0x38, 0xa5, 0xf5, 0xaa, 0x91, 0x83, 0x88, 0x6c };
  static unsigned char length[2] = { 0x00, 0x01 };
  static unsigned char m[53], t[16];

  if(aes_cmac(salt, w, 32, t) == 0)
    return(0);

  memcpy(&m[0], length, 2);
  memcpy(&m[2], a2, 7);
  memcpy(&m[9], a1, 7);
  memcpy(&m[16], n2, 16);
  memcpy(&m[32], n1, 16);
  memcpy(&m[48], btle, 4);

  m[52] = 0; // Counter 
  if(aes_cmac(t, m, sizeof(m), mackey) == 0)
    return(0);
  m[52] = 1; // Counter 
  return(aes_cmac(t, m, sizeof(m), ltk));
  }


// w[16] n1[16] n2[16] r[16] io_cap[3] a1[7] a2[7] res[16]
int calcf6(unsigned char* w, unsigned char* n1,
           unsigned char* n2, unsigned char* r, unsigned char* io_cap,
           unsigned char* a1, unsigned char* a2, unsigned char* res)
  {
  static unsigned char m[65];

  memcpy(&m[0], a2, 7);
  memcpy(&m[7], a1, 7);
  memcpy(&m[14], io_cap, 3);
  memcpy(&m[17], r, 16);
  memcpy(&m[33], n2, 16);
  memcpy(&m[49], n1, 16);

  return aes_cmac(w, m, sizeof(m), res);
  }

// u[32] v[32] x[16] y[16]
int calcg2(unsigned char* u, unsigned char* v,
           unsigned char* x, unsigned char* y)
  {
  static unsigned char m[80],tmp[16];
  unsigned int retval;
  
  memcpy(&m[0], y, 16);
  memcpy(&m[16], v, 32);
  memcpy(&m[48], u, 32);

  if(aes_cmac(x, m, sizeof(m), tmp) == 0)  
    return(0);

  retval= *((unsigned long *)tmp);
  retval %= 1000000;

  return((int)retval);
  }



int calce(unsigned char *key,unsigned char *in,unsigned char *out,int flipflag)
  {
static unsigned char sbox[256] = {
0x63,0x7C,0x77,0x7B,0xF2,0x6B,0x6F,0xC5,0x30,0x01,0x67,0x2B,0xFE,0xD7,0xAB,0x76,
0xCA,0x82,0xC9,0x7D,0xFA,0x59,0x47,0xF0,0xAD,0xD4,0xA2,0xAF,0x9C,0xA4,0x72,0xC0,
0xB7,0xFD,0x93,0x26,0x36,0x3F,0xF7,0xCC,0x34,0xA5,0xE5,0xF1,0x71,0xD8,0x31,0x15,
0x04,0xC7,0x23,0xC3,0x18,0x96,0x05,0x9A,0x07,0x12,0x80,0xE2,0xEB,0x27,0xB2,0x75,
0x09,0x83,0x2C,0x1A,0x1B,0x6E,0x5A,0xA0,0x52,0x3B,0xD6,0xB3,0x29,0xE3,0x2F,0x84,
0x53,0xD1,0x00,0xED,0x20,0xFC,0xB1,0x5B,0x6A,0xCB,0xBE,0x39,0x4A,0x4C,0x58,0xCF,
0xD0,0xEF,0xAA,0xFB,0x43,0x4D,0x33,0x85,0x45,0xF9,0x02,0x7F,0x50,0x3C,0x9F,0xA8,
0x51,0xA3,0x40,0x8F,0x92,0x9D,0x38,0xF5,0xBC,0xB6,0xDA,0x21,0x10,0xFF,0xF3,0xD2,
0xCD,0x0C,0x13,0xEC,0x5F,0x97,0x44,0x17,0xC4,0xA7,0x7E,0x3D,0x64,0x5D,0x19,0x73,
0x60,0x81,0x4F,0xDC,0x22,0x2A,0x90,0x88,0x46,0xEE,0xB8,0x14,0xDE,0x5E,0x0B,0xDB,
0xE0,0x32,0x3A,0x0A,0x49,0x06,0x24,0x5C,0xC2,0xD3,0xAC,0x62,0x91,0x95,0xE4,0x79,
0xE7,0xC8,0x37,0x6D,0x8D,0xD5,0x4E,0xA9,0x6C,0x56,0xF4,0xEA,0x65,0x7A,0xAE,0x08,
0xBA,0x78,0x25,0x2E,0x1C,0xA6,0xB4,0xC6,0xE8,0xDD,0x74,0x1F,0x4B,0xBD,0x8B,0x8A,
0x70,0x3E,0xB5,0x66,0x48,0x03,0xF6,0x0E,0x61,0x35,0x57,0xB9,0x86,0xC1,0x1D,0x9E,
0xE1,0xF8,0x98,0x11,0x69,0xD9,0x8E,0x94,0x9B,0x1E,0x87,0xE9,0xCE,0x55,0x28,0xDF,
0x8C,0xA1,0x89,0x0D,0xBF,0xE6,0x42,0x68,0x41,0x99,0x2D,0x0F,0xB0,0x54,0xBB,0x16 };
static unsigned char gfp2[256] = {
0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,0x12,0x14,0x16,0x18,0x1A,0x1C,0x1E,
0x20,0x22,0x24,0x26,0x28,0x2A,0x2C,0x2E,0x30,0x32,0x34,0x36,0x38,0x3A,0x3C,0x3E,
0x40,0x42,0x44,0x46,0x48,0x4A,0x4C,0x4E,0x50,0x52,0x54,0x56,0x58,0x5A,0x5C,0x5E,
0x60,0x62,0x64,0x66,0x68,0x6A,0x6C,0x6E,0x70,0x72,0x74,0x76,0x78,0x7A,0x7C,0x7E,
0x80,0x82,0x84,0x86,0x88,0x8A,0x8C,0x8E,0x90,0x92,0x94,0x96,0x98,0x9A,0x9C,0x9E,
0xA0,0xA2,0xA4,0xA6,0xA8,0xAA,0xAC,0xAE,0xB0,0xB2,0xB4,0xB6,0xB8,0xBA,0xBC,0xBE,
0xC0,0xC2,0xC4,0xC6,0xC8,0xCA,0xCC,0xCE,0xD0,0xD2,0xD4,0xD6,0xD8,0xDA,0xDC,0xDE,
0xE0,0xE2,0xE4,0xE6,0xE8,0xEA,0xEC,0xEE,0xF0,0xF2,0xF4,0xF6,0xF8,0xFA,0xFC,0xFE,
0x1B,0x19,0x1F,0x1D,0x13,0x11,0x17,0x15,0x0B,0x09,0x0F,0x0D,0x03,0x01,0x07,0x05,
0x3B,0x39,0x3F,0x3D,0x33,0x31,0x37,0x35,0x2B,0x29,0x2F,0x2D,0x23,0x21,0x27,0x25,
0x5B,0x59,0x5F,0x5D,0x53,0x51,0x57,0x55,0x4B,0x49,0x4F,0x4D,0x43,0x41,0x47,0x45,
0x7B,0x79,0x7F,0x7D,0x73,0x71,0x77,0x75,0x6B,0x69,0x6F,0x6D,0x63,0x61,0x67,0x65,
0x9B,0x99,0x9F,0x9D,0x93,0x91,0x97,0x95,0x8B,0x89,0x8F,0x8D,0x83,0x81,0x87,0x85,
0xBB,0xB9,0xBF,0xBD,0xB3,0xB1,0xB7,0xB5,0xAB,0xA9,0xAF,0xAD,0xA3,0xA1,0xA7,0xA5,
0xDB,0xD9,0xDF,0xDD,0xD3,0xD1,0xD7,0xD5,0xCB,0xC9,0xCF,0xCD,0xC3,0xC1,0xC7,0xC5,
0xFB,0xF9,0xFF,0xFD,0xF3,0xF1,0xF7,0xF5,0xEB,0xE9,0xEF,0xED,0xE3,0xE1,0xE7,0xE5 };
static unsigned char gfp3[256] = {
0x00,0x03,0x06,0x05,0x0C,0x0F,0x0A,0x09,0x18,0x1B,0x1E,0x1D,0x14,0x17,0x12,0x11,
0x30,0x33,0x36,0x35,0x3C,0x3F,0x3A,0x39,0x28,0x2B,0x2E,0x2D,0x24,0x27,0x22,0x21,
0x60,0x63,0x66,0x65,0x6C,0x6F,0x6A,0x69,0x78,0x7B,0x7E,0x7D,0x74,0x77,0x72,0x71,
0x50,0x53,0x56,0x55,0x5C,0x5F,0x5A,0x59,0x48,0x4B,0x4E,0x4D,0x44,0x47,0x42,0x41,
0xC0,0xC3,0xC6,0xC5,0xCC,0xCF,0xCA,0xC9,0xD8,0xDB,0xDE,0xDD,0xD4,0xD7,0xD2,0xD1,
0xF0,0xF3,0xF6,0xF5,0xFC,0xFF,0xFA,0xF9,0xE8,0xEB,0xEE,0xED,0xE4,0xE7,0xE2,0xE1,
0xA0,0xA3,0xA6,0xA5,0xAC,0xAF,0xAA,0xA9,0xB8,0xBB,0xBE,0xBD,0xB4,0xB7,0xB2,0xB1,
0x90,0x93,0x96,0x95,0x9C,0x9F,0x9A,0x99,0x88,0x8B,0x8E,0x8D,0x84,0x87,0x82,0x81,
0x9B,0x98,0x9D,0x9E,0x97,0x94,0x91,0x92,0x83,0x80,0x85,0x86,0x8F,0x8C,0x89,0x8A,
0xAB,0xA8,0xAD,0xAE,0xA7,0xA4,0xA1,0xA2,0xB3,0xB0,0xB5,0xB6,0xBF,0xBC,0xB9,0xBA,
0xFB,0xF8,0xFD,0xFE,0xF7,0xF4,0xF1,0xF2,0xE3,0xE0,0xE5,0xE6,0xEF,0xEC,0xE9,0xEA,
0xCB,0xC8,0xCD,0xCE,0xC7,0xC4,0xC1,0xC2,0xD3,0xD0,0xD5,0xD6,0xDF,0xDC,0xD9,0xDA,
0x5B,0x58,0x5D,0x5E,0x57,0x54,0x51,0x52,0x43,0x40,0x45,0x46,0x4F,0x4C,0x49,0x4A,
0x6B,0x68,0x6D,0x6E,0x67,0x64,0x61,0x62,0x73,0x70,0x75,0x76,0x7F,0x7C,0x79,0x7A,
0x3B,0x38,0x3D,0x3E,0x37,0x34,0x31,0x32,0x23,0x20,0x25,0x26,0x2F,0x2C,0x29,0x2A,
0x0B,0x08,0x0D,0x0E,0x07,0x04,0x01,0x02,0x13,0x10,0x15,0x16,0x1F,0x1C,0x19,0x1A };
static unsigned char rcon[256] = {
0x00,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36,0x6C,0xD8,0xAB,0x4D,0x9A,
0x2F,0x5E,0xBC,0x63,0xC6,0x97,0x35,0x6A,0xD4,0xB3,0x7D,0xFA,0xEF,0xC5,0x91,0x39,
0x72,0xE4,0xD3,0xBD,0x61,0xC2,0x9F,0x25,0x4A,0x94,0x33,0x66,0xCC,0x83,0x1D,0x3A,
0x74,0xE8,0xCB,0x8D,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,0x36,0x6C,0xD8,
0xAB,0x4D,0x9A,0x2F,0x5E,0xBC,0x63,0xC6,0x97,0x35,0x6A,0xD4,0xB3,0x7D,0xFA,0xEF,
0xC5,0x91,0x39,0x72,0xE4,0xD3,0xBD,0x61,0xC2,0x9F,0x25,0x4A,0x94,0x33,0x66,0xCC,
0x83,0x1D,0x3A,0x74,0xE8,0xCB,0x8D,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1B,
0x36,0x6C,0xD8,0xAB,0x4D,0x9A,0x2F,0x5E,0xBC,0x63,0xC6,0x97,0x35,0x6A,0xD4,0xB3,
0x7D,0xFA,0xEF,0xC5,0x91,0x39,0x72,0xE4,0xD3,0xBD,0x61,0xC2,0x9F,0x25,0x4A,0x94,
0x33,0x66,0xCC,0x83,0x1D,0x3A,0x74,0xE8,0xCB,0x8D,0x01,0x02,0x04,0x08,0x10,0x20,
0x40,0x80,0x1B,0x36,0x6C,0xD8,0xAB,0x4D,0x9A,0x2F,0x5E,0xBC,0x63,0xC6,0x97,0x35,
0x6A,0xD4,0xB3,0x7D,0xFA,0xEF,0xC5,0x91,0x39,0x72,0xE4,0xD3,0xBD,0x61,0xC2,0x9F,
0x25,0x4A,0x94,0x33,0x66,0xCC,0x83,0x1D,0x3A,0x74,0xE8,0xCB,0x8D,0x01,0x02,0x04,
0x08,0x10,0x20,0x40,0x80,0x1B,0x36,0x6C,0xD8,0xAB,0x4D,0x9A,0x2F,0x5E,0xBC,0x63,
0xC6,0x97,0x35,0x6A,0xD4,0xB3,0x7D,0xFA,0xEF,0xC5,0x91,0x39,0x72,0xE4,0xD3,0xBD,
0x61,0xC2,0x9F,0x25,0x4A,0x94,0x33,0x66,0xCC,0x83,0x1D,0x3A,0x74,0xE8,0xCB,0x8D };

  int n,i,j,k;
  static unsigned char cp[16],w[176],temp[4],dp[16];
  unsigned char temp0,*wp;

  if(flipflag == 0)
    {
    for(n = 0 ; n < 16 ; ++n)
      {
      cp[n] = in[n];
      w[n] = key[n];
      }
    }
  else
    {
    for(n = 0 ; n < 16 ; ++n)
      {
      cp[n] = in[15-n];
      w[n] = key[15-n];
      }
    }

  i = 4;
  while(i < 44)
    {
    k = (i-1) << 2;
    for(n = 0 ; n < 4 ; ++n)
      temp[n] = w[k+n]; 
     
    if((i % 4) == 0)
      {
      temp0 = temp[0];
      temp[0] = temp[1];
      temp[1] = temp[2];
      temp[2] = temp[3];
      temp[3] = temp0;

      for(j = 0 ; j < 4 ; ++j)
        temp[j] = sbox[temp[j]];
      temp[0] ^= rcon[i/4];
      }
     
    k = (i-4) << 2;
    for(j = 0 ; j < 4 ; ++j)
      temp[j] ^= w[k+j];

    k = (i << 2);
    for(j = 0 ; j < 4 ; ++j)
      w[k+j] = temp[j];

    ++i;
    }

  for(n = 0 ; n < 16 ; ++n)
    cp[n] ^= w[n];   

  for(k = 1 ; k < 11 ; ++k)
    {
    for(n = 0 ; n < 16 ; ++n)
      cp[n] = sbox[cp[n]];

    for(i = 0 ; i < 4 ; ++i)
      {
      for(j = 0 ; j < 4 ; ++j)
        {
        n = (i << 2) + j;
        dp[n] = cp[(((i+j) % 4) << 2) + j];
        }
      }

    if(k == 10)
      {
      for(n = 0 ; n < 16 ; ++n)
        cp[n] = dp[n];
      }
    else
      {
      for(i = 0 ; i < 4 ; ++i)
        {
        n = i << 2; 
        cp[n] = (gfp2[dp[n]] ^ gfp3[dp[n+1]] ^ dp[n+2] ^ dp[n+3]);
        cp[n+1] = (dp[n] ^ gfp2[dp[n+1]] ^ gfp3[dp[n+2]] ^ dp[n+3]);
        cp[n+2] = (dp[n] ^ dp[n+1] ^ gfp2[dp[n+2]] ^ gfp3[dp[n+3]]);
        cp[n+3] = (gfp3[dp[n]] ^ dp[n+1] ^ dp[n+2] ^ gfp2[dp[n+3]]);
        }
      } 

    wp = w + 16*k;
    for(n = 0 ; n < 16 ; ++n)
      cp[n] ^= wp[n];
    }   
   
  if(flipflag == 0)
    {
    for(n = 0 ; n < 16 ; ++n)
      out[n] = cp[n];
    }
  else
    {
    for(n = 0 ; n < 16 ; ++n)
      out[n] = cp[15-n];
    }

  return(1);
  }

int aes_cmac( unsigned char* keyx,unsigned char* in,int len, unsigned char* outx)
  {
  int n,i,flag;
  static unsigned char x[16],y[16],rb[16],m[80]; 
  static unsigned char out[16],key[16],k1[16],k2[16];

  if(len > 80)
    return(0);

  for(n = 0 ; n < 16 ; ++n)
    {
    x[n] = 0;
    rb[n] = 0;
    key[n] = keyx[15-n];
    }
  rb[15] = 0x87;

  for(n = 0 ; n < 80 ; ++n)
    m[n] = 0;
   
  calce(key,x,y,0);
  bleft(k1,y);
  if(y[0] & 0x80)
    bxor(k1,k1,rb);
 
  bleft(k2,k1);
  if(k1[0] & 0x80)
    bxor(k2, k2,rb);

  n = (len/16);
  flag = 0;
  if(len % 16 != 0)
    ++n;
  if(n == 0)
    n = 1;
  else if((len % 16) == 0)
    flag = 1;

  for(i = 0 ; i < len ; ++i)
    m[i] = in[len-1-i];

  if(flag == 0)
    {
    m[len] = 0x80;
    bxor(m+(n-1)*16, m+(n-1)*16, k2);
    }
  else
    bxor(m+(n-1)*16, m+(n-1)*16, k1);
  
  for(i = 0; i < n - 1; ++i)
    {
    bxor(y, m+16*i, x);
    calce(key,y,x,0);
    }
  bxor(y,m+(n-1)*16,x);
  calce(key,y,out,0);
  for(n = 0 ; n < 16 ; ++n)
    outx[n] = out[15-n];
  return(1);
  }

void bxor(unsigned char *c,unsigned char *a,unsigned char *b)
  {
  int n;

  for (n = 0; n < 16; ++n)
    c[n] = a[n] ^ b[n];
  }

void bleft(unsigned char *b,unsigned char *a)
  {
  int n;
  unsigned char oflow;
    
  oflow = 0;
  for (n = 15; n >= 0; --n)
    {
    b[n] = a[n] << 1;
    b[n] |= oflow;
    oflow = (a[n] & 0x80) ? 1 : 0;
    }
  }


void getrand(unsigned char *s,int len)
  {
  int r,k,n; 

  n = 8;
  k = 0;
  r = 0;
  while(k < len)
    {
    if((n & 8) != 0)
      {
      r = rand();
      n = 1;
      }
    else
      r >>= 8;
    s[k] = r & 0xFF;
    n <<= 1;
    ++k;  
    }  
  }


#ifndef PICOSTACK
int readkeyfile(unsigned char **table,int *count)
  {
  FILE *stream;
  int k,retval,xcount;
  
  stream = fopen(gpar.datfile,"rb");
  if(stream == NULL)
    return(0);
 
  retval = 0;
  xcount = fgetc(stream);
      
  if(xcount != 0 && xcount <= 250 && xcount != EOF)
    {
    k = xcount*22;
    *table = (unsigned char *)malloc(k);
    if(*table != NULL && fread(*table,1,k,stream) == (unsigned int)k)
      {
      *count = xcount;
      retval = 1;
      }
    }
  fclose(stream);
  return(retval);
  }

void writekeyfile(unsigned char *table,int count)
  {
  FILE *stream;
  
  stream = fopen(gpar.datfile,"wb");  
  if(stream == NULL)
    return;    

  fputc(count,stream);
  fwrite(table,1,count*22,stream);  
  fclose(stream);
  }
  
void freekeytable(unsigned char **table)
  {
  if(*table != NULL)
    free(*table);
  *table = NULL;
  }
    
unsigned char *newkeytable(int count)
  {
  return((unsigned char*)malloc(22*count));
  }
#endif

#ifndef BTFWINDOWS

 // Linux + PICOSTACK

int checkfilename(char *t,char *s)
  {
  return(1);
  }

int inithci()
  {
#ifdef PICOSTACK
  hcisock();
  return(1);
#else
  int n;
      
  bluezdown();

  if(hcisock() == 0)
    {
    NPRINT "Trying to unblock bluetooth with rfkill\n");
    flushprint();
    sleep_ms(500);
    n = system("rfkill unblock bluetooth");
    if(n != 0)
      n = 0;
    sleep_ms(500);
    bluezdown();
    if(hcisock() == 0)
      {
      NPRINT "No root permission or Bluetooth (hci%d) is off or crashed\n",gpar.devid); 
      NPRINT "Must run with root permission via sudo as follows:\n"); 
      NPRINT "  sudo ./btferret (for C) or  sudo python3 btferret.py\n");
      flushprint();
      return(0);
      }
    }
  return(1);
#endif  
  }

/************** OPEN HCI SOCKET ******        
return 0=fail
       1= OK and sets gpar.hci= socket handle
*************************************/       


int hcisock()
  {
#ifndef PICOSTACK
  int dd;
  //struct sockaddr_hci sa;
  unsigned char sa[6];
  
  if(gpar.hci > 0)
    return(1);
     
  VPRINT "Open HCI user socket\n");

         // AF_BLUETOOTH=31
  VPRINT "Open BTPROTO socket\n");
  dd = socket(31, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, BTPROTO_HCI);

  if(dd < 0)
    {
    VPRINT "Socket open error\n");
    flushprint();
    return(0);
    }
 
  VPRINT "Bind to Bluetooth devid user channel\n");

  sa[0] = 31;  // hci_family = AF_BLUETOOTH
  sa[1] = 0;
  sa[2] = gpar.devid & 0xFF;   // hci_dev = hci0/1/2...
  sa[3] = (gpar.devid >> 8) & 0xFF;
  sa[4] = 1;    // hci_channel = HCI_CHANNEL_USER
  sa[5] = 0;
   
  if(bind(dd,(struct sockaddr *)sa,sizeof(sa)) < 0)
    {
    VPRINT "Bind failed\n");
    close(dd);
    flushprint();
    return(0);
    }

  gpar.hci = dd;    
#endif

  VPRINT "Reset\n");
  sendhci(btreset,0);
  statusok(0,btreset);

  VPRINT "Set event masks\n");
  sendhci(eventmask,0);
  statusok(0,eventmask);
  sendhci(lemask,0);
  statusok(0,lemask);

  VPRINT "Set page/inquiry scan and timeouts = 10 secs\n");
  
  sendhci(scanip,0);  // SCAN_PAGE | SCAN_INQUIRY    
  statusok(0,scanip);
  sendhci(setcto,0);  // connection timeout = 10 sec 
  statusok(0,setcto);
  sendhci(setpto,0);  // page timeout = 10 sec
  statusok(0,setpto);
    
  VPRINT "HCI Socket OK\n");
  flushprint();
  return(1);
  }
#endif

#if (!defined(BTFWINDOWS) && !defined(PICOSTACK))                       

int bluezdown()
  {
  int dd,retval;
  
 
  VPRINT "Bluez down\n");

  retval = 0;
  
  dd = socket(31, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, BTPROTO_HCI);

  if(dd >= 0)
    {
    if(ioctl(dd,HCIDEVDOWN,gpar.devid) >= 0)  // hci0
      retval = 1;
    close(dd); 
    }
            
  if(retval == 0)
    VPRINT "Bluez down failed\n");
       
  flushprint();  
  sleep_ms(500);  
  return(retval);
  }


int closehci()
  {
  if(gpar.hci <= 0)
    return(0);
 
  VPRINT "HCI closed\n");
  close(gpar.hci);
  gpar.hci = -1;
  flushprint();
  return(1);
  }

int sendpack(unsigned char *buf,int len)
  {
  int ntogo,nwrit;
  unsigned long long timstart;
  unsigned char *cmd;
  
  ntogo = len;  
  timstart = time_ms();  
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
      sleep_ms(1);
        
    if(ntogo != 0 && (int)(time_ms() - timstart) > 5000)   // 5 sec timeout
      {
      NPRINT "Send CMD timeout - may need to reboot\n");
      return(0);
      }
    }
  while(ntogo != 0);
  
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

  tim0 = time_ms();
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
  while((int)(time_ms() - tim0) < toms);
  
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
      NPRINT "Invalid packet\n");
      blen = 0;
      packlen = 0;
      return(0);  // fatal
      }
   
    if(readbytes(buf,&blen,nread,1000) == 0)
      {
      NPRINT "Timed out waiting\n");
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
        NPRINT "Orphan extra data\n");
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
    NPRINT "Timed out waiting\n");
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
        NPRINT "Missing extra data\n");
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

void printn(char *buf,int len)
  {
  int dum;
  
  dum = write(STDOUT_FILENO,buf,len);
  if(dum != 0)
    dum = 0;
  }

void scroll_back()
  {
  int n,incn,count,startn,startnx,endn,dum;

  if(check_init(0) == 0)
    return;
 
  if(gpar.prte == gpar.prts)
    {
    printf("**** Have scrolled up to start\n");
    return;
    }
  printf("****** SCROLL UP start new page\n");     
  // from current last line prte go back 5 lines
  // ptre muat be one past end line 0A
  count = 0;
  startn = gpar.prts;
  startnx = gpar.prts;
  endn = -1;
  n = gpar.prte;
  do
    {
    if(gpar.s[n] == 10)
      {
      ++count;
      if(count == 10 || count == 40 || count == 90)  // found good end
        {
        incn = n+1;
        if(incn == gpar.prtw)  // wraps
          incn = 0;
        if(count == 10)  // found good end 10 lines up
          endn = incn;
        else if(count == 40) // found good start that is sure to fit on screen
          startnx = incn;
        else 
          startn = incn;  // found good start for large number of lines per screen
        }
      }
 
    --n; 
    if(gpar.prtw != 0 && n < 0)
      n = gpar.prtw-1;  // wraps   
    } 
  while(n != gpar.prts && count < 90);
  
  if(endn >= 0)
    gpar.prte = endn;  

  if(startnx == gpar.prts)
    printf("Have scrolled to start of stored output\n");
  else
    {
    if(gpar.prte >= startn)  
      dum = write(STDOUT_FILENO,gpar.s+startn,gpar.prte - startn);
    else
      {
      dum = write(STDOUT_FILENO,gpar.s+startn,gpar.prtw - startn);
      dum = write(STDOUT_FILENO,gpar.s,gpar.prte);
      }
    printf("****** SCROLLED UP 10 lines\n");
    }
  if(dum != 0)
    dum = 0;    
  }
  
void scroll_forward()
  {
  int n,incn,count,startn,endn,dum;

  if(check_init(0) == 0)
    return;
  
  if(gpar.prte == gpar.prtp)
    {
    // Have scrolled down to the end - do nothing
    return;
    }
     
   
  // from current last line prte go forward 10 lines
  // ptre muat be one past end line 0A
  startn = gpar.prte;   // start from last
  count = 0;
  endn = -1;
  n = gpar.prte;
  do
    {
    if(gpar.s[n] == 10)
      {
      ++count;
      if(count == 20)  // found good end
        {
        incn = n+1;  // one past end of line
        if(incn == gpar.prtw)
          incn = 0;
        endn = incn;
        }
      }
        
    ++n;
    if(n == gpar.prtw)
      n = 0;   // wraps
    } 
  while(n != gpar.prtp && count < 20);
 
 
  if(endn >= 0)
    gpar.prte = endn;  // 10 lines 0n  
  else
    gpar.prte = gpar.prtp;  // end of data
     
  if(gpar.prte >= startn)  
    dum = write(STDOUT_FILENO,gpar.s+startn,gpar.prte - startn);
  else
    {
    dum = write(STDOUT_FILENO,gpar.s+startn,gpar.prtw - startn);
    dum = write(STDOUT_FILENO,gpar.s,gpar.prte);
    }

  if(gpar.prte == gpar.prtp)
    printf("**** SCROLLED DOWN - reached end\n");
  if(dum != 0)
    dum = 0; 
  }

/******************** TIME in milliseconds *************       
return time in milliseconds since first call reset
****************************************************/       

unsigned long long time_ms()
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

void sleep_ms(uint32_t ms)
  {
  usleep(ms * 1000);
  }

void inputpin(char *prompt,char *sbuf)
  {
  int j;
  char *dum;
  
  printf("%s",prompt);
  j = setkeymode(0);
  do
    {
    dum = fgets(sbuf,16,stdin);
    }
  while(sbuf[0] == 10);
  if(dum != NULL)
    dum = NULL;
  setkeymode(j); 
  }

void serverexit(int flag)
  {
  return;
  }
  

/************ SET INPUT MODE *************
setkeymode(1) = readkey() returns immediately if no key press 
                and does not wait for a key - for readserial key press exits
setkeymode(0) = restore original key mode (only after setkeymode(1) 
***************************************/
  
int setkeymode(int setflag)
  {
  int oldflag;
  struct termios tattr;
  static int flag = 0;
  static int savflag = 0;
  static struct termios saved_attributes;

  if(savflag == 0)  
    {  // Save the terminal attributes so we can restore them later
    tcgetattr(STDIN_FILENO, &saved_attributes);
    savflag = 1;
    }
    
  oldflag = flag;
  
  if(flag == setflag)
    return(oldflag); 
    
  if(setflag == 0)
    {
    tcsetattr(STDIN_FILENO,TCSANOW,&saved_attributes); 
    flag = 0;
    return(oldflag);
    } 
    
  /* Set the terminal modes. */
  tcgetattr(STDIN_FILENO,&tattr);
  tattr.c_lflag &= ~(ICANON|ECHO|ISIG|ECHONL|IEXTEN);                   
  tattr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  tattr.c_oflag = OPOST|ONLCR;   
  tattr.c_cflag &= ~(CSIZE | PARENB);
  tattr.c_cflag |= CS8;
  tattr.c_cc[VMIN] = 0;
  tattr.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO,TCSAFLUSH,&tattr);
  flag = 1; 
  return(oldflag);
  } 


int readkey()
  {
  static unsigned char keystack[1024];
  static int stn = 0;
  unsigned char c;
  unsigned short *seq;
  int n,k,j,i,flag,cret,ks0;
  /***** look up table. Each entry (padded to 6-bytes by zeroes) =
  btferret custom code,key code sequence returned by read(STDIN)
  **********/

  unsigned short seq0[1] = { 0 };

  unsigned short seq27[190] = {
  1,27,91,80,0,0,
  2,27,91,50,126,0,
  3,27,91,51,126,0,
  4,27,91,49,126,0,
  5,27,91,52,126,0,
  6,27,91,53,126,0,
  7,27,91,54,126,0,
  14,27,91,91,65,0,
  15,27,91,91,66,0,
  16,27,91,91,67,0,
  17,27,91,91,68,0,
  18,27,91,91,69,0,
  19,27,91,49,55,126,
  20,27,91,49,56,126,
  21,27,91,49,57,126,
  22,27,91,50,48,126,
  23,27,91,50,49,126,
  24,27,91,50,51,126,
  25,27,91,50,52,126,
  28,27,91,67,0,0,
  29,27,91,68,0,0,
  30,27,91,66,0,0,
  31,27,91,65,0,0,
  471,27,91,50,53,126,
  472,27,91,50,54,126,
  473,27,91,50,56,126,
  474,27,91,50,57,126,
  475,27,91,51,49,126,
  476,27,91,51,50,126,
  477,27,91,51,51,126,
  478,27,91,51,52,126,
  0 };
  
  
  unsigned short seqxx[200] = {
  11,194,163,0,0,0,
  12,194,172,0,0,0,
  481,195,166,0,0,0,
  482,226,128,157,0,0,
  483,194,162,0,0,0,
  484,195,176,0,0,0,
  486,196,145,0,0,0,
  487,197,139,0,0,0,
  488,196,167,0,0,0,
  489,226,134,146,0,0,
  490,204,137,0,0,0,
  491,196,184,0,0,0,
  492,197,130,0,0,0,
  493,194,181,0,0,0,
  495,195,184,0,0,0,
  496,195,190,0,0,0,
  498,194,182,0,0,0,
  499,195,159,0,0,0,
  500,197,167,0,0,0,
  501,226,134,147,0,0,
  502,226,128,156,0,0,
  504,194,187,0,0,0,
  505,226,134,144,0,0,
  506,194,171,0,0,0,
  507,194,185,0,0,0,
  508,194,178,0,0,0,
  509,194,179,0,0,0,
  510,226,130,172,0,0,
  511,194,189,0,0,0,
  512,194,190,0,0,0,
  513,226,148,128,0,0,
  514,194,183,0,0,0,
  515,204,163,0,0,0,
  0 };
  
  // use gpar.keyboard for non-GB tables  
 
  if(stn > 0)
    {
    c = keystack[0];
    n = 1;
    --stn;
    for(k = 0 ; k < stn ; ++k)
      keystack[k] = keystack[k+1];
    }
  else
    n = read(STDIN_FILENO,&c,1);
    
  if(n == 0)
    return(0);
    
  cret = c;
    
  if(c == 27 || (c >= 194 && c <= 197) || c == 226 || c == 204)
    {
    do
      {
      n = read(STDIN_FILENO,keystack+stn,1);
      if(n == 1 && stn < 1023)
        ++stn;
      }
    while(n == 1);
    // chars in c,keystack
    ks0 = keystack[0];
    if(stn == 0)
      {
      if(c == 226)
        cret = 0;
      return(cret);  // inc ESC alone
      }
    else if(c == 27 && ks0 > 0 && ks0 < 128 && ks0 != 91)
      {   
      cret = ks0;
      if(cret == 13)
        cret = 10;   // Enter
      else if(cret == 127)
        cret = 8;
      cret |= 0x100;  // Alt Gr table  27 + single char
      --stn;
      for(k = 0 ; k < stn ; ++k)
         keystack[k] = keystack[k+1];    
      }    
    else 
      {  // search look up
      cret = 0;  // fail
      if(c == 27)
        {
        if(ks0 == 91)
          seq = seq27;
        else
          seq = seq0;
        }
      else
        seq = seqxx;           
      for(j = 0 ; seq[j] != 0 && cret == 0 ; j += 6)
        {
        if(seq[j+1] == c)
          {
          flag = 0;
          i = 0;
          while(seq[j+i+2] != 0 && i < 4 && flag == 0)
            {
            if(i >= stn || seq[j+i+2] != keystack[i])
              flag = 1;
            ++i;
            }
          // i = number of chars
          if(flag == 0)
            {    
            stn -= i;
            for(k = 0 ; k < stn ; ++k)
              keystack[k] = keystack[k+i];    
            cret = seq[j];
            }
          }     
        }
      if(cret == 0)
        {  // search fail
        i = 0; // pop
        if(c == 27)
          {  // no send ESC
          cret = ks0;
          i = 1;
          }
        else if(c == 226 && stn >= 2)
          {
          i = 2;
          cret = 0;
          }
        else if(stn >= 1 && ((c >= 194 && c <= 197) || c == 204))
          {
          i = 1;
          cret = 0;
          }
        else
          cret = 0;
        
        if(i != 0)
          {
          stn -= i;
          for(k = 0 ; k < stn ; ++k)
            keystack[k] = keystack[k+i];
          }
        }
      }   // end search
    }
  else
    {
    if(cret == 13)
      cret = 10;   // Enter
    else if(cret == 127)
      cret = 8;
    else if(cret > 0 && cret < 32 && cret != 9 && cret != 27)
      cret += 224;  // CTL table  CTL a = 225
    }      
  
          
  return(cret);
  }

int getdatfile(char *s)
  {
  strcpy(s,"/etc/btferret.dat");
  return(1);
  }
   
// end ifndef BTFWINDOWS + PICOSTACK 
#endif


int user_function(int n0,int n1,int n2,int n3,unsigned char *dat0,unsigned char *dat1)
  {  
  // your code here
  // For Python - recompile the module via 
  //     python3 btfpymake.py build
  return(0);
  }
