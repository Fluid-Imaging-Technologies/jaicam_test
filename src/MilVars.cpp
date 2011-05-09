/*
 * $Id: MilVars.cpp,v 1.1.1.1 2009/01/06 19:15:52 scott Exp $
 *
 */

#include <windows.h>

#include "MilVars.h"

// global scope, exported in MilVars.h
MilVars gMilVars;

MilVars::MilVars()
{
	_MilApplication = 0;
	_MilLiveSystem = -1;	
}

MilVars::~MilVars()
{
	releaseSystemID();

	if (_MilApplication) {
		MappFree(_MilApplication);
		_MilApplication = 0;
	}
}

MIL_ID MilVars::getApplicationID()
{
	if (!_MilApplication) {
		_MilApplication = MappAlloc(M_DEFAULT, M_NULL);
	}

	return _MilApplication;		
}

MIL_ID MilVars::getStaticSystemID()
{
	releaseSystemID();

	if (!getApplicationID()) {
		return -1;
	}

	return M_DEFAULT_HOST;
}

MIL_ID MilVars::getLiveSystemID()
{
	if (_MilLiveSystem >= 0) {
		return _MilLiveSystem;
	}

	if (!getApplicationID()) {
		return -1;
	}

#if !defined (_DEBUG)
	MappControl(M_ERROR, M_PRINT_DISABLE);
#endif

	_MilLiveSystem = MsysAlloc(M_SYSTEM_METEOR_II_1394, M_DEFAULT, M_COMPLETE, M_NULL);
	
	if ((_MilLiveSystem == M_NULL) || (M_NULL_ERROR != MappGetError(M_CURRENT, M_NULL))) {
		_MilLiveSystem = -1;
	}

#if !defined (_DEBUG)
	MappControl(M_ERROR, M_PRINT_ENABLE);
#endif

	return _MilLiveSystem;
}

void MilVars::releaseSystemID()
{
	if (_MilLiveSystem >= 0) {
		MsysFree(_MilLiveSystem);
		_MilLiveSystem = -1;
	}
}