/*
 * $Id: JaiGigECamera.cpp,v 1.39 2009/12/28 19:08:14 scott Exp $
 *
 */

#include <process.h>

#include "JaiGigECamera.h"
#include "Context.h"
#include "CameraContext.h"

#include <Jai_Factory_Dynamic.h>


#define BAYER_GAIN_RED 0
#define BAYER_GAIN_GREEN 1
#define BAYER_GAIN_BLUE 2

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
	_imageReady = false;
	_hGainNode = 0;
    _hExposureNode = 0;
	_bayerGain[BAYER_GAIN_RED] = 0x01000;
	_bayerGain[BAYER_GAIN_GREEN] = 0x01000;
	_bayerGain[BAYER_GAIN_BLUE] = 0x01000;

	_milDisplay = 0;
	_milImageBuf = 0;

	memset(&_rgbBuff, 0, sizeof(_rgbBuff));

	InitializeCriticalSection(&_csCopyImage);
}

JaiGigECamera::~JaiGigECamera() 
{
	freeCamera();

	if (_milDisplay) {
		MdispFree(_milDisplay);
		_milDisplay = 0;
	}

	if (_milImageBuf) {
		MbufFree(_milImageBuf);
		_milImageBuf = 0;
	}

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
			_cameraInitErrors++;
			return false;
		}
	}

	result = J_Factory_UpdateCameraList(_hFactory, &hasChanged);
	if (result != J_ST_SUCCESS) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff), 
				"Failed to update camera list", _TRUNCATE);
		_cameraInitErrors++;
		return false;
	}

	result = J_Factory_GetNumOfCameras(_hFactory, &deviceCount);
	if (result != J_ST_SUCCESS) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
				"Failed to get number of cameras", _TRUNCATE);
		_cameraInitErrors++;
		return false;
	}

	if (deviceCount == 0) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff), 
				"No cameras found", _TRUNCATE);
		_cameraInitErrors++;
		return false;
	}

	for (uint32_t i = 0; i < deviceCount; i++) {
		size = J_CAMERA_ID_SIZE;
		memset(_jCameraId, 0, sizeof(_jCameraId));

		result = J_Factory_GetCameraIDByIndex(_hFactory, i, _jCameraId, &size);
		if (result != J_ST_SUCCESS) {
			strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff), 
					"Failed to get camera id", _TRUNCATE);
			_cameraInitErrors++;
			return false;
		}

		if (strstr(_jCameraId, "INT=>FD"))
			break;
	}

	if (strlen(_jCameraId) < 0) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff), 
				"No cameras using the Filter Driver", _TRUNCATE);
		_cameraInitErrors++;
		return false;
	}

	result = J_Camera_Open(_hFactory, _jCameraId, &_hCamera);
	if (result != J_ST_SUCCESS) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff), 
				"Failed to open camera", _TRUNCATE);
		_cameraInitErrors++;
		return false;
	}

	if (!applyNonConfigurableSettings()) {
		_cameraInitErrors++;
		return false;
	}

	if (!applySettings(ctx)) {
		_cameraInitErrors++;
		return false;
	}

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

void JaiGigECamera::imageStreamCallback(J_tIMAGE_INFO *pAqImageInfo)
{
    ++_imageCount;
	
	if (!_rgbBuff.pImageBuffer) {
		if (J_ST_SUCCESS != J_Image_Malloc(pAqImageInfo, &_rgbBuff))
			return;
	}

	if (J_ST_SUCCESS != J_Image_FromRawToImageEx(pAqImageInfo, 
												&_rgbBuff,
												BAYER_EXTEND,
												_bayerGain[BAYER_GAIN_RED], 
												_bayerGain[BAYER_GAIN_GREEN], 
												_bayerGain[BAYER_GAIN_BLUE]))
		return;

	MbufPutColor(_milImageBuf, M_PACKED | M_BGR24, M_ALL_BANDS, _rgbBuff.pImageBuffer);
}

void JaiGigECamera::imageStreamCallback2(J_tIMAGE_INFO *pAqImageInfo)
{
	J_STATUS_TYPE result;
	unsigned char *p, *q;

    ++_imageCount;
	
	if (!_rgbBuff.pImageBuffer) {
		if (J_ST_SUCCESS != J_Image_Malloc(pAqImageInfo, &_rgbBuff))
			return;
	}

	p = NULL;
	MbufInquire(_milImageBuf, M_HOST_ADDRESS, &p);

	if (!p)
		return;

	q = _rgbBuff.pImageBuffer;
	_rgbBuff.pImageBuffer = p;

	result = J_Image_FromRawToImageEx(pAqImageInfo, 
										&_rgbBuff,
										BAYER_EXTEND,
										_bayerGain[BAYER_GAIN_RED], 
										_bayerGain[BAYER_GAIN_GREEN], 
										_bayerGain[BAYER_GAIN_BLUE]);

	_rgbBuff.pImageBuffer = q;

	if (result == J_ST_SUCCESS)
		MbufControl(_milImageBuf, M_MODIFIED, M_DEFAULT);
}

