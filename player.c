#include "potato.h"
#include <time.h>
int max(int s1, int s2, int s3){
  if(s1 < s2){
    if(s2 < s3){
      return s3;
    }
    else{
      return s2;
    }
  }
  else{
    if(s1 < s3){
      return s3;
    }
    else{
      return s1;
    }
  }
}

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
    close(ps_socket);
    close(p_socket);
    exit(EXIT_FAILURE);
  }
  if(!strncmp(b, temp, strlen(b))) {
    p->rt_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(p->rt_socket < 0) {
      fprintf(stderr, "Create rt_socket fails!\n");
      close(p_socket);
      close(ps_socket);
      exit(EXIT_FAILURE);
    }
    if(connect(p->rt_socket, (struct sockaddr *)(&(p->rt_addr_info)), sizeof(p->rt_addr_info)) < 0) {
      send(p_socket, f, strlen(f), 0);
      fprintf(stderr, "Connect right player fails!\n");
      close(p_socket);
      close(ps_socket);
      exit(EXIT_FAILURE);
    }
  }
  rc2 = send(p_socket, s, strlen(s), 0);
  p->lt_socket = accept(ps_socket, (struct sockaddr *)&p->rt_addr_info, &sz);
  if(p->lt_socket < 0) {
    fprintf(stderr, "accept rt_socket fails!\n");
    close(p_socket);
    close(ps_socket);
    exit(EXIT_FAILURE);
  }
  //printf("successful link! Player ID is %d. rt_socket is %d, lt_socket is %d\n", p->player_ID, p->rt_socket, p->lt_socket);
}

