		/*********************************************
		 * Maitre du calcul du probleme des N reines *
		 *********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "algo.h"    /* juste pour RG et RG_FULL */
#include "rezo.h"
#include "machines.h"

#ifndef MACHINES
  #define MACHINES 1
#endif

#ifndef RG_MASTER
  #define RG_MASTER (RG/5)
#endif

static long nb0 = 0;
static long nb1 = 0;
static long nb2 = 0;

static int niveau = 1;
static int etat[50][4];

static int fini = 0;

int fd[MACHINES];
struct sockaddr_in saddr;
struct sockaddr_in myaddr;
struct hostent *hp;


void mul2 ()
{
   nb1 <<= 1;
   nb2 <<= 1;
   if ((signed) nb1 < 0) { nb1 <<= 1; nb1 >>= 1; nb2++;};
}

int pipo_next ()
{
/* on utilise etat pour sauvegarder completement les variables locales */
/* pour pouvoir ne plus etre recursif.... */
   unsigned int h = etat[niveau][0];
   unsigned int m = etat[niveau][1];
   unsigned int d = etat[niveau][2];
   unsigned int i = etat[niveau][3];
   unsigned int tot = (h|m|d) ^ RG_FULL;

   if (niveau == 1)
     tot = (RG_FULL >> (RG/2));


   if ((niveau == 2) && ((RG&1) == 1) && (etat[2][0] == (1 << (RG/2))) )
     tot &= (RG_FULL >> (RG/2));

   for (; i <= tot; i <<= 1)
     if ( (tot&i) != 0 )
     {
       etat[niveau][3] = i << 1;
       niveau ++;
       etat[niveau][0] = h|i;
       etat[niveau][1] = ((m|i)<<1)&RG_FULL;
       etat[niveau][2] = (d|i)>>1;
       etat[niveau][3] = 1;
       if (niveau > RG_MASTER) {
         return 1;
       }
       else  return pipo_next ();
     };

    niveau --;
    if (niveau == 0) return -1; /* fin */
    return 0;
}

void grand_pipo (int ma)
{
  int res;
  long buf[3];
  int i;

  /* regarde s'il reste des calculs a donner, et compte les machines qui ont
fini */
  if (fini>0) { fini++; return;};

  /* attend de trouver qqchose d'interessant */
  /* ie la fin des calculs, ou un calcul a faire */
  while ((res = pipo_next ())==0);
  if (res == -1) {fini = 1; return;};

  /* on convertit avant d'envoyer sur le rezo */
  for (i=0; i < 3; i++)
    buf[i] = htonl (etat[niveau][i]);
  write (fd[ma], buf, 3 * sizeof (long));
  printf ("(%2d) %d %d %d %d %d %d %d\n", ma, etat[0][3], etat[1][3], 
          etat[2][3], etat[3][3], etat[4][3], etat[5][3], etat[6][3]);
  /* ce niveau a ete fait par le client. Il faut donc redescendre */
  niveau --;
}

void itere_pipo ()
{
   int i;
   fd_set rd;

   etat[1][0] = etat[1][1] = etat[1][2] = 0;
   etat[1][3] = 1;
   /* Lance un premier calcul sur chaque machine */
   for (i=0; i < MACHINES; i++)
      grand_pipo (i);

   /* relance des calculs quand il y en a besoin */
   while (fini < MACHINES)
   {
      FD_ZERO (&rd);
      for(i=0; i < MACHINES; i++)
	FD_SET (fd[i], &rd);
      /* on attend qu'un des esclaves dise qqchose... */
      select (256, &rd, 0, 0, 0);
      for (i=0; i < MACHINES; i++)
         if (FD_ISSET (fd[i], &rd))
         {
	    read (fd[i], &nb0, sizeof (long));
            /* on recupere le resultat */
	    nb1 += ntohl (nb0);
	    if ((signed) nb1 < 0) { nb1 <<= 1; nb1 >>= 1; nb2++;};
	    grand_pipo (i);
	 };
   };
}

int main ()
{
  int i;
  memset (&saddr, 0, sizeof (struct sockaddr_in));
  memset (&myaddr, 0, sizeof (struct sockaddr_in));

  for(i=0; i<MACHINES; i++)
  {
     if ((hp = gethostbyname (machines[i])) == NULL) {
       saddr.sin_addr.s_addr = inet_addr (machines[i]);
       if (saddr.sin_addr.s_addr == -1) {
	 fprintf (stderr, "Unknown host: %s\n", machines[i]);
	 exit (1);
       };
     }
     else
       saddr.sin_addr = *(struct in_addr *) (hp->h_addr_list[0]);

     saddr.sin_port = htons(PORT);
     saddr.sin_family = AF_INET;

     myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
     myaddr.sin_port = 0;
     myaddr.sin_family = AF_INET;

     if ((fd[i] = socket (PF_INET, SOCK_STREAM, 0)) < 0) {
       perror ("socket");
       exit (1);
     };
     if (connect (fd[i], (struct sockaddr *) &saddr, sizeof (saddr))) {
       fprintf(stderr," Connection a : %s\n",machines[i]);
       perror ("connection au serveur...");
       exit (-1);
     };
     {
       long sonrg;
       read(fd[i],&sonrg,sizeof(long));
       if(ntohl(sonrg) != RG)
       {
          fprintf(stderr," Le client sur %s a un RG de %d et moi de %d\n",
	          machines[i],ntohl(sonrg),RG);
       }
     }
  };

   itere_pipo ();
   printf ("%d reines -> %d + (%d * 2^%d ) sols\n",
	   RG, nb1, nb2, 8 * sizeof (long));
}
