#include <Windows.h>
#include <stdint.h>
#include <Xinput.h>
#include <Dsound.h>
#include <math.h>

// testing new convention for static keyword
#define internal	   static
#define local_persist  static
#define global_persist static
#define bool32		   int32_t
/*********************************************************************************************************/
/*GLOBAL_VARIABLES*/
/*********************************************************************************************************/
global_persist bool running;
/****************************/
/*CLEAN_UP_NEEDED*/
// global_persist bool Latencytest = false;
global_persist LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
#define Pi32 3.141592653589793238462643
/****************************/

/*STRUCTURES*/

struct Win32_SoundProperty
{
	int32_t	 SamplePerSecond;
	int32_t	 BytesPerSample;
	int32_t	 SampleBufferSize;
	int32_t	 ToneHz;
	int16_t	 ToneVolume;
	int32_t	 WavePeriod;
	int32_t	 HalfWavePeriod;
	uint32_t RunningSampleIndex;
	int32_t	 Latency;
};

struct WindowDimension
{
	int Width;
	int Height;
};
struct Win32_OffScreenBuffer
{
	void*	   PointBITMAPMemory;
	BITMAPINFO BitMapInfo;
	int		   BitMapWidth;
	int		   BitMapHeight;
	int		   BytePerPixel;
	int		   Pitch;
};
global_persist struct Win32_OffScreenBuffer BackBuffer;

/*********************************************************************************************************/
/*********************************************************************************************************/

