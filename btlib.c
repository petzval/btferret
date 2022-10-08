#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "btlib.h"

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

struct sockaddr_hci
  {
  unsigned short hci_family;
  unsigned short hci_dev;      
  unsigned short hci_channel;
  };

struct sockaddr_un
  {
  unsigned short sun_family;
  char sun_path[108];
  };

struct sockaddr_rc
  {
  unsigned short rc_family;
  unsigned char rc_bdaddr[6];
  unsigned char rc_channel;
  };  

struct sockaddr_l2
  {
  unsigned short l2_family;
  unsigned short l2_psm;
  unsigned char l2_bdaddr[6];
  unsigned short l2_cid;
  unsigned char l2_bdaddr_type; 
  };
  
  
struct hci_filter
  {
  unsigned long type_mask;
  unsigned long event_mask[2];
  unsigned short opcode;
  };

struct hci_dev_req
  {
  unsigned short dev_id;
  unsigned long dev_opt;
  };


/************** END BLUETOOTH DEFINES ********/


#define VERSION 3
   // max possible NUMDEVS = 256 
#define NUMDEVS 256
#define NAMELEN 34
#define TOPUPCREDIT 200
  // ATT_MTU data
#define LEDATLEN 20

struct cticdata 
  {
  int type;    // CTIC_UNUSED=allocated but unused CTIC_ACTIVE=used CTIC_END=terminate
  int cticn;   // index
  int size;    // number of bytes 0=unknown
  int perm;    // permissions 0=unknown 02=read 04=write no ack 08=write ack
  int notify;   // 1=enable notifications
  char name[NAMELEN];  // name of characteristic - your choice
  int chandle;    // characteristic handle 0=unknown 
  int uuidtype;   // 0=unknown 2=2-byte 16=16-byte
  int iflag;      // 1=have read info from remote device
  unsigned char value[LEDATLEN];
  char uuid[16];
  int (*callback)();
  struct cticdata *nextctic;  // next in chain
  };

#define CTIC_UNUSED 0
#define CTIC_ACTIVE 1
#define CTIC_END    2
struct cticdata cticnull = {CTIC_END};  // terminate ctic chain 

struct devdata
  {
  int type;                   // 0=not used    BTYPE_  values   
  char name[NAMELEN];         // name of LE device - your choice
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
  int nx;                     // extra data stack index
  int dhandle[2];             // handle returned by connect  [0]=lo [1]=hi
  int linkflag;
  char linkey[16];
  char pincode[64];
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
// KEY_ON  1
// PASSKEY_LOCAL 0
// PASSKEY_REMOTE 2
// PASSKEY_OFF 4
#define KEY_FILE 8
#define KEY_NEW  16
#define KEY_SENT 32


#define BTYPE_ALL 0
#define DIRN_FOR  0
#define DIRN_REV  1

// timer
#define TIM_RUN 0
#define TIM_LOCK 1
#define TIM_FREE 2
#define TIM_OVER (0xF0000000 >> 10)
             // 72.8 hours before overflow
             
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

  int blockflag;       
  int hci;             // file desc for hci/acl commands sendhci/readhci
  int devid;           // hciX number
  int bluez;           // 0/1 bluez down/up for server functions
  int timout;          // long time out for replies ms
  int toshort;         // short time out replies
  int meshflag;        // 1=R 2=W
  int readerror;       // 0=none 1=time out 2=key press  
  int lebufsize;
  int leclientwait;    // delay when connect as LE client
                       // to see any requests from server
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

 
  int maxpage;
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


int meshpacket(char *buf);
void clscanx(void);
int lescanx(void);
int clconnect(int ndevice,int channel,int method);
int leservices(int ndevice,int flags,char *uuid);
int clservices(int ndevice,int flags,char *uuid);
void printlocalchannels(void);
int services(int ndevice,int flags,char *uuid);
int printchannels(int ndevice,int flags,struct servicedata *serv,int servlen);
int printctics0(int devicen,int flags);
int printctics1(int devicen);
int savectic(int devicen,struct servicedata *serv,int servlen);
int finduuidtext(int uuid);
void hexdump(unsigned char *buf, int len);
void printascii(char *s,int len);
unsigned char calcfcs(unsigned char *s,int count);
int pushins(long long int typebit,int ndevice,int len,unsigned char *s);
int addins(int nx,int len,unsigned char *s);
void popins(void);
int decodesdp(char *sin,int len,struct servicedata *serv,int servlen);
int decodedes(char *sin,int len,struct sdpdata *sdpp);
int openremotesdp(int ndevice);
int clconnect0(int ndevice);
int clconnectxx(int ndevice);
void clearins(int ndevice);
int connectpsm(int psm,int channel,int ndevice);
int disconnectl2(int ndevice,int psm);
void setcredits(int ndevice);
int readpack(char *c,int clen,int *ndevice,int count,int endchar,int toflag,int abtflag);
int readrf(int *ndevice,char *inbuff,int count,char endchar,int flag,int timeoutms);
int readhci(int ndevice,long long int mustflag,long long int lookflag,int timout,int toshort);
int setkeymode(int setflag);
int leconnect(int ndevice);
int openremotesdpx(int ndevice);
int sconnectx(int ndevice);
unsigned int timems(int flag);
unsigned int strinstr(char *s,char *t);
int hexchar(char c);
int entrylen(unsigned int *ind,int in);
int devalloc(void);
struct cticdata *cticalloc(int dn); 
struct cticdata *ctic(int ndevice,int n);
int devok(int ndevice);
int devokp(int ndevice);
int devlist(int mask);
void flushprint();
int bluezdown(void);
int readkey(void);
int devn(int node);
int devnp(int node);
int devnfrombadd(char *badd,int type,int dirn);
char *baddstr(char *badd,int dirn);
int disconnectdev(int ndevice);
void meshreadon(void);
void meshreadoff(void);
int statusok(int flag,char *md);
int readline(FILE *stream,char *s);
int newnode(void);
int classicserverx(int clientnode);
int psmcheck(int n,long long lookflag);
void immediate(long long lookflag);
int bincmp(char *s,char *t,int count,int dirn);
void waitdis(int ndevice,unsigned int timout);
int writecticx(int node,int cticn,unsigned char *data,int count,int notflag,int (*callback)());
void replysdp(int ndevice,int in,char *uuid,char *name);
int addaid(char *sdp,char *aid,int *rn,int aidj,int aidk,int aidn);
void rwlinkey(int rwflag,int ndevice);
int localctics(void);
void leserver(int ndevice,int count,unsigned char *dat);
int nextctichandle(int start,int end,int *handle);
int findcticuuid(int start,int end,unsigned char *uuidrev,int size);
char *cticerrs(struct cticdata * cp);
void addname(void);

int closehci(void);
int sendhci(unsigned char *cmd,int ndevice);
int hcisock(void);
int findhci(long long int type,int devicen,int popflag);
void printins(void);

int sconnect(int ndevice);
int readserial(int *ndevice,char *inbuff,int count,char endchar,int exitflags,int timeoutms);
char *strtohexx(char *s,int slen,int *num);

void readleatt(int node,int handle);


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
#define IN_IMMED  ((long long int)1 << 63)

/***************** END Received PACKET TYPES *************/

#define AUTO_RF  1
#define AUTO_DIS 2
#define AUTO_MSC 3

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
unsigned char lemask[20] = { 12,0,0,0,0x01,0x01,0x20,0x08,0x1F,0x04,0,0,0,0,0,0 };
unsigned char lesetscan[16] = { 11,0,0,0,0x01,0x0B,0x20,0x07,0x01,0x10,0x00,0x10,0x00,0x00,0x02 };
unsigned char locbadd[10] = {4,0,0,0,0x01,0x09,0x10,0};
unsigned char leopen[40] = {29,0,S2_BADD,0,
  1,0x0D,0x20,0x19,0x60,0,0x60,0,0,0,0x7C,0x17,0x2D,0xC0,0x1E,0,0,0x18,0,0x28,0,0,0,0x11,0x01,0,0,0,0}; // len 29
unsigned char leupdate[40] = {18,0,0,0,
  1,0x13,0x20,0x0E,0,0,0x18,0,0x28,0,0,0,0x11,0x01,0,0,0,0}; // len 18
unsigned char lerrf[20] = {6,0,0,0,1,0x16,0x20,0x02,0,0}; // len 6
                                 // LE open [10]... board address     [23][24] = timeout x 10ms
unsigned char lecancel[8] = {4,0,0,0,0x01,0x0E,0x20,0};

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

unsigned char lereaduuid2[32] = {16,0,S2_HAND,0,2,0x40,0,11,0,7,0,4,0,0x08,0x01,0,0xFF,0xFF,0x03,0x28};   
unsigned char lereaduuid16[40] = {30,0,S2_HAND,0,2,0x40,0,25,0,21,0,4,0,0x08,0x01,0,0xFF,0xFF,
0x00,0xFF,0xEE,0xDD,0xCC,0xBB,0xAA,0x99,0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11};   
unsigned char leconf[20] = {10,0,S2_HAND,0,2,0x40,0,5,0,1,0,4,0,0x1E};  // len 12 

unsigned char mtuset[20] = {12,0,S2_HAND,0,2,0x40,0,7,0,3,0,4,0,0x02,0x19,0x00};  // len 14

               //  packet size = len+9      [3][4] = len + 4  [5][6] = len  [9] = data 
unsigned char attdata[1024] = { 0,0,S2_HAND,0,0x02,0,0,0,0,0,0,0x04,0x00 };

unsigned char lescanon[10] = {6,0,0,0,1,0x0C,0x20,2,1,0};  // scan for LE devices on  duplicate filter off
unsigned char lescanonf[10] = {6,0,0,0,1,0x0C,0x20,2,1,1};  // scan for LE devices on  duplicate filter on
unsigned char lescanoff[10] = {6,0,0,0,1,0x0C,0x20,2,0,0};  // scan for LE devices off 
       // n data = 8   [PAKHEADSIZE+11] = board
unsigned char leadvert[40] = { 36,0,0,0,0x01,0x08,0x20,0x20,0x0F,0x08,0xFF,0x34,0x12,
0x00,0x00,0xC0,0xDE,0x99,0x05,0x08,0x61,0x62,0x63,0x64,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

        // set advertising parmeters  [8]=type 0=connectable 3=non connectable, unidirected adv
                                                      //        min int   max int    x0.625ms 0800 = 1.28s  0200=320ms                               
unsigned char leadparam[32] =   { 19,0,0,0,0x01,0x06,0x20,0x0F,0x00,0x02,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x00 };
unsigned char leadparam4[32] =  { 19,0,0,0,0x01,0x06,0x20,0x0F,0x00,0x08,0x00,0x08,0x01,0x00,0x00,0xB8,0x27,0xEB,0xF1,0x50,0xC3,0x07,0x00 };
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

unsigned char msccmdrsp[64] = { 17,0,S2_HAND | S2_DCIDC | S2_FCS2,S3_ADDMSC,0x02,0x0C,0x00,0x0C,0x00,0x08,0x00,0x40,0x00,0x03,0xEF,0x09,0xE3,0x05,0x0B,0x8D,
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
unsigned char encrypt[16] = {  7,0,S2_HAND,0,0x01,0x13,0x04,0x03,0x0C,0x00,0x01};

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
0x00,0x04,0x00,0x00,0x00};
                    // [PAKHEADSIZE+17] = 04  no resources - connection failed   scid/dcid ignored  OK reply was S3_SCID1 | S3_DCID2

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
unsigned char spcomp[20] =   { 10,0,S2_BADD,0,0x01,0x2C,0x04,0x06,0x11,0x22,0x33,0x44,0x55,0x66 };


unsigned char baseuuid[16] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0xFF};    

  // LE server
unsigned char lereadreply[LEDATLEN+20] = {11,0,S2_HAND,0,2,0x40,0,0x06,0,0x02,0,4,0,0x0B,0};  // length 10+number bytes

unsigned char le05reply[40]  = {15,0,S2_HAND,0,2,0x40,0,0x0A,0,0x06,0,4,0,0x05,0x01}; 
 
unsigned char le09replyv[LEDATLEN+24] =  {32,0,S2_HAND,0,2,0x40,0,0x1B,0,0x17,0,4,0,0x09};

unsigned char leack[16] =  {10,0,S2_HAND,0,2,0x40,0,0x05,0,0x01,0,4,0,0x13};
unsigned char lemtu[16] =  {12,0,S2_HAND,0,2,0x40,0,0x07,0,0x03,0,4,0,0x03,LEDATLEN+3,0};


unsigned char fob05[20]  = {15,0,S2_HAND,0,2,0x40,0,0x0A,0,0x06,0,4,0,0x05,0x01,0x01,0x00,0x00,0x28}; 
unsigned char fob09[20]  = {15,0,S2_HAND,0,2,0x40,0,0x0A,0,0x06,0,4,0,0x09,0x04,0x01,0x00,0x00,0x18}; 
unsigned char fob11[24]  = {17,0,S2_HAND,0,2,0x40,0,0x0C,0,0x08,0,4,0,0x11,0x06,0x01,0x00,0xFF,0xFF,0x00,0x18}; 
 
unsigned char lefail[20] =  {14,0,S2_HAND,0,2,0x40,0,9,0,5,0,4,0,0x01,0x08,0,0,0x0A};  

unsigned char custuuid[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
char custname[64] = "None";

/********************** END sendhci() PACKETS *********************/


/*************** PRINT BUFFER **********/

#define PRBUFSZ 65536
#define PRLINESZ 256
#define PRPAGESZ 8192

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
"Insufficient Authentication",
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
  int n,dn,cn,k,j,sn,hn,i,len,flag,errflag,errcount,getout;
  int clflag,leflag,readret,meshcount,lecap;
  unsigned int ind[16];
  struct cticdata *cp;
  char s[256],*data,*es,buf[128],*uuid;
  FILE *stream;
  static char errs[16] = {"   ERROR **** "};
  static unsigned char wln[8] = { 252,0,0,0,1,0x13,0x0C,0xF8 };
    
     // global parameters 

  printf("Initialising...\n");
   
  gpar.timout = 1000;   // reply wait ms time out
  gpar.toshort = 25;    // ms
  gpar.meshflag = 0;    // le advertising off
  gpar.readerror = 0;
  gpar.leclientwait = 750;
    
  gpar.prtp = 0;   // current end of buffer
  gpar.dump = PRBUFSZ-PRLINESZ;  // dump destination
  gpar.dumpn = 0;  // char count
  gpar.prtp0 = 0;  // start next print 
  gpar.prtw = 0;   // wrap index
  gpar.prts = 0;   // start of circular buffer
  gpar.prte = 0;   // end of print for scroll
   
  gpar.s = (char*)calloc(PRBUFSZ,1);
  instack = (char*)malloc(INSTACKSIZE); 
  
  if(gpar.s == NULL || instack == NULL)
    {
    printf("Memory allocate fail\n");
    return(0);
    }
    
  for(n = 0 ; n < INSTACKSIZE ; ++n)
    instack[n] = INS_FREE;

  insdat = instack + INSHEADSIZE;     
    
  set_print_flag(PRINT_NORMAL);  
  //set_print_flag(PRINT_VERBOSE);

  gpar.maxpage = 0;
  gpar.hci = -1;       // hci socket
  gpar.devid = hcin;   // hci0
  gpar.blockflag = 0;

  register_serial(strtohex("FCF05AFD-67D8-4F41-83F5-7BEE22C03CDB",NULL),"My custom serial");  
    
  gpar.bluez = 1;   // assume bluez up
  bluezdown();      // down

  errcount = 0;
  meshcount = 0;
      
  // zero entries  n=first undefined
  for(k = 0 ; k < NUMDEVS ; ++k)
    dev[k] = NULL;

  dn = devalloc();
  if(dn != 0)
    return(0);
    
  dev[0]->type = BTYPE_LO;
  dev[0]->meshindex = 1;  // first message index
  dev[0]->node = 0;   // node not found    
  strcpy(dev[0]->name,"not in devices.txt");
  
  clearins(0);   // initialise BT packet input stack 
   
  if(hcisock() == 0)    
    {   
    NPRINT "** ERROR ** Must run with root permission via sudo as follows:\n"); 
    NPRINT "sudo ./btferret\n");
    flushprint();
    return(0);
    }

  lecap = 0;
  flag = 0;
  VPRINT "Read local supported commands\n");
  sendhci(locsup,0);
  do
    {
    readhci(0,IN_STATOK,0,gpar.timout,gpar.toshort);
    n = findhci(IN_STATOK,0,INS_POP);
    if(n >= 0 && insdatn[1] == locsup[PAKHEADSIZE+1] && insdatn[2] == locsup[PAKHEADSIZE+2])
      { 
      flag = 1;
      if((insdatn[29] & 0xA2) == 0xA2 && (insdatn[30] & 0x3E) == 0x3E)
        lecap = 1;  // LE capable
      }
    }
  while(n >= 0 && flag == 0);
  
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
    errcount = 1000;
    }

  if(lecap != 0)
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

    VPRINT "Set LE advertising parameters\n");
    sendhci(leadparam,0);
    statusok(0,leadparam);
    VPRINT "Set LE advertising data\n");
    sendhci(leadvert,0);  // reset mesh packet index = 0
    statusok(0,leadvert);
    }
    
    
  stream = fopen(filename,"r");
  if(stream == NULL)
    {
    NPRINT "Unable to open devices info file %s\n",filename);
    readret = 0;
    }
  else
    readret = readline(stream,s);  // read first line
  
  NPRINT "Device data from %s file\n",filename);
  flushprint();
    
      
  while(readret > 0 && errcount < 6)
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
    ind[11] = strlen(s) << 16;
    ind[12] = 0x80000000;  // terminate flag
      
    if(ind[4] != 0)
      NPRINT "  %s\n",s);
    else  
      NPRINT "%s\n",s);
    flushprint();

 
    flag = 0;   // type of line not determined
                // 1=device
                // 2=le characteristic

    cp = &cticnull;  // for safety
    
    clflag = ind[0] + ind[1] + ind[2] + ind[3] + ind[9] + ind[10];
    leflag = ind[4] + ind[5] + ind[6] + ind[7] + ind[8];

    es = NULL;
     
    if(clflag == 0 && leflag == 0)
      es = "Must start with DEVICE= or LECHAR=";
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
        dn = devalloc();
        if(dn < 0)
          return(0);  // fatal alloc error
        flag = 1;
        }
      }
    else 
      {
      // ind[5] == 0 && ind[6] == 0  no handle/UUID
      cp = cticalloc(dn); // return ctic pointer - may be to cticnull=failed
      if(cp->type != CTIC_UNUSED)
        return(0);   // fatal alloc error
      flag = 2;  // characteristic entry
      }
     
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
        }  // end ind[0] DEVICE
        
      if(ind[1] != 0)
        {  // TYPE  
        sn = ind[1] & 0xFFFF;
        if(strncasecmp(s+sn,"CLASSIC",7) == 0)
          dev[dn]->type = BTYPE_CL;
        else if(strncasecmp(s+sn,"LE",2) == 0)   
          dev[dn]->type = BTYPE_LE;
        else if(strncasecmp(s+sn,"MESH",2) == 0)   
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
        }  // end ind[2] ADDRESS
      
      if(ind[3] != 0)
        {   // PIN 
        sn = ind[3] & 0xFFFF;
        i = 0;
        while(i < 63 && s[sn+i] != 0 && s[sn+i] != ' ')
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


      if(errflag != 0)
        ++errcount;

      // check if local device / mesh
      if(flag == 1)
        {
        if(devnfrombadd(dev[dn]->baddr,BTYPE_ALL,DIRN_FOR) == 0)
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
      readret = readline(stream,s);  // read next line
    }  // end read file line loop
    
  if(stream != NULL)
    fclose(stream);
   
  if(dev[0]->node == 0)
    {
    dev[0]->node = newnode();
    sprintf(buf,"Node %d",dev[0]->node);
    strcpy(dev[0]->name,buf);    
    NPRINT "\nThis local device has been allocated NODE = %d\n",dev[0]->node);
    NPRINT "It should be added to the %s file as follows:\n",filename);
    NPRINT "DEVICE=this device name  TYPE=MESH  NODE=choose  ADDRESS=%s\n",baddstr(dev[0]->baddr,0));
    }
  else
    {  // write local name
    for(n = 0 ; n < 8 ; ++n)
      s[n] = wln[n];
    for(n = 8 ; n < 256 ; ++n)
      s[n] = 0;
    strcpy(s+8,dev[0]->name);
    sendhci(s,0);
    }
      
  if(localctics() == 0)
    ++errcount;    
 
  if(lecap == 0)
    {
    NPRINT "\n*** Bluetooth adapter is not LE capable ***\n");
    NPRINT "*** Mesh/LE functions will not work *******\n");
    }
  else
    {
    VPRINT "Set LE advertising data with device name\n");
    addname();
    sendhci(leadvert,0);  // reset mesh packet index = 0
    statusok(0,leadvert);
    }

  flushprint();

  if(errcount == 0)
    {
    rwlinkey(0,0);
    atexit(close_all);
    return(1);
    }
    
    
  printf("\n************ initblue() FAILED ************\n");
         
  return(0);  
  }


