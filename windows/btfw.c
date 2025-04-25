#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <time.h>
#undef ERROR_TIMEOUT
#include "btlib.h"

// Version 21.2

#define VERSION 21

int btfdespatch(char c);  // btferretw.c
int mycode1(void);  // mycode.c
int mycode2(void);
int mycode3(void);  
int mycode4(void);
int mycode5(void);  
int mycode6(void);
int mycode7(void);  
int mycode8(void);
int mycode9(void);
int mycode10(void);

void emergencystop(void);
void btfthread(PVOID pvoid);
void btfcmdthread(PVOID pvoid);
void mycodethread(PVOID pvoid);

int connectdongle(int clicom);
int tryconn(int comport);
int sendcmd(int opcode,int *ndat,int ndatlen,
                    unsigned char *dat,int datlen);
int readreply(int wantopcode,int wantid,unsigned char *rdat,int rlen,int timeout);
void getints(unsigned char *buf,int *ndat,int ndatlen);
int readn(unsigned char *buf,int len);
int writen(unsigned char *dat,int datlen);
void autoconnect(PVOID pvoid);
int autoconnectx(int flag);
void manconnect(void);
int ping(void);
int sendkeyx(int key);
void haltdongle(void);
void exitlib(void);
int readfromreg(int flag);
int readcomreg(int *comlist);
int saveinreg(int flag);
void print(char *txt);
void printn(char* txt, int len);
int findtext(char *s);
int savetxt(char *filename);
int input_string(char *prompt,char *s,int len,char *curval);
int input_radio(char *prompt,char *select);
int input_select(char *prompt,char *select);
int input_filename(char *prompt,char *s,int len,int rwflag,char *curval);
int input_integer(char *ps,int *curval);
short inputfile(char *fname,int rwflag);  


// Windows external
int sendpack(unsigned char* buf, int len);
int readpack(unsigned char* buf, int toms);
void printn(char* buf, int len);
int inithci(void);
int closehci(void);
void closecrypt(void);
int calce(unsigned char* key, unsigned char* in, unsigned char* out);
int aes_cmac(unsigned char* key, unsigned char* msg, int msglen, unsigned char* res);
void getrand(unsigned char* s, int len);
void inputpin(char* prompt, char* sbuf);
int setkeymode(int setflag);
int readkey(void);
unsigned long long time_ms();
int checkfilename(char* funs, char* s);
int getdatfile(char *s);
void serverexit(int flag);
// in btlib.h
// void scroll_back(void);
// void scroll_forward(void);
// unsigned long long time_ms(void);
// void sleep_ms(int ms);
// end Windows external


BOOL CALLBACK DlgProc(HWND hDlg,UINT iMsg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK editsub(HWND,UINT,WPARAM,LPARAM);


#define TEXTSIZE 32768
#define DLG_PROMPT 70
#define DLG_STRING 71
#define DLG_BROWSE 72
#define DLG_SELECT 73
#define DLG_YES 74
#define DLG_NO  75
#define DLG_OK 76
#define DLG_CANCEL 77
#define DLG_RAD0 78
#define DLG_RAD1 79
#define DLG_RAD2 80
#define DLG_RAD3 81
#define DLG_RAD4 82
#define DLG_RAD5 83
#define DLG_RAD6 84
#define DLG_RAD7 85

#define DLG_STOP 35
#define DLG_DONGLE 0
#define DLG_RUN 2
#define DLG_BTF 3
#define DLG_XSERV 4

struct gdat
  {
  HWND hwndMain;
  HWND hwndOut;
  HWND hDlg;
  HWND button;
  HINSTANCE hInst;
  HMENU hMenu;

  int comport;
  int runflag; // 0=start 1=bluetooth OK 2=btferret started 3=btferret cmd 4=mycode
  int clientx;
  int clienty;
  HANDLE hCom;  // dongle COM port
  unsigned long hthread;
  int id;
  int threadstop;
  int scroll;
  int rwflag;
  int findoffset;
  short btfcmd;
  short mycode;
  char devices[256];  // devices.txt file for btferret
  char devicesx[256];  // temp store for inputfile
  char docpath[256];
  char findtxt[256];
  unsigned char dongdat[8192];


  char txt[TEXTSIZE];     // screen text
  unsigned int txtn;  // txt[] number of bytes
  char *dlgtxt;  // text returned by dialog input_string
  int dlglen;    // size of buffer to receive dlgtxt
  char *dlgprompt;  // prompt text for dialog
  int dlgflag;
  int selectval;
  int selectindex;
  char *selectlist;


  WNDPROC oldedit;
  };

struct gdat gparw;

/********
1 = Pause     8 =            17 = F4     24 = F11
2 = Insert    9 =            18 = F5     25 = F12
3 = Del      10 =            19 = F6     27 = Esc
4 = Home     11 =            20 = F7     28 = Right arrow
5 = End      14 = F1         21 = F8     29 = Left arrow
6 = PgUp     15 = F2         22 = F9     30 = Down arrow
7 = PgDn     16 = F3         23 = F10    31 = Up arrow

ASCII codes 'a' = 97  (valid range 32-126)
CTL a = 225
SHIFT F1 = 471
ALT combinations not supported
*********/

unsigned char vkcode[256];

unsigned char vkdata[80] = {
VK_PAUSE,1,
VK_INSERT,2,
VK_DELETE,3,
VK_HOME,4,
VK_END,5,
VK_PRIOR,6,
VK_NEXT,7,
VK_F1,14,
VK_F2,15,
VK_F3,16,
VK_F4,17,
VK_F5,18,
VK_F6,19,
VK_F7,20,
VK_F8,21,
VK_F9,22,
VK_F10,23,
VK_F11,24,
VK_F12,25,
VK_RIGHT,28,
VK_LEFT,29,
VK_DOWN,30,
VK_UP,31,
0,0
};


LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM) ;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)
  {
  HWND        hwnd ;
  MSG         msg ;
  WNDCLASSEX  wndclass ;
  int n;

  gparw.clientx = 600;
  gparw.clienty = 600;
  gparw.hwndOut = NULL;
  gparw.hCom = INVALID_HANDLE_VALUE;  
  gparw.id = 0;
  gparw.threadstop = 0;
  gparw.comport = 0;
  gparw.runflag = 0;
  gparw.hthread = 0;
  gparw.rwflag = 0;
  gparw.findoffset = 0;
  gparw.txt[0] = 0;
  gparw.txtn = 0;



  gparw.devices[0] = 0;
  gparw.docpath[0] = 0;
  gparw.findtxt[0] = 0;

  for(n = 0 ; n < 256 ; ++n)
    vkcode[n] = 0;
  for(n = 0 ; vkdata[n] != 0 ; n += 2)
    vkcode[vkdata[n] & 0xFF] = vkdata[n+1]; 


  wndclass.cbSize        = sizeof(wndclass) ;
  wndclass.style         = CS_HREDRAW | CS_VREDRAW;
  wndclass.lpfnWndProc   = WndProc;
  wndclass.cbClsExtra    = 0;
  wndclass.cbWndExtra    = 0;
  wndclass.hInstance     = hInstance;
  wndclass.hIcon         = LoadIcon(hInstance,IDI_APPLICATION);
  wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wndclass.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
  wndclass.lpszMenuName  = "BTFWMenu";
  wndclass.lpszClassName = "BTFW";
  wndclass.hIconSm       = LoadIcon(hInstance,IDI_APPLICATION);

  RegisterClassEx (&wndclass) ;

  hwnd = CreateWindow ("BTFW",  // window class name
         "BTferret",     // window caption
         WS_OVERLAPPEDWINDOW,     // window style
         0,       // initial x position
         0,       // initial y position
         800,     // initial x size
         600,     // initial y size
         NULL,    // parent window handle
         NULL,    // window menu handle
         hInstance,  // program instance handle
         NULL) ;     // creation parameters
   
  gparw.hInst = hInstance;

  ShowWindow (hwnd, iCmdShow) ;
  UpdateWindow (hwnd) ;

  while (GetMessage (&msg, NULL, 0, 0))
    {
    TranslateMessage (&msg);
    DispatchMessage (&msg);
    }
  return msg.wParam ;
  }


