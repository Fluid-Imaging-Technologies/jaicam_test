/*
 * $Id: CameraType.cpp,v 1.26 2010/06/03 12:46:54 peter Exp $
 *
 * The effectiveMaxFrameRates are just an estimate. Need to nail down some
 * true max frame rates via experimentation. Better to stay conservative.
 */

#include "CameraType.h"
#include "version.h"

CameraType gCameraDB[NUM_KNOWN_CAMERAS];

static bool camera_db_initialized = false;


/*
  =============================================================================
  =============================================================================
*/
void initializeCameraDB()
{
	if (!camera_db_initialized) {
		for (int i = 0; i < NUM_KNOWN_CAMERAS; i++) {
			gCameraDB[i].initialize(i);
		}
	
		// set the internal names
		strncpy_s(gCameraDB[CAMERA_TYPE_PROSILICA_EC1020]._internalName, 
					CAMERA_INTERNAL_NAME_SIZE, "PROSILICA_EC1020", _TRUNCATE);
				
		strncpy_s(gCameraDB[CAMERA_TYPE_PROSILICA_EC1020C]._internalName, 
					CAMERA_INTERNAL_NAME_SIZE, "PROSILICA_EC1020C", _TRUNCATE);

		strncpy_s(gCameraDB[CAMERA_TYPE_SONY_XCD_X710]._internalName, 
					CAMERA_INTERNAL_NAME_SIZE, "SONY_XCD_X710", _TRUNCATE);

		strncpy_s(gCameraDB[CAMERA_TYPE_SONY_DFW_X710]._internalName, 
					CAMERA_INTERNAL_NAME_SIZE, "SONY_DFW_X710", _TRUNCATE);

		strncpy_s(gCameraDB[CAMERA_TYPE_BLUEFOX_121G]._internalName, 
					CAMERA_INTERNAL_NAME_SIZE, "BLUEFOX_121G", _TRUNCATE);

		strncpy_s(gCameraDB[CAMERA_TYPE_BLUEFOX_121C]._internalName, 
					CAMERA_INTERNAL_NAME_SIZE, "BLUEFOX_121C", _TRUNCATE);

		strncpy_s(gCameraDB[CAMERA_TYPE_BLUEFOX_121C_BW]._internalName, 
					CAMERA_INTERNAL_NAME_SIZE, "BLUEFOX_121C_BW", _TRUNCATE);

		strncpy_s(gCameraDB[CAMERA_TYPE_SONY_XCD_SX90]._internalName, 
					CAMERA_INTERNAL_NAME_SIZE, "SONY_XCD_SC90", _TRUNCATE);

		strncpy_s(gCameraDB[CAMERA_TYPE_SONY_XCD_SX90CR]._internalName, 
					CAMERA_INTERNAL_NAME_SIZE, "SONY_XCD_SX90CR", _TRUNCATE);

		strncpy_s(gCameraDB[CAMERA_TYPE_JAI_CB080_GE]._internalName, 
					CAMERA_INTERNAL_NAME_SIZE, "JAI_CB080_GE", _TRUNCATE);

		camera_db_initialized = true;
	}
}

/*
  =============================================================================
  =============================================================================
*/
bool isBlueFoxCamera(int id)
{
	if (id >= 0 && id < NUM_KNOWN_CAMERAS) {
		if (CAMERA_DRIVER_MATRIX_USB == gCameraDB[id]._driver) {
			return true;
		}
	}

	return false;
}

/*
  =============================================================================
  =============================================================================
*/
CameraType::CameraType()
{
	_id = 0;
	_driver = CAMERA_DRIVER_MIL_FIREWIRE;
	_isColor = false;
	memset(_displayName, 0, sizeof(_displayName));
	memset(_milInitString, 0, sizeof(_milInitString));
	_defaultWidth = 1024;
	_defaultHeight = 768;
	_bayerPattern = BAYER_PATTERN_NONE;
	_maxFrameRate = 12;

	_can_auto_adjust_white_balance = false;
	_can_auto_adjust_intensity = true;

	for (int i = 0; i < NUM_CAMERA_CONTROLS; i++) {
		_can_adjust[i] = false;
		_hardware_min[i] = 0;
		_hardware_max[i] = 0;
		_hardware_default[i] = 0;
		_can_use_for_auto_intensity[i] = false;
	}

	_defaultBayerCoefficients[0] = 1.0;
	_defaultBayerCoefficients[1] = 1.0;
	_defaultBayerCoefficients[2] = 1.0;

	_can_use_fpga_for_trigger_and_flash = false;

	initialize(CAMERA_TYPE_JAI_CB080_GE);
}

