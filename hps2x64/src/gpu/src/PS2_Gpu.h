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


#ifndef _PS2_GPU_H_
#define _PS2_GPU_H_

#include "types.h"
#include "Debug.h"

#include "WinApiHandler.h"
//#include "GNUAsmUtility_x64.h"


//#define DRAW_MULTIPLE_PIXELS

//#include "emmintrin.h"

//#define _ENABLE_SSE2_TRIANGLE_MONO
//#define _ENABLE_SSE2_TRIANGLE_GRADIENT
//#define _ENABLE_SSE2_RECTANGLE_MONO


#ifdef _ENABLE_SSE2

// need to include this file to use SSE2 intrinsics
//#include "emmintrin.h"
//#include "smmintrin.h"

#endif


//using namespace x64Asm::Utilities;


namespace Playstation2
{

	class GPU
	{
	
		static Debug::Log debug;
		
		static WindowClass::Window *DisplayOutput_Window;
		static WindowClass::Window *FrameBuffer_DebugWindow;
	
	public:
	
		static GPU *_GPU;
		
		
		//////////////////////////
		//	General Parameters	//
		//////////////////////////
		
		// where the registers start at
		static const long Regs_Start = 0x10003000;
		
		// where the registers end at
		static const long Regs_End = 0x100037f0;
	
		// distance between groups of registers
		static const long Reg_Size = 0x10;
		
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

		// the revision and id of the GPU
		static const u32 c_ulGS_Revision = 0x1b;
		static const u32 c_ulGS_ID = 0x55;
		
		static const char* GIFRegNames [ 11 ];
		static const char* GPUReg0Names [ 15 ];
		static const char* GPUReg1Names [ 9 ];
		static const char* GPURegsGp_Names [ 0x63 ];
		
		static const char* PixelFormat_Names [ 64 ];
		static const char* TransferDir_Names [ 4 ];
		
		// GPU Clock Speed in Hertz
		//static const long c_iClockSpeed = 53222400;
		//static const double c_dClockSpeed = 53222400.0L;
		static const unsigned long long c_llClockSpeed1 = 147456000;
		static const unsigned long long c_llClockSpeed2 = 149500000;

		static const double c_dClockSpeed1 = 147456000.0L;
		static const double c_dClockSpeed2 = 149500000.0L;
		
		
		// the number of gpu cycles for every cpu cycle or vice versa
		static const double c_dGPUPerCPU_Cycles = ( 1.0L / 2.0L );
		static const double c_dCPUPerGPU_Cycles = ( 2.0L / 1.0L );
		
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
		
		// the last path/current path data is being transferred through
		u32 CurrentPath;
		
		// the amount of data in the fifo
		u32 FifoSize;

		static const int c_iFrameBuffer_DisplayWidth = 640;
		static const int c_iFrameBuffer_DisplayHeight = 960;
		
		// used to buffer pixels before drawing to the screen
		u32 PixelBuffer [ c_iFrameBuffer_DisplayWidth * c_iFrameBuffer_DisplayHeight ] __attribute__ ((aligned (32)));
		
		//static const int c_iRamSize = 4194304;
		//u64 RAM [ c_iRamSize >> 3 ] __attribute__ ((aligned (32)));

		
		static const unsigned long c_iRAM_Size = 4194304;
		
		// PS2 VRAM is 4MB
		// ***todo*** show the VRAM when debugging as 1024x1024 if showing a graphical representation, with 32-bit pixels (rgba)
		// ***todo*** also show the frame buffers in 1024x1024 windows, but showing the frame buffers with correct size
		union
		{
			u8 RAM8 [ c_iRAM_Size ] __attribute__ ((aligned (32)));
			u16 RAM16 [ c_iRAM_Size >> 1 ] __attribute__ ((aligned (32)));
			u32 RAM32 [ c_iRAM_Size >> 2 ] __attribute__ ((aligned (32)));
			u64 RAM64 [ c_iRAM_Size >> 3 ] __attribute__ ((aligned (32)));
		};
		
		
		// size of the main program window
		static u32 MainProgramWindow_Width;
		static u32 MainProgramWindow_Height;
		
		// maximum width/height of a polygon allowed
		//static const s32 c_MaxPolygonWidth = 1023;
		//static const s32 c_MaxPolygonHeight = 511;

		static u64 Read ( u32 Address, u64 Mask );
		static void Write ( u32 Address, u64 Data, u64 Mask );
		
		void DMA_Read ( u32* Data, int ByteReadCount );
		void DMA_Write ( u32* Data, int ByteWriteCount );
		
		// these return the amound of data written/read
		static u32 DMA_WriteBlock ( u64* Data, u32 QuadwordCount );
		static u32 DMA_ReadBlock ( u64* Data, u32 QuadwordCount );
		
		// xgkick
		static void Path1_WriteBlock ( u64* Data );
		
		// vif1
		static void Path2_WriteBlock ( u64* Data, u32 QuadwordCount );
		static void Path2_ReadBlock ( u64* Data, u32 QuadwordCount );
		
		void Start ();

		// returns either vblank interrupt signal, gpu interrupt signal, or no interrupt signal
		void Run ();
		void Reset ();
		
		// need to specify what window to display graphical output to (this should be the main program window)
		// *** TODO *** probably also need to call this whenever the main program window size changes
		void SetDisplayOutputWindow ( u32 width, u32 height, WindowClass::Window* DisplayOutput );
		
		void Draw_Screen ();
		
		void Copy_Buffer ( u32* dstbuf, u32* srcbuf, u32 dstbuffer_width, u32 dstbuffer_height, u32 srcbuffer_width, u32 srcbuffer_height );
		void Draw_FrameBuffers ();
		

		void GIF_FIFO_Execute ( u64 ull0, u64 ull1 );
		
		void TransferDataIn32 ( u32* Data, u32 WordCount32 );
		void TransferDataOut ();
		
		// path1 needs to know when to stop feeding data
		u32 EndOfPacket, Tag_Done;
		
		
		// Priveleged Regsiter structs //
		
		union DISPFB_t
		{
			struct
			{
				// Frame Base Address (Base Address/2048)
				// bits 0-8
				u64 FBP : 9;
				
				// Frame Buffer Width (Width in pixels/64)
				u64 FBW : 6;
				
				// Pixel Format
				// 00000: PSMCT32, 00001: PSMCT24, 00010: PSMCT16, 01010: PSMCT16S, 10010: PS-GPU24
				u64 PSM : 5;
				
				// unknown
				// bits 20-31
				u64 unk0 : 12;
				
				// X pixel position in Buffer
				// bits 32-42
				u64 DBX : 11;
				
				// Y pixel position in Buffer
				// bits 43-53
				u64 DBY : 11;
			};
			
			u64 Value;
		};
		
		
		// this must have to do with how it is displayed on the TV/Monitor
		union DISPLAY_t
		{
			struct
			{
				// X VCK/CLK position in display
				// bits 0-11
				u64 DX : 12;
				
				// Y position in display
				// bits 12-22
				u64 DY : 11;
				