LRESULT APIENTRY WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
  {
  HDC hdc ;
  PAINTSTRUCT ps ;
  SYSTEMTIME st;
  int cmd,comport;
  char *s;

  char dat[128];  //############
 
  switch (iMsg)
    {     
    case WM_SIZE:
      gparw.clientx = LOWORD(lParam);
      gparw.clienty = HIWORD(lParam);
      if(gparw.hwndOut != NULL)
        MoveWindow(gparw.hwndOut,0,0,gparw.clientx,gparw.clienty,TRUE);
      return(0);
            
    case WM_CREATE :

      gparw.hwndMain = hwnd;
      gparw.hMenu = GetMenu(hwnd);
     
      gparw.hwndOut = CreateWindow("edit",NULL,
                   WS_CHILD | WS_VISIBLE |
                   WS_VSCROLL | WS_HSCROLL | WS_BORDER | ES_LEFT | ES_MULTILINE |
                   ES_AUTOHSCROLL | ES_AUTOVSCROLL,
                   0,0,gparw.clientx,gparw.clienty,hwnd,(HMENU)1000,gparw.hInst,NULL);
      SendMessage(gparw.hwndOut,WM_SETFONT,(WPARAM)GetStockObject(ANSI_FIXED_FONT),
                       MAKELPARAM(FALSE,0));

      // subclass edit
      gparw.oldedit = (WNDPROC)SetWindowLong(gparw.hwndOut,GWL_WNDPROC,(LPARAM)editsub);
      
      sprintf(dat,"BTferret for Windows (Version %d)\n",VERSION);
      print(dat);
      print("Requires COM port PiZero dongle running btfdongle\n");

      s = getenv("USERPROFILE");
      if(s != NULL)
        {
        strcpy(gparw.docpath,s);
        strcat(gparw.docpath,"\\Documents\\");
        }

      readfromreg(0);  // COM gparw.comport
      
      if(gparw.comport != 0)
        {
        sprintf(dat,"Auto open will try COM%d first\n",gparw.comport);
        print(dat);
        }
      readfromreg(1);  // devices file
      if(gparw.devices[0] == 0)
        {
        strcpy(gparw.devices,gparw.docpath);
        strcat(gparw.devices,"devices.txt");
        }

      GetSystemTime(&st);
      srand((unsigned int)(st.wMilliseconds + (st.wSecond << 10)));
 
      SetFocus(gparw.hwndOut);             
      return(0);

    case WM_SETFOCUS:
      SetFocus(gparw.hwndOut);
      return(0);

    case WM_PAINT:
      hdc = BeginPaint (hwnd, &ps) ;
      EndPaint (hwnd, &ps) ;
      return(0);

    case WM_TIMER:
      KillTimer(hwnd,1);
      if(gparw.hthread != 0)
        {
        print("*** Open has hung up unexpectedly ***\n");
        print("It will be terminated, with unpredictable results\n");
        print("Most likely cause is the dongle - try rebooting it\n");
        TerminateThread((HANDLE)gparw.hthread,0);
        gparw.hthread = 0;  
        if(gparw.hCom != INVALID_HANDLE_VALUE)
          {
          CloseHandle(gparw.hCom);
          gparw.hCom = INVALID_HANDLE_VALUE;
          }
        EnableMenuItem(gparw.hMenu,DLG_DONGLE,MF_BYPOSITION | MF_ENABLED);
        EnableMenuItem(gparw.hMenu,30,MF_ENABLED);
        EnableMenuItem(gparw.hMenu,31,MF_ENABLED);
        DrawMenuBar(gparw.hwndMain);
        }
      return(0);

    case WM_USER:
      // save comport
      saveinreg(0);
      return(0);
       
    case WM_COMMAND:
      cmd = LOWORD(wParam);
      if(cmd >= 100 && cmd <= 121)
        {
        if (GetMenuString(gparw.hMenu, cmd, dat, 32, MF_BYCOMMAND) > 0)
          {
          gparw.btfcmd = (short)dat[0];
          _beginthread(btfcmdthread,0,&gparw.btfcmd);
          }
        else
          print("Command error\n");
        }
      else
        {  
        switch(cmd)
          {
          case 30:
            print("Auto open....\n");
            readfromreg(0);  // COM
            gparw.mycode = 0;  // auto flag - search if fail
            SetTimer(hwnd,1,8000,NULL);
            gparw.hthread = _beginthread(autoconnect,0,(void*)&gparw.mycode); 
            break;
          case 31:
            comport = input_integer("Input COM port number (e.g. 5 for COM5)",NULL);
            if(comport > 0)
              {
              // gparw.comport = comport;
              gparw.mycode = comport;   // manual flag != 0 try one only
              SetTimer(hwnd,1,5000,NULL);
              gparw.hthread = _beginthread(autoconnect,0,(void*)&gparw.mycode);
              }
            else
              print("Invalid COM port");
            break;
          case 32:
            ping();
            break; 
          case 33:
            haltdongle();
            break;     
          case 34:
            exitlib();
            break;
          case 35:
            emergencystop();
            break;
          case 39:
            if(gparw.hCom != INVALID_HANDLE_VALUE)
              {
              CloseHandle(gparw.hCom);
              gparw.hCom = INVALID_HANDLE_VALUE;
              }
            SendMessage(hwnd,WM_CLOSE,0,0L);
            return(0);
          case 40:
            strcpy(gparw.devicesx,gparw.docpath);
            strcat(gparw.devicesx,"btout.txt");
            input_filename("Save screen text to file",gparw.devicesx,256,1,NULL);
            if(!(gparw.devicesx[0] == 'x' && gparw.devicesx[1] == 0))
              savetxt(gparw.devicesx); 
            break;
          case 41:
            gparw.findoffset = 0;
            gparw.txt[0] = 0;
            gparw.txtn = 0;
            print(gparw.txt);
            break;
          case 42:
            SetFocus(gparw.hwndOut);
            SendMessage(gparw.hwndOut,EM_SETSEL,0,gparw.txtn);
            break;  

          case 43:
            gparw.findoffset = 0;
            input_string("Enter text to find",gparw.findtxt,256,NULL);
            if(!(gparw.findtxt[0] == 'x' && gparw.findtxt[1] == 0))
              findtext(gparw.findtxt);
            break;
          case 44:
            findtext(gparw.findtxt);
            break;
          case 50:
            strcpy(gparw.devicesx,gparw.devices);
            input_filename("Where is the devices file?",gparw.devicesx,256,0,gparw.devicesx);
            if(!(gparw.devicesx[0] == 'x' && gparw.devicesx[1] == 0))
              {  // not Cancel
              if(strcmp(gparw.devices,gparw.devicesx) != 0)
                {
                print("New devices file\n");
                strcpy(gparw.devices,gparw.devicesx);
                saveinreg(1);
                }
              gparw.hthread = _beginthread(btfthread,0,NULL);
              }    
            break;
          case 51:  
            gparw.mycode = 1;
            gparw.hthread = _beginthread(mycodethread,0,(void*)&gparw.mycode);    
            break;
          case 52:
            gparw.mycode = 2;
            gparw.hthread = _beginthread(mycodethread,0,(void*)&gparw.mycode); 
            break;
          case 53:  
            gparw.mycode = 3;
            gparw.hthread = _beginthread(mycodethread,0,(void*)&gparw.mycode); 
            break;
          case 54:
            gparw.mycode = 4;
            gparw.hthread = _beginthread(mycodethread,0,(void*)&gparw.mycode); 
            break;
          case 55:  
            gparw.mycode = 5;
            gparw.hthread = _beginthread(mycodethread,0,(void*)&gparw.mycode);    
            break;
          case 56:
            gparw.mycode = 6;
            gparw.hthread = _beginthread(mycodethread,0,(void*)&gparw.mycode); 
            break;
          case 57:  
            gparw.mycode = 7;
            gparw.hthread = _beginthread(mycodethread,0,(void*)&gparw.mycode); 
            break;
          case 58:
            gparw.mycode = 8;
            gparw.hthread = _beginthread(mycodethread,0,(void*)&gparw.mycode); 
            break;
          case 59:
            gparw.mycode = 9;
            gparw.hthread = _beginthread(mycodethread, 0, (void*)&gparw.mycode);
            break;
          case 60:
            gparw.mycode = 10;
            gparw.hthread = _beginthread(mycodethread, 0, (void*)&gparw.mycode);
            break;

          case 90:  // exit server
            sendkeyx(exitchar());
            break;
          }       // end switch cmd
        }         // end cmd < 100     
      return(0);  // end WM_COMMAND
               

    case WM_DESTROY :
      PostQuitMessage (0);
      return(0);
    }

  return DefWindowProc (hwnd, iMsg, wParam, lParam);
  }

