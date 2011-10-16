#ifndef PREEMPTIVE_SET_H
#define PREEMPTIVE_SET_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#define MAX_COLORS 64
#define FULL UINT64_MAX

typedef uint64_t pset_t;
/*
 * `char2pset` turns character into pset.  If it's not a possible
 * value it returns the empty set.  While `pset2str` does the oposite
 * and takes the string on which you want to write as a parameter.
 */
pset_t char2pset (char c);
void pset2str (char[] , pset_t);

/*
 * `pset_full` returns the pset where all colors are set. The color
 * range is given as a parameter.
 * `pset_empty` just returns the empty set.
 */
pset_t pset_full (size_t);
pset_t pset_empty (void);

/*
 * `pset_set` sets the given color, while `pset_discard` does the
 * oposite.
 */
pset_t pset_set (pset_t, char);
pset_t pset_discard (pset_t, char);

/*
 * The familiar boolean operators on sets
 */
pset_t pset_negate (pset_t);
pset_t pset_and (pset_t, pset_t);
pset_t pset_or (pset_t, pset_t);
pset_t pset_xor (pset_t, pset_t);

/*
 * `pset_is_included` checks for set inclusion
 */
bool pset_is_included (pset_t, pset_t);

/*
 * `pset_is_singleton` checks if the parameter has only one member.
 * While `pset_cardinality` returns the number of elements on the
 * parameter.
 */
bool pset_is_singleton (pset_t);
size_t pset_cardinality (pset_t);

#endif /* PREEMPTIVE_SET_H */
