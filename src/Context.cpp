/*
 * Stub of VS Context.cpp
 *
 */

#include "Context.h"

Context::Context() 
{	
}

Context::~Context() 
{	
}

Context& Context::operator=(Context &rhs)
{
	if (this != &rhs)	
		_camera = rhs._camera;

	return *this;
}