LRESULT CALLBACK editsub(HWND hwnd,UINT iMsg,WPARAM wParam,LPARAM lParam)
  {
  int kc,n,flag;
  static char btfcode[32] = { "iabcspdDvtrwjRTfkulmnq" };

  kc = LOWORD(wParam);

  if(gparw.runflag < 3 && iMsg == WM_KEYDOWN && kc == VK_F3)
    {  // next find
    SendMessage(gparw.hwndMain,WM_COMMAND,44,0);
    return(0);
    }
    

  if (iMsg == WM_CHAR && gparw.runflag == 2)
    {  // Btferret running - key calls btfcode
    flag = 0;
    for (n = 0; btfcode[n] != 0 && flag == 0; ++n)
      {
      if (kc == (int)btfcode[n])
        flag = 1;
      }
    if (flag == 0)
      return(0);
    gparw.btfcmd = (short)kc;
    _beginthread(btfcmdthread, 0, &gparw.btfcmd);
    return(0);
    }
  else if(iMsg == WM_CHAR)
    {
    if(kc == 163)
      kc = 11;
    if(kc < 127)
      {
      if(kc == 13)
        kc = 10;
      else if(kc >= 1 && kc <= 31 && kc != 27 && kc != 8 && kc != 9)
        kc += 224;  // CTL a = 225
      sendkeyx(kc);
      return(0);
      }
    }
  else if(iMsg == WM_KEYDOWN)
    {
    if(wParam >= 0 && wParam < 256)
      {
      kc = vkcode[wParam];
      if(kc != 0)
        {
        if(kc >= 14 && kc <= 21 && (GetKeyState(VK_SHIFT) & 0xFFFE) != 0)
          kc += 457;  // shift F1 = 471
        // if((GetKeyState(VK_RMENU) & 0xFFFE) != 0)  // ALtGr
        sendkeyx(kc);
        return(0);
        }
      }
    }

  return(CallWindowProc(gparw.oldedit,hwnd,iMsg,wParam,lParam));
  }





void emergencystop()
  {
  int ret;
  char *prom;

  if(gparw.runflag != 0 && gparw.hthread != 0)
    {
    prom = "**** EMERGENCY STOP ****\nThis will stop the Run code\n\
WARNING - Use only as a last resort to halt code that has\n\
hung up. It may produce dangerous results\n\
Do you wish to proceed?";
    ret = input_select(prom,"0 - No\n1 - Yes");
    if(ret == 1)
      {
      TerminateThread((HANDLE)gparw.hthread,0);
      gparw.hthread = 0;  
      gparw.runflag = 0; 
      EnableMenuItem(gparw.hMenu,DLG_BTF,MF_BYPOSITION | MF_GRAYED);  
      EnableMenuItem(gparw.hMenu,DLG_RUN,MF_BYPOSITION | MF_ENABLED); 
      EnableMenuItem(gparw.hMenu,DLG_STOP,MF_GRAYED); 
      EnableMenuItem(gparw.hMenu,DLG_XSERV,MF_BYPOSITION | MF_GRAYED); 
      DrawMenuBar(gparw.hwndMain);
      print("Run code stopped\n");
      }
    }
  else
    print("No running code\n");
  }

void btfthread(PVOID pvoid)
  {
  gparw.threadstop = 0;
  if(init_blue(gparw.devices) != 0)
    {
    gparw.runflag = 2; 
    EnableMenuItem(gparw.hMenu,DLG_BTF,MF_BYPOSITION | MF_ENABLED);  
    EnableMenuItem(gparw.hMenu,DLG_RUN,MF_BYPOSITION | MF_GRAYED);  
    DrawMenuBar(gparw.hwndMain);
    print("> ");
    }
  gparw.hthread = 0;     
  _endthread();
  }
void btfcmdthread(PVOID pvoid)
  {
  short index;
  char cmd,txt[8]; 

  index = *((short*)pvoid);
  cmd = (char)index;

  gparw.threadstop = 0;  //##########
  gparw.runflag = 3;
  txt[0] = cmd;  
  txt[1] = 10;
  txt[2] = 0;
  print(txt);

  EnableMenuItem(gparw.hMenu,DLG_BTF,MF_BYPOSITION | MF_GRAYED);  
  EnableMenuItem(gparw.hMenu,DLG_STOP,MF_ENABLED);

  DrawMenuBar(gparw.hwndMain);
  btfdespatch(cmd);   // btfcode[index]);
  print("> ");
 
  EnableMenuItem(gparw.hMenu,DLG_STOP, MF_GRAYED);
  if(cmd == 'q')
      {  // quit
      gparw.runflag = 1; 
      EnableMenuItem(gparw.hMenu,DLG_RUN,MF_BYPOSITION | MF_ENABLED); 
      print("\nBTferret exit\n");
      }
    else
      {
      gparw.runflag = 2;
      EnableMenuItem(gparw.hMenu, DLG_BTF, MF_BYPOSITION | MF_ENABLED);
      }
    DrawMenuBar(gparw.hwndMain);
 
  gparw.hthread = 0;
  _endthread();
  }

void serverexit(int flag)
  {
  if(flag == 0)
    EnableMenuItem(gparw.hMenu, DLG_XSERV, MF_BYPOSITION | MF_GRAYED);
  else
    EnableMenuItem(gparw.hMenu, DLG_XSERV, MF_BYPOSITION | MF_ENABLED);
  DrawMenuBar(gparw.hwndMain);
  }


int testaes()
  {
  int n,ret;
  static unsigned char key[16] = { 0x03,0xA5,0x40,0x47,0x53,0x49,0x2B,0x39,0xA7,0xFF,0xB9,0xB8,0x90,0x1F,0xC7,0x8E };
  static unsigned char msg[16] = { 0x41,0x26,0xAD,0x87,0x48,0xC7,0x52,0xBE,0x16,0xB3,0xC7,0xEA,0x82,0x0F,0xD6,0x59 };
  unsigned char res[16];
  char txt[128],txt0[32];

  ret = aes_cmac(key,msg,16,res);
  sprintf(txt,"RET = %d\n",ret);
  print(txt);
  txt[0] = 0;
  for(n = 0 ; n < 16 ; ++n)
    {
    sprintf(txt0," %02X",res[n]);
    strcat(txt,txt0);
    }
  strcat(txt,"\n");
  print(txt);
  // should return 91 08 44 54 2D 36 4B 86 54 49 24 78 87 8D 19 BB
  return(1);
  }

void mycodethread(PVOID pvoid)
  {
  short *mycodep,mycode;

  mycodep = (short*)(pvoid);
  mycode = *mycodep;

  if(mycode < 1 || mycode > 10)
    {
    gparw.hthread = 0;
    _endthread();
    return;
    }
  gparw.threadstop = 0;
  gparw.runflag = 4;
  EnableMenuItem(gparw.hMenu,DLG_RUN,MF_BYPOSITION | MF_GRAYED);
  EnableMenuItem(gparw.hMenu,DLG_STOP,MF_ENABLED);
  DrawMenuBar(gparw.hwndMain);
  if(mycode == 1)
    mycode1();
  else if(mycode == 2)
    mycode2();
  else if(mycode == 3)
    mycode3();
  else if(mycode == 4)
    mycode4();
  else if(mycode == 5)
    mycode5();
  else if(mycode == 6)
    mycode6();
  else if(mycode == 7)
    mycode7();
  else if(mycode == 8)
    mycode8();
  else if (mycode == 9)
    mycode9();
  else if (mycode == 10)
    mycode10();


  gparw.runflag = 1;
  EnableMenuItem(gparw.hMenu,DLG_RUN,MF_BYPOSITION | MF_ENABLED);
  EnableMenuItem(gparw.hMenu,DLG_STOP,MF_GRAYED);
  DrawMenuBar(gparw.hwndMain);
  gparw.hthread = 0;
  print("Mycode exit\n");
  _endthread();
  }

