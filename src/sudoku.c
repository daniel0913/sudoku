#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <preemptive_set.h>

#include "sudoku.h"
#include "heuristics.h"
#include "parser.h"
#include "main.h"

static bool random_choice = false;

bool strict = false;
bool verbose = false;
char* exec_name;

FILE* output_stream;

size_t grid_size = 0;
pset_t** grid;

typedef struct choice {
  pset_t **grid; /* Original grid */
  size_t x;      /* x-coordinate of the changed cell */
  size_t y;      /* y-coordinate of the changed cell */
  pset_t choice; /* storage of the choice we made */

  struct choice *previous; /* Link to the previous choice
         		    * or NULL if it is the first choice */
} choice_t;

static void
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

static size_t
stack_depth (const choice_t* stack)
{
  if (stack == NULL)
    return (0);
  return (1 + stack_depth (stack->previous));
}

static void 
stack_free (choice_t* stack)
{
  if (stack == NULL)
    return;
  choice_t* prev = stack->previous;
  
  grid_free (stack->grid);
  free (stack);

  return (stack_free (prev));
}

static void
out_of_memory ()
{
  fprintf (stderr, "%s: error: out of memory!\n", exec_name);
  usage (EXIT_FAILURE);
}

static pset_t**
grid_copy (const pset_t** grid)
{
  pset_t** new_grid = grid_alloc ();

  for (unsigned int i = 0; i < grid_size; i++)
    for (unsigned int j = 0; j < grid_size; j++)
      new_grid[i][j] = grid[i][j];
  
  return (new_grid);
}

/*
 * stack_pop is used for backtracking, it brings the grid passed as an
 * argument to a state where the last choice was made and removes that
 * choice as a possibility
 */

static choice_t*
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

static choice_t*
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

static int
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

/*
 * shuffles the elements on the array `arr` (of size `size`)
 * in a random permutation
 */
static void
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
grid_print (const pset_t** grid)
{
  char str[MAX_COLORS+1] = {0};
  size_t max_cardinality = 0;

  if (grid_size == 1 && !random_choice)
    {
      pset2str (str,grid[0][0]);
      fprintf (output_stream, "%s\n", str);
      return;
    }
  
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