void recv_rt_ps_info(p_player p, int p_socket, int r_port, int ps_port) {
  int rc = 0;
  struct sockaddr_in in_temp;
  //printf("receive message!\n");
  while(rc != sizeof(struct sockaddr_in)) {
    rc = rc + recv(p_socket, &in_temp, sizeof(struct sockaddr_in), 0);
  }
  if(rc < 0){
    fprintf(stderr, "Get right player information fails!\n");
    close(r_port);
    close(ps_port);
    exit(EXIT_FAILURE);
  }
  p->rt_addr_info = in_temp;
  //if(rc >= 0 ){
    //printf("Players set successfully! ps_port is %d\n, p_socket is %d\n", ps_port, p_socket);
  //}
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
  if(player_socket < 0) {
    exit(EXIT_FAILURE);
  }
  rm_info.sin_family = AF_INET;
  rm_info.sin_port = htons(r_port);
  memmove(&(rm_info.sin_addr), ringmaster->h_addr, ringmaster->h_length);
  /* connect to socket at above addr and port */
  if((connect(player_socket, (struct sockaddr *)&rm_info, sizeof(rm_info))) < 0){
    //fprintf(stderr, "player connect ringmaster fails!\n");
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
  //printf("Connected as player %d\n", p->player_ID);
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
  printf("Connected as player %d out of %d total players\n", ptr->player_ID, ptr->num_players);
  setup_server(ptr, &ps_socket, &ps_port, r_port, p_socket);
  recv_rt_ps_info(ptr, p_socket, r_port, ps_port);
  link_player(ptr, p_socket, ps_socket, r_port, ps_port);
  //send(ptr->rt_socket, "right", 5 , 0);
  //char buf[6];
  //recv(ptr->lt_socket, buf, 5, 0);
  fd_set readfds;
  int i = 0;
  char h[] = "head";
  char t[] = "tail";
  //char c[] = "clean";
  char e[] = "exit";
  char temp[100];
  int n_players = ptr ->num_players;
  int n_hops = ptr -> num_hops;
  int potato[n_hops];
  memset(potato, -1, sizeof(int) * n_hops);
  srand((unsigned int)time(NULL) + ptr -> player_ID);
  //printf("%d", potato[0]);
  while(1){
    int rc = 0;
    FD_ZERO(&readfds);
    FD_SET(p_socket, &readfds);
    FD_SET(ptr -> rt_socket, &readfds);
    FD_SET(ptr -> lt_socket, &readfds);
    if(select(max(p_socket, ptr -> rt_socket, ptr -> lt_socket)+1, &readfds, NULL, NULL, NULL) < 0){
      close(r_port);
      close(ps_port);
      close(p_socket);
      close(ps_socket);
      fprintf(stderr, "select mode fails!\n");
      exit(EXIT_FAILURE);
    }
    //printf("after select\n");
    if(FD_ISSET(p_socket, &readfds)) {
      while(rc != 4) {
	rc = rc + recv(p_socket, temp, 4, 0);
      }
      if(!strncmp(h, temp, strlen(h))) {
	rc = 0;
	//potato = (int *)malloc(n_hops * sizeof(int));
	while(rc != n_hops * sizeof(int)) {
	  rc = rc + recv(p_socket, potato + (rc/sizeof(int)), sizeof(int) * n_hops, 0);
	  // memset(potato, -1, potato);
	  //printf("I get potato from master!\n");
	}
      }
      else if(!strncmp(e, temp, strlen(e))) {
	close(ps_port);
	close(r_port);
	close(ps_socket);
	close(ptr -> rt_socket);
	close(ptr -> lt_socket);
	exit(0);
      }
    }
    else if(FD_ISSET(ptr->lt_socket, &readfds)) {
      rc = 0;
      while(rc != n_hops * sizeof(int)) {
	rc = rc + recv(ptr -> lt_socket, potato + (rc/sizeof(int)), sizeof(int) * n_hops, 0);
	//printf("get potato from left!\n");
      }
    }
    else if(FD_ISSET(ptr->rt_socket, &readfds)) {
      rc = 0;
      while(rc != n_hops * sizeof(int)) {
	rc = rc + recv(ptr -> rt_socket, potato + (rc/sizeof(int)), sizeof(int) * n_hops, 0);
      }
    }
    if(rc < 0) {
      fprintf(stderr, "send potato to another fails!\n");
      close(ps_port);
      close(r_port);
      close(ps_socket);
      close(ptr -> rt_socket);
      close(ptr -> lt_socket);
      exit(EXIT_FAILURE);
    }
    for(i = 0; i < n_hops; i++) {
      if(potato[i] == -1) {
	potato[i] = ptr->player_ID;
	break;
      }
    }
    if( i == n_hops - 1 ){
      printf("I'm it\n");
      if(send(p_socket, potato, n_hops * sizeof(int), 0) < 0){
	fprintf(stderr, "send potato back to ringmaster fails!\n");
	close(ps_port);
	close(r_port);
	close(ps_socket);
	close(ptr -> rt_socket);
	close(ptr -> lt_socket);
	exit(EXIT_FAILURE);
      }
      continue;
    } 
    int random = rand(); //send to neighbor
    if(ptr -> player_ID == 0) {
      rc = 0;
      if(random % 2 == 0) { //send to left
	printf("Sending potato to %d\n", n_players - 1);
	rc = send(ptr -> lt_socket, potato, n_hops * sizeof(int), 0);
      }
      else {
	printf("Sending potato to %d\n", ptr ->player_ID + 1);
	rc = send(ptr -> rt_socket, potato, n_hops * sizeof(int), 0);
      }
    }
    else if(ptr -> player_ID == n_players - 1){
      if(random % 2 == 0) { //send to left
	printf("Sending potato to %d\n", ptr -> player_ID - 1);
	rc = send(ptr -> lt_socket, potato, n_hops * sizeof(int), 0);
      }
      else { //send to right
	printf("Sending potato to %d\n", 0);
	rc = send(ptr -> rt_socket, potato, n_hops * sizeof(int), 0);
      }
    }
    else{
      if(random % 2 == 0) { //send to left
	printf("Sending potato to %d\n", ptr -> player_ID - 1);
	rc = send(ptr -> lt_socket, potato, n_hops * sizeof(int), 0);
      }
      else { //send to right
	printf("Sending potato to %d\n", ptr -> player_ID + 1);
	rc = send(ptr -> rt_socket, potato, n_hops * sizeof(int), 0);
      }
    }
    if(rc < 0) {
      fprintf(stderr, "send fails!\n");
      close(ps_port);
      close(r_port);
      close(ps_socket);
      close(ptr -> rt_socket);
      close(ptr -> lt_socket);
      exit(EXIT_FAILURE);
    }
  }
  free(ptr);
}