int sendkeyx(int key)
  {
  int n,c;
  unsigned long long tim0;
  static int keystack[256];
  static int ksn = 0;
  static int lock = 0;

  if(lock != 0)
    {
    tim0 = time_ms();
    while(lock != 0 && (time_ms() - tim0) < 10)
      ;
    if(lock != 0)
      return(0);
    }
  
  lock = 1;
  c = 0;
  
  if(key == 0)
    {
    if(ksn != 0)
      {
      c = keystack[0];
      --ksn;
      for(n = 0 ; n < ksn ; ++n)
        keystack[n] = keystack[n+1];
      }
    }
  else if(ksn < 255)
    {
    keystack[ksn] = key;
    ++ksn;
    }
  lock = 0;
  return(c);
  }

void haltdongle()
  {
  int ret;
  char *prom;

  prom = "Halt and power off dongle gracefully.\n\
This is the recommended way of turning the dongle off\n\
before the PC is turned off.\n\
Do you wish to proceed?";

  ret = input_select(prom,"0 - No\n1 - Yes");
  if(ret == 1)
    {
    sendcmd(2,NULL,0,NULL,0);
    print("Halting dongle gracefully and power off\nUnplug/replug USB to restart\n");
    }
  }

void exitlib()
  {
  int id,ret;
  char *prom;

  prom = "Exit dongle btfdongle code. This will make the\n\
dongle unresponsive until btfdongle is re-started\n\
Do you wish to proceed?";

  ret = input_select(prom,"0 - No\n1 - Yes");

  if(ret == 1)
    {  
    print("Exit dongle btfdongle code\n"); 
    id = sendcmd(3,NULL,0,NULL,0);
    // no reply
    EnableMenuItem(gparw.hMenu,32,MF_GRAYED);
    EnableMenuItem(gparw.hMenu,33,MF_GRAYED);
    EnableMenuItem(gparw.hMenu,34,MF_GRAYED);
    EnableMenuItem(gparw.hMenu,DLG_RUN,MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(gparw.hMenu,DLG_BTF,MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(gparw.hMenu,DLG_XSERV,MF_BYPOSITION | MF_GRAYED);
    DrawMenuBar(gparw.hwndMain);
    CloseHandle(gparw.hCom);
    gparw.hCom = INVALID_HANDLE_VALUE;
    }
  }

void print(char *txt)
  {
  printn(txt,strlen(txt));
  }

void printn(char *txt,int len)
  {
  int n;
  unsigned int k;

  if((int)gparw.txtn > TEXTSIZE - len - 2000)
    {  // buffer full
    k = 4000;
    while(gparw.txt[k] != 10 && k < gparw.txtn)
      ++k;
    if(k == gparw.txtn)
      {
      gparw.txt[0] = 0;
      gparw.txtn = 0;
      }
    else
      {
      ++k;
      n = 0;
      while(k < gparw.txtn)
        {
        gparw.txt[n] = gparw.txt[k];
        ++n;
        ++k;
        }
      gparw.txt[n] = 0;
      gparw.txtn = n;  
      }
    }
  
  for(n = 0 ; n < len ; ++n)
    {
    if(txt[n] == 10 && gparw.txtn != 0 && gparw.txt[gparw.txtn-1] != 13)
      {  // ensure 0D 0A line end
      gparw.txt[gparw.txtn] = 13;
      ++gparw.txtn;
      }
    gparw.txt[gparw.txtn] = txt[n];
    ++gparw.txtn;
    }
  gparw.txt[gparw.txtn] = 0;
  
  SendMessage(gparw.hwndOut,WM_SETTEXT,0,(LPARAM)(gparw.txt)); 

  n = SendMessage(gparw.hwndOut,EM_GETLINECOUNT,0,0);     
  SendMessage(gparw.hwndOut,EM_LINESCROLL,0,n);    
  }

int savetxt(char *filename)
  {
  int ret;
  char txt[256];
  FILE *stream;

  ret = 0;
  stream = fopen(filename,"wb");
  if(stream == NULL)
    {
    print("Open file failed\n");
    return(0);
    }
  if(fwrite(gparw.txt,1,gparw.txtn,stream) == gparw.txtn)
    {
    ret = 1;
    sprintf(txt,"Screen text saved to %s\n",filename);
    print(txt);
    }
  else
    print("Failed to write screen text to file\n");
  fclose(stream);
  return(ret);
  }


int findtext(char *s)
  {
  int n0;
  char *s0;
 
  s0 = strstr(gparw.txt+gparw.findoffset,s);
  if(s0 == NULL)
    {
    print("Text not found\n");
    return(0);
    }
  strcpy(gparw.findtxt,s);
  n0 = s0 - gparw.txt;
  gparw.findoffset = n0 + strlen(s);
  SetFocus(gparw.hwndOut);
  SendMessage(gparw.hwndOut,EM_SETSEL,n0,gparw.findoffset);
  SendMessage(gparw.hwndOut,EM_SCROLLCARET,0,0);
  return(1);
  }


int readreply(int wantopcode,int wantid,unsigned char *rdat,int rlen,int timeout)
  {
  int opcode,len,nread,retval,ib0,readflag;
  unsigned char head[8];
  unsigned char id;
  BOOL ret;
  unsigned long long tim0;

  if(gparw.hCom == INVALID_HANDLE_VALUE)
    return(-4);

  retval = -1;
  tim0 = time_ms();
  do
    {
    ret = ReadFile(gparw.hCom,head,1,&nread,NULL); 
    if(ret == 0)
      ret = 0;
    if(nread == 1)
      {
      ib0 = head[0];
      if(ib0 == 0xF8 || ib0 == 0xF7)
        {
        ret = readn(head+1,4);
        opcode = (int)(head[1]);
        id = head[2];
        len = (head[4] << 8) + head[3];
           
        readflag = 0;  // must read len
        if(ib0 == 0xF8 && opcode == wantopcode && id == wantid)
          {  // got reply
          if(len <= rlen)
            {
            retval = readn(rdat,len);  // buffer size OK
            readflag = 1;
            }
          else
            {
            print("Buffer too small in readreply\n");
            retval = 0;
            }
          }
        if(readflag == 0)
          nread = readn(gparw.dongdat,len);  // 8192 dump
        }
      // else not F7/F8
      }
    if(timeout > 0)
      {
      if((int)(time_ms() - tim0) > timeout)
        {
        retval = -2;
        print("Timed out waiting for reply from dongle\n");
        }
      }
    if(gparw.threadstop != 0)
      {
      print("Reply thread stop\n");
      retval = -3;
      }
    }
  while(retval == -1);  
  return(retval);
  }

void getints(unsigned char *buf,int *ndat,int ndatlen)
  {
  int n;
  long *lp;
  unsigned char *cp;

  cp = buf;
  for(n = 0 ; n < ndatlen ; ++n)
    {
    lp = (long*)cp;
    ndat[n] = (int)(*lp);
    cp += 4;
    }
  return;
  }

int readn(unsigned char *buf,int len)
  {
  int ntogo,nr,nread;

  if(len == 0)
    return(0);

  nread = 0;
  ntogo = len;
  do
    {
    if(ReadFile(gparw.hCom,buf+nread,ntogo,&nr,NULL) != 0)
      {
      if(nr > 0)
        {
        ntogo -= nr;
        nread += nr;
        }
      } 
    }
  while(ntogo > 0);    
  return(nread);
  }

int sendcmd(int opcode,int *ndat,int ndatlen,
                    unsigned char *dat,int datlen)
  {
  int n,dn,flag,nsend;
  unsigned char *cp,cmd[40];
  long *sp;
  static unsigned char id = 0;

  if(gparw.hCom == INVALID_HANDLE_VALUE)
    return(0);

  ++id;
  if(id == 0)
    id = 1;
 
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

  if((ndatlen*4)+datlen < 30 || dat == NULL)
    {
    flag = 0;  // one send
    if(datlen > 0 && dat != NULL)
      {
      for(n = 0 ; n < datlen ; ++n)
        {
        cmd[dn+5] = dat[n];
        ++dn;
        }
      }
    nsend = dn+5;
    }
  else
    {
    nsend = dn+5;  
    dn += datlen;
    flag = 1;
    }
     
  cmd[0] = 0xF7;
  cmd[1] = opcode;
  cmd[2] = id;
  cmd[3] = dn & 0xFF;
  cmd[4] = (dn >> 8) & 0xFF;

  writen(cmd,nsend);
  if(flag != 0)
    writen(dat,datlen);

  return(id); 
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
    if(WriteFile(gparw.hCom,dat+nwrit,ntogo,&wn,NULL) != 0)
      {
      ntogo -= wn;
      nwrit += wn;
      } 
    }
  while(ntogo > 0);
  return(nwrit);  
  }

int ping()
  {
  int id,ret,dongtype,dongver;
  unsigned long dat[4];
  char buf[64];

  static char* dtype[2] = {
  "Pi Zero/Pi4",
  "Unknown" };

  static char* errs[7] = {
  "Bluetooth OK\n",
  "Dongle failed to open BTROTO socket\n",
  "Dongle failed to bind socket\n",
  "Dongle failed to start Bluetooth\n"
  "No reply from dongle - try re-starting it\n",
  "Unknown dongle type - Use latest Windows version\n",
  "Windows version > Dongle version - Use latest versions\n" };

  ret = 0;
  print("Ping dongle\n");
  id = sendcmd(1,NULL,0,NULL,0);
  if(readreply(1,id,(unsigned char*)dat,12,2000) == 12)
    {
    dongver = dat[0] & 0xFFFF;
    sprintf(buf,"Dongle replied OK. Version %d\n",dongver);
    print(buf);

    // If new Windows needs new dongle
    //if (dongver < VERSION)
    //  ret = 6;
    
    dongtype = (dat[0] >> 16) & 0xFFFF;
    if(dongtype < 0 || dongtype > 0)
      {  // valid dongtype = 0
      dongtype = 1;
      ret = 5;
      }
    sprintf(buf, "Dongle type = %s\n", dtype[dongtype]);
    print(buf);

    if(ret == 0)
      {
      ret = (int)dat[1];
      if(ret < 0 || ret > 3)
        ret = 3;
      if((dat[2] & 0xFF) != 0)
        {
        sprintf(buf,"Failed %d setup calls\n",(dat[2] & 0xFF));
        print(buf);
        }
      }
    }
  else
    ret = 4;

  print(errs[ret]);
  return(ret);  // 0=OK
  }

void manconnect()
  {
  int comport;

  comport = input_integer("Input COM port number (e.g. 5 for COM5)",NULL);
  if(comport > 0)
    tryconn(comport);
  else
    print("Invalid COM port");
  }

void autoconnect(PVOID pvoid)
  {
  short *cport;

  cport = (short*)(pvoid);  // 0=auto 1=manual
  EnableMenuItem(gparw.hMenu,DLG_DONGLE,MF_BYPOSITION | MF_GRAYED);
  DrawMenuBar(gparw.hwndMain);

  autoconnectx((int)*cport);
  KillTimer(gparw.hwndMain,1);
  gparw.hthread = 0;
  EnableMenuItem(gparw.hMenu,DLG_DONGLE,MF_BYPOSITION | MF_ENABLED);
  DrawMenuBar(gparw.hwndMain);
  _endthread();
  }

int autoconnectx(int comport)
  {
  char buf[16];
  int n,cport,comlist[16];

  if(comport == 0)
    {  // auto
    cport = gparw.comport;
    }
  else
    {  // manual
    cport = comport;
    }

   if(cport != 0)
    {
    if(tryconn(cport) != 0)
      return(1);
    }

  if(cport != 0 && comport != 0)
    {
    // stop manual
    return(0);
    }
  // else no stop auto

  readcomreg(comlist);

  if(comlist[0] == 0)
    print("No COM ports found\n");

  for(n = 0 ; n < 16 && comlist[n] != 0 ; ++n)
    {
    if(comlist[n] != gparw.comport)
      {
      sprintf(buf,"Try COM%d\n",comlist[n]);
      print(buf);
      if(tryconn(comlist[n]) != 0)
        return(1);
      }    
    }
  print("Failed to find dongle running btfdongle\n");
  return(0);
  }

int tryconn(int comport)
  {
  int ret,pret;
  char txt[64];

  ret = connectdongle(comport);
  if(ret != 0)
    {
    sprintf(txt,"COM%d open OK\n",comport);
    print(txt);
    pret = ping();
    if(pret == 0)
      {
      gparw.runflag = 1;  // Bluetooth OK
    //  print("Use Shut down to power down dongle before turning PC off\n");
    //  sprintf(txt,"Dongle running btfdongle version %d OK\n",ver);
    //  print(txt);
      if(gparw.comport != comport)
        {
        gparw.comport = comport;
        SendMessage(gparw.hwndMain,WM_USER,0,0);
        // calls saveinreg(0); outside thread in case terminate
        }
      EnableMenuItem(gparw.hMenu,32,MF_ENABLED);
      EnableMenuItem(gparw.hMenu,33,MF_ENABLED);
      EnableMenuItem(gparw.hMenu,34,MF_ENABLED);
      EnableMenuItem(gparw.hMenu,30,MF_GRAYED);
      EnableMenuItem(gparw.hMenu,31,MF_GRAYED);
      EnableMenuItem(gparw.hMenu,DLG_RUN,MF_BYPOSITION | MF_ENABLED);
      DrawMenuBar(gparw.hwndMain);
      return(1);
      }
    else
      print("Dongle setup failed or not running btfdongle\n");
    CloseHandle(gparw.hCom); 
    gparw.hCom = INVALID_HANDLE_VALUE;
    }
  else
    {
    sprintf(txt,"COM%d open failed\n",comport);
    print(txt);
    }
  return(0);
  }

int connectdongle(int clicom)
  {
  DCB dcb;
  DWORD dwError;
  BOOL fSuccess;
  COMMTIMEOUTS cto;
  static char cliport[16] = {"\\\\.\\COM10"};

  if(clicom < 10)
    {
    cliport[7] = (char)(clicom + '0');
    cliport[8] = 0;
    }
  else
    {
    cliport[7] = (char)((clicom/10) + '0');
    cliport[8] = (char)((clicom%10) + '0');
    cliport[9] = 0;
    }

  gparw.hCom = CreateFile( cliport,
    GENERIC_READ | GENERIC_WRITE,
    0,    // comm devices must be opened w/exclusive-access 
    NULL, // no security attributes 
    OPEN_EXISTING, // comm devices must use OPEN_EXISTING 
    0,    // not overlapped I/O 
    NULL  // hTemplate must be NULL for comm devices
    );

 
  if(gparw.hCom == INVALID_HANDLE_VALUE) 
    {
    dwError = GetLastError();
    print("COM open error\n");
    return(0);
    }

  dcb.DCBlength = sizeof(dcb);
  fSuccess = GetCommState(gparw.hCom,(LPDCB)&dcb);
  if(!fSuccess) 
    {
    print("GetCommState error\n");
    CloseHandle(gparw.hCom);
    gparw.hCom = INVALID_HANDLE_VALUE;
    return(0);
    }

  dcb.BaudRate = CBR_115200;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT;
  // dcb.fOutX = 0;
  // dcb.fInX = 0;
  // dcb.fNull = 0;

  fSuccess = SetCommState(gparw.hCom, &dcb);
  if(!fSuccess)  
   {
   print("SetCommState error\n");
   CloseHandle(gparw.hCom);
   gparw.hCom = INVALID_HANDLE_VALUE;
   return(0);
   }

  cto.ReadIntervalTimeout = 5; 
  cto.ReadTotalTimeoutMultiplier=5; 
  cto.ReadTotalTimeoutConstant=5; 
  cto.WriteTotalTimeoutMultiplier=5; 
  cto.WriteTotalTimeoutConstant=5; 

  fSuccess = SetCommTimeouts(gparw.hCom,&cto);
  if(!fSuccess)  
   {
   print("SetCommTimeouts error\n");
   //CloseHandle(gparw.hCom);
   //gparw.hCom = INVALID_HANDLE_VALUE;
   //return(0);
   }

  return(1);
  }

int saveinreg(int flag)
  {
  int retval;
  HKEY hkMain;
  HKEY hkClass;
  DWORD dwDisp;
  DWORD val;

  retval = 0;
  if(RegOpenKeyEx(HKEY_CURRENT_USER,"SOFTWARE",0,
                  KEY_ALL_ACCESS,&hkMain) == ERROR_SUCCESS)
    {
    if(RegCreateKeyEx(hkMain,"Btferret",0,"",
              REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,
              &hkClass,&dwDisp) == ERROR_SUCCESS)
      {
      if(flag == 0)
        {
        val = (DWORD)gparw.comport;
        if(RegSetValueEx(hkClass,"COM",0,REG_DWORD,(CONST BYTE *)&val,4) == ERROR_SUCCESS)
          retval = 1;
        }
      else
        {
        if(RegSetValueEx(hkClass,"devices",0,REG_SZ,(CONST BYTE *)gparw.devices,strlen(gparw.devices)) == ERROR_SUCCESS)
          retval = 1;
        }
      RegCloseKey(hkClass);
      }
    RegCloseKey(hkMain);
    }
  return(retval);
  }

int readfromreg(int flag)
  {
  int retval;
  HKEY hkMain;
  HKEY hkClass;
  DWORD dwb;
  DWORD bs;
  unsigned char val[256];
  static char *name[] = {
  "COM",
  "devices"
  };

     
  retval = 0;
  bs = 256;
  if(RegOpenKeyEx(HKEY_CURRENT_USER,"SOFTWARE",0,
           KEY_READ,&hkMain) == ERROR_SUCCESS)
    {
    if(RegOpenKeyEx(hkMain,"Btferret",0,
            KEY_READ,&hkClass) == ERROR_SUCCESS)
      {
      if(RegQueryValueEx(hkClass,name[flag],0,&dwb,val,&bs) == ERROR_SUCCESS)
        {
        if(flag == 0)
          gparw.comport = *((DWORD*)val);
        else
          strcpy(gparw.devices,val);
        retval = 1;
        }
      RegCloseKey(hkClass);
      }
    RegCloseKey(hkMain);
    }
  return(retval);
  }

int readcomreg(int *comlist)
  {
  int retval,n;
  HKEY hkMain;
  HKEY hkClass;
  HKEY hkDev;
  DWORD bs,bs2,index,type;
  long ret;
  char buf[128],buf2[128];

  for(n = 0 ; n < 16 ; ++n)
    comlist[n] = 0;
    
  retval = 0;
  bs = 4;
  ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"HARDWARE",0,
           KEY_READ,&hkMain);
  if(ret == ERROR_SUCCESS)
    {
    if(RegOpenKeyEx(hkMain,"DEVICEMAP",0,
            KEY_READ,&hkDev) == ERROR_SUCCESS)
      {
      if(RegOpenKeyEx(hkDev,"SERIALCOMM",0,
            KEY_READ,&hkClass) == ERROR_SUCCESS)
        {
        index = 0;
        do
          {
          bs = 128;
          bs2 = 128;
          ret = RegEnumValue(hkClass,index,buf,&bs,NULL,&type,buf2,&bs2);
          if(ret == ERROR_SUCCESS)
            {
            // buf=name of key  buf2 = "COMXX"
            comlist[index] = 0;
            if(buf2[0] == 'C' && buf2[1] == 'O' && buf2[2] == 'M')
              {
              comlist[index] = buf2[3] - '0';
              if(buf2[4] >= '0' && buf2[4] <= '9')
                comlist[index] = 10*comlist[index] + buf2[4] - '0';
              }
            }
          ++index;
          }
        while(ret == ERROR_SUCCESS && index < 15);
        RegCloseKey(hkClass);
        }
      RegCloseKey(hkDev);
      }
    RegCloseKey(hkMain);
    }
  return(retval);
  }

