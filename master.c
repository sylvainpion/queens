		/*********************************************
		 * Maitre du calcul du probleme des N reines *
		 *********************************************/

/* $Id: master.c,v 1.13 1996/06/25 16:56:00 jo Exp syl $ */

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

#ifndef DBG_LVL
#define DBG_LVL 3    /* 0 -> rien
                        1 -> arrivees/depart de machines
                        2 -> fin/reveils
                        3 -> avancement du calcul */
#endif

#ifndef RG_MASTER
  #define RG_MASTER (RG/5)
#endif

#ifndef MACHINES        /* nb max de machines */
  #define MACHINES 200
#endif

#define MODULO 16777216

/* Pour les decomptes */
static unsigned int nb0 = 0;
static unsigned int nb1 = 0;
static unsigned int nb2 = 0;

/* gestion des machines et des calculs a faire... */
static int niveau = 1;
static int etat[50][4];
static int nombre_machines = 0;
static unsigned int calcul_en_cours[MACHINES][3];
static unsigned int calcul_a_refaire[MACHINES][3];
static int calcul_bidon[3];
static int to_redo = 0;

static int fini = 0;
static int fini_count = 0;

/* les infos sur les machines */
int fd[MACHINES];
int soc_princ;
int ok[MACHINES];
int fini_machine[MACHINES];
char machines[MACHINES][100];

/* tentative de gestion des pb des clients... */
static void handle_pipe(int nosig)
{
#if DBG_LVL > 0
  fprintf(stdout,"LVL1: Un pb de socket, j'ignore le signal... \n");
#endif
  signal(SIGPIPE,handle_pipe);
}

void accept_connection();

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

