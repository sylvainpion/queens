		/**********************************************
		 * Esclave du calcul du probleme des N reines *
		 **********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#include "algo.h"
#include "rezo.h"

int fd;
struct sockaddr_in saddr;

main()
{
  int ns;
  unsigned long buf[10];
  struct hostent *luient;
  struct sockaddr_in lui;
  int l = sizeof (lui);

  memset (&saddr, 0, sizeof (struct sockaddr_in));
  saddr.sin_port = htons (PORT);
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = INADDR_ANY;

  if ((fd = socket (PF_INET, SOCK_STREAM, 0)) < 0) {
    perror ("socket");
    exit (1);
  };

  if (bind (fd, (struct sockaddr *) &saddr, sizeof (saddr)) < 0) {
    perror ("bind");
    exit (1);
  };

  if (listen (fd, 5) < 0) {
    perror ("listen");
    exit (1);
  };

  printf ("Attente de connections, port %d\n", PORT);
  ns = accept (fd, (struct sockaddr *) &lui, &l);
  printf ("Connection !!!\n");
  buf[0] = htonl(RG);
  write (ns, buf, sizeof (long));
  for (;;)
  {
     read (ns, buf, 4*sizeof (long));
     nb = 0;
     pipo_str (ntohl (buf[0]), ntohl (buf[1]), ntohl (buf[2]));
     nb = htonl (nb);
     write (ns, &nb, sizeof (long));
  };
}