int input_radio(char *prom,char *select)
  {
  
  gparw.dlgprompt = prom;
  gparw.dlgtxt = NULL;

  gparw.dlgflag = 6;
  gparw.selectlist = select;
  DialogBox(gparw.hInst,"AboutBox",gparw.hwndMain,DlgProc);
  return(gparw.selectval);
  }


int input_select(char *prom,char *select)
  {
  int n,ynflag;
 
  n = 0;
  ynflag = 0;
  while(select[n] != 0 && ynflag == 0)
    {
    if(select[n] == '0' && select[n+1] == ' ' && select[n+2] > 32 && select[n+3] == ' ' &&
       select[n+4] == 'N' && select[n+5] == 'o' && select[n+6] < 32)
      {
      n += 7;
      while(select[n] != 0 && ynflag == 0)
        {
        if(select[n] == '1' && select[n+1] == ' ' && select[n+2] > 32 && select[n+3] == ' ' &&
           select[n+4] == 'Y' && select[n+5] == 'e' && select[n+6] == 's' && select[n+7] < 32)
          {
          ynflag = 1;
          }
        ++n;
        }
      }
    ++n;
    }
    
      
  gparw.dlgprompt = prom;
  gparw.dlgtxt = NULL;

  if(ynflag != 0)
    {
    gparw.dlgflag = 5;
    }
  else
    {
    gparw.dlgflag = 4;
    gparw.selectlist = select;
    }

  DialogBox(gparw.hInst,"AboutBox",gparw.hwndMain,DlgProc);
  return(gparw.selectval);
  }