internal void Win32_PlayBuffer(Win32_SoundProperty* SoundOutput, DWORD PlayCursor)
{
	PlayCursor = (PlayCursor + (SoundOutput->Latency * SoundOutput->BytesPerSample)) % SoundOutput->SampleBufferSize;
	DWORD LockOffSet = (SoundOutput->RunningSampleIndex * SoundOutput->BytesPerSample) % SoundOutput->SampleBufferSize;
	void* Region1;
	DWORD Region1Size;
	void* Region2;
	DWORD Region2Size;
	DWORD BytesToLock = 0;

	if (PlayCursor == LockOffSet)
	{
		// BytesToLock = SampleBufferSize;
	}
	else if (PlayCursor < LockOffSet)
	{
		BytesToLock = SoundOutput->SampleBufferSize - LockOffSet + PlayCursor;
	}
	else
	{
		BytesToLock = PlayCursor - LockOffSet;
	}

	if (SUCCEEDED(
			GlobalSecondaryBuffer->Lock(LockOffSet, BytesToLock, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		int16_t* SampleOut = (int16_t*)Region1;
		for (int i = 0; i < Region1Size / SoundOutput->BytesPerSample; i++)
		{
			float	t = 2.0f * Pi32 * (((float)SoundOutput->RunningSampleIndex / (float)SoundOutput->WavePeriod));
			float	SineValue = sinf(t);
			int16_t Value	  = int16_t(SineValue * SoundOutput->ToneVolume);
			*SampleOut++	  = Value;
			*SampleOut++	  = Value;
			++SoundOutput->RunningSampleIndex;
		}
		SampleOut = (int16_t*)Region2;
		for (int i = 0; i < Region2Size / SoundOutput->BytesPerSample; i++)
		{
			float	t = 2.0f * Pi32 * (((float)SoundOutput->RunningSampleIndex / (float)SoundOutput->WavePeriod));
			float	SineValue = sinf(t);
			int16_t Value	  = int16_t(SineValue * SoundOutput->ToneVolume);
			*SampleOut++	  = Value;
			*SampleOut++	  = Value;
			++SoundOutput->RunningSampleIndex;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)

typedef DIRECT_SOUND_CREATE(direct_sound_create);
/*DYNAMICALLY LOADING FUNCTION FROM A LIBARY MANUALLY*/
/*XINPUTGETSTATE SUPPORT*/
#define X_INPUT_GET_STATE(name)                                                                                        \
	DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState) // defined a macro for the creation of stub function
/*The Function definition is pulled from the header and is defined to create a function pointer that would dynamically
   link with either the manually loaded windows function or the stub*/

typedef X_INPUT_GET_STATE(X_Input_Get_State); // creating the replacement definition

X_INPUT_GET_STATE(XInputGetStateStub) // stub function
{
	return ERROR_DEVICE_NOT_CONNECTED;
};
/*The stub function is created to avoid crashes when the program fails to load the intended function the stub essential
  acts as a fake funtion that replace and should be designed with the conditional that the funtion is affecting into
  mind*/

global_persist X_Input_Get_State* XInputGetState_ =
	XInputGetStateStub; // creating a function pointer that points to the function that dynamically loads

#define XInputGetState XInputGetState_; // saftey mechanics
/*created to protect other programmer that may collaberate from accidently calling the orginal function by replacing
  the function name of orginal windows with the macro */

internal void Win32_InitSound(HWND handle, int32_t SamplesPerSec, int32_t BufferSize)
{
	// Loading sound libary
	HMODULE SoundLib = LoadLibraryA("dsound.dll");
	if (SoundLib)
	{
		// Creating the required function pointer.. and assigning the pointer
		direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(SoundLib, "DirectSoundCreate");
		// direct sound
		LPDIRECTSOUND DirectSound;
		if (SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};

			WaveFormat.wFormatTag	   = WAVE_FORMAT_PCM;
			WaveFormat.nChannels	   = 2;
			WaveFormat.nSamplesPerSec  = SamplesPerSec;
			WaveFormat.wBitsPerSample  = 16;
			WaveFormat.nBlockAlign	   = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize		   = 0;

			// SetCooperativeLevel set how device instances interact with each other
			if (SUCCEEDED(DirectSound->SetCooperativeLevel(handle, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription	= {};
				BufferDescription.dwSize		= sizeof(BufferDescription);
				BufferDescription.dwFlags		= DSBCAPS_PRIMARYBUFFER;
				BufferDescription.dwBufferBytes = BufferSize;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				// primary biffer allocation
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					// sets the format for primary buffer
					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
					}
					else
					{
					}
				}
				else
				{
				}
			}
			else
			{
				// logging
			}
			DSBUFFERDESC SecondaryBufferDescription	 = {};
			SecondaryBufferDescription.dwFlags		 = 0;
			SecondaryBufferDescription.dwBufferBytes = BufferSize;
			SecondaryBufferDescription.lpwfxFormat	 = &WaveFormat;
			SecondaryBufferDescription.dwSize		 = sizeof(SecondaryBufferDescription);

			if (SUCCEEDED(DirectSound->CreateSoundBuffer(&SecondaryBufferDescription, &GlobalSecondaryBuffer, 0)))
			{
			}
		}
		else
		{
			// logging
		}
	}
}

/*XINPUTSETSTATE SUPPORT*/
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)

typedef X_INPUT_SET_STATE(X_Input_Set_State);

X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
};

global_persist X_Input_Set_State* XInputSetState_ = XInputSetStateStub;

#define XInputSetState XInputSetState_;

internal void LoadXLibary()
{
	HMODULE XInputLibary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibary)
	{
		XInputLibary = LoadLibraryA("xinput1_3.dll");
	}
	if (XInputLibary)
	{
		XInputGetState_ = (X_Input_Get_State*)(GetProcAddress(XInputLibary, "XInputGetState"));
		XInputSetState_ = (X_Input_Set_State*)(GetProcAddress(XInputLibary, "XInputSetState"));
	}
}