char *cticerrs(struct cticdata * cp)
  {
  static char errs[80];
    
  sprintf(errs,"\n  ERROR *** Local node %d LE characteristic %s\n      ",dev[0]->node,cp->name); 
  return(errs);
  }
  


int localctics()
  {
  int n,k,j,i,uuidn,handle,flag,getout,min,max;
  struct cticdata *cp,*cpx;
  char *errs;
  
  
  for(j = 1 ; devok(j) != 0 ; ++j)
    {
    for(n = 0 ; ctic(j,n)->type == CTIC_ACTIVE ; ++n)
      {
      cp = ctic(j,n);
      if(cp->chandle == 0 && cp->uuidtype == 0)
        {
        NPRINT "\n  ERROR *** %s Node %d LECHAR=%s - No handle or UUID\n",dev[j]->name,dev[j]->node,cp->name);
        return(0);
        }
      }  
    }
    
    
  // check handles
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
  handle = 4;  // first alloc = 5
  
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
              --min;
            if((cpx->perm & 0x30) != 0)
              ++max;          
            if(handle >= min && handle <= max)
              {
              flag = 1;
              handle = max;
              }
            }
          }
        }
      while(flag != 0);
        
      cp->chandle = handle;
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
      NPRINT "%ssize must be 1-%d\n",errs,n,LEDATLEN);
      return(0);
      }

    /*******/ 
    if(cp->uuidtype == 2 && cp->uuid[0] == 0x2A && cp->uuid[1] == 0)
      { // device name
      j = 0;
      while(dev[0]->name[j] != 0 && j < LEDATLEN-1 && j < cp->size-1)
        {
        cp->value[j] = dev[0]->name[j];
        ++j;
        }
      cp->value[j] = 0;
      }
     /********/
     
      
      
      
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


char *baddstr(char *badd,int dirn)
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
  
  ndevice = devnp(node);
  if(ndevice >= 0)
    return(baddstr(dev[ndevice]->baddr,0));
  else
    return("00:00:00:00:00:00");
  }
  


int devnfrombadd(char *badd,int type,int dirn)
  {
  int n;
  
  for(n = 0 ; devok(n) != 0 ; ++n)
    {    
    if(bincmp(badd,dev[n]->baddr,6,dirn) != 0 && (type == 0 || (type & dev[n]->type) != 0))
      return(n);
    }
  return(-1);
  }
 


 
 
void mesh_on()
  {  // turn on LE advertising
  sendhci(leadvon,0);
  statusok(0,leadvon);
  gpar.meshflag |= MESH_W;
  } 

void mesh_off()
  {  // turn off LE advertising
  sendhci(leadvoff,0);
  statusok(0,leadvoff);
  gpar.meshflag &= ~MESH_W;
  } 

void meshreadon()
  {
  if((gpar.meshflag & MESH_R) != 0)
    return;
  sendhci(lescanon,0);
  statusok(0,lescanon);
  gpar.meshflag |= MESH_R;
  }

void meshreadoff()
  {
  if((gpar.meshflag & MESH_R) == 0)
    return;
  sendhci(lescanoff,0);
  statusok(0,lescanoff);
  gpar.meshflag &= ~MESH_R;
  }


int write_mesh(char *buf,int count)
  {
  int n;
  
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
    
  addname();  
    
  sendhci(leadvert,0);
  if(statusok(0,leadvert) == 0)
    return(0);
  return(count);  
  }


void addname()
  {   // add device name and pad zeoroes
  int n,maxm,m,k,padn;
  
  n = leadvert[PAKHEADSIZE+5] - 5;
  maxm = 23 - n;
  if(maxm < 1)  // no room for name
    padn = n + 11;
  else
    {
    m = strlen(dev[0]->name);
    if(m > maxm)
      m = maxm;
    leadvert[PAKHEADSIZE+4] = n+m+8;
    leadvert[PAKHEADSIZE+n+11] = m+1;
    leadvert[PAKHEADSIZE+n+12] = 0x08; // name code
    for(k = 0 ; k < m ; ++k)
      leadvert[PAKHEADSIZE+n+k+13] = dev[0]->name[k];
    padn = n + m + 13;
    }

   // pad with 0
  for(k = padn ; k <= 35 ; ++k)
    leadvert[PAKHEADSIZE+k] = 0;
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
    }
  else  // must be CTIC_UNUSED - already allocated
    cp = *cpp;    
     
  cp->type = CTIC_UNUSED;
  cp->cticn = j;
  cp->size = 0;   
  cp->perm = 0;
  cp->notify = 0;
  cp->chandle = 0;
  cp->uuidtype = 0;
  cp->iflag = 0;
  cp->nextctic = &cticnull;  // with type = CTIC_END
  cp->callback = NULL;
  
  for(j = 0 ; j < 16 ; ++j)
    cp->uuid[j] = 0;
  
  for(j = 0 ; j < LEDATLEN ; ++j)
    cp->value[j] = 0;
      
  return(cp);    
  }

int devalloc()
  {
  int j,dn;
  struct devdata *dp;
  char lkey[16] = { 0x08,0x7A,0xC7,0xFB,0x8C,0x86,0xF3,0xCF,0x36,0xF4,0x0C,0xD8,0xDD,0xA2,0xF9,0xD3 }; 
    
  dn = 0;
  while(devok(dn) != 0)
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
    }
  
  dp = dev[dn];  
  dp->conflag = 0;
  dp->meshindex = 0;
  dp->leaddtype = 0;
  dp->lecflag = 0;
  dp->dhandle[0] = 0;
  dp->dhandle[1] = 0;
  dp->nx = -1;
 
  for(j = 0 ; j < 6 ; ++j)
    dp->baddr[j] = 0;
  for(j = 0 ; j < 16 ; ++j)
    dp->linkey[j] = lkey[j];
  dp->linkflag = 0;        
  dp->type = 0;
  dev[dn]->name[0] = 0; 
  dp->node = 0;
  dev[dn]->pincode[0] = 0;
  dp->id = 1;
  dp->psm = 3;
  dp->credits = 0;
  dp->method = METHOD_HCI;
  dp->ctic = &cticnull;   
  return(dn);    
  }




/******************** LE SCAN ***********/
  
void le_scan()
  {
  lescanx();
  flushprint();
 
  } 
  
  
int lescanx()
  {    
  int n,j,k,i,tn,ndevice,count,repn,type,newcount;
  unsigned char *rp;
  struct devdata *dp;
  char buf[64];
  double fac;
  //static double powa[5] = { 1,10,100,1000,10000 };
  //static double powb[10] = { 1,1.25,1.6,2.0,2.5,3.16,4,5,6.3,8.0 };
  
  NPRINT "Scanning for LE devices - 10 seconds..\n");
  flushprint();
 
 
  VPRINT "Enable LE scan\n");
  sendhci(lescanonf,0); 
  if(statusok(0,lescanonf) == 0)
    {
    NPRINT "Scan on failed\n");
    return(0);
    }
    
  readhci(0,0,IN_LESCAN,10000,0);  // may be multiple scan replies                
  
  VPRINT "Disable LE scan\n");
  sendhci(lescanoff,0);
  statusok(0,lescanoff);  
   
     
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

      NPRINT "%d FOUND %s\n",count+1,baddstr(rp+2,1));      
     
      flushprint();
      
      type = BTYPE_LE;  // unless find mesh
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
                              (rp[j+3] == (((rp[5] ^ rp[6]) ^ rp[7]) ) | 0xC0 ) )
          {
          NPRINT "    Mesh device\n");
          type = BTYPE_ME;
          /*** print mesh data                                  
          for(k = 0 ; k < rp[j]-5 ; ++k)
            NPRINT " %02X",rp[j+6+k]);
          NPRINT "\n");  
          ***/
          
          }              
        else if(rp[j+1] == 8 || rp[j+1] == 9)  // found name 8=short 9=full
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
            NPRINT "    Name = %s\n",buf);
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
                NPRINT "    iBeacon =");
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
                }         
              k = 0;
              while(k < rp[j]-1)
                {
                if((tn == 6 || tn == 7) && k > 0 && (k & 0x0F) == 0)
                  NPRINT "\n       ");  // 16-byte UUIDs new line  
                NPRINT " %02X",rp[j+2+k]);
                ++k;
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
        
      // compare board address with known devices  no type check
              
  
      ndevice = devnfrombadd(rp+2,BTYPE_LE | BTYPE_ME,DIRN_REV);
     
      if(ndevice < 0 && (rp[1] & 1) != 0 && buf[0] != 0)
        {  // no board address match for random address and have name in buf
           // look for name match
        for(k = 1 ; ndevice < 0 && devok(k) != 0 ; ++k)
          {
          if(dev[k]->type == BTYPE_LE && (dev[k]->leaddtype & 1) != 0)
            {
            if(strcmp(buf,dev[k]->name) == 0)
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
        NPRINT "    Known device %d %s\n",dev[ndevice]->node,dev[ndevice]->name);
        if(dev[ndevice]->type == BTYPE_ME && dev[ndevice]->node >= 1000)
          NPRINT "    Add to devices.txt with node < 1000 to authorise mesh packets\n");
        if(dev[ndevice]->type != BTYPE_LE && dev[ndevice]->type != BTYPE_ME)
          NPRINT "    But not listed as LE or Mesh\n");
        }
      else
        {  // not already stored
        if(type == BTYPE_ME)  // auto detect mesh device
          {
          NPRINT "    SECURITY NOTE - To receive mesh packets from this device\n");
          NPRINT "                    add to devices.txt with node < 1000\n");
          }
        // find next free entry
        ndevice = devalloc();
        if(ndevice < 0)  // failed
          {
          instack[n] = INS_POP;
          return(0);
          }
        ++newcount;
        dp = dev[ndevice];
        dp->type = type;  // BTYPE_LE or BTYPE_ME
        dp->leaddtype |= rp[1] & 1;  // type public/random
        dp->leaddtype |= 2;          // found by scan
        dp->node = newnode();
        for(k = 0 ; k < 6 ; ++k)
          dp->baddr[k] = rp[7-k];
        
        if(buf[0] == 0)
          {  // no name
          if(type == BTYPE_ME)  // auto detect - security risk
            sprintf(buf,"Mesh node %d",dp->node);
          else
            {
            k = devnfrombadd(dp->baddr,BTYPE_CL,DIRN_FOR);
            if(k > 0 && bincmp(dev[k]->name,"Classic node 1",14,DIRN_FOR) == 0)
              strcpy(buf,dev[k]->name);  // is also a classic with name 
            else
              {
              sprintf(buf,"LE node %d",dp->node);
              NPRINT "    No name so set = %s\n",buf);
              }
            }
          }
        else
          {  // got name
          k = devnfrombadd(dp->baddr,BTYPE_CL,DIRN_FOR);
          if(k > 0 && bincmp(dev[k]->name,"Classic node 1",14,DIRN_FOR) != 0)
            strcpy(dev[k]->name,buf);  // is also a classic without name 
          }
          
        strcpy(dev[ndevice]->name,buf);
                      
        NPRINT "    New device %s\n",dev[ndevice]->name);
        }  
           
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
  
  ndevice = devnp(node);
  
  if(ndevice < 0)
    return(0);
    
  return(dev[ndevice]->type);
  }

  
