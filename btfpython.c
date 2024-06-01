//############ VERSION 16 #################

#include <Python.h>

static PyObject* Classic_scan(PyObject* self,PyObject* args);
static PyObject* Classic_server(PyObject* self,PyObject* args);
static PyObject* Close_all(PyObject* self,PyObject* args);
static PyObject* Connect_node(PyObject* self,PyObject* args);
static PyObject* Ctic_name(PyObject* self,PyObject* args);
static PyObject* Ctic_ok(PyObject* self,PyObject* args);
static PyObject* Device_address(PyObject* self,PyObject* args);
static PyObject* Device_info(PyObject* self,PyObject* args);
static PyObject* Device_name(PyObject* self,PyObject* args);
static PyObject* Device_type(PyObject* self,PyObject* args);
static PyObject* Device_connected(PyObject* self,PyObject* args);
static PyObject* Disconnect_node(PyObject* self,PyObject* args);
static PyObject* Find_channel(PyObject* self,PyObject* args);
static PyObject* Find_ctics(PyObject* self,PyObject* args);
static PyObject* Find_ctic_index(PyObject* self,PyObject* args);
static PyObject* Hid_key_code(PyObject* self,PyObject* args);
static PyObject* Init_blue(PyObject* self,PyObject* args);
static PyObject* Init_blue_ex(PyObject* self,PyObject* args);
static PyObject* Keys_to_callback(PyObject* self,PyObject* args);
static PyObject* Le_pair(PyObject* self,PyObject* args);
static PyObject* Le_scan(PyObject* self,PyObject* args);
static PyObject* Le_server(PyObject* self,PyObject* args);
static PyObject* List_channels(PyObject* self,PyObject* args);
static PyObject* List_ctics(PyObject* self,PyObject* args);
static PyObject* List_uuid(PyObject* self,PyObject* args);
static PyObject* Localnode(PyObject* self,PyObject* args);
static PyObject* Mesh_on(PyObject* self,PyObject* args);
static PyObject* Mesh_off(PyObject* self,PyObject* args);
static PyObject* Mesh_server(PyObject* self,PyObject* args);
static PyObject* Node_server(PyObject* self,PyObject* args);
static PyObject* Notify_ctic(PyObject* self,PyObject* args);
static PyObject* Output_file(PyObject* self,PyObject* args);
static PyObject* Read_all_clear(PyObject* self,PyObject* args);
static PyObject* Read_ctic(PyObject* self,PyObject* args);
static PyObject* Read_error(PyObject* self,PyObject* args);
static PyObject* Read_mesh(PyObject* self,PyObject* args);
static PyObject* Read_node_clear(PyObject* self,PyObject* args);
static PyObject* Read_node_count(PyObject* self,PyObject* args);
static PyObject* Read_node_endchar(PyObject* self,PyObject* args);
static PyObject* Read_all_endchar(PyObject* self,PyObject* args);
static PyObject* Read_all_clear(PyObject* self,PyObject* args);
static PyObject* Read_node_clear(PyObject* self,PyObject* args);
static PyObject* Read_notify(PyObject* self,PyObject* args);
static PyObject* Register_serial(PyObject* self,PyObject* args);
static PyObject* Scroll_back(PyObject* self,PyObject* args);
static PyObject* Scroll_forward(PyObject* self,PyObject* args);
static PyObject* Set_flags(PyObject* self,PyObject* args);
static PyObject* Set_le_interval(PyObject* self,PyObject* args);
static PyObject* Set_le_interval_update(PyObject* self,PyObject* args);
static PyObject* Set_le_interval_server(PyObject* self,PyObject* args);
static PyObject* Set_le_random_address(PyObject* self,PyObject* args);
static PyObject* Set_le_wait(PyObject* self,PyObject* args);
static PyObject* Set_print_flag(PyObject* self,PyObject* args);
static PyObject* Strtohex(PyObject* self,PyObject* args);
static PyObject* Wait_for_disconnect(PyObject* self,PyObject* args);
static PyObject* Write_ctic(PyObject* self,PyObject* args);
static PyObject* Write_mesh(PyObject* self,PyObject* args);
static PyObject* Write_node(PyObject* self,PyObject* args);
static PyObject* Print_data(PyObject* self,PyObject* args);


