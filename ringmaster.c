#include "potato.h"
#include <time.h>

void link_player(int *sockfd, int n_players, int r_port) {
  char b[] = "begin";
  char s[] = "success";
  char f[] = "failure";
  char e[] = "exit";
  char temp[100];
  memset(temp, 0, sizeof(temp));
  for(int i = 0; i < n_players; i++) {
    int rc1 = 0, rc2 = 0;
    while(rc1 != strlen(b)){
      rc1 = rc1 + send(sockfd[i], b, strlen(b), 0);
    }
    if(rc1 < 0) {
      fprintf(stderr, "Send begin siganl fails!\n");
      close(r_port);
      exit(EXIT_FAILURE);
    }
    while(rc2 != strlen(s)) {
      rc2 = rc2 +recv(sockfd[i], temp, strlen(s), 0);
    }
    if(rc2 < 0) {
      fprintf(stderr, "receive from player fails!\n");
      close(r_port);
      exit(EXIT_FAILURE);
    }
    if(!strncmp(s, temp, strlen(s))) {
      //printf("successful link! sockfd is %d\n", sockfd[i]);
      continue;
    }
    else{
      fprintf(stderr, "Ring between player %d and player %d\n fails!\n", i, i + 1);
      close(r_port);
      exit(EXIT_FAILURE);
    }
  }
  //printf("successful link!\n");
}

void send_rt_ps_info(int *sockfd, struct sockaddr_in * ps_info_arr, int n_players, int r_port){
  for(int i = 0; i < n_players; i++) {
    //printf("send information\n");
    int rc = 0;
    while(rc != sizeof(struct sockaddr_in)) {
      rc = rc + send(sockfd[i], &ps_info_arr[(i+1)% n_players], sizeof(struct sockaddr_in), 0);
    }
    if(rc < 0){
      fprintf(stderr, "Send player's server information fails!\n");
      close(r_port);
      exit(EXIT_FAILURE);
    }
  }
  //printf("Ringmaster sets successfully\n");
} 


struct sockaddr_in * receive_players_info(int *sockfd, int n_players, int r_port) {
  struct sockaddr_in * ps_info_arr = (struct sockaddr_in *)malloc(n_players * sizeof(struct sockaddr_in));
  for(int i = 0; i < n_players; i++) {
    int rc = 0;
    while(rc != sizeof(struct sockaddr_in)) {
      rc = rc + recv(sockfd[i], &ps_info_arr[i], sizeof(struct sockaddr_in), 0);
    }
    if(rc < 0){
      fprintf(stderr, "Get player's server information fails!\n");
      close(r_port);
      exit(EXIT_FAILURE);
    }
  }
  return ps_info_arr;
}

//check command parameters
void split_command(int n_arg, char *arg[], int * p_port, int * p_players, int * p_hops) {
  if(n_arg != 4){
    fprintf(stderr, "Usage: ./ringmaster <port_num> <num_players> <num_hops>\n");
    exit(EXIT_FAILURE);
  }
  *p_port = atoi(arg[1]);
  if(*p_port < 51015 || *p_port > 51097) {
    fprintf(stderr, "Error: the port must be from 51015 to 51097!\n");
    exit(EXIT_FAILURE);
  }
  *p_players = atoi(arg[2]);
  if(*p_players < 2) {
    fprintf(stderr, "Error: the number of players must be larger than one!\n");
    exit(EXIT_FAILURE);
  }
  *p_hops = atoi(arg[3]);
  if(*p_hops < 0 || *p_hops > 512) {
    fprintf(stderr, "Error: the number of hops must be from 0 to 512!\n");
    exit(EXIT_FAILURE);
  }
}

//get ringmaster ip and port 
struct hostent * get_host_info(char *hostname, struct hostent * host, size_t len) {
  //int z;
  if((gethostname(hostname, len)) < 0 ) {
    fprintf(stderr, "Error: gethostname fails!\n");
    exit(EXIT_FAILURE);
  }
  if((host = gethostbyname(hostname)) == NULL) {
    fprintf(stderr, "Error: gethostbyname fails!\n");
    exit(EXIT_FAILURE);
  }
  return host;
}