/* appelee quand une connection bugue */
void mort_client(int ma)
{
     int i;
#if DBG_LVL > 0
     perror("LVL1: Ecriture :");
#endif
     ok[ma]=0;
     nombre_machines --;
     close(fd[ma]);
     /* pour faire recommencer le meme... */
#if DBG_LVL >0
     printf ("LVL1: %s nous a quittee..., il reste %d machines.\n",
             machines[ma], nombre_machines);
#endif

    if (fini_machine[ma])
    {
#if DBG_LVL >1
       printf ("LVL2: pas de relance du calcul, il etait fini.\n");
#endif
       fini_machine[ma]=0; fini_count --;
       return;
    }

     /* on stock le message a renvoyer a une machine saine... */
     memcpy(calcul_a_refaire[to_redo],calcul_en_cours[ma],3*sizeof(int));
     to_redo++;
     for (i=0;i<MACHINES;i++)
     {
        if (fini_machine[i]&& ok[i])
        {
            /* on veut reveiller des machines pour finir le calcul... */
            fini_count--;
            fini_machine[i]=0;
#if DBG_LVL > 0
            fprintf(stdout,"LVL1: Je reveille %s...\n",machines[i]);
#endif
#if DBG_LVL > 1
            printf("LVL2: Je reveille %s...\n",machines[i]);
#endif
            memcpy(calcul_en_cours[i],calcul_bidon,3*sizeof(int));
            if(write(fd[i],calcul_en_cours[i],
		     3*sizeof(int)) !=3*sizeof(int))
            {
#if DBG_LVL > 0
               fprintf(stdout,"LVL1: Ca va mal\n");
#endif
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

  if (fini)
  {
    fini_count++;
    fini_machine[ma]=1;
#if DBG_LVL > 1
    printf ("LVL2: %s a termine.\n", machines[ma]);
#endif
    return;
  };

  /* attend de trouver qqchose d'interessant
   * ie la fin des calculs, ou un calcul a faire */

  while ((res = pipo_next ())==0);
  if (res == -1)
  {fini_machine[ma]=1;fini = 1;fini_count=1;
#if DBG_LVL > 1
    printf ("LVL2: %s a termine.\n", machines[ma]);
#endif
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
#if DBG_LVL > 2
  printf ("LVL3: (%2d) %d %d %d %d %d %d %d\n", ma, etat[1][3], etat[2][3], 
          etat[3][3], etat[4][3], etat[5][3], etat[6][3], etat[7][3]);
#endif

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

   while ((fini_count < nombre_machines)||(fini==0))
   {
      FD_ZERO (&rd);
      for(i=0; i < MACHINES; i++)
        if(ok[i])
	  FD_SET (fd[i], &rd);
      FD_SET (soc_princ, &rd);

      /* on attend qu'un des esclaves dise qqchose... */

      select (256, &rd, 0, 0, 0);
      if(FD_ISSET(soc_princ,&rd))
         accept_connection();
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
#if DBG_LVL > 2
                  printf("LVL3: Je relance un calcul... \n");
#endif
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

void init()
{
  signal(SIGPIPE,handle_pipe);
  calcul_bidon[0]=calcul_bidon[1]=calcul_bidon[2]=htonl(RG_FULL);
}

void accept_connection()
{
  int i;
  int un=1;
  int numero=0;
  static struct sockaddr_in lui;
  static struct hostent *luient;
  static int l=sizeof (lui);

  for(i=0;i<MACHINES;i++)
  {
    if (!ok[i]) {numero=i; break; }
  }
  if (ok[numero])
  {
    fprintf(stdout,"LVL0: J\'ai atteint le nombre maximum de clients... (%d)\n",
	    MACHINES);
    close(accept(soc_princ, (struct sockaddr *) &lui, &l));
    return;
  }
  fd[numero]= accept(soc_princ, (struct sockaddr *) &lui, &l);
  if (setsockopt (fd[numero],SOL_SOCKET,SO_KEEPALIVE,&un,sizeof(int)) < 0) {
    perror ("setsockopt");
    return;
  };
  luient = gethostbyaddr ((char *) & (lui.sin_addr.s_addr),
			  sizeof(lui.sin_addr.s_addr),AF_INET);
	     
  if(!luient)
  {
#if DBG_LVL>0
     fprintf(stdout,"LVL1: Je ne sais pas qui se connecte...\n");
#endif
     sprintf(machines[numero],"%u.%u.%u.%u",
	     (unsigned char) lui.sin_addr.s_addr,
	     *( ((unsigned char *) & lui.sin_addr.s_addr) + 1),
	     *( ((unsigned char *) & lui.sin_addr.s_addr) + 2),
	     *( ((unsigned char *) & lui.sin_addr.s_addr) + 3));
  } else
  {
     strncpy(machines[numero],luient->h_name,sizeof(machines[numero]));
  }
#if DBG_LVL > 0
  printf("LVL1: Connexion de %s\n",machines[numero]);
#endif
  {
    unsigned short int sonrg;
    read (fd[numero], &sonrg, sizeof(unsigned short int));
    if (ntohs(sonrg) != RG)
      {
	fprintf(stdout,"LVL0: Le client sur %s a un RG de %d et moi de %d\n",
		machines[numero], ntohs (sonrg), RG);
#if DBG_LVL > 0
	printf("LVL1: Refusee... Mauvais RG\n");
#endif
	close(fd[numero]);
	return;
      }
  };

  ok[numero]=1;
  fini_machine[numero]=0;
  nombre_machines++;
  /* on la fait demarrer... */
  write(fd[numero],calcul_bidon,3*sizeof(int));
}

void init_rezo()
{
  int un=1;
  struct sockaddr_in myaddr;

  memset (&myaddr, 0, sizeof (struct sockaddr_in));
  
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(PORT);
  myaddr.sin_family = AF_INET;

  if ((soc_princ = socket (PF_INET, SOCK_STREAM, 0)) < 0) {
    perror ("socket");
    exit(1);
  };
  

  if (setsockopt (soc_princ,SOL_SOCKET,SO_REUSEADDR,&un,sizeof(int))< 0) {
    perror ("setsockopt");
    exit(1);
  };

  if (bind (soc_princ, (struct sockaddr *) &myaddr, sizeof (myaddr)) < 0) {
    perror ("bind");
    exit (1);
  };

  if (listen (soc_princ, 5) < 0) {
    perror ("listen");
    exit (1);
  };
  printf("Attente de connections port :%d\n",PORT);
}

int main ()
{
  init();
  init_rezo();
  itere_pipo ();
  printf ("%d reines -> 2 * (%d + (%d * %d )) sols\n",
          RG, nb1, nb2, MODULO);
  return 0;
}
