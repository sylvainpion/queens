#include "algo.h"

unsigned int nb = 0;
    
/* Attention, on ne peut pas appeler pipo_str (0,0,0),
 * ca ne marche plus mnt, il faut donc commencer a la 2eme colonne. 
 * Il faut aussi que "m" ne deborde pas, c'est vital.
 */

void pipo_str (unsigned int h, unsigned int m, unsigned int d)
{
  unsigned int tot = (h|m|d) ^ RG_FULL;
                     /* les bits a 1 sont les positions a tester */

  if (tot == 0) {return;};

  { 
    unsigned int i = tot | (tot<<1);
    i |= i<<2;   i |= i<<4;    /* J'isole le bit de poids faible. */
    i |= i<<8;   i |= i<<16;
    i ^= i<<1;
  
    if ((i|h) == RG_FULL) { ++nb; return; };
 
    do {
      if ( (tot&i) != 0 )
        pipo_str ((h|i), ((m|i) << 1) & RG_FULL, (d|i) >> 1);
      i <<= 1;
    } while (i <= tot);
  };
}

