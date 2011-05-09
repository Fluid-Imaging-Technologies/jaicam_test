/*
 * Stub of VS CameraContext.h
 *
 */

#pragma once

#include "version.h"
#include "CameraType.h"


class ConvolutionFilter
{
public:
	char _display_name[32];
	int _mil_constant;
};


class CameraContext
{
public:
	CameraContext();
	~CameraContext();

	CameraContext& operator=(CameraContext &rhs);

	bool isGrabBufferYUV();	
	
	// Camera Tab
	long _camera_val[NUM_CAMERA_CONTROLS];

	double _bayerCoefficients[3];
	bool _use_fpga_for_trigger_and_flash;

	// Advanced Camera dialog
	int _camera_id;
	char _camera_init_string[64];
};
