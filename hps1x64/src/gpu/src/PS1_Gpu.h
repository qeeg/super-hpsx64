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


#ifndef _PS1_GPU_H_
#define _PS1_GPU_H_

#include "types.h"
#include "Debug.h"

#include "WinApiHandler.h"
#include "GNUAsmUtility_x64.h"


//#define DRAW_MULTIPLE_PIXELS

#include "emmintrin.h"

#define _ENABLE_SSE2_TRIANGLE_MONO
#define _ENABLE_SSE2_TRIANGLE_GRADIENT
#define _ENABLE_SSE2_RECTANGLE_MONO


#ifdef _ENABLE_SSE2

// need to include this file to use SSE2 intrinsics
#include "emmintrin.h"
//#include "smmintrin.h"

#endif


using namespace x64Asm::Utilities;


namespace Playstation1
{

	class GPU
	{
	
		static Debug::Log debug;
		static GPU *_GPU;
		
		static WindowClass::Window *DisplayOutput_Window;
		static WindowClass::Window *FrameBuffer_DebugWindow;
	
	public:
		// GPU input buffer is 64 bytes (16 words)
	
		//////////////////////////
		//	General Parameters	//
		//////////////////////////
		
		// where the registers start at
		static const long Regs_Start = 0x1f801810;
		
		// where the registers end at
		static const long Regs_End = 0x1f801814;
	
		// distance between groups of registers
		static const long Reg_Size = 0x4;
		
		// need to pack lots of info into the structure for debugging and read/write of hardware registers
		struct HW_Register
		{
			bool ReadOK;
			bool WriteOK;
			bool Unknown;
			char* Name;
			u32 Address;
			u32 SizeInBytes;
			u32* DataPtr;
		};
		
		HW_Register Registers [ Regs_End - Regs_Start + Reg_Size ];

		
		// GPU Clock Speed in Hertz
		static const long c_iClockSpeed = 53222400;
		static const double c_dClockSpeed = 53222400.0L;

		// the number of gpu cycles for every cpu cycle or vice versa
		static const double c_dGPUPerCPU_Cycles = ( 11.0L / 7.0L );
		static const double c_dCPUPerGPU_Cycles = ( 7.0L / 11.0L );
		
		static const u64 c_BIAS = ( 1ULL << 31 );
		static const u64 c_BIAS24 = ( 1ULL << 23 );
		static inline s64 _Round ( s64 FixedPointValue )
		{
			return ( FixedPointValue + c_BIAS );
		}
		
		static inline s64 _Round24 ( s64 FixedPointValue )
		{
			return ( FixedPointValue + c_BIAS24 );
		}
		
		static inline u64 _Abs ( s64 Value )
		{
			return ( ( Value >> 63 ) ^ Value ) - ( Value >> 63 );
		}
		
		static inline u32 _Abs ( s32 Value )
		{
			return ( ( Value >> 31 ) ^ Value ) - ( Value >> 31 );
		}
		
		// cycle that the next event will happen at for this device
		u64 NextEvent_Cycle, NextEvent_Cycle_Vsync;

		// the cycle that device is busy until
		u64 BusyUntil_Cycle;

		// used to buffer pixels before drawing to the screen
		u32 PixelBuffer [ 1024 * 512 ] __attribute__ ((aligned (32)));
		
		// size of the main program window
		static u32 MainProgramWindow_Width;
		static u32 MainProgramWindow_Height;
		
		// maximum width/height of a polygon allowed
		static const s32 c_MaxPolygonWidth = 1023;
		static const s32 c_MaxPolygonHeight = 511;

		static u32 Read ( u32 Address );
		static void Write ( u32 Address, u32 Data, u32 Mask );
		
		void DMA_Read ( u32* Data, int ByteReadCount );
		void DMA_Write ( u32* Data, int ByteWriteCount );
		void DMA_WriteBlock ( u32* Data, u32 BS );
		
		void Start ();

		// returns either vblank interrupt signal, gpu interrupt signal, or no interrupt signal
		void Run ();
		void Reset ();
		
		// need to specify what window to display graphical output to (this should be the main program window)
		// *** TODO *** probably also need to call this whenever the main program window size changes
		void SetDisplayOutputWindow ( u32 width, u32 height, WindowClass::Window* DisplayOutput );
		
		void Draw_Screen ();
		void Draw_FrameBuffer ();
		
		double GetCycles_SinceLastPixel ();
		double GetCycles_SinceLastHBlank ();
		double GetCycles_SinceLastVBlank ();
		double GetCycles_ToNextPixel ();
		double GetCycles_ToNextHBlank ();
		double GetCycles_ToNextVBlank ();

		double GetCycles_SinceLastPixel ( double dAtCycle );
		double GetCycles_SinceLastHBlank ( double dAtCycle );
		double GetCycles_SinceLastVBlank ( double dAtCycle );
		double GetCycles_ToNextPixel ( double dAtCycle );
		double GetCycles_ToNextHBlank ( double dAtCycle );
		double GetCycles_ToNextVBlank ( double dAtCycle );
		
		double GetCycles_ToNextScanlineStart ();
		double GetCycles_ToNextFieldStart ();
		double GetCycles_SinceLastScanlineStart ();
		double GetCycles_SinceLastFieldStart ();
		
		double GetCycles_ToNextScanlineStart ( double dAtCycle );
		double GetCycles_ToNextFieldStart ( double dAtCycle );
		double GetCycles_SinceLastScanlineStart ( double dAtCycle );
		double GetCycles_SinceLastFieldStart ( double dAtCycle );
		
		
		// gets the total number of scanlines since start of program
		u64 GetScanline_Count ();
		
		// gets the scanline number you are on (for ntsc that would be 0-524, for pal 0-624)
		u64 GetScanline_Number ();
		
		bool isHBlank ();
		bool isVBlank ();
		
		bool isHBlank ( double dAtCycle );
		bool isVBlank ( double dAtCycle );
		
		// get the next event (vblank)
		void GetNextEvent ();
		void GetNextEvent_V ();
		void Update_NextEvent ();
		
		void SetNextEvent ( u64 CycleOffset );
		void SetNextEvent_Cycle ( u64 Cycle );
		void Update_NextEventCycle ();
		
		

		// get the cycle number that a particular pixel gets hit at
		// x and y are the global pixel x,y
		//u64 Get_PixelCycle ( u64 x, u64 y );
		
		//u64 Get_PixelX ();
		//u64 Get_PixelY ();
		//u64 Get_TotalPixelCount ();
		
		//u64 Count_Pixels ( u64 StartCycle, u64 StopCycle );
		//u64 Count_HBlanks ( u64 StartCycle, u64 StopCycle );
		//u64 Count_VBlanks ( u64 StartCycle, u64 StopCycle );
		
		//u64 GetPixel_Cycle ( s64 Number );
		//u64 GetHBlank_Cycle ( s64 Number );
		//u64 GetVBlank_Cycle ( s64 Number );

		
		//////////////////////////////////
		//	Device Specific Parameters	//
		//////////////////////////////////
		
		static const u32 GPU_VERSION = 2;
		
		// GPU Data/Command register
		static const long GPU_DATA = 0x1f801810;

		
		// this is the timer for syncing to 50/60 fps
		// platform dependent, so will only be here temporarily
		u64 VideoTimerStart, VideoTimerStop;
		
		// cycle timings used for drawing (very important on a PS1)
		
		// *** no alpha blending *** //
		static const double dFrameBufferRectangle_02_CyclesPerPixel = 0.5;
		static const double dMoveImage_80_CyclesPerPixel = 1.0;
		
		static const double dMonoTriangle_20_CyclesPerPixel = 0.5;
		static const double dGradientTriangle_30_CyclesPerPixel = 0.6;
		static const double dTextureTriangle4_24_CyclesPerPixel = 0.9;	//3.38688;
		static const double dTextureTriangle8_24_CyclesPerPixel = 1.0;	//6.77376;
		static const double dTextureTriangle16_24_CyclesPerPixel = 1.1;	//11.2896;
		static const double dTextureTriangle4_34_Gradient_CyclesPerPixel = 1.0;	//4.8384;
		static const double dTextureTriangle8_34_Gradient_CyclesPerPixel = 1.1;	//9.6768;
		static const double dTextureTriangle16_34_Gradient_CyclesPerPixel = 1.2;	//18.816;
		static const double dSprite4_64_Cycles = 1.0;
		static const double dSprite8_64_Cycles = 1.1;
		static const double dSprite16_64_Cycles = 1.2;
		static const double dCyclesPerSprite8x8_74_4bit = 64.0;
		static const double dCyclesPerSprite8x8_74_8bit = 64.0;
		static const double dCyclesPerSprite8x8_74_16bit = 64.0;
		static const double dCyclesPerSprite16x16_7c_4bit = 256.0;
		static const double dCyclesPerSprite16x16_7c_8bit = 256.0;
		static const double dCyclesPerSprite16x16_7c_16bit = 256.0;

		// *** alpha blending *** //
		static const double dAlphaBlending_CyclesPerPixel = 0.5;
		
		// *** brightness calculation *** //
		static const double dBrightnessCalculation_CyclesPerPixel = 0.5;
		
		
		// this is the result from a query command sent to the GPU_CTRL register
		u32 GPU_DATA_Read;
		
		union DATA_Write_Format
		{
			// Command | BGR
			struct
			{
				// Color / Shading info
				
				// bits 0-7
				u8 Red;
				
				// bits 8-15
				u8 Green;
				
				// bits 16-23
				u8 Blue;
				
				// the command for the packet
				// bits 24-31
				u8 Command;
			};
			
			// y | x
			struct
			{
				// 16-bit values of y and x in the frame buffer
				// these look like they are signed
				
				// bits 0-10 - x-coordinate
				//s16 x;
				s32 x : 11;
				
				// bits 11-15 - not used
				s32 NotUsed0 : 5;
				
				// bits 16-26 - y-coordinate
				//s16 y;
				s32 y : 11;
				
				// bits 27-31 - not used
				s32 NotUsed1 : 5;
			};
			
			struct
			{
				u8 u;
				u8 v;
				u16 filler11;
			};
			
			// clut | v | u
			struct
			{
				u16 filler13;
				
				struct
				{
					// x-coordinate x/16
					// bits 0-5
					u16 x : 6;
					
					// y-coordinate 0-511
					// bits 6-14
					u16 y : 9;
					
					// bit 15 - Unknown/Unused (should be 0)
					u16 unknown0 : 1;
				};
			} clut;
			
			// h | w
			struct
			{
				u16 w;
				u16 h;
			};
			
			// tpage | v | u
			struct
			{
				// filler for u and v
				u32 filler9 : 16;
				
				// texture page x-coordinate
				// X*64
				// bits 0-3
				u32 tx : 4;

				// texture page y-coordinate
				// 0 - 0; 1 - 256
				// bit 4
				u32 ty : 1;
				
				// Semi-transparency mode
				// 0: 0.5xB+0.5 xF; 1: 1.0xB+1.0 xF; 2: 1.0xB-1.0 xF; 3: 1.0xB+0.25xF
				// bits 5-6
				u32 abr : 2;
				
				// Color mode
				// 0 - 4bit CLUT; 1 - 8bit CLUT; 2 - 15bit direct color; 3 - 15bit direct color
				// bits 7-8
				u32 tp : 2;
				
				// bits 9-10 - Unused
				u32 zero0 : 2;
				
				// bit 11 - same as GP0(E1).bit11 - Texture Disable
				// 0: Normal; 1: Disable if GP1(09h).Bit0=1
				u32 TextureDisable : 1;

				// bits 12-15 - Unused (should be 0)
				u32 Zero0 : 4;
			} tpage;
			
			u32 Value;
				
		};
		
		template<typename T>
		struct t_vector
		{
			T x, y, u, v, r, g, b;
			u32 bgr;
		};

		// make the pointers local
		//t_vector<s32> *pv, *pv0, *pv1, *pv2, *pv3;
		//t_vector<s64> *pdLeft, *pdRight, *pdAcross;
		t_vector<s32> vec, vec0, vec1, vec2, vec3, tvec, tvec0, tvec1, tvec2, tvec3;
		t_vector<s64> dLeft, dRight, dAcross;
		
		// it's actually better to put the x, y values in an array, in case I want to vectorize later
		// but then it takes more effort to sort the coordinates
		//s32 vx [ 4 ], vy [ 4 ], vu [ 4 ], vv [ 4 ];
		
		u32 NumberOfPixelsDrawn;
		s32 x, y, x0, y0, x1, y1, x2, y2, x3, y3;
		u32 w, h, clut_x, clut_y, tpage_tx, tpage_ty, tpage_abr, tpage_tp, command_tge, command_abe, command_abr;
		u32 u3, v3, sX, sY, dX, dY, iX, iY, bgr, bgr0, bgr1, bgr2, bgr3, bgr_temp;
		s32 x_save [ 4 ], y_save [ 4 ];
		u32 bgr_save [ 5 ], u_save [ 4 ], v_save [ 4 ];
		s64 dx_left, dx_right;
		s64 dR_left, dR_right, dG_left, dG_right, dB_left, dB_right, dR_across, dG_across, dB_across, iR, iG, iB, R_left, R_right, G_left, G_right, B_left, B_right;
		s64 dU_left, dU_right, dV_left, dV_right, dU_across, dV_across, U_left, U_right, V_left, V_right, iU, iV;
		s32 r0, r1, r2, g0, g1, g2, b0, b1, b2;
		s32 u, v, u0, v0, u1, v1, u2, v2;
		s64 x_left, x_right, x_across;
		s32 Line;
		u32 ClutOffset;
		
		// to get the modulo needed for windowed graphics drawing, I'll use a lookup-table
		// dimension 1 is twx/twy, dimension #2 is window tww/twh, dimension #3 is value
		//static u8 Modulo_LUT [ 32 ] [ 32 ] [ 256 ];

		//static void Generate_Modulo_LUT ();

		// GPU Status/Control register
		static const long GPU_CTRL = 0x1f801814;

		union GPU_CTRL_Read_Format
		{
			struct
			{
				// bits 0-3
				// Texture page X = tx*64: 0-0;1-64;2-128;3-196;4-...
				u32 TX : 4;

				// bit 4
				// Texture page Y: 0-0;1-256
				u32 TY : 1;

				// bits 5-6
				// Semi-transparent state: 00-0.5xB+0.5xF;01-1.0xB+1.0xF;10-1.0xB-1.0xF;11-1.0xB+0.25xF
				u32 ABR : 2;

				// bits 7-8
				// Texture page color mode: 00-4bit CLUT;01-8bit CLUT; 10-15bit
				u32 TP : 2;

				// bit 9
				// 0-dither off; 1-dither on
				u32 DTD : 1;

				// bit 10
				// 0-Draw to display area prohibited;1-Draw to display area allowed
				// 1 - draw all fields; 0 - only allow draw to display for even fields
				u32 DFE : 1;

				// bit 11
				// 0-off;1-on, apply mask bit to drawn pixels
				u32 MD : 1;

				// bit 12
				// 0-off;1-on, no drawing pixels with mask bit set
				u32 ME : 1;

				// bits 13-15
				//u32 Unknown1 : 3;
				
				// bit 13 - reserved (seems to be always set?)
				u32 Reserved : 1;
				
				// bit 14 - Reverse Flag (0: Normal; 1: Distorted)
				u32 ReverseFlag : 1;
				
				// bit 15 - Texture disable (0: Normal; 1: Disable textures)
				u32 TextureDisable : 1;

				// bits 16-18
				// screen width is: 000-256 pixels;010-320 pixels;100-512 pixels;110-640 pixels;001-368 pixels
				u32 WIDTH : 3;

				// bit 19
				// 0-screen height is 240 pixels; 1-screen height is 480 pixels
				u32 HEIGHT : 1;

				// bit 20
				// 0-NTSC;1-PAL
				u32 VIDEO : 1;

				// bit 21
				// 0- 15 bit direct mode;1- 24-bit direct mode
				u32 ISRGB24 : 1;

				// bit 22
				// 0-Interlace off; 1-Interlace on
				u32 ISINTER : 1;

				// bit 23
				// 0-Display enabled;1-Display disabled
				u32 DEN : 1;

				// bits 24-25
				//u32 Unknown0 : 2;
				
				// bit 24 - Interrupt Request (0: off; 1: on) [GP0(0x1f)/GP1(0x02)]
				u32 IRQ : 1;
				
				// bit 25 - DMA / Data Request
				// When GP1(0x04)=0 -> always zero
				// when GP1(0x04)=1 -> FIFO State (0: full; 1: NOT full)
				// when GP1(0x04)=2 -> Same as bit 28
				// when GP1(0x04)=3 -> same as bit 27
				u32 DataRequest : 1;

				// bit 26
				// Ready to receive CMD word
				// 0: NOT Ready; 1: Ready
				// 0-GPU is busy;1-GPU is idle
				u32 BUSY : 1;

				// bit 27
				// 0-Not ready to send image (packet 0xc0);1-Ready to send image
				u32 IMG : 1;

				// bit 28
				// Ready to receive DMA block
				// 0: NOT Ready; 1: Ready
				// 0-Not ready to receive commands;1-Ready to receive commands
				u32 COM : 1;

				// bit 29-30
				// 00-DMA off, communication through GP0;01-?;10-DMA CPU->GPU;11-DMA GPU->CPU
				u32 DMA : 2;

				// bit 31
				// 0-Drawing even lines in interlace mode;1-Drawing uneven lines in interlace mode
				u32 LCF : 1;
			};
			
			u32 Value;
		};

		// this will hold the gpu status
		GPU_CTRL_Read_Format GPU_CTRL_Read;

		union CTRL_Write_Format
		{
			struct
			{
				u32 Parameter : 24;
				
				u32 Command : 8;
			};
			
			u32 Value;
		};
		
		////////////////////////////
		// CTRL Register Commands //
		////////////////////////////
		
		// resets the GPU, turns off the screen, and sets status to 0x14802000
		// parameter: 0
		static const u32 CTRL_Write_ResetGPU = 0;

		// resets the command buffer
		// parameter: 0
		static const u32 CTRL_Write_ResetBuffer = 1;
		
		// resets the IRQ
		// parameter: 0
		static const u32 CTRL_Write_ResetIRQ = 2;
		
		// turns on/off the display
		// parameter: 0 - display enable; 1 - display disable
		static const u32 CTRL_Write_DisplayEnable = 3;
		
		// sets DMA direction
		// parameter: 0 - dma disabled; 1 - DMA ?; 2 - DMA Cpu to Gpu; 3 - DMA Gpu to Cpu
		static const u32 CTRL_Write_DMASetup = 4;

		// sets top left corner of display area
		// parameter:
		// bit  $00-$09  X (0-1023)
		// bit  $0A-$12  Y (0-512)
		// = Y<<10 + X
		static const u32 CTRL_Write_DisplayArea = 5;
		
		// sets the horizontal display range
		// parameter:
		// bit  $00-$0b   X1 ($1f4-$CDA)
		// bit  $0c-$17   X2
		// = X1+X2<<12
		static const u32 CTRL_Write_HorizontalDisplayRange = 6;
		
		// sets the vertical display range
		// parameter:
		// bit  $00-$09   Y1
		// bit  $0a-$14   Y2
		// = Y1+Y2<<10
		static const u32 CTRL_Write_VerticalDisplayRange = 7;
		
		// sets the display mode
		// parameter:
		// bit  $00-$01  Width 0
		// bit  $02      Height
		// bit  $03      Videomode     See above
		// bit  $04      Isrgb24
		// bit  $05      Isinter
		// bit  $06      Width1
		// bit  $07      Reverseflag
		static const u32 CTRL_Write_DisplayMode = 8;
		
		// returns GPU info
		// parameter:
		// $000000  appears to return Draw area top left?
		// $000001  appears to return Draw area top left?
		// $000002
		// $000003  Draw area top left
		// $000004  Draw area bottom right
		// $000005  Draw offset
		// $000006  appears to return draw offset?
		// $000007  GPU Type, should return 2 for a standard GPU.
		static const u32 CTRL_Write_GPUInfo = 0x10;
		
		
		// other commands are unknown until testing
		
		///////////////////
		// Pixel Formats //
		///////////////////
		
		// BGR format with highest bit being M
		union Pixel_15bit_Format
		{
			struct
			{
				u16 Red : 5;
				u16 Green : 5;
				u16 Blue : 5;

				// bit to mask pixel (1 - masked; 0 - not masked)
				u16 M : 1;
			};
			
			u16 Value;
		};

		union Pixel_24bit_Format
		{
			// 2 pixels encoded using 3 frame buffer pixels
			// G0 | R0 | R1 | B0 | B1 | G1
			struct
			{
				// this format is stored in frame buffer as 2 pixels in a 48 bit space (3 "pixels" of space)
				u8 Green1;
				u8 Blue1;
				u8 Blue0;
				u8 Red1;
				u8 Red0;
				u8 Green0;
			};
			
			struct
			{
				u16 Pixel2;
				u16 Pixel1;
				u16 Pixel0;
			};
		};
		
		union Pixel_8bit_Format
		{
			struct
			{
				// here, there are two indexes stored in one "pixel", but in reversed order
				
				// the index for the pixel on the left in CLUT
				// bits 0-7
				u8 I0;

				// the index for the pixel on the right in CLUT (it is reversed)
				// bits 8-15
				u8 I1;
			};
			
			u16 Value;
		};
		
		union Pixel_4bit_Format
		{
			struct
			{
				// here, there are four indexes stored in one "pixel", but in reversed order
				
				// the index for the leftmost pixel in CLUT (it is reversed)
				u16 I0 : 4;
				
				u16 I1 : 4;
				u16 I2 : 4;
				
				// the index for the rightmost pixel in CLUT (it is reversed)
				u16 I3 : 4;
				
				
			};
			
			u16 Value;
		};

		
		static const u32 c_VRAM_Size = 1048576;
		
		// VRAM is 1MB with a pixel size of 16-bits
		u16 VRAM [ c_VRAM_Size / 2 ] __attribute__ ((aligned (32)));
		
		/*
		// GPU input buffer is 64 bytes (16 32-bit words)
		u32 ReadIndex;
		u32 WriteIndex;
		u32 BufferValid_Bitmap;
		u32 Buffer [ 16 ];
		*/
		
		// *** the cycles can change in PS2 mode, so calculate cycle dependent values dynamically *** //
		
		// System data
		static const double SystemBus_CyclesPerSec_PS1Mode = 33868800.0L;
		
		static const double SystemBus_CyclesPerSec_PS2Mode1 = 36864000.0L;
		static const double SystemBus_CyclesPerSec_PS2Mode2 = 37375000.0L;
		
		// Raster data
		static const double NTSC_FieldsPerSec = 59.94005994L;
		static const double PAL_FieldsPerSec = 50.0L;
		static const double NTSC_FramesPerSec = ( 59.94005994L / 2.0L );	//NTSC_FieldsPerSec / 2;
		static const double PAL_FramesPerSec = ( 50.0L / 2.0L );	//PAL_FieldsPerSec / 2;
		
		// calculate dynamically
		//static const double NTSC_CyclesPerFrame = ( 33868800.0L / ( 59.94005994L / 2.0L ) );
		//static const double PAL_CyclesPerFrame = ( 33868800.0L / ( 50.0L / 2.0L ) );
		//static const double NTSC_FramesPerCycle = 1.0L / ( 33868800.0L / ( 59.94005994L / 2.0L ) );
		//static const double PAL_FramesPerCycle = 1.0L / ( 33868800.0L / ( 50.0L / 2.0L ) );
		double NTSC_CyclesPerFrame;		// = ( 33868800.0L / ( 59.94005994L / 2.0L ) );
		double PAL_CyclesPerFrame;		// = ( 33868800.0L / ( 50.0L / 2.0L ) );
		double NTSC_FramesPerCycle;		// = 1.0L / ( 33868800.0L / ( 59.94005994L / 2.0L ) );
		double PAL_FramesPerCycle;		// = 1.0L / ( 33868800.0L / ( 50.0L / 2.0L ) );
		
		// the EVEN field must be the field that starts at scanline number zero, which means that the even field has 263 scanlines
		// so, for now I just have these mixed up since the EVEN scanlines go from 0-524, making 263 scanlines, and the ODD go from 1-523, making 262
		
		// EVEN scanlines from 0-524 (263 out of 525 total)
		static const u32 NTSC_ScanlinesPerField_Even = 263;
		
		// ODD scanlines from 1-523 (262 out of 525 total)
		static const u32 NTSC_ScanlinesPerField_Odd = 262;
		
		// 525 total scanlines for NTSC
		static const u32 NTSC_ScanlinesPerFrame = 525;	//NTSC_ScanlinesPerField_Odd + NTSC_ScanlinesPerField_Even;
		
		// EVEN scanlines from 0-624 (313 out of 625 total)
		static const u32 PAL_ScanlinesPerField_Even = 313;
		
