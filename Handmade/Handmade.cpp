#include "Handmade.h"
#include <stdint.h>

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

void GameUpdateAndRender(Win32_OffScreenBuffer Buffer, int XOffset, int YOffset)
{
	RenderWeirdGradient(Buffer,XOffset,YOffset);
}