int input_integer(char *prom,int *curval)
  {
  char s[32];

  gparw.dlgflag = 1;
  gparw.dlgprompt = prom;
  if(curval == NULL)
    s[0] = 0;
  else
    sprintf(s,"%d",*curval);
  gparw.dlgtxt = s;
  gparw.dlglen = 32;

  DialogBox(gparw.hInst,"AboutBox",gparw.hwndMain,DlgProc);

  if(s[0] == 0 || (s[0] == 'x' && s[1] == 0))
    return(-1);
  return(atoi(s));
  }

int input_string(char *prom,char *s,int len,char *curval)
  {
  gparw.dlgflag = 2;
  gparw.dlgprompt = prom;
  gparw.dlgtxt = s;
  if(curval != NULL)
    strcpy(gparw.dlgtxt,curval);
  else
    gparw.dlgtxt[0] = 0;
  gparw.dlglen = len;

  DialogBox(gparw.hInst,"AboutBox",gparw.hwndMain,DlgProc);
  
  return(gparw.selectval);
  }

int input_filename(char *prom,char *s,int len,int rwflag,char *curval)
  {
  gparw.dlgflag = 3;  // browse
  gparw.dlgprompt = prom;
  gparw.dlgtxt = s;
  gparw.dlglen = len;
  gparw.rwflag = rwflag;
  if(curval != NULL)
    strcpy(gparw.dlgtxt,curval);
  else
    gparw.dlgtxt[0] = 0;

  DialogBox(gparw.hInst,"AboutBox",gparw.hwndMain,DlgProc);
  
  return(gparw.selectval);
  }


/******** DIALOG *********
gparw.dlgflag
0 = not running
1 = input_integer
2 = input_string
3 = input_filename
4 = input_select
5 = inputyesno
6 = input_radio
****************/


