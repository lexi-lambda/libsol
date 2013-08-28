
#ifndef SOLAR_H
#define	SOLAR_H

#include "solop.h"

#define SOLAR_DEFINITION(name) SolObject name (SolList arguments, SolObject self)

SolObject solar_load(char* filename);

#endif	/* SOLAR_H */

