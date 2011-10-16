#include <preemptive_set.h>

const char color_table[] = "123456789"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz"
                           "@&*";
pset_t 
char2pset (char c)
{
  for (int i = 0; i < MAX_COLORS; i++)
    {
      if (color_table[i] == c)
	return (((pset_t) 1) << i);
    }
  return (0);
}

void 
pset2str (char string[MAX_COLORS + 1], pset_t pset)
{
  int j = 0;
  for (int i = 0; i < MAX_COLORS; i++)
    {
      if (pset & 1)
	{
	  string[j] = color_table[i];
	  j++;
	}
      pset >>= 1;
    }
  string[j] = '\0';
}

pset_t 
pset_full (size_t color_range)
{
  return (color_range > MAX_COLORS ? FULL 
	  : FULL >> (MAX_COLORS - color_range));
}

pset_t 
pset_empty (void)
{
  return (0);
}

pset_t 
pset_set (pset_t pset, char c)
{
  return (pset | char2pset (c));
}

pset_t 
pset_discard (pset_t pset, char c)
{
  return (pset & (~ char2pset (c)));
}

pset_t 
pset_negate (pset_t pset)
{
  return (~pset);
}

pset_t 
pset_and (pset_t pset1, pset_t pset2)
{
  return (pset1 & pset2);
}

pset_t 
pset_or (pset_t pset1, pset_t pset2)
{
  return (pset1 | pset2);
}

pset_t 
pset_xor (pset_t pset1, pset_t pset2)
{
  return (pset1 ^ pset2);
}

bool 
pset_is_included (pset_t pset1, pset_t pset2)
{
  return ((pset1 | pset2) == pset2);
}

bool 
pset_is_singleton (pset_t pset)
{
  return (pset == 0 ? false : (pset & (- pset)) == pset);
}

size_t 
pset_cardinality (pset_t pset)
{
  static const uint64_t m1  = 0x5555555555555555;
  static const uint64_t m2  = 0x3333333333333333;
  static const uint64_t m4  = 0x0f0f0f0f0f0f0f0f;
  static const uint64_t h01 = 0x0101010101010101; 
  
  pset -= (pset >> 1) & m1;
  pset = (pset & m2) + ((pset >> 2) & m2);
  pset = (pset + (pset >> 4)) & m4;
  return (pset * h01)>>56;
}
