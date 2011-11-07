#include <stdio.h>
#include <stdlib.h>
#include <preemptive_set.h>

#include "sudoku.h"
#include "main.h"

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
