		/*********************************************
		 * Maitre du calcul du probleme des N reines *
		 *********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

#include "algo.h"    /* juste pour RG et RG_FULL */
#include "rezo.h"
#include "machines.h"

#ifndef MACHINES
  #define MACHINES 1
#endif

#ifndef RG_MASTER
  #define RG_MASTER (RG/5)
#endif

#define MODULO 65536

static unsigned int nb0 = 0;
static unsigned int nb1 = 0;
static unsigned int nb2 = 0;

static int niveau = 1;
static int etat[50][4];
static int nombre_machines=0;
static unsigned int calcul_en_cours[MACHINES][4];
static unsigned int *calcul_a_refaire[MACHINES];
static int calcul_bidon[] = {-1,-1,-1};
static int to_redo = 0;

static int fini = 0;

int fd[MACHINES];
int ok[MACHINES];
int fini_machine[MACHINES];
struct sockaddr_in saddr;
struct sockaddr_in myaddr;
struct hostent *hp;

/* tentative de gestion des pb des fils... */
static void handle_pipe(int nosig)
{
  fprintf(stderr," Un pb de socket ... :-((( \n");
  fprintf(stderr, " J'ignore le signal... \n");
  signal(SIGPIPE,handle_pipe);
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

void mort_client(int ma)
{
     int i;
     perror("Ecriture :");
     ok[ma]=0;
     nombre_machines --;
     /* pour faire recommencer le meme... */
     printf ("%s nous a quittee..., il reste %d machines.",machines[ma],
       nombre_machines);
     calcul_a_refaire[to_redo]=calcul_en_cours[ma];
     to_redo++;
     for (i=0;i<MACHINES;i++)
     {
        if (fini_machine[i]&& ok[i])
        {
            fini--;
            fini_machine[i]=0;
            fprintf(stderr,"Je reveille %s...\n",machines[i]);
            memcpy(calcul_en_cours[i],calcul_bidon,3*sizeof(int));
            if(write(fd[i],calcul_bidon,
              3*sizeof(int)) !=3*sizeof(int))
            {
               mort_client(i); /* la on deprime un peu... */
            }
        }
     }
}


void grand_pipo (int ma)
{
  int res;
  int i;

  /* regarde s'il reste des calculs a donner,
   * et compte les machines qui ont fini      */

  if (fini>0)
  {
    fini++;
    fini_machine[ma]=1;
    printf ("             %d a termine.\n", ma);
    return;
  };

  /* attend de trouver qqchose d'interessant
   * ie la fin des calculs, ou un calcul a faire */

  while ((res = pipo_next ())==0);
  if (res == -1)
  {fini_machine[ma]=0;fini = 1;
    printf ("             %d a termine.\n", ma);
    return;
  }

  /* on convertit avant d'envoyer sur le rezo */

  for (i=0; i < 3; i++)
    calcul_en_cours[ma][i] = htonl (etat[niveau][i]);
  res = write (fd[ma], calcul_en_cours[ma], 3 * sizeof (unsigned int));
  if (res != 3* sizeof(unsigned int))
  {
     /* pour faire recommencer le meme... */
     mort_client(ma);
  }
  printf ("(%2d) %d %d %d %d %d %d %d\n", ma, etat[0][3], etat[1][3], 
          etat[2][3], etat[3][3], etat[4][3], etat[5][3], etat[6][3]);

  /* ce niveau a ete fait par le client.
   * Il faut donc redescendre */

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
      if(ok[i])
         grand_pipo (i);

   /* relance des calculs quand il y en a besoin */

   if(nombre_machines==0)
   {
      fprintf(stderr,"Faut pas se foutre du monde... Je suis le MAITRE,\n");
      fprintf(stderr,"je ne vais quand meme pas bosser...\n");
      exit(-1);
   }
   while (fini < nombre_machines)
   {
      FD_ZERO (&rd);
      for(i=0; i < MACHINES; i++)
	FD_SET (fd[i], &rd);

      /* on attend qu'un des esclaves dise qqchose... */

      if(nombre_machines==0)
      { fprintf(stderr, "Y'a plus personne pour bosser\n");
        exit(1);
      }
      select (256, &rd, 0, 0, 0);
      for (i=0; i < MACHINES; i++)
         if ((FD_ISSET (fd[i], &rd))&& (ok[i]))
         {
	    if(read (fd[i], &nb0, sizeof (unsigned int))==
	         sizeof(unsigned int))
	    {
	       nb1 += ntohl (nb0);
	       nb2 += nb1 / MODULO;
	       nb1 = nb1 % MODULO;
	       if (to_redo >0)
	       {
                  printf("Je relance un calcul...\n");
	          memcpy(calcul_en_cours[i],calcul_a_refaire[--to_redo],
		    3 * sizeof(unsigned int));
		  if(write(fd[i],calcul_en_cours[i],
		     3*sizeof(unsigned int)) != 3 * sizeof(unsigned int))
		  {
		     mort_client(i);
		  }
	       } else
		  grand_pipo (i);
            }
	    else
	    {
	       mort_client(i);
	    }
	 };
   };
}

int main ()
{
  int i;
  int un=1;

  signal(SIGPIPE,handle_pipe);

  memset (&saddr, 0, sizeof (struct sockaddr_in));
  memset (&myaddr, 0, sizeof (struct sockaddr_in));

  for(i=0; i<MACHINES; i++)
  {
     if ((hp = gethostbyname (machines[i])) == NULL) {
       saddr.sin_addr.s_addr = inet_addr (machines[i]);
       if (saddr.sin_addr.s_addr == -1) {
	 fprintf (stderr, "Unknown host: %s\n", machines[i]);
         continue;
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
         continue;
     };
     if (setsockopt (fd[i],SOL_SOCKET,SO_KEEPALIVE,&un,1) < 0) {
       perror ("setsockopt");
         continue;
     };
     if (connect (fd[i], (struct sockaddr *) &saddr, sizeof (saddr))) {
       fprintf(stderr," Connection a : %s\n",machines[i]);
       perror ("connection au serveur...");
         continue;
     };
     {
       unsigned short int sonrg;
       read (fd[i], &sonrg, sizeof(unsigned short int));
       if (ntohs(sonrg) != RG)
       {
         fprintf(stderr," Le client sur %s a un RG de %d et moi de %d\n",
	         machines[i], ntohs (sonrg), RG);
         continue;
       }
     };
     printf("%s est avec nous...\n",machines[i]);
     nombre_machines++;
     ok[i]=1;
  };

  itere_pipo ();
  printf ("%d reines -> 2 * (%d + (%d * %d )) sols\n",
          RG, nb1, nb2, MODULO);
  return 0;
}
