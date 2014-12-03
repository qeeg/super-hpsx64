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


#include "PS2_Gpu.h"
#include <math.h>
#include "PS2_Timer.h"
#include "Reciprocal.h"




using namespace Playstation2;
//using namespace x64Asm::Utilities;
using namespace Math::Reciprocal;



//#define USE_DIVIDE_GCC
//#define USE_MULTIPLY_CUSTOM


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


// enable debugging

#ifdef _DEBUG_VERSION_

#define INLINE_DEBUG_ENABLE

/*
#define INLINE_DEBUG_READ
#define INLINE_DEBUG_WRITE
#define INLINE_DEBUG_DMA_WRITE



#define INLINE_DEBUG_PRIMITIVE


#define INLINE_DEBUG_FIFO
#define INLINE_DEBUG_PATH1_WRITE
#define INLINE_DEBUG_PATH2_WRITE


//#define INLINE_DEBUG_SPRITE
//#define INLINE_DEBUG_XFER


#define INLINE_DEBUG_INVALID
*/

//#define INLINE_DEBUG_DRAW_SCREEN
//#define INLINE_DEBUG_RASTER_VBLANK_START
//#define INLINE_DEBUG_RASTER_VBLANK_END



//#define INLINE_DEBUG_RASTER_SCANLINE


//#define INLINE_DEBUG_PRIMITIVE_TEST
//#define INLINE_DEBUG_TRANSFER_IN
//#define INLINE_DEBUG_TRANSFER_IN_2

//#define INLINE_DEBUG_DRAWKICK

//#define INLINE_DEBUG_EXECUTE

//#define INLINE_DEBUG_DRAWSTART
//#define INLINE_DEBUG_EVENT
//#define INLINE_DEBUG_VARS
//#define INLINE_DEBUG_EXECUTE_NAME

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
u32* GPU::_R5900_Status_12;
u32* GPU::_R5900_Cause_13;
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


static const char* GPU::GIFRegNames [ 11 ] = { "GIF_CTRL", "GIF_MODE", "GIF_STAT", "Reserved", "GIF_TAG0", "GIF_TAG1", "GIF_TAG2", "GIF_TAG3", "GIF_CNT", "GIF_P3CNT", "GIF_P3TAG" };
static const char* GPU::GPUReg0Names [ 15 ] = { "GPU_PMODE", "GPU_SMODE1", "GPU_SMODE2", "GPU_SRFSH", "GPU_SYNCH1", "GPU_SYNCH2", "GPU_SYNCV", "GPU_DISPFB1",
												"GPU_DISPLAY1", "GPU_DISPFB2", "GPU_DISPLAY2", "GPU_EXTBUF", "GPU_EXTDATA", "GPU_EXTWRITE", "GPU_BGCOLOR" };
static const char* GPU::GPUReg1Names [ 9 ] = { "GPU_CSR", "GPU_IMR", "Reserved", "Reserved", "GPU_BUSDIR", "Reserved", "Reserved", "Reserved", "GPU_SIGLBLID" };

static const char* GPU::GPURegsGp_Names [ 0x63 ] = { "PRIM", "RGBAQ", "ST", "UV", "XYZF2", "XYZ2", "TEX0_1", "TEX0_2",	// 0x00-0x07
													"CLAMP_1", "CLAMP_2", "FOG", "", "XYZF3", "XYZ3", "", "",				// 0x08-0x0f
													"", "", "", "", "TEX1_1", "TEX1_2", "TEX2_1", "TEX2_2",					// 0x10-0x17
													"XYOFFSET_1", "XYOFFSET_2", "PRMODECONT", "PRMODE", "TEXCLUT", "", "", "",	// 0x18-0x1f
													"", "", "SCANMSK", "", "", "", "", "",									// 0x20-0x27
													"", "", "", "", "", "", "", "",											// 0x28-0x2f
													"", "", "", "", "MIPTBP1_1", "MIPTBP1_2", "MIPTBP2_1", "MIPTBP2_2",		// 0x30-0x37
													"", "", "", "TEXA", "", "FOGCOL", "", "TEXFLUSH",						// 0x38-0x3f
													"SCISSOR_1", "SCISSOR_2", "ALPHA_1", "ALPHA_2", "DIMX", "DTHE", "COLCLAMP", "TEST_1",	// 0x40-0x47
													"TEST_2", "PABE", "FBA_1", "FBA_2", "FRAME_1", "FRAME_2", "ZBUF_1", "ZBUF_2",	// 0x48-0x4f
													"BITBLTBUF", "TRXPOS", "TRXREG", "TRXDIR", "HWREG", "", "", "",			// 0x50-0x57
													"", "", "", "", "", "", "", "",											// 0x58-0x5f
													"SIGNAL", "FINISH", "LABEL" };											// 0x60-0x62

// 000000: PSMCT32, 000001: PSMCT24, 000010: PSMCT16, 001010: PSMCT16S, 010011: PSMT8, 010100: PSMT4, 011011: PSMT8H,
// 100100: PSMT4HL, 101100: PSMT4HH, 110000: PSMZ32, 110001: PSMZ24, 110010: PSMZ16, 111010: PSMZ16S
static const char* GPU::PixelFormat_Names [ 64 ] =  { "PSMCT32", "PSMCT24", "PSMCT16", "UNK", "UNK", "UNK", "UNK", "UNK",	// 0-7
													"UNK", "UNK", "PSMCT16S", "UNK", "UNK", "UNK", "UNK", "UNK",			// 8-15
													"UNK", "UNK", "UNK", "PSMT8", "PSMT4", "UNK", "UNK", "UNK",				// 16-23
													"UNK", "UNK", "UNK", "PSMT8H", "UNK", "UNK", "UNK", "UNK",				// 24-31
													"UNK", "UNK", "UNK", "UNK", "PSMT4HL", "UNK", "UNK", "UNK",				// 32-39
													"UNK", "UNK", "UNK", "UNK", "PSMT4HH", "UNK", "UNK", "UNK",				// 40-47
													"PSMZ32", "PSMZ24", "PSMZ16", "UNK", "UNK", "UNK", "UNK", "UNK",		// 48-55
													"UNK", "UNK", "PSMZ16S", "UNK", "UNK", "UNK", "UNK", "UNK"				// 56-63
													};

static const char* GPU::TransferDir_Names [ 4 ] = { "UpperLeft->LowerRight", "LowerLeft->UpperRight", "UpperRight->LowerLeft", "LowerRight->UpperLeft" };



GPU::GPU ()
{

	cout << "Running GPU constructor...\n";
}

void GPU::Reset ()
{
	// zero object
	memset ( this, 0, sizeof( GPU ) );

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

	// default value for IMR is 0x7f00
	GPURegs1.IMR.Value = 0x7f00;
}