bool JaiGigECamera::initializeMILWindow()
{
	if (!_milImageBuf) {
		_milImageBuf = MbufAllocColor(M_DEFAULT_HOST,
									3,
									gCameraDB[_camera_id]._defaultWidth,
									gCameraDB[_camera_id]._defaultHeight,
									8 + M_UNSIGNED,
									M_IMAGE | M_DISP | M_BGR24 | M_PACKED,
									M_NULL);

		if (!_milImageBuf)
			return false;

		MbufClear(_milImageBuf, 0.0);
	}

	if (!_milDisplay) {
		_milDisplay = MdispAlloc(M_DEFAULT_HOST, 
							M_DEFAULT, 
							M_DEF_DISPLAY_FORMAT, 
							M_DEFAULT, 
							M_NULL);

		if (!_milDisplay)
			return false;

		MdispSelectWindow(_milDisplay, _milImageBuf, NULL);	
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

	if (!initializeMILWindow())
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

#if defined (SHOW_JAI_WINDOW)
	if (_hView) {
		result = J_Image_CloseViewWindow(_hView);
		
		if (result != J_ST_SUCCESS)
			MessageBox(0, "Could not close view window!", "Error", MB_OK);

		_hView = NULL;
	}
#endif

	if (_rgbBuff.pImageBuffer)
		J_Image_Free(&_rgbBuff);

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

uint32_t JaiGigECamera::convertBayerGainToJaiFormat(double bayerGain)
{
	bayerGain *= 4096.0;

	// 8X
	if (bayerGain > 131072.0)
		return 131072;
	
	// ~0.01X
	if (bayerGain < 40.0)
		return 40;
	
	return (uint32_t) bayerGain;
}

bool JaiGigECamera::applySettings(Context *ctx)
{
	long current, min, max;
	CameraContext *cc = &ctx->_camera;
		
	if (cc->_bayerCoefficients[BAYER_GAIN_RED] > 0.0)
		_bayerGain[BAYER_GAIN_RED] = convertBayerGainToJaiFormat(cc->_bayerCoefficients[BAYER_GAIN_RED]);

	if (cc->_bayerCoefficients[BAYER_GAIN_GREEN] > 0.0)
		_bayerGain[BAYER_GAIN_GREEN] = convertBayerGainToJaiFormat(cc->_bayerCoefficients[BAYER_GAIN_GREEN]);

	if (cc->_bayerCoefficients[BAYER_GAIN_BLUE] > 0.0)
		_bayerGain[BAYER_GAIN_BLUE] = convertBayerGainToJaiFormat(cc->_bayerCoefficients[BAYER_GAIN_BLUE]);


	if (!getShutterValue(&current, &min, &max)) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
				"Failed to read current shutter", _TRUNCATE);

		return false;
	}
	
	if (!setShutterValue(cc->_camera_val[CAMERA_SHUTTER])) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
				"Failed to set shutter", _TRUNCATE);

		return false;
	}

	if (!getGainValue(&current, &min, &max)) {
		strncpy_s(_cameraInitErrorBuff, sizeof(_cameraInitErrorBuff),
				"Failed to read current gain", _TRUNCATE);

		return false;
	}
	
	if (!setGainValue(cc->_camera_val[CAMERA_GAIN])) {
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

StringList* JaiGigECamera::getFeatureList(HWND hWnd)
{
	J_STATUS_TYPE result;
	uint32_t count, size, i;
	NODE_HANDLE hNode;
	char name[256];
	StringList *list;

	if (!_hCamera)
		return NULL;

	result = J_Camera_GetNumOfNodes(_hCamera, &count);
	if (result != J_ST_SUCCESS)
		return NULL;

	if (count < 1)
		return NULL;

	list = new StringList();
	if (!list)
		return NULL;

	for (i = 0; i < count; i++) {
		result = J_Camera_GetNodeByIndex(_hCamera, i, &hNode);
		if (result != J_ST_SUCCESS) {
			MessageBox(hWnd, "Error geting node handle", "Error", MB_OK);
			break;
		}

		memset(name, 0, sizeof(name));
		size = sizeof(name);

		result = J_Node_GetName(hNode, name, &size, 0);
		if (result != J_ST_SUCCESS) {
			MessageBox(hWnd, "Error getting node name", "Error", MB_OK);
			break;
		}

		if (!list->addString(name)) {
			MessageBox(hWnd, "Error adding node name string to list", "Error", MB_OK);
			break;
		}
	}

	return list;
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