/*
  =============================================================================
  =============================================================================
*/
void CameraType::setParameters(int index, bool can_adjust, int auto_intensity_adjustment_order,
							   long hw_min, long hw_max, long hw_default)
{
	if (index >= 0 || index < NUM_CAMERA_CONTROLS) {
		_can_adjust[index] = can_adjust;		
		_can_use_for_auto_intensity[index] = auto_intensity_adjustment_order > 0;
		_auto_intensity_adjustment_order[index] = auto_intensity_adjustment_order;
		_hardware_min[index] = hw_min;
		_hardware_max[index] = hw_max;
		_hardware_default[index] = hw_default;
	}
}

/*
  =============================================================================
  =============================================================================
*/
void CameraType::setFlashAdjustmentOrder(int order)
{
	_auto_intensity_adjustment_order_flash = order;
}

/*
  =============================================================================
  =============================================================================
*/
void CameraType::initialize(int id)
{
	switch (id) 
	{
	case CAMERA_TYPE_PROSILICA_EC1020:		
		initialize(CAMERA_DRIVER_MIL_FIREWIRE,
					false,
					"Prosilica EC1020 (bw)",
					"M_1024X768_Y_FORMAT_7_0",
					1024,
					768,
					BAYER_PATTERN_NONE,
					12);

		setParameters(CAMERA_SHUTTER, true, 0, 1, 4095, 1);
		setParameters(CAMERA_GAIN, true, 2, 0, 22, 10);
		setFlashAdjustmentOrder(1);		
		break;

	case CAMERA_TYPE_PROSILICA_EC1020C:
		initialize(CAMERA_DRIVER_MIL_FIREWIRE,
					true,
					"Prosilica EC1020C (color)",
					"M_1024X768_RGB_FORMAT_7_0",
					1024,
					768,
					BAYER_PATTERN_NONE,
					12);
		
		setParameters(CAMERA_SHUTTER, true, 0, 1, 4095, 1);
		setParameters(CAMERA_GAIN, true, 2, 0, 22, 10);
		setParameters(CAMERA_WHITE_BALANCE_U, true, 0, 0, 255, 60);
		setParameters(CAMERA_WHITE_BALANCE_V, true, 0, 0, 255, 60);
		// do we want to keep these?
		//setParameters(CAMERA_BRIGHTNESS, true, false, 0, 255);
		//setParameters(CAMERA_GAMMA, true, false, 0, 1);
		setFlashAdjustmentOrder(1);
		break;

	case CAMERA_TYPE_SONY_XCD_X710:
		initialize(CAMERA_DRIVER_MIL_FIREWIRE,
					false,
					"Sony XCD-X710 (bw)",
					"M_1024X768_Y@15",
					1024,
					768,
					BAYER_PATTERN_NONE,
					12);

		setParameters(CAMERA_SHUTTER, true, 2, 3, 1150, 129);
		setParameters(CAMERA_GAIN, true, 3, 384, 1023, 400);
		setFlashAdjustmentOrder(1);	
		break;

	case CAMERA_TYPE_SONY_DFW_X710:
		initialize(CAMERA_DRIVER_MIL_FIREWIRE,
					true,
					"Sony DFW-X710 (color)",
					"M_1024X768_YUV422@15",
					1024,
					768,
					BAYER_PATTERN_NONE,
					12);

		setParameters(CAMERA_SHUTTER, true, 2, 3, 1150, 45);
		setParameters(CAMERA_GAIN, true, 3, 70, 551, 100);
		setParameters(CAMERA_WHITE_BALANCE_U, true, 0, 1792, 2304, 2000);
		setParameters(CAMERA_WHITE_BALANCE_V, true, 0, 1792, 2304, 2000);
		setParameters(CAMERA_BRIGHTNESS, true, 4, 0, 127, 0);
		setParameters(CAMERA_GAMMA, true, 0, 128, 130, 129);
		setParameters(CAMERA_HUE, true, 0, 83, 173, 100);
		setParameters(CAMERA_SATURATION, true, 0, 0, 511, 50);
		setParameters(CAMERA_SHARPNESS, true, 0, 0, 7, 0);
		setFlashAdjustmentOrder(1);	
		break;

	case CAMERA_TYPE_BLUEFOX_121G:
		initialize(CAMERA_DRIVER_MATRIX_USB,
					false,
					"mvBlueFOX 121G/221G (bw)",
					NULL,
					1024,
					768,
					BAYER_PATTERN_NONE,
					20);


#if defined (CAMERA_VERSION_INTERNAL) || defined (FCDIAGNOSTICS)
		_can_use_fpga_for_trigger_and_flash = true;
		setParameters(CAMERA_SHUTTER, true, 2, 72, 20000, 100);
#else
		setParameters(CAMERA_SHUTTER, true, 2, 72, 500, 100);
#endif

		setParameters(CAMERA_GAIN, true, 3, 0, 2997, 200);
		setFlashAdjustmentOrder(1);	
		break;

	case CAMERA_TYPE_BLUEFOX_121C:
		initialize(CAMERA_DRIVER_MATRIX_USB,
					true,
					"mvBlueFOX 121C/221C (color)",
					NULL,
					1024,
					768,
					BAYER_PATTERN_NONE,
					20);

#if 0
		// leave false for now, it is done without user control
		_can_auto_adjust_white_balance = true;
#endif

#if defined (CAMERA_VERSION_INTERNAL) || defined (FCDIAGNOSTICS)
		_can_use_fpga_for_trigger_and_flash = true;
		setParameters(CAMERA_SHUTTER, true, 2, 72, 20000, 100);
#else
		setParameters(CAMERA_SHUTTER, true, 2, 72, 500, 100);
#endif

		setParameters(CAMERA_GAIN, true, 3, 0, 2997, 100);
		setParameters(CAMERA_SHARPNESS, true, 0, 0, 1, 0);

#if 0
		setParameters(CAMERA_RED_GAIN, true, 0, 20, 500, 100);
		setParameters(CAMERA_GREEN_GAIN, true, 0, 20, 500, 100);
		setParameters(CAMERA_BLUE_GAIN, true, 0, 20, 500, 100);
		setParameters(CAMERA_COLOR_GAIN, true, 0, 20, 500, 100);
#endif
		setFlashAdjustmentOrder(1);	
		break;

	case CAMERA_TYPE_BLUEFOX_121C_BW:
		initialize(CAMERA_DRIVER_MATRIX_USB,
					false,
					"mvBlueFOX 121C/221C (used as bw)",
					NULL,
					1024,
					768,
					BAYER_PATTERN_NONE,
					20);

#if defined (CAMERA_VERSION_INTERNAL) || defined (FCDIAGNOSTICS)
		_can_use_fpga_for_trigger_and_flash = true;
		setParameters(CAMERA_SHUTTER, true, 2, 72, 20000, 100);
#else
		setParameters(CAMERA_SHUTTER, true, 2, 72, 500, 100);
#endif
		setParameters(CAMERA_GAIN, true, 3, 0, 2997, 100);
		setFlashAdjustmentOrder(1);	
		break;

	case CAMERA_TYPE_SONY_XCD_SX90:
		initialize(CAMERA_DRIVER_MIL_FIREWIRE,
					false,
					"Sony XCD-SX90 (bw)",
					"M_1280X960_Y_FORMAT_7_0",
					1280,
					960,
					BAYER_PATTERN_NONE,
#if defined (CAMERA_VERSION_INTERNAL) || defined (FCDIAGNOSTICS)
					30);
#else
					22);
#endif


#if defined (CAMERA_VERSION_INTERNAL) || defined (FCDIAGNOSTICS)
		setParameters(CAMERA_SHUTTER, true, 2, 3, 40, 8);
#else
		setParameters(CAMERA_SHUTTER, true, 2, 6, 16, 8);
#endif

		setParameters(CAMERA_GAIN, true, 3, 0, 680, 0);
		setParameters(CAMERA_WHITE_BALANCE_U, false, 0, 1792, 2559, 2150);
		setParameters(CAMERA_WHITE_BALANCE_V, false, 0, 1792, 2559, 2150);
		setParameters(CAMERA_BRIGHTNESS, true, 4, 0, 2047, 0);
		setParameters(CAMERA_GAMMA, true, 0, 0, 3, 0);
		setParameters(CAMERA_HUE, false, 0, 1792, 2559, 2290);
		setParameters(CAMERA_SATURATION, false, 0, 64, 511, 300);
		setParameters(CAMERA_SHARPNESS, true, 0, 0, 7, 0);
		setFlashAdjustmentOrder(1);	
		break;

	case CAMERA_TYPE_SONY_XCD_SX90CR:
		initialize(CAMERA_DRIVER_MIL_FIREWIRE,
					true,
					"Sony XCD-SX90CR (color)",
					"M_1280X960_RAW8_FORMAT_7_0",
					1280,
					960,
					BAYER_PATTERN_GB,
#if defined (CAMERA_VERSION_INTERNAL) || defined (FCDIAGNOSTICS)
					30);
#else
					20);