static PyMethodDef BtfpyMethods[] =
  {
  {"Classic_scan",Classic_scan,METH_VARARGS,"Classic scan"},
  {"Classic_server",Classic_server,METH_VARARGS,"Classic server"},
  {"Close_all",Close_all,METH_VARARGS,"Close all"},
  {"Connect_node",Connect_node,METH_VARARGS,"Connect node"},
  {"Ctic_name",Ctic_name,METH_VARARGS,"Ctic name"},
  {"Ctic_ok",Ctic_ok,METH_VARARGS,"Ctic OK"},
  {"Device_address",Device_address,METH_VARARGS,"Device address"},
  {"Device_connected",Device_connected,METH_VARARGS,"Device connected"},
  {"Device_info",Device_info,METH_VARARGS,"Device info"},
  {"Device_name",Device_name,METH_VARARGS,"Device name"},
  {"Device_type",Device_type,METH_VARARGS,"Device type"},
  {"Disconnect_node",Disconnect_node,METH_VARARGS,"Disconnect node"},
  {"Find_channel",Find_channel,METH_VARARGS,"Find channel"},
  {"Find_ctics",Find_ctics,METH_VARARGS,"Find ctics"},
  {"Find_ctic_index",Find_ctic_index,METH_VARARGS,"Find ctic index"},
  {"Hid_key_code",Hid_key_code,METH_VARARGS,"HID key code"},
  {"Init_blue",Init_blue,METH_VARARGS,"Init blue"},
  {"Init_blue_ex",Init_blue_ex,METH_VARARGS,"Init blue ex"},
  {"Keys_to_callback",Keys_to_callback,METH_VARARGS,"Keys to callback"},
  {"Le_pair",Le_pair,METH_VARARGS,"LE pair"},
  {"Le_scan",Le_scan,METH_VARARGS,"LE scan"},
  {"Le_server",Le_server,METH_VARARGS,"LE server"},
  {"List_channels",List_channels,METH_VARARGS,"List channels"},
  {"List_ctics",List_ctics,METH_VARARGS,"List ctics"},
  {"List_uuid",List_uuid,METH_VARARGS,"List uuid"},
  {"Localnode",Localnode,METH_VARARGS,"Local node"},
  {"Mesh_on",Mesh_on,METH_VARARGS,"Mesh on"},
  {"Mesh_off",Mesh_off,METH_VARARGS,"Mesh off"},
  {"Mesh_server",Mesh_server,METH_VARARGS,"Mesh server"},
  {"Node_server",Node_server,METH_VARARGS,"Node server"},
  {"Notify_ctic",Notify_ctic,METH_VARARGS,"Notify ctic"},
  {"Output_file",Output_file,METH_VARARGS,"Output file"},
  {"Read_ctic",Read_ctic,METH_VARARGS,"Read ctic"},
  {"Read_error",Read_error,METH_VARARGS,"Read error"},
  {"Read_mesh",Read_mesh,METH_VARARGS,"Read mesh"},
  {"Read_node_count",Read_node_count,METH_VARARGS,"Read node count"},
  {"Read_node_endchar",Read_node_endchar,METH_VARARGS,"Read node endchar"},
  {"Read_all_endchar",Read_all_endchar,METH_VARARGS,"Read all endchar"},
  {"Read_node_clear",Read_node_clear,METH_VARARGS,"Read node clear"},
  {"Read_all_clear",Read_all_clear,METH_VARARGS,"Read all clear"},
  {"Read_mesh",Read_mesh,METH_VARARGS,"Read mesh"},
  {"Read_notify",Read_notify,METH_VARARGS,"Read notify"},
  {"Register_serial",Register_serial,METH_VARARGS,"Register serial"},
  {"Scroll_back",Scroll_back,METH_VARARGS,"Scroll back"},
  {"Scroll_forward",Scroll_forward,METH_VARARGS,"Scroll forward"},
  {"Set_flags",Set_flags,METH_VARARGS,"Set flags"},
  {"Set_le_interval",Set_le_interval,METH_VARARGS,"Set LE interval"},
  {"Set_le_interval_update",Set_le_interval_update,METH_VARARGS,"Set LE interval update"},
  {"Set_le_interval_server",Set_le_interval_server,METH_VARARGS,"Set LE interval server"},
  {"Set_le_random_address",Set_le_random_address,METH_VARARGS,"Set LE random address"},
  {"Set_le_wait",Set_le_wait,METH_VARARGS,"Set LE wait"},
  {"Set_print_flag",Set_print_flag,METH_VARARGS,"Set print flag"},
  {"Strtohex",Strtohex,METH_VARARGS,"Str to hex"},
  {"Wait_for_disconnect",Wait_for_disconnect,METH_VARARGS,"Wait for disconnect"},
  {"Write_ctic",Write_ctic,METH_VARARGS,"Write ctic"},
  {"Write_mesh",Write_mesh,METH_VARARGS,"Write mesh"},
  {"Write_node",Write_node,METH_VARARGS,"Write node"},
  {"Print_data",Print_data,METH_VARARGS,"Print data"},


  {NULL,NULL,0,NULL}
  };


static struct PyModuleDef btfpy = {PyModuleDef_HEAD_INIT,"btfpy","",-1,BtfpyMethods };
PyMODINIT_FUNC PyInit_btfpy()
  {
  PyObject* module;
  
  module = PyModule_Create(&btfpy); 


  PyModule_AddIntConstant(module,"BTYPE_LO",BTYPE_LO);
  PyModule_AddIntConstant(module,"BTYPE_CL",BTYPE_CL);
  PyModule_AddIntConstant(module,"BTYPE_LE",BTYPE_LE);
  PyModule_AddIntConstant(module,"BTYPE_ME",BTYPE_ME);

  PyModule_AddIntConstant(module,"BTYPE_CONNECTED",BTYPE_CONNECTED);
  PyModule_AddIntConstant(module,"BTYPE_DISCONNECTED",BTYPE_DISCONNECTED);
  PyModule_AddIntConstant(module,"BTYPE_SHORT",BTYPE_SHORT);

  PyModule_AddIntConstant(module,"EXIT_TIMEOUT",EXIT_TIMEOUT);
  PyModule_AddIntConstant(module,"EXIT_KEY",EXIT_KEY);
   
  PyModule_AddIntConstant(module,"CHANNEL_NODE",CHANNEL_NODE);
  PyModule_AddIntConstant(module,"CHANNEL_STORED",CHANNEL_STORED);
  PyModule_AddIntConstant(module,"CHANNEL_NEW",CHANNEL_NEW);
  PyModule_AddIntConstant(module,"CHANNEL_LE",CHANNEL_LE);

  PyModule_AddIntConstant(module,"UUID_2",UUID_2);
  PyModule_AddIntConstant(module,"UUID_16",UUID_16);

  PyModule_AddIntConstant(module,"LIST_SHORT",LIST_SHORT);
  PyModule_AddIntConstant(module,"LIST_FULL",LIST_FULL);
  PyModule_AddIntConstant(module,"CTIC_R",CTIC_R);
  PyModule_AddIntConstant(module,"CTIC_W",CTIC_W);
  PyModule_AddIntConstant(module,"CTIC_NOTIFY",CTIC_NOTIFY);

  PyModule_AddIntConstant(module,"SERVER_CONTINUE",SERVER_CONTINUE);
  PyModule_AddIntConstant(module,"SERVER_EXIT",SERVER_EXIT);
  PyModule_AddIntConstant(module,"SERVER_BTLETIMER",SERVER_BTLETIMER);
  PyModule_AddIntConstant(module,"SERVER_BTLESTOP",SERVER_BTLESTOP);
 
  PyModule_AddIntConstant(module,"ERROR_TIMEOUT",ERROR_TIMEOUT);
  PyModule_AddIntConstant(module,"ERROR_KEY",ERROR_KEY);
  PyModule_AddIntConstant(module,"ERROR_FATAL",ERROR_FATAL);
  PyModule_AddIntConstant(module,"ERROR_DISCONNECT",ERROR_DISCONNECT);

  PyModule_AddIntConstant(module,"PRINT_NONE",PRINT_NONE);
  PyModule_AddIntConstant(module,"PRINT_NORMAL",PRINT_NORMAL);
  PyModule_AddIntConstant(module,"PRINT_VERBOSE",PRINT_VERBOSE);

  PyModule_AddIntConstant(module,"NOTIFY_ENABLE",NOTIFY_ENABLE);
  PyModule_AddIntConstant(module,"NOTIFY_DISABLE",NOTIFY_DISABLE);

  PyModule_AddIntConstant(module,"LE_CONNECT",LE_CONNECT);
  PyModule_AddIntConstant(module,"LE_READ",LE_READ);
  PyModule_AddIntConstant(module,"LE_WRITE",LE_WRITE);
  PyModule_AddIntConstant(module,"LE_DISCONNECT",LE_DISCONNECT);
  PyModule_AddIntConstant(module,"LE_TIMER",LE_TIMER);
  PyModule_AddIntConstant(module,"LE_BTLETIMER",LE_BTLETIMER);
  PyModule_AddIntConstant(module,"LE_KEYPRESS",LE_KEYPRESS);

  PyModule_AddIntConstant(module,"KEY_OFF",KEY_OFF);
  PyModule_AddIntConstant(module,"PASSKEY_OFF",PASSKEY_OFF);
  PyModule_AddIntConstant(module,"AUTHENTICATION_OFF",AUTHENTICATION_OFF);
  PyModule_AddIntConstant(module,"KEY_ON",KEY_ON);
  PyModule_AddIntConstant(module,"PASSKEY_LOCAL",PASSKEY_LOCAL);
  PyModule_AddIntConstant(module,"PASSKEY_REMOTE",PASSKEY_REMOTE);
  PyModule_AddIntConstant(module,"PASSKEY_FIXED",PASSKEY_FIXED);
  PyModule_AddIntConstant(module,"PASSKEY_RANDOM",PASSKEY_RANDOM);
  PyModule_AddIntConstant(module,"JUST_WORKS",JUST_WORKS);
  PyModule_AddIntConstant(module,"BOND_NEW",BOND_NEW);
  PyModule_AddIntConstant(module,"BOND_REPAIR",BOND_REPAIR);
  PyModule_AddIntConstant(module,"AUTHENTICATION_ON",AUTHENTICATION_ON);
  PyModule_AddIntConstant(module,"SECURE_CONNECT",SECURE_CONNECT);

  PyModule_AddIntConstant(module,"NO_CONN",NO_CONN);
  PyModule_AddIntConstant(module,"NODE_CONN",NODE_CONN);
  PyModule_AddIntConstant(module,"CLASSIC_CONN",CLASSIC_CONN);
  PyModule_AddIntConstant(module,"LE_CONN",LE_CONN);

  PyModule_AddIntConstant(module,"ANY_DEVICE",ANY_DEVICE);
  PyModule_AddIntConstant(module,"READ_WAIT",READ_WAIT);
  PyModule_AddIntConstant(module,"PACKET_ENDCHAR",PACKET_ENDCHAR);

  PyModule_AddIntConstant(module,"FLAG_ON",FLAG_ON);
  PyModule_AddIntConstant(module,"FLAG_OFF",FLAG_OFF);
  PyModule_AddIntConstant(module,"ENABLE_OBEX",ENABLE_OBEX);
  
  return(module);
  }


