		/**********************************************
		 * Esclave du calcul du probleme des N reines *
		 **********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "algo.h"
#include "rezo.h"

char *maitre="holiday";

int fd;
struct sockaddr_in saddr;
struct sockaddr_in myaddr;

int main ()
{
  int un=1;
  unsigned short rgpt = htons(RG);
  unsigned int buf[10];
  struct hostent *hp;

  memset (&saddr, 0, sizeof (struct sockaddr_in));
  memset (&myaddr, 0, sizeof (struct sockaddr_in));

  if ((hp = gethostbyname (maitre)) == NULL) {
    saddr.sin_addr.s_addr = inet_addr (maitre);
    if (saddr.sin_addr.s_addr == -1) {
      fprintf (stderr, "Unknown host: %s\n", maitre);
      exit(1);
    };
  }
  else
    saddr.sin_addr = *(struct in_addr *) (hp->h_addr_list[0]);

  saddr.sin_port = htons(PORT);
  saddr.sin_family = AF_INET;

  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = 0;
  myaddr.sin_family = AF_INET;

  if ((fd = socket (PF_INET, SOCK_STREAM, 0)) < 0) {
    perror ("socket");
    exit(1);
  };

  if (setsockopt (fd,SOL_SOCKET,SO_KEEPALIVE,&un,sizeof(int)) < 0) {
    perror ("setsockopt");
    exit(1);
  };

  if (connect(fd, (struct sockaddr *) &saddr, sizeof (saddr)))
  {
    perror ("Connection...");
    exit(1);
  }


  write (fd, &rgpt, sizeof (unsigned short int));

  for (;;)
  {
     read (fd, buf, 3*sizeof (unsigned int));
     nb = 0;
     pipo_str (ntohl (buf[0]), ntohl (buf[1]), ntohl (buf[2]));
     nb = htonl (nb);
     write (fd, &nb, sizeof (unsigned int));
  };
}