#endif
	
#if defined (CAMERA_VERSION_INTERNAL) || defined (FCDIAGNOSTICS)
		setParameters(CAMERA_SHUTTER, true, 2, 3, 40, 9);
#else
		setParameters(CAMERA_SHUTTER, true, 2, 6, 16, 9);
#endif

		setParameters(CAMERA_GAIN, true, 3, 0, 511, 0);
		setParameters(CAMERA_WHITE_BALANCE_U, true, 0, 1792, 2559, 2150);
		setParameters(CAMERA_WHITE_BALANCE_V, true, 0, 1792, 2559, 2250);
		setParameters(CAMERA_BRIGHTNESS, true, 4, 0, 2047, 0);
		setParameters(CAMERA_GAMMA, true, 0, 0, 3, 0);
		setParameters(CAMERA_HUE, true, 0, 1792, 2559, 2290);
		setParameters(CAMERA_SATURATION, true, 0, 64, 511, 300);
		_defaultBayerCoefficients[0] = 0.9799;
		_defaultBayerCoefficients[1] = 0.9932;
		_defaultBayerCoefficients[2] = 1.0210;
		setFlashAdjustmentOrder(1);	
		break;

	case CAMERA_TYPE_JAI_CB080_GE:
		initialize(CAMERA_DRIVER_JAI_GIGE,
					true,
					"JAI CB-080 GE (color)",
					NULL,
					1032,
					778,
					BAYER_PATTERN_RG,
					30);
				
		setParameters(CAMERA_SHUTTER, true, 0, 1, 30, 5);
		setParameters(CAMERA_GAIN, true, 0, 1, 5000, 300);
		break;