PyObject *py_callback = NULL;
PyObject *py_lecallback = NULL;
PyObject *py_ncallback = NULL;
PyObject *py_mecallback = NULL;

int py_cmn_callback(PyObject *pycallback,int clientnode,unsigned char *dat,int datlen);
int py_mesh_callback(PyObject *pycallback,int clientnode,unsigned char *dat,int datlen);
int py_le_callback(PyObject *pycallback,int node,int op,int cticn);
void py_notify_callback(PyObject *pycallback,int node,int cticn,unsigned char *dat,int datlen);
int objtobuf(PyObject *obj,unsigned char *buf,int bufsize);
void printerror(PyObject *obj);
void hexprint(unsigned char *buf,int len);


int py_cmn_callback(PyObject *pycallback,int clientnode,unsigned char *dat,int datlen)
  {
  int n,err;
  PyObject *args,*xobj,*ret,*cnode,*dlen;
  
  if(PyErr_Occurred() != NULL)
    {
    PyErr_Print(); 
    PyErr_Clear();
    }

  ret = NULL;
  err = 0;
  xobj = PyBytes_FromStringAndSize((char*)dat,datlen);
  if(xobj == NULL)
    err = 1;
  else
    {
    cnode = PyLong_FromLong(clientnode);
    if(cnode == NULL)
      err = 1;
    else
      {
      dlen =  PyLong_FromLong(datlen);
      if(dlen == NULL)
        err = 1;
      else
        {
        args = PyTuple_Pack(3,cnode,xobj,dlen);
        if(args == NULL)
          err = 1;
        else
          {
          ret = PyObject_CallObject(pycallback,args);
          Py_DECREF(args);
          }
        Py_DECREF(dlen);
        }
      Py_DECREF(cnode);
      }
    Py_DECREF(xobj);
    }
    
  if(err != 0)  
    {
    printf("Callback system error - Server stopped\n");
    PyErr_Clear();
    return(SERVER_EXIT);
    }     
    
  if(ret == NULL || PyErr_Occurred() != NULL)
    {
    if(PyErr_Occurred() != NULL)
      PyErr_Print(); 
    printf("Error in Classic/Node callback code - Server stopped\n");
    PyErr_Clear();
    if(ret != NULL)
      Py_DECREF(ret);
    return(SERVER_EXIT);
    }
  n = PyLong_AsLong(ret);
  Py_DECREF(ret);
  return(n);
  }

int py_mesh_callback(PyObject *pycallback,int clientnode,unsigned char *dat,int datlen)
  {
  int n,err;
  PyObject *args,*xobj,*ret,*cnode,*dlen;

  if(PyErr_Occurred() != NULL)
    {
    PyErr_Print(); 
    PyErr_Clear();
    }

  ret = NULL;  
  err = 0;
  xobj = PyBytes_FromStringAndSize((char*)dat,datlen);
  if(xobj == NULL)
    err = 1;
  else
    {
    cnode = PyLong_FromLong(clientnode);
    if(cnode == NULL)
      err = 1;
    else
      {
      dlen =  PyLong_FromLong(datlen);
      if(dlen == NULL)
         err = 1;
      else
        {
        args = PyTuple_Pack(3,cnode,xobj,dlen);
        if(args == NULL)
          err = 1;
        else
          {
          ret = PyObject_CallObject(pycallback,args);
          Py_DECREF(args);
          }
        Py_DECREF(dlen);
        }
      Py_DECREF(cnode);
      }
    Py_DECREF(xobj);
    }
    
  if(err != 0)  
    {
    printf("Callback system error - Server stopped\n");
    PyErr_Clear();
    return(SERVER_EXIT);
    }     

  if(ret == NULL || PyErr_Occurred() != NULL)
    {
    if(PyErr_Occurred() != NULL)
      PyErr_Print(); 
    printf("Error in Mesh callback code - Server stopped\n");
    PyErr_Clear();
    if(ret != NULL)
      Py_DECREF(ret);
    return(SERVER_EXIT);
    }
  n = PyLong_AsLong(ret);
  Py_DECREF(ret);
  return(n);
  }


