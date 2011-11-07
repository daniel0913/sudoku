#ifndef PARSER_H
#define PARSER_H

/*
 * Parses the grid in the stream `in` and expects it to be opened
 * beforehand. Exits the program with an error if it can't parse the
 * input. It doesn't close the file. And writes the parsed grid in the
 * global variable `grid` and the size of it on the global variable
 * `grid_size`. 
 */
void grid_parser (FILE *in);

#endif /* PARSER_H */
