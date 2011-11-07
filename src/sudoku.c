#include <getopt.h>
#include <libgen.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include <preemptive_set.h>

#include "sudoku.h"

#define MAX_GRID_SIZE 64

static bool verbose = false;
static bool random_choice = false;
static bool strict = false;
static char* exec_name;

static FILE* output_stream;

static size_t grid_size = 0;
static pset_t** grid; 

void usage (int);
void grid_print (const pset_t**);
int grid_heuristics (pset_t**);
pset_t** grid_copy (const pset_t**);
void grid_free (pset_t**);
void out_of_memory ();
void check_size_of_grid (int s, FILE *in);
pset_t** grid_alloc (void);


typedef struct choice {
  pset_t **grid; /* Original grid */
  size_t x;      /* x-coordinate of the changed cell */
  size_t y;      /* y-coordinate of the changed cell */
  pset_t choice; /* storage of the choice we made */

  struct choice *previous; /* Link to the previous choice
         		    * or NULL if it is the first choice */
} choice_t;

void
stack_print (const choice_t* stack)
{
  if (stack == NULL)
    return;
  
   char str1[MAX_COLORS + 1];
   char str2[MAX_COLORS + 1];
   pset2str (str1, stack->grid[stack->x][stack->y]);
   pset2str (str2, stack->choice);
   if (verbose)
     grid_print ((const pset_t**) stack->grid);

   fprintf (output_stream,
	    "Next choice at: grid[%d][%d] = '%s', and choice is = '%s'\n",
	   (int) stack->x, (int) stack->y, str1, str2);
}

size_t
stack_depth (const choice_t* stack)
{
  if (stack == NULL)
    return (0);
  return (1 + stack_depth (stack->previous));
}

void 
stack_free (choice_t* stack)
{
  if (stack == NULL)
    return;
  choice_t* prev = stack->previous;
  
  grid_free (stack->grid);
  free (stack);

  stack = NULL;
  
  return (stack_free (prev));
}

/*
 * stack_pop is used for backtracking, it brings the grid passed as an
 * argument to a state where the last choice was made and removes that
 * choice as a possibility
 */

choice_t*
stack_pop (choice_t* stack, pset_t** grid)
{

  if (stack == NULL)
    return (NULL);
  
  choice_t* prev = stack->previous; 

  stack->grid[stack->x][stack->y] = pset_and (stack->grid[stack->x][stack->y],
					      pset_negate (stack->choice));
  
  for (unsigned int i = 0; i < grid_size; i++)
    for (unsigned int j = 0; j < grid_size; j++)
      grid[i][j] = stack->grid[i][j];

  grid_free (stack->grid);
  free (stack);

  return (prev);
}

/*
 * stack_push chooses the first cell with the least choice if
 * random_choice is false otherwise it chooses one of the cells with
 * the least choice to be made randomly. Saves the choice in the stack
 * and returns the new stack.
 */

choice_t*
stack_push (const choice_t* stack, pset_t** grid)
{
  size_t min_cardinality = MAX_COLORS + 1;
  unsigned int* min_is;
  unsigned int  min_i = 0;
  unsigned int* min_js;
  unsigned int  min_j = 0;

  int num_mins = 0;
  
  min_is = malloc (grid_size * grid_size * sizeof (unsigned int));
  min_js = malloc (grid_size * grid_size * sizeof (unsigned int));

  if (min_js == NULL || min_js == NULL)
    out_of_memory ();

  for (unsigned int i = 0; i < grid_size; i++)
    for (unsigned int j = 0; j < grid_size; j++)
      {
	size_t cdn = pset_cardinality (grid[i][j]);
	if (!(cdn <= 1) && cdn <= min_cardinality)
	  {
	    min_cardinality = cdn;
	    min_is[num_mins] = i;
	    min_js[num_mins] = j;
	    num_mins++;
	  }
      }
  if (random_choice)
    {
      min_i = min_is[rand () % num_mins];
      min_j = min_js[rand () % num_mins];
    }
  else 
    {
      min_i = min_is[0];
      min_j = min_js[0];
    }

  free (min_js);
  free (min_is);
  
  if (min_cardinality == MAX_COLORS + 1)
    return ((choice_t*) stack);

  choice_t* our_choice = malloc (sizeof (choice_t));
  if (our_choice == NULL)
    out_of_memory ();

  our_choice->grid   = grid_copy ((const pset_t**) grid);
  our_choice->x      = min_i;
  our_choice->y      = min_j;
  our_choice->choice = pset_leftmost (grid[min_i][min_j]);
  if (verbose)
    stack_print (our_choice);
  our_choice->previous = (choice_t*) stack;

  grid[min_i][min_j] = pset_leftmost (grid[min_i][min_j]);

  return (our_choice);
}  