				// Horizontal magnification
				// 0000: means times 1, 0001: means times 2, etc
				// bits 23-26
				u64 MAGH : 4;
				
				// Vertical magnification
				// 0000: means times 1, 0001: means times 2, etc
				// bits 27-28
				u64 MAGV : 2;
				
				// unknown
				// bits 29-31
				u64 unk0 : 3;
				
				// width of display minus one in VCK/CLK
				// bits 32-43
				u64 DW : 12;
				
				// height of display minus one in pixels
				u64 DH : 11;
			};
			
			u64 Value;
		};
		
		
		// Interrupt mask
		union IMR_t
		{
			struct
			{
				// unknown
				// bits 0-7
				u64 unk0 : 8;
				
				// SIGNAL interrupt mask
				// 0: enable, 1: disable; initial value=1
				// bit 8
				u64 SIGMSK : 1;
				
				// FINISH interrupt mask
				// bit 9
				u64 FINISHMSK : 1;
				
				// HSYNC interrupt mask
				// bit 10
				u64 HSMSK : 1;
				
				// VSYNC interrupt mask
				// bit 11
				u64 VSMSK : 1;
				
				// Rectangular Area Write interrupt mask
				// bit 12
				u64 EDWMSK : 1;
			};
			
			u64 Value;
		};
		
		
		union SMODE2_t
		{
			struct
			{
				// Interlace Mode
				// 0: disable, 1: enable
				// bit 0
				u64 INTER : 1;
				
				// field/frame mode
				// 0: Read every OTHER line, 1: Read EVERY line
				u64 FFMD : 1;
				
				// DPMS??
				// 0: on, 1: ready, 2: hold, 3: off
				u64 DPMS : 2;
			};
			
			u64 Value;
		};
		
		
		// GIF TAG
		
		// lower part
		union GIFTag0_t
		{
			struct
			{
				// bits 0-14 - NLOOP - Repeat Count
				u64 NLOOP : 15;
				
				// bit 15 - EOP - End of packet - 1: End of packet
				u64 EOP : 1;
				
				// bits 16-45 - not used
				u64 NotUsed0 : 30;
				
				// bit 46 - PRE - PRIM Field enable - 1: Output PRIM field to PRIM register
				u64 PRE : 1;
				
				// bits 47-57 PRIM
				u64 PRIM : 11;
				
				// bits 58-59 - FLG - 00: Packed, 01: Reglist, 10: Image, 11: Disabled/Image
				u64 FLG : 2;
				
				// bits 60-63 - NREG - Register descriptor - (where 0 means 16)
				u64 REGS : 4;
			};
			
			struct
			{
				u32 Lo;
				u32 Hi;
			};
			
			u64 Value;
		};
		
		// upper part
		union GIFTag1_t
		{
			u8 Regs [ 8 ];
			
			struct
			{
				u32 Lo;
				u32 Hi;
			};
			
			u64 Value;
		};
		
		
		static const int c_iNumPaths = 3;
		
		bool bFIFOTransfer_InProgress;
		GIFTag0_t GIFTag0 [ c_iNumPaths ];
		GIFTag1_t GIFTag1 [ c_iNumPaths ];
		
		// TransferCount is the current count of the transfer
		// TransferSize is the number of quadwords to be transferred
		u32 ulLoopCount [ c_iNumPaths ], ulRegCount [ c_iNumPaths ], ulNumRegs [ c_iNumPaths ], ulTransferCount [ c_iNumPaths ], ulTransferSize [ c_iNumPaths ];
		
		u32 TransferType;
		enum { TRANSFERTYPE_PACKED = 0, TRANSFERTYPE_REGLIST, TRANSFERTYPE_IMAGE, TRANSFERTYPE_DISABLED };

		// PS2 GIF Registers
		
		// 0x1000 3000 - GIF CTRL
		static const u32 GIF_CTRL = 0x10003000;
		// bit 0 - RST - write 1 for reset
		// bit 3 - PSE - 0: transfer restart, 1: transfer start
		
		union GIF_CTRL_t
		{
			struct
			{
				// bit 0 - RST - write 1 for reset
				u32 RST : 1;
				
				// bit 1-2
				u32 zero0 : 2;
				
				// bit 3 - PSE - 0: transfer restart, 1: transfer start
				u32 PSE : 1;
				
			};
			
			u32 Value;
		};
		
		// 0x1000 3010 - GIF MODE
		static const u32 GIF_MODE = 0x10003010;
		// bit 0 - M3R - Path3 Mask - 0: Mask cancel, 1: Mask
		// bit 2 - IMT - Path3 transfer mode - 0: continuous, 1: in every 8 qwords
		
		union GIF_MODE_t
		{
			struct
			{
				// bit 0 - M3R - Path3 Mask - 0: Mask cancel, 1: Mask
				u32 M3R : 1;
				
				// bit 1
				u32 zero0 : 1;
				
				// bit 2 - IMT - Path3 transfer mode - 0: continuous, 1: in every 8 qwords
				u32 IMT : 1;
			};
			
			u32 Value;
		};
		
		
		// 0x1000 3020 - GIF STAT - GIF Status
		static const u32 GIF_STAT = 0x10003020;
		
		union GIF_STAT_Format
		{
			struct
			{
				// bit 0 - M3R Status - 0: enable, 1: disable
				u32 M3R : 1;
				
				// bit 1 - M3P Status - 0: enable, 1: disable
				u32 M3P : 1;
				
				// bit 2 - IMT Status - 0: continuous, 1: every 8 qwords
				u32 IMT : 1;
				
				// bit 3 - PSE Status - 0: normal, 1: stopped
				u32 PSE : 1;
				
				// bit 4 - zero?
				u32 zero0 : 1;
				
				// bit 5 - IP3 - 0: not interrupted, 1: interrupted path3 transfer
				u32 IP3 : 1;
				
				// bit 6 - P3Q - 0: no request, 1: request to wait for processing in path3
				u32 P3Q : 1;
				
				// bit 7 - P2Q - 0: no request, 1: request to wait for processing in path2
				u32 P2Q : 1;
				
				// bit 8 - P1Q - 0: no request, 1: request to wait for processing in path1
				u32 P1Q : 1;
				
				// bit 9 - OPH - Output path - 0: Idle, 1: Outputting data
				u32 OPH : 1;
				
				// bit 10-11 - APATH - 00: Idle, 01: transferring data via path1, 10: transferring data via path2, 11: transferring data via path3
				u32 APATH : 2;
				
				// bit 12 - DIR - Transfer direction - 0: to GPU, 1: from GPU
				u32 DIR : 1;
				
				// bits 13-23 - zero?
				u32 zero1 : 11;
				
				// bit 24-28 - FQC - Count of items in GIF FIFO (0-16 in qwords)
				u32 FQC : 5;
				
			};
			
			u32 Value;
		};
		