BOOL CALLBACK DlgProc(HWND hDlg,UINT iMsg,WPARAM wParam,LPARAM lParam)
  {
  DWORD dbu;
  int n,j,nlines,del,delradio,charwidth,charheight,getout;
  char selitem[64],tmpfname[256];
  int vn,vflag;
  float wfac,hfac;
  static int val[256];
  static int hidelist[7][8] = {
  {0},
  {DLG_BROWSE,DLG_SELECT,DLG_YES,DLG_NO,0},
  {DLG_BROWSE,DLG_SELECT,DLG_YES,DLG_NO,0},
  {DLG_SELECT,DLG_YES,DLG_NO,0},
  {DLG_STRING,DLG_BROWSE,DLG_YES,DLG_NO,0},
  {DLG_STRING,DLG_BROWSE,DLG_SELECT,DLG_OK,0},
  {DLG_STRING,DLG_BROWSE,DLG_SELECT,DLG_YES,DLG_NO,0},
  };

  switch(iMsg)
    {
    case WM_INITDIALOG:
      SendDlgItemMessage(hDlg,DLG_PROMPT,WM_SETFONT,
         (WPARAM)GetStockObject(SYSTEM_FIXED_FONT),0L);
      SendDlgItemMessage(hDlg,DLG_STRING,WM_SETFONT,
         (WPARAM)GetStockObject(SYSTEM_FIXED_FONT),0L);
      SendDlgItemMessage(hDlg,DLG_SELECT,WM_SETFONT,
         (WPARAM)GetStockObject(SYSTEM_FIXED_FONT),0L);

      for(n = 0 ; n < 8 ; ++n)
         ShowWindow(GetDlgItem(hDlg,DLG_RAD0+n),SW_HIDE); 

      for(n = 0 ; hidelist[gparw.dlgflag][n] != 0 && n < 8 ; ++n)
         ShowWindow(GetDlgItem(hDlg,hidelist[gparw.dlgflag][n]),SW_HIDE);

      dbu = GetDialogBaseUnits();
      charwidth = (short)LOWORD(dbu); 
      wfac = (float)charwidth/4;
      charheight = (short)HIWORD(dbu);
      hfac = (float)charheight/8; 
      /*****
      Convert dialog units in rc file
      to screen units used by MoveWindow
      Screen x = Dialog x * wfac
      Screen y = Dialog y * hfac
      ******/
      nlines = 1;
      n = 0;
      while(gparw.dlgprompt[n] != 0)
        {
        if(gparw.dlgprompt[n] == 0x0A)
          ++nlines;
        ++n;
        }
      
      del = nlines*8;  // height of text in dialog units
      delradio = 0;
      vn = 0;

      if((gparw.dlgflag == 4 || gparw.dlgflag == 6) && gparw.selectlist != NULL)
        {  // parse selectlist
        vn = 0;
        n = 0;
        getout = 0;
        do
          {
          while((gparw.selectlist[n] == ' ' || gparw.selectlist[n] < 32) &&
                    gparw.selectlist[n] != 0)
            ++n;
          if(gparw.selectlist[n] != 0)
            {
            j = 0;
            vflag = 0;
            val[vn] = 0;
            while(gparw.selectlist[n] >= 32 && gparw.selectlist[n] != 0)
              {
              if(vflag == 0 && gparw.selectlist[n] >= '0' && gparw.selectlist[n] <= '9')
                val[vn] = (val[vn]*10) + gparw.selectlist[n] - '0';
              else
                vflag = 1;
              selitem[j] = gparw.selectlist[n];
              if(j < 63)
                ++j;
              selitem[j] = 0;
              ++n;
              }
            if(strlen(selitem) > 0)
              {
              if(gparw.dlgflag == 4)
                SendMessage(GetDlgItem(hDlg,DLG_SELECT),CB_ADDSTRING,0,(LONG)(LPSTR)selitem);
              else
                {
                ShowWindow(GetDlgItem(hDlg,DLG_RAD0+vn),SW_SHOW); 
                SetDlgItemText(hDlg,DLG_RAD0+vn,selitem);
                }                
              if(vn < 255)
                ++vn;
              if(gparw.dlgflag == 6 && vn == 8)
                getout = 1;
              }
            }
          }
        while(getout == 0 && gparw.selectlist[n] != 0);
        if(gparw.dlgflag == 4)
          SendMessage(GetDlgItem(hDlg,DLG_SELECT),CB_SETCURSEL,0,0L);
        else
          {
          CheckRadioButton(hDlg,DLG_RAD0,DLG_RAD7,DLG_RAD0);
          delradio = vn*8;
          }
        gparw.selectindex = 0;
        }

     
      MoveWindow(GetDlgItem(hDlg,DLG_PROMPT),(int)(10*wfac),(int)(10*hfac),
                                 (int)(300*wfac),(int)((del+5)*hfac),TRUE);     
      MoveWindow(GetDlgItem(hDlg,DLG_STRING),(int)(10*wfac),(int)((del+20)*hfac),
                                 (int)(220*wfac),(int)(14*hfac),TRUE); 
      MoveWindow(GetDlgItem(hDlg,DLG_SELECT),(int)(10*wfac),(int)((del+20)*hfac),
                                 (int)(180*wfac),(int)(200*hfac),TRUE);
      if(gparw.dlgflag == 6)
        {
        for(n = 0 ; n < vn ; ++n)
          MoveWindow(GetDlgItem(hDlg,DLG_RAD0+n),(int)(10*wfac),(int)((del+20+10*n)*hfac),
                                 (int)(280*wfac),(int)(14*hfac),TRUE);  
        }
  
      MoveWindow(GetDlgItem(hDlg,DLG_BROWSE),(int)(240*wfac),(int)((del+20)*hfac),
                                 (int)(50*wfac),(int)(14*hfac),TRUE);       
      MoveWindow(GetDlgItem(hDlg,DLG_OK),(int)(10*wfac),(int)((del+delradio+40)*hfac),
                                 (int)(50*wfac),(int)(14*hfac),TRUE);
      MoveWindow(GetDlgItem(hDlg,DLG_NO),(int)(10*wfac),(int)((del+40)*hfac),
                                 (int)(50*wfac),(int)(14*hfac),TRUE);  
      MoveWindow(GetDlgItem(hDlg,DLG_YES),(int)(70*wfac),(int)((del+40)*hfac),
                                 (int)(50*wfac),(int)(14*hfac),TRUE); 
      MoveWindow(GetDlgItem(hDlg,DLG_CANCEL),(int)(130*wfac),(int)((del+delradio+40)*hfac),
                                 (int)(50*wfac),(int)(14*hfac),TRUE); 
      MoveWindow(hDlg,300,100,(int)(310*wfac),(int)((del+delradio+80)*hfac),TRUE); 

      SetDlgItemText(hDlg,DLG_PROMPT,gparw.dlgprompt);
      if((gparw.dlgflag == 3 || gparw.dlgflag == 1 || gparw.dlgflag == 2) && strlen(gparw.dlgtxt) > 0)
        {
        SetDlgItemText(hDlg,DLG_STRING,gparw.dlgtxt);
        SendMessage(hDlg,WM_USER,0,0); // remove selection
        }


      return(TRUE);

    case WM_USER:
      SendMessage(GetDlgItem(hDlg,DLG_STRING),EM_SETSEL,-1,0);  
      return(TRUE);   
    case WM_COMMAND:
      switch(LOWORD(wParam))
        {
        case DLG_RAD0:
        case DLG_RAD1:
        case DLG_RAD2:
        case DLG_RAD3:
        case DLG_RAD4:
        case DLG_RAD5:
        case DLG_RAD6:
        case DLG_RAD7:
          CheckRadioButton(hDlg,DLG_RAD0,DLG_RAD7,LOWORD(wParam));
          gparw.selectindex = LOWORD(wParam) - DLG_RAD0;
          return(TRUE);
        case DLG_BROWSE:
          strcpy(tmpfname,gparw.dlgtxt);
          if(inputfile(tmpfname,gparw.rwflag) != 0)
            {
            strcpy(gparw.dlgtxt,tmpfname);
            SetDlgItemText(hDlg,DLG_STRING,gparw.dlgtxt);   
            }
          return(TRUE);
        case DLG_YES:
          gparw.selectval = 1;
          EndDialog(hDlg,0);
          return(TRUE);
        case DLG_NO:
          gparw.selectval = 0;
          EndDialog(hDlg,0);
          return(TRUE);
        case DLG_OK:
          if (gparw.dlgflag == 2 || gparw.dlgflag == 3)
            gparw.selectval = 1;
          if(gparw.dlgflag == 4)
            {
            gparw.selectindex = SendMessage(GetDlgItem(hDlg,DLG_SELECT),CB_GETCURSEL,0,0L);
            gparw.selectval = val[gparw.selectindex];
            }
          else if(gparw.dlgflag == 6)
            {
            gparw.selectval = val[gparw.selectindex];
            }
          else if(gparw.dlgflag != 5)
            {
            gparw.dlgtxt[0] = 0;
            GetDlgItemText(hDlg,DLG_STRING,gparw.dlgtxt,gparw.dlglen);
            } 
          EndDialog(hDlg,0);
          return(TRUE);
        case DLG_CANCEL:
          if(gparw.dlgflag == 2 || gparw.dlgflag == 3)
            gparw.selectval = 0;
          else
            gparw.selectval = -1;
          if(gparw.dlgtxt != NULL)
            {
            gparw.dlgtxt[0] = 'x';
            gparw.dlgtxt[1] = 0;
            }
          EndDialog(hDlg,0);
          return(TRUE);
        }
      break;
    }
  return(FALSE);
  }