int py_le_callback(PyObject *pycallback,int clientnode,int op,int cticn)
  {
  int n,err;
  PyObject *args,*ret,*cnode,*opr,*ctn; 
  

  if(PyErr_Occurred() != NULL)
    {
    PyErr_Print(); 
    PyErr_Clear();
    }
  
  ret = NULL;
  err = 0;
  cnode = PyLong_FromLong(clientnode);
  if(cnode == NULL)
    err = 1;
  else
    {
    opr = PyLong_FromLong(op);
    if(opr == NULL)
      err = 1;
    else
      {
      ctn = PyLong_FromLong(cticn);
      if(ctn == NULL)
        err = 1;
      else
        {
        args = PyTuple_Pack(3,cnode,opr,ctn);
        if(args == NULL)
          err = 1;
        else
          {
          ret = PyObject_CallObject(pycallback,args);
          Py_DECREF(args);
          }
        Py_DECREF(ctn);
        }
      Py_DECREF(opr);
      }
    Py_DECREF(cnode);
    }
  
  if(err != 0)  
    {
    printf("Callback system error - Server stopped\n");
    PyErr_Clear();
    return(SERVER_EXIT);
    }
               
  if(ret == NULL || PyErr_Occurred() != NULL)
    {
    if(PyErr_Occurred() != NULL)
      PyErr_Print(); 
    printf("Error in LE callback code - Server stopped\n");
    PyErr_Clear();
    if(ret != NULL)
      Py_DECREF(ret);
    return(SERVER_EXIT);
    }
    
  n = PyLong_AsLong(ret);
  Py_DECREF(ret);
  return(n);
  }

void py_notify_callback(PyObject *pycallback,int clientnode,int cticn,unsigned char *dat,int datlen)
  {
  int err;
  PyObject *args,*xobj,*ret,*cnode,*ctn,*dlen;
 
  if(PyErr_Occurred() != NULL)
    {
    PyErr_Print(); 
    PyErr_Clear();
    }
    
  ret = NULL;
  err = 0;
  xobj = PyBytes_FromStringAndSize((char*)dat,datlen);
  if(xobj == NULL)
    err = 1;
  else
    {   
    cnode = PyLong_FromLong(clientnode);
    if(cnode == NULL)
      err = 1;
    else
      {
      ctn = PyLong_FromLong(cticn);
      if(ctn == NULL)
        err = 1;
      else
        {
        dlen = PyLong_FromLong(datlen);    
        if(dlen == NULL)
          err = 1;
        else
          {
          args = PyTuple_Pack(4,cnode,ctn,xobj,dlen);
          if(args == NULL)
            err = 1;
          else
            {
            ret = PyObject_CallObject(pycallback,args);
            Py_DECREF(args);
            }
          Py_DECREF(dlen);
          }
        Py_DECREF(ctn);
        }
      Py_DECREF(cnode);
      }
    Py_DECREF(xobj);
    }
    
  if(err != 0)
    {
    printf("Callback system error - Server stopped\n");
    PyErr_Clear();
    return;
    }     

  if(ret == NULL || PyErr_Occurred() != NULL)
    {
    if(PyErr_Occurred() != NULL)
      PyErr_Print(); 
    PyErr_Clear();
    if(ret != NULL)
      Py_DECREF(ret);
    printf("Error in Notify callback code\n");
    }
  Py_DECREF(ret);
  } 

// void classic_scan(void);
static PyObject* Classic_scan(PyObject* self,PyObject* args)
  {
  classic_scan();
  Py_RETURN_NONE; 
  }
  
//int classic_server(int clientnode,int (*callback)(),char endchar,int keyflag);
static PyObject* Classic_server(PyObject* self,PyObject* args)
  {
  int n,node,endchar,keyflag;
  
  n = 0;   
  if(PyObject_Size(args) != 4 || !PyArg_ParseTuple(args,"iOii",&node,&py_callback,&endchar,&keyflag))
    printerror((PyObject*)Classic_server);
  else
    {
    if(!PyFunction_Check(py_callback))
      printerror((PyObject*)Classic_server);
    else
      n = classic_server(node,NULL,(char)(endchar & 0xFF),keyflag);
    }      
  return Py_BuildValue("i",n); 
  }
  
//void close_all(void);
static PyObject* Close_all(PyObject* self,PyObject* args)
  {
  close_all();
  Py_RETURN_NONE; 
  }
  
//int connect_node(int node,int channelflag,int channel);
static PyObject* Connect_node(PyObject* self,PyObject* args)
  {
  int n,node,flag,channel;

  if(PyObject_Size(args) != 3 || !PyArg_ParseTuple(args,"iii",&node,&flag,&channel))
    {
    printerror((PyObject*)Connect_node);
    n = 0;
    }
  else  
    n = connect_node(node,flag,channel);  
  return Py_BuildValue("i",n); 
  }
  
//char *ctic_name(int node,int cticn);
static PyObject* Ctic_name(PyObject* self,PyObject* args)
  {
  int node,cticn;
  char *buf;
  static char buferr[16] = { "Error" };
  
  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"ii",&node,&cticn))
    {
    printerror((PyObject*)Ctic_name);
    buf = buferr;
    }
  else
    buf = ctic_name(node,cticn);  
  return Py_BuildValue("s",buf);  
  }
  
//int ctic_ok(int node,int cticn);
static PyObject* Ctic_ok(PyObject* self,PyObject* args)
  {
  int n,node,cticn;

  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"ii",&node,&cticn))
    {
    printerror((PyObject*)Ctic_ok);
    n = 0;
    }
  else  
    n = ctic_ok(node,cticn);  
  return Py_BuildValue("i",n); 
  }

//char *device_address(int node);
static PyObject* Device_address(PyObject* self,PyObject* args)
  {
  int node;
  char *buf;
  static char buferr[20] = { "00:00:00:00:00:00" };
  
  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"i",&node))
    {
    printerror((PyObject*)Device_address);
    buf = buferr;
    }
  else
    buf = device_address(node);  
  return Py_BuildValue("s",buf);  
  }
  
//int device_connected(int node);
static PyObject* Device_connected(PyObject* self,PyObject* args)
  {
  int n,node;

  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"i",&node))
    {
    printerror((PyObject*)Device_connected);
    n = 0;
    }
  else
    n = device_connected(node);  
  return Py_BuildValue("i",n); 
  }
  
//int device_info(int mask);
static PyObject* Device_info(PyObject* self,PyObject* args)
  {
  int n,mask;

  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"i",&mask))
    {
    printerror((PyObject*)Device_info);
    n = 0;
    }
  else
    n = device_info(mask);  
  return Py_BuildValue("i",n); 
  }

