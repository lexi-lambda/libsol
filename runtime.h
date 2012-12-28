/* 
 * File:   runtime.h
 * Author: Jake
 *
 * Created on November 19, 2012, 11:21 AM
 */

#ifndef RUNTIME_H
#define	RUNTIME_H

#include "sol.h"
#include "solfunc.h"
#include "sollist.h"
#include "solop.h"
#include "soltoken.h"
#include "soltypes.h"

void sol_runtime_init();
void sol_runtime_destroy();

#endif	/* RUNTIME_H */

