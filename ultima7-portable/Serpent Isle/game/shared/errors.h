#ifndef SHARED_ERRORS_H
#define SHARED_ERRORS_H

/* The game's fatal hook chain, which game code reporting an error runs too. */
#include "../main/mem/errors.h"

namespace Shared {

/* Formats the message, runs the fatal hook, prints it and ends the program with code 1. */
void FatalError(char *fmt, ...);

}

#endif