		// ODD scanlines from 1-623 (312 out of 625 total)
		static const u32 PAL_ScanlinesPerField_Odd = 312;
		
		// 625 total scanlines for PAL
		static const u32 PAL_ScanlinesPerFrame = 625;	//PAL_ScanlinesPerField_Odd + PAL_ScanlinesPerField_Even;

		static const double NTSC_FieldsPerScanline_Even = 1.0L / 263.0L;
		static const double NTSC_FieldsPerScanline_Odd = 1.0L / 262.0L;
		static const double NTSC_FramesPerScanline = 1.0L / 525.0L;
		
		static const double PAL_FieldsPerScanline_Even = 1.0L / 313.0L;
		static const double PAL_FieldsPerScanline_Odd = 1.0L / 312.0L;
		static const double PAL_FramesPerScanline = 1.0L / 625.0L;
		
		
		
		static const double NTSC_ScanlinesPerSec = ( 525.0L * ( 59.94005994L / 2.0L ) );	//NTSC_ScanlinesPerFrame * NTSC_FramesPerSec; //15734.26573425;
		static const double PAL_ScanlinesPerSec = ( 625.0L * ( 50.0L / 2.0L ) );	//PAL_ScanlinesPerFrame * PAL_FramesPerSec; //15625;
		
		
		// calculate dynamically
		//static const double NTSC_CyclesPerScanline = ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );	//SystemBus_CyclesPerSec / NTSC_ScanlinesPerSec; //2152.5504000022;
		//static const double PAL_CyclesPerScanline = ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );	//SystemBus_CyclesPerSec / PAL_ScanlinesPerSec; //2167.6032;
		//static const double NTSC_ScanlinesPerCycle = 1.0L / ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		//static const double PAL_ScanlinesPerCycle = 1.0L / ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		double NTSC_CyclesPerScanline;		// = ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );	//SystemBus_CyclesPerSec / NTSC_ScanlinesPerSec; //2152.5504000022;
		double PAL_CyclesPerScanline;		// = ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );	//SystemBus_CyclesPerSec / PAL_ScanlinesPerSec; //2167.6032;
		double NTSC_ScanlinesPerCycle;		// = 1.0L / ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		double PAL_ScanlinesPerCycle;		// = 1.0L / ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		
		
		
		// calculate dynamically
		//static const double NTSC_CyclesPerField_Even = 263.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		//static const double NTSC_CyclesPerField_Odd = 262.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		//static const double PAL_CyclesPerField_Even = 313.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		//static const double PAL_CyclesPerField_Odd = 312.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		//static const double NTSC_FieldsPerCycle_Even = 1.0L / ( 263.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		//static const double NTSC_FieldsPerCycle_Odd = 1.0L / ( 262.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		//static const double PAL_FieldsPerCycle_Even = 1.0L / ( 313.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		//static const double PAL_FieldsPerCycle_Odd = 1.0L / ( 312.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		double NTSC_CyclesPerField_Even;	// = 263.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		double NTSC_CyclesPerField_Odd;		// = 262.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		double PAL_CyclesPerField_Even;		// = 313.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		double PAL_CyclesPerField_Odd;		// = 312.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		double NTSC_FieldsPerCycle_Even;	// = 1.0L / ( 263.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		double NTSC_FieldsPerCycle_Odd;		// = 1.0L / ( 262.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		double PAL_FieldsPerCycle_Even;		// = 1.0L / ( 313.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		double PAL_FieldsPerCycle_Odd;		// = 1.0L / ( 312.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );



		// calculate dynamically
		//static const double NTSC_DisplayAreaCycles = 240.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		//static const double PAL_DisplayAreaCycles = 288.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		double NTSC_DisplayAreaCycles;	// = 240.0L * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		double PAL_DisplayAreaCycles;	// = 288.0L * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );

		
		//static const double NTSC_CyclesPerVBlank_Even = ( 263.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		//static const double NTSC_CyclesPerVBlank_Odd = ( 262.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		//static const double PAL_CyclesPerVBlank_Even = ( 313.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		//static const double PAL_CyclesPerVBlank_Odd = ( 312.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		//static const double NTSC_VBlanksPerCycle_Even = 1.0L / ( ( 263.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		//static const double NTSC_VBlanksPerCycle_Odd = 1.0L / ( ( 262.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		//static const double PAL_VBlanksPerCycle_Even = 1.0L / ( ( 313.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		//static const double PAL_VBlanksPerCycle_Odd = 1.0L / ( ( 312.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		double NTSC_CyclesPerVBlank_Even;	// = ( 263.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		double NTSC_CyclesPerVBlank_Odd;	// = ( 262.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		double PAL_CyclesPerVBlank_Even;	// = ( 313.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		double PAL_CyclesPerVBlank_Odd;		// = ( 312.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		double NTSC_VBlanksPerCycle_Even;	// = 1.0L / ( ( 263.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		double NTSC_VBlanksPerCycle_Odd;	// = 1.0L / ( ( 262.0L - 240.0L ) * ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		double PAL_VBlanksPerCycle_Even;	// = 1.0L / ( ( 313.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		double PAL_VBlanksPerCycle_Odd;		// = 1.0L / ( ( 312.0L - 288.0L ) * ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );



		// the viewable scanlines (inclusive)
		static const long PAL_Field0_Viewable_YStart = 23;
		static const long PAL_Field0_Viewable_YEnd = 310;
		static const long PAL_Field1_Viewable_YStart = 336;
		static const long PAL_Field1_Viewable_YEnd = 623;
		
		static const u32 NTSC_VBlank = 240;
		static const u32 PAL_VBlank = 288;
		u32 Display_Width, Display_Height, Raster_Width, Raster_Height;
		u32 Raster_X, Raster_Y, Raster_HBlank;
		u32 Raster_Pixel_INC;
		
		// cycle at which the raster settings last changed at
		u64 RasterChange_StartCycle;
		u64 RasterChange_StartPixelCount;
		u64 RasterChange_StartHBlankCount;
		
		double dCyclesPerFrame, dCyclesPerField0, dCyclesPerField1, dDisplayArea_Cycles, dVBlank0Area_Cycles, dVBlank1Area_Cycles, dHBlankArea_Cycles;
		double dFramesPerCycle, dFieldsPerCycle0, dFieldsPerCycle1; //, dDisplayArea_Cycles, dVBlank0Area_Cycles, dVBlank1Area_Cycles, dHBlankArea_Cycles;
		u64 CyclesPerHBlank, CyclesPerVBlank;

		u64 CyclesPerPixel_INC;
		double dCyclesPerPixel, dCyclesPerScanline;
		double dPixelsPerCycle, dScanlinesPerCycle;
		u64 PixelCycles;
		u32 X_Pixel, Y_Pixel;
		u64 Global_XPixel, Global_YPixel;
		u32 HBlank_X;
		u32 VBlank_Y;
		u32 ScanlinesPerField0, ScanlinesPerField1;
		u32 Raster_XMax;
		u32 Raster_YMax;	// either 525 or 625
		
		
		double dGPU_NextEventCycle;
		u64 iGPU_NextEventCycle, iCyclesPerScanline;
		
		
		//double dNextScanlineStart_Cycle, dNextVsyncStart_Cycle, dNextDrawStart_Cycle;
		
		// *** todo *** keeping track of these may be used to speed up timers
		//double dLastScanlineStart_Cycle, dNextScanlineStart_Cycle, dLastHBlank_Cycle, dNextHBlank_Cycle, dLastVBlank_Cycle, dNextVBlank_Cycle, dLastDrawStart_Cycle, dNextDrawStart_Cycle;
		
		
		static const u32 HBlank_X_LUT [ 8 ];
		static const u32 VBlank_Y_LUT [ 2 ];
		static const u32 Raster_XMax_LUT [ 2 ] [ 8 ];
		static const u32 Raster_YMax_LUT [ 2 ];
		u64 CyclesPerPixel_INC_Lookup [ 2 ] [ 8 ];
		double CyclesPerPixel_Lookup [ 2 ] [ 8 ];
		double PixelsPerCycle_Lookup [ 2 ] [ 8 ];
		
		static const int c_iGPUCyclesPerPixel_256 = 10;
		static const int c_iGPUCyclesPerPixel_320 = 8;
		static const int c_iGPUCyclesPerPixel_368 = 7;
		static const int c_iGPUCyclesPerPixel_512 = 5;
		static const int c_iGPUCyclesPerPixel_640 = 4;
		
		static const u32 c_iGPUCyclesPerPixel [];
		
		static const double c_dGPUCyclesPerScanline_NTSC = 53222400.0L / ( 525.0L * 30.0L );
		static const double c_dGPUCyclesPerScanline_PAL = 53222400.0L / ( 625.0L * 30.0L );

		
		static const int c_iVisibleArea_StartX_Cycle = 584; //544;
		static const int c_iVisibleArea_EndX_Cycle = 3192;	//3232;
		static const int c_iVisibleArea_StartY_Pixel_NTSC = 15;
		static const int c_iVisibleArea_EndY_Pixel_NTSC = 257;
		static const int c_iVisibleArea_StartY_Pixel_PAL = 34;
		static const int c_iVisibleArea_EndY_Pixel_PAL = 292;
		
		static const int c_iScreenOutput_MaxWidth = ( c_iVisibleArea_EndX_Cycle >> 2 ) - ( c_iVisibleArea_StartX_Cycle >> 2 );
		static const int c_iScreenOutput_MaxHeight = ( c_iVisibleArea_EndY_Pixel_PAL - c_iVisibleArea_StartY_Pixel_PAL ) << 1;
		
		// raster functions
		void UpdateRaster_VARS ( void );
		
		// returns interrupt data
		//u32 UpdateRaster ( void );
		void UpdateRaster ( void );
		
		// updates LCF
		// note: must have Y_Pixel (CurrentScanline) updated first
		void Update_LCF ();
		
		//static const u32 DisplayWidth_Values [] = { 256, 320, 368, 512, 640 };
		//static const u32 NTSC_RasterWidth_Values [] = { 341, 426, 487, 682, 853 };
		//static const u32 PAL_RasterWidth_Values [] = { 340, 426, 486, 681, 851 };
		
		//static const u32 NTSC_DisplayHeight_Values [] = { 240, 480 };
		//static const u32 PAL_DisplayHeight_Values [] = { 288, 576 };
		//static const u32 NTSC_RasterHeight_OddField = 263;
		//static const u32 NTSC_RasterHeight_EvenField = 262;
		//static const u32 NTSC_RasterHeight_Total = 525;
		//static const u32 PAL_RasterHeight_OddField = 313;
		//static const u32 PAL_RasterHeight_EvenField = 312;
		//static const u32 PAL_RasterHeight_Total = 625;
		
