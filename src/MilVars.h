/*
 *
 */

#pragma once

#include <mil.h>

#if M_MIL_CURRENT_INT_VERSION < 0x0900
typedef long MIL_INT32;
#endif

class MilVars
{
public:
	MilVars();
	~MilVars();

	MIL_ID getApplicationID();
	MIL_ID getLiveSystemID();
	void releaseSystemID();

private:
	MIL_ID getStaticSystemID();
	MIL_ID _MilApplication;
	MIL_ID _MilLiveSystem;
};

extern MilVars gMilVars;
