#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

void error_msg_and_die(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}