// actually, you need to start objects after everything has been initialized
void GPU::Start ()
{
	cout << "Running GPU::Start...\n";
	
#ifdef INLINE_DEBUG_ENABLE
	debug.Create ( "PS2_GPU_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering GPU::Start";
#endif

	cout << "Resetting GPU...\n";

	Reset ();

	cout << "Testing GPU...\n";
	
	///////////////////////////////
	// *** TESTING ***
	//GPU_CTRL_Read.Value = 0x14802000;
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




void GPU::Run ()
{
	// want to run the events in the right order, so need to check for exact match with current cycle count
	if ( NextEvent_Cycle != *_DebugCycleCount ) return;
	

	// must be start of a scanline //	
	
	// update scanline number
	lScanline = lNextScanline;
	lNextScanline += 2;
	if ( lNextScanline >= lMaxScanline )
	{
		// End of VBLANK //
		lNextScanline -= lMaxScanline;
		
		// trigger end of vblank interrupt for intc
		SetInterrupt_Vsync_End ();
		
#ifdef INLINE_DEBUG_RASTER_VBLANK_END
	debug << "\r\n\r\n***VBLANK END*** " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; BusyCycles=" << dec << BusyCycles << " Scanline_Number=" << lScanline;
#endif
	}


	// handle interlacing //
	
	// check SMODE2 to see if in interlaced mode or not
	if ( GPURegs0.SMODE2.INTER )
	{
		// Interlaced mode //
		
		// set the field being drawn in CSR
		// 0: even, 1: odd
		GPURegs1.CSR.FIELD = lScanline & 1;
	}
	else
	{
		// NON-Interlaced mode //
		
		// set the field being drawn??
		// maybe it changes per scanline?? or per draw??
		GPURegs1.CSR.FIELD = lScanline & 1;
	}

	// check if this is vblank or time to draw screen
	if ( ( lScanline & ~1 ) == lVBlank )
	{
		// vblank //
#ifdef INLINE_DEBUG_RASTER_VBLANK_START
	debug << "\r\n\r\n***VBLANK START*** " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; BusyCycles=" << dec << BusyCycles << " Scanline_Number=" << lScanline;
	debug << " Interlaced?=" << GPURegs0.SMODE2.INTER << " Field=" << GPURegs1.CSR.FIELD << " Nfield=" << GPURegs1.CSR.NFIELD;
#endif

		// set NFIELD (gets set at VSYNC??)
		GPURegs1.CSR.NFIELD = lScanline & 1;
		//GPURegs1.CSR.NFIELD = 0;
		
		// update count of frames for debugging
		Frame_Count++;
		
		
		// draw screen //

		// draw output to program window @ vblank! - if output window is available
		// this is actually probably wrong. Should actually draw screen after vblank is over
		if ( DisplayOutput_Window )
		{
			Draw_Screen ();
			
			if ( DebugWindow_Enabled )
			{
				Draw_FrameBuffers ();
			}
		}
		
		// handle vblank //
		
		// check if vblank is masked
		if ( !GPURegs1.IMR.VSMSK )
		{
			// make sure that bit is clear (enabled) in CSR
			if ( !GPURegs1.CSR.VSINT )
			{
				// trigger interrupt due to vsync
				SetInterrupt ();
				
				// set bit in CSR (vsync event?? or maybe vsync interrupt occurred)
				GPURegs1.CSR.VSINT = 1;
			}
		}
		
		
		// trigger start of vblank interrupt for intc
		SetInterrupt_Vsync_Start ();
	}

#ifdef INLINE_DEBUG_RASTER_SCANLINE
	debug << "\r\n\r\n***SCANLINE*** " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; BusyCycles=" << dec << BusyCycles << " Scanline=" << lScanline << " llScanlineStart=" << llScanlineStart << " llHBlankStart=" << llHBlankStart << " llNextScanlineStart=" << llNextScanlineStart << " lVBlank=" << lVBlank << " CyclesPerScanline=" << dCyclesPerScanline;
	debug << " Interlaced?=" << GPURegs0.SMODE2.INTER << " Field=" << GPURegs1.CSR.FIELD;
#endif
	
	// update timers //
	// do this before updating the next event
	Timers::_TIMERS->UpdateTimer ( 0 );
	Timers::_TIMERS->UpdateTimer ( 1 );
	Timers::_TIMERS->UpdateTimer ( 2 );
	Timers::_TIMERS->UpdateTimer ( 3 );
	
	// get the cycle number at start of the next scanline and update associated variables
	Update_NextEvent ();
	
	// update timer events //
	Timers::_TIMERS->Get_NextEvent ( 0 );
	Timers::_TIMERS->Get_NextEvent ( 1 );
	Timers::_TIMERS->Get_NextEvent ( 2 );
	Timers::_TIMERS->Get_NextEvent ( 3 );
	
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
	
	dScanlineStart = dNextScanlineStart;
	dNextScanlineStart += dCyclesPerScanline;
	dHBlankStart = dNextScanlineStart - dHBlankArea_Cycles;
	
	// convert to integers
	llScanlineStart = (u64) dScanlineStart;
	llNextScanlineStart = (u64) dNextScanlineStart;
	llHBlankStart = (u64) dHBlankStart;
	
	SetNextEvent_Cycle ( llNextScanlineStart );
	
	/*
	dGPU_NextEventCycle += dCyclesPerScanline;
	//iGPU_NextEventCycle += iCyclesPerScanline;
	//dCyclesToNext = (double)(*_DebugCycleCount)
	//CycleOffset1 = (u64) dGPU_NextEventCycle;
	
	NextEvent_Cycle = (u64) dGPU_NextEventCycle;
	
	if ( ( dGPU_NextEventCycle - ( (double) NextEvent_Cycle ) ) > 0.0L ) NextEvent_Cycle++;
	
	//SetNextEvent_Cycle ( iGPU_NextEventCycle );
	Update_NextEventCycle ();
	*/
	
#ifdef INLINE_DEBUG_EVENT
	debug << "\r\nGPU::Update_NextEvent CycleOffset=" << dec << dCyclesPerScanline;
#endif
}


void GPU::GetNextEvent ()
{
	//u64 CycleOffset1;	//, CycleOffset2;
	
	// *todo* should also probably combine this stuff with updating the current scanline number and re-calibrating the timers
	lScanline = GetScanline_Number ();
	lNextScanline = lScanline + 2;
	if ( lNextScanline >= lMaxScanline )
	{
		// End of VBLANK //
		lNextScanline -= lMaxScanline;
	}
	
	dScanlineStart = GetScanline_Start ();
	dNextScanlineStart = dScanlineStart + dCyclesPerScanline;
	dHBlankStart = dNextScanlineStart - dHBlankArea_Cycles;
	
	// convert to integers
	llScanlineStart = (u64) dScanlineStart;
	llNextScanlineStart = (u64) dNextScanlineStart;
	llHBlankStart = (u64) dHBlankStart;
	
	// the next gpu event is at the start of the next scanline
	//dGPU_NextEventCycle = dScanlineStart;
	SetNextEvent_Cycle ( llNextScanlineStart );
	
	/*
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
	*/
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


static u64 GPU::Read ( u32 Address, u64 Mask )
{
	u32 Temp;
	u64 Output = 0;
	u32 lReg;

#ifdef INLINE_DEBUG_READ
	debug << "\r\n\r\nGPU::Read; " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << " Mask=" << Mask;
#endif

	// will set some values here for now
	switch ( Address )
	{
		case GIF_STAT:
		
			// check if the GPU is busy with something
			if ( *_DebugCycleCount < _GPU->BusyUntil_Cycle )
			{
				// set the current path
				_GPU->GIFRegs.STAT.APATH = _GPU->CurrentPath;
				
				Temp = ( _GPU->FifoSize > 16 ) ? 16 : _GPU->FifoSize;
				
				_GPU->GIFRegs.STAT.FQC = Temp;
				
				// request to wait
				_GPU->GIFRegs.STAT.P1Q = 1;
				_GPU->GIFRegs.STAT.P2Q = 1;
				_GPU->GIFRegs.STAT.P3Q = 1;
			}
			else
			{
				// fifo must be empty
				_GPU->FifoSize = 0;
				Temp = 0;
				_GPU->GIFRegs.STAT.FQC = 0;
				
				// clear path
				_GPU->GIFRegs.STAT.APATH = 0;
				
				// no request to wait
				_GPU->GIFRegs.STAT.P1Q = 0;
				_GPU->GIFRegs.STAT.P2Q = 0;
				_GPU->GIFRegs.STAT.P3Q = 0;
			}
			
			break;
			
		case GPU_CSR:
			_GPU->GPURegs1.CSR.REV = c_ulGS_Revision;
			_GPU->GPURegs1.CSR.ID = c_ulGS_ID;
			
			// test if drawing is complete
			if ( *_DebugCycleCount < _GPU->BusyUntil_Cycle )
			{
				if ( _GPU->FifoSize )
				{
					// fifo is full or close to full ??
					_GPU->GPURegs1.CSR.FIFO = 2;
				}
				else if ( !_GPU->FifoSize )
				{
					// fifo is empty
					_GPU->GPURegs1.CSR.FIFO = 1;
				}
			}
			else
			{
				// fifo is empty
				_GPU->GPURegs1.CSR.FIFO = 1;
			}
			
			// bits 13-14 should be set to 1 ??
			//_GPU->GPURegs1.CSR.Value |= 0x6000;
			// setting bit 14 to 1 means that fifo is empty
			// ***todo*** implement correct fifo status
			//_GPU->GPURegs1.CSR.Value |= 0x4000;
			
			break;
			
		default:
			break;
	}
	
	lReg = ( Address >> 4 ) & 0xf;

	// check if these are GIF Regs or GPU Priveleged Registers
	switch ( Address & 0xf000 )
	{
		// GPU priveleged registers group 0
		case 0x0000:
#ifdef INLINE_DEBUG_READ
			if ( lReg < 15 )
			{
			debug << "; " << _GPU->GPUReg0Names [ lReg ];
			}
#endif

			Output = _GPU->GPURegs0.Regs [ lReg ];
			break;
			
		// GPU priveleged registers group 1
		case 0x1000:
#ifdef INLINE_DEBUG_READ
			if ( lReg < 9 )
			{
			debug << "; " << _GPU->GPUReg1Names [ lReg ];
			}
#endif

			Output = _GPU->GPURegs1.Regs [ lReg ];
			break;
			
		// GIF Registers
		case 0x3000:
#ifdef INLINE_DEBUG_READ
			if ( lReg < 11 )
			{
			debug << "; " << _GPU->GIFRegNames [ lReg ];
			}
#endif

			Output = _GPU->GIFRegs.Regs [ lReg ];
			break;
			
		// GIF FIFO
		case 0x6000:
#ifdef INLINE_DEBUG_READ
			debug << "; GIF_FIFO";
#endif

			break;
	}
	

	
	/*
	switch ( Address )
	{
		case GIF_CTRL:	// 0x10003000
#ifdef INLINE_DEBUG_READ
			debug << "; GIF_CTRL";
#endif

			break;
			
		case GIF_MODE:	// 0x10003010
#ifdef INLINE_DEBUG_READ
			debug << "; GIF_MODE";
#endif

			break;
			
		case GIF_STAT:	// 0x10003020
#ifdef INLINE_DEBUG_READ
			debug << "; GIF_STAT";
#endif

			Output = _GPU->GIF_STAT_Reg.Value;
			break;
			
		case GIF_FIFO:	// 0x10006000
#ifdef INLINE_DEBUG_READ
			debug << "; GIF_FIFO";
#endif

			break;
	}
	*/
	
	
#ifdef INLINE_DEBUG_READ
	debug << "; Output=" << hex << Output;
#endif

	return Output;
}


static void GPU::Write ( u32 Address, u64 Data, u64 Mask )
{
	u32 lReg;
	u32 ulTempArray [ 4 ];
	GIFTag1_t Arg0, Arg1;
	
#ifdef INLINE_DEBUG_WRITE
	debug << "\r\n\r\nGPU::Write; " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Data = " << Data << " Mask=" << Mask;
#endif

	// perform actions before write
	switch ( Address )
	{
			
		default:
			break;
	}

	
	// get register number being written to
	lReg = ( Address >> 4 ) & 0xf;
	
	// check if these are GIF Regs or GPU Priveleged Registers
	switch ( Address & 0xf000 )
	{
		// GPU priveleged registers group 0
		case 0x0000:
#ifdef INLINE_DEBUG_WRITE
			if ( lReg < 15 )
			{
			debug << "; " << _GPU->GPUReg0Names [ lReg ];
			}
#endif

			_GPU->GPURegs0.Regs [ lReg ] = Data;
			break;
			
		// GPU priveleged registers group 1
		case 0x1000:
#ifdef INLINE_DEBUG_WRITE
			if ( lReg < 9 )
			{
			debug << "; " << _GPU->GPUReg1Names [ lReg ];
			}
#endif

			_GPU->GPURegs1.Regs [ lReg ] = Data;
			break;
			
		// GIF Registers
		case 0x3000:
#ifdef INLINE_DEBUG_WRITE
			if ( lReg < 11 )
			{
			debug << "; " << _GPU->GIFRegNames [ lReg ];
			}
#endif

			_GPU->GIFRegs.Regs [ lReg ] = Data;
			break;
			
		// GIF FIFO
		case 0x6000:
#ifdef INLINE_DEBUG_WRITE
			debug << "; GIF_FIFO";
			if ( !Mask ) debug << "; Value=" << hex << ((u32*)Data) [ 0 ] << " " << ((u32*)Data) [ 1 ] << " " << ((u32*)Data) [ 2 ] << " " << ((u32*)Data) [ 3 ];
#endif

			// transferring data via path 3
			_GPU->CurrentPath = 3;
			
			// if device not busy, then clear fifo size
			if ( *_DebugCycleCount < _GPU->BusyUntil_Cycle )
			{
				_GPU->FifoSize = 0;
			}
			else
			{
				// otherwise, add the data into fifo since device is busy
				_GPU->FifoSize++;
			}

			// make sure write is 128-bit
			if ( !Mask )
			{
				//Arg0.Lo = ((u32*)Data) [ 3 ];
				//Arg0.Hi = ((u32*)Data) [ 2 ];
				Arg0.Value = ((u64*)Data) [ 0 ];
				
				//Arg1.Lo = ((u32*)Data) [ 1 ];
				//Arg1.Hi = ((u32*)Data) [ 0 ];
				Arg1.Value = ((u64*)Data) [ 1 ];
				
				// pass the data in the correct order
				_GPU->GIF_FIFO_Execute ( Arg0.Value, Arg1.Value );
			}
			
			break;
	}

	// perform actions after write
	switch ( Address )
	{
		case GIF_CTRL:
		
			if ( Data & 1 )
			{
				// reset command written
				
				// clear the count of items in the GIF FIFO
				_GPU->GIFRegs.STAT.FQC = 0;
				
				// reset
				for ( int i = 0; i < c_iNumPaths; i++ )
				{
					_GPU->ulTransferCount [ i ] = 0;
					_GPU->ulTransferSize [ i ] = 0;
				}
			}
			
			break;
			
		case GIF_FIFO:
		
			// increment number of items in GIF FIFO
			_GPU->GIFRegs.STAT.FQC++;
			
			// assume data was transferred for now
			_GPU->GIFRegs.STAT.FQC = 0;
			
			break;
			
		case GPU_CSR:
#ifdef INLINE_DEBUG_WRITE
			debug << "; GPU_CSR";
#endif
		
			if ( _GPU->GPURegs1.CSR.RESET )
			{
#ifdef INLINE_DEBUG_WRITE
			debug << "; RESET";
#endif
				// reset GPU
				
				// clear reset flag??
				_GPU->GPURegs1.CSR.RESET = 0;
				
				// fifo is empty
				_GPU->GPURegs1.CSR.FIFO = 0x1;
			}
			
			// clear any interrupts for set bits in CSR (bits 0-4)
			_GPU->GPURegs1.CSR.Value = _GPU->GPURegs1.CSR.Value & ~( Data & 0x1f );
			
			break;
			
		default:
			break;
	}

	/*
	switch ( Address )
	{
		case GIF_CTRL:	// 0x10003000
#ifdef INLINE_DEBUG_WRITE
			debug << "; GIF_CTRL";
#endif

			// check for reset
			if ( Data & 1 )
			{
				// reset command written
				
				// clear the count of items in the GIF FIFO
				_GPU->GIF_STAT_Reg.FQC = 0;
			}

			break;
			
		case GIF_MODE:	// 0x10003010
#ifdef INLINE_DEBUG_WRITE
			debug << "; GIF_MODE";
#endif

			break;
			
		case GIF_STAT:	// 0x10003020
#ifdef INLINE_DEBUG_WRITE
			debug << "; GIF_STAT";
#endif

			break;
			
		case GIF_FIFO:	// 0x10006000
#ifdef INLINE_DEBUG_WRITE
			debug << "; GIF_FIFO";
			if ( !Mask ) debug << "; Value=" << ((u64*)Data) [ 0 ] << " " << ((u64*)Data) [ 1 ];
#endif

			// increment number of items in GIF FIFO
			_GPU->GIF_STAT_Reg.FQC++;
			
			// assume data was transferred
			_GPU->GIF_STAT_Reg.FQC = 0;

			break;
	}
	*/
	
}



void GPU::GIF_FIFO_Execute ( u64 ull0, u64 ull1 )
{
	// ***todo*** use pointers instead as input
	
	u32 ulDestReg;
	
	if ( !ulTransferCount [ CurrentPath ] )
	{
		// new packet, new primitive...
		StartPrimitive ();
		
		GIFTag0 [ CurrentPath ].Value = ull0;
		//_GPU->GIFTag0 [ CurrentPath ].Lo = ((u32*)Data) [ 3 ];
		//_GPU->GIFTag0 [ CurrentPath ].Hi = ((u32*)Data) [ 2 ];
		
		GIFTag1 [ CurrentPath ].Value = ull1;
		//_GPU->GIFTag1 [ CurrentPath ].Lo = ((u32*)Data) [ 1 ];
		//_GPU->GIFTag1 [ CurrentPath ].Hi = ((u32*)Data) [ 0 ];
	
#ifdef INLINE_DEBUG_FIFO
			debug << "; GIFTag0 [ CurrentPath ]=" << hex << GIFTag0 [ CurrentPath ].Value;
			debug << "; GIFTag1 [ CurrentPath ]=" << hex << GIFTag1 [ CurrentPath ].Value;
#endif

		// check if packed, reglist, or Image transfer
		
		// get the number of registers in list
		ulRegCount [ CurrentPath ] = 0;
		
		// get the number of transfers to expect
		ulLoopCount [ CurrentPath ] = 0;
		
		// set num regs
		ulNumRegs [ CurrentPath ] = ( ( !GIFTag0 [ CurrentPath ].REGS ) ? ( 16 ) : ( GIFTag0 [ CurrentPath ].REGS ) );
		
		// get the number of data packets in dwords (64-bit) for reglist mode, qwords (128-bit) for other modes
		switch ( GIFTag0 [ CurrentPath ].FLG )
		{
			// PACKED
			case 0:
#ifdef INLINE_DEBUG_FIFO
			debug << "; PACKED";
#endif

				// check if should store PRIM value (Packed mode only)
				if ( GIFTag0 [ CurrentPath ].PRE )
				{
#ifdef INLINE_DEBUG_FIFO
					debug << "; PRE";
					debug << " PRIM=" << hex << GIFTag0 [ CurrentPath ].PRIM;
#endif

					//GPURegsGp.PRIM.Value = GIFTag0 [ CurrentPath ].PRIM;
					WriteReg ( PRIM, GIFTag0 [ CurrentPath ].PRIM );
				}
				
				// transfer size in 64-bit units
				ulTransferSize [ CurrentPath ] = ( GIFTag0 [ CurrentPath ].NLOOP * ulNumRegs [ CurrentPath ] ) << 1;
				break;
			
			// REGLIST
			case 1:
#ifdef INLINE_DEBUG_FIFO
			debug << "; REGLIST";
#endif

				ulTransferSize [ CurrentPath ] = ( GIFTag0 [ CurrentPath ].NLOOP * ulNumRegs [ CurrentPath ] );
				break;
				
			// Image/disabled
			case 2:
#ifdef INLINE_DEBUG_FIFO
			debug << "; IMAGE/DISABLED";
#endif

				ulTransferSize [ CurrentPath ] = ( GIFTag0 [ CurrentPath ].NLOOP ) << 1;
				break;
		}
		
#ifdef INLINE_DEBUG_FIFO
			debug << "; TransferSize=" << hex << ulTransferSize [ CurrentPath ];
#endif

		// include the GIF Tag in TransferSize
		ulTransferSize [ CurrentPath ] += 2;
		
#ifdef INLINE_DEBUG_FIFO
			debug << "; TransferSize(+GIFTAG)=" << hex << ulTransferSize [ CurrentPath ];
#endif

//#ifdef INLINE_DEBUG_INVALID
//	if ( ulTransferSize [ CurrentPath ] > 10000 )
//	{
//		cout << dec << "\nulTransferSize [ CurrentPath ]>10000. Cycle#" << *_DebugCycleCount;
//		debug << dec << "\r\nPossible Error: ulTransferSize [ CurrentPath ]>10000. Cycle#" << *_DebugCycleCount;
//	}
//#endif

		
		// update transfer count
		ulTransferCount [ CurrentPath ] += 2;
	}
	else
	{
		// check transfer mode - packed, reglist, image?
		switch ( GIFTag0 [ CurrentPath ].FLG )
		{
			// PACKED
			case 0:
#ifdef INLINE_DEBUG_FIFO
			debug << "; PACKED";
			//debug << " Size=" << dec << ulTransferSize [ CurrentPath ];
			//debug << " Count=" << dec << ulTransferCount [ CurrentPath ];
			//debug << "; GIFTag0 [ CurrentPath ]=" << hex << GIFTag0 [ CurrentPath ].Value;
			//debug << "; GIFTag1 [ CurrentPath ]=" << hex << GIFTag1 [ CurrentPath ].Value;
#endif
			
				// get the register to send to
				ulDestReg = ( GIFTag1 [ CurrentPath ].Value >> ( ulRegCount [ CurrentPath ] << 2 ) ) & 0xf;
				
				// update reg count
				ulRegCount [ CurrentPath ]++;
				
				// if greater or equal to number of registers, reset
				if ( ulRegCount [ CurrentPath ] >= ulNumRegs [ CurrentPath ] ) ulRegCount [ CurrentPath ] = 0;
				
				WriteReg_Packed ( ulDestReg, ull0, ull1 );
				
				break;
			
			// REGLIST
			case 1:
#ifdef INLINE_DEBUG_FIFO
			debug << "; REGLIST";
#endif
			
				// get the register to send to
				ulDestReg = ( GIFTag1 [ CurrentPath ].Value >> ( ulRegCount [ CurrentPath ] << 2 ) ) & 0xf;
				
				// update reg count
				ulRegCount [ CurrentPath ]++;
				
				// if greater or equal to number of registers, reset
				if ( ulRegCount [ CurrentPath ] >= ulNumRegs [ CurrentPath ] ) ulRegCount [ CurrentPath ] = 0;
				
				// send value to register
				switch ( ulDestReg )
				{
					// RESERVED
					// NOP
					case 0xe:
					case 0xf:
#ifdef INLINE_DEBUG_FIFO
			debug << "; NOP";
#endif

						break;
						
					default:
#ifdef INLINE_DEBUG_FIFO
			if ( ulDestReg < c_iGPURegsGp_Count )
			{
			debug << "; " << GPURegsGp_Names [ ulDestReg ];
			}
#endif

						//GPURegsGp.Regs [ ulDestReg ] = ull0;
						WriteReg ( ulDestReg, ull0 );
						break;
				}
				
				
				// --------------------------------------------
				
				if ( ( ulTransferCount [ CurrentPath ] + 1 ) < ulTransferSize [ CurrentPath ] )
				{
					// get the register to send to
					ulDestReg = ( GIFTag1 [ CurrentPath ].Value >> ( ulRegCount [ CurrentPath ] << 2 ) ) & 0xf;
					
					// update reg count
					ulRegCount [ CurrentPath ]++;
					
					// if greater or equal to number of registers, reset
					if ( ulRegCount [ CurrentPath ] >= ulNumRegs [ CurrentPath ] ) ulRegCount [ CurrentPath ] = 0;
					
					// send value to register
					switch ( ulDestReg )
					{
						// RESERVED
						// NOP
						case 0xe:
						case 0xf:
#ifdef INLINE_DEBUG_FIFO
						debug << "; NOP";
#endif

							break;
							
						default:
#ifdef INLINE_DEBUG_FIFO
						if ( ulDestReg < c_iGPURegsGp_Count )
						{
						debug << "; " << GPURegsGp_Names [ ulDestReg ];
						}
#endif

							//GPURegsGp.Regs [ ulDestReg ] = ull0;
							WriteReg ( ulDestReg, ull1 );
							break;
					}
					
				}
				
				break;
			
			// IMAGE/disabled
			default:
#ifdef INLINE_DEBUG_IMAGE
			debug << "; IMAGE/DISABLED";
#endif
			
#ifdef INLINE_DEBUG_XFER
	debug << dec << " DOff32=" << XferDstOffset32 << " X=" << XferX << " Y=" << XferY << " DX=" << XferDstX << " DY=" << XferDstY << " XferWidth=" << XferWidth << " XferHeight=" << XferHeight << " BufWidth=" << XferDstBufWidth;
#endif

				// send data to destination
				TransferDataIn32 ( (u32*) ( &ull0 ), 2 );
				TransferDataIn32 ( (u32*) ( &ull1 ), 2 );
				
				break;
		}
		
		// update transfer count
		ulTransferCount [ CurrentPath ] += 2;
	}
	
	// if greater than number of items to transfer, then done
	if ( ulTransferCount [ CurrentPath ] >= ulTransferSize [ CurrentPath ] )
	{
		ulTransferCount [ CurrentPath ] = 0;
		ulTransferSize [ CurrentPath ] = 0;
		
		// this is the end of the tag
		Tag_Done = 1;
		
		// check if path1 transfer is complete (End of packet)
		if ( GIFTag0 [ CurrentPath ].EOP ) EndOfPacket = 1;
	}
}


void GPU::WriteReg_Packed ( u32 lIndex, u64 ull0, u64 ull1 )	//u64* pValue )
{
	u64 Data;
	
	// make sure index is within range
	if ( lIndex >= c_iGPURegsGp_Count )
	{
#ifdef INLINE_DEBUG_INVALID
	debug << dec << "\r\nWriteReg index outside range. lIndex=" << lIndex << " Cycle#" << *_DebugCycleCount;
#endif

		cout << "\nhps2x64 ALERT: GPU: WriteReg index outside range.\n";
		return;
	}
	
	switch ( lIndex )
	{
		// PRIM
		case 0:
			Data = ull0;
			
			//WriteReg ( 0, pValue [ 0 ] );
			//WriteReg ( 0, ull0 );
			break;
			
		// RGBAQ
		case 1:
			//Data = ( pValue [ 0 ] & 0xff ) | ( ( pValue [ 0 ] >> 32 ) & 0xff ) | ( ( pValue [ 1 ] & 0xff ) << 16 ) | ( ( pValue [ 1 ] >> 8 ) & 0xff000000 );
			Data = ( ull0 & 0xff ) | ( ( ull0 >> 32 ) & 0xff ) | ( ( ull1 & 0xff ) << 16 ) | ( ( ull1 >> 8 ) & 0xff000000 );
			
			// ***TODO*** add in Q value
			Data |= ( Internal_Q << 32 );
			
			//WriteReg ( 1, Data );
			
			break;
			
		// ST
		case 2:
			//Data = pValue [ 0 ];
			Data = ull0;
			
			// save Q value
			//Internal_Q = pValue [ 1 ];
			Internal_Q = ull1;
			
			//WriteReg ( 2, Data );
			
			break;
			
		// UV
		case 3:
			//Data = ( pValue [ 0 ] & 0x3fff ) | ( ( pValue [ 0 ] >> 16 ) & 0x3fff0000 );
			Data = ( ull0 & 0x3fff ) | ( ( ull0 >> 16 ) & 0x3fff0000 );
			
			//WriteReg ( 3, Data );
			break;
			
		// XYZF2
		case 4:
			//Data = ( pValue [ 0 ] & 0xffff ) | ( ( pValue [ 0 ] >> 16 ) & 0xffff0000 ) | ( ( ( pValue [ 1 ] >> 4 ) & 0xffffff ) << 32 ) | ( ( ( pValue [ 1 ] >> 36 ) & 0xff ) << 56 );
			Data = ( ull0 & 0xffff ) | ( ( ull0 >> 16 ) & 0xffff0000 ) | ( ( ( ull1 >> 4 ) & 0xffffff ) << 32 ) | ( ( ( ull1 >> 36 ) & 0xff ) << 56 );
			
			if ( ( ull1 >> 47 ) & 1 )
			{
				lIndex += 8;
				//WriteReg ( 0xc, Data );
			}
			else
			{
				//WriteReg ( 0x4, Data );
			}
			
			break;
			
		// XYZ2
		case 5:
			//Data = ( pValue [ 0 ] & 0xffff ) | ( ( pValue [ 0 ] >> 16 ) & 0xffff0000 ) | ( ( ( pValue [ 1 ] ) & 0xffffffff ) << 32 );
			Data = ( ull0 & 0xffff ) | ( ( ull0 >> 16 ) & 0xffff0000 ) | ( ( ( ull1 ) & 0xffffffff ) << 32 );
			
			if ( ( ull1 >> 47 ) & 1 )
			{
				lIndex += 8;
				//WriteReg ( 0xd, Data );
			}
			else
			{
				//WriteReg ( 0x5, Data );
			}
			
			break;
			
		// FOG (F)
		case 0xa:
			//Data = ( ( ( pValue [ 1 ] >> 36 ) & 0xff ) << 56 );
			Data = ( ( ( ull1 >> 36 ) & 0xff ) << 56 );
			
			//WriteReg ( 0xa, Data );
			break;
			
		case 0xe:
			//Data = pValue [ 0 ];
			Data = ull0;
			
			//WriteReg ( ull1 & 0xff, ull0 );
			lIndex = ull1 & 0xff;
			break;
			
		// NOP ??
		case 0xf:
			return;
			break;
			
		// write ull0 to the register specified
		default:
			/*
#ifdef INLINE_DEBUG_INVALID
	debug << dec << "\r\nWriteReg_Packed index outside range. lIndex=" << lIndex << " Cycle#" << *_DebugCycleCount;
#endif

			cout << "\nhps2x64: ALERT: WriteReg_Packed encountered invalid REG number.";
			*/
			
			Data = ull0;
			break;
	}
	
#ifdef INLINE_DEBUG_FIFO
			if ( lIndex < c_iGPURegsGp_Count )
			{
			debug << "; " << GPURegsGp_Names [ lIndex ];
			}
#endif

	// write the value to register
	WriteReg ( lIndex, Data );
}


void GPU::WriteReg ( u32 lIndex, u64 Value )
{
	// make sure index is within range
	if ( lIndex >= c_iGPURegsGp_Count )
	{
#ifdef INLINE_DEBUG_INVALID
	debug << dec << "\r\nWriteReg index outside range. lIndex=" << lIndex << " Cycle#" << *_DebugCycleCount;
#endif

		cout << "\nhps2x64 ALERT: GPU: WriteReg index outside range.\n";
		return;
	}
	
	GPURegsGp.Regs [ lIndex ] = Value;
	
	// certain registers require an action be performed
	switch ( lIndex )
	{
		// graphics drawing //
		
		case XYZ2:
			// sets value with vertex kick and possible drawing kick
			
			// x -> bits 0-15 -> 12.4 fixed point
			//SetX ( Value & 0xffff );
			// y -> bits 16-31 -> 12.4 fixed point
			//SetY ( ( Value >> 16 ) & 0xffff );
			// z -> bits 32-63
			//SetZ ( ( Value >> 32 ) );
			SetXYZ ( Value );
			
			// vertex kick
			VertexKick ();
			
			// drawing kick depends on the primitive -> I'll let drawing kick handle that...
			DrawingKick ();
			break;
			
		case XYZF2:
			// sets value with vertex kick and possible drawing kick
			
			// x -> bits 0-15 -> 12.4 fixed point
			//SetX ( Value & 0xffff );
			// y -> bits 16-31 -> 12.4 fixed point
			//SetY ( ( Value >> 16 ) & 0xffff );
			// z -> bits 32-55
			//SetZ ( ( Value >> 32 ) & 0xffffff );
			// f -> bits 56-63
			//SetF ( ( Value >> 56 ) & 0xff );
			SetXYZ ( Value );
			SetF ( Value );
			
			// vertex kick
			VertexKick ();
			
			// drawing kick depends on the primitive -> I'll let drawing kick handle that...
			DrawingKick ();
			break;
			
		case XYZ3:
			// sets value with vertex kick, but no drawing kick
			
			// x -> bits 0-15 -> 12.4 fixed point
			//SetX ( Value & 0xffff );
			// y -> bits 16-31 -> 12.4 fixed point
			//SetY ( ( Value >> 16 ) & 0xffff );
			// z -> bits 32-63
			//SetZ ( ( Value >> 32 ) );
			SetXYZ ( Value );
			
			// vertex kick
			VertexKick ();
			break;
			
		case XYZF3:
			// sets value with vertex kick, but no drawing kick
			
			// x -> bits 0-15 -> 12.4 fixed point
			//SetX ( Value & 0xffff );
			// y -> bits 16-31 -> 12.4 fixed point
			//SetY ( ( Value >> 16 ) & 0xffff );
			// z -> bits 32-55
			//SetZ ( ( Value >> 32 ) & 0xffffff );
			// f -> bits 56-63
			//SetF ( ( Value >> 56 ) & 0xff );
			SetXYZ ( Value );
			SetF ( Value );
			
			// vertex kick
			VertexKick ();
			break;
			
		case RGBAQ:
		
			// r -> bits 0-7
			//SetR ( Value & 0xff );
			// g -> bits 8-15
			//SetG ( ( Value >> 8 ) & 0xff );
			// b -> bits 16-23
			//SetB ( ( Value >> 16 ) & 0xff );
			// a -> bits 24-31
			//SetA ( ( Value >> 24 ) & 0xff );
			// q -> bits 32-63
			//SetQ ( ( Value >> 32 ) );
			SetRGBAQ ( Value );
			
			break;
			
		case UV:
			// sets the uv coords for vertex
			
			// u -> bits 0-13
			//SetU ( Value & 0x3fff );
			// v -> bits 16-29
			//SetV ( ( Value >> 16 ) & 0x3fff );
			SetUV ( Value );
			
			break;
			
		case ST:
		
			// s -> bits 0-31
			//SetS ( Value );
			// t -> bits 32-63
			//SetT ( ( Value >> 32 ) );
			SetST ( Value );
			
			break;
			
		case FOG:
		
			// f -> bits 56-63
			//SetF ( ( Value >> 56 ) & 0xff );
			SetF ( Value );
			break;
			
			
		// data transfer //
			
		case BITBLTBUF:
		
			// get source buffer offset (SBP : word address/64)
			XferSrcOffset32 = GPURegsGp.BITBLTBUF.SBP << 6;
			
			// get source buffer width (in pixels) (SBW : number of pixels/64)
			XferSrcBufWidth = GPURegsGp.BITBLTBUF.SBW << 6;
			
			// ***todo*** get source pixel format
			// pretend it is 4 bytes per pixel for now
			//XferSrcPixelSize = 4;
			
			// get dest buffer offset (DBP : word address/64)
			XferDstOffset32 = GPURegsGp.BITBLTBUF.DBP << 6;
			
			// get dest buffer width (in pixels) (DBW : pixels/64)
			XferDstBufWidth = GPURegsGp.BITBLTBUF.DBW << 6;
			
			// ***todo*** get dest pixel format
			//XferDstPixelSize = 4;
			
			break;
			
		case TRXPOS:
			
			// transfer x,y for source
			XferSrcX = GPURegsGp.TRXPOS.SSAX;
			XferSrcY = GPURegsGp.TRXPOS.SSAY;
			
			// set current transfer x position
			XferX = 0;
			
			// transfer x,y for dest
			XferDstX = GPURegsGp.TRXPOS.DSAX;
			XferDstY = GPURegsGp.TRXPOS.DSAY;
			
			// set current transfer y position
			XferY = 0;
			
			// set pixel shift and count for 24-bit pixels
			PixelShift = 0;
			PixelCount = 0;
			
			// ***todo*** get transfer method/direction
			// transmission method only applies to Local->Local data transfer
			
			break;
			
		case TRXREG:
		
			// get transfer width (in pixels)
			XferWidth = GPURegsGp.TRXREG.RRW;
			
			// get transfer height (in pixels)
			XferHeight = GPURegsGp.TRXREG.RRH;
			
			break;
			
		case TRXDIR:
		
			// ***todo*** get transfer direction, and start transfer immediately if GPU->GPU transfer
			if ( ( Value & 3 ) == 2 )
			{
				cout << "\nhps2x64: ALERT: GPU: Local->Local transfer started!!!\n";
			}
			
			break;
			
		default:
			break;
	}
}


void GPU::DrawingKick ()
{
	// array for vertex index before drawing kick for each primitive
	static const u32 c_ulPrimDrawKick [] = { 1, 2, 2, 3, 3, 3, 2, -1 };
	
#ifdef INLINE_DEBUG_DRAWKICK
	debug << "\r\nDrawKick: " << hex << " VertexCount=" << lVertexCount << " PRIM=" << GPURegsGp.PRIM.Value << " Needed=" << c_ulPrimDrawKick [ GPURegsGp.PRIM.Value & 0x7 ];
#endif

	if ( lVertexCount >= c_ulPrimDrawKick [ GPURegsGp.PRIM.Value & 0x7 ] )
	{
		DrawPrimitive ();
	}
}


void GPU::DrawPrimitive ()
{
	static const char* c_sPrimNames [] = { "Point", "Line", "Line Strip", "Triangle", "Triangle Strip", "Triangle Fan", "Sprite", "Reserved" };

	// draw the primitive
	
#ifdef INLINE_DEBUG_PRIMITIVE
	debug << "\r\nDrawPrimitive: " << c_sPrimNames [ GPURegsGp.PRIM.Value & 0x7 ];
#endif

	// update the primitive count (for debugging)
	Primitive_Count++;

	switch ( GPURegsGp.PRIM.Value & 0x7 )
	{
		// Point
		case 0:
			DrawPoint ();
			break;
			
		// Line
		case 1:
			DrawLine ();
			break;
		
		// Line Strip
		case 2:
		
			// only need the last two coords for line strip
			lVertexQ_Index &= 1;
			
			DrawLine ();
			break;
		
		// Triangle
		case 3:
			DrawTriangle ();
			break;
		
		// Triangle Strip
		case 4:
		
			// only need the last three coords for the triangle in strip
			if ( lVertexQ_Index > 2 ) lVertexQ_Index = 0;
			
			DrawTriangle ();
			break;
			
		// Triangle Fan
		case 5:
		
			// only need the first coord and the last two for triangle fan
			if ( lVertexQ_Index > 2 ) lVertexQ_Index = 1;
			
			DrawTriangleFan ();
			break;
			
		// Sprite
		case 6:
			DrawSprite ();
			break;
			
		// Reserved
		case 7:
#ifdef INLINE_DEBUG_PRIMITIVE
	debug << "\r\nALERT: Reserved Primitive";
#endif

			cout << "hps2x64 ALERT: GPU: Trying to draw reserved primitive.";
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
	
	//return ( ( lScanline & ~1 ) == lVBlank );
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
	return (u64) ( ( (double) ( *_DebugCycleCount ) ) / dCyclesPerScanline );
}


u64 GPU::GetScanline_Number ()
{
	u64 Scanline_Number;
	
	// *** divide ***
	//return ( ( (u64) ( ( *_DebugCycleCount ) / dCyclesPerScanline ) ) % ( (u64) lMaxScanline ) );
	Scanline_Number = ( ( (u64) ( ( *_DebugCycleCount ) / dCyclesPerScanline ) ) % ( (u64) lMaxScanline ) );
	
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


double GPU::GetScanline_Start ()
{
#ifdef USE_DIVIDE_GCC
	// *** divide ***
	return ( ( (double) ( *_DebugCycleCount ) ) / dCyclesPerScanline );
#else
	return ( ( (double) ( *_DebugCycleCount ) ) * dScanlinesPerCycle );
#endif
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
	if ( HBlank_X != HBlank_X_LUT [ 0 ] ||
		lVBlank != VBlank_Y_LUT [ 0 ] ||
		Raster_XMax != Raster_XMax_LUT [ 0 ] [ 0 ] ||
		lMaxScanline != Raster_YMax_LUT [ 0 ] )
	{
#ifdef INLINE_DEBUG_VARS
	debug << "\r\nChange; StartCycle=" << dec << *_DebugCycleCount;
#endif

		// ***TODO*** need to update timers before clearing the pixel counts //
		
		// update timers using settings from before they change
		// *todo* really only need to update timers if they are pulling signal from hblank
		Timers::_TIMERS->UpdateTimer ( 0 );
		Timers::_TIMERS->UpdateTimer ( 1 );
		Timers::_TIMERS->UpdateTimer ( 2 );
		Timers::_TIMERS->UpdateTimer ( 3 );
		
		// at end of routine, calibrate timers

		//RasterChange_StartCycle = *_DebugCycleCount;
		SettingsChange = true;
	}


	HBlank_X = HBlank_X_LUT [ 0 ];

#ifdef INLINE_DEBUG_VARS
	debug << "; HBlank_X = " << dec << HBlank_X;
#endif

	lVBlank = VBlank_Y_LUT [ 0 ];

#ifdef INLINE_DEBUG_VARS
	debug << "; lVBlank = " << lVBlank;
#endif

	Raster_XMax = Raster_XMax_LUT [ 0 ] [ 0 ];

#ifdef INLINE_DEBUG_VARS
	debug << "; Raster_XMax = " << Raster_XMax;
#endif

	lMaxScanline = Raster_YMax_LUT [ 0 ];

#ifdef INLINE_DEBUG_VARS
	debug << "; lMaxScanline = " << lMaxScanline;
#endif

	CyclesPerPixel_INC = CyclesPerPixel_INC_Lookup [ 0 ] [ 0 ];
	dCyclesPerPixel = CyclesPerPixel_Lookup [ 0 ] [ 0 ];
	dPixelsPerCycle = PixelsPerCycle_Lookup [ 0 ] [ 0 ];
	
	// check if ntsc or pal
	if ( 1 )
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
	
	// the Display_Height is the lVBlank if interlaced, otherwise it is lVBlank/2
	if ( 0 /*GPU_CTRL_Read.ISINTER*/ )
	{
		Display_Height = lVBlank;
	}
	else
	{
		Display_Height = ( lVBlank >> 1 );
	}
	

	// check if the settings changed
	if ( SettingsChange )
	{
		// settings changed //
		
		// get the next event for gpu if settings change
		GetNextEvent ();
		
		// calibrate timers 0,1,2
		// *todo* really only need to update timers if they are pulling signal from hblank
		Timers::_TIMERS->CalibrateTimer ( 0 );
		Timers::_TIMERS->CalibrateTimer ( 1 );
		Timers::_TIMERS->CalibrateTimer ( 2 );
		Timers::_TIMERS->CalibrateTimer ( 3 );
		
		// if doing calibrate, then also must do update of next event
		// *todo* really only need to update timers if they are pulling signal from hblank
		Timers::_TIMERS->Get_NextEvent ( 0 );
		Timers::_TIMERS->Get_NextEvent ( 1 );
		Timers::_TIMERS->Get_NextEvent ( 2 );
		Timers::_TIMERS->Get_NextEvent ( 3 );
	}

#ifdef INLINE_DEBUG_VARS
	debug << "->UpdateRaster_VARS";
#endif
}


void GPU::Draw_Screen ()
{
	static const int c_iVisibleArea_StartY [] = { c_iVisibleArea_StartY_Pixel_NTSC, c_iVisibleArea_StartY_Pixel_PAL };
	static const int c_iVisibleArea_EndY [] = { c_iVisibleArea_EndY_Pixel_NTSC, c_iVisibleArea_EndY_Pixel_PAL };
	
	
#ifdef INLINE_DEBUG_DRAW_SCREEN
	debug << "\r\n***Frame Draw***";
	debug << " FBOffset/2048=" << hex << GPURegs0.DISPFB2.FBP;
	debug << " FBWidth/64 (for screen output/in pixels)=" << dec << GPURegs0.DISPFB2.FBW;
	debug << " FBHeight-1 (for screen output/in pixels)=" << dec << GPURegs0.DISPLAY2.DH;
	debug << " XPixel1=" << dec << GPURegs0.DISPFB1.DBX;
	debug << " YPixel1=" << dec << GPURegs0.DISPFB1.DBY;
	debug << " XPixel2=" << dec << GPURegs0.DISPFB2.DBX;
	debug << " YPixel2=" << dec << GPURegs0.DISPFB2.DBY;
	debug << " PixelFormat=" << PixelFormat_Names [ GPURegs0.DISPFB2.PSM ];
#endif
	
	u32* buf_ptr;
	u32 *buf_ptr32;
	u16 *buf_ptr16;
	s32 draw_buffer_offset;
	s32 draw_width, draw_height;
	s32 start_x, start_y;
	s32 Index = 0;
	
	u32 Pixel16, Pixel32;
	
	draw_buffer_offset = GPURegs0.DISPFB2.FBP << 11;
	
	buf_ptr = & ( RAM32 [ GPURegs0.DISPFB2.FBP >> 4 ] );
	
	draw_width = GPURegs0.DISPFB2.FBW << 6;
	draw_height = GPURegs0.DISPLAY2.DH + 1;
	start_x = GPURegs0.DISPFB2.DBX;
	start_y = GPURegs0.DISPFB2.DBY;
	
	// draw starting from correct position
	//buf_ptr = buf_ptr [ start_x + ( start_y * draw_width ) ];
	
	if ( !GPURegs0.DISPFB2.PSM || GPURegs0.DISPFB2.PSM == 1 )
	{
		// 24/32-bit pixels in frame buffer //
		
		buf_ptr32 = & ( RAM32 [ GPURegs0.DISPFB2.FBP >> 4 ] );
		
		// copy the pixels into pixel buffer
		for ( int y = start_y + ( draw_height - 1 ); y >= start_y; y-- )
		{
			for ( int x = start_x; x < ( start_x + draw_width ); x++ )
			{
				PixelBuffer [ Index++ ] = buf_ptr32 [ x + ( y * draw_width ) ];
			}
		}
	}
	else
	{
		// assume 16-bit pixels for frame buffer for now //
		
		buf_ptr16 = (u16*) ( & ( RAM32 [ GPURegs0.DISPFB2.FBP >> 4 ] ) );
		
		for ( int y = start_y + ( draw_height - 1 ); y >= start_y; y-- )
		{
			for ( int x = start_x; x < ( start_x + draw_width ); x++ )
			{
				Pixel16 = buf_ptr16 [ x + ( y * draw_width ) ];
				Pixel32 = ( ( Pixel16 & 0x1f ) << 3 ) | ( ( ( Pixel16 >> 5 ) & 0x1f ) << ( 8 + 3 ) ) | ( ( ( Pixel16 >> 10 ) & 0x1f ) << ( 16 + 3 ) );
				PixelBuffer [ Index++ ] = Pixel32;
			}
		}
	}
	
	/*
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
					*ptr_pixelbuffer64++ = ( ( Pixel64 & 0x1f0000001fLL ) << 3 ) | ( ( Pixel64 & 0x3e0000003e0LL ) << 6 ) | ( ( Pixel64 & 0x7c0000007c00LL ) << 9 );
				}
				for ( ; current_x < current_xmax; current_x++ )
				{
					Pixel16 = *ptr_vram++;
					Pixel32_0 = ( ( Pixel16 & 0x1f ) << 3 ) | ( ( Pixel16 & 0x3e0 ) << 6 ) | ( ( Pixel16 & 0x7c00 ) << 9 );
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
	*/
		
	// *** output of pixel buffer to screen *** //

	// make this the current window we are drawing to
	DisplayOutput_Window->OpenGL_MakeCurrentWindow ();
	
	//glPixelZoom ( (float)MainProgramWindow_Width / (float)VisibleArea_Width, (float)MainProgramWindow_Height / (float)VisibleArea_Height );
	glPixelZoom ( (float)MainProgramWindow_Width / (float)draw_width, (float)MainProgramWindow_Height / (float)draw_height );
	//glDrawPixels ( VisibleArea_Width, VisibleArea_Height, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid*) PixelBuffer );
	glDrawPixels ( draw_width, draw_height, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid*) PixelBuffer );
	
	// update screen
	DisplayOutput_Window->FlipScreen ();
	
	// this is no longer the current window we are drawing to
	DisplayOutput_Window->OpenGL_ReleaseWindow ();
	
	
}


void GPU::Copy_Buffer ( u32* dstbuf, u32* srcbuf, u32 dstbuffer_width, u32 dstbuffer_height, u32 srcbuffer_width, u32 srcbuffer_height )
{
	
	u32 Pixel, FramePixel;
	s32 Pixel_X, Pixel_Y;
	
	u32 *output_buf;
	
	//srcbuf = & ( RAM32 [ FrameBufferStartOffset32 ] );

	
	FramePixel = 0;
	
	/////////////////////////////////////////////////////////////////
	// Draw contents of frame buffer
	//for ( Pixel_Y = FrameBuffer_Height - 1; Pixel_Y >= 0; Pixel_Y-- )
	for ( Pixel_Y = srcbuffer_height - 1; Pixel_Y >= 0; Pixel_Y-- )
	{
		output_buf = & ( dstbuf [ Pixel_Y * dstbuffer_width ] );
		
		if ( Pixel_Y < dstbuffer_height && Pixel_Y < srcbuffer_height )
		{
			//for ( Pixel_X = 0; Pixel_X < FrameBuffer_Width; Pixel_X++ )
			for ( Pixel_X = 0; ( Pixel_X < dstbuffer_width ) && ( Pixel_X < srcbuffer_width ); Pixel_X++ )
			{
				//Pixel = VRAM [ Pixel_X + ( Pixel_Y << 10 ) ];
				Pixel = srcbuf [ Pixel_X + ( Pixel_Y * srcbuffer_width ) ];
				
				//PixelBuffer [ FramePixel++ ] = ( ( Pixel & 0x1f ) << ( 3 ) ) | ( ( (Pixel >> 5) & 0x1f ) << ( 3 + 8 ) ) | ( ( (Pixel >> 10) & 0x1f ) << ( 3 + 16 ) );
				*output_buf++ = Pixel & 0xffffff;
			}
		}
	}
	
	// make this the current window we are drawing to
	//FrameBuffer_DebugWindow->OpenGL_MakeCurrentWindow ();
	
	//glPixelZoom ( (float)MainProgramWindow_Width / (float)DrawWidth, (float)MainProgramWindow_Height / (float)DrawHeight );
	//glDrawPixels ( FrameBuffer_Width, FrameBuffer_Height, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid*) PixelBuffer );

	// update screen
	//FrameBuffer_DebugWindow->FlipScreen ();
	
	// this is no longer the current window we are drawing to
	//FrameBuffer_DebugWindow->OpenGL_ReleaseWindow ();
	
}


void GPU::Draw_FrameBuffers ()
{
	// frame buffer data
	FRAME_t *Frame = &GPURegsGp.FRAME_1;
	
	Copy_Buffer ( & ( PixelBuffer [ 0 ] ), & ( RAM32 [ Frame [ 0 ].FBP << 11 ] ), c_iFrameBuffer_DisplayWidth, c_iFrameBuffer_DisplayHeight, Frame [ 0 ].FBW << 6, 480 );
	Copy_Buffer ( & ( PixelBuffer [ 480 * c_iFrameBuffer_DisplayWidth ] ), & ( RAM32 [ Frame [ 1 ].FBP << 11 ] ), c_iFrameBuffer_DisplayWidth, c_iFrameBuffer_DisplayHeight, Frame [ 1 ].FBW << 6, 480 );
	
	// make this the current window we are drawing to
	FrameBuffer_DebugWindow->OpenGL_MakeCurrentWindow ();
	
	//glPixelZoom ( (float)MainProgramWindow_Width / (float)DrawWidth, (float)MainProgramWindow_Height / (float)DrawHeight );
	glDrawPixels ( c_iFrameBuffer_DisplayWidth, c_iFrameBuffer_DisplayHeight, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid*) PixelBuffer );

	// update screen
	FrameBuffer_DebugWindow->FlipScreen ();
	
	// this is no longer the current window we are drawing to
	FrameBuffer_DebugWindow->OpenGL_ReleaseWindow ();
}





void GPU::DMA_Read ( u32* Data, int ByteReadCount )
{
#ifdef INLINE_DEBUG_DMA_READ
	debug << "\r\n\r\nDMA_Read " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; Data = " << hex << Data [ 0 ];
#endif

}




void GPU::DMA_Write ( u32* Data, int ByteWriteCount )
{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "\r\n\r\nDMA_Write " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; Data = " << hex << Data [ 0 ];
#endif

}


static u32 GPU::DMA_WriteBlock ( u64* Data, u32 QuadwordCount )
{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "\r\n\r\nDMA_WriteBlock " << hex << setw ( 8 ) << *_DebugPC << " QWC=" << QuadwordCount << " " << dec << *_DebugCycleCount << hex << "; Data= ";
	//for ( int i = 0; i < ( QuadwordCount * 2 ); i++ ) debug << " " << Data [ i ];
	//debug << "\r\n";
#endif

	// this would be path 3
	_GPU->CurrentPath = 3;

	for ( int i = 0; i < QuadwordCount; i++ )
	{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "\r\n";
	debug << hex << Data [ 0 ] << " " << Data [ 1 ];
#endif

		_GPU->GIF_FIFO_Execute ( Data [ 0 ], Data [ 1 ] );
		
		Data += 2;
	}
	
	// for now, trigger signals
	// ***TODO*** add correct event triggering
	//_GPU->GPURegs1.CSR.Value |= 0xf;
	
	// for now: check for GPU interrupt
	//if ( _GPU->GPURegs1.CSR.Value & ( _GPU->GPURegs1.IMR >> 8 ) )
	//{
//#ifdef INLINE_DEBUG_DMA_WRITE
//	debug << "; GSINT";
//#endif
	//
	//	SetInterrupt ();
	//}
	
	// return the amount of data written
	return QuadwordCount;
}


static u32 GPU::DMA_ReadBlock ( u64* Data, u32 QuadwordCount )
{
#ifdef INLINE_DEBUG_DMA_READ
	debug << "\r\n\r\nDMA_ReadBlock " << hex << setw ( 8 ) << *_DebugPC << " QWC=" << QuadwordCount << " " << dec << *_DebugCycleCount << hex << "; Data= ";
	debug << "\r\n";
#endif

}


// xgkick
static void GPU::Path1_WriteBlock ( u64* Data )
{
#ifdef INLINE_DEBUG_PATH1_WRITE
	debug << "\r\n\r\nPATH1_WriteBlock " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << hex << "; Data= ";
	debug << " " << Data [ 0 ] << " " << Data [ 1 ];
#endif

	// for path1, need to keep reading data until end of packet reached
	_GPU->CurrentPath = 1;
	
	// if device not busy, then clear fifo size
	if ( *_DebugCycleCount < _GPU->BusyUntil_Cycle )
	{
		_GPU->FifoSize = 0;
	}
	else
	{
		// otherwise, add the data into fifo since device is busy
		_GPU->FifoSize++;
	}
	
	// path1 is not done since it is just starting
	_GPU->Tag_Done = 0;

	// loop while path1 is not done
	while ( !_GPU->Tag_Done )
	{
#ifdef INLINE_DEBUG_PATH1_WRITE
	debug << "\r\n";
	debug << hex << Data [ 0 ] << " " << Data [ 1 ];
#endif

		_GPU->GIF_FIFO_Execute ( Data [ 0 ], Data [ 1 ] );
		Data += 2;
	}
	
	// testing INT
	//SetInterrupt ();
}

// vif1
static void GPU::Path2_WriteBlock ( u64* Data, u32 QuadwordCount )
{
#ifdef INLINE_DEBUG_PATH2_WRITE
	debug << "\r\n\r\nPATH2_WriteBlock " << hex << setw ( 8 ) << *_DebugPC << " QWC=" << QuadwordCount << " " << dec << *_DebugCycleCount << hex << "; Data= ";
	//for ( int i = 0; i < ( QuadwordCount * 2 ); i++ ) debug << " " << Data [ i ];
	//debug << "\r\n";
#endif

	_GPU->CurrentPath = 2;

	// if device not busy, then clear fifo size
	if ( *_DebugCycleCount < _GPU->BusyUntil_Cycle )
	{
		_GPU->FifoSize = 0;
	}
	else
	{
		// otherwise, add the data into fifo since device is busy
		_GPU->FifoSize++;
	}
	
	for ( int i = 0; i < QuadwordCount; i++ )
	{
#ifdef INLINE_DEBUG_PATH2_WRITE
	debug << "\r\n";
	debug << hex << Data [ 0 ] << " " << Data [ 1 ];
#endif

		_GPU->GIF_FIFO_Execute ( Data [ 0 ], Data [ 1 ] );
		
		Data += 2;
	}
	
	// testing INT
	//SetInterrupt ();
}


// vif1
static void GPU::Path2_ReadBlock ( u64* Data, u32 QuadwordCount )
{
}



void GPU::SetDrawVars ()
{
#if defined INLINE_DEBUG_SETDRAWVARS || defined INLINE_DEBUG_PRIMITIVE
	debug << "; SetDrawVars";
#endif

	// scissor data
	SCISSOR_t *Scissor = &GPURegsGp.SCISSOR_1;
	
	// frame buffer data
	FRAME_t *Frame = &GPURegsGp.FRAME_1;
	
	// z-buffer data
	ZBUF_t *ZBuf = &GPURegsGp.ZBUF_1;
	
	XYOFFSET_t *Offset = &GPURegsGp.XYOFFSET_1;
	
	// get context
	if ( GPURegsGp.PRMODECONT & 1 )
	{
		// attributes in PRIM //
		Ctx = GPURegsGp.PRIM.CTXT;
		TextureMapped = GPURegsGp.PRIM.TME;
		Gradient = GPURegsGp.PRIM.IIP;
	}
	else
	{
		// attributes in PRMODE //
		Ctx = GPURegsGp.PRMODE.CTXT;
		TextureMapped = GPURegsGp.PRMODE.TME;
		Gradient = GPURegsGp.PRMODE.IIP;
	}
	
#if defined INLINE_DEBUG_SETDRAWVARS || defined INLINE_DEBUG_PRIMITIVE
	debug << " Context#" << Ctx << " Texture=" << TextureMapped;
#endif

	// get window for context
	Window_XLeft = Scissor [ Ctx ].SCAX0;
	Window_XRight = Scissor [ Ctx ].SCAX1;
	Window_YTop = Scissor [ Ctx ].SCAY0;
	Window_YBottom = Scissor [ Ctx ].SCAY1;
	
#if defined INLINE_DEBUG_SETDRAWVARS || defined INLINE_DEBUG_PRIMITIVE
	debug << dec << " Window: XLeft=" << Window_XLeft << " XRight=" << Window_XRight << " YTop=" << Window_YTop << " YBottom=" << Window_YBottom;
#endif

	// get x,y offset for context (12.4 fixed point format)
	Coord_OffsetX = Offset [ Ctx ].OFX;
	Coord_OffsetY = Offset [ Ctx ].OFY;
	
#if defined INLINE_DEBUG_SETDRAWVARS || defined INLINE_DEBUG_PRIMITIVE
	debug << dec << " Offset: OFX=" << dec << ( Coord_OffsetX >> 4 ) << " OFY=" << ( Coord_OffsetY >> 4 );
	//debug << dec << " (hex)Offset: OFX=" << hex << Coord_OffsetX << " OFY=" << Coord_OffsetY;
#endif

	// get frame buffer info for context
	FrameBufferStartOffset32 = Frame [ Ctx ].FBP << 11;
	FrameBufferWidth_Pixels = Frame [ Ctx ].FBW << 6;
	
	FrameBuffer_PixelFormat = Frame [ Ctx ].SPSM;
	
#if defined INLINE_DEBUG_SETDRAWVARS || defined INLINE_DEBUG_PRIMITIVE
	debug << dec << " FrameBufferStartOffset32=" << FrameBufferStartOffset32 << " FrameBufferWidth_Pixels=" << FrameBufferWidth_Pixels;
#endif

}


void GPU::DrawPoint ()
{
#if defined INLINE_DEBUG_POINT || defined INLINE_DEBUG_PRIMITIVE
	debug << "; RenderPoint";
#endif

	// render in 8bits per pixel, then convert down when done
	
	static const int c_iVectorSize = 1;

	u32 NumberOfPixelsDrawn;
	
	// if writing 16-bit pixels, then this could change to a u16 pointer
	u32 *ptr, *buf;
	
	u32 DestPixel, bgr, bgr_temp;
	
	// 12.4 fixed point
	s32 x0, y0;
	
	// get frame buffer pointer
	buf = & ( RAM32 [ FrameBufferStartOffset32 ] );
	
	// get x,y
	x0 = xyz [ 0 ].X;
	y0 = xyz [ 0 ].Y;
	
	// get fill color
	bgr = rgbaq [ 0 ].Value & 0xffffffffL;
	
#if defined INLINE_DEBUG_POINT || defined INLINE_DEBUG_PRIMITIVE
	debug << hex << " bgr=" << bgr;
	debug << dec << "; Coords: x0=" << ( x0 >> 4 ) << " y0=" << ( y0 >> 4 );
#endif

	//////////////////////////////////////////
	// get coordinates on screen
	// *note* this is different from PS1, where you would add the offsets..
	x0 -= Coord_OffsetX;
	y0 -= Coord_OffsetY;
	
	// coords are in 12.4 fixed point
	x0 >>= 4;
	y0 >>= 4;
	
#if defined INLINE_DEBUG_POINT || defined INLINE_DEBUG_PRIMITIVE
	debug << dec << "; FinalCoords: x0=" << x0 << " y0=" << y0;
#endif


	// check if sprite is within draw area
	if ( x0 < Window_XLeft || x0 > Window_XRight || y0 < Window_YTop || y0 > Window_YBottom ) return;
#if defined INLINE_DEBUG_POINT_TEST || defined INLINE_DEBUG_PRIMITIVE_TEST
	else
	{
		debug << dec << "\r\nDrawPoint" << " FinalCoords: x0=" << x0 << " y0=" << y0;
		debug << " FrameBufPixFmt=" << PixelFormat_Names [ FrameBuffer_PixelFormat ];
	}
#endif
	
	// for points, already did all the needed window checks at this point //
	
	NumberOfPixelsDrawn = 1;
	
		ptr = & ( buf [ x0 + ( y0 * FrameBufferWidth_Pixels ) ] );
		
			// read pixel from frame buffer if we need to check mask bit
			// ***todo*** PS2 pixel mask
			//DestPixel = *ptr;
			
			//bgr_temp = bgr;

			// semi-transparency
			// ***todo*** PS2 semi-transparency
			//if ( command_abe )
			//{
			//	bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read.ABR );
			//}
			
			// check if we should set mask bit when drawing
			// ***todo*** PS2 pixel mask
			//bgr_temp |= SetPixelMask;

			// draw pixel if we can draw to mask pixels or mask bit not set
			// ***todo*** PS2 pixel mask
			//if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
			*ptr = bgr;
}


void GPU::DrawLine ()
{
#if defined INLINE_DEBUG_TRIANGLE || defined INLINE_DEBUG_PRIMITIVE
	debug << "; DrawLine";
#endif

	// get common variables needed to draw object
	SetDrawVars ();

	// check for some important conditions
	if ( Window_XRight < Window_XLeft )
	{
		cout << "\nhps2x64 ALERT: GPU: Window_XRight < Window_XLeft.\n";
		return;
	}
	
	if ( Window_YBottom < Window_YTop )
	{
		cout << "\nhps2x64 ALERT: GPU: Window_YBottom < Window_YTop.\n";
		return;
	}
	
	// check if object is texture mapped
	switch ( TextureMapped )
	{
		case 0:
			// draw single-color triangle //
			RenderLine ();
			break;
			
		case 1:
			// draw texture-mapped triangle //
			RenderLine ();
			break;
	}
}

void GPU::RenderLine ()
{
#if defined INLINE_DEBUG_LINE || defined INLINE_DEBUG_PRIMITIVE
	debug << "; RenderLine";
#endif

	// render in 8bits per pixel, then convert down when done
	
	static const int c_iVectorSize = 1;

	s32 StartX, EndX, StartY, EndY;
	u32 PixelsPerLine;
	u32 NumberOfPixelsDrawn;
	
	// if writing 16-bit pixels, then this could change to a u16 pointer
	u32 *ptr, *buf;
	
	u32 DestPixel, bgr, bgr_temp;
	
	// 12.4 fixed point
	s32 x0, y0, x1, y1;
	
	s32 Line, x_across;
	
	// get frame buffer pointer
	buf = & ( RAM32 [ FrameBufferStartOffset32 ] );
	
	// get x,y
	x0 = xyz [ 0 ].X;
	y0 = xyz [ 0 ].Y;
	x1 = xyz [ 1 ].X;
	y1 = xyz [ 1 ].Y;
	
	// get fill color
	bgr = rgbaq [ 0 ].Value & 0xffffffffL;
	
#if defined INLINE_DEBUG_LINE || defined INLINE_DEBUG_PRIMITIVE
	debug << hex << " bgr=" << bgr;
	debug << dec << "; Coords: x0=" << ( x0 >> 4 ) << " y0=" << ( y0 >> 4 ) << " x1=" << ( x1 >> 4 ) << " y1=" << ( y1 >> 4 );
#endif

	//////////////////////////////////////////
	// get coordinates on screen
	// *note* this is different from PS1, where you would add the offsets..
	x0 -= Coord_OffsetX;
	y0 -= Coord_OffsetY;
	x1 -= Coord_OffsetX;
	y1 -= Coord_OffsetY;
	
	// coords are in 12.4 fixed point
	x0 >>= 4;
	y0 >>= 4;
	x1 >>= 4;
	y1 >>= 4;
	
#if defined INLINE_DEBUG_LINE || defined INLINE_DEBUG_PRIMITIVE
	debug << dec << "; FinalCoords: x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1;
#endif


	// check if sprite is within draw area
	if ( x1 < Window_XLeft || x0 > Window_XRight || y1 < Window_YTop || y0 > Window_YBottom ) return;
#if defined INLINE_DEBUG_LINE_TEST || defined INLINE_DEBUG_PRIMITIVE_TEST
	else
	{
		debug << dec << "\r\nDrawRectangle" << " FinalCoords: x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1;
		debug << " FrameBufPixFmt=" << PixelFormat_Names [ FrameBuffer_PixelFormat ];
	}
#endif
	
	// on PS2, coords are in 12.4 fixed point
	StartX = x0;
	EndX = x1;
	StartY = y0;
	EndY = y1;

	if ( StartY < Window_YTop )
	{
		StartY = Window_YTop;
	}
	
	if ( EndY > ((s32)Window_YBottom) )
	{
		EndY = Window_YBottom;
	}
	
	if ( StartX < ((s32)Window_XLeft) )
	{
		StartX = Window_XLeft;
	}
	
	if ( EndX > ((s32)Window_XRight) )
	{
		EndX = Window_XRight;
	}

	
	// ***todo*** draw the line //
	/*
	NumberOfPixelsDrawn = ( EndX - StartX + 1 ) * ( EndY - StartY + 1 );
	
	for ( Line = StartY; Line <= EndY; Line++ )
	{
		ptr = & ( buf [ StartX + ( Line * FrameBufferWidth_Pixels ) ] );
		
		// draw horizontal line
		//for ( x_across = StartX; x_across <= EndX; x_across++ )
		for ( x_across = StartX; x_across <= EndX; x_across += c_iVectorSize )
		{
			// read pixel from frame buffer if we need to check mask bit
			// ***todo*** PS2 pixel mask
			//DestPixel = *ptr;
			
			bgr_temp = bgr;

			// semi-transparency
			// ***todo*** PS2 semi-transparency
			//if ( command_abe )
			//{
			//	bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read.ABR );
			//}
			
			// check if we should set mask bit when drawing
			// ***todo*** PS2 pixel mask
			//bgr_temp |= SetPixelMask;

			// draw pixel if we can draw to mask pixels or mask bit not set
			// ***todo*** PS2 pixel mask
			//if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
			*ptr = bgr_temp;
			
			// update pointer for pixel out
			ptr += c_iVectorSize;
		}
	}
	*/
}


void GPU::DrawTriangle ()
{
#if defined INLINE_DEBUG_TRIANGLE || defined INLINE_DEBUG_PRIMITIVE
	debug << "; DrawTriangle";
#endif

	// get common variables needed to draw object
	SetDrawVars ();

	// check for some important conditions
	if ( Window_XRight < Window_XLeft )
	{
		cout << "\nhps2x64 ALERT: GPU: Window_XRight < Window_XLeft.\n";
		return;
	}
	
	if ( Window_YBottom < Window_YTop )
	{
		cout << "\nhps2x64 ALERT: GPU: Window_YBottom < Window_YTop.\n";
		return;
	}
	
	// check if object is texture mapped
	switch ( TextureMapped )
	{
		case 0:
		
			// check if this is single-color or gradient
			if ( Gradient )
			{
				//RenderTriangleGradient ();
				DrawTriangle_Gradient32 ();
			}
			else
			{
				// draw single-color triangle //
				//RenderTriangleColor ();
				DrawTriangle_Mono32 ();
			}
			
			break;
			
		case 1:
			// draw texture-mapped triangle //
			RenderTriangleTexture ();
			break;
	}
}

void GPU::RenderTriangleColor ()
{
#if defined INLINE_DEBUG_TRIANGLE || defined INLINE_DEBUG_PRIMITIVE
	debug << "; RenderTriangleColor";
#endif

	// render in 8bits per pixel, then convert down when done
	
	static const int c_iVectorSize = 1;

	s32 StartX, EndX, StartY, EndY;
	u32 PixelsPerLine;
	u32 NumberOfPixelsDrawn;
	
	// if writing 16-bit pixels, then this could change to a u16 pointer
	u32 *ptr, *buf;
	
	u32 DestPixel, bgr, bgr_temp;
	
	// 12.4 fixed point
	s32 x0, y0, x1, y1, x2, y2;
	
	s32 Line, x_across;
	
	// get frame buffer pointer
	buf = & ( RAM32 [ FrameBufferStartOffset32 ] );
	
	// get x,y
	x0 = xyz [ 0 ].X;
	y0 = xyz [ 0 ].Y;
	x1 = xyz [ 1 ].X;
	y1 = xyz [ 1 ].Y;
	x2 = xyz [ 2 ].X;
	y2 = xyz [ 2 ].Y;
	
	// get fill color
	bgr = rgbaq [ 0 ].Value & 0xffffffffL;
	
#if defined INLINE_DEBUG_TRIANGLE || defined INLINE_DEBUG_PRIMITIVE
	debug << hex << " bgr=" << bgr;
	debug << dec << "; Coords: x0=" << ( x0 >> 4 ) << " y0=" << ( y0 >> 4 ) << " x1=" << ( x1 >> 4 ) << " y1=" << ( y1 >> 4 ) << " x2=" << ( x2 >> 4 ) << " y2=" << ( y2 >> 4 );
#endif

	//////////////////////////////////////////
	// get coordinates on screen
	// *note* this is different from PS1, where you would add the offsets..
	x0 -= Coord_OffsetX;
	y0 -= Coord_OffsetY;
	x1 -= Coord_OffsetX;
	y1 -= Coord_OffsetY;
	x2 -= Coord_OffsetX;
	y2 -= Coord_OffsetY;
	
	// coords are in 12.4 fixed point
	x0 >>= 4;
	y0 >>= 4;
	x1 >>= 4;
	y1 >>= 4;
	x2 >>= 4;
	y2 >>= 4;
	
#if defined INLINE_DEBUG_TRIANGLE || defined INLINE_DEBUG_PRIMITIVE
	debug << dec << "; FinalCoords: x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2;
#endif

	// for testing, set bounding coords to x0,y0,x1,y1
	StartX = x0;
	if ( x1 < StartX )
	{
		StartX = x1;
	}

	if ( x2 < StartX )
	{
		StartX = x2;
	}

	EndX = x0;
	if ( x1 > EndX )
	{
		EndX = x1;
	}

	if ( x2 > EndX )
	{
		EndX = x2;
	}

	StartY = y0;
	if ( y1 < StartY )
	{
		StartY = y1;
	}

	if ( y2 < StartY )
	{
		StartY = y2;
	}

	EndY = y0;
	if ( y1 > EndY )
	{
		EndY = y1;
	}

	if ( y2 > EndY )
	{
		EndY = y2;
	}


	// check if sprite is within draw area
	if ( EndX < Window_XLeft || StartX > Window_XRight || EndY < Window_YTop || StartY > Window_YBottom ) return;
#if defined INLINE_DEBUG_TRIANGLE_TEST || defined INLINE_DEBUG_PRIMITIVE_TEST
	else
	{
		debug << dec << "\r\nDrawTriangleColor" << " FinalCoords: x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2;
		debug << " Shading=" << GPURegsGp.PRIM.IIP;
		debug << " AlphaBlending=" << GPURegsGp.PRIM.ABE;
		debug << " FrameBufPixFmt=" << PixelFormat_Names [ FrameBuffer_PixelFormat ];
	}
#endif
	

	if ( StartY < Window_YTop )
	{
		StartY = Window_YTop;
	}
	
	if ( EndY > Window_YBottom )
	{
		EndY = Window_YBottom;
	}
	
	if ( StartX < Window_XLeft )
	{
		StartX = Window_XLeft;
	}
	
	if ( EndX > Window_XRight )
	{
		EndX = Window_XRight;
	}

	
	// ***todo*** draw the triangle *** //
	
}



void GPU::RenderTriangleTexture ()
{
#if defined INLINE_DEBUG_TRIANGLE || defined INLINE_DEBUG_PRIMITIVE
	debug << "; RenderTriangleTexture";
#endif

	// render in 8bits per pixel, then convert down when done
	
	static const int c_iVectorSize = 1;

	s32 StartX, EndX, StartY, EndY;
	u32 PixelsPerLine;
	u32 NumberOfPixelsDrawn;
	
	// if writing 16-bit pixels, then this could change to a u16 pointer
	u32 *ptr, *buf;
	
	u32 DestPixel, bgr, bgr_temp;
	
	u32 PixelFormat;
	u32 TexBufWidth, TexWidth, TexHeight, TexWidth_Mask, TexHeight_Mask;
	
	// 12.4 fixed point
	s32 x0, y0, x1, y1, x2, y2;
	s32 u0, v0, u1, v1, u2, v2;
	
	s32 Line, x_across;
	
	
	TEX0_t *TEX0 = &GPURegsGp.TEX0_1;
	
	// get frame buffer pointer
	buf = & ( RAM32 [ FrameBufferStartOffset32 ] );
	
	
	TexBufWidth = TEX0 [ Ctx ].TBW0 << 6;
	TexWidth = 1 << TEX0 [ Ctx ].TW;
	TexHeight = 1 << TEX0 [ Ctx ].TH;
	TexWidth_Mask = TexWidth - 1;
	TexHeight_Mask = TexHeight - 1;
	
	/////////////////////////////////////////////////////////
	// Get offset into texture page
	//TextureOffset = ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 );
	TexBufStartOffset32 = TEX0 [ Ctx ].TBP0 << 6;
	
	// get the pixel format (16 bit, 32 bit, etc)
	PixelFormat = TEX0 [ Ctx ].PSM;
	
	
	// get x,y
	x0 = xyz [ 0 ].X;
	y0 = xyz [ 0 ].Y;
	x1 = xyz [ 1 ].X;
	y1 = xyz [ 1 ].Y;
	x2 = xyz [ 2 ].X;
	y2 = xyz [ 2 ].Y;
	
	// get u,v
	u0 = uv [ 0 ].U;
	v0 = uv [ 0 ].V;
	u1 = uv [ 1 ].U;
	v1 = uv [ 1 ].V;
	u2 = uv [ 2 ].U;
	v2 = uv [ 2 ].V;
	
	// get fill color
	//bgr = rgbaq [ 0 ].Value & 0xffffffffL;
	
#if defined INLINE_DEBUG_TRIANGLE || defined INLINE_DEBUG_PRIMITIVE
	debug << hex << " bgr=" << bgr;
	debug << dec << "; Coords: x0=" << ( x0 >> 4 ) << " y0=" << ( y0 >> 4 ) << " x1=" << ( x1 >> 4 ) << " y1=" << ( y1 >> 4 ) << " x2=" << ( x2 >> 4 ) << " y2=" << ( y2 >> 4 );
#endif

	//////////////////////////////////////////
	// get coordinates on screen
	// *note* this is different from PS1, where you would add the offsets..
	x0 -= Coord_OffsetX;
	y0 -= Coord_OffsetY;
	x1 -= Coord_OffsetX;
	y1 -= Coord_OffsetY;
	x2 -= Coord_OffsetX;
	y2 -= Coord_OffsetY;
	
	// coords are in 12.4 fixed point
	x0 >>= 4;
	y0 >>= 4;
	x1 >>= 4;
	y1 >>= 4;
	x2 >>= 4;
	y2 >>= 4;
	
	// tex coords are in 10.4 fixed point
	u0 <<= 12;
	v0 <<= 12;
	u1 <<= 12;
	v1 <<= 12;
	u2 <<= 12;
	v2 <<= 12;
	
#if defined INLINE_DEBUG_TRIANGLE || defined INLINE_DEBUG_PRIMITIVE
	debug << dec << "; FinalCoords: x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2;
#endif

	// for testing, set bounding coords to x0,y0,x1,y1
	StartX = x0;
	if ( x1 < StartX )
	{
		StartX = x1;
	}

	if ( x2 < StartX )
	{
		StartX = x2;
	}

	EndX = x0;
	if ( x1 > EndX )
	{
		EndX = x1;
	}

	if ( x2 > EndX )
	{
		EndX = x2;
	}

	StartY = y0;
	if ( y1 < StartY )
	{
		StartY = y1;
	}

	if ( y2 < StartY )
	{
		StartY = y2;
	}

	EndY = y0;
	if ( y1 > EndY )
	{
		EndY = y1;
	}

	if ( y2 > EndY )
	{
		EndY = y2;
	}


	// check if sprite is within draw area
	if ( EndX < Window_XLeft || StartX > Window_XRight || EndY < Window_YTop || StartY > Window_YBottom ) return;
#if defined INLINE_DEBUG_TRIANGLE_TEST || defined INLINE_DEBUG_PRIMITIVE_TEST
	else
	{
		debug << dec << "\r\nDrawTriangleTexture" << " FinalCoords: x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2;
		debug << dec << " u0=" << ( u0 >> 16 ) << " v0=" << ( v0 >> 16 ) << " u1=" << ( u1 >> 16 ) << " v1=" << ( v1 >> 16 ) << " u2=" << ( u2 >> 16 ) << " v2=" << ( v2 >> 16 );
		debug << hex << "; TexBufPtr32/64=" << ( TexBufStartOffset32 >> 6 );
		debug << dec << "; TexWidth=" << TexWidth << " TexHeight=" << TexHeight;
		debug << "; TexPixFmt=" << PixelFormat_Names [ PixelFormat ];
		debug << hex << "; FrameBufPtr32/64=" << ( FrameBufferStartOffset32 >> 6 );
		debug << " FrameBufPixFmt=" << PixelFormat_Names [ FrameBuffer_PixelFormat ];
	}
#endif
	

	if ( StartY < Window_YTop )
	{
		StartY = Window_YTop;
	}
	
	if ( EndY > Window_YBottom )
	{
		EndY = Window_YBottom;
	}
	
	if ( StartX < Window_XLeft )
	{
		StartX = Window_XLeft;
	}
	
	if ( EndX > Window_XRight )
	{
		EndX = Window_XRight;
	}

	
	// ***todo*** draw the triangle *** //
	
}


void GPU::DrawTriangleFan ()
{
}

void GPU::DrawSprite ()
{
#if defined INLINE_DEBUG_SPRITE || defined INLINE_DEBUG_PRIMITIVE
	debug << "; DrawSprite";
	debug << "; (before)BusyUntil_Cycle=" << dec << BusyUntil_Cycle;
#endif

	// sprite is being drawn, so will start a new primitive after this
	StartPrimitive ();

	// get common variables needed to draw object
	SetDrawVars ();

	// check for some important conditions
	if ( Window_XRight < Window_XLeft )
	{
		cout << "\nhps2x64 ALERT: GPU: Window_XRight < Window_XLeft.\n";
		return;
	}
	
	if ( Window_YBottom < Window_YTop )
	{
		cout << "\nhps2x64 ALERT: GPU: Window_YBottom < Window_YTop.\n";
		return;
	}
	
	// check if object is texture mapped
	switch ( TextureMapped )
	{
		case 0:
			// draw single-color rectangle //
			RenderRectangle ();
			break;
			
		case 1:
			// draw texture-mapped sprite //
			RenderSprite ();
			break;
	}
	
#if defined INLINE_DEBUG_SPRITE || defined INLINE_DEBUG_PRIMITIVE
	debug << "; (after)BusyUntil_Cycle=" << dec << BusyUntil_Cycle;
#endif
}


// rendering functions //

void GPU::RenderRectangle ()
{
#if defined INLINE_DEBUG_SPRITE || defined INLINE_DEBUG_PRIMITIVE
	debug << "; RenderRectangle";
#endif

	// render in 8bits per pixel, then convert down when done
	
	static const int c_iVectorSize = 1;

	s32 StartX, EndX, StartY, EndY;
	u32 PixelsPerLine;
	u32 NumberOfPixelsDrawn;
	
	// if writing 16-bit pixels, then this could change to a u16 pointer
	u32 *ptr, *buf;
	u32 *ptr32;
	u16 *ptr16;
	
	u32 DestPixel, bgr, bgr_temp;
	
	// 12.4 fixed point
	s32 x0, y0, x1, y1;
	
	s32 Line, x_across;
	
	// get frame buffer pointer
	buf = & ( RAM32 [ FrameBufferStartOffset32 ] );
	
	// get x,y
	x0 = xyz [ 0 ].X;
	x1 = xyz [ 1 ].X;
	y0 = xyz [ 0 ].Y;
	y1 = xyz [ 1 ].Y;
	
	// get fill color
	bgr = rgbaq [ 0 ].Value & 0xffffffffL;
	
#if defined INLINE_DEBUG_SPRITE || defined INLINE_DEBUG_PRIMITIVE
	debug << hex << " bgr=" << bgr;
	debug << dec << "; Coords: x0=" << ( x0 >> 4 ) << " y0=" << ( y0 >> 4 ) << " x1=" << ( x1 >> 4 ) << " y1=" << ( y1 >> 4 );
#endif

	//////////////////////////////////////////
	// get coordinates on screen
	// *note* this is different from PS1, where you would add the offsets..
	x0 -= Coord_OffsetX;
	y0 -= Coord_OffsetY;
	x1 -= Coord_OffsetX;
	y1 -= Coord_OffsetY;
	
	// coords are in 12.4 fixed point
	x0 >>= 4;
	y0 >>= 4;
	x1 >>= 4;
	y1 >>= 4;
	
#if defined INLINE_DEBUG_SPRITE || defined INLINE_DEBUG_PRIMITIVE
	debug << dec << "; FinalCoords: x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1;
#endif


	// check if sprite is within draw area
	if ( x1 < Window_XLeft || x0 > Window_XRight || y1 < Window_YTop || y0 > Window_YBottom ) return;
#if defined INLINE_DEBUG_SPRITE_TEST || defined INLINE_DEBUG_PRIMITIVE_TEST
	else
	{
		debug << dec << "\r\nDrawRectangle" << " FinalCoords: x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1;
		debug << hex << " bgr=" << bgr;
		debug << " FrameBufPixFmt=" << PixelFormat_Names [ FrameBuffer_PixelFormat ];
		debug << hex << "; FrameBufPtr32/64=" << ( FrameBufferStartOffset32 >> 6 );
		debug << " FrameBufPixFmt=" << PixelFormat_Names [ FrameBuffer_PixelFormat ];
	}
#endif
	
	// on PS2, coords are in 12.4 fixed point
	StartX = x0;
	EndX = x1;
	StartY = y0;
	EndY = y1;

	if ( StartY < Window_YTop )
	{
		StartY = Window_YTop;
	}
	
	if ( EndY > Window_YBottom )
	{
		EndY = Window_YBottom;
	}
	
	if ( StartX < Window_XLeft )
	{
		StartX = Window_XLeft;
	}
	
	if ( EndX > Window_XRight )
	{
		EndX = Window_XRight;
	}

	
	NumberOfPixelsDrawn = ( EndX - StartX + 1 ) * ( EndY - StartY + 1 );
	
	// there are probably multiple pixel pipelines, so  might need to divide by like 8 or 16 or something
	if ( BusyUntil_Cycle < *_DebugCycleCount )
	{
		BusyUntil_Cycle = *_DebugCycleCount + ( NumberOfPixelsDrawn >> 4 );
	}
	//else
	//{
	//	BusyUntil_Cycle += NumberOfPixelsDrawn;
	//}
	
	if ( !FrameBuffer_PixelFormat || FrameBuffer_PixelFormat == 1 )
	{
		// 32-bit frame buffer //
		
		for ( Line = StartY; Line <= EndY; Line++ )
		{
			ptr32 = & ( buf [ StartX + ( Line * FrameBufferWidth_Pixels ) ] );
			
			// draw horizontal line
			//for ( x_across = StartX; x_across <= EndX; x_across++ )
			for ( x_across = StartX; x_across <= EndX; x_across += c_iVectorSize )
			{
				// read pixel from frame buffer if we need to check mask bit
				// ***todo*** PS2 pixel mask
				//DestPixel = *ptr;
				
				bgr_temp = bgr;

				// semi-transparency
				// ***todo*** PS2 semi-transparency
				//if ( command_abe )
				//{
				//	bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read.ABR );
				//}
				
				// check if we should set mask bit when drawing
				// ***todo*** PS2 pixel mask
				//bgr_temp |= SetPixelMask;

				// draw pixel if we can draw to mask pixels or mask bit not set
				// ***todo*** PS2 pixel mask
				//if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
				*ptr32 = bgr_temp;
				
				// update pointer for pixel out
				ptr32 += c_iVectorSize;
			}
		}
	}
	else
	{
		// assume 16-bit frame buffer for now //
		
		// ***todo*** bgr conversion from 32-bit->16-bit required ???
		
		for ( Line = StartY; Line <= EndY; Line++ )
		{
			ptr16 = (u16*) ( & ( buf [ StartX + ( Line * ( FrameBufferWidth_Pixels >> 1 ) ) ] ) );
			
			// draw horizontal line
			//for ( x_across = StartX; x_across <= EndX; x_across++ )
			for ( x_across = StartX; x_across <= EndX; x_across += c_iVectorSize )
			{
				// read pixel from frame buffer if we need to check mask bit
				// ***todo*** PS2 pixel mask
				//DestPixel = *ptr;
				
				bgr_temp = bgr;

				// semi-transparency
				// ***todo*** PS2 semi-transparency
				//if ( command_abe )
				//{
				//	bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read.ABR );
				//}
				
				// check if we should set mask bit when drawing
				// ***todo*** PS2 pixel mask
				//bgr_temp |= SetPixelMask;

				// draw pixel if we can draw to mask pixels or mask bit not set
				// ***todo*** PS2 pixel mask
				//if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
				*ptr16 = bgr_temp;
				
				// update pointer for pixel out
				ptr16 += c_iVectorSize;
			}
		}
	}

}

void GPU::RenderSprite ()
{
#if defined INLINE_DEBUG_SPRITE || defined INLINE_DEBUG_PRIMITIVE
	debug << "; RenderSprite";
#endif

	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;

	// notes: looks like sprite size is same as specified by w/h

	//u32 Pixel,
	
	u32 TexelIndex;
	
	u32 color_add;
	
	u32 *ptr_texture;
	u32 *ptr_clut;
	u32 clut_xoffset, clut_yoffset;
	
	//u16 *ptr;
	s32 StartX, EndX, StartY, EndY;
	u32 NumberOfPixelsDrawn;
	
	// if writing 16-bit pixels, then this could change to a u16 pointer
	u32 *ptr, *buf;
	u32 *ptr32;
	u16 *ptr16;
	u16 *ptr_texture16;
	u16 *buf16;
	
	u32 DestPixel, bgr, bgr_temp;
	
	// 12.4 fixed point
	s32 x0, y0, x1, y1;
	s32 u0, v0, u1, v1;
	
	s32 iU, iV, dudx, dvdy;
	
	s32 Line, x_across;
	
	
	//u32 tge;
	
	//u32 DestPixel, PixelMask = 0, SetPixelMask = 0;
	u32 TexCoordX, TexCoordY;
	u32 Shift1 = 0, Shift2 = 0, And1 = 0, And2 = 0;
	u32 TextureOffset;
	
	u32 And3, Shift3, Shift4;
	
	u32 PixelsPerLine;
	
	u32 PixelFormat;
	u32 CLUTBufBase32, CLUTPixelFormat, CLUTStoreMode, CLUTOffset;
	u32 TexBufWidth, TexWidth, TexHeight, TexWidth_Mask, TexHeight_Mask;
	u32 TexBPP;

	TEX0_t *TEX0 = &GPURegsGp.TEX0_1;

	
	// get frame buffer pointer
	buf = & ( RAM32 [ FrameBufferStartOffset32 ] );
	
	
	//u32 TWYTWH, TWXTWW, Not_TWH, Not_TWW;
	//TWYTWH = ( ( TWY & TWH ) << 3 );
	//TWXTWW = ( ( TWX & TWW ) << 3 );
	//Not_TWH = ~( TWH << 3 );
	//Not_TWW = ~( TWW << 3 );
	//if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	//if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
	TexBufWidth = TEX0 [ Ctx ].TBW0 << 6;
	TexWidth = 1 << TEX0 [ Ctx ].TW;
	TexHeight = 1 << TEX0 [ Ctx ].TH;
	TexWidth_Mask = TexWidth - 1;
	TexHeight_Mask = TexHeight - 1;
	
	
	/////////////////////////////////////////////////////////
	// Get offset into texture page
	//TextureOffset = ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 );
	TexBufStartOffset32 = TEX0 [ Ctx ].TBP0 << 6;
	
	// get the pixel format (16 bit, 32 bit, etc)
	PixelFormat = TEX0 [ Ctx ].PSM;
	CLUTPixelFormat = TEX0 [ Ctx ].CPSM;
	
	// get base pointer to color lookup table (32-bit word address divided by 64)
	CLUTBufBase32 = TEX0 [ Ctx ].CBP << 6;
	
	// storage mode ??
	CLUTStoreMode = TEX0 [ Ctx ].CSM;
	
	// clut offset ??
	CLUTOffset = TEX0 [ Ctx ].CSA;

#if defined INLINE_DEBUG_SPRITE || defined INLINE_DEBUG_PRIMITIVE
	debug << "; TexPixFmt=" << PixelFormat_Names [ PixelFormat ];
#endif

	//tge = command_tge;
	//if ( ( bgr & 0x00ffffff ) == 0x00808080 ) tge = 1;
	
	//////////////////////////////////////////////////////
	// Get offset into color lookup table
	//u32 ClutOffset = ( clut_x << 4 ) + ( clut_y << 10 );
	
	//clut_xoffset = clut_x << 4;
	clut_xoffset = 0;
	//ptr_clut = & ( VRAM [ clut_y << 10 ] );
	ptr_clut = & ( RAM32 [ CLUTBufBase32 ] );
	//ptr_texture = & ( VRAM [ ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 ) ] );
	ptr_texture = & ( RAM32 [ TexBufStartOffset32 ] );
	
	/*
	if ( tpage_tp == 0 )
	{
		And2 = 0xf;
		
		Shift1 = 2; Shift2 = 2;
		And1 = 3; And2 = 0xf;
	}
	else if ( tpage_tp == 1 )
	{
		And2 = 0xff;
		
		Shift1 = 1; Shift2 = 3;
		And1 = 1; And2 = 0xff;
	}
	*/
	
	// assume bpp = 32, ptr = 32-bit
	Shift1 = 0;
	
	if ( PixelFormat == 0x14 )
	{
		// bpp = 4, ptr = 32-bit
		Shift1 = 3;
		Shift2 = 2;
		And1 = 0x7; And2 = 0xf;
		Shift3 = 1;
	}
	
	if ( PixelFormat == 0x2 )
	{
		// bpp = 16, texture ptr = 32-bit
		Shift1 = 1;
		Shift2 = 4;
		And1 = 0x1; And2 = 0xffff;
	}
	
	// assume clut bpp = 24/32, 32-bit ptr
	And3 = 0; Shift4 = 0;
	
	if ( CLUTPixelFormat == 0x2 )
	{
		// bpp = 16, ptr = 32-bit
		And3 = 1;
		Shift4 = 4;
		
		/*
		if ( FrameBuffer_PixelFormat == 0 || FrameBuffer_PixelFormat == 1 )
		{
			Pixel_SrcMask = 0x1f;
			Pixel_SrcBpp = 5;
			Pixel_DstShift1 = 3;
		}
		*/
	}
	
	// important: reading from buffer in 32-bit units, so the texture buffer width should be divided accordingly
	//TexBufWidth >>= Shift1;
	
	
	color_add = bgr;

	// initialize number of pixels drawn
	NumberOfPixelsDrawn = 0;
	
	
	// get x,y
	x0 = xyz [ 0 ].X;
	x1 = xyz [ 1 ].X;
	y0 = xyz [ 0 ].Y;
	y1 = xyz [ 1 ].Y;
	
	// get u,v
	u0 = uv [ 0 ].U;
	u1 = uv [ 1 ].U;
	v0 = uv [ 0 ].V;
	v1 = uv [ 1 ].V;
	
#if defined INLINE_DEBUG_SPRITE || defined INLINE_DEBUG_PRIMITIVE
	debug << hex << " bgr=" << bgr;
	debug << dec << "; Coords: x0=" << ( x0 >> 4 ) << " y0=" << ( y0 >> 4 ) << " x1=" << ( x1 >> 4 ) << " y1=" << ( y1 >> 4 );
	debug << dec << " u0=" << ( u0 >> 4 ) << " v0=" << ( v0 >> 4 ) << " u1=" << ( u1 >> 4 ) << " v1=" << ( v1 >> 4 );
#endif

	//////////////////////////////////////////
	// get coordinates on screen
	// *note* this is different from PS1, where you would add the offsets..
	x0 -= Coord_OffsetX;
	y0 -= Coord_OffsetY;
	x1 -= Coord_OffsetX;
	y1 -= Coord_OffsetY;
	
	
	// coords are in 12.4 fixed point
	x0 >>= 4;
	y0 >>= 4;
	x1 >>= 4;
	y1 >>= 4;
	
	// tex coords are in 10.4 fixed point
	//u0 >>= 4;
	//v0 >>= 4;
	//u1 >>= 4;
	//v1 >>= 4;
	u0 <<= 12;
	v0 <<= 12;
	u1 <<= 12;
	v1 <<= 12;
	
	if ( x1 - x0 ) dudx = ( u1 - u0 ) / ( x1 - x0 );
	if ( y1 - y0 ) dvdy = ( v1 - v0 ) / ( y1 - y0 );
	
#if defined INLINE_DEBUG_SPRITE || defined INLINE_DEBUG_PRIMITIVE
	debug << dec << "; FinalCoords: x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1;
	debug << hex << " u0=" << u0 << " v0=" << v0 << " u1=" << u1 << " v1=" << v1;
#endif


	// check if sprite is within draw area
	if ( x1 < Window_XLeft || x0 > Window_XRight || y1 < Window_YTop || y0 > Window_YBottom ) return;
#if defined INLINE_DEBUG_SPRITE_TEST || defined INLINE_DEBUG_PRIMITIVE_TEST
	else
	{
		debug << dec << "\r\nDrawSprite" << " FinalCoords: x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1;
		debug << dec << " u0=" << ( u0 >> 16 ) << " v0=" << ( v0 >> 16 ) << " u1=" << ( u1 >> 16 ) << " v1=" << ( v1 >> 16 );
		debug << hex << "; TexBufPtr32/64=" << ( TexBufStartOffset32 >> 6 );
		debug << dec << "; TexWidth=" << TexWidth << " TexHeight=" << TexHeight;
		debug << "; TexPixFmt=" << PixelFormat_Names [ PixelFormat ];
		debug << hex << "; CLUTBufPtr32/64=" << ( CLUTBufBase32 >> 6 );
		debug << "; CLUTPixFmt=" << PixelFormat_Names [ CLUTPixelFormat ];
		debug << hex << "; CLUTOffset/16=" << CLUTOffset;
		debug << "; CSM=" << CLUTStoreMode;
		debug << hex << "; FrameBufPtr32/64=" << ( FrameBufferStartOffset32 >> 6 );
		debug << dec << " FrameBufWidth=" << FrameBufferWidth_Pixels;
		debug << " FrameBufPixFmt=" << PixelFormat_Names [ FrameBuffer_PixelFormat ];
		debug << " Shift1=" << hex << Shift1;
		debug << " And3=" << hex << And3;
	}
#endif
	

	
	StartX = x0;
	EndX = x1;
	StartY = y0;
	EndY = y1;

	if ( StartY < Window_YTop )
	{
		v0 += ( Window_YTop - StartY ) * dvdy;
		StartY = Window_YTop;
	}
	
	if ( EndY > Window_YBottom )
	{
		EndY = Window_YBottom;
	}
	
	if ( StartX < Window_XLeft )
	{
		u0 += ( Window_XLeft - StartX ) * dudx;
		StartX = Window_XLeft;
	}
	
	if ( EndX > Window_XRight )
	{
		EndX = Window_XRight;
	}

	
	//iV = v;
	iV = v0;
	
	NumberOfPixelsDrawn = ( EndX - StartX + 1 ) * ( EndY - StartY + 1 );
		
	// there are probably multiple pixel pipelines, so  might need to divide by like 8 or 16 or something
	// multiplying by 2 since the pixels has to be read before drawing
	//BusyUntil_Cycle = *_DebugCycleCount + ( NumberOfPixelsDrawn << 1 );
	if ( BusyUntil_Cycle < *_DebugCycleCount )
	{
		BusyUntil_Cycle = *_DebugCycleCount + ( ( NumberOfPixelsDrawn << 1 ) >> 4 );
	}
	//else
	//{
	//	BusyUntil_Cycle += ( NumberOfPixelsDrawn << 1 );
	//}


	if ( !FrameBuffer_PixelFormat || FrameBuffer_PixelFormat == 1 )
	{
		// 32-bit frame buffer //

		for ( Line = StartY; Line <= EndY; Line++ )
		{
			// need to start texture coord from left again
			//iU = u;
			iU = u0;
			
			//TexCoordY = (u8) ( ( iV & ~( TWH << 3 ) ) | ( ( TWY & TWH ) << 3 ) );
			//TexCoordY = (u8) ( ( iV & Not_TWH ) | ( TWYTWH ) );
			//TexCoordY <<= 10;
			TexCoordY = ( iV >> 16 ) & TexHeight_Mask;

			//ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			ptr32 = & ( buf [ StartX + ( Line * FrameBufferWidth_Pixels ) ] );
			

			// draw horizontal line
			//for ( x_across = StartX; x_across <= EndX; x_across++ )
			for ( x_across = StartX; x_across <= EndX; x_across += c_iVectorSize )
			{
				//TexCoordX = (u8) ( ( iU & Not_TWW ) | ( TWXTWW ) );
				TexCoordX = ( iU >> 16 ) & TexWidth_Mask;
				
				//bgr = ptr_texture [ ( TexCoordX >> Shift1 ) + TexCoordY ];
				bgr = ptr_texture [ ( TexCoordX >> Shift1 ) + ( TexCoordY * ( TexBufWidth >> Shift1 ) ) ];
				
				
				if ( Shift1 > 1 )
				{
					// assume 16-bit pixels for color lookup for now //
					
					u32 index = ( ( bgr >> ( ( TexCoordX & And1 ) << Shift2 ) ) & And2 );
					bgr = ptr_clut [ ( clut_xoffset + ( index >> Shift3 ) ) /* & FrameBuffer_XMask */ ];
					bgr >>= ( ( index & And3 ) << Shift4 );
					
					// convert pixel
					bgr = ConvertPixel16To24 ( bgr );
				}
				else if ( Shift1 == 1 )
				{
					// 16-bit pixels in texture buffer //
					bgr = ( ( bgr >> ( ( TexCoordX & And1 ) << Shift2 ) ) & And2 );
					
					// convert pixel
					bgr = ConvertPixel16To24 ( bgr );
				}

						
				//if ( bgr )
				//{
					// read pixel from frame buffer if we need to check mask bit
					//DestPixel = *ptr;
					
					bgr_temp = bgr;
		
					/*
					if ( !tge )
					{
						// brightness calculation
						bgr_temp = ColorMultiply1624 ( bgr_temp, color_add );
					}
					
					// semi-transparency
					if ( command_abe && ( bgr & 0x8000 ) )
					{
						bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read.ABR );
					}
					
					// check if we should set mask bit when drawing
					bgr_temp |= SetPixelMask | ( bgr & 0x8000 );
					*/

					// draw pixel if we can draw to mask pixels or mask bit not set
					//if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
					*ptr32 = bgr_temp;
				//}
						
				/////////////////////////////////////////////////////////
				// interpolate texture coords across
				//iU += c_iVectorSize;
				iU += dudx;
				
				// update pointer for pixel out
				ptr32 += c_iVectorSize;
					
			}
			
			/////////////////////////////////////////////////////////
			// interpolate texture coords down
			//iV++;
			iV += dvdy;
		}
	}
	else
	{
		// assume 16-bit frame buffer //
		
		// ***todo*** convert 32-bit pixels to 16-bit ???
		
		ptr_texture16 = (u16*) ptr_texture;
		buf16 = (u16*) buf;
		
		for ( Line = StartY; Line <= EndY; Line++ )
		{
			// need to start texture coord from left again
			//iU = u;
			iU = u0;
			
			//TexCoordY = (u8) ( ( iV & ~( TWH << 3 ) ) | ( ( TWY & TWH ) << 3 ) );
			//TexCoordY = (u8) ( ( iV & Not_TWH ) | ( TWYTWH ) );
			//TexCoordY <<= 10;
			TexCoordY = ( iV >> 16 ) & TexHeight_Mask;

			//ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			ptr16 = & ( buf16 [ StartX + ( Line * FrameBufferWidth_Pixels ) ] );
			

			// draw horizontal line
			//for ( x_across = StartX; x_across <= EndX; x_across++ )
			for ( x_across = StartX; x_across <= EndX; x_across += c_iVectorSize )
			{
				//TexCoordX = (u8) ( ( iU & Not_TWW ) | ( TWXTWW ) );
				TexCoordX = ( iU >> 16 ) & TexWidth_Mask;
				
				//bgr = ptr_texture [ ( TexCoordX >> Shift1 ) + TexCoordY ];
				bgr = ptr_texture [ ( TexCoordX >> Shift1 ) + ( TexCoordY * ( TexBufWidth >> Shift1 ) ) ];
				
				
				if ( Shift1 > 1 )
				{
					// pixel size smaller than 16-bits //
					
					u32 index = ( ( bgr >> ( ( TexCoordX & And1 ) << Shift2 ) ) & And2 );
					bgr = ptr_clut [ ( clut_xoffset + ( index >> Shift3 ) ) /* & FrameBuffer_XMask */ ];
					bgr >>= ( ( index & And3 ) << Shift4 );
					
					// convert pixel (assuming 16-bit pixel size)
					bgr = ConvertPixel16To24 ( bgr );
				}
				else if ( Shift1 == 1 )
				{
					// 16-bit pixels //
					//bgr = ( ( bgr >> ( ( TexCoordX & And1 ) << Shift2 ) ) & And2 );
					bgr = ptr_texture16 [ TexCoordX + ( TexCoordY * TexBufWidth ) ];
				}

						
				//if ( bgr )
				//{
					// read pixel from frame buffer if we need to check mask bit
					//DestPixel = *ptr;
					
					bgr_temp = bgr;
		
					/*
					if ( !tge )
					{
						// brightness calculation
						bgr_temp = ColorMultiply1624 ( bgr_temp, color_add );
					}
					
					// semi-transparency
					if ( command_abe && ( bgr & 0x8000 ) )
					{
						bgr_temp = SemiTransparency16 ( DestPixel, bgr_temp, GPU_CTRL_Read.ABR );
					}
					
					// check if we should set mask bit when drawing
					bgr_temp |= SetPixelMask | ( bgr & 0x8000 );
					*/

					// draw pixel if we can draw to mask pixels or mask bit not set
					//if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
					*ptr16 = bgr_temp;
				//}
						
				/////////////////////////////////////////////////////////
				// interpolate texture coords across
				//iU += c_iVectorSize;
				iU += dudx;
				
				// update pointer for pixel out
				ptr16 += c_iVectorSize;
					
			}
			
			/////////////////////////////////////////////////////////
			// interpolate texture coords down
			//iV++;
			iV += dvdy;
		}
	}
}




// 3-d drawing //


void GPU::DrawTriangle_Mono32 ()
{	
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;

	//u32 Pixel, TexelIndex, Y1_OnLeft;
	//u32 color_add;
	
	//u32 Y1_OnLeft;
	
	s32 x0, y0, x1, y1, x2, y2;
	s64 x [ 2 ], dxdy [ 2 ];
	u32 bgr, bgr_temp;
	u32 *ptr32;
	u32* VRAM32;
	
	u32 NumberOfPixelsDrawn;
	s32 Line, x_across;
	
	u32 Coord0 = 0, Coord1 = 1, Coord2 = 2;
	u32 X1Index = 0, X0Index = 1;
	
	s64 Temp;
	s64 LeftMostX, RightMostX;
	
	s32 StartX, EndX, StartY, EndY;
	s64 Error_Left;
	
	
	
	
	u32 DestPixel, PixelMask = 0, SetPixelMask = 0;
	
	//if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	//if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
	
	VRAM32 = & ( RAM32 [ FrameBufferStartOffset32 ] );
	
	// get the color for the triangle
	bgr = rgbaq [ 0 ].Value & 0xffffffffULL;
	
	// initialize number of pixels drawn
	NumberOfPixelsDrawn = 0;
	
	///////////////////////////////////
	// put top coordinates in x0,y0
	//if ( y1 < y0 )
	if ( xyz [ Coord1 ].Y < xyz [ Coord0 ].Y )
	{
		//Swap ( x0, x1 );
		//Swap ( y0, y1 );
		Swap ( Coord0, Coord1 );
	}
	
	//if ( y2 < y0 )
	if ( xyz [ Coord2 ].Y < xyz [ Coord0 ].Y )
	{
		//Swap ( x0, x2 );
		//Swap ( y0, y2 );
		Swap ( Coord2, Coord0 );
	}
	
	///////////////////////////////////////
	// put middle coordinates in x1,y1
	//if ( y2 < y1 )
	if ( xyz [ Coord2 ].Y < xyz [ Coord1 ].Y )
	{
		//Swap ( x1, x2 );
		//Swap ( y1, y2 );
		Swap ( Coord2, Coord1 );
	}
	
	// get x,y
	x0 = xyz [ Coord0 ].X;
	y0 = xyz [ Coord0 ].Y;
	x1 = xyz [ Coord1 ].X;
	y1 = xyz [ Coord1 ].Y;
	x2 = xyz [ Coord2 ].X;
	y2 = xyz [ Coord2 ].Y;
	
#if defined INLINE_DEBUG_TRIANGLE || defined INLINE_DEBUG_PRIMITIVE
	debug << hex << " bgr=" << bgr;
	debug << dec << "; Coords: x0=" << ( x0 >> 4 ) << " y0=" << ( y0 >> 4 ) << " x1=" << ( x1 >> 4 ) << " y1=" << ( y1 >> 4 );
#endif

	//////////////////////////////////////////
	// get coordinates on screen
	// *note* this is different from PS1, where you would add the offsets..
	x0 -= Coord_OffsetX;
	y0 -= Coord_OffsetY;
	x1 -= Coord_OffsetX;
	y1 -= Coord_OffsetY;
	x2 -= Coord_OffsetX;
	y2 -= Coord_OffsetY;
	
	
	// coords are in 12.4 fixed point
	x0 >>= 4;
	y0 >>= 4;
	x1 >>= 4;
	y1 >>= 4;
	x2 >>= 4;
	y2 >>= 4;
	
	// get the left/right most x
	LeftMostX = ( ( x0 < x1 ) ? x0 : x1 );
	LeftMostX = ( ( x2 < LeftMostX ) ? x2 : LeftMostX );
	RightMostX = ( ( x0 > x1 ) ? x0 : x1 );
	RightMostX = ( ( x2 > RightMostX ) ? x2 : RightMostX );

	// check if triangle is within draw area
	//if ( RightMostX <= ((s32)DrawArea_TopLeftX) || LeftMostX > ((s32)DrawArea_BottomRightX) || y2 <= ((s32)DrawArea_TopLeftY) || y0 > ((s32)DrawArea_BottomRightY) ) return;
	if ( RightMostX <= Window_XLeft || LeftMostX > Window_XRight || y2 <= Window_YTop || y0 > Window_YBottom ) return;
#if defined INLINE_DEBUG_TRIANGLE_TEST || defined INLINE_DEBUG_PRIMITIVE_TEST
	else
	{
		debug << dec << "\r\nDrawTriangleMono" << " FinalCoords: x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2;
		debug << hex << "; FrameBufPtr32/64=" << ( FrameBufferStartOffset32 >> 6 );
		debug << " FrameBufPixFmt=" << PixelFormat_Names [ FrameBuffer_PixelFormat ];
	}
#endif
	
	// skip drawing if distance between vertices is greater than max allowed by GPU
	//if ( ( _Abs( x1 - x0 ) > c_MaxPolygonWidth ) || ( _Abs( x2 - x1 ) > c_MaxPolygonWidth ) || ( y1 - y0 > c_MaxPolygonHeight ) || ( y2 - y1 > c_MaxPolygonHeight ) )
	//{
	//	// skip drawing polygon
	//	return;
	//}
	
	
	
	/////////////////////////////////////////////////
	// draw top part of triangle
	
	// denominator is negative when x1 is on the left, positive when x1 is on the right
	s64 t0 = y1 - y2;
	s64 t1 = y0 - y2;
	s64 denominator = ( ( x0 - x2 ) * t0 ) - ( ( x1 - x2 ) * t1 );
	
	// check if x1 is on left or right //
	if ( denominator > 0 )
	{
		// x1 is on the right //
		X1Index = 1;
		X0Index = 0;
	}

	///////////////////////////////////////////
	// start at y0
	//Line = y0;
	
	
	//if ( denominator < 0 )
	//{
		// x1 is on the left and x0 is on the right //
		
		////////////////////////////////////
		// get slopes
		
		// start by assuming x1 is on the left and x0 is on the right //
		
		if ( y1 - y0 )
		{
			// triangle is pointed on top //
			
			/////////////////////////////////////////////
			// init x on the left and right
			//x_left = ( ((s64)x0) << 32 );
			//x_right = x_left;
			x [ 0 ] = ( ((s64)x0) << 32 );
			x [ 1 ] = x [ 0 ];
			
			//dx_left = (((s64)( x1 - x0 )) << 32 ) / ((s64)( y1 - y0 ));
			//dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
			dxdy [ X1Index ] = (((s64)( x1 - x0 )) << 32 ) / ((s64)( y1 - y0 ));
			dxdy [ X0Index ] = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
		}
		else
		{
			// Triangle is flat on top //
			
			// change x_left and x_right where y1 is on left
			//x_left = ( ((s64)x1) << 32 );
			//x_right = ( ((s64)x0) << 32 );
			x [ X1Index ] = ( ((s64)x1) << 32 );
			x [ X0Index ] = ( ((s64)x0) << 32 );
			
			// this means that x0 and x1 are on the same line
			// so if the height of the entire polygon is zero, then we are done
			if ( y2 - y1 )
			{
				//dx_left = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
				//dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
				dxdy [ X1Index ] = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
				dxdy [ X0Index ] = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
			}
		}
	//}

	/////////////////////////////////////////////////
	// swap left and right if they are reversed
	//if ( ( ( x_right + dx_right ) < ( x_left + dx_left ) ) || ( x_right < x_left ) )
	/*
	if ( denominator > 0 )
	{
		// x1,y1 is on the right //
		
		//Swap ( x_left, x_right );
		//Swap ( dx_left, dx_right );
		Swap ( x_left, x_right );
		Swap ( dx_left, dx_right );
	}
	*/
	
	////////////////
	// *** TODO *** at this point area of full triangle can be calculated and the rest of the drawing can be put on another thread *** //
	
	
	
	StartY = y0;
	EndY = y1;

	if ( StartY < Window_YTop )
	{
		
		if ( EndY < Window_YTop )
		{
			Temp = EndY - StartY;
			StartY = EndY;
		}
		else
		{
			Temp = Window_YTop - StartY;
			StartY = Window_YTop;
		}
		
		//x_left += dx_left * Temp;
		//x_right += dx_right * Temp;
		x [ 0 ] += dxdy [ 0 ] * Temp;
		x [ 1 ] += dxdy [ 1 ] * Temp;
	}
	
	if ( EndY > Window_YBottom )
	{
		EndY = Window_YBottom + 1;
	}

	
	//////////////////////////////////////////////
	// draw down to y1
	//while ( Line < y1 )
	for ( Line = StartY; Line < EndY; Line++ )
	{
		//cy = Line;

		//StartX = _Round( x_left ) >> 32;
		//EndX = _Round( x_right ) >> 32;
		StartX = _Round( x [ 0 ] ) >> 32;
		EndX = _Round( x [ 1 ] ) >> 32;
		
		//if ( Line >= ((s32)DrawArea_TopLeftY) && Line <= ((s32)DrawArea_BottomRightY) )
		//{
		if ( StartX <= Window_XRight && EndX > Window_XLeft )
		{
			if ( StartX < Window_XLeft )
			{
				//Temp = DrawArea_TopLeftX - StartX;
				StartX = Window_XLeft;
			}
			
			if ( EndX > Window_XRight )
			{
				EndX = Window_XRight + 1;
				//EndX = DrawArea_BottomRightX;
			}
			
			//ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			ptr32 = & ( VRAM32 [ StartX + ( Line * FrameBufferWidth_Pixels ) ] );
			
			if ( StartX == EndX && !dxdy [ 0 ] )
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
			

			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across < EndX; x_across++ )
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{

				/*
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
				*/

				// draw pixel if we can draw to mask pixels or mask bit not set
				//if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
				*ptr32 = bgr;
				
				//ptr++;
				ptr32 += c_iVectorSize;
			}
			
		}
		
		//////////////////////////////////
		// draw next line
		//Line++;
		
		/////////////////////////////////////
		// update x on left and right
		//x_left += dx_left;
		//x_right += dx_right;
		x [ 0 ] += dxdy [ 0 ];
		x [ 1 ] += dxdy [ 1 ];
	}

	
	////////////////////////////////////////////////
	// draw bottom part of triangle

	/////////////////////////////////////////////
	// init x on the left and right
	
	//////////////////////////////////////////////////////
	// check if y1 is on the left or on the right
	/*
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
	*/

	
	x [ X1Index ] = ( ((s64)x1) << 32 );
	
	// check if triangle is flat on the bottom //
	if ( y2 - y1 )
	{
		// triangle is pointed on the bottom //
		dxdy [ X1Index ] = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
	}
	
	// the line starts at y1 from here
	//Line = y1;

	StartY = y1;
	EndY = y2;

	if ( StartY < Window_YTop )
	{
		
		if ( EndY < Window_YTop )
		{
			Temp = EndY - StartY;
			StartY = EndY;
		}
		else
		{
			Temp = Window_YTop - StartY;
			StartY = Window_YTop;
		}
		
		//x_left += dx_left * Temp;
		//x_right += dx_right * Temp;
		x [ 0 ] += dxdy [ 0 ] * Temp;
		x [ 1 ] += dxdy [ 1 ] * Temp;
	}
	
	if ( EndY > Window_YBottom )
	{
		EndY = Window_YBottom + 1;
	}

	
	//////////////////////////////////////////////
	// draw down to y2
	//while ( Line < y2 )
	for ( Line = StartY; Line < EndY; Line++ )
	{
		//cy = Line;

		//StartX = _Round( x_left ) >> 32;
		//EndX = _Round( x_right ) >> 32;
		StartX = _Round( x [ 0 ] ) >> 32;
		EndX = _Round( x [ 1 ] ) >> 32;
		
		//if ( Line >= ((s32)DrawArea_TopLeftY) && Line <= ((s32)DrawArea_BottomRightY) )
		//{
		if ( StartX <= Window_XRight && EndX > Window_XLeft )
		{
			if ( StartX < Window_XLeft )
			{
				//Temp = DrawArea_TopLeftX - StartX;
				StartX = Window_XLeft;
			}
			
			if ( EndX > Window_XRight )
			{
				EndX = Window_XRight + 1;
				//EndX = DrawArea_BottomRightX;
			}
			
			//ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			ptr32 = & ( VRAM32 [ StartX + ( Line * FrameBufferWidth_Pixels ) ] );
			
			if ( StartX == EndX && !dxdy [ 0 ] )
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
			

			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across < EndX; x_across++ )
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
				// read pixel from frame buffer if we need to check mask bit
				/*
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
				*/
				*ptr32 = bgr;
				
				//ptr++;
				ptr32 += c_iVectorSize;
			}
			
		}
		
		/////////////////////////////////////
		// update x on left and right
		//x_left += dx_left;
		//x_right += dx_right;
		x [ 0 ] += dxdy [ 0 ];
		x [ 1 ] += dxdy [ 1 ];
	}

}




