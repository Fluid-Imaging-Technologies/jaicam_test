/*
 * Stub of VS CameraContext.cpp
 *
 */

#include "version.h"

#include "CameraContext.h"
#include "Context.h"

CameraContext::CameraContext() 
{
	initializeCameraDB();

	_camera_id = CAMERA_TYPE_JAI_CB080_GE;

	_camera_val[CAMERA_SHUTTER] = 35;
	_camera_val[CAMERA_GAIN] = 0;

	 _bayerCoefficients[0] = 1.15;
	 _bayerCoefficients[1] = 1.0;
	 _bayerCoefficients[2] = 1.25;
}

CameraContext::~CameraContext() 
{
}

CameraContext& CameraContext::operator=(CameraContext &rhs)
{
	int i;

	if (this != &rhs) {
		_camera_id = rhs._camera_id;

		for (i = 0; i < NUM_CAMERA_CONTROLS; i++)
			_camera_val[i] = rhs._camera_val[i];

		for (i = 0; i < 3; i++)
			_bayerCoefficients[i] = rhs._bayerCoefficients[i];

		_use_fpga_for_trigger_and_flash = rhs._use_fpga_for_trigger_and_flash;

		if (strlen(rhs._camera_init_string) > 0)
			strncpy_s(_camera_init_string, sizeof(_camera_init_string), rhs._camera_init_string, _TRUNCATE);
		else
			memset(_camera_init_string, 0, sizeof(_camera_init_string));
	}

	return *this;
}

bool CameraContext::isGrabBufferYUV()
{
	if (gCameraDB[_camera_id]._isColor) {
		if (gCameraDB[_camera_id]._driver == CAMERA_DRIVER_MIL_FIREWIRE) {
			if (strlen(_camera_init_string) > 0 && strstr(_camera_init_string, "YUV")) {
				return true;
			}
		}
	}

	return false;
}

