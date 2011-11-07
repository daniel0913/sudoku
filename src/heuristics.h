#ifndef HEURISTICS_H
#define HEURISTICS_H

/*
 * Tries solving the grid using three implemented heuristics which
 * are: 1. Locked candidates removal 2. Cross-hatching 3. Lone number
 *
 * Returns 0 if it succeeds solving, 1 if it can't solve it, at this
 * point we have to make a guess or 2 if the grid we're trying to
 * solve is inconsistent and impossible to solve. 
 */
int grid_heuristics (pset_t** grid);

#endif /* HEURISTICS_H */