/*
 * Given a grid, number_of_solutions tries solving it and when it
 * arrives to a solution then it backtracks and tries to find other
 * solutions while counting the number of them, this number is
 * returned in the end
 */

int
number_of_solutions (pset_t** grid)
{
  choice_t* stack = NULL;
  int sols = 0;
  
  for (;;)
    {
      switch (grid_heuristics (grid))
	{
	case 0:
	  sols++;
	  if (stack == NULL)
	    {
	      stack_free (stack);
	      return (sols);
	    }
	  else
	    {
	      stack = stack_pop (stack, grid);
	      break;
	    }
	case 1:
	  stack = stack_push (stack, grid);
	  break;
	case 2:
	  if (stack == NULL)
	    {
	      return (sols);
	    }
	  stack = stack_pop (stack, grid);
	  break;
	}
    }
}

/*
 * Tries solving the grid with heuristics and when they don't work it
 * guesses a cell with stack_push
 */

bool 
grid_solver (pset_t** grid)
{
  choice_t* stack = NULL;
  
  for (;;)
    {
      switch (grid_heuristics (grid))
	{
	case 0:
	  if (!random_choice) 
	    {
	      fprintf (output_stream, "Grid has been solved\n");
	      grid_print ((const pset_t**) grid);
	    }
	  stack_free (stack);
	  return (true);
	case 1:
	  stack = stack_push (stack, grid);
	  break;
	case 2:
	  if (stack == NULL)
	    {
	      if (!random_choice)
		fprintf (output_stream, "Grid could not be solved\n");
	      return (false);
	    }
	  stack = stack_pop (stack, grid);
	  break;
	}
    }
}

void
get_block (const pset_t** grid, unsigned int k, pset_t* block[grid_size])
{
  size_t block_size = sqrt (grid_size);

  int block_index = 0;
  
  int init_i = (k / block_size) * block_size;
  int init_j = (k * block_size) % grid_size;
  
  for (unsigned int i = 0; i < block_size; i++)
    for (unsigned int j = 0; j < block_size; j++)
      {
	block[block_index] = (pset_t*) &grid[init_i + i][init_j + j];
	block_index++;
      }
}

bool
subgrid_map (pset_t** grid, bool (*func) (pset_t* subgrid[grid_size]))
{
  pset_t* line_subgrid[grid_size];
  pset_t* column_subgrid[grid_size];
  pset_t* block_subgrid[grid_size];

  bool acc = true;
  
  for (unsigned int i = 0; i < grid_size; i++)
    {
      for (unsigned int j = 0; j < grid_size; j++)
	{
	  column_subgrid[j] = &grid[j][i];
	  line_subgrid[j] = &grid[i][j];
	}

      acc = func (line_subgrid) && func (column_subgrid) && acc;
    }

  for (unsigned int i = 0; i < grid_size; i++)
    {
      get_block ((const pset_t**) grid, i, block_subgrid);
      acc = func (block_subgrid) && acc;
    }

  return (acc);
}

bool
all_different (pset_t* subgrid[grid_size])
{
  pset_t acc = 0;
  
  for (unsigned int i = 0; i < grid_size; i++)
    {
      acc = pset_xor (acc, *subgrid[i]);
      if (!pset_is_singleton (*subgrid[i]))
	return (false);
    }
  return (acc == pset_full (grid_size));
}

bool
subgrid_consistency (pset_t* subgrid[grid_size])
{
  pset_t acc = 0;
  
  for (unsigned int i = 0; i < grid_size; i++)
    acc = pset_or (acc, *subgrid[i]);
    
  for (unsigned int i = 0; i < grid_size; i++)
    for (unsigned int j = 0; j < grid_size; j++)
      {
	if ((pset_is_singleton (*subgrid[i]) 
	    && pset_is_singleton (*subgrid[j])
	    && (*subgrid[i] == *subgrid[j])
	     && i != j) || *subgrid[i] == pset_empty ())
	  return (false);
      }
  
  return (acc == pset_full (grid_size));
}

bool
grid_consistency (pset_t** grid)
{
  return (subgrid_map (grid, &subgrid_consistency));
}

