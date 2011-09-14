#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>

#include "sudoku.h"

bool verbose = false;

static void
usage(int status)
{
  if(status == EXIT_SUCCESS)
    {
      printf("Usage: sudoku [OPTION] FILE...\n"
             "Solve Sudoku puzzles of variable sizes (1-4)\n"
	     "\n"
	     "  -o, --output=FILE   write result to FILE\n"
             "  -v, --verbose       verbose output\n"
	     "  -V, --version       display version and exit\n"
	     "  -h, --help          display this help\n");
             
    }
  else
    {
      fprintf(stderr, "Try `sudoku --help` for more information\n");
    }
}

static void
version(void)
{
  printf("%s %d.%d.%d\n"
	 "This software is a sudoku solver\n", 
	 PROG_NAME, PROG_VERSION, PROG_SUBVERSION, PROG_REVISION);
}

int 
main(int argc, char* argv[])
{
  int optc; 
  struct option long_opts[] = 
    {
      {"output",  required_argument, 0, 'o'}, 
      {"verbose", no_argument,       0, 'v'},
      {"version", no_argument,       0, 'V'},
      {"help",    no_argument,       0, 'h'},
      {0, 0, 0, 0}
    };
  
  if(argc < 2)
    {
      fprintf(stderr, "No argument given\n");
      usage(EXIT_FAILURE);
      exit(EXIT_FAILURE);
    }

  while((optc = getopt_long(argc, argv, "o:vVh", long_opts, NULL)) != -1)
    {
      switch(optc)
	{
	case 'o':
	  printf("Output to %s\n", optarg);
	  break;

	case 'v':
	  verbose = true;
	  break; 

	case 'V':
	  version();
	  break;

	case 'h':
	  usage(EXIT_SUCCESS);
	  break;

	default: 
	  usage(EXIT_FAILURE);
	}
    }
}
