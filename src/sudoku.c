#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <libgen.h>

#include "sudoku.h"

bool verbose = false;
char* exec_name;

FILE* output_stream;

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
  FILE* fp; 
  struct option long_opts[] = 
    {
      {"output",  required_argument, 0, 'o'}, 
      {"verbose", no_argument,       0, 'v'},
      {"version", no_argument,       0, 'V'},
      {"help",    no_argument,       0, 'h'},
      {0, 0, 0, 0}
    };

  exec_name = argv[0];
  output_stream = stdout;

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
  
  if ((fclose (output_stream)) != 0)
    {
      fprintf (stderr, "File cannot be closed\n");
      exit (EXIT_FAILURE);
    }
  exit (EXIT_SUCCESS);
}
