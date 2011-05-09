/*
 * Stub of VS BaseCamera.h
 */

#pragma once

#include "version.h"
#include <mil.h>
#include "CameraConstants.h"

class Context;
class CameraContext;

class BaseCamera
{
public:
	BaseCamera(int engNum);
	virtual ~BaseCamera() {}

	virtual bool initialize(Context *ctx, HANDLE hStopEvent);
	virtual bool freeCamera() = 0;

	// MIL firewire driver interface
	virtual bool grabImage(MIL_ID milImageBuf) { return false; }
	virtual bool cancelGrab() { return false; }
	virtual bool imageAcquired(MIL_ID milImageBuff) { return false; }
	virtual bool isGrabPending() { return false; }	


	// Matrix usb driver interface
	virtual bool startImageGrabThread() { return false; }
	virtual bool stopImageGrabThread() { return false; }
	virtual bool fetchImage(MIL_ID milImageBuff) { return false; }

	virtual bool applySettings(Context *ctx) = 0;

	bool isInitialized() { return _initialized; }
	
	bool getInitErrors(char *buff, int maxlen);

	virtual bool getGainValue(long *current, long *min, long *max) { return false; }
	virtual bool setGainValue(long val) { return false; }

	virtual bool getShutterValue(long *current, long *min, long *max) { return false; }
	virtual bool setShutterValue(long val) { return false; }

	virtual bool getWhiteBalanceUValue(long *current, long *min, long *max) { return false; }
	virtual bool setWhiteBalanceUValue(long val) { return false; }

	virtual bool getWhiteBalanceVValue(long *current, long *min, long *max) { return false; }
	virtual bool setWhiteBalanceVValue(long val) { return false; }

	virtual bool getBrightnessValue(long *current, long *min, long *max) { return false; }
	virtual bool setBrightnessValue(long val) { return false; }

	virtual bool getGammaValue(long *current, long *min, long *max) { return false; }
	virtual bool setGammaValue(long val) { return false; }

	virtual bool getHueValue(long *current, long *min, long *max) { return false; }
	virtual bool setHueValue(long val) { return false; }

	virtual bool getSaturationValue(long *current, long *min, long *max) { return false; }
	virtual bool setSaturationValue(long val) { return false; }

	virtual bool getSharpnessValue(long *current, long *min, long *max) { return false; }
	virtual bool setSharpnessValue(long val) { return false; }

	virtual bool getRedGainValue(long *current, long *min, long *max) { return false; }
	virtual bool setRedGainValue(long val) { return false; }

	virtual bool getGreenGainValue(long *current, long *min, long *max) { return false; }
	virtual bool setGreenGainValue(long val) { return false; }

	virtual bool getBlueGainValue(long *current, long *min, long *max) { return false; }
	virtual bool setBlueGainValue(long val) { return false; }

	virtual bool getColorGainValue(long *current, long *min, long *max) { return false; }
	virtual bool setColorGainValue(long val) { return false; }

	virtual bool canUserAdjust(int setting_index);

	void clearAllCacheValues();
	bool isValidCacheValue(int index);
	void invalidateCacheValue(int index);
	long getCacheValue(int index);
	void setCacheValue(int index, long value);

protected:
	int _engNum;
	int _camera_id;
	bool _initialized;
	char _logBuff[512];
	char _cameraInitErrorBuff[2048];
	bool _cameraInitErrors;
	
	bool _isColor;

private:
	long _cacheValue[NUM_CAMERA_PROPERTIES];
};
