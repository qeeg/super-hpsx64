/*
	Copyright (C) 2012-2016

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


// need a define to toggle sse2 on and off for now
// on 64-bit systems, sse2 is supposed to come as standard
//#define _ENABLE_SSE2
//#define _ENABLE_SSE41


#include "PS1_Gpu.h"
#include <math.h>
#include "PS1_Timer.h"
#include "Reciprocal.h"




using namespace Playstation1;
using namespace x64Asm::Utilities;
using namespace Math::Reciprocal;



//#define USE_DIVIDE_GCC
//#define USE_MULTIPLY_CUSTOM

// templates take too long too compile and won't be needed in later versions
/*
#define USE_TEMPLATES_RECTANGLE
#define USE_TEMPLATES_RECTANGLE8
#define USE_TEMPLATES_RECTANGLE16
#define USE_TEMPLATES_SPRITE
#define USE_TEMPLATES_SPRITE8
#define USE_TEMPLATES_SPRITE16
#define USE_TEMPLATES_TRIANGLE_MONO
#define USE_TEMPLATES_RECTANGLE_MONO
#define USE_TEMPLATES_TRIANGLE_TEXTURE
#define USE_TEMPLATES_RECTANGLE_TEXTURE
#define USE_TEMPLATES_TRIANGLE_GRADIENT
#define USE_TEMPLATES_RECTANGLE_GRADIENT
#define USE_TEMPLATES_TRIANGLE_TEXTUREGRADIENT
#define USE_TEMPLATES_RECTANGLE_TEXTUREGRADIENT
*/

// enable debugging

#ifdef _DEBUG_VERSION_

#define INLINE_DEBUG_ENABLE

//#define INLINE_DEBUG_WRITE
//#define INLINE_DEBUG_DMA_WRITE
//#define INLINE_DEBUG_EXECUTE
//#define INLINE_DEBUG_RASTER_SCANLINE
//#define INLINE_DEBUG_RASTER_VBLANK

//#define INLINE_DEBUG_DRAWSTART
//#define INLINE_DEBUG_EVENT
//#define INLINE_DEBUG_VARS
//#define INLINE_DEBUG_EXECUTE_NAME
//#define INLINE_DEBUG_DRAW_SCREEN

//#define INLINE_DEBUG_DISPLAYAREA
//#define INLINE_DEBUG_DISPLAYMODE
//#define INLINE_DEBUG_DISPLAYENABLE
//#define INLINE_DEBUG_DISPLAYOFFSET
//#define INLINE_DEBUG_DISPLAYRANGE
//#define INLINE_DEBUG_MASK
//#define INLINE_DEBUG_WINDOW

//#define INLINE_DEBUG_TRIANGLE_MONO_PIXELSDRAWN
//#define INLINE_DEBUG_TRIANGLE_TEXTURE
//#define INLINE_DEBUG_TEXTURE_RECTANGLE
//#define INLINE_DEBUG_PARTIAL_TRIANGLE

//#define INLINE_DEBUG_RUN_MONO
//#define INLINE_DEBUG_RUN_SHADED
//#define INLINE_DEBUG_RUN_TEXTURE
//#define INLINE_DEBUG_RUN_SPRITE
//#define INLINE_DEBUG_RUN_TRANSFER

//#define INLINE_DEBUG_RASTER


//#define INLINE_DEBUG
//#define INLINE_DEBUG_READ
//#define INLINE_DEBUG_DMA_READ
//#define INLINE_DEBUG_TRIANGLE_MONO
//#define INLINE_DEBUG_TRIANGLE_GRADIENT
//#define INLINE_DEBUG_TRIANGLE_GRADIENT_TEST
//#define INLINE_DEBUG_TRIANGLE_TEXTURE
//#define INLINE_DEBUG_TRIANGLE_TEXTURE_GRADIENT
//#define INLINE_DEBUG_TRIANGLE_MONO_TEST

#endif




u32* GPU::_DebugPC;
u64* GPU::_DebugCycleCount;

//u32* GPU::_Intc_Master;
u32* GPU::_Intc_Stat;
u32* GPU::_Intc_Mask;
u32* GPU::_R3000A_Status_12;
u32* GPU::_R3000A_Cause_13;
u64* GPU::_ProcStatus;

//GPU::t_InterruptCPU GPU::InterruptCPU;


GPU* GPU::_GPU;


u64* GPU::_NextSystemEvent;


// needs to be removed sometime - no longer needed
u32* GPU::DebugCpuPC;


WindowClass::Window *GPU::DisplayOutput_Window;
WindowClass::Window *GPU::FrameBuffer_DebugWindow;

u32 GPU::MainProgramWindow_Width;
u32 GPU::MainProgramWindow_Height;


bool GPU::DebugWindow_Enabled;
//WindowClass::Window *GPU::DebugWindow;

// dimension 1 is twx/twy, dimension #2 is window tww/twh, dimension #3 is value
//u8 GPU::Modulo_LUT [ 32 ] [ 32 ] [ 256 ];


Debug::Log GPU::debug;


const u32 GPU::HBlank_X_LUT [ 8 ] = { 256, 368, 320, 0, 512, 0, 640, 0 };
const u32 GPU::VBlank_Y_LUT [ 2 ] = { 480, 576 };
const u32 GPU::Raster_XMax_LUT [ 2 ] [ 8 ] = { { 341, 487, 426, 0, 682, 0, 853, 0 }, { 340, 486, 426, 0, 681, 0, 851, 0 } };
const u32 GPU::Raster_YMax_LUT [ 2 ] = { 525, 625 };

const u32 GPU::c_iGPUCyclesPerPixel [] = { 10, 7, 8, 0, 5, 0, 4, 0 };


const s64 GPU::c_iDitherValues [] = { -4LL << 32, 0LL << 32, -3LL << 32, 1LL << 32,
									2LL << 32, -2LL << 32, 3LL << 32, -1LL << 32,
									-3LL << 32, 1LL << 32, -4LL << 32, 0LL << 32,
									3LL << 32, -1LL << 32, 2LL << 32, -2LL << 32 };

const s64 GPU::c_iDitherZero [] = { 0, 0, 0, 0,
									0, 0, 0, 0,
									0, 0, 0, 0,
									0, 0, 0, 0 };

const s64 GPU::c_iDitherValues24 [] = { -4LL << 24, 0LL << 24, -3LL << 24, 1LL << 24,
									2LL << 24, -2LL << 24, 3LL << 24, -1LL << 24,
									-3LL << 24, 1LL << 24, -4LL << 24, 0LL << 24,
									3LL << 24, -1LL << 24, 2LL << 24, -2LL << 24 };

const s32 GPU::c_iDitherValues4 [] = { -4LL << 4, 0LL << 4, -3LL << 4, 1LL << 4,
									2LL << 4, -2LL << 4, 3LL << 4, -1LL << 4,
									-3LL << 4, 1LL << 4, -4LL << 4, 0LL << 4,
									3LL << 4, -1LL << 4, 2LL << 4, -2LL << 4 };

const s16 GPU::c_viDitherValues16_add [] = { 0<<8, 0<<8, 0<<8, 1<<8, 0<<8, 0<<8, 0<<8, 1<<8, 0<<8, 0<<8, 0<<8, 1<<8, 0<<8, 0<<8, 0<<8, 1<<8,
											2<<8, 0<<8, 3<<8, 0<<8, 2<<8, 0<<8, 3<<8, 0<<8, 2<<8, 0<<8, 3<<8, 0<<8, 2<<8, 0<<8, 3<<8, 0<<8,
											0<<8, 1<<8, 0<<8, 0<<8, 0<<8, 1<<8, 0<<8, 0<<8, 0<<8, 1<<8, 0<<8, 0<<8, 0<<8, 1<<8, 0<<8, 0<<8,
											3<<8, 0<<8, 2<<8, 0<<8, 3<<8, 0<<8, 2<<8, 0<<8, 3<<8, 0<<8, 2<<8, 0<<8, 3<<8, 0<<8, 2<<8, 0<<8 };

const s16 GPU::c_viDitherValues16_sub [] = { 4<<8, 0<<8, 3<<8, 0<<8, 4<<8, 0<<8, 3<<8, 0<<8, 4<<8, 0<<8, 3<<8, 0<<8, 4<<8, 0<<8, 3<<8, 0<<8,
											0<<8, 2<<8, 0<<8, 1<<8, 0<<8, 2<<8, 0<<8, 1<<8, 0<<8, 2<<8, 0<<8, 1<<8, 0<<8, 2<<8, 0<<8, 1<<8,
											3<<8, 0<<8, 4<<8, 0<<8, 3<<8, 0<<8, 4<<8, 0<<8, 3<<8, 0<<8, 4<<8, 0<<8, 3<<8, 0<<8, 4<<8, 0<<8,
											0<<8, 1<<8, 0<<8, 2<<8, 0<<8, 1<<8, 0<<8, 2<<8, 0<<8, 1<<8, 0<<8, 2<<8, 0<<8, 1<<8, 0<<8, 2<<8 };
								



GPU::GPU ()
{

	cout << "Running GPU constructor...\n";
/*
#ifdef INLINE_DEBUG_ENABLE
	debug.Create ( "GPU_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering GPU constructor";
#endif

	cout << "Resetting GPU...\n";

	Reset ();

	cout << "Testing GPU...\n";
	
	///////////////////////////////
	// *** TESTING ***
	GPU_CTRL_Read.Value = 0x14802000;
	UpdateRaster_VARS ();
	// *** END TESTING ***
	////////////////////////////////

	cout << "done\n";

#ifdef INLINE_DEBUG
	debug << "->Exiting GPU constructor";
#endif

	cout << "Exiting...\n";
*/
}

void GPU::Reset ()
{
	double dBusCycleRate;
	
	// zero object
	memset ( this, 0, sizeof( GPU ) );
	
#ifdef PS2_COMPILE

	dBusCycleRate = SystemBus_CyclesPerSec_PS2Mode1;

#else

	dBusCycleRate = SystemBus_CyclesPerSec_PS1Mode;
	
#endif

	NTSC_CyclesPerFrame = ( dBusCycleRate / ( 59.94005994L / 2.0L ) );
	PAL_CyclesPerFrame = ( dBusCycleRate / ( 50.0L / 2.0L ) );
	NTSC_FramesPerCycle = 1.0L / ( dBusCycleRate / ( 59.94005994L / 2.0L ) );
	PAL_FramesPerCycle = 1.0L / ( dBusCycleRate / ( 50.0L / 2.0L ) );

	NTSC_CyclesPerScanline = ( dBusCycleRate / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
	PAL_CyclesPerScanline = ( dBusCycleRate / ( 625.0L * ( 50.0L / 2.0L ) ) );
	NTSC_ScanlinesPerCycle = 1.0L / ( dBusCycleRate / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
	PAL_ScanlinesPerCycle = 1.0L / ( dBusCycleRate / ( 625.0L * ( 50.0L / 2.0L ) ) );

	NTSC_CyclesPerField_Even = 263.0L * ( dBusCycleRate / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
	NTSC_CyclesPerField_Odd = 262.0L * ( dBusCycleRate / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
	PAL_CyclesPerField_Even = 313.0L * ( dBusCycleRate / ( 625.0L * ( 50.0L / 2.0L ) ) );
	PAL_CyclesPerField_Odd = 312.0L * ( dBusCycleRate / ( 625.0L * ( 50.0L / 2.0L ) ) );
	NTSC_FieldsPerCycle_Even = 1.0L / ( 263.0L * ( dBusCycleRate / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
	NTSC_FieldsPerCycle_Odd = 1.0L / ( 262.0L * ( dBusCycleRate / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
	PAL_FieldsPerCycle_Even = 1.0L / ( 313.0L * ( dBusCycleRate / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
	PAL_FieldsPerCycle_Odd = 1.0L / ( 312.0L * ( dBusCycleRate / ( 625.0L * ( 50.0L / 2.0L ) ) ) );

	NTSC_DisplayAreaCycles = 240.0L * ( dBusCycleRate / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
	PAL_DisplayAreaCycles = 288.0L * ( dBusCycleRate / ( 625.0L * ( 50.0L / 2.0L ) ) );

	NTSC_CyclesPerVBlank_Even = ( 263.0L - 240.0L ) * ( dBusCycleRate / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
	NTSC_CyclesPerVBlank_Odd = ( 262.0L - 240.0L ) * ( dBusCycleRate / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
	PAL_CyclesPerVBlank_Even = ( 313.0L - 288.0L ) * ( dBusCycleRate / ( 625.0L * ( 50.0L / 2.0L ) ) );
	PAL_CyclesPerVBlank_Odd = ( 312.0L - 288.0L ) * ( dBusCycleRate / ( 625.0L * ( 50.0L / 2.0L ) ) );
	NTSC_VBlanksPerCycle_Even = 1.0L / ( ( 263.0L - 240.0L ) * ( dBusCycleRate / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
	NTSC_VBlanksPerCycle_Odd = 1.0L / ( ( 262.0L - 240.0L ) * ( dBusCycleRate / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
	PAL_VBlanksPerCycle_Even = 1.0L / ( ( 313.0L - 288.0L ) * ( dBusCycleRate / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
	PAL_VBlanksPerCycle_Odd = 1.0L / ( ( 312.0L - 288.0L ) * ( dBusCycleRate / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		

	CyclesPerPixel_INC_Lookup [ 0 ] [ 0 ] = NTSC_CyclesPerPixelINC_256;
	CyclesPerPixel_INC_Lookup [ 0 ] [ 1 ] = NTSC_CyclesPerPixelINC_368;
	CyclesPerPixel_INC_Lookup [ 0 ] [ 2 ] = NTSC_CyclesPerPixelINC_320;
	CyclesPerPixel_INC_Lookup [ 0 ] [ 4 ] = NTSC_CyclesPerPixelINC_512;
	CyclesPerPixel_INC_Lookup [ 0 ] [ 6 ] = NTSC_CyclesPerPixelINC_640;

	CyclesPerPixel_INC_Lookup [ 1 ] [ 0 ] = PAL_CyclesPerPixelINC_256;
	CyclesPerPixel_INC_Lookup [ 1 ] [ 1 ] = PAL_CyclesPerPixelINC_368;
	CyclesPerPixel_INC_Lookup [ 1 ] [ 2 ] = PAL_CyclesPerPixelINC_320;
	CyclesPerPixel_INC_Lookup [ 1 ] [ 4 ] = PAL_CyclesPerPixelINC_512;
	CyclesPerPixel_INC_Lookup [ 1 ] [ 6 ] = PAL_CyclesPerPixelINC_640;
	
	CyclesPerPixel_Lookup [ 0 ] [ 0 ] = NTSC_CyclesPerPixel_256;
	CyclesPerPixel_Lookup [ 0 ] [ 1 ] = NTSC_CyclesPerPixel_368;
	CyclesPerPixel_Lookup [ 0 ] [ 2 ] = NTSC_CyclesPerPixel_320;
	CyclesPerPixel_Lookup [ 0 ] [ 4 ] = NTSC_CyclesPerPixel_512;
	CyclesPerPixel_Lookup [ 0 ] [ 6 ] = NTSC_CyclesPerPixel_640;

	CyclesPerPixel_Lookup [ 1 ] [ 0 ] = PAL_CyclesPerPixel_256;
	CyclesPerPixel_Lookup [ 1 ] [ 1 ] = PAL_CyclesPerPixel_368;
	CyclesPerPixel_Lookup [ 1 ] [ 2 ] = PAL_CyclesPerPixel_320;
	CyclesPerPixel_Lookup [ 1 ] [ 4 ] = PAL_CyclesPerPixel_512;
	CyclesPerPixel_Lookup [ 1 ] [ 6 ] = PAL_CyclesPerPixel_640;
	

	PixelsPerCycle_Lookup [ 0 ] [ 0 ] = NTSC_PixelsPerCycle_256;
	PixelsPerCycle_Lookup [ 0 ] [ 1 ] = NTSC_PixelsPerCycle_368;
	PixelsPerCycle_Lookup [ 0 ] [ 2 ] = NTSC_PixelsPerCycle_320;
	PixelsPerCycle_Lookup [ 0 ] [ 4 ] = NTSC_PixelsPerCycle_512;
	PixelsPerCycle_Lookup [ 0 ] [ 6 ] = NTSC_PixelsPerCycle_640;

	PixelsPerCycle_Lookup [ 1 ] [ 0 ] = PAL_PixelsPerCycle_256;
	PixelsPerCycle_Lookup [ 1 ] [ 1 ] = PAL_PixelsPerCycle_368;
	PixelsPerCycle_Lookup [ 1 ] [ 2 ] = PAL_PixelsPerCycle_320;
	PixelsPerCycle_Lookup [ 1 ] [ 4 ] = PAL_PixelsPerCycle_512;
	PixelsPerCycle_Lookup [ 1 ] [ 6 ] = PAL_PixelsPerCycle_640;



	// initialize command buffer mode
	BufferMode = MODE_NORMAL;
	
	//////////////////////////////////////////
	// mark GPU as not busy
	GPU_CTRL_Read.BUSY = 1;
	
	/////////////////////////////////////////////
	// mark GPU as ready to receive commands
	GPU_CTRL_Read.COM = 1;

	// initialize size of command buffer
	//BufferSize = 0;
	
	// GPU not currently busy
	//BusyCycles = 0;
}



// actually, you need to start objects after everything has been initialized
void GPU::Start ()
{
	cout << "Running GPU::Start...\n";
	
#ifdef INLINE_DEBUG_ENABLE
	debug.Create ( "GPU_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering GPU::Start";
#endif

	cout << "Resetting GPU...\n";

	Reset ();

	cout << "Testing GPU...\n";
	
	///////////////////////////////
	// *** TESTING ***
	GPU_CTRL_Read.Value = 0x14802000;
	UpdateRaster_VARS ();
	// *** END TESTING ***
	////////////////////////////////
	
	// set as current GPU object
	_GPU = this;
	
	// generate LUTs
	//Generate_Modulo_LUT ();

	cout << "done\n";

#ifdef INLINE_DEBUG
	debug << "->Exiting GPU::Start";
#endif

	cout << "Exiting GPU::Start...\n";
}



void GPU::SetDisplayOutputWindow ( u32 width, u32 height, WindowClass::Window* DisplayOutput )
{
	MainProgramWindow_Width = width;
	MainProgramWindow_Height = height;
	DisplayOutput_Window = DisplayOutput;
}


void GPU::Update_LCF ()
{
	if ( ( Y_Pixel & ~1 ) >= VBlank_Y )
	{
		// in the vblank area //
		
		if ( GPU_CTRL_Read.ISINTER )
		{
			// screen is interlaced //
			
			// toggle LCF at vblank when interlaced
			//GPU_CTRL_Read.LCF ^= 1;
			GPU_CTRL_Read.LCF = ( Y_Pixel ^ 1 ) & 1;
		}
		else
		{
			// screen is NOT interlaced //
			
			// clear LCF at vblank when NOT interlaced
			GPU_CTRL_Read.LCF = 0;
		}
	}
	else
	{
		// NOT in VBLANK //
		
		// check if NOT interlaced //
		if ( !GPU_CTRL_Read.ISINTER )
		{
			// toggle LCF per scanline when NOT interlaced AND not in VBLANK
			GPU_CTRL_Read.LCF ^= 1;
		}
	}
}


void GPU::Run ()
{

	//u64 Scanline_Number;
	
	if ( NextEvent_Cycle != *_DebugCycleCount ) return;
	
	// get the scanline number
	//Scanline_Number = GetScanline_Number ();

	// must be vblank or time to draw screen //
	
	
	// update scanline number
	Y_Pixel += 2;
	if ( Y_Pixel >= Raster_YMax )
	{
		// End of VBLANK //
		Y_Pixel -= Raster_YMax;
	}
	
	if ( ( Y_Pixel & ~1 ) < VBlank_Y )
	{
		// NOT in VBLANK //
		
		// check if NOT interlaced //
		if ( !GPU_CTRL_Read.ISINTER )
		{
			// toggle LCF per scanline when NOT interlaced AND not in VBLANK
			GPU_CTRL_Read.LCF ^= 1;
		}
	}

	

	// check if this is vblank or time to draw screen
	//if ( GetCycles_ToNextDrawStart () < dCyclesPerField0 )
	//{
	if ( ( Y_Pixel & ~1 ) == VBlank_Y )
	//if ( NextEvent_Cycle == NextEvent_Cycle_Vsync )
	{
		// vblank //
#ifdef INLINE_DEBUG_RASTER_VBLANK
	debug << "\r\n\r\n***VBLANK*** " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; (before) LCF=" << GPU_CTRL_Read.LCF << "; CTRL=" << hex << GPU_CTRL_Read.Value << "; BusyCycles=" << dec << BusyCycles << " Scanline_Number=" << Y_Pixel;
#endif
		
		// update count of frames for debugging
		Frame_Count++;
		
		// toggle lcf at vblank
		// *** testing ***
		// *note* only do this when screen is interlaced, otherwise do this every scanline
		if ( GPU_CTRL_Read.ISINTER )
		{
			// screen is interlaced //
			
			// toggle LCF at vblank when interlaced
			//GPU_CTRL_Read.LCF ^= 1;
			GPU_CTRL_Read.LCF = ( Y_Pixel ^ 1 ) & 1;
		}
		else
		{
			// screen is NOT interlaced //
			
			// clear LCF at vblank when NOT interlaced
			GPU_CTRL_Read.LCF = 0;
		}
		
		// send vblank signal
		SetInterrupt_Vsync ();
	//}
	//else
	//{
//#ifdef INLINE_DEBUG_RASTER_VBLANK
//	debug << "\r\n\r\n***SCREEN DRAW*** " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; (before) LCF=" << GPU_CTRL_Read.LCF << "; CTRL=" << hex << GPU_CTRL_Read.Value << "; BusyCycles=" << dec << BusyCycles;
//#endif
		// draw screen //

		// draw output to program window @ vblank! - if output window is available
		// this is actually probably wrong. Should actually draw screen after vblank is over
		if ( DisplayOutput_Window )
		{
			Draw_Screen ();
			
			if ( DebugWindow_Enabled )
			{
				Draw_FrameBuffer ();
			}
		}
		
#ifdef INLINE_DEBUG_RASTER_VBLANK
			debug << "; (after) LCF=" << GPU_CTRL_Read.LCF;
#endif

		//GetNextEvent_V ();
		
		
	}

#ifdef INLINE_DEBUG_RASTER_SCANLINE
	debug << "\r\n\r\n***SCANLINE*** " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; (after) LCF=" << GPU_CTRL_Read.LCF << "; CTRL=" << hex << GPU_CTRL_Read.Value << "; BusyCycles=" << dec << BusyCycles << " Scanline_Number=" << Y_Pixel << " VBlank_Y=" << VBlank_Y << " CyclesPerScanline=" << dCyclesPerScanline;
#endif
	
	// get the cycle at which we reach next vblank or draw start
	//GetNextEvent ();
	Update_NextEvent ();
	
	// update timers //
	Timers::_TIMERS->UpdateTimer ( 0 );
	Timers::_TIMERS->UpdateTimer ( 1 );
	Timers::_TIMERS->UpdateTimer ( 2 );
	
	// update timer events //
	Timers::_TIMERS->Get_NextEvent ( 0, NextEvent_Cycle );
	Timers::_TIMERS->Get_NextEvent ( 1, NextEvent_Cycle );
	Timers::_TIMERS->Get_NextEvent ( 2, NextEvent_Cycle );
	
	// need to update pixel clock
	//UpdateRaster ();
}


void GPU::SetNextEvent ( u64 CycleOffset )
{
	NextEvent_Cycle = CycleOffset + *_DebugCycleCount;
	
	Update_NextEventCycle ();
}

void GPU::SetNextEvent_Cycle ( u64 Cycle )
{
	NextEvent_Cycle = Cycle;
	
	Update_NextEventCycle ();
}

void GPU::Update_NextEventCycle ()
{
	if ( NextEvent_Cycle > *_DebugCycleCount && ( NextEvent_Cycle < *_NextSystemEvent || *_NextSystemEvent <= *_DebugCycleCount ) ) *_NextSystemEvent = NextEvent_Cycle;
}


void GPU::Update_NextEvent ()
{
	//u64 CycleOffset1;
	//double dCyclesToNext;
	
	dGPU_NextEventCycle += dCyclesPerScanline;
	//iGPU_NextEventCycle += iCyclesPerScanline;
	//dCyclesToNext = (double)(*_DebugCycleCount)
	//CycleOffset1 = (u64) dGPU_NextEventCycle;
	NextEvent_Cycle = (u64) dGPU_NextEventCycle;
	if ( ( dGPU_NextEventCycle - ( (double) NextEvent_Cycle ) ) > 0.0L ) NextEvent_Cycle++;
	//SetNextEvent_Cycle ( iGPU_NextEventCycle );
	Update_NextEventCycle ();
	
#ifdef INLINE_DEBUG_EVENT
	debug << "\r\nGPU::Update_NextEvent CycleOffset=" << dec << dCyclesPerScanline;
#endif
}

void GPU::GetNextEvent_V ()
{
	u64 CycleOffset1;


#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// *** testing *** get next vsync event
	CycleOffset1 = (u64) ceil ( GetCycles_ToNextVBlank () );
#else
	CycleOffset1 = CEILD ( GetCycles_ToNextVBlank () );
#endif

	
	NextEvent_Cycle_Vsync = *_DebugCycleCount + CycleOffset1;

#ifdef INLINE_DEBUG_EVENT
	debug << "\r\nGPU::GetNextEvent CycleOffset=" << dec << dCyclesToNext;
#endif

	//SetNextEvent_V ( CycleOffset1 );
}

void GPU::GetNextEvent ()
{
	u64 CycleOffset1;	//, CycleOffset2;
	//double dCyclesToNext;
	
	//dCyclesToNext = GetCycles_ToNextScanline ();
	//dCyclesToNext = GetCycles_ToNextVBlank ();
	//dGPU_NextEventCycle = ((double) (*_DebugCycleCount)) + dCyclesToNext;
	
	// *** testing *** run gpu event per scanline
	//CycleOffset1 = (u64) ceil ( GetCycles_ToNextVBlank () );
	
	dGPU_NextEventCycle = GetCycles_ToNextScanlineStart ();
	//CycleOffset1 = (u64) ceil ( GetCycles_ToNextScanlineStart () );
	CycleOffset1 = (u64) ceil ( dGPU_NextEventCycle );
	
	// need to store cycle number of the next scanline start
	dGPU_NextEventCycle += (double) ( *_DebugCycleCount );
	
	//CycleOffset1 = (u64) ceil ( dGPU_NextEventCycle );

#ifdef INLINE_DEBUG_EVENT
	debug << "\r\nGPU::GetNextEvent CycleOffset=" << dec << dCyclesToNext;
#endif

	//if ( CycleOffset1 < CycleOffset2 )
	//{
		// set the vblank as the next event
		SetNextEvent ( CycleOffset1 );
	//}
	//else
	//{
		// set drawing the screen as the next event
		//SetNextEvent ( CycleOffset2 );
	//}
}


static u32 GPU::Read ( u32 Address )
{
	u32 Temp;

#ifdef INLINE_DEBUG_READ
	debug << "\r\n\r\nGPU::Read; " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
#endif
	
	// handle register specific issues before read
	
	// Read GPU register value
	switch ( Address )
	{
		case GPU_DATA:
#ifdef INLINE_DEBUG_READ
			debug << "; GPU_DATA = ";
#endif

			// incoming read of DATA register from bus
			
			Temp = _GPU->ProcessDataRegRead ();
						
#ifdef INLINE_DEBUG_READ
			debug << hex << Temp;
#endif

			return Temp;
			
			break;
			
		case GPU_CTRL:
		
			// determine if gpu is busy or not and set flags accordingly before returning them
			if ( *_DebugCycleCount >= _GPU->BusyUntil_Cycle )
			{
				// gpu is not busy
				_GPU->GPU_CTRL_Read.BUSY = 1;
				_GPU->GPU_CTRL_Read.COM = 1;
			}


			// *note* if gpu is not in interlaced mode, then LCF changes per scanline
			// ***TODO*** LCF is zero during vblank?
			/*
			if ( !_GPU->GPU_CTRL_Read.ISINTER )
			{
				// graphics mode is NOT interlaced //
				
				u64 Scanline_Number, Scanline_Offset;


#ifdef USE_DIVIDE_GCC
				// *** divide ***
				Scanline_Number = ((u64)( ( *_DebugCycleCount ) / _GPU->dCyclesPerScanline ) );
#else
				Scanline_Number = (u64) ( ( *_DebugCycleCount ) * _GPU->dScanlinesPerCycle );
#endif


				// *** testing *** get value of LCF for current scanline
				//_GPU->GPU_CTRL_Read.LCF = ((u32) ( ( *_DebugCycleCount ) / _GPU->dCyclesPerScanline ) ) & 1;
				_GPU->GPU_CTRL_Read.LCF = ( (u32) Scanline_Number ) & 1;
				
				// check if scanline is in vblank
				if ( !_GPU->GPU_CTRL_Read.VIDEO )
				{
					// NTSC //
					
#ifdef USE_DIVIDE_GCC
					// *** divide ***
					// modulo the number of scanlines to get scanline offset from start of the full frame
					Scanline_Offset = Scanline_Number % NTSC_ScanlinesPerFrame;
#else
					Scanline_Offset = RMOD ( Scanline_Number, NTSC_ScanlinesPerFrame, NTSC_FramesPerScanline );
#endif
					
					// check if it is in the even field or the odd field
					if ( Scanline_Offset < NTSC_ScanlinesPerField_Even )
					{
						// even field //
						
						// check that we are past vblank
						if ( Scanline_Offset >= NTSC_VBlank )
						{
							// vblank area //
							_GPU->GPU_CTRL_Read.LCF = 0;
						}
						
					}
					else
					{
						// odd field //
						
						// start from zero
						Scanline_Offset -= NTSC_ScanlinesPerField_Even;
						
						// check that we are past vblank
						if ( Scanline_Offset >= NTSC_VBlank )
						{
							// vblank area //
							_GPU->GPU_CTRL_Read.LCF = 0;
						}
					}
				}
				else
				{
					// PAL //
					
#ifdef USE_DIVIDE_GCC
					// *** divide ***
					// modulo the number of scanlines to get scanline offset from start of the full frame
					Scanline_Offset = Scanline_Number % PAL_ScanlinesPerFrame;
#else
					Scanline_Offset = RMOD ( Scanline_Number, PAL_ScanlinesPerFrame, PAL_FramesPerScanline );
#endif
					
					// check if it is in the even field or the odd field
					if ( Scanline_Offset < PAL_ScanlinesPerField_Even )
					{
						// even field //
						
						// check that we are past vblank
						if ( Scanline_Offset >= PAL_VBlank )
						{
							// vblank area //
							_GPU->GPU_CTRL_Read.LCF = 0;
						}
						
					}
					else
					{
						// odd field //
						
						// start from zero
						Scanline_Offset -= PAL_ScanlinesPerField_Even;
						
						// check that we are past vblank
						if ( Scanline_Offset >= PAL_VBlank )
						{
							// vblank area //
							_GPU->GPU_CTRL_Read.LCF = 0;
						}
					}
				}
			}
			*/
			
			// set data request bit
			switch ( _GPU->GPU_CTRL_Read.DMA )
			{
				case 0:
					// always zero //
					_GPU->GPU_CTRL_Read.DataRequest = 0;
					break;
					
				case 1:
					
					if ( _GPU->BufferSize >= 16 )
					{
						// buffer is full //
						_GPU->GPU_CTRL_Read.DataRequest = 0;
					}
					else
					{
						// buffer is NOT full //
						_GPU->GPU_CTRL_Read.DataRequest = 1;
					}
					
					break;
					
				case 2:
					// same as bit 28
					_GPU->GPU_CTRL_Read.DataRequest = _GPU->GPU_CTRL_Read.COM;
					break;
					
				case 3:
					// same as bit 27
					_GPU->GPU_CTRL_Read.DataRequest = _GPU->GPU_CTRL_Read.IMG;
					break;
			}
			
			if ( _GPU->BufferMode == MODE_IMAGEIN )
			{
				// gpu is receiving an image, so is not ready for commands and is busy
				_GPU->GPU_CTRL_Read.BUSY = 0;
			}
			

#ifdef INLINE_DEBUG_READ
			debug << "; GPU_CTRL = " << hex << _GPU->GPU_CTRL_Read.Value << " CycleCount=" << dec << *_DebugCycleCount << " BusyUntil=" << _GPU->BusyUntil_Cycle;
#endif

			// incoming read of CTRL register from bus
			return _GPU->GPU_CTRL_Read.Value;
			break;
			
			
		default:
#ifdef INLINE_DEBUG_READ
			debug << "; Invalid";
#endif
		
			// invalid GPU Register
			cout << "\nhps1x64 ALERT: Unknown GPU READ @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << "\n";
			break;
	}
	
}

static void GPU::Write ( u32 Address, u32 Data, u32 Mask )
{
#ifdef INLINE_DEBUG_WRITE
	debug << "\r\n\r\nGPU::Write; " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Data = " << Data;
#endif

	// *** testing *** check if mask is a word write
	if ( Mask != 0xffffffff )
	{
		cout << "\nhps1x64 ALERT: GPU::Write Mask=" << hex << Mask;
	}
	
	// Write GPU register value
	switch ( Address )
	{
		case GPU_DATA:
#ifdef INLINE_DEBUG_WRITE
			debug << "; GPU_DATA";
#endif

			// incoming write to DATA register from bus
			
			_GPU->ProcessDataRegWrite ( Data );
			
			break;
			
		case GPU_CTRL:
#ifdef INLINE_DEBUG_WRITE
			debug << "; GPU_CTRL";
#endif

			// incoming write to CTRL register from bus
			
			// check if the GPU is busy

			// step 2: execute command written to CTRL register
			// the commands beyond 0x3f are mirrors of 0x0-0x3f
			switch ( ( Data >> 24 ) & 0x3f )
			{
				case CTRL_Write_ResetGPU:
#ifdef INLINE_DEBUG_WRITE
			debug << "; ResetGPU";
#endif
				
					// *note* this is supposed to reset the gpu
					_GPU->BufferSize = 0;
					//_GPU->X_Pixel = 0;
					//_GPU->Y_Pixel = 0;
				
					// set status to 0x14802000
					_GPU->GPU_CTRL_Read.Value = 0x14802000;
					
					// the resolution may have changed
					_GPU->UpdateRaster_VARS ();
					
					break;
					
				case CTRL_Write_ResetBuffer:
#ifdef INLINE_DEBUG_WRITE
			debug << "; ResetBuffer";
#endif
				
					_GPU->BufferSize = 0;
					break;
					
				case CTRL_Write_ResetIRQ:
#ifdef INLINE_DEBUG_WRITE
			debug << "; ResetIRQ";
#endif
				
					break;
					
				case CTRL_Write_DisplayEnable:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_DISPLAYENABLE
			debug << "\r\n; DisplayEnable";
			debug << "; Data=" << hex << Data;
#endif
				
					_GPU->GPU_CTRL_Read.DEN = Data & 1;
					break;
					
				case CTRL_Write_DMASetup:
#ifdef INLINE_DEBUG_WRITE
			debug << "; DMASetup";
#endif

					_GPU->GPU_CTRL_Read.DMA = Data & 3;
					
					break;
					
				case CTRL_Write_DisplayArea:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_DISPLAYAREA
			debug << "\r\n; DisplayArea";
#endif

					//DrawArea_TopLeftX = Data & 0x3ff;
					//DrawArea_TopLeftY = ( Data >> 10 ) & 0x1ff;
					_GPU->ScreenArea_TopLeftX = Data & 0x3ff;
					_GPU->ScreenArea_TopLeftY = ( Data >> 10 ) & 0x1ff;
					
					
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_DISPLAYAREA
			//debug << dec << "DrawArea_TopLeftX=" << DrawArea_TopLeftX << " DrawArea_TopLeftY=" << DrawArea_TopLeftY;
			debug << dec << "ScreenArea_TopLeftX=" << _GPU->ScreenArea_TopLeftX << " ScreenArea_TopLeftY=" << _GPU->ScreenArea_TopLeftY;
#endif

					break;
					
				case CTRL_Write_HorizontalDisplayRange:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_DISPLAYRANGE
			debug << "\r\n; HorizontalDisplayRange";
#endif

					////////////////////////////////////////////////////////////////
					// horizontal display range is given as location of the bit!!
					// so to get byte location in memory you divide by 8
					_GPU->DisplayRange_Horizontal = Data & 0xffffff;
					_GPU->DisplayRange_X1 = _GPU->DisplayRange_Horizontal & 0xfff;
					_GPU->DisplayRange_X2 = _GPU->DisplayRange_Horizontal >> 12;
					
#if defined INLINE_DEBUG_DISPLAYRANGE
			debug << "; DisplayRange_Horizontal=" << hex << _GPU->DisplayRange_Horizontal << dec << "; X1=" << _GPU->DisplayRange_X1 << "; X2=" << _GPU->DisplayRange_X2 << "; (X2-X1)/8=" << ((_GPU->DisplayRange_X2-_GPU->DisplayRange_X1)>>3);
#endif
					break;
					
				case CTRL_Write_VerticalDisplayRange:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_DISPLAYRANGE
			debug << "\r\n; VerticalDisplayRange";
#endif

					_GPU->DisplayRange_Vertical = Data & 0x1fffff;
					_GPU->DisplayRange_Y1 = _GPU->DisplayRange_Vertical & 0x3ff;
					_GPU->DisplayRange_Y2 = _GPU->DisplayRange_Vertical >> 10;
					
#if defined INLINE_DEBUG_DISPLAYRANGE
			debug << "; DisplayRange_Vertical=" << hex << _GPU->DisplayRange_Vertical << dec << "; Y1=" << _GPU->DisplayRange_Y1 << "; Y2=" << _GPU->DisplayRange_Y2 << "; Y2-Y1=" << (_GPU->DisplayRange_Y2-_GPU->DisplayRange_Y1);
#endif
					break;
					
				case CTRL_Write_DisplayMode:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_DISPLAYMODE
			debug << "\r\n; DisplayMode";
			debug << "; Data=" << hex << Data;
#endif
					_GPU->GPU_CTRL_Read.WIDTH = ( ( Data & 3 ) << 1 ) | ( ( Data >> 6 ) & 1 );
					_GPU->GPU_CTRL_Read.HEIGHT = ( Data >> 2 ) & 1;
					_GPU->GPU_CTRL_Read.VIDEO = ( Data >> 3 ) & 1;
					_GPU->GPU_CTRL_Read.ISRGB24 = ( Data >> 4 ) & 1;
					_GPU->GPU_CTRL_Read.ISINTER = ( Data >> 5 ) & 1;
					
					// the resolution may have changed
					_GPU->UpdateRaster_VARS ();
					
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_DISPLAYMODE
	debug << "; WIDTH=" << _GPU->GPU_CTRL_Read.WIDTH << "; HEIGHT=" << _GPU->GPU_CTRL_Read.HEIGHT << "; VIDEO=" << _GPU->GPU_CTRL_Read.VIDEO << "; ISRGB24=" << _GPU->GPU_CTRL_Read.ISRGB24 << "; ISINTER=" << _GPU->GPU_CTRL_Read.ISINTER;
#endif
					break;
					
				// has main register followed by another 15 mirrors, so in total 16
				case CTRL_Write_GPUInfo:
				case CTRL_Write_GPUInfo+0x1:
				case CTRL_Write_GPUInfo+0x2:
				case CTRL_Write_GPUInfo+0x3:
				case CTRL_Write_GPUInfo+0x4:
				case CTRL_Write_GPUInfo+0x5:
				case CTRL_Write_GPUInfo+0x6:
				case CTRL_Write_GPUInfo+0x7:
				case CTRL_Write_GPUInfo+0x8:
				case CTRL_Write_GPUInfo+0x9:
				case CTRL_Write_GPUInfo+0xa:
				case CTRL_Write_GPUInfo+0xb:
				case CTRL_Write_GPUInfo+0xc:
				case CTRL_Write_GPUInfo+0xd:
				case CTRL_Write_GPUInfo+0xe:
				case CTRL_Write_GPUInfo+0xf:
#ifdef INLINE_DEBUG_WRITE
			debug << "; GPUInfo";
#endif
				
					switch ( Data & 7 )
					{
						// read texture window setting
						case 2:
			
							_GPU->GPU_DATA_Read = ( _GPU->TWY << 15 ) | ( _GPU->TWX << 10 ) | ( _GPU->TWH << 5 ) | ( _GPU->TWW );
							break;
							
						case 3:
#ifdef INLINE_DEBUG_WRITE
			debug << "; DrawAreaTopLeft";
#endif

							// return draw area top left
							_GPU->GPU_DATA_Read = _GPU->DrawArea_TopLeftX | ( _GPU->DrawArea_TopLeftY << 10 );
							break;
							
						case 4:
#ifdef INLINE_DEBUG_WRITE
			debug << "; DrawAreaBottomRight";
#endif

							// return draw area bottom right
							_GPU->GPU_DATA_Read = _GPU->DrawArea_BottomRightX | ( _GPU->DrawArea_BottomRightY << 10 );
							break;
							
						case 5:
#ifdef INLINE_DEBUG_WRITE
			debug << "; DrawOffset";
#endif

							// return draw offset
							_GPU->GPU_DATA_Read = _GPU->DrawArea_OffsetX | ( _GPU->DrawArea_OffsetY << 11 );
							break;
							
						
						case 7:
#ifdef INLINE_DEBUG_WRITE
			debug << "; GPUType";
#endif

							// return GPU type
							_GPU->GPU_DATA_Read = GPU_VERSION;
							break;
						
						// returns zero
						case 8:
							_GPU->GPU_DATA_Read = 0;
							break;
					}
					
					break;
					
				default:
#ifdef INLINE_DEBUG_WRITE
			debug << "; Unknown";
#endif

					// unknown GPU command
					cout << "\nhps1x64 Error: Unknown GPU command @ Cycle#" << dec << *_DebugCycleCount << " PC=" << hex << *_DebugPC << " Command=" << Data << "\n";
					break;
			}
			
			break;
			
			
		default:
#ifdef INLINE_DEBUG_WRITE
			debug << "; Invalid";
#endif
		
			// invalid GPU Register
			cout << "\nhps1x64 ALERT: Unknown GPU WRITE @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << " Data=" << Data << "\n";
			break;
	}
	
}






double GPU::GetCycles_SinceLastPixel ()
{
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// modulo cycle count with cycles in a pixel
	return fmod ( (double) (*_DebugCycleCount), dCyclesPerPixel );
#else
	return RMOD ( (double) (*_DebugCycleCount), dCyclesPerPixel, dPixelsPerCycle );
#endif
}

double GPU::GetCycles_SinceLastPixel ( double dAtCycle )
{
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	return fmod ( dAtCycle, dCyclesPerPixel );
#else
	return RMOD ( dAtCycle, dCyclesPerPixel, dPixelsPerCycle );
#endif
}



double GPU::GetCycles_SinceLastHBlank ()
{
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// get offset from start of scanline
	return fmod ( ( (double) (*_DebugCycleCount) ) + dHBlankArea_Cycles, dCyclesPerScanline );
#else
	return RMOD ( ( (double) (*_DebugCycleCount) ) + dHBlankArea_Cycles, dCyclesPerScanline, dScanlinesPerCycle );
#endif
}

double GPU::GetCycles_SinceLastHBlank ( double dAtCycle )
{
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	return fmod ( dAtCycle + dHBlankArea_Cycles, dCyclesPerScanline );
#else
	return RMOD ( dAtCycle + dHBlankArea_Cycles, dCyclesPerScanline, dScanlinesPerCycle );
#endif
}


double GPU::GetCycles_SinceLastVBlank ()
{
	double dOffsetFromVBlankStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// modulo cycle count with cycles in a frame (2 fields)
	dOffsetFromVBlankStart = fmod ( ( (double) (*_DebugCycleCount) ) + dVBlank1Area_Cycles, dCyclesPerFrame );
#else
	dOffsetFromVBlankStart = RMOD ( ( (double) (*_DebugCycleCount) ) + dVBlank1Area_Cycles, dCyclesPerFrame, dFramesPerCycle );
#endif
	
	if ( dOffsetFromVBlankStart >= dCyclesPerField1 ) dOffsetFromVBlankStart -= dCyclesPerField1;
	
	return dOffsetFromVBlankStart;
}

double GPU::GetCycles_SinceLastVBlank ( double dAtCycle )
{
	double dOffsetFromVBlankStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// modulo cycle count with cycles in a frame (2 fields)
	dOffsetFromVBlankStart = fmod ( dAtCycle + dVBlank1Area_Cycles, dCyclesPerFrame );
#else
	dOffsetFromVBlankStart = RMOD ( dAtCycle + dVBlank1Area_Cycles, dCyclesPerFrame, dFramesPerCycle );
#endif
	
	if ( dOffsetFromVBlankStart >= dCyclesPerField1 ) dOffsetFromVBlankStart -= dCyclesPerField1;
	
	return dOffsetFromVBlankStart;
}


double GPU::GetCycles_ToNextPixel ()
{
	double dOffsetFromPixelStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// modulo cycle count with cycles in a pixel
	dOffsetFromPixelStart = fmod ( (double) (*_DebugCycleCount), dCyclesPerPixel );
#else
	dOffsetFromPixelStart = RMOD ( (double) (*_DebugCycleCount), dCyclesPerPixel, dPixelsPerCycle );
#endif
	
	return dCyclesPerPixel - dOffsetFromPixelStart;
}


double GPU::GetCycles_ToNextPixel ( double dAtCycle )
{
	double dOffsetFromPixelStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// modulo cycle count with cycles in a pixel
	dOffsetFromPixelStart = fmod ( dAtCycle, dCyclesPerPixel );
#else
	dOffsetFromPixelStart = RMOD ( dAtCycle, dCyclesPerPixel, dPixelsPerCycle );
#endif
	
	return dCyclesPerPixel - dOffsetFromPixelStart;
}



double GPU::GetCycles_ToNextHBlank ()
{
	double dOffsetFromHBlankStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// get offset from start of scanline
	//dOffsetFromScanlineStart = fmod ( (double) (*_DebugCycleCount), dCyclesPerScanline );
	dOffsetFromHBlankStart = fmod ( ( (double) (*_DebugCycleCount) ) + dHBlankArea_Cycles, dCyclesPerScanline );
#else
	dOffsetFromHBlankStart = RMOD ( ( (double) (*_DebugCycleCount) ) + dHBlankArea_Cycles, dCyclesPerScanline, dScanlinesPerCycle );
#endif
	
	// return the cycles to the next hblank
	return dCyclesPerScanline - dOffsetFromHBlankStart;
}

double GPU::GetCycles_ToNextHBlank ( double dAtCycle )
{
	double dOffsetFromHBlankStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// get offset from start of scanline
	//dOffsetFromScanlineStart = fmod ( (double) (*_DebugCycleCount), dCyclesPerScanline );
	dOffsetFromHBlankStart = fmod ( dAtCycle + dHBlankArea_Cycles, dCyclesPerScanline );
#else
	dOffsetFromHBlankStart = RMOD ( dAtCycle + dHBlankArea_Cycles, dCyclesPerScanline, dScanlinesPerCycle );
#endif
	
	// return the cycles to the next hblank
	return dCyclesPerScanline - dOffsetFromHBlankStart;
}


double GPU::GetCycles_ToNextVBlank ()
{
	double dOffsetFromFrameStart, dOffsetFromVBlankStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// modulo cycle count with cycles in a frame (2 fields)
	dOffsetFromFrameStart = fmod ( ( (double) (*_DebugCycleCount) ) + dVBlank1Area_Cycles, dCyclesPerFrame );
#else
	dOffsetFromFrameStart = RMOD ( ( (double) (*_DebugCycleCount) ) + dVBlank1Area_Cycles, dCyclesPerFrame, dFramesPerCycle );
#endif
	
	dOffsetFromVBlankStart = dCyclesPerField1 - dOffsetFromFrameStart;
	if ( dOffsetFromVBlankStart <= 0 ) dOffsetFromVBlankStart += dCyclesPerField0;
	
	return dOffsetFromVBlankStart;
}

double GPU::GetCycles_ToNextVBlank ( double dAtCycle )
{
	double dOffsetFromFrameStart, dOffsetFromVBlankStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// modulo cycle count with cycles in a frame (2 fields)
	dOffsetFromFrameStart = fmod ( dAtCycle + dVBlank1Area_Cycles, dCyclesPerFrame );
#else
	dOffsetFromFrameStart = RMOD ( dAtCycle + dVBlank1Area_Cycles, dCyclesPerFrame, dFramesPerCycle );
#endif
	
	dOffsetFromVBlankStart = dCyclesPerField1 - dOffsetFromFrameStart;
	if ( dOffsetFromVBlankStart <= 0 ) dOffsetFromVBlankStart += dCyclesPerField0;
	
	return dOffsetFromVBlankStart;
}




// also need functions to see if currently in hblank or vblank
bool GPU::isHBlank ()
{
	double dOffsetFromHBlankStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// get offset from start of scanline
	//dOffsetFromScanlineStart = fmod ( (double) (*_DebugCycleCount), dCyclesPerScanline );
	dOffsetFromHBlankStart = fmod ( ( (double) (*_DebugCycleCount) ) + dHBlankArea_Cycles, dCyclesPerScanline );
#else
	dOffsetFromHBlankStart = RMOD ( ( (double) (*_DebugCycleCount) ) + dHBlankArea_Cycles, dCyclesPerScanline, dScanlinesPerCycle );
#endif
	
	return ( dOffsetFromHBlankStart < dHBlankArea_Cycles );
}

bool GPU::isHBlank ( double dAtCycle )
{
	double dOffsetFromHBlankStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// get offset from start of scanline
	//dOffsetFromScanlineStart = fmod ( (double) (*_DebugCycleCount), dCyclesPerScanline );
	dOffsetFromHBlankStart = fmod ( dAtCycle + dHBlankArea_Cycles, dCyclesPerScanline );
#else
	dOffsetFromHBlankStart = RMOD ( dAtCycle + dHBlankArea_Cycles, dCyclesPerScanline, dScanlinesPerCycle );
#endif
	
	return ( dOffsetFromHBlankStart < dHBlankArea_Cycles );
}


bool GPU::isVBlank ()
{
	double dOffsetFromVBlankStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// modulo cycle count with cycles in a frame (2 fields)
	dOffsetFromVBlankStart = fmod ( ( (double) (*_DebugCycleCount) ) + dVBlank1Area_Cycles, dCyclesPerFrame );
#else
	dOffsetFromVBlankStart = RMOD ( ( (double) (*_DebugCycleCount) ) + dVBlank1Area_Cycles, dCyclesPerFrame, dFramesPerCycle );
#endif
	
	if ( dOffsetFromVBlankStart >= dCyclesPerField1 ) return ( ( dOffsetFromVBlankStart - dCyclesPerField1 ) < dVBlank0Area_Cycles );

	return ( dOffsetFromVBlankStart < dVBlank1Area_Cycles );
	
	//return ( ( Y_Pixel & ~1 ) == VBlank_Y );
}

bool GPU::isVBlank ( double dAtCycle )
{
	double dOffsetFromVBlankStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// modulo cycle count with cycles in a frame (2 fields)
	dOffsetFromVBlankStart = fmod ( dAtCycle + dVBlank1Area_Cycles, dCyclesPerFrame );
#else
	dOffsetFromVBlankStart = RMOD ( dAtCycle + dVBlank1Area_Cycles, dCyclesPerFrame, dFramesPerCycle );
#endif
	
	if ( dOffsetFromVBlankStart >= dCyclesPerField1 ) return ( ( dOffsetFromVBlankStart - dCyclesPerField1 ) < dVBlank0Area_Cycles );

	return ( dOffsetFromVBlankStart < dVBlank1Area_Cycles );
}




u64 GPU::GetScanline_Count ()
{
	// *** divide ***
	return (u64) ( ( *_DebugCycleCount ) / dCyclesPerScanline );
}


u64 GPU::GetScanline_Number ()
{
	u64 Scanline_Number;
	
	// *** divide ***
	//return ( ( (u64) ( ( *_DebugCycleCount ) / dCyclesPerScanline ) ) % ( (u64) Raster_YMax ) );
	Scanline_Number = ( ( (u64) ( ( *_DebugCycleCount ) / dCyclesPerScanline ) ) % ( (u64) Raster_YMax ) );
	
	// check if we are in field 0 or 1
	if ( Scanline_Number < ScanlinesPerField0 )
	{
		// field 0 //
		return ( Scanline_Number << 1 );
	}
	else
	{
		// field 1 //
		return 1 + ( ( Scanline_Number - ScanlinesPerField0 ) << 1 );
	}
}



double GPU::GetCycles_ToNextScanlineStart ()
{
#ifdef INLINE_DEBUG_EVENT
	debug << "\r\nGPU::GetCycles_ToNextScanline";
#endif

	double dOffsetFromScanlineStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// get offset from start of scanline
	dOffsetFromScanlineStart = fmod ( (double) (*_DebugCycleCount), dCyclesPerScanline );
#else
	dOffsetFromScanlineStart = RMOD ( (double) (*_DebugCycleCount), dCyclesPerScanline, dScanlinesPerCycle );
#endif
	
	// return the offset to the start of the next scanline
	return dCyclesPerScanline - dOffsetFromScanlineStart;
}

double GPU::GetCycles_ToNextScanlineStart ( double dAtCycle )
{
#ifdef INLINE_DEBUG_EVENT
	debug << "\r\nGPU::GetCycles_ToNextScanline";
#endif

	double dOffsetFromScanlineStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// get offset from start of scanline
	dOffsetFromScanlineStart = fmod ( dAtCycle, dCyclesPerScanline );
#else
	dOffsetFromScanlineStart = RMOD ( dAtCycle, dCyclesPerScanline, dScanlinesPerCycle );
#endif
	
	// return the offset to the start of the next scanline
	return dCyclesPerScanline - dOffsetFromScanlineStart;
}



double GPU::GetCycles_ToNextFieldStart ()
{
#ifdef INLINE_DEBUG_DRAWSTART
	debug << "\r\nGPU::GetCycles_ToNextFieldStart";
#endif

	double dOffsetFromFrameStart, dOffsetFromFieldStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// modulo cycle count with cycles in a frame (2 fields)
	dOffsetFromFrameStart = fmod ( (double) (*_DebugCycleCount), dCyclesPerFrame );
#else
	dOffsetFromFrameStart = RMOD ( (double) (*_DebugCycleCount), dCyclesPerFrame, dFramesPerCycle );
#endif
	
	dOffsetFromFieldStart = dCyclesPerField0 - dOffsetFromFrameStart;
	if ( dOffsetFromFieldStart <= 0 ) dOffsetFromFieldStart += dCyclesPerField1;
	
	return dOffsetFromFieldStart;
}

double GPU::GetCycles_ToNextFieldStart ( double dAtCycle )
{
#ifdef INLINE_DEBUG_DRAWSTART
	debug << "\r\nGPU::GetCycles_ToNextFieldStart";
#endif

	double dOffsetFromFrameStart, dOffsetFromFieldStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// modulo cycle count with cycles in a frame (2 fields)
	dOffsetFromFrameStart = fmod ( dAtCycle, dCyclesPerFrame );
#else
	dOffsetFromFrameStart = RMOD ( dAtCycle, dCyclesPerFrame, dFramesPerCycle );
#endif
	
	dOffsetFromFieldStart = dCyclesPerField0 - dOffsetFromFrameStart;
	if ( dOffsetFromFieldStart <= 0 ) dOffsetFromFieldStart += dCyclesPerField1;
	
	return dOffsetFromFieldStart;
}


double GPU::GetCycles_SinceLastScanlineStart ()
{
#ifdef INLINE_DEBUG_EVENT
	debug << "\r\nGPU::GetCycles_ToNextScanline";
#endif

	double dOffsetFromScanlineStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// get offset from start of scanline
	dOffsetFromScanlineStart = fmod ( (double) (*_DebugCycleCount), dCyclesPerScanline );
#else
	dOffsetFromScanlineStart = RMOD ( (double) (*_DebugCycleCount), dCyclesPerScanline, dScanlinesPerCycle );
#endif
	
	// return the offset to the start of the next scanline
	return dOffsetFromScanlineStart;
}

double GPU::GetCycles_SinceLastScanlineStart ( double dAtCycle )
{
#ifdef INLINE_DEBUG_EVENT
	debug << "\r\nGPU::GetCycles_ToNextScanline";
#endif

	double dOffsetFromScanlineStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// get offset from start of scanline
	dOffsetFromScanlineStart = fmod ( dAtCycle, dCyclesPerScanline );
#else
	dOffsetFromScanlineStart = RMOD ( dAtCycle, dCyclesPerScanline, dScanlinesPerCycle );
#endif
	
	// return the offset to the start of the next scanline
	return dOffsetFromScanlineStart;
}



double GPU::GetCycles_SinceLastFieldStart ()
{
#ifdef INLINE_DEBUG_DRAWSTART
	debug << "\r\nGPU::GetCycles_ToNextFieldStart";
#endif

	double dOffsetFromFrameStart, dOffsetFromFieldStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// modulo cycle count with cycles in a frame (2 fields)
	dOffsetFromFrameStart = fmod ( (double) (*_DebugCycleCount), dCyclesPerFrame );
#else
	dOffsetFromFrameStart = RMOD ( (double) (*_DebugCycleCount), dCyclesPerFrame, dFramesPerCycle );
#endif
	
	if ( dOffsetFromFrameStart >= dCyclesPerField0 ) dOffsetFromFrameStart -= dCyclesPerField0;
	
	return dOffsetFromFrameStart;
}

double GPU::GetCycles_SinceLastFieldStart ( double dAtCycle )
{
#ifdef INLINE_DEBUG_DRAWSTART
	debug << "\r\nGPU::GetCycles_ToNextFieldStart";
#endif

	double dOffsetFromFrameStart, dOffsetFromFieldStart;
	
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	// modulo cycle count with cycles in a frame (2 fields)
	dOffsetFromFrameStart = fmod ( dAtCycle, dCyclesPerFrame );
#else
	dOffsetFromFrameStart = RMOD ( dAtCycle, dCyclesPerFrame, dFramesPerCycle );
#endif
	
	if ( dOffsetFromFrameStart >= dCyclesPerField0 ) dOffsetFromFrameStart -= dCyclesPerField0;
	
	return dOffsetFromFrameStart;
}







void GPU::UpdateRaster_VARS ( void )
{
#ifdef INLINE_DEBUG_VARS
	debug << "\r\n->UpdateRaster_VARS";
#endif

	u32 HBlank_PixelCount;
	bool SettingsChange;
	
	// assume settings will not change
	SettingsChange = false;
	

	// if the display settings are going to change, then need to mark cycle at which they changed
	if ( HBlank_X != HBlank_X_LUT [ GPU_CTRL_Read.WIDTH ] ||
		VBlank_Y != VBlank_Y_LUT [ GPU_CTRL_Read.VIDEO ] ||
		Raster_XMax != Raster_XMax_LUT [ GPU_CTRL_Read.VIDEO ] [ GPU_CTRL_Read.WIDTH ] ||
		Raster_YMax != Raster_YMax_LUT [ GPU_CTRL_Read.VIDEO ] )
	{
#ifdef INLINE_DEBUG_VARS
	debug << "\r\nChange; StartCycle=" << dec << *_DebugCycleCount;
#endif

		// ***TODO*** need to update timers before clearing the pixel counts //
		
		// update timers 0 and 1 using settings from before they change
		Timers::_TIMERS->UpdateTimer ( 0 );
		Timers::_TIMERS->UpdateTimer ( 1 );
		
		// at end of routine, calibrate timers

		//RasterChange_StartCycle = *_DebugCycleCount;
		SettingsChange = true;
	}


	HBlank_X = HBlank_X_LUT [ GPU_CTRL_Read.WIDTH ];

#ifdef INLINE_DEBUG_VARS
	debug << "; HBlank_X = " << dec << HBlank_X;
#endif

	VBlank_Y = VBlank_Y_LUT [ GPU_CTRL_Read.VIDEO ];

#ifdef INLINE_DEBUG_VARS
	debug << "; VBlank_Y = " << VBlank_Y;
#endif

	Raster_XMax = Raster_XMax_LUT [ GPU_CTRL_Read.VIDEO ] [ GPU_CTRL_Read.WIDTH ];

#ifdef INLINE_DEBUG_VARS
	debug << "; Raster_XMax = " << Raster_XMax;
#endif

	Raster_YMax = Raster_YMax_LUT [ GPU_CTRL_Read.VIDEO ];

#ifdef INLINE_DEBUG_VARS
	debug << "; Raster_YMax = " << Raster_YMax;
#endif

	CyclesPerPixel_INC = CyclesPerPixel_INC_Lookup [ GPU_CTRL_Read.VIDEO ] [ GPU_CTRL_Read.WIDTH ];
	dCyclesPerPixel = CyclesPerPixel_Lookup [ GPU_CTRL_Read.VIDEO ] [ GPU_CTRL_Read.WIDTH ];
	dPixelsPerCycle = PixelsPerCycle_Lookup [ GPU_CTRL_Read.VIDEO ] [ GPU_CTRL_Read.WIDTH ];
	
	// check if ntsc or pal
	if ( GPU_CTRL_Read.VIDEO )
	{
		// is PAL //
		dCyclesPerScanline = PAL_CyclesPerScanline;
		dCyclesPerFrame = PAL_CyclesPerFrame;
		dCyclesPerField0 = PAL_CyclesPerField_Even;
		dCyclesPerField1 = PAL_CyclesPerField_Odd;
		
		dScanlinesPerCycle = PAL_ScanlinesPerCycle;
		dFramesPerCycle = PAL_FramesPerCycle;
		dFieldsPerCycle0 = PAL_FieldsPerCycle_Even;
		dFieldsPerCycle1 = PAL_FieldsPerCycle_Odd;

		
		dDisplayArea_Cycles = PAL_DisplayAreaCycles;
		dVBlank0Area_Cycles = PAL_CyclesPerVBlank_Even;
		dVBlank1Area_Cycles = PAL_CyclesPerVBlank_Odd;
		
		// also need the scanlines per field
		ScanlinesPerField0 = PAL_ScanlinesPerField_Even;
		ScanlinesPerField1 = PAL_ScanlinesPerField_Odd;
	}
	else
	{
		// is NTSC //
		dCyclesPerScanline = NTSC_CyclesPerScanline;
		dCyclesPerFrame = NTSC_CyclesPerFrame;
		dCyclesPerField0 = NTSC_CyclesPerField_Even;
		dCyclesPerField1 = NTSC_CyclesPerField_Odd;
		
		dScanlinesPerCycle = NTSC_ScanlinesPerCycle;
		dFramesPerCycle = NTSC_FramesPerCycle;
		dFieldsPerCycle0 = NTSC_FieldsPerCycle_Even;
		dFieldsPerCycle1 = NTSC_FieldsPerCycle_Odd;
		
		
		dDisplayArea_Cycles = NTSC_DisplayAreaCycles;
		dVBlank0Area_Cycles = NTSC_CyclesPerVBlank_Even;
		dVBlank1Area_Cycles = NTSC_CyclesPerVBlank_Odd;
		
		// also need the scanlines per field
		ScanlinesPerField0 = NTSC_ScanlinesPerField_Even;
		ScanlinesPerField1 = NTSC_ScanlinesPerField_Odd;
	}

	// get number of pixels in hblank area
	HBlank_PixelCount = Raster_XMax - HBlank_X;
	
	// multiply by cycles per pixel
	dHBlankArea_Cycles = ( (double) HBlank_PixelCount ) * dCyclesPerPixel;

	
#ifdef INLINE_DEBUG_VARS
	debug << "; CyclesPerPixel_INC = " << CyclesPerPixel_INC;
#endif

	// the Display_Width is the HBlank_X
	Display_Width = HBlank_X;
	
	// the Display_Height is the VBlank_Y if interlaced, otherwise it is VBlank_Y/2
	if ( GPU_CTRL_Read.ISINTER )
	{
		Display_Height = VBlank_Y;
	}
	else
	{
		Display_Height = ( VBlank_Y >> 1 );
	}
	

	// check if the settings changed
	if ( SettingsChange )
	{
		// settings changed //
		
		
		// set the current scanline number
		// this does not appear to be used anymore
		// before I got this wrong, but this is a better idea
		Y_Pixel = GetScanline_Number ();
		
		// this needs to be updated whenever the settings change
		Update_LCF ();
		
		// get the next event for gpu if settings change
		GetNextEvent ();
		GetNextEvent_V ();
		
		// calibrate timers 0 and 1
		Timers::_TIMERS->CalibrateTimer ( 0 );
		Timers::_TIMERS->CalibrateTimer ( 1 );
		
		// if doing calibrate, then also must do update of next event
		Timers::_TIMERS->Get_NextEvent ( 0, NextEvent_Cycle );
		Timers::_TIMERS->Get_NextEvent ( 1, NextEvent_Cycle );
	}

	// get the new cycle of the next event after updating the vars
	//GetNextEvent ();

#ifdef INLINE_DEBUG_VARS
	debug << "->UpdateRaster_VARS";
#endif
}


void GPU::Draw_Screen ()
{
	static const int c_iVisibleArea_StartY [] = { c_iVisibleArea_StartY_Pixel_NTSC, c_iVisibleArea_StartY_Pixel_PAL };
	static const int c_iVisibleArea_EndY [] = { c_iVisibleArea_EndY_Pixel_NTSC, c_iVisibleArea_EndY_Pixel_PAL };
	
	// so the max viewable width for PAL is 3232/4-544/4 = 808-136 = 672
	// so the max viewable height for PAL is 292-34 = 258
	
	// actually, will initially start with a 1 pixel border based on screen width/height and then will shift if something is off screen

	// need to know visible range of screen for NTSC and for PAL (each should be different)
	// NTSC visible y range is usually from 16-256 (0x10-0x100) (height=240)
	// PAL visible y range is usually from 35-291 (0x23-0x123) (height=256)
	// NTSC visible x range is.. I don't know. start with from about gpu cycle#544 to about gpu cycle#3232 (must use gpu cycles since res changes)
	s32 VisibleArea_StartX, VisibleArea_EndX, VisibleArea_StartY, VisibleArea_EndY, VisibleArea_Width, VisibleArea_Height;
	
	// there the frame buffer pixel, and then there's the screen buffer pixel
	u32 Pixel16, Pixel32_0, Pixel32_1;
	u64 Pixel64;
	
	u32 runx_1, runx_2;
	
	// this allows you to calculate horizontal pixels
	u32 GPU_CyclesPerPixel;
	
	
	Pixel_24bit_Format Pixel24;
	
	//u32 Pixel, FramePixel, CurrentPixel24;
	//s32 DrawWidth, DrawHeight, Pixel_X, Pixel_Y;
	//s32 StartX, StartY, EndX, EndY;
	//u32 OutputBuffer_X, OutputBuffer_Y, TV_X, TV_Y;
	
	s32 FramePixel_X, FramePixel_Y;
	
	// need to know where to draw the actual image at
	s32 Draw_StartX, Draw_StartY, Draw_EndX, Draw_EndY, Draw_Width, Draw_Height;
	
	u32 XResolution, YResolution;
	
	union
	{
		u32 *ptr_pixelbuffer;
		u64 *ptr_pixelbuffer64;
	};
	
	union
	{
		u16 *ptr_vram;
		u32 *ptr_vram32;
		u64 *ptr_vram64;
	};
	
	s32 TopBorder_Height, BottomBorder_Height, LeftBorder_Width, RightBorder_Width;
	s32 current_x, current_y, current_width, current_height, current_size, current_xmax, current_ymax;
	
#ifdef _ENABLE_SSE2
	__m128i vRedMask, vGreenMask, vBlueMask;
	__m128i vPixelsIn, vPixelsOut1, vPixelsOut2, vZero = _mm_setzero_si128 ();
	vRedMask = _mm_set1_epi32 ( 0x1f );
	vGreenMask = _mm_set1_epi32 ( 0x3e0 );
	vBlueMask = _mm_set1_epi32 ( 0x7c00 );
#endif
	
	// GPU cycles per pixel depends on width
	GPU_CyclesPerPixel = c_iGPUCyclesPerPixel [ GPU_CTRL_Read.WIDTH ];
	
	// get the pixel to start and stop drawing at
	Draw_StartX = DisplayRange_X1 / GPU_CyclesPerPixel;
	Draw_EndX = DisplayRange_X2 / GPU_CyclesPerPixel;
	Draw_StartY = DisplayRange_Y1;
	Draw_EndY = DisplayRange_Y2;
	
	Draw_Width = Draw_EndX - Draw_StartX;
	Draw_Height = Draw_EndY - Draw_StartY;
	
	// get the pixel to start and stop at for visible area
	VisibleArea_StartX = c_iVisibleArea_StartX_Cycle / GPU_CyclesPerPixel;
	VisibleArea_EndX = c_iVisibleArea_EndX_Cycle / GPU_CyclesPerPixel;
	
	// visible area start and end y depends on pal/ntsc
	VisibleArea_StartY = c_iVisibleArea_StartY [ GPU_CTRL_Read.VIDEO ];
	VisibleArea_EndY = c_iVisibleArea_EndY [ GPU_CTRL_Read.VIDEO ];
	
	VisibleArea_Width = VisibleArea_EndX - VisibleArea_StartX;
	VisibleArea_Height = VisibleArea_EndY - VisibleArea_StartY;
	
	if ( GPU_CTRL_Read.ISINTER && GPU_CTRL_Read.HEIGHT )
	{
		// 480i mode //
		
		VisibleArea_EndY += Draw_Height;
		VisibleArea_Height += Draw_Height;
		
		Draw_EndY += Draw_Height;
		
		Draw_Height <<= 1;
	}
	
#ifdef INLINE_DEBUG_DRAW_SCREEN
	debug << "\r\nGPU::Draw_Screen; GPU_CyclesPerPixel=" << dec << GPU_CyclesPerPixel << " Draw_StartX=" << Draw_StartX << " Draw_EndX=" << Draw_EndX;
	debug << "\r\nDraw_StartY=" << Draw_StartY << " Draw_EndY=" << Draw_EndY << " VisibleArea_StartX=" << VisibleArea_StartX;
	debug << "\r\nVisibleArea_EndX=" << VisibleArea_EndX << " VisibleArea_StartY=" << VisibleArea_StartY << " VisibleArea_EndY=" << VisibleArea_EndY;
#endif

	// *** new stuff *** //

	//FramePixel = 0;
	ptr_pixelbuffer = PixelBuffer;
	
	if ( !GPU_CTRL_Read.DEN )
	{
		BottomBorder_Height = VisibleArea_EndY - Draw_EndY;
		LeftBorder_Width = Draw_StartX - VisibleArea_StartX;
		TopBorder_Height = Draw_StartY - VisibleArea_StartY;
		RightBorder_Width = VisibleArea_EndX - Draw_EndX;
		
		if ( BottomBorder_Height < 0 ) BottomBorder_Height = 0;
		if ( LeftBorder_Width < 0 ) LeftBorder_Width = 0;
		
		//cout << "\n(before)Left=" << dec << LeftBorder_Width << " Bottom=" << BottomBorder_Height << " Draw_Width=" << Draw_Width << " VisibleArea_Width=" << VisibleArea_Width;
		
		
		current_ymax = Draw_Height + BottomBorder_Height;
		current_xmax = Draw_Width + LeftBorder_Width;
		
		// make suree that ymax and xmax are not greater than the size of visible area
		if ( current_xmax > VisibleArea_Width )
		{
			// entire image is not on the screen, so take from left border and recalc xmax //

			LeftBorder_Width -= ( current_xmax - VisibleArea_Width );
			if ( LeftBorder_Width < 0 ) LeftBorder_Width = 0;
			current_xmax = Draw_Width + LeftBorder_Width;
			
			// make sure again we do not draw past the edge of screen
			if ( current_xmax > VisibleArea_Width ) current_xmax = VisibleArea_Width;
		}
		
		if ( current_ymax > VisibleArea_Height )
		{
			BottomBorder_Height -= ( current_ymax - VisibleArea_Height );
			if ( BottomBorder_Height < 0 ) BottomBorder_Height = 0;
			current_ymax = Draw_Height + BottomBorder_Height;
			
			// make sure again we do not draw past the edge of screen
			if ( current_ymax > VisibleArea_Height ) current_ymax = VisibleArea_Height;
		}
		
		//cout << "\n(after)Left=" << dec << LeftBorder_Width << " Bottom=" << BottomBorder_Height << " Draw_Width=" << Draw_Width << " VisibleArea_Width=" << VisibleArea_Width;
		//cout << "\n(after2)current_xmax=" << current_xmax << " current_ymax=" << current_ymax;
		
		current_y = BottomBorder_Height;
		
		// put in bottom border
		current_size = BottomBorder_Height * VisibleArea_Width;
		current_x = 0;
#ifdef _ENABLE_SSE2
		for ( ; current_x < ( current_size - 3 ); current_x += 4 )
		{
			//*ptr_pixelbuffer64++ = 0;
			_mm_store_si128 ( (__m128i*) ptr_pixelbuffer, vZero );
			ptr_pixelbuffer += 4;
		}
#endif
		for ( ; current_x < ( current_size - 1 ); current_x += 2 )
		{
			*ptr_pixelbuffer64++ = 0;
		}
		for ( ; current_x < current_size; current_x++ )
		{
			*ptr_pixelbuffer++ = 0;
		}

		
		// put in screen
		
		FramePixel_Y = ScreenArea_TopLeftY + Draw_Height - 1;
		FramePixel_X = ScreenArea_TopLeftX;
		
		//for ( current_y = 0; current_y < Draw_Height; current_y++ )
		for ( ; current_y < current_ymax; current_y++ )
		{
			// put in the left border
			current_x = 0;
#ifdef _ENABLE_SSE2
			for ( ; current_x < ( LeftBorder_Width - 3 ); current_x += 4 )
			{
				//*ptr_pixelbuffer64++ = 0;
				_mm_storeu_si128 ( (__m128i*) ptr_pixelbuffer, vZero );
				ptr_pixelbuffer += 4;
			}
#endif
			for ( ; current_x < ( LeftBorder_Width - 1 ); current_x += 2 )
			{
				*ptr_pixelbuffer64++ = 0;
			}
			for ( ; current_x < LeftBorder_Width; current_x++ )
			{
				*ptr_pixelbuffer++ = 0;
			}
			
			// *** important note *** this wraps around the VRAM
			//ptr_vram = & (VRAM [ FramePixel_X + ( FramePixel_Y << 10 ) ]);
			ptr_vram = & (VRAM [ FramePixel_X + ( ( FramePixel_Y & FrameBuffer_YMask ) << 10 ) ]);
			
			// put in screeen pixels
			if ( !GPU_CTRL_Read.ISRGB24 )
			{
#ifdef _ENABLE_SSE2
				for ( ; current_x < ( current_xmax - 7 ); current_x += 8 )
				{
					//*ptr_pixelbuffer64++ = 0;
					vPixelsIn = _mm_loadu_si128 ((__m128i const*) ptr_vram);
					
					vPixelsOut1 = _mm_unpacklo_epi16 ( vPixelsIn, vZero );
					vPixelsOut2 = _mm_unpackhi_epi16 ( vPixelsIn, vZero );
					
					vPixelsOut1 = _mm_or_si128 ( _mm_slli_epi32 ( _mm_and_si128 ( vPixelsOut1, vRedMask ), 3 ), _mm_or_si128 ( _mm_slli_epi32 ( _mm_and_si128 ( vPixelsOut1, vGreenMask ), 6 ), _mm_slli_epi32 ( _mm_and_si128 ( vPixelsOut1, vBlueMask ), 9 ) ) );
					vPixelsOut2 = _mm_or_si128 ( _mm_slli_epi32 ( _mm_and_si128 ( vPixelsOut2, vRedMask ), 3 ), _mm_or_si128 ( _mm_slli_epi32 ( _mm_and_si128 ( vPixelsOut2, vGreenMask ), 6 ), _mm_slli_epi32 ( _mm_and_si128 ( vPixelsOut2, vBlueMask ), 9 ) ) );
					
					_mm_storeu_si128 ( (__m128i*) ptr_pixelbuffer, vPixelsOut1 );
					ptr_pixelbuffer += 4;
					_mm_storeu_si128 ( (__m128i*) ptr_pixelbuffer, vPixelsOut2 );
					ptr_pixelbuffer += 4;
					
					ptr_vram += 8;
				}
#endif
				for ( ; current_x < ( current_xmax - 1 ); current_x += 2 )
				{
					Pixel64 = *ptr_vram32++;
					Pixel64 |= Pixel64 << 16;
					
					// the previous pixel conversion is wrong, this is the correct one
					*ptr_pixelbuffer64++ = ( ( Pixel64 & 0x1f0000001fLL ) << 3 ) | ( ( Pixel64 & 0x3e0000003e0LL ) << 6 ) | ( ( Pixel64 & 0x7c0000007c00LL ) << 9 ) |
											( ( Pixel64 & 0x1c0000001cLL ) >> 2 ) | ( ( Pixel64 & 0x38000000380LL ) << 1 ) | ( ( Pixel64 & 0x700000007000LL ) << 4 );
				}
				for ( ; current_x < current_xmax; current_x++ )
				{
					Pixel16 = *ptr_vram++;
					
					// the previous pixel conversion is wrong
					//Pixel32_0 = ( ( Pixel16 & 0x1f ) << 3 ) | ( ( Pixel16 & 0x3e0 ) << 6 ) | ( ( Pixel16 & 0x7c00 ) << 9 );
					Pixel32_0 = ( ( Pixel16 & 0x1f ) << 3 ) | ( ( Pixel16 & 0x3e0 ) << 6 ) | ( ( Pixel16 & 0x7c00 ) << 9 ) |
								( ( Pixel16 & 0x1c ) >> 2 ) | ( ( Pixel16 & 0x380 ) << 1 ) | ( ( Pixel16 & 7000 ) << 4 );
					*ptr_pixelbuffer++ = Pixel32_0;
				}
			}
			else
			{
				for ( ; current_x < current_xmax; current_x += 2 )
				{
					Pixel24.Pixel0 = *ptr_vram++;
					Pixel24.Pixel1 = *ptr_vram++;
					Pixel24.Pixel2 = *ptr_vram++;
					
					// draw first pixel
					Pixel32_0 = ( ((u32)Pixel24.Red0) ) | ( ((u32)Pixel24.Green0) << 8 ) | ( ((u32)Pixel24.Blue0) << 16 );
					
					// draw second pixel
					Pixel32_1 = ( ((u32)Pixel24.Red1) ) | ( ((u32)Pixel24.Green1) << 8 ) | ( ((u32)Pixel24.Blue1) << 16 );
					
					*ptr_pixelbuffer++ = Pixel32_0;
					*ptr_pixelbuffer++ = Pixel32_1;
				}
			}
			
			// put in right border
#ifdef _ENABLE_SSE2
			for ( ; current_x < ( VisibleArea_Width - 3 ); current_x += 4 )
			{
				//*ptr_pixelbuffer64++ = 0;
				_mm_storeu_si128 ( (__m128i*) ptr_pixelbuffer, vZero );
				ptr_pixelbuffer += 4;
			}
#endif
			for ( ; current_x < ( VisibleArea_Width - 1 ); current_x += 2 )
			{
				*ptr_pixelbuffer64++ = 0;
			}
			for ( ; current_x < VisibleArea_Width; current_x++ )
			{
				*ptr_pixelbuffer++ = 0;
			}
			
			// go to next line in frame buffer
			FramePixel_Y--;
		}
		
		// put in top border
		//current_size = TopBorder_Height * VisibleArea_Width;
		current_size = ( VisibleArea_Height - current_y ) * VisibleArea_Width;
		current_x = 0;
		for ( ; current_x < ( current_size - 1 ); current_x += 2 )
		{
			*ptr_pixelbuffer64++ = 0;
		}
		for ( ; current_x < current_size; current_x++ )
		{
			*ptr_pixelbuffer++ = 0;
		}
	}
	else
	{
		// display disabled //
		
		current_size = VisibleArea_Height * VisibleArea_Width;
		current_x = 0;
#ifdef _ENABLE_SSE2
		for ( ; current_x < ( current_size - 3 ); current_x += 4 )
		{
			//*ptr_pixelbuffer64++ = 0;
			_mm_store_si128 ( (__m128i*) ptr_pixelbuffer, vZero );
			ptr_pixelbuffer += 4;
		}
#endif
		for ( ; current_x < ( current_size - 1 ); current_x += 2 )
		{
			*ptr_pixelbuffer64++ = 0;
		}
		for ( ; current_x < current_size; current_x++ )
		{
			*ptr_pixelbuffer++ = 0;
		}

	}
	
		
	// *** output of pixel buffer to screen *** //

	// make this the current window we are drawing to
	DisplayOutput_Window->OpenGL_MakeCurrentWindow ();
	
	//glPixelZoom ( (float)MainProgramWindow_Width / (float)DrawWidth, (float)MainProgramWindow_Height / (float)DrawHeight );
	//glDrawPixels ( DrawWidth, DrawHeight, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid*) PixelBuffer );
	glPixelZoom ( (float)MainProgramWindow_Width / (float)VisibleArea_Width, (float)MainProgramWindow_Height / (float)VisibleArea_Height );
	glDrawPixels ( VisibleArea_Width, VisibleArea_Height, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid*) PixelBuffer );
	
	// update screen
	DisplayOutput_Window->FlipScreen ();
	
	// this is no longer the current window we are drawing to
	DisplayOutput_Window->OpenGL_ReleaseWindow ();
}


void GPU::Draw_FrameBuffer ()
{
	u32 Pixel, FramePixel;
	s32 Pixel_X, Pixel_Y;
	
	// make this the current window we are drawing to
	FrameBuffer_DebugWindow->OpenGL_MakeCurrentWindow ();
	
	FramePixel = 0;
	
	/////////////////////////////////////////////////////////////////
	// Draw contents of frame buffer
	for ( Pixel_Y = FrameBuffer_Height - 1; Pixel_Y >= 0; Pixel_Y-- )
	{
		for ( Pixel_X = 0; Pixel_X < FrameBuffer_Width; Pixel_X++ )
		{
			Pixel = VRAM [ Pixel_X + ( Pixel_Y << 10 ) ];
			PixelBuffer [ FramePixel++ ] = ( ( Pixel & 0x1f ) << ( 3 ) ) | ( ( (Pixel >> 5) & 0x1f ) << ( 3 + 8 ) ) | ( ( (Pixel >> 10) & 0x1f ) << ( 3 + 16 ) );
		}
	}
	
	//glPixelZoom ( (float)MainProgramWindow_Width / (float)DrawWidth, (float)MainProgramWindow_Height / (float)DrawHeight );
	glDrawPixels ( FrameBuffer_Width, FrameBuffer_Height, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid*) PixelBuffer );

	// update screen
	FrameBuffer_DebugWindow->FlipScreen ();
	
	// this is no longer the current window we are drawing to
	FrameBuffer_DebugWindow->OpenGL_ReleaseWindow ();
}




void GPU::DMA_Read ( u32* Data, int ByteReadCount )
{
	u32 pix0, pix1;
#ifdef INLINE_DEBUG_DMA_READ
	debug << "\r\n\r\nDMA_Read " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; Data = " << hex << Data [ 0 ];
#endif

	Data [ 0 ] = ProcessDataRegRead ();
}

// will use "55" to determine where you need termination code of 0x55555555
static const u32 GPU_SizeOfCommands [ 256 ] = 
{
	/////////////////////////////////
	// GPU commands
	1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	// 0x00-0x0f
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	// 0x10-0x1f
	
	////////////////////////////////
	// Polygon commands
	4, 4, 4, 4, 7, 7, 7, 7, 5, 5, 5, 5, 9, 9, 9, 9,	// 0x20-0x2f
	6, 6, 6, 6, 9, 9, 9, 9, 8, 8, 8, 8, 12, 12, 12, 12,	// 0x30-0x3f
	
	///////////////////////////////////
	// Line commands
	3, 3, 3, 3, 3, 3, 3, 3, 55, 55, 55, 55, 55, 55, 55, 55,	// 0x40-0x4f
	4, 4, 4, 4, 4, 4, 4, 4, 66, 66, 66, 66, 66, 66, 66, 66,	// 0x50-0x5f
	
	/////////////////////////////////
	// Sprite commands
	3, 3, 3, 3, 4, 4, 4, 4, 2, 2, 2, 2, 0, 0, 0, 0,	// 0x60-0x6f
	2, 2, 2, 2, 3, 3, 3, 3, 2, 2, 2, 2, 3, 3, 3, 3,	// 0x70-0x7f
	
	//////////////////////////////////
	// Transfer commands
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,	// 0x80-0x8f
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,	// 0x90-0x9f
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,	// 0xa0-0xaf
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,	// 0xb0-0xbf
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,	// 0xc0-0xcf
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,	// 0xd0-0xdf
	
	///////////////////////////////////
	// Environment commands
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	// 0xe0-0xef
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1	};	// 0xf0-0xff




void GPU::DMA_Write ( u32* Data, int ByteWriteCount )
{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "\r\n\r\nDMA_Write " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; Data = " << hex << Data [ 0 ];
#endif

	ProcessDataRegWrite ( Data [ 0 ] );
}


void GPU::DMA_WriteBlock ( u32* Data, u32 BS )
{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "\r\n\r\nDMA_WriteBlock " << hex << setw ( 8 ) << *_DebugPC << " BS=" << BS << " " << dec << *_DebugCycleCount << hex << "; Data= ";		//<< hex << Data [ 0 ];
	for ( int i = 0; i < BS; i++ ) debug << " " << (u32) Data [ i ];
	debug << "\r\n";
#endif

	for ( int i = 0; i < BS; i++ )
	{
		ProcessDataRegWrite ( Data [ i ] );
	}
}


void GPU::ExecuteGPUBuffer ()
{
	static const u32 CyclesPerSpritePixel = 2;
	
	command_tge = Buffer [ 0 ].Command & 1;
	command_abe = ( Buffer [ 0 ].Command >> 1 ) & 1;
	
	// clear busy cycles
	BusyCycles = 0;

	////////////////////////////////////////
	// Check what the command is
	switch ( Buffer [ 0 ].Command /* & 0xfc */ )
	{
		////////////////////////////////////////
		// GPU Commands
		
		case 0x00:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME
			debug << "\r\nMust be ResetGPUArgBuffer";
#endif

			BufferSize = 0;
			break;
		
		case 0x01:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME
			debug << "\r\nClearCache";
#endif
			///////////////////////////////////////////
			// clear cache
			break;
			
		case 0x02:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME
			debug << "\r\nFrameBufferRectangle";
#endif
			///////////////////////////////////////////
			// frame buffer rectangle draw
			GetBGR ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetHW ( Buffer [ 2 ] );
			
#if defined INLINE_DEBUG_EXECUTE
			debug << dec << " x=" << x << " y=" << y << " h=" << h << " w=" << w << hex << " bgr=" << bgr;
#endif

			Draw_FrameBufferRectangle_02 ();

#if defined INLINE_DEBUG_EXECUTE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
			
		case 0x03:
		
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME
			debug << "\r\nUnknown Command " << hex << (u32) Buffer [ 0 ].Command;
#endif

			// *** Unknown *** //
			// command doesn't seem to have a noticeable effect
			//cout << "\nhps1x64 WARNING: Unknown GPU command: " << hex << (u32) Buffer [ 0 ].Command << "\n";
			
			break;
			
		
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18:
		case 0x19:
		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d:
		case 0x1e:
		
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME
			debug << "\r\nUnknown Command " << hex << (u32) Buffer [ 0 ].Command;
#endif

			// *** Unknown *** //
			
#ifdef VERBOSE_GPU
			cout << "\nhps1x64 WARNING: Unknown GPU command: " << hex << (u32) Buffer [ 0 ].Command << "\n";
#endif
			
			break;
			
			
		case 0x1f:
		
			// Interrupt Request //
			cout << "\nhps1x64 WARNING: GPU Interrupt Requested\n";
			
			break;
			
		//////////////////////////////////////////////////
		// Polygon commands
			
		case 0x20:
		case 0x21:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_MONO
			debug << "\r\nMonochromeTriangle Abe=0";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif

			///////////////////////////////////////////////
			// monochrome 3-point polygon
			// *** TODO *** BGR should be passed as 24-bit color value, not 15-bit
			GetBGR ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetXY1 ( Buffer [ 2 ] );
			GetXY2 ( Buffer [ 3 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_MONO
			debug << "\r\n" << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2 << hex << " bgr=" << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_TRIANGLE_MONO
			Draw_MonoTriangle_20_t <0> ();
#else
			Draw_MonoTriangle_20 ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_MONO
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;

			
		case 0x22:
		case 0x23:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_MONO
			debug << "\r\nMonochromeTriangle Abe=1";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif
		
			///////////////////////////////////////////////
			// monochrome 3-point polygon
			GetBGR ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetXY1 ( Buffer [ 2 ] );
			GetXY2 ( Buffer [ 3 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_MONO
			debug << "\r\n" << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2 << hex << " bgr=" << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_TRIANGLE_MONO
			Draw_MonoTriangle_20_t <1> ();
#else
			Draw_MonoTriangle_20 ();
#endif

#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_MONO
			debug << ";BusyCycles=" << BusyCycles;
#endif

			break;

			
		
		
			
		case 0x24:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nTexturedTriangle";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif
			///////////////////////////////////////////////
			// textured 3-point polygon
			GetBGR24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			GetUV1 ( Buffer [ 4 ] );
			GetTPAGE ( Buffer [ 4 ] );
			GetXY2 ( Buffer [ 5 ] );
			GetUV2 ( Buffer [ 6 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\n" << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2 << hex << " bgr=" << bgr;
			debug << "\r\n" << dec << " u0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2;
			debug << "\r\n" << dec << "clut_x=" << clut_x << " clut_y=" << clut_y << " tx=" << tpage_tx << " ty=" << tpage_ty << " tp=" << tpage_tp << " abr=" << tpage_abr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_TRIANGLE_TEXTURE
			Draw_TextureTriangle_24_t <0,0> ();
#else
			Draw_TextureTriangle_24 ();
#endif
			
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;


		case 0x25:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nTexturedTriangle Abe=0 Tge=1";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif
			///////////////////////////////////////////////
			// textured 3-point polygon
			GetBGR24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			GetUV1 ( Buffer [ 4 ] );
			GetTPAGE ( Buffer [ 4 ] );
			GetXY2 ( Buffer [ 5 ] );
			GetUV2 ( Buffer [ 6 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2 << " clutx=" << clut_x << " cluty =" << clut_y << " bgr=" << hex << bgr;
			debug << "\r\n" << dec << " u0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_TRIANGLE_TEXTURE
			Draw_TextureTriangle_24_t <0,1> ();
#else
			Draw_TextureTriangle_24 ();
#endif
			
			// *** testing busy cycles ***
			//BusyCycles = 100;
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		case 0x26:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nTexturedTriangle Abe=1 Tge=0";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif
			///////////////////////////////////////////////
			// textured 3-point polygon
			GetBGR24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			GetUV1 ( Buffer [ 4 ] );
			GetTPAGE ( Buffer [ 4 ] );
			GetXY2 ( Buffer [ 5 ] );
			GetUV2 ( Buffer [ 6 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2 << " clutx=" << clut_x << " cluty =" << clut_y << " bgr=" << hex << bgr;
			debug << "\r\n" << dec << " u0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_TRIANGLE_TEXTURE
			Draw_TextureTriangle_24_t <1,0> ();
#else
			Draw_TextureTriangle_24 ();
#endif
			
			// *** testing busy cycles ***
			//BusyCycles = 100;
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		
		case 0x27:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nTexturedTriangle Abe=1 Tge=1";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif
			///////////////////////////////////////////////
			// textured 3-point polygon
			GetBGR24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			GetUV1 ( Buffer [ 4 ] );
			GetTPAGE ( Buffer [ 4 ] );
			GetXY2 ( Buffer [ 5 ] );
			GetUV2 ( Buffer [ 6 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2 << " clutx=" << clut_x << " cluty =" << clut_y << " bgr=" << hex << bgr;
			debug << "\r\n" << dec << " u0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_TRIANGLE_TEXTURE
			Draw_TextureTriangle_24_t <1,1> ();
#else
			Draw_TextureTriangle_24 ();
#endif

			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		
			
		case 0x28:
		case 0x29:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_MONO
			debug << "\r\nMonochromeRectangle Abe=0";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif
			//////////////////////////////////////////////
			// monochrome 4-point polygon
			GetBGR ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetXY1 ( Buffer [ 2 ] );
			GetXY2 ( Buffer [ 3 ] );
			GetXY3 ( Buffer [ 4 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_MONO
			debug << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2 << " x3=" << x3 << " y3=" << y3 << " bgr=" << hex << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_RECTANGLE_MONO
			Draw_MonoRectangle_28_t <0> ();
#else
			Draw_MonoRectangle_28 ();
#endif
			
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_MONO
			debug << ";BusyCycles=" << BusyCycles << ";DrawArea_TopLeftX=" << DrawArea_TopLeftX << ";DrawArea_OffsetX=" << DrawArea_OffsetX << ";DrawArea_TopLeftY=" << DrawArea_TopLeftY << ";DrawArea_OffsetY=" << DrawArea_OffsetY;
#endif
			break;
			
			
		
		
		case 0x2a:
		case 0x2b:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_MONO
			debug << "\r\nMonochromeRectangle Abe=1";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif
			//////////////////////////////////////////////
			// monochrome 4-point polygon
			GetBGR ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetXY1 ( Buffer [ 2 ] );
			GetXY2 ( Buffer [ 3 ] );
			GetXY3 ( Buffer [ 4 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_MONO
			debug << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2 << " x3=" << x3 << " y3=" << y3 << " bgr=" << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_RECTANGLE_MONO
			Draw_MonoRectangle_28_t <1> ();
#else
			Draw_MonoRectangle_28 ();
#endif
			
			// *** testing busy cycles ***
			//BusyCycles = 100;
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_MONO
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		
		
		
			
		case 0x2c:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nTexturedRectangle";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif

			////////////////////////////////////////////
			// textured 4-point polygon
			GetBGR24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			GetTPAGE ( Buffer [ 4 ] );
			GetUV1 ( Buffer [ 4 ] );
			GetXY2 ( Buffer [ 5 ] );
			GetUV2 ( Buffer [ 6 ] );
			GetXY3 ( Buffer [ 7 ] );
			GetUV3 ( Buffer [ 8 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2 << " x3=" << x3 << " y3=" << y3;
			debug << "\r\nu0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2 << " u3=" << u3 << " v3=" << v3;
			debug << "\r\ntpage_tx=" << tpage_tx << " tpage_ty=" << tpage_ty << " tpage_abr=" << tpage_abr << " tpage_tp=" << tpage_tp << " clut_x=" << clut_x << " clut_y=" << clut_y << " bgr=" << hex << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_RECTANGLE_TEXTURE
			Draw_TextureRectangle_2c_t <0,0> ();
#else
			Draw_TextureRectangle_2c ();
#endif

			// *** testing busy cycles ***
			//BusyCycles = 100;
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
		case 0x2d:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nTexturedRectangle Abe=0 Tge=1";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif

			////////////////////////////////////////////
			// textured 4-point polygon
			GetBGR24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			GetTPAGE ( Buffer [ 4 ] );
			GetUV1 ( Buffer [ 4 ] );
			GetXY2 ( Buffer [ 5 ] );
			GetUV2 ( Buffer [ 6 ] );
			GetXY3 ( Buffer [ 7 ] );
			GetUV3 ( Buffer [ 8 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2 << " x3=" << x3 << " y3=" << y3;
			debug << "\r\nu0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2 << " u3=" << u3 << " v3=" << v3;
			debug << "\r\ntpage_tx=" << tpage_tx << " tpage_ty=" << tpage_ty << " tpage_abr=" << tpage_abr << " tpage_tp=" << tpage_tp << " clut_x=" << clut_x << " clut_y=" << clut_y << " bgr=" << hex << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_RECTANGLE_TEXTURE
			Draw_TextureRectangle_2c_t <0,1> ();
#else
			Draw_TextureRectangle_2c ();
#endif

			// *** testing busy cycles ***
			//BusyCycles = 100;
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		
		case 0x2e:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nTexturedRectangle Abe=1 Tge=0";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif

			////////////////////////////////////////////
			// textured 4-point polygon
			GetBGR24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			GetTPAGE ( Buffer [ 4 ] );
			GetUV1 ( Buffer [ 4 ] );
			GetXY2 ( Buffer [ 5 ] );
			GetUV2 ( Buffer [ 6 ] );
			GetXY3 ( Buffer [ 7 ] );
			GetUV3 ( Buffer [ 8 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2 << " x3=" << x3 << " y3=" << y3;
			debug << "\r\nu0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2 << " u3=" << u3 << " v3=" << v3;
			debug << "\r\ntpage_tx=" << tpage_tx << " tpage_ty=" << tpage_ty << " tpage_abr=" << tpage_abr << " tpage_tp=" << tpage_tp << " clut_x=" << clut_x << " clut_y=" << clut_y << " bgr=" << hex << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_RECTANGLE_TEXTURE
			Draw_TextureRectangle_2c_t <1,0> ();
#else
			Draw_TextureRectangle_2c ();
#endif

			// *** testing busy cycles ***
			//BusyCycles = 100;
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		
		case 0x2f:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nTexturedRectangle Abe=1 Tge=1";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif

			////////////////////////////////////////////
			// textured 4-point polygon
			GetBGR24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			GetTPAGE ( Buffer [ 4 ] );
			GetUV1 ( Buffer [ 4 ] );
			GetXY2 ( Buffer [ 5 ] );
			GetUV2 ( Buffer [ 6 ] );
			GetXY3 ( Buffer [ 7 ] );
			GetUV3 ( Buffer [ 8 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2 << " x3=" << x3 << " y3=" << y3;
			debug << "\r\nu0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2 << " u3=" << u3 << " v3=" << v3;
			debug << "\r\ntpage_tx=" << tpage_tx << " tpage_ty=" << tpage_ty << " tpage_abr=" << tpage_abr << " tpage_tp=" << tpage_tp << " clut_x=" << clut_x << " clut_y=" << clut_y << " bgr=" << hex << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_RECTANGLE_TEXTURE
			Draw_TextureRectangle_2c_t <1,1> ();
#else
			Draw_TextureRectangle_2c ();
#endif

			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;

		
		case 0x30:
		case 0x31:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SHADED
			debug << "\r\nShadedTriangle Abe=0";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif

			///////////////////////////////////////
			// gradated 3-point polygon
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetBGR1_24 ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			GetBGR2_24 ( Buffer [ 4 ] );
			GetXY2 ( Buffer [ 5 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SHADED
			debug << dec << " x0=" << x0 << " y0=" << y0 << " bgr0=" << bgr0 << " x1=" << x1 << " y1=" << y1 << " bgr1=" << bgr1 << " x2=" << x2 << " y2=" << y2 << " bgr2=" << bgr2;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_TRIANGLE_GRADIENT
			Draw_GradientTriangle_30_t <0> ();
#else
			Draw_GradientTriangle_30 ();
#endif
			
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SHADED
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
		
		
		case 0x32:
		case 0x33:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SHADED
			debug << "\r\nShadedTriangle Abe=1";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif

			///////////////////////////////////////
			// gradated 3-point polygon
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetBGR1_24 ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			GetBGR2_24 ( Buffer [ 4 ] );
			GetXY2 ( Buffer [ 5 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SHADED
			debug << dec << " x0=" << x0 << " y0=" << y0 << " bgr0=" << hex << bgr0 << " x1=" << dec << x1 << " y1=" << y1 << " bgr1=" << hex << bgr1 << " x2=" << dec << x2 << " y2=" << y2 << " bgr2=" << hex << bgr2;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_TRIANGLE_GRADIENT
			Draw_GradientTriangle_30_t <1> ();
#else
			Draw_GradientTriangle_30 ();
#endif
			
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SHADED
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		
		
			
		case 0x34:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nShadedTexturedTriangle";
#endif
			////////////////////////////////////////
			// gradated textured 3-point polygon
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetBGR1_24 ( Buffer [ 3 ] );
			GetXY1 ( Buffer [ 4 ] );
			GetTPAGE ( Buffer [ 5 ] );
			GetUV1 ( Buffer [ 5 ] );
			GetBGR2_24 ( Buffer [ 6 ] );
			GetXY2 ( Buffer [ 7 ] );
			GetUV2 ( Buffer [ 8 ] );
			
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " bgr0=" << bgr0 << " x1=" << x1 << " y1=" << y1 << " bgr1=" << bgr1 << " x2=" << x2 << " y2=" << y2 << " bgr2=" << bgr2;
			debug << "\r\nu0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2;
			debug << "\r\ntpage_tx=" << tpage_tx << " tpage_ty=" << tpage_ty << " tpage_abr=" << tpage_abr << " tpage_tp=" << tpage_tp << " clut_x=" << clut_x << " clut_y=" << clut_y;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
			debug << dec << " X=" << TextureWindow_X << " Y=" << TextureWindow_Y << " Height=" << TextureWindow_Height << " Width=" << TextureWindow_Width;
#endif

#ifdef USE_TEMPLATES_TRIANGLE_TEXTUREGRADIENT
			Draw_TextureGradientTriangle_34_t <0,0> ();
#else
			Draw_TextureGradientTriangle_34 ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;

			
		case 0x35:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nShadedTexturedTriangle Abe=0 Tge=1";
#endif
			////////////////////////////////////////
			// gradated textured 3-point polygon
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetBGR1_24 ( Buffer [ 3 ] );
			GetXY1 ( Buffer [ 4 ] );
			GetTPAGE ( Buffer [ 5 ] );
			GetUV1 ( Buffer [ 5 ] );
			GetBGR2_24 ( Buffer [ 6 ] );
			GetXY2 ( Buffer [ 7 ] );
			GetUV2 ( Buffer [ 8 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " bgr0=" << bgr0 << " x1=" << x1 << " y1=" << y1 << " bgr1=" << bgr1 << " x2=" << x2 << " y2=" << y2 << " bgr2=" << bgr2;
			debug << "\r\nu0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2;
			debug << "\r\ntpage_tx=" << tpage_tx << " tpage_ty=" << tpage_ty << " tpage_abr=" << tpage_abr << " tpage_tp=" << tpage_tp << " clut_x=" << clut_x << " clut_y=" << clut_y;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
			debug << dec << " X=" << TextureWindow_X << " Y=" << TextureWindow_Y << " Height=" << TextureWindow_Height << " Width=" << TextureWindow_Width;
#endif

#ifdef USE_TEMPLATES_TRIANGLE_TEXTUREGRADIENT
			Draw_TextureGradientTriangle_34_t <0,1> ();
#else
			Draw_TextureGradientTriangle_34 ();
#endif
			
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;

		
		case 0x36:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nShadedTexturedTriangle Abe=1 Tge=0";
#endif
			////////////////////////////////////////
			// gradated textured 3-point polygon
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetBGR1_24 ( Buffer [ 3 ] );
			GetXY1 ( Buffer [ 4 ] );
			GetTPAGE ( Buffer [ 5 ] );
			GetUV1 ( Buffer [ 5 ] );
			GetBGR2_24 ( Buffer [ 6 ] );
			GetXY2 ( Buffer [ 7 ] );
			GetUV2 ( Buffer [ 8 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " bgr0=" << bgr0 << " x1=" << x1 << " y1=" << y1 << " bgr1=" << bgr1 << " x2=" << x2 << " y2=" << y2 << " bgr2=" << bgr2;
			debug << "\r\nu0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2;
			debug << "\r\ntpage_tx=" << tpage_tx << " tpage_ty=" << tpage_ty << " tpage_abr=" << tpage_abr << " tpage_tp=" << tpage_tp << " clut_x=" << clut_x << " clut_y=" << clut_y;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
			debug << dec << " X=" << TextureWindow_X << " Y=" << TextureWindow_Y << " Height=" << TextureWindow_Height << " Width=" << TextureWindow_Width;
#endif

#ifdef USE_TEMPLATES_TRIANGLE_TEXTUREGRADIENT
			Draw_TextureGradientTriangle_34_t <1,0> ();
#else
			Draw_TextureGradientTriangle_34 ();
#endif
			
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;

		
		case 0x37:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nShadedTexturedTriangle Abe=1 Tge=1";
#endif
			////////////////////////////////////////
			// gradated textured 3-point polygon
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetBGR1_24 ( Buffer [ 3 ] );
			GetXY1 ( Buffer [ 4 ] );
			GetTPAGE ( Buffer [ 5 ] );
			GetUV1 ( Buffer [ 5 ] );
			GetBGR2_24 ( Buffer [ 6 ] );
			GetXY2 ( Buffer [ 7 ] );
			GetUV2 ( Buffer [ 8 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " bgr0=" << bgr0 << " x1=" << x1 << " y1=" << y1 << " bgr1=" << bgr1 << " x2=" << x2 << " y2=" << y2 << " bgr2=" << bgr2;
			debug << "\r\nu0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2;
			debug << "\r\ntpage_tx=" << tpage_tx << " tpage_ty=" << tpage_ty << " tpage_abr=" << tpage_abr << " tpage_tp=" << tpage_tp << " clut_x=" << clut_x << " clut_y=" << clut_y;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
			debug << dec << " X=" << TextureWindow_X << " Y=" << TextureWindow_Y << " Height=" << TextureWindow_Height << " Width=" << TextureWindow_Width;
#endif

#ifdef USE_TEMPLATES_TRIANGLE_TEXTUREGRADIENT
			Draw_TextureGradientTriangle_34_t <1,1> ();
#else
			Draw_TextureGradientTriangle_34 ();
#endif
			
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;

		

		
		case 0x38:
		case 0x39:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SHADED
			debug << "\r\nShadedRectangle Abe=0";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif

			//////////////////////////////////////////
			// gradated 4-point polygon
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetBGR1_24 ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			GetBGR2_24 ( Buffer [ 4 ] );
			GetXY2 ( Buffer [ 5 ] );
			GetBGR3_24 ( Buffer [ 6 ] );
			GetXY3 ( Buffer [ 7 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SHADED
			debug << dec << " x0=" << x0 << " y0=" << y0 << " bgr0=" << bgr0 << " x1=" << x1 << " y1=" << y1 << " bgr1=" << bgr1 << " x2=" << x2 << " y2=" << y2 << " bgr2=" << bgr2 << " x3=" << x3 << " y3=" << y3 << " bgr3=" << bgr3;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_RECTANGLE_GRADIENT
			Draw_GradientRectangle_38_t <0> ();
#else
			Draw_GradientRectangle_38 ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SHADED
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
		
		
		
		case 0x3a:
		case 0x3b:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SHADED
			debug << "\r\nShadedRectangle Abe=1";
			debug << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif

			//////////////////////////////////////////
			// gradated 4-point polygon
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetBGR1_24 ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			GetBGR2_24 ( Buffer [ 4 ] );
			GetXY2 ( Buffer [ 5 ] );
			GetBGR3_24 ( Buffer [ 6 ] );
			GetXY3 ( Buffer [ 7 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SHADED
			debug << dec << " x0=" << x0 << " y0=" << y0 << " bgr0=" << bgr0 << " x1=" << x1 << " y1=" << y1 << " bgr1=" << bgr1 << " x2=" << x2 << " y2=" << y2 << " bgr2=" << bgr2 << " x3=" << x3 << " y3=" << y3 << " bgr3=" << bgr3;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif

#ifdef USE_TEMPLATES_RECTANGLE_GRADIENT
			Draw_GradientRectangle_38_t <1> ();
#else
			Draw_GradientRectangle_38 ();
#endif

#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SHADED
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		

		
		case 0x3c:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nShadedTexturedRectangle";
#endif
			/////////////////////////////////////////
			// gradated textured 4-point polygon
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetBGR1_24 ( Buffer [ 3 ] );
			GetXY1 ( Buffer [ 4 ] );
			GetTPAGE ( Buffer [ 5 ] );
			GetUV1 ( Buffer [ 5 ] );
			GetBGR2_24 ( Buffer [ 6 ] );
			GetXY2 ( Buffer [ 7 ] );
			GetUV2 ( Buffer [ 8 ] );
			GetBGR3_24 ( Buffer [ 9 ] );
			GetXY3 ( Buffer [ 10 ] );
			GetUV3 ( Buffer [ 11 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " bgr0=" << bgr0 << " x1=" << x1 << " y1=" << y1 << " bgr1=" << bgr1 << " x2=" << x2 << " y2=" << y2 << " bgr2=" << bgr2 << " x3=" << x3 << " y3=" << y3 << " bgr3=" << bgr3;
			debug << "\r\nu0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2 << " u3=" << u3 << " v3=" << v3;
			debug << "\r\ntpage_tx=" << tpage_tx << " tpage_ty=" << tpage_ty << " tpage_abr=" << tpage_abr << " tpage_tp=" << tpage_tp << " clut_x=" << clut_x << " clut_y=" << clut_y << " bgr=" << hex << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
			debug << dec << " X=" << TextureWindow_X << " Y=" << TextureWindow_Y << " Height=" << TextureWindow_Height << " Width=" << TextureWindow_Width;
#endif

#ifdef USE_TEMPLATES_RECTANGLE_TEXTUREGRADIENT
			Draw_TextureGradientRectangle_3c_t <0,0> ();
#else
			Draw_TextureGradientRectangle_3c ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
			
		case 0x3d:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nShadedTexturedRectangle Abe=0 Tge=1";
#endif
			/////////////////////////////////////////
			// gradated textured 4-point polygon
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetBGR1_24 ( Buffer [ 3 ] );
			GetXY1 ( Buffer [ 4 ] );
			GetTPAGE ( Buffer [ 5 ] );
			GetUV1 ( Buffer [ 5 ] );
			GetBGR2_24 ( Buffer [ 6 ] );
			GetXY2 ( Buffer [ 7 ] );
			GetUV2 ( Buffer [ 8 ] );
			GetBGR3_24 ( Buffer [ 9 ] );
			GetXY3 ( Buffer [ 10 ] );
			GetUV3 ( Buffer [ 11 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " bgr0=" << bgr0 << " x1=" << x1 << " y1=" << y1 << " bgr1=" << bgr1 << " x2=" << x2 << " y2=" << y2 << " bgr2=" << bgr2 << " x3=" << x3 << " y3=" << y3 << " bgr3=" << bgr3;
			debug << "\r\nu0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2 << " u3=" << u3 << " v3=" << v3;
			debug << "\r\ntpage_tx=" << tpage_tx << " tpage_ty=" << tpage_ty << " tpage_abr=" << tpage_abr << " tpage_tp=" << tpage_tp << " clut_x=" << clut_x << " clut_y=" << clut_y << " bgr=" << hex << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
			debug << dec << " X=" << TextureWindow_X << " Y=" << TextureWindow_Y << " Height=" << TextureWindow_Height << " Width=" << TextureWindow_Width;
#endif

#ifdef USE_TEMPLATES_RECTANGLE_TEXTUREGRADIENT
			Draw_TextureGradientRectangle_3c_t <0,1> ();
#else
			Draw_TextureGradientRectangle_3c ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		
		case 0x3e:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nShadedTexturedRectangle Abe=1 Tge=0";
#endif
			/////////////////////////////////////////
			// gradated textured 4-point polygon
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetBGR1_24 ( Buffer [ 3 ] );
			GetXY1 ( Buffer [ 4 ] );
			GetTPAGE ( Buffer [ 5 ] );
			GetUV1 ( Buffer [ 5 ] );
			GetBGR2_24 ( Buffer [ 6 ] );
			GetXY2 ( Buffer [ 7 ] );
			GetUV2 ( Buffer [ 8 ] );
			GetBGR3_24 ( Buffer [ 9 ] );
			GetXY3 ( Buffer [ 10 ] );
			GetUV3 ( Buffer [ 11 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " bgr0=" << bgr0 << " x1=" << x1 << " y1=" << y1 << " bgr1=" << bgr1 << " x2=" << x2 << " y2=" << y2 << " bgr2=" << bgr2 << " x3=" << x3 << " y3=" << y3 << " bgr3=" << bgr3;
			debug << "\r\nu0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2 << " u3=" << u3 << " v3=" << v3;
			debug << "\r\ntpage_tx=" << tpage_tx << " tpage_ty=" << tpage_ty << " tpage_abr=" << tpage_abr << " tpage_tp=" << tpage_tp << " clut_x=" << clut_x << " clut_y=" << clut_y << " bgr=" << hex << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
			debug << dec << " X=" << TextureWindow_X << " Y=" << TextureWindow_Y << " Height=" << TextureWindow_Height << " Width=" << TextureWindow_Width;
#endif

#ifdef USE_TEMPLATES_RECTANGLE_TEXTUREGRADIENT
			Draw_TextureGradientRectangle_3c_t <1,0> ();
#else
			Draw_TextureGradientRectangle_3c ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		
		case 0x3f:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TEXTURE
			debug << "\r\nShadedTexturedRectangle Abe=1 Tge=1";
#endif
			/////////////////////////////////////////
			// gradated textured 4-point polygon
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV0 ( Buffer [ 2 ] );
			GetBGR1_24 ( Buffer [ 3 ] );
			GetXY1 ( Buffer [ 4 ] );
			GetTPAGE ( Buffer [ 5 ] );
			GetUV1 ( Buffer [ 5 ] );
			GetBGR2_24 ( Buffer [ 6 ] );
			GetXY2 ( Buffer [ 7 ] );
			GetUV2 ( Buffer [ 8 ] );
			GetBGR3_24 ( Buffer [ 9 ] );
			GetXY3 ( Buffer [ 10 ] );
			GetUV3 ( Buffer [ 11 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " bgr0=" << bgr0 << " x1=" << x1 << " y1=" << y1 << " bgr1=" << bgr1 << " x2=" << x2 << " y2=" << y2 << " bgr2=" << bgr2 << " x3=" << x3 << " y3=" << y3 << " bgr3=" << bgr3;
			debug << "\r\nu0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1 << " u2=" << u2 << " v2=" << v2 << " u3=" << u3 << " v3=" << v3;
			debug << "\r\ntpage_tx=" << tpage_tx << " tpage_ty=" << tpage_ty << " tpage_abr=" << tpage_abr << " tpage_tp=" << tpage_tp << " clut_x=" << clut_x << " clut_y=" << clut_y << " bgr=" << hex << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
			debug << dec << " X=" << TextureWindow_X << " Y=" << TextureWindow_Y << " Height=" << TextureWindow_Height << " Width=" << TextureWindow_Width;
#endif

#ifdef USE_TEMPLATES_RECTANGLE_TEXTUREGRADIENT
			Draw_TextureGradientRectangle_3c_t <1,1> ();
#else
			Draw_TextureGradientRectangle_3c ();
#endif

#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TEXTURE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		
		

		////////////////////////////////////////
		// Line commands
		
		case 0x40:
		case 0x41:
		case 0x44:
		case 0x45:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_LINE
			debug << "\r\nMonochromeLine";
#endif
			//////////////////////////////////////////
			// monochrome line
			GetBGR ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetXY1 ( Buffer [ 2 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " bgr=" << hex << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif
			
			Draw_MonoLine_40 ();
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
		case 0x42:
		case 0x43:
		case 0x46:
		case 0x47:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_LINE
			debug << "\r\nMonochromeLine Abe=1";
#endif
			//////////////////////////////////////////
			// monochrome line
			GetBGR ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetXY1 ( Buffer [ 2 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " bgr=" << hex << bgr;
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif
			
			Draw_MonoLine_40 ();
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		
			
		case 0x48:
		case 0x49:
		case 0x4c:
		case 0x4d:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_LINE
			debug << "\r\nMonochromePolyline";
#endif
			/////////////////////////////////////////////
			// monochrome polyline
			
			// get color
			GetBGR ( Buffer [ 0 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << " bgr=" << hex << bgr;
#endif
			
			// draw until termination code is reached
			for ( int i = 1; ( Buffer [ i + 1 ].Value & 0xf000f000 ) != 0x50005000; i++ )
			{
				GetXY0 ( Buffer [ i ] );
				GetXY1 ( Buffer [ i + 1 ] );
				
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1;
#endif
				
				Draw_MonoLine_40 ();
			}

#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;

			
		case 0x4a:
		case 0x4b:
		case 0x4e:
		case 0x4f:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_LINE
			debug << "\r\nMonochromePolyline Abe=1";
#endif
			/////////////////////////////////////////////
			// monochrome polyline
			
			// get color
			GetBGR ( Buffer [ 0 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << " bgr=" << hex << bgr;
#endif
			
			// draw until termination code is reached
			for ( int i = 1; ( Buffer [ i + 1 ].Value & 0xf000f000 ) != 0x50005000; i++ )
			{
				GetXY0 ( Buffer [ i ] );
				GetXY1 ( Buffer [ i + 1 ] );
				
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << dec << " x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1;
#endif
				
				Draw_MonoLine_40 ();
			}

#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << dec << "\r\nDrawArea: OffsetX=" << DrawArea_OffsetX << " OffsetY=" << DrawArea_OffsetY << " TopLeftX=" << DrawArea_TopLeftX << " TopLeftY=" << DrawArea_TopLeftY << " BottomRightX=" << DrawArea_BottomRightX << " BottomRightY=" << DrawArea_BottomRightY;
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		
			
		case 0x50:
		case 0x51:
		case 0x54:
		case 0x55:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_LINE
			debug << "\r\nShadedLine";
#endif
			///////////////////////////////////////////////
			// gradated line
			// *** TODO ***
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetBGR1_24 ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			
			Draw_ShadedLine_50 ();
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;

			
		case 0x52:
		case 0x53:
		case 0x56:
		case 0x57:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_LINE
			debug << "\r\nShadedLine Abe=1";
#endif
			///////////////////////////////////////////////
			// gradated line
			// *** TODO ***
			GetBGR0_24 ( Buffer [ 0 ] );
			GetXY0 ( Buffer [ 1 ] );
			GetBGR1_24 ( Buffer [ 2 ] );
			GetXY1 ( Buffer [ 3 ] );
			
			Draw_ShadedLine_50 ();
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
			
		case 0x58:
		case 0x59:
		case 0x5c:
		case 0x5d:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_LINE
			debug << "\r\nShadedPolyline";
#endif
			/////////////////////////////////////////////////
			// gradated line polyline
			
			// draw until termination code is reached
			for ( int i = 0; ( Buffer [ i + 2 ].Value & 0xf000f000 ) != 0x50005000; i += 2 )
			{
				// get color
				GetBGR0_24 ( Buffer [ i ] );
				
				// get coord
				GetXY0 ( Buffer [ i + 1 ] );
				
				// get color
				GetBGR1_24 ( Buffer [ i + 2 ] );

				// get coord
				GetXY1 ( Buffer [ i + 3 ] );
				
				Draw_ShadedLine_50 ();
			}
			
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
		
		case 0x5a:
		case 0x5b:
		case 0x5e:
		case 0x5f:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_LINE
			debug << "\r\nShadedPolyline Abe=1";
#endif
			/////////////////////////////////////////////////
			// gradated line polyline
			
			// draw until termination code is reached
			for ( int i = 0; ( Buffer [ i + 2 ].Value & 0xf000f000 ) != 0x50005000; i += 2 )
			{
				// get color
				GetBGR0_24 ( Buffer [ i ] );
				
				// get coord
				GetXY0 ( Buffer [ i + 1 ] );
				
				// get color
				GetBGR1_24 ( Buffer [ i + 2 ] );

				// get coord
				GetXY1 ( Buffer [ i + 3 ] );
				
				Draw_ShadedLine_50 ();
			}
			
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_LINE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
			
		///////////////////////////////////////
		// Sprite commands
			
		case 0x60:
		case 0x61:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nRectangle";
#endif
			////////////////////////////////////////////
			// rectangle
			GetBGR ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetHW ( Buffer [ 2 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " h=" << h << " w=" << w << hex << " bgr=" << Buffer [ 0 ].Value;
#endif

#ifdef USE_TEMPLATES_RECTANGLE
			Draw_Rectangle_60_t <0> ();
#else
			Draw_Rectangle_60 ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;

			
		case 0x62:
		case 0x63:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nRectangle Abe=1";
#endif
			////////////////////////////////////////////
			// rectangle
			GetBGR ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetHW ( Buffer [ 2 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " h=" << h << " w=" << w << hex << " bgr=" << Buffer [ 0 ].Value;
#endif

#ifdef USE_TEMPLATES_RECTANGLE
			Draw_Rectangle_60_t <1> ();
#else
			Draw_Rectangle_60 ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;

			
		case 0x64:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nSprite";
#endif
			////////////////////////////////////////////
			// Sprite
			GetBGR24 ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV ( Buffer [ 2 ] );
			GetHW ( Buffer [ 3 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " u=" << u << " v=" << v << " h=" << h << " w=" << w << " bgr=" << bgr << " clut_x=" << clut_x << " clut_y=" << clut_y;
			debug << "\r\nTP=" << GPU_CTRL_Read.TP << " ABR=" << GPU_CTRL_Read.ABR;
#endif

#ifdef USE_TEMPLATES_SPRITE
			Draw_Sprite_64_t <0,0> ();
#else
			Draw_Sprite_64 ();
#endif
			
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
		case 0x65:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nSprite Abe=0 Tge=1";
#endif
			////////////////////////////////////////////
			// Sprite
			GetBGR24 ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV ( Buffer [ 2 ] );
			GetHW ( Buffer [ 3 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " u=" << u << " v=" << v << " h=" << h << " w=" << w << " bgr=" << bgr << " clut_x=" << clut_x << " clut_y=" << clut_y;
			debug << "\r\nTP=" << GPU_CTRL_Read.TP << " ABR=" << GPU_CTRL_Read.ABR;
#endif

#ifdef USE_TEMPLATES_SPRITE
			Draw_Sprite_64_t <0,1> ();
#else
			Draw_Sprite_64 ();
#endif

#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
		case 0x66:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nSprite Abe=1 Tge=0";
#endif
			////////////////////////////////////////////
			// Sprite
			GetBGR24 ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV ( Buffer [ 2 ] );
			GetHW ( Buffer [ 3 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " u=" << u << " v=" << v << " h=" << h << " w=" << w << " bgr=" << bgr << " clut_x=" << clut_x << " clut_y=" << clut_y;
			debug << "\r\nTP=" << GPU_CTRL_Read.TP << " ABR=" << GPU_CTRL_Read.ABR;
#endif

#ifdef USE_TEMPLATES_SPRITE
			Draw_Sprite_64_t <1,0> ();
#else
			Draw_Sprite_64 ();
#endif

#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;


		case 0x67:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nSprite Abe=1 Tge=1";
#endif
			////////////////////////////////////////////
			// Sprite
			GetBGR24 ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV ( Buffer [ 2 ] );
			GetHW ( Buffer [ 3 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " u=" << u << " v=" << v << " h=" << h << " w=" << w << " bgr=" << bgr << " clut_x=" << clut_x << " clut_y=" << clut_y;
			debug << "\r\nTP=" << GPU_CTRL_Read.TP << " ABR=" << GPU_CTRL_Read.ABR;
#endif

#ifdef USE_TEMPLATES_SPRITE
			Draw_Sprite_64_t <1,1> ();
#else
			Draw_Sprite_64 ();
#endif

#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
			
			
		case 0x68:
		case 0x69:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nPixel";
#endif
			///////////////////////////////////////////
			// dot
			GetBGR ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			
			Draw_Pixel_68 ();
			
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
			
		case 0x6a:
		case 0x6b:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nPixel Abe=1";
#endif
			///////////////////////////////////////////
			// dot
			GetBGR ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			
			Draw_Pixel_68 ();
			
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
			
		case 0x6c:
		
			// note is copied from martin korth psx documentation
			// GP0(6Ch) - Textured Rectangle, 1x1 (nonsense), opaque, texture-blending

			cout << "\nhps1x64 WARNING: GPU Command 0x6c : 1x1 textured rectangle; opaque; texture-blending\n";
			break;
			
		case 0x6d:
		
			// note is copied from martin korth psx documentation
			// Textured Rectangle, 1x1 (nonsense), opaque, raw-texture
			
			cout << "\nhps1x64 WARNING: GPU Command 0x6d : 1x1 textured rectangle; opaque; raw-texture\n";
			break;
			
			
		case 0x6e:
		
			// note is copied from martin korth psx documentation
			// Textured Rectangle, 1x1 (nonsense), semi-transp, texture-blending
			
			cout << "\nhps1x64 WARNING: GPU Command 0x6e : 1x1 textured rectangle; semi-transparent; texture-blending\n";
			break;
			
			
		case 0x6f:
		
			// note is copied from martin korth psx documentation
			// Textured Rectangle, 1x1 (nonsense), semi-transp, raw-texture
			
			cout << "\nhps1x64 WARNING: GPU Command 0x6f : 1x1 textured rectangle; semi-transparent; raw-texture\n";
			break;
			
		case 0x70:
		case 0x71:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nRectangle8";
#endif
			/////////////////////////////////////////
			// 8x8 rectangle
			GetBGR ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			
#ifdef USE_TEMPLATES_RECTANGLE8
			Draw_Rectangle8x8_70_t <0> ();
#else
			Draw_Rectangle8x8_70 ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
			
		case 0x72:
		case 0x73:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nRectangle8 Abe=1";
#endif
			/////////////////////////////////////////
			// 8x8 rectangle
			GetBGR ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			
#ifdef USE_TEMPLATES_RECTANGLE8
			Draw_Rectangle8x8_70_t <1> ();
#else
			Draw_Rectangle8x8_70 ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
		
		
			
		case 0x74:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nSprite8";
#endif

			///////////////////////////////////////////
			// 8x8 sprite
			GetBGR24 ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV ( Buffer [ 2 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " u=" << u << " v=" << v << " clut_x=" << clut_x << " clut_y=" << clut_y << hex << " bgr=" << bgr;
			debug << "\r\nTP=" << GPU_CTRL_Read.TP << " ABR=" << GPU_CTRL_Read.ABR;
#endif

#ifdef USE_TEMPLATES_SPRITE8
			Draw_Sprite8x8_74_t <0,0> ();
#else
			Draw_Sprite8x8_74 ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;



		case 0x75:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nSprite8 Abe=0 Tge=1";
#endif

			///////////////////////////////////////////
			// 8x8 sprite
			GetBGR24 ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV ( Buffer [ 2 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " u=" << u << " v=" << v << " clut_x=" << clut_x << " clut_y=" << clut_y << hex << " bgr=" << bgr;
			debug << "\r\nTP=" << GPU_CTRL_Read.TP << " ABR=" << GPU_CTRL_Read.ABR;
#endif


#ifdef USE_TEMPLATES_SPRITE8
			Draw_Sprite8x8_74_t <0,1> ();
#else
			Draw_Sprite8x8_74 ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
			
		case 0x76:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nSprite8 Abe=1 Tge=0";
#endif

			///////////////////////////////////////////
			// 8x8 sprite
			GetBGR24 ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV ( Buffer [ 2 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " u=" << u << " v=" << v << " clut_x=" << clut_x << " clut_y=" << clut_y << hex << " bgr=" << bgr;
			debug << "\r\nTP=" << GPU_CTRL_Read.TP << " ABR=" << GPU_CTRL_Read.ABR;
#endif

#ifdef USE_TEMPLATES_SPRITE8
			Draw_Sprite8x8_74_t <1,0> ();
#else
			Draw_Sprite8x8_74 ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;



		case 0x77:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nSprite8 Abe=1 Tge=1";
#endif

			///////////////////////////////////////////
			// 8x8 sprite
			GetBGR24 ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV ( Buffer [ 2 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " u=" << u << " v=" << v << " clut_x=" << clut_x << " clut_y=" << clut_y << hex << " bgr=" << bgr;
			debug << "\r\nTP=" << GPU_CTRL_Read.TP << " ABR=" << GPU_CTRL_Read.ABR;
#endif

#ifdef USE_TEMPLATES_SPRITE8
			Draw_Sprite8x8_74_t <1,1> ();
#else
			Draw_Sprite8x8_74 ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
			
			
		case 0x78:
		case 0x79:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nRectangle16";
#endif
			/////////////////////////////////////////
			// 16x16 rectangle
			GetBGR ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			
#ifdef USE_TEMPLATES_RECTANGLE16
			Draw_Rectangle16x16_78_t <0> ();
#else
			Draw_Rectangle16x16_78 ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
			
		case 0x7a:
		case 0x7b:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nRectangle16 Abe=1";
#endif
			/////////////////////////////////////////
			// 16x16 rectangle
			GetBGR ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			
#ifdef USE_TEMPLATES_RECTANGLE16
			Draw_Rectangle16x16_78_t <1> ();
#else
			Draw_Rectangle16x16_78 ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
			
			
		case 0x7c:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nSprite16";
#endif
			//////////////////////////////////////////
			// 16x16 sprite
			GetBGR24 ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV ( Buffer [ 2 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " u=" << u << " v=" << v << " clut_x=" << clut_x << " clut_y=" << clut_y << hex << " bgr=" << bgr;
			debug << "\r\nTP=" << GPU_CTRL_Read.TP << " ABR=" << GPU_CTRL_Read.ABR << " CycleCount=" << dec << *_DebugCycleCount;
#endif

#ifdef USE_TEMPLATES_SPRITE16
			Draw_Sprite16x16_7c_t <0,0> ();
#else
			Draw_Sprite16x16_7c ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;


		case 0x7d:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nSprite16 Abe=0 Tge=1";
#endif
			//////////////////////////////////////////
			// 16x16 sprite
			GetBGR24 ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV ( Buffer [ 2 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " u=" << u << " v=" << v << " clut_x=" << clut_x << " clut_y=" << clut_y << hex << " bgr=" << bgr;
			debug << "\r\nTP=" << GPU_CTRL_Read.TP << " ABR=" << GPU_CTRL_Read.ABR << " CycleCount=" << dec << *_DebugCycleCount;
#endif

#ifdef USE_TEMPLATES_SPRITE16
			Draw_Sprite16x16_7c_t <0,1> ();
#else
			Draw_Sprite16x16_7c ();
#endif

#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
			
		case 0x7e:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nSprite16 Abe=1 Tge=0";
#endif
			//////////////////////////////////////////
			// 16x16 sprite
			GetBGR24 ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV ( Buffer [ 2 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " u=" << u << " v=" << v << " clut_x=" << clut_x << " clut_y=" << clut_y << hex << " bgr=" << bgr;
			debug << "\r\nTP=" << GPU_CTRL_Read.TP << " ABR=" << GPU_CTRL_Read.ABR << " CycleCount=" << dec << *_DebugCycleCount;
#endif

#ifdef USE_TEMPLATES_SPRITE16
			Draw_Sprite16x16_7c_t <1,0> ();
#else
			Draw_Sprite16x16_7c ();
#endif
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;



		case 0x7f:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_SPRITE
			debug << "\r\nSprite16 Abe=1 Tge=1";
#endif
			//////////////////////////////////////////
			// 16x16 sprite
			GetBGR24 ( Buffer [ 0 ] );
			GetXY ( Buffer [ 1 ] );
			GetCLUT ( Buffer [ 2 ] );
			GetUV ( Buffer [ 2 ] );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << dec << " x=" << x << " y=" << y << " u=" << u << " v=" << v << " clut_x=" << clut_x << " clut_y=" << clut_y << hex << " bgr=" << bgr;
			debug << "\r\nTP=" << GPU_CTRL_Read.TP << " ABR=" << GPU_CTRL_Read.ABR << " CycleCount=" << dec << *_DebugCycleCount;
#endif

#ifdef USE_TEMPLATES_SPRITE16
			Draw_Sprite16x16_7c_t <1,1> ();
#else
			Draw_Sprite16x16_7c ();
#endif

#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_SPRITE
			debug << ";BusyCycles=" << BusyCycles;
#endif
			break;
			
			
		////////////////////////////////////////
		// Transfer commands
			
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		case 0x88:
		case 0x89:
		case 0x8a:
		case 0x8b:
		case 0x8c:
		case 0x8d:
		case 0x8e:
		case 0x8f:
		case 0x90:
		case 0x91:
		case 0x92:
		case 0x93:
		case 0x94:
		case 0x95:
		case 0x96:
		case 0x97:
		case 0x98:
		case 0x99:
		case 0x9a:
		case 0x9b:
		case 0x9c:
		case 0x9d:
		case 0x9e:
		case 0x9f:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TRANSFER
			debug << "\r\nMoveImage";
			debug << " " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif
			
			///////////////////////////////////////////
			// move image in frame buffer
			sX = Buffer [ 1 ].x;
			sY = Buffer [ 1 ].y;
			dX = Buffer [ 2 ].x;
			dY = Buffer [ 2 ].y;
			h = Buffer [ 3 ].h;
			w = Buffer [ 3 ].w;
			
			Transfer_MoveImage_80 ();
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TRANSFER
	debug << "; COMMAND: IMAGE MOVE; h = " << dec << h << "; w = " << w << "; sX=" << sX << "; sY=" << sY << "; dX=" << dX << "; dY=" << dY;
#endif
			break;
			
		case 0xa0:
		case 0xa1:
		case 0xa2:
		case 0xa3:
		case 0xa4:
		case 0xa5:
		case 0xa6:
		case 0xa7:
		case 0xa8:
		case 0xa9:
		case 0xaa:
		case 0xab:
		case 0xac:
		case 0xad:
		case 0xae:
		case 0xaf:
		case 0xb0:
		case 0xb1:
		case 0xb2:
		case 0xb3:
		case 0xb4:
		case 0xb5:
		case 0xb6:
		case 0xb7:
		case 0xb8:
		case 0xb9:
		case 0xba:
		case 0xbb:
		case 0xbc:
		case 0xbd:
		case 0xbe:
		case 0xbf:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TRANSFER
			debug << "\r\nImportImage";
			debug << " " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif
			//////////////////////////////////////////
			// send image to frame buffer
			dX = Buffer [ 1 ].x;
			dY = Buffer [ 1 ].y;
			h = Buffer [ 2 ].h;
			w = Buffer [ 2 ].w;
			iX = 0;
			iY = 0;
			
			// xpos & 0x3ff
			dX &= 0x3ff;
			
			// ypos & 0x1ff
			dY &= 0x1ff;
			
			// Xsiz=((Xsiz-1) AND 3FFh)+1
			w = ( ( w - 1 ) & 0x3ff ) + 1;
			
			// Ysiz=((Ysiz-1) AND 1FFh)+1
			h = ( ( h - 1 ) & 0x1ff ) + 1;
			
			BufferMode = MODE_IMAGEIN;
			
			// set busy cycles to 1 so that we get debug info
			BusyCycles = 1;
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TRANSFER
	debug << dec << " x=" << dX << " y=" << dY << " h=" << h << " w=" << w;
#endif
			break;
			
		case 0xc0:
		case 0xc1:
		case 0xc2:
		case 0xc3:
		case 0xc4:
		case 0xc5:
		case 0xc6:
		case 0xc7:
		case 0xc8:
		case 0xc9:
		case 0xca:
		case 0xcb:
		case 0xcc:
		case 0xcd:
		case 0xce:
		case 0xcf:
		case 0xd0:
		case 0xd1:
		case 0xd2:
		case 0xd3:
		case 0xd4:
		case 0xd5:
		case 0xd6:
		case 0xd7:
		case 0xd8:
		case 0xd9:
		case 0xda:
		case 0xdb:
		case 0xdc:
		case 0xdd:
		case 0xde:
		case 0xdf:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_TRANSFER
			debug << "\r\nExportImage";
			debug << " " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif
			/////////////////////////////////////////
			// copy image from frame buffer
			sX = Buffer [ 1 ].x;
			sY = Buffer [ 1 ].y;
			h = Buffer [ 2 ].h;
			w = Buffer [ 2 ].w;
			iX = 0;
			iY = 0;
			
			// xpos & 0x3ff
			sX &= 0x3ff;
			
			// ypos & 0x1ff
			sY &= 0x1ff;
			
			// Xsiz=((Xsiz-1) AND 3FFh)+1
			w = ( ( w - 1 ) & 0x3ff ) + 1;
			
			// Ysiz=((Ysiz-1) AND 1FFh)+1
			h = ( ( h - 1 ) & 0x1ff ) + 1;
			
			BufferMode = MODE_IMAGEOUT;
			
			////////////////////////////////////////////////////////
			// mark GPU as being ready to send an image
			GPU_CTRL_Read.IMG = 1;
			
			// set busy cycles to 1 so that we get debug info
			BusyCycles = 1;
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_TRANSFER
	debug << dec << " x=" << sX << " y=" << sY << " h=" << h << " w=" << w;
#endif
			break;
			
		///////////////////////////////////////
		// Environment commands
		
		
		case 0xe0:
		
			// Unknown //
			
#ifdef VERBOSE_GPU
			cout << "\nhps1x64 WARNING: Unknown GPU command: " << hex << (u32) Buffer [ 0 ].Command << "\n";
#endif
			
			break;
			
			
		
		case 0xe1:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_DISPLAYMODE || defined INLINE_DEBUG_RUN_ENVIRON
			debug << "\r\nSetDrawMode";
#endif
			////////////////////////////////////////////
			// draw mode setting
			
			
			// sets GPU Status up to bit 10 or more depending on GPU version - I'll use 11 bits total
			// gpu version 2 only sets bits 0x0-0xa - I'll stick with this - 11 bits
			GPU_CTRL_Read.Value = ( GPU_CTRL_Read.Value & 0xfffff800 ) | ( Buffer [ 0 ].Value & 0x7ff );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_DISPLAYMODE || defined INLINE_DEBUG_RUN_ENVIRON
			debug << dec << " TX=" << GPU_CTRL_Read.TX << " TY=" << GPU_CTRL_Read.TY << " ABR=" << GPU_CTRL_Read.ABR << " TP=" << GPU_CTRL_Read.TP << " DTD=" << GPU_CTRL_Read.DTD << " DFE=" << GPU_CTRL_Read.DFE;
#endif

			break;
			
		case 0xe2:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_ENVIRON || defined INLINE_DEBUG_WINDOW
			debug << "\r\nSetTextureWindow";
#endif
			////////////////////////////////////////////
			// texture window setting
			TWX = ( Buffer [ 0 ].Value >> 10 ) & 0x1f;
			TWY = ( Buffer [ 0 ].Value >> 15 ) & 0x1f;
			TWH = ( Buffer [ 0 ].Value >> 5 ) & 0x1f;
			TWW = Buffer [ 0 ].Value & 0x1f;
			
			// it's actually value*8 for x and y
			TextureWindow_X = ( TWX << 3 );
			TextureWindow_Y = ( TWY << 3 );
			
			// it's actually 256-(value*8) for width and height
			TextureWindow_Height = 256 - ( TWH << 3 );
			TextureWindow_Width = 256 - ( TWW << 3 );
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_ENVIRON || defined INLINE_DEBUG_WINDOW
			debug << dec << " X=" << TextureWindow_X << " Y=" << TextureWindow_Y << " Height=" << TextureWindow_Height << " Width=" << TextureWindow_Width;
#endif
			
			break;
			
		case 0xe3:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_DISPLAYAREA || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_ENVIRON
			debug << "\r\nSetDrawingAreaTopLeft";
#endif
			////////////////////////////////////////////
			// set drawing area top left
			//DrawArea_TopLeftX = Buffer [ 0 ].Value & 0x3ff;
			//DrawArea_TopLeftY = ( Buffer [ 0 ].Value >> 10 ) & 0x3ff;
			iREG_DrawArea_TopLeftX = Buffer [ 0 ].Value & 0x3ff;
			iREG_DrawArea_TopLeftY = ( Buffer [ 0 ].Value >> 10 ) & 0x3ff;
			
			// *problem* this might cause problems if drawing area is outside the framebuffer
			DrawArea_TopLeftX = iREG_DrawArea_TopLeftX;
			DrawArea_TopLeftY = iREG_DrawArea_TopLeftY;
			
			// perform a boundary check on the x and y
			if ( DrawArea_TopLeftX >= FrameBuffer_Width ) DrawArea_TopLeftX = FrameBuffer_Width - 1;
			if ( DrawArea_TopLeftY >= FrameBuffer_Height ) DrawArea_TopLeftY = FrameBuffer_Height - 1;
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_DISPLAYAREA || defined INLINE_DEBUG_RUN_ENVIRON
			debug << dec << "; X = " << DrawArea_TopLeftX << "; Y = " << DrawArea_TopLeftY;
#endif

			break;
			
		case 0xe4:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_DISPLAYAREA || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_ENVIRON
			debug << "\r\nSetDrawingAreaBottomRight";
#endif
			/////////////////////////////////////////////
			// set drawing area bottom right
			iREG_DrawArea_BottomRightX = Buffer [ 0 ].Value & 0x3ff;
			iREG_DrawArea_BottomRightY = ( Buffer [ 0 ].Value >> 10 ) & 0x3ff;
			
			// *problem* this might cause problems if drawing area is outside the framebuffer
			DrawArea_BottomRightX = iREG_DrawArea_BottomRightX;
			DrawArea_BottomRightY = iREG_DrawArea_BottomRightY;
			
			// perform a boundary check on the x and y
			if ( DrawArea_BottomRightX >= FrameBuffer_Width ) DrawArea_BottomRightX = FrameBuffer_Width - 1;
			if ( DrawArea_BottomRightY >= FrameBuffer_Height ) DrawArea_BottomRightY = FrameBuffer_Height - 1;
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_DISPLAYAREA || defined INLINE_DEBUG_RUN_ENVIRON
			debug << dec << "; X = " << DrawArea_BottomRightX << "; Y = " << DrawArea_BottomRightY;
#endif

			break;
			
		case 0xe5:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_DISPLAYOFFSET || defined INLINE_DEBUG_EXECUTE_NAME || defined INLINE_DEBUG_RUN_ENVIRON
			debug << "\r\nSetDrawingOffset";
#endif

			///////////////////////////////////////////////
			// drawing offset
			// *note* draw offset is signed and both x and y go from -1024 to +1023 (11 bits)
			s32 sTemp;
			
			// get x offset
			sTemp = Buffer [ 0 ].Value & 0x7ff;
			
			// sign extend
			sTemp = ( sTemp << 21 );
			sTemp = ( sTemp >> 21 );
			
			// store
			DrawArea_OffsetX = sTemp;
			
			// get y offset
			sTemp = ( Buffer [ 0 ].Value >> 11 ) & 0x7ff;
			
			// sign extend
			sTemp = ( sTemp << 21 );
			sTemp = ( sTemp >> 21 );
			
			// store
			DrawArea_OffsetY = sTemp;
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_DISPLAYOFFSET || defined INLINE_DEBUG_RUN_ENVIRON
			debug << dec << "; X = " << DrawArea_OffsetX << "; Y = " << DrawArea_OffsetY;
#endif

			break;
			
		case 0xe6:
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_ENVIRON || defined INLINE_DEBUG_MASK
			debug << "\r\nMaskSetting";
#endif
			////////////////////////////////////////////////
			// mask setting
			SetMaskBitWhenDrawing = Buffer [ 0 ].Value & 1;
			DoNotDrawToMaskAreas = ( Buffer [ 0 ].Value >> 1 ) & 1;
			
			// set gpu status flags
			GPU_CTRL_Read.MD = SetMaskBitWhenDrawing;
			GPU_CTRL_Read.ME = DoNotDrawToMaskAreas;
			
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_RUN_ENVIRON || defined INLINE_DEBUG_MASK
			debug << "; SetMaskBitWhenDrawing=" << SetMaskBitWhenDrawing << "; DoNotDrawToMaskAreas=" << DoNotDrawToMaskAreas;
#endif

			break;


		case 0xe7:
		case 0xe8:
		case 0xe9:
		case 0xea:
		case 0xeb:
		case 0xec:
		case 0xed:
		case 0xee:
		case 0xef:
		case 0xf0:
		case 0xf1:
		case 0xf2:
		case 0xf3:
		case 0xf4:
		case 0xf5:
		case 0xf6:
		case 0xf7:
		case 0xf8:
		case 0xf9:
		case 0xfa:
		case 0xfb:
		case 0xfc:
		case 0xfd:
		case 0xfe:
		case 0xff:
		
#if defined INLINE_DEBUG_EXECUTE || defined INLINE_DEBUG_EXECUTE_NAME
			debug << "\r\nUnknown Command " << hex << (u32) Buffer [ 0 ].Command;
#endif

			// Unknown //
			
#ifdef VERBOSE_GPU
			cout << "\nhps1x64 WARNING: Unknown GPU command: " << hex << (u32) Buffer [ 0 ].Command << "\n";
#endif

			break;


			
		default:
			cout << "\nhps1x64 WARNING: GPU::ExecuteGPUBuffer: Unknown GPU Command @ Cycle#" << dec << *_DebugCycleCount << " PC=" << hex << *_DebugPC << " Command=" << (u32) Buffer [ 0 ].Command;
			break;
			
	}
	
	
	if ( !GPU_CTRL_Read.DFE )
	{
		// *** testing *** only drew half of scanlines so half cycles
		BusyCycles >>= 1;
	}
	
	
	////////////////////////////////////////////////////////////////////////////////////
	// check for how long GPU should be busy for after executing the last command
	if ( BusyCycles )
	{
		/////////////////////////////////////
		// mark GPU as busy
		GPU_CTRL_Read.BUSY = 0;
		
		////////////////////////////////////////////////
		// mark GPU as not ready to receive commands
		GPU_CTRL_Read.COM = 0;
		
		// update the cycle that device is busy until
		BusyUntil_Cycle = BusyCycles + *_DebugCycleCount;
		
		// update count of primitives drawn (will not count invisible primitives for now
		Primitive_Count++;
	}
	else
	{
		///////////////////////
		// GPU is not busy
		GPU_CTRL_Read.BUSY = 1;
		GPU_CTRL_Read.COM = 1;
	}

}


void GPU::TransferPixelPacketIn ( u32 Data )
{
	u32 pix0, pix1;
	u32 DestPixel, PixelMask = 0, SetPixelMask = 0;
	
#ifdef INLINE_DEBUG_PIX_WRITE
	debug << "; TRANSFER PIX IN; h = " << dec << h << "; w = " << w << "; iX = " << iX << "; iY = " << iY;
#endif

	if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
	
	//////////////////////////////////////////////////////
	// transfer pixel of image to VRAM
	pix0 = Data & 0xffff;
	pix1 = ( Data >> 16 );
	
	// transfer pix0
	//if ( ( dX + iX ) < FrameBuffer_Width && ( dY + iY ) < FrameBuffer_Height )
	//{
		bgr2 = pix0;
		
		// read pixel from frame buffer if we need to check mask bit
		DestPixel = VRAM [ ( (dX + iX) & 0x3ff ) + ( ( (dY + iY) & 0x1ff ) << 10 ) ];
		
		//VRAM [ (dX + iX) + ( (dY + iY) << 10 ) ] = pix0;
		
		// check if we should set mask bit when drawing
		//if ( GPU_CTRL_Read.MD ) bgr2 |= 0x8000;
		bgr2 |= SetPixelMask;

		// draw pixel if we can draw to mask pixels or mask bit not set
		if ( ! ( DestPixel & PixelMask ) ) VRAM [ ( (dX + iX) & 0x3ff ) + ( ( (dY + iY) & 0x1ff ) << 10 ) ] = bgr2;
		//VRAM [ ( (dX + iX) & 0x3ff ) + ( ( (dY + iY) & 0x1ff ) << 10 ) ] = bgr2;
	//}
	//else
	//{
		//cout << "\nGPU::TransferPixelPacketIn: Error: Transferring pix0 outside of VRAM bounds. x=" << dec << (dX+iX) << " y=" << (dY+iY) << " DrawArea_OffsetX=" << DrawArea_OffsetX << " DrawArea_OffsetY=" << DrawArea_OffsetY;
	//}

	
	// update x
	iX++;
	
	// if it is at width then go to next line
	if ( iX == w )
	{
		iX = 0;
		iY++;
		
		// if this was the last pixel, then we are done
		if ( iY == h )
		{
			/////////////////////////////////////
			// set buffer mode back to normal
			BufferMode = MODE_NORMAL;
			
			////////////////////////////////////////
			// done
			return;
		}
	}
	
	
	// transfer pix 1
	//if ( ( dX + iX ) < FrameBuffer_Width && ( dY + iY ) < FrameBuffer_Height )
	//{
		//VRAM [ (dX + iX) + ( (dY + iY) << 10 ) ] = pix1;
		bgr2 = pix1;
		
		// read pixel from frame buffer if we need to check mask bit
		DestPixel = VRAM [ ( (dX + iX) & 0x3ff ) + ( ( (dY + iY) & 0x1ff ) << 10 ) ];
		
		//VRAM [ (dX + iX) + ( (dY + iY) << 10 ) ] = pix0;
		
		// check if we should set mask bit when drawing
		//if ( GPU_CTRL_Read.MD ) bgr2 |= 0x8000;
		bgr2 |= SetPixelMask;

		// draw pixel if we can draw to mask pixels or mask bit not set
		if ( ! ( DestPixel & PixelMask ) ) VRAM [ ( (dX + iX) & 0x3ff ) + ( ( (dY + iY) & 0x1ff ) << 10 ) ] = bgr2;
		//VRAM [ ( (dX + iX) & 0x3ff ) + ( ( (dY + iY) & 0x1ff ) << 10 ) ] = bgr2;
	//}
	//else
	//{
		//cout << "\nGPU::TransferPixelPacketIn: Error: Transferring pix1 outside of VRAM bounds. x=" << dec << (dX+iX) << " y=" << (dY+iY) << " DrawArea_OffsetX=" << DrawArea_OffsetX << " DrawArea_OffsetY=" << DrawArea_OffsetY;
	//}

	
	// update x
	iX++;
	
	// if it is at width then go to next line
	if ( iX == w )
	{
		iX = 0;
		iY++;
		
		// if this was the last pixel, then we are done
		if ( iY == h )
		{
			/////////////////////////////////////
			// set buffer mode back to normal
			BufferMode = MODE_NORMAL;
			
			////////////////////////////////////////
			// done
			return;
		}
	}
	
}

u32 GPU::TransferPixelPacketOut ()
{
#ifdef INLINE_DEBUG_PIX_READ
	debug << "; TRANSFER PIX OUT; h = " << dec << h << "; w = " << w << "; iX = " << iX << "; iY = " << iY;
#endif

	u32 pix0, pix1;
	u32 SetPixelMask = 0;
	
	if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x80008000;
	
	//////////////////////////////////////////////////////
	// transfer pixel of image from VRAM
	
	// transfer pix0
	//if ( ( sX + iX ) < FrameBuffer_Width && ( sY + iY ) < FrameBuffer_Height )
	//{
		pix0 = VRAM [ ( (sX + iX) & 0x3ff ) + ( ( (sY + iY) & 0x1ff ) << 10 ) ];
	//}
	//else
	//{
		//cout << "\nGPU::TransferPixelPacketOut: Error: Transferring pix0 outside of VRAM bounds. x=" << dec << (sX+iX) << " y=" << (sY+iY) << " DrawArea_OffsetX=" << DrawArea_OffsetX << " DrawArea_OffsetY=" << DrawArea_OffsetY;
	//}
	
	// update x
	iX++;
	
	// if it is at width then go to next line
	if ( iX == w )
	{
		iX = 0;
		iY++;
		
		// if this was the last pixel, then we are done
		if ( iY == h )
		{
			/////////////////////////////////////
			// set buffer mode back to normal
			BufferMode = MODE_NORMAL;
			
			////////////////////////////////////////////////////////
			// mark GPU no longer ready to send an image
			GPU_CTRL_Read.IMG = 0;
			
			////////////////////////////////////////
			// done
			return pix0;
		}
	}
	
	
	// transfer pix 1
	//if ( ( sX + iX ) < FrameBuffer_Width && ( sY + iY ) < FrameBuffer_Height )
	//{
		pix1 = VRAM [ ( (sX + iX) & 0x3ff ) + ( ( (sY + iY) & 0x1ff ) << 10 ) ];
	//}
	//else
	//{
		//cout << "\nGPU::TransferPixelPacketOut: Error: Transferring pix1 outside of VRAM bounds. x=" << dec << (sX+iX) << " y=" << (sY+iY) << " DrawArea_OffsetX=" << DrawArea_OffsetX << " DrawArea_OffsetY=" << DrawArea_OffsetY;
	//}
	
	// update x
	iX++;
	
	// if it is at width then go to next line
	if ( iX == w )
	{
		iX = 0;
		iY++;
		
		// if this was the last pixel, then we are done
		if ( iY == h )
		{
			/////////////////////////////////////
			// set buffer mode back to normal
			BufferMode = MODE_NORMAL;
			
			////////////////////////////////////////////////////////
			// mark GPU no longer ready to send an image
			GPU_CTRL_Read.IMG = 0;
			
			////////////////////////////////////////
			// done
		}
	}
	
	return pix0 | ( pix1 << 16 );
	//return pix0 | ( pix1 << 16 ) | SetPixelMask;
}


void GPU::ProcessDataRegWrite ( u32 Data )
{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "; DataRegWrite";
#endif

	u32 pix0, pix1;

	// make sure we are not sending or receiving images
	//if ( BufferMode != MODE_IMAGEIN )
	if ( BufferMode == MODE_NORMAL )
	{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "; NORMAL; Data = " << hex << Data << ";(Before) BufferSize=" << dec << BufferSize;
#endif

		// add data into buffer
		if ( BufferSize < 16 )
		{
			Buffer [ BufferSize++ ].Value = Data;
			//BufferSize &= 0xf;
		}
		else
		{
			// extended past edge of gpu buffer //
			cout << "\nhps1x64 ERROR: GPU: Extended past end of buffer.\n";
		}

#ifdef INLINE_DEBUG_DMA_WRITE
	debug << ";(After) BufferSize=" << dec << BufferSize;
#endif
		// check if we have all the arguments that are needed to execute command
		if ( BufferSize == GPU_SizeOfCommands [ Buffer [ 0 ].Command ] )
		{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "; EXEC BUFFER; Command = " << hex << ((u32)(Buffer [ 0 ].Command));
#endif
			ExecuteGPUBuffer ();
			BufferSize = 0;
		}
		
		// if drawing a poly line, then check for termination code
		// note: must add-in the "BufferSize > 1" part since it is possible command could be interpreted as termination code
		else if ( GPU_SizeOfCommands [ Buffer [ 0 ].Command ] == 55 && ( ( Buffer [ BufferSize - 1 ].Value & 0xf000f000 ) == 0x50005000 ) && ( BufferSize > 1 ) )
		{
			ExecuteGPUBuffer ();
			BufferSize = 0;
		}
		else if ( GPU_SizeOfCommands [ Buffer [ 0 ].Command ] == 66 && ( ( Buffer [ BufferSize - 1 ].Value & 0xf000f000 ) == 0x50005000 ) && ( BufferSize > 4 ) && !( ( BufferSize - 1 ) & 0x1 ) )
		{
			// shaded poly-line //
			ExecuteGPUBuffer ();
			BufferSize = 0;
		}
		
	}
	
	//////////////////////////////////////////////////////////////
	// Check if we are receiving an image
	else if ( BufferMode == MODE_IMAGEIN )
	{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "; IMAGE IN";
#endif

		// receive a pixel from bus
		TransferPixelPacketIn ( Data );

	}
	else
	{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "; InvalidBufferMode; BufferMode=" << dec << BufferMode << " w=" << w << " h=" << h;
#endif
	}
}


u32 GPU::ProcessDataRegRead ()
{
	u32 pix0, pix1;
#ifdef INLINE_DEBUG_DMA_READ
	debug << "; DataRegRead";
#endif

	if ( BufferMode == MODE_NORMAL )
	{
#ifdef INLINE_DEBUG_DMA_READ
		debug << "; READING RESULT=" << hex << GPU_DATA_Read;
#endif

		// if the GPU is not transferring an image from the GPU, then it must be reading a result from command sent to CTRL reg
		return GPU_DATA_Read;
	}
	
	//////////////////////////////////////////////////////////////////////
	// check if GPU is transferring an image from GPU
	else if ( BufferMode == MODE_IMAGEOUT )
	{
#ifdef INLINE_DEBUG_DMA_READ
	debug << "; IMAGE OUT";
#endif

		// send a pixel to bus
		return TransferPixelPacketOut ();
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void GPU::Draw_FrameBufferRectangle_02 ()
{
	// *** todo *** fix wrapping and sizing of frame buffer fill //
	
#ifdef _ENABLE_SSE2
	// with sse2, can send 8 pixels at a time
	static const int c_iVectorSize = 8;
#else
	// before sse2, was sending 4 pixels at a time
	static const int c_iVectorSize = 4;
#endif

	// ptr to vram
	//u16 *ptr;
	u64 *ptr;
	u64 bgr64;
	u32 width1, width2, height1, height2, xmax, ymax;
	
#ifdef _ENABLE_SSE2
	__m128i vbgr;
	vbgr = _mm_set1_epi16 ( bgr );
#endif
	
	/////////////////////////////////////
	// mark GPU as busy
	GPU_CTRL_Read.BUSY = 0;
	
	////////////////////////////////////////////////
	// mark GPU as not ready to receive commands
	GPU_CTRL_Read.COM = 0;
	
	// set bgr64
	bgr64 = bgr;
	bgr64 |= ( bgr64 << 16 );
	bgr64 |= ( bgr64 << 32 );
	
	
	// Xpos=(Xpos AND 3F0h)
	x &= 0x3f0;
	
	// ypos & 0x1ff
	y &= 0x1ff;
	
	// Xsiz=((Xsiz AND 3FFh)+0Fh) AND (NOT 0Fh)
	w = ( ( w & 0x3ff ) + 0xf ) & ~0xf;
	
	// Ysiz=((Ysiz AND 1FFh))
	h &= 0x1ff;
	
	
	// *** NOTE: coordinates wrap *** //
	
	///////////////////////////////////////////////
	// set amount of time GPU will be busy for
	BusyCycles += (u32) ( ( (u64) h * (u64) w * dFrameBufferRectangle_02_CyclesPerPixel ) );
	
	// get width of segment 1 and segment 2
	xmax = x + w;
	width2 = 0;
	if ( xmax > FrameBuffer_Width ) width2 = xmax - FrameBuffer_Width;
	width1 = w - width2;
	
	// get height of segment 1 and segment 2
	ymax = y + h;
	height2 = 0;
	if ( ymax > FrameBuffer_Height ) height2 = ymax - FrameBuffer_Height;
	height1 = h - height2;
	
	
	// draw segment 1 height
	for ( ; y < ymax; y++ )
	{
		// wraparound y
		iY = y & 0x1ff;
		
		//ptr = & (VRAM [ x + ( iY << 10 ) ]);
		ptr = (u64*) ( & (VRAM [ x + ( iY << 10 ) ]) );
		

		// draw segment 1 width
		//for ( iX = 0; iX < width1; iX += 4 )
		for ( iX = 0; iX < width1; iX += c_iVectorSize )
		{
#ifdef _ENABLE_SSE2
			_mm_store_si128 ( (__m128i*) ptr, vbgr );
			ptr += 2;
#else
			*ptr++ = bgr64;
#endif
		}

		
		//ptr = & (VRAM [ y << 10 ]);
		//ptr -= FrameBuffer_Width;
		//ptr = & (VRAM [ iY << 10 ]);
		ptr = (u64*) ( & (VRAM [ iY << 10 ]) );
		
		// draw segment 2 width
		//for ( ; iX < w; iX += 4 )
		for ( ; iX < w; iX += c_iVectorSize )
		{
#ifdef _ENABLE_SSE2
			_mm_store_si128 ( (__m128i*) ptr, vbgr );
			ptr += 2;
#else
			*ptr++ = bgr64;
#endif
		}
	}
	
}


void GPU::Draw_Rectangle_60 ()
{
#ifdef _ENABLE_SSE2
	// with sse2, can send 8 pixels at a time
	static const int c_iVectorSize = 8;
#else
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;
#endif

	//u32 Pixel;
	
	s32 StartX, EndX, StartY, EndY;
	u32 PixelsPerLine;
	u16 *ptr;
	
#ifdef _ENABLE_SSE2
	__m128i DestPixel, PixelMask, SetPixelMask;
	__m128i vbgr, vbgr_temp, vStartX, vEndX, vx_across, vSeq, vVectorSize;
	
	vSeq = _mm_set_epi16 ( 7, 6, 5, 4, 3, 2, 1, 0 );
	vVectorSize = _mm_set1_epi16 ( c_iVectorSize );
	
	vbgr = _mm_set1_epi16 ( bgr );
	PixelMask = _mm_setzero_si128 ();
	SetPixelMask = _mm_setzero_si128 ();
	if ( GPU_CTRL_Read.ME ) PixelMask = _mm_set1_epi16 ( 0x8080 );
	if ( GPU_CTRL_Read.MD ) SetPixelMask = _mm_set1_epi16 ( 0x8000 );
#else
	u32 DestPixel, PixelMask = 0, SetPixelMask = 0;
	
	if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
#endif

	

	// initialize number of pixels drawn
	NumberOfPixelsDrawn = 0;
	
	// check for some important conditions
	if ( DrawArea_BottomRightX < DrawArea_TopLeftX )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightX < DrawArea_TopLeftX.\n";
		return;
	}
	
	
	if ( DrawArea_BottomRightY < DrawArea_TopLeftY )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightY < DrawArea_TopLeftY.\n";
		return;
	}
	
	// get top left corner of sprite and bottom right corner of sprite
	x0 = x;
	y0 = y;
	x1 = x + w - 1;
	y1 = y + h - 1;
	
	//////////////////////////////////////////
	// get coordinates on screen
	x0 = DrawArea_OffsetX + x0;
	y0 = DrawArea_OffsetY + y0;
	x1 = DrawArea_OffsetX + x1;
	y1 = DrawArea_OffsetY + y1;
	
	// check if sprite is within draw area
	if ( x1 < ((s32)DrawArea_TopLeftX) || x0 > ((s32)DrawArea_BottomRightX) || y1 < ((s32)DrawArea_TopLeftY) || y0 > ((s32)DrawArea_BottomRightY) ) return;
	
	
	
	
	StartX = x0;
	EndX = x1;
	StartY = y0;
	EndY = y1;

	if ( StartY < ((s32)DrawArea_TopLeftY) )
	{
		StartY = DrawArea_TopLeftY;
	}
	
	if ( EndY > ((s32)DrawArea_BottomRightY) )
	{
		EndY = DrawArea_BottomRightY;
	}
	
	if ( StartX < ((s32)DrawArea_TopLeftX) )
	{
		StartX = DrawArea_TopLeftX;
	}
	
	if ( EndX > ((s32)DrawArea_BottomRightX) )
	{
		EndX = DrawArea_BottomRightX;
	}

	
	NumberOfPixelsDrawn = ( EndX - StartX + 1 ) * ( EndY - StartY + 1 );
	
#ifdef _ENABLE_SSE2
	vStartX = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX + 1 );
#endif
	
	for ( Line = StartY; Line <= EndY; Line++ )
	{
		ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
		
#ifdef _ENABLE_SSE2
		vx_across = vStartX;
#endif

		// draw horizontal line
		//for ( x_across = StartX; x_across <= EndX; x_across++ )
		for ( x_across = StartX; x_across <= EndX; x_across += c_iVectorSize )
		{
#ifdef _ENABLE_SSE2
			DestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
			vbgr_temp = vbgr;
			if ( command_abe ) vbgr_temp = vSemiTransparency16( DestPixel, vbgr_temp, GPU_CTRL_Read.ABR );
			vbgr_temp = _mm_or_si128 ( vbgr_temp, SetPixelMask );
			_mm_maskmoveu_si128 ( vbgr_temp, _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( DestPixel, 15 ), PixelMask ), _mm_cmplt_epi16 ( vx_across, vEndX ) ), (char*) ptr );
			vx_across = _mm_add_epi16 ( vx_across, vVectorSize );
#else
			// read pixel from frame buffer if we need to check mask bit
			DestPixel = *ptr;
			
			bgr_temp = bgr;

			// semi-transparency
			if ( command_abe )
			{
				bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read.ABR );
			}
			
			// check if we should set mask bit when drawing
			bgr_temp |= SetPixelMask;

			// draw pixel if we can draw to mask pixels or mask bit not set
			if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
#endif
			
			// update pointer for pixel out
			ptr += c_iVectorSize;
		}
	}
	
	// set the amount of time drawing used up
	BusyCycles = NumberOfPixelsDrawn * 1;
}


void GPU::Draw_Rectangle8x8_70 ()
{
	w = 8; h = 8;
	Draw_Rectangle_60 ();
}


void GPU::Draw_Rectangle16x16_78 ()
{
	w = 16; h = 16;
	Draw_Rectangle_60 ();
}



void GPU::Draw_Pixel_68 ()
{
	u32 DestPixel, PixelMask = 0;

	// check for some important conditions
	if ( DrawArea_BottomRightX < DrawArea_TopLeftX )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightX < DrawArea_TopLeftX.\n";
		return;
	}
	
	
	if ( DrawArea_BottomRightY < DrawArea_TopLeftY )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightY < DrawArea_TopLeftY.\n";
		return;
	}
	
	
	if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	
	/////////////////////////////////////
	// mark GPU as busy
	GPU_CTRL_Read.BUSY = 0;
	
	////////////////////////////////////////////////
	// mark GPU as not ready to receive commands
	GPU_CTRL_Read.COM = 0;
	
	///////////////////////////////////////////////
	// set amount of time GPU will be busy for
	BusyCycles += 1;
	
	/////////////////////////////////////////
	// Draw the pixel
	Absolute_DrawX = DrawArea_OffsetX + x;
	Absolute_DrawY = DrawArea_OffsetY + y;

	// make sure we are putting pixel within draw area
	if ( Absolute_DrawX >= DrawArea_TopLeftX && Absolute_DrawY >= DrawArea_TopLeftY
	&& Absolute_DrawX <= DrawArea_BottomRightX && Absolute_DrawY <= DrawArea_BottomRightY )
	{
		bgr2 = bgr;
		
		// read pixel from frame buffer if we need to check mask bit
		DestPixel = VRAM [ Absolute_DrawX + ( Absolute_DrawY << 10 ) ];
		
		// semi-transparency
		if ( command_abe )
		{
			bgr2 = SemiTransparency16 ( DestPixel, bgr2, GPU_CTRL_Read.ABR );
		}
		
		// check if we should set mask bit when drawing
		if ( GPU_CTRL_Read.MD ) bgr2 |= 0x8000;

		// draw pixel if we can draw to mask pixels or mask bit not set
		if ( ! ( DestPixel & PixelMask ) ) VRAM [ Absolute_DrawX + ( Absolute_DrawY << 10 ) ] = bgr2;
	}
}






void GPU::Transfer_MoveImage_80 ()
{
	u32 SrcPixel, DstPixel;
	u32 DestPixel, PixelMask = 0, SetPixelMask = 0;
	
	u32 SrcStartX, SrcStartY, DstStartX, DstStartY, Height, Width, SrcXRun, DstXRun, Width1, Width2, CurX, CurY;
	u16 *SrcPtr, *DstPtr, *SrcLinePtr, *DstLinePtr;
	
	///////////////////////////////////////////////
	// set amount of time GPU will be busy for
	BusyCycles += h * w * dMoveImage_80_CyclesPerPixel;	//CyclesPerPixelMove;

	// nocash psx specifications: transfer/move vram-to-vram use masking
	if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
	
	// xpos & 0x3ff
	//sX &= 0x3ff;
	SrcStartX = sX & 0x3ff;
	//dX &= 0x3ff;
	DstStartX = dX & 0x3ff;
	
	// ypos & 0x1ff
	//sY &= 0x1ff;
	SrcStartY = sY & 0x1ff;
	//dY &= 0x1ff;
	DstStartY = dY & 0x1ff;
	
	// Xsiz=((Xsiz-1) AND 3FFh)+1
	Width = ( ( w - 1 ) & 0x3ff ) + 1;
	
	// Ysiz=((Ysiz-1) AND 1FFh)+1
	Height = ( ( h - 1 ) & 0x1ff ) + 1;
	
	// *** NOTE: coordinates wrap *** //
	
	SrcXRun = FrameBuffer_Width - SrcStartX;
	SrcXRun = ( Width <= SrcXRun ) ? Width : SrcXRun;
	
	DstXRun = FrameBuffer_Width - DstStartX;
	DstXRun = ( Width <= DstXRun ) ? Width : DstXRun;
	
	Width1 = ( SrcXRun < DstXRun ) ? SrcXRun : DstXRun;
	Width2 = ( SrcXRun > DstXRun ) ? SrcXRun : DstXRun;
	
	for ( CurY = 0; CurY < Height; CurY++ )
	{
		// start Src/Dst pointers for line
		SrcLinePtr = & ( VRAM [ ( ( SrcStartY + CurY ) & FrameBuffer_YMask ) << 10 ] );
		DstLinePtr = & ( VRAM [ ( ( DstStartY + CurY ) & FrameBuffer_YMask ) << 10 ] );
		
		SrcPtr = & ( SrcLinePtr [ ( SrcStartX ) & FrameBuffer_XMask ] );
		DstPtr = & ( DstLinePtr [ ( DstStartX ) & FrameBuffer_XMask ] );
		
		for ( CurX = 0; CurX < Width1; CurX++ )
		{
			SrcPixel = *SrcPtr++;
			DstPixel = *DstPtr;
			
			SrcPixel |= SetPixelMask;
			
			if ( ! ( DstPixel & PixelMask ) ) *DstPtr++ = SrcPixel;
		}
		
		SrcPtr = & ( SrcLinePtr [ ( SrcStartX + CurX ) & FrameBuffer_XMask ] );
		DstPtr = & ( DstLinePtr [ ( DstStartX + CurX ) & FrameBuffer_XMask ] );
		
		for ( ; CurX < Width2; CurX++ )
		{
			SrcPixel = *SrcPtr++;
			DstPixel = *DstPtr;
			
			SrcPixel |= SetPixelMask;
			
			if ( ! ( DstPixel & PixelMask ) ) *DstPtr++ = SrcPixel;
		}
	
		SrcPtr = & ( SrcLinePtr [ ( SrcStartX + CurX ) & FrameBuffer_XMask ] );
		DstPtr = & ( DstLinePtr [ ( DstStartX + CurX ) & FrameBuffer_XMask ] );
		
		for ( ; CurX < Width; CurX++ )
		{
			SrcPixel = *SrcPtr++;
			DstPixel = *DstPtr;
			
			SrcPixel |= SetPixelMask;
			
			if ( ! ( DstPixel & PixelMask ) ) *DstPtr++ = SrcPixel;
		}
	}
	
	/*
	////////////////////////////////////////////////
	// transfer pixels in frame buffer
	for ( yy = 0; yy < h; yy++ )
	{
		// set total number of pixels to transfer on line
		XSizeLeft = XSize;
		
		while ( XSizeLeft )
		{
			SrcEndX = SrcX + XSizeLeft;
			DstEndX = SrcX + XSizeLeft;
			
			// get the 
			RunXSize = XSizeLeft;
			
			
			// check MIN how many pixels before you need to wrap coords
			if ( SrcEndX > FrameBuffer_XSize ) XSizeLeft = FrameBuffer_XSize - SrcX;
			
			// check number of pixels left
		}
		
		
		for ( xx = 0; xx < w; xx++ )
		{
			bgr2 = VRAM [ ( (xx + sX) & 0x3ff ) + ( ( (xx + sY) & 0x1ff ) << 10 ) ];
			
			DestPixel = VRAM [ ( (yy + dX) & 0x3ff ) + ( ( (yy + dY) & 0x1ff ) << 10 ) ];
			
			// check if we should set mask bit when drawing
			//if ( GPU_CTRL_Read.MD ) bgr2 |= 0x8000;
			bgr2 |= SetPixelMask;
			
			if ( ! ( DestPixel & PixelMask ) ) VRAM [ ( (xx + dX) & 0x3ff ) + ( ( (yy + dY) & 0x1ff ) << 10 ) ] = bgr2;
		}
	}
	*/
}




////////////////////////////////////////////////////////////////
// *** Triangle Drawing ***



void GPU::DrawTriangle_Mono ()
{	
#ifdef _ENABLE_SSE2
	// with sse2, can send 8 pixels at a time
	static const int c_iVectorSize = 8;
#else
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;
#endif

	//u32 Pixel, TexelIndex, Y1_OnLeft;
	//u32 color_add;
	
	//u32 Y1_OnLeft;
	
	u16 *ptr;
	//u32 TexCoordX, TexCoordY;
	//u32 Shift1 = 0, Shift2 = 0, And1 = 0, And2 = 0;
	s64 Temp;
	s64 LeftMostX, RightMostX;
	
	s32 StartX, EndX, StartY, EndY;
	s64 Error_Left;
	
#ifdef _ENABLE_SSE2
	__m128i DestPixel, PixelMask, SetPixelMask;
	__m128i vbgr, vbgr_temp, vStartX, vEndX, vx_across, vSeq, vVectorSize;
	
	vSeq = _mm_set_epi16 ( 7, 6, 5, 4, 3, 2, 1, 0 );
	vVectorSize = _mm_set1_epi16 ( c_iVectorSize );
	
	vbgr = _mm_set1_epi16 ( bgr );
	PixelMask = _mm_setzero_si128 ();
	SetPixelMask = _mm_setzero_si128 ();
	if ( GPU_CTRL_Read.ME ) PixelMask = _mm_set1_epi16 ( 0x8080 );
	if ( GPU_CTRL_Read.MD ) SetPixelMask = _mm_set1_epi16 ( 0x8000 );
#else
	u32 DestPixel, PixelMask = 0, SetPixelMask = 0;
	
	if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
#endif
	
	
	// initialize number of pixels drawn
	NumberOfPixelsDrawn = 0;
	
	// check for some important conditions
	if ( DrawArea_BottomRightX < DrawArea_TopLeftX )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightX < DrawArea_TopLeftX.\n";
		return;
	}
	
	if ( DrawArea_BottomRightY < DrawArea_TopLeftY )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightY < DrawArea_TopLeftY.\n";
		return;
	}
	

	
	//////////////////////////////////////////
	// get coordinates on screen
	x0 = DrawArea_OffsetX + x0;
	y0 = DrawArea_OffsetY + y0;
	x1 = DrawArea_OffsetX + x1;
	y1 = DrawArea_OffsetY + y1;
	x2 = DrawArea_OffsetX + x2;
	y2 = DrawArea_OffsetY + y2;
	
	
	
	
	//u32 NibblesPerPixel;
	
	//if ( tpage_tp == 0 ) NibblesPerPixel = 1; else if ( tpage_tp == 1 ) NibblesPerPixel = 2; else NibblesPerPixel = 4;

	//if ( tpage_tp == 0 )
	//{
	//	Shift1 = 2; Shift2 = 2; And1 = 3; And2 = 0xf;
	//}
	//else if ( tpage_tp == 1 )
	//{
	//	Shift1 = 1; Shift2 = 3; And1 = 1; And2 = 0xff;
	//}
	
	
	///////////////////////////////////
	// put top coordinates in x0,y0
	if ( y1 < y0 )
	{
		//Swap ( x0, x1 );
		//Swap ( y0, y1 );
		Swap ( x0, x1 );
		Swap ( y0, y1 );
	}
	
	if ( y2 < y0 )
	{
		//Swap ( x0, x2 );
		//Swap ( y0, y2 );
		Swap ( x0, x2 );
		Swap ( y0, y2 );
	}
	
	///////////////////////////////////////
	// put middle coordinates in x1,y1
	if ( y2 < y1 )
	{
		//Swap ( x1, x2 );
		//Swap ( y1, y2 );
		Swap ( x1, x2 );
		Swap ( y1, y2 );
	}
	
	// get the left/right most x
	LeftMostX = ( ( x0 < x1 ) ? x0 : x1 );
	LeftMostX = ( ( x2 < LeftMostX ) ? x2 : LeftMostX );
	RightMostX = ( ( x0 > x1 ) ? x0 : x1 );
	RightMostX = ( ( x2 > RightMostX ) ? x2 : RightMostX );

	// check if sprite is within draw area
	if ( RightMostX <= ((s32)DrawArea_TopLeftX) || LeftMostX > ((s32)DrawArea_BottomRightX) || y2 <= ((s32)DrawArea_TopLeftY) || y0 > ((s32)DrawArea_BottomRightY) ) return;
	
	// skip drawing if distance between vertices is greater than max allowed by GPU
	if ( ( _Abs( x1 - x0 ) > c_MaxPolygonWidth ) || ( _Abs( x2 - x1 ) > c_MaxPolygonWidth ) || ( y1 - y0 > c_MaxPolygonHeight ) || ( y2 - y1 > c_MaxPolygonHeight ) )
	{
		// skip drawing polygon
		return;
	}
	
	
	
	/////////////////////////////////////////////////
	// draw top part of triangle
	
	// denominator is negative when x1 is on the left, positive when x1 is on the right
	s64 t0 = y1 - y2;
	s64 t1 = y0 - y2;
	s64 denominator = ( ( x0 - x2 ) * t0 ) - ( ( x1 - x2 ) * t1 );

	///////////////////////////////////////////
	// start at y0
	//Line = y0;
	
	
	//if ( denominator < 0 )
	//{
		// x1 is on the left and x0 is on the right //
		
		////////////////////////////////////
		// get slopes
		
		if ( y1 - y0 )
		{
			/////////////////////////////////////////////
			// init x on the left and right
			x_left = ( ((s64)x0) << 32 );
			x_right = x_left;
			
			dx_left = (((s64)( x1 - x0 )) << 32 ) / ((s64)( y1 - y0 ));
			dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
		}
		else
		{
			// change x_left and x_right where y1 is on left
			x_left = ( ((s64)x1) << 32 );
			x_right = ( ((s64)x0) << 32 );
			
			// this means that x0 and x1 are on the same line
			// so if the height of the entire polygon is zero, then we are done
			if ( y2 - y1 )
			{
				dx_left = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
				dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
			}
		}
	//}

	/////////////////////////////////////////////////
	// swap left and right if they are reversed
	//if ( ( ( x_right + dx_right ) < ( x_left + dx_left ) ) || ( x_right < x_left ) )
	if ( denominator > 0 )
	{
		// x1,y1 is on the right //
		
		//Swap ( x_left, x_right );
		//Swap ( dx_left, dx_right );
		Swap ( x_left, x_right );
		Swap ( dx_left, dx_right );
	}
	
	
	////////////////
	// *** TODO *** at this point area of full triangle can be calculated and the rest of the drawing can be put on another thread *** //
	
	
	
	StartY = y0;
	EndY = y1;

	if ( StartY < ((s32)DrawArea_TopLeftY) )
	{
		
		if ( EndY < ((s32)DrawArea_TopLeftY) )
		{
			Temp = EndY - StartY;
			StartY = EndY;
		}
		else
		{
			Temp = DrawArea_TopLeftY - StartY;
			StartY = DrawArea_TopLeftY;
		}
		
		x_left += dx_left * Temp;
		x_right += dx_right * Temp;
	}
	
	if ( EndY > ((s32)DrawArea_BottomRightY) )
	{
		EndY = DrawArea_BottomRightY + 1;
	}

	
	//////////////////////////////////////////////
	// draw down to y1
	//while ( Line < y1 )
	for ( Line = StartY; Line < EndY; Line++ )
	{
		//cy = Line;

		StartX = _Round( x_left ) >> 32;
		EndX = _Round( x_right ) >> 32;
		
		//if ( Line >= ((s32)DrawArea_TopLeftY) && Line <= ((s32)DrawArea_BottomRightY) )
		//{
		if ( StartX <= ((s32)DrawArea_BottomRightX) && EndX > ((s32)DrawArea_TopLeftX) )
		{
			if ( StartX < ((s32)DrawArea_TopLeftX) )
			{
				//Temp = DrawArea_TopLeftX - StartX;
				StartX = DrawArea_TopLeftX;
			}
			
			if ( EndX > ((s32)DrawArea_BottomRightX) )
			{
				EndX = DrawArea_BottomRightX + 1;
				//EndX = DrawArea_BottomRightX;
			}
			
			ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			
			
			if ( StartX == EndX && !dx_left )
			{
#ifdef INLINE_DEBUG_TRIANGLE_MONO_TEST
	debug << "\r\nTOP. StartX=" << dec << StartX << " EndX=" << EndX << " Line=" << Line << " y0=" << y0 << " y1=" << y1 << " y2=" << y2;
	debug << " x0=" << x0 << " x1=" << x1 << " x2=" << x2 << " RightMostX=" << RightMostX;
#endif

				EndX += 1;
			}
			
			
			// ??
			//if ( EndX >= RightMostX ) EndX--;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;
			
#ifdef _ENABLE_SSE2
	vx_across = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX );
#endif

			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across < EndX; x_across++ )
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
#ifdef _ENABLE_SSE2
				DestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
				vbgr_temp = vbgr;
				if ( command_abe ) vbgr_temp = vSemiTransparency16( DestPixel, vbgr_temp, GPU_CTRL_Read.ABR );
				vbgr_temp = _mm_or_si128 ( vbgr_temp, SetPixelMask );
				_mm_maskmoveu_si128 ( vbgr_temp, _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( DestPixel, 15 ), PixelMask ), _mm_cmplt_epi16 ( vx_across, vEndX ) ), (char*) ptr );
				vx_across = _mm_add_epi16 ( vx_across, vVectorSize );
#else

				// read pixel from frame buffer if we need to check mask bit
				DestPixel = *ptr;
				
				bgr_temp = bgr;
	
				// semi-transparency
				if ( command_abe )
				{
					bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read.ABR );
				}
				
				// check if we should set mask bit when drawing
				bgr_temp |= SetPixelMask;

				// draw pixel if we can draw to mask pixels or mask bit not set
				if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
#endif
				
				//ptr++;
				ptr += c_iVectorSize;
			}
			
		}
		
		//////////////////////////////////
		// draw next line
		//Line++;
		
		/////////////////////////////////////
		// update x on left and right
		x_left += dx_left;
		x_right += dx_right;
	}

	
	////////////////////////////////////////////////
	// draw bottom part of triangle

	/////////////////////////////////////////////
	// init x on the left and right
	
	//////////////////////////////////////////////////////
	// check if y1 is on the left or on the right
	//if ( Y1_OnLeft )
	if ( denominator < 0 )
	{
		// y1 is on the left //
		
		x_left = ( ((s64)x1) << 32 );
		
		if ( y2 - y1 )
		{
			dx_left = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
		}
	}
	else
	{
		// y1 is on the right //
		
		x_right = ( ((s64)x1) << 32 );
		
		if ( y2 - y1 )
		{
			dx_right = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
		}
	}
	
	// the line starts at y1 from here
	//Line = y1;

	StartY = y1;
	EndY = y2;

	if ( StartY < ((s32)DrawArea_TopLeftY) )
	{
		
		if ( EndY < ((s32)DrawArea_TopLeftY) )
		{
			Temp = EndY - StartY;
			StartY = EndY;
		}
		else
		{
			Temp = DrawArea_TopLeftY - StartY;
			StartY = DrawArea_TopLeftY;
		}
		
		x_left += dx_left * Temp;
		x_right += dx_right * Temp;
	}
	
	if ( EndY > ((s32)DrawArea_BottomRightY) )
	{
		EndY = DrawArea_BottomRightY + 1;
	}

	
	//////////////////////////////////////////////
	// draw down to y2
	//while ( Line < y2 )
	for ( Line = StartY; Line < EndY; Line++ )
	{
		//cy = Line;

		StartX = _Round( x_left ) >> 32;
		EndX = _Round( x_right ) >> 32;
		
		//if ( Line >= ((s32)DrawArea_TopLeftY) && Line <= ((s32)DrawArea_BottomRightY) )
		//{
		if ( StartX <= ((s32)DrawArea_BottomRightX) && EndX > ((s32)DrawArea_TopLeftX) )
		{
			if ( StartX < ((s32)DrawArea_TopLeftX) )
			{
				//Temp = DrawArea_TopLeftX - StartX;
				StartX = DrawArea_TopLeftX;
			}
			
			if ( EndX > ((s32)DrawArea_BottomRightX) )
			{
				EndX = DrawArea_BottomRightX + 1;
				//EndX = DrawArea_BottomRightX;
			}
			
			ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			
			
			if ( StartX == EndX && !dx_left )
			{
#ifdef INLINE_DEBUG_TRIANGLE_MONO_TEST
	debug << "\r\nBOTTOM. StartX=" << dec << StartX << " EndX=" << EndX << " Line=" << Line << " y0=" << y0 << " y1=" << y1 << " y2=" << y2;
	debug << " x0=" << x0 << " x1=" << x1 << " x2=" << x2 << " RightMostX=" << RightMostX;
#endif
				EndX += 1;
			}
			
			
			// ??
			//if ( EndX >= RightMostX ) EndX--;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;
			
#ifdef _ENABLE_SSE2
	vx_across = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX );
#endif

			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across < EndX; x_across++ )
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
#ifdef _ENABLE_SSE2
				DestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
				vbgr_temp = vbgr;
				if ( command_abe ) vbgr_temp = vSemiTransparency16( DestPixel, vbgr_temp, GPU_CTRL_Read.ABR );
				vbgr_temp = _mm_or_si128 ( vbgr_temp, SetPixelMask );
				_mm_maskmoveu_si128 ( vbgr_temp, _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( DestPixel, 15 ), PixelMask ), _mm_cmplt_epi16 ( vx_across, vEndX ) ), (char*) ptr );
				vx_across = _mm_add_epi16 ( vx_across, vVectorSize );
#else
				// read pixel from frame buffer if we need to check mask bit
				DestPixel = *ptr;
				
				bgr_temp = bgr;
	
				// semi-transparency
				if ( command_abe )
				{
					bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read.ABR );
				}
				
				// check if we should set mask bit when drawing
				bgr_temp |= SetPixelMask;

				// draw pixel if we can draw to mask pixels or mask bit not set
				if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
#endif
				
				//ptr++;
				ptr += c_iVectorSize;
			}
			
		}
		
		/////////////////////////////////////
		// update x on left and right
		x_left += dx_left;
		x_right += dx_right;
	}

}




void GPU::DrawTriangle_Gradient ()
{
#ifdef _ENABLE_SSE2
	// with sse2, can send 8 pixels at a time
	static const int c_iVectorSize = 8;
#else
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;
#endif

	//u32 Pixel, TexelIndex,
	
	//u32 Y1_OnLeft;
	
	//u32 color_add;
	
	u16 *ptr;
	
	s64 Temp;
	s64 LeftMostX, RightMostX;
	
	s32 StartX, EndX, StartY, EndY;

	s64 Error_Left;
	
	s64* DitherArray;
	s64* DitherLine;
	s64 DitherValue;
	
#ifdef _ENABLE_SSE2
	s16* vDitherArray_add;
	s16* vDitherArray_sub;
	s16* vDitherLine_add;
	s16* vDitherLine_sub;
	__m128i viR1, viG1, viB1, viR2, viG2, viB2, vRed, vGreen, vBlue, vdR_across, vdG_across, vdB_across, vDitherValue_add, vDitherValue_sub, vTemp;
	
	__m128i DestPixel, PixelMask, SetPixelMask;
	__m128i vbgr, vbgr_temp, vStartX, vEndX, vx_across, vSeq, vVectorSize;
	__m128i vSeq32_1, vSeq32_2;
	
	vSeq32_1 = _mm_set_epi32 ( 3, 2, 1, 0 );
	vSeq32_2 = _mm_set_epi32 ( 7, 6, 5, 4 );
	
	vSeq = _mm_set_epi16 ( 7, 6, 5, 4, 3, 2, 1, 0 );
	vVectorSize = _mm_set1_epi16 ( c_iVectorSize );
	
	vbgr = _mm_set1_epi16 ( bgr );
	PixelMask = _mm_setzero_si128 ();
	SetPixelMask = _mm_setzero_si128 ();
	if ( GPU_CTRL_Read.ME ) PixelMask = _mm_set1_epi16 ( 0x8080 );
	if ( GPU_CTRL_Read.MD ) SetPixelMask = _mm_set1_epi16 ( 0x8000 );
#else
	
	s64 Red, Green, Blue;
	u32 DestPixel, PixelMask = 0, SetPixelMask = 0;
	
	if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
#endif
	
	// initialize number of pixels drawn
	NumberOfPixelsDrawn = 0;
	
	// check for some important conditions
	if ( DrawArea_BottomRightX < DrawArea_TopLeftX )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightX < DrawArea_TopLeftX.\n";
		return;
	}
	
	if ( DrawArea_BottomRightY < DrawArea_TopLeftY )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightY < DrawArea_TopLeftY.\n";
		return;
	}
	
	///////////////////////////////////////////////////
	// Initialize dithering
	
#ifdef _ENABLE_SSE2
	vDitherArray_add = (s16*) c_iDitherZero;
	vDitherArray_sub = (s16*) c_iDitherZero;
#else
	DitherArray = c_iDitherZero;
#endif
	
	if ( GPU_CTRL_Read.DTD )
	{
#ifdef _ENABLE_SSE2
		vDitherArray_add = c_viDitherValues16_add;
		vDitherArray_sub = c_viDitherValues16_sub;
#else
		//DitherArray = c_iDitherValues;
		DitherArray = c_iDitherValues24;
#endif
	}
	
	//////////////////////////////////////////
	// get coordinates on screen
	x0 = DrawArea_OffsetX + x0;
	y0 = DrawArea_OffsetY + y0;
	x1 = DrawArea_OffsetX + x1;
	y1 = DrawArea_OffsetY + y1;
	x2 = DrawArea_OffsetX + x2;
	y2 = DrawArea_OffsetY + y2;
	
	
	
	
	////////////////////////////////
	// y1 starts on the left
	//Y1_OnLeft = 1;

	r0 = bgr0 & 0xff;
	r1 = bgr1 & 0xff;
	r2 = bgr2 & 0xff;
	g0 = ( bgr0 >> 8 ) & 0xff;
	g1 = ( bgr1 >> 8 ) & 0xff;
	g2 = ( bgr2 >> 8 ) & 0xff;
	b0 = ( bgr0 >> 16 ) & 0xff;
	b1 = ( bgr1 >> 16 ) & 0xff;
	b2 = ( bgr2 >> 16 ) & 0xff;

	
	
	///////////////////////////////////
	// put top coordinates in x0,y0
	if ( y1 < y0 )
	{
		Swap ( x0, x1 );
		Swap ( y0, y1 );
		
		Swap ( r0, r1 );
		Swap ( g0, g1 );
		Swap ( b0, b1 );
	}
	
	if ( y2 < y0 )
	{
		Swap ( x0, x2 );
		Swap ( y0, y2 );
		
		Swap ( r0, r2 );
		Swap ( g0, g2 );
		Swap ( b0, b2 );
	}
	
	///////////////////////////////////////
	// put middle coordinates in x1,y1
	if ( y2 < y1 )
	{
		Swap ( x1, x2 );
		Swap ( y1, y2 );
		
		Swap ( r1, r2 );
		Swap ( g1, g2 );
		Swap ( b1, b2 );
	}
	
	// get the left/right most x
	LeftMostX = ( ( x0 < x1 ) ? x0 : x1 );
	LeftMostX = ( ( x2 < LeftMostX ) ? x2 : LeftMostX );
	RightMostX = ( ( x0 > x1 ) ? x0 : x1 );
	RightMostX = ( ( x2 > RightMostX ) ? x2 : RightMostX );

	// check if sprite is within draw area
	if ( RightMostX <= ((s32)DrawArea_TopLeftX) || LeftMostX > ((s32)DrawArea_BottomRightX) || y2 <= ((s32)DrawArea_TopLeftY) || y0 > ((s32)DrawArea_BottomRightY) ) return;
	
	// skip drawing if distance between vertices is greater than max allowed by GPU
	if ( ( _Abs( x1 - x0 ) > c_MaxPolygonWidth ) || ( _Abs( x2 - x1 ) > c_MaxPolygonWidth ) || ( y1 - y0 > c_MaxPolygonHeight ) || ( y2 - y1 > c_MaxPolygonHeight ) )
	{
		// skip drawing polygon
		return;
	}
	
	
	////////////////////////////////////////////////
	// *** TODO *** get length of longest scanline
	//long long height = y2 - y0;
	//if ( !height ) return;
	//long long temp = ( ( (s64) ( y1 - y0 ) ) << 32 ) / height;
	//long long longest = ( temp * ( (s64) ( x2 - x0 ) ) ) + ( ( (s64) ( x0 - x1 ) ) << 32 );
	//if ( !longest ) return;
	
	// get the absolute value of longest scanline
	//longest = _Abs ( longest );
	
	// calculate across
	// denominator is negative when x1 is on the left, positive when x1 is on the right
	s64 t0 = y1 - y2;
	s64 t1 = y0 - y2;
	s64 denominator = ( ( x0 - x2 ) * t0 ) - ( ( x1 - x2 ) * t1 );
	if ( denominator )
	{
		dR_across = ( ( (s64) ( ( ( r0 - r2 ) * t0 ) - ( ( r1 - r2 ) * t1 ) ) ) << 24 ) / denominator;
		dG_across = ( ( (s64) ( ( ( g0 - g2 ) * t0 ) - ( ( g1 - g2 ) * t1 ) ) ) << 24 ) / denominator;
		dB_across = ( ( (s64) ( ( ( b0 - b2 ) * t0 ) - ( ( b1 - b2 ) * t1 ) ) ) << 24 ) / denominator;
		
		//denominator = ( 1ll << 32 ) / denominator;
		//dR_across = ( ( (s64) ( ( ( r0 - r2 ) * t0 ) - ( ( r1 - r2 ) * t1 ) ) ) * denominator ) >> 8;
		//dG_across = ( ( (s64) ( ( ( g0 - g2 ) * t0 ) - ( ( g1 - g2 ) * t1 ) ) ) * denominator ) >> 8;
		//dB_across = ( ( (s64) ( ( ( b0 - b2 ) * t0 ) - ( ( b1 - b2 ) * t1 ) ) ) * denominator ) >> 8;
	}
	
	//debug << dec << "\r\nx0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2;
	//debug << "\r\nfixed: denominator=" << denominator;
	
	/////////////////////////////////////////////////
	// draw top part of triangle
	

	///////////////////////////////////////////
	// start at y0
	//Line = y0;
	
	/////////////////////////////////////////////
	// init x on the left and right
	x_left = ( ((s64)x0) << 32 );
	x_right = x_left;
	
		
	//R_left = ( ((s64)r0) << 32 );
	//G_left = ( ((s64)g0) << 32 );
	//B_left = ( ((s64)b0) << 32 );
	R_left = ( ((s64)r0) << 24 );
	G_left = ( ((s64)g0) << 24 );
	B_left = ( ((s64)b0) << 24 );
	R_right = R_left;
	G_right = G_left;
	B_right = B_left;
	

	////////////////////////////////////
	// get slopes
	
	if ( y1 - y0 )
	{
		dx_left = (((s64)( x1 - x0 )) << 32 ) / ((s64)( y1 - y0 ));
		dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
		
		//dR_left = (((s64)( r1 - r0 )) << 32 ) / ((s64)( y1 - y0 ));
		//dG_left = (((s64)( g1 - g0 )) << 32 ) / ((s64)( y1 - y0 ));
		//dB_left = (((s64)( b1 - b0 )) << 32 ) / ((s64)( y1 - y0 ));
		dR_left = (((s64)( r1 - r0 )) << 24 ) / ((s64)( y1 - y0 ));
		dG_left = (((s64)( g1 - g0 )) << 24 ) / ((s64)( y1 - y0 ));
		dB_left = (((s64)( b1 - b0 )) << 24 ) / ((s64)( y1 - y0 ));
		
		//dR_right = (((s64)( r2 - r0 )) << 32 ) / ((s64)( y2 - y0 ));
		//dG_right = (((s64)( g2 - g0 )) << 32 ) / ((s64)( y2 - y0 ));
		//dB_right = (((s64)( b2 - b0 )) << 32 ) / ((s64)( y2 - y0 ));
		dR_right = (((s64)( r2 - r0 )) << 24 ) / ((s64)( y2 - y0 ));
		dG_right = (((s64)( g2 - g0 )) << 24 ) / ((s64)( y2 - y0 ));
		dB_right = (((s64)( b2 - b0 )) << 24 ) / ((s64)( y2 - y0 ));
	}
	else
	{
		// change x_left and x_right where y1 is on left
		x_left = ( ((s64)x1) << 32 );
		x_right = ( ((s64)x0) << 32 );
		
		//R_left = ( ((s64)r1) << 32 );
		//R_right = ( ((s64)r0) << 32 );
		//G_left = ( ((s64)g1) << 32 );
		//G_right = ( ((s64)g0) << 32 );
		//B_left = ( ((s64)b1) << 32 );
		//B_right = ( ((s64)b0) << 32 );
		R_left = ( ((s64)r1) << 24 );
		R_right = ( ((s64)r0) << 24 );
		G_left = ( ((s64)g1) << 24 );
		G_right = ( ((s64)g0) << 24 );
		B_left = ( ((s64)b1) << 24 );
		B_right = ( ((s64)b0) << 24 );
		
		// this means that x0 and x1 are on the same line
		// so if the height of the entire polygon is zero, then we are done
		if ( y2 - y1 )
		{
			dx_left = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
		
			//dR_left = (((s64)( r2 - r1 )) << 32 ) / ((s64)( y2 - y1 ));
			//dG_left = (((s64)( g2 - g1 )) << 32 ) / ((s64)( y2 - y1 ));
			//dB_left = (((s64)( b2 - b1 )) << 32 ) / ((s64)( y2 - y1 ));
			dR_left = (((s64)( r2 - r1 )) << 24 ) / ((s64)( y2 - y1 ));
			dG_left = (((s64)( g2 - g1 )) << 24 ) / ((s64)( y2 - y1 ));
			dB_left = (((s64)( b2 - b1 )) << 24 ) / ((s64)( y2 - y1 ));
			
			//dR_right = (((s64)( r2 - r0 )) << 32 ) / ((s64)( y2 - y0 ));
			//dG_right = (((s64)( g2 - g0 )) << 32 ) / ((s64)( y2 - y0 ));
			//dB_right = (((s64)( b2 - b0 )) << 32 ) / ((s64)( y2 - y0 ));
			dR_right = (((s64)( r2 - r0 )) << 24 ) / ((s64)( y2 - y0 ));
			dG_right = (((s64)( g2 - g0 )) << 24 ) / ((s64)( y2 - y0 ));
			dB_right = (((s64)( b2 - b0 )) << 24 ) / ((s64)( y2 - y0 ));
		}
	}

	/////////////////////////////////////////////////
	// swap left and right if they are reversed
	//if ( ( ( x_right + dx_right ) < ( x_left + dx_left ) ) || ( x_right < x_left ) )
	if ( denominator > 0 )
	{
		// x1,y1 is on the right //
		
		//Y1_OnLeft = 0;
		
		Swap ( x_left, x_right );
		Swap ( dx_left, dx_right );

		Swap ( dR_left, dR_right );
		Swap ( dG_left, dG_right );
		Swap ( dB_left, dB_right );

		Swap ( R_left, R_right );
		Swap ( G_left, G_right );
		Swap ( B_left, B_right );
	}
	
	
	
	
	StartY = y0;
	EndY = y1;

	if ( StartY < ((s32)DrawArea_TopLeftY) )
	{
		
		if ( EndY < ((s32)DrawArea_TopLeftY) )
		{
			Temp = EndY - StartY;
			StartY = EndY;
		}
		else
		{
			Temp = DrawArea_TopLeftY - StartY;
			StartY = DrawArea_TopLeftY;
		}
		
		x_left += dx_left * Temp;
		x_right += dx_right * Temp;
		
		R_left += dR_left * Temp;
		G_left += dG_left * Temp;
		B_left += dB_left * Temp;
		
		R_right += dR_right * Temp;
		G_right += dG_right * Temp;
		B_right += dB_right * Temp;
	}
	
	if ( EndY > ((s32)DrawArea_BottomRightY) )
	{
		EndY = DrawArea_BottomRightY + 1;
	}

	
	//////////////////////////////////////////////
	// draw down to y1
	for ( Line = StartY; Line < EndY; Line++ )
	{
		StartX = _Round( x_left ) >> 32;
		EndX = _Round( x_right ) >> 32;
		
		if ( StartX <= ((s32)DrawArea_BottomRightX) && EndX > ((s32)DrawArea_TopLeftX) )
		{
			/*
			dR_across = 0;
			dG_across = 0;
			dB_across = 0;
			
			if ( StartX != EndX )
			{
				//dR_across = ( ( ( R_right - R_left ) << 16 ) / ( x_right - x_left ) ) << 16;
				//dG_across = ( ( ( G_right - G_left ) << 16 ) / ( x_right - x_left ) ) << 16;
				//dB_across = ( ( ( B_right - B_left ) << 16 ) / ( x_right - x_left ) ) << 16;
				dR_across = ( ( ( R_right - R_left ) << ( 31 ) ) / ( x_right - x_left ) ) << 1;
				dG_across = ( ( ( G_right - G_left ) << ( 31 ) ) / ( x_right - x_left ) ) << 1;
				dB_across = ( ( ( B_right - B_left ) << ( 31 ) ) / ( x_right - x_left ) ) << 1;
				
				debug << "\r\nactual: y=" << Line << " dR_across=" << dR_across << " dG_across=" << dG_across << " dB_across=" << dB_across;

				
#ifdef _ENABLE_SSE2
				vdR_across = _mm_set1_epi32( ( dR_across ) * c_iVectorSize );
				vdG_across = _mm_set1_epi32( ( dG_across ) * c_iVectorSize );
				vdB_across = _mm_set1_epi32( ( dB_across ) * c_iVectorSize );
#endif
			}
			*/
			
			iR = R_left;
			iG = G_left;
			iB = B_left;
			
			if ( StartX < ((s32)DrawArea_TopLeftX) )
			{
				Temp = DrawArea_TopLeftX - StartX;
				StartX = DrawArea_TopLeftX;
				
				iR += dR_across * Temp;
				iG += dG_across * Temp;
				iB += dB_across * Temp;
			}
			
			if ( EndX > ((s32)DrawArea_BottomRightX) )
			{
				EndX = DrawArea_BottomRightX + 1;
				//EndX = DrawArea_BottomRightX;
			}
			
			ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			DitherLine = & ( DitherArray [ ( Line & 0x3 ) << 2 ] );
			
			if ( StartX == EndX && !dx_left ) EndX += 1;
			//if ( EndX >= RightMostX ) EndX--;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;
			
#ifdef _ENABLE_SSE2
	vx_across = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX );
	
	viR1 = _mm_add_epi32 ( _mm_set1_epi32 ( iR ), _custom_mul_32 ( _mm_set1_epi32 ( dR_across ), vSeq32_1 ) );
	viR2 = _mm_add_epi32 ( _mm_set1_epi32 ( iR ), _custom_mul_32 ( _mm_set1_epi32 ( dR_across ), vSeq32_2 ) );

	viG1 = _mm_add_epi32 ( _mm_set1_epi32 ( iG ), _custom_mul_32 ( _mm_set1_epi32 ( dG_across ), vSeq32_1 ) );
	viG2 = _mm_add_epi32 ( _mm_set1_epi32 ( iG ), _custom_mul_32 ( _mm_set1_epi32 ( dG_across ), vSeq32_2 ) );

	viB1 = _mm_add_epi32 ( _mm_set1_epi32 ( iB ), _custom_mul_32 ( _mm_set1_epi32 ( dB_across ), vSeq32_1 ) );
	viB2 = _mm_add_epi32 ( _mm_set1_epi32 ( iB ), _custom_mul_32 ( _mm_set1_epi32 ( dB_across ), vSeq32_2 ) );
	
	vDitherValue_add = _mm_loadu_si128 ((__m128i const*) ( &(vDitherArray_add [ ( StartX & 0x3 ) + ( ( Line & 0x3 ) << 4 ) ]) ));
	vDitherValue_sub = _mm_loadu_si128 ((__m128i const*) ( &(vDitherArray_sub [ ( StartX & 0x3 ) + ( ( Line & 0x3 ) << 4 ) ]) ));
#endif

//#define TEST10

//#ifdef TEST10
//	debug << hex << "\r\nr0=" << r0 << " r1=" << r1 << " r2=" << r2 << " g0=" << g0 << " g1=" << g1 << " g2=" << g2;
//	debug << " dR_left=" << dR_left << " dR_right=" << dR_right << " dG_left=" << dG_left << " dG_right=" << dG_right << " dB_left=" << dB_left << " dB_right=" << dB_right;
//	debug << hex << "\r\niR=" << iR << " iG=" << iG << " iB=" << iB << " dR_across=" << dR_across << " dG_across=" << dG_across << " dB_across=" << dB_across;
//	debug << " R_left=" << R_left << " R_right=" << R_right << " G_left=" << G_left << " G_right=" << G_right << " B_left=" << B_left << " B_right=" << B_right;
//#endif

			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across < EndX; x_across++ )
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
#ifdef _ENABLE_SSE2
					vRed = _mm_packs_epi32 ( _mm_srai_epi32 ( viR1, 16 ), _mm_srai_epi32 ( viR2, 16 ) );
					vRed = _mm_adds_epu16 ( vRed, vDitherValue_add );
					vRed = _mm_subs_epu16 ( vRed, vDitherValue_sub );
					
					vGreen = _mm_packs_epi32 ( _mm_srai_epi32 ( viG1, 16 ), _mm_srai_epi32 ( viG2, 16 ) );
					vGreen = _mm_adds_epu16 ( vGreen, vDitherValue_add );
					vGreen = _mm_subs_epu16 ( vGreen, vDitherValue_sub );
					
					vBlue = _mm_packs_epi32 ( _mm_srai_epi32 ( viB1, 16 ), _mm_srai_epi32 ( viB2, 16 ) );
					vBlue = _mm_adds_epu16 ( vBlue, vDitherValue_add );
					vBlue = _mm_subs_epu16 ( vBlue, vDitherValue_sub );
					
					vRed = _mm_srli_epi16 ( vRed, 11 );
					vGreen = _mm_srli_epi16 ( vGreen, 11 );
					vBlue = _mm_srli_epi16 ( vBlue, 11 );
					
					//vRed = _mm_slli_epi16 ( vRed, 0 );
					vGreen = _mm_slli_epi16 ( vGreen, 5 );
					vBlue = _mm_slli_epi16 ( vBlue, 10 );
					
					vbgr_temp = _mm_or_si128 ( _mm_or_si128 ( vRed, vGreen ), vBlue );
					
					DestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
					//vbgr_temp = vbgr;
					if ( command_abe ) vbgr_temp = vSemiTransparency16( DestPixel, vbgr_temp, GPU_CTRL_Read.ABR );
					vbgr_temp = _mm_or_si128 ( vbgr_temp, SetPixelMask );
					_mm_maskmoveu_si128 ( vbgr_temp, _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( DestPixel, 15 ), PixelMask ), _mm_cmplt_epi16 ( vx_across, vEndX ) ), (char*) ptr );
					
					
					vx_across = _mm_add_epi16 ( vx_across, vVectorSize );
					
					viR1 = _mm_add_epi32 ( viR1, vdR_across );
					viR2 = _mm_add_epi32 ( viR2, vdR_across );
					viG1 = _mm_add_epi32 ( viG1, vdG_across );
					viG2 = _mm_add_epi32 ( viG2, vdG_across );
					viB1 = _mm_add_epi32 ( viB1, vdB_across );
					viB2 = _mm_add_epi32 ( viB2, vdB_across );
#else
					//bgr = ( _Round( iR ) >> 32 ) | ( ( _Round( iG ) >> 32 ) << 8 ) | ( ( _Round( iB ) >> 32 ) << 16 );
					//bgr = ( _Round( iR ) >> 35 ) | ( ( _Round( iG ) >> 35 ) << 5 ) | ( ( _Round( iB ) >> 35 ) << 10 );
					DitherValue = DitherLine [ x_across & 0x3 ];
					
					// perform dither
					Red = iR + DitherValue;
					Green = iG + DitherValue;
					Blue = iB + DitherValue;
					
					//Red = Clamp5 ( ( iR + DitherValue ) >> 27 );
					//Green = Clamp5 ( ( iG + DitherValue ) >> 27 );
					//Blue = Clamp5 ( ( iB + DitherValue ) >> 27 );
					
					// perform shift
					Red >>= 27;
					Green >>= 27;
					Blue >>= 27;
					
					// if dithering, perform signed clamp to 5 bits
					Red = AddSignedClamp<s64,5> ( Red );
					Green = AddSignedClamp<s64,5> ( Green );
					Blue = AddSignedClamp<s64,5> ( Blue );
					
					bgr = ( Blue << 10 ) | ( Green << 5 ) | Red;
					
					// shade pixel color
				
					// read pixel from frame buffer if we need to check mask bit
					DestPixel = *ptr;
					
					// *** testing ***
					//debug << "\r\nDestPixel=" << hex << DestPixel;
					
					bgr_temp = bgr;
		
					// *** testing ***
					//debug << " (before)bgr_temp=" << hex << bgr_temp;
					
					// semi-transparency
					if ( command_abe )
					{
						bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read.ABR );
					}
					
					// check if we should set mask bit when drawing
					bgr_temp |= SetPixelMask;

					// *** testing ***
					//debug << " (before)bgr_temp=" << hex << bgr_temp;
					//debug << " SetPixelMask=" << SetPixelMask << " PixelMask=" << PixelMask << " DestPixel=" << DestPixel;
					
					// draw pixel if we can draw to mask pixels or mask bit not set
					if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
						
					iR += dR_across;
					iG += dG_across;
					iB += dB_across;
#endif
				
				//ptr++;
				ptr += c_iVectorSize;
			}
			
		}
		
		//////////////////////////////////
		// draw next line
		//Line++;
		
		/////////////////////////////////////
		// update x on left and right
		x_left += dx_left;
		x_right += dx_right;
		
		R_left += dR_left;
		G_left += dG_left;
		B_left += dB_left;
		R_right += dR_right;
		G_right += dG_right;
		B_right += dB_right;
	}

	
	////////////////////////////////////////////////
	// draw bottom part of triangle

	/////////////////////////////////////////////
	// init x on the left and right
	
	//////////////////////////////////////////////////////
	// check if y1 is on the left or on the right
	//if ( Y1_OnLeft )
	if ( denominator < 0 )
	{
		x_left = ( ((s64)x1) << 32 );

		//R_left = ( ((s64)r1) << 32 );
		//G_left = ( ((s64)g1) << 32 );
		//B_left = ( ((s64)b1) << 32 );
		R_left = ( ((s64)r1) << 24 );
		G_left = ( ((s64)g1) << 24 );
		B_left = ( ((s64)b1) << 24 );
		
		if ( y2 - y1 )
		{
			dx_left = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			
			//dR_left = (((s64)( r2 - r1 )) << 32 ) / ((s64)( y2 - y1 ));
			//dG_left = (((s64)( g2 - g1 )) << 32 ) / ((s64)( y2 - y1 ));
			//dB_left = (((s64)( b2 - b1 )) << 32 ) / ((s64)( y2 - y1 ));
			dR_left = (((s64)( r2 - r1 )) << 24 ) / ((s64)( y2 - y1 ));
			dG_left = (((s64)( g2 - g1 )) << 24 ) / ((s64)( y2 - y1 ));
			dB_left = (((s64)( b2 - b1 )) << 24 ) / ((s64)( y2 - y1 ));
		}
	}
	else
	{
		x_right = ( ((s64)x1) << 32 );

		//R_right = ( ((s64)r1) << 32 );
		//G_right = ( ((s64)g1) << 32 );
		//B_right = ( ((s64)b1) << 32 );
		R_right = ( ((s64)r1) << 24 );
		G_right = ( ((s64)g1) << 24 );
		B_right = ( ((s64)b1) << 24 );
		
		if ( y2 - y1 )
		{
			dx_right = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			
			//dR_right = (((s64)( r2 - r1 )) << 32 ) / ((s64)( y2 - y1 ));
			//dG_right = (((s64)( g2 - g1 )) << 32 ) / ((s64)( y2 - y1 ));
			//dB_right = (((s64)( b2 - b1 )) << 32 ) / ((s64)( y2 - y1 ));
			dR_right = (((s64)( r2 - r1 )) << 24 ) / ((s64)( y2 - y1 ));
			dG_right = (((s64)( g2 - g1 )) << 24 ) / ((s64)( y2 - y1 ));
			dB_right = (((s64)( b2 - b1 )) << 24 ) / ((s64)( y2 - y1 ));
		}
	}
	
	// *** testing ***
	//debug << "\r\nfixed: dR_across=" << dR_across << " dG_across=" << dG_across << " dB_across=" << dB_across;
	
	// the line starts at y1 from here
	//Line = y1;

	StartY = y1;
	EndY = y2;

	if ( StartY < ((s32)DrawArea_TopLeftY) )
	{
		
		if ( EndY < ((s32)DrawArea_TopLeftY) )
		{
			Temp = EndY - StartY;
			StartY = EndY;
		}
		else
		{
			Temp = DrawArea_TopLeftY - StartY;
			StartY = DrawArea_TopLeftY;
		}
		
		x_left += dx_left * Temp;
		x_right += dx_right * Temp;
		
		R_left += dR_left * Temp;
		G_left += dG_left * Temp;
		B_left += dB_left * Temp;
		
		R_right += dR_right * Temp;
		G_right += dG_right * Temp;
		B_right += dB_right * Temp;
	}
	
	if ( EndY > ((s32)DrawArea_BottomRightY) )
	{
		EndY = DrawArea_BottomRightY + 1;
	}

	
	//////////////////////////////////////////////
	// draw down to y2
	for ( Line = StartY; Line < EndY; Line++ )
	{
		StartX = _Round( x_left ) >> 32;
		EndX = _Round( x_right ) >> 32;
		
		if ( StartX <= ((s32)DrawArea_BottomRightX) && EndX > ((s32)DrawArea_TopLeftX) )
		{
			/*
			dR_across = 0;
			dG_across = 0;
			dB_across = 0;
			
			if ( StartX != EndX )
			{
				//dR_across = ( ( ( R_right - R_left ) << 23 ) / ( x_right - x_left ) );
				//dG_across = ( ( ( G_right - G_left ) << 23 ) / ( x_right - x_left ) );
				//dB_across = ( ( ( B_right - B_left ) << 23 ) / ( x_right - x_left ) );
				dR_across = ( ( ( R_right - R_left ) << ( 31 ) ) / ( x_right - x_left ) ) << 1;
				dG_across = ( ( ( G_right - G_left ) << ( 31 ) ) / ( x_right - x_left ) ) << 1;
				dB_across = ( ( ( B_right - B_left ) << ( 31 ) ) / ( x_right - x_left ) ) << 1;
				
				debug << "\r\nactual: y=" << Line << " dR_across=" << dR_across << " dG_across=" << dG_across << " dB_across=" << dB_across;
				
#ifdef _ENABLE_SSE2
				vdR_across = _mm_set1_epi32( ( dR_across ) * c_iVectorSize );
				vdG_across = _mm_set1_epi32( ( dG_across ) * c_iVectorSize );
				vdB_across = _mm_set1_epi32( ( dB_across ) * c_iVectorSize );
#endif
			}
			*/
			
			iR = R_left;
			iG = G_left;
			iB = B_left;
			
			if ( StartX < ((s32)DrawArea_TopLeftX) )
			{
				Temp = DrawArea_TopLeftX - StartX;
				StartX = DrawArea_TopLeftX;
				
				iR += dR_across * Temp;
				iG += dG_across * Temp;
				iB += dB_across * Temp;
			}
			
			if ( EndX > ((s32)DrawArea_BottomRightX) )
			{
				EndX = DrawArea_BottomRightX + 1;
				//EndX = DrawArea_BottomRightX;
			}
			
			ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			DitherLine = & ( DitherArray [ ( Line & 0x3 ) << 2 ] );
			
			if ( StartX == EndX && !dx_left ) EndX += 1;
			//if ( EndX >= RightMostX ) EndX--;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;
			
#ifdef _ENABLE_SSE2
	vx_across = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX );
	
	viR1 = _mm_add_epi32 ( _mm_set1_epi32 ( iR ), _custom_mul_32 ( _mm_set1_epi32 ( dR_across ), vSeq32_1 ) );
	viR2 = _mm_add_epi32 ( _mm_set1_epi32 ( iR ), _custom_mul_32 ( _mm_set1_epi32 ( dR_across ), vSeq32_2 ) );

	viG1 = _mm_add_epi32 ( _mm_set1_epi32 ( iG ), _custom_mul_32 ( _mm_set1_epi32 ( dG_across ), vSeq32_1 ) );
	viG2 = _mm_add_epi32 ( _mm_set1_epi32 ( iG ), _custom_mul_32 ( _mm_set1_epi32 ( dG_across ), vSeq32_2 ) );

	viB1 = _mm_add_epi32 ( _mm_set1_epi32 ( iB ), _custom_mul_32 ( _mm_set1_epi32 ( dB_across ), vSeq32_1 ) );
	viB2 = _mm_add_epi32 ( _mm_set1_epi32 ( iB ), _custom_mul_32 ( _mm_set1_epi32 ( dB_across ), vSeq32_2 ) );
	
	vDitherValue_add = _mm_loadu_si128 ((__m128i const*) ( &(vDitherArray_add [ ( StartX & 0x3 ) + ( ( Line & 0x3 ) << 4 ) ]) ));
	vDitherValue_sub = _mm_loadu_si128 ((__m128i const*) ( &(vDitherArray_sub [ ( StartX & 0x3 ) + ( ( Line & 0x3 ) << 4 ) ]) ));
#endif

			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across < EndX; x_across++ )
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
#ifdef _ENABLE_SSE2
					vRed = _mm_packs_epi32 ( _mm_srai_epi32 ( viR1, 16 ), _mm_srai_epi32 ( viR2, 16 ) );
					vRed = _mm_adds_epu16 ( vRed, vDitherValue_add );
					vRed = _mm_subs_epu16 ( vRed, vDitherValue_sub );
					
					vGreen = _mm_packs_epi32 ( _mm_srai_epi32 ( viG1, 16 ), _mm_srai_epi32 ( viG2, 16 ) );
					vGreen = _mm_adds_epu16 ( vGreen, vDitherValue_add );
					vGreen = _mm_subs_epu16 ( vGreen, vDitherValue_sub );
					
					vBlue = _mm_packs_epi32 ( _mm_srai_epi32 ( viB1, 16 ), _mm_srai_epi32 ( viB2, 16 ) );
					vBlue = _mm_adds_epu16 ( vBlue, vDitherValue_add );
					vBlue = _mm_subs_epu16 ( vBlue, vDitherValue_sub );
					
					vRed = _mm_srli_epi16 ( vRed, 11 );
					vGreen = _mm_srli_epi16 ( vGreen, 11 );
					vBlue = _mm_srli_epi16 ( vBlue, 11 );
					
					//vRed = _mm_slli_epi16 ( vRed, 0 );
					vGreen = _mm_slli_epi16 ( vGreen, 5 );
					vBlue = _mm_slli_epi16 ( vBlue, 10 );
					
					vbgr_temp = _mm_or_si128 ( _mm_or_si128 ( vRed, vGreen ), vBlue );
					
					DestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
					//vbgr_temp = vbgr;
					if ( command_abe ) vbgr_temp = vSemiTransparency16( DestPixel, vbgr_temp, GPU_CTRL_Read.ABR );
					vbgr_temp = _mm_or_si128 ( vbgr_temp, SetPixelMask );
					_mm_maskmoveu_si128 ( vbgr_temp, _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( DestPixel, 15 ), PixelMask ), _mm_cmplt_epi16 ( vx_across, vEndX ) ), (char*) ptr );
					
					
					vx_across = _mm_add_epi16 ( vx_across, vVectorSize );
					
					viR1 = _mm_add_epi32 ( viR1, vdR_across );
					viR2 = _mm_add_epi32 ( viR2, vdR_across );
					viG1 = _mm_add_epi32 ( viG1, vdG_across );
					viG2 = _mm_add_epi32 ( viG2, vdG_across );
					viB1 = _mm_add_epi32 ( viB1, vdB_across );
					viB2 = _mm_add_epi32 ( viB2, vdB_across );
#else
					//bgr = ( _Round( iR ) >> 32 ) | ( ( _Round( iG ) >> 32 ) << 8 ) | ( ( _Round( iB ) >> 32 ) << 16 );
					//bgr = ( _Round( iR ) >> 35 ) | ( ( _Round( iG ) >> 35 ) << 5 ) | ( ( _Round( iB ) >> 35 ) << 10 );
					DitherValue = DitherLine [ x_across & 0x3 ];
					//bgr = ( ( iR + DitherValue ) >> 35 ) | ( ( ( iG + DitherValue ) >> 35 ) << 5 ) | ( ( ( iB + DitherValue ) >> 35 ) << 10 );

					// perform dither
					Red = iR + DitherValue;
					Green = iG + DitherValue;
					Blue = iB + DitherValue;
					
					//Red = Clamp5 ( ( iR + DitherValue ) >> 27 );
					//Green = Clamp5 ( ( iG + DitherValue ) >> 27 );
					//Blue = Clamp5 ( ( iB + DitherValue ) >> 27 );
					
					// perform shift
					Red >>= 27;
					Green >>= 27;
					Blue >>= 27;
					
					// if dithering, perform signed clamp to 5 bits
					Red = AddSignedClamp<s64,5> ( Red );
					Green = AddSignedClamp<s64,5> ( Green );
					Blue = AddSignedClamp<s64,5> ( Blue );
					
					bgr = ( Blue << 10 ) | ( Green << 5 ) | Red;
					
					// shade pixel color
				
					// read pixel from frame buffer if we need to check mask bit
					DestPixel = *ptr;
					
					bgr_temp = bgr;
		
					// semi-transparency
					if ( command_abe )
					{
						bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read.ABR );
					}
					
					// check if we should set mask bit when drawing
					bgr_temp |= SetPixelMask;

					// draw pixel if we can draw to mask pixels or mask bit not set
					if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;

					
				iR += dR_across;
				iG += dG_across;
				iB += dB_across;
#endif
				
				//ptr++;
				ptr += c_iVectorSize;
			}
			
		}
		
		/////////////////////////////////////
		// update x on left and right
		x_left += dx_left;
		x_right += dx_right;
		
		R_left += dR_left;
		G_left += dG_left;
		B_left += dB_left;
		R_right += dR_right;
		G_right += dG_right;
		B_right += dB_right;
	}
		
}




// draw texture mapped triangle
void GPU::DrawTriangle_Texture ()
{
#ifdef _ENABLE_SSE2
	// with sse2, can send 8 pixels at a time
	static const int c_iVectorSize = 8;
#else
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;
#endif

	u32 clut_xoffset;

	u32 Pixel, TexelIndex, Y1_OnLeft;
	
	u32 color_add;
	
	u16 *ptr_texture, *ptr_clut;
	u16 *ptr;
	
	s32 StartX, EndX, StartY, EndY;
	s64 Temp, TMin, TMax;
	s64 LeftMostX, RightMostX;

#ifdef _ENABLE_SSE2
	__m128i DestPixel, PixelMask, SetPixelMask;
	__m128i vbgr, vbgr_temp, vStartX, vEndX, vx_across, vSeq, vVectorSize;
	__m128i vbgr_temp_transparent, vbgr_select;
	
	__m128i vSeq1_32, vSeq2_32;
	
	__m128i TexCoordX, TexCoordY, Mask, tMask;	//, vTexCoordX2, vTexCoordY1, vTexCoordY2;
	u32 And2 = 0;
	__m128i And1, Shift1, Shift2;
	__m128i vIndex1, vIndex2;
	__m128i viU, viU1, viU2, viV, viV1, viV2;
	__m128i vdV_across, vdU_across;
	
	//u32 TWYTWH, Not_TWH;
	__m128i TWXTWW, Not_TWW, TWYTWH, Not_TWH;
	__m128i color_add_r, color_add_g, color_add_b;
	__m128i vRound24;
	
	vRound24 = _mm_set1_epi32 ( 0x00800000 );
	
	color_add_r = _mm_set1_epi16 ( bgr & 0xff );
	color_add_g = _mm_set1_epi16 ( ( bgr >> 8 ) & 0xff );
	color_add_b = _mm_set1_epi16 ( ( bgr >> 16 ) & 0xff );
	
	Mask = _mm_set1_epi16 ( 0xff );
	tMask = _mm_set1_epi16 ( 0x8000 );
	
	vSeq1_32 = _mm_set_epi32 ( 3, 2, 1, 0 );
	vSeq2_32 = _mm_set_epi32 ( 7, 6, 5, 4 );
	
	TWYTWH = _mm_set1_epi16 ( ( ( TWY & TWH ) << 3 ) );
	TWXTWW = _mm_set1_epi16 ( ( ( TWX & TWW ) << 3 ) );
	
	Not_TWH = _mm_set1_epi16 ( ~( TWH << 3 ) );
	Not_TWW = _mm_set1_epi16 ( ~( TWW << 3 ) );
	
	And1 = _mm_setzero_si128 ();
	Shift1 = _mm_setzero_si128 ();
	
	//TextureOffset = _mm_set1_epi32 ( ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 ) );
	
	vSeq = _mm_set_epi16 ( 7, 6, 5, 4, 3, 2, 1, 0 );
	vVectorSize = _mm_set1_epi16 ( c_iVectorSize );
	
	vbgr = _mm_set1_epi16 ( bgr );
	PixelMask = _mm_setzero_si128 ();
	SetPixelMask = _mm_setzero_si128 ();
	if ( GPU_CTRL_Read.ME ) PixelMask = _mm_set1_epi16 ( 0x8080 );
	if ( GPU_CTRL_Read.MD ) SetPixelMask = _mm_set1_epi16 ( 0x8000 );
#else

	u32 DestPixel, PixelMask = 0, SetPixelMask = 0;
	u32 TexCoordX, TexCoordY;
	u32 Shift1 = 0, Shift2 = 0, And1 = 0, And2 = 0;
	

	s64 Error_Left;
	
	s64 TexOffset_X, TexOffset_Y;
	
	u32 TWYTWH, TWXTWW, Not_TWH, Not_TWW;
	
	TWYTWH = ( ( TWY & TWH ) << 3 );
	TWXTWW = ( ( TWX & TWW ) << 3 );
	
	Not_TWH = ~( TWH << 3 );
	Not_TWW = ~( TWW << 3 );
	
	if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
#endif
	
	// initialize number of pixels drawn
	NumberOfPixelsDrawn = 0;
	
	clut_xoffset = clut_x << 4;
	ptr_clut = & ( VRAM [ clut_y << 10 ] );
	ptr_texture = & ( VRAM [ ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 ) ] );
	
	// check for some important conditions
	if ( DrawArea_BottomRightX < DrawArea_TopLeftX )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightX < DrawArea_TopLeftX.\n";
		return;
	}
	
	if ( DrawArea_BottomRightY < DrawArea_TopLeftY )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightY < DrawArea_TopLeftY.\n";
		return;
	}
	
	
	//////////////////////////////////////////
	// get coordinates on screen
	x0 = DrawArea_OffsetX + x0;
	y0 = DrawArea_OffsetY + y0;
	x1 = DrawArea_OffsetX + x1;
	y1 = DrawArea_OffsetY + y1;
	x2 = DrawArea_OffsetX + x2;
	y2 = DrawArea_OffsetY + y2;
	
	
	////////////////////////////////
	// y1 starts on the left
	Y1_OnLeft = 1;
	
	color_add = bgr;

	//////////////////////////////////////////////////////
	// Get offset into color lookup table
	u32 ClutOffset = ( clut_x << 4 ) + ( clut_y << 10 );
	
	/////////////////////////////////////////////////////////
	// Get offset into texture page
	//u32 TextureOffset = ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 );
	
	
	if ( tpage_tp == 0 )
	{
		And2 = 0xf;
		
#ifdef _ENABLE_SSE2
		Shift1 = _mm_set_epi32 ( 0, 0, 0, 2 );
		Shift2 = _mm_set_epi32 ( 0, 0, 0, 2 );
		And1 = _mm_set1_epi16 ( 3 );
#else
		Shift1 = 2; Shift2 = 2;
		And1 = 3; And2 = 0xf;
#endif
	}
	else if ( tpage_tp == 1 )
	{
		And2 = 0xff;
		
#ifdef _ENABLE_SSE2
		Shift1 = _mm_set_epi32 ( 0, 0, 0, 1 );
		Shift2 = _mm_set_epi32 ( 0, 0, 0, 3 );
		And1 = _mm_set1_epi16 ( 1 );
#else
		Shift1 = 1; Shift2 = 3;
		And1 = 1; And2 = 0xff;
#endif
	}
	
	
	///////////////////////////////////
	// put top coordinates in x0,y0
	if ( y1 < y0 )
	{
		Swap ( x0, x1 );
		Swap ( y0, y1 );
		
		Swap ( u0, u1 );
		Swap ( v0, v1 );
	}
	
	if ( y2 < y0 )
	{
		Swap ( x0, x2 );
		Swap ( y0, y2 );
		
		Swap ( u0, u2 );
		Swap ( v0, v2 );
	}
	
	///////////////////////////////////////
	// put middle coordinates in x1,y1
	if ( y2 < y1 )
	{
		Swap ( x1, x2 );
		Swap ( y1, y2 );
		
		Swap ( u1, u2 );
		Swap ( v1, v2 );
	}
	
	// get the left/right most x
	LeftMostX = ( ( x0 < x1 ) ? x0 : x1 );
	LeftMostX = ( ( x2 < LeftMostX ) ? x2 : LeftMostX );
	RightMostX = ( ( x0 > x1 ) ? x0 : x1 );
	RightMostX = ( ( x2 > RightMostX ) ? x2 : RightMostX );

	// check if sprite is within draw area
	if ( RightMostX <= ((s32)DrawArea_TopLeftX) || LeftMostX > ((s32)DrawArea_BottomRightX) || y2 <= ((s32)DrawArea_TopLeftY) || y0 > ((s32)DrawArea_BottomRightY) ) return;
	
	// skip drawing if distance between vertices is greater than max allowed by GPU
	if ( ( _Abs( x1 - x0 ) > c_MaxPolygonWidth ) || ( _Abs( x2 - x1 ) > c_MaxPolygonWidth ) || ( y1 - y0 > c_MaxPolygonHeight ) || ( y2 - y1 > c_MaxPolygonHeight ) )
	{
		// skip drawing polygon
		return;
	}
	
	
	
	/////////////////////////////////////////////////
	// draw top part of triangle
	

	///////////////////////////////////////////
	// start at y0
	//Line = y0;
	
	/////////////////////////////////////////////
	// init x on the left and right
	x_left = ( ((s64)x0) << 32 );
	x_right = x_left;
	U_left = ( ((s64)u0) << 24 );
	V_left = ( ((s64)v0) << 24 );
	U_right = U_left;
	V_right = V_left;
		
	////////////////////////////////////
	// get slopes
	
	if ( y1 - y0 )
	{
		dx_left = (((s64)( x1 - x0 )) << 32 ) / ((s64)( y1 - y0 ));
		
		dU_left = (((s64)( u1 - u0 )) << 24 ) / ((s64)( y1 - y0 ));
		dV_left = (((s64)( v1 - v0 )) << 24 ) / ((s64)( y1 - y0 ));
		
		dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
		
		dU_right = (((s64)( u2 - u0 )) << 24 ) / ((s64)( y2 - y0 ));
		dV_right = (((s64)( v2 - v0 )) << 24 ) / ((s64)( y2 - y0 ));
	}
	else
	{
		// change x_left and x_right where y1 is on left
		x_left = ( ((s64)x1) << 32 );
		x_right = ( ((s64)x0) << 32 );
		
		// change U_left and V_right where y1 is on left
		U_left = ( ((s64)u1) << 24 );
		U_right = ( ((s64)u0) << 24 );
		V_left = ( ((s64)v1) << 24 );
		V_right = ( ((s64)v0) << 24 );
		
		// this means that x0 and x1 are on the same line
		// so if the height of the entire polygon is zero, then we are done
		if ( y2 - y1 )
		{
			dx_left = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			
			dU_left = (((s64)( u2 - u1 )) << 24 ) / ((s64)( y2 - y1 ));
			dV_left = (((s64)( v2 - v1 )) << 24 ) / ((s64)( y2 - y1 ));
			
			dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
		
			dU_right = (((s64)( u2 - u0 )) << 24 ) / ((s64)( y2 - y0 ));
			dV_right = (((s64)( v2 - v0 )) << 24 ) / ((s64)( y2 - y0 ));
		}
	}

	/////////////////////////////////////////////////
	// swap left and right if they are reversed
	if ( ( ( x_right + dx_right ) < ( x_left + dx_left ) ) || ( x_right < x_left ) )
	{
		// x1,y1 is on the right //
		
		Y1_OnLeft = 0;
		
		Swap ( x_left, x_right );
		Swap ( dx_left, dx_right );

		Swap ( dU_left, dU_right );
		Swap ( dV_left, dV_right );

		Swap ( U_left, U_right );
		Swap ( V_left, V_right );
	}
	
	////////////////
	// *** TODO *** at this point area of full triangle can be calculated and the rest of the drawing can be put on another thread *** //
	
	// calculate across
	s64 t0 = y1 - y2;
	s64 t1 = y0 - y2;
	s64 denominator = ( ( x0 - x2 ) * t0 ) - ( ( x1 - x2 ) * t1 );
	if ( denominator )
	{
		//dR_across = ( ( (s64) ( ( ( r0 - r2 ) * t0 ) - ( ( r1 - r2 ) * t1 ) ) ) << 24 ) / denominator;
		//dG_across = ( ( (s64) ( ( ( g0 - g2 ) * t0 ) - ( ( g1 - g2 ) * t1 ) ) ) << 24 ) / denominator;
		//dB_across = ( ( (s64) ( ( ( b0 - b2 ) * t0 ) - ( ( b1 - b2 ) * t1 ) ) ) << 24 ) / denominator;
		dU_across = ( ( (s64) ( ( ( u0 - u2 ) * t0 ) - ( ( u1 - u2 ) * t1 ) ) ) << 24 ) / denominator;
		dV_across = ( ( (s64) ( ( ( v0 - v2 ) * t0 ) - ( ( v1 - v2 ) * t1 ) ) ) << 24 ) / denominator;
	}
	
	
	StartY = y0;
	EndY = y1;

	if ( StartY < ((s32)DrawArea_TopLeftY) )
	{
		
		if ( EndY < ((s32)DrawArea_TopLeftY) )
		{
			Temp = EndY - StartY;
			StartY = EndY;
		}
		else
		{
			Temp = DrawArea_TopLeftY - StartY;
			StartY = DrawArea_TopLeftY;
		}
		
		x_left += dx_left * Temp;
		x_right += dx_right * Temp;
		
		U_left += dU_left * Temp;
		V_left += dV_left * Temp;
		
		U_right += dU_right * Temp;
		V_right += dV_right * Temp;
	}
	
	if ( EndY > ((s32)DrawArea_BottomRightY) )
	{
		EndY = DrawArea_BottomRightY + 1;
	}

	
	// *** testing ***
	//if ( StartY == EndY ) EndY += 1;

	
	//////////////////////////////////////////////
	// draw down to y1
	//while ( Line < y1 )
	for ( Line = StartY; Line < EndY; Line++ )
	{
		//cy = Line;

		StartX = _Round( x_left ) >> 32;
		EndX = _Round( x_right ) >> 32;
		
		//if ( Line >= ((s32)DrawArea_TopLeftY) && Line <= ((s32)DrawArea_BottomRightY) )
		//{
		if ( StartX <= ((s32)DrawArea_BottomRightX) && EndX > ((s32)DrawArea_TopLeftX) )
		{
			/*
			dU_across = 0;
			dV_across = 0;
				
			if ( StartX != EndX )
			{
				//dU_across = ( ( ( U_right - U_left ) << 16 ) / ( x_right - x_left ) ) << 16;
				//dV_across = ( ( ( V_right - V_left ) << 16 ) / ( x_right - x_left ) ) << 16;
				dU_across = ( ( ( U_right - U_left ) << 31 ) / ( x_right - x_left ) ) << 1;
				dV_across = ( ( ( V_right - V_left ) << 31 ) / ( x_right - x_left ) ) << 1;

#ifdef _ENABLE_SSE2
				vdU_across = _mm_set1_epi32( ( dU_across ) * c_iVectorSize );
				vdV_across = _mm_set1_epi32( ( dV_across ) * c_iVectorSize );
#endif
			}
			*/
			
			iU = U_left;
			iV = V_left;
			
			
			if ( EndX - StartX > 1 )
			{
				// if the right edge is straighter, then recalculate iu,iv
				if ( _Abs ( dU_right ) < _Abs ( dU_left ) )
				{
					iU = U_right - ( dU_across * ( EndX - StartX ) );
				}
				
				if ( _Abs ( dV_right ) < _Abs ( dV_left ) )
				{
					iV = V_right - ( dV_across * ( EndX - StartX ) );
				}
			}
			
			
			if ( StartX < ((s32)DrawArea_TopLeftX) )
			{
				Temp = DrawArea_TopLeftX - StartX;
				StartX = DrawArea_TopLeftX;
				
				iU += dU_across * Temp;
				iV += dV_across * Temp;
			}
			
			if ( EndX > ((s32)DrawArea_BottomRightX) )
			{
				EndX = DrawArea_BottomRightX + 1;
				//EndX = DrawArea_BottomRightX;
			}
			
			ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			
			// *** testing ***
			if ( StartX == EndX && !dx_left ) EndX += 1;
			//if ( EndX >= RightMostX ) EndX--;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;


#ifdef _ENABLE_SSE2
	vx_across = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX );
	
	viU1 = _mm_add_epi32 ( _mm_set1_epi32 ( iU ), _custom_mul_32 ( _mm_set1_epi32 ( dU_across ), vSeq1_32 ) );
	viU2 = _mm_add_epi32 ( _mm_set1_epi32 ( iU ), _custom_mul_32 ( _mm_set1_epi32 ( dU_across ), vSeq2_32 ) );

	viV1 = _mm_add_epi32 ( _mm_set1_epi32 ( iV ), _custom_mul_32 ( _mm_set1_epi32 ( dV_across ), vSeq1_32 ) );
	viV2 = _mm_add_epi32 ( _mm_set1_epi32 ( iV ), _custom_mul_32 ( _mm_set1_epi32 ( dV_across ), vSeq2_32 ) );
#endif


			// draw horizontal line
			// x_left and x_right need to be rounded off
			for ( x_across = StartX; x_across < EndX; x_across++ )
			{
#ifdef _ENABLE_SSE2
				viV =  _mm_packs_epi32 ( _mm_srli_epi32 ( _mm_add_epi32 ( viV1, vRound24 ), 24 ), _mm_srli_epi32 ( _mm_add_epi32 ( viV2, vRound24 ), 24 ) );
				viU =  _mm_packs_epi32 ( _mm_srli_epi32 ( _mm_add_epi32 ( viU1, vRound24 ), 24 ), _mm_srli_epi32 ( _mm_add_epi32 ( viU2, vRound24 ), 24 ) );
				
				TexCoordY = _mm_and_si128 ( _mm_or_si128 ( _mm_and_si128 ( viV, Not_TWH ), TWYTWH ), Mask );
				TexCoordX = _mm_and_si128 ( _mm_or_si128 ( _mm_and_si128 ( viU, Not_TWW ), TWXTWW ), Mask );
				vIndex1 = _mm_srl_epi16 ( TexCoordX, Shift1 );
				if ( And2 )
				{
					vIndex2 = _mm_sll_epi16 ( _mm_and_si128 ( TexCoordX, And1 ), Shift2 );
					
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 0 ) + ( _mm_extract_epi16 ( TexCoordY, 0 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 0 ) ) & And2 ) ) & FrameBuffer_XMask ], 0 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 1 ) + ( _mm_extract_epi16 ( TexCoordY, 1 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 1 ) ) & And2 ) ) & FrameBuffer_XMask ], 1 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 2 ) + ( _mm_extract_epi16 ( TexCoordY, 2 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 2 ) ) & And2 ) ) & FrameBuffer_XMask ], 2 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 3 ) + ( _mm_extract_epi16 ( TexCoordY, 3 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 3 ) ) & And2 ) ) & FrameBuffer_XMask ], 3 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 4 ) + ( _mm_extract_epi16 ( TexCoordY, 4 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 4 ) ) & And2 ) ) & FrameBuffer_XMask ], 4 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 5 ) + ( _mm_extract_epi16 ( TexCoordY, 5 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 5 ) ) & And2 ) ) & FrameBuffer_XMask ], 5 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 6 ) + ( _mm_extract_epi16 ( TexCoordY, 6 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 6 ) ) & And2 ) ) & FrameBuffer_XMask ], 6 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 7 ) + ( _mm_extract_epi16 ( TexCoordY, 7 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 7 ) ) & And2 ) ) & FrameBuffer_XMask ], 7 );
				}
				else
				{
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 0 ) + ( _mm_extract_epi16 ( TexCoordY, 0 ) << 10 ) ], 0 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 1 ) + ( _mm_extract_epi16 ( TexCoordY, 1 ) << 10 ) ], 1 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 2 ) + ( _mm_extract_epi16 ( TexCoordY, 2 ) << 10 ) ], 2 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 3 ) + ( _mm_extract_epi16 ( TexCoordY, 3 ) << 10 ) ], 3 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 4 ) + ( _mm_extract_epi16 ( TexCoordY, 4 ) << 10 ) ], 4 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 5 ) + ( _mm_extract_epi16 ( TexCoordY, 5 ) << 10 ) ], 5 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 6 ) + ( _mm_extract_epi16 ( TexCoordY, 6 ) << 10 ) ], 6 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 7 ) + ( _mm_extract_epi16 ( TexCoordY, 7 ) << 10 ) ], 7 );
				}
#else
					//TexCoordY = (u8) ( ( ( _Round ( iV ) >> 32 ) & ~( TWH << 3 ) ) | ( ( TWY & TWH ) << 3 ) );
					//TexCoordX = (u8) ( ( ( _Round ( iU ) >> 32 ) & ~( TWW << 3 ) ) | ( ( TWX & TWW ) << 3 ) );
					TexCoordY = (u8) ( ( ( _Round24 ( iV ) >> 24 ) & Not_TWH ) | TWYTWH );
					TexCoordX = (u8) ( ( ( _Round24 ( iU ) >> 24 ) & Not_TWW ) | TWXTWW );
					
					// *** testing ***
					//debug << dec << "\r\nTexCoordX=" << TexCoordX << " TexCoordY=" << TexCoordY;
					
					//bgr = VRAM [ TextureOffset + ( TexCoordX >> Shift1 ) + ( TexCoordY << 10 ) ];
					bgr = ptr_texture [ ( TexCoordX >> Shift1 ) + ( TexCoordY << 10 ) ];
					
					if ( Shift1 )
					{
						TexelIndex = ( bgr >> ( ( TexCoordX & And1 ) << Shift2 ) ) & And2;
						//bgr = VRAM [ ( ( ( clut_x << 4 ) + TexelIndex ) & FrameBuffer_XMask ) + ( clut_y << 10 ) ];
						bgr = ptr_clut [ ( clut_xoffset + TexelIndex ) & FrameBuffer_XMask ];
					}
#endif
					
					// *** testing ***
					//debug << "; TexelIndex=" << TexelIndex << hex << "; bgr=" << bgr;
					
#ifdef _ENABLE_SSE2
				DestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
				vbgr_temp = vbgr;
				if ( !command_tge ) vbgr_temp = vColorMultiply1624 ( vbgr_temp, color_add_r, color_add_g, color_add_b );
				if ( command_abe )
				{
					vbgr_temp_transparent = vSemiTransparency16( DestPixel, vbgr_temp, GPU_CTRL_Read.ABR );
					vbgr_select = _mm_srai_epi16 ( vbgr, 15 );
					vbgr_temp = _mm_or_si128 ( _mm_andnot_si128( vbgr_select, vbgr_temp ), _mm_and_si128 ( vbgr_select, vbgr_temp_transparent ) );
				}
				vbgr_temp = _mm_or_si128 ( _mm_or_si128 ( vbgr_temp, SetPixelMask ), _mm_and_si128 ( vbgr, tMask ) );
				_mm_maskmoveu_si128 ( vbgr_temp, _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( DestPixel, 15 ), PixelMask ), _mm_andnot_si128 ( _mm_cmpeq_epi16 ( vbgr, _mm_setzero_si128 () ), _mm_cmplt_epi16 ( vx_across, vEndX ) ) ), (char*) ptr );
				vx_across = _mm_add_epi16 ( vx_across, vVectorSize );
				//viU = _mm_add_epi16 ( viU, vVectorSize );
				viU1 = _mm_add_epi16 ( viU1, vdU_across );
				viU2 = _mm_add_epi16 ( viU2, vdU_across );
				viV1 = _mm_add_epi16 ( viV1, vdV_across );
				viV2 = _mm_add_epi16 ( viV2, vdV_across );
#else
					if ( bgr )
					{
						// shade pixel color
					
						// read pixel from frame buffer if we need to check mask bit
						//DestPixel = VRAM [ cx + ( cy << 10 ) ];
						DestPixel = *ptr;
						
						bgr_temp = bgr;
			
						if ( !command_tge )
						{
							// brightness calculation
							//bgr_temp = Color24To16 ( ColorMultiply24 ( Color16To24 ( bgr_temp ), color_add ) );
							bgr_temp = ColorMultiply1624 ( bgr_temp, color_add );
						}
						
						// semi-transparency
						if ( command_abe && ( bgr & 0x8000 ) )
						{
							bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, tpage_abr );
						}
						
						// check if we should set mask bit when drawing
						//if ( GPU_CTRL_Read.MD ) bgr_temp |= 0x8000;
						bgr_temp |= SetPixelMask | ( bgr & 0x8000 );

						// draw pixel if we can draw to mask pixels or mask bit not set
						//if ( ! ( DestPixel & PixelMask ) ) VRAM [ cx + ( cy << 10 ) ] = bgr_temp;
						if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
					}
					
					/////////////////////////////////////////////////////
					// update number of cycles used to draw polygon
					//NumberOfPixelsDrawn++;
				//}
				
				/////////////////////////////////////////////////////////
				// interpolate texture coords
				iU += dU_across;
				iV += dV_across;
#endif
				
				//ptr++;
				ptr += c_iVectorSize;
			}
			
		}
		
		/////////////////////////////////////
		// update x on left and right
		x_left += dx_left;
		x_right += dx_right;
		
		U_left += dU_left;
		V_left += dV_left;
		U_right += dU_right;
		V_right += dV_right;
	}

	
	////////////////////////////////////////////////
	// draw bottom part of triangle

	/////////////////////////////////////////////
	// init x on the left and right
	
	//////////////////////////////////////////////////////
	// check if y1 is on the left or on the right
	if ( Y1_OnLeft )
	{
		x_left = ( ((s64)x1) << 32 );

		U_left = ( ((s64)u1) << 24 );
		V_left = ( ((s64)v1) << 24 );

		if ( y2 - y1 )
		{
			dx_left = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			
			dU_left = (((s64)( u2 - u1 )) << 24 ) / ((s64)( y2 - y1 ));
			dV_left = (((s64)( v2 - v1 )) << 24 ) / ((s64)( y2 - y1 ));
		}
	}
	else
	{
		x_right = ( ((s64)x1) << 32 );

		U_right = ( ((s64)u1) << 24 );
		V_right = ( ((s64)v1) << 24 );

		if ( y2 - y1 )
		{
			dx_right = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			
			dU_right = (((s64)( u2 - u1 )) << 24 ) / ((s64)( y2 - y1 ));
			dV_right = (((s64)( v2 - v1 )) << 24 ) / ((s64)( y2 - y1 ));
		}
	}
	
	// the line starts at y1 from here
	//Line = y1;

	StartY = y1;
	EndY = y2;

	if ( StartY < ((s32)DrawArea_TopLeftY) )
	{
		
		if ( EndY < ((s32)DrawArea_TopLeftY) )
		{
			Temp = EndY - StartY;
			StartY = EndY;
		}
		else
		{
			Temp = DrawArea_TopLeftY - StartY;
			StartY = DrawArea_TopLeftY;
		}
		
		x_left += dx_left * Temp;
		x_right += dx_right * Temp;
		
		U_left += dU_left * Temp;
		V_left += dV_left * Temp;
		
		U_right += dU_right * Temp;
		V_right += dV_right * Temp;
	}
	
	if ( EndY > ((s32)DrawArea_BottomRightY) )
	{
		EndY = DrawArea_BottomRightY + 1;
	}

	// *** testing ***
	//if ( StartY == EndY ) EndY += 1;
	
	//////////////////////////////////////////////
	// draw down to y2
	//while ( Line < y2 )
	for ( Line = StartY; Line < EndY; Line++ )
	{
		//cy = Line;

		StartX = _Round( x_left ) >> 32;
		EndX = _Round( x_right ) >> 32;
		
		//if ( Line >= ((s32)DrawArea_TopLeftY) && Line <= ((s32)DrawArea_BottomRightY) )
		//{
		if ( StartX <= ((s32)DrawArea_BottomRightX) && EndX > ((s32)DrawArea_TopLeftX) )
		{
			/*
			dU_across = 0;
			dV_across = 0;
			
			if ( x_left != x_right )
			{
				//dU_across = ( ( ( U_right - U_left ) << 16 ) / ( x_right - x_left ) ) << 16;
				//dV_across = ( ( ( V_right - V_left ) << 16 ) / ( x_right - x_left ) ) << 16;
				dU_across = ( ( ( U_right - U_left ) << 31 ) / ( x_right - x_left ) ) << 1;
				dV_across = ( ( ( V_right - V_left ) << 31 ) / ( x_right - x_left ) ) << 1;
				
#ifdef _ENABLE_SSE2
				vdU_across = _mm_set1_epi32( ( dU_across ) * c_iVectorSize );
				vdV_across = _mm_set1_epi32( ( dV_across ) * c_iVectorSize );
#endif
			}
			*/
			
			iU = U_left;
			iV = V_left;
			

			if ( EndX - StartX > 1 )
			{
				// if the right edge is straighter, then recalculate iu,iv
				if ( _Abs ( dU_right ) < _Abs ( dU_left ) )
				{
					iU = U_right - ( dU_across * ( EndX - StartX ) );
				}
				
				if ( _Abs ( dV_right ) < _Abs ( dV_left ) )
				{
					iV = V_right - ( dV_across * ( EndX - StartX ) );
				}
			}
			
			
			if ( StartX < ((s32)DrawArea_TopLeftX) )
			{
				Temp = DrawArea_TopLeftX - StartX;
				StartX = DrawArea_TopLeftX;
				
				iU += dU_across * Temp;
				iV += dV_across * Temp;
			}
			
			if ( EndX > ((s32)DrawArea_BottomRightX) )
			{
				EndX = DrawArea_BottomRightX + 1;
				//EndX = DrawArea_BottomRightX;
			}
			
			ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			
			// *** testing ***
			if ( StartX == EndX && !dx_left ) EndX += 1;
			//if ( EndX >= RightMostX ) EndX--;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;
			
#ifdef _ENABLE_SSE2
	vx_across = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX );
	
	viU1 = _mm_add_epi32 ( _mm_set1_epi32 ( iU ), _custom_mul_32 ( _mm_set1_epi32 ( dU_across ), vSeq1_32 ) );
	viU2 = _mm_add_epi32 ( _mm_set1_epi32 ( iU ), _custom_mul_32 ( _mm_set1_epi32 ( dU_across ), vSeq2_32 ) );

	viV1 = _mm_add_epi32 ( _mm_set1_epi32 ( iV ), _custom_mul_32 ( _mm_set1_epi32 ( dV_across ), vSeq1_32 ) );
	viV2 = _mm_add_epi32 ( _mm_set1_epi32 ( iV ), _custom_mul_32 ( _mm_set1_epi32 ( dV_across ), vSeq2_32 ) );
#endif

			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across < EndX; x_across++ )
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
#ifdef _ENABLE_SSE2
				viV =  _mm_packs_epi32 ( _mm_srli_epi32 ( _mm_add_epi32 ( viV1, vRound24 ), 24 ), _mm_srli_epi32 ( _mm_add_epi32 ( viV2, vRound24 ), 24 ) );
				viU =  _mm_packs_epi32 ( _mm_srli_epi32 ( _mm_add_epi32 ( viU1, vRound24 ), 24 ), _mm_srli_epi32 ( _mm_add_epi32 ( viU2, vRound24 ), 24 ) );
				
				TexCoordY = _mm_and_si128 ( _mm_or_si128 ( _mm_and_si128 ( viV, Not_TWH ), TWYTWH ), Mask );
				TexCoordX = _mm_and_si128 ( _mm_or_si128 ( _mm_and_si128 ( viU, Not_TWW ), TWXTWW ), Mask );
				vIndex1 = _mm_srl_epi16 ( TexCoordX, Shift1 );
				if ( And2 )
				{
					vIndex2 = _mm_sll_epi16 ( _mm_and_si128 ( TexCoordX, And1 ), Shift2 );
					
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 0 ) + ( _mm_extract_epi16 ( TexCoordY, 0 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 0 ) ) & And2 ) ) & FrameBuffer_XMask ], 0 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 1 ) + ( _mm_extract_epi16 ( TexCoordY, 1 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 1 ) ) & And2 ) ) & FrameBuffer_XMask ], 1 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 2 ) + ( _mm_extract_epi16 ( TexCoordY, 2 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 2 ) ) & And2 ) ) & FrameBuffer_XMask ], 2 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 3 ) + ( _mm_extract_epi16 ( TexCoordY, 3 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 3 ) ) & And2 ) ) & FrameBuffer_XMask ], 3 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 4 ) + ( _mm_extract_epi16 ( TexCoordY, 4 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 4 ) ) & And2 ) ) & FrameBuffer_XMask ], 4 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 5 ) + ( _mm_extract_epi16 ( TexCoordY, 5 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 5 ) ) & And2 ) ) & FrameBuffer_XMask ], 5 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 6 ) + ( _mm_extract_epi16 ( TexCoordY, 6 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 6 ) ) & And2 ) ) & FrameBuffer_XMask ], 6 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 7 ) + ( _mm_extract_epi16 ( TexCoordY, 7 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 7 ) ) & And2 ) ) & FrameBuffer_XMask ], 7 );
				}
				else
				{
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 0 ) + ( _mm_extract_epi16 ( TexCoordY, 0 ) << 10 ) ], 0 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 1 ) + ( _mm_extract_epi16 ( TexCoordY, 1 ) << 10 ) ], 1 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 2 ) + ( _mm_extract_epi16 ( TexCoordY, 2 ) << 10 ) ], 2 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 3 ) + ( _mm_extract_epi16 ( TexCoordY, 3 ) << 10 ) ], 3 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 4 ) + ( _mm_extract_epi16 ( TexCoordY, 4 ) << 10 ) ], 4 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 5 ) + ( _mm_extract_epi16 ( TexCoordY, 5 ) << 10 ) ], 5 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 6 ) + ( _mm_extract_epi16 ( TexCoordY, 6 ) << 10 ) ], 6 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 7 ) + ( _mm_extract_epi16 ( TexCoordY, 7 ) << 10 ) ], 7 );
				}
#else
					//TexCoordY = (u8) ( ( ( _Round ( iV ) >> 32 ) & Not_TWH ) | TWYTWH );
					//TexCoordX = (u8) ( ( ( _Round ( iU ) >> 32 ) & Not_TWW ) | TWXTWW );
					TexCoordY = (u8) ( ( ( _Round24 ( iV ) >> 24 ) & Not_TWH ) | TWYTWH );
					TexCoordX = (u8) ( ( ( _Round24 ( iU ) >> 24 ) & Not_TWW ) | TWXTWW );
					
					// *** testing ***
					//debug << dec << "\r\nTexCoordX=" << TexCoordX << " TexCoordY=" << TexCoordY;
					
					//bgr = VRAM [ TextureOffset + ( TexCoordX >> Shift1 ) + ( TexCoordY << 10 ) ];
					bgr = ptr_texture [ ( TexCoordX >> Shift1 ) + ( TexCoordY << 10 ) ];
					
					
					if ( Shift1 )
					{
						TexelIndex = ( bgr >> ( ( TexCoordX & And1 ) << Shift2 ) ) & And2;
						//bgr = VRAM [ ( ( ( clut_x << 4 ) + TexelIndex ) & FrameBuffer_XMask ) + ( clut_y << 10 ) ];
						bgr = ptr_clut [ ( clut_xoffset + TexelIndex ) & FrameBuffer_XMask ];
					}
#endif
					
					// *** testing ***
					//debug << "; TexelIndex=" << TexelIndex << hex << "; bgr=" << bgr;
					
#ifdef _ENABLE_SSE2
				DestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
				vbgr_temp = vbgr;
				if ( !command_tge ) vbgr_temp = vColorMultiply1624 ( vbgr_temp, color_add_r, color_add_g, color_add_b );
				if ( command_abe )
				{
					vbgr_temp_transparent = vSemiTransparency16( DestPixel, vbgr_temp, GPU_CTRL_Read.ABR );
					vbgr_select = _mm_srai_epi16 ( vbgr, 15 );
					vbgr_temp = _mm_or_si128 ( _mm_andnot_si128( vbgr_select, vbgr_temp ), _mm_and_si128 ( vbgr_select, vbgr_temp_transparent ) );
				}
				vbgr_temp = _mm_or_si128 ( _mm_or_si128 ( vbgr_temp, SetPixelMask ), _mm_and_si128 ( vbgr, tMask ) );
				_mm_maskmoveu_si128 ( vbgr_temp, _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( DestPixel, 15 ), PixelMask ), _mm_andnot_si128 ( _mm_cmpeq_epi16 ( vbgr, _mm_setzero_si128 () ), _mm_cmplt_epi16 ( vx_across, vEndX ) ) ), (char*) ptr );
				vx_across = _mm_add_epi16 ( vx_across, vVectorSize );
				//viU = _mm_add_epi16 ( viU, vVectorSize );
				viU1 = _mm_add_epi16 ( viU1, vdU_across );
				viU2 = _mm_add_epi16 ( viU2, vdU_across );
				viV1 = _mm_add_epi16 ( viV1, vdV_across );
				viV2 = _mm_add_epi16 ( viV2, vdV_across );
#else
					if ( bgr )
					{
						// shade pixel color
					
						// read pixel from frame buffer if we need to check mask bit
						//DestPixel = VRAM [ cx + ( cy << 10 ) ];
						DestPixel = *ptr;
						
						bgr_temp = bgr;
			
						if ( !command_tge )
						{
							// brightness calculation
							//bgr_temp = Color24To16 ( ColorMultiply24 ( Color16To24 ( bgr_temp ), color_add ) );
							bgr_temp = ColorMultiply1624 ( bgr_temp, color_add );
						}
						
						// semi-transparency
						if ( command_abe && ( bgr & 0x8000 ) )
						{
							bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, tpage_abr );
						}
						
						// check if we should set mask bit when drawing
						//if ( GPU_CTRL_Read.MD ) bgr_temp |= 0x8000;
						bgr_temp |= SetPixelMask | ( bgr & 0x8000 );

						// draw pixel if we can draw to mask pixels or mask bit not set
						//if ( ! ( DestPixel & PixelMask ) ) VRAM [ cx + ( cy << 10 ) ] = bgr_temp;
						if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
					}
					
				/////////////////////////////////////////////////////////
				// interpolate texture coords
				iU += dU_across;
				iV += dV_across;
#endif
				
				//ptr++;
				ptr += c_iVectorSize;
			}
			
		}
		
		/////////////////////////////////////
		// update x on left and right
		x_left += dx_left;
		x_right += dx_right;
		
		U_left += dU_left;
		V_left += dV_left;
		U_right += dU_right;
		V_right += dV_right;
	}

}





void GPU::DrawTriangle_TextureGradient ()
{
#ifdef _ENABLE_SSE2
	// with sse2, can send 8 pixels at a time
	static const int c_iVectorSize = 8;
#else
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;
#endif

	u32 clut_xoffset;

	u32 Pixel, TexelIndex;
	//u32 Y1_OnLeft;
	
	u32 color_add;
	
	u16 *ptr_texture, *ptr_clut;
	u16 *ptr;
	
	s64 Temp, TMin, TMax;
	s64 LeftMostX, RightMostX;
	
	s32 StartX, EndX, StartY, EndY;
	
	s32* DitherArray;
	s32* DitherLine;
	s32 DitherValue;
	

#ifdef _ENABLE_SSE2
	s16* vDitherArray_add;
	s16* vDitherArray_sub;
	s16* vDitherLine_add;
	s16* vDitherLine_sub;
	__m128i viR1, viG1, viB1, viR2, viG2, viB2, vRed, vGreen, vBlue, vdR_across, vdG_across, vdB_across, vDitherValue_add, vDitherValue_sub, vTemp;
	
	__m128i DestPixel, PixelMask, SetPixelMask;
	__m128i vbgr, vbgr_temp, vStartX, vEndX, vx_across, vSeq, vVectorSize;
	__m128i vbgr_temp_transparent, vbgr_select;
	
	__m128i vSeq1_32, vSeq2_32;
	
	__m128i TexCoordX, TexCoordY, Mask, tMask;	//, vTexCoordX2, vTexCoordY1, vTexCoordY2;
	u32 And2 = 0;
	__m128i And1, Shift1, Shift2;
	__m128i vIndex1, vIndex2;
	__m128i viU, viU1, viU2, viV, viV1, viV2;
	__m128i vdV_across, vdU_across;
	
	//u32 TWYTWH, Not_TWH;
	__m128i TWXTWW, Not_TWW, TWYTWH, Not_TWH;
	//__m128i color_add_r, color_add_g, color_add_b;
	__m128i vRound24;
	
	vRound24 = _mm_set1_epi32 ( 0x00800000 );
	
	//color_add_r = _mm_set1_epi16 ( bgr & 0xff );
	//color_add_g = _mm_set1_epi16 ( ( bgr >> 8 ) & 0xff );
	//color_add_b = _mm_set1_epi16 ( ( bgr >> 16 ) & 0xff );
	
	Mask = _mm_set1_epi16 ( 0xff );
	tMask = _mm_set1_epi16 ( 0x8000 );
	
	vSeq1_32 = _mm_set_epi32 ( 3, 2, 1, 0 );
	vSeq2_32 = _mm_set_epi32 ( 7, 6, 5, 4 );
	
	TWYTWH = _mm_set1_epi16 ( ( ( TWY & TWH ) << 3 ) );
	TWXTWW = _mm_set1_epi16 ( ( ( TWX & TWW ) << 3 ) );
	
	Not_TWH = _mm_set1_epi16 ( ~( TWH << 3 ) );
	Not_TWW = _mm_set1_epi16 ( ~( TWW << 3 ) );
	
	And1 = _mm_setzero_si128 ();
	Shift1 = _mm_setzero_si128 ();
	
	//TextureOffset = _mm_set1_epi32 ( ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 ) );
	
	vSeq = _mm_set_epi16 ( 7, 6, 5, 4, 3, 2, 1, 0 );
	vVectorSize = _mm_set1_epi16 ( c_iVectorSize );
	
	//vbgr = _mm_set1_epi16 ( bgr );
	PixelMask = _mm_setzero_si128 ();
	SetPixelMask = _mm_setzero_si128 ();
	if ( GPU_CTRL_Read.ME ) PixelMask = _mm_set1_epi16 ( 0x8080 );
	if ( GPU_CTRL_Read.MD ) SetPixelMask = _mm_set1_epi16 ( 0x8000 );
#else

	u32 DestPixel, PixelMask = 0, SetPixelMask = 0;
	u32 TexCoordX, TexCoordY;
	u32 Shift1 = 0, Shift2 = 0, And1 = 0, And2 = 0;

	s64 Error_Left;
	//s64 TexOffset_X, TexOffset_Y;
	
	
	s16 Red, Green, Blue;
	
	u32 TWYTWH, TWXTWW, Not_TWH, Not_TWW;
	
	TWYTWH = ( ( TWY & TWH ) << 3 );
	TWXTWW = ( ( TWX & TWW ) << 3 );
	
	Not_TWH = ~( TWH << 3 );
	Not_TWW = ~( TWW << 3 );
	
	if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
#endif

	
	// initialize number of pixels drawn
	NumberOfPixelsDrawn = 0;
	
	clut_xoffset = clut_x << 4;
	ptr_clut = & ( VRAM [ clut_y << 10 ] );
	ptr_texture = & ( VRAM [ ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 ) ] );
	
	// check for some important conditions
	if ( DrawArea_BottomRightX < DrawArea_TopLeftX )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightX < DrawArea_TopLeftX.\n";
		return;
	}
	
	if ( DrawArea_BottomRightY < DrawArea_TopLeftY )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightY < DrawArea_TopLeftY.\n";
		return;
	}
	
	///////////////////////////////////////////////////
	// Initialize dithering
	
#ifdef _ENABLE_SSE2
	vDitherArray_add = (s16*) c_iDitherZero;
	vDitherArray_sub = (s16*) c_iDitherZero;
#else
	DitherArray = (s32*) c_iDitherZero;
#endif
	
	if ( GPU_CTRL_Read.DTD )
	{
#ifdef _ENABLE_SSE2
		vDitherArray_add = c_viDitherValues16_add;
		vDitherArray_sub = c_viDitherValues16_sub;
#else
		//DitherArray = c_iDitherValues;
		DitherArray = c_iDitherValues4;
#endif
	}
	
	//////////////////////////////////////////
	// get coordinates on screen
	x0 = DrawArea_OffsetX + x0;
	y0 = DrawArea_OffsetY + y0;
	x1 = DrawArea_OffsetX + x1;
	y1 = DrawArea_OffsetY + y1;
	x2 = DrawArea_OffsetX + x2;
	y2 = DrawArea_OffsetY + y2;
	
	
	////////////////////////////////
	// y1 starts on the left
	//Y1_OnLeft = 1;

	r0 = bgr0 & 0xff;
	r1 = bgr1 & 0xff;
	r2 = bgr2 & 0xff;
	g0 = ( bgr0 >> 8 ) & 0xff;
	g1 = ( bgr1 >> 8 ) & 0xff;
	g2 = ( bgr2 >> 8 ) & 0xff;
	b0 = ( bgr0 >> 16 ) & 0xff;
	b1 = ( bgr1 >> 16 ) & 0xff;
	b2 = ( bgr2 >> 16 ) & 0xff;
	
	//////////////////////////////////////////////////////
	// Get offset into color lookup table
	//u32 ClutOffset = ( clut_x << 4 ) + ( clut_y << 10 );
	
	/////////////////////////////////////////////////////////
	// Get offset into texture page
	//u32 TextureOffset = ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 );
	
	
	//u32 NibblesPerPixel;
	
	//if ( tpage_tp == 0 ) NibblesPerPixel = 1; else if ( tpage_tp == 1 ) NibblesPerPixel = 2; else NibblesPerPixel = 4;

	if ( tpage_tp == 0 )
	{
		And2 = 0xf;
		
#ifdef _ENABLE_SSE2
		Shift1 = _mm_set_epi32 ( 0, 0, 0, 2 );
		Shift2 = _mm_set_epi32 ( 0, 0, 0, 2 );
		And1 = _mm_set1_epi16 ( 3 );
#else
		Shift1 = 2; Shift2 = 2;
		And1 = 3; And2 = 0xf;
#endif
	}
	else if ( tpage_tp == 1 )
	{
		And2 = 0xff;
		
#ifdef _ENABLE_SSE2
		Shift1 = _mm_set_epi32 ( 0, 0, 0, 1 );
		Shift2 = _mm_set_epi32 ( 0, 0, 0, 3 );
		And1 = _mm_set1_epi16 ( 1 );
#else
		Shift1 = 1; Shift2 = 3;
		And1 = 1; And2 = 0xff;
#endif
	}
	
	
	///////////////////////////////////
	// put top coordinates in x0,y0
	if ( y1 < y0 )
	{
		Swap ( x0, x1 );
		Swap ( y0, y1 );
		
		Swap ( u0, u1 );
		Swap ( v0, v1 );
		
		Swap ( r0, r1 );
		Swap ( g0, g1 );
		Swap ( b0, b1 );
	}
	
	if ( y2 < y0 )
	{
		Swap ( x0, x2 );
		Swap ( y0, y2 );
		
		Swap ( u0, u2 );
		Swap ( v0, v2 );
		
		Swap ( r0, r2 );
		Swap ( g0, g2 );
		Swap ( b0, b2 );
	}
	
	///////////////////////////////////////
	// put middle coordinates in x1,y1
	if ( y2 < y1 )
	{
		Swap ( x1, x2 );
		Swap ( y1, y2 );
		
		Swap ( u1, u2 );
		Swap ( v1, v2 );
		
		Swap ( r1, r2 );
		Swap ( g1, g2 );
		Swap ( b1, b2 );
	}
	
	// get the left/right most x
	LeftMostX = ( ( x0 < x1 ) ? x0 : x1 );
	LeftMostX = ( ( x2 < LeftMostX ) ? x2 : LeftMostX );
	RightMostX = ( ( x0 > x1 ) ? x0 : x1 );
	RightMostX = ( ( x2 > RightMostX ) ? x2 : RightMostX );

	// check if sprite is within draw area
	if ( RightMostX <= ((s32)DrawArea_TopLeftX) || LeftMostX > ((s32)DrawArea_BottomRightX) || y2 <= ((s32)DrawArea_TopLeftY) || y0 > ((s32)DrawArea_BottomRightY) ) return;
	
	// skip drawing if distance between vertices is greater than max allowed by GPU
	if ( ( _Abs( x1 - x0 ) > c_MaxPolygonWidth ) || ( _Abs( x2 - x1 ) > c_MaxPolygonWidth ) || ( y1 - y0 > c_MaxPolygonHeight ) || ( y2 - y1 > c_MaxPolygonHeight ) )
	{
		// skip drawing polygon
		return;
	}
	
	
	
	/////////////////////////////////////////////////
	// draw top part of triangle
	

	///////////////////////////////////////////
	// start at y0
	//Line = y0;
	
	/////////////////////////////////////////////
	// init x on the left and right
	x_left = ( ((s64)x0) << 32 );
	x_right = x_left;
	U_left = ( ((s64)u0) << 24 );
	V_left = ( ((s64)v0) << 24 );
	U_right = U_left;
	V_right = V_left;
		
	R_left = ( ((s64)r0) << 24 );
	G_left = ( ((s64)g0) << 24 );
	B_left = ( ((s64)b0) << 24 );
	R_right = R_left;
	G_right = G_left;
	B_right = B_left;

	////////////////////////////////////
	// get slopes
	
	if ( y1 - y0 )
	{
		dx_left = (((s64)( x1 - x0 )) << 32 ) / ((s64)( y1 - y0 ));
		dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
		
		dU_left = (((s64)( u1 - u0 )) << 24 ) / ((s64)( y1 - y0 ));
		dV_left = (((s64)( v1 - v0 )) << 24 ) / ((s64)( y1 - y0 ));
		
		dU_right = (((s64)( u2 - u0 )) << 24 ) / ((s64)( y2 - y0 ));
		dV_right = (((s64)( v2 - v0 )) << 24 ) / ((s64)( y2 - y0 ));
		
		dR_left = (((s64)( r1 - r0 )) << 24 ) / ((s64)( y1 - y0 ));
		dG_left = (((s64)( g1 - g0 )) << 24 ) / ((s64)( y1 - y0 ));
		dB_left = (((s64)( b1 - b0 )) << 24 ) / ((s64)( y1 - y0 ));
		
		dR_right = (((s64)( r2 - r0 )) << 24 ) / ((s64)( y2 - y0 ));
		dG_right = (((s64)( g2 - g0 )) << 24 ) / ((s64)( y2 - y0 ));
		dB_right = (((s64)( b2 - b0 )) << 24 ) / ((s64)( y2 - y0 ));
	}
	else
	{
		// change x_left and x_right where y1 is on left
		x_left = ( ((s64)x1) << 32 );
		x_right = ( ((s64)x0) << 32 );
		
		// change U_left and V_right where y1 is on left
		U_left = ( ((s64)u1) << 24 );
		U_right = ( ((s64)u0) << 24 );
		V_left = ( ((s64)v1) << 24 );
		V_right = ( ((s64)v0) << 24 );
		
		R_left = ( ((s64)r1) << 24 );
		R_right = ( ((s64)r0) << 24 );
		G_left = ( ((s64)g1) << 24 );
		G_right = ( ((s64)g0) << 24 );
		B_left = ( ((s64)b1) << 24 );
		B_right = ( ((s64)b0) << 24 );
		
		// this means that x0 and x1 are on the same line
		// so if the height of the entire polygon is zero, then we are done
		if ( y2 - y1 )
		{
			dx_left = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
			
			dU_left = (((s64)( u2 - u1 )) << 24 ) / ((s64)( y2 - y1 ));
			dV_left = (((s64)( v2 - v1 )) << 24 ) / ((s64)( y2 - y1 ));
			
			dU_right = (((s64)( u2 - u0 )) << 24 ) / ((s64)( y2 - y0 ));
			dV_right = (((s64)( v2 - v0 )) << 24 ) / ((s64)( y2 - y0 ));
			
			dR_left = (((s64)( r2 - r1 )) << 24 ) / ((s64)( y2 - y1 ));
			dG_left = (((s64)( g2 - g1 )) << 24 ) / ((s64)( y2 - y1 ));
			dB_left = (((s64)( b2 - b1 )) << 24 ) / ((s64)( y2 - y1 ));
			
			dR_right = (((s64)( r2 - r0 )) << 24 ) / ((s64)( y2 - y0 ));
			dG_right = (((s64)( g2 - g0 )) << 24 ) / ((s64)( y2 - y0 ));
			dB_right = (((s64)( b2 - b0 )) << 24 ) / ((s64)( y2 - y0 ));
		}
	}

	// calculate across
	s64 t0 = y1 - y2;
	s64 t1 = y0 - y2;
	s64 denominator = ( ( x0 - x2 ) * t0 ) - ( ( x1 - x2 ) * t1 );
	if ( denominator )
	{
		dR_across = ( ( (s64) ( ( ( r0 - r2 ) * t0 ) - ( ( r1 - r2 ) * t1 ) ) ) << 24 ) / denominator;
		dG_across = ( ( (s64) ( ( ( g0 - g2 ) * t0 ) - ( ( g1 - g2 ) * t1 ) ) ) << 24 ) / denominator;
		dB_across = ( ( (s64) ( ( ( b0 - b2 ) * t0 ) - ( ( b1 - b2 ) * t1 ) ) ) << 24 ) / denominator;
		dU_across = ( ( (s64) ( ( ( u0 - u2 ) * t0 ) - ( ( u1 - u2 ) * t1 ) ) ) << 24 ) / denominator;
		dV_across = ( ( (s64) ( ( ( v0 - v2 ) * t0 ) - ( ( v1 - v2 ) * t1 ) ) ) << 24 ) / denominator;
	}
	
	
	/////////////////////////////////////////////////
	// swap left and right if they are reversed
	//if ( ( ( x_right + dx_right ) < ( x_left + dx_left ) ) || ( x_right < x_left ) )
	if ( denominator > 0 )
	{
		// x1,y1 is on the right //
		
		//Y1_OnLeft = 0;
		
		Swap ( x_left, x_right );
		Swap ( dx_left, dx_right );

		Swap ( dU_left, dU_right );
		Swap ( dV_left, dV_right );

		Swap ( U_left, U_right );
		Swap ( V_left, V_right );
		
		Swap ( dR_left, dR_right );
		Swap ( dG_left, dG_right );
		Swap ( dB_left, dB_right );

		Swap ( R_left, R_right );
		Swap ( G_left, G_right );
		Swap ( B_left, B_right );
	}


	
	
	StartY = y0;
	EndY = y1;

	if ( StartY < ((s32)DrawArea_TopLeftY) )
	{
		
		if ( EndY < ((s32)DrawArea_TopLeftY) )
		{
			Temp = EndY - StartY;
			StartY = EndY;
		}
		else
		{
			Temp = DrawArea_TopLeftY - StartY;
			StartY = DrawArea_TopLeftY;
		}
		
		x_left += dx_left * Temp;
		x_right += dx_right * Temp;
		
		U_left += dU_left * Temp;
		V_left += dV_left * Temp;
		R_left += dR_left * Temp;
		G_left += dG_left * Temp;
		B_left += dB_left * Temp;
		
		U_right += dU_right * Temp;
		V_right += dV_right * Temp;
		R_right += dR_right * Temp;
		G_right += dG_right * Temp;
		B_right += dB_right * Temp;
	}
	
	if ( EndY > ((s32)DrawArea_BottomRightY) )
	{
		EndY = DrawArea_BottomRightY + 1;
	}

	
	//////////////////////////////////////////////
	// draw down to y1
	for ( Line = StartY; Line < EndY; Line++ )
	{
		//cy = Line;

		StartX = _Round( x_left ) >> 32;
		EndX = _Round( x_right ) >> 32;
		
		//if ( Line >= ((s32)DrawArea_TopLeftY) && Line <= ((s32)DrawArea_BottomRightY) )
		//{
		if ( StartX <= ((s32)DrawArea_BottomRightX) && EndX > ((s32)DrawArea_TopLeftX) )
		{
			/*
			dU_across = 0;
			dV_across = 0;
				
			dR_across = 0;
			dG_across = 0;
			dB_across = 0;
			
			if ( x_left != x_right )
			{
				//dU_across = ( ( ( U_right - U_left ) << 16 ) / ( x_right - x_left ) ) << 16;
				//dV_across = ( ( ( V_right - V_left ) << 16 ) / ( x_right - x_left ) ) << 16;
				//dR_across = ( ( ( R_right - R_left ) << 16 ) / ( x_right - x_left ) ) << 16;
				//dG_across = ( ( ( G_right - G_left ) << 16 ) / ( x_right - x_left ) ) << 16;
				//dB_across = ( ( ( B_right - B_left ) << 16 ) / ( x_right - x_left ) ) << 16;
				
				dU_across = ( ( ( U_right - U_left ) << 31 ) / ( x_right - x_left ) ) << 1;
				dV_across = ( ( ( V_right - V_left ) << 31 ) / ( x_right - x_left ) ) << 1;
				
				dR_across = ( ( ( R_right - R_left ) << 31 ) / ( x_right - x_left ) ) << 1;
				dG_across = ( ( ( G_right - G_left ) << 31 ) / ( x_right - x_left ) ) << 1;
				dB_across = ( ( ( B_right - B_left ) << 31 ) / ( x_right - x_left ) ) << 1;
				
#ifdef _ENABLE_SSE2
				vdU_across = _mm_set1_epi32( ( dU_across ) * c_iVectorSize );
				vdV_across = _mm_set1_epi32( ( dV_across ) * c_iVectorSize );
				
				vdR_across = _mm_set1_epi32( ( dR_across ) * c_iVectorSize );
				vdG_across = _mm_set1_epi32( ( dG_across ) * c_iVectorSize );
				vdB_across = _mm_set1_epi32( ( dB_across ) * c_iVectorSize );
#endif
			}
			*/
			
			iU = U_left;
			iV = V_left;
			
			
			if ( EndX - StartX > 1 )
			{
				// if the right edge is straighter, then recalculate iu,iv
				if ( _Abs ( dU_right ) < _Abs ( dU_left ) )
				{
					iU = U_right - ( dU_across * ( EndX - StartX ) );
				}
				
				if ( _Abs ( dV_right ) < _Abs ( dV_left ) )
				{
					iV = V_right - ( dV_across * ( EndX - StartX ) );
				}
			}
			
			
			iR = R_left;
			iG = G_left;
			iB = B_left;
			
			if ( StartX < ((s32)DrawArea_TopLeftX) )
			{
				Temp = DrawArea_TopLeftX - StartX;
				StartX = DrawArea_TopLeftX;
				
				iU += dU_across * Temp;
				iV += dV_across * Temp;
				
				iR += dR_across * Temp;
				iG += dG_across * Temp;
				iB += dB_across * Temp;
			}
			
			if ( EndX > ((s32)DrawArea_BottomRightX) )
			{
				EndX = DrawArea_BottomRightX + 1;
				//EndX = DrawArea_BottomRightX;
			}
			
			ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			DitherLine = & ( DitherArray [ ( Line & 0x3 ) << 2 ] );

			
			if ( StartX == EndX && !dx_left ) EndX += 1;
			//if ( EndX >= RightMostX ) EndX--;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;

#ifdef _ENABLE_SSE2
	vx_across = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX );
	
	viU1 = _mm_add_epi32 ( _mm_set1_epi32 ( iU ), _custom_mul_32 ( _mm_set1_epi32 ( dU_across ), vSeq1_32 ) );
	viU2 = _mm_add_epi32 ( _mm_set1_epi32 ( iU ), _custom_mul_32 ( _mm_set1_epi32 ( dU_across ), vSeq2_32 ) );
	viV1 = _mm_add_epi32 ( _mm_set1_epi32 ( iV ), _custom_mul_32 ( _mm_set1_epi32 ( dV_across ), vSeq1_32 ) );
	viV2 = _mm_add_epi32 ( _mm_set1_epi32 ( iV ), _custom_mul_32 ( _mm_set1_epi32 ( dV_across ), vSeq2_32 ) );
	
	viR1 = _mm_add_epi32 ( _mm_set1_epi32 ( iR ), _custom_mul_32 ( _mm_set1_epi32 ( dR_across ), vSeq1_32 ) );
	viR2 = _mm_add_epi32 ( _mm_set1_epi32 ( iR ), _custom_mul_32 ( _mm_set1_epi32 ( dR_across ), vSeq2_32 ) );
	viG1 = _mm_add_epi32 ( _mm_set1_epi32 ( iG ), _custom_mul_32 ( _mm_set1_epi32 ( dG_across ), vSeq1_32 ) );
	viG2 = _mm_add_epi32 ( _mm_set1_epi32 ( iG ), _custom_mul_32 ( _mm_set1_epi32 ( dG_across ), vSeq2_32 ) );
	viB1 = _mm_add_epi32 ( _mm_set1_epi32 ( iB ), _custom_mul_32 ( _mm_set1_epi32 ( dB_across ), vSeq1_32 ) );
	viB2 = _mm_add_epi32 ( _mm_set1_epi32 ( iB ), _custom_mul_32 ( _mm_set1_epi32 ( dB_across ), vSeq2_32 ) );
	
	vDitherValue_add = _mm_loadu_si128 ((__m128i const*) ( &(vDitherArray_add [ ( StartX & 0x3 ) + ( ( Line & 0x3 ) << 4 ) ]) ));
	vDitherValue_sub = _mm_loadu_si128 ((__m128i const*) ( &(vDitherArray_sub [ ( StartX & 0x3 ) + ( ( Line & 0x3 ) << 4 ) ]) ));
#endif

			// draw horizontal line
			// x_left and x_right need to be rounded off
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
#ifdef _ENABLE_SSE2
				viV =  _mm_packs_epi32 ( _mm_srli_epi32 ( _mm_add_epi32 ( viV1, vRound24 ), 24 ), _mm_srli_epi32 ( _mm_add_epi32 ( viV2, vRound24 ), 24 ) );
				viU =  _mm_packs_epi32 ( _mm_srli_epi32 ( _mm_add_epi32 ( viU1, vRound24 ), 24 ), _mm_srli_epi32 ( _mm_add_epi32 ( viU2, vRound24 ), 24 ) );
				
				TexCoordY = _mm_and_si128 ( _mm_or_si128 ( _mm_and_si128 ( viV, Not_TWH ), TWYTWH ), Mask );
				TexCoordX = _mm_and_si128 ( _mm_or_si128 ( _mm_and_si128 ( viU, Not_TWW ), TWXTWW ), Mask );
				vIndex1 = _mm_srl_epi16 ( TexCoordX, Shift1 );
				if ( And2 )
				{
					vIndex2 = _mm_sll_epi16 ( _mm_and_si128 ( TexCoordX, And1 ), Shift2 );
					
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 0 ) + ( _mm_extract_epi16 ( TexCoordY, 0 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 0 ) ) & And2 ) ) & FrameBuffer_XMask ], 0 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 1 ) + ( _mm_extract_epi16 ( TexCoordY, 1 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 1 ) ) & And2 ) ) & FrameBuffer_XMask ], 1 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 2 ) + ( _mm_extract_epi16 ( TexCoordY, 2 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 2 ) ) & And2 ) ) & FrameBuffer_XMask ], 2 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 3 ) + ( _mm_extract_epi16 ( TexCoordY, 3 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 3 ) ) & And2 ) ) & FrameBuffer_XMask ], 3 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 4 ) + ( _mm_extract_epi16 ( TexCoordY, 4 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 4 ) ) & And2 ) ) & FrameBuffer_XMask ], 4 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 5 ) + ( _mm_extract_epi16 ( TexCoordY, 5 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 5 ) ) & And2 ) ) & FrameBuffer_XMask ], 5 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 6 ) + ( _mm_extract_epi16 ( TexCoordY, 6 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 6 ) ) & And2 ) ) & FrameBuffer_XMask ], 6 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 7 ) + ( _mm_extract_epi16 ( TexCoordY, 7 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 7 ) ) & And2 ) ) & FrameBuffer_XMask ], 7 );
				}
				else
				{
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 0 ) + ( _mm_extract_epi16 ( TexCoordY, 0 ) << 10 ) ], 0 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 1 ) + ( _mm_extract_epi16 ( TexCoordY, 1 ) << 10 ) ], 1 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 2 ) + ( _mm_extract_epi16 ( TexCoordY, 2 ) << 10 ) ], 2 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 3 ) + ( _mm_extract_epi16 ( TexCoordY, 3 ) << 10 ) ], 3 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 4 ) + ( _mm_extract_epi16 ( TexCoordY, 4 ) << 10 ) ], 4 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 5 ) + ( _mm_extract_epi16 ( TexCoordY, 5 ) << 10 ) ], 5 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 6 ) + ( _mm_extract_epi16 ( TexCoordY, 6 ) << 10 ) ], 6 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 7 ) + ( _mm_extract_epi16 ( TexCoordY, 7 ) << 10 ) ], 7 );
				}
#else
				
				//iX = x_across;
				//cx = iX;
			
				// make sure we are putting pixel within draw area
				//if ( x_across >= ((s32)DrawArea_TopLeftX) && x_across <= ((s32)DrawArea_BottomRightX) )
				//{
					//color_add = ( _Round( iR ) >> 35 ) | ( ( _Round( iG ) >> 35 ) << 5 ) | ( ( _Round( iB ) >> 35 ) << 10 );
					//color_add = ( _Round( iR ) >> 32 ) | ( ( _Round( iG ) >> 32 ) << 8 ) | ( ( _Round( iB ) >> 32 ) << 16 );
					//DitherValue = DitherLine [ x_across & 0x3 ] << 4;
					DitherValue = DitherLine [ x_across & 0x3 ];

					//TexCoordY = (u8) ( ( ( _Round ( iV ) >> 32 ) & ~( TWH << 3 ) ) | ( ( TWY & TWH ) << 3 ) );
					//TexCoordX = (u8) ( ( ( _Round ( iU ) >> 32 ) & ~( TWW << 3 ) ) | ( ( TWX & TWW ) << 3 ) );
					TexCoordY = (u8) ( ( ( _Round24 ( iV ) >> 24 ) & Not_TWH ) | TWYTWH );
					TexCoordX = (u8) ( ( ( _Round24 ( iU ) >> 24 ) & Not_TWW ) | TWXTWW );
					
					//bgr = VRAM [ TextureOffset + ( TexCoordX >> Shift1 ) + ( TexCoordY << 10 ) ];
					bgr = ptr_texture [ ( TexCoordX >> Shift1 ) + ( TexCoordY << 10 ) ];
					
					if ( Shift1 )
					{
						TexelIndex = ( bgr >> ( ( TexCoordX & And1 ) << Shift2 ) ) & And2;
						//bgr = VRAM [ ( ( ( clut_x << 4 ) + TexelIndex ) & FrameBuffer_XMask ) + ( clut_y << 10 ) ];
						bgr = ptr_clut [ ( clut_xoffset + TexelIndex ) & FrameBuffer_XMask ];
					}
#endif

					
#ifdef _ENABLE_SSE2
					DestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
					vbgr_temp = vbgr;
					if ( !command_tge )
					{
						vRed = _mm_packs_epi32 ( _mm_srai_epi32 ( viR1, 16 ), _mm_srai_epi32 ( viR2, 16 ) );
						vRed = _mm_adds_epu16 ( vRed, vDitherValue_add );
						vRed = _mm_subs_epu16 ( vRed, vDitherValue_sub );
						
						vGreen = _mm_packs_epi32 ( _mm_srai_epi32 ( viG1, 16 ), _mm_srai_epi32 ( viG2, 16 ) );
						vGreen = _mm_adds_epu16 ( vGreen, vDitherValue_add );
						vGreen = _mm_subs_epu16 ( vGreen, vDitherValue_sub );
						
						vBlue = _mm_packs_epi32 ( _mm_srai_epi32 ( viB1, 16 ), _mm_srai_epi32 ( viB2, 16 ) );
						vBlue = _mm_adds_epu16 ( vBlue, vDitherValue_add );
						vBlue = _mm_subs_epu16 ( vBlue, vDitherValue_sub );
						
						vRed = _mm_srli_epi16 ( vRed, 8 );
						vGreen = _mm_srli_epi16 ( vGreen, 8 );
						vBlue = _mm_srli_epi16 ( vBlue, 8 );
						vbgr_temp = vColorMultiply1624 ( vbgr_temp, vRed, vGreen, vBlue );
					}
					if ( command_abe )
					{
						vbgr_temp_transparent = vSemiTransparency16( DestPixel, vbgr_temp, tpage_abr );
						vbgr_select = _mm_srai_epi16 ( vbgr, 15 );
						vbgr_temp = _mm_or_si128 ( _mm_andnot_si128( vbgr_select, vbgr_temp ), _mm_and_si128 ( vbgr_select, vbgr_temp_transparent ) );
					}
					vbgr_temp = _mm_or_si128 ( _mm_or_si128 ( vbgr_temp, SetPixelMask ), _mm_and_si128 ( vbgr, tMask ) );
					_mm_maskmoveu_si128 ( vbgr_temp, _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( DestPixel, 15 ), PixelMask ), _mm_andnot_si128 ( _mm_cmpeq_epi16 ( vbgr, _mm_setzero_si128 () ), _mm_cmplt_epi16 ( vx_across, vEndX ) ) ), (char*) ptr );
					
					vx_across = _mm_add_epi16 ( vx_across, vVectorSize );

					viU1 = _mm_add_epi32 ( viU1, vdU_across );
					viU2 = _mm_add_epi32 ( viU2, vdU_across );
					viV1 = _mm_add_epi32 ( viV1, vdV_across );
					viV2 = _mm_add_epi32 ( viV2, vdV_across );
					
					viR1 = _mm_add_epi32 ( viR1, vdR_across );
					viR2 = _mm_add_epi32 ( viR2, vdR_across );
					viG1 = _mm_add_epi32 ( viG1, vdG_across );
					viG2 = _mm_add_epi32 ( viG2, vdG_across );
					viB1 = _mm_add_epi32 ( viB1, vdB_across );
					viB2 = _mm_add_epi32 ( viB2, vdB_across );
#else
					if ( bgr )
					{
						// shade pixel color
					
						// read pixel from frame buffer if we need to check mask bit
						//DestPixel = VRAM [ cx + ( cy << 10 ) ];
						DestPixel = *ptr;
						
						bgr_temp = bgr;
			
						if ( !command_tge )
						{
							// shade pixel
							Red = ( ( (s16) ( iR >> 24 ) ) * ( (s16) ( bgr_temp & 0x1f ) ) );
							Green = ( ( (s16) ( iG >> 24 ) ) * ( (s16) ( ( bgr_temp >> 5 ) & 0x1f ) ) );
							Blue = ( ( (s16) ( iB >> 24 ) ) * ( (s16) ( ( bgr_temp >> 10 ) & 0x1f ) ) );
						
							// apply dithering if it is enabled
							// dithering must be applied after the color multiply
							Red = Red + DitherValue;
							Green = Green + DitherValue;
							Blue = Blue + DitherValue;
							
							// clamp
							//Red = Clamp5 ( Red >> 7 );
							//Green = Clamp5 ( Green >> 7 );
							//Blue = Clamp5 ( Blue >> 7 );
							Red = SignedClamp<s16,5> ( Red >> 7 );
							Green = SignedClamp<s16,5> ( Green >> 7 );
							Blue = SignedClamp<s16,5> ( Blue >> 7 );
							
							// combine
							bgr_temp = ( Blue << 10 ) | ( Green << 5 ) | ( Red );
						}
						
						
						// semi-transparency
						if ( command_abe && ( bgr & 0x8000 ) )
						{
							bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, tpage_abr );
						}
						
						// check if we should set mask bit when drawing
						//if ( GPU_CTRL_Read.MD ) bgr_temp |= 0x8000;
						bgr_temp |= SetPixelMask | ( bgr & 0x8000 );

						// draw pixel if we can draw to mask pixels or mask bit not set
						//if ( ! ( DestPixel & PixelMask ) ) VRAM [ cx + ( cy << 10 ) ] = bgr_temp;
						if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
					}
					
					/////////////////////////////////////////////////////
					// update number of cycles used to draw polygon
					//NumberOfPixelsDrawn++;
				//}
				
				/////////////////////////////////////////////////////////
				// interpolate texture coords
				iU += dU_across;
				iV += dV_across;
				
				iR += dR_across;
				iG += dG_across;
				iB += dB_across;
#endif
				
				ptr += c_iVectorSize;
			}
			
		}
		
		//////////////////////////////////
		// draw next line
		//Line++;
		
		/////////////////////////////////////
		// update x on left and right
		x_left += dx_left;
		x_right += dx_right;
		
		U_left += dU_left;
		V_left += dV_left;
		U_right += dU_right;
		V_right += dV_right;
		
		R_left += dR_left;
		G_left += dG_left;
		B_left += dB_left;
		R_right += dR_right;
		G_right += dG_right;
		B_right += dB_right;
	}

	
	////////////////////////////////////////////////
	// draw bottom part of triangle

	/////////////////////////////////////////////
	// init x on the left and right
	
	//////////////////////////////////////////////////////
	// check if y1 is on the left or on the right
	//if ( Y1_OnLeft )
	if ( denominator < 0 )
	{
		x_left = ( ((s64)x1) << 32 );

		U_left = ( ((s64)u1) << 24 );
		V_left = ( ((s64)v1) << 24 );

		R_left = ( ((s64)r1) << 24 );
		G_left = ( ((s64)g1) << 24 );
		B_left = ( ((s64)b1) << 24 );
		
		if ( y2 - y1 )
		{
			dx_left = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			
			dU_left = (((s64)( u2 - u1 )) << 24 ) / ((s64)( y2 - y1 ));
			dV_left = (((s64)( v2 - v1 )) << 24 ) / ((s64)( y2 - y1 ));
			
			dR_left = (((s64)( r2 - r1 )) << 24 ) / ((s64)( y2 - y1 ));
			dG_left = (((s64)( g2 - g1 )) << 24 ) / ((s64)( y2 - y1 ));
			dB_left = (((s64)( b2 - b1 )) << 24 ) / ((s64)( y2 - y1 ));
		}
	}
	else
	{
		x_right = ( ((s64)x1) << 32 );

		U_right = ( ((s64)u1) << 24 );
		V_right = ( ((s64)v1) << 24 );

		R_right = ( ((s64)r1) << 24 );
		G_right = ( ((s64)g1) << 24 );
		B_right = ( ((s64)b1) << 24 );
		
		if ( y2 - y1 )
		{
			dx_right = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			
			dU_right = (((s64)( u2 - u1 )) << 24 ) / ((s64)( y2 - y1 ));
			dV_right = (((s64)( v2 - v1 )) << 24 ) / ((s64)( y2 - y1 ));
			
			dR_right = (((s64)( r2 - r1 )) << 24 ) / ((s64)( y2 - y1 ));
			dG_right = (((s64)( g2 - g1 )) << 24 ) / ((s64)( y2 - y1 ));
			dB_right = (((s64)( b2 - b1 )) << 24 ) / ((s64)( y2 - y1 ));
		}
	}
	
	// the line starts at y1 from here
	//Line = y1;

	StartY = y1;
	EndY = y2;

	if ( StartY < ((s32)DrawArea_TopLeftY) )
	{
		
		if ( EndY < ((s32)DrawArea_TopLeftY) )
		{
			Temp = EndY - StartY;
			StartY = EndY;
		}
		else
		{
			Temp = DrawArea_TopLeftY - StartY;
			StartY = DrawArea_TopLeftY;
		}
		
		x_left += dx_left * Temp;
		x_right += dx_right * Temp;
		
		U_left += dU_left * Temp;
		V_left += dV_left * Temp;
		R_left += dR_left * Temp;
		G_left += dG_left * Temp;
		B_left += dB_left * Temp;
		
		U_right += dU_right * Temp;
		V_right += dV_right * Temp;
		R_right += dR_right * Temp;
		G_right += dG_right * Temp;
		B_right += dB_right * Temp;
	}
	
	if ( EndY > ((s32)DrawArea_BottomRightY) )
	{
		EndY = DrawArea_BottomRightY + 1;
	}

	
	//////////////////////////////////////////////
	// draw down to y2
	for ( Line = StartY; Line < EndY; Line++ )
	{
		//cy = Line;

		StartX = _Round( x_left ) >> 32;
		EndX = _Round( x_right ) >> 32;
		
		//if ( Line >= ((s32)DrawArea_TopLeftY) && Line <= ((s32)DrawArea_BottomRightY) )
		//{
		if ( StartX <= ((s32)DrawArea_BottomRightX) && EndX > ((s32)DrawArea_TopLeftX) )
		{
			/*
			dU_across = 0;
			dV_across = 0;
				
			dR_across = 0;
			dG_across = 0;
			dB_across = 0;
			
			if ( x_left != x_right )
			{
				dU_across = ( ( ( U_right - U_left ) << 31 ) / ( x_right - x_left ) ) << 1;
				dV_across = ( ( ( V_right - V_left ) << 31 ) / ( x_right - x_left ) ) << 1;
				
				dR_across = ( ( ( R_right - R_left ) << 31 ) / ( x_right - x_left ) ) << 1;
				dG_across = ( ( ( G_right - G_left ) << 31 ) / ( x_right - x_left ) ) << 1;
				dB_across = ( ( ( B_right - B_left ) << 31 ) / ( x_right - x_left ) ) << 1;
				
#ifdef _ENABLE_SSE2
				vdU_across = _mm_set1_epi32( ( dU_across ) * c_iVectorSize );
				vdV_across = _mm_set1_epi32( ( dV_across ) * c_iVectorSize );
				
				vdR_across = _mm_set1_epi32( ( dR_across ) * c_iVectorSize );
				vdG_across = _mm_set1_epi32( ( dG_across ) * c_iVectorSize );
				vdB_across = _mm_set1_epi32( ( dB_across ) * c_iVectorSize );
#endif
			}
			*/
			
			iU = U_left;
			iV = V_left;
			


			if ( EndX - StartX > 1 )
			{
				// if the right edge is straighter, then recalculate iu,iv
				if ( _Abs ( dU_right ) < _Abs ( dU_left ) )
				{
					iU = U_right - ( dU_across * ( EndX - StartX ) );
				}
				
				if ( _Abs ( dV_right ) < _Abs ( dV_left ) )
				{
					iV = V_right - ( dV_across * ( EndX - StartX ) );
				}
			}

			
			iR = R_left;
			iG = G_left;
			iB = B_left;
			
			if ( StartX < ((s32)DrawArea_TopLeftX) )
			{
				Temp = DrawArea_TopLeftX - StartX;
				StartX = DrawArea_TopLeftX;
				
				iU += dU_across * Temp;
				iV += dV_across * Temp;
				
				iR += dR_across * Temp;
				iG += dG_across * Temp;
				iB += dB_across * Temp;
			}
			
			if ( EndX > ((s32)DrawArea_BottomRightX) )
			{
				EndX = DrawArea_BottomRightX + 1;
				//EndX = DrawArea_BottomRightX;
			}
			
			ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			DitherLine = & ( DitherArray [ ( Line & 0x3 ) << 2 ] );
			
			// ??
			if ( StartX == EndX && !dx_left ) EndX += 1;
			//if ( EndX >= RightMostX ) EndX--;
			
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;
			
#ifdef _ENABLE_SSE2
	vx_across = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX );
	
	viU1 = _mm_add_epi32 ( _mm_set1_epi32 ( iU ), _custom_mul_32 ( _mm_set1_epi32 ( dU_across ), vSeq1_32 ) );
	viU2 = _mm_add_epi32 ( _mm_set1_epi32 ( iU ), _custom_mul_32 ( _mm_set1_epi32 ( dU_across ), vSeq2_32 ) );
	viV1 = _mm_add_epi32 ( _mm_set1_epi32 ( iV ), _custom_mul_32 ( _mm_set1_epi32 ( dV_across ), vSeq1_32 ) );
	viV2 = _mm_add_epi32 ( _mm_set1_epi32 ( iV ), _custom_mul_32 ( _mm_set1_epi32 ( dV_across ), vSeq2_32 ) );
	
	viR1 = _mm_add_epi32 ( _mm_set1_epi32 ( iR ), _custom_mul_32 ( _mm_set1_epi32 ( dR_across ), vSeq1_32 ) );
	viR2 = _mm_add_epi32 ( _mm_set1_epi32 ( iR ), _custom_mul_32 ( _mm_set1_epi32 ( dR_across ), vSeq2_32 ) );
	viG1 = _mm_add_epi32 ( _mm_set1_epi32 ( iG ), _custom_mul_32 ( _mm_set1_epi32 ( dG_across ), vSeq1_32 ) );
	viG2 = _mm_add_epi32 ( _mm_set1_epi32 ( iG ), _custom_mul_32 ( _mm_set1_epi32 ( dG_across ), vSeq2_32 ) );
	viB1 = _mm_add_epi32 ( _mm_set1_epi32 ( iB ), _custom_mul_32 ( _mm_set1_epi32 ( dB_across ), vSeq1_32 ) );
	viB2 = _mm_add_epi32 ( _mm_set1_epi32 ( iB ), _custom_mul_32 ( _mm_set1_epi32 ( dB_across ), vSeq2_32 ) );
	
	vDitherValue_add = _mm_loadu_si128 ((__m128i const*) ( &(vDitherArray_add [ ( StartX & 0x3 ) + ( ( Line & 0x3 ) << 4 ) ]) ));
	vDitherValue_sub = _mm_loadu_si128 ((__m128i const*) ( &(vDitherArray_sub [ ( StartX & 0x3 ) + ( ( Line & 0x3 ) << 4 ) ]) ));
#endif
			
			// draw horizontal line
			// x_left and x_right need to be rounded off
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
#ifdef _ENABLE_SSE2
				viV =  _mm_packs_epi32 ( _mm_srli_epi32 ( _mm_add_epi32 ( viV1, vRound24 ), 24 ), _mm_srli_epi32 ( _mm_add_epi32 ( viV2, vRound24 ), 24 ) );
				viU =  _mm_packs_epi32 ( _mm_srli_epi32 ( _mm_add_epi32 ( viU1, vRound24 ), 24 ), _mm_srli_epi32 ( _mm_add_epi32 ( viU2, vRound24 ), 24 ) );
				
				TexCoordY = _mm_and_si128 ( _mm_or_si128 ( _mm_and_si128 ( viV, Not_TWH ), TWYTWH ), Mask );
				TexCoordX = _mm_and_si128 ( _mm_or_si128 ( _mm_and_si128 ( viU, Not_TWW ), TWXTWW ), Mask );
				vIndex1 = _mm_srl_epi16 ( TexCoordX, Shift1 );
				if ( And2 )
				{
					vIndex2 = _mm_sll_epi16 ( _mm_and_si128 ( TexCoordX, And1 ), Shift2 );
					
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 0 ) + ( _mm_extract_epi16 ( TexCoordY, 0 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 0 ) ) & And2 ) ) & FrameBuffer_XMask ], 0 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 1 ) + ( _mm_extract_epi16 ( TexCoordY, 1 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 1 ) ) & And2 ) ) & FrameBuffer_XMask ], 1 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 2 ) + ( _mm_extract_epi16 ( TexCoordY, 2 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 2 ) ) & And2 ) ) & FrameBuffer_XMask ], 2 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 3 ) + ( _mm_extract_epi16 ( TexCoordY, 3 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 3 ) ) & And2 ) ) & FrameBuffer_XMask ], 3 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 4 ) + ( _mm_extract_epi16 ( TexCoordY, 4 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 4 ) ) & And2 ) ) & FrameBuffer_XMask ], 4 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 5 ) + ( _mm_extract_epi16 ( TexCoordY, 5 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 5 ) ) & And2 ) ) & FrameBuffer_XMask ], 5 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 6 ) + ( _mm_extract_epi16 ( TexCoordY, 6 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 6 ) ) & And2 ) ) & FrameBuffer_XMask ], 6 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 7 ) + ( _mm_extract_epi16 ( TexCoordY, 7 ) << 10 ) ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 7 ) ) & And2 ) ) & FrameBuffer_XMask ], 7 );
				}
				else
				{
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 0 ) + ( _mm_extract_epi16 ( TexCoordY, 0 ) << 10 ) ], 0 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 1 ) + ( _mm_extract_epi16 ( TexCoordY, 1 ) << 10 ) ], 1 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 2 ) + ( _mm_extract_epi16 ( TexCoordY, 2 ) << 10 ) ], 2 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 3 ) + ( _mm_extract_epi16 ( TexCoordY, 3 ) << 10 ) ], 3 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 4 ) + ( _mm_extract_epi16 ( TexCoordY, 4 ) << 10 ) ], 4 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 5 ) + ( _mm_extract_epi16 ( TexCoordY, 5 ) << 10 ) ], 5 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 6 ) + ( _mm_extract_epi16 ( TexCoordY, 6 ) << 10 ) ], 6 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 7 ) + ( _mm_extract_epi16 ( TexCoordY, 7 ) << 10 ) ], 7 );
				}
#else
				
				//iX = x_across;
				//cx = iX;
			
				// make sure we are putting pixel within draw area
				//if ( x_across >= ((s32)DrawArea_TopLeftX) && x_across <= ((s32)DrawArea_BottomRightX) )
				//{
					//color_add = ( _Round( iR ) >> 35 ) | ( ( _Round( iG ) >> 35 ) << 5 ) | ( ( _Round( iB ) >> 35 ) << 10 );
					//color_add = ( _Round( iR ) >> 32 ) | ( ( _Round( iG ) >> 32 ) << 8 ) | ( ( _Round( iB ) >> 32 ) << 16 );
					DitherValue = DitherLine [ x_across & 0x3 ];

					//TexCoordY = (u8) ( ( ( _Round ( iV ) >> 32 ) & ~( TWH << 3 ) ) | ( ( TWY & TWH ) << 3 ) );
					//TexCoordX = (u8) ( ( ( _Round ( iU ) >> 32 ) & ~( TWW << 3 ) ) | ( ( TWX & TWW ) << 3 ) );
					TexCoordY = (u8) ( ( ( _Round24 ( iV ) >> 24 ) & Not_TWH ) | TWYTWH );
					TexCoordX = (u8) ( ( ( _Round24 ( iU ) >> 24 ) & Not_TWW ) | TWXTWW );
					
					//bgr = VRAM [ TextureOffset + ( TexCoordX >> Shift1 ) + ( TexCoordY << 10 ) ];
					bgr = ptr_texture [ ( TexCoordX >> Shift1 ) + ( TexCoordY << 10 ) ];
					
					if ( Shift1 )
					{
						TexelIndex = ( bgr >> ( ( TexCoordX & And1 ) << Shift2 ) ) & And2;
						//bgr = VRAM [ ( ( ( clut_x << 4 ) + TexelIndex ) & FrameBuffer_XMask ) + ( clut_y << 10 ) ];
						bgr = ptr_clut [ ( clut_xoffset + TexelIndex ) & FrameBuffer_XMask ];
					}
#endif


#ifdef _ENABLE_SSE2
					DestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
					vbgr_temp = vbgr;
					if ( !command_tge )
					{
						vRed = _mm_packs_epi32 ( _mm_srai_epi32 ( viR1, 16 ), _mm_srai_epi32 ( viR2, 16 ) );
						vRed = _mm_adds_epu16 ( vRed, vDitherValue_add );
						vRed = _mm_subs_epu16 ( vRed, vDitherValue_sub );
						
						vGreen = _mm_packs_epi32 ( _mm_srai_epi32 ( viG1, 16 ), _mm_srai_epi32 ( viG2, 16 ) );
						vGreen = _mm_adds_epu16 ( vGreen, vDitherValue_add );
						vGreen = _mm_subs_epu16 ( vGreen, vDitherValue_sub );
						
						vBlue = _mm_packs_epi32 ( _mm_srai_epi32 ( viB1, 16 ), _mm_srai_epi32 ( viB2, 16 ) );
						vBlue = _mm_adds_epu16 ( vBlue, vDitherValue_add );
						vBlue = _mm_subs_epu16 ( vBlue, vDitherValue_sub );
						
						vRed = _mm_srli_epi16 ( vRed, 8 );
						vGreen = _mm_srli_epi16 ( vGreen, 8 );
						vBlue = _mm_srli_epi16 ( vBlue, 8 );
						vbgr_temp = vColorMultiply1624 ( vbgr_temp, vRed, vGreen, vBlue );
					}
					if ( command_abe )
					{
						vbgr_temp_transparent = vSemiTransparency16( DestPixel, vbgr_temp, tpage_abr );
						vbgr_select = _mm_srai_epi16 ( vbgr, 15 );
						vbgr_temp = _mm_or_si128 ( _mm_andnot_si128( vbgr_select, vbgr_temp ), _mm_and_si128 ( vbgr_select, vbgr_temp_transparent ) );
					}
					vbgr_temp = _mm_or_si128 ( _mm_or_si128 ( vbgr_temp, SetPixelMask ), _mm_and_si128 ( vbgr, tMask ) );
					_mm_maskmoveu_si128 ( vbgr_temp, _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( DestPixel, 15 ), PixelMask ), _mm_andnot_si128 ( _mm_cmpeq_epi16 ( vbgr, _mm_setzero_si128 () ), _mm_cmplt_epi16 ( vx_across, vEndX ) ) ), (char*) ptr );
					
					vx_across = _mm_add_epi16 ( vx_across, vVectorSize );

					viU1 = _mm_add_epi32 ( viU1, vdU_across );
					viU2 = _mm_add_epi32 ( viU2, vdU_across );
					viV1 = _mm_add_epi32 ( viV1, vdV_across );
					viV2 = _mm_add_epi32 ( viV2, vdV_across );
					
					viR1 = _mm_add_epi32 ( viR1, vdR_across );
					viR2 = _mm_add_epi32 ( viR2, vdR_across );
					viG1 = _mm_add_epi32 ( viG1, vdG_across );
					viG2 = _mm_add_epi32 ( viG2, vdG_across );
					viB1 = _mm_add_epi32 ( viB1, vdB_across );
					viB2 = _mm_add_epi32 ( viB2, vdB_across );
#else

					if ( bgr )
					{
						// shade pixel color
					
						// read pixel from frame buffer if we need to check mask bit
						DestPixel = *ptr;
						
						bgr_temp = bgr;
			
						if ( !command_tge )
						{
							// shade pixel
							Red = ( ( (s16) ( iR >> 24 ) ) * ( (s16) ( bgr_temp & 0x1f ) ) );
							Green = ( ( (s16) ( iG >> 24 ) ) * ( (s16) ( ( bgr_temp >> 5 ) & 0x1f ) ) );
							Blue = ( ( (s16) ( iB >> 24 ) ) * ( (s16) ( ( bgr_temp >> 10 ) & 0x1f ) ) );
						
							// apply dithering if it is enabled
							// dithering must be applied after the color multiply
							Red = Red + DitherValue;
							Green = Green + DitherValue;
							Blue = Blue + DitherValue;
							
							// clamp
							//Red = Clamp5 ( Red >> 7 );
							//Green = Clamp5 ( Green >> 7 );
							//Blue = Clamp5 ( Blue >> 7 );
							Red = SignedClamp<s16,5> ( Red >> 7 );
							Green = SignedClamp<s16,5> ( Green >> 7 );
							Blue = SignedClamp<s16,5> ( Blue >> 7 );
							
							// combine
							bgr_temp = ( Blue << 10 ) | ( Green << 5 ) | ( Red );
						}

						// semi-transparency
						if ( command_abe && ( bgr & 0x8000 ) )
						{
							bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, tpage_abr );
						}
						
						// check if we should set mask bit when drawing
						//if ( GPU_CTRL_Read.MD ) bgr_temp |= 0x8000;
						bgr_temp |= SetPixelMask | ( bgr & 0x8000 );

						// draw pixel if we can draw to mask pixels or mask bit not set
						//if ( ! ( DestPixel & PixelMask ) ) VRAM [ cx + ( cy << 10 ) ] = bgr_temp;
						if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
					}
					
					/////////////////////////////////////////////////////
					// update number of cycles used to draw polygon
					//NumberOfPixelsDrawn++;
				//}
				
				/////////////////////////////////////////////////////////
				// interpolate texture coords
				iU += dU_across;
				iV += dV_across;
				
				iR += dR_across;
				iG += dG_across;
				iB += dB_across;
#endif
				
				ptr += c_iVectorSize;
			}
			
		}
		
		//////////////////////////////////
		// draw next line
		//Line++;
		
		/////////////////////////////////////
		// update x on left and right
		x_left += dx_left;
		x_right += dx_right;
		
		U_left += dU_left;
		V_left += dV_left;
		U_right += dU_right;
		V_right += dV_right;
		
		R_left += dR_left;
		G_left += dG_left;
		B_left += dB_left;
		R_right += dR_right;
		G_right += dG_right;
		B_right += dB_right;
	}
		
}




///////////////////////////////////////////////////////////////////////////
// *** Sprite Drawing ***


void GPU::DrawSprite ()
{
#ifdef _ENABLE_SSE2
	// with sse2, can send 8 pixels at a time
	static const int c_iVectorSize = 8;
#else
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;
#endif

	// notes: looks like sprite size is same as specified by w/h

	//u32 Pixel,
	
	u32 TexelIndex;
	
	u32 color_add;
	
	u16 *ptr_texture, *ptr_clut;
	u32 clut_xoffset, clut_yoffset;
	
	u16 *ptr;
	s32 StartX, EndX, StartY, EndY;
	
	u32 tge;
	
#ifdef _ENABLE_SSE2
	__m128i DestPixel, PixelMask, SetPixelMask;
	__m128i vbgr, vbgr_temp, vStartX, vEndX, vx_across, vSeq, vVectorSize;
	__m128i vbgr_temp_transparent, vbgr_select;
	
	__m128i vSeq1_32, vSeq2_32;
	
	u32 TexCoordY;
	__m128i TexCoordX, Mask, tMask;	//, vTexCoordX2, vTexCoordY1, vTexCoordY2;
	u32 And2 = 0;
	__m128i And1, Shift1, Shift2;
	__m128i TextureOffset, vIndex1, vIndex2;
	__m128i viU, viV;
	
	u32 TWYTWH, Not_TWH;
	__m128i TWXTWW, Not_TWW;
	__m128i color_add_r, color_add_g, color_add_b;
	
	color_add_r = _mm_set1_epi16 ( bgr & 0xff );
	color_add_g = _mm_set1_epi16 ( ( bgr >> 8 ) & 0xff );
	color_add_b = _mm_set1_epi16 ( ( bgr >> 16 ) & 0xff );
	
	Mask = _mm_set1_epi16 ( 0xff );
	tMask = _mm_set1_epi16 ( 0x8000 );
	
	vSeq1_32 = _mm_set_epi32 ( 3, 2, 1, 0 );
	vSeq2_32 = _mm_set_epi32 ( 7, 6, 5, 4 );
	
	TWYTWH = ( ( TWY & TWH ) << 3 );
	TWXTWW = _mm_set1_epi16 ( ( ( TWX & TWW ) << 3 ) );
	
	Not_TWH = ~( TWH << 3 );
	Not_TWW = _mm_set1_epi16 ( ~( TWW << 3 ) );
	
	And1 = _mm_setzero_si128 ();
	Shift1 = _mm_setzero_si128 ();
	
	TextureOffset = _mm_set1_epi32 ( ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 ) );
	
	vSeq = _mm_set_epi16 ( 7, 6, 5, 4, 3, 2, 1, 0 );
	vVectorSize = _mm_set1_epi16 ( c_iVectorSize );
	
	vbgr = _mm_set1_epi16 ( bgr );
	PixelMask = _mm_setzero_si128 ();
	SetPixelMask = _mm_setzero_si128 ();
	if ( GPU_CTRL_Read.ME ) PixelMask = _mm_set1_epi16 ( 0x8080 );
	if ( GPU_CTRL_Read.MD ) SetPixelMask = _mm_set1_epi16 ( 0x8000 );
#else
	u32 DestPixel, PixelMask = 0, SetPixelMask = 0;
	u32 TexCoordX, TexCoordY;
	u32 Shift1 = 0, Shift2 = 0, And1 = 0, And2 = 0;
	u32 TextureOffset;

	u32 TWYTWH, TWXTWW, Not_TWH, Not_TWW;
	
	TWYTWH = ( ( TWY & TWH ) << 3 );
	TWXTWW = ( ( TWX & TWW ) << 3 );
	
	Not_TWH = ~( TWH << 3 );
	Not_TWW = ~( TWW << 3 );

	
	if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
	
	/////////////////////////////////////////////////////////
	// Get offset into texture page
	TextureOffset = ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 );
#endif
	
	u32 PixelsPerLine;

	tge = command_tge;
	if ( ( bgr & 0x00ffffff ) == 0x00808080 ) tge = 1;
	
	//////////////////////////////////////////////////////
	// Get offset into color lookup table
	//u32 ClutOffset = ( clut_x << 4 ) + ( clut_y << 10 );
	
	clut_xoffset = clut_x << 4;
	ptr_clut = & ( VRAM [ clut_y << 10 ] );
	ptr_texture = & ( VRAM [ ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 ) ] );
	
	if ( tpage_tp == 0 )
	{
		And2 = 0xf;
		
#ifdef _ENABLE_SSE2
		Shift1 = _mm_set_epi32 ( 0, 0, 0, 2 );
		Shift2 = _mm_set_epi32 ( 0, 0, 0, 2 );
		And1 = _mm_set1_epi16 ( 3 );
#else
		Shift1 = 2; Shift2 = 2;
		And1 = 3; And2 = 0xf;
#endif
	}
	else if ( tpage_tp == 1 )
	{
		And2 = 0xff;
		
#ifdef _ENABLE_SSE2
		Shift1 = _mm_set_epi32 ( 0, 0, 0, 1 );
		Shift2 = _mm_set_epi32 ( 0, 0, 0, 3 );
		And1 = _mm_set1_epi16 ( 1 );
#else
		Shift1 = 1; Shift2 = 3;
		And1 = 1; And2 = 0xff;
#endif
	}
	
	
	color_add = bgr;

	// initialize number of pixels drawn
	NumberOfPixelsDrawn = 0;
	
	// check for some important conditions
	if ( DrawArea_BottomRightX < DrawArea_TopLeftX )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightX < DrawArea_TopLeftX.\n";
		return;
	}
	
	if ( DrawArea_BottomRightY < DrawArea_TopLeftY )
	{
		//cout << "\nhps1x64 ALERT: GPU: DrawArea_BottomRightY < DrawArea_TopLeftY.\n";
		return;
	}
	
	// get top left corner of sprite and bottom right corner of sprite
	x0 = x;
	y0 = y;
	x1 = x + w - 1;
	y1 = y + h - 1;
	
	//////////////////////////////////////////
	// get coordinates on screen
	x0 = DrawArea_OffsetX + x0;
	y0 = DrawArea_OffsetY + y0;
	x1 = DrawArea_OffsetX + x1;
	y1 = DrawArea_OffsetY + y1;
	
	// check if sprite is within draw area
	if ( x1 < ((s32)DrawArea_TopLeftX) || x0 > ((s32)DrawArea_BottomRightX) || y1 < ((s32)DrawArea_TopLeftY) || y0 > ((s32)DrawArea_BottomRightY) ) return;
	
	

	
	StartX = x0;
	EndX = x1;
	StartY = y0;
	EndY = y1;

	if ( StartY < ((s32)DrawArea_TopLeftY) )
	{
		v += ( DrawArea_TopLeftY - StartY );
		StartY = DrawArea_TopLeftY;
	}
	
	if ( EndY > ((s32)DrawArea_BottomRightY) )
	{
		EndY = DrawArea_BottomRightY;
	}
	
	if ( StartX < ((s32)DrawArea_TopLeftX) )
	{
		u += ( DrawArea_TopLeftX - StartX );
		StartX = DrawArea_TopLeftX;
	}
	
	if ( EndX > ((s32)DrawArea_BottomRightX) )
	{
		EndX = DrawArea_BottomRightX;
	}

	
	iV = v;
	
	NumberOfPixelsDrawn = ( EndX - StartX + 1 ) * ( EndY - StartY + 1 );
		
#ifdef _ENABLE_SSE2
	viV = _mm_set1_epi32 ( iV );
	vEndX = _mm_set1_epi16 ( EndX + 1 );
#endif


//#define DEBUG_DRAWSPRITE
#ifdef DEBUG_DRAWSPRITE
	debug << "\r\nTWX=" << TWX << " TWY=" << TWY << " TWW=" << TWW << " TWH=" << TWH << " TextureWindow_X=" << TextureWindow_X << " TextureWindow_Y=" << TextureWindow_Y << " TextureWindow_Width=" << TextureWindow_Width << " TextureWindow_Height=" << TextureWindow_Height;
#endif

	for ( Line = StartY; Line <= EndY; Line++ )
	{
#ifdef _ENABLE_SSE2
		viU = _mm_add_epi16 ( _mm_set1_epi16 ( u ), vSeq );
		//viU2 = _mm_add_epi32 ( _mm_set1_epi32 ( u ), vSeq2_32 );
		//TexCoordY = _mm_sll_epi32 ( _mm_set1_epi32 ( (u8) ( ( iV & ~( TWH << 3 ) ) | ( ( TWY & TWH ) << 3 ) ) ), 10 );
#else
			// need to start texture coord from left again
			iU = u;
			//TexCoordY = (u8) ( ( iV & ~( TWH << 3 ) ) | ( ( TWY & TWH ) << 3 ) );
#endif

			TexCoordY = (u8) ( ( iV & Not_TWH ) | ( TWYTWH ) );
			TexCoordY <<= 10;

			ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			
#ifdef _ENABLE_SSE2
		vx_across = _mm_add_epi16 ( _mm_set1_epi16 ( StartX ), vSeq );
#endif

			// draw horizontal line
			//for ( x_across = StartX; x_across <= EndX; x_across++ )
			for ( x_across = StartX; x_across <= EndX; x_across += c_iVectorSize )
			{
#ifdef _ENABLE_SSE2
				TexCoordX = _mm_and_si128 ( _mm_or_si128 ( _mm_and_si128 ( viU, Not_TWW ), TWXTWW ), Mask );
				vIndex1 = _mm_srl_epi16 ( TexCoordX, Shift1 );
				if ( And2 )
				{
					vIndex2 = _mm_sll_epi16 ( _mm_and_si128 ( TexCoordX, And1 ), Shift2 );
					
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 0 ) + TexCoordY ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 0 ) ) & And2 ) ) & FrameBuffer_XMask ], 0 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 1 ) + TexCoordY ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 1 ) ) & And2 ) ) & FrameBuffer_XMask ], 1 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 2 ) + TexCoordY ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 2 ) ) & And2 ) ) & FrameBuffer_XMask ], 2 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 3 ) + TexCoordY ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 3 ) ) & And2 ) ) & FrameBuffer_XMask ], 3 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 4 ) + TexCoordY ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 4 ) ) & And2 ) ) & FrameBuffer_XMask ], 4 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 5 ) + TexCoordY ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 5 ) ) & And2 ) ) & FrameBuffer_XMask ], 5 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 6 ) + TexCoordY ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 6 ) ) & And2 ) ) & FrameBuffer_XMask ], 6 );
					bgr = ptr_texture [ _mm_extract_epi16 ( vIndex1, 7 ) + TexCoordY ];
					vbgr = _mm_insert_epi16 ( vbgr, ptr_clut [ ( clut_xoffset + ( ( bgr >> _mm_extract_epi16 ( vIndex2, 7 ) ) & And2 ) ) & FrameBuffer_XMask ], 7 );
				}
				else
				{
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 0 ) + TexCoordY ], 0 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 1 ) + TexCoordY ], 1 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 2 ) + TexCoordY ], 2 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 3 ) + TexCoordY ], 3 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 4 ) + TexCoordY ], 4 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 5 ) + TexCoordY ], 5 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 6 ) + TexCoordY ], 6 );
					vbgr = _mm_insert_epi16 ( vbgr, ptr_texture [ _mm_extract_epi16 ( vIndex1, 7 ) + TexCoordY ], 7 );
				}
#else
					//TexCoordX = (u8) ( ( iU & ~( TWW << 3 ) ) | ( ( TWX & TWW ) << 3 ) );
					TexCoordX = (u8) ( ( iU & Not_TWW ) | ( TWXTWW ) );
					
					//bgr = VRAM [ TextureOffset + ( TexCoordX >> Shift1 ) + ( TexCoordY << 10 ) ];
					//bgr = ptr_texture [ ( TexCoordX >> Shift1 ) + ( TexCoordY << 10 ) ];
					bgr = ptr_texture [ ( TexCoordX >> Shift1 ) + TexCoordY ];
					
					if ( Shift1 )
					{
						//TexelIndex = ( ( bgr >> ( ( TexCoordX & And1 ) << Shift2 ) ) & And2 );
						//bgr = VRAM [ ( ( ( clut_x << 4 ) + TexelIndex ) & FrameBuffer_XMask ) + ( clut_y << 10 ) ];
						bgr = ptr_clut [ ( clut_xoffset + ( ( bgr >> ( ( TexCoordX & And1 ) << Shift2 ) ) & And2 ) ) & FrameBuffer_XMask ];
					}
#endif

					
#ifdef _ENABLE_SSE2
				DestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
				vbgr_temp = vbgr;
				if ( !tge ) vbgr_temp = vColorMultiply1624 ( vbgr_temp, color_add_r, color_add_g, color_add_b );
				if ( command_abe )
				{
					vbgr_temp_transparent = vSemiTransparency16( DestPixel, vbgr_temp, GPU_CTRL_Read.ABR );
					vbgr_select = _mm_srai_epi16 ( vbgr, 15 );
					vbgr_temp = _mm_or_si128 ( _mm_andnot_si128( vbgr_select, vbgr_temp ), _mm_and_si128 ( vbgr_select, vbgr_temp_transparent ) );
				}
				vbgr_temp = _mm_or_si128 ( _mm_or_si128 ( vbgr_temp, SetPixelMask ), _mm_and_si128 ( vbgr, tMask ) );
				_mm_maskmoveu_si128 ( vbgr_temp, _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( DestPixel, 15 ), PixelMask ), _mm_andnot_si128 ( _mm_cmpeq_epi16 ( vbgr, _mm_setzero_si128 () ), _mm_cmplt_epi16 ( vx_across, vEndX ) ) ), (char*) ptr );
				vx_across = _mm_add_epi16 ( vx_across, vVectorSize );
				viU = _mm_add_epi16 ( viU, vVectorSize );
#else
					if ( bgr )
					{
						// read pixel from frame buffer if we need to check mask bit
						DestPixel = *ptr;	//VRAM [ x_across + ( Line << 10 ) ];
						
						bgr_temp = bgr;
			
						if ( !tge )
						{
							// brightness calculation
							//bgr_temp = Color24To16 ( ColorMultiply24 ( Color16To24 ( bgr_temp ), color_add ) );
							bgr_temp = ColorMultiply1624 ( bgr_temp, color_add );
						}
						
						// semi-transparency
						if ( command_abe && ( bgr & 0x8000 ) )
						{
							bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read.ABR );
						}
						
						// check if we should set mask bit when drawing
						//if ( GPU_CTRL_Read.MD ) bgr_temp |= 0x8000;
						bgr_temp |= SetPixelMask | ( bgr & 0x8000 );

						// draw pixel if we can draw to mask pixels or mask bit not set
						//if ( ! ( DestPixel & PixelMask ) ) VRAM [ x_across + ( Line << 10 ) ] = bgr_temp;
						if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
					}
#endif
					
				/////////////////////////////////////////////////////////
				// interpolate texture coords across
				//iU++;	//+= dU_across;
				iU += c_iVectorSize;
				
				// update pointer for pixel out
				//ptr++;
				ptr += c_iVectorSize;
					
			}
		
		/////////////////////////////////////////////////////////
		// interpolate texture coords down
		iV++;	//+= dV_left;
	}
}




////////////////////////////////////////////////////////////////////////////////////////////

void GPU::Draw_MonoTriangle_20 ()
{
	DrawTriangle_Mono ();
	
	// *** TODO *** calculate cycles to draw here
	// check for alpha blending - add in an extra cycle for this for now
	if ( command_abe )
	{
		BusyCycles += NumberOfPixelsDrawn * dAlphaBlending_CyclesPerPixel;
	}
	
	// add in cycles to draw mono-triangle
	BusyCycles += NumberOfPixelsDrawn * dMonoTriangle_20_CyclesPerPixel;
}

void GPU::Draw_MonoRectangle_28 ()
{
	x_save [ 0 ] = x0; x_save [ 1 ] = x1; x_save [ 2 ] = x2; x_save [ 3 ] = x3;
	y_save [ 0 ] = y0; y_save [ 1 ] = y1; y_save [ 2 ] = y2; y_save [ 3 ] = y3;
	bgr_save [ 0 ] = bgr;
	
	//DrawTriangle_Mono ();
	Draw_MonoTriangle_20 ();
	
	x0 = x_save [ 1 ];
	y0 = y_save [ 1 ];
	x1 = x_save [ 2 ];
	y1 = y_save [ 2 ];
	x2 = x_save [ 3 ];
	y2 = y_save [ 3 ];
	
	bgr = bgr_save [ 0 ];
	
	//DrawTriangle_Mono ();
	Draw_MonoTriangle_20 ();
	
}

void GPU::Draw_GradientTriangle_30 ()
{
	
	if ( ( bgr0 & 0x00ffffff ) == ( bgr1 & 0x00ffffff ) && ( bgr0 & 0x00ffffff ) == ( bgr2 & 0x00ffffff ) )
	{
		GetBGR ( Buffer [ 0 ] );
		DrawTriangle_Mono ();
	}
	else
	{
		DrawTriangle_Gradient ();
	}
	
	// *** TODO *** calculate cycles to draw here
	// check for alpha blending - add in an extra cycle for this for now
	if ( command_abe )
	{
		BusyCycles += NumberOfPixelsDrawn * dAlphaBlending_CyclesPerPixel;
	}
	
	// add in cycles to draw mono-triangle
	BusyCycles += NumberOfPixelsDrawn * dGradientTriangle_30_CyclesPerPixel;
}


void GPU::Draw_GradientRectangle_38 ()
{
	x_save [ 0 ] = x0; x_save [ 1 ] = x1; x_save [ 2 ] = x2; x_save [ 3 ] = x3;
	y_save [ 0 ] = y0; y_save [ 1 ] = y1; y_save [ 2 ] = y2; y_save [ 3 ] = y3;
	bgr_save [ 0 ] = bgr0; bgr_save [ 1 ] = bgr1; bgr_save [ 2 ] = bgr2; bgr_save [ 3 ] = bgr3;
	
	//DrawTriangle_Gradient ();
	Draw_GradientTriangle_30 ();
	
	x0 = x_save [ 1 ];
	y0 = y_save [ 1 ];
	x1 = x_save [ 2 ];
	y1 = y_save [ 2 ];
	x2 = x_save [ 3 ];
	y2 = y_save [ 3 ];
	
	bgr0 = bgr_save [ 1 ];
	bgr1 = bgr_save [ 2 ];
	bgr2 = bgr_save [ 3 ];
	
	//DrawTriangle_Gradient ();
	Draw_GradientTriangle_30 ();

}



void GPU::Draw_TextureTriangle_24 ()
{

	//static const double dTextureTriangle4_CyclesPerPixel = 3.38688;
	//static const double dTextureTriangle8_CyclesPerPixel = 6.77376;
	//static const double dTextureTriangle16_CyclesPerPixel = 11.2896;
	//static const double dTextureTriangle4_Gradient_CyclesPerPixel = 4.8384;
	//static const double dTextureTriangle8_Gradient_CyclesPerPixel = 9.6768;
	//static const double dTextureTriangle16_Gradient_CyclesPerPixel = 18.816;
	
	u32 tge;
	tge = command_tge;
	
	if ( ( bgr & 0x00ffffff ) == 0x00808080 )
	{
		command_tge = 1;
	}
	
	DrawTriangle_Texture ();
	
	// restore tge
	command_tge = tge;
	
	// check for alpha blending - add in an extra cycle for this for now
	if ( command_abe )
	{
		BusyCycles += NumberOfPixelsDrawn * dAlphaBlending_CyclesPerPixel;
	}
	
	// check for brightness calculation
	if ( !command_tge )
	{
		BusyCycles += NumberOfPixelsDrawn * dBrightnessCalculation_CyclesPerPixel;
	}
	
	switch ( tpage_tp )
	{
		case 0:		// 4-bit clut
			BusyCycles += NumberOfPixelsDrawn * dTextureTriangle4_24_CyclesPerPixel;	//dTextureTriangle4_CyclesPerPixel;
			break;
			
		case 1:		// 8-bit clut
			BusyCycles += NumberOfPixelsDrawn * dTextureTriangle8_24_CyclesPerPixel;	//dTextureTriangle8_CyclesPerPixel;
			break;
			
		case 2:		// 15-bit color
			BusyCycles += NumberOfPixelsDrawn * dTextureTriangle16_24_CyclesPerPixel;	//dTextureTriangle16_CyclesPerPixel;
			break;
	}
}

void GPU::Draw_TextureRectangle_2c ()
{
	//static const double dTextureTriangle4_CyclesPerPixel = 3.38688;
	//static const double dTextureTriangle8_CyclesPerPixel = 6.77376;
	//static const double dTextureTriangle16_CyclesPerPixel = 11.2896;
	//static const double dTextureTriangle4_Gradient_CyclesPerPixel = 4.8384;
	//static const double dTextureTriangle8_Gradient_CyclesPerPixel = 9.6768;
	//static const double dTextureTriangle16_Gradient_CyclesPerPixel = 18.816;

	
	x_save [ 0 ] = x0; x_save [ 1 ] = x1; x_save [ 2 ] = x2; x_save [ 3 ] = x3;
	y_save [ 0 ] = y0; y_save [ 1 ] = y1; y_save [ 2 ] = y2; y_save [ 3 ] = y3;
	bgr_save [ 0 ] = bgr; bgr_save [ 1 ] = bgr0; bgr_save [ 2 ] = bgr1; bgr_save [ 3 ] = bgr2; bgr_save [ 4 ] = bgr3;
	u_save [ 0 ] = u0; u_save [ 1 ] = u1; u_save [ 2 ] = u2; u_save [ 3 ] = u3;
	v_save [ 0 ] = v0; v_save [ 1 ] = v1; v_save [ 2 ] = v2; v_save [ 3 ] = v3;
	
	//DrawTriangle_Texture ();
	Draw_TextureTriangle_24 ();

	/*
	switch ( tpage_tp )
	{
		case 0:		// 4-bit clut
			BusyCycles = NumberOfPixelsDrawn * dTextureTriangle4_CyclesPerPixel;
			break;
			
		case 1:		// 8-bit clut
			BusyCycles = NumberOfPixelsDrawn * dTextureTriangle8_CyclesPerPixel;
			break;
			
		case 2:		// 15-bit color
			BusyCycles = NumberOfPixelsDrawn * dTextureTriangle16_CyclesPerPixel;
			break;
	}
	*/
	
	x0 = x_save [ 1 ];
	y0 = y_save [ 1 ];
	x1 = x_save [ 2 ];
	y1 = y_save [ 2 ];
	x2 = x_save [ 3 ];
	y2 = y_save [ 3 ];


	bgr = bgr_save [ 0 ];
	//bgr0 = bgr_save [ 2 ];
	//bgr1 = bgr_save [ 3 ];
	//bgr2 = bgr_save [ 4 ];
	//bgr3 = bgr_save [ 4 ];
	
	u0 = u_save [ 1 ];
	v0 = v_save [ 1 ];
	u1 = u_save [ 2 ];
	v1 = v_save [ 2 ];
	u2 = u_save [ 3 ];
	v2 = v_save [ 3 ];
	
	//DrawTriangle_Texture ();
	Draw_TextureTriangle_24 ();

	/*
	switch ( tpage_tp )
	{
		case 0:		// 4-bit clut
			BusyCycles += NumberOfPixelsDrawn * dTextureTriangle4_CyclesPerPixel;
			break;
			
		case 1:		// 8-bit clut
			BusyCycles += NumberOfPixelsDrawn * dTextureTriangle8_CyclesPerPixel;
			break;
			
		case 2:		// 15-bit color
			BusyCycles += NumberOfPixelsDrawn * dTextureTriangle16_CyclesPerPixel;
			break;
	}
	*/

}


void GPU::Draw_TextureGradientTriangle_34 ()
{
	//static const double dTextureTriangle4_CyclesPerPixel = 3.38688;
	//static const double dTextureTriangle8_CyclesPerPixel = 6.77376;
	//static const double dTextureTriangle16_CyclesPerPixel = 11.2896;
	//static const double dTextureTriangle4_Gradient_CyclesPerPixel = 4.8384;
	//static const double dTextureTriangle8_Gradient_CyclesPerPixel = 9.6768;
	//static const double dTextureTriangle16_Gradient_CyclesPerPixel = 18.816;
	
	u32 tge;
	tge = command_tge;
	
	if ( ( bgr0 & 0x00ffffff ) == ( bgr1 & 0x00ffffff ) && ( bgr0 & 0x00ffffff ) == ( bgr2 & 0x00ffffff ) )
	{
		if ( ( bgr0 & 0x00ffffff ) == 0x00808080 )
		{
			command_tge = 1;
		}
		else
		{
			bgr = bgr0;
		}
		
		DrawTriangle_Texture ();
	}
	else
	{
		if ( command_tge )
		{
			DrawTriangle_Texture ();
		}
		else
		{
			DrawTriangle_TextureGradient ();
		}
	}
	
	// restore tge
	command_tge = tge;

	/*
	switch ( tpage_tp )
	{
		case 0:		// 4-bit clut
			BusyCycles = NumberOfPixelsDrawn * dTextureTriangle4_CyclesPerPixel;
			break;
			
		case 1:		// 8-bit clut
			BusyCycles = NumberOfPixelsDrawn * dTextureTriangle8_CyclesPerPixel;
			break;
			
		case 2:		// 15-bit color
			BusyCycles = NumberOfPixelsDrawn * dTextureTriangle16_CyclesPerPixel;
			break;
	}
	*/
	
	// check for alpha blending - add in an extra cycle for this for now
	if ( command_abe )
	{
		BusyCycles += NumberOfPixelsDrawn * dAlphaBlending_CyclesPerPixel;
	}
	
	// check for brightness calculation
	if ( !command_tge )
	{
		BusyCycles += NumberOfPixelsDrawn * dBrightnessCalculation_CyclesPerPixel;
	}
	
	switch ( tpage_tp )
	{
		case 0:		// 4-bit clut
			BusyCycles += NumberOfPixelsDrawn * dTextureTriangle4_34_Gradient_CyclesPerPixel;	//dTextureTriangle4_CyclesPerPixel;
			break;
			
		case 1:		// 8-bit clut
			BusyCycles += NumberOfPixelsDrawn * dTextureTriangle8_34_Gradient_CyclesPerPixel;	//dTextureTriangle8_CyclesPerPixel;
			break;
			
		case 2:		// 15-bit color
			BusyCycles += NumberOfPixelsDrawn * dTextureTriangle16_34_Gradient_CyclesPerPixel;	//dTextureTriangle16_CyclesPerPixel;
			break;
	}
}


void GPU::Draw_TextureGradientRectangle_3c ()
{
	//static const double dTextureTriangle4_CyclesPerPixel = 3.38688;
	//static const double dTextureTriangle8_CyclesPerPixel = 6.77376;
	//static const double dTextureTriangle16_CyclesPerPixel = 11.2896;
	//static const double dTextureTriangle4_Gradient_CyclesPerPixel = 4.8384;
	//static const double dTextureTriangle8_Gradient_CyclesPerPixel = 9.6768;
	//static const double dTextureTriangle16_Gradient_CyclesPerPixel = 18.816;
	
	x_save [ 0 ] = x0; x_save [ 1 ] = x1; x_save [ 2 ] = x2; x_save [ 3 ] = x3;
	y_save [ 0 ] = y0; y_save [ 1 ] = y1; y_save [ 2 ] = y2; y_save [ 3 ] = y3;
	bgr_save [ 0 ] = bgr; bgr_save [ 1 ] = bgr0; bgr_save [ 2 ] = bgr1; bgr_save [ 3 ] = bgr2; bgr_save [ 4 ] = bgr3;
	u_save [ 0 ] = u0; u_save [ 1 ] = u1; u_save [ 2 ] = u2; u_save [ 3 ] = u3;
	v_save [ 0 ] = v0; v_save [ 1 ] = v1; v_save [ 2 ] = v2; v_save [ 3 ] = v3;
	
	bgr = 0x808080;
	
	//DrawTriangle_Texture ();
	Draw_TextureGradientTriangle_34 ();
	
	/*
	switch ( tpage_tp )
	{
		case 0:		// 4-bit clut
			BusyCycles = NumberOfPixelsDrawn * dTextureTriangle4_CyclesPerPixel;
			break;
			
		case 1:		// 8-bit clut
			BusyCycles = NumberOfPixelsDrawn * dTextureTriangle8_CyclesPerPixel;
			break;
			
		case 2:		// 15-bit color
			BusyCycles = NumberOfPixelsDrawn * dTextureTriangle16_CyclesPerPixel;
			break;
	}
	*/
	
	x0 = x_save [ 1 ];
	y0 = y_save [ 1 ];
	x1 = x_save [ 2 ];
	y1 = y_save [ 2 ];
	x2 = x_save [ 3 ];
	y2 = y_save [ 3 ];


	bgr = 0x808080;
	bgr0 = bgr_save [ 2 ];
	bgr1 = bgr_save [ 3 ];
	bgr2 = bgr_save [ 4 ];
	//bgr3 = bgr_save [ 4 ];
	
	u0 = u_save [ 1 ];
	v0 = v_save [ 1 ];
	u1 = u_save [ 2 ];
	v1 = v_save [ 2 ];
	u2 = u_save [ 3 ];
	v2 = v_save [ 3 ];
	
	//DrawTriangle_Texture ();
	Draw_TextureGradientTriangle_34 ();

	/*
	switch ( tpage_tp )
	{
		case 0:		// 4-bit clut
			BusyCycles += NumberOfPixelsDrawn * dTextureTriangle4_CyclesPerPixel;
			break;
			
		case 1:		// 8-bit clut
			BusyCycles += NumberOfPixelsDrawn * dTextureTriangle8_CyclesPerPixel;
			break;
			
		case 2:		// 15-bit color
			BusyCycles += NumberOfPixelsDrawn * dTextureTriangle16_CyclesPerPixel;
			break;
	}
	*/

}




void GPU::Draw_Sprite_64 ()
{
	//static const double dSprite4_Cycles = 1.2027;
	//static const double dSprite8_Cycles = 1.89;
	//static const double dSprite16_Cycles = 3.3075;
	
	tpage_tx = GPU_CTRL_Read.TX;
	tpage_ty = GPU_CTRL_Read.TY;
	tpage_tp = GPU_CTRL_Read.TP;
	
	DrawSprite ();

	// set number of cycles it takes to draw sprite
	switch ( tpage_tp )
	{
		case 0:		// 4-bit clut
			BusyCycles = NumberOfPixelsDrawn * dSprite4_64_Cycles;
			break;
			
		case 1:		// 8-bit clut
			BusyCycles = NumberOfPixelsDrawn * dSprite8_64_Cycles;
			break;
			
		case 2:		// 15-bit color
			BusyCycles = NumberOfPixelsDrawn * dSprite16_64_Cycles;
			break;
	}

}



void GPU::Draw_Sprite8x8_74 ()
{
	static const u32 SpriteSize = 8;
	
	//static const int CyclesPerSprite8x8_4bit = 121;
	//static const int CyclesPerSprite8x8_8bit = 121;
	//static const int CyclesPerSprite8x8_16bit = 212;
	

	//static const int TextureSizeX = 256;
	//static const int TextureSizeY = 32;


	tpage_tx = GPU_CTRL_Read.TX;
	tpage_ty = GPU_CTRL_Read.TY;
	tpage_tp = GPU_CTRL_Read.TP;
	tpage_abr = GPU_CTRL_Read.ABR;
	
	w = 8; h = 8;
	DrawSprite ();

	// set number of cycles it takes to draw 8x8 sprite
	switch ( tpage_tp )
	{
		case 0:		// 4-bit clut
			BusyCycles = dCyclesPerSprite8x8_74_4bit;	//121;
			break;
			
		case 1:		// 8-bit clut
			BusyCycles = dCyclesPerSprite8x8_74_8bit;	//121;
			break;
			
		case 2:		// 15-bit color
			BusyCycles = dCyclesPerSprite8x8_74_16bit;	//212;
			break;
	}


}



void GPU::Draw_Sprite16x16_7c ()
{
	static const u32 SpriteSize = 16;
	
	//static const int CyclesPerSprite16x16_4bit = 308;
	//static const int CyclesPerSprite16x16_8bit = 484;
	//static const int CyclesPerSprite16x16_16bit = 847;

	//static const int TextureSizeX = 256;
	//static const int TextureSizeY = 32;

	tpage_tx = GPU_CTRL_Read.TX;
	tpage_ty = GPU_CTRL_Read.TY;
	tpage_tp = GPU_CTRL_Read.TP;
	tpage_abr = GPU_CTRL_Read.ABR;
	
	w = 16; h = 16;
	DrawSprite ();

	// set number of cycles it takes to draw 16x16 sprite
	switch ( tpage_tp )
	{
		case 0:		// 4-bit clut
			BusyCycles = dCyclesPerSprite16x16_7c_4bit;	//308;
			break;
			
		case 1:		// 8-bit clut
			BusyCycles = dCyclesPerSprite16x16_7c_8bit;	//484;
			break;
			
		case 2:		// 15-bit color
			BusyCycles = dCyclesPerSprite16x16_7c_16bit;	//847;
			break;
	}


}



void GPU::Draw_MonoLine_40 ()
{
	static const double dMonoLine_40_CyclesPerPixel = 1.0L;

	s32 distance, x_distance, y_distance;
	
	s64 dxdc;
	s64 dydc;
	s64 line_x, line_y;
	
	u32 DestPixel, PixelMask = 0;
	
	NumberOfPixelsDrawn = 0;

	if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	
	//////////////////////////////////////////
	// get coordinates on screen
	x0 = DrawArea_OffsetX + x0;
	y0 = DrawArea_OffsetY + y0;
	x1 = DrawArea_OffsetX + x1;
	y1 = DrawArea_OffsetY + y1;

	// get size of line
	x_distance = _Abs( x1 - x0 );
	y_distance = _Abs( y1 - y0 );
	if ( x_distance > y_distance ) distance = x_distance; else distance = y_distance;
	
	// going from y0 to y1, get the change in y for every change in count
	if ( distance )
	{
		dxdc = ( ( (s64)x1 - (s64)x0 ) << 32 ) / distance;
		dydc = ( ( (s64)y1 - (s64)y0 ) << 32 ) / distance;
	}
	
	// init line x,y
	line_x = ( ( (s64) x0 ) << 32 );
	line_y = ( ( (s64) y0 ) << 32 );
	
	// draw the line
	for ( u32 i = 0; i <= distance; i++ )
	{
		// get x coord
		iX = ( _Round( line_x ) >> 32 );
		
		// get y coord
		iY = ( _Round( line_y ) >> 32 );
		
		if ( iX >= DrawArea_TopLeftX && iY >= DrawArea_TopLeftY
		&& iX <= DrawArea_BottomRightX && iY <= DrawArea_BottomRightY )
		{
			bgr2 = bgr;
			
			// read pixel from frame buffer if we need to check mask bit
			DestPixel = VRAM [ iX + ( iY << 10 ) ];
		
			// semi-transparency
			if ( command_abe )
			{
				bgr2 = SemiTransparency16 ( VRAM [ iX + ( iY << 10 ) ], bgr, GPU_CTRL_Read.ABR );
			}
				
			// draw point
			
			// check if we should set mask bit when drawing
			if ( GPU_CTRL_Read.MD ) bgr2 |= 0x8000;

			// draw pixel if we can draw to mask pixels or mask bit not set
			if ( ! ( DestPixel & PixelMask ) ) VRAM [ iX + ( iY << 10 ) ] = bgr2;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn++;
		}
		
		line_x += dxdc;
		line_y += dydc;
	}
	
	BusyCycles += NumberOfPixelsDrawn * dMonoLine_40_CyclesPerPixel;
}



void GPU::Draw_ShadedLine_50 ()
{
	static const double dShadedLine_50_CyclesPerPixel = 2.0L;

	s32 distance, x_distance, y_distance;
	
	s64 dxdc;
	s64 dydc;
	s64 drdc, dgdc, dbdc;
	s64 line_x, line_y;
	s64 line_r, line_g, line_b;
	
	u32 DestPixel, PixelMask = 0;
	
	NumberOfPixelsDrawn = 0;

	if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	
	// get color components
	r0 = bgr0 & 0xff;
	r1 = bgr1 & 0xff;
	g0 = ( bgr0 >> 8 ) & 0xff;
	g1 = ( bgr1 >> 8 ) & 0xff;
	b0 = ( bgr0 >> 16 ) & 0xff;
	b1 = ( bgr1 >> 16 ) & 0xff;
	
	//////////////////////////////////////////
	// get coordinates on screen
	x0 = DrawArea_OffsetX + x0;
	y0 = DrawArea_OffsetY + y0;
	x1 = DrawArea_OffsetX + x1;
	y1 = DrawArea_OffsetY + y1;

	// get size of line
	x_distance = _Abs( x1 - x0 );
	y_distance = _Abs( y1 - y0 );
	if ( x_distance > y_distance ) distance = x_distance; else distance = y_distance;
	
	// going from y0 to y1, get the change in y for every change in count
	if ( distance )
	{
		dxdc = ( ( (s64)x1 - (s64)x0 ) << 32 ) / distance;
		dydc = ( ( (s64)y1 - (s64)y0 ) << 32 ) / distance;
		drdc = ( ( (s64)r1 - (s64)r0 ) << 32 ) / distance;
		dgdc = ( ( (s64)g1 - (s64)g0 ) << 32 ) / distance;
		dbdc = ( ( (s64)b1 - (s64)b0 ) << 32 ) / distance;
	}
	
	// init line x,y
	line_x = ( ( (s64) x0 ) << 32 );
	line_y = ( ( (s64) y0 ) << 32 );
	line_r = ( ( (u64) r0 ) << 32 );
	line_g = ( ( (u64) g0 ) << 32 );
	line_b = ( ( (u64) b0 ) << 32 );
	
	
	// draw the line
	for ( u32 i = 0; i <= distance; i++ )
	{
		// get coords
		iX = ( _Round( line_x ) >> 32 );
		iY = ( _Round( line_y ) >> 32 );
		
		if ( iX >= DrawArea_TopLeftX && iY >= DrawArea_TopLeftY
		&& iX <= DrawArea_BottomRightX && iY <= DrawArea_BottomRightY )
		{
			bgr = ( _Round( line_r ) >> 35 ) | ( ( _Round( line_g ) >> 35 ) << 5 ) | ( ( _Round( line_b ) >> 35 ) << 10 );
			
			// read pixel from frame buffer if we need to check mask bit
			DestPixel = VRAM [ iX + ( iY << 10 ) ];
		
			// semi-transparency
			if ( command_abe )
			{
				bgr = SemiTransparency16 ( VRAM [ iX + ( iY << 10 ) ], bgr, GPU_CTRL_Read.ABR );
			}
				
			// draw point
			
			// check if we should set mask bit when drawing
			if ( GPU_CTRL_Read.MD ) bgr |= 0x8000;

			// draw pixel if we can draw to mask pixels or mask bit not set
			if ( ! ( DestPixel & PixelMask ) ) VRAM [ iX + ( iY << 10 ) ] = bgr;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn++;
		}
		
		line_x += dxdc;
		line_y += dydc;
		line_r += drdc;
		line_g += dgdc;
		line_b += dbdc;
	}
	
	BusyCycles += NumberOfPixelsDrawn * dShadedLine_50_CyclesPerPixel;
}



void GPU::Set_ScreenSize ( int _width, int _height )
{
	DisplayOutput_Window->OpenGL_MakeCurrentWindow ();
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glOrtho (0, _width, _height, 0, 0, 1);
	glMatrixMode (GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	DisplayOutput_Window->OpenGL_ReleaseWindow ();
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void GPU::DebugWindow_Enable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	static const char* DebugWindow_Caption = "PS1 FrameBuffer Debug Window";
	static const int DebugWindow_X = 10;
	static const int DebugWindow_Y = 10;
	static const int DebugWindow_Width = 1024;
	static const int DebugWindow_Height = 512;
	
	int i;
	volatile u32 xsize, ysize;
	stringstream ss;
	
	cout << "\nGPU::DebugWindow_Enable";
	
	if ( !DebugWindow_Enabled )
	{
		// create the main debug window
		xsize = DebugWindow_Width;
		ysize = DebugWindow_Height;
		FrameBuffer_DebugWindow = new WindowClass::Window ();
		FrameBuffer_DebugWindow->GetRequiredWindowSize ( &xsize, &ysize, FALSE );
		FrameBuffer_DebugWindow->Create ( DebugWindow_Caption, DebugWindow_X, DebugWindow_Y, xsize /*DebugWindow_Width*/, ysize /*DebugWindow_Height + 50*/ );
		FrameBuffer_DebugWindow->DisableCloseButton ();
		
		cout << "\nFramebuffer: xsize=" << xsize << "; ysize=" << ysize;
		FrameBuffer_DebugWindow->GetWindowSize ( &xsize, &ysize );
		cout << "\nWindow Size. xsize=" << xsize << "; ysize=" << ysize;
		FrameBuffer_DebugWindow->GetViewableArea ( &xsize, &ysize );
		cout << "\nViewable Size. xsize=" << xsize << "; ysize=" << ysize;
		
		cout << "\nCreated main debug window";
		
		/////////////////////////////////////////////////////////
		// enable opengl for the frame buffer window
		FrameBuffer_DebugWindow->EnableOpenGL ();
		glMatrixMode (GL_PROJECTION);
		glLoadIdentity ();
		glOrtho (0, DebugWindow_Width, DebugWindow_Height, 0, 0, 1);
		glMatrixMode (GL_MODELVIEW);

		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);

		// this window is no longer the window we want to draw to
		FrameBuffer_DebugWindow->OpenGL_ReleaseWindow ();
		
		DebugWindow_Enabled = true;
		
		cout << "\nEnabled opengl for frame buffer window";

		// update the value lists
		DebugWindow_Update ();
	}
	
		cout << "\n->GPU::DebugWindow_Enable";

#endif

}

static void GPU::DebugWindow_Disable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	if ( DebugWindow_Enabled )
	{
		delete FrameBuffer_DebugWindow;
	
		// disable debug window
		DebugWindow_Enabled = false;
	}
	
#endif

}

static void GPU::DebugWindow_Update ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	if ( DebugWindow_Enabled )
	{
		_GPU->Draw_FrameBuffer ();
	}
	
#endif

}


