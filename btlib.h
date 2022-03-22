  // Version 3
  // devdata type values
#define BTYPE_LO 1
#define BTYPE_CL 2
#define BTYPE_LE 4
#define BTYPE_ME 8

  // mask flags for for devinfo only
#define BTYPE_CONNECTED    (1 << 16)
#define BTYPE_DISCONNECTED (1 << 17)
#define BTYPE_SHORT        (1 << 18)

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
#define SERVER_CONTINUE 0
#define SERVER_EXIT 1
 
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
  // link key
#define KEY_OFF 0
#define KEY_ON  1
#define PASSKEY_OFF 4
#define PASSKEY_LOCAL 0
#define PASSKEY_REMOTE 2
  // connect type
#define NO_CONN       0
#define NODE_CONN     1 
#define CLASSIC_CONN  2
#define LE_CONN       3


#define READ_WAIT -1

void classic_scan(void);
int classic_server(int clientnode,int (*callback)(),char endchar,int keyflag);
void close_all(void);

int connect_node(int node,int channelflag,int channel);
char *ctic_name(int node,int cticn);
int ctic_ok(int node,int cticn);

char *device_address(int node);
int device_connected(int node);
int device_info(int mask);
char *device_name(int node);
int device_type(int node);
int disconnect_node(int node);

int find_channel(int node,int flag,char *uuid);
int find_ctics(int node);
int find_ctic_index(int node,int flag,char *uuid);

int init_blue(char *filename);
int init_blue_ex(char *filename,int hcin);

void le_scan(void);
int le_server(int(*callback)(),int timerds);

int list_channels(int node,int flag);
int list_ctics(int node,int flag);
int list_uuid(int node,char *uuid);

int localnode(void);

void mesh_on(void);
void mesh_off(void);
void mesh_server(int (*callback)());

int node_server(int clientnode,int (*callback)(),char endchar);

int notify_ctic(int node,int cticn,int notifyflag,int (*callback)());

int output_file(char *filemame);


int read_ctic(int node,int cticn,unsigned char *inbuf,int bufsize);
int read_error(void);

int read_mesh(int *node,char *inbuf,int bufsize,int exitflag,int timeoutms);

int read_node_count(int node,char *inbuf,int count,int exitflag,int timeoutms);

int read_node_endchar(int node,char *inbuf,int bufsize,char endchar,int exitflag,int timeoutms);
int read_all_endchar(int *node,char *inbuf,int bufsize,char endchar,int exitflag,int timeoutms);

void read_node_clear(int node);
void read_all_clear(void);

void read_notify(int timeoutms);

void register_serial(char *uuid,char *name);

void scroll_back(void);
void scroll_forward(void);
int set_le_wait(int waitms);
int set_print_flag(int flag);
char *strtohex(char *s,int *num);


int wait_for_disconnect(int node,int timout);
int write_ctic(int node,int cticn,unsigned char *outbuf,int count);
int write_mesh(char *outbuf,int count);
int write_node(int node,unsigned char *outbuf,int count);






