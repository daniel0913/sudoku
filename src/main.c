#include <libgen.h>
#include <getopt.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <preemptive_set.h>

#include "parser.h"
#include "sudoku.h"

void
usage (int status)
{
  if (status == EXIT_SUCCESS)
    {
      printf (
	"Usage: %s [OPTION] FILE\n"
        "Solve Sudoku puzzles of variable sizes (1-64)\n"
	"\n"
	"  -o, --output=FILE   write result to FILE\n"
	"  -s, --strict        generate a unique-solution grid"
	"  -g, --generate=SIZE generates a grid of size SIZE (by default 9)\n"
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
