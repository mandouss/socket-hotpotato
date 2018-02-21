#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

struct _player{
  int player_ID;
  int socket_rm;
  int num_players;
  int num_hops;
  struct sockaddr_in ps_info;
  struct sockaddr_in rt_addr_info;
  int rt_socket;
  int lt_socket;
};
typedef struct _player player;
typedef struct _player * p_player;
