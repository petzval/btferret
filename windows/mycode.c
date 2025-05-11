#include <stdio.h>
#include <stdlib.h>
#include "btlib.h"

int mycode1(void);
int mycode2(void);
int mycode3(void);
int mycode4(void);
int mycode5(void);
int mycode6(void);
int mycode7(void);
int mycode8(void);

// Input/Output functions in btfw.c
void print(char* txt);
int input_string(char* prompt, char* s, int len, char* curval);
int input_radio(char* prompt, char* select);
int input_select(char* prompt, char* select);
int input_filename(char* prompt, char* s, int len, int rwflag, char* curval);
int input_integer(char* ps, int* curval);

int mycode1()
  {
  int n,defval;
  char inputstr[32],txt[256],filename[128];
  
  // Edit location of devices file - must use double backslash in Windows file names
  // if(init_blue("C:\\Users\\xxxxx\\Documents\\BTferret\\devices.txt") == 0)
  //   return(0);

  /*********** INPUT/OUTPUT DEMO CODE ************/
  
  print("My code 1 - demonstrates input/output functions\n");

  n = 56;
  sprintf(txt,"Print with variable = %d\n",n);
  print(txt);

  n = input_integer("Mycode1 illustrates Windows input/output\n\
Full documentation for Mycode Bluetooth programming:\n\
    https://github.com/petzval/btferret\n\nInput integer with no default value",NULL);  
  sprintf(txt, "input_integer returned %d\n",n);
  print(txt);

  defval = 123;
  n = input_integer("Input integer with default value",&defval);
  sprintf(txt, "input_integer returned %d\n", n);
  print(txt);
  
  n = input_string("Input string with no default value",inputstr,32,NULL);
  sprintf(txt, "input_string returned %d string = %s\n",n,inputstr);
  print(txt);

  n = input_string("Input string with default value", inputstr, 32,"Default string");
  sprintf(txt, "input_string returned %d string = %s\n",n,inputstr);
  print(txt);

  n = input_select("Input select (No/Yes)", "0 - No\n1 - Yes");
  sprintf(txt, "input_select with no/yes options returned %d\n", n);
  print(txt);

  n = input_select("Input select","1 - One\n3 - Three\n20 - Twenty");
  sprintf(txt, "input_select returned %d\n", n);
  print(txt);

  n = input_radio("Input radio", "1 - One\n3 - Three\n20 - Twenty");
  sprintf(txt, "input_radio returned %d\n", n);
  print(txt);

  n = input_filename("Input file name to read",filename, 128, 0, NULL);
  sprintf(txt,"input_filename returned %d name = %s\n",n,filename);
  print(txt);

  /********* END INPUT/OUTPUT DEMO **************/
    
  // close_all();

  return(0);
  }

int mycode2()
  {
  print("My code 2\n");  


  // Edit location of devices file - must use double backslash in Windows file names
  // if(init_blue("C:\\Users\\xxxxx\\Documents\\BTferret\\devices.txt") == 0)
  //   return(0);

  // your code here

  // close_all();
  
  return(0);
  }
  
int mycode3()
  {
  print("My code 3\n");
   
  // if(init_blue("C:\\Users\\xxxxx\\Documents\\BTferret\\devices.txt") == 0)
  //   return(0);

  // your code here

  // close_all();

  return(0);   
  }  

int mycode4()
  {
  print("My code 4\n");
  
  // if(init_blue("C:\\Users\\xxxxx\\Documents\\BTferret\\devices.txt") == 0)
  //   return(0);

  // your code here

  // close_all();
  
  return(0);
  }

int mycode5()
  {
  print("My code 5\n");
  
  // if(init_blue("C:\\Users\\xxxxx\\Documents\\BTferret\\devices.txt") == 0)
  //   return(0);

  // your code here

  // close_all();
  
  return(0);
  }

int mycode6()
  {
  print("My code 6\n");
  
  // if(init_blue("C:\\Users\\xxxxx\\Documents\\BTferret\\devices.txt") == 0)
  //   return(0);

  // your code here

  // close_all(); 

  return(0);
  }

int mycode7()
  {
  print("My code 7\n");
  
  // if (init_blue("C:\\Users\\xxxxx\\Documents\\BTferret\\devices.txt") == 0)
  //   return(0);

  // your code here

  // close_all();

  return(0);
  }

int mycode8()
  {
  print("My code 8\n");
  
  // if(init_blue("C:\\Users\\xxxxx\\Documents\\BTferret\\devices.txt") == 0)
  //   return(0);

  // your code here

  // close_all();

  return(0);   
  }  

int mycode9()
  {
  print("My code 9\n");
  
  // if(init_blue("C:\\Users\\xxxxx\\Documents\\BTferret\\devices.txt") == 0)
  //   return(0);

  // your code here

  // close_all();

  return(0);
  }

int mycode10()
  {
  print("My code 10\n");

  // if(init_blue("C:\\Users\\xxxxx\\Documents\\BTferret\\devices.txt") == 0)
  //   return(0);

  // your code here

  // close_all();  

  return(0);
  }