		//GIF_STAT_Format GIF_STAT_Reg;
		
		
		// 0x1000 3040 - GIF TAG0
		// 0x1000 3050 - GIF TAG1
		// 0x1000 3060 - GIF TAG2
		// 0x1000 3070 - GIF TAG3
		// 0x1000 3080 - GIF CNT
		// 0x1000 3090 - GIF P3CNT
		// 0x1000 30a0 - GIF P3TAG


		// 0x100030X0
		union GIFRegs_t
		{
			u32 Regs [ 0x10 ];
			
			struct
			{
				GIF_CTRL_t CTRL;
				GIF_MODE_t MODE;
				GIF_STAT_Format STAT;
				u32 Reserved0;
				u32 TAG0;
				u32 TAG1;
				u32 TAG2;
				u32 TAG3;
				u32 CNT;
				u32 P3CNT;
				u32 P3TAG;
			};
		};
		
		GIFRegs_t GIFRegs;

		
		
		// 0x1000 6000 - GIF FIFO
		static const u32 GIF_FIFO = 0x10006000;
		
		
		
		// 0x120000X0
		//u64 GPURegs0 [ 0x10 ];
		
		// 0x120010X0
		//u64 GPURegs1 [ 0x10 ];
		
		// PS2 GPU Registers
		
		// 0x1200 0000 - PMODE
		static const u32 GPU_PMODE = 0x12000000;
		// 0x1200 0010 - SMODE1 - Sync
		static const u32 GPU_SMODE1 = 0x12000010;
		// 0x1200 0020 - SMODE2 - Sync
		static const u32 GPU_SMODE2 = 0x12000020;
		// 0x1200 0030 - SRFSH - DRAM refresh
		static const u32 GPU_SRFSH = 0x12000030;
		// 0x1200 0040 - SYNCH1 - Sync
		static const u32 GPU_SYNCH1 = 0x12000040;
		// 0x1200 0050 - SYNCH2 - Sync
		static const u32 GPU_SYNCH2 = 0x12000050;
		// 0x1200 0060 - SYNCV - Sync/Start
		static const u32 GPU_SYNCV = 0x12000060;
		
		// 0x120000X0
		union GPURegs0_t
		{
			u64 Regs [ 0x10 ];
			
			struct
			{
				u64 PMODE;
				u64 SMODE1;
				SMODE2_t SMODE2;
				u64 SRFSH;
				u64 SYNCH1;
				u64 SYNCH2;
				u64 SYNCV;
				DISPFB_t DISPFB1;
				DISPLAY_t DISPLAY1;
				DISPFB_t DISPFB2;
				DISPLAY_t DISPLAY2;
				u64 EXTBUF;
				u64 EXTDATA;
				u64 EXTWRITE;
				u64 BGCOLOR;
			};
		};
		
		// 0x120000X0
		GPURegs0_t GPURegs0;
		
		
		// 0x1200 1000 - CSR - GS status
		static const u32 GPU_CSR = 0x12001000;
		// when writing
		
		union CSR_t
		{
			struct
			{
				// bit 0 - SIGNAL - Signal event control - (write) 1: old event is cleared and event is enabled
				// (read) 0: signal event has not been generated, 1: signal event has been generated
				u64 SIGNAL : 1;
				
				// bit 1 - FINISH - Finsih event control - (write) 1: old event is cleared and event is enabled
				// (read) 0: event has not been generated, 1: event has been generated
				u64 FINISH : 1;
				
				// bit 2 - HSINT - Hsync interrupt control - (write) 1: old event is cleared and event is enabled
				// (read) 0: event has not been generated, 1: event has been generated
				u64 HSINT : 1;
				
				// bit 3 - VSINT - Vsync interrupt control - (write) 1: old event is cleared and event is enabled
				// (read) 0: event has not been generated, 1: event has been generated
				u64 VSINT : 1;
				
				// bit 4 - EDWINT - Rectangular area write termination interrupt control - (write) 1: old event is cleared and event is enabled
				// (read) 0: event has not been generated, 1: event has been generated
				u64 EDWINT : 1;
				
				// bits 5-7
				u64 res0 : 3;
				
				// bit 8 - FLUSH - Drawing suspend and FIFO clear
				// (read) 0: not flushed, 1: flushed
				u64 FLUSH : 1;
				
				// bit 9 - RESET - GS Reset - 0: not reset, 1: reset
				u64 RESET : 1;
				
				// bits 10-11
				u64 res1 : 2;
				
				// bit 12 - NFIELD - Output value of NFIELD
				u64 NFIELD : 1;
				
				// bit 13 - FIELD - The field that is being currently displayed
				// (interlace mode) 0: even, 1: odd
				// (non-interlace mode) 0: even, 1: odd
				u64 FIELD : 1;
				
				// bits 14-15 - FIFO - Host FIFO Status - 0: not empty not almost full, 1: empty, 2: almost full
				u64 FIFO : 2;
				
				// bits 16-23 - GS Revision Number
				u64 REV : 8;
				
				// bits 24-31 - GS ID
				u64 ID : 8;
			};
			
			u64 Value;
		};
		
		
		// 0x120010X0
		static const int c_iGPURegs1_Count = 0x10;
		union GPURegs1_t
		{
			u64 Regs [ c_iGPURegs1_Count ];
			
			struct
			{
				CSR_t CSR;
				IMR_t IMR;
				u64 Reserved0;
				u64 Reserved1;
				u64 BUSDIR;
				u64 Reserved2;
				u64 Reserved3;
				u64 Reserved4;
				u64 SIGLBLID;
			};
		};
		
		// 0x120010X0
		GPURegs1_t GPURegs1;
		
		
		// priveleged GPU registers
		static const int c_iGPURegsPr_Count = 20;
		union GPURegsPr_t
		{
			u64 Regs [ c_iGPURegsPr_Count ];
		
			struct
			{
				u64 PMODE;
				u64 SMODE1;
				u64 SMODE2;
				u64 SRFSH;
				u64 SYNCH1;
				u64 SYNCH2;
				u64 SYNCV;
				u64 DISPLAYFB1;
				u64 DISPLAY1;
				u64 DISPLAYFB2;
				u64 DISPLAY2;
				u64 EXTBUF;
				u64 EXTDATA;
				u64 EXTWRITE;
				u64 BGCOLOR;
				
				u64 CSR;
				u64 IMR;
				u64 BUSDIR;
				u64 SIGLBLID;
				
				u64 Reserved;
			};
		};
		
		GPURegsPr_t GPURegsPr;
		
		
		
		
		
		
		
		union FRAME_t
		{
			struct
			{
				// frame buffer pointer (word address/2048) (word size is 32-bits)
				u64 FBP : 9;
				
				// unknown
				u64 unk0 : 7;
				
				// frame buffer width (number of pixels/64) (values go from 1 to 32 ONLY)
				u64 FBW : 6;
				
				// unknown
				u64 unk1 : 2;
				
				// buffer pixel format
				// 000000: PSMCT32, 000001: PSMCT24, 000010: PSMCT16, 001010: PSMCT16S
				// 110000: PSMZ32, 110001: PSMZ24, 110010: PSMZ16, 111010: PSMZ16S
				u64 SPSM : 6;
				
