/*
 * profInt.h --
 *
 *	Internal declarations of the profile module.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _PROFINT
#define _PROFINT


/*
 * A histogram of PC samples is kept for use by gprof.
 * Each sample is a counter that gets incremented when
 * the PC is in the range for the counter.  The PC's are
 * clustered in groups of 1, 2, 4, 8... values and there
 * is a counter for each group.  A groupsize of 1 means there
 * is a counter for every possible PC value.  The even sizes of
 * the groups lets us generate the index into the array of counters
 * by shifting.  (The about to shift also takes into account the
 * size of an instruction, it averages that to two bytes!)
 */

#define PROF_PC_GROUP_SIZE	2
#define PROF_GROUP_SHIFT	1
#define PROF_ARC_GROUP_SHIFT	4
#define PROF_INSTR_SIZE_SHIFT	2
#define PROF_PC_SHIFT		(PROF_GROUP_SHIFT + PROF_INSTR_SIZE_SHIFT)

/*
 * Storage is set aside to hold call graph arc execution counts.
 * The number of arcs stored is the number of instruction in the
 * kernel divided by CALL_RATIO. ie. This represents the proportion
 * of the instructions that are calls.
 */

#define PROF_CALL_RATIO 	1

/*
 * A raw call graph arc just includes the callee's PC and the number of
 * times the arc was executed.  The caller of the arc is the index of the
 * arcIndex index shifted by PROF_ARC_SHIFT.
 */
#define PROF_ARC_SHIFT		(PROF_ARC_GROUP_SHIFT + PROF_INSTR_SIZE_SHIFT)
typedef struct ProfRawArc {
	int	calleePC;
	int	count;
	struct ProfRawArc *link;
} ProfRawArc;

typedef struct ProfArc {
	int	callerPC;
	int	calleePC;
	int	count;
} ProfArc;

extern int		profArcListSize;
extern ProfRawArc	*profArcList;
extern ProfRawArc	*profArcListFreePtr;
extern ProfRawArc	*profArcListEndPtr;

extern int		profArcIndexSize;
extern ProfRawArc	**profArcIndex;

/*
 * An of/off switch for profiling.
 */

extern Boolean profEnabled;

#endif _PROFINT
