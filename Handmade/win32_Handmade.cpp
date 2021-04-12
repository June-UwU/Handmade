#include <Windows.h>
#include <stdint.h>

//testing new convention for static keyword
#define internal static
#define local_persist static
#define global_persist static
/*********************************************************************************************************/
                                          /*GLOBAL_VARIABLES*/
/*********************************************************************************************************/
global_persist bool running;
/****************************/
     /*CLEAN_UP_NEEDED*/
/****************************/
global_persist void* PointBITMAPMemory;
global_persist BITMAPINFO BitMapInfo;
global_persist int BitMapWidth;
global_persist int BitMapHeight;
/*********************************************************************************************************/
/*********************************************************************************************************/

//function to create a bitmap memory if not initialized and to update the old one with new
internal void Win32_ResizeDIBSection(int width ,int height)
{
	if (PointBITMAPMemory)
	{
		VirtualFree(PointBITMAPMemory, 0, MEM_RELEASE);
		//Vitual Protect can be used here to catch any stale pointer to find 'use after free bug' and flag it down
	}
	BitMapHeight = height;
	BitMapWidth = width;
	BitMapInfo.bmiHeader.biSize = sizeof(BitMapInfo.bmiHeader);//gives the size of bmiheader only since it is required to traverse to the palette color
	BitMapInfo.bmiHeader.biWidth = BitMapWidth;
	BitMapInfo.bmiHeader.biHeight = -BitMapHeight;
	//negative to symbolize a top down bitmap
	BitMapInfo.bmiHeader.biPlanes = 1;
	BitMapInfo.bmiHeader.biBitCount = 32;//asked 32 for DWORD alignment which is due for explanation,4-byte bit alignment
	BitMapInfo.bmiHeader.biCompression = BI_RGB;
	BitMapInfo.bmiHeader.biSizeImage = 0;
	BitMapInfo.bmiHeader.biXPelsPerMeter = 0;
	BitMapInfo.bmiHeader.biClrUsed = 0;
	BitMapInfo.bmiHeader.biClrImportant = 0;
	//allocating the buffer ourselves
	int BytePerPixel = 4;
	int BitMapMemorySizeBytes = 4 * BitMapWidth * BitMapHeight;
	PointBITMAPMemory = VirtualAlloc(NULL, BitMapMemorySizeBytes, MEM_COMMIT, PAGE_READWRITE);
	//Two Modes:MEE_COMMIT,MEM_RESERVE commit allocates the memory for current use, 
	//reserve allocates but doesn't use the memory range
	//linear rasterization
	uint8_t* Row = (uint8_t*)PointBITMAPMemory;
	int Pitch = width * BytePerPixel;
	for (int Y = 0; Y < BitMapHeight; Y++)
	{

		uint8_t* Pixel = (uint8_t*)Row;
		for (int X = 0; X < BitMapWidth; X++) 
		{
			/*                 1  2  3  4    [ByteOrder]
			* Pixel in memory: 00 00 00 00
			* LITTLE ENDIAN ARCHITECTURE EFFECT
			* Pixel in memory: BB GG RR xx
			* 0xxxBBGGRR on 32 bit pixel buffer
			*/

			/*WELCOME TO 1980*/
			//blue channel
			*Pixel = (uint8_t)X;
			++Pixel;

			//green channel
			*Pixel = (uint8_t)Y;
			++Pixel;

			//red channel
			*Pixel = (uint8_t)X;
			++Pixel;

			//alignment padding
			*Pixel = 0;
			++Pixel;
		}
		Row += Pitch;
	}
}

// actual Drawing function takes a bitmap
internal void Win32_UpdateWindows(HDC Context, RECT*WindowRECT,int X, int Y, int Width, int Height)
{
	int Windowwidth = WindowRECT->right - WindowRECT->left;
	int WindowHeight = WindowRECT->bottom - WindowRECT->top;
	// StretchDIBits function copies the color data for a rectangle of pixels in a DIB, JPEG, or PNG image to the specified destination rectangle
	//shouldn't the destinations be the window ..?
	StretchDIBits(Context, 0, 0, Windowwidth, WindowHeight
		, 0, 0, BitMapWidth, BitMapHeight
		,PointBITMAPMemory, &BitMapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK WindowProc(
	HWND handle,
	UINT Message,
	WPARAM wParam,
	LPARAM lParam
)
{
	LRESULT Result = 0;
	switch (Message)
	{
	case WM_SIZE:
	{
		//Gets the Client RECT Area and preps it for drawing from the bitmap
		RECT ClientRECT;
		GetClientRect(handle, &ClientRECT);
		int height = ClientRECT.bottom - ClientRECT.top;
		int width = ClientRECT.right - ClientRECT.left;
		Win32_ResizeDIBSection(width, height);
	}break;
	case WM_PAINT://the "start painting" message
	{
		PAINTSTRUCT Paint;//paint struct that gets set by windows to get useful data back
		HDC DeviceContext = BeginPaint(handle, &Paint);//notifies the beginning of paint,Also returns a device context(takes a long pointer to PAINTSTRUCT)
		int X = Paint.rcPaint.left;
		int Y = Paint.rcPaint.top;
		int Width = Paint.rcPaint.right - Paint.rcPaint.left;
		int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
		RECT ClientRECT;
		GetClientRect(handle, &ClientRECT);
		Win32_UpdateWindows(DeviceContext , &ClientRECT,X , Y, Width, Height);
		EndPaint(handle, &Paint);//notifies the end of painting
	}break;
	//WM_QUIT handles the actual exit conditions
	case WM_QUIT:
	{
		PostQuitMessage(0);
	}break;
	//WM_CLOSE handles the close  button
	case WM_CLOSE:
	{
		Result = 0;
		running = false;//making the loop condition false
	}break;
	default:
	{
		Result = DefWindowProc(handle, Message, wParam, lParam); //Default window proc
	}
	}
	return (Result);
}
int WINAPI wWinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PWSTR cmdline, int ShowCode)
{
	//UINT windows define : typedef unsigned int UINT
	WNDCLASS wc = {}; //{} equals zero
	wc.style = CS_OWNDC|CS_HREDRAW;//CS_OWNDC creates a device context for every window generated
	wc.lpfnWndProc =WindowProc ; //windows message procedure , normally uses DefWndProcc
	wc.hInstance = Instance; //current instance of the window
	//wc.hIcon = ; no Icon for now
	//wc.hCursor = ; no cursor
	//wc.hbrBackground = ; not setting background
	//wc.lpszMenuName = ; no classic window menu
	wc.lpszClassName = L"HandmadeWindowClass";// window class name to identify the class and create the window using the class we created


	RegisterClass(&wc);//registering the window class

	HWND handle = CreateWindowEx(0, wc.lpszClassName, L"Handmade", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0
		, 0, Instance, 0); //creating the actual windows..managed through handles
	if (handle)
	{
		running = true;
		while(running)
		{
			MSG message;
			BOOL MSGResult;//typedef BOOL int
			MSGResult = GetMessage(&message, 0, 0, 0);//pulling message
			TranslateMessage(&message);//key bind translation
			DispatchMessageA(&message);//message dispatch
			if (MSGResult == -1) //error check
			{
				return -1;
			}
		}
	}
	else
	{
		//Logging
	}
	//ShowWindow(handle, 0); wasn't required since window was created visible
	return 0;
}