				// unknown
				u64 unk2 : 2;
				
				// frame buffer drawing mask
				// 0: bit is updated in buffer, 1: bit is not updated in buffer
				u64 FBMSK : 32;
			};
			
			u64 Value;
		};



		union BITBLTBUF_t
		{
			struct
			{
				// source buffer ptr (word address/64) (where words are 32-bit words)
				u64 SBP : 14;
				
				// unknown
				u64 unk0 : 2;
				
				// source buffer width (number of pixels/64)
				u64 SBW : 6;
				
				// unknown
				u64 unk1 : 2;
				
				// source pixel format
				// 000000: PSMCT32, 000001: PSMCT24, 000010: PSMCT16, 001010: PSMCT16S, 010011: PSMT8, 010100: PSMT4, 011011: PSMT8H,
				// 100100: PSMT4HL, 101100: PSMT4HH, 110000: PSMZ32, 110001: PSMZ24, 110010: PSMZ16, 111010: PSMZ16S
				u64 SPSM : 6;
				
				// unknown
				u64 unk2 : 2;
				
				// destination buffer pointer (word address/64)
				u64 DBP : 14;
				
				// unknown
				u64 unk3 : 2;
				
				// destination buffer width (number of pixels/64)
				u64 DBW : 6;
				
				// unknown
				u64 unk4 : 2;
				
				// destination pixel format
				u64 DPSM : 6;
				
				// unknown
				u64 unk5 : 2;
			};
			
			u64 Value;
		};
		
		
		union TRXDIR_t
		{
			struct
			{
				// 00: Host->GPU, 01: GPU->Host, 10: GPU->GPU, 11: none
				u64 XDIR : 2;
			};

			u64 Value;
		};
		
		
		union TRXPOS_t
		{
			struct
			{
				// x-coord to start transfer at for source (upper-left corner)
				u64 SSAX : 11;
				
				// unknown
				u64 unk0 : 5;
				
				// y-coord to start transfer at for source (upper-left corner)
				u64 SSAY : 11;
				
				// unknown
				u64 unk1 : 5;
				
				// x-coord to start transfer at for dest (upper-left corner)
				u64 DSAX : 11;
				
				// unknown
				u64 unk2 : 5;
				
				// y-coord to start transfer at for dest (upper-left corner)
				u64 DSAY : 11;
				
				// transmission method
				// 00: upper-left->lower-right, 01: lower-left->upper-right, 10: upper-right->lower-left, 11: lower-right->upper-left
				u64 DIR : 2;
				
				// unknown
				u64 unk3 : 3;
			};
			
			u64 Value;
		};
		
		
		union TRXREG_t
		{
			struct
			{
				// width of data to transfer in pixels
				u64 RRW : 12;
				
				// unknown
				u64 unk0 : 20;
				
				// height of data to transfer in pixels
				u64 RRH : 12;
				
				// unknown
				u64 unk1 : 20;
			};
			
			u64 Value;
		};
		
		
		union ZBUF_t
		{
			struct
			{
				// z-buffer pointer (word address/2048)
				u64 ZBP : 9;
				
				// unknown
				u64 unk0 : 15;
				
				// z-value format
				// 0000: PSMZ32, 0001: PSMZ24, 0010: PSMZ16, 1010: PSMZ16S
				u64 PSM : 4;
				
				// unknown
				u64 unk1 : 4;
				
				// z-value drawing mask
				// 0: z-buffer is updated, 1: z-buffer is not updated, even if depth test is passed
				u64 ZMSK : 1;
			};
			
			u64 Value;
		};
		
		
		union XYOFFSET_t
		{
			struct
			{
				// x/y offsets are unsigned
				// x-offset (12.4 fixed point)
				u64 OFX : 16;
				
				// unknown
				u64 unk0 : 16;
				
				// y-offset (12.4 fixed point)
				u64 OFY : 16;
				
				// unknown
				u64 unk1 : 16;
			};
			
			u64 Value;
		};
		
		// these look to be positive-only values
		union SCISSOR_t
		{
			struct
			{
				// x-coord for upper-left corner of drawing window
				u64 SCAX0 : 11;
				
				// unknown
				u64 unk0 : 5;
				
				// x-coord for lower-right corner of drawing window
				u64 SCAX1 : 11;
				
				// unknown
				u64 unk1 : 5;
				
				// y-coord for upper-left corner of drawing window
				u64 SCAY0 : 11;
				
				// unknown
				u64 unk2 : 5;
				
				// y-coord for lower-right corner of drawing window
				u64 SCAY1 : 11;
				
				// unknown
				u64 unk3 : 5;
			};
			
			u64 Value;
		};
		
		
		union PRIM_t
		{
			struct
			{
				// type of primitive
				// 000: pixel, 001: line, 010: line strip, 011: triangle, 100: triangle strip, 101: triangle fan, 110: sprite, 111: reserved
				u64 PRIM : 3;
				
				// shading
				// 0: flat, 1: gouraud
				u64 IIP : 1;
				
				// texture mapping
				// 0: off, 1: on
				u64 TME : 1;
				
				// fogging
				// 0: off, 1: on
				u64 FGE : 1;
				
				// alpha blending
				// 0: off, 1: on
				u64 ABE : 1;
				
				// single-pass anti-aliasing
				// 0: off, 1: on
				u64 AA1 : 1;
				
				// texture coordinate specification
				// 0: STQ (enables perspective mapping), 1: UV (no perspective mapping like PS1)
				u64 FST : 1;
				
				// context
				// 0: using context 0, 1: using context 1
				u64 CTXT : 1;
				
				// fragment value control (RGBAFSTQ change via DDA) ??
				// 0: unfixed (normal), 1: fixed
				u64 FIX : 1;
			};
			
			u64 Value;
		};
		
		
		union TEST_t
		{
			struct
			{
				// Test Alpha
				// 0: off, 1: on
				u64 ATE : 1;
				
				// Alpha test type
				// 000: always fail, 001: always pass, 010: Alpha<AREF pass, 011: Alpha<=AREF pass,
				// 100: Alpha=AREF pass, 101: Alpha>=AREF pass, 110: Alpha>AREF pass, 111: Alpha!=AREF pass
				u64 ATST : 3;
				
				// Alpha compare value
				u64 AREF : 8;
				
				// what to do when alpha test failed
				// 00: neither frame buffer nor z-buffer updated, 01: only update frame buffer, 10: only update z-buffer, 11: only update frame buffer RGB
				u64 AFAIL : 2;
				
				// destination alpha test
				// 0: off, 1: on
				u64 DATE : 1;
				
				// destination alpha test type
				// 0: pixels with destination alpha of 0 pass, 1: pixels with destination alpha of 1 pass
				u64 DATM : 1;
				
				// z-buffer depth test
				// 0: off, 1: on
				u64 ZTE : 1;
				
				// depth test type
				// 00: all pixels fail, 01: all pixels pass, 10: Z>=ZBUFFER pass, 11: Z>ZBUFFER pass
				u64 ZTST : 2;
			};
			
