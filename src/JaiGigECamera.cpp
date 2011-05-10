/*
 * $Id: JaiGigECamera.cpp,v 1.39 2009/12/28 19:08:14 scott Exp $
 *
 */

#include <process.h>

#include "JaiGigECamera.h"
#include "Context.h"
#include "CameraContext.h"

#include <Jai_Factory_Dynamic.h>

#define NODE_NAME_WIDTH         "Width"
#define NODE_NAME_HEIGHT        "Height"
#define NODE_NAME_PIXELFORMAT   "PixelFormat"
#define NODE_NAME_GAIN          "GainRaw"
#define NODE_NAME_EXPOSURE      "ExposureTimeRaw"
#define NODE_NAME_ACQSTART      "AcquisitionStart"
#define NODE_NAME_ACQSTOP       "AcquisitionStop"
#define NODE_NAME_EXPOSURE_MODE "ExposureMode"

JaiGigECamera::JaiGigECamera(int engNum) : BaseCamera(engNum)
{
	_hFactory = 0;
	_hCamera = 0;
	memset(_jCameraId, 0, sizeof(_jCameraId));
	_hStreamThread = 0;
	_hView = 0;
	_imageReady = false;
	_hGainNode = 0;
    _hExposureNode = 0;

	InitializeCriticalSection(&_csCopyImage);
}

JaiGigECamera::~JaiGigECamera() 
{
	freeCamera();

	DeleteCriticalSection(&_csCopyImage);
}

bool JaiGigECamera::initialize(Context *ctx, HANDLE hStopEvent)
{
	J_STATUS_TYPE result;
	unsigned int deviceCount, size;
	bool8_t hasChanged = false;

	if (_initialized) {
		return true;
	}

	if (!BaseCamera::initialize(ctx, hStopEvent)) {
		return false;
	}

	CameraContext *cc = &ctx->_camera;

	if (!_hFactory) {
		result = J_Factory_Open("", &_hFactory);
		if (result != J_ST_SUCCESS) {
			strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
					"Failed to open factory", _TRUNCATE);
			return false;
		}
	}

	result = J_Factory_UpdateCameraList(_hFactory, &hasChanged);
	if (result != J_ST_SUCCESS) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff), 
				"Failed to update camera list", _TRUNCATE);
		return false;
	}

	result = J_Factory_GetNumOfCameras(_hFactory, &deviceCount);
	if (result != J_ST_SUCCESS) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
				"Failed to get number of cameras", _TRUNCATE);
		return false;
	}

	if (deviceCount == 0) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff), 
				"No cameras found", _TRUNCATE);
		return false;
	}

	for (uint32_t i = 0; i < deviceCount; i++) {
		size = J_CAMERA_ID_SIZE;
		memset(_jCameraId, 0, sizeof(_jCameraId));

		result = J_Factory_GetCameraIDByIndex(_hFactory, i, _jCameraId, &size);
		if (result != J_ST_SUCCESS) {
			strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff), 
					"Failed to get camera id", _TRUNCATE);
			return false;
		}

		if (strstr(_jCameraId, "INT=>FD"))
			break;
	}

	if (strlen(_jCameraId) < 0) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff), 
				"No cameras using the Filter Driver", _TRUNCATE);
		return false;
	}

	result = J_Camera_Open(_hFactory, _jCameraId, &_hCamera);
	if (result != J_ST_SUCCESS) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff), 
				"Failed to open camera", _TRUNCATE);
		return false;
	}

	if (!applyNonConfigurableSettings())
		return false;

	if (!applySettings(ctx))
		return false;

	_initialized = true;

	return true;
}

bool JaiGigECamera::freeCamera()
{
	J_STATUS_TYPE result;

	stopImageGrabThread();

	if (_hCamera) {
		result = J_Camera_Close(_hCamera);

		if (result != J_ST_SUCCESS)
			MessageBox(0, "Failed to close camera", "Error", MB_OK);
		else
			_hCamera = 0;
	}

	if (_hFactory) {
		result = J_Factory_Close(_hFactory);

		if (result != J_ST_SUCCESS)
			MessageBox(0, "Failed to close factory", "Error", MB_OK);
		else
			_hFactory = 0;
	}

	_initialized = false;

	return true;
}

