#include <math.h>

#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include <preemptive_set.h>

#include "sudoku.h"
#include "main.h"

static void
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

static bool
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

static bool
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

static bool
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

static bool
grid_consistency (pset_t** grid)
{
  return (subgrid_map (grid, &subgrid_consistency));
}

static bool
grid_solved (pset_t** grid)
{
  return (subgrid_map (grid, &all_different));
}

static bool
rm_naked_set (pset_t* naked_set[grid_size], pset_t* subgrid[grid_size])
{
  bool changed = false;
  int upto = pset_cardinality (*naked_set[0]);

  for (unsigned int i = 0; i < grid_size; i++)
    {
      for (int j = 0; j < upto; j++)
	if (subgrid[i] == naked_set[j])
	  goto continue_outter_loop;

      if (pset_and (*subgrid[i], *naked_set[0]) != 0)
	changed = true;

      *subgrid[i] = pset_and (*subgrid[i], pset_negate (*naked_set[0]));
      
    continue_outter_loop: ;
    }
  return (changed);
}

static bool
naked_set (pset_t* subgrid[grid_size])
{
  pset_t* eq_classes[grid_size][grid_size];
  unsigned int cardinality_class[grid_size];

  bool changed = false;
  
  bool assigned = false;
  int used_classes = 0;
  
  for (unsigned int i = 0; i < grid_size; i++)
    cardinality_class[i] = 0;

  for (unsigned int i = 0; i < grid_size; i++)
    {
      for (int j = 0; j < used_classes; j++)
	if (*subgrid[i] == *(eq_classes[j][0]))
	  {
	    eq_classes[j][cardinality_class[j]] = subgrid[i];
	    cardinality_class[j]++;
	    assigned = true;
	  }
      if (!assigned)
	{
	  eq_classes[used_classes][0] = subgrid[i];
	  cardinality_class[used_classes] = 1;
	  used_classes++;
	}
      assigned = false;
    }

  for (int i = 0; i < used_classes; i++)
    {
      bool tmp = false;
      if (cardinality_class[i] >= pset_cardinality (*(eq_classes[i][0])))
	tmp = rm_naked_set (eq_classes[i], subgrid);
      changed = changed || tmp;
    }
  return (changed);
}

/*
 * Crosses off the `colors` in `grid` either from row `row` or the
 * column `column` (-1 signifies that that it should not be crossed
 * off of the row/column, but not on the `k`th block.
 */

static bool
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
static bool
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

  bool tmp = naked_set (subgrid);
  changed = changed || tmp;
  
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