int localnode()
  {
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
  

  if( (mask & BTYPE_SHORT) != 0)
    return(devlist(mask));

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
        NPRINT "%d  Local (%s)  ",dp->node,dp->name);        
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
            NPRINT "LE");    
          else if(con == NODE_CONN)
            NPRINT "NODE");
          }       
        }

          
      NPRINT "\n      %s",baddstr(dp->baddr,0));
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
  
  
int devlist(int mask)
  {
  int n,i,j,k,xn,count,flag,maxlen;
  unsigned char vn[NUMDEVS],len[NUMDEVS];
  char *s,buf[8];
  struct devdata *dp;
  
  maxlen = 0;
  count = 0;
  
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
      vn[count] = n;
      if(count < NUMDEVS-1)
        ++count;
      }
    }
  
  if(count == 0)
    return(0);         
   
 
  flag = 0;        // one column
  xn = count;      // last vn index + 1
 
  if(count > 5)
    {
    flag = 1;   // two column
    xn = (count+1)/2;    // last vn index + 1  of first column
         // find max length of first column       
    for(n = 0 ; n < xn ; ++n)
      {
      sprintf(buf,"%d",dev[vn[n]]->node);
      len[n] = strlen(buf);
      if(vn[n] == 0)
        len[n] += 5;  // Local
      else
        len[n] += strlen(dev[vn[n]]->name);
 
      if(len[n] > maxlen)
        maxlen = len[n];
      }
    }
    
  
  for(n = 0 ; n < xn ; ++n)
    {
    i = vn[n];
    if(i == 0)
      s = "Local";
    else
      s = dev[i]->name;

    NPRINT " %d - %s",dev[i]->node,s);

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
        NPRINT "%d - %s",dev[j]->node,dev[j]->name);
        }
      }    
         
    NPRINT "\n");
    flushprint();
    } 
  
  
  return(count);
  }  
 
 
/******** MESH SERVER **********/ 
  
void mesh_server(int(*callback)())
  {
  int nread,retval,clientnode;
  char buf[32];

  mesh_on(); 
  meshpacket(NULL);  // enable unknown device message
   
  NPRINT "Mesh server listening (x = stop server)\n");
  flushprint();
  do
    {
    nread = read_mesh(&clientnode,buf,32,EXIT_KEY,0);
    if(nread > 0)
      {
      retval = (*callback)(clientnode,buf,nread);
      }
    flushprint();
    }
  while(retval == SERVER_CONTINUE && read_error() == 0);

  if(read_error() == ERROR_KEY)
    NPRINT "Key press stop server\n");
  else if(read_error() == ERROR_FATAL)
    NPRINT "Fatal error stop server\n");
      
  flushprint();    
  }







/*********** LE SERVER ***********/


int le_server(int(*callback)(int clientnode,int operation,int cticn, void* pvParameter),int timerds, void *pvParameter)
  {
  int n,nread,key,ndevice,retval,timecount,oldkm,op,cticn;
  unsigned char ledat[2];
  struct devdata *dp;
  
  mesh_on();   
  oldkm = setkeymode(1); 
  
  NPRINT "Listening for LE clients to connect (x=stop server)\n");
  NPRINT "Advertising as %s\n",baddstr(dev[0]->baddr,0));
  flushprint();
  ndevice = 0;
  timecount = 0;
  retval = SERVER_CONTINUE;  
  do
    {
    readhci(LE_SERV,IN_LECMD,0,100,0);  
    n = findhci(IN_LECMD,0,INS_POP);
    if(n >= 0)
      {   
      ndevice = instack[n+3];
      dp = dev[ndevice];  
      op = insdatn[0];
      cticn = insdatn[1];
       
      if(op == LE_DISCONNECT)
        VPRINT "%s has disconnected\n",dp->name);
      else if(op == LE_CONNECT)
        {
        VPRINT "%s has connected\n",dp->name);
        mesh_on(); 
        }
        
      flushprint();
      popins();     
        
      if(callback != NULL)   
        retval = callback(dp->node,op,cticn, pvParameter);
     
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

    if(timerds > 0)
      {
      ++timecount;
      if(timecount >= timerds)
        {
        if(callback != NULL)
           retval = callback(localnode(),LE_TIMER,0,pvParameter);
        timecount = 0;
        }
      }
   
    flushprint();
    popins();
    key = readkey();
    }
  while(retval == SERVER_CONTINUE && key != 'x');
 
  setkeymode(oldkm);
  
  do
    {
    n = findhci(IN_LECMD,0,INS_POP);
    }
  while(n >= 0);
  
    
  if(key == 'x')
    NPRINT "Key press stop server...\n");

  if(ndevice != 0 && (dp->conflag & CON_LX) != 0)
    {
    NPRINT "LE Server disconnecting\n");
    disconnect_node(dp->node);
    }  

  flushprint();

  popins();    
  mesh_on();
  
  return(1);
  }  



  
  
/*********** NODE SERVER ***********/


int node_server(int clientnode,int (*callback)(int clientnode,char *buf,int count),char endchar)
  {
  int nread,key,ndevice,retval,oldkm;
  char buf[1024];
  struct devdata *dp;
     
  ndevice = devn(clientnode);
  if(ndevice <= 0 || dev[ndevice]->type != BTYPE_ME)
    {
    NPRINT "Invalid client node\n");
    flushprint();
    return(0);
    }
    
  dp = dev[ndevice];
  
  mesh_on();   
  oldkm = setkeymode(1);
  
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
      return(0);
      }
    }    
  
  
  popins();  
  setkeymode(oldkm);

  NPRINT "Connected OK\n",dp->name);
  NPRINT "Waiting for data from %s (x = stop server)\n",dp->name);

  do
    {
    nread = read_node_endchar(clientnode,buf,1024,endchar,EXIT_KEY,0);
    if(nread > 0)
      {
      retval = (*callback)(clientnode,buf,nread);
      }
    }
  while(retval == SERVER_CONTINUE && read_error() == 0);
  
  if(read_error() == ERROR_KEY)
    NPRINT "Key press stop server...\n");
  else if(read_error() == ERROR_FATAL)
    NPRINT "Fatal error stop server...\n");
      
  flushprint();  
  sleep(2);    // allow time for any last reply sent by callback to transmit      
  disconnect_node(clientnode);  // sever initiated here
                                // client should be running
                                // wait_for_disconnect
    
  mesh_on();
  
  return(1);
  }  
    
/********** CLASSIC SERVER ****************/



int classic_server(int clientnode,int (*callback)(int clientnode,char *buf,int count),char endchar,int keyflag)
  {
  int n,nread,key,ndevice,retval,oldkm,tryflag;
  char buf[1024];
  struct devdata *dp;
    
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

  dp->conflag = CON_SERVER;   
   
  dp->linkflag &= KEY_FILE | KEY_NEW;

  dp->linkflag |= keyflag & (KEY_ON | PASSKEY_LOCAL | PASSKEY_REMOTE | PASSKEY_OFF);  

  VPRINT "Set event mask\n");
  sendhci(eventmask,0);
  statusok(0,eventmask);

  VPRINT "Set simple pair mode on\n");
  sendhci(setspm,ndevice);
  statusok(0,setspm);

  flushprint();
  
  oldkm = setkeymode(1);
  tryflag = 0;
  
  while((dp->conflag & CON_RF) == 0)
    {     
    if(tryflag == 0)
      {
      NPRINT "Listening for %s to connect (x=cancel)\n",dp->name);
      flushprint();
      }
    retval = 0;
    do
      {     
      dp->conflag = CON_SERVER;
      readhci(ndevice,IN_CONREQ,0,750,0);
      n = findhci(IN_CONREQ,ndevice,INS_POP);
      if(n >= 0)
        {
        popins();
        retval = classicserverx(ndevice);
        flushprint();
        popins();
        }
      
      if(tryflag != 0 && (dp->conflag & CON_RF) == 0) 
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
    while((dp->conflag & CON_RF) == 0 && key != 'x' && key != 'k' && retval != 2);
    
    if((dp->conflag & CON_RF) == 0)
      {
      if(retval == 2)
        {
        if(dp->type == BTYPE_CL)
          tryflag = 1;
        dp->conflag = 0;
        NPRINT "%s disconnected\n",dp->name);
        flushprint();
        }       
      else
        tryflag = 0;
          

      if(key == 'k')
        {
        // flip KEY
        dp->linkflag ^= KEY_ON;
        if((dp->linkflag & KEY_ON) == 0)  
          NPRINT "Will not send a link key\n");
        else
          NPRINT "Will send a link key\n");
        flushprint();
        }

      if(key == 'x')
        {
        dp->conflag = 0;
        if(key == 'x')
          NPRINT "Cancelled by x press\n");
        flushprint();
        setkeymode(oldkm);
        popins();
        return(0);
        }
      }

    }    
  
  
  popins();  
  setkeymode(oldkm);

  NPRINT "Connected OK\n",dp->name);
  NPRINT "Waiting for data from %s (x = stop server)\n",dp->name);

  do
    {
    nread = read_node_endchar(clientnode,buf,1024,endchar,EXIT_KEY,0);
    if(nread > 0)
      {
      retval = (*callback)(clientnode,buf,nread);
      }
    }
  while(retval == SERVER_CONTINUE && read_error() == 0);
  
  if(retval != SERVER_EXIT)
    {
    if(read_error() == ERROR_KEY)
      NPRINT "Key press - stopping server\n");
    else if(read_error() == ERROR_FATAL)
      NPRINT "Fatal error - stopping server\n");
    else if(read_error() == ERROR_DISCONNECT)
      NPRINT "%s has disconnected - stopping server\n",dp->name);
    }
        
  flushprint();  
  sleep(2);    // allow time for any last reply sent by callback to transmit      
  disconnect_node(clientnode);  // sever initiated here
                                // client should be running
                                // wait_for_disconnect
      
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

  readhci(ndevice,IN_AUTOEND,0,10000,0); 
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

*****************************/

void register_serial(char *uuid,char *name)
  {
  if(uuid != NULL && name != NULL)
    replysdp(0,0,uuid,name);
  else
    NPRINT "NULL parameter\n");
  }
 
void replysdp(int ndevice,int in,char *uuid,char *name)
  {
  int n,sflag,uuidn,aid,aidlen,rn,uuidflag,aidj,aidk,ln,totlen;
  struct devdata *dp;
  unsigned char *ssarep,*des;
  unsigned char sdpreply[256];
    
  static char uuid1200[5] = { 0x35,0x03,0x19,0x12,0x00 };
  static char uuid0003[5] = { 0x35,0x03,0x19,0x00,0x03 };
  static char uuid1101[5] = { 0x35,0x03,0x19,0x11,0x01 };
  static char uuid0100[5] = { 0x35,0x03,0x19,0x01,0x00 };
  static char aidone[5] =   { 0x35,0x03,0x09 };
  static char aidrange[5] = { 0x35,0x05,0x0A };
   
static unsigned char ssaaidfail[32] = { 19,0,S2_HAND | S2_SDP,0,0x02,0x0B,0x20,0x0E,0x00,
0x0A,0x00,0x41,0x00,0x05,0x00,0x00,0x00,0x05,0x00,0x02,0x35,0x00,0x00 }; 
   // 03 reply with handle [21] = handle 
static unsigned char ssahandle[32] = { 23,0,S2_HAND | S2_SDP,0,0x02,0x0B,0x20,0x12,0x00,
0x0E,0x00,0x41,0x00,0x03,0x00,0x00,0x00,0x09,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00 }; 
static unsigned char ssahandle3[40] = { 31,0,S2_HAND | S2_SDP,0,0x02,0x0B,0x20,0x1A,0x00,
0x16,0x00,0x41,0x00,0x03,0x00,0x00,0x00,0x11,0x00,0x03,0x00,
0x03,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x03,0x00 };    
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
static unsigned char aid5[10] = { 8,
0x09,0x00,0x05,0x35,0x03,0x19,0x10,0x02 };
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
    else if(bincmp(des,uuid0003,5,DIRN_FOR) != 0 ||     
            bincmp(des,uuid0100,5,DIRN_FOR) != 0 )
      uuidflag = 4;  // 3 records     
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
      {  // handle specified  0/1/2/3
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
    else if(uuidflag == 4)
      ssarep = ssahandle3;  // 1,2 and 3 
    else
      {  
      ssarep = ssahandle; 
      ssahandle[PAKHEADSIZE+21] = uuidflag;  // 00010000-03
      }
    }
  else
    {  // 04/06 aid request
    if(uuidflag < 0 || aidj < 0 || aidj < 0)
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
      sdpreply[PAKHEADSIZE+16] = 0x35;
      sdpreply[PAKHEADSIZE+17] = 0;    
      if(insdat[in] == 0x06)
        rn += 2;
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
        
      if(uuidflag == 1 || uuidflag == 4)
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
      if(uuidflag == 1 || uuidflag == 2 || uuidflag == 4)
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
      if(uuidflag == 3 || uuidflag == 4)
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
  
      if(insdat[in] == 0x06)
        {
        sdpreply[PAKHEADSIZE+9] = 0x07;
        sdpreply[PAKHEADSIZE+17] = totlen;
        }
      else
        {
        sdpreply[PAKHEADSIZE+9] = 0x05;
        totlen = sdpreply[PAKHEADSIZE+17];
        }
               
      sdpreply[PAKHEADSIZE+rn] = 0;  // continue
        
      sdpreply[PAKHEADSIZE+15] = totlen+2;
      sdpreply[PAKHEADSIZE+13] = totlen+5;
      sdpreply[PAKHEADSIZE+5] = totlen+10;
      sdpreply[PAKHEADSIZE+3] = totlen+14;
      sdpreply[0] = totlen+19;
      }
    }
     
  flushprint();
  sendhci(ssarep,ndevice);  
  }

