#include "windows.h"
#include "resource.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];								// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];								// The title bar text

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

//dialog callback procedure declaration
INT_PTR CALLBACK DialogProc(
  HWND hwndDlg,  
  UINT uMsg,     
  WPARAM wParam, 
  LPARAM lParam  
);

//function to create region based on the bitmap
void createRegion(HWND);
//function to paint the region window
void paintRegion();

//global variables
//handle to region
HRGN hRgn = NULL;
//dest and memory DCs
HDC hdcDest, hdcMem;
//buffer to hold bitmap data
BITMAP bmpInfo;

int APIENTRY WinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPSTR     lpCmdLine,
					 int       nCmdShow)
{
	MSG msg;

	strcpy(szWindowClass,"mywindowclass");
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	//message loop
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL;

	return RegisterClassEx(&wcex);
}

//handle to main application window
HWND ghWnd = NULL;

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int iRet = 0;

	hInst = hInstance; // Store instance handle in our global variable

	ghWnd = CreateWindow(
		szWindowClass, 
		szTitle, 
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 
		0, 
		100, 
		150, 
		NULL, 
		NULL, 
		hInstance, 
		NULL);

	if (!ghWnd)
	{
		return FALSE;
	}

	//create dialog box
	HWND hwndDlg = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), ghWnd, (DLGPROC)DialogProc);
	//create region
	createRegion(hwndDlg);

	//dont have to show or update the window
	//ShowWindow(hWnd, nCmdShow);
	//UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
	case WM_DESTROY:
		DeleteObject(hRgn);
		//Delete the memory DC
		DeleteDC(hdcMem);
		//delete the dest DC
		DeleteDC(hdcDest);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

//global variable for moving region using mouse
BOOL mouseState = 0;
POINT CursPt = {0};

//dialog callback procedure
INT_PTR CALLBACK DialogProc(
  HWND hwnd,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
{
	int ix = 0, iy = 0;

	switch (uMsg)
	{
	case WM_PAINT:
		paintRegion();
		ValidateRect(hwnd,FALSE);
		return TRUE;
	case WM_MOVE:
		paintRegion();
		return TRUE;
	case WM_LBUTTONDOWN:
		mouseState = 1;
		return TRUE;
	case WM_MOUSEMOVE:
		if(mouseState)
		{
			GetCursorPos(&CursPt);
			PostMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM( CursPt.x, CursPt.y));
			mouseState = 0;
		}
		return TRUE;
	case WM_RBUTTONDOWN:
		//destroy the dialog box
		EndDialog(hwnd,0);
		//send message to destroy the main application window and exit
		SendMessage(ghWnd,WM_DESTROY,NULL,NULL);
		return TRUE;
	}
	return FALSE;
}

void createRegion(HWND hwndDlg)
{
	//get the destination device context
	hdcDest = GetDC(hwndDlg);

	//create a memory DC
	hdcMem = CreateCompatibleDC(NULL);

	//image file name
	char imageFile[] = "a.bmp";
	// Load the image
	HANDLE hBitmap = (HBITMAP)LoadImage(GetModuleHandle(NULL), imageFile, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if(!hBitmap)
	{
		//handle error here
		return;
	}

	//Get information about the bitmap..
	GetObject(hBitmap, sizeof(bmpInfo), &bmpInfo);	// Get info about the bitmap 

	//Select the bitmap into the dc
	HGDIOBJ hGdiObj = SelectObject(hdcMem, hBitmap);

	//create an empty region
	hRgn = CreateRectRgn(0,0,0,0);

	//Create a region from a bitmap with transparency colour of white
	//change the pixel values for a different transparency color
	//ex - RGB(0,0,0) will mean a transparency color of black.. so the areas
	//of the bitmap not used to create the window will be black
	COLORREF crTransparent = RGB(255, 255, 255);

	int iX = 0;
	int iRet = 0;
	int iY;
	for (iY = 0; iY < bmpInfo.bmHeight; iY++)
	{
		do
		{
			//skip over transparent pixels at start of lines.
			while (iX < bmpInfo.bmWidth && GetPixel(hdcMem, iX, iY) == crTransparent)
				iX++;
			//remember this pixel
			int iLeftX = iX;
			//now find first non transparent pixel
			while (iX < bmpInfo.bmWidth && GetPixel(hdcMem, iX, iY) != crTransparent)
				++iX;
			//create a temp region on this info
			HRGN hRgnTemp = CreateRectRgn(iLeftX, iY, iX, iY+1);
			//combine into main region.
			iRet = CombineRgn(hRgn, hRgn, hRgnTemp, RGN_OR);
			if(iRet == ERROR)
			{
				return;
			}
			//delete the temp region for next pass
			DeleteObject(hRgnTemp);
		}while(iX < bmpInfo.bmWidth);
		iX = 0;
	}

	//Centre it on current desktop
	iRet = SetWindowRgn(hwndDlg, hRgn, TRUE);
	if(!iRet)
	{
		return;
	}

	iX = (GetSystemMetrics(SM_CXSCREEN)) / 2 - (bmpInfo.bmWidth / 2);
	iY = (GetSystemMetrics(SM_CYSCREEN)) / 2 - (bmpInfo.bmHeight / 2);
	iRet = SetWindowPos(hwndDlg, HWND_TOPMOST, iX, iY, bmpInfo.bmWidth, bmpInfo.bmHeight, NULL);

	//Copy the memory dc into the screen dc
	paintRegion();

	//delete the bitmap
	DeleteObject(hBitmap);
}

void paintRegion()
{
	//transfer color data from the source device context to the destination device context
	BitBlt(hdcDest, 0, 0, bmpInfo.bmWidth, bmpInfo.bmHeight, hdcMem, 0, 0, SRCCOPY);
}