#pragma once
#include "Defines.h"
#include <Dsound.h>

struct  GameScreenBuffer
{
	void* Memory;
	int BytesPerPixel;
	int Height;
	int Width;
};

void GameUpdateAndRender(GameScreenBuffer* Buffer, int XOffset, int YOffset);