//set up ringmaster
void setup_ringmaster(int *ptr_s, struct hostent * rm, int r_port) {
  struct sockaddr_in rm_info;
  int optval = 1;
  int back_log = 10;
  if((*ptr_s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    fprintf(stderr, "Error: ringmaster socket setup fails!\n");
    exit(EXIT_FAILURE);
  }
  rm_info.sin_family = AF_INET;
  rm_info.sin_port = htons(r_port);
  memmove(&(rm_info.sin_addr), rm->h_addr, rm->h_length);
  //printf("1.address: %s\n", inet_ntop(rm->h_addrtype, *pptr, str, sizeof(str))); 
  //printf("1.%d\n", rm_info.sin_addr.s_addr);
  //printf("2.%s\n", inet_ntoa(rm_info.sin_addr));
  if(bind(*ptr_s, (struct sockaddr *)&rm_info, sizeof(rm_info)) < 0){
    fprintf(stderr, "ringmaster bind fail!\n");
    close(*ptr_s);
    exit(EXIT_FAILURE);
  }
  setsockopt(*ptr_s, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
  if(listen(*ptr_s, back_log) < 0){
    fprintf(stderr,"ringmaster listen fails!\n");
    close(*ptr_s);
    exit(EXIT_FAILURE);
  }
  //printf("%d\n", *ptr_s);
}

void accept_connect(int *sockfd, p_player *p, int n_players, int rm_socket, int r_port, int n_hops) {
  struct sockaddr_in player_info;
  int temp_s = rm_socket;
  int port = r_port;
  int addr_size = sizeof(player_info);
  for(int i = 0; i < n_players; i++) {
    p[i] = (p_player)malloc(sizeof(struct _player));
    memset(p[i], '0', sizeof(struct _player));
    if(p[i] == NULL) {
      fprintf(stderr, "malloc fails!\n");
      exit(EXIT_FAILURE);
    }
    if((sockfd[i] = accept(temp_s, (struct sockaddr *)&player_info, &addr_size)) < 0) {
      fprintf(stderr, "ringmaster accept fails!\n");
      close(temp_s);
      exit(EXIT_FAILURE);
    }
    //player_host = gethostbyaddr((char *)&player_info.sin_addr, sizeof(struct in_addr), AF_INET);
    p[i] -> player_ID = i;
    p[i] -> socket_rm = temp_s; 
    p[i] -> num_players = n_players;
    p[i] -> num_hops = n_hops;
    printf("Player %d is ready to play\n", p[i]->player_ID);
    if(send(sockfd[i], p[i], sizeof(struct _player), 0) < 0) {
      close(temp_s);
      close(port);
      exit(EXIT_FAILURE);
    }
  }
}

void exit_clean(int * sockfd, int n_players, int r_port, int rm_socket){
  char e[] = "exit";
  char c[] = "clean";
  for(int i = 0; i < n_players; i++) {
    if(send(sockfd[i], e, strlen(e), 0) < 0){
      fprintf(stderr, "send exit signal fails!\n");
      close(rm_socket);
      close(r_port);
      exit(EXIT_FAILURE);
    }
    /*if(send(sockfd[i], c, strlen(c), 0) < 0){
      fprintf(stderr, "send close signal fails!\n");
      close(rm_socket);
      close(r_port);
      exit(EXIT_FAILURE);
      }*/
  }
}

void dont_play_game(int rm_socket, int n_players,int n_hops, int r_port, int * sockfd){
  exit_clean(sockfd, n_players, r_port, rm_socket);
  printf("Trace of potato: \n");  
}

void potato2player(fd_set readfds, int * sockfd, int *potato,int n_players, int n_hops, int r_port, int rm_socket) {
  char h[] = "head";
  srand( (unsigned int) time(NULL) );
  int random= rand()% n_players;
  printf("Ready to start the game, sending potato to player: %d\n", random);
  if(send(sockfd[random], h, strlen(h), 0) < 0){
    fprintf(stderr, "send initialization fails!\n");
    close(rm_socket);
    close(r_port);
    exit(EXIT_FAILURE);
  }
  if(send(sockfd[random], potato, sizeof(int)*n_hops, 0) < 0){
    fprintf(stderr, "send potato fails!\n");
    close(rm_socket);
    close(r_port);
    exit(EXIT_FAILURE);
  }
}

void print_trace(int * potato, int n_hops) {
  printf("Trace of potato: \n");
  int i;
  for(i = 0; i < n_hops - 1; i++) {
    printf("%d(i = %d), ", potato[i], i);
  }
  printf("%d(i = %d)\n", potato[i], i);
}

void play_game(int n_players, int n_hops, int r_port, int rm_socket, int * sockfd, int * potato) {
  int i;
  fd_set readfds;
  FD_ZERO(&readfds);
  for(int i = 0; i < n_players; i++){
    FD_SET(sockfd[i], &readfds);
  }
  
  potato2player(readfds, sockfd, potato, n_players, n_hops, r_port, rm_socket);

  //search the end player
  if(select(65535, &readfds, NULL, NULL, NULL) < 0) {
    fprintf(stderr, "select mode fails!\n");
    close(rm_socket);
    close(r_port);
    exit(EXIT_FAILURE);
  }
  for(i = 0; i < n_players; i++) {
    if(FD_ISSET(sockfd[i],&readfds)){
      FD_CLR(sockfd[i], &readfds);
      break;
    }
  }
  int rc = 0;
  while(rc!= (n_hops) * sizeof(int)) {
    rc = rc + recv(sockfd[i], potato + rc/sizeof(int), n_hops*sizeof(int), 0);
  }
  if(rc < 0){
    fprintf(stderr, "receive potato trace fails!\n");
    close(r_port);
    close(rm_socket);
    exit(EXIT_FAILURE);
  }
  exit_clean(sockfd, n_players, r_port, rm_socket);
  print_trace(potato, n_hops);
}

int main(int argc, char *argv[]){
  int r_port, n_players, n_hops; //ringmaster server port
  int rm_socket; //ringmaster socket
  char hostname[100];
  
  split_command(argc, argv, &r_port, &n_players, &n_hops);

  int sockfd[n_players]; //players' socket
  p_player p[n_players]; //player informtion
  
  int potato[n_hops];
  memset(potato, -1, n_hops * sizeof(int));

  struct hostent *ringmaster = NULL;
  struct sockaddr_in player_addr_info;
  struct sockaddr_in *ps_info_arr = NULL;

  ringmaster = get_host_info(hostname, ringmaster, sizeof(hostname));
  
  printf("Potato Ringmaster");
  printf("Players = %d \n", n_players);
  printf("Hops = %d\n", n_hops);

  setup_ringmaster(&rm_socket, ringmaster, r_port); // initialize ringmaster as server
  accept_connect(sockfd, p, n_players, rm_socket, r_port, n_hops); //waiting players(clients) connect
  ps_info_arr = receive_players_info(sockfd, n_players, r_port); // get players address.. from players
  send_rt_ps_info(sockfd, ps_info_arr, n_players, r_port);  //send player address.. of its right neighbor
  link_player(sockfd, n_players, r_port);
  
  if(n_hops == 0){ // no game
    dont_play_game(rm_socket, n_players, n_hops, r_port, sockfd);
  }
  else{
    int i;
    fd_set readfds;
    FD_ZERO(&readfds);
    for(int i = 0; i < n_players; i++){
      FD_SET(sockfd[i], &readfds);
    }
    char h[] = "head";
    srand( (unsigned int) time(NULL) );
    int random= rand()% n_players;

    //send potato to random
    printf("Ready to start the game, sending potato to player: %d\n", random);
    if(send(sockfd[random], h, strlen(h), 0) < 0){ // first player siganl
      fprintf(stderr, "send initialization fails!\n");
      close(rm_socket);
      exit(EXIT_FAILURE);
    }
    if(send(sockfd[random], potato, sizeof(int)*n_hops, 0) < 0){ //send potato
      fprintf(stderr, "send potato fails!\n");
      close(rm_socket);
      exit(EXIT_FAILURE);
    }
    
    //wait signal to end the game
    if(select(65535, &readfds, NULL, NULL, NULL) < 0) {
      fprintf(stderr, "select mode fails!\n");
      close(rm_socket);
      exit(EXIT_FAILURE);
    }
    for(i = 0; i < n_players; i++) {
      if(FD_ISSET(sockfd[i],&readfds)){
	FD_CLR(sockfd[i], &readfds);
	break;
      }
    }
    int rc = 0;
    while(rc!= (n_hops) * sizeof(int)) {
      rc = rc + recv(sockfd[i], potato + rc/sizeof(int), n_hops*sizeof(int), 0);
    }
    if(rc < 0){
      fprintf(stderr, "receive potato trace fails!\n");
      close(r_port);
      close(rm_socket);
      exit(EXIT_FAILURE);
    }
    exit_clean(sockfd, n_players, r_port, rm_socket);
    print_trace(potato, n_hops);
    
    //play_game(n_players, n_hops, r_port, rm_socket, sockfd, potato);
  }
  for(int i = 0; i < n_players; i++) {
    free(p[i]);
  }
  free(ps_info_arr);
}
