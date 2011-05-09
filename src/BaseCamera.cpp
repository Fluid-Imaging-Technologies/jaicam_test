/*
 * Stub of VS BaseCamera.cpp
 *
 */

#include "BaseCamera.h"
#include "Context.h"
#include "CameraContext.h"

#define UNINITIALIZED_CACHE_VALUE 0x3fffffff


BaseCamera::BaseCamera(int engNum)
{
	_engNum = engNum;
	_camera_id = -1;
	_initialized = false;
	_cameraInitErrors = false;
	_isColor = false;

	clearAllCacheValues();
}

bool BaseCamera::getInitErrors(char *buff, int maxlen)
{
	int err_len;

	if (_cameraInitErrors) {
		if (buff && maxlen > 64) {
			err_len = strlen(_cameraInitErrorBuff);

			if (err_len >= maxlen) {
				err_len = maxlen - 1;
			}

			strncpy_s(buff, maxlen, _cameraInitErrorBuff, err_len);
		}
	}

	return _cameraInitErrors;
}

bool BaseCamera::initialize(Context *ctx, HANDLE hStopEvent)
{
	if (!ctx)
		return false;

	CameraContext *cc = &ctx->_camera;

	_camera_id = cc->_camera_id;
	_isColor = gCameraDB[_camera_id]._isColor;

	clearAllCacheValues();

	return true;
}

bool BaseCamera::canUserAdjust(int setting_index)
{
	if (_camera_id < 0 || _camera_id >= NUM_KNOWN_CAMERAS) {
		return false;
	}

	return gCameraDB[_camera_id]._can_adjust[setting_index];
}

void BaseCamera::clearAllCacheValues()
{
	for (int i = 0; i < NUM_CAMERA_PROPERTIES; i++) {
		_cacheValue[i] = UNINITIALIZED_CACHE_VALUE;
	}
}

bool BaseCamera::isValidCacheValue(int index) 
{
	if (index < 0 || index >= NUM_CAMERA_PROPERTIES) {
		return false;
	}

	return  (_cacheValue[index] != UNINITIALIZED_CACHE_VALUE);
}

void BaseCamera::invalidateCacheValue(int index)
{
	if (index >= 0 && index < NUM_CAMERA_PROPERTIES) {
		_cacheValue[index] = UNINITIALIZED_CACHE_VALUE;
	}
}

long BaseCamera::getCacheValue(int index)
{
	if (index < 0 || index >= NUM_CAMERA_PROPERTIES) {
		return UNINITIALIZED_CACHE_VALUE;
	}

	return  _cacheValue[index];
}

void BaseCamera::setCacheValue(int index, long value)
{
	if (index >= 0 && index < NUM_CAMERA_PROPERTIES) {
		_cacheValue[index] = value;
	}
}