//char *device_name(int node);
static PyObject* Device_name(PyObject* self,PyObject* args)
  {
  int node;
  char *buf;
  static char buferr[16] = { "Error" };
  
  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"i",&node))
    {
    printerror((PyObject*)Device_name);
    buf = buferr;
    }
  else
    buf = device_name(node);  
  return Py_BuildValue("s",buf);  
  }
  
//int device_type(int node);
static PyObject* Device_type(PyObject* self,PyObject* args)
  {
  int n,node;

  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"i",&node))
    {
    printerror((PyObject*)Device_type);
    n = 0;
    }
  else
    n = device_type(node);  
  return Py_BuildValue("i",n); 
  }

//int disconnect_node(int node);
static PyObject* Disconnect_node(PyObject* self,PyObject* args)
  {
  int n,node;

  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"i",&node))
    {
    printerror((PyObject*)Disconnect_node);
    n = 0;
    }
  else
    n = disconnect_node(node);  
  return Py_BuildValue("i",n); 
  }

//int find_channel(int node,int flag,unsigned char *uuid);
static PyObject* Find_channel(PyObject* self,PyObject* args)
  {
  int n,node,flag,len;
  PyObject *obj;
  unsigned char buf[64];

  n = -1;
  if(PyObject_Size(args) != 3 || !PyArg_ParseTuple(args,"iiO",&node,&flag,&obj))
    printerror((PyObject*)Find_channel);
  else  
    {
    len = objtobuf(obj,buf,64);
    if((flag == UUID_2 && len >= 2) || (flag == UUID_16 && len >= 16))
      n = find_channel(node,flag,buf);
    else
      printf("Find channel invalid UUID\n");
    }  
  return Py_BuildValue("i",n);  
  }
  
//int find_ctics(int node);
static PyObject* Find_ctics(PyObject* self,PyObject* args)
  {
  int n,node;

  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"i",&node))
    {
    printerror((PyObject*)Find_ctics);
    n = -1;
    }
  else  
    n = find_ctics(node);  
  return Py_BuildValue("i",n);  
  }
  
//int find_ctic_index(int node,int flag,char *uuid);
static PyObject* Find_ctic_index(PyObject* self,PyObject* args)
  {
  int n,node,flag,len;
  unsigned char buf[64];
  PyObject *obj;
  
  n = -1;
  if(PyObject_Size(args) != 3 || !PyArg_ParseTuple(args,"iiO",&node,&flag,&obj))
    printerror((PyObject*)Find_ctic_index);
  else  
    {
    len = objtobuf(obj,buf,64);
    if((flag == UUID_2 && len >= 2) || (flag == UUID_16 && len >= 16))
      n = find_ctic_index(node,flag,buf);
    else
      printf("Find ctic index invalid UUID\n");
    }  
  return Py_BuildValue("i",n);  
  }
  
//int hid_key_code(int key);
static PyObject* Hid_key_code(PyObject* self,PyObject* args)
  {
  int n,key;

  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"i",&key))
    {
    printerror((PyObject*)Hid_key_code);
    n = 0;
    }
  else  
    n = hid_key_code(key);  
  return Py_BuildValue("i",n);  
  }  

//int init_blue(char *filename);
static PyObject* Init_blue(PyObject* self,PyObject* args)
  {
  int n;
  char *buf;
  
  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"s",&buf))
    {
    printerror((PyObject*)Init_blue);
    n  = 0;
    }
  else   
    n = init_blue(buf);

  return Py_BuildValue("i",n);  
  }
  
//int init_blue_ex(char *filename,int hcin);
static PyObject* Init_blue_ex(PyObject* self,PyObject* args)
  {
  int n,dev;
  char *buf;
  
  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"si",&buf,&dev))
    {
    printerror((PyObject*)Init_blue_ex);
    n  = 0;
    }
  else   
    n = init_blue_ex(buf,dev);

  return Py_BuildValue("i",n);  
  }
  
//int keys_to_callback(int flag,int keyboard);
static PyObject* Keys_to_callback(PyObject* self,PyObject* args)
  {
  int n,flag,keyboard;
   
  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"ii",&flag,&keyboard))
    {
    printerror((PyObject*)Keys_to_callback);
    n = 0;
    }
  else
    n = keys_to_callback(flag,keyboard);
    
  return Py_BuildValue("i",n);  
  }
  
//int le_pair(int node,int flags,int passkey);
static PyObject* Le_pair(PyObject* self,PyObject* args)
  {
  int n,node,flags,passkey;

  if(PyObject_Size(args) != 3 || !PyArg_ParseTuple(args,"iii",&node,&flags,&passkey))
    {
    printerror((PyObject*)Le_pair);
    n = 0;
    }
  else  
    n = le_pair(node,flags,passkey);  
  return Py_BuildValue("i",n);  
  }

//void le_scan(void);
static PyObject* Le_scan(PyObject* self,PyObject* args)
  {
  le_scan();
  Py_RETURN_NONE; 
  }
  
//int le_server(int(*callback)(),int timerds);
static PyObject* Le_server(PyObject* self,PyObject* args)
  {
  int n,timerds;
  
  n = 0;   
  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"Oi",&py_lecallback,&timerds))
    printerror((PyObject*)Le_server);
  else
    {
    if(!PyFunction_Check(py_lecallback))
      printerror((PyObject*)Le_server); 
    else
      n = le_server(NULL,timerds);
    }      
  return Py_BuildValue("i",n); 
  }

//int list_channels(int node,int flag);
static PyObject* List_channels(PyObject* self,PyObject* args)
  {
  int n,node,flag;

  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"ii",&node,&flag))
    {
    printerror((PyObject*)List_channels);
    n = -1;
    }
  else
    {
    n = list_channels(node,flag);  
    }
  return Py_BuildValue("i",n); 
  }
  
//int list_ctics(int node,int flag);
static PyObject* List_ctics(PyObject* self,PyObject* args)
  {
  int n,node,flag;

  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"ii",&node,&flag))
    {
    printerror((PyObject*)List_ctics);
    n = -1;
    }
  else  
    n = list_ctics(node,flag);  
  return Py_BuildValue("i",n); 
  }
  
//int list_uuid(int node,unsigned char *uuid);
static PyObject* List_uuid(PyObject* self,PyObject* args)
  {
  int n,node,len;
  unsigned char buf[64];
  PyObject *obj;
  
  n = 0;
  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"iO",&node,&obj))
    printerror((PyObject*)List_uuid);
  else  
    {
    len = objtobuf(obj,buf,64);
    if(len >= 2) 
      n = list_uuid(node,buf);
    else
      printf("List uuid invalid UUID\n");
    }  
  return Py_BuildValue("i",n);  
  }

