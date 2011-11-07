#ifndef SUDOKU_H
#define SUDOKU_H

#define PROG_NAME "sudoku"

#define PROG_VERSION    1
#define PROG_SUBVERSION 0
#define PROG_REVISION   0

#define MAX_GRID_SIZE 64

/*
 * Prints the grid given as argument, while also handling the
 * alignment of columns. Prints '_' in case of a full pset.
 */
 
void grid_print (const pset_t** grid);

/*
 * Checks the size of the grid if it's a square of a integer and less
 * then the specified maximum grid. You pass the as a second parameter
 * the input file you want to close in case of a wrong grid size, or
 * NULL otherwise
 */
void check_size_of_grid (int s, FILE *in);

/* All non-error messages are written to this stream */
extern FILE* output_stream;
/* The name of the exectuable taken from argv[0] */
extern char* exec_name;
/* Verbosity and strictness flag, are true in case of such options given */
extern bool verbose;
extern bool strict;

/* 
 * These values get assigned after the grid is parsed after which time
 * they won't be assigned to
 */ 
extern size_t grid_size;
extern pset_t** grid; 

/*
 * Tries allocating memory for a grid of `grid_size` size exits the
 * program if it can't
 */
pset_t** grid_alloc (void);

/*
 * Frees the grid, in case of a NULL argument it does nothing 
 */
void grid_free (pset_t** grid);

/*
 * Tries solving the grid and returns true if it succeeds and false
 * otherwise (which would mean that the grid is inconsistent and
 * unsolvable.
 * While generate_grid generates a grid of size `size`
 * makes it a grid with only one possible solution if the strict flag
 * is true
 */

bool grid_solver (pset_t** grid);
void generate_grid (int size);

#endif /* SUDOKU_H */