bool 
grid_solved (pset_t** grid)
{
  return (subgrid_map (grid, &all_different));
}

/*
 * Crosses off the `colors` in `grid` either from row `row` or the
 * column `column` (-1 signifies that that it should not be crossed
 * off of the row/column, but not on the `k`th block.
 */

bool
cross_off_candidate (pset_t** grid, pset_t colors, 
		     int row, int col, int k)
{
  bool changed = false;
  size_t block_size = sqrt (grid_size);

  unsigned int init_i = (k / block_size) * block_size;
  unsigned int init_j = (k * block_size) % grid_size;

  if (row >= 0 && col < 0) 
    {
      for (unsigned int c = 0; c < grid_size; c++)
	if (c < init_j || c >= (init_j + block_size))
	  {
	    pset_t tmp = grid[init_i + row][c];
	    
	    grid[init_i + row][c] = pset_and (grid[init_i + row][c],
					      pset_negate (colors));
	    if (tmp != grid[init_i + row][c])
	      changed = true;
	  }
    }

  if (col >= 0 && row < 0)
    {
      for (unsigned int c = 0; c < grid_size; c++)
	if (c < init_i || c >= (init_i + block_size))
	  {
	    pset_t tmp = grid[c][init_j + col];

	    grid[c][init_j + col] = pset_and (grid[c][init_j + col],
					      pset_negate (colors));
	    if (tmp != grid[c][init_j + col])
	      changed = true;
	  }
    }
  
  return (changed);
}

/*
 * A heuristic that removes the candidates that are locked in a
 * column/row inside the kth block from the cells in that column/row
 */ 
bool
rm_locked_candidates (pset_t** grid, int k)
{
  bool changed = false;
  pset_t row_acc, col_acc;
  size_t block_size = sqrt (grid_size);
  int row = 0;
  int col = 0;
  
  pset_t* block[grid_size];
  pset_t* row_locked_candidates = malloc (sizeof (pset_t) * block_size);
  pset_t* col_locked_candidates = malloc (sizeof (pset_t) * block_size);
  if (row_locked_candidates == NULL || col_locked_candidates == NULL)
    {
      fprintf (stderr, "%s: out of memory\n", exec_name);
      usage (EXIT_FAILURE);
    }
  bzero (row_locked_candidates, sizeof (pset_t) * block_size);
  bzero (col_locked_candidates, sizeof (pset_t) * block_size);
  
  get_block ((const pset_t**) grid, k, block);
  
  for (unsigned int i = 0; i < grid_size; i += block_size)
    {
      for (unsigned int j = i; j < i+block_size; j++)
	if (!pset_is_singleton (*block[j]))
	  row_locked_candidates[row] = pset_or (row_locked_candidates[row],
					    *block[j]);
      row++;
    }

  for (unsigned int i = 0; i < block_size; i++)
    {
      for (unsigned int j = i; j < grid_size; j += block_size)
	if (!pset_is_singleton (*block[j]))
	  col_locked_candidates[col] = pset_or (col_locked_candidates[col],
						*block[j]);
      col++;
    }
  
  for (unsigned int i = 0; i < block_size; i++)
    {
      row_acc = row_locked_candidates[i];
      col_acc = col_locked_candidates[i];
      
      for (unsigned int j = 0; j < block_size; j++)
	if (j != i)
	  {
	    row_acc = pset_and (row_acc, pset_negate (row_locked_candidates[j]));
	    col_acc = pset_and (col_acc, pset_negate (col_locked_candidates[j]));
	  }
      if (row_acc != pset_empty ())
	{
	  bool tmp = cross_off_candidate (grid, row_acc, i, -1, k);
	  changed = changed || tmp;
	}
      if (col_acc != pset_empty ())
	{
	  bool tmp = cross_off_candidate (grid, col_acc, -1, i, k);
	  changed = changed || tmp;
	}
    }

  free (row_locked_candidates);
  free (col_locked_candidates);
  
  return (changed);
}