//int localnode(void);
static PyObject* Localnode(PyObject* self,PyObject* args)
  {
  int n;
  
  n = localnode();
  return Py_BuildValue("i",n);   
  }

//void mesh_on(void);
static PyObject* Mesh_on(PyObject* self,PyObject* args)
  {
  mesh_on();
  Py_RETURN_NONE; 
  }
  
//void mesh_off(void);
static PyObject* Mesh_off(PyObject* self,PyObject* args)
  {
  mesh_off();
  Py_RETURN_NONE; 
  }
  
//void mesh_server(int (*callback)());

static PyObject* Mesh_server(PyObject* self,PyObject* args)
  {
  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"O",&py_mecallback))
    printerror((PyObject*)Mesh_server);
  else
    {
    if(!PyFunction_Check(py_mecallback))
      printerror((PyObject*)Mesh_server); 
    else  
      mesh_server(NULL);
    }      
  Py_RETURN_NONE; 
  }

//int node_server(int clientnode,int (*callback)(),char endchar);
static PyObject* Node_server(PyObject* self,PyObject* args)
  {
  int n,node,endchar;
   
  n = 0;   
  if(PyObject_Size(args) != 3 || !PyArg_ParseTuple(args,"iOi",&node,&py_callback,&endchar))
    printerror((PyObject*)Node_server);
  else
    {
    if(!PyFunction_Check(py_callback))
      printerror((PyObject*)Node_server); 
    else
      n = node_server(node,NULL,(char)(endchar & 0xFF));
    }      
  return Py_BuildValue("i",n); 
  }

//int notify_ctic(int node,int cticn,int notifyflag,int (*callback)());
static PyObject* Notify_ctic(PyObject* self,PyObject* args)
  {
  int n,node,cticn,notifyflag;
  
  n = 0;   
  if(PyObject_Size(args) != 4 || !PyArg_ParseTuple(args,"iiiO",&node,&cticn,&notifyflag,&py_ncallback))
    printerror((PyObject*)Notify_ctic);
  else
    {
    if(!PyFunction_Check(py_ncallback))
      printerror((PyObject*)Notify_ctic); 
    else
      n = notify_ctic(node,cticn,notifyflag,NULL);
    }      
  return Py_BuildValue("i",n); 
  }

//int output_file(char *filemame);
static PyObject* Output_file(PyObject* self,PyObject* args)
  {
  int n;
  char *buf;
  
  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"s",&buf))
    {
    printerror((PyObject*)Output_file);
    n = 0;
    }
  else
    n = output_file(buf);  
  return Py_BuildValue("i",n); 
  }

//int read_ctic(int node,int cticn,unsigned char *inbuf,int bufsize);
// Read_ctic(node,cticn)
static PyObject* Read_ctic(PyObject* self,PyObject* args)
  {
  int n,node,cticn;
  PyObject *xobj;
  unsigned char buf[512];
 
  buf[0] = 0;
  n = 0;
  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"ii",&node,&cticn))
    printerror((PyObject*)Read_ctic);
  else
    n = read_ctic(node,cticn,buf,512);
    
  xobj = PyBytes_FromStringAndSize((char*)buf,n);
  if(xobj == NULL)
    printf("Read_ctic fail\n");   
  return(xobj); 
  }

//int read_error(void);
static PyObject* Read_error(PyObject* self,PyObject* args)
  {
  int n;
  
  n = read_error();
  return Py_BuildValue("i",n);   
  }

//int read_mesh(int *node,char *inbuf,int bufsize,int exitflag,int timeoutms);
//    Read_mesh(exitflag,toms)
static PyObject* Read_mesh(PyObject* self,PyObject* args)
  {
  int n,node,exitflag,toms;
  //PyObject *val;
  PyObject *xobj;
  unsigned char buf[64];
  
  buf[0] = 0;
  n = 0;
  node = 0; 
  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"ii",&exitflag,&toms))
    printerror((PyObject*)Read_mesh);
  else
    n = read_mesh(&node,buf,64,exitflag,toms);  
    
  xobj = PyBytes_FromStringAndSize((char*)buf,n);  
  if(xobj == NULL)
    printf("Read_mesh fail\n");
    
  return Py_BuildValue("Oi",xobj,node);   
  }

//int read_node_count(int node,char *inbuf,int count,int exitflag,int timeoutms);
//    Read_node_count(int node,int count,int exitflag,int timeoutms);
static PyObject* Read_node_count(PyObject* self,PyObject* args)
  {
  int n,count,node,exitflag,toms;
  PyObject *xobj;
  unsigned char buf[1024];
  
  buf[0] = 0;
  n = 0;
  if(PyObject_Size(args) != 4 || !PyArg_ParseTuple(args,"iiii",&node,&count,&exitflag,&toms))
    printerror((PyObject*)Read_node_count);
  else
    {
    if(count < 0 || count > 1000)
      printf("Read_node max count = 1000\n");
    else
      n = read_node_count(node,buf,count,exitflag,toms);
    }
    
  xobj = PyBytes_FromStringAndSize((char*)buf,n);
  if(xobj == NULL)
    printf("Read_node fail\n");   
  return(xobj); 
  }

//int read_node_endchar(int node,char *inbuf,int bufsize,char endchar,int exitflag,int timeoutms);
//    Read_node_endchar(node,endchar,exitflag,toms)
static PyObject* Read_node_endchar(PyObject* self,PyObject* args)
  {
  int n,node,exitflag,toms,endchar;
  PyObject *xobj;
  unsigned char buf[1024];
  
  buf[0] = 0;
  n = 0;
  if(PyObject_Size(args) != 4 || !PyArg_ParseTuple(args,"iiii",&node,&endchar,&exitflag,&toms))
    printerror((PyObject*)Read_node_endchar);
  else
    n = read_node_endchar(node,buf,1024,(char)(endchar & 0xFF),exitflag,toms);  
      
  xobj = PyBytes_FromStringAndSize((char*)buf,n);
  if(xobj == NULL)
    printf("Read_node fail\n");   
  return(xobj); 
  }

//int read_all_endchar(int *node,char *inbuf,int bufsize,char endchar,int exitflag,int timeoutms);
//    Read_all_endchar(char endchar,int exitflag,int timeoutms);
static PyObject* Read_all_endchar(PyObject* self,PyObject* args)
  {
  int n,node,exitflag,toms,endchar;
  PyObject *xobj;
  unsigned char buf[1024];

  buf[0] = 0;
  n = 0;  
  if(PyObject_Size(args) != 3 || !PyArg_ParseTuple(args,"iii",&endchar,&exitflag,&toms))
    printerror((PyObject*)Read_all_endchar);
  else
    n = read_all_endchar(&node,buf,1024,(char)(endchar & 0xFF),exitflag,toms);  

  xobj = PyBytes_FromStringAndSize((char*)buf,n);
  if(xobj == NULL)
    printf("Read_node fail\n");   
  return(Py_BuildValue("Oi",xobj,node));   
  }