internal WindowDimension Win32GetWindowDimension(HWND handle)
{
	WindowDimension Result;
	RECT			ClientRECT;
	GetClientRect(handle, &ClientRECT);
	Result.Height = ClientRECT.bottom - ClientRECT.top;
	Result.Width  = ClientRECT.right - ClientRECT.left;
	return Result;
}
internal void RenderWeirdGradient(Win32_OffScreenBuffer Buffer, int XOffset, int YOffset)
{
	// may need to clear this to black
	uint8_t* Row = (uint8_t*)Buffer.PointBITMAPMemory;
	for (int Y = 0; Y < Buffer.BitMapHeight; Y++)
	{
		uint32_t* Pixel = (uint32_t*)Row;
		for (int X = 0; X < Buffer.BitMapWidth; X++)
		{
			/*                 1  2  3  4    [ByteOrder]
			 * Pixel in memory: 00 00 00 00
			 * LITTLE ENDIAN ARCHITECTURE EFFECT
			 * Pixel in memory: BB GG RR xx
			 * 0xxxBBGGRR on 32 bit pixel buffer
			 */

			/*WELCOME TO 1980
			//blue channel
			*Pixel = (uint8_t)(X + XOffset);
			++Pixel;

			//green channel
			*Pixel = (uint8_t)(Y+YOffset);
			++Pixel;

			//red channel
			*Pixel = 0;
			++Pixel;

			//alignment padding
			*Pixel = 0;
			++Pixel;*/

			/*
			 * MEMORY  : BB GG RR XX
			 * REGISTER: XX RR GG BB
			 */
			uint8_t Blue  = (X + XOffset);
			uint8_t Green = (Y + YOffset);
			uint8_t Red	  = (X - YOffset);
			*Pixel++	  = ((Red << 16) | (Green << 8) | Blue);
		}
		Row += Buffer.Pitch;
	}
}