bool
subgrid_heuristics (pset_t** subgrid)
{
  bool changed = false;

  /*
   * The cross-hatching heuristic. Crosses off the already seen
   * singletons in the subgrid.
   */
  for (unsigned int i = 0; i < grid_size; i++)
    {
      if (pset_is_singleton(*subgrid[i]))
	for (unsigned j = 0; j < grid_size; j++)
	  if (i != j && pset_is_included (*subgrid[i], *subgrid[j]))
	    {
	      *subgrid[j] = pset_and (pset_negate (*subgrid[i]), 
				      *subgrid[j]);
	      changed = true;
	    }
    }

  /*
   * The lone number heuristic. Finds a color in a cell which is not
   * to be found anywhere else. And assigns that color to that
   * respective cell. 
   */
  pset_t acc;

  for (unsigned int i = 0; i < grid_size; i++)
    {
      acc = *subgrid[i];
      if (pset_is_singleton (acc))
	continue;
      
      for (unsigned int j = 0; j < grid_size; j++)
	{
	  if (i != j)
	    acc = pset_and (acc, pset_negate (*subgrid[j]));
	}
      if (pset_is_singleton (acc))
	{
	  *subgrid[i] = acc;
	  changed = true;
	}
    }

  return (!changed);
}

int
grid_heuristics (pset_t** grid)
{
  bool not_changed = false;

  while (!not_changed)
    {
      if (verbose)
	{
	  grid_print ((const pset_t**) grid);
	  fprintf (output_stream, "\n");
	}
      not_changed = subgrid_map (grid, &subgrid_heuristics);
      if (not_changed)
      	for (unsigned int k = 0; k < grid_size; k++)
      	  if (rm_locked_candidates (grid, k))
      	    {
      	      not_changed = false;
      	      break;
      	    }
      if (!grid_consistency (grid))
	return (2);
    }

  if (grid_solved (grid))
    return (0);

  if (grid_consistency (grid))
    return (1);
  else
    return (2);
}

/*
 * shuffles the elements on the array `arr` (of size `size`)
 * in a random permutation
 */
void
shuffle (int* const arr, int size)
{
  srand (time (NULL));

  for (int i = 0; i < size - 1; i++)
    {
      int r  = i + (rand () % (size - i));
      int t  = arr[i];
      arr[i] = arr[r];
      arr[r] = t;
    }
}

void
generate_grid (int size)
{
  check_size_of_grid (size, NULL);

  int num_elements = size * size;
  int empty_cells;

  grid_size = size;
  grid = grid_alloc ();
  
  for (unsigned int i = 0; i < grid_size; i++)
    for (unsigned int j = 0; j < grid_size; j++)
      grid[i][j] = pset_full (grid_size);
      
  random_choice = true; 
  srand (time (NULL));
  
  grid_solver (grid);
  
  int* arr = malloc (num_elements * (sizeof (int)));

  if (arr == NULL)
    out_of_memory ();

  for (int i = 0; i < num_elements; i++)
    arr[i] = i;
  shuffle (arr, num_elements);
  
  if (strict)
    {
      for (int i = 0; i < num_elements; i++)
	{
	  pset_t tmp = grid[arr[i] / grid_size][arr[i] % grid_size];
	  grid[arr[i] / grid_size][arr[i] % grid_size] = pset_full (grid_size);

	  pset_t** orig1 = grid_copy ((const pset_t**) grid);
	  pset_t** orig2 = grid_copy ((const pset_t**) grid);
	  
	  if (grid_heuristics (orig1) != 0 && number_of_solutions (orig2) != 1)
	    {
	      grid[arr[i] / grid_size][arr[i] % grid_size] = tmp;
	      break;
	    }
	  grid_free (orig1);
	  grid_free (orig2);
	}
    }
  else
    {
      empty_cells = (2 * num_elements) / 3;

      for (int i = 0; i < empty_cells; i++)
	{
	  grid[arr[i] / grid_size][arr[i] % grid_size] =
	    pset_full (grid_size);
	}
    }
  
  free (arr);
  grid_print ((const pset_t**) grid);
  grid_free (grid);
}

void
grid_free (pset_t** grid)
{
  if  (grid == NULL)
    return;

  for (unsigned int i = 0; i < grid_size; i++)
    free (grid[i]);

  free (grid);
  grid = NULL;
}

pset_t**
grid_alloc (void)
{
  pset_t** ret;

  ret = calloc (grid_size, sizeof (pset_t*));
  if (ret == NULL)
    out_of_memory ();
  for (unsigned int i = 0; i < grid_size; i++)
    {
      ret[i] = calloc (grid_size, sizeof (pset_t));
      if (ret[i] == NULL)
	out_of_memory ();
    }
  return (ret);
}

void
out_of_memory ()
{
  fprintf (stderr, "%s: error: out of memory!\n", exec_name);
  usage (EXIT_FAILURE);
}