#if 0
	case CAMERA_TYPE_POINT_GREY_FL2_08S2C:
		initialize(CAMERA_DRIVER_MIL_FIREWIRE,
					true,
					"Point Grey FL2-08S2C (color)",
					"M_1024X768_YUV422@15",
					1024,
					768,
					BAYER_PATTERN_NONE,
					30,
					16);

		initAdjustments(true, true, true, true, true, false, false, true, true, false, false, false, false, false, false, false, false, false);
		break;
#endif

	default:
		return;
	}

	_id = id;
}

/*
  =============================================================================
  =============================================================================
*/
void CameraType::initialize(int driver, bool color, const char *name, 
							const char *init, int width, 
							int height, int bayer, int frameRate)
{
	if (driver == CAMERA_DRIVER_MIL_FIREWIRE || driver == CAMERA_DRIVER_MATRIX_USB) {
		_driver = driver;
	}

	_isColor = color;
	
	if (name && *name) {
		strncpy_s(_displayName, sizeof(_displayName), name, _TRUNCATE);
	}

	if (init && *init) {
		strncpy_s(_milInitString, sizeof(_milInitString), init, _TRUNCATE);
	}

	if (width > 0 && width <= 4096) {
		_defaultWidth = width;
	}

	if (height > 0 && height <= 4096) {
		_defaultHeight = height;
	}

	if (bayer >= BAYER_PATTERN_NONE && bayer <= BAYER_PATTERN_RG) {
		_bayerPattern = bayer;
	}

	if (frameRate > 0 && frameRate < 32) {
		_maxFrameRate = frameRate;
	}
}

/*
  =============================================================================
  =============================================================================
*/

int DefaultCameraId()
{
	for (int i = 0; i < NUM_KNOWN_CAMERAS; i++) {
		if (!strcmp(gCameraDB[i]._internalName, DEFAULT_CAMERA)) {
			return i;
		}
	}
	return CAMERA_TYPE_JAI_CB080_GE;
}