int addaid(char *sdp,char *aid,int *rn,int aidj,int aidk,int aidn)
  {
  int n;
  
  if(aidn < aidj || aidn > aidk)
    return(0);
    
  for(n = 0 ; n < aid[0] ; ++n)
    sdp[PAKHEADSIZE+*rn+n] = aid[n+1];
  *rn += aid[0];
  return(aid[0]);
  }

  
int bincmp(char *s,char *t,int count,int dirn)
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
 
     // ndevice checked
        
  dp = dev[ndevice];
  
     // if ndevice is BTYPE_LE - connect as LE client to LE server
     // if nedevce is BTYPE_ME - a Pi mesh device:
     //      dp->lecflag = 0  connect as node client to mesh device listening as node server
     //                    1  connect as LE client to mesh device listening as LE server
  
  if(dev[ndevice]->type == BTYPE_ME)
    mesh_on();

  VPRINT "SEND LE connect to %s\n",dp->name);
    
  sendhci(leopen,ndevice);
      
  readhci(ndevice,IN_LEHAND,0,5000,gpar.toshort);   
  
  if(dp->conflag != 0)
    {    // IN_LEHAND not saved to stack
    if(dp->type == BTYPE_LE || (dp->type == BTYPE_ME && dp->lecflag != 0))
      NPRINT "Connect OK as LE client\n"); 
    else    
      NPRINT "Connect OK as NODE client\n");
    VPRINT "Handle = %02X%02X\n",dp->dhandle[1],dp->dhandle[0]);
    if(dev[ndevice]->type != BTYPE_ME && gpar.leclientwait > 0)
      readhci(0,0,0,gpar.leclientwait,0);  // server may request attributes
    popins();
    flushprint();
    return(1);
    }      
  
  sendhci(lecancel,0);  // cancel open command
  statusok(1,lecancel);         // may also return IN_LEHAND with fail status
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
  unsigned int timstart;
 
  flushprint();
    
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);

  timstart = timems(TIM_LOCK);
  
  while(dev[ndevice]->conflag != 0 && timems(TIM_RUN) - timstart < timout)
    {
    readhci(0,0,0,25,0);  
    flushprint();
    }
   
  timems(TIM_FREE);
  
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
  
  flushprint();
  
  return(1);
  }
  

int closehci()
  {
  int retval;
  
  if(gpar.hci <= 0)
    {
    VPRINT "HCI not connected\n");
    retval = 0;
    }
  else
    {  
    VPRINT "HCI closed\n");
    close(gpar.hci);
    gpar.hci = -1;
    retval = 1;
    }
    
 
  flushprint();

   
  return(retval);
  }


void waitdis(int ndevice,unsigned int timout)
  {
  unsigned int timstart;
  struct devdata *dp;
 
  // wait for dp->conflag=0 set by readhci
  
  dp = dev[ndevice];
  timstart = timems(TIM_LOCK);
  
  while(dp->conflag != 0 && timems(TIM_RUN) - timstart < timout)
      readhci(0,0,0,20,0);   // sets conflag=0 on event 05
      
  timems(TIM_FREE);
  }
  

/********** CLOSE SOCKETS ***********/

void close_all()
  {
  int n;
  static int flag = 0;
  
  if(flag != 0)
    return;
    
  meshreadoff();   
  mesh_off();
         
  for(n = 0 ; devok(n) != 0 ; ++n)
    disconnectdev(n);  
    
  closehci();
  rwlinkey(1,0);
  
  flushprint();
  flag = 1;  // disable atexit call
  }  
  
  
/***********  WRITE CHARACTERISTIC *****************
device = device index in dev[ndevice]
cticn = characteristic index in dev[ndevice].ctic[cticn]
data = array of bytes to write - low byte first 
count=0 use stored size
*****************/

int write_ctic(int node,int cticn,unsigned char *data,int count)
  {
  return(writecticx(node,cticn,data,count,0,NULL));
  }


int notify_ctic(int node,int cticn,int notifyflag,int (*callback)())
  {
  unsigned char data[2];
   
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
  int n,k,chandle,locsize,ndevice;
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
      NPRINT "Characteristic has no notify permission\n");
      flushprint();
      return(0);
      }

    if(count == 0 || count > cp->size || notflag != 0)
      locsize = cp->size;  // known number of bytes in device info
    else
      locsize = count;

    if(locsize > LEDATLEN)
      locsize = LEDATLEN;

    if(notflag == NOTIFY_ENABLE)
      cp->notify = 1;
    else if(notflag == NOTIFY_DISABLE)
      cp->notify = 0;
    else
      {
      for(n = 0 ; n < locsize ; ++n)
        cp->value[n] = data[n];
      }

      
    for(n = 1 ; cp->notify != 0 && devok(n) != 0 ; ++n)
      {  // search all other devices for notify
      dp = dev[n];
      if((dp->conflag & CON_LX) != 0)
        {  // device n is connected as LE client
           // send notification
        VPRINT "Send %s notification to %s\n",cp->name,dp->name); 
        cmd = lenotify + PAKHEADSIZE;         
                          // set characteristic handle
        cmd[10] = (unsigned char)(cp->chandle & 0xFF);
        cmd[11] = (unsigned char)((cp->chandle >> 8) & 0xFF);
        for(k = 0 ; k < locsize ; ++k)     // set data
          cmd[12+k] = data[k];     // low byte first

        cmd[3] = (locsize+7) & 0xFF;
        cmd[4] = ((locsize+7) >> 8) & 0xFF;
  
        cmd[5] = (locsize+3) & 0xFF;
        cmd[6] = ((locsize+3) >> 8) & 0xFF;
  
        lenotify[0] = 12+locsize;  // 13 for 1 byte
        
        sendhci(lenotify,n);         
        }
      }
     
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
  
  if(notflag == 0 && (cp->perm & 0x0C) == 0)
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
    
  if(cp->size == 0 && count == 0)
    {
    NPRINT "Must specify byte count\n");
    flushprint();
    return(0);
    }
    
  if(count == 0)
    locsize = cp->size;  // known number of bytes in device info
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
      VPRINT "Enable notify/indicate for handle %04X\n",chandle);
      data[0] = 1;  // enable notify
      cp->callback = callback;
      } 
    else
      {
      VPRINT "Disable notify/indicate for handle %04X\n",chandle);
      data[0] = 0;
      cp->callback = NULL;
      }
    data[1] = 0;
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
  
  lewrite[0] = 12+locsize;  // 13 for 1 byte
  
  if((cp->perm & 8) == 0 || notflag != 0)   // no acknowledge 
    cmd[9] = 0x52;  // write command opcode
  else                      // acknowledge
    cmd[9] = 0x12;  // write request opcode

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
    
  if((cp->perm & 8) == 0 || notflag != 0)
    {   // no response or error reply
    readhci(0,0,0,25,0);  // peek event 13 or notify maybe
    popins();
    flushprint();   
    return(count);  // OK
    }

  readhci(ndevice,0,IN_ATTDAT,gpar.timout,gpar.toshort);     // 2 replies if acknowledge  
  n = findhci(IN_ATTDAT,ndevice,INS_POP);  
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
      NPRINT "  Error %d %s\n",insdatn[4],errorle[k]);
      popins();
      flushprint();
      return(0); 
      }  
    }
  
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
 
  gpar.readerror = ERROR_FATAL;
 
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);

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
       
  readhci(ndevice,IN_ATTDAT,0,gpar.timout,gpar.toshort);
      
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
            cp->size = retval;
          }
                
        if(retval > datlen-1)
          {
          NPRINT "  Need larger buffer in read_ctic\n");
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
  int n,n0,nwrit,ntogo,len,chan,errflag;
  unsigned int timstart;
  unsigned char *cmd;
  struct devdata *dp;
  char *ms;
    
  cmd = s + PAKHEADSIZE;   // skip over header bytes to command 
  len = s[0] + (s[1] << 8);        // length of cmd from header

    
  // s[2/3] = flags to set info in cmd string
 
  errflag = 0;
  
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
        VPRINT "< L2CAP 0001 Opcode:id = %02X:%02X   ",cmd[9],cmd[10]);
        if(cmd[9] == 2 || cmd[9] == 4 || cmd[9] == 6 || cmd[9] == 10)
          VPRINT "Expects reply %02X:%02X\n",cmd[9]+1,cmd[10]);
        else if(cmd[9] == 3 || cmd[9] == 5 || cmd[9] == 7  || cmd[9] == 11)
          VPRINT "is reply to %02X:%02X\n",cmd[9]-1,cmd[10]);
        else
         VPRINT "\n");              
        }
      else if(chan == 4)
        VPRINT "< L2CAP 0004 Opcode = %02X\n",cmd[9]);       
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
        VPRINT "< ? Opcode %02X\n",cmd[9]);
      }
    else if(cmd[0] == 1)
      VPRINT "< HCI OGF=%02X OCF=%02X\n",(cmd[2] >> 2) & 0x3F,((cmd[2] << 8) & 0x300) + cmd[1]);
    else 
      VPRINT "< Unknown");
      
    hexdump(cmd,len);
    }  // end printflag
    
  ntogo = len;  // first header entry is length of cmd
  timstart = timems(TIM_LOCK);  
  do
    {
    nwrit = write(gpar.hci,cmd,ntogo);
    
    if(nwrit > 0)
      {
      ntogo -= nwrit;
      s += nwrit;
      }   
    if(timems(TIM_RUN) - timstart > 2000)   // 2 sec timeout
      {
      NPRINT "Send CMD timeout\n");
      timems(TIM_FREE);
      return(0);
      }
    }
  while(ntogo != 0);
  timems(TIM_FREE);

  return(1);
  }


