/*
  Testing the JAI GigE camera with MIL9
*/

#include "version.h"
#include <stdio.h>

#include "Context.h"
#include "JaiGigECamera.h"
#include "MilVars.h"

#include "resource.h"

bool InitApplication(HINSTANCE hInstance, int cmdShow);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void wmCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
void allocateCamera(HWND hWnd);
void freeCamera(HWND hWnd);
void initializeCamera(HWND hWnd);
void streamOn(HWND hWnd);
void streamOff(HWND hWnd);

HINSTANCE ghInstance;
char szMainWinClass[] = "jai_test_winclass";
Context ctx;
JaiGigECamera *cam;
HANDLE hStopEvent;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, 
					int cmdShow)
{
	MSG msg;

	memset(&msg, 0, sizeof(MSG));

	if (!InitApplication(hInstance, cmdShow))
		return 0;
	

	while (GetMessage(&msg, (HWND) NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (cam)
		delete cam;
		
	return msg.wParam;
}

bool InitApplication(HINSTANCE hInstance, int cmdShow)
{
	WNDCLASS wc;
	HWND hWnd;

	memset(&wc, 0, sizeof(wc));
				
	wc.lpfnWndProc = MainWndProc;																
	wc.hInstance = ghInstance; 						
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);			
	wc.lpszClassName = (LPCSTR) szMainWinClass;		

//	InitCommonControls();	   
	
	if (!RegisterClass(&wc))
		return false;

	hWnd = CreateWindow((LPCSTR)szMainWinClass,
						"JAI Camera Test",
						WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
						CW_USEDEFAULT, CW_USEDEFAULT, 400, 200,
						NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return false;

	ShowWindow(hWnd, cmdShow);
	UpdateWindow(hWnd);

	return true;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
/*
	case WM_PAINT:
		wmPaint(hWnd);
		break;
*/	
	case WM_COMMAND:
		wmCommand(hWnd, wParam, lParam);
		break;			  

	case WM_CLOSE:
		freeCamera(hWnd);
		DestroyWindow(hWnd);
		break;
/*
	case WM_CREATE:
		return wmCreate(hWnd, wParam, lParam);		
*/
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
		break;
	}						   

	return 0;
}

void wmCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam)) {
	case ID_CAMERA_INITIALIZE:
		initializeCamera(hWnd);
		break;

	case ID_CAMERA_FREE:
		freeCamera(hWnd);
		break;

	case ID_CAMERA_STREAMON:
		streamOn(hWnd);
		break;

	case ID_CAMERA_STREAMOFF:
		streamOff(hWnd);
		break;

	case ID_FILE_EXIT:
		PostMessage(hWnd, WM_CLOSE, 0, 0);
		break;

	default:
		break;
	}
}

void initializeCamera(HWND hWnd)
{
	if (cam)
		return;

	hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!hStopEvent) {
		MessageBox(hWnd, "CreateEvent failed", "Error", MB_OK);
		return;
	}

	cam = new JaiGigECamera(0);
	if (!cam) {
		MessageBox(hWnd, "Allocate new camera failed", "Error", MB_OK);
		return;
	}
	
	if (!cam->initialize(&ctx, hStopEvent)) {
		MessageBox(hWnd, "Camera initialization failed", "Error", MB_OK);
		return;
	}

	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_INITIALIZE, MF_GRAYED);
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_FREE, MF_ENABLED); 
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_STREAMON, MF_ENABLED);
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_STREAMOFF, MF_GRAYED);
}

void freeCamera(HWND hWnd)
{
	if (cam) {
		delete cam;
		cam = NULL;
	}

	if (hStopEvent) {
		CloseHandle(hStopEvent);
		hStopEvent = 0;
	}

	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_INITIALIZE, MF_ENABLED);
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_FREE, MF_GRAYED); 
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_STREAMON, MF_GRAYED);
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_STREAMOFF, MF_GRAYED);
}

void streamOn(HWND hWnd)
{
	if (!cam)
		return;

	if (!cam->startImageGrabThread()) {
		MessageBox(hWnd, "startImageGrabThread() failed", "Error", MB_OK);
		return;
	}

	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_STREAMON, MF_GRAYED);
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_STREAMOFF, MF_ENABLED);
}

void streamOff(HWND hWnd)
{
	char buff[64];

	if (!cam)
		return;

	if (!cam->stopImageGrabThread()) {
		MessageBox(hWnd, "stopImageGrabThread() failed", "Error", MB_OK);
		return;
	}

	sprintf_s(buff, sizeof(buff), "Received %d images", cam->_imageCount);
	MessageBox(hWnd, buff, "Stream Off", MB_OK);

	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_STREAMON, MF_ENABLED);
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_STREAMOFF, MF_GRAYED);
}