// function to create a bitmap memory if not initialized and to update the old one with new
internal void Win32_ResizeDIBSection(Win32_OffScreenBuffer* Buffer, int width, int height)
{
	if (Buffer->PointBITMAPMemory)
	{
		VirtualFree(Buffer->PointBITMAPMemory, 0, MEM_RELEASE);
		// Vitual Protect can be used here to catch any stale pointer to find 'use after free bug' and flag it down
	}
	Buffer->BytePerPixel				= 4;
	Buffer->BitMapHeight				= height;
	Buffer->BitMapWidth					= width;
	Buffer->BitMapInfo.bmiHeader.biSize = sizeof(
		Buffer->BitMapInfo
			.bmiHeader); // gives the size of bmiheader only since it is required to traverse to the palette color
	Buffer->BitMapInfo.bmiHeader.biWidth  = Buffer->BitMapWidth;
	Buffer->BitMapInfo.bmiHeader.biHeight = -Buffer->BitMapHeight;
	// negative to symbolize a top down bitmap
	Buffer->BitMapInfo.bmiHeader.biPlanes = 1;
	Buffer->BitMapInfo.bmiHeader.biBitCount =
		32; // asked 32 for DWORD alignment which is due for explanation,4-byte bit alignment
	Buffer->BitMapInfo.bmiHeader.biCompression	 = BI_RGB;
	Buffer->BitMapInfo.bmiHeader.biSizeImage	 = 0;
	Buffer->BitMapInfo.bmiHeader.biXPelsPerMeter = 0;
	Buffer->BitMapInfo.bmiHeader.biClrUsed		 = 0;
	Buffer->BitMapInfo.bmiHeader.biClrImportant	 = 0;
	// allocating the buffer ourselves
	int BitMapMemorySizeBytes = 4 * Buffer->BitMapWidth * Buffer->BitMapHeight;
	Buffer->PointBITMAPMemory = VirtualAlloc(NULL, BitMapMemorySizeBytes, MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch			  = width * Buffer->BytePerPixel;
	// Two Modes:MEE_COMMIT,MEM_RESERVE commit allocates the memory for current use,
	// reserve allocates but doesn't use the memory range
	// RenderWeirdGradient(120, 0);
}

// actual Drawing function takes a bitmap
internal void Win32_UpdateWindows(HDC Context, Win32_OffScreenBuffer Buffer, int X, int Y, int Width, int Height)
{
	// StretchDIBits function copies the color data for a rectangle of pixels in a DIB, JPEG, or PNG image to the
	// specified destination rectangle
	// shouldn't the destinations be the window ..?
	StretchDIBits(
		Context,
		0,
		0,
		Width,
		Height,
		0,
		0,
		Buffer.BitMapWidth,
		Buffer.BitMapHeight,
		Buffer.PointBITMAPMemory,
		&Buffer.BitMapInfo,
		DIB_RGB_COLORS,
		SRCCOPY);
}

LRESULT CALLBACK WindowProc(HWND handle, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;
	switch (Message)
	{
	case WM_PAINT: // the "start painting" message
	{
		PAINTSTRUCT Paint;								// paint struct that gets set by windows to get useful data back
		HDC DeviceContext = BeginPaint(handle, &Paint); // notifies the beginning of paint,Also returns a device
														// context(takes a long pointer to PAINTSTRUCT)
		int				X		  = Paint.rcPaint.left;
		int				Y		  = Paint.rcPaint.top;
		int				Width	  = Paint.rcPaint.right - Paint.rcPaint.left;
		int				Height	  = Paint.rcPaint.bottom - Paint.rcPaint.top;
		WindowDimension Dimension = Win32GetWindowDimension(handle);
		Win32_UpdateWindows(DeviceContext, BackBuffer, X, Y, Width, Height);
		EndPaint(handle, &Paint); // notifies the end of painting
	}
	break;
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		// Latencytest = true;
		uint32_t VK_Code = wParam;
		bool	 WasDown = ((lParam & (1 << 30)) != 0);
		bool	 IsDown	 = ((lParam & (1 << 31)) == 0);
		if (WasDown != IsDown)
		{
			if (VK_Code == 'W')
			{
			}
			if (VK_Code == 'A')
			{
			}
			if (VK_Code == 'S')
			{
			}
			if (VK_Code == 'D')
			{
			}
			if (VK_Code == VK_ESCAPE)
			{
				if (IsDown)
				{
					OutputDebugStringA("down \n");
				}
				if (WasDown)
				{
					OutputDebugStringA("up \n");
				}
			}
			if (VK_Code == VK_SPACE)
			{
			}
			if (VK_Code == VK_UP)
			{
			}
			if (VK_Code == VK_DOWN)
			{
			}
			if (VK_Code == VK_LEFT)
			{
			}
			if (VK_Code == VK_RIGHT)
			{
			}
		}
	}
	break;
	// WM_QUIT handles the actual exit conditions
	case WM_QUIT:
	{
		PostQuitMessage(0);
	}
	break;
	// WM_CLOSE handles the close  button
	case WM_CLOSE:
	{
		Result	= 0;
		running = false; // making the loop condition false
	}
	break;
	default:
	{
		Result = DefWindowProc(handle, Message, wParam, lParam); // Default window proc
	}
	}
	return (Result);
}
int WINAPI wWinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PWSTR cmdline, int ShowCode)
{
	// UINT windows define : typedef unsigned int UINT
	WNDCLASS wc = {}; //{} equals zero
	// the noise on this call...why..?
	//[THOUGHTS]:the noise goes away as soon as the window is larger than 1280X720..so maybe compression fault..?
	Win32_ResizeDIBSection(&BackBuffer, 1280, 720);
	wc.style	   = CS_OWNDC | CS_HREDRAW; // CS_OWNDC creates a device context for every window generated
	wc.lpfnWndProc = WindowProc;			// windows message procedure , normally uses DefWndProcc
	wc.hInstance   = Instance;				// current instance of the window
	// wc.hIcon = ; no Icon for now
	// wc.hCursor = ; no cursor
	// wc.hbrBackground = ; not setting background
	// wc.lpszMenuName = ; no classic window menu
	wc.lpszClassName = L"HandmadeWindowClass"; // window class name to identify the class and create the window using
											   // the class we created
	LARGE_INTEGER LastTime;
	QueryPerformanceCounter(&LastTime);
	LARGE_INTEGER CounterFrequency;
	QueryPerformanceFrequency(&CounterFrequency);

	RegisterClass(&wc); // registering the window class

	HWND handle = CreateWindowEx(
		0,
		wc.lpszClassName,
		L"Handmade",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		Instance,
		0); // creating the actual windows..managed through handles
	if (handle)
	{
		int					XOffset		= 0;
		int					YOffset		= 0;
		Win32_SoundProperty SoundOutput = {};
		SoundOutput.SamplePerSecond		= 48000;
		SoundOutput.BytesPerSample		= 4;
		SoundOutput.SampleBufferSize	= SoundOutput.BytesPerSample * SoundOutput.SamplePerSecond;
		SoundOutput.ToneHz				= 256;
		SoundOutput.ToneVolume			= 5000;
		SoundOutput.WavePeriod			= SoundOutput.SamplePerSecond / SoundOutput.ToneHz;
		SoundOutput.HalfWavePeriod		= SoundOutput.WavePeriod / 2;
		SoundOutput.RunningSampleIndex	= 0;
		SoundOutput.Latency				= SoundOutput.SamplePerSecond / 15;
		Win32_InitSound(handle, SoundOutput.SamplePerSecond, SoundOutput.SamplePerSecond * SoundOutput.BytesPerSample);

		GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

		running			   = true;
		uint64_t LastRDTSC = __rdtsc();
		while (running)
		{
			MSG message;
			while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) // while to flush the message queue
			{
				if (message.message == WM_QUIT)
				{
					running = false;
				}
				TranslateMessage(&message); // key bind translation
				DispatchMessageA(&message); // message dispatch
			}

			for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
			{
				XINPUT_STATE ControllerState;
				if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
				{
					// controller is plugged
					// check if the polling rate is too high
					XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

					bool Up			   = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
					bool Down		   = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
					bool Left		   = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
					bool Right		   = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
					bool Start		   = (Pad->wButtons & XINPUT_GAMEPAD_START);
					bool Back		   = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
					bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
					bool LeftShoulder  = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
					bool AButton	   = (Pad->wButtons & XINPUT_GAMEPAD_A);
					bool BButton	   = (Pad->wButtons & XINPUT_GAMEPAD_B);
					bool XButton	   = (Pad->wButtons & XINPUT_GAMEPAD_X);
					bool YButton	   = (Pad->wButtons & XINPUT_GAMEPAD_Y);

					int16_t StickX = Pad->sThumbLX;
					int16_t StickY = Pad->sThumbLY;
				}
				else
				{
					// controller is unavailable
				}
			}
			RenderWeirdGradient(BackBuffer, XOffset, YOffset);

			DWORD PlayCursor;
			DWORD WriteCursor;

			if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
			{
				Win32_PlayBuffer(&SoundOutput, PlayCursor);
			}

			HDC				Context	  = GetDC(handle);
			WindowDimension Dimension = Win32GetWindowDimension(handle);
			Win32_UpdateWindows(Context, BackBuffer, 0, 0, Dimension.Width, Dimension.Height);
			ReleaseDC(handle, Context);
			++XOffset;
			++YOffset;
			LARGE_INTEGER EndTime;
			QueryPerformanceCounter(&EndTime);
			uint64_t EndRDTSC	 = __rdtsc();
			int32_t	 TimeElapsed = EndTime.QuadPart - LastTime.QuadPart;

			int32_t FPS		   = CounterFrequency.QuadPart / TimeElapsed;
			int32_t MSPerFrame = 1000 * TimeElapsed / CounterFrequency.QuadPart;
			int32_t RDTSC	   = (EndRDTSC - LastRDTSC) / (1000 * 1000);

			char OutputString[256]{};
			// the fck..?no array decay..?
			// used wide string...poggers
			// done with micro counter ....investigate the inconsistent delay...
			wsprintfA(OutputString, "FPS : %d MSPerFrame : %d RDTSC : %d\n", FPS, MSPerFrame, RDTSC);
			OutputDebugStringA(OutputString);
			LastTime  = EndTime;
			LastRDTSC = EndRDTSC;
		}
	}
	else
	{
		// Logging
	}
	// ShowWindow(handle, 0); wasn't required since window was created visible
	return 0;
}
