/*
 * Stub of VS Context.h
 *
 */

#pragma once

#include "version.h"
#include "CameraContext.h"

class Context
{
public:
	Context();
	~Context();

	Context& operator=(Context &rhs);

	CameraContext _camera;
};