		static const double NTSC_CyclesPerPixel_256 = ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 341.0L );
		static const double NTSC_CyclesPerPixel_320 = ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 426.0L );
		static const double NTSC_CyclesPerPixel_368 = ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 487.0L );
		static const double NTSC_CyclesPerPixel_512 = ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 682.0L );
		static const double NTSC_CyclesPerPixel_640 = ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 853.0L );
		static const double PAL_CyclesPerPixel_256 = ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 340.0L );
		static const double PAL_CyclesPerPixel_320 = ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 426.0L );
		static const double PAL_CyclesPerPixel_368 = ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 486.0L );
		static const double PAL_CyclesPerPixel_512 = ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 681.0L );
		static const double PAL_CyclesPerPixel_640 = ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 851.0L );
		
		static const double NTSC_PixelsPerCycle_256 = 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 341.0L );
		static const double NTSC_PixelsPerCycle_320 = 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 426.0L );
		static const double NTSC_PixelsPerCycle_368 = 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 487.0L );
		static const double NTSC_PixelsPerCycle_512 = 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 682.0L );
		static const double NTSC_PixelsPerCycle_640 = 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 853.0L );
		static const double PAL_PixelsPerCycle_256 = 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 340.0L );
		static const double PAL_PixelsPerCycle_320 = 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 426.0L );
		static const double PAL_PixelsPerCycle_368 = 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 486.0L );
		static const double PAL_PixelsPerCycle_512 = 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 681.0L );
		static const double PAL_PixelsPerCycle_640 = 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 851.0L );
		
		
		static const u64 NTSC_CyclesPerPixelINC_256 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 341.0L ) ))) + ( 1 << 8 );
		static const u64 NTSC_CyclesPerPixelINC_320 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 426.0L ) ))) + ( 1 << 8 );
		static const u64 NTSC_CyclesPerPixelINC_368 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 487.0L ) ))) + ( 1 << 8 );
		static const u64 NTSC_CyclesPerPixelINC_512 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 682.0L ) ))) + ( 1 << 8 );
		static const u64 NTSC_CyclesPerPixelINC_640 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 853.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_256 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 340.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_320 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 426.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_368 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 486.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_512 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 681.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_640 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 33868800.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 851.0L ) ))) + ( 1 << 8 );
		
		//static const double NTSC_CyclesPerPixel_Values [] = { 6.3124645161, 5.0529352113, 4.4200213552, 3.1562322581, 2.5235057444 };
		//static const double PAL_CyclesPerPixel_Values [] = { 6.3753035294, 5.0882704225, 4.4600888889, 3.1829709251, 2.5471247944 };
		
		// the last command written to CTRL register
		u32 CommandReady;
		CTRL_Write_Format NextCommand;
		
		// we need to know if the GPU is busy or not
		s64 BusyCycles;
		
		// we need a "command buffer" for GPU
		// stuff written to DATA register or via dma goes into command buffer
		u32 BufferMode;
		enum { MODE_NORMAL, MODE_IMAGEIN, MODE_IMAGEOUT };
		DATA_Write_Format Buffer [ 16 ];
		u32 BufferSize;
		u32 PixelsLeftToTransfer;
		
		// dither values array
		static const s64 c_iDitherValues [];
		static const s64 c_iDitherZero [];
		static const s64 c_iDitherValues24 [];
		static const s32 c_iDitherValues4 [];
		static const s16 c_viDitherValues16_add [];
		static const s16 c_viDitherValues16_sub [];

		// draw area data
		
		// upper left hand corner of frame buffer area being drawn to the tv
		u32 ScreenArea_TopLeftX;
		u32 ScreenArea_TopLeftY;
		
		// bounding rectangles of the area being drawn to - anything outside of this is not drawn
		u32 DrawArea_TopLeftX;
		u32 DrawArea_TopLeftY;
		u32 DrawArea_BottomRightX;
		u32 DrawArea_BottomRightY;
		
		// also need to maintain the internal coords to return when requested
		u32 iREG_DrawArea_TopLeftX;
		u32 iREG_DrawArea_TopLeftY;
		u32 iREG_DrawArea_BottomRightX;
		u32 iREG_DrawArea_BottomRightY;
		
		// the offset from the upper left hand corner of frame buffer that is being drawn to. Applies to all drawing
		// these are signed and go from -1024 to +1023
		s32 DrawArea_OffsetX;
		s32 DrawArea_OffsetY;
		
		// specifies amount of data being sent to the tv
		u32 DisplayRange_Horizontal;
		u32 DisplayRange_Vertical;
		u32 DisplayRange_X1;
		u32 DisplayRange_X2;
		u32 DisplayRange_Y1;
		u32 DisplayRange_Y2;
		
		// texture window
		u32 TWX, TWY, TWW, TWH;
		u32 TextureWindow_X;
		u32 TextureWindow_Y;
		u32 TextureWindow_Width;
		u32 TextureWindow_Height;
		
		// mask settings
		u32 SetMaskBitWhenDrawing;
		u32 DoNotDrawToMaskAreas;
		
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// the tx and ty in the GPU_CTRL register are actually texture offsets into where the textures start at
		// add these to the texture offsets supplied by the command to get the real offsets in the frame buffer
		//u32 Texture_OffsetX, Texture_OffsetY;

		// *note* these should be signed
		s32 Absolute_DrawX, Absolute_DrawY;
		
		// also need to count primitives for debugging (step primitive, etc.)
		u32 Primitive_Count;
		
		// also need to count frames for debugging
		u32 Frame_Count;
		
		static const u32 FrameBuffer_Width = 1024;
		static const u32 FrameBuffer_Height = 512;
		static const u32 FrameBuffer_XMask = FrameBuffer_Width - 1;
		static const u32 FrameBuffer_YMask = FrameBuffer_Height - 1;
		
		// constructor
		GPU ();
		
		// debug info
		static u32* DebugCpuPC;
		

		void ExecuteGPUBuffer ();
		
		void ProcessDataRegWrite ( u32 Data );
		u32 ProcessDataRegRead ();
		
		void TransferPixelPacketIn ( u32 Data );
		u32 TransferPixelPacketOut ();
		
		/////////////////////////////////////////////////////////
		// functions for getting data from command buffer
		inline void GetBGR24 ( DATA_Write_Format Data ) { bgr = Data.Value; }

		inline void GetBGR0_24 ( DATA_Write_Format Data ) { bgr0 = Data.Value; }
		inline void GetBGR1_24 ( DATA_Write_Format Data ) { bgr1 = Data.Value; }
		inline void GetBGR2_24 ( DATA_Write_Format Data ) { bgr2 = Data.Value; }
		inline void GetBGR3_24 ( DATA_Write_Format Data ) { bgr3 = Data.Value; }
		
		inline void GetBGR ( DATA_Write_Format Data ) { bgr = ( ((u32)Data.Red) >> 3 ) | ( ( ((u32)Data.Green) >> 3 ) << 5 ) | ( ( ((u32)Data.Blue) >> 3 ) << 10 ); }
		inline void GetXY ( DATA_Write_Format Data ) { x = Data.x; y = Data.y; }
		inline void GetUV ( DATA_Write_Format Data ) { u = Data.u; v = Data.v; }
		inline void GetBGR0 ( DATA_Write_Format Data ) { bgr0 = ( Data.Red >> 3 ) | ( ( Data.Green >> 3 ) << 5 ) | ( ( Data.Blue >> 3 ) << 10 ); }
		inline void GetXY0 ( DATA_Write_Format Data ) { x0 = Data.x; y0 = Data.y; }
		inline void GetUV0 ( DATA_Write_Format Data ) { u0 = Data.u; v0 = Data.v; }
		inline void GetBGR1 ( DATA_Write_Format Data ) { bgr1 = ( Data.Red >> 3 ) | ( ( Data.Green >> 3 ) << 5 ) | ( ( Data.Blue >> 3 ) << 10 ); }
		inline void GetXY1 ( DATA_Write_Format Data ) { x1 = Data.x; y1 = Data.y; }
		inline void GetUV1 ( DATA_Write_Format Data ) { u1 = Data.u; v1 = Data.v; }
		inline void GetBGR2 ( DATA_Write_Format Data ) { bgr2 = ( Data.Red >> 3 ) | ( ( Data.Green >> 3 ) << 5 ) | ( ( Data.Blue >> 3 ) << 10 ); }
		inline void GetXY2 ( DATA_Write_Format Data ) { x2 = Data.x; y2 = Data.y; }
		inline void GetUV2 ( DATA_Write_Format Data ) { u2 = Data.u; v2 = Data.v; }
		inline void GetBGR3 ( DATA_Write_Format Data ) { bgr3 = ( Data.Red >> 3 ) | ( ( Data.Green >> 3 ) << 5 ) | ( ( Data.Blue >> 3 ) << 10 ); }
		inline void GetXY3 ( DATA_Write_Format Data ) { x3 = Data.x; y3 = Data.y; }
		inline void GetUV3 ( DATA_Write_Format Data ) { u3 = Data.u; v3 = Data.v; }
		inline void GetHW ( DATA_Write_Format Data ) { h = Data.h; w = Data.w; }
		inline void GetCLUT ( DATA_Write_Format Data ) { clut_x = Data.clut.x; clut_y = Data.clut.y; }
		inline void GetTPAGE ( DATA_Write_Format Data ) { tpage_tx = Data.tpage.tx; tpage_ty = Data.tpage.ty; tpage_abr = Data.tpage.abr; tpage_tp = Data.tpage.tp; GPU_CTRL_Read.Value = ( GPU_CTRL_Read.Value & ~0x1ff ) | ( ( Data.Value >> 16 ) & 0x1ff ); }

		////////////////////////////////////////////////////////
		// Functions to process GPU buffer commands
		
		void Draw_GradientTextureTriangle_34 ();
		void Draw_GradientTextureRectangle_3c ();
		
		void Draw_MonoLine_40 ();
		void Draw_ShadedLine_50 ();
		void Draw_PolyLine_48 ();
		void Draw_GradientLine_50 ();
		void Draw_GradientPolyLine_58 ();
		
		void Draw_Sprite8_74 ();
		void Draw_Sprite16_7c ();
		
		// below is complete

		void Draw_FrameBufferRectangle_02 ();
		
		void Draw_MonoTriangle_20 ();
		void Draw_MonoRectangle_28 ();
		void Draw_GradientTriangle_30 ();
		void Draw_GradientRectangle_38 ();
		void Draw_TextureTriangle_24 ();
		void Draw_TextureRectangle_2c ();
		void Draw_TextureGradientTriangle_34 ();
		void Draw_TextureGradientRectangle_3c ();
		
		void Draw_Sprite_64 ();
		void Draw_Sprite8x8_74 ();
		void Draw_Sprite16x16_7c ();

		void Draw_Rectangle_60 ();
		void Draw_Pixel_68 ();
		void Draw_Rectangle8x8_70 ();
		void Draw_Rectangle16x16_78 ();
		
		void Transfer_MoveImage_80 ();
		
		
		///////////////////////////////////////////////////////////////////////
		// Function to perform actual drawing of primitives on screen area
		
		void DrawSprite ();
		
		
		void DrawTriangle_Mono ();
		void DrawTriangle_Gradient ();
		
		
		void DrawTriangle_Texture ();
		void DrawTriangle_TextureGradient ();
		
		
		// sets the portion of frame buffer that is showing on screen
		void Set_ScreenSize ( int _width, int _height );

		
		// inline functions
		
		inline static s32 GetRed16 ( u16 Color ) { return ( ( Color >> 10 ) & 0x1f ); }
		inline static s32 GetGreen16 ( u16 Color ) { return ( ( Color >> 5 ) & 0x1f ); }
		inline static s32 GetBlue16 ( u16 Color ) { return ( Color & 0x1f ); }
		inline static s32 SetRed16 ( u16 Color ) { return ( ( Color & 0x1f ) << 10 ); }
		inline static s32 SetGreen16 ( u16 Color ) { return ( ( Color & 0x1f ) << 5 ); }
		inline static s32 SetBlue16 ( u16 Color ) { return ( Color & 0x1f ); }
		
		inline static s32 GetRed24 ( u32 Color ) { return ( ( Color >> 16 ) & 0xff ); }
		inline static s32 GetGreen24 ( u32 Color ) { return ( ( Color >> 8 ) & 0xff ); }
		inline static s32 GetBlue24 ( u32 Color ) { return ( Color & 0xff ); }
		inline static s32 SetRed24 ( u32 Color ) { return ( ( Color & 0xff ) << 16 ); }
		inline static s32 SetGreen24 ( u32 Color ) { return ( ( Color & 0xff ) << 8 ); }
		inline static s32 SetBlue24 ( u32 Color ) { return ( Color & 0xff ); }
		
		//inline static u32 Clamp5 ( s32 Component )
		//{
		//	if ( Component < 0 ) Component = 0;
		//	if ( Component > 0x1f ) Component = 0x1f;
		//	return Component;
		//}
		
		//inline static u32 Clamp8 ( s32 Component )
		//{
		//	if ( Component < 0 ) Component = 0;
		//	if ( Component > 0xff ) Component = 0xff;
		//	return Component;
		//}
		
		inline static u32 Clamp5 ( long n )
		{
			long a = 0x1f;
			a -= n;
			a >>= 31;
			a |= n;
			n >>= 31;
			n = ~n;
			n &= a;
			return n & 0x1f;
		}
		
		// signed clamp to use after an ADD operation
		// CLAMPTO - this is the number of bits to clamp to
		template<typename T, const int CLAMPTO>
		inline static T AddSignedClamp ( T n )
		{
			n &= ~( n >> ( ( sizeof ( T ) * 8 ) - 1 ) );
			return ( n | ( ( n << ( ( sizeof ( T ) * 8 ) - ( CLAMPTO + 1 ) ) ) >> ( ( sizeof ( T ) * 8 ) - 1 ) ) ) & ( ( 1 << CLAMPTO ) - 1 );
		}
		
		// unsigned clamp to use after an ADD operation
		// CLAMPTO - this is the number of bits to clamp to
		template<typename T, const int CLAMPTO>
		inline static T AddUnsignedClamp ( T n )
		{
			//n &= ~( n >> ( ( sizeof ( T ) * 8 ) - 1 ) );
			return ( n | ( ( n << ( ( sizeof ( T ) * 8 ) - ( CLAMPTO + 1 ) ) ) >> ( ( sizeof ( T ) * 8 ) - ( CLAMPTO + 1 ) ) ) ) & ( ( 1 << CLAMPTO ) - 1 );
		}
		
		// signed clamp to use after an ANY (MULTIPLY, etc) operation, but should take longer than the one above
		// CLAMPTO - this is the mask for number of bits to clamp to
		template<typename T, const int CLAMPTO>
		inline static T SignedClamp ( T n )
		{
			long a = ( ( 1 << CLAMPTO ) - 1 );	//0x1f;
			a -= n;
			a >>= ( ( sizeof ( T ) * 8 ) - 1 );	//31;
			a |= n;
			n >>= ( ( sizeof ( T ) * 8 ) - 1 );	//31;
			n = ~n;
			n &= a;
			return n & ( ( 1 << CLAMPTO ) - 1 );	//0x1f;
		}


		// signed clamp to use after an ANY (MULTIPLY, etc) operation, but should take longer than the one above
		// CLAMPTO - this is the mask for number of bits to clamp to
		template<typename T, const int CLAMPTO>
		inline static T UnsignedClamp ( T n )
		{
			long a = ( ( 1 << CLAMPTO ) - 1 );	//0x1f;
			a -= n;
			a >>= ( ( sizeof ( T ) * 8 ) - 1 );	//31;
			n |= a;
			//n >>= ( ( sizeof ( T ) * 8 ) - 1 );	//31;
			//n = ~n;
			//n &= a;
			return n & ( ( 1 << CLAMPTO ) - 1 );	//0x1f;
		}
		
		
		inline static u32 Clamp8 ( long n )
		{
			long a = 0xff;
			a -= n;
			a >>= 31;
			a |= n;
			n >>= 31;
			n = ~n;
			n &= a;
			return n & 0xff;
		}
		
		template<typename T>
		inline static void Swap ( T& Op1, T& Op2 )
		{
			T Temp = Op1;
			Op1 = Op2;
			Op2 = Temp;
		}
		

		inline static u32 Color16To24 ( u16 Color ) { return SetRed24(GetRed16(Color)<<3) | SetGreen24(GetGreen16(Color)<<3) | SetBlue24(GetBlue16(Color)<<3); }
		inline static u16 Color24To16 ( u32 Color ) { return SetRed16(GetRed24(Color)>>3) | SetGreen16(GetGreen24(Color)>>3) | SetBlue16(GetBlue24(Color)>>3); }
		
		inline static u16 MakeRGB16 ( u32 R, u32 G, u32 B ) { return ( ( R & 0x1f ) << 10 ) | ( ( G & 0x1f ) << 5 ) | ( B & 0x1f ); }
		inline static u32 MakeRGB24 ( u32 R, u32 G, u32 B ) { return ( ( R & 0xff ) << 16 ) | ( ( G & 0xff ) << 8 ) | ( B & 0xff ); }
		
		inline static u16 ColorMultiply16 ( u16 Color1, u16 Color2 )
		{
			return SetRed16 ( Clamp5 ( ( GetRed16 ( Color1 ) * GetRed16 ( Color2 ) ) >> 4 ) ) |
					SetGreen16 ( Clamp5 ( ( GetGreen16 ( Color1 ) * GetGreen16 ( Color2 ) ) >> 4 ) ) |
					SetBlue16 ( Clamp5 ( ( GetBlue16 ( Color1 ) * GetBlue16 ( Color2 ) ) >> 4 ) );
		}
		
		inline static u32 ColorMultiply24 ( u32 Color1, u32 Color2 )
		{
			return SetRed24 ( Clamp8 ( ( GetRed24 ( Color1 ) * GetRed24 ( Color2 ) ) >> 7 ) ) |
					SetGreen24 ( Clamp8 ( ( GetGreen24 ( Color1 ) * GetGreen24 ( Color2 ) ) >> 7 ) ) |
					SetBlue24 ( Clamp8 ( ( GetBlue24 ( Color1 ) * GetBlue24 ( Color2 ) ) >> 7 ) );
		}
		
		inline static u16 ColorMultiply1624 ( u64 Color16, u64 Color24 )
		{
			static const u32 c_iBitsPerPixel16 = 5;
			static const u32 c_iRedShift16 = c_iBitsPerPixel16 * 2;
			static const u32 c_iRedMask16 = ( 0x1f << c_iRedShift16 );
			static const u32 c_iGreenShift16 = c_iBitsPerPixel16 * 1;
			static const u32 c_iGreenMask16 = ( 0x1f << c_iGreenShift16 );
			static const u32 c_iBlueShift16 = 0;
			static const u32 c_iBlueMask16 = ( 0x1f << c_iBlueShift16 );
		
			static const u32 c_iBitsPerPixel24 = 8;
			static const u32 c_iRedShift24 = c_iBitsPerPixel24 * 2;
			static const u32 c_iRedMask24 = ( 0xff << ( c_iBitsPerPixel24 * 2 ) );
			static const u32 c_iGreenShift24 = c_iBitsPerPixel24 * 1;
			static const u32 c_iGreenMask24 = ( 0xff << ( c_iBitsPerPixel24 * 1 ) );
			static const u32 c_iBlueShift24 = 0;
			static const u32 c_iBlueMask24 = ( 0xff << ( c_iBitsPerPixel24 * 0 ) );
			
			s64 Red, Green, Blue;
			
			// the multiply should put it in 16.23 fixed point, but need it back in 8.8
			Red = ( ( Color16 & c_iRedMask16 ) * ( Color24 & c_iRedMask24 ) );
			Red |= ( ( Red << ( 64 - ( 16 + 23 ) ) ) >> 63 );
			
			// to get to original position, shift back ( 23 - 8 ) = 15, then shift right 7, for total of 7 + 15 = 22 shift right
			// top bit (38) needs to end up in bit 15, so that would actually shift right by 23
			Red >>= 23;
			
			// the multiply should put it in 16.10 fixed point, but need it back in 8.30
			Green = ( ( (u32) ( Color16 & c_iGreenMask16 ) ) * ( (u32) ( Color24 & c_iGreenMask24 ) ) );
			Green |= ( ( Green << ( 64 - ( 16 + 10 ) ) ) >> 63 );
			
			// top bit (25) needs to end up in bit (10)
			Green >>= 15;
			
			// the multiply should put it in 13.0 fixed point
			Blue = ( ( (u16) ( Color16 & c_iBlueMask16 ) ) * ( (u16) ( Color24 & c_iBlueMask24 ) ) );
			Blue |= ( ( Blue << ( 64 - ( 13 + 0 ) ) ) >> 63 );
			
			// top bit (12) needs to end up in bit 5
			Blue >>= 7;
			
			return ( Red & c_iRedMask16 ) | ( Green & c_iGreenMask16 ) | ( Blue & c_iBlueMask16 );
			
			
			//return SetRed24 ( Clamp8 ( ( GetRed24 ( Color1 ) * GetRed24 ( Color2 ) ) >> 7 ) ) |
			//		SetGreen24 ( Clamp8 ( ( GetGreen24 ( Color1 ) * GetGreen24 ( Color2 ) ) >> 7 ) ) |
			//		SetBlue24 ( Clamp8 ( ( GetBlue24 ( Color1 ) * GetBlue24 ( Color2 ) ) >> 7 ) );
		}


		inline static u16 ColorMultiply241624 ( u64 Color16, u64 Color24 )
		{
			static const u32 c_iBitsPerPixel16 = 5;
			static const u32 c_iRedShift16 = c_iBitsPerPixel16 * 2;
			static const u32 c_iRedMask16 = ( 0x1f << c_iRedShift16 );
			static const u32 c_iGreenShift16 = c_iBitsPerPixel16 * 1;
			static const u32 c_iGreenMask16 = ( 0x1f << c_iGreenShift16 );
			static const u32 c_iBlueShift16 = 0;
			static const u32 c_iBlueMask16 = ( 0x1f << c_iBlueShift16 );
		
			static const u32 c_iBitsPerPixel24 = 8;
			static const u32 c_iRedShift24 = c_iBitsPerPixel24 * 2;
			static const u32 c_iRedMask24 = ( 0xff << ( c_iBitsPerPixel24 * 2 ) );
			static const u32 c_iGreenShift24 = c_iBitsPerPixel24 * 1;
			static const u32 c_iGreenMask24 = ( 0xff << ( c_iBitsPerPixel24 * 1 ) );
			static const u32 c_iBlueShift24 = 0;
			static const u32 c_iBlueMask24 = ( 0xff << ( c_iBitsPerPixel24 * 0 ) );
			
			s64 Red, Green, Blue;
			
			// the multiply should put it in 16.23 fixed point, but need it back in 8.8
			Red = ( ( Color16 & c_iRedMask16 ) * ( Color24 & c_iRedMask24 ) );
			Red |= ( ( Red << ( 64 - ( 16 + 23 ) ) ) >> 63 );
			
			// to get to original position, shift back ( 23 - 8 ) = 15, then shift right 7, for total of 7 + 15 = 22 shift right
			// top bit (38) needs to end up in bit 23, so that would actually shift right by 15
			Red >>= 15;
			
			// the multiply should put it in 16.10 fixed point, but need it back in 8.30
			Green = ( ( Color16 & c_iGreenMask16 ) * ( Color24 & c_iGreenMask24 ) );
			Green |= ( ( Green << ( 64 - ( 16 + 10 ) ) ) >> 63 );
			
			// top bit (25) needs to end up in bit 15, so shift right by 10
			Green >>= 10;
			
			// the multiply should put it in 13.0 fixed point
			Blue = ( ( Color16 & c_iBlueMask16 ) * ( Color24 & c_iBlueMask24 ) );
			Blue |= ( ( Blue << ( 64 - ( 13 + 0 ) ) ) >> 63 );
			
			// top bit (12) needs to end up in bit 7
			Blue >>= 5;
			
			return ( Red & c_iRedMask24 ) | ( Green & c_iGreenMask24 ) | ( Blue & c_iBlueMask24 );
			
			
			//return SetRed24 ( Clamp8 ( ( GetRed24 ( Color1 ) * GetRed24 ( Color2 ) ) >> 7 ) ) |
			//		SetGreen24 ( Clamp8 ( ( GetGreen24 ( Color1 ) * GetGreen24 ( Color2 ) ) >> 7 ) ) |
			//		SetBlue24 ( Clamp8 ( ( GetBlue24 ( Color1 ) * GetBlue24 ( Color2 ) ) >> 7 ) );
		}

		
		inline static u16 SemiTransparency16 ( u16 B, u16 F, u32 abrCode )
		{
			static const u32 ShiftSame = 0;
			static const u32 ShiftHalf = 1;
			static const u32 ShiftQuarter = 2;
			
			static const u32 c_iBitsPerPixel = 5;
			//static const u32 c_iShiftHalf_Mask = ~( ( 1 << 4 ) + ( 1 << 9 ) );
			static const u32 c_iShiftHalf_Mask = ~( ( 1 << ( c_iBitsPerPixel - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 2 ) - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 3 ) - 1 ) ) );
			//static const u32 c_iShiftQuarter_Mask = ~( ( 3 << 3 ) + ( 3 << 8 ) );
			static const u32 c_iShiftQuarter_Mask = ~( ( 3 << ( c_iBitsPerPixel - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 2 ) - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 3 ) - 2 ) ) );
			//static const u32 c_iClamp_Mask = ( ( 1 << 5 ) + ( 1 << 10 ) + ( 1 << 15 ) );
			static const u32 c_iClampMask = ( ( 1 << ( c_iBitsPerPixel ) ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) + ( 1 << ( c_iBitsPerPixel * 3 ) ) );
			static const u32 c_iLoBitMask = ( ( 1 ) + ( 1 << c_iBitsPerPixel ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) );
			static const u32 c_iPixelMask = 0x7fff;
			
			u32 Red, Green, Blue;
			
			u32 Color, Actual, Mask;
			
			switch ( abrCode )
			{
				// 0.5xB+0.5 xF
				case 0:
					//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftHalf ) + ( GetRed16( F ) >> ShiftHalf ) ) ) |
					//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftHalf ) + ( GetGreen16( F ) >> ShiftHalf ) ) ) |
					//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftHalf ) + ( GetBlue16( F ) >> ShiftHalf ) ) );
					
					Mask = B & F & c_iLoBitMask;
					Color = ( ( B >> 1 ) & c_iShiftHalf_Mask ) + ( ( F >> 1 ) & c_iShiftHalf_Mask ) + Mask;
					return Color;
					
					break;
				
				// 1.0xB+1.0 xF
				case 1:
					//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftSame ) + ( GetRed16( F ) >> ShiftSame ) ) ) |
					//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftSame ) + ( GetGreen16( F ) >> ShiftSame ) ) ) |
					//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftSame ) + ( GetBlue16( F ) >> ShiftSame ) ) );
					
					B &= c_iPixelMask;
					F &= c_iPixelMask;
					Actual = B + F;
					Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Color = Actual - Mask;
					Mask -= ( Mask >> c_iBitsPerPixel );
					Color |= Mask;
					return Color;
					
					break;
					
				// 1.0xB-1.0 xF
				case 2:
					//Color = SetRed16 ( Clamp5 ( (s32) ( GetRed16 ( B ) >> ShiftSame ) - (s32) ( GetRed16( F ) >> ShiftSame ) ) ) |
					//		SetGreen16 ( Clamp5 ( (s32) ( GetGreen16 ( B ) >> ShiftSame ) - (s32) ( GetGreen16( F ) >> ShiftSame ) ) ) |
					//		SetBlue16 ( Clamp5 ( (s32) ( GetBlue16 ( B ) >> ShiftSame ) - (s32) ( GetBlue16( F ) >> ShiftSame ) ) );
					
					B &= c_iPixelMask;
					F &= c_iPixelMask;
					Actual = B - F;
					Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Color = Actual + Mask;
					Mask -= ( Mask >> c_iBitsPerPixel );
					Color &= ~Mask;
					return Color;
					
					break;
					
				// 1.0xB+0.25xF
				case 3:
					//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftSame ) + ( GetRed16( F ) >> ShiftQuarter ) ) ) |
					//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftSame ) + ( GetGreen16( F ) >> ShiftQuarter ) ) ) |
					//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftSame ) + ( GetBlue16( F ) >> ShiftQuarter ) ) );
					
					B &= c_iPixelMask;
					F = ( F >> 2 ) & c_iShiftQuarter_Mask;
					Actual = B + F;
					Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Color = Actual - Mask;
					Mask -= ( Mask >> c_iBitsPerPixel );
					Color |= Mask;
					return Color;
					
					break;
			}
			
			return Color;
		}


#ifdef _ENABLE_SSE2
		inline static __m128i vSemiTransparency16 ( __m128i B, __m128i F, u32 abrCode )
		{
			static const u32 ShiftSame = 0;
			static const u32 ShiftHalf = 1;
			static const u32 ShiftQuarter = 2;
			
			static const u32 c_iBitsPerPixel = 5;
			//static const u32 c_iShiftHalf_Mask = ~( ( 1 << 4 ) + ( 1 << 9 ) );
			static const u32 c_iShiftHalf_Mask = ~( ( 1 << ( c_iBitsPerPixel - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 2 ) - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 3 ) - 1 ) ) );
			//static const u32 c_iShiftQuarter_Mask = ~( ( 3 << 3 ) + ( 3 << 8 ) );
			static const u32 c_iShiftQuarter_Mask = ~( ( 3 << ( c_iBitsPerPixel - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 2 ) - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 3 ) - 2 ) ) );
			//static const u32 c_iClamp_Mask = ( ( 1 << 5 ) + ( 1 << 10 ) + ( 1 << 15 ) );
			static const u32 c_iClampMask = ( ( 1 << ( c_iBitsPerPixel ) ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) + ( 1 << ( c_iBitsPerPixel * 3 ) ) );
			static const u32 c_iLoBitMask = ( ( 1 ) + ( 1 << c_iBitsPerPixel ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) );
			static const u32 c_iPixelMask = 0x7fff;
			
			//u32 Red, Green, Blue;
			
			//u32 Color, Actual, Mask;
			__m128i Color, Actual, Mask;
			__m128i vTemp;
			
			switch ( abrCode )
			{
				// 0.5xB+0.5 xF
				case 0:
					//Mask = B & F & c_iLoBitMask;
					Mask = _mm_and_si128 ( _mm_and_si128 ( B, F ), _mm_set1_epi16 ( c_iLoBitMask ) );
					
					//Color = ( ( B >> 1 ) & c_iShiftHalf_Mask ) + ( ( F >> 1 ) & c_iShiftHalf_Mask ) + Mask;
					vTemp = _mm_set1_epi16 ( c_iShiftHalf_Mask );
					Color = _mm_add_epi16 ( _mm_add_epi16 ( _mm_and_si128( _mm_srli_epi16 ( B, 1 ), vTemp ), _mm_and_si128( _mm_srli_epi16 ( F, 1 ), vTemp ) ), Mask );
					
					return Color;
					
					break;
				
				// 1.0xB+1.0 xF
				case 1:
					//B &= c_iPixelMask;
					//F &= c_iPixelMask;
					vTemp = _mm_set1_epi16 ( c_iPixelMask );
					B = _mm_and_si128 ( B, vTemp );
					F = _mm_and_si128 ( F, vTemp );
					
					//Actual = B + F;
					Actual = _mm_add_epi16 ( B, F );
					
					//Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Mask = _mm_and_si128 ( _mm_xor_si128 ( _mm_xor_si128 ( B, F ), Actual ), _mm_set1_epi16 ( c_iClampMask ) );
					
					//Color = Actual - Mask;
					Color = _mm_sub_epi16 ( Actual, Mask );
					
					//Mask -= ( Mask >> c_iBitsPerPixel );
					Mask = _mm_sub_epi16 ( Mask, _mm_srli_epi16 ( Mask, c_iBitsPerPixel ) );
					
					//Color |= Mask;
					Color = _mm_or_si128 ( Color, Mask );
					
					return Color;
					
					break;
					
				// 1.0xB-1.0 xF
				case 2:
					//B &= c_iPixelMask;
					//F &= c_iPixelMask;
					vTemp = _mm_set1_epi16 ( c_iPixelMask );
					B = _mm_and_si128 ( B, vTemp );
					F = _mm_and_si128 ( F, vTemp );
					
					//Actual = B - F;
					Actual = _mm_sub_epi16 ( B, F );
					
					//Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Mask = _mm_and_si128 ( _mm_xor_si128 ( _mm_xor_si128 ( B, F ), Actual ), _mm_set1_epi16 ( c_iClampMask ) );
					
					//Color = Actual + Mask;
					Color = _mm_add_epi16 ( Actual, Mask );

					//Mask -= ( Mask >> c_iBitsPerPixel );
					Mask = _mm_sub_epi16 ( Mask, _mm_srli_epi16 ( Mask, c_iBitsPerPixel ) );

					//Color &= ~Mask;
					Color = _mm_andnot_si128 ( Mask, Color );

					return Color;
					
					break;
					
				// 1.0xB+0.25xF
				case 3:
					//B &= c_iPixelMask;
					vTemp = _mm_set1_epi16 ( c_iPixelMask );
					B = _mm_and_si128 ( B, vTemp );
					
					//F = ( F >> 2 ) & c_iShiftQuarter_Mask;
					vTemp = _mm_set1_epi16 ( c_iShiftQuarter_Mask );
					F = _mm_and_si128 ( _mm_srli_epi16 ( F, 2 ), vTemp );
					
					//Actual = B + F;
					Actual = _mm_add_epi16 ( B, F );
					
					//Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Mask = _mm_and_si128 ( _mm_xor_si128 ( _mm_xor_si128 ( B, F ), Actual ), _mm_set1_epi16 ( c_iClampMask ) );
					
					//Color = Actual - Mask;
					Color = _mm_sub_epi16 ( Actual, Mask );
					
					//Mask -= ( Mask >> c_iBitsPerPixel );
					Mask = _mm_sub_epi16 ( Mask, _mm_srli_epi16 ( Mask, c_iBitsPerPixel ) );
					
					//Color |= Mask;
					Color = _mm_or_si128 ( Color, Mask );
					
					return Color;
					
					break;
			}
			
			return Color;
		}


		
		
		// code from stack overflow @ http://stackoverflow.com/questions/10500766/sse-multiplication-of-4-32-bit-integers
		static inline __m128i _custom_mul_32(const __m128i &a, const __m128i &b)
		{
#ifdef _ENABLE_SSE41  // modern CPU - use SSE 4.1
			return _mm_mullo_epi32(a, b);
#else               // old CPU - use SSE 2
			__m128i tmp1 = _mm_mul_epu32(a,b); /* mul 2,0*/
			__m128i tmp2 = _mm_mul_epu32( _mm_srli_si128(a,4), _mm_srli_si128(b,4)); /* mul 3,1 */
			return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE (0,0,2,0)), _mm_shuffle_epi32(tmp2, _MM_SHUFFLE (0,0,2,0))); /* shuffle results to [63..0] and pack */
#endif
		}

		
		// code from stack overflow @ http://stackoverflow.com/questions/4360920/whats-the-most-efficient-way-to-load-and-extract-32-bit-integer-values-from-a-1
		inline s32 _custom_get0(const __m128i& vec){return _mm_cvtsi128_si32 (vec);}
		inline s32 _custom_get1(const __m128i& vec){return _mm_cvtsi128_si32 (_mm_shuffle_epi32(vec,0x55));}
		inline s32 _custom_get2(const __m128i& vec){return _mm_cvtsi128_si32 (_mm_shuffle_epi32(vec,0xAA));}
		inline s32 _custom_get3(const __m128i& vec){return _mm_cvtsi128_si32 (_mm_shuffle_epi32(vec,0xFF));}
		
		// all inputs should be 16-bit vector values
		inline static __m128i vColorMultiply1624 ( __m128i& Color16, __m128i& vr_Color24, __m128i& vg_Color24, __m128i& vb_Color24 )
		{
			/*
			static const u32 c_iBitsPerPixel16 = 5;
			static const u32 c_iRedShift16 = c_iBitsPerPixel16 * 2;
			static const u32 c_iRedMask16 = ( 0x1f << c_iRedShift16 );
			static const u32 c_iGreenShift16 = c_iBitsPerPixel16 * 1;
			static const u32 c_iGreenMask16 = ( 0x1f << c_iGreenShift16 );
			static const u32 c_iBlueShift16 = 0;
			static const u32 c_iBlueMask16 = ( 0x1f << c_iBlueShift16 );
		
			static const u32 c_iBitsPerPixel24 = 8;
			static const u32 c_iRedShift24 = c_iBitsPerPixel24 * 2;
			static const u32 c_iRedMask24 = ( 0xff << ( c_iBitsPerPixel24 * 2 ) );
			static const u32 c_iGreenShift24 = c_iBitsPerPixel24 * 1;
			static const u32 c_iGreenMask24 = ( 0xff << ( c_iBitsPerPixel24 * 1 ) );
			static const u32 c_iBlueShift24 = 0;
			static const u32 c_iBlueMask24 = ( 0xff << ( c_iBitsPerPixel24 * 0 ) );
			*/
			
			//s64 Red, Green, Blue;
			__m128i Red, Green, Blue, vMask;
			
			Red = _mm_slli_epi16 ( Color16, 11 );
			Green = _mm_slli_epi16 ( _mm_srli_epi16 ( Color16, 5 ), 11 );
			Blue = _mm_slli_epi16 ( _mm_srli_epi16 ( Color16, 10 ), 11 );
			
			// multiply
			Red = _mm_mulhi_epu16 ( Red, vr_Color24 );
			Green = _mm_mulhi_epu16 ( Green, vg_Color24 );
			Blue = _mm_mulhi_epu16 ( Blue, vb_Color24 );
			
			vMask = _mm_set1_epi16 ( 0x1f << 2 );
			
			// saturate
			Red = _mm_and_si128 ( _mm_or_si128 ( Red, _mm_cmpgt_epi16 ( Red, vMask ) ), vMask );
			Green = _mm_and_si128 ( _mm_or_si128 ( Green, _mm_cmpgt_epi16 ( Green, vMask ) ), vMask );
			Blue = _mm_and_si128 ( _mm_or_si128 ( Blue, _mm_cmpgt_epi16 ( Blue, vMask ) ), vMask );
			
			// combine
			return _mm_or_si128 ( _mm_or_si128 ( _mm_srli_epi16 ( Red, 2 ), _mm_slli_epi16 ( Green, 3 ) ), _mm_slli_epi16 ( Blue, 8 ) );
			
			/*
			// ------
			
			// get components
			Red = _mm_srli_epi16 ( _mm_slli_epi16 ( Color16, 11 ), 11 );
			Green = _mm_srli_epi16 ( _mm_slli_epi16 ( Color16, 6 ), 11 );
			Blue = _mm_srli_epi16 ( _mm_slli_epi16 ( Color16, 1 ), 11 );
			
			// multiply
			Red = _mm_mullo_epi16 ( Red, vr_Color24 );
			Green = _mm_mullo_epi16 ( Green, vg_Color24 );
			Blue = _mm_mullo_epi16 ( Blue, vb_Color24 );
			
			// saturate
			Red = _mm_or_si128 ( Red, _mm_srai_epi16 ( _mm_slli_epi16 ( Red, 3 ), 15 ) );
			Green = _mm_or_si128 ( Green, _mm_srai_epi16 ( _mm_slli_epi16 ( Green, 3 ), 15 ) );
			Blue = _mm_or_si128 ( Blue, _mm_srai_epi16 ( _mm_slli_epi16 ( Blue, 3 ), 15 ) );
			
			// mask
			Red = _mm_srli_epi16 ( _mm_slli_epi16 ( Red, 4 ), 11 );
			Green = _mm_srli_epi16 ( _mm_slli_epi16 ( Green, 4 ), 11 );
			Blue = _mm_srli_epi16 ( _mm_slli_epi16 ( Blue, 4 ), 11 );
			
			// combine
			return _mm_or_si128 ( Red, _mm_or_si128 ( _mm_slli_epi16 ( Green, 5 ), _mm_slli_epi16 ( Blue, 10 ) ) );
			*/
		}
		