void JaiGigECamera::imageStreamCallback(J_tIMAGE_INFO * pAqImageInfo)
{
    ++_imageCount;

	if (_hView)
		J_Image_ShowImage(_hView, pAqImageInfo);
}

bool JaiGigECamera::openViewWindow()
{
	J_STATUS_TYPE result;
	SIZE sz;
	POINT pt;

	sz.cx = gCameraDB[_camera_id]._defaultWidth;
	sz.cy = gCameraDB[_camera_id]._defaultHeight;

	pt.x = 400;
	pt.y = 20;

	result = J_Image_OpenViewWindow("Image View", &pt, &sz, &_hView);

	if (result != J_ST_SUCCESS) {
		MessageBox(0, "Could not open view window", "Error", MB_OK);
		return false;
	}

	return true;
}

bool JaiGigECamera::startImageGrabThread() 
{ 
	J_STATUS_TYPE result;
	uint32_t img_size;

	if (_hStreamThread) {
		MessageBox(0, "Camera image thread already running", "Error", MB_OK);
		return false;
	}

	if (!_initialized) {
		MessageBox(0, "Camera not initialized", "Error", MB_OK);
		return false;
	}

	_imageReady = false;
	_imageCount = 0;

	if (!openViewWindow())
		return false;

	img_size = gCameraDB[_camera_id]._defaultWidth * gCameraDB[_camera_id]._defaultHeight;

	result = J_Image_OpenStream(_hCamera, 0, 
								reinterpret_cast<J_IMG_CALLBACK_OBJECT>(this), 
								reinterpret_cast<J_IMG_CALLBACK_FUNCTION>(&JaiGigECamera::imageStreamCallback), 
								&_hStreamThread, 
								img_size);

    if (result != J_ST_SUCCESS) {
        MessageBox(0, "Error opening image stream", "Error", MB_OK);
        return false;
    }

	result = J_Camera_ExecuteCommand(_hCamera, NODE_NAME_ACQSTART);
   
	if (result != J_ST_SUCCESS) {
        MessageBox(0, "Could not Start Acquisition!", "Error", MB_OK);
        J_Image_CloseStream(_hStreamThread);
		_hStreamThread = 0;
    }

	return true; 
}

bool JaiGigECamera::stopImageGrabThread() 
{ 
    J_STATUS_TYPE result;

    if (_hStreamThread) {
		if (_hCamera) {
			result = J_Camera_ExecuteCommand(_hCamera, NODE_NAME_ACQSTOP);
        
			if (result != J_ST_SUCCESS)
				MessageBox(0, "Could not Stop Acquisition thread!", "Error", MB_OK);
		}

        result = J_Image_CloseStream(_hStreamThread);

        if (result != J_ST_SUCCESS)
            MessageBox(0, "Could not close stream thread!", "Error", MB_OK);

        _hStreamThread = NULL;
    }

	if (_hView) {
		result = J_Image_CloseViewWindow(_hView);
		
		if (result != J_ST_SUCCESS)
			MessageBox(0, "Could not close view window!", "Error", MB_OK);

		_hView = NULL;
	}

	return true;
}

bool JaiGigECamera::fetchImage(MIL_ID milImageBuff) 
{ 
	bool copy_ok = false;
	/*
	if (_imageReady) {
		if (TryEnterCriticalSection(&_csCopyImage)) {
			if (milImageBuff) {

			}

		}

		_imageReady = false;

		LeaveCriticalSection(&_csCopyImage);
	}
	*/
	return copy_ok;
}