			u64 Value;
		};
		
		
		union XYZ_t
		{
			struct
			{
				// x/y coords are unsigned
				// 12.4 fixed point
				u64 X : 16;
				
				// 12.4 fixed point
				u64 Y : 16;
				
				u64 Z : 32;
			};
			
			u64 Value;
		};
		
		union XYZF_t
		{
			struct
			{
				// x/y coords are unsigned
				// 12.4 fixed point
				u64 X : 16;
				
				// 12.4 fixed point
				u64 Y : 16;
				
				u64 Z : 24;
				
				u64 F : 8;
			};
			
			u64 Value;
		};
		
		union RGBAQ_t
		{
			struct
			{
				u64 R : 8;
				u64 G : 8;
				u64 B : 8;
				u64 A : 8;
				u64 Q : 32;
			};
			
			u64 Value;
		};
		
		union ST_t
		{
			struct
			{
				u32 S;
				u32 T;
			};
			
			u64 Value;
		};
		
		union UV_t
		{
			struct
			{
				// 10.4 fixed point format
				u64 U : 14;
				
				// padding
				u64 padding0 : 2;
				
				// 10.4 fixed point format
				u64 V : 14;
			};
			
			u64 Value;
		};
		
		
		union TEX0_t
		{
			struct
			{
				// texture base pointer (32-bit word address/64)
				u64 TBP0 : 14;
				
				// texture buffer width (texels/64)
				u64 TBW0 : 6;
				
				// texture format
				// 000000: PSMCT32, 000001: PSMCT24, 000010: PSMCT16, 001010: PSMCT16S, 010011: PSMT8, 010100: PSMT4, 011011: PSMT8H,
				// 100100: PSMT4HL, 101100: PSMT4HH, 110000: PSMZ32, 110001: PSMZ24, 110010: PSMZ16, 111010: PSMZ16S
				u64 PSM : 6;
				
				// texture width (2^x)
				u64 TW : 4;
				
				// texture height (2^x)
				u64 TH : 4;
				
				// texture color components type
				// 0: RGB, 1: RGBA
				u64 TCC : 1;
				
				// color LUT buffer base pointer (32-bit word address/64)
				u64 CBP : 14;
				
				// color LUT pixel format
				// 0000: PSMCT32, 0010: PSMCT16, 1010: PSMCT16S
				u64 CPSM : 4;
				
				// color LUT mode
				// 0: CSM1, 1: CSM2
				u64 CSM : 1;
				
				// color LUT entry offset (offset/16)
				u64 CSA : 5;
				
				// color LUT buffer load mode
				u64 CLD : 3;
			};
			
			u64 Value;
		};
		
		// TEX1 register appears to be MIPMAP information
		
		// TEX2 register has a subset of the items in TEX0
		
		// general purpose GPU registers
		static const int c_iGPURegsGp_Count = 0x63;
		union GPURegsGp_t
		{
			u64 Regs [ c_iGPURegsGp_Count ];
		
			struct
			{
				PRIM_t PRIM;	// 0x00
				RGBAQ_t RGBAQ;
				ST_t ST;
				UV_t UV;
				XYZF_t XYZF2;
				XYZ_t XYZ2;
				TEX0_t TEX0_1;
				TEX0_t TEX0_2;
				u64 CLAMP_1;
				u64 CLAMP_2;
				u64 FOG;
				
				u64 res0;	// 0x0b
				
				XYZF_t XYZF3;
				XYZ_t XYZ3;	// 0x0d
				
				// 0xe-0x13 - 6 registers
				u64 res1 [ 6 ];
				
				u64 TEX1_1;	// 0x14
				u64 TEX1_2;
				u64 TEX2_1;
				u64 TEX2_2;
				XYOFFSET_t XYOFFSET_1;
				XYOFFSET_t XYOFFSET_2;
				u64 PRMODECONT;
				PRIM_t PRMODE;
				u64 TEXCLUT;	// 0x1c
				
				// 0x1d-0x21 - 5 registers
				u64 res2 [ 5 ];
				
				u64 SCANMSK;	// 0x22
				
				// 0x23-0x33 - 17 registers
				u64 res3 [ 17 ];
				
				u64 MIPTBP1_1;	// 0x34
				u64 MIPTBP1_2;
				u64 MIPTBP2_1;
				u64 MIPTBP2_2;	// 0x37
				
				// 0x38-0x3a - 3 registers
				u64 res4 [ 3 ];
				
				u64 TEXA;	// 0x3b
				
				u64 res5;	// 0x3c
				
				u64 FOGCOL;	// 0x3d
				
				u64 res6;	// 0x3e
				
				u64 TEXFLUSH;	// 0x3f
				SCISSOR_t SCISSOR_1;
				SCISSOR_t SCISSOR_2;
				u64 ALPHA_1;
				u64 ALPHA_2;
				u64 DIMX;
				u64 DTHE;
				u64 COLCLAMP;
				TEST_t TEST_1;
				TEST_t TEST_2;
				u64 PABE;
				u64 FBA_1;
				u64 FBA_2;
				FRAME_t FRAME_1;
				FRAME_t FRAME_2;
				ZBUF_t ZBUF_1;
				ZBUF_t ZBUF_2;
				BITBLTBUF_t BITBLTBUF;
				TRXPOS_t TRXPOS;
				TRXREG_t TRXREG;
				TRXDIR_t TRXDIR;
				u64 HWREG;	//0x54
				
				// 0x55-0x5f - 11 registers
				u64 res7 [ 11 ];
				
				u64 SIGNAL;	// 0x60
				u64 FINISH;
				u64 LABEL;	// 0x62
			};
		};
		
