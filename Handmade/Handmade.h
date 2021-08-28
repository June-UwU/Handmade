#pragma once
#include "Defines.h"
#include <Dsound.h>


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

void GameUpdateAndRender(Win32_OffScreenBuffer Buffer, int XOffset, int YOffset);