bool JaiGigECamera::applyNonConfigurableSettings()
{
	J_STATUS_TYPE result;
	NODE_HANDLE hNode;
	int64_t val;
	int w, h;


	result = J_Camera_GetNodeByName(_hCamera, NODE_NAME_PIXELFORMAT, &hNode);
	if (result != J_ST_SUCCESS) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
				"Failed to get PixelFormat node", _TRUNCATE);
		return false;
	}

	result = J_Node_GetValueInt64(hNode, 0, &val);
	if (result != J_ST_SUCCESS) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
				"Failed to get PixelFormat value", _TRUNCATE);
		return false;
	}

	if (val != J_GVSP_PIX_BAYRG8) {
		result = J_Node_SetValueInt64(hNode, 0, (int64_t) J_GVSP_PIX_BAYRG8);
		if (result != J_ST_SUCCESS) {
			strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
					"Failed to set PixelFormat to BAYRG8", _TRUNCATE);
			return false;
		}
	}

	// make sure width and height are what we expect
	result = J_Camera_GetValueInt64(_hCamera, NODE_NAME_WIDTH, &val);
    if (result != J_ST_SUCCESS) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
				"Failed to get camera actual width", _TRUNCATE);
		return false;
    }

	w = (int) val;

    result = J_Camera_GetValueInt64(_hCamera, NODE_NAME_HEIGHT, &val);
    if (result != J_ST_SUCCESS) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
				"Failed to get camera actual height", _TRUNCATE);
		return false;
    }

	h = (int) val;

	if ((w != gCameraDB[_camera_id]._defaultWidth) || (h != gCameraDB[_camera_id]._defaultHeight)) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff), 
				"Inconsistency in camera reported image size and internal camera database",
				_TRUNCATE);
		return false;
	}

	return true;
}

bool JaiGigECamera::applySettings(Context *ctx)
{
	long current, min, max;

	if (!getShutterValue(&current, &min, &max)) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
				"Failed to read current shutter", _TRUNCATE);

		return false;
	}
	
	if (!setShutterValue(ctx->_camera._camera_val[CAMERA_SHUTTER])) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
				"Failed to set shutter", _TRUNCATE);

		return false;
	}

	if (!getGainValue(&current, &min, &max)) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
				"Failed to read current gain", _TRUNCATE);

		return false;
	}
	
	if (!setGainValue(ctx->_camera._camera_val[CAMERA_GAIN])) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
				"Failed to set gain", _TRUNCATE);

		return false;
	}

	return true;
}

bool JaiGigECamera::getGainValue(long *current, long *min, long *max)
{
	J_STATUS_TYPE result;
	int64_t val;

	if (!_hGainNode) {
		result = J_Camera_GetNodeByName(_hCamera, NODE_NAME_GAIN, &_hGainNode);

		if (result != J_ST_SUCCESS)
			return false;			
	}

	if (current) {
		if (isValidCacheValue(CAMERA_GAIN)) {
			*current = getCacheValue(CAMERA_GAIN);
		}
		else {
			result = J_Node_GetValueInt64(_hGainNode, TRUE, &val);

			if (result != J_ST_SUCCESS)
				return false;

			*current = (long) val;
			setCacheValue(CAMERA_GAIN, *current);
		}
	}

	if (min) {
		if (isValidCacheValue(CACHE_CAMERA_GAIN_MIN)) {
			*min = getCacheValue(CACHE_CAMERA_GAIN_MIN);
		}
		else {
			result = J_Node_GetMinInt64(_hGainNode, &val);

			if (result != J_ST_SUCCESS)
				return false;

			*min = (long) val;
			setCacheValue(CACHE_CAMERA_GAIN_MIN, *min);
		}
	}

	if (max) {
		if (isValidCacheValue(CACHE_CAMERA_GAIN_MAX)) {
			*max = getCacheValue(CACHE_CAMERA_GAIN_MAX);
		}
		else {
			result = J_Node_GetMaxInt64(_hGainNode, &val);

			if (result != J_ST_SUCCESS)
				return false;

			*max = (long) val;
			setCacheValue(CACHE_CAMERA_GAIN_MAX, *max);
		}
	}

	return true;
}