		GPURegsGp_t GPURegsGp;
		
		
		// general purpose register indexes
		static const u32 PRIM = 0x0;	// 0x00
		static const u32 RGBAQ = 0x1;
		static const u32 ST = 0x2;
		static const u32 UV = 0x3;
		static const u32 XYZF2 = 0x4;
		static const u32 XYZ2 = 0x5;
		static const u32 TEX0_1 = 0x6;
		static const u32 TEX0_2 = 0x7;
		static const u32 CLAMP_1 = 0x8;
		static const u32 CLAMP_2 = 0x9;
		static const u32 FOG = 0xa;
		static const u32 XYZF3 = 0xc;
		static const u32 XYZ3 = 0xd;	// 0x0d
		static const u32 TEX1_1 = 0x14;	// 0x14
		static const u32 TEX1_2 = 0x15;
		static const u32 TEX2_1 = 0x16;
		static const u32 TEX2_2 = 0x17;
		static const u32 XYOFFSET_1 = 0x18;
		static const u32 XYOFFSET_2 = 0x19;
		static const u32 PRMODECONT = 0x1a;
		static const u32 PRMODE = 0x1b;
		static const u32 TEXCLUT = 0x1c;	// 0x1c
		static const u32 SCANMSK = 0x22;	// 0x22
		static const u32 MIPTBP1_1 = 0x34;	// 0x34
		static const u32 MIPTBP1_2 = 0x35;
		static const u32 MIPTBP2_1 = 0x36;
		static const u32 MIPTBP2_2 = 0x37;	// 0x37
		static const u32 TEXA = 0x3b;	// 0x3b
		static const u32 FOGCOL = 0x3d;	// 0x3d
		static const u32 TEXFLUSH = 0x3f;	// 0x3f
		static const u32 SCISSOR_1 = 0x40;
		static const u32 SCISSOR_2 = 0x41;
		static const u32 ALPHA_1 = 0x42;
		static const u32 ALPHA_2 = 0x43;
		static const u32 DIMX = 0x44;
		static const u32 DTHE = 0x45;
		static const u32 COLCLAMP = 0x46;
		static const u32 TEST_1 = 0x47;
		static const u32 TEST_2 = 0x48;
		static const u32 PABE = 0x49;
		static const u32 FBA_1 = 0x4a;
		static const u32 FBA_2 = 0x4b;
		static const u32 FRAME_1 = 0x4c;
		static const u32 FRAME_2 = 0x4d;
		static const u32 ZBUF_1 = 0x4e;
		static const u32 ZBUF_2 = 0x4f;
		static const u32 BITBLTBUF = 0x50;
		static const u32 TRXPOS = 0x51;
		static const u32 TRXREG = 0x52;
		static const u32 TRXDIR = 0x53;
		static const u32 HWREG = 0x54;	//0x54
		static const u32 SIGNAL = 0x60;	// 0x60
		static const u32 FINISH = 0x61;
		static const u32 LABEL = 0x62;	// 0x62
		
		
		
		
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
		
		// gets cycle @ start of scanline
		double GetScanline_Start ();
		
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
		

		
		//////////////////////////////////
		//	Device Specific Parameters	//
		//////////////////////////////////
		

		
		
		
		// System data
		//static const double SystemBus_CyclesPerSec = 147456000.0L;
		
		// Raster data
		static const double NTSC_FieldsPerSec = 59.94005994L;
		static const double PAL_FieldsPerSec = 50.0L;
		static const double NTSC_FramesPerSec = ( 59.94005994L / 2.0L );	//NTSC_FieldsPerSec / 2;
		static const double PAL_FramesPerSec = ( 50.0L / 2.0L );	//PAL_FieldsPerSec / 2;
		static const double NTSC_CyclesPerFrame = ( 147456000.0L / ( 59.94005994L / 2.0L ) );
		static const double PAL_CyclesPerFrame = ( 147456000.0L / ( 50.0L / 2.0L ) );
		