/*****
rwflag 0=read 1=write
******/

short inputfile(char *fname,int rwflag)
  {
  short retval,n,lastn;
  OPENFILENAME ofn;
  char filename[256],initdir[256],file[256];
  static char ftype[] = {"All Files (*.*)\0*.*\0TXT Files\0*.txt\0\0" };

  filename[0] = '\0';

   // Initialize
  ofn.lStructSize       = sizeof(OPENFILENAME) ;
  ofn.hwndOwner         = gparw.hwndMain;
  ofn.hInstance         = NULL;
  ofn.lpstrFilter       = (LPSTR)ftype;
  ofn.lpstrCustomFilter = NULL;
  ofn.nMaxCustFilter    = 0;
  ofn.nFilterIndex      = 0;
  ofn.nMaxFile          = 256;
  ofn.nMaxFileTitle     = 256;
  ofn.lpstrInitialDir   = initdir;
  ofn.lpstrTitle        = NULL;
  ofn.nFileOffset       = 0;
  ofn.nFileExtension    = 0;
  ofn.lpstrDefExt       = NULL;
  ofn.lCustData         = 0L ;
  ofn.lpfnHook = NULL;
  ofn.lpTemplateName    = NULL;
  ofn.lpstrFile         = (LPSTR)file; 
  ofn.lpstrFileTitle    = (LPSTR)filename;


   // set initdir from fname 
  lastn = 0;
  while(fname[lastn] != '\0')
    ++lastn;

  while(lastn >= 0 && fname[lastn] != 92 &&
                  fname[lastn] != ':')
    --lastn;
      // lastn = end of path spec : or \ or -1
  if(lastn >= 0)
    {
    for(n = 0 ; n <= lastn ; ++n)
     initdir[n] = fname[n];
    }
  initdir[lastn+1] = '\0';

  n = 0;
  while(fname[lastn+n+1] != '\0' && n < 255)
    {
    file[n] = fname[lastn+n+1];
    ++n;
    }
  file[n] = '\0';

  if(rwflag == 0)   // read 
    {
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
                OFN_NOCHANGEDIR;
    retval = GetOpenFileName(&ofn);
    }
  else     // write
    {
    ofn.Flags = OFN_CREATEPROMPT | OFN_HIDEREADONLY |
         OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    retval = GetSaveFileName(&ofn);
    }
  if(retval == 0)
    return(0);  // cancel or error
  
  n = 0;
  while(file[n] != 0 && n < 255)
    {  
    fname[n] = file[n];
    ++n;
    }
  fname[n] = '\0';
  return(1);
  }



// Windows external
int sendpack(unsigned char* buf, int len)
  {
  int id;
  int ndat[1];
  unsigned char rdat[4];

  ndat[0] = len;
  id = sendcmd(5, ndat, 1, buf, len);
  readreply(5, id, rdat, 4, 0);
  
  return(rdat[0]);
  }  

int readpack(unsigned char* buf, int toms)
  {
  int id,len;
  int ndat[1];
  // char txt[256];

  ndat[0] = toms;
  id = sendcmd(6, ndat,1,NULL,0);
  len = readreply(6, id, buf, 8192, 0);
  if(len < 0)
    len = 0;

  return(len);
  }

int inithci(void)
  {
  return(1);
  }
int closehci(void)
  {
  return(1);
  }
void closecrypt(void)
  {
  return;
  }
int calce(unsigned char* key, unsigned char* in, unsigned char* out)
  {
  int n,id,ret;
  unsigned char buf[32],rdat[32];

  for(n = 0 ; n < 16 ; ++n)
    {
    buf[n] = key[n];
    buf[n+16] = in[n];
    }

  id = sendcmd(7, NULL,0, buf, 32);
  readreply(7, id, rdat, 20, 2000);
  ret = *((long*)rdat);
  for(n = 0 ; n < 16 ; ++n)
    out[n] = rdat[n+4];

  return(ret);
  }

int aes_cmac(unsigned char* key, unsigned char* msg, int msglen, unsigned char* res)
  {
  int n, id, ret,ndat[1];
  unsigned char rdat[32];
  static unsigned char *buf = NULL;
  static int buflen = 0;

  if(buf == NULL || buflen < msglen + 32)
    {
    if(buf != NULL)
      free(buf);
    buflen = msglen + 32;
    if(buflen < 128)
      buflen = 128;
    buf = malloc(buflen);
    }

  for (n = 0; n < 16; ++n)
    buf[n] = key[n];
  for(n = 0 ; n < msglen ; ++n)
    buf[n+16] = msg[n];
  
  ndat[0] = msglen;
  id = sendcmd(8, ndat, 1, buf, msglen+16);
  readreply(8, id, rdat, 20, 2000);
  ret = *((long*)rdat);
  for (n = 0 ; n < 16; ++n)
    res[n] = rdat[n + 4];

  return(ret);
  }

void getrand(unsigned char* s, int len)
  {
  int r, k, n;

  n = 8;
  k = 0;
  r = 0;
  while (k < len)
    {
    if ((n & 8) != 0)
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
  return;
  }

void inputpin(char* prom, char* sbuf)
  {
  input_string(prom,sbuf,16,NULL);
  return;
  }
int setkeymode(int setflag)
  {
  return(0);
  }
int readkey(void)
  {
  return(sendkeyx(0));
  }
// in btlib.h
void scroll_back(void)
  {
  return;
  }
void scroll_forward(void)
  {
  return;
  }
unsigned long long time_ms()
  {
  return(GetTickCount64());
  }
void sleep_ms(int ms)
  {
  unsigned long long tim;

  tim = time_ms() + ms;
  while(time_ms() < tim)
    ;
  return;
  }

int checkfilename(char* funs, char* s)
{
  int n, flag;

  flag = 0;
  for (n = 0; s[n] != 0; ++n)
  {
    if (s[n] == 92)
      flag |= 1;
    else if (s[n] < 32)
      flag |= 2;
  }
  if (flag == 1)
    return(1);


  print("*** ERROR *** in ");
  print(funs);
  print(" - Cancelled\nNo backslash or single backslash control character in file name\n");
  print("For Windows use double backslash e.g. C:\\\\cat\\\\dog\\\\vole.txt\n");

  return(0);
}

int getdatfile(char *dfile)
  {
  char *s;

  s = getenv("USERPROFILE");
  if(s == NULL)
    {
    dfile[0] = 0;
    return(0);
    }
  strcpy(dfile,s);
  strcat(dfile,"\\Documents\\btferret.dat");
  return(1);
  }