void GPU::DrawTriangle_Gradient32 ()
{
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;

	//u32 Pixel, TexelIndex,
	
	//u32 Y1_OnLeft;
	
	//u32 color_add;
	
	//u16 *ptr;
	
	s64 Temp;
	s64 LeftMostX, RightMostX;
	
	s32 StartX, EndX, StartY, EndY;

	s64 Error_Left;
	
	s64* DitherArray;
	s64* DitherLine;
	s64 DitherValue;
	
	s32 iR, iG, iB;
	s32 x0, y0, x1, y1, x2, y2;
	s64 x [ 2 ], dxdy [ 2 ];
	u32 r [ 2 ], g [ 2 ], b [ 2 ];
	s32 drdy [ 2 ], dgdy [ 2 ], dbdy [ 2 ];
	u32 bgr, bgr_temp;
	u32 *ptr32;
	u32* VRAM32;
	
	u32 NumberOfPixelsDrawn;
	s32 Line, x_across;
	
	s32 drdx, dgdx, dbdx;
	
	u32 Coord0 = 0, Coord1 = 1, Coord2 = 2;
	u32 X1Index = 0, X0Index = 1;
	
	s64 Red, Green, Blue;
	u32 DestPixel, PixelMask = 0, SetPixelMask = 0;
	
	//if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	//if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
	
	// initialize number of pixels drawn
	NumberOfPixelsDrawn = 0;
	
	
	///////////////////////////////////////////////////
	// Initialize dithering
	/*
	DitherArray = c_iDitherZero;
	
	if ( GPU_CTRL_Read.DTD )
	{
		//DitherArray = c_iDitherValues;
		DitherArray = c_iDitherValues24;
	}
	*/
	
	/*
	//////////////////////////////////////////
	// get coordinates on screen
	x0 = DrawArea_OffsetX + x0;
	y0 = DrawArea_OffsetY + y0;
	x1 = DrawArea_OffsetX + x1;
	y1 = DrawArea_OffsetY + y1;
	x2 = DrawArea_OffsetX + x2;
	y2 = DrawArea_OffsetY + y2;
	*/
	
	
	
	////////////////////////////////
	// y1 starts on the left
	//Y1_OnLeft = 1;
	/*
	r0 = bgr0 & 0xff;
	r1 = bgr1 & 0xff;
	r2 = bgr2 & 0xff;
	g0 = ( bgr0 >> 8 ) & 0xff;
	g1 = ( bgr1 >> 8 ) & 0xff;
	g2 = ( bgr2 >> 8 ) & 0xff;
	b0 = ( bgr0 >> 16 ) & 0xff;
	b1 = ( bgr1 >> 16 ) & 0xff;
	b2 = ( bgr2 >> 16 ) & 0xff;
	*/
	
	
	///////////////////////////////////
	// put top coordinates in x0,y0
	VRAM32 = & ( RAM32 [ FrameBufferStartOffset32 ] );
	
	// get the color for the triangle
	//bgr = rgbaq [ 0 ].Value & 0xffffffffULL;
	
	// initialize number of pixels drawn
	NumberOfPixelsDrawn = 0;
	
	///////////////////////////////////
	// put top coordinates in x0,y0
	//if ( y1 < y0 )
	if ( xyz [ Coord1 ].Y < xyz [ Coord0 ].Y )
	{
		Swap ( Coord0, Coord1 );
	}
	
	//if ( y2 < y0 )
	if ( xyz [ Coord2 ].Y < xyz [ Coord0 ].Y )
	{
		Swap ( Coord2, Coord0 );
	}
	
	///////////////////////////////////////
	// put middle coordinates in x1,y1
	//if ( y2 < y1 )
	if ( xyz [ Coord2 ].Y < xyz [ Coord1 ].Y )
	{
		Swap ( Coord2, Coord1 );
	}
	
	// get x,y
	x0 = xyz [ Coord0 ].X;
	y0 = xyz [ Coord0 ].Y;
	x1 = xyz [ Coord1 ].X;
	y1 = xyz [ Coord1 ].Y;
	x2 = xyz [ Coord2 ].X;
	y2 = xyz [ Coord2 ].Y;
	
#if defined INLINE_DEBUG_TRIANGLE || defined INLINE_DEBUG_PRIMITIVE
	debug << hex << " bgr=" << bgr;
	debug << dec << "; Coords: x0=" << ( x0 >> 4 ) << " y0=" << ( y0 >> 4 ) << " x1=" << ( x1 >> 4 ) << " y1=" << ( y1 >> 4 );
#endif

	//////////////////////////////////////////
	// get coordinates on screen
	// *note* this is different from PS1, where you would add the offsets..
	x0 -= Coord_OffsetX;
	y0 -= Coord_OffsetY;
	x1 -= Coord_OffsetX;
	y1 -= Coord_OffsetY;
	x2 -= Coord_OffsetX;
	y2 -= Coord_OffsetY;
	
	
	// coords are in 12.4 fixed point
	x0 >>= 4;
	y0 >>= 4;
	x1 >>= 4;
	y1 >>= 4;
	x2 >>= 4;
	y2 >>= 4;
	
	// get the left/right most x
	LeftMostX = ( ( x0 < x1 ) ? x0 : x1 );
	LeftMostX = ( ( x2 < LeftMostX ) ? x2 : LeftMostX );
	RightMostX = ( ( x0 > x1 ) ? x0 : x1 );
	RightMostX = ( ( x2 > RightMostX ) ? x2 : RightMostX );

	// check if triangle is within draw area
	//if ( RightMostX <= ((s32)DrawArea_TopLeftX) || LeftMostX > ((s32)DrawArea_BottomRightX) || y2 <= ((s32)DrawArea_TopLeftY) || y0 > ((s32)DrawArea_BottomRightY) ) return;
	if ( RightMostX <= Window_XLeft || LeftMostX > Window_XRight || y2 <= Window_YTop || y0 > Window_YBottom ) return;
#if defined INLINE_DEBUG_TRIANGLE_TEST || defined INLINE_DEBUG_PRIMITIVE_TEST
	else
	{
		debug << dec << "\r\nDrawTriangleGradient";
		debug << dec << " FinalCoords: x0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2;
		debug << hex << " rgba0=" << ( rgbaq [ Coord0 ].Value & 0xffffffff ) << " rgba1=" << ( rgbaq [ Coord1 ].Value & 0xffffffff ) << " rgba2=" << ( rgbaq [ Coord2 ].Value & 0xffffffff );
		debug << hex << "; FrameBufPtr32/64=" << ( FrameBufferStartOffset32 >> 6 );
		debug << " FrameBufPixFmt=" << PixelFormat_Names [ FrameBuffer_PixelFormat ];
	}
#endif
	
	// skip drawing if distance between vertices is greater than max allowed by GPU
	//if ( ( _Abs( x1 - x0 ) > c_MaxPolygonWidth ) || ( _Abs( x2 - x1 ) > c_MaxPolygonWidth ) || ( y1 - y0 > c_MaxPolygonHeight ) || ( y2 - y1 > c_MaxPolygonHeight ) )
	//{
	//	// skip drawing polygon
	//	return;
	//}
	
	
	
	/////////////////////////////////////////////////
	// draw top part of triangle
	
	// denominator is negative when x1 is on the left, positive when x1 is on the right
	s64 t0 = y1 - y2;
	s64 t1 = y0 - y2;
	s64 denominator = ( ( x0 - x2 ) * t0 ) - ( ( x1 - x2 ) * t1 );
	
	// check if x1 is on left or right //
	if ( denominator > 0 )
	{
		// x1 is on the right //
		X1Index = 1;
		X0Index = 0;
	}
	
	// calculate across
	if ( denominator )
	{
		//drdx = ( ( (s64) ( ( ( r0 - r2 ) * t0 ) - ( ( r1 - r2 ) * t1 ) ) ) << 24 ) / denominator;
		//dgdx = ( ( (s64) ( ( ( g0 - g2 ) * t0 ) - ( ( g1 - g2 ) * t1 ) ) ) << 24 ) / denominator;
		//dbdx = ( ( (s64) ( ( ( b0 - b2 ) * t0 ) - ( ( b1 - b2 ) * t1 ) ) ) << 24 ) / denominator;
		drdx = ( ( ( ( (s64) ( rgbaq [ Coord0 ].R - rgbaq [ Coord2 ].R ) ) * t0 ) - ( ( (s64) ( rgbaq [ Coord1 ].R - rgbaq [ Coord2 ].R ) ) * t1 ) ) << 24 ) / denominator;
		dgdx = ( ( ( ( (s64) ( rgbaq [ Coord0 ].G - rgbaq [ Coord2 ].G ) ) * t0 ) - ( ( (s64) ( rgbaq [ Coord1 ].G - rgbaq [ Coord2 ].G ) ) * t1 ) ) << 24 ) / denominator;
		dbdx = ( ( ( ( (s64) ( rgbaq [ Coord0 ].B - rgbaq [ Coord2 ].B ) ) * t0 ) - ( ( (s64) ( rgbaq [ Coord1 ].B - rgbaq [ Coord2 ].B ) ) * t1 ) ) << 24 ) / denominator;
	}
	
	//debug << dec << "\r\nx0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2;
	//debug << "\r\nfixed: denominator=" << denominator;
	
	/////////////////////////////////////////////////
	// draw top part of triangle
	
	// need to set the x0 index unconditionally
	x [ X0Index ] = ( ((s64)x0) << 32 );
	
	r [ X0Index ] = ( rgbaq [ Coord0 ].R << 24 );
	g [ X0Index ] = ( rgbaq [ Coord0 ].G << 24 );
	b [ X0Index ] = ( rgbaq [ Coord0 ].B << 24 );
	
	if ( y1 - y0 )
	{
		// triangle is pointed on top //
		
		/////////////////////////////////////////////
		// init x on the left and right
		//x_left = ( ((s64)x0) << 32 );
		//x_right = x_left;
		x [ X1Index ] = x [ X0Index ];
		
		r [ X1Index ] = r [ X0Index ];
		g [ X1Index ] = g [ X0Index ];
		b [ X1Index ] = b [ X0Index ];
		
		//dx_left = (((s64)( x1 - x0 )) << 32 ) / ((s64)( y1 - y0 ));
		//dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
		dxdy [ X1Index ] = (((s64)( x1 - x0 )) << 32 ) / ((s64)( y1 - y0 ));
		dxdy [ X0Index ] = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
		
		drdy [ X1Index ] = (((s32)( rgbaq [ Coord1 ].R - rgbaq [ Coord0 ].R )) << 24 ) / ((s32)( y1 - y0 ));
		dgdy [ X1Index ] = (((s32)( rgbaq [ Coord1 ].G - rgbaq [ Coord0 ].G )) << 24 ) / ((s32)( y1 - y0 ));
		dbdy [ X1Index ] = (((s32)( rgbaq [ Coord1 ].B - rgbaq [ Coord0 ].B )) << 24 ) / ((s32)( y1 - y0 ));
		
		drdy [ X0Index ] = (((s32)( rgbaq [ Coord2 ].R - rgbaq [ Coord0 ].R )) << 24 ) / ((s32)( y2 - y0 ));
		dgdy [ X0Index ] = (((s32)( rgbaq [ Coord2 ].G - rgbaq [ Coord0 ].G )) << 24 ) / ((s32)( y2 - y0 ));
		dbdy [ X0Index ] = (((s32)( rgbaq [ Coord2 ].B - rgbaq [ Coord0 ].B )) << 24 ) / ((s32)( y2 - y0 ));
	}
	else
	{
		// Triangle is flat on top //
		
		// change x_left and x_right where y1 is on left
		//x_left = ( ((s64)x1) << 32 );
		//x_right = ( ((s64)x0) << 32 );
		x [ X1Index ] = ( ((s64)x1) << 32 );
		
		r [ X1Index ] = ( rgbaq [ Coord1 ].R << 24 );
		g [ X1Index ] = ( rgbaq [ Coord1 ].G << 24 );
		b [ X1Index ] = ( rgbaq [ Coord1 ].B << 24 );
		
		// this means that x0 and x1 are on the same line
		// so if the height of the entire polygon is zero, then we are done
		if ( y2 - y1 )
		{
			//dx_left = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			//dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
			dxdy [ X1Index ] = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			dxdy [ X0Index ] = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
			
			// only need to set dr,dg,db for the x0/x2 side here
			drdy [ X0Index ] = (((s32)( rgbaq [ Coord2 ].R - rgbaq [ Coord0 ].R )) << 24 ) / ((s32)( y2 - y0 ));
			dgdy [ X0Index ] = (((s32)( rgbaq [ Coord2 ].G - rgbaq [ Coord0 ].G )) << 24 ) / ((s32)( y2 - y0 ));
			dbdy [ X0Index ] = (((s32)( rgbaq [ Coord2 ].B - rgbaq [ Coord0 ].B )) << 24 ) / ((s32)( y2 - y0 ));
		}
	}
	
	
	
	/*
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
	*/
	
	
	
	StartY = y0;
	EndY = y1;

	if ( StartY < Window_YTop )
	{
		
		if ( EndY < Window_YTop )
		{
			Temp = EndY - StartY;
			StartY = EndY;
		}
		else
		{
			Temp = Window_YTop - StartY;
			StartY = Window_YTop;
		}
		
		//x_left += dx_left * Temp;
		//x_right += dx_right * Temp;
		x [ 0 ] += dxdy [ 0 ] * Temp;
		x [ 1 ] += dxdy [ 1 ] * Temp;
		
		//R_left += dR_left * Temp;
		//G_left += dG_left * Temp;
		//B_left += dB_left * Temp;
		r [ 0 ] += drdy [ 0 ] * Temp;
		g [ 0 ] += dgdy [ 0 ] * Temp;
		b [ 0 ] += dbdy [ 0 ] * Temp;
		
		//R_right += dR_right * Temp;
		//G_right += dG_right * Temp;
		//B_right += dB_right * Temp;
		r [ 1 ] += drdy [ 1 ] * Temp;
		g [ 1 ] += dgdy [ 1 ] * Temp;
		b [ 1 ] += dbdy [ 1 ] * Temp;
	}
	
	if ( EndY > Window_YBottom )
	{
		EndY = Window_YBottom + 1;
	}

	
	//////////////////////////////////////////////
	// draw down to y1
	for ( Line = StartY; Line < EndY; Line++ )
	{
		//StartX = _Round( x_left ) >> 32;
		//EndX = _Round( x_right ) >> 32;
		StartX = _Round( x [ 0 ] ) >> 32;
		EndX = _Round( x [ 1 ] ) >> 32;
		
		if ( StartX <= Window_XRight && EndX > Window_XLeft )
		{
			//iR = R_left;
			//iG = G_left;
			//iB = B_left;
			iR = r [ 0 ];
			iG = g [ 0 ];
			iB = b [ 0 ];
			
			if ( StartX < Window_XLeft )
			{
				Temp = Window_XLeft - StartX;
				StartX = Window_XLeft;
				
				//iR += dR_across * Temp;
				//iG += dG_across * Temp;
				//iB += dB_across * Temp;
				iR += drdx * Temp;
				iG += dgdx * Temp;
				iB += dbdx * Temp;
			}
			
			if ( EndX > Window_XRight )
			{
				EndX = Window_XRight + 1;
				//EndX = DrawArea_BottomRightX;
			}
			
			//ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			//DitherLine = & ( DitherArray [ ( Line & 0x3 ) << 2 ] );
			ptr32 = & ( VRAM32 [ StartX + ( Line * FrameBufferWidth_Pixels ) ] );
			
			if ( StartX == EndX && !dxdy [ 0 ] ) EndX += 1;
			//if ( EndX >= RightMostX ) EndX--;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;
			


			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across < EndX; x_across++ )
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
					/*
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
					*/
					
					Red = iR >> 24;
					Green = iG >> 24;
					Blue = iB >> 24;
					
					//bgr = ( Blue << 10 ) | ( Green << 5 ) | Red;
					bgr = ( Blue << 16 ) | ( Green << 8 ) | Red;
					
					// shade pixel color
				
					/*
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
					*/

					// *** testing ***
					//debug << " (before)bgr_temp=" << hex << bgr_temp;
					//debug << " SetPixelMask=" << SetPixelMask << " PixelMask=" << PixelMask << " DestPixel=" << DestPixel;
					
					// draw pixel if we can draw to mask pixels or mask bit not set
					//if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
					*ptr32 = bgr;
						
					//iR += dR_across;
					//iG += dG_across;
					//iB += dB_across;
					iR += drdx;
					iG += dgdx;
					iB += dbdx;
				
				//ptr++;
				ptr32 += c_iVectorSize;
			}
			
		}
		
		//////////////////////////////////
		// draw next line
		//Line++;
		
		/////////////////////////////////////
		// update x on left and right
		//x_left += dx_left;
		//x_right += dx_right;
		x [ 0 ] += dxdy [ 0 ];
		x [ 1 ] += dxdy [ 1 ];
		
		//R_left += dR_left;
		//G_left += dG_left;
		//B_left += dB_left;
		//R_right += dR_right;
		//G_right += dG_right;
		//B_right += dB_right;
		r [ 0 ] += drdy [ 0 ];
		g [ 0 ] += dgdy [ 0 ];
		b [ 0 ] += dbdy [ 0 ];
		r [ 1 ] += drdy [ 1 ];
		g [ 1 ] += dgdy [ 1 ];
		b [ 1 ] += dbdy [ 1 ];
	}

	
	////////////////////////////////////////////////
	// draw bottom part of triangle

	/////////////////////////////////////////////
	// init x on the left and right
	
	// set values on the x1 side
	x [ X1Index ] = ( ((s64)x1) << 32 );
	
	r [ X1Index ] = ( rgbaq [ Coord1 ].R << 24 );
	g [ X1Index ] = ( rgbaq [ Coord1 ].G << 24 );
	b [ X1Index ] = ( rgbaq [ Coord1 ].B << 24 );
	
	if ( y2 - y1 )
	{
		// triangle is pointed on the bottom //
		dxdy [ X1Index ] = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
		
		drdy [ X1Index ] = (((s64)( rgbaq [ Coord2 ].R - rgbaq [ Coord1 ].R )) << 24 ) / ((s64)( y2 - y1 ));
		dgdy [ X1Index ] = (((s64)( rgbaq [ Coord2 ].G - rgbaq [ Coord1 ].G )) << 24 ) / ((s64)( y2 - y1 ));
		dbdy [ X1Index ] = (((s64)( rgbaq [ Coord2 ].B - rgbaq [ Coord1 ].B )) << 24 ) / ((s64)( y2 - y1 ));
	}
	
	/*
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
	*/
	
	// *** testing ***
	//debug << "\r\nfixed: dR_across=" << dR_across << " dG_across=" << dG_across << " dB_across=" << dB_across;
	
	// the line starts at y1 from here
	//Line = y1;

	StartY = y1;
	EndY = y2;

	if ( StartY < Window_YTop )
	{
		
		if ( EndY < Window_YTop )
		{
			Temp = EndY - StartY;
			StartY = EndY;
		}
		else
		{
			Temp = Window_YTop - StartY;
			StartY = Window_YTop;
		}
		
		//x_left += dx_left * Temp;
		//x_right += dx_right * Temp;
		x [ 0 ] += dxdy [ 0 ] * Temp;
		x [ 1 ] += dxdy [ 1 ] * Temp;
		
		//R_left += dR_left * Temp;
		//G_left += dG_left * Temp;
		//B_left += dB_left * Temp;
		//R_right += dR_right * Temp;
		//G_right += dG_right * Temp;
		//B_right += dB_right * Temp;
		
		r [ 0 ] += drdy [ 0 ] * Temp;
		g [ 0 ] += dgdy [ 0 ] * Temp;
		b [ 0 ] += dbdy [ 0 ] * Temp;
		
		r [ 1 ] += drdy [ 1 ] * Temp;
		g [ 1 ] += dgdy [ 1 ] * Temp;
		b [ 1 ] += dbdy [ 1 ] * Temp;
	}
	
	if ( EndY > Window_YBottom )
	{
		EndY = Window_YBottom + 1;
	}

	
	//////////////////////////////////////////////
	// draw down to y2
	for ( Line = StartY; Line < EndY; Line++ )
	{
		//StartX = _Round( x_left ) >> 32;
		//EndX = _Round( x_right ) >> 32;
		StartX = _Round( x [ 0 ] ) >> 32;
		EndX = _Round( x [ 1 ] ) >> 32;
		
		if ( StartX <= Window_XRight && EndX > Window_XLeft )
		{
			
			//iR = R_left;
			//iG = G_left;
			//iB = B_left;
			iR = r [ 0 ];
			iG = g [ 0 ];
			iB = b [ 0 ];
			
			if ( StartX < Window_XLeft )
			{
				Temp = Window_XLeft - StartX;
				StartX = Window_XLeft;
				
				//iR += dR_across * Temp;
				//iG += dG_across * Temp;
				//iB += dB_across * Temp;
				iR += drdx * Temp;
				iG += dgdx * Temp;
				iB += dbdx * Temp;
			}
			
			if ( EndX > Window_XRight )
			{
				EndX = Window_XRight + 1;
				//EndX = DrawArea_BottomRightX;
			}
			
			//ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			//DitherLine = & ( DitherArray [ ( Line & 0x3 ) << 2 ] );
			ptr32 = & ( VRAM32 [ StartX + ( Line * FrameBufferWidth_Pixels ) ] );
			
			if ( StartX == EndX && !dxdy [ 0 ] ) EndX += 1;
			//if ( EndX >= RightMostX ) EndX--;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;
			

			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across < EndX; x_across++ )
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
					/*
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
					*/
					
					Red = iR >> 24;
					Green = iG >> 24;
					Blue = iB >> 24;
					
					//bgr = ( Blue << 10 ) | ( Green << 5 ) | Red;
					bgr = ( Blue << 16 ) | ( Green << 8 ) | Red;
					
					// shade pixel color
					/*
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
					*/

					// draw pixel if we can draw to mask pixels or mask bit not set
					//if ( ! ( DestPixel & PixelMask ) ) *ptr = bgr_temp;
					*ptr32 = bgr;
					
				//iR += dR_across;
				//iG += dG_across;
				//iB += dB_across;
				iR += drdx;
				iG += dgdx;
				iB += dbdx;
				
				//ptr++;
				ptr32 += c_iVectorSize;
			}
			
		}
		
		/////////////////////////////////////
		// update x on left and right
		//x_left += dx_left;
		//x_right += dx_right;
		x [ 0 ] += dxdy [ 0 ];
		x [ 1 ] += dxdy [ 1 ];
		
		//R_left += dR_left;
		//G_left += dG_left;
		//B_left += dB_left;
		//R_right += dR_right;
		//G_right += dG_right;
		//B_right += dB_right;
		r [ 0 ] += drdy [ 0 ];
		g [ 0 ] += dgdy [ 0 ];
		b [ 0 ] += dbdy [ 0 ];
		r [ 1 ] += drdy [ 1 ];
		g [ 1 ] += dgdy [ 1 ];
		b [ 1 ] += dbdy [ 1 ];
	}
		
}