//void read_node_clear(int node);
static PyObject* Read_node_clear(PyObject* self,PyObject* args)
  {
  int node;

  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"i",&node))
    printerror((PyObject*)Read_node_clear);
  else  
   read_node_clear(node);
   
  Py_RETURN_NONE; 
  }
  
//void read_all_clear(void);
static PyObject* Read_all_clear(PyObject* self,PyObject* args)
  {
  read_all_clear();
  Py_RETURN_NONE; 
  }

//void read_notify(int timeoutms);
static PyObject* Read_notify(PyObject* self,PyObject* args)
  {
  int toms;
  
  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"i",&toms))
    printerror((PyObject*)Read_notify);
  else  
    read_notify(toms);
   
  Py_RETURN_NONE; 
  }

//void register_serial(char *uuid,char *name);
static PyObject* Register_serial(PyObject* self,PyObject* args)
  {
  int len;
  unsigned char buf[64];
  char *name;
  PyObject *obj;
  
  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"Os",&obj,&name))
    printerror((PyObject*)Register_serial);
  else
    {
    len = objtobuf(obj,buf,64);
    if(len == 16)
      register_serial(buf,name);
    else
      printf("Register serial UUID must be 16-byte\n");
    }  
  Py_RETURN_NONE; 
  }

//void scroll_back(void);
static PyObject* Scroll_back(PyObject* self,PyObject* args)
  {
  scroll_back();
  Py_RETURN_NONE; 
  }
  
//void scroll_forward(void);
static PyObject* Scroll_forward(PyObject* self,PyObject* args)
  {
  scroll_forward();
  Py_RETURN_NONE; 
  }

//void set_flags(int flags,int onoff);
static PyObject* Set_flags(PyObject* self,PyObject* args)
  {
  int flags,onoff;

  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"ii",&flags,&onoff))
    printerror((PyObject*)Set_flags);
  else
    set_flags(flags,onoff);  
  Py_RETURN_NONE; 
  }

//int set_le_interval(int min,int max);
static PyObject* Set_le_interval(PyObject* self,PyObject* args)
  {
  int n,min,max;

  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"ii",&min,&max))
    {
    printerror((PyObject*)Set_le_interval);
    n = 0;
    }
  else
    n = set_le_interval(min,max);  
  return Py_BuildValue("i",n); 
  }
  
//int set_le_interval_update(int node,int min,int max);
static PyObject* Set_le_interval_update(PyObject* self,PyObject* args)
  {
  int n,node,min,max;

  if(PyObject_Size(args) != 3 || !PyArg_ParseTuple(args,"iii",&node,&min,&max))
    {
    printerror((PyObject*)Set_le_interval_update);
    n = 0;
    }
  else
    n = set_le_interval_update(node,min,max);  
  return Py_BuildValue("i",n); 
  }
  
//int set_le_interval_server(int node,int min,int max);
static PyObject* Set_le_interval_server(PyObject* self,PyObject* args)
  {
  int n,node,min,max;

  if(PyObject_Size(args) != 3 || !PyArg_ParseTuple(args,"iii",&node,&min,&max))
    {
    printerror((PyObject*)Set_le_interval_server);
    n = 0;
    }
  else
    n = set_le_interval_server(node,min,max);  
  return Py_BuildValue("i",n); 
  }

//void set_le_random_address(unsigned char *add);
static PyObject* Set_le_random_address(PyObject* self,PyObject* args)
  {
  int len;
  PyObject *obj;
  unsigned char buf[64];
  
  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"O",&obj))
    printerror((PyObject*)Set_le_random_address);
  else
    {
    len = objtobuf(obj,buf,64);
    if(len == 6)  
      set_le_random_address(buf);
    else
      printf("Set LE random address not 6 bytes\n");
    }  
  Py_RETURN_NONE; 
  }

//int set_le_wait(int waitms);
static PyObject* Set_le_wait(PyObject* self,PyObject* args)
  {
  int n,wait;

  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"i",&wait))
    {
    printerror((PyObject*)Set_le_wait);
    n = 0;
    }
  else
    n = set_le_wait(wait);  
  return Py_BuildValue("i",n); 
  }
  
//int set_print_flag(int flag);
static PyObject* Set_print_flag(PyObject* self,PyObject* args)
  {
  int n,flag;

  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"i",&flag))
    {
    printerror((PyObject*)Set_print_flag);
    n = 1;
    }
  else
    n = set_print_flag(flag);  
  return Py_BuildValue("i",n); 
  }
  
//char *strtohex(char *s,int *num);
//   Strtohex(char *s)
static PyObject* Strtohex(PyObject* self,PyObject* args)
  {
  int n;
  PyObject *xobj;
  char *s;
  unsigned char *buf;
  unsigned char dummy;
  
  n = 0;
  buf = &dummy;
  dummy = 0;
    
  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"s",&s))
    printerror((PyObject*)Strtohex);
  else
    {
    buf = strtohex(s,&n);
    if(n == 0)
      buf = &dummy;
    }
    
  xobj = PyBytes_FromStringAndSize((char*)buf,n);  
  if(xobj == NULL)
    printf("Strtohex fail\n");   
  return(xobj);
  }

//int wait_for_disconnect(int node,int timout);
static PyObject* Wait_for_disconnect(PyObject* self,PyObject* args)
  {
  int n,node,timout;

  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"ii",&node,&timout))
    {
    printerror((PyObject*)Wait_for_disconnect);
    n = 0;
    }
  else  
    n = wait_for_disconnect(node,timout);  
  return Py_BuildValue("i",n); 
  }
  
//int write_ctic(int node,int cticn,unsigned char *outbuf,int count);
//    Write_ctic(node,cticn,DATA,count)
static PyObject* Write_ctic(PyObject* self,PyObject* args)
  {
  int n,node,cticn,len,count;
  PyObject *obj;
  unsigned char buf[512];
  
  n = 0;
  if(PyObject_Size(args) != 4 || !PyArg_ParseTuple(args,"iiOi",&node,&cticn,&obj,&count))
    printerror((PyObject*)Write_ctic);
  else
    {
    len = objtobuf(obj,buf,512);
    if(len >= 0)
      {
      if(count <= 0)
        count = len;
      else if(len < count)
        {  
        count = len;
        printf("Write ctic data smaller than count\n");
        }
      n = write_ctic(node,cticn,buf,count);
      }
    }  
  return Py_BuildValue("i",n); 
  }

