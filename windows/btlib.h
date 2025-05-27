#include <stdint.h>
  // for btlib.c Version 23
  // devdata type values
#define BTYPE_LO 1
#define BTYPE_CL 2
#define BTYPE_LE 4
#define BTYPE_ME 8

  // mask flags for for devinfo only
#define BTYPE_CONNECTED    (1 << 16)
#define BTYPE_DISCONNECTED (1 << 17)
#define BTYPE_SHORT        (1 << 18)
#define BTYPE_ANY          (1 << 19)
#define BTYPE_SERME        (1 << 20)

   // flags for read
#define EXIT_TIMEOUT  1
#define EXIT_KEY      2 
   
   // RFCOMM channel for connect_node()
#define CHANNEL_NODE    0
#define CHANNEL_STORED  1  
#define CHANNEL_NEW     2
#define CHANNEL_LE      3 
   // regservice flags
#define UUID_2  0
#define UUID_16 1

   // list flags
#define LIST_SHORT 0
#define LIST_FULL  1
#define CTIC_R       (1 << 4)
#define CTIC_W       (1 << 5)
#define CTIC_NOTIFY  (1 << 6)

  // callback returns
#define SERVER_EXIT_CONNECTED 2
#define SERVER_CONTINUE 1
#define SERVER_EXIT 0

   // read_error() returns
#define ERROR_TIMEOUT 1
#define ERROR_KEY     2 
#define ERROR_FATAL   3
#define ERROR_DISCONNECT 4

  // set_print_flag()
#define PRINT_NONE    0
#define PRINT_NORMAL  1
#define PRINT_VERBOSE 2

  // LE notify
#define NOTIFY_ENABLE  1
#define NOTIFY_DISABLE 2

  // LE server operations
#define LE_CONNECT 1
#define LE_READ    2
#define LE_WRITE   3
#define LE_DISCONNECT 4
#define LE_TIMER 5
#define LE_KEYPRESS 7
#define SERVER_TIMER 5
#define CLASSIC_DATA 8
#define LE_NOTIFY_ENABLE 9
#define LE_NOTIFY_DISABLE 10

  // link key
#define KEY_OFF 0
#define PASSKEY_OFF 0
#define AUTHENTICATION_OFF 0
#define KEY_ON  1
#define PASSKEY_LOCAL (1 << 1)
#define PASSKEY_REMOTE (1 << 2)
#define PASSKEY_FIXED (1 << 3)
#define PASSKEY_RANDOM (1 << 4)
#define JUST_WORKS (1 << 5)
#define BOND_NEW   (1 << 6)
#define BOND_REPAIR (1 << 7)
#define AUTHENTICATION_ON (1 << 8)
#define SECURE_CONNECT (1 << 9)
  // connect type
#define NO_CONN       0
#define NODE_CONN     1 
#define CLASSIC_CONN  2
#define LE_CONN       3
  // settings
#define FLAG_OFF    0
#define FLAG_ON     1
#define ENABLE_OBEX 1
#define HID_MULTI   2
#define FAST_TIMER  4

#define ANY_DEVICE 0
#define ALL_DEVICES 0
#define READ_WAIT -1
#define PACKET_ENDCHAR 254


void classic_scan(void);
int classic_server(int clientnode,int(*callback)(int,unsigned char*,int),char endchar,int keyflag);
void close_all(void);
int cmd_stack_ptr(void);
int connect_node(int node,int channelflag,int channel);
char *ctic_name(int node,int cticn);
int ctic_ok(int node,int cticn);

char *device_address(int node);
int device_connected(int node);
int device_info(int mask);
int device_info_ex(int mask,char *buf,int len);
char *device_name(int node);
int device_type(int node);
int disconnect_node(int node);
int exitchar();

int find_channel(int node,int flag,unsigned char *uuid);
int find_ctics(int node);
int find_ctic_index(int node,int flag,unsigned char *uuid);

int hid_key_code(int key);

int init_blue(char *filename);
int init_blue_ex(char *filename,int hcin);

int keys_to_callback(int flag,int keyboard);

unsigned char* le_advert(int node);
void le_handles(int node,int lasthandle);
int le_interval(int node);
int le_pair(int node,int flags,int passkey);
void le_scan(void);
int le_server(int(*callback)(int,int,int),int timerds);

int list_channels(int node,int flag);
int list_channels_ex(int node,char *buf,int len);
int list_ctics(int node,int flag);
int list_ctics_ex(int node,int flag,char *buf,int len);
int list_uuid(int node,unsigned char *uuid);

int localnode(void);

void mesh_on(void);
void mesh_off(void);
void mesh_server(int(*callback)(int,unsigned char*,int));

int node_server(int clientnode,int(*callback)(int,unsigned char*,int),char endchar);

int notify_ctic(int node,int cticn,int notifyflag,int(*callback)(int,int,unsigned char*,int));

int output_file(char *filemame);


int read_ctic(int node,int cticn,unsigned char *inbuf,int bufsize);
int read_error(void);

int read_mesh(int *node,unsigned char *inbuf,int bufsize,int exitflag,int timeoutms);

int read_node_count(int node,unsigned char *inbuf,int count,int exitflag,int timeoutms);

int read_node_endchar(int node,unsigned char *inbuf,int bufsize,char endchar,int exitflag,int timeoutms);
int read_all_endchar(int *node,unsigned char *inbuf,int bufsize,char endchar,int exitflag,int timeoutms);

void read_node_clear(int node);
void read_all_clear(void);

void read_notify(int timeoutms);

void register_serial(unsigned char *uuid,char *name);

void save_pair_info(void);

void scroll_back(void);
void scroll_forward(void);
void set_flags(int flags,int onoff);
int set_le_interval(int min,int max);
int set_le_interval_update(int node,int min,int max);
int set_le_interval_server(int node,int min,int max);
void set_le_random_address(unsigned char *add);
int set_le_wait(int waitms);
void set_notify_node(int node);
int set_print_flag(int flag);
void sleep_ms(uint32_t ms);

unsigned char *strtohex(char *s,int *num);

unsigned long long time_ms(void);

int universal_server(int(*callback)(int,int,int,unsigned char*,int),char endchar,int keyflag,int timerds);

int user_function(int n0,int n1,int n2,int n3,unsigned char *dat0,unsigned char *dat1);
void uuid_advert(unsigned char *uuid);

int wait_for_disconnect(int node,int timout);
int write_ctic(int node,int cticn,unsigned char *outbuf,int count);
int write_mesh(unsigned char *outbuf,int count);
int write_node(int node,unsigned char *outbuf,int count);






