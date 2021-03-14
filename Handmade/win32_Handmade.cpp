#include <Windows.h>
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
	}break;
	case WM_DESTROY:
	{
	}break;
	case WM_ACTIVATEAPP:
	{
	}break;
	default:
	{
		Result = DefWindowProc(handle, Message, wParam, lParam); //Default window proc
	}
	}
	return Result;
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


	RegisterClass(&wc);

	HWND handle = CreateWindowEx(0, wc.lpszClassName, L"Handmade", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0
		, 0, Instance, 0);
	if (handle)
	{
		//logging
	}
	//ShowWindow(handle, 0);
	MSG message;
	BOOL MSGResult;
	while (MSGResult = GetMessage(&message, 0, 0, 0) > 0)
	{
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}
	if (MSGResult == -1)
	{
		//Logging
	}
	
	return 0;
}