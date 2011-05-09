/*
 * JAI CB-080 GE camera class for VS
 *
 */

#pragma once

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

	int _imageCount;

private:
	bool applyNonConfigurableSettings();
	void imageStreamCallback(J_tIMAGE_INFO * pAqImageInfo);

	// for testing
	bool openViewWindow();

	CRITICAL_SECTION _csCopyImage;
	FACTORY_HANDLE _hFactory;
	char _jCameraId[J_CAMERA_ID_SIZE];
	CAM_HANDLE _hCamera;
	THRD_HANDLE _hStreamThread;
	VIEW_HANDLE _hView;
	bool _imageReady;
};