// transfer functions //

void GPU::TransferDataIn32 ( u32* Data, u32 WordCount32 )
{
#ifdef INLINE_DEBUG_TRANSFER_IN
	if ( !XferX && !XferY )
	{
		debug << "\r\nTransferIn: ";
		debug << dec << " WC=" << WordCount32;
		debug << dec << " Width=" << XferWidth << " Height=" << XferHeight;
		debug << hex << " DESTPTR32/64=" << GPURegsGp.BITBLTBUF.DBP;
		//debug << " InPixFmt=" << PixelFormat_Names [ GPURegsGp.BITBLTBUF.SPSM ];
		debug << dec << " DestBufWidth=" << XferDstBufWidth;
		debug << " OutPixFmt=" << PixelFormat_Names [ GPURegsGp.BITBLTBUF.DPSM ];
		debug << " TransferDir=" << TransferDir_Names [ GPURegsGp.TRXPOS.DIR ];
		debug << dec << " XferX=" << XferX << " XferY=" << XferY;
		debug << dec << " XferDstX=" << XferDstX << " XferDstY=" << XferDstY;
		debug << " @Cycle#" << dec << *_DebugCycleCount;
	}
#endif

#ifdef INLINE_DEBUG_TRANSFER_IN_2
	debug << "\r\n";
	debug << " XferX=" << XferX << " XferY=" << XferY;
	//debug << " XferDstX=" << XferDstX << " XferDstY=" << XferDstY;
#endif

	//u64 PixelShift
	u64 PixelLoad;
	u32 Pixel0;
	//u64 PixelCount;

	// get pointer to dest buffer
	//u32 *DestBuffer;
	
	u32 *Data32;
	u16 *Data16;
	u8 *Data8;
	
	u32 *buf32;
	u16 *buf16;
	
	u32 *DestBuffer32;
	u16 *DestBuffer16;
	u8 *DestBuffer8;
	
	// check that there is data to transfer
	if ( !XferWidth || !XferHeight ) return;
	
	// if the X specified is greater than buffer width, then modulo
	if ( XferDstX >= XferDstBufWidth ) XferDstX %= XferDstBufWidth;
	
	buf32 = & ( RAM32 [ XferDstOffset32 ] );
	buf16 = (u16*) buf32;
	
	// check if source pixel format is 24-bit
	// this is transferring TO GPU, so must check the destination for the correct value
	if ( ( GPURegsGp.BITBLTBUF.DPSM & 7 ) == 1 )
	{
		// 24-bit pixels //
		
		DestBuffer32 = & ( buf32 [ ( XferX + XferDstX ) + ( ( XferY + XferDstY ) * XferDstBufWidth ) ] );
		Data32 = Data;
		
		//PixelCount = 0;
		//PixelShift = 0;
		
		while ( ( XferY < XferHeight ) && WordCount32 )
		{
			// check if you need to load
			if ( PixelCount < 3 )
			{
				// load next data
				PixelLoad = *Data32++;
				
				WordCount32--;
				
				// put into pixel
				PixelShift |= ( PixelLoad << ( PixelCount << 3 ) );
				
				PixelCount += 4;
			}
				
			// get pixel
			Pixel0 = PixelShift & 0xffffff;
				
			// transfer pixel
			*DestBuffer32++ = Pixel0;
			
			// shift (24-bit pixels)
			PixelShift >>= 24;
			
			// update pixel count
			PixelCount -= 3;
			
			// update x
			XferX++;
			
			// check if greater than width
			if ( XferX >= XferWidth )
			{
				// go to next line
				XferX = 0;
				XferY++;
				
				// set buffer pointer
				DestBuffer32 = & ( buf32 [ ( XferX + XferDstX ) + ( ( XferY + XferDstY ) * XferDstBufWidth ) ] );
			}
			
			if ( ( XferX + XferDstX ) == XferDstBufWidth )
			{
				// wrap around
				DestBuffer32 = & ( buf32 [ ( ( XferY + XferDstY ) * XferDstBufWidth ) ] );
			}
			
		}
	}
	else
	{
		if ( ( GPURegsGp.BITBLTBUF.DPSM & 7 ) == 0 )
		{
			// 32-bit pixels
			DestBuffer32 = & ( buf32 [ ( XferX + XferDstX ) + ( ( XferY + XferDstY ) * XferDstBufWidth ) ] );
			Data32 = Data;
			
			while ( ( XferY < XferHeight ) && WordCount32 )
			{
			
				// transfer a pixel
				*DestBuffer32++ = *Data32++;
				
				// update x
				XferX++;
				
				
				// check if greater than width
				if ( XferX >= XferWidth )
				{
					// go to next line
					XferX = 0;
					XferY++;
					
					// set buffer pointer
					DestBuffer32 = & ( buf32 [ ( XferX + XferDstX ) + ( ( XferY + XferDstY ) * XferDstBufWidth ) ] );
				}
				
				if ( ( XferX + XferDstX ) == XferDstBufWidth )
				{
					// wrap around
					DestBuffer32 = & ( buf32 [ ( ( XferY + XferDstY ) * XferDstBufWidth ) ] );
				}
				
				// don't go past the number of pixels available to read
				WordCount32--;
			}
		}
		else if ( ( GPURegsGp.BITBLTBUF.DPSM & 7 ) == 2 )
		{
			// 16-bit pixels //
			DestBuffer16 = & ( buf16 [ ( XferX + XferDstX ) + ( ( XferY + XferDstY ) * XferDstBufWidth ) ] );
			Data16 = (u16*) Data;
			
			// 2 times the pixels
			WordCount32 <<= 1;
			
			while ( ( XferY < XferHeight ) && WordCount32 )
			{
			
				// transfer a pixel
				*DestBuffer16++ = *Data16++;
				
				// update x
				XferX++;
				
				
				// check if greater than width
				if ( XferX >= XferWidth )
				{
					// go to next line
					XferX = 0;
					XferY++;
					
					// set buffer pointer
					DestBuffer16 = & ( buf16 [ ( XferX + XferDstX ) + ( ( XferY + XferDstY ) * XferDstBufWidth ) ] );
				}
				
				if ( ( XferX + XferDstX ) >= XferDstBufWidth )
				{
					// wrap around
					DestBuffer16 = & ( buf16 [ ( ( XferY + XferDstY ) * XferDstBufWidth ) ] );
				}
				
				// don't go past the number of pixels available to read
				WordCount32--;
			}
		}
		else if ( ( GPURegsGp.BITBLTBUF.DPSM & 7 ) == 3 )
		{
			// 8-bit pixels //
			DestBuffer8 = & ( RAM8 [ ( XferDstOffset32 << 2 ) + ( XferX + XferDstX ) + ( ( XferY + XferDstY ) * XferDstBufWidth ) ] );
			Data8 = (u8*) Data;
			
			// 4 times the pixels
			WordCount32 <<= 2;
			
			while ( ( XferY < XferHeight ) && WordCount32 )
			{
			
				// transfer a pixel
				*DestBuffer8++ = *Data8++;
				
				// update x
				XferX++;
				
				
				// check if greater than width
				if ( XferX >= XferWidth )
				{
					// go to next line
					XferX = 0;
					XferY++;
					
					// set buffer pointer
					DestBuffer8 = & ( RAM8 [ ( XferDstOffset32 << 2 ) + ( XferX + XferDstX ) + ( ( XferY + XferDstY ) * XferDstBufWidth ) ] );
				}
				
				if ( ( XferX + XferDstX ) == XferDstBufWidth )
				{
					// wrap around
					DestBuffer8 = & ( RAM8 [ ( XferDstOffset32 << 2 ) + ( ( XferY + XferDstY ) * XferDstBufWidth ) ] );
				}
				
				// don't go past the number of pixels available to read
				WordCount32--;
			}
		}
		else if ( ( GPURegsGp.BITBLTBUF.DPSM & 7 ) == 4 )
		{
			// 4-bit pixels //
			DestBuffer8 = & ( RAM8 [ ( XferDstOffset32 << 2 ) + ( XferX + XferDstX ) + ( ( XferY + XferDstY ) * ( XferDstBufWidth >> 1 ) ) ] );
			Data8 = (u8*) Data;
			
			// 8 times the pixels, but transferring 2 at a time, so times 4
			WordCount32 <<= 2;
			
			while ( ( XferY < XferHeight ) && WordCount32 )
			{
			
				// transfer a pixel
				*DestBuffer8++ = *Data8++;
				
				// update x
				XferX++;
				XferX++;
				
				// check if greater than width
				if ( XferX >= XferWidth )
				{
					// go to next line
					XferX = 0;
					XferY++;
					
					// set buffer pointer
					DestBuffer8 = & ( RAM8 [ ( XferDstOffset32 << 2 ) + ( XferX + XferDstX ) + ( ( XferY + XferDstY ) * ( XferDstBufWidth >> 1 ) ) ] );
				}
				
				if ( ( XferX + XferDstX ) == XferDstBufWidth )
				{
					// wrap around
					DestBuffer8 = & ( RAM8 [ ( XferDstOffset32 << 2 ) + ( ( XferY + XferDstY ) * ( XferDstBufWidth >> 1 ) ) ] );
				}
				
				// don't go past the number of pixels available to read
				WordCount32--;
			}
		}
	}
}


void GPU::TransferDataOut ()
{
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void GPU::DebugWindow_Enable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	static const char* DebugWindow_Caption = "PS2 FrameBuffers Debug Window";
	static const int DebugWindow_X = 10;
	static const int DebugWindow_Y = 10;
	static const int DebugWindow_Width = c_iFrameBuffer_DisplayWidth;
	static const int DebugWindow_Height = c_iFrameBuffer_DisplayHeight;
	
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
		_GPU->Draw_FrameBuffers ();
	}
	
#endif

}