/******* STATUS OK ********
look for standard status OK reply
flag=0 expect status reply only
     1 may be more replies to ditch

return 1 = seen status OK reply
       0 = not seen reply or status not OK
*********************/

     
int statusok(int flag,char *cmd)
  {
  int n,retval,repflag;
  
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
  unsigned char b0,b3,*datp,ledat[2];   
  int len,blen,wantlen,xwantlen,add,doneflag,crflag,disflag,xdevicen,lesflag,eflag;
  int gotn,k,j,n0,nxx,chan,mask,xflag,xprintflag,devicen,stopverb,firstpacket;
  int conreqflag,conreqid,disreqflag,retval,timendms,savtimendms,datlen,ascflag;
  long long int locmustflag,gotflag;
  unsigned int timstart,timx;
  struct devdata *dp,*condp;
  static long long int sflag;
  static int level = 0;   
  unsigned char buf[2048];
  
  lesflag = ndevice & LE_SERV;
  ndevice &= 0xFF;

  if(ndevice != 0 && (dev[ndevice]->conflag & CON_LX) != 0)
    lesflag = 1;   // mesh device is LE server
    

  if(level > 8)
    {
    NPRINT "ERROR - Notify callback has spawned too many nested reads\n"); 
    flushprint();
    return(0);
    }
         
  doneflag = 0;
  timendms = timout;
   
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
    
  for(k = 0 ; devok(k) != 0 ; ++k)
    dev[k]->nx = -1;  // extra data
             
  timstart = timems(TIM_LOCK);         
   
  
  gotn = 0;   // number of reply
  devicen = 0; 
  blen = 0;         // existing buffer length
  wantlen = 8192;   // expected messaage length - new message flag
  xwantlen = 0; 
  xflag = 0;
  xprintflag = 0;
  conreqflag = 0;
  disreqflag = 0;
  disflag = 0;
  
  do   
    {
    if(wantlen == 8192 && blen == 0 && xflag == 0)
      {
      timx = timems(TIM_RUN);
      immediate(lookflag | locmustflag);
      timendms += timems(TIM_RUN) - timx;  // ignore time spent in immediate
      
      if(locmustflag != 0 && findhci(mustflag,ndevice,INS_NOPOP) >= 0) 
        {
        locmustflag = 0;
        doneflag = 1;
        timendms = toshort;
        timstart = timems(TIM_RUN);
        }
      }
      
    // next message may loop with data in buf
    do     // wait for complete message
      { 
      if(blen != 0 && wantlen == 8192)   // find expected messaage length
        {
        b0 = buf[0];
        if(!(b0==1 || b0==2 || b0==4))
          {   
          NPRINT "Unknown reply type\n");
          // clear reads and exit
          hexdump(buf,blen);
                        
          timstart = timems(TIM_RUN);  // start timer
          timendms = toshort;
          do
            {
            len = read(gpar.hci,buf,sizeof(buf));
          
            if(len > 0)  // restart timeout - still toshort
              {
              timstart = timems(TIM_RUN);    // restart timer
              }          
            }
          while(timems(TIM_RUN) - timstart < timendms);
   
          flushprint();
          --level;
          
          timems(TIM_FREE);
          return(0);
          }
          
        if(b0 == 1 && blen > 3)
          wantlen = buf[3] + 4;
        else if(b0 == 2 && blen > 4)
          wantlen = buf[3] + (buf[4] << 8) + 5;        
        else if(b0 == 4 && blen > 2)
          wantlen = buf[2] + 3;
        }
       
      if(blen < wantlen)   // need more or short TO to exit        
        {         
        do       // read block of data - may be less than or more than one line
          {
          len = read(gpar.hci,&buf[blen],sizeof(buf)-blen);                  
           
           // len = number of bytes read  0=EOF -1=error     
         
          if(len <= 0 && (timendms == 0 || (timems(TIM_RUN) - timstart) >= timendms))  
            {    // nothing read and timed out - normal exit route                         
            if(len > 0 || blen > 0)
              NPRINT "Exit with partial reply\n");

            
            if(mustflag == 0 || doneflag != 0 || (disflag != 0 && ndevice == devicen) )
              {
              retval = 1;  // OK normal exit or waiting for packet from disconnected device
              }
            else
              {
              if(mustflag != IN_DATA && mustflag != IN_CONREQ && lesflag == 0)
                VPRINT "Timed out waiting for expected packet\n"); 
 
              retval = 0;
              }
            flushprint();
            popins();
            
       
            --level;
            
            timems(TIM_FREE);
            return(retval);     
            }
          }
        while(len <= 0);  // want to exit if len=0 end of file

        if(doneflag != 0)
          {  // want quick exit but more coming - reset short TO
          // NPRINT "Reset TO 2\n");   
          timstart = timems(TIM_RUN);  // restart timer
          timendms = toshort;   // x 1024 ms to us     
          }
        blen += len;  // new length of buffer
        }                 
      }    
    while(blen < wantlen);
    
    gotflag = 0;  // no save to instack
    crflag = 0;   // no print credit decrement
    ascflag = 0;    // not serial data for ascii print
    disflag = 0;   // unexpected disconnect
    stopverb = 0;   // stop verbose le scan 
    chan = 0;
    xprintflag = 0;
    devicen = 0;      // sending device unknown
  
    if(buf[0] == 4)    // HCI events
      {
      n0 = 0;  // offset of board address or handle for device identify
      if(buf[1] == 0x3E && buf[3] == 1) 
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
      else if( (sflag & IN_BADD) != 0 && buf[1] == 0x0E && buf[4] == 0x09 && buf[5] == 0x10)   
          gotflag = IN_BADD;           // command complete with board address if buf[6]=0                   
      else if( (sflag & IN_STATOK) != 0 && buf[1] == 0x0E)   //  && buf[6] == 0 )      
         gotflag = IN_STATOK;            // command complete Status=buf[6]
      else if(buf[1] == 3)    //  (sflag & IN_CLHAND) != 0 && buf[3] == 0)  // status = 0   
        {  
        gotflag = IN_CLHAND;
        n0 = 6;
        } 
      else if( (sflag & IN_CONREQ) != 0 && buf[1] == 0x04)
        {
        gotflag = IN_CONREQ;
        n0 = 3;
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
      else if( (sflag & IN_ENCR) != 0 && buf[1] == 0x08 && buf[3] == 0 )  // status=0   
        {
        gotflag = IN_ENCR;
        n0 = 4 | 0x80;   // handle not board add
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
     
       
        // find sending devicen  
       
      if((n0 & 0x80) != 0)
        {  // handle at n0
        n0 &= 0x7F;
        for(k = 1 ; devicen == 0 && devok(k) != 0 ; ++k)
          {
          if( dev[k]->type && (BTYPE_CL | BTYPE_LE | BTYPE_ME) != 0 && dev[k]->conflag != 0)
            {        // hi 4 bits of buf[2] = flags
            if(dev[k]->dhandle[0] == buf[n0] && dev[k]->dhandle[1] == (buf[n0+1] & 15))
              devicen = k;
            }
          }
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
          if((sflag & IN_LEHAND) != 0 || (sflag & IN_LECMD) != 0)
            {  // is waiting for this connection
            if(lesflag != 0)
              {  // LE server accepts any client
              if(devicen == 0)
                {  // unknown     
                devicen = devalloc();
                if(devicen > 0)
                  {            
                  dp = dev[devicen];
                  dp->type = BTYPE_LE; 
                  dp->node = newnode();          
                             
                  for(k = 0 ; k < 6 ; ++k)
                    dp->baddr[k] = buf[14-k];
        
                  strcpy(dev[devicen]->name,baddstr(dp->baddr,0));
                  }
                }                                 
              if(devicen != 0)
                {
                ledat[0] = LE_CONNECT;
                ledat[1] = (unsigned char)devicen;
                pushins(IN_LECMD,devicen,2,ledat);
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

            if(lesflag != 0)
              dp->conflag = CON_LX;   // LE server       
            else if(dp->type == BTYPE_LE || (dp->type == BTYPE_ME && dp->lecflag != 0))
              dp->conflag = CON_LE;   // LE connected as LE
            else
              dp->conflag = CON_MESH;  // LE connected as mesh device
            doneflag = 1;  
            timstart = timems(TIM_RUN);
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
          
        if((gpar.meshflag & MESH_W) != 0)
          mesh_on();
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
            timstart = timems(TIM_RUN);
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
        nxx = pushins(gotflag,devicen,buf[2],&buf[3]);
           
      if(disflag != 0 && devicen != 0)
        {
        dp = dev[devicen];   
            
        if(dp->conflag != 0 && !((dp->conflag & CON_RF) == 0 && (dp->conflag & CON_SERVER) != 0))    
          NPRINT "%s has disconnected\n",dev[devicen]->name);
        if((dp->conflag & CON_MESH) != 0)
          mesh_on();
        dp->conflag = 0;
        dp->lecflag = 0;
        if(lesflag != 0)
          {
          ledat[0] = LE_DISCONNECT;
          ledat[1] = 0;
          pushins(IN_LECMD,devicen,2,ledat);
          }
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

      if(dev[devicen]->nx != -1)
        {
        gotflag = IN_DATA;   // extra LE data
        firstpacket = 0;
        }
      else
        {   // first packet
        firstpacket = 1;        
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
            }
          else if(dev[devicen]->type == BTYPE_ME && (dev[devicen]->conflag & CON_LE) == 0)
            gotflag = IN_DATA;    // mesh data
          else
            {
            gotflag = IN_ATTDAT;  // LE
            
            if((dev[devicen]->conflag & CON_LE) != 0 && (buf[9] & 1) == 0 && buf[9] <= 0x20)
              {  // even opcode = request from server - let odd opcodes go to ATTDAT 
              gotflag = 0;  // ditch
              if(buf[9] == 0x02)
                {  // MTU exhange
                VPRINT "SEND MTU exchange reply\n");
                sendhci(lemtu,devicen);
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
          }          
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
          else if(buf[10] == 0xEF && buf[9] == 0x03 && (buf[12] == 0xE1 || buf[12] == 0xE3))
            gotflag = IN_MSC | IN_IMMED;
          else if( (buf[10] & 0xEF) == 0xEF && add != 0)   // EF or FF
            gotflag = IN_DATA;    
          else if(buf[10] == 0x53)
            gotflag = IN_DISCH | IN_IMMED;  // close CONTROL/RFCOMM            
          }
        }  // end not extra
   
      nxx = -2;   // stack index
          
      if(gotflag != 0)
        {        
        if(gotflag == IN_DATA)
          {
          if(firstpacket == 0)   // extra data
            {
            // add to existing stack entry
            datlen = buf[3] + (buf[4] << 8);  // length
            VPRINT "Following packet [3][4] = %04X extra bytes from [5]..\n",datlen);
            datp = buf+5;
            xwantlen -= datlen;
            nxx = dev[devicen]->nx;
            if(xwantlen <= 0 && datlen > 0 && nxx >= 0 && ((long long int)1 << instack[nxx]) == IN_DATA && (dev[devicen]->conflag & CON_RF) != 0)
              --datlen;  // last packet of serial data - not FCS  
            if(nxx >= 0 && datlen > 0)
              dev[devicen]->nx = addins(nxx,datlen,datp);  // nx may change
            if(xwantlen <= 0)
              {
              --xflag;  // got all extra bytes
              dev[devicen]->nx = -1;
              if(xflag == 0)
                timendms = savtimendms;
              }  
            xprintflag = 2;         
            ascflag = 1;
            }
          else if(chan == 4)   // ATT LE node first data
            {
            datlen = buf[3] + (buf[4] << 8) - 4;
            datp = buf+9;
            nxx = pushins(gotflag,devicen,datlen,datp);
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
                --condp->credits;
                crflag = 1;
                }
              if(buf[5] + (buf[6] << 8) + 9 - wantlen != 0)
                {  // multiple packets - this packet less than datlen
                datlen = wantlen - 12 - k; 
                }
              if(datlen > 0)
                nxx = pushins(gotflag,devicen,datlen,datp);
              ascflag = 1;  // may print ascii with hexdump          
              }
            }
          }   // end IN_DATA 
        else
          {   // normal push
          nxx = pushins(gotflag,devicen,buf[3] + (buf[4] << 8) - 4,buf+9);
          }
        }  // end gotflag
     
      if(firstpacket != 0)
        {  // first packet   
        xwantlen = buf[5] + (buf[6] << 8) + 9 - wantlen;  // extra needed in next messages
        if(xwantlen > 0)
          {  // need more
          dev[devicen]->nx = nxx;  // may be -2 - data read but not stored
          if(xflag == 0)
            {
            savtimendms = timendms;  // restore when got entire packet
            timendms += 500;         // more time       
            }
          ++xflag;
          }
        }  
                  
      }  // end 02 packet   
  
    if(ndevice == 0 || (ndevice != 0 && devicen != 0 && ndevice == devicen))      
      locmustflag &= ~gotflag;  // zero bit - got this one - device must match
                               
    ++gotn;  // reply count  
       
                   
    if(gpar.printflag == PRINT_VERBOSE && stopverb == 0)
      {         // only print le scan reply gotflag == LE_SCAN or meshflag = 1
    //  igflag = 0;
      b0 = buf[0];

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
        for(k = 3 ; k < wantlen && k < 13 ; ++k)
          VPRINT " %02X",buf[k]);
        if(k == wantlen)
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
        }
      else if(b0 == 2)
        {     
        if(chan == 1)
          {
          VPRINT "> L2CAP 0001 Opcode:id = %02X:%02X",buf[9],buf[10]);
          if(buf[9] == 2 || buf[9] == 4 || buf[9] == 6 || buf[9] == 10)
            VPRINT "  Must send reply %02X:%02X\n",buf[9]+1,buf[10]);
          else if(buf[9] == 3 || buf[9] == 5 || buf[9] == 7  || buf[9] == 11)
            VPRINT "  is reply from %02X:%02X\n",buf[9]-1,buf[10]);
          else
           VPRINT "\n");
          }
        else if(chan == 4)
          {
          VPRINT "> L2CAP 0004 Opcode = %02X\n",buf[9]);
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
        else
          {
          VPRINT "> L2CAP %04X ? = ",chan);
          for(k = 9 ; k < wantlen && k < 20 ; ++k)
            VPRINT " %02X",buf[k]);
          if(k == wantlen)
            VPRINT "\n");
          else
            VPRINT "...\n");
          }
        }
      else
        VPRINT "> Unknown\n");
    
      hexdump(buf,wantlen);
      if(ascflag != 0)   // serial read data - print if all ascii
        printascii(datp,datlen);
      
    
      if(xflag != 0)
        {
        VPRINT "Need an extra %04X bytes\n",xwantlen);
        }
      }   // end print
    

    // top up credits
    if(crflag != 0)
      {
      VPRINT "  Has used a credit. Number remaining = %d\n",condp->credits);  
      if(condp->credits < 3)
        setcredits(devicen);   // top up credits
      } 


    if(blen == wantlen)
      {    // have got exact message length
      blen = 0;
      }
    else
      {   // have got part of next packet as well
          // wipe last packet length wantlen - copy next down
          // starts at buf[wantlen] ends at buf[blen-1]
          // copy to buf[0]             
      for(k = wantlen ; k < blen ; ++k)
        buf[k-wantlen] = buf[k];       
      blen -= wantlen;
      }

    wantlen = 8192;  // new message flag
       
       
    if((xflag == 0 && doneflag == 0 && mustflag != 0 && locmustflag == 0) ||
       (disflag != 0 && ndevice == devicen)  )
      {  // done - switch to short timeout
         // or waiting for packet from device that has disconnected unexpectedly 
      doneflag = 1;  
      timstart = timems(TIM_RUN);
      timendms = toshort;
      }
    flushprint();        
    }
  while(1);  
   
  return(1);
  } 

void immediate(long long lookflag)
  {
  int n,k,j,devicen,id,ch,psm;
  int bn,getout,cticn,chandle;
  unsigned char buf[16];
  long long int gotflag;
  struct devdata *dp;
  struct cticdata *cp;
      
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
    
    gotflag = (long long int)1 << (instack[n] & 0x3F);
    devicen = instack[n+3];
    dp = dev[devicen];
    
    
    if(gotflag == IN_ATTDAT && (dp->conflag & CON_LX) != 0)
      {  
      leserver(devicen,instack[n+1]+(instack[n+2] << 8),insdat+n);
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
      VPRINT "GOT MSC %02X\n",j);
      VPRINT "SEND MSC reply\n");
      msccmdrsp[PAKHEADSIZE+9] = 0x01;  // reply
      msccmdrsp[PAKHEADSIZE+12] = j;  // E3 or E1
      sendhci(msccmdrsp,devicen);
      if((lookflag & IN_AUTOEND) != 0 && j == 0xE1)
        {
        buf[0] = AUTO_MSC;
        pushins(IN_AUTOEND,devicen,1,buf);
        }
      }
    else if(gotflag == IN_L2ASKCT)
      {
      psm = insdat[n+4];
      if((dp->conflag & CON_SERVER) == 0 ||
       !( (psm == 1 && (dp->conflag & CON_PSM1) == 0) ||
          (psm == 3 && (dp->conflag & CON_PSM3) == 0) ) )
        {  // only allow server psm 1/3 
        VPRINT "  GOT L2 connect request. Fob it off\n");
        sendhci(foboff,devicen);
        }
      else
        {
        dp->id = insdat[n+1];  // ID from request
   
        VPRINT "GOT L2 connect request psm %d channel %02X%02X\n",insdat[n+4],insdat[n+7],insdat[n+6]);  
  
        if(psm == 1)   // psm 1 for SDP
          {
          dp->psm = 1;
          dp->scid[2] = 0x41;  // psm 1 for SSA request
          dp->scid[3] = 0x00; 
          dp->dcid[2] = insdat[n+6];
          dp->dcid[3] = insdat[n+7];
          }
        else  // psm 3 for RFCOMM
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
      if((dp->linkflag & PASSKEY_OFF) != 0)
        j = 3;   // no i/o 
      else if((dp->linkflag & PASSKEY_REMOTE) != 0)
        j = 2;  // remote prints passkey - keyboard enter here 
      else
        j = 1;  // display y/n prints passkey here PASSKEY_LOCAL 
      
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
        printf ("Input PIN code (can set in device info via PIN=)\n? ");
        j = setkeymode(0);
        do
          {
          fgets(dp->pincode,64,stdin);
          }
        while(dp->pincode[0] == 10);
        setkeymode(j); 
        j = 0;
        while(j < 63 && dp->pincode[j] != 0 && dp->pincode[j] != 10)
          ++j;
        dp->pincode[j] = 0;
        }
      else
        {
        NPRINT "Using PIN=%s from device info\n",dp->pincode);
        j = strlen(dp->pincode);
        }
        
      pincode[PAKHEADSIZE+10] = j;   
      strcpy(pincode+PAKHEADSIZE+11,dp->pincode);
      VPRINT "SEND PIN code\n");
      VPRINT "  Set [10] PIN length\n");
      VPRINT "  Set [11] PIN = %s\n",dp->pincode);
     
      sendhci(pincode,devicen);
      }     
    else if(gotflag == IN_PASSREQ)
      {      
      VPRINT "GOT passkey request (Event 34)\n");
      flushprint();
      printf("Input passkey displayed on remote device\n? ");
      j = setkeymode(0);
      do
        {
        fgets(buf,16,stdin);
        }
      while(buf[0] == 10);
      setkeymode(j); 

      j = atoi(buf);
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
   
      printf("Passkey = %d  Valid for 10 seconds\n",insdat[n+6] + (insdat[n+7] << 8) + (insdat[n+8] << 16));
       
       
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
        VPRINT "%s %s notify =",dp->name,cp->name);
        for(j = 0 ; j < bn ; ++j)
          VPRINT " %02X",insdat[n+3+j]);
        VPRINT "\n");
        if(cp->callback != NULL)
          (*cp->callback)(dp->node,cticn,insdat+n+3,bn);
        }
      }
    else 
      VPRINT "Unrecognised immediate\n");
                      
    instack[n] = INS_POP;
    }

 
    
  flushprint();
  }



