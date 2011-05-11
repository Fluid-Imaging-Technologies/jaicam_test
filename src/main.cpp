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
void dumpFeatureList(HWND hWnd);

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

	if (!gMilVars.getApplicationID())
		return 0;

	if (!InitApplication(hInstance, cmdShow))
		return 0;
	
	while (GetMessage(&msg, (HWND) NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (cam)
		delete cam;
	
	gMilVars.releaseSystemID();

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
						CW_USEDEFAULT, CW_USEDEFAULT, 400, 100,
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

	case ID_CAMERA_FEATURELIST:
		dumpFeatureList(hWnd);
		break;

	case ID_FILE_EXIT:
		PostMessage(hWnd, WM_CLOSE, 0, 0);
		break;

	default:
		break;
	}
}

void dumpFeatureList(HWND hWnd)
{
	int size, i;
	const char *s;
	char *buff;
	HANDLE fh;
	unsigned long byteswritten;

	StringList *list = cam->getFeatureList(hWnd);

	if (!list)
		return;

	size = 64;

	for (i = 0; i < list->getNumStrings(); i++) {
		s = list->getString(i);

		if (s)
			size += 4 + strlen(s);
	}

	buff = new char[size];

	if (!buff) {
		delete list;
		return;
	}

	memset(buff, 0, size);

	for (i = 0; i < list->getNumStrings(); i++) {
		s = list->getString(i);

		if (s && strlen(s) > 0) {
			strcat(buff, s);
			strcat(buff, "\r\n");
		}
	}
	
	fh = CreateFile("./featurelist.txt",
					GENERIC_WRITE,
					NULL,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if (fh) {
		WriteFile(fh, buff, strlen(buff), &byteswritten, NULL);
		CloseHandle(fh);
	}
	
	delete [] buff;
	delete list;
}

void initializeCamera(HWND hWnd)
{
	char *buff;

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
		buff = new char[2048];

		if (buff) {
			memset(buff, 0, 2048);
			cam->getInitErrors(buff, 2048);
			MessageBox(hWnd, buff, "Camera Init Fail", MB_OK);
			delete [] buff;
		}
		else {
			MessageBox(hWnd, "NOMEM", "Camera Init Fail", MB_OK);
		}

		return;
	}

	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_INITIALIZE, MF_GRAYED);
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_FREE, MF_ENABLED); 
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_STREAMON, MF_ENABLED);
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_STREAMOFF, MF_GRAYED);
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_FEATURELIST, MF_ENABLED);
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
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_FEATURELIST, MF_GRAYED);
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
	//char buff[64];

	if (!cam)
		return;

	if (!cam->stopImageGrabThread()) {
		MessageBox(hWnd, "stopImageGrabThread() failed", "Error", MB_OK);
		return;
	}

	//sprintf_s(buff, sizeof(buff), "Received %d images", cam->_imageCount);
	//MessageBox(hWnd, buff, "Stream Off", MB_OK);

	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_STREAMON, MF_ENABLED);
	EnableMenuItem(GetMenu(hWnd), ID_CAMERA_STREAMOFF, MF_GRAYED);
}