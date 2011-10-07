#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <libgen.h>
#include <string.h>

#include <preemptive_set.h>

#include "sudoku.h"

bool verbose = false;
char* exec_name;

FILE* output_stream;

size_t grid_size = 0;
pset_t** grid; 

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
  exit (status);
}

void
grid_free (pset_t** grid)
{
  for (unsigned int i = 0; i < grid_size; i++)
    free (grid[i]);
  free (grid);
}

static void
out_of_memory ()
{
  fprintf (stderr, "%s: error: out of memory!\n", exec_name);
  usage (EXIT_FAILURE);
}

static pset_t**
grid_alloc (void)
{
  pset_t** ret;

  ret = (pset_t**) calloc (grid_size, sizeof (pset_t*));
  if (ret == NULL)
    out_of_memory ();
  for (unsigned int i = 0; i < grid_size; i++)
    {
      ret[i] = (pset_t*) calloc (grid_size, sizeof (pset_t));
      if (ret[i] == NULL)
	out_of_memory ();
    }
  return (ret);
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
	  size_t m = max_cardinality - pset_cardinality(grid[i][j]);
	  
	  pset2str (str, grid[i][j]);
	  fprintf (output_stream, "%s ", str);
	  for (;m > 0; m--)
	    fputc (' ', output_stream);
	}
      fprintf (output_stream, "\n");
    }
}

static bool
check_input_char (char c)
{
  if (c == '_')
    return (true);

  bool digit = (c >= '1') && (c <= '9');

  switch (grid_size)
    {
    case 1:
      return (c == '1');
    case 4:
      return ((c >= '1') && (c <= '4'));
    case 9:
      return (digit);
    case 16:
      return (((c >= 'A') && (c <= 'G')) || digit);
    case 25: 
      return (((c >= 'A') && (c <= 'P')) || digit);
    case 36:
      return (((c >= 'A') && (c <= 'Z')) || c == 'a' || digit);
    case 49:
      return (((c >= 'A') && (c <= 'Z')) || 
	      ((c >= 'a') && (c <= 'n')) || digit);
    case 64:
      return (((c >= 'A') && (c <= 'Z')) || 
	      ((c >= 'a') && (c <= 'z')) || 
	      c == '@' || c == '*' || c == '&' || digit);
    default:
      return (false);
    }
}

static void
bad_character (int line_number, char c)
{
  fprintf (stderr, 
	   "%s: error: wrong character \'%c\' at line %d\n", 
	   exec_name, c, line_number);
  usage (EXIT_FAILURE);
}

static void
bad_number_of_lines (void)
{
  fprintf (stderr,
	   "%s: error: too many/few lines in the grid\n",
	   exec_name);
  usage (EXIT_FAILURE);
}

static void
bad_line (int line_number)
{
  fprintf (stderr,
	   "%s: error: line %d is malformed (wrong number of cells)\n",
	   exec_name, line_number);
  usage (EXIT_FAILURE);
}

static void
check_size_of_grid (int s)
{
  if (s != 1  && s != 4  && s != 9  && s != 16 &&
      s != 25 && s != 36 && s != 49 && s != 64)
    {
      fprintf (stderr, "Wrong grid size: %d\n", s);
      usage (EXIT_FAILURE);
    }
}

void
grid_parser (FILE *in)
{
  char current_char;
  unsigned int i = 0; /* current line */
  unsigned int j = 0; /* current column */

  char first_line[64];
  
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
	  for (char c = fgetc (in); c != '\n' && c != EOF; c = fgetc (in))
	    ;
	  break;
	  
	case '\n':
	  /*
	   * empty lines should be ignored
	   */
	  if (j == 0) 
	      break;

	  if (i == 0)
	    {
	      check_size_of_grid (j);
	      grid_size = j;
	      grid = grid_alloc ();
	      for (unsigned int k = 0; k < grid_size; k++)
		{
		  if (!check_input_char (first_line[k]))
		    bad_character (i + 1, first_line[k]);
		  if (first_line[k] == '_')
		    grid[0][k] = pset_full (grid_size);
		  else
		    grid[0][k] = char2pset (first_line[k]);
		}
	    }
	  if (j < grid_size)
	    bad_line (i + 1);

	  i++;
	  j = 0;	  
	  break;

	default:
	  if (grid_size != 0 && i >= grid_size)
	    bad_number_of_lines ();
	  
	  if (i == 0)
	    first_line[j] = current_char;
	  else
	    {
	      if (!check_input_char(current_char))
		bad_character(i + 1, current_char);
	      if (current_char == '_')
		grid[i][j] = pset_full (grid_size);
	      else
		grid[i][j] = char2pset (current_char);
	    }
	  j++;

	  if (grid_size != 0 && j > grid_size)
	    bad_line (i + 1);	  
	  
	  if (j > 64)
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
      bad_number_of_lines ();
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
      grid_print (grid);
    }

  if ((output_stream != stdout && fclose (output_stream) != 0) ||
      (in != NULL && fclose (in) != 0))
    {
      fprintf (stderr, "File cannot be closed\n");
      exit (EXIT_FAILURE);
    }
  exit (EXIT_SUCCESS);
}