/******* LE SERVER ************

handles 1-3
opcode 05/09/11 packet data returned in reply to reuqest opcodes 04/08/10
eog = end of group handle inserted in 11 reply as 4th/5th bytes

handle  opcode 05 reply     opcode 09/11 reply  (first byte = number of following bytes)            
0001    01 01 00 00 28      04 01 00 00 18        empty service UUID=2800  valueUUID=1800 Generic Access  eog = 0001
0002    01 02 00 00 28      04 02 00 01 18        empty service UUID=2800  valueUUID=1801 Generic Attribute  eog = 0002
0003    01 03 00 00 28      12 03 00 FF...11      private characteristic services UUID=2800 eog = FFFF
                                                  value UUID = 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF FF              
handles 0004...  characteristics

***********************/


void leserver(int ndevice,int count,unsigned char *dat)
  {
  int n,dn,cticn,flag,notflag,handle,node,start,end,startx,size,uuidtype;
  unsigned char cmd[2],*s,errcode;
  struct cticdata *cp;

  
  VPRINT "GOT LE server opcode %02X from %s\n",dat[0],dev[ndevice]->name); 
  flushprint();

  cticn = 0;
  errcode = 0;  
  node = dev[0]->node;
  
  if(dat[0] == 0x52 || dat[0] == 0x12 || dat[0] == 0x0A)
    {  // read/write
    flag = 0;      
    handle = dat[1] + (dat[2] << 8);

    // find cticn of handle
  
    for(cticn = 0 ; ctic(0,cticn)->type == CTIC_ACTIVE && flag == 0 ; ++cticn)
      {
      cp = ctic(0,cticn);
           
      if(cp->chandle == handle || ((cp->perm & 0x30) != 0 && handle == cp->chandle+1) )
        {    
        if(cp->chandle == handle)
          notflag = 0;
        else
          notflag = 1;

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
              write_ctic(node,cticn,dat+3,count-3);
              if(dat[0] == 0x12)
                {  // no check cp->perm & 8 write with ack
                VPRINT "Send acknowledgement\n");
                sendhci(leack,ndevice);  // send ack
                }
              cmd[0] = LE_WRITE;
              }
            }
          else  // notify descriptor
            {
            cp->notify = dat[3];
            if(cp->notify == 0)
              VPRINT "%s notify disable\n",cp->name);
            else
              VPRINT "%s notify enable\n",cp->name);
            if(dat[0] == 0x12)
              {
              VPRINT "Send acknowledgement\n");
              sendhci(leack,ndevice);  // send ack
              }
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
              n = 0;
              while(n < cp->size && n < LEDATLEN)
                {
                lereadreply[PAKHEADSIZE+10+n] = cp->value[n];
                ++n;
                }
              size = n;
              cmd[0] = LE_READ;
              }
            }  
          else   // notify descriptor
            {
            size = 2;
            lereadreply[PAKHEADSIZE+10] = cp->notify;
            lereadreply[PAKHEADSIZE+11] = 0;
            }
            
          if(errcode == 0)
            {            
            lereadreply[0] = 10 + size;
            lereadreply[PAKHEADSIZE+3] = size+5;
            lereadreply[PAKHEADSIZE+5] = size+1;
            VPRINT "SEND characteristic %s\n",cp->name);
            sendhci(lereadreply,ndevice);
            }          
          }
        if(errcode == 0 && notflag == 0)
          {
          cmd[1] = cticn;
          pushins(IN_LECMD,ndevice,2,cmd);
          }   
        flag = 1;
        }
      }
    if(flag == 0)
      {
      NPRINT "%s trying to read/write invalid handle %04X\n",dev[ndevice]->name,handle);
      errcode = 1;
      }
      
    if(errcode != 0 && dat[0] == 0x52)
      errcode = 0;   // no error return for 52
    }
  else if(dat[0] == 0x04)
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
    if(start >= 1 && start <= 3)
      {  // handles 1-5  2 byte UUID
      s[11] = dat[1];  // handle
      s[12] = dat[2];
      s[13] = 0;
      s[14] = 0x28;  
      flag = 1;
      }
    else if(start > 3)
      {    
      cticn = nextctichandle(start,end,&handle);
      if(cticn >= 0)
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
        else  // value uuid
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
        s[11] = dat[1];  // handle
        s[12] = dat[2];    
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
  else if(dat[0] == 0x08 || dat[0] == 0x10)
    { 
    flag = 0;
    start = dat[1]+(dat[2] << 8);
    end = dat[3]+(dat[4] << 8);
    uuidtype = count - 5;
    VPRINT "Read attribute info for handles %04X to %04X and\n  UUID ",start,end);
    for(n = 0 ; n < uuidtype ; ++n)
      VPRINT "%02X",dat[uuidtype+4-n]);
    VPRINT "\n");
  
    s = le09replyv+PAKHEADSIZE;       
       
    if(dat[0] == 0x08)
      dn = 0;
    else
      {
      dn = 2;   // insert end of group handle
      s[13] = 0xFF;
      s[14] = 0xFF;
      }
      
    if((start == 1 || start == 2) && dat[5] == 0 && dat[6] == 0x28)
      {
      handle = start;       
      size = 2;
      s[14+dn] = 0x18;
      if(start == 1)
        {   // 1800
        s[13+dn] = 0;
        if(dn != 0)  // 0x10 request
          {
          s[13] = start;  // end of group - this handle 
          s[14] = 0;         
          }
        }
      else
        {  // 1801
        s[13+dn] = 1;
        if(dn != 0)
          {
          s[13] = start; // end of group = this handle
          s[14] = 0;
          }
        }
      flag = 1;
      }             
    else if(start == 3 && dat[5] == 0 && dat[6] == 0x28)
      {   // ps entry
      handle = 3; 
      size = 16;
      for(n = 0 ; n < 16 ; ++n)
        s[28-n+dn] = baseuuid[n];
      // 10 eog = FFFF     
      flag = 1;
      }
    else
      {  // characteristics 
      startx = start;
      do
        {
        cticn = nextctichandle(startx,end,&handle);
        if(cticn >= 0)
          {
          cp = ctic(0,cticn);
          if(uuidtype == 2 && handle == cp->chandle-1 && dat[5] == 0x03 && dat[6] == 0x28)
            {   // info 2803
            size = cp->uuidtype + 3;  // beyond 09 len handlo handhi
            s[13+dn] = cp->perm;
            s[14+dn] = cp->chandle & 0xFF;  // value handle
            s[15+dn] = cp->chandle >> 8;
            for(n = 0 ; n < cp->uuidtype ; ++n)
              s[n+dn+16] = cp->uuid[cp->uuidtype-n-1];
            if(dn != 0)
              {  // end of group = value handle or +1 if notify
              n = cp->chandle;
              if((cp->perm & 0x30) != 0)
                ++n; 
              s[13] = n & 0xFF;
              s[14] = n >> 8;
              }
            flag = 1;
            }
          else if(uuidtype == 2 && handle == cp->chandle+1 && dat[5] == 0x02 && dat[6] == 0x29)
            { // notify control 2902
            size = 2;
            s[13+dn] = cp->notify; 
            s[14+dn] = 0;
            flag = 1;
            // 10 eog = FFFF
            }
          else if(uuidtype == cp->uuidtype && handle == cp->chandle &&
                                        bincmp(cp->uuid,dat+5,cp->uuidtype,DIRN_REV) != 0)  
            {   // value
            size = cp->size;
            if(size > LEDATLEN)
              size = LEDATLEN;
            for(n = 0 ; n < size ; ++n)
              s[13+n+dn] = cp->value[n];
            flag = 1; 
            // 10 eog = FFFF
            }
          startx = handle + 1;
          }
        }
      while(flag == 0 && cticn >= 0 && startx <= end);           
      }

    
    if(flag == 0)
      {
      VPRINT "Attribute/UUID not found\n");
      errcode = 0x0A; // att not found
      }
    else
      {
      if(dat[0] == 0x08)
        s[9] = 0x09;  // 08 reply
      else
        {
        s[9] = 0x11;   // 10 reply
        size += 2;
        }
      s[11] = handle & 0xFF;
      s[12] = handle >> 8;
      s[10] = size + 2;
      s[5] = size + 4;
      s[3] = size + 8;
      le09replyv[0] = size + 13;
      
      VPRINT "SEND reply opcode %02X for handle %02X%02X\n",s[9],s[12],s[11]);
      sendhci(le09replyv,ndevice);      
      }  
    } 
  else if(dat[0] == 0x02)
    {  // MTU exhange
    VPRINT "SEND MTU exchange reply\n");
    sendhci(lemtu,ndevice);
    }  
  else
    { // request not supported
    VPRINT "Opcode not supported\n");
    if(dat[0] <= 0x20)
      errcode = 0x06;
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
  }


int nextctichandle(int start,int end,int *handle)
  {
  int n,cticn,minhandle,del,notdel;
  struct cticdata *cp;
  
  *handle = 0;
  minhandle = 0xFFFF;
  cticn = -1;
  // find lowest handle between start and end
  for(n = 0 ; ctic(0,n)->type == CTIC_ACTIVE  ; ++n)
    {
    cp = ctic(0,n);
    notdel = 0;
    if((cp->perm & 0x30) != 0)
      notdel = 1;  // include next handle notify control
    for(del = -1 ; del <= notdel ; ++del)
      {
      if(cp->chandle+del >= start && cp->chandle+del <= end)
        {
        if(cp->chandle+del < minhandle)
          {
          minhandle = cp->chandle+del;
          cticn = n;
          *handle = minhandle;
          }
        }
      }  
    }
  return(cticn);
  }

void rwlinkey(int rwflag,int ndevice)
  {
  int n,k,j,i,addcount,flag;
  unsigned char *badd,*key;
  struct devdata *dp;
  FILE *stream;
  static char fname[256]; 
  static int count = -1;
  static int delflag = 0;
  static char *table;

  if(count < 0)
    {  // first read
    if(rwflag != 0)
      return; 
    n = readlink("/proc/self/exe",fname,256);
    flag = 0;
    if(n > 2)
      {    
      --n; 
      while(n > 0 && fname[n] != '/')
        --n;
      if(n >= 0 && fname[n] == '/')
        {
        fname[n+1] = 0;
        flag = 1;
        }
      }
    if(flag == 0)
      fname[0] = 0;
    strcat(fname,"link.key");  
    }
        
  if(rwflag == 0)
    {  // read
    if(count < 0)
      {
      count = 0;
      stream = fopen(fname,"rb");
      if(stream == NULL)
        return;
 
      n = 0;
      count = fgetc(stream);
      k = count*22;
      table = (char *)malloc(k);

      if(table == NULL)
        n = 1;
      else if(fread(table,1,k,stream) != k)
        n = 1;

      fclose(stream);
      if(n != 0)
        {
        NPRINT "Read link.key file failed\n");
        return;
        }
      }
     
    for(k = 0 ; k < count ; ++k)
      {
      badd = table + k*22;
      key = badd+6;
      n = devnfrombadd(badd,BTYPE_CL,DIRN_FOR);
      if( (ndevice == 0 && n > 0) || (ndevice > 0 && n == ndevice) )
        {  // all on init (ndevice=0)  or new classic ndevice only
        dp = dev[n];
        for(n = 0 ; n < 16 ; ++n)
          dp->linkey[n] = key[n];
        dp->linkflag |= KEY_FILE;        
        }
      }
  
    }
  else if(rwflag == 1)
    {  // write   
    // update table
    flag = 0;  // no changes to table
    if(count > 0 && delflag == 0) 
      {
      for(k = 0 ; k < count ; ++k)
        {
        badd = table + k*22;
        key = badd+6;
        n = devnfrombadd(badd,BTYPE_CL,DIRN_FOR);
        if(n > 0)
          { 
          dp = dev[n];
          if((dp->linkflag & KEY_NEW) != 0)
            {   // must be KEY_FILE also
            for(n = 0 ; n < 16 ; ++n)
              key[n] = dp->linkey[n];
            dp->linkflag &= ~KEY_NEW;
            flag = 1;
            }
          }
        }
      }
        
    // count NEW additions not in table
    addcount = 0;    
    for(n = 1 ; devok(n) != 0 ; ++n)
      {
      if((dev[n]->linkflag & KEY_NEW && dev[n]->type == BTYPE_CL) != 0)
        ++addcount;
      }
 
    if(flag == 0 && delflag == 0 && addcount == 0)   
      return;   // no changes
      
    if(count + addcount > 255)
      {
      NPRINT "Too many link keys - delete link.key file to reset\n");
      return;
      }

    stream = fopen(fname,"wb");  
    if(stream == NULL)
      return;
    
    fputc(count+addcount,stream);
    if(count > 0)
      fwrite(table,1,count*22,stream);
    k = 0;
    for(n = 1 ; k < addcount && devok(n) != 0 ; ++n)
      {
      dp = dev[n];
      if((dp->linkflag & KEY_NEW) != 0 && dev[n]->type == BTYPE_CL)
        {
        fwrite(dp->baddr,1,6,stream);
        fwrite(dp->linkey,1,16,stream);
        ++k;
        }
      }
    fclose(stream);
    }
  else if(rwflag == 2 && count > 0 && ndevice > 0)
    {  // delete
    flag = 0;
    for(k = 0 ; k < count && flag == 0 ; ++k)
      {
      badd = table + k*22;
      n = devnfrombadd(badd,BTYPE_CL,DIRN_FOR);
      if(n == ndevice)
        {    // found - remove
        flag = 1;
        delflag = 1;
        dev[n]->linkflag &= ~KEY_FILE;
        for(j = k ; j < count ; ++j)
          {
          badd = table + k*22;
          for(i = 0 ; i < 22 ; ++i)
            badd[i] = badd[i+22];
          }
         --count;
        }
      }         
    } 
  else if(rwflag == 3 && count > 0) 
    {
    NPRINT "%s\n",fname);
    for(k = 0 ; k < count ; ++k)
      {
      badd = table + k*22;
      n = devnfrombadd(badd,BTYPE_CL,DIRN_FOR);
      i = 0;
      if(n >= 0)
        {
        NPRINT "%s =",dev[n]->name);
        i = 6;
        }
      for(j = i ; j < 22 ; ++j)
        {
        NPRINT " %02X",badd[j]);
        if(j == 5)
          NPRINT " =");
        }
      NPRINT "\n");
      flushprint();
      }
    }
  }  

  

