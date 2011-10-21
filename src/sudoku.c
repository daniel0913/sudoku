#include <getopt.h>
#include <libgen.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <preemptive_set.h>

#include "sudoku.h"

#define MAX_GRID_SIZE 64

static bool verbose = false;
static char* exec_name;

static FILE* output_stream;

static size_t grid_size = 0;
static pset_t** grid; 

static void usage (int);
void grid_print (pset_t**);
int grid_heuristics (pset_t**);
pset_t** grid_copy (pset_t**);
void grid_free (pset_t**);

typedef struct choice {
  pset_t **grid; /* Original grid */
  size_t x;      /* x-coordinate of the changed cell */
  size_t y;      /* y-coordinate of the changed cell */
  pset_t choice; /* storage of the choice we made */

  struct choice *previous; /* Link to the previous choice
         		    * or NULL if it is the first choice */
} choice_t;

choice_t* stack_pop (choice_t* stack, pset_t** grid)
{
  
}

void
stack_print (choice_t* stack)
{
  if (stack == NULL)
    return;
  
   char str1[MAX_COLORS + 1];
   char str2[MAX_COLORS + 1];
   pset2str (str1, stack->grid[stack->x][stack->y]);
   pset2str (str2, stack->choice);
   if (verbose)
     grid_print (stack->grid);

   printf ("Next choice at: grid[%d][%d] = '%s', and choice is = '%s'\n", 
	   (int) stack->x, (int) stack->y, str1, str2);
}

size_t
stack_depth (choice_t* stack)
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
  return (stack_free (prev));
}

choice_t*
stack_push (choice_t* stack, pset_t **grid)
{
  size_t min_cardinality = MAX_COLORS + 1;
  unsigned int min_i = 0;
  unsigned int min_j = 0;

  for (unsigned int i = 0; i < grid_size; i++)
    for (unsigned int j = 0; j < grid_size; j++)
      {
	size_t cdn = pset_cardinality (grid[i][j]);
	if (cdn != 1 && cdn < min_cardinality)
	  {
	    min_cardinality = cdn;
	    min_i = i;
	    min_j = j;
	  }
      }

  if (min_cardinality == MAX_COLORS + 1)
    return (stack);

  choice_t* our_choice = malloc (sizeof (choice_t));
  our_choice->grid   = grid_copy (grid);
  our_choice->x      = min_i;
  our_choice->y      = min_j;
  our_choice->choice = pset_leftmost (grid[min_i][min_j]);
  our_choice->previous = stack;

  grid[min_i][min_j] = pset_leftmost (grid[min_i][min_j]);

  return (our_choice);
}  

void
grid_choice (pset_t** grid)
{
  size_t min_cardinality = MAX_COLORS + 1;
  unsigned int min_i = 0;
  unsigned int min_j = 0;

  for (unsigned int i = 0; i < grid_size; i++)
    for (unsigned int j = 0; j < grid_size; j++)
      {
	size_t cdn = pset_cardinality (grid[i][j]);
	if (cdn != 1 && cdn < min_cardinality)
	  {
	    min_cardinality = cdn;
	    min_i = i;
	    min_j = j;
	  }
      }

  if (min_cardinality == MAX_COLORS + 1)
    return;

  char str1[MAX_COLORS + 1];
  char str2[MAX_COLORS + 1];
  pset2str (str1, grid[min_i][min_j]);
  pset2str (str2, pset_leftmost (grid[min_i][min_j]));
  printf ("Next choice at: grid[%d][%d] = '%s', and choice is = '%s'\n", 
	  min_i, min_j, str1, str2);
  
  grid[min_i][min_j] = pset_leftmost (grid[min_i][min_j]);
}

bool 
grid_solver (pset_t** grid)
{
  for (;;)
    {
      switch (grid_heuristics (grid))
	{
	case 0:
	  fprintf (output_stream, "Grid has been solved\n");
	  grid_print (grid);
	  return (true);
	case 1:
	  grid_choice (grid);
	  break;
	case 2:
	  fprintf (output_stream, "Grid is not consistent\n");
	  return (false);
	}
    }
}

static void
get_block (pset_t** grid, unsigned int k, pset_t* block[grid_size])
{
  size_t block_size = sqrt (grid_size);

  int block_index = 0;
  
  int init_i = (k / block_size) * block_size;
  int init_j = (k * block_size) % grid_size;
  
  for (unsigned int i = 0; i < block_size; i++)
    for (unsigned int j = 0; j < block_size; j++)
      {
	block[block_index] = &grid[init_i + i][init_j + j];
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
      get_block (grid, i, block_subgrid);
      acc = func (block_subgrid) && acc;
    }

  return (acc);
}