#endif



#ifdef _ENABLE_SSE2_TRIANGLE_MONO
		template<const int ABRCODE>
		inline static __m128i vSemiTransparency16_t ( __m128i B, __m128i F, const __m128i c_vLoBitMask, const __m128i c_vPixelMask, const __m128i c_vClampMask, const __m128i c_vShiftHalf_Mask, const __m128i c_vShiftQuarter_Mask )
		{
			static const u32 ShiftSame = 0;
			static const u32 ShiftHalf = 1;
			static const u32 ShiftQuarter = 2;
			
			static const u32 c_iBitsPerPixel = 5;
			static const u32 c_iShiftHalf_Mask = ~( ( 1 << ( c_iBitsPerPixel - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 2 ) - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 3 ) - 1 ) ) );
			static const u32 c_iShiftQuarter_Mask = ~( ( 3 << ( c_iBitsPerPixel - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 2 ) - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 3 ) - 2 ) ) );
			static const u32 c_iClampMask = ( ( 1 << ( c_iBitsPerPixel ) ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) + ( 1 << ( c_iBitsPerPixel * 3 ) ) );
			static const u32 c_iLoBitMask = ( ( 1 ) + ( 1 << c_iBitsPerPixel ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) );
			static const u32 c_iPixelMask = 0x7fff;
			
			//u32 Color, Actual, Mask;
			__m128i Color, Actual, Mask;
			//__m128i vTemp;
			
			// constants needed c_vLoBitMask, c_vPixelMask, c_vClampMask, c_vShiftHalfMask, c_vShiftQuarterMask
			switch ( ABRCODE )
			{
				// 0.5xB+0.5 xF
				case 0:
					//Mask = B & F & c_iLoBitMask;
					Mask = _mm_and_si128 ( _mm_and_si128 ( B, F ), c_vLoBitMask );
					
					//Color = ( ( B >> 1 ) & c_iShiftHalf_Mask ) + ( ( F >> 1 ) & c_iShiftHalf_Mask ) + Mask;
					//vTemp = _mm_set1_epi16 ( c_vShiftHalf_Mask );
					Color = _mm_add_epi16 ( _mm_add_epi16 ( _mm_and_si128( _mm_srli_epi16 ( B, 1 ), c_vShiftHalf_Mask ), _mm_and_si128( _mm_srli_epi16 ( F, 1 ), c_vShiftHalf_Mask ) ), Mask );
					
					return Color;
					
					break;
				
				// 1.0xB+1.0 xF
				case 1:
					//B &= c_iPixelMask;
					//F &= c_iPixelMask;
					//vTemp = _mm_set1_epi16 ( c_vPixelMask );
					B = _mm_and_si128 ( B, c_vPixelMask );
					F = _mm_and_si128 ( F, c_vPixelMask );
					
					//Actual = B + F;
					Actual = _mm_add_epi16 ( B, F );
					
					//Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Mask = _mm_and_si128 ( _mm_xor_si128 ( _mm_xor_si128 ( B, F ), Actual ), c_vClampMask );
					
					//Color = Actual - Mask;
					Color = _mm_sub_epi16 ( Actual, Mask );
					
					//Mask -= ( Mask >> c_iBitsPerPixel );
					Mask = _mm_sub_epi16 ( Mask, _mm_srli_epi16 ( Mask, c_iBitsPerPixel ) );
					
					//Color |= Mask;
					Color = _mm_or_si128 ( Color, Mask );
					
					return Color;
					
					break;
					
				// 1.0xB-1.0 xF
				case 2:
					//B &= c_iPixelMask;
					//F &= c_iPixelMask;
					//vTemp = _mm_set1_epi16 ( c_vPixelMask );
					B = _mm_and_si128 ( B, c_vPixelMask );
					F = _mm_and_si128 ( F, c_vPixelMask );
					
					//Actual = B - F;
					Actual = _mm_sub_epi16 ( B, F );
					
					//Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Mask = _mm_and_si128 ( _mm_xor_si128 ( _mm_xor_si128 ( B, F ), Actual ), c_vClampMask );
					
					//Color = Actual + Mask;
					Color = _mm_add_epi16 ( Actual, Mask );

					//Mask -= ( Mask >> c_iBitsPerPixel );
					Mask = _mm_sub_epi16 ( Mask, _mm_srli_epi16 ( Mask, c_iBitsPerPixel ) );

					//Color &= ~Mask;
					Color = _mm_andnot_si128 ( Mask, Color );

					return Color;
					
					break;
					
				// 1.0xB+0.25xF
				case 3:
					//B &= c_iPixelMask;
					//vTemp = _mm_set1_epi16 ( c_vPixelMask );
					B = _mm_and_si128 ( B, c_vPixelMask );
					
					//F = ( F >> 2 ) & c_iShiftQuarter_Mask;
					//vTemp = _mm_set1_epi16 ( c_vShiftQuarter_Mask );
					F = _mm_and_si128 ( _mm_srli_epi16 ( F, 2 ), c_vShiftQuarter_Mask );
					
					//Actual = B + F;
					Actual = _mm_add_epi16 ( B, F );
					
					//Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Mask = _mm_and_si128 ( _mm_xor_si128 ( _mm_xor_si128 ( B, F ), Actual ), c_vClampMask );
					
					//Color = Actual - Mask;
					Color = _mm_sub_epi16 ( Actual, Mask );
					
					//Mask -= ( Mask >> c_iBitsPerPixel );
					Mask = _mm_sub_epi16 ( Mask, _mm_srli_epi16 ( Mask, c_iBitsPerPixel ) );
					
					//Color |= Mask;
					Color = _mm_or_si128 ( Color, Mask );
					
					return Color;
					
					break;
			}
			
			//return Color;
		}
#endif
		
		
		inline u32 SemiTransparency24 ( u32 B, u32 F, u32 abrCode )
		{
			static const u32 ShiftSame = 0;
			static const u32 ShiftHalf = 1;
			static const u32 ShiftQuarter = 2;
			
			static const u32 c_iBitsPerPixel = 8;
			//static const u32 c_iShiftHalf_Mask = ~( ( 1 << 4 ) + ( 1 << 9 ) );
			static const u32 c_iShiftHalf_Mask = ~( ( 1 << ( c_iBitsPerPixel - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 2 ) - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 3 ) - 1 ) ) );
			//static const u32 c_iShiftQuarter_Mask = ~( ( 3 << 3 ) + ( 3 << 8 ) );
			static const u32 c_iShiftQuarter_Mask = ~( ( 3 << ( c_iBitsPerPixel - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 2 ) - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 3 ) - 2 ) ) );
			//static const u32 c_iClamp_Mask = ( ( 1 << 5 ) + ( 1 << 10 ) + ( 1 << 15 ) );
			static const u32 c_iClampMask = ( ( 1 << ( c_iBitsPerPixel ) ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) + ( 1 << ( c_iBitsPerPixel * 3 ) ) );
			static const u32 c_iLoBitMask = ( ( 1 ) + ( 1 << c_iBitsPerPixel ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) );
			
			u32 Color, Actual, Mask;
			
			//u32 Red, Green, Blue, Color;
			
			switch ( abrCode )
			{
				// 0.5xB+0.5 xF
				case 0:
					//Color = SetRed24 ( Clamp8 ( ( GetRed24 ( B ) >> ShiftHalf ) + ( GetRed24( F ) >> ShiftHalf ) ) ) |
					//		SetGreen24 ( Clamp8 ( ( GetGreen24 ( B ) >> ShiftHalf ) + ( GetGreen24( F ) >> ShiftHalf ) ) ) |
					//		SetBlue24 ( Clamp8 ( ( GetBlue24 ( B ) >> ShiftHalf ) + ( GetBlue24( F ) >> ShiftHalf ) ) );
					
					Mask = B & F & c_iLoBitMask;
					Color = ( ( B >> 1 ) & c_iShiftHalf_Mask ) + ( ( F >> 1 ) & c_iShiftHalf_Mask ) + Mask;
					return Color;
					
					break;
				
				// 1.0xB+1.0 xF
				case 1:
					//Color = SetRed24 ( Clamp8 ( ( GetRed24 ( B ) >> ShiftSame ) + ( GetRed24( F ) >> ShiftSame ) ) ) |
					//		SetGreen24 ( Clamp8 ( ( GetGreen24 ( B ) >> ShiftSame ) + ( GetGreen24( F ) >> ShiftSame ) ) ) |
					//		SetBlue24 ( Clamp8 ( ( GetBlue24 ( B ) >> ShiftSame ) + ( GetBlue24( F ) >> ShiftSame ) ) );
					
					Actual = B + F;
					Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Color = Actual - Mask;
					Mask -= ( Mask >> c_iBitsPerPixel );
					Color |= Mask;
					return Color;
					
					break;
					
				// 1.0xB-1.0 xF
				case 2:
					//Color = SetRed24 ( Clamp8 ( (s32) ( GetRed24 ( B ) >> ShiftSame ) - (s32) ( GetRed24( F ) >> ShiftSame ) ) ) |
					//		SetGreen24 ( Clamp8 ( (s32) ( GetGreen24 ( B ) >> ShiftSame ) - (s32) ( GetGreen24( F ) >> ShiftSame ) ) ) |
					//		SetBlue24 ( Clamp8 ( (s32) ( GetBlue24 ( B ) >> ShiftSame ) - (s32) ( GetBlue24( F ) >> ShiftSame ) ) );
					
					Actual = B - F;
					Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Color = Actual + Mask;
					Mask -= ( Mask >> c_iBitsPerPixel );
					Color &= ~Mask;
					return Color;
					
					break;
					
				// 1.0xB+0.28xF
				case 3:
					//Color = SetRed24 ( Clamp8 ( ( GetRed24 ( B ) >> ShiftSame ) + ( GetRed24( F ) >> ShiftQuarter ) ) ) |
					//		SetGreen24 ( Clamp8 ( ( GetGreen24 ( B ) >> ShiftSame ) + ( GetGreen24( F ) >> ShiftQuarter ) ) ) |
					//		SetBlue24 ( Clamp8 ( ( GetBlue24 ( B ) >> ShiftSame ) + ( GetBlue24( F ) >> ShiftQuarter ) ) );
					
					F = ( F >> 2 ) & c_iShiftQuarter_Mask;
					Actual = B + F;
					Mask = ( B ^ F ^ Actual ) & c_iClampMask;
					Color = Actual - Mask;
					Mask -= ( Mask >> c_iBitsPerPixel );
					Color |= Mask;
					return Color;
					
					break;
			}
			
			return Color;
		}

		
		// checks if it is ok to draw to ps1 frame buffer or not by looking at DFE and LCF
		inline bool isDrawOk ()
		{
			if ( GPU_CTRL_Read.DFE || ( !GPU_CTRL_Read.DFE && !GPU_CTRL_Read.LCF ) )
			{
				return true;
			}
			else
			{
				//BusyCycles = 0;
				return false;
				//return true;
			}
		}
		

		static const u32 c_InterruptBit = 1;
		static const u32 c_InterruptBit_Vsync = 0;

		//static u32* _Intc_Master;
		static u32* _Intc_Stat;
		static u32* _Intc_Mask;
		static u32* _R3000A_Status_12;
		static u32* _R3000A_Cause_13;
		static u64* _ProcStatus;
		
		inline void ConnectInterrupt ( u32* _IStat, u32* _IMask, u32* _R3000A_Status, u32* _R3000A_Cause, u64* _ProcStat )
		{
			_Intc_Stat = _IStat;
			_Intc_Mask = _IMask;
			//_Intc_Master = _IMaster;
			_R3000A_Cause_13 = _R3000A_Cause;
			_R3000A_Status_12 = _R3000A_Status;
			_ProcStatus = _ProcStat;
		}
		
		
		inline void SetInterrupt ()
		{
			//*_Intc_Master |= ( 1 << c_InterruptBit );
			*_Intc_Stat |= ( 1 << c_InterruptBit );
			if ( *_Intc_Stat & *_Intc_Mask ) *_R3000A_Cause_13 |= ( 1 << 10 );
			
			if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 ); else *_ProcStatus &= ~( 1 << 20 );
		}
		
		inline void ClearInterrupt ()
		{
			//*_Intc_Master &= ~( 1 << c_InterruptBit );
			*_Intc_Stat &= ~( 1 << c_InterruptBit );
			if ( ! ( *_Intc_Stat & *_Intc_Mask ) ) *_R3000A_Cause_13 &= ~( 1 << 10 );
			
			if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 ); else *_ProcStatus &= ~( 1 << 20 );
		}

		
		
		inline void SetInterrupt_Vsync ()
		{
			//*_Intc_Master |= ( 1 << c_InterruptBit_Vsync );
			*_Intc_Stat |= ( 1 << c_InterruptBit_Vsync );
			if ( *_Intc_Stat & *_Intc_Mask ) *_R3000A_Cause_13 |= ( 1 << 10 );
			
			if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 ); else *_ProcStatus &= ~( 1 << 20 );
		}
		
		inline void ClearInterrupt_Vsync ()
		{
			//*_Intc_Master &= ~( 1 << c_InterruptBit_Vsync );
			*_Intc_Stat &= ~( 1 << c_InterruptBit_Vsync );
			if ( ! ( *_Intc_Stat & *_Intc_Mask ) ) *_R3000A_Cause_13 &= ~( 1 << 10 );
			
			if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 ); else *_ProcStatus &= ~( 1 << 20 );
		}

		
		static u64* _NextSystemEvent;

		
		////////////////////////////////
		// Debug Info
		static u32* _DebugPC;
		static u64* _DebugCycleCount;
		
		static bool DebugWindow_Enabled;
		static void DebugWindow_Enable ();
		static void DebugWindow_Disable ();
		static void DebugWindow_Update ();


template<const long ABE, const long TGE>
void SelectSprite_t ();

template<const long ABE, const long TGE>
void Draw_Sprite_64_t ();
template<const long ABE, const long TGE>
void Draw_Sprite8x8_74_t ();
template<const long ABE, const long TGE>
void Draw_Sprite16x16_7c_t ();


template <const long ABRCODE>		
inline static u16 SemiTransparency16_t ( u16 B, u16 F )
{
	static const u32 ShiftSame = 0;
	static const u32 ShiftHalf = 1;
	static const u32 ShiftQuarter = 2;
	
	static const u32 c_iBitsPerPixel = 5;
	//static const u32 c_iShiftHalf_Mask = ~( ( 1 << 4 ) + ( 1 << 9 ) );
	static const u32 c_iShiftHalf_Mask = ~( ( 1 << ( c_iBitsPerPixel - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 2 ) - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 3 ) - 1 ) ) );
	//static const u32 c_iShiftQuarter_Mask = ~( ( 3 << 3 ) + ( 3 << 8 ) );
	static const u32 c_iShiftQuarter_Mask = ~( ( 3 << ( c_iBitsPerPixel - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 2 ) - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 3 ) - 2 ) ) );
	//static const u32 c_iClamp_Mask = ( ( 1 << 5 ) + ( 1 << 10 ) + ( 1 << 15 ) );
	static const u32 c_iClampMask = ( ( 1 << ( c_iBitsPerPixel ) ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) + ( 1 << ( c_iBitsPerPixel * 3 ) ) );
	static const u32 c_iLoBitMask = ( ( 1 ) + ( 1 << c_iBitsPerPixel ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) );
	static const u32 c_iPixelMask = 0x7fff;
	
	//u32 Red, Green, Blue;
	
	u32 Color, Actual, Mask;
	
	switch ( ABRCODE )
	{
		// 0.5xB+0.5 xF
		case 0:
			//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftHalf ) + ( GetRed16( F ) >> ShiftHalf ) ) ) |
			//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftHalf ) + ( GetGreen16( F ) >> ShiftHalf ) ) ) |
			//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftHalf ) + ( GetBlue16( F ) >> ShiftHalf ) ) );
			
			Mask = B & F & c_iLoBitMask;
			Color = ( ( B >> 1 ) & c_iShiftHalf_Mask ) + ( ( F >> 1 ) & c_iShiftHalf_Mask ) + Mask;
			return Color;
			
			break;
		
		// 1.0xB+1.0 xF
		case 1:
			//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftSame ) + ( GetRed16( F ) >> ShiftSame ) ) ) |
			//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftSame ) + ( GetGreen16( F ) >> ShiftSame ) ) ) |
			//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftSame ) + ( GetBlue16( F ) >> ShiftSame ) ) );
			
			B &= c_iPixelMask;
			F &= c_iPixelMask;
			Actual = B + F;
			Mask = ( B ^ F ^ Actual ) & c_iClampMask;
			Color = Actual - Mask;
			Mask -= ( Mask >> c_iBitsPerPixel );
			Color |= Mask;
			return Color;
			
			break;
			
		// 1.0xB-1.0 xF
		case 2:
			//Color = SetRed16 ( Clamp5 ( (s32) ( GetRed16 ( B ) >> ShiftSame ) - (s32) ( GetRed16( F ) >> ShiftSame ) ) ) |
			//		SetGreen16 ( Clamp5 ( (s32) ( GetGreen16 ( B ) >> ShiftSame ) - (s32) ( GetGreen16( F ) >> ShiftSame ) ) ) |
			//		SetBlue16 ( Clamp5 ( (s32) ( GetBlue16 ( B ) >> ShiftSame ) - (s32) ( GetBlue16( F ) >> ShiftSame ) ) );
			
			B &= c_iPixelMask;
			F &= c_iPixelMask;
			Actual = B - F;
			Mask = ( B ^ F ^ Actual ) & c_iClampMask;
			Color = Actual + Mask;
			Mask -= ( Mask >> c_iBitsPerPixel );
			Color &= ~Mask;
			return Color;
			
			break;
			
		// 1.0xB+0.25xF
		case 3:
			//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftSame ) + ( GetRed16( F ) >> ShiftQuarter ) ) ) |
			//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftSame ) + ( GetGreen16( F ) >> ShiftQuarter ) ) ) |
			//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftSame ) + ( GetBlue16( F ) >> ShiftQuarter ) ) );
			
			B &= c_iPixelMask;
			F = ( F >> 2 ) & c_iShiftQuarter_Mask;
			Actual = B + F;
			Mask = ( B ^ F ^ Actual ) & c_iClampMask;
			Color = Actual - Mask;
			Mask -= ( Mask >> c_iBitsPerPixel );
			Color |= Mask;
			return Color;
			
			break;
	}
	
	//return Color;
}






template <const long ABRCODE>		
inline static u64 SemiTransparency16_64t ( u64 B, u64 F )
{
	static const u32 ShiftSame = 0;
	static const u32 ShiftHalf = 1;
	static const u32 ShiftQuarter = 2;
	
	static const u32 c_iBitsPerPixel = 5;
	
	static const u64 c_iShiftHalf_Mask64 = ( ~( ( 1 << ( c_iBitsPerPixel - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 2 ) - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 3 ) - 1 ) ) ) ) & 0xffff;
	static const u64 c_iShiftQuarter_Mask64 = ( ~( ( 3 << ( c_iBitsPerPixel - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 2 ) - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 3 ) - 2 ) ) ) ) & 0xffff;
	
	static const u64 c_iClampMask64 = ( ( 1 << ( c_iBitsPerPixel ) ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) + ( 1 << ( c_iBitsPerPixel * 3 ) ) );
	static const u64 c_iLoBitMask64 = ( ( 1 ) + ( 1 << c_iBitsPerPixel ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) );
	static const u64 c_iPixelMask64 = 0x7fff;
	
	static const u64 c_iShiftHalf_Mask = ( c_iShiftHalf_Mask64 ) + ( c_iShiftHalf_Mask64 << 16 ) + ( c_iShiftHalf_Mask64 << 32 ) + ( c_iShiftHalf_Mask64 << 48 );
	static const u64 c_iShiftQuarter_Mask = ( c_iShiftQuarter_Mask64 ) + ( c_iShiftQuarter_Mask64 << 16 ) + ( c_iShiftQuarter_Mask64 << 32 ) + ( c_iShiftQuarter_Mask64 << 48 );
	
	static const u64 c_iClampMask = ( c_iClampMask64 ) + ( c_iClampMask64 << 16 ) + ( c_iClampMask64 << 32 ) + ( c_iClampMask64 << 48 );
	static const u64 c_iLoBitMask = ( c_iLoBitMask64 ) + ( c_iLoBitMask64 << 16 ) + ( c_iLoBitMask64 << 32 ) + ( c_iLoBitMask64 << 48 );
	static const u64 c_iPixelMask = ( c_iPixelMask64 ) + ( c_iPixelMask64 << 16 ) + ( c_iPixelMask64 << 32 ) + ( c_iPixelMask64 << 48 );
	
	u64 Color, Actual, Mask;
	
	switch ( ABRCODE )
	{
		// 0.5xB+0.5 xF
		case 0:
			//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftHalf ) + ( GetRed16( F ) >> ShiftHalf ) ) ) |
			//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftHalf ) + ( GetGreen16( F ) >> ShiftHalf ) ) ) |
			//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftHalf ) + ( GetBlue16( F ) >> ShiftHalf ) ) );
			
			Mask = B & F & c_iLoBitMask;
			Color = ( ( B >> 1 ) & c_iShiftHalf_Mask ) + ( ( F >> 1 ) & c_iShiftHalf_Mask ) + Mask;
			return Color;
			
			break;
		
		// 1.0xB+1.0 xF
		case 1:
			//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftSame ) + ( GetRed16( F ) >> ShiftSame ) ) ) |
			//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftSame ) + ( GetGreen16( F ) >> ShiftSame ) ) ) |
			//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftSame ) + ( GetBlue16( F ) >> ShiftSame ) ) );
			
			B &= c_iPixelMask;
			F &= c_iPixelMask;
			Actual = B + F;
			Mask = ( B ^ F ^ Actual ) & c_iClampMask;
			Color = Actual - Mask;
			Mask -= ( Mask >> c_iBitsPerPixel );
			Color |= Mask;
			return Color;
			
			break;
			
		// 1.0xB-1.0 xF
		case 2:
			//Color = SetRed16 ( Clamp5 ( (s32) ( GetRed16 ( B ) >> ShiftSame ) - (s32) ( GetRed16( F ) >> ShiftSame ) ) ) |
			//		SetGreen16 ( Clamp5 ( (s32) ( GetGreen16 ( B ) >> ShiftSame ) - (s32) ( GetGreen16( F ) >> ShiftSame ) ) ) |
			//		SetBlue16 ( Clamp5 ( (s32) ( GetBlue16 ( B ) >> ShiftSame ) - (s32) ( GetBlue16( F ) >> ShiftSame ) ) );
			
			B &= c_iPixelMask;
			F &= c_iPixelMask;
			Actual = B - F;
			Mask = ( B ^ F ^ Actual ) & c_iClampMask;
			Color = Actual + Mask;
			Mask -= ( Mask >> c_iBitsPerPixel );
			Color &= ~Mask;
			return Color;
			
			break;
			
		// 1.0xB+0.25xF
		case 3:
			//Color = SetRed16 ( Clamp5 ( ( GetRed16 ( B ) >> ShiftSame ) + ( GetRed16( F ) >> ShiftQuarter ) ) ) |
			//		SetGreen16 ( Clamp5 ( ( GetGreen16 ( B ) >> ShiftSame ) + ( GetGreen16( F ) >> ShiftQuarter ) ) ) |
			//		SetBlue16 ( Clamp5 ( ( GetBlue16 ( B ) >> ShiftSame ) + ( GetBlue16( F ) >> ShiftQuarter ) ) );
			
			B &= c_iPixelMask;
			F = ( F >> 2 ) & c_iShiftQuarter_Mask;
			Actual = B + F;
			Mask = ( B ^ F ^ Actual ) & c_iClampMask;
			Color = Actual - Mask;
			Mask -= ( Mask >> c_iBitsPerPixel );
			Color |= Mask;
			return Color;
			
			break;
	}
	
	//return Color;
}