void printascii(char *s,int len)
  {
  int n,flag;
  char buf[64],sav;
  
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


int meshpacket(char *s)
  {
  int k,ndevice,repn,retval;
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
                         (rp[12] == (((rp[5] ^ rp[6]) ^ rp[7]) ) | 0xC0 ) )
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

int addins(int nx,int len,unsigned char *s)
  {
  int n,xn,k,newlen,oldlen;

  // find last entry
 
  if(nx < 0)
    return(-1);

  oldlen = instack[nx+1] + (instack[nx+2] << 8);
  newlen = oldlen + len;
    
  k = 0;
  while(instack[k] != INS_FREE)
    {
    xn = k;  // last entry
    k += instack[k+1] + (instack[k+2] << 8) + INSHEADSIZE;  // next type
    }
  
    // xn is last
  
  if(xn != nx)
    {   // not last entry - need new
    xn = pushins(1,instack[nx+3],oldlen,instack+nx+INSHEADSIZE);
    if(xn < 0)
      return(-1);
    instack[xn] = instack[nx];  // replace type 1
    instack[nx] = INS_POP;  // ditch old    
    }

   // xn is last - can extend

  
  instack[xn+1] = (newlen & 255);
  instack[xn+2] = (newlen >> 8) & 255;
  n = xn + oldlen + INSHEADSIZE;
  if(n + len > INSTACKSIZE-16)
    {
    NPRINT "Serial buffer full - use read_all_clear()\n");
    return(-1);
    }
  for(k = 0 ; k < len ; ++k)
    {
    instack[n] = s[k];
    ++n;
    }  
  instack[n] = INS_FREE;  // next free
  return(xn);
  }

/******* POP IN STACK **********
pop all type = INS_POP
********************/

void popins()
  {
  int n,k,j,lastffn,lastn,del,lastwasff,count;
  
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
        del = k - lastffn;
   
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
  NPRINT "**INSTACK** \n");
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
  }


int bluezdown()
  {
  int dd,retval;
  
  if(gpar.bluez == 0)
    return(1);  // already down
    
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
    NPRINT "Bluez down failed\n");
       
  flushprint();  
  gpar.bluez = 0;  // bluez down
  sleep(1);  
  return(retval);
  }


/************** OPEN HCI SOCKET ******        
return 0=fail
       1= OK and sets gpar.hci= socket handle
*************************************/       


int hcisock()
  {
  int dd;
  struct sockaddr_hci sa;
  struct hci_filter flt;

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

         // HCI_EVENT_PKT | HCI_ACLDATA_PKT = 14
         //               | HCI_COMMAND_PKT = 16
         //  EVT = 10   ACL = 4   CMD = 2
  flt.type_mask = 0x16;    
  flt.event_mask[0] = 0xFFFFFFFF;
  flt.event_mask[1] = 0xFFFFFFFF;
  flt.opcode = 0;
  /** same as ****
  hci_filter_clear(&flt);
  hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
  hci_filter_set_ptype(HCI_ACLDATA_PKT, &flt);
  hci_filter_all_events(&flt);
  ***************/

  VPRINT "Set HCI filter\n");
        
  if(setsockopt(dd, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0)
    {
    VPRINT "HCI filter setup failed\n");
    close(dd);
    }   
 
  VPRINT "Bind to Bluetooth devid user channel\n");

  sa.hci_family = 31;   // AF_BLUETOOTH;
  sa.hci_dev = gpar.devid;    // hci0/1/2...
  sa.hci_channel = 1;   // HCI_CHANNEL_USER    
  
  if(bind(dd,(struct sockaddr *)&sa,sizeof(sa)) < 0)
    {
    VPRINT "Bind failed\n");
    close(dd);
    flushprint();
    return(0);
    }

  gpar.hci = dd;    

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
  int n,sockflags,retval,serr,len,blockflag;
  struct sockaddr_l2 sa;
  int status,s;
  char *c,*b;
  fd_set wrset;
  struct timeval tv;
 
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


/********* read key - needs non-blocking read *******
set_input_mode
****************************************************/

int readkey()
  {
  char c;
  int retval;
  
  retval = 0;
  if(read(STDIN_FILENO,&c,1) == 1)
    retval = (int)c;

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
 
  VPRINT "Set event mask\n");
  sendhci(eventmask,0);
  statusok(0,eventmask);
  

  VPRINT "Set simple pair mode on\n");
  sendhci(setspm,ndevice);
  statusok(0,setspm);

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
  int n,k,tryflag;
  struct devdata *dp;
  
    
  dp = dev[ndevice];     
  dp->linkflag &= KEY_NEW | KEY_FILE;  // clear KEY_  PASSKEY_ 
  if(dp->type == BTYPE_CL)
    {
    if((dp->linkflag & (KEY_NEW | KEY_FILE)) == 0)
      dp->linkflag |= KEY_OFF | PASSKEY_LOCAL; 
    else
      dp->linkflag |= KEY_ON | PASSKEY_LOCAL;
    } 
  else  // mesh pi
    dp->linkflag |= KEY_OFF | PASSKEY_OFF;
         
  VPRINT "Open classic connection to %s\n",baddstr(dp->baddr,0));
      
  tryflag = 0;

  if(sendhci(clopen,ndevice) == 0)
    return(0); 
    
  readhci(ndevice,IN_CLHAND,0,10000,gpar.toshort);
         // sets conflag if OK - no store on stack
  if(dp->conflag == 0)
    {
    sendhci(clcancel,ndevice);  // cancel open command
    statusok(1,clcancel);
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
        /*****
        if(dp->type == BTYPE_CL)
          {
          if((dp->linkflag & KEY_SENT) != 0)
            {
            if((dp->linkflag & KEY_FILE) != 0)
              rwlinkey(2,ndevice);  // delete key - clears KEY_FILE
            dp->linkflag &= ~KEY_NEW;
            NPRINT "Link key may be unknown or invalid\n");
            NPRINT "On remote device - unpair or unpair and re-pair\n");
            }
          }
        *******/  
        return(0);
        }
      }
    else 
      tryflag = 0;
    }
  while(tryflag != 0);     
    
  VPRINT "GOT Authentication/pair OK (Event 06)\n");
 
   
  VPRINT "SEND encrypt\n");    
  sendhci(encrypt,ndevice);
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

int find_channel(int node,int flag,char *uuid)
  {
  int flags,retval,ndevice;
  
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
    retval = clservices(ndevice,flags,uuid);
  
  if(retval < 0)
    retval = 0;
    
  flushprint();
  return(retval);
  }

int list_channels(int node,int flag)
  {
  int flags,retval,ndevice;
  
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
    retval = clservices(ndevice,flags,NULL);

  flushprint();
  return(retval);
  }

 
/********** READ SERVICES *******/


int list_uuid(int node,char *uuid)
  {
  int retval,ndevice;
    
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);
         
  if(dev[ndevice]->type == BTYPE_LE || (dev[ndevice]->type == BTYPE_ME && (dev[ndevice]->conflag & CON_LE) != 0))
    retval = leservices(ndevice,SRVC_UUID,uuid);
  else if(dev[ndevice]->type == BTYPE_CL  || dev[ndevice]->type == BTYPE_ME)
    retval = clservices(ndevice,SRVC_UUID,uuid);
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
  
int clservices(int ndevice,int flags,char *uuid)
  {
  int n,j,k,retval,ncont,sn,savto,locndevice,type;
  int headsz,locuuid,getout,savpf,flag;
  struct devdata *dp;
  unsigned char *cmd,*dat;
  unsigned char sdat[8192];
  struct servicedata serv[SERVDAT];
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

  if(dp->conflag != 0)
    {
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
    
    retval = decodesdp(sdat,sn,serv,SERVDAT);
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
      return(printchannels(ndevice,flags,serv,SERVDAT));  // print RFCOMM channels   
    }
    
  flushprint(); 
  return(-1);  // no data
  }


int printchannels(int ndevice,int flags,struct servicedata *serv,int servlen)
  {
  int n,k,len,count;
  
  count = 0;
   
  if(serv[0].channel == 0)
    {
    NPRINT "No services found\n");
    flushprint();
    return(0);
    }
   
  NPRINT "\n%s RFCOMM serial channels\n",dev[ndevice]->name);  
  flushprint();
      
  for(n = 0 ; n < servlen && serv[n].channel != 0 ; ++n)
      {
      NPRINT "  %d  %s\n",serv[n].channel,serv[n].data+1);  // data[0]=name length  data[1].. name
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
    retval = printctics0(ndevice,flag);  
     
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
  }

int leservices(int ndevice,int flag,char *uuid)
  {
  int n,n0,k,j,len,lasth,num,count,getout;
  int loop,chn,locuuid,cancelflag,failcount;
  unsigned char *cmd,buf[8];
  struct servicedata serv[SERVDAT];
  struct cticdata *cp;
  static char *perms[16] = {" ? ","r  ","w  ","rw ","wa ","rwa"," ? "," ? ","rwn?","rn ","wn ","rwn","wan","rwan","??n","??n"  };

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
    
    
  lasth = 0;  // last handle read
  cmd = lereaduuid2 + PAKHEADSIZE;
  count = 0; 
  getout = 0;
  loop = 0;
  failcount = 0;
  
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
            if((insdatn[n0+2] & 0x0E) != 0)   // r/w permissions
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
        ++failcount; 
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


int find_ctic_index(int node,int flag,char *uuid)
  {
  int n,k,getout,ndevice;
  struct cticdata *cp;
  
      
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
  int k,j,pn,count,len,del;
  struct cticdata *cp;
  static char *perms[16] = {" ? ","r  ","w  ","rw ","wa ","rwa"," ? "," ? ","rwn?","rn ","wn ","rwn","wan","rwan","??n","??n"  };
  char sizes[8];     

  count = 0;

  if(ctic(ndevice,0)->type != CTIC_ACTIVE)
    {
    if(dev[ndevice]->type == BTYPE_LE)
      NPRINT "     No characteristics - Read services to find\n");
    return(0);
    }
    
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
    if((cp->perm & 0x30) != 0)
      pn |= 8;  // notify
                         
    len = strlen(cp->name);                 
                     
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




int printctics0(int devicen,int flags)
  {
  int n,j,jn,k,xn,len,maxlen,count,flag,delflag,perm;
  int vn[2048];   // list of vailid index
  struct cticdata *cp;
   
  if(ctic(devicen,0)->type != CTIC_ACTIVE)
    return(0);
       
  maxlen = 0;  
  count = 0;
    
  for(n = 0 ; ctic(devicen,n)->type == CTIC_ACTIVE ; ++n)
    {
    perm = ctic(devicen,n)->perm;      
    if(  (flags & (CTIC_R | CTIC_W | CTIC_NOTIFY)) == 0 || (perm == 0 && (flags & CTIC_NOTIFY) == 0) ||
         ( (flags & CTIC_NOTIFY) != 0 && (perm & 0x30) != 0) ||
         ( (flags & CTIC_R)   != 0 && (perm & 0x02) != 0) ||
         ( (flags & CTIC_W)   != 0 && (perm & 0x0C) != 0)  ) 
      {
      vn[count] = n;   // save index of valid entry
      if(count < 2047)
        ++count;          
      }
    }
  
  if(count == 0)
    return(0);
 
  flag = 0;        // one column
  xn = count;      // last vn index + 1
  if(count > 5)
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
  NPRINT "ctic      LE characteristics\nindex\n"); 
  for(n = 0 ; n < xn ; ++n)
    {
    cp = ctic(devicen,vn[n]);
    NPRINT " %d  %s",vn[n],cp->name);

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
         
    NPRINT "\n");
    flushprint();
    }   
    
  NPRINT "\n");
  flushprint();
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
  int n,handle,k,j,uuidtype,flag; 
  struct cticdata *cp; 
  char *errs,buf[64];

  if(devokp(devicen) == 0 || serv[0].channel == 0)
    return(0);
   
  errs = "not correct in local devices.txt info";
  
    // look for existing entries in device info 

  for(n = 0 ; ctic(devicen,n)->type == CTIC_ACTIVE ; ++n)
    {
    cp = ctic(devicen,n);
    cp->iflag = 0;
       
    // look for handle match
    handle = cp->chandle;
    for(k = 0 ; k < servlen && serv[k].channel != 0 && handle != 0 ; ++k)
      {
      if(serv[k].channel == 0x10000 && serv[k].handle == handle)
        {
        cp->iflag = 1;
        handle = 0;                  // exit k loop
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
    uuidtype = cp->uuidtype;
    for(k = 0 ; k < servlen && serv[k].channel != 0 && uuidtype != 0 ; ++k)
      {
      if(serv[k].channel == 0x10000 && bincmp(cp->uuid,serv[k].uuid,cp->uuidtype,DIRN_FOR) != 0)
        {
        cp->iflag = 1;
        uuidtype = 0;                // exit k loop
        serv[k].channel = 0x20000;   // ditch
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
    if(serv[n].channel == 0x10000 && (serv[n].perm & 0x0E) != 0)
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
      // read value to find size
      if((cp->perm & 2) != 0) 
        {
        if(read_ctic(dev[devicen]->node,cp->cticn,buf,64) == 0)
          NPRINT "  Error reading characteristic index %d\n",cp->cticn);
        } 
      } 
    }

     
  NPRINT "Characteristics saved to device info\n");
   
  flushprint();
  return(1);
  }



/*********** CONNECT PSM ************
psm = 1  control for readservices
      3 data for read/write
*************************************/
      
int connectpsm(int psm,int channel,int ndevice)
  {
  int n,n0,okflag,count;
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

 
  okflag = 0;     
  n = findhci(IN_L2REPCT,ndevice,INS_POP);
  if(n < 0)  
    {
    NPRINT "NO L2 connect reply %d %d\n",okflag,count);
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
  int n;
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
  int n,savto;

  clscanx();
  flushprint();
  } 


void clscanx()
  {
  int n,k,j,repn,nrep,ndevice,count,newcount;
  unsigned char *rp;
  struct devdata *dp;
  char buf[64];
   
  NPRINT "Scanning for Classic devices - 10 seconds\n");
    
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
      NPRINT "%d FOUND %s\n",count+1,baddstr(rp,1));      
      
      ndevice = devnfrombadd(rp,BTYPE_CL | BTYPE_ME,DIRN_REV);
      if(ndevice >= 0)
        {
        NPRINT "   Known device %d = %s\n",dev[ndevice]->node,dev[ndevice]->name);
        if(dev[ndevice]->type != BTYPE_CL && dev[ndevice]->type != BTYPE_ME)
          NPRINT "   But not listed as Classic or Mesh\n"); 
        }
      else
        {     
        ndevice = devalloc();
        if(ndevice < 0)
          {
          instack[n] = INS_POP;
          return;    
          }
        
        ++newcount;     
        dp = dev[ndevice];
        dp->type = BTYPE_CL;
        dp->node = newnode();          
                              
        for(j = 0 ; j < 6 ; ++j)
          dp->baddr[j] = rp[5-j];

        NPRINT "   Trying to read name..\n");
        flushprint();        
        sendhci(cname,ndevice);      
        readhci(ndevice,IN_CNAME,0,8000,gpar.toshort);   
        j = findhci(IN_CNAME,ndevice,INS_POP);
        if(j < 0)
          {   // no name
          k = devnfrombadd(dp->baddr,BTYPE_LE,DIRN_FOR);
          if(k > 0 && bincmp(dev[k]->name,"LE node 1",9,DIRN_FOR) == 0)
            strcpy(buf,dev[k]->name);  // is also LE with name
          else
            {
            sprintf(buf,"Classic node %d",dp->node);
            NPRINT "   Unable to read name so set = %s\n",buf);
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
           
          k = devnfrombadd(dp->baddr,BTYPE_LE,DIRN_FOR);
          if(k > 0 && bincmp(dev[k]->name,"LE node 1",9,DIRN_FOR) != 0)
            strcpy(dev[k]->name,buf);  // is also LE without name
          }

        strcpy(dev[ndevice]->name,buf);
                 
        NPRINT "   New device %s\n",dev[ndevice]->name);
        rwlinkey(0,ndevice);  // link key in file?
        }
      rp += 14;
      ++count;
      }
    instack[n] = INS_POP;
    flushprint();
    }
  while(1); 
  }

