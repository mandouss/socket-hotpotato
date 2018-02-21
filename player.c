#include "potato.h"

void link_player(p_player p, int p_socket, int ps_socket, int r_port, int ps_port) {
  int sz = sizeof(p->rt_addr_info);
  char temp[100];
  char b[] = "begin";
  char s[] = "success";
  char f[] = "fail";
  //char e[] = "exit";
  memset(temp, 0, sizeof(temp));
  int rc1 = 0, rc2 = 0; 
  while(rc1 != strlen(b)){
    rc1 = rc1 + recv(p_socket, temp, strlen(b), 0); // if "begin" is send to player from ringmster
  }
  if(rc1 < 0) {
    fprintf(stderr, "Receive begin signal from ringmaster fails!\n");
    close(p_socket);
    close(ps_port);
    close(r_port);
    exit(EXIT_FAILURE);
  }
  if(!strncmp(b, temp, strlen(b))) {
    p->rt_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(p->rt_socket < 0) {
      fprintf(stderr, "Create rt_socket fails!\n");
      close(p_socket);
      close(ps_port);
      close(r_port);
      exit(EXIT_FAILURE);
    }
    rc2 = connect(p->rt_socket, (struct sockaddr *)(&(p->rt_addr_info)), sizeof(p->rt_addr_info));
    if(rc2 < 0) {
      perror("connect with rt player ");
      send(p_socket, f, strlen(f), 0);
      //fprintf(stderr, "Connect right player fails!\n");
      //close(p_socket);
      //close(ps_port);
      //exit(EXIT_FAILURE);
    }
  }
  rc2 = send(p_socket, s, strlen(s), 0);
  p->lt_socket = accept(ps_socket, (struct sockaddr *)&p->rt_addr_info, &sz);
  if(p->lt_socket < 0) {
    fprintf(stderr, "accept rt_socket fails!\n");
    close(p_socket);
    close(ps_port);
    close(r_port);
    exit(EXIT_FAILURE);
  }
  printf("successful link!\n");
}

void recv_rt_ps_info(p_player p, int p_socket, int r_port, int ps_port) {
  int rc = 0;
  struct sockaddr_in in_temp;
  printf("receive message!\n");
  while(rc != sizeof(struct sockaddr_in)) {
    rc = rc + recv(p_socket, &in_temp, sizeof(struct sockaddr_in), 0);
  }
  if(rc < 0){
    fprintf(stderr, "Get right player information fails!\n");
    close(p_socket);
    close(ps_port);
    close(r_port);
    exit(EXIT_FAILURE);
  }
  p->rt_addr_info = in_temp;
  if(rc >= 0 ){
    printf("Players set successfully!\n");
  }
}

void setup_server(p_player p, int *ptr_ps_socket, int *ps_port, int r_port, int p_socket){
  //struct sockaddr_in ps_info;
  struct hostent *ps;
  char hostname[100];
  int optval = 1;
  int back_log = 10;
  if((gethostname(hostname, sizeof(hostname))) < 0){
    fprintf(stderr, "Error: gethostname for players fails!\n");
    close(r_port);
    exit(EXIT_FAILURE);
  }
  if((ps = gethostbyname(hostname)) == NULL) {
    fprintf(stderr, "Error: gethostbyname for players fails!\n");
    close(r_port);
    exit(EXIT_FAILURE);
  }
  if((*ptr_ps_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    fprintf(stderr, "Error: player(as server) socket setup fails!\n");
    close(r_port);
    exit(EXIT_FAILURE);
  }
  *ps_port = 10000+p->player_ID;
  p->ps_info.sin_family = AF_INET;
  int rc = -1;
  while(rc < 0){
    p->ps_info.sin_port = htons(*ps_port); 
    memmove(&(p->ps_info.sin_addr), ps->h_addr, ps->h_length);
    rc = bind(*ptr_ps_socket, (struct sockaddr *)&p->ps_info, sizeof(p->ps_info));
    ++(*ps_port);
  }
  if (rc < 0) {
    fprintf(stderr, "player as server bind fail!\n");
    close(*ptr_ps_socket);
    close(*ps_port);
    close(r_port);
    exit(EXIT_FAILURE);
  }
  if(send(p_socket, &(p->ps_info), sizeof(p->ps_info), 0) < 0){
    fprintf(stderr,"send information to ringmaster fails!\n");
    close(*ptr_ps_socket);
    close(*ps_port);
    close(r_port);
    exit(EXIT_FAILURE);
  }
  if((rc=listen(*ptr_ps_socket, back_log)) < 0){
    fprintf(stderr,"ringmaster listen fails!\n");
    close(*ptr_ps_socket);
    close(*ps_port);
    close(r_port);
    exit(EXIT_FAILURE);
  }
  //printf("%d\n", rc);
  //printf("%d\n", *ptr_ps_socket);
}

int setup_client(p_player p, char *arg[], int r_port){
  struct sockaddr_in rm_info;
  struct hostent *ringmaster = gethostbyname(arg[1]);
  int player_socket = socket(AF_INET, SOCK_STREAM, 0);
  rm_info.sin_family = AF_INET;
  rm_info.sin_port = htons(r_port);
  memmove(&(rm_info.sin_addr), ringmaster->h_addr, ringmaster->h_length);
  /* connect to socket at above addr and port */
  if((connect(player_socket, (struct sockaddr *)&rm_info, sizeof(rm_info))) < 0){
    fprintf(stderr, "player connect ringmaster fails!\n");
    close(player_socket);
    close(r_port);
    exit(EXIT_FAILURE);
  }
  //Get the complete player info from master
  int rc = 0;
  while(rc!= sizeof(struct _player)) {
    rc = rc + recv(player_socket, p, sizeof(struct _player), 0);
  }
  if(rc < 0) {
    fprintf(stderr, "player receive from ringmaster fails!\n");
    close(player_socket);
    close(r_port);
    exit(EXIT_FAILURE);
  }
  printf("Connected as player %d\n", p->player_ID);
  //printf("rc: %d", rc);
  //printf("p_socket: %d", player_socket);
  return player_socket;
}

void split_command(int n_arg, char *arg[], int * r_port) {
  if(n_arg != 3) {
    fprintf(stderr, "Usage: ./player <host_name> <port_num>\n");
    exit(EXIT_FAILURE);
  }
  if(gethostbyname(arg[1]) == NULL){
    fprintf(stderr, "host not found!\n");
    exit(EXIT_FAILURE);
  }
  *r_port = atoi(arg[2]);
  if(*r_port < 51015 || *r_port > 51097){
    fprintf(stderr, "Error: the port must be from 51015 to 51097!\n");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv) {
  int r_port, ps_port;
  int p_socket;
  int ps_socket;
  p_player ptr = (p_player)malloc(sizeof(struct _player));
  memset(ptr, 0, sizeof(struct _player));
  split_command(argc, argv, &r_port);
  p_socket = setup_client(ptr, argv, r_port);
  printf("p_socket: %d\n", p_socket);
  setup_server(ptr, &ps_socket, &ps_port, r_port, p_socket);
  recv_rt_ps_info(ptr, p_socket, r_port, ps_port);
  link_player(ptr, p_socket, ps_socket, r_port, ps_port);
  free(ptr);
}
