#include <stdio.h>
#include <stdlib.h>

#include "algo.h"

int main ()
{
  unsigned int i = 1;
  for (; i < (1<<(RG/2)); i <<= 1) {
#ifdef DEBUG
    printf("1ere col: %d\n", i);
#endif
    pipo_str (i, i<<1, i>>1);
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
    };
  };
  printf("%d reines => %d solutions\n", RG, 2*nb);
  return 0;
}
