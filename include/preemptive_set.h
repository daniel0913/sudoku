#ifndef PREEMPTIVE_SET_H
#define PREEMPTIVE_SET_H

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_COLORS 64
#define FULL UINT64_MAX

typedef uint64_t pset_t;

pset_t char2pset (char c);
void pset2str (char[] , pset_t);
pset_t pset_full (size_t);
pset_t pset_empty ();
pset_t pset_set (pset_t, char);
pset_t pset_discard (pset_t, char);
pset_t pset_negate (pset_t);
pset_t pset_and (pset_t, pset_t);
pset_t pset_or (pset_t, pset_t);
pset_t pset_xor (pset_t, pset_t);
bool pset_is_included (pset_t, pset_t);
bool pset_is_singleton (pset_t);
size_t pset_cardinality (pset_t);

#endif /* PREEMPTIVE_SET_H */