/******** PRINT SDP DATA **************/

  
int decodesdp(char *sin,int len,struct servicedata *serv,int servlen)
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
  
int decodedes(char *sin,int len,struct sdpdata *sdpp)  //  int level,int aidflag,int record,unsigned int callaid)
  {
  int k,type,size,loop,textflag,ntogo,locaidflag,aidflip,loclevel,prflag,getout,uuidn;
  unsigned int datlen,uuid;
  char *s;
  static int szlook[8] = {1,2,4,8,16,101,102,104};
   
  
  loclevel = sdpp->level;
  locaidflag = 0;  // set 1 if this is aid level
  aidflip = 0;     // aid/value alternate
  
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
          sdpp->serv[sdpp->chdn].data[0] = strlen(sdpp->serv[sdpp->chdn].data+1);  // length of string to data[0]
          sdpp->nameflag = 1;   // look for aid = 0100 service name
          }

               
        prflag = 0;
        }
          
      if(sdpp->aid == 0x0100 && (type == 4 || type == 8) && sdpp->chdn >= 0 && sdpp->nameflag != 0)
        {    // aid = 0100 service name text
        k = 0;
        while(k < datlen && k < 62)   // data[64]
          {       
          sdpp->serv[sdpp->chdn].data[k+1] = s[k];  // name to data[1]...
          ++k;
          }
        sdpp->serv[sdpp->chdn].data[k+1] = 0;
        sdpp->serv[sdpp->chdn].data[0] = k;  // length to data[0]
       
        sdpp->nameflag = 0;
        }
        
      if(gpar.printflag == PRINT_VERBOSE)
        {   
        textflag = 0;
        if(type == 4 || type == 8)
          textflag = 1;
        for(k = 0 ; k < datlen && prflag != 0; ++k)
          {
          if(textflag == 0)
            {
            VPRINT "%02X ",s[k]);
            if( ((k+1) % 20) == 0)
              VPRINT "\n");
            }
          else
            {
            VPRINT "%c",s[k]);
            if( ((k+1) % 40) == 0)
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
  
  ndevice = devnp(node);
  if(ndevice < 0)
    return(0);
  
  if(dev[ndevice]->conflag != 0)
    {
    NPRINT "Already connected\n");
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

  VPRINT "SEND MSC CMD to set modem status\n");   // 64
  msccmdrsp[PAKHEADSIZE+9] = 0x03;
  msccmdrsp[PAKHEADSIZE+12] = 0xE3;
  sendhci(msccmdrsp,ndevice);
 // readhci(0,0,0,50,0);  // peek

  VPRINT "SEND MSC RSP\n"); // 66 to receive data?
  msccmdrsp[PAKHEADSIZE+12] = 0xE1;
  sendhci(msccmdrsp,ndevice);
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
  int n,xlength,datn,nwrit,retval,flag,ndevice,pflag;
  unsigned char *bs,*dat;
  char buf[128],xbuf[8];

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
  flushprint();
  if(retval == 0)
    return(0);
  return(count);
  }


/*************** READ SERIAL DATA ***********/


int read_node_count(int node,char *buf,int count,int exitflag,int timeoutms)
  {
  int locnode;
  
  locnode = node;
  return(readserial(&locnode,buf,count,0,(exitflag & 3) | EXIT_COUNT,timeoutms));
  }

int read_node_endchar(int node,char *buf,int bufsize,char endchar,int exitflag,int timeoutms)
  {
  int locnode;
      
  locnode = node;    
  return(readserial(&locnode,buf,bufsize,endchar,(exitflag & 3) | EXIT_CHAR,timeoutms));
  }

int read_all_endchar(int *node,char *buf,int bufsize,char endchar,int exitflag,int timeoutms)
  {
  int locnode,ndevice,retval;
  
  locnode = FROM_ALLCON;
  retval = readserial(&locnode,buf,bufsize,endchar,(exitflag & 3) | EXIT_CHAR,timeoutms);
   
  if(locnode == 0)
    *node = 0;
  else
    *node = dev[locnode]->node;  // known sender
    
  return(retval);
  }

int read_mesh(int *node,char *buf,int bufsize,int exitflag,int timeoutms)
  {
  int ndevice,locnode,retval;
 
  locnode = FROM_MESH;
  retval = readserial(&locnode,buf,bufsize,0,(exitflag & 3),timeoutms);

  if(locnode == 0)
    *node = 0;
  else
    *node = dev[locnode]->node;  // known sender
   
  return(retval);
  }

int read_error()
  {
  return(gpar.readerror);
  }


void read_node_clear(int node)
  {
  int n,ndevice;
  char temp;        
    
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
  readhci(0,0,0,100,10);
  clearins(0);
  } 


void read_notify(int timeoutms)
  {  
  int to,tox,tim0,kmsav,key;
  
  if(timeoutms < 0)
    to = 0;
  else
    to = timeoutms; 

  if(to <= 1000)
    {
    tox = to;
    to = 0;
    }
  else
    tox = 50;
   
  kmsav = setkeymode(1);
   
  tim0 = timems(TIM_LOCK);
  do
    {
    readhci(0,0,0,tox,0);
    key = readkey();
    }
  while(timems(TIM_RUN) - tim0 < to && key != 'x');
  timems(TIM_FREE);
  setkeymode(kmsav);
  return;
  }



int readserial(int *node,char *inbuff,int count,char endchar,int flag,int timeoutms)
  {
  int n,nread,meshcon,clcon,onedevn,getout,ndevice,oldkm;
  unsigned int timstart;

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
    meshreadon();   // start scan - sets MESH_R
              
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
  
  meshreadoff();
  
  if(oldkm != 2)  // key mode has changed 
    setkeymode(oldkm);  // restore
  
  flushprint();

  return(nread);
  }    
     
        

/*********** RECEIVE CHARS ************/
        
int readrf(int *ndevice,char *inbuff,int count,char endchar,int inflag,int timeoutms)
  {
  int n,k,len,getout,devicen,oneloop,flag,gotn,ndev,meshflag;
  char lastchar,*dat;
  unsigned int timstart;
     

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
  timstart = timems(TIM_LOCK);
  oneloop = 0;  // stop immediate exit until done one read

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
          
          if(devok(ndev) != 0 &&
              ( (*ndevice & FROM_MESH)    != 0 && dev[ndev]->type == BTYPE_ME)  ||
              ( (*ndevice & FROM_CLCON)   != 0 && (dev[ndev]->conflag & CON_RF) != 0) ||
              ( (*ndevice & FROM_MESHCON) != 0 && (dev[ndev]->conflag & CON_MESH) != 0)  )
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
            timems(TIM_FREE);
            return(gotn);   // run out of memory
            }

          if( meshflag == 0 && 
             (  ( (flag & EXIT_CHAR) != 0 && lastchar == endchar) ||
                ( (flag & EXIT_COUNT) != 0 && gotn == count) ) )
            {
            popins();
            gpar.readerror = 0;  // OK
            timems(TIM_FREE);
            return(gotn);  // found endchar or got count or got mesh packet - normal exit - done OK
            }

          } 
        while(getout == 0);  // loop for next char
        if(meshflag != 0)
          {  // got one IN_DATA mesh packet
          popins();
          gpar.readerror = 0;  // OK
          timems(TIM_FREE);
          return(gotn);
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
      timems(TIM_FREE);
      return(gotn);
      }


    if( (flag & EXIT_TIMEOUT) != 0)
      {
      if(timems(TIM_RUN) - timstart > timeoutms)
        {
        VPRINT "Serial read time out\n");
        gpar.readerror = ERROR_TIMEOUT;
        timems(TIM_FREE);
        return(gotn);
        }
      }
       
    if( (flag & EXIT_KEY) != 0)
      {
      if(readkey() == 'x')
        {
        VPRINT "Serial read aborted by key press\n");
        gpar.readerror = ERROR_KEY;
        timems(TIM_FREE);
        return(gotn);
        }
      }

    // look for a new IN_DATA
    
    readhci(devicen,IN_DATA,0,25,0);   

    oneloop = 1;
    flushprint();
              
    }  // loop to read IN_DATA or another readhci
  while(1);   // keep going 
  
  return(0);
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
  
int set_le_wait(int waitms)
  {
  if(waitms >= 0)
    gpar.leclientwait = waitms;
  return(gpar.leclientwait); 
  }

int set_print_flag(int flag)
  {
  int oldflag;
  
  oldflag = gpar.printflag;
  
  if(!(flag == PRINT_NONE || flag == PRINT_NORMAL || flag == PRINT_VERBOSE))
    return(oldflag);     
    
  gpar.printflag = flag;

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
      write(STDOUT_FILENO,gpar.s+gpar.prtp0,gpar.prtp - gpar.prtp0);
      n = gpar.prtp - gpar.prtp0;
      }
    else if(gpar.prtw > gpar.prtp0) // should be anyway 
      {
      write(STDOUT_FILENO,gpar.s+gpar.prtp0,gpar.prtw - gpar.prtp0);
      n = gpar.prtw - gpar.prtp0;
      write(STDOUT_FILENO,gpar.s,gpar.prtp);
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

void scroll_back()
  {
  int n,incn,count,startn,startnx,endn;
 
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
      write(STDOUT_FILENO,gpar.s+startn,gpar.prte - startn);
    else
      {
      write(STDOUT_FILENO,gpar.s+startn,gpar.prtw - startn);
      write(STDOUT_FILENO,gpar.s,gpar.prte);
      }
    printf("****** SCROLLED UP 10 lines\n");
    }   
  }
  
void scroll_forward()
  {
  int n,incn,count,startn,endn;

  
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
    write(STDOUT_FILENO,gpar.s+startn,gpar.prte - startn);
  else
    {
    write(STDOUT_FILENO,gpar.s+startn,gpar.prtw - startn);
    write(STDOUT_FILENO,gpar.s,gpar.prte);
    }

  if(gpar.prte == gpar.prtp)
    printf("**** SCROLLED DOWN - reached end\n");

  }

/******************** TIME in milliseconds *************       
return time in milliseconds since first call reset
approximate because assumes 1024 = 1000
****************************************************/       

unsigned int timems(int flag)
  {
  unsigned int dt;
  int dtn;
  struct timespec ts;
  static unsigned int ntim0;
  static unsigned int tim0;
  static int xflag = 0;  // force reset on first call
  static int count = 0;
  

  if(flag == TIM_FREE)
    {
    --count;
    return(0);
    }
  else if(flag == TIM_LOCK)
    ++count;
       
  clock_gettime(CLOCK_MONOTONIC_RAW,&ts);

  if(xflag == 0)
    {
    tim0 = ts.tv_sec;
    ntim0 = ts.tv_nsec; 
    xflag = 1;
    return(0);  // zero time on first call
    }   
    
  dt = ts.tv_sec - tim0;  // whole seconds
  if((dt & TIM_OVER) == TIM_OVER && flag == TIM_LOCK && count == 0)
    {   // overflow 
    tim0 = ts.tv_sec;
    ntim0 = ts.tv_nsec; 
    return(0);
    }
    
  dtn = ts.tv_nsec - ntim0; // fractional ns  
  if(dtn < 0)
    { 
    dtn += 1e9;
    --dt;
    }
    
  return( (dt << 10) + (dtn >> 20) );   // approx ms
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
      while(s[n+k] != 0 && s[n+k] != '=')
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

char *strtohex(char *s,int *num)
  {
  return(strtohexx(s,strlen(s),num));
  }

char *strtohexx(char *s,int slen,int *num)
  {
  int n,j,j0,k,vn,flag,count,xcount,hxcount,errflag;
  char *val;
  static int del[4] = {0,'0','a'-10,'A'-10};
  static char val0[64];
  static char val1[64];
  static char val2[64];
  static char val3[64];
  static char *valx[4] = { val0,val1,val2,val3 };
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

int readline(FILE *stream,char *s)
  {
  int n,c;
  
  n = 0;
  s[0] = 0;
  do
    {
    c = fgetc(stream);
    if(c == ';')
      {
      do
        c = fgetc(stream);
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

/**** TEST LE SERVER attribute replies ******
 send opcode 04/08/10 requests
 leaves replies on stack
************************************/ 

void readleatt(int node,int handle)
  {
  int n,k,uuidtype,ndevice;
  char *s;
  
  static unsigned char opx2[32] = {16,0,S2_HAND,0,2,0x40,0,11,0,7,0,4,0,0x10,0x01,0,0x01,0x00,0x00,0x2A};     
  static unsigned char opx16[40] = {30,0,S2_HAND,0,2,0x40,0,25,0,21,0,4,0,0x10,0x01,0,0x01,0x00,
                                        0x00,0xFF,0xEE,0xDD,0xCC,0xBB,0xAA,0x99,0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11};   
  static unsigned char op4[32] = {14,0,S2_HAND,0,2,0x40,0,9,0,5,0,4,0,0x04,0x01,0,0x01,0x00};     
 
  ndevice = devn(node);
 
  read_all_clear();
  op4[14] = handle;
  op4[16] = handle;
  sendhci(op4,ndevice);  
  readhci(0,0,0,500,0);
  n = findhci(IN_ATTDAT,ndevice,INS_NOPOP);
  if(n < 0)
    return;

  uuidtype = 0;
  if(insdatn[1] == 1)
    uuidtype = 2;
  else if(insdatn[1] == 2)
    uuidtype = 16;
  else
   return;    
 
  if(uuidtype == 2)
    {
    opx2[14] = handle;
    opx2[16] = handle;
    opx2[18] = insdatn[4];
    opx2[19] = insdatn[5];
    
    opx2[13] = 0x08;
    sendhci(opx2,ndevice);
    readhci(0,0,0,500,0);

    opx2[13] = 0x10;
    sendhci(opx2,ndevice);
    readhci(0,0,0,500,0);
    }
  else if(uuidtype == 16)
    {
    opx16[14] = handle;
    opx16[16] = handle;
    for(k = 0 ; k < 16 ; ++k)
      opx16[k+18] = insdatn[k+4];
  
    opx16[13] = 0x08;
    sendhci(opx16,devn(node));
    readhci(0,0,0,500,0);
    
    opx16[13] = 0x10;
    sendhci(opx16,devn(node));
    readhci(0,0,0,500,0);      
    }
    

  }