template<const long PIXELMASK, const long long SETPIXELMASK, const long ABE, const long ABRCODE>
inline void GPU::Draw_Rectangle_t ()
{
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;
	static const int c_iVectorSize8 = 8;

	s32 x0, y0, x1, y1;
	u64 pixel, pixel_temp;
	
	s32 StartX, EndX, StartY, EndY;
	//u32 PixelsPerLine;
	u16 *ptr;
	
	u32 DestPixel;
	
#ifdef _ENABLE_SSE2_RECTANGLE_MONO
	static const u32 c_iBitsPerPixel = 5;
	static const u32 c_iShiftHalf_Mask = ~( ( 1 << ( c_iBitsPerPixel - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 2 ) - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 3 ) - 1 ) ) );
	static const u32 c_iShiftQuarter_Mask = ~( ( 3 << ( c_iBitsPerPixel - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 2 ) - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 3 ) - 2 ) ) );
	static const u32 c_iClampMask = ( ( 1 << ( c_iBitsPerPixel ) ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) + ( 1 << ( c_iBitsPerPixel * 3 ) ) );
	static const u32 c_iLoBitMask = ( ( 1 ) + ( 1 << c_iBitsPerPixel ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) );
	static const u32 c_iPixelMask = 0x7fff;
	static const __m128i c_vLoBitMask = _mm_set1_epi16 ( c_iLoBitMask );
	static const __m128i c_vPixelMask = _mm_set1_epi16 ( c_iPixelMask );
	static const __m128i c_vClampMask = _mm_set1_epi16 ( c_iClampMask );
	static const __m128i c_vShiftHalf_Mask = _mm_set1_epi16 ( c_iShiftHalf_Mask );
	static const __m128i c_vShiftQuarter_Mask = _mm_set1_epi16 ( c_iShiftQuarter_Mask );
	__m128i vDestPixel, vPixelMask, vSetPixelMask, vStoreMask;
	__m128i vStartX, vEndX, vx_across, vVectorSize;
	__m128i vpixel, vpixel_temp;
	
	static const __m128i vSeq = _mm_set_epi16 ( 7, 6, 5, 4, 3, 2, 1, 0 );
	vVectorSize = _mm_set1_epi16 ( c_iVectorSize8 );
	
	vpixel = _mm_set1_epi16 ( bgr );
	//PixelMask = _mm_setzero_si128 ();
	//SetPixelMask = _mm_setzero_si128 ();
	if ( PIXELMASK ) { vPixelMask = _mm_set1_epi16 ( 0x8080 ); }
	if ( SETPIXELMASK ) { vSetPixelMask = _mm_set1_epi16 ( 0x8000 ); }
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
	
	// get the pixel color
	pixel = bgr;
	
	//if ( !PIXELMASK )
	//{
	//	pixel |= ( pixel << 16 ) | ( pixel << 32 ) | ( pixel << 48 );
	//}
	
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

#ifdef _ENABLE_SSE2_RECTANGLE_MONO
	vStartX = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX + 1 );
#endif

	NumberOfPixelsDrawn = ( EndX - StartX + 1 ) * ( EndY - StartY + 1 );
	
	if ( ABE )
	{
#ifdef _ENABLE_SSE2_RECTANGLE_MONO
		vpixel_temp = vpixel;
#else
		pixel_temp = pixel;
#endif
	}
	
	if ( SETPIXELMASK && !ABE )
	{
#ifdef _ENABLE_SSE2_RECTANGLE_MONO
		vpixel = _mm_or_si128 ( vpixel, vSetPixelMask );
#else
		// check if we should set mask bit when drawing
		pixel |= SETPIXELMASK;
#endif
	}
	
	for ( Line = StartY; Line <= EndY; Line++ )
	{
		ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
		
#ifdef _ENABLE_SSE2_RECTANGLE_MONO
	vx_across = vStartX;
#endif

		// draw horizontal line
#ifdef _ENABLE_SSE2_RECTANGLE_MONO
		for ( x_across = StartX; x_across <= EndX; x_across += c_iVectorSize8 )
		{
			if ( ABE || PIXELMASK )
			{
				vDestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
			}
			
			if ( ABE )
			{
				vpixel = vSemiTransparency16_t<ABRCODE>( vDestPixel, vpixel_temp, c_vLoBitMask, c_vPixelMask, c_vClampMask, c_vShiftHalf_Mask, c_vShiftQuarter_Mask );
				
				if ( SETPIXELMASK )
				{
					vpixel = _mm_or_si128 ( vpixel, vSetPixelMask );
				}
			}
			
			if ( PIXELMASK )
			{
				vStoreMask = _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( vDestPixel, 15 ), vPixelMask ), _mm_cmplt_epi16 ( vx_across, vEndX ) );
			}
			else
			{
				vStoreMask = _mm_cmplt_epi16 ( vx_across, vEndX );
			}
			
			_mm_maskmoveu_si128 ( vpixel, vStoreMask, (char*) ptr );
			vx_across = _mm_add_epi16 ( vx_across, vVectorSize );
			
			ptr += c_iVectorSize8;
		}
#else
		for ( x_across = StartX; x_across <= EndX; x_across += c_iVectorSize )
		{
			if ( ABE || PIXELMASK )
			{
				// read pixel from frame buffer if we need to check mask bit
				DestPixel = *ptr;
			}
			
			// semi-transparency
			if ( ABE )
			{
				pixel = SemiTransparency16_t<ABRCODE> ( DestPixel, pixel_temp );
				
				if ( SETPIXELMASK )
				{
					// check if we should set mask bit when drawing
					pixel |= SETPIXELMASK;
				}
			}
			

			// draw pixel if we can draw to mask pixels or mask bit not set
			if ( PIXELMASK )
			{
				if ( ! ( DestPixel & PIXELMASK ) ) *ptr = pixel;
			}
			else
			{
				*ptr = pixel;
			}
			
			// update pointer for pixel out
			ptr += c_iVectorSize;
		}
#endif
	}
	
	// set the amount of time drawing used up
	BusyCycles = NumberOfPixelsDrawn * 1;
}




template<const long PIXELMASK, const long ABE, const long ABRCODE>
inline void GPU::SelectRectangle3_t ()
{
	if ( GPU_CTRL_Read.MD )
	{
		Draw_Rectangle_t <PIXELMASK,0x8000800080008000LL,ABE,ABRCODE> ();
	}
	else
	{
		Draw_Rectangle_t <PIXELMASK,0,ABE,ABRCODE> ();
	}
}


template<const long ABE, const long ABRCODE>
inline void GPU::SelectRectangle2_t ()
{
	if ( GPU_CTRL_Read.ME )
	{
		SelectRectangle3_t <0x8000,ABE,ABRCODE> ();
	}
	else
	{
		SelectRectangle3_t <0,ABE,ABRCODE> ();
	}
}


template<const long ABE>
void GPU::SelectRectangle_t ()
{
	switch ( GPU_CTRL_Read.ABR )
	{
		case 0:
			SelectRectangle2_t <ABE,0> ();
			break;
			
		case 1:
			SelectRectangle2_t <ABE,1> ();
			break;
			
		case 2:
			SelectRectangle2_t <ABE,2> ();
			break;
			
		case 3:
			SelectRectangle2_t <ABE,3> ();
			break;
	}
}


template<const long ABE>
void GPU::Draw_Rectangle_60_t ()
{
	SelectRectangle_t <ABE> ();
}

template<const long ABE>
void GPU::Draw_Rectangle8x8_70_t ()
{
	w = 8; h = 8;
	//Draw_Rectangle_60 ();
	SelectRectangle_t <ABE> ();
}


template<const long ABE>
void GPU::Draw_Rectangle16x16_78_t ()
{
	w = 16; h = 16;
	//Draw_Rectangle_60 ();
	SelectRectangle_t <ABE> ();
}





//template<const long PIXELMASK, const long SETPIXELMASK, const long ABE, const long ABRCODE, const long TGE, const long TP>
//void GPU::PlotPixel_Texture ( 




template<const long PIXELMASK, const long SETPIXELMASK, const long ABE, const long ABRCODE, const long TGE, const long TP>
inline void GPU::DrawSprite_t ()
{
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;

	// notes: looks like sprite size is same as specified by w/h

	//u32 Pixel,
	
	s32 x0, y0, x1, y1;
	u32 pixel, pixel_temp;
	s64 iU, iV;
	
	u32 TexelIndex;
	
	u32 color_add;
	
	u16 *ptr_texture, *ptr_clut;
	u32 clut_xoffset, clut_yoffset;
	
	u16 *ptr;
	s32 StartX, EndX, StartY, EndY;
	
	//u32 tge;
	
	u32 DestPixel;
	u32 TexCoordX, TexCoordY;
	
#ifdef _ENABLE_SSE2_SPRITE
	static const u32 c_iBitsPerPixel = 5;
	static const u32 c_iShiftHalf_Mask = ~( ( 1 << ( c_iBitsPerPixel - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 2 ) - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 3 ) - 1 ) ) );
	static const u32 c_iShiftQuarter_Mask = ~( ( 3 << ( c_iBitsPerPixel - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 2 ) - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 3 ) - 2 ) ) );
	static const u32 c_iClampMask = ( ( 1 << ( c_iBitsPerPixel ) ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) + ( 1 << ( c_iBitsPerPixel * 3 ) ) );
	static const u32 c_iLoBitMask = ( ( 1 ) + ( 1 << c_iBitsPerPixel ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) );
	static const u32 c_iPixelMask = 0x7fff;
	static const __m128i c_vLoBitMask = _mm_set1_epi16 ( c_iLoBitMask );
	static const __m128i c_vPixelMask = _mm_set1_epi16 ( c_iPixelMask );
	static const __m128i c_vClampMask = _mm_set1_epi16 ( c_iClampMask );
	static const __m128i c_vShiftHalf_Mask = _mm_set1_epi16 ( c_iShiftHalf_Mask );
	static const __m128i c_vShiftQuarter_Mask = _mm_set1_epi16 ( c_iShiftQuarter_Mask );
	__m128i vDestPixel, vPixelMask, vSetPixelMask, vStoreMask;
	__m128i vStartX, vEndX, vx_across, vVectorSize;
	__m128i vpixel, vpixel_temp;
	
	static const __m128i vSeq = _mm_set_epi16 ( 7, 6, 5, 4, 3, 2, 1, 0 );
	vVectorSize = _mm_set1_epi16 ( c_iVectorSize8 );
	
	vpixel = _mm_set1_epi16 ( bgr );
	//PixelMask = _mm_setzero_si128 ();
	//SetPixelMask = _mm_setzero_si128 ();
	if ( PIXELMASK ) { vPixelMask = _mm_set1_epi16 ( 0x8080 ); }
	if ( SETPIXELMASK ) { vSetPixelMask = _mm_set1_epi16 ( 0x8000 ); }
	
	// variables for textures //
	__m128i vTWYTWH, vTWXTWW, vNot_TWH, vNot_TWW;
	__m128i viU, viV, 
#endif

	u32 TWYTWH, TWXTWW, Not_TWH, Not_TWW;
	
	TWYTWH = ( ( TWY & TWH ) << 3 );
	TWXTWW = ( ( TWX & TWW ) << 3 );
	
	Not_TWH = ~( TWH << 3 );
	Not_TWW = ~( TWW << 3 );

	
	//if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	//if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
	
	/////////////////////////////////////////////////////////
	// Get offset into texture page
	//TextureOffset = ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 );
	
	//u32 PixelsPerLine;

	//tge = command_tge;
	//if ( ( bgr & 0x00ffffff ) == 0x00808080 ) tge = 1;
	
	//////////////////////////////////////////////////////
	// Get offset into color lookup table
	//u32 ClutOffset = ( clut_x << 4 ) + ( clut_y << 10 );
	
	clut_xoffset = clut_x << 4;
	ptr_clut = & ( VRAM [ clut_y << 10 ] );
	ptr_texture = & ( VRAM [ ( tpage_tx << 6 ) + ( ( tpage_ty << 8 ) << 10 ) ] );
	
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
		


//#define DEBUG_DRAWSPRITE
#ifdef DEBUG_DRAWSPRITE
	debug << "\r\nTWX=" << TWX << " TWY=" << TWY << " TWW=" << TWW << " TWH=" << TWH << " TextureWindow_X=" << TextureWindow_X << " TextureWindow_Y=" << TextureWindow_Y << " TextureWindow_Width=" << TextureWindow_Width << " TextureWindow_Height=" << TextureWindow_Height;
#endif

	for ( Line = StartY; Line <= EndY; Line++ )
	{
			// need to start texture coord from left again
			iU = u;
			//TexCoordY = (u8) ( ( iV & ~( TWH << 3 ) ) | ( ( TWY & TWH ) << 3 ) );

			TexCoordY = (u8) ( ( iV & Not_TWH ) | ( TWYTWH ) );
			TexCoordY <<= 10;

			ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			

			// draw horizontal line
			//for ( x_across = StartX; x_across <= EndX; x_across++ )
			for ( x_across = StartX; x_across <= EndX; x_across += c_iVectorSize )
			{
					TexCoordX = (u8) ( ( iU & Not_TWW ) | ( TWXTWW ) );
					
					// vars needed: TexCoordX, TexCoordY, clut_xoffset, color_add, ptr_texture, ptr_clut
					switch ( TP )
					{
						case 0:
							//And2 = 0xf;
							//Shift1 = 2; Shift2 = 2;
							//And1 = 3; And2 = 0xf;
							//bgr = ptr_texture [ ( TexCoordX >> Shift1 ) + TexCoordY ];
							//bgr = ptr_clut [ ( clut_xoffset + ( ( bgr >> ( ( TexCoordX & And1 ) << Shift2 ) ) & And2 ) ) & FrameBuffer_XMask ];
							pixel = ptr_texture [ ( TexCoordX >> 2 ) + TexCoordY ];
							pixel = ptr_clut [ ( clut_xoffset + ( ( pixel >> ( ( TexCoordX & 3 ) << 2 ) ) & 0xf ) ) & FrameBuffer_XMask ];
							break;
							
						case 1:
							//And2 = 0xff;
							//Shift1 = 1; Shift2 = 3;
							//And1 = 1; And2 = 0xff;
							pixel = ptr_texture [ ( TexCoordX >> 1 ) + TexCoordY ];
							pixel = ptr_clut [ ( clut_xoffset + ( ( pixel >> ( ( TexCoordX & 1 ) << 3 ) ) & 0xff ) ) & FrameBuffer_XMask ];
							break;
							
						case 2:
							pixel = ptr_texture [ ( TexCoordX ) + TexCoordY ];
							break;
					}
					
					
					if ( pixel )
					{
						if ( ABE || PIXELMASK )
						{
							// read pixel from frame buffer if we need to check mask bit
							DestPixel = *ptr;
						}

						if ( ABE || !TGE )
						{
							pixel_temp = pixel & 0x8000;
						}
				
						//if ( !tge )
						if ( !TGE )
						{
							// brightness calculation
							pixel = ColorMultiply1624 ( pixel, color_add );
						}
						
						// semi-transparency
						//if ( command_abe && ( bgr & 0x8000 ) )
						if ( ABE )
						{
							if ( pixel_temp )
							{
								pixel = SemiTransparency16_t <ABRCODE> ( DestPixel, pixel );
							}
						}
						
						if ( ABE || !TGE )
						{
							// check if we should set mask bit when drawing
							//bgr_temp |= SetPixelMask | ( bgr & 0x8000 );
							pixel |= pixel_temp;
						}
						
						if ( SETPIXELMASK )
						{
							// set pixel mask if specified
							pixel |= SETPIXELMASK;
						}

						// draw pixel if we can draw to mask pixels or mask bit not set
						if ( PIXELMASK )
						{
							if ( ! ( DestPixel & PIXELMASK ) ) *ptr = pixel;
						}
						else
						{
							*ptr = pixel;
						}
					}
					
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


template<const long PIXELMASK, const long SETPIXELMASK, const long ABE, const long ABRCODE, const long TGE, const long TP>
inline void GPU::SelectSprite5_t ()
{
	if ( ( bgr & 0xffffff ) == 0x808080 )
	{
		DrawSprite_t <PIXELMASK, SETPIXELMASK, ABE, ABRCODE, 1, TP> ();
	}
	else
	{
		DrawSprite_t <PIXELMASK, SETPIXELMASK, ABE, ABRCODE, TGE, TP> ();
	}
}



template<const long PIXELMASK, const long ABE, const long ABRCODE, const long TGE, const long TP>
inline void GPU::SelectSprite4_t ()
{
	if ( GPU_CTRL_Read.MD )
	{
		SelectSprite5_t <PIXELMASK, 0x8000, ABE, ABRCODE, TGE, TP> ();
	}
	else
	{
		SelectSprite5_t <PIXELMASK, 0, ABE, ABRCODE, TGE, TP> ();
	}
}


template<const long ABE, const long ABRCODE, const long TGE, const long TP>
inline void GPU::SelectSprite3_t ()
{
	if ( GPU_CTRL_Read.ME )
	{
		SelectSprite4_t<0x8000, ABE, ABRCODE, TGE, TP> ();
	}
	else
	{
		SelectSprite4_t<0, ABE, ABRCODE, TGE, TP> ();
	}
}


template<const long ABE, const long ABRCODE, const long TGE>
inline void GPU::SelectSprite2_t ()
{
	switch ( GPU_CTRL_Read.TP )
	{
		case 0:
			SelectSprite3_t<ABE, ABRCODE, TGE, 0> ();
			break;
			
		case 1:
			SelectSprite3_t<ABE, ABRCODE, TGE, 1> ();
			break;
			
		case 2:
			SelectSprite3_t<ABE, ABRCODE, TGE, 2> ();
			break;
	}
}




template<const long PIXELMASK, const long long SETPIXELMASK, const long ABE, const long ABRCODE>
void GPU::DrawTriangle_Mono_t ()
{	
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;
	static const int c_iVectorSize8 = 8;

	//s32 x0, y0, x1, y1;
	u64 pixel, pixel_temp;
	
	u16 *ptr;
	s64 Temp;
	s64 LeftMostX, RightMostX;
	
	s32 StartX, EndX, StartY, EndY;
	//s64 Error_Left;
	
	u32 DestPixel;
	
#ifdef _ENABLE_SSE2_TRIANGLE_MONO
	static const u32 c_iBitsPerPixel = 5;
	static const u32 c_iShiftHalf_Mask = ~( ( 1 << ( c_iBitsPerPixel - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 2 ) - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 3 ) - 1 ) ) );
	static const u32 c_iShiftQuarter_Mask = ~( ( 3 << ( c_iBitsPerPixel - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 2 ) - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 3 ) - 2 ) ) );
	static const u32 c_iClampMask = ( ( 1 << ( c_iBitsPerPixel ) ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) + ( 1 << ( c_iBitsPerPixel * 3 ) ) );
	static const u32 c_iLoBitMask = ( ( 1 ) + ( 1 << c_iBitsPerPixel ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) );
	static const u32 c_iPixelMask = 0x7fff;
	static const __m128i c_vLoBitMask = _mm_set1_epi16 ( c_iLoBitMask );
	static const __m128i c_vPixelMask = _mm_set1_epi16 ( c_iPixelMask );
	static const __m128i c_vClampMask = _mm_set1_epi16 ( c_iClampMask );
	static const __m128i c_vShiftHalf_Mask = _mm_set1_epi16 ( c_iShiftHalf_Mask );
	static const __m128i c_vShiftQuarter_Mask = _mm_set1_epi16 ( c_iShiftQuarter_Mask );
	__m128i vDestPixel, vPixelMask, vSetPixelMask, vStoreMask;
	__m128i /*vbgr, vbgr_temp,*/ vStartX, vEndX, vx_across, vSeq, vVectorSize;
	__m128i vpixel, vpixel_temp;
	
	vSeq = _mm_set_epi16 ( 7, 6, 5, 4, 3, 2, 1, 0 );
	vVectorSize = _mm_set1_epi16 ( c_iVectorSize8 );
	
	vpixel = _mm_set1_epi16 ( bgr );
	//PixelMask = _mm_setzero_si128 ();
	//SetPixelMask = _mm_setzero_si128 ();
	if ( PIXELMASK ) { vPixelMask = _mm_set1_epi16 ( 0x8080 ); }
	if ( SETPIXELMASK ) { vSetPixelMask = _mm_set1_epi16 ( 0x8000 ); }
#endif


	// get the pixel
	pixel = bgr;
	
	//if ( !PIXELMASK )
	//{
	//	pixel |= ( pixel << 16 ) | ( pixel << 32 ) | ( pixel << 48 );
	//}
	
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

	/////////////////////////////////////////////////
	// swap left and right if they are reversed
	//if ( ( ( x_right + dx_right ) < ( x_left + dx_left ) ) || ( x_right < x_left ) )
	if ( denominator > 0 )
	{
		// x1,y1 is on the right //
		
		//Y1_OnLeft = 0;
		
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

	
	if ( ABE )
	{
#ifdef _ENABLE_SSE2_TRIANGLE_MONO
		vpixel_temp = vpixel;
#else
		pixel_temp = pixel;
#endif
	}
	
	if ( SETPIXELMASK && !ABE )
	{
#ifdef _ENABLE_SSE2_TRIANGLE_MONO
		vpixel = _mm_or_si128 ( vpixel, vSetPixelMask );
#else
		// check if we should set mask bit when drawing
		pixel |= SETPIXELMASK;
#endif
	}
	
	
	//////////////////////////////////////////////
	// draw down to y1
	for ( Line = StartY; Line < EndY; Line++ )
	{

		StartX = _Round( x_left ) >> 32;
		EndX = _Round( x_right ) >> 32;
		
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
			
#ifdef _ENABLE_SSE2_TRIANGLE_MONO
	vx_across = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX );
#endif
			
			// ??
			//if ( EndX >= RightMostX ) EndX--;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;
			

			// draw horizontal line
			// x_left and x_right need to be rounded off
			x_across = StartX;
#ifdef _ENABLE_SSE2_TRIANGLE_MONO
			for ( ; x_across < EndX; x_across += c_iVectorSize8 )
			{
				if ( ABE || PIXELMASK )
				{
					vDestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
				}
				
				if ( ABE )
				{
					//vbgr_temp = vbgr;
					//if ( command_abe ) vbgr_temp = vSemiTransparency16( DestPixel, vbgr_temp, GPU_CTRL_Read.ABR );
					vpixel = vSemiTransparency16_t<ABRCODE>( vDestPixel, vpixel_temp, c_vLoBitMask, c_vPixelMask, c_vClampMask, c_vShiftHalf_Mask, c_vShiftQuarter_Mask );
					
					if ( SETPIXELMASK )
					{
						vpixel = _mm_or_si128 ( vpixel, vSetPixelMask );
					}
				}
				
				if ( PIXELMASK )
				{
					vStoreMask = _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( vDestPixel, 15 ), vPixelMask ), _mm_cmplt_epi16 ( vx_across, vEndX ) );
				}
				else
				{
					vStoreMask = _mm_cmplt_epi16 ( vx_across, vEndX );
				}
				
				_mm_maskmoveu_si128 ( vpixel, vStoreMask, (char*) ptr );
				vx_across = _mm_add_epi16 ( vx_across, vVectorSize );
				
				ptr += c_iVectorSize8;
			}
#else
			for ( ; x_across < EndX; x_across += c_iVectorSize )
			{
				if ( ABE || PIXELMASK )
				{
					// read pixel from frame buffer if we need to check mask bit
					DestPixel = *ptr;
				}
				
				// semi-transparency
				if ( ABE )
				{
					pixel = SemiTransparency16_t<ABRCODE> ( DestPixel, pixel_temp );
					
					if ( SETPIXELMASK )
					{
						// check if we should set mask bit when drawing
						pixel |= SETPIXELMASK;
					}
				}
				

				// draw pixel if we can draw to mask pixels or mask bit not set
				if ( PIXELMASK )
				{
					if ( ! ( DestPixel & PIXELMASK ) ) *ptr = pixel;
				}
				else
				{
					*ptr = pixel;
				}
				
				//ptr++;
				ptr += c_iVectorSize;
			}
			
#endif
			
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
			
#ifdef _ENABLE_SSE2_TRIANGLE_MONO
	vx_across = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX );
#endif
			
			// ??
			//if ( EndX >= RightMostX ) EndX--;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;
			

			// draw horizontal line
			// x_left and x_right need to be rounded off
			x_across = StartX;
