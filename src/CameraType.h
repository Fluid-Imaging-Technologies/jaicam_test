/*
 * Stub of VS CameraType.h
 *
 */

#pragma once

#include "CameraConstants.h"

#define CAMERA_TYPE_PROSILICA_EC1020		0
#define CAMERA_TYPE_PROSILICA_EC1020C		1
#define CAMERA_TYPE_SONY_XCD_X710			2
#define CAMERA_TYPE_SONY_DFW_X710			3
#define CAMERA_TYPE_BLUEFOX_121G			4
#define CAMERA_TYPE_BLUEFOX_121C			5
#define CAMERA_TYPE_BLUEFOX_121C_BW			6
#define CAMERA_TYPE_SONY_XCD_SX90			7
#define CAMERA_TYPE_SONY_XCD_SX90CR			8
#define CAMERA_TYPE_JAI_CB080_GE			9
#define NUM_KNOWN_CAMERAS					10

#if 0
#define CAMERA_TYPE_POINT_GREY_FL2_08S2C	9
#endif

#define DEFAULT_CAMERA		"JAI_CB080_GE"

#define BAYER_PATTERN_NONE	0
#define BAYER_PATTERN_BG	1
#define BAYER_PATTERN_GB	2
#define BAYER_PATTERN_GR	3
#define BAYER_PATTERN_RG	4

#define CAMERA_DRIVER_MIL_FIREWIRE		0
#define CAMERA_DRIVER_MATRIX_USB		1
#define CAMERA_DRIVER_JAI_GIGE			2

#define CAMERA_INTERNAL_NAME_SIZE 32

class CameraType
{
public:
	CameraType();

	void initialize(int id);
	
	int _id;
	char _internalName[CAMERA_INTERNAL_NAME_SIZE];
	int _driver;
	bool _isColor;	
	char _displayName[64];
	char _milInitString[64];
	int _defaultWidth;
	int _defaultHeight;
	int _bayerPattern;
	double _defaultBayerCoefficients[3];
	int _maxFrameRate;

	bool _can_adjust[NUM_CAMERA_CONTROLS];
	long _hardware_min[NUM_CAMERA_CONTROLS];
	long _hardware_max[NUM_CAMERA_CONTROLS];
	long _hardware_default[NUM_CAMERA_CONTROLS];
	bool _can_use_for_auto_intensity[NUM_CAMERA_CONTROLS];
	int _auto_intensity_adjustment_order[NUM_CAMERA_CONTROLS];

	bool _can_auto_adjust_white_balance;
	bool _can_auto_adjust_intensity;
	int _auto_intensity_adjustment_order_flash;

	bool _can_use_fpga_for_trigger_and_flash;

private:
	void initialize(int driver, bool color, const char *displayName, const char *init, 
					int width, int height, int bayer, int frameRate); 
					
	void setParameters(int index, bool can_adjust, int auto_intensity_adjustment_order, 
						long hw_min, long hw_max, long hw_default);
	void setFlashAdjustmentOrder(int order);
};

void initializeCameraDB();
bool isBlueFoxCamera(int id);
int DefaultCameraId();

extern CameraType gCameraDB[NUM_KNOWN_CAMERAS];