bool JaiGigECamera::setGainValue(long val)
{
	J_STATUS_TYPE result;
	int64_t val64 = val;

	if (!isValidCacheValue(CAMERA_GAIN) || val != getCacheValue(CAMERA_GAIN)) {
		result = J_Camera_SetValueInt64(_hCamera, NODE_NAME_GAIN, val64);

		if (result != J_ST_SUCCESS)
				return false;
		else
			setCacheValue(CAMERA_GAIN, val);
	}

	return true;
}

bool JaiGigECamera::getShutterValue(long *current, long *min, long *max)
{
	J_STATUS_TYPE result;
	int64_t val;

	if (!_hExposureNode) {
		result = J_Camera_GetNodeByName(_hCamera, NODE_NAME_EXPOSURE, &_hExposureNode);

		if (result != J_ST_SUCCESS)
			return false;			
	}

	if (current) {
		if (isValidCacheValue(CAMERA_SHUTTER)) {
			*current = getCacheValue(CAMERA_SHUTTER);
		}
		else {
			result = J_Node_GetValueInt64(_hExposureNode, TRUE, &val);

			if (result != J_ST_SUCCESS)
				return false;

			*current = (long) val;
			setCacheValue(CAMERA_SHUTTER, *current);
		}
	}

	if (min) {
		if (isValidCacheValue(CACHE_CAMERA_SHUTTER_MIN)) {
			*min = getCacheValue(CACHE_CAMERA_SHUTTER_MIN);
		}
		else {
			result = J_Node_GetMinInt64(_hExposureNode, &val);

			if (result != J_ST_SUCCESS)
				return false;

			*min = (long) val;
			setCacheValue(CACHE_CAMERA_SHUTTER_MIN, *min);
		}
	}

	if (max) {
		if (isValidCacheValue(CACHE_CAMERA_SHUTTER_MAX)) {
			*max = getCacheValue(CACHE_CAMERA_SHUTTER_MAX);
		}
		else {
			result = J_Node_GetMaxInt64(_hExposureNode, &val);

			if (result != J_ST_SUCCESS)
				return false;

			*max = (long) val;
			setCacheValue(CACHE_CAMERA_SHUTTER_MAX, *max);
		}
	}

	return true;
}

bool JaiGigECamera::setShutterValue(long val)
{
	J_STATUS_TYPE result;
	int64_t val64 = val;

	if (!isValidCacheValue(CAMERA_SHUTTER) || val != getCacheValue(CAMERA_SHUTTER)) {
		result = J_Camera_SetValueInt64(_hCamera, NODE_NAME_EXPOSURE, val64);

		if (result != J_ST_SUCCESS)
				return false;
		else
			setCacheValue(CAMERA_SHUTTER, val);
	}

	return true;
}

/*
void getCameraInfo(HWND hWnd)
{
	J_STATUS_TYPE status;
	uint32_t size;
	NODE_HANDLE hNode;
	int width, height, bpp;
	int64_t val64;
	char msg[2 * J_CAMERA_INFO_SIZE]; 
	char cameraModelName[J_CAMERA_INFO_SIZE];
	char cameraMAC[J_CAMERA_INFO_SIZE];
	char pixelFormat[64];
	size = J_CAMERA_INFO_SIZE;
	
	memset(cameraModelName, 0, sizeof(cameraModelName));

	status = J_Factory_GetCameraInfo(hFactory, cameraId, CAM_INFO_MODELNAME,
									cameraModelName, &size);
	if (status != J_ST_SUCCESS) {
		MessageBox(hWnd, "Failed to get camera modelname info", "Error", MB_OK);
		return;
	}

	size = J_CAMERA_INFO_SIZE;
	memset(cameraMAC, 0, sizeof(cameraMAC));

	status = J_Factory_GetCameraInfo(hFactory, cameraId, CAM_INFO_MAC,
									cameraMAC, &size);
	if (status != J_ST_SUCCESS) {
		MessageBox(hWnd, "Failed to get camera MAC info", "Error", MB_OK);
		return;
	}
}
*/