static bool
all_different (pset_t* subgrid[grid_size])
{
  pset_t acc = 0;
  
  for (unsigned int i = 0; i < grid_size; i++)
    acc = pset_xor (acc, *subgrid[i]);

  return (acc == pset_full (grid_size));
}

static bool
subgrid_consistency (pset_t* subgrid[grid_size])
{
  pset_t acc = 0;
  
  for (unsigned int i = 0; i < grid_size; i++)
    acc = pset_or (acc, *subgrid[i]);
    
  for (unsigned int i = 0; i < grid_size; i++)
    for (unsigned int j = 0; j < grid_size; j++)
      {
	if (pset_is_singleton (*subgrid[i]) 
	    && pset_is_singleton (*subgrid[j])
	    && (*subgrid[i] == *subgrid[j])
	    && i != j)
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

static bool
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
	  grid_print (grid);
	  fprintf (output_stream, "\n");
	}
      not_changed = subgrid_map (grid, &subgrid_heuristics);
    }

  if (grid_solved (grid))
    return (0);

  if (grid_consistency (grid))
    return (1);
  else
    return (2);
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

static pset_t**
grid_alloc (void)
{
  pset_t** ret;

  ret = calloc (grid_size, sizeof (pset_t*));
  if (ret == NULL)
    goto out_of_memory;
  for (unsigned int i = 0; i < grid_size; i++)
    {
      ret[i] = calloc (grid_size, sizeof (pset_t));
      if (ret[i] == NULL)
	goto out_of_memory;
    }
  return (ret);

 out_of_memory:
  fprintf (stderr, "%s: error: out of memory!\n", exec_name);
  usage (EXIT_FAILURE);
  return (NULL);
}

pset_t**
grid_copy (pset_t** grid)
{
  pset_t** new_grid = grid_alloc ();

  for (unsigned int i = 0; i < grid_size; i++)
    for (unsigned int j = 0; j < grid_size; j++)
      new_grid[i][j] = grid[i][j];
  
  return (new_grid);
}

void
grid_print (pset_t** grid)
{
  char str[MAX_COLORS+1] = {0};
  size_t max_cardinality = 0;

  for (unsigned int i = 0; i < grid_size; i++)
    for (unsigned int j = 0; j < grid_size; j++)
      {
	if (pset_cardinality(grid[i][j]) > max_cardinality)
	  max_cardinality = pset_cardinality(grid[i][j]);
      }
  
  for (unsigned int i = 0; i < grid_size; i++)
    {
      for (unsigned int j = 0; j < grid_size; j++)
	{
	  size_t max_length = max_cardinality - pset_cardinality(grid[i][j]);
	  
	  pset2str (str, grid[i][j]);
	  fprintf (output_stream, "%s ", str);
	  for (;max_length > 0; max_length--)
	    fputc (' ', output_stream);
	}
      fprintf (output_stream, "\n");
    }
}

static bool
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

static void
bad_character (int line_number, char c, FILE* in)
{
  fclose (in);
  fprintf (stderr, 
	   "%s: error: wrong character \'%c\' at line %d\n", 
	   exec_name, c, line_number);
  usage (EXIT_FAILURE);
}

static void
bad_number_of_lines (FILE* in)
{
  fclose (in);
  fprintf (stderr,
	   "%s: error: too many/few lines in the grid\n",
	   exec_name);
  usage (EXIT_FAILURE);
}

static void
bad_line (int line_number, FILE* in)
{
  fclose (in);
  fprintf (stderr,
	   "%s: error: line %d is malformed (wrong number of cells)\n",
	   exec_name, line_number);
  usage (EXIT_FAILURE);
}

static void
check_size_of_grid (int s, FILE *in)
{
  if (s != 1  && s != 4  && s != 9  && s != 16 &&
      s != 25 && s != 36 && s != 49 && s != 64)
    {
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

static void
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

static void
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
  int optc;
  FILE* fp, *in; 
  struct option long_opts[] = 
    {
      {"output",  required_argument, 0, 'o'}, 
      {"verbose", no_argument,       0, 'v'},
      {"version", no_argument,       0, 'V'},
      {"help",    no_argument,       0, 'h'},
      {NULL, 0, NULL, 0}
    };

  exec_name = argv[0];
  output_stream = stdout;
  in = NULL;

  while ((optc = getopt_long (argc, argv, "o:vVh", long_opts, NULL)) != -1)
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

  if ((output_stream != stdout && fclose (output_stream) != 0) ||
      (in != NULL && fclose (in) != 0))
    {
      fprintf (stderr, "File cannot be closed\n");
      exit (EXIT_FAILURE);
    }
  exit (EXIT_SUCCESS);
}