#ifdef _ENABLE_SSE2_TRIANGLE_MONO
			for ( ; x_across < EndX; x_across += c_iVectorSize8 )
			{
				if ( ABE || PIXELMASK )
				{
					vDestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
				}
				
				if ( ABE )
				{
					//vbgr_temp = vbgr;
					//if ( command_abe ) vbgr_temp = vSemiTransparency16( DestPixel, vbgr_temp, GPU_CTRL_Read.ABR );
					vpixel = vSemiTransparency16_t<ABRCODE>( vDestPixel, vpixel_temp, c_vLoBitMask, c_vPixelMask, c_vClampMask, c_vShiftHalf_Mask, c_vShiftQuarter_Mask );
					
					if ( SETPIXELMASK )
					{
						vpixel = _mm_or_si128 ( vpixel, vSetPixelMask );
					}
				}
				
				if ( PIXELMASK )
				{
					vStoreMask = _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( vDestPixel, 15 ), vPixelMask ), _mm_cmplt_epi16 ( vx_across, vEndX ) );
				}
				else
				{
					vStoreMask = _mm_cmplt_epi16 ( vx_across, vEndX );
				}
				
				_mm_maskmoveu_si128 ( vpixel, vStoreMask, (char*) ptr );
				vx_across = _mm_add_epi16 ( vx_across, vVectorSize );
				
				ptr += c_iVectorSize8;
			}
#else
			for ( ; x_across < EndX; x_across += c_iVectorSize )
			{
				if ( ABE || PIXELMASK )
				{
					// read pixel from frame buffer if we need to check mask bit
					DestPixel = *ptr;
				}
				
				// semi-transparency
				if ( ABE )
				{
					pixel = SemiTransparency16_t<ABRCODE> ( DestPixel, pixel_temp );
					
					if ( SETPIXELMASK )
					{
						// check if we should set mask bit when drawing
						pixel |= SETPIXELMASK;
					}
				}
				

				// draw pixel if we can draw to mask pixels or mask bit not set
				if ( PIXELMASK )
				{
					if ( ! ( DestPixel & PIXELMASK ) ) *ptr = pixel;
				}
				else
				{
					*ptr = pixel;
				}
				
				//ptr++;
				ptr += c_iVectorSize;
			}
#endif
		}
		
		/////////////////////////////////////
		// update x on left and right
		x_left += dx_left;
		x_right += dx_right;
	}

}



template<const long PIXELMASK, const long ABE, const long ABRCODE>
inline void GPU::SelectTriangle_Mono3_t ()
{
	if ( GPU_CTRL_Read.MD )
	{
		DrawTriangle_Mono_t <PIXELMASK,0x8000800080008000LL,ABE,ABRCODE> ();
	}
	else
	{
		DrawTriangle_Mono_t <PIXELMASK,0,ABE,ABRCODE> ();
	}
}


template<const long ABE, const long ABRCODE>
inline void GPU::SelectTriangle_Mono2_t ()
{
	if ( GPU_CTRL_Read.ME )
	{
		SelectTriangle_Mono3_t <0x8000,ABE,ABRCODE> ();
	}
	else
	{
		SelectTriangle_Mono3_t <0,ABE,ABRCODE> ();
	}
}


template<const long ABE>
void GPU::SelectTriangle_Mono_t ()
{
	switch ( GPU_CTRL_Read.ABR )
	{
		case 0:
			SelectTriangle_Mono2_t <ABE,0> ();
			break;
			
		case 1:
			SelectTriangle_Mono2_t <ABE,1> ();
			break;
			
		case 2:
			SelectTriangle_Mono2_t <ABE,2> ();
			break;
			
		case 3:
			SelectTriangle_Mono2_t <ABE,3> ();
			break;
	}
}


template<const long ABE>
void GPU::Draw_MonoTriangle_20_t ()
{
	//DrawTriangle_Mono ();
	SelectTriangle_Mono_t <ABE> ();
	
	// *** TODO *** calculate cycles to draw here
	// check for alpha blending - add in an extra cycle for this for now
	if ( command_abe )
	{
		BusyCycles += NumberOfPixelsDrawn * dAlphaBlending_CyclesPerPixel;
	}
	
	// add in cycles to draw mono-triangle
	BusyCycles += NumberOfPixelsDrawn * dMonoTriangle_20_CyclesPerPixel;
}

template<const long ABE>
void GPU::Draw_MonoRectangle_28_t ()
{
	x_save [ 0 ] = x0; x_save [ 1 ] = x1; x_save [ 2 ] = x2; x_save [ 3 ] = x3;
	y_save [ 0 ] = y0; y_save [ 1 ] = y1; y_save [ 2 ] = y2; y_save [ 3 ] = y3;
	bgr_save [ 0 ] = bgr;
	
	//Draw_MonoTriangle_20 ();
	Draw_MonoTriangle_20_t <ABE> ();
	
	x0 = x_save [ 1 ];
	y0 = y_save [ 1 ];
	x1 = x_save [ 2 ];
	y1 = y_save [ 2 ];
	x2 = x_save [ 3 ];
	y2 = y_save [ 3 ];
	
	bgr = bgr_save [ 0 ];
	
	//Draw_MonoTriangle_20 ();
	Draw_MonoTriangle_20_t <ABE> ();
}




template<const long PIXELMASK, const long SETPIXELMASK, const long ABE, const long ABRCODE, const long TGE, const long TP>
void GPU::DrawTriangle_Texture_t ()
{
	//debug << "DrawTriangle_Texture_t->";
	
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;

	//s32 x0, y0, x1, y1;
	u32 pixel, pixel_temp;
	//s64 iU, iV;
	//s64 x_left, x_right, dx_left, dx_right, U_left, U_right, dU_left, dU_right, V_left, V_right, dV_left, dV_right;
	
	u32 clut_xoffset;

	//u32 Pixel, TexelIndex;
	//u32 Y1_OnLeft;
	
	u32 color_add;
	
	u16 *ptr_texture, *ptr_clut;
	u16 *ptr;
	
	s32 StartX, EndX, StartY, EndY;
	s64 Temp, TMin, TMax;
	s64 LeftMostX, RightMostX;

	u32 DestPixel;
	
	//u32 PixelMask = 0, SetPixelMask = 0;
	//u32 Shift1 = 0, Shift2 = 0, And1 = 0, And2 = 0;
	

	s64 Error_Left;
	
	//s64 TexOffset_X, TexOffset_Y;
	u32 TexCoordX, TexCoordY;
	
	u32 TWYTWH, TWXTWW, Not_TWH, Not_TWW;
	
	TWYTWH = ( ( TWY & TWH ) << 3 );
	TWXTWW = ( ( TWX & TWW ) << 3 );
	
	Not_TWH = ~( TWH << 3 );
	Not_TWW = ~( TWW << 3 );
	
	//if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	//if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;
	
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
	//Y1_OnLeft = 1;
	
	color_add = bgr;

	//////////////////////////////////////////////////////
	// Get offset into color lookup table
	//u32 ClutOffset = ( clut_x << 4 ) + ( clut_y << 10 );
	
	
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
	
	
	// calculate across
	s64 t0 = y1 - y2;
	s64 t1 = y0 - y2;
	s64 denominator = ( ( x0 - x2 ) * t0 ) - ( ( x1 - x2 ) * t1 );
	if ( denominator )
	{
		dU_across = ( ( (s64) ( ( ( u0 - u2 ) * t0 ) - ( ( u1 - u2 ) * t1 ) ) ) << 24 ) / denominator;
		dV_across = ( ( (s64) ( ( ( v0 - v2 ) * t0 ) - ( ( v1 - v2 ) * t1 ) ) ) << 24 ) / denominator;
	}
	
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



			// draw horizontal line
			// x_left and x_right need to be rounded off
			for ( x_across = StartX; x_across < EndX; x_across++ )
			{
					TexCoordY = (u8) ( ( ( _Round24 ( iV ) >> 24 ) & Not_TWH ) | TWYTWH );
					TexCoordX = (u8) ( ( ( _Round24 ( iU ) >> 24 ) & Not_TWW ) | TWXTWW );
					
					// *** testing ***
					//debug << dec << "\r\nTexCoordX=" << TexCoordX << " TexCoordY=" << TexCoordY;
					
					switch ( TP )
					{
						case 0:
							pixel = ptr_texture [ ( TexCoordX >> 2 ) + ( TexCoordY << 10 ) ];
							pixel = ptr_clut [ ( clut_xoffset + ( ( pixel >> ( ( TexCoordX & 3 ) << 2 ) ) & 0xf ) ) & FrameBuffer_XMask ];
							break;
							
						case 1:
							pixel = ptr_texture [ ( TexCoordX >> 1 ) + ( TexCoordY << 10 ) ];
							pixel = ptr_clut [ ( clut_xoffset + ( ( pixel >> ( ( TexCoordX & 1 ) << 3 ) ) & 0xff ) ) & FrameBuffer_XMask ];
							break;
							
						case 2:
							pixel = ptr_texture [ ( TexCoordX ) + ( TexCoordY << 10 ) ];
							break;
					}
					
					
					if ( pixel )
					{
						if ( ABE || PIXELMASK )
						{
							// read pixel from frame buffer if we need to check mask bit
							DestPixel = *ptr;
						}

						if ( ABE || !TGE )
						{
							pixel_temp = pixel & 0x8000;
						}
				
						//if ( !tge )
						if ( !TGE )
						{
							// brightness calculation
							pixel = ColorMultiply1624 ( pixel, color_add );
						}
						
						// semi-transparency
						//if ( command_abe && ( bgr & 0x8000 ) )
						if ( ABE )
						{
							if ( pixel_temp )
							{
								pixel = SemiTransparency16_t <ABRCODE> ( DestPixel, pixel );
							}
						}
						
						if ( ABE || !TGE )
						{
							// check if we should set mask bit when drawing
							//bgr_temp |= SetPixelMask | ( bgr & 0x8000 );
							pixel |= pixel_temp;
						}
						
						if ( SETPIXELMASK )
						{
							// set pixel mask if specified
							pixel |= SETPIXELMASK;
						}

						// draw pixel if we can draw to mask pixels or mask bit not set
						if ( PIXELMASK )
						{
							if ( ! ( DestPixel & PIXELMASK ) ) *ptr = pixel;
						}
						else
						{
							*ptr = pixel;
						}
					}
					
					/////////////////////////////////////////////////////
					// update number of cycles used to draw polygon
					//NumberOfPixelsDrawn++;
				//}
				
				/////////////////////////////////////////////////////////
				// interpolate texture coords
				iU += dU_across;
				iV += dV_across;
				
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
	//if ( Y1_OnLeft )
	if ( denominator < 0 )
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
			
			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across < EndX; x_across++ )
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
					TexCoordY = (u8) ( ( ( _Round24 ( iV ) >> 24 ) & Not_TWH ) | TWYTWH );
					TexCoordX = (u8) ( ( ( _Round24 ( iU ) >> 24 ) & Not_TWW ) | TWXTWW );
					
					// *** testing ***
					//debug << dec << "\r\nTexCoordX=" << TexCoordX << " TexCoordY=" << TexCoordY;
					
					switch ( TP )
					{
						case 0:
							pixel = ptr_texture [ ( TexCoordX >> 2 ) + ( TexCoordY << 10 ) ];
							pixel = ptr_clut [ ( clut_xoffset + ( ( pixel >> ( ( TexCoordX & 3 ) << 2 ) ) & 0xf ) ) & FrameBuffer_XMask ];
							break;
							
						case 1:
							pixel = ptr_texture [ ( TexCoordX >> 1 ) + ( TexCoordY << 10 ) ];
							pixel = ptr_clut [ ( clut_xoffset + ( ( pixel >> ( ( TexCoordX & 1 ) << 3 ) ) & 0xff ) ) & FrameBuffer_XMask ];
							break;
							
						case 2:
							pixel = ptr_texture [ ( TexCoordX ) + ( TexCoordY << 10 ) ];
							break;
					}
					
					
					if ( pixel )
					{
						if ( ABE || PIXELMASK )
						{
							// read pixel from frame buffer if we need to check mask bit
							DestPixel = *ptr;
						}

						if ( ABE || !TGE )
						{
							pixel_temp = pixel & 0x8000;
						}
				
						//if ( !tge )
						if ( !TGE )
						{
							// brightness calculation
							pixel = ColorMultiply1624 ( pixel, color_add );
						}
						
						// semi-transparency
						//if ( command_abe && ( bgr & 0x8000 ) )
						if ( ABE )
						{
							if ( pixel_temp )
							{
								pixel = SemiTransparency16_t <ABRCODE> ( DestPixel, pixel );
							}
						}
						
						if ( ABE || !TGE )
						{
							// check if we should set mask bit when drawing
							//bgr_temp |= SetPixelMask | ( bgr & 0x8000 );
							pixel |= pixel_temp;
						}
						
						if ( SETPIXELMASK )
						{
							// set pixel mask if specified
							pixel |= SETPIXELMASK;
						}

						// draw pixel if we can draw to mask pixels or mask bit not set
						if ( PIXELMASK )
						{
							if ( ! ( DestPixel & PIXELMASK ) ) *ptr = pixel;
						}
						else
						{
							*ptr = pixel;
						}
					}
					
				/////////////////////////////////////////////////////////
				// interpolate texture coords
				iU += dU_across;
				iV += dV_across;
				
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



template<const long PIXELMASK, const long SETPIXELMASK, const long ABE, const long ABRCODE, const long TGE, const long TP>
inline void GPU::SelectTriangle_Texture5_t ()
{
	//debug << "SelectTriangle_Texture5_t->";
	
	if ( ( bgr & 0xffffff ) == 0x808080 )
	{
		DrawTriangle_Texture_t <PIXELMASK, SETPIXELMASK, ABE, ABRCODE, 1, TP> ();
	}
	else
	{
		DrawTriangle_Texture_t <PIXELMASK, SETPIXELMASK, ABE, ABRCODE, TGE, TP> ();
	}
}



template<const long PIXELMASK, const long ABE, const long ABRCODE, const long TGE, const long TP>
inline void GPU::SelectTriangle_Texture4_t ()
{
	//debug << "SelectTriangle_Texture4_t->";
	
	if ( GPU_CTRL_Read.MD )
	{
		SelectTriangle_Texture5_t <PIXELMASK, 0x8000, ABE, ABRCODE, TGE, TP> ();
	}
	else
	{
		SelectTriangle_Texture5_t <PIXELMASK, 0, ABE, ABRCODE, TGE, TP> ();
	}
}


template<const long ABE, const long ABRCODE, const long TGE, const long TP>
inline void GPU::SelectTriangle_Texture3_t ()
{
	//debug << "SelectTriangle_Texture3_t->";
	
	if ( GPU_CTRL_Read.ME )
	{
		SelectTriangle_Texture4_t<0x8000, ABE, ABRCODE, TGE, TP> ();
	}
	else
	{
		SelectTriangle_Texture4_t<0, ABE, ABRCODE, TGE, TP> ();
	}
}


template<const long ABE, const long ABRCODE, const long TGE>
inline void GPU::SelectTriangle_Texture2_t ()
{
	//debug << "SelectTriangle_Texture2_t->";
	
	switch ( tpage_tp )
	{
		case 0:
			SelectTriangle_Texture3_t<ABE, ABRCODE, TGE, 0> ();
			break;
			
		case 1:
			SelectTriangle_Texture3_t<ABE, ABRCODE, TGE, 1> ();
			break;
			
		case 2:
			SelectTriangle_Texture3_t<ABE, ABRCODE, TGE, 2> ();
			break;
	}
}


template<const long ABE, const long TGE>
void GPU::SelectTriangle_Texture_t ()
{
	//debug << "SelectTriangle_Texture_t->";
	
	switch ( tpage_abr )
	{
		case 0:
			SelectTriangle_Texture2_t<ABE, 0, TGE> ();
			break;
			
		case 1:
			SelectTriangle_Texture2_t<ABE, 1, TGE> ();
			break;
			
		case 2:
			SelectTriangle_Texture2_t<ABE, 2, TGE> ();
			break;
			
		case 3:
			SelectTriangle_Texture2_t<ABE, 3, TGE> ();
			break;
	}
}



template<const long ABE, const long TGE>
void GPU::Draw_TextureTriangle_24_t ()
{
	//debug << "Draw_TextureTriangle_24_t->";
	//u32 tge;
	//tge = command_tge;
	
	//if ( ( bgr & 0x00ffffff ) == 0x00808080 )
	//{
	//	command_tge = 1;
	//}
	
	//DrawTriangle_Texture ();
	SelectTriangle_Texture_t <ABE,TGE> ();
	
	// restore tge
	//command_tge = tge;
	
	// check for alpha blending - add in an extra cycle for this for now
	if ( ABE )
	{
		BusyCycles += NumberOfPixelsDrawn * dAlphaBlending_CyclesPerPixel;
	}
	
	// check for brightness calculation
	if ( !TGE )
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

template<const long ABE, const long TGE>
void GPU::Draw_TextureRectangle_2c_t ()
{
	//debug << "\r\nDraw_TextureRectangle_2c_t->";

	x_save [ 0 ] = x0; x_save [ 1 ] = x1; x_save [ 2 ] = x2; x_save [ 3 ] = x3;
	y_save [ 0 ] = y0; y_save [ 1 ] = y1; y_save [ 2 ] = y2; y_save [ 3 ] = y3;
	bgr_save [ 0 ] = bgr; bgr_save [ 1 ] = bgr0; bgr_save [ 2 ] = bgr1; bgr_save [ 3 ] = bgr2; bgr_save [ 4 ] = bgr3;
	u_save [ 0 ] = u0; u_save [ 1 ] = u1; u_save [ 2 ] = u2; u_save [ 3 ] = u3;
	v_save [ 0 ] = v0; v_save [ 1 ] = v1; v_save [ 2 ] = v2; v_save [ 3 ] = v3;
	
	//Draw_TextureTriangle_24 ();
	Draw_TextureTriangle_24_t <ABE,TGE> ();

	x0 = x_save [ 1 ];
	y0 = y_save [ 1 ];
	x1 = x_save [ 2 ];
	y1 = y_save [ 2 ];
	x2 = x_save [ 3 ];
	y2 = y_save [ 3 ];


	bgr = bgr_save [ 0 ];
	
	u0 = u_save [ 1 ];
	v0 = v_save [ 1 ];
	u1 = u_save [ 2 ];
	v1 = v_save [ 2 ];
	u2 = u_save [ 3 ];
	v2 = v_save [ 3 ];
	
	//Draw_TextureTriangle_24 ();
	Draw_TextureTriangle_24_t <ABE,TGE> ();
}




template<const long PIXELMASK, const long SETPIXELMASK, const long ABE, const long ABRCODE, const long DTD>
void GPU::DrawTriangle_Gradient_t ()
{
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;
	static const int c_iVectorSize4 = 4;
	static const int c_iVectorSize8 = 8;

	u32 pixel;
	
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
	
	
	s64 Red, Green, Blue;
	
	u32 DestPixel;
	
#ifdef _ENABLE_SSE2_TRIANGLE_GRADIENT
	static const u32 c_iBitsPerPixel = 5;
	static const u32 c_iShiftHalf_Mask = ~( ( 1 << ( c_iBitsPerPixel - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 2 ) - 1 ) ) + ( 1 << ( ( c_iBitsPerPixel * 3 ) - 1 ) ) );
	static const u32 c_iShiftQuarter_Mask = ~( ( 3 << ( c_iBitsPerPixel - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 2 ) - 2 ) ) + ( 3 << ( ( c_iBitsPerPixel * 3 ) - 2 ) ) );
	static const u32 c_iClampMask = ( ( 1 << ( c_iBitsPerPixel ) ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) + ( 1 << ( c_iBitsPerPixel * 3 ) ) );
	static const u32 c_iLoBitMask = ( ( 1 ) + ( 1 << c_iBitsPerPixel ) + ( 1 << ( c_iBitsPerPixel * 2 ) ) );
	static const u32 c_iPixelMask = 0x7fff;
	static const __m128i c_vLoBitMask = _mm_set1_epi16 ( c_iLoBitMask );
	static const __m128i c_vPixelMask = _mm_set1_epi16 ( c_iPixelMask );
	static const __m128i c_vClampMask = _mm_set1_epi16 ( c_iClampMask );
	static const __m128i c_vShiftHalf_Mask = _mm_set1_epi16 ( c_iShiftHalf_Mask );
	static const __m128i c_vShiftQuarter_Mask = _mm_set1_epi16 ( c_iShiftQuarter_Mask );
	__m128i vDestPixel, vPixelMask, vSetPixelMask, vStoreMask;
	__m128i vStartX, vEndX, vx_across, vVectorSize;
	__m128i vpixel, vpixel_temp;
	
	static const __m128i vSeq = _mm_set_epi16 ( 7, 6, 5, 4, 3, 2, 1, 0 );
	vVectorSize = _mm_set1_epi16 ( c_iVectorSize8 );
	
	//vpixel = _mm_set1_epi16 ( bgr );
	//PixelMask = _mm_setzero_si128 ();
	//SetPixelMask = _mm_setzero_si128 ();
	if ( PIXELMASK ) { vPixelMask = _mm_set1_epi16 ( 0x8080 ); }
	if ( SETPIXELMASK ) { vSetPixelMask = _mm_set1_epi16 ( 0x8000 ); }
	
	static const __m128i vSeq32_1 = _mm_set_epi32 ( 3, 2, 1, 0 );
	static const __m128i vSeq32_2 = _mm_set_epi32 ( 7, 6, 5, 4 );
	
	// variables needed for gradient //
	
	s16 *vDitherArray_add, *vDitherArray_sub, *vDitherLine_add, *vDitherLine_sub;
	__m128i vR_Start, vG_Start, vB_Start, viR, viG, viB, vDitherValue_add, vDitherValue_sub, vdR_across4, vdG_across4, vdB_across4, vRed, vGreen, vBlue;
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
	
	//DitherArray = c_iDitherZero;
	
	if ( DTD )
	{
#ifdef _ENABLE_SSE2_TRIANGLE_GRADIENT
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
		
#ifdef _ENABLE_SSE2_TRIANGLE_GRADIENT
		vdR_across4 = _mm_set1_epi32 ( dR_across * c_iVectorSize4 );
		vdG_across4 = _mm_set1_epi32 ( dG_across * c_iVectorSize4 );
		vdB_across4 = _mm_set1_epi32 ( dB_across * c_iVectorSize4 );
		vR_Start = _mm_set_epi32 ( dR_across * 3, dR_across * 2, dR_across, 0 );
		vG_Start = _mm_set_epi32 ( dG_across * 3, dG_across * 2, dG_across, 0 );
		vB_Start = _mm_set_epi32 ( dB_across * 3, dB_across * 2, dB_across, 0 );
#endif
	}
	
	//debug << dec << "\r\nx0=" << x0 << " y0=" << y0 << " x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2;
	//debug << "\r\nfixed: denominator=" << denominator;
	
	/////////////////////////////////////////////////
	// draw top part of triangle
	

	/////////////////////////////////////////////
	// init x on the left and right
	

	////////////////////////////////////
	// get slopes
	
	if ( y1 - y0 )
	{
		x_left = ( ((s64)x0) << 32 );
		x_right = x_left;
		
		R_left = ( ((s64)r0) << 24 );
		G_left = ( ((s64)g0) << 24 );
		B_left = ( ((s64)b0) << 24 );
		//R_right = R_left;
		//G_right = G_left;
		//B_right = B_left;
		
		if ( denominator < 0 )
		{
			// y1 is on the left //
			dx_left = (((s64)( x1 - x0 )) << 32 ) / ((s64)( y1 - y0 ));
			dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
			
			dR_left = (((s64)( r1 - r0 )) << 24 ) / ((s64)( y1 - y0 ));
			dG_left = (((s64)( g1 - g0 )) << 24 ) / ((s64)( y1 - y0 ));
			dB_left = (((s64)( b1 - b0 )) << 24 ) / ((s64)( y1 - y0 ));
		}
		else
		{
			// y1 is on the right //
			dx_right = (((s64)( x1 - x0 )) << 32 ) / ((s64)( y1 - y0 ));
			dx_left = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
			
			dR_left = (((s64)( r2 - r0 )) << 24 ) / ((s64)( y2 - y0 ));
			dG_left = (((s64)( g2 - g0 )) << 24 ) / ((s64)( y2 - y0 ));
			dB_left = (((s64)( b2 - b0 )) << 24 ) / ((s64)( y2 - y0 ));
		}
	}
	else
	{
		// top is flat //
		
		if ( denominator < 0 )
		{
			// y1 is on the left //
			
			// change x_left and x_right where y1 is on left
			//x_left = ( ((s64)x1) << 32 );
			x_right = ( ((s64)x0) << 32 );
			
			/*
			R_left = ( ((s64)r1) << 24 );
			G_left = ( ((s64)g1) << 24 );
			B_left = ( ((s64)b1) << 24 );
			*/
		}
		else
		{
			// y0 is on the left //
			
			x_right = ( ((s64)x1) << 32 );
			x_left = ( ((s64)x0) << 32 );
			
			R_left = ( ((s64)r0) << 24 );
			G_left = ( ((s64)g0) << 24 );
			B_left = ( ((s64)b0) << 24 );
		}
		
		// this means that x0 and x1 are on the same line
		// so if the height of the entire polygon is zero, then we are done
		if ( y2 - y1 )
		{
			if ( denominator < 0 )
			{
				// y1 is on the left //
				//dx_left = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
				dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
			
				//dR_left = (((s64)( r2 - r1 )) << 24 ) / ((s64)( y2 - y1 ));
				//dG_left = (((s64)( g2 - g1 )) << 24 ) / ((s64)( y2 - y1 ));
				//dB_left = (((s64)( b2 - b1 )) << 24 ) / ((s64)( y2 - y1 ));
				
			}
			else
			{
				// y1 is on the right //
				dx_right = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
				dx_left = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
				
				dR_left = (((s64)( r2 - r0 )) << 24 ) / ((s64)( y2 - y0 ));
				dG_left = (((s64)( g2 - g0 )) << 24 ) / ((s64)( y2 - y0 ));
				dB_left = (((s64)( b2 - b0 )) << 24 ) / ((s64)( y2 - y0 ));
			}
		}
	}

	/////////////////////////////////////////////////
	// swap left and right if they are reversed
	/*
	if ( denominator > 0 )
	{
		// x1,y1 is on the right //
		
		Swap ( x_left, x_right );
		Swap ( dx_left, dx_right );

		Swap ( dR_left, dR_right );
		Swap ( dG_left, dG_right );
		Swap ( dB_left, dB_right );

		Swap ( R_left, R_right );
		Swap ( G_left, G_right );
		Swap ( B_left, B_right );
	}
	else
	{
		// x1, y1 is on the left //
	}
	*/
	
	
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
		
		//R_right += dR_right * Temp;
		//G_right += dG_right * Temp;
		//B_right += dB_right * Temp;
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
			}
			
			ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			
			if ( DTD )
			{
#ifdef _ENABLE_SSE2_TRIANGLE_GRADIENT
				vDitherValue_add = _mm_loadu_si128 ((__m128i const*) ( &(vDitherArray_add [ ( StartX & 0x3 ) + ( ( Line & 0x3 ) << 4 ) ]) ));
				vDitherValue_sub = _mm_loadu_si128 ((__m128i const*) ( &(vDitherArray_sub [ ( StartX & 0x3 ) + ( ( Line & 0x3 ) << 4 ) ]) ));
#else
				DitherLine = & ( DitherArray [ ( Line & 0x3 ) << 2 ] );
#endif
			}
			
			if ( StartX == EndX && !dx_left ) EndX += 1;
			