pset_t**
grid_copy (const pset_t** grid)
{
  pset_t** new_grid = grid_alloc ();

  for (unsigned int i = 0; i < grid_size; i++)
    for (unsigned int j = 0; j < grid_size; j++)
      new_grid[i][j] = grid[i][j];
  
  return (new_grid);
}

void
grid_print (const pset_t** grid)
{
  char str[MAX_COLORS+1] = {0};
  size_t max_cardinality = 0;

  for (unsigned int i = 0; i < grid_size; i++)
    for (unsigned int j = 0; j < grid_size; j++)
      {
	if (pset_cardinality (grid[i][j]) > max_cardinality
	    && grid[i][j] != pset_full (grid_size))
	  max_cardinality = pset_cardinality(grid[i][j]);
      }
  
  for (unsigned int i = 0; i < grid_size; i++)
    {
      for (unsigned int j = 0; j < grid_size; j++)
	{
	  size_t spaces_length;
	  
	  if (grid[i][j] == pset_full (grid_size))
	    {
	      spaces_length = max_cardinality;
	      fprintf (output_stream, "_");
	    }
	  else 
	    {
	      spaces_length = max_cardinality - pset_cardinality(grid[i][j]);

	      pset2str (str, grid[i][j]);
	      fprintf (output_stream, "%s ", str);
	    }
	  for (;spaces_length > 0; spaces_length--)
	    fputc (' ', output_stream);
	}
      fprintf (output_stream, "\n");
    }
}

bool
check_input_char (char c)
{
  const char tbl[] = "123456789"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                     "abcdefghijklmnopqrstuvwxyz"
                     "@&*";
  if (c == '_')
    return (true);

  for (unsigned int i = 0; i < grid_size; i++)
    if (c == tbl[i])
      return (true);
  
  return (false);
}

void
bad_character (int line_number, char c, FILE* in)
{
  fclose (in);
  fprintf (stderr, 
	   "%s: error: wrong character \'%c\' at line %d\n", 
	   exec_name, c, line_number);
  usage (EXIT_FAILURE);
}

void
bad_number_of_lines (FILE* in)
{
  fclose (in);
  fprintf (stderr,
	   "%s: error: too many/few lines in the grid\n",
	   exec_name);
  usage (EXIT_FAILURE);
}

void
bad_line (int line_number, FILE* in)
{
  fclose (in);
  fprintf (stderr,
	   "%s: error: line %d is malformed (wrong number of cells)\n",
	   exec_name, line_number);
  usage (EXIT_FAILURE);
}

void
check_size_of_grid (int s, FILE *in)
{
  if (s != 1  && s != 4  && s != 9  && s != 16 &&
      s != 25 && s != 36 && s != 49 && s != 64)
    {
      if (in != NULL)
	fclose (in);
      fprintf (stderr, "Wrong grid size: %d\n", s);
      usage (EXIT_FAILURE);
    }
}

void
grid_parser (FILE *in)
{
  int current_char, c;
  unsigned int i = 0; /* current line */
  unsigned int j = 0; /* current column */

  char first_line[MAX_GRID_SIZE];
  
  while ((current_char = fgetc (in)) != EOF)
    {
      switch (current_char)
	{
	case ' ':
	case '\t':
	  break;

	case '#':
	  /*
	   * After reading the '#' character, read the rest of the line
	   */
	  for (c = fgetc (in); c != '\n' && c != EOF; c = fgetc (in))
	    ;
	  if (c == EOF)
	    break;
	  
	case '\n':
	  /*
	   * empty lines should be ignored
	   */
	  if (j == 0) 
	      break;

	  if (i == 0)
	    {
	      check_size_of_grid (j, in);
	      grid_size = j;
	      grid = grid_alloc ();
	      
	      for (unsigned int k = 0; k < grid_size; k++)
		{
		  if (!check_input_char (first_line[k]))
		    bad_character (i + 1, first_line[k], in);
		  if (first_line[k] == '_')
		    grid[0][k] = pset_full (grid_size);
		  else
		    grid[0][k] = char2pset (first_line[k]);
		}
	    }
	  if (j < grid_size)
	    bad_line (i + 1, in);

	  i++;
	  j = 0;	  
	  break;

	default:
	  if (grid_size != 0 && i >= grid_size)
	    bad_number_of_lines (in);

	  if (grid_size != 0 && j >= grid_size)
	    bad_line (i + 1, in);
	  
	  if (i == 0)
	    first_line[j] = current_char;
	  else
	    {
	      if (!check_input_char(current_char))
		bad_character(i + 1, current_char, in);
	      if (current_char == '_')
		grid[i][j] = pset_full (grid_size);
	      else
		grid[i][j] = char2pset (current_char);
	    }
	  j++;

	  if (j > MAX_GRID_SIZE)
	    {
	      fprintf (stderr, "Grid is too big\n");
	      usage (EXIT_FAILURE);
	    }
	}
    }
  /*
   * The case of the grid of size 1 without a newline at the end
   */
  if (grid_size == 0 && j == 1)
    {
      grid_size = 1;
      grid = grid_alloc ();

      if (first_line[0] == '_')
	grid[0][0] = pset_full (grid_size);
      else
	grid[0][0] = char2pset (first_line[0]);
    }
  /*
   * The right part of the disjunction handles the case of reading the
   * grid that doesn't have a newline at the end, which should be read
   * as a correct one.
   */
  if (i < grid_size - 1 || ((i == grid_size - 1) && (j != grid_size)))
      bad_number_of_lines (in);
}

