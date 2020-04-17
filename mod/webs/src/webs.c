#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include "inc/gsf.h"
#include "mod/bsp/inc/bsp.h"

#include "http.h"
#include "mod_call.h"


GSF_LOG_GLOBAL_INIT("WEBS", 8*1024);


int rep_recv(char *in, int isize, char *out, int *osize, int err)
{ 
  int ret = 0, i = 0;
  
  char *p = NULL, *body = NULL;
  char req[128] = {0};
  char args[128] = {0};
  
  char *pin = (char*)malloc(isize+1);
  memcpy(pin, in, isize);
  pin[isize] = '\0';
  
  if(p = strstr(pin, "id:"))
    sscanf(p, "id:%127[^, \r]", req);
  if(p = strstr(pin, "args:"))
    sscanf(p, "args:%127[^, \r]", args);
  
  printf("parse => id:[%s], args:[%s]\n", req, args);
  
  body = strstr(pin, "data:");
  if(body)body+=strlen("data:");
  
  printf("parse => request:%s, data:\n%s\n"
        , req, body);
  
  ret = mod_call(req, args, body, out, *osize);
  
  *osize = strlen(out);
  free(pin);
  return 0;
}

void *web_pub, *web_req, *web_sub;
int upg_stat = 0;

int sub_recv(char *msg, int size, int err)
{
  char msgstr[64];
  gsf_msg_t *pmsg = (gsf_msg_t*)msg;

  if(pmsg->id != GSF_EV_BSP_UPGRADE)
    return 0;

  sprintf(msgstr, "progress:%d", pmsg->err);
  nm_pub_send(web_pub, msgstr, strlen(msgstr));

  if(pmsg->err < 0 || pmsg->err >= 100)
  {
    upg_stat = 0;
  }
  
  return 0;
}

int main(void)
{
  // You can use chrome to open index.html to test;
  
  //nanomsg port;
  web_pub  = nm_pub_listen("ws://*:7789");
  web_req  = nm_rep_listen("ws://*:7790"
                  , NM_REP_MAX_WORKERS
                  , NM_REP_OSIZE_MAX
                  , rep_recv); 
  web_sub = nm_sub_conn("ipc:///tmp/bsp_pub"
                  , sub_recv);

  while(1)
  {
    //register To;
    GSF_MSG_DEF(gsf_mod_reg_t, reg, 8*1024);
    reg->mid = GSF_MOD_ID_WEBS;
    strcpy(reg->uri, "");
    int ret = GSF_MSG_SENDTO(GSF_ID_MOD_CLI, 0, SET, GSF_CLI_REGISTER, sizeof(gsf_mod_reg_t), GSF_IPC_BSP, 2000);
    printf("GSF_CLI_REGISTER To:%s, ret:%d, size:%d\n", GSF_IPC_BSP, ret, __rsize);

    static int cnt = 3;
    if(ret == 0)
      break;
    if(cnt-- < 0)
      return -1;
    sleep(1);
  }


  // udpsend port;
  //rawudp_open("192.168.1.104", 5555);

  //libws port;
  //ws_flv_open(7788);
  
  //mongoose http&ws port;
  http_open(80);

  while(1)
  {
    sleep(1);
  }
  
__err:
  return 0;
}