#ifdef _ENABLE_SSE2_TRIANGLE_GRADIENT
	vx_across = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX );
	
	viR = _mm_add_epi32 ( vR_Start, _mm_set1_epi32 ( iR ) );
	viG = _mm_add_epi32 ( vG_Start, _mm_set1_epi32 ( iG ) );
	viB = _mm_add_epi32 ( vB_Start, _mm_set1_epi32 ( iB ) );
#endif

			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;
			

			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across < EndX; x_across++ )
#ifdef _ENABLE_SSE2_TRIANGLE_GRADIENT
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize8 )
			{
				if ( ABE || PIXELMASK )
				{
					vDestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
				}
				
				if ( DTD )
				{
					vRed = _mm_srai_epi32 ( viR, 16 );
					viR = _mm_add_epi32 ( viR, vdR_across4 );
					vRed = _mm_packs_epi32 ( vRed, _mm_srai_epi32 ( viR, 16 ) );
					vRed = _mm_adds_epu16 ( vRed, vDitherValue_add );
					vRed = _mm_subs_epu16 ( vRed, vDitherValue_sub );
					
					vGreen = _mm_srai_epi32 ( viG, 16 );
					viG = _mm_add_epi32 ( viG, vdG_across4 );
					vGreen = _mm_packs_epi32 ( vGreen, _mm_srai_epi32 ( viG, 16 ) );
					vGreen = _mm_adds_epu16 ( vGreen, vDitherValue_add );
					vGreen = _mm_subs_epu16 ( vGreen, vDitherValue_sub );
					
					vBlue = _mm_srai_epi32 ( viB, 16 );
					viB = _mm_add_epi32 ( viB, vdB_across4 );
					vBlue = _mm_packs_epi32 ( vBlue, _mm_srai_epi32 ( viB, 16 ) );
					vBlue = _mm_adds_epu16 ( vBlue, vDitherValue_add );
					vBlue = _mm_subs_epu16 ( vBlue, vDitherValue_sub );
					
					vRed = _mm_srli_epi16 ( vRed, 11 );
					vGreen = _mm_srli_epi16 ( vGreen, 11 );
					vBlue = _mm_srli_epi16 ( vBlue, 11 );
					
				}
				else
				{
					vRed = _mm_srli_epi32 ( viR, 27 );
					viR = _mm_add_epi32 ( viR, vdR_across4 );
					vRed = _mm_packs_epi32 ( vRed, _mm_srli_epi32 ( viR, 27 ) );
					
					vGreen = _mm_srli_epi32 ( viG, 27 );
					viG = _mm_add_epi32 ( viG, vdG_across4 );
					vGreen = _mm_packs_epi32 ( vGreen, _mm_srli_epi32 ( viG, 27 ) );
					
					vBlue = _mm_srli_epi32 ( viB, 27 );
					viB = _mm_add_epi32 ( viB, vdB_across4 );
					vBlue = _mm_packs_epi32 ( vBlue, _mm_srli_epi32 ( viB, 27 ) );
				}
					
				//vRed = _mm_slli_epi16 ( vRed, 0 );
				vGreen = _mm_slli_epi16 ( vGreen, 5 );
				vBlue = _mm_slli_epi16 ( vBlue, 10 );
				
				vpixel = _mm_or_si128 ( _mm_or_si128 ( vRed, vGreen ), vBlue );
				
				if ( ABE )
				{
					vpixel = vSemiTransparency16_t<ABRCODE>( vDestPixel, vpixel, c_vLoBitMask, c_vPixelMask, c_vClampMask, c_vShiftHalf_Mask, c_vShiftQuarter_Mask );
				}
				
				if ( SETPIXELMASK )
				{
					vpixel = _mm_or_si128 ( vpixel, vSetPixelMask );
				}
				
				if ( PIXELMASK )
				{
					vStoreMask = _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( vDestPixel, 15 ), vPixelMask ), _mm_cmplt_epi16 ( vx_across, vEndX ) );
				}
				else
				{
					vStoreMask = _mm_cmplt_epi16 ( vx_across, vEndX );
				}
				
				_mm_maskmoveu_si128 ( vpixel, vStoreMask, (char*) ptr );
				
				vx_across = _mm_add_epi16 ( vx_across, vVectorSize );
				
				viR = _mm_add_epi32 ( viR, vdR_across4 );
				//viR2 = _mm_add_epi32 ( viR2, vdR_across );
				viG = _mm_add_epi32 ( viG, vdG_across4 );
				//viG2 = _mm_add_epi32 ( viG2, vdG_across );
				viB = _mm_add_epi32 ( viB, vdB_across4 );
				//viB2 = _mm_add_epi32 ( viB2, vdB_across );
				
				ptr += c_iVectorSize8;
			}
#else
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
				if ( ABE || PIXELMASK )
				{
					// read pixel from frame buffer if we need to check mask bit
					DestPixel = *ptr;
				}
					
				if ( DTD )
				{
					DitherValue = DitherLine [ x_across & 0x3 ];
					
					// perform dither
					Red = iR + DitherValue;
					Green = iG + DitherValue;
					Blue = iB + DitherValue;
					
					// perform shift
					Red >>= 27;
					Green >>= 27;
					Blue >>= 27;
					
					// if dithering, perform signed clamp to 5 bits
					Red = AddSignedClamp<s64,5> ( Red );
					Green = AddSignedClamp<s64,5> ( Green );
					Blue = AddSignedClamp<s64,5> ( Blue );
				}
				else
				{
					// perform shift
					Red = ( iR >> 27 );
					Green = ( iG >> 27 );
					Blue = ( iB >> 27 );
				}
					
				// combine
				pixel = ( Blue << 10 ) | ( Green << 5 ) | Red;
					
					
				// *** testing ***
				//debug << "\r\nDestPixel=" << hex << DestPixel;
				
					// *** testing ***
					//debug << " (before)bgr_temp=" << hex << bgr_temp;
					
				// semi-transparency
				if ( ABE )
				{
					pixel = SemiTransparency16_t<ABRCODE> ( DestPixel, pixel );
				}
					
				if ( SETPIXELMASK )
				{
					// check if we should set mask bit when drawing
					pixel |= SETPIXELMASK;
				}

					// *** testing ***
					//debug << " (before)bgr_temp=" << hex << bgr_temp;
					//debug << " SetPixelMask=" << SetPixelMask << " PixelMask=" << PixelMask << " DestPixel=" << DestPixel;
					
				if ( PIXELMASK )
				{
					// draw pixel if we can draw to mask pixels or mask bit not set
					if ( ! ( DestPixel & PIXELMASK ) ) *ptr = pixel;
				}
				else
				{
					*ptr = pixel;
				}
						
				iR += dR_across;
				iG += dG_across;
				iB += dB_across;
				
				//ptr++;
				ptr += c_iVectorSize;
			}
#endif
		}
		
		
		/////////////////////////////////////
		// update x on left and right
		x_left += dx_left;
		x_right += dx_right;
		
		R_left += dR_left;
		G_left += dG_left;
		B_left += dB_left;
		//R_right += dR_right;
		//G_right += dG_right;
		//B_right += dB_right;
	}

	
	////////////////////////////////////////////////
	// draw bottom part of triangle

	/////////////////////////////////////////////
	// init x on the left and right
	
	//////////////////////////////////////////////////////
	// check if y1 is on the left or on the right
	if ( denominator < 0 )
	{
		// y1 is on the left //
		
		x_left = ( ((s64)x1) << 32 );

		R_left = ( ((s64)r1) << 24 );
		G_left = ( ((s64)g1) << 24 );
		B_left = ( ((s64)b1) << 24 );
		
		if ( y2 - y1 )
		{
			dx_left = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			
			dR_left = (((s64)( r2 - r1 )) << 24 ) / ((s64)( y2 - y1 ));
			dG_left = (((s64)( g2 - g1 )) << 24 ) / ((s64)( y2 - y1 ));
			dB_left = (((s64)( b2 - b1 )) << 24 ) / ((s64)( y2 - y1 ));
		}
	}
	else
	{
		// y1 is on the right //
		
		x_right = ( ((s64)x1) << 32 );

		/*
		R_right = ( ((s64)r1) << 24 );
		G_right = ( ((s64)g1) << 24 );
		B_right = ( ((s64)b1) << 24 );
		*/
		
		if ( y2 - y1 )
		{
			dx_right = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			
			/*
			dR_right = (((s64)( r2 - r1 )) << 24 ) / ((s64)( y2 - y1 ));
			dG_right = (((s64)( g2 - g1 )) << 24 ) / ((s64)( y2 - y1 ));
			dB_right = (((s64)( b2 - b1 )) << 24 ) / ((s64)( y2 - y1 ));
			*/
		}
	}
	
	
	// *** testing ***
	//debug << "\r\nfixed: dR_across=" << dR_across << " dG_across=" << dG_across << " dB_across=" << dB_across;
	

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
		
		//R_right += dR_right * Temp;
		//G_right += dG_right * Temp;
		//B_right += dB_right * Temp;
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
			}
			
			ptr = & ( VRAM [ StartX + ( Line << 10 ) ] );
			
			if ( DTD )
			{
#ifdef _ENABLE_SSE2_TRIANGLE_GRADIENT
				vDitherValue_add = _mm_loadu_si128 ((__m128i const*) ( &(vDitherArray_add [ ( StartX & 0x3 ) + ( ( Line & 0x3 ) << 4 ) ]) ));
				vDitherValue_sub = _mm_loadu_si128 ((__m128i const*) ( &(vDitherArray_sub [ ( StartX & 0x3 ) + ( ( Line & 0x3 ) << 4 ) ]) ));
#else
				DitherLine = & ( DitherArray [ ( Line & 0x3 ) << 2 ] );
#endif
			}
			
			if ( StartX == EndX && !dx_left ) EndX += 1;
			
#ifdef _ENABLE_SSE2_TRIANGLE_GRADIENT
	vx_across = _mm_add_epi16 ( vSeq, _mm_set1_epi16 ( StartX ) );
	vEndX = _mm_set1_epi16 ( EndX );
	
	viR = _mm_add_epi32 ( vR_Start, _mm_set1_epi32 ( iR ) );
	viG = _mm_add_epi32 ( vG_Start, _mm_set1_epi32 ( iG ) );
	viB = _mm_add_epi32 ( vB_Start, _mm_set1_epi32 ( iB ) );
#endif

			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;
			

			// draw horizontal line
			// x_left and x_right need to be rounded off
			//for ( x_across = StartX; x_across < EndX; x_across++ )
#ifdef _ENABLE_SSE2_TRIANGLE_GRADIENT
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize8 )
			{
				if ( ABE || PIXELMASK )
				{
					vDestPixel = _mm_loadu_si128 ((__m128i const*) ptr);
				}
				
				if ( DTD )
				{
					vRed = _mm_srai_epi32 ( viR, 16 );
					viR = _mm_add_epi32 ( viR, vdR_across4 );
					vRed = _mm_packs_epi32 ( vRed, _mm_srai_epi32 ( viR, 16 ) );
					vRed = _mm_adds_epu16 ( vRed, vDitherValue_add );
					vRed = _mm_subs_epu16 ( vRed, vDitherValue_sub );
					
					vGreen = _mm_srai_epi32 ( viG, 16 );
					viG = _mm_add_epi32 ( viG, vdG_across4 );
					vGreen = _mm_packs_epi32 ( vGreen, _mm_srai_epi32 ( viG, 16 ) );
					vGreen = _mm_adds_epu16 ( vGreen, vDitherValue_add );
					vGreen = _mm_subs_epu16 ( vGreen, vDitherValue_sub );
					
					vBlue = _mm_srai_epi32 ( viB, 16 );
					viB = _mm_add_epi32 ( viB, vdB_across4 );
					vBlue = _mm_packs_epi32 ( vBlue, _mm_srai_epi32 ( viB, 16 ) );
					vBlue = _mm_adds_epu16 ( vBlue, vDitherValue_add );
					vBlue = _mm_subs_epu16 ( vBlue, vDitherValue_sub );
					
					vRed = _mm_srli_epi16 ( vRed, 11 );
					vGreen = _mm_srli_epi16 ( vGreen, 11 );
					vBlue = _mm_srli_epi16 ( vBlue, 11 );
					
				}
				else
				{
					vRed = _mm_srli_epi32 ( viR, 27 );
					viR = _mm_add_epi32 ( viR, vdR_across4 );
					vRed = _mm_packs_epi32 ( vRed, _mm_srli_epi32 ( viR, 27 ) );
					
					vGreen = _mm_srli_epi32 ( viG, 27 );
					viG = _mm_add_epi32 ( viG, vdG_across4 );
					vGreen = _mm_packs_epi32 ( vGreen, _mm_srli_epi32 ( viG, 27 ) );
					
					vBlue = _mm_srli_epi32 ( viB, 27 );
					viB = _mm_add_epi32 ( viB, vdB_across4 );
					vBlue = _mm_packs_epi32 ( vBlue, _mm_srli_epi32 ( viB, 27 ) );
				}
					
				//vRed = _mm_slli_epi16 ( vRed, 0 );
				vGreen = _mm_slli_epi16 ( vGreen, 5 );
				vBlue = _mm_slli_epi16 ( vBlue, 10 );
				
				vpixel = _mm_or_si128 ( _mm_or_si128 ( vRed, vGreen ), vBlue );
				
				if ( ABE )
				{
					vpixel = vSemiTransparency16_t<ABRCODE>( vDestPixel, vpixel, c_vLoBitMask, c_vPixelMask, c_vClampMask, c_vShiftHalf_Mask, c_vShiftQuarter_Mask );
				}
				
				if ( SETPIXELMASK )
				{
					vpixel = _mm_or_si128 ( vpixel, vSetPixelMask );
				}
				
				if ( PIXELMASK )
				{
					vStoreMask = _mm_andnot_si128 ( _mm_and_si128 ( _mm_srai_epi16 ( vDestPixel, 15 ), vPixelMask ), _mm_cmplt_epi16 ( vx_across, vEndX ) );
				}
				else
				{
					vStoreMask = _mm_cmplt_epi16 ( vx_across, vEndX );
				}
				
				_mm_maskmoveu_si128 ( vpixel, vStoreMask, (char*) ptr );
				
				vx_across = _mm_add_epi16 ( vx_across, vVectorSize );
				
				viR = _mm_add_epi32 ( viR, vdR_across4 );
				//viR2 = _mm_add_epi32 ( viR2, vdR_across );
				viG = _mm_add_epi32 ( viG, vdG_across4 );
				//viG2 = _mm_add_epi32 ( viG2, vdG_across );
				viB = _mm_add_epi32 ( viB, vdB_across4 );
				//viB2 = _mm_add_epi32 ( viB2, vdB_across );
				
				ptr += c_iVectorSize8;
			}
#else
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
				if ( ABE || PIXELMASK )
				{
					// read pixel from frame buffer if we need to check mask bit
					DestPixel = *ptr;
				}
					
				if ( DTD )
				{
					DitherValue = DitherLine [ x_across & 0x3 ];
					
					// perform dither
					Red = iR + DitherValue;
					Green = iG + DitherValue;
					Blue = iB + DitherValue;
					
					// perform shift
					Red >>= 27;
					Green >>= 27;
					Blue >>= 27;
					
					// if dithering, perform signed clamp to 5 bits
					Red = AddSignedClamp<s64,5> ( Red );
					Green = AddSignedClamp<s64,5> ( Green );
					Blue = AddSignedClamp<s64,5> ( Blue );
				}
				else
				{
					// perform shift
					Red = ( iR >> 27 );
					Green = ( iG >> 27 );
					Blue = ( iB >> 27 );
				}
					
				// combine
				pixel = ( Blue << 10 ) | ( Green << 5 ) | Red;
					
					
				// *** testing ***
				//debug << "\r\nDestPixel=" << hex << DestPixel;
				
					// *** testing ***
					//debug << " (before)bgr_temp=" << hex << bgr_temp;
					
				// semi-transparency
				if ( ABE )
				{
					pixel = SemiTransparency16_t<ABRCODE> ( DestPixel, pixel );
				}
					
				if ( SETPIXELMASK )
				{
					// check if we should set mask bit when drawing
					pixel |= SETPIXELMASK;
				}

					// *** testing ***
					//debug << " (before)bgr_temp=" << hex << bgr_temp;
					//debug << " SetPixelMask=" << SetPixelMask << " PixelMask=" << PixelMask << " DestPixel=" << DestPixel;
					
				if ( PIXELMASK )
				{
					// draw pixel if we can draw to mask pixels or mask bit not set
					if ( ! ( DestPixel & PIXELMASK ) ) *ptr = pixel;
				}
				else
				{
					*ptr = pixel;
				}

					
				iR += dR_across;
				iG += dG_across;
				iB += dB_across;
				
				//ptr++;
				ptr += c_iVectorSize;
			}
#endif
		}
		
		/////////////////////////////////////
		// update x on left and right
		x_left += dx_left;
		x_right += dx_right;
		
		R_left += dR_left;
		G_left += dG_left;
		B_left += dB_left;
		//R_right += dR_right;
		//G_right += dG_right;
		//B_right += dB_right;
	}
		
}



template<const long PIXELMASK, const long SETPIXELMASK, const long ABE, const long ABRCODE>
inline void GPU::SelectTriangle_Gradient4_t ()
{
	if ( GPU_CTRL_Read.DTD )
	{
		DrawTriangle_Gradient_t <PIXELMASK,SETPIXELMASK,ABE,ABRCODE,1> ();
	}
	else
	{
		DrawTriangle_Gradient_t <PIXELMASK,SETPIXELMASK,ABE,ABRCODE,0> ();
	}
}



template<const long PIXELMASK, const long ABE, const long ABRCODE>
inline void GPU::SelectTriangle_Gradient3_t ()
{
	if ( GPU_CTRL_Read.MD )
	{
		SelectTriangle_Gradient4_t <PIXELMASK,0x8000,ABE,ABRCODE> ();
	}
	else
	{
		SelectTriangle_Gradient4_t <PIXELMASK,0,ABE,ABRCODE> ();
	}
}


template<const long ABE, const long ABRCODE>
inline void GPU::SelectTriangle_Gradient2_t ()
{
	if ( GPU_CTRL_Read.ME )
	{
		SelectTriangle_Gradient3_t <0x8000,ABE,ABRCODE> ();
	}
	else
	{
		SelectTriangle_Gradient3_t <0,ABE,ABRCODE> ();
	}
}


template<const long ABE>
void GPU::SelectTriangle_Gradient_t ()
{
	switch ( GPU_CTRL_Read.ABR )
	{
		case 0:
			SelectTriangle_Gradient2_t <ABE,0> ();
			break;
			
		case 1:
			SelectTriangle_Gradient2_t <ABE,1> ();
			break;
			
		case 2:
			SelectTriangle_Gradient2_t <ABE,2> ();
			break;
			
		case 3:
			SelectTriangle_Gradient2_t <ABE,3> ();
			break;
	}
}


template<const long ABE>
void GPU::Draw_GradientTriangle_30_t ()
{
	if ( ( bgr0 & 0x00ffffff ) == ( bgr1 & 0x00ffffff ) && ( bgr0 & 0x00ffffff ) == ( bgr2 & 0x00ffffff ) )
	{
		GetBGR ( Buffer [ 0 ] );
		SelectTriangle_Mono_t <ABE> ();
	}
	else
	{
		SelectTriangle_Gradient_t <ABE> ();
	}
	
	//SelectTriangle_Gradient_t <ABE> ();
	
	// *** TODO *** calculate cycles to draw here
	// check for alpha blending - add in an extra cycle for this for now
	if ( ABE )
	{
		BusyCycles += NumberOfPixelsDrawn * dAlphaBlending_CyclesPerPixel;
	}
	
	// add in cycles to draw mono-triangle
	BusyCycles += NumberOfPixelsDrawn * dGradientTriangle_30_CyclesPerPixel;
}


template<const long ABE>
void GPU::Draw_GradientRectangle_38_t ()
{
	x_save [ 0 ] = x0; x_save [ 1 ] = x1; x_save [ 2 ] = x2; x_save [ 3 ] = x3;
	y_save [ 0 ] = y0; y_save [ 1 ] = y1; y_save [ 2 ] = y2; y_save [ 3 ] = y3;
	bgr_save [ 0 ] = bgr0; bgr_save [ 1 ] = bgr1; bgr_save [ 2 ] = bgr2; bgr_save [ 3 ] = bgr3;
	
	Draw_GradientTriangle_30_t <ABE> ();
	
	x0 = x_save [ 1 ];
	y0 = y_save [ 1 ];
	x1 = x_save [ 2 ];
	y1 = y_save [ 2 ];
	x2 = x_save [ 3 ];
	y2 = y_save [ 3 ];
	
	bgr0 = bgr_save [ 1 ];
	bgr1 = bgr_save [ 2 ];
	bgr2 = bgr_save [ 3 ];
	
	Draw_GradientTriangle_30_t <ABE> ();

}