void
usage (int status)
{
  if (status == EXIT_SUCCESS)
    {
      printf ("Usage: %s [OPTION] FILE...\n"
              "Solve Sudoku puzzles of variable sizes (1-4)\n"
	      "\n"
	      "  -o, --output=FILE   write result to FILE\n"
              "  -v, --verbose       verbose output\n"
	      "  -V, --version       display version and exit\n"
	      "  -h, --help          display this help\n", 
	      basename(exec_name));
    }
  else
    {
      fprintf (stderr, "Try `%s --help` for more information\n", 
	       basename(exec_name));
    }
  grid_free (grid);
  
  fclose (output_stream);  
  exit (status);
}

void
version (void)
{
  printf ("%s %d.%d.%d\n"
	  "This software is a sudoku solver\n", 
	  PROG_NAME, PROG_VERSION, PROG_SUBVERSION, PROG_REVISION);
  exit (EXIT_SUCCESS);
}

int 
main (int argc, char* argv[])
{
  bool generate = false;
  int optc;
  FILE* fp, *in; 
  struct option long_opts[] = 
    {
      {"output",   required_argument, 0, 'o'},
      {"generate", no_argument,       0, 'g'},
      {"strict",   no_argument,       0, 's'},
      {"verbose",  no_argument,       0, 'v'},
      {"version",  no_argument,       0, 'V'},
      {"help",     no_argument,       0, 'h'},
      {NULL, 0, NULL, 0}
    };

  exec_name = argv[0];
  output_stream = stdout;
  in = NULL;

  while ((optc = getopt_long (argc, argv, "o:vVshg", long_opts, NULL)) != -1)
    {
      switch (optc)
	{
	case 'o':
	  fp = fopen (optarg, "w");
	  if (fp == NULL)
	    {
	      fprintf (stderr, "Cannot open file: %s\n", optarg);
	      exit (EXIT_FAILURE);
	    }
	  else
	    output_stream = fp;
	  break;

	case 'g':
	  generate = true;
	  break;

	case 's':
	  strict = true;
	  break;
	  
	case 'v':
	  verbose = true;
	  break; 

	case 'V':
	  version ();
	  break;

	case 'h':
	  usage (EXIT_SUCCESS);
	  break;
	  
	default: 
	  usage (EXIT_FAILURE);
	}
    }
  if (generate)
    {
      if (optind == argc - 1)
	generate_grid (atoi (argv[optind]));
      if (optind == argc)
	generate_grid (9);
      if (optind != argc - 1 && optind != argc)
	usage (EXIT_FAILURE);

      goto free_output;
    }
  
  if (optind != argc -1)
    usage (EXIT_FAILURE);
  else
    {
      in = fopen (argv[optind], "r");
      if(in == NULL)
	{
	  fprintf (stderr, "Cannot open file: %s\n", argv[optind]);
	  usage (EXIT_FAILURE);
	}
      grid_parser (in);
      grid_solver (grid);
      grid_free (grid);
    }
 free_output:
  if ((output_stream != stdout && fclose (output_stream) != 0) ||
      (in != NULL && fclose (in) != 0))
    {
      fprintf (stderr, "File cannot be closed\n");
      exit (EXIT_FAILURE);
    }
  exit (EXIT_SUCCESS);
}
