#include <stdio.h>
#include <stdlib.h>

#include "algo.h"

#define MODULO 16777216

static unsigned int nb1 = 0;
static unsigned int nb2 = 0;

int main ()
{
  unsigned int i = 1;
  for (; i < (1<<(RG/2)); i <<= 1) {
#ifdef DEBUG
    printf("1ere col: %d\n", i);
#endif
    pipo_str (i, i<<1, i>>1);
    nb2 += nb % MODULO;
    nb1 += (nb / MODULO) + (nb2 / MODULO);
    nb2 = nb2 % MODULO;
    nb = 0;
  };
  if ((RG&1) == 1) {
    unsigned int j = 1;
#ifdef DEBUG
    printf("Milieu: %d\n", i);
#endif
    for (; j < (i>>1); j <<= 1) {
#ifdef DEBUG
      printf("2eme col: %d\n", j);
#endif
      pipo_str (i|j, ((i<<1)|j)<<1, ((i>>1)|j)>>1);
      nb2 += nb % MODULO;
      nb1 += (nb / MODULO) + (nb2 / MODULO);
      nb2 = nb2 % MODULO;
      nb = 0;
    };
  };
  printf("%d reines => 2*(%d + %d * %d) solutions\n", RG, nb2, nb1, MODULO);
  return 0;
}
