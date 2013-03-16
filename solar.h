/* 
 * File:   solar.h
 * Author: Jake
 *
 * Created on January 15, 2013, 1:12 PM
 */

#ifndef SOLAR_H
#define	SOLAR_H

#include "solop.h"

#define SOLAR_DEFINITION(name) SolObject name (SolList arguments, SolObject self)

void solar_load(char* filename);

#endif	/* SOLAR_H */