		static const double NTSC_FramesPerCycle = 1.0L / ( 147456000.0L / ( 59.94005994L / 2.0L ) );
		static const double PAL_FramesPerCycle = 1.0L / ( 147456000.0L / ( 50.0L / 2.0L ) );
		
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
		static const double NTSC_CyclesPerScanline = ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );	//SystemBus_CyclesPerSec / NTSC_ScanlinesPerSec; //2152.5504000022;
		static const double PAL_CyclesPerScanline = ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );	//SystemBus_CyclesPerSec / PAL_ScanlinesPerSec; //2167.6032;
		
		static const double NTSC_ScanlinesPerCycle = 1.0L / ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		static const double PAL_ScanlinesPerCycle = 1.0L / ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		
		
		
		static const double NTSC_CyclesPerField_Even = 263.0L * ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		static const double NTSC_CyclesPerField_Odd = 262.0L * ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		static const double PAL_CyclesPerField_Even = 313.0L * ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		static const double PAL_CyclesPerField_Odd = 312.0L * ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		
		static const double NTSC_FieldsPerCycle_Even = 1.0L / ( 263.0L * ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		static const double NTSC_FieldsPerCycle_Odd = 1.0L / ( 262.0L * ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		static const double PAL_FieldsPerCycle_Even = 1.0L / ( 313.0L * ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		static const double PAL_FieldsPerCycle_Odd = 1.0L / ( 312.0L * ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );



		static const double NTSC_DisplayAreaCycles = 240.0L * ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		static const double PAL_DisplayAreaCycles = 288.0L * ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		
		static const double NTSC_CyclesPerVBlank_Even = ( 263.0L - 240.0L ) * ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		static const double NTSC_CyclesPerVBlank_Odd = ( 262.0L - 240.0L ) * ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) );
		static const double PAL_CyclesPerVBlank_Even = ( 313.0L - 288.0L ) * ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		static const double PAL_CyclesPerVBlank_Odd = ( 312.0L - 288.0L ) * ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) );
		
		static const double NTSC_VBlanksPerCycle_Even = 1.0L / ( ( 263.0L - 240.0L ) * ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		static const double NTSC_VBlanksPerCycle_Odd = 1.0L / ( ( 262.0L - 240.0L ) * ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) );
		static const double PAL_VBlanksPerCycle_Even = 1.0L / ( ( 313.0L - 288.0L ) * ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );
		static const double PAL_VBlanksPerCycle_Odd = 1.0L / ( ( 312.0L - 288.0L ) * ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) );



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
		
		
		// the current start cycles for the current scanline (in both double and long long)
		// recalculate whenever the internal resolution gets changed, even slightly
		double dScanlineStart, dNextScanlineStart, dHBlankStart;
		unsigned long long llScanlineStart, llNextScanlineStart, llHBlankStart;
		unsigned long lScanline, lNextScanline, lVBlank, lMaxScanline;
		
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
		
		//static const u32 c_iGPUCyclesPerPixel [];
		
		static const double c_dGPUCyclesPerScanline_NTSC = 147456000.0L / ( 525.0L * 30.0L );
		static const double c_dGPUCyclesPerScanline_PAL = 147456000.0L / ( 625.0L * 30.0L );

		
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
		
		static const double NTSC_CyclesPerPixel_256 = ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 341.0L );
		static const double NTSC_CyclesPerPixel_320 = ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 426.0L );
		static const double NTSC_CyclesPerPixel_368 = ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 487.0L );
		static const double NTSC_CyclesPerPixel_512 = ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 682.0L );
		static const double NTSC_CyclesPerPixel_640 = ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 853.0L );
		static const double PAL_CyclesPerPixel_256 = ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 340.0L );
		static const double PAL_CyclesPerPixel_320 = ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 426.0L );
		static const double PAL_CyclesPerPixel_368 = ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 486.0L );
		static const double PAL_CyclesPerPixel_512 = ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 681.0L );
		static const double PAL_CyclesPerPixel_640 = ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 851.0L );
		
		static const double NTSC_PixelsPerCycle_256 = 1.0L / ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 341.0L );
		static const double NTSC_PixelsPerCycle_320 = 1.0L / ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 426.0L );
		static const double NTSC_PixelsPerCycle_368 = 1.0L / ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 487.0L );
		static const double NTSC_PixelsPerCycle_512 = 1.0L / ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 682.0L );
		static const double NTSC_PixelsPerCycle_640 = 1.0L / ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 853.0L );
		static const double PAL_PixelsPerCycle_256 = 1.0L / ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 340.0L );
		static const double PAL_PixelsPerCycle_320 = 1.0L / ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 426.0L );
		static const double PAL_PixelsPerCycle_368 = 1.0L / ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 486.0L );
		static const double PAL_PixelsPerCycle_512 = 1.0L / ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 681.0L );
		static const double PAL_PixelsPerCycle_640 = 1.0L / ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 851.0L );
		
		
		static const u64 NTSC_CyclesPerPixelINC_256 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 341.0L ) ))) + ( 1 << 8 );
		static const u64 NTSC_CyclesPerPixelINC_320 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 426.0L ) ))) + ( 1 << 8 );
		static const u64 NTSC_CyclesPerPixelINC_368 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 487.0L ) ))) + ( 1 << 8 );
		static const u64 NTSC_CyclesPerPixelINC_512 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 682.0L ) ))) + ( 1 << 8 );
		static const u64 NTSC_CyclesPerPixelINC_640 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 147456000.0L / ( 525.0L * ( 59.94005994L / 2.0L ) ) ) / 853.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_256 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 340.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_320 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 426.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_368 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 486.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_512 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 681.0L ) ))) + ( 1 << 8 );
		static const u64 PAL_CyclesPerPixelINC_640 = ((u64) (( (1ull) << 63 ) * ( 1.0L / ( ( 147456000.0L / ( 625.0L * ( 50.0L / 2.0L ) ) ) / 851.0L ) ))) + ( 1 << 8 );
		
		//static const double NTSC_CyclesPerPixel_Values [] = { 6.3124645161, 5.0529352113, 4.4200213552, 3.1562322581, 2.5235057444 };
		//static const double PAL_CyclesPerPixel_Values [] = { 6.3753035294, 5.0882704225, 4.4600888889, 3.1829709251, 2.5471247944 };
		
		// the last command written to CTRL register
		//u32 CommandReady;
		//CTRL_Write_Format NextCommand;
		
		// we need to know if the GPU is busy or not
		s64 BusyCycles;
		
		// we need a "command buffer" for GPU
		// stuff written to DATA register or via dma goes into command buffer
		//u32 BufferMode;
		//enum { MODE_NORMAL, MODE_IMAGEIN, MODE_IMAGEOUT };
		//DATA_Write_Format Buffer [ 16 ];
		//u32 BufferSize;
		//u32 PixelsLeftToTransfer;
		
		// dither values array
		//static const s64 c_iDitherValues [];
		//static const s64 c_iDitherZero [];
		//static const s64 c_iDitherValues24 [];
		//static const s32 c_iDitherValues4 [];
		//static const s16 c_viDitherValues16_add [];
		//static const s16 c_viDitherValues16_sub [];

		// draw area data
		
		// upper left hand corner of frame buffer area being drawn to the tv
		u32 ScreenArea_TopLeftX;
		u32 ScreenArea_TopLeftY;
		
		// bounding rectangles of the area being drawn to - anything outside of this is not drawn
		//u32 DrawArea_TopLeftX;
		//u32 DrawArea_TopLeftY;
		//u32 DrawArea_BottomRightX;
		//u32 DrawArea_BottomRightY;
		
		// also need to maintain the internal coords to return when requested
		//u32 iREG_DrawArea_TopLeftX;
		//u32 iREG_DrawArea_TopLeftY;
		//u32 iREG_DrawArea_BottomRightX;
		//u32 iREG_DrawArea_BottomRightY;
		
		// the offset from the upper left hand corner of frame buffer that is being drawn to. Applies to all drawing
		// these are signed and go from -1024 to +1023
		//s32 DrawArea_OffsetX;
		//s32 DrawArea_OffsetY;
		
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
		
		/*
		static const u32 FrameBuffer_Width = 1024;
		static const u32 FrameBuffer_Height = 512;
		static const u32 FrameBuffer_XMask = FrameBuffer_Width - 1;
		static const u32 FrameBuffer_YMask = FrameBuffer_Height - 1;
		*/
		
		// constructor
		GPU ();
		
		// debug info
		static u32* DebugCpuPC;
		
		
		// write to a general purpose register
		void WriteReg ( u32 lIndex, u64 Value );
		void WriteReg_Packed ( u32 lIndex, u64 ull0, u64 ull1 );
		
		// perform drawing kick
		void DrawingKick ();
		
		// perform actual drawing of primitive (triangle, sprite, etc)
		void DrawPrimitive ();
		
		
		u32 lVertexCount;
		u32 lVertexQ_Index;
		static const u32 c_iVertexQ_Size = 4;
		static const u32 c_iVertexQ_Mask = c_iVertexQ_Size - 1;
		//u32 x [ 4 ], y [ 4 ], z [ 4 ], f [ 4 ], r [ 4 ], g [ 4 ], b [ 4 ], a [ 4 ], q [ 4 ], u [ 4 ], v [ 4 ], s [ 4 ], t [ 4 ];
		
		XYZ_t xyz [ c_iVertexQ_Size ];
		RGBAQ_t rgbaq [ c_iVertexQ_Size ];
		UV_t uv [ c_iVertexQ_Size ];
		ST_t st [ c_iVertexQ_Size ];
		XYZF_t f [ c_iVertexQ_Size ];
		
		// there is also an internal Q register for packed mode
		u64 Internal_Q;
		
		inline void StartPrimitive () { lVertexCount = 0; lVertexQ_Index = 0; }

		/*
		inline void SetX ( u32 Value ) { x [ lVertexQ_Index ] = Value; }
		inline void SetY ( u32 Value ) { y [ lVertexQ_Index ] = Value; }
		inline void SetZ ( u32 Value ) { z [ lVertexQ_Index ] = Value; }
		inline void SetF ( u32 Value ) { f [ lVertexQ_Index ] = Value; }
		inline void SetR ( u32 Value ) { r [ lVertexQ_Index ] = Value; }
		inline void SetG ( u32 Value ) { g [ lVertexQ_Index ] = Value; }
		inline void SetB ( u32 Value ) { b [ lVertexQ_Index ] = Value; }
		inline void SetA ( u32 Value ) { a [ lVertexQ_Index ] = Value; }
		inline void SetQ ( u32 Value ) { q [ lVertexQ_Index ] = Value; }
		inline void SetU ( u32 Value ) { u [ lVertexQ_Index ] = Value; }
		inline void SetV ( u32 Value ) { v [ lVertexQ_Index ] = Value; }
		inline void SetS ( u32 Value ) { s [ lVertexQ_Index ] = Value; }
		inline void SetT ( u32 Value ) { t [ lVertexQ_Index ] = Value; }
		*/
		
		inline void SetF ( u64 Value ) { f [ lVertexQ_Index & c_iVertexQ_Mask ].Value = Value; }
		inline void SetXYZ ( u64 Value ) { xyz [ lVertexQ_Index & c_iVertexQ_Mask ].Value = Value; }
		inline void SetRGBAQ ( u64 Value ) { rgbaq [ lVertexQ_Index & c_iVertexQ_Mask ].Value = Value; }
		inline void SetUV ( u64 Value ) { uv [ lVertexQ_Index & c_iVertexQ_Mask ].Value = Value; }
		inline void SetST ( u64 Value ) { st [ lVertexQ_Index & c_iVertexQ_Mask ].Value = Value; }
		
		inline void VertexKick () { lVertexQ_Index++; lVertexCount++; }
		
		
		void ExecuteGPUBuffer ();
		
		void ProcessDataRegWrite ( u32 Data );
		u32 ProcessDataRegRead ();
		
		void TransferPixelPacketIn ( u32 Data );
		u32 TransferPixelPacketOut ();
		
		
		///////////////////////////////////////////////////////////////////////
		// Function to perform actual drawing of primitives on screen area


		void DrawPoint ();
		void DrawLine ();
		void DrawTriangle ();
		void DrawTriangleFan ();
		void DrawSprite ();

		
		// global rendering variables //
		
		// context
		u32 Ctx;
		
		// frame buffer
		u32 FrameBufferStartOffset32, FrameBufferWidth_Pixels, FrameBuffer_PixelFormat;
		
		// texture buffer
		u32 TexBufStartOffset32, TexBufWidth_Pixels, TexBuf_PixelFormat;
		
		// clut
		u32 CLUTStartOffset32, CLUTWidth_Pixels, CLUT_BitsPerPixel;
		
		// bit blt
		u32 XferSrcOffset32, XferDstOffset32, XferSrcBufWidth, XferDstBufWidth, XferSrcX, XferSrcY, XferDstX, XferDstY, XferSrcPixelSize, XferDstPixelSize, XferWidth, XferHeight;
		u32 XferX, XferY;
		u64 PixelCount;
		u64 PixelShift;
		
		// window coords (these values are inclusive)
		// window coords are unsigned
		s32 Window_XLeft, Window_XRight, Window_YTop, Window_YBottom;
		
		// x,y offset (12.4 fixed point format -> would imagine this is signed)
		s32 Coord_OffsetX, Coord_OffsetY;
		
		// object texture mapped?
		u32 TextureMapped;

		// gradient?
		u32 Gradient;
		
		
		void SetDrawVars ();
		void RenderPoint ();
		void RenderLine ();
		void RenderTriangleColor ();
		void RenderTriangleTexture ();
		void RenderRectangle ();
		void RenderSprite ();
		
		void DrawTriangle_Mono32 ();
		void DrawTriangle_Gradient32 ();
		
		
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
		
		
		inline static u32 ConvertPixel16To24 ( u32 Pixel16 )
		{
			u32 Pixel32;
			Pixel32 = ( Pixel16 & 0x1f ) << 3;
			Pixel32 |= ( Pixel16 & 0x3e0 ) << 6;
			Pixel32 |= ( Pixel16 & 0x7c00 ) << 9;
			return Pixel32;
		}

		
		
		
		
		

		static const u32 c_InterruptCpuNotifyBit = 0;
		static const u32 c_InterruptBit = 0;
		static const u32 c_InterruptBit_Vsync_Start = 2;
		static const u32 c_InterruptBit_Vsync_End = 3;

		static u32* _Intc_Stat;
		static u32* _Intc_Mask;
		static u32* _R5900_Status_12;
		static u32* _R5900_Cause_13;
		static u64* _ProcStatus;
		
		inline void ConnectInterrupt ( u32* _IStat, u32* _IMask, u32* _R5900_Status, u32* _R5900_Cause, u64* _ProcStat )
		{
			_Intc_Stat = _IStat;
			_Intc_Mask = _IMask;
			_R5900_Cause_13 = _R5900_Cause;
			_R5900_Status_12 = _R5900_Status;
			_ProcStatus = _ProcStat;
		}
		
		
		inline void SetInterrupt_Vsync_Start ()
		{
			*_Intc_Stat |= ( 1 << c_InterruptBit_Vsync_Start );
			if ( *_Intc_Stat & *_Intc_Mask ) *_R5900_Cause_13 |= ( 1 << 10 );
			
			if ( ( *_R5900_Cause_13 & *_R5900_Status_12 & 0xff00 ) && ( *_R5900_Status_12 & 1 ) )
			{
				*_ProcStatus |= ( 1 << c_InterruptCpuNotifyBit );
			}
			else
			{
				*_ProcStatus &= ~( 1 << c_InterruptCpuNotifyBit );
			}
		}
		
		inline void SetInterrupt_Vsync_End ()
		{
			*_Intc_Stat |= ( 1 << c_InterruptBit_Vsync_End );
			if ( *_Intc_Stat & *_Intc_Mask ) *_R5900_Cause_13 |= ( 1 << 10 );
			
			if ( ( *_R5900_Cause_13 & *_R5900_Status_12 & 0xff00 ) && ( *_R5900_Status_12 & 1 ) )
			{
				*_ProcStatus |= ( 1 << c_InterruptCpuNotifyBit );
			}
			else
			{
				*_ProcStatus &= ~( 1 << c_InterruptCpuNotifyBit );
			}
		}
		
		static inline void SetInterrupt ()
		{
			*_Intc_Stat |= ( 1 << c_InterruptBit );
			if ( *_Intc_Stat & *_Intc_Mask ) *_R5900_Cause_13 |= ( 1 << 10 );
			
			// ***TODO*** more stuff needs to probably be checked on an R5900 to determine if interrupts are enabled or not
			// it probably doesn't matter, but should probably correct that
			if ( ( *_R5900_Cause_13 & *_R5900_Status_12 & 0xff00 ) && ( *_R5900_Status_12 & 1 ) )
			{
				*_ProcStatus |= ( 1 << c_InterruptCpuNotifyBit );
			}
			else
			{
				*_ProcStatus &= ~( 1 << c_InterruptCpuNotifyBit );
			}
		}
		
		
		/*
		inline void ClearInterrupt ()
		{
			//*_Intc_Master &= ~( 1 << c_InterruptBit );
			*_Intc_Stat &= ~( 1 << c_InterruptBit );
			if ( ! ( *_Intc_Stat & *_Intc_Mask ) ) *_R3000A_Cause_13 &= ~( 1 << 10 );
			
			if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 ); else *_ProcStatus &= ~( 1 << 20 );
		}

		inline void ClearInterrupt_Vsync ()
		{
			//*_Intc_Master &= ~( 1 << c_InterruptBit_Vsync );
			*_Intc_Stat &= ~( 1 << c_InterruptBit_Vsync );
			if ( ! ( *_Intc_Stat & *_Intc_Mask ) ) *_R3000A_Cause_13 &= ~( 1 << 10 );
			
			if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 ); else *_ProcStatus &= ~( 1 << 20 );
		}
		*/

		
		static u64* _NextSystemEvent;

		
		////////////////////////////////
		// Debug Info
		static u32* _DebugPC;
		static u64* _DebugCycleCount;
		
		static bool DebugWindow_Enabled;
		static void DebugWindow_Enable ();
		static void DebugWindow_Disable ();
		static void DebugWindow_Update ();

	};
}


#endif