//int write_mesh(char *outbuf,int count);
//    Write_mesh(DATA,count)
static PyObject* Write_mesh(PyObject* self,PyObject* args)
  {
  int n,len,count;
  PyObject *obj;
  unsigned char buf[64];

  n = 0;  
  if(PyObject_Size(args) != 2 || !PyArg_ParseTuple(args,"Oi",&obj,&count))
    printerror((PyObject*)Write_mesh);
  else
    {
    len = objtobuf(obj,buf,64);
    if(len >= 0)
      {
      if(count <= 0)
        count = len;
      else if(len < count)
        {
        count = len;
        printf("Write mesh data smaller than count\n");
        }  
      n = write_mesh(buf,count);
      }
    }  
  return Py_BuildValue("i",n); 
  }
  
//int write_node(int node,unsigned char *outbuf,int count);
//    Write_node(node,DATA,count)
static PyObject* Write_node(PyObject* self,PyObject* args)
  {
  int n,len,node,count,xcount;
  PyObject *obj;
  unsigned char buf[1024]; 
 
  n = 0;
  if(PyObject_Size(args) != 3 || !PyArg_ParseTuple(args,"iOi",&node,&obj,&count))
    printerror((PyObject*)Write_node);
  else
    {
    len = objtobuf(obj,buf,1024);
    if(len >= 0)
      {
      if(count <= 0)
        xcount = len;
      else if(len < count)
        {
        xcount = len;
        printf("Write node data smaller than count\n");
        }
      else
        xcount = count;
        
      n = write_node(node,buf,xcount);
      }
    }
  return Py_BuildValue("i",n); 
  }

static PyObject* Print_data(PyObject* self,PyObject* args)
  {
  int len;
  PyObject *obj,*robj;
  unsigned char buf[1024]; 

  buf[0] = 0;
  len = 0;
  obj = NULL;
    
  if(PyObject_Size(args) != 1 || !PyArg_ParseTuple(args,"O",&obj))
     printerror((PyObject*)Print_data);
  else
    {    
    len = objtobuf(obj,buf,1024);
    if(len < 0)
      printf("DATA Invalid\n");
    else
      {     
      printf("DATA OK. Count = %d\n",len);
      if(len > 0) 
        hexprint(buf,len);
      }
    }
      
  robj = PyBytes_FromStringAndSize((char*)buf,len);
  return(robj); 
  }


int objtobuf(PyObject *obj,unsigned char *buf,int bufsize)
  {
  int n,j,xn,xlen,len,chk;
  Py_ssize_t size;
  unsigned char xbuf[1024];
  const char *dat;
  long valx;
  PyObject *val;
  
  buf[0] = 0;
  
  if(obj == NULL)
    {
    printf("Invalid data\n");
    return(-1);
    }
       
  if(PyUnicode_Check(obj) != 0)
    {
    // UNICODE 
    dat = PyUnicode_AsUTF8AndSize(obj,&size);
    if(dat == NULL)
      {
      printf("String data failed\n");
      return(-1);
      }
    len = size;
    if(len > bufsize)
      {
      printf("Data too long\n");
      return(-1);
      }
    for(n = 0 ; n < len ; ++n)
      buf[n] = dat[n];
    
    return(len);
    } 
  else if(PyBytes_Check(obj) != 0)
    {
    // BYTES
    len = PyBytes_Size(obj);
    if(len > bufsize)
      {
      printf("Data too long\n");
      return(-1);
      }

    dat = PyBytes_AsString(obj);
    if(dat == NULL)
      {
      printf("Bytes data failed\n");
      return(-1);
      }
      
    for(n = 0 ; n < len ; ++n)
      buf[n] = dat[n];
      
    return(len);
    }
  else if(PyByteArray_Check(obj) != 0)
    {
    // BYTEARRAY
    len = PyByteArray_Size(obj);
    if(len > bufsize)
      {
      printf("Data too long\n");
      return(-1);
      }

    dat = PyByteArray_AsString(obj);
    if(dat == NULL)
      {
      printf("Bytearray data failed\n");
      return(-1);
      }
      
    for(n = 0 ; n < len ; ++n)
      buf[n] = dat[n];
      
    return(len);
    }
  else if(PyList_Check(obj) != 0)
    {
    // LIST  
    len = PyList_Size(obj);
  
    xn = 0;
    for(n = 0 ; n < len ; ++n)
      {
      val = PyList_GetItem(obj,n);
      chk = PyLong_Check(val);
      if(chk != 0)
        {
        valx = PyLong_AsLong(val);
        if(valx < 0 || valx > 255)
          {
          printf("List item out of range 0-255\n");
          return(-1);
          }  
        if(xn >= bufsize)
          {
          printf("Data too long\n");
          return(-1);
          }
        buf[xn] = (unsigned char)(valx & 0xFF);
        ++xn;
        }
      else
        {
        // LIST MEMBER re-enter
        xlen = objtobuf(val,xbuf,1024);
        if(xlen < 0)
          return(-1);
        if(xn + xlen >= bufsize)
          {
          printf("Data too long\n");
          return(-1);
          }
               
        for(j = 0 ; j < xlen ; ++j)
          {
          buf[xn] = xbuf[j];
          ++xn;
          } 
        }
      }
    return(xn);
    }
  return(-1);
  }


void printerror(PyObject *obj)
  {
  int n,fn;
  const char *s;

  PyErr_Clear();
  fn = -1;
  n = 0;
  while(fn < 0 && BtfpyMethods[n].ml_name != NULL)
    {
    if((PyObject*)BtfpyMethods[n].ml_meth == obj)
      fn = n;
    ++n;
    }
  if(fn >= 0)
    s = BtfpyMethods[fn].ml_name;
  else
    s = "Method";
    
  printf("%s arguments error - number or type\n",s);   
  }

void hexprint(unsigned char *buf, int len)
  {
  int i,j,i0,n;
  unsigned char c;
  
  if(len <= 0)
    {
    printf("  No data\n");
    return;
    }
  
  i = 0;
  printf(" Index   Data\n");
  do
    {
    i0 = i;
    n = 0;
    printf("  %04X   ",i0);
    do
      {
      if(n == 8)
        printf(" ");
      printf("%02X ",buf[i]);
      ++n;
      ++i;
      }
    while(n < 16 && i < len);

    printf("  ");    
    j = i0;
    n = 0;
    do
      {
      c = buf[j];
      if(c < 32 || c > 126)
        c = '.';
      printf("%c",c);
      ++n;
      ++j;
      }
    while(n < 16 && j < len); 
       
    printf("\n"); 
    }
  while(i < len); 
  }

