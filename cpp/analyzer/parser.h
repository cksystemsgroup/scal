#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "operation.h"
#include "element.h"

Operation** parse_logfile(char* filename, int num_ops);

Element** parse_linearization(FILE* input, int num_ops);

#endif // PARSER_H
