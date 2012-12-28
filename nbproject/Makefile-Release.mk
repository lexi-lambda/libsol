#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-MacOSX
CND_DLIB_EXT=dylib
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include libsol-Makefile.mk

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/solop.o \
	${OBJECTDIR}/soltypes.o \
	${OBJECTDIR}/soltoken.o \
	${OBJECTDIR}/runtime.o \
	${OBJECTDIR}/sol.o \
	${OBJECTDIR}/sollist.o \
	${OBJECTDIR}/solfunc.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libsol.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libsol.a: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libsol.a
	${AR} -rv ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libsol.a ${OBJECTFILES} 
	$(RANLIB) ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libsol.a

${OBJECTDIR}/solop.o: solop.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/solop.o solop.c

${OBJECTDIR}/soltypes.o: soltypes.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/soltypes.o soltypes.c

${OBJECTDIR}/soltoken.o: soltoken.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/soltoken.o soltoken.c

${OBJECTDIR}/runtime.o: runtime.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/runtime.o runtime.c

${OBJECTDIR}/sol.o: sol.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/sol.o sol.c

${OBJECTDIR}/sollist.o: sollist.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/sollist.o sollist.c

${OBJECTDIR}/solfunc.o: solfunc.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -std=c99 -MMD -MP -MF $@.d -o ${OBJECTDIR}/solfunc.o solfunc.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libsol.a

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
