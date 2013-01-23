/* 
 * File:   solar.h
 * Author: Jake
 *
 * Created on January 15, 2013, 1:12 PM
 */

#ifndef SOLAR_H
#define	SOLAR_H

#include "solop.h"


void solar_load(char* filename);

void solar_register_function(char* object, char* name, SolOperatorRef function, bool on_prototype);

#endif	/* SOLAR_H */

