/*
 * JAI CB-080 GE camera class for VS
 *
 */

#pragma once

#include "StringList.h"
#include "BaseCamera.h"

#define JAI_SDK_DYNAMIC_LOAD 1
#include <jai_factory.h>


class JaiGigECamera : public BaseCamera
{
public:
	JaiGigECamera(int engNum);
	virtual ~JaiGigECamera();

	virtual bool initialize(Context *ctx, HANDLE hStopEvent);
	virtual bool freeCamera();

	virtual bool startImageGrabThread();
	virtual bool stopImageGrabThread();
	virtual bool fetchImage(MIL_ID milImageBuff);

	virtual bool applySettings(Context *ctx);

	virtual bool getGainValue(long *current, long *min, long *max);
	virtual bool setGainValue(long val);

	virtual bool getShutterValue(long *current, long *min, long *max);
	virtual bool setShutterValue(long val);

	StringList* getFeatureList(HWND hWnd);

	int _imageCount;

private:
	bool applyNonConfigurableSettings();
	void imageStreamCallback(J_tIMAGE_INFO * pAqImageInfo);
	void imageStreamCallback2(J_tIMAGE_INFO * pAqImageInfo);
	uint32_t convertBayerGainToJaiFormat(double bayerGain);

	// for testing
	bool initializeMILWindow();
	MIL_ID _milImageBuf;
	MIL_ID _milDisplay;

	CRITICAL_SECTION _csCopyImage;
	FACTORY_HANDLE _hFactory;
	char _jCameraId[J_CAMERA_ID_SIZE];
	CAM_HANDLE _hCamera;
	THRD_HANDLE _hStreamThread;
	bool _imageReady;
	uint32_t _bayerGain[3];
	J_tIMAGE_INFO _rgbBuff;
	NODE_HANDLE _hGainNode;
    NODE_HANDLE _hExposureNode;  
};