template<const long PIXELMASK, const long SETPIXELMASK, const long ABE, const long ABRCODE, const long TGE, const long TP,const long DTD>
void GPU::DrawTriangle_TextureGradient_t ()
{
	// before sse2, was sending 1 pixels at a time
	static const int c_iVectorSize = 1;

	u32 clut_xoffset;
	
	u32 pixel, pixel_temp;

	//u32 Pixel, TexelIndex;
	
	u32 color_add;
	
	u16 *ptr_texture, *ptr_clut;
	u16 *ptr;
	
	s64 Temp, TMin, TMax;
	s64 LeftMostX, RightMostX;
	
	s32 StartX, EndX, StartY, EndY;
	
	s32* DitherArray;
	s32* DitherLine;
	s32 DitherValue;
	

	u32 DestPixel;
	u32 TexCoordX, TexCoordY;
	
	//u32 PixelMask = 0, SetPixelMask = 0;
	//u32 Shift1 = 0, Shift2 = 0, And1 = 0, And2 = 0;

	s64 Error_Left;
	
	
	s16 Red, Green, Blue;
	
	u32 TWYTWH, TWXTWW, Not_TWH, Not_TWW;
	
	TWYTWH = ( ( TWY & TWH ) << 3 );
	TWXTWW = ( ( TWX & TWW ) << 3 );
	
	Not_TWH = ~( TWH << 3 );
	Not_TWW = ~( TWW << 3 );
	
	//if ( GPU_CTRL_Read.ME ) PixelMask = 0x8000;
	//if ( GPU_CTRL_Read.MD ) SetPixelMask = 0x8000;

	
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
	
	//DitherArray = (s32*) c_iDitherZero;
	
	if ( DTD )
	{
		//DitherArray = c_iDitherValues;
		DitherArray = c_iDitherValues4;
	}
	
	//////////////////////////////////////////
	// get coordinates on screen
	x0 = DrawArea_OffsetX + x0;
	y0 = DrawArea_OffsetY + y0;
	x1 = DrawArea_OffsetX + x1;
	y1 = DrawArea_OffsetY + y1;
	x2 = DrawArea_OffsetX + x2;
	y2 = DrawArea_OffsetY + y2;
	
	

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
	

	///////////////////////////////////////////
	// start at y0
	//Line = y0;
	
	/////////////////////////////////////////////
	// init x on the left and right
	/*
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
	*/

	////////////////////////////////////
	// get slopes


	if ( y1 - y0 )
	{
		x_left = ( ((s64)x0) << 32 );
		x_right = x_left;
		U_left = ( ((s64)u0) << 24 );
		V_left = ( ((s64)v0) << 24 );
		U_right = U_left;
		V_right = V_left;
		
		R_left = ( ((s64)r0) << 24 );
		G_left = ( ((s64)g0) << 24 );
		B_left = ( ((s64)b0) << 24 );
		//R_right = R_left;
		//G_right = G_left;
		//B_right = B_left;
		
		if ( denominator < 0 )
		{
			// y1 is on the left //
			dx_left = (((s64)( x1 - x0 )) << 32 ) / ((s64)( y1 - y0 ));
			dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
			
			dU_left = (((s64)( u1 - u0 )) << 24 ) / ((s64)( y1 - y0 ));
			dV_left = (((s64)( v1 - v0 )) << 24 ) / ((s64)( y1 - y0 ));
			dU_right = (((s64)( u2 - u0 )) << 24 ) / ((s64)( y2 - y0 ));
			dV_right = (((s64)( v2 - v0 )) << 24 ) / ((s64)( y2 - y0 ));
			
			dR_left = (((s64)( r1 - r0 )) << 24 ) / ((s64)( y1 - y0 ));
			dG_left = (((s64)( g1 - g0 )) << 24 ) / ((s64)( y1 - y0 ));
			dB_left = (((s64)( b1 - b0 )) << 24 ) / ((s64)( y1 - y0 ));
		}
		else
		{
			// y1 is on the right //
			dx_right = (((s64)( x1 - x0 )) << 32 ) / ((s64)( y1 - y0 ));
			dx_left = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
			
			dU_right = (((s64)( u1 - u0 )) << 24 ) / ((s64)( y1 - y0 ));
			dV_right = (((s64)( v1 - v0 )) << 24 ) / ((s64)( y1 - y0 ));
			dU_left = (((s64)( u2 - u0 )) << 24 ) / ((s64)( y2 - y0 ));
			dV_left = (((s64)( v2 - v0 )) << 24 ) / ((s64)( y2 - y0 ));
			
			dR_left = (((s64)( r2 - r0 )) << 24 ) / ((s64)( y2 - y0 ));
			dG_left = (((s64)( g2 - g0 )) << 24 ) / ((s64)( y2 - y0 ));
			dB_left = (((s64)( b2 - b0 )) << 24 ) / ((s64)( y2 - y0 ));
		}
	}
	else
	{
		// top is flat //
		
		if ( denominator < 0 )
		{
			// y1 is on the left //
			
			// change x_left and x_right where y1 is on left
			//x_left = ( ((s64)x1) << 32 );
			x_right = ( ((s64)x0) << 32 );
			
			//U_left = ( ((s64)u1) << 24 );
			U_right = ( ((s64)u0) << 24 );
			//V_left = ( ((s64)v1) << 24 );
			V_right = ( ((s64)v0) << 24 );
			
			/*
			R_left = ( ((s64)r1) << 24 );
			G_left = ( ((s64)g1) << 24 );
			B_left = ( ((s64)b1) << 24 );
			*/
		}
		else
		{
			// y0 is on the left //
			
			x_right = ( ((s64)x1) << 32 );
			x_left = ( ((s64)x0) << 32 );
			
			U_right = ( ((s64)u1) << 24 );
			U_left = ( ((s64)u0) << 24 );
			V_right = ( ((s64)v1) << 24 );
			V_left = ( ((s64)v0) << 24 );
			
			R_left = ( ((s64)r0) << 24 );
			G_left = ( ((s64)g0) << 24 );
			B_left = ( ((s64)b0) << 24 );
		}
		
		// this means that x0 and x1 are on the same line
		// so if the height of the entire polygon is zero, then we are done
		if ( y2 - y1 )
		{
			if ( denominator < 0 )
			{
				// y1 is on the left //
				//dx_left = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
				dx_right = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
			
				//dU_left = (((s64)( u2 - u1 )) << 24 ) / ((s64)( y2 - y1 ));
				//dV_left = (((s64)( v2 - v1 )) << 24 ) / ((s64)( y2 - y1 ));
				dU_right = (((s64)( u2 - u0 )) << 24 ) / ((s64)( y2 - y0 ));
				dV_right = (((s64)( v2 - v0 )) << 24 ) / ((s64)( y2 - y0 ));
				
				//dR_left = (((s64)( r2 - r1 )) << 24 ) / ((s64)( y2 - y1 ));
				//dG_left = (((s64)( g2 - g1 )) << 24 ) / ((s64)( y2 - y1 ));
				//dB_left = (((s64)( b2 - b1 )) << 24 ) / ((s64)( y2 - y1 ));
				
			}
			else
			{
				// y1 is on the right //
				dx_right = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
				dx_left = (((s64)( x2 - x0 )) << 32 ) / ((s64)( y2 - y0 ));
				
				dU_right = (((s64)( u2 - u1 )) << 24 ) / ((s64)( y2 - y1 ));
				dV_right = (((s64)( v2 - v1 )) << 24 ) / ((s64)( y2 - y1 ));
				dU_left = (((s64)( u2 - u0 )) << 24 ) / ((s64)( y2 - y0 ));
				dV_left = (((s64)( v2 - v0 )) << 24 ) / ((s64)( y2 - y0 ));
				
				dR_left = (((s64)( r2 - r0 )) << 24 ) / ((s64)( y2 - y0 ));
				dG_left = (((s64)( g2 - g0 )) << 24 ) / ((s64)( y2 - y0 ));
				dB_left = (((s64)( b2 - b0 )) << 24 ) / ((s64)( y2 - y0 ));
			}
		}
	}

	/*
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
	*/

	
	
	/////////////////////////////////////////////////
	// swap left and right if they are reversed
	/*
	if ( denominator > 0 )
	{
		// x1,y1 is on the right //
		
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
	*/

	
	
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
		StartX = _Round( x_left ) >> 32;
		EndX = _Round( x_right ) >> 32;
		
		if ( StartX <= ((s32)DrawArea_BottomRightX) && EndX > ((s32)DrawArea_TopLeftX) )
		{
			
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
			
			if ( DTD )
			{
				DitherLine = & ( DitherArray [ ( Line & 0x3 ) << 2 ] );
			}
			
			if ( StartX == EndX && !dx_left ) EndX += 1;
			//if ( EndX >= RightMostX ) EndX--;
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;


			// draw horizontal line
			// x_left and x_right need to be rounded off
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
					if ( DTD )
					{
						DitherValue = DitherLine [ x_across & 0x3 ];
					}

					TexCoordY = (u8) ( ( ( _Round24 ( iV ) >> 24 ) & Not_TWH ) | TWYTWH );
					TexCoordX = (u8) ( ( ( _Round24 ( iU ) >> 24 ) & Not_TWW ) | TWXTWW );
					
					// *** testing ***
					//debug << dec << "\r\nTexCoordX=" << TexCoordX << " TexCoordY=" << TexCoordY;
					
					switch ( TP )
					{
						case 0:
							pixel = ptr_texture [ ( TexCoordX >> 2 ) + ( TexCoordY << 10 ) ];
							pixel = ptr_clut [ ( clut_xoffset + ( ( pixel >> ( ( TexCoordX & 3 ) << 2 ) ) & 0xf ) ) & FrameBuffer_XMask ];
							break;
							
						case 1:
							pixel = ptr_texture [ ( TexCoordX >> 1 ) + ( TexCoordY << 10 ) ];
							pixel = ptr_clut [ ( clut_xoffset + ( ( pixel >> ( ( TexCoordX & 1 ) << 3 ) ) & 0xff ) ) & FrameBuffer_XMask ];
							break;
							
						case 2:
							pixel = ptr_texture [ ( TexCoordX ) + ( TexCoordY << 10 ) ];
							break;
					}
					
					
					if ( pixel )
					{
						if ( ABE || PIXELMASK )
						{
							// read pixel from frame buffer if we need to check mask bit
							DestPixel = *ptr;
						}

						if ( ABE || !TGE )
						{
							pixel_temp = pixel & 0x8000;
						}
				
						//if ( !tge )
						if ( !TGE )
						{
							// brightness calculation
							// shade pixel
							Red = ( ( (s16) ( iR >> 24 ) & 0xff ) * ( (s16) ( pixel & 0x1f ) ) );
							Green = ( ( (s16) ( iG >> 24 ) & 0xff ) * ( (s16) ( ( pixel >> 5 ) & 0x1f ) ) );
							Blue = ( ( (s16) ( iB >> 24 ) & 0xff ) * ( (s16) ( ( pixel >> 10 ) & 0x1f ) ) );
						
							if ( DTD )
							{
								// apply dithering if it is enabled
								// dithering must be applied after the color multiply
								Red = Red + DitherValue;
								Green = Green + DitherValue;
								Blue = Blue + DitherValue;
							}
							
							// clamp
							//Red = Clamp5 ( Red >> 7 );
							//Green = Clamp5 ( Green >> 7 );
							//Blue = Clamp5 ( Blue >> 7 );
							Red = SignedClamp<s16,5> ( Red >> 7 );
							Green = SignedClamp<s16,5> ( Green >> 7 );
							Blue = SignedClamp<s16,5> ( Blue >> 7 );
							
							// combine
							pixel = ( Blue << 10 ) | ( Green << 5 ) | ( Red );
						}
						
						// semi-transparency
						//if ( command_abe && ( bgr & 0x8000 ) )
						if ( ABE )
						{
							if ( pixel_temp )
							{
								pixel = SemiTransparency16_t <ABRCODE> ( DestPixel, pixel );
							}
						}
						
						if ( ABE || !TGE )
						{
							// check if we should set mask bit when drawing
							//bgr_temp |= SetPixelMask | ( bgr & 0x8000 );
							pixel |= pixel_temp;
						}
						
						if ( SETPIXELMASK )
						{
							// set pixel mask if specified
							pixel |= SETPIXELMASK;
						}

						// draw pixel if we can draw to mask pixels or mask bit not set
						if ( PIXELMASK )
						{
							if ( ! ( DestPixel & PIXELMASK ) ) *ptr = pixel;
						}
						else
						{
							*ptr = pixel;
						}
					}
				
				/////////////////////////////////////////////////////////
				// interpolate texture coords
				iU += dU_across;
				iV += dV_across;
				
				iR += dR_across;
				iG += dG_across;
				iB += dB_across;
				
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
		
		R_left += dR_left;
		G_left += dG_left;
		B_left += dB_left;
		//R_right += dR_right;
		//G_right += dG_right;
		//B_right += dB_right;
	}

	
	////////////////////////////////////////////////
	// draw bottom part of triangle

	/////////////////////////////////////////////
	// init x on the left and right
	
	
	
	
	//////////////////////////////////////////////////////
	// check if y1 is on the left or on the right
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

		//R_right = ( ((s64)r1) << 24 );
		//G_right = ( ((s64)g1) << 24 );
		//B_right = ( ((s64)b1) << 24 );
		
		if ( y2 - y1 )
		{
			dx_right = (((s64)( x2 - x1 )) << 32 ) / ((s64)( y2 - y1 ));
			
			dU_right = (((s64)( u2 - u1 )) << 24 ) / ((s64)( y2 - y1 ));
			dV_right = (((s64)( v2 - v1 )) << 24 ) / ((s64)( y2 - y1 ));
			
			//dR_right = (((s64)( r2 - r1 )) << 24 ) / ((s64)( y2 - y1 ));
			//dG_right = (((s64)( g2 - g1 )) << 24 ) / ((s64)( y2 - y1 ));
			//dB_right = (((s64)( b2 - b1 )) << 24 ) / ((s64)( y2 - y1 ));
		}
	}
	

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
		//R_right += dR_right * Temp;
		//G_right += dG_right * Temp;
		//B_right += dB_right * Temp;
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
			
			if ( DTD )
			{
				DitherLine = & ( DitherArray [ ( Line & 0x3 ) << 2 ] );
			}
			
			// ??
			if ( StartX == EndX && !dx_left ) EndX += 1;
			//if ( EndX >= RightMostX ) EndX--;
			
			
			/////////////////////////////////////////////////////
			// update number of cycles used to draw polygon
			NumberOfPixelsDrawn += EndX - StartX;
			
			
			// draw horizontal line
			// x_left and x_right need to be rounded off
			for ( x_across = StartX; x_across < EndX; x_across += c_iVectorSize )
			{
					if ( DTD )
					{
						DitherValue = DitherLine [ x_across & 0x3 ];
					}

					TexCoordY = (u8) ( ( ( _Round24 ( iV ) >> 24 ) & Not_TWH ) | TWYTWH );
					TexCoordX = (u8) ( ( ( _Round24 ( iU ) >> 24 ) & Not_TWW ) | TWXTWW );
					
					// *** testing ***
					//debug << dec << "\r\nTexCoordX=" << TexCoordX << " TexCoordY=" << TexCoordY;
					
					switch ( TP )
					{
						case 0:
							pixel = ptr_texture [ ( TexCoordX >> 2 ) + ( TexCoordY << 10 ) ];
							pixel = ptr_clut [ ( clut_xoffset + ( ( pixel >> ( ( TexCoordX & 3 ) << 2 ) ) & 0xf ) ) & FrameBuffer_XMask ];
							break;
							
						case 1:
							pixel = ptr_texture [ ( TexCoordX >> 1 ) + ( TexCoordY << 10 ) ];
							pixel = ptr_clut [ ( clut_xoffset + ( ( pixel >> ( ( TexCoordX & 1 ) << 3 ) ) & 0xff ) ) & FrameBuffer_XMask ];
							break;
							
						case 2:
							pixel = ptr_texture [ ( TexCoordX ) + ( TexCoordY << 10 ) ];
							break;
					}
					
					
					if ( pixel )
					{
						if ( ABE || PIXELMASK )
						{
							// read pixel from frame buffer if we need to check mask bit
							DestPixel = *ptr;
						}

						if ( ABE || !TGE )
						{
							pixel_temp = pixel & 0x8000;
						}
				
						//if ( !tge )
						if ( !TGE )
						{
							// brightness calculation
							// shade pixel
							Red = ( ( (s16) ( iR >> 24 ) & 0xff ) * ( (s16) ( pixel & 0x1f ) ) );
							Green = ( ( (s16) ( iG >> 24 ) & 0xff ) * ( (s16) ( ( pixel >> 5 ) & 0x1f ) ) );
							Blue = ( ( (s16) ( iB >> 24 ) & 0xff ) * ( (s16) ( ( pixel >> 10 ) & 0x1f ) ) );
						
							if ( DTD )
							{
								// apply dithering if it is enabled
								// dithering must be applied after the color multiply
								Red = Red + DitherValue;
								Green = Green + DitherValue;
								Blue = Blue + DitherValue;
							}
							
							// clamp
							//Red = Clamp5 ( Red >> 7 );
							//Green = Clamp5 ( Green >> 7 );
							//Blue = Clamp5 ( Blue >> 7 );
							Red = SignedClamp<s16,5> ( Red >> 7 );
							Green = SignedClamp<s16,5> ( Green >> 7 );
							Blue = SignedClamp<s16,5> ( Blue >> 7 );
							
							// combine
							pixel = ( Blue << 10 ) | ( Green << 5 ) | ( Red );
						}
						
						// semi-transparency
						//if ( command_abe && ( bgr & 0x8000 ) )
						if ( ABE )
						{
							if ( pixel_temp )
							{
								pixel = SemiTransparency16_t <ABRCODE> ( DestPixel, pixel );
							}
						}
						
						if ( ABE || !TGE )
						{
							// check if we should set mask bit when drawing
							//bgr_temp |= SetPixelMask | ( bgr & 0x8000 );
							pixel |= pixel_temp;
						}
						
						if ( SETPIXELMASK )
						{
							// set pixel mask if specified
							pixel |= SETPIXELMASK;
						}

						// draw pixel if we can draw to mask pixels or mask bit not set
						if ( PIXELMASK )
						{
							if ( ! ( DestPixel & PIXELMASK ) ) *ptr = pixel;
						}
						else
						{
							*ptr = pixel;
						}
					}
				
				/////////////////////////////////////////////////////////
				// interpolate texture coords
				iU += dU_across;
				iV += dV_across;
				
				iR += dR_across;
				iG += dG_across;
				iB += dB_across;
				
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
		
		R_left += dR_left;
		G_left += dG_left;
		B_left += dB_left;
		R_right += dR_right;
		G_right += dG_right;
		B_right += dB_right;
	}

}




template<const long PIXELMASK, const long SETPIXELMASK, const long ABE, const long ABRCODE, const long TGE, const long TP>
inline void GPU::SelectTriangle_TextureGradient5_t ()
{
	//debug << "SelectTriangle_Texture5_t->";
	
	if ( GPU_CTRL_Read.DTD )
	{
		DrawTriangle_TextureGradient_t <PIXELMASK, SETPIXELMASK, ABE, ABRCODE, TGE, TP,1> ();
	}
	else
	{
		DrawTriangle_TextureGradient_t <PIXELMASK, SETPIXELMASK, ABE, ABRCODE, TGE, TP,0> ();
	}
}



template<const long PIXELMASK, const long ABE, const long ABRCODE, const long TGE, const long TP>
inline void GPU::SelectTriangle_TextureGradient4_t ()
{
	//debug << "SelectTriangle_Texture4_t->";
	
	if ( GPU_CTRL_Read.MD )
	{
		SelectTriangle_TextureGradient5_t <PIXELMASK, 0x8000, ABE, ABRCODE, TGE, TP> ();
	}
	else
	{
		SelectTriangle_TextureGradient5_t <PIXELMASK, 0, ABE, ABRCODE, TGE, TP> ();
	}
}


template<const long ABE, const long ABRCODE, const long TGE, const long TP>
inline void GPU::SelectTriangle_TextureGradient3_t ()
{
	//debug << "SelectTriangle_Texture3_t->";
	
	if ( GPU_CTRL_Read.ME )
	{
		SelectTriangle_TextureGradient4_t<0x8000, ABE, ABRCODE, TGE, TP> ();
	}
	else
	{
		SelectTriangle_TextureGradient4_t<0, ABE, ABRCODE, TGE, TP> ();
	}
}


template<const long ABE, const long ABRCODE, const long TGE>
inline void GPU::SelectTriangle_TextureGradient2_t ()
{
	//debug << "SelectTriangle_Texture2_t->";
	
	switch ( tpage_tp )
	{
		case 0:
			SelectTriangle_TextureGradient3_t<ABE, ABRCODE, TGE, 0> ();
			break;
			
		case 1:
			SelectTriangle_TextureGradient3_t<ABE, ABRCODE, TGE, 1> ();
			break;
			
		case 2:
			SelectTriangle_TextureGradient3_t<ABE, ABRCODE, TGE, 2> ();
			break;
	}
}


template<const long ABE, const long TGE>
void GPU::SelectTriangle_TextureGradient_t ()
{
	//debug << "SelectTriangle_Texture_t->";
	
	switch ( tpage_abr )
	{
		case 0:
			SelectTriangle_TextureGradient2_t<ABE, 0, TGE> ();
			break;
			
		case 1:
			SelectTriangle_TextureGradient2_t<ABE, 1, TGE> ();
			break;
			
		case 2:
			SelectTriangle_TextureGradient2_t<ABE, 2, TGE> ();
			break;
			
		case 3:
			SelectTriangle_TextureGradient2_t<ABE, 3, TGE> ();
			break;
	}
}



template<const long ABE, const long TGE>
void GPU::Draw_TextureGradientTriangle_34_t ()
{
	//u32 tge;
	//tge = command_tge;
	
	if ( ( bgr0 & 0x00ffffff ) == ( bgr1 & 0x00ffffff ) && ( bgr0 & 0x00ffffff ) == ( bgr2 & 0x00ffffff ) )
	{
		if ( ( bgr0 & 0x00ffffff ) == 0x00808080 )
		{
			//command_tge = 1;
			SelectTriangle_Texture_t <ABE,1> ();
		}
		else
		{
			bgr = bgr0;
			SelectTriangle_Texture_t <ABE,TGE> ();
		}
		
		//DrawTriangle_Texture ();
	}
	else
	{
		if ( TGE )
		{
			SelectTriangle_Texture_t <ABE,TGE> ();
		}
		else
		{
			SelectTriangle_TextureGradient_t <ABE,TGE> ();
		}
	}
	
	// restore tge
	//command_tge = tge;

	
	// check for alpha blending - add in an extra cycle for this for now
	if ( ABE )
	{
		BusyCycles += NumberOfPixelsDrawn * dAlphaBlending_CyclesPerPixel;
	}
	
	// check for brightness calculation
	if ( !TGE )
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


template<const long ABE, const long TGE>
void GPU::Draw_TextureGradientRectangle_3c_t ()
{
	x_save [ 0 ] = x0; x_save [ 1 ] = x1; x_save [ 2 ] = x2; x_save [ 3 ] = x3;
	y_save [ 0 ] = y0; y_save [ 1 ] = y1; y_save [ 2 ] = y2; y_save [ 3 ] = y3;
	bgr_save [ 0 ] = bgr; bgr_save [ 1 ] = bgr0; bgr_save [ 2 ] = bgr1; bgr_save [ 3 ] = bgr2; bgr_save [ 4 ] = bgr3;
	u_save [ 0 ] = u0; u_save [ 1 ] = u1; u_save [ 2 ] = u2; u_save [ 3 ] = u3;
	v_save [ 0 ] = v0; v_save [ 1 ] = v1; v_save [ 2 ] = v2; v_save [ 3 ] = v3;
	
	bgr = 0x808080;
	
	//Draw_TextureGradientTriangle_34 ();
	Draw_TextureGradientTriangle_34_t <ABE,TGE> ();
	
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
	
	//Draw_TextureGradientTriangle_34 ();
	Draw_TextureGradientTriangle_34_t <ABE,TGE> ();
}




	};

	
	



// *** Template Functions *** //

//using namespace Playstation1;



template<const long ABE, const long TGE>
void GPU::SelectSprite_t ()
{
	switch ( GPU_CTRL_Read.ABR )
	{
		case 0:
			SelectSprite2_t<ABE, 0, TGE> ();
			break;
			
		case 1:
			SelectSprite2_t<ABE, 1, TGE> ();
			break;
			
		case 2:
			SelectSprite2_t<ABE, 2, TGE> ();
			break;
			
		case 3:
			SelectSprite2_t<ABE, 3, TGE> ();
			break;
	}
}


template<const long ABE, const long TGE>
void GPU::Draw_Sprite_64_t ()
{
	tpage_tx = GPU_CTRL_Read.TX;
	tpage_ty = GPU_CTRL_Read.TY;
	tpage_tp = GPU_CTRL_Read.TP;
	
	SelectSprite_t <ABE,TGE> ();

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



template<const long ABE, const long TGE>
void GPU::Draw_Sprite8x8_74_t ()
{
	static const u32 SpriteSize = 8;
	
	tpage_tx = GPU_CTRL_Read.TX;
	tpage_ty = GPU_CTRL_Read.TY;
	tpage_tp = GPU_CTRL_Read.TP;
	//tpage_abr = GPU_CTRL_Read.ABR;
	
	w = 8; h = 8;
	SelectSprite_t <ABE,TGE> ();

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



template<const long ABE, const long TGE>
void GPU::Draw_Sprite16x16_7c_t ()
{
	static const u32 SpriteSize = 16;
	
	tpage_tx = GPU_CTRL_Read.TX;
	tpage_ty = GPU_CTRL_Read.TY;
	tpage_tp = GPU_CTRL_Read.TP;
	//tpage_abr = GPU_CTRL_Read.ABR;
	
	w = 16; h = 16;
	SelectSprite_t <ABE,TGE> ();

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


};



#endif

