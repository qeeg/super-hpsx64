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


#ifndef _PS2_VU_H_
#define _PS2_VU_H_

#include "types.h"
#include "Debug.h"

//#include "WinApiHandler.h"
//#include "GNUAsmUtility_x64.h"

#ifdef ENABLE_GUI_DEBUGGER
#include "WinApiHandler.h"
// Windows API specific code is in this header file
#include "DebugValueList.h"
#include "DebugMemoryViewer.h"
#include "DisassemblyViewer.h"
#include "BreakpointWindow.h"
#endif


#include "VU_Instruction.h"

//#include "emmintrin.h"



#ifdef _ENABLE_SSE2

// need to include this file to use SSE2 intrinsics
//#include "emmintrin.h"
//#include "smmintrin.h"

#endif


using namespace Vu;
//using namespace x64Asm::Utilities;




namespace Playstation2
{

	class VU1;

	class VU
	{
	
		static Debug::Log debug;
		
	public:
	
		static const int c_iMaxInstances = 2;
		static VU *_VU [ c_iMaxInstances ];
		
		// this variable has nothing to do with whether it will be VU0 or VU1
		static int iInstance;
		
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

		static const int c_iNumVuRegs = 24;
		static const char* VU0RegNames [ c_iNumVuRegs ];
		static const char* VU1RegNames [ c_iNumVuRegs ];
		
		// VU Clock Speed in Hertz
		static const unsigned long long c_llClockSpeed1 = 147456000;
		static const unsigned long long c_llClockSpeed2 = 149500000;

		static const double c_dClockSpeed1 = 147456000.0L;
		static const double c_dClockSpeed2 = 149500000.0L;
		
		
		// the number of vu cycles for every cpu cycle or vice versa
		static const double c_dVUPerCPU_Cycles = ( 1.0L / 2.0L );
		static const double c_dCPUPerVU_Cycles = ( 2.0L / 1.0L );
		
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
		
		
		
		// 0x11000000 is physical address of where micromem starts at
		static const u32 c_ulMicroMem0_Start = 0x0;	//0x11000000;
		static const u32 c_ulMicroMem0_Size = 0x1000;	// 4 KB
		static const u32 c_ulMicroMem0_Mask = c_ulMicroMem0_Size - 1;
		
		static const u32 c_ulVuMem0_Start = 0x0;	//0x11004000;
		static const u32 c_ulVuMem0_Size = 0x1000;	// 4 KB
		static const u32 c_ulVuMem0_Mask = c_ulVuMem0_Size - 1;

		
		static const u32 c_ulMicroMem1_Start = 0x0;	//0x11008000;
		static const u32 c_ulMicroMem1_Size = 0x4000;	// 16 KB
		static const u32 c_ulMicroMem1_Mask = c_ulMicroMem1_Size - 1;
		
		static const u32 c_ulVuMem1_Start = 0x0;	//0x1100c000;
		static const u32 c_ulVuMem1_Size = 0x4000;	// 16 KB
		static const u32 c_ulVuMem1_Mask = c_ulVuMem1_Size - 1;
		
		
		u32* GetMemPtr ( u32 Address32 );
		

		/*
		struct ControlRegs_t
		{
		
			union
			{

				// there are 32 COP2 control registers
				unsigned long Regs [ 32 ];
				
				struct
				{
					u32 Reg0;
					u32 Reg1;
					u32 Reg2;
					u32 Reg3;
					u32 Reg4;
					u32 Reg5;
					u32 Reg6;
					u32 Reg7;
					u32 Reg8;
					u32 Reg9;
					u32 Reg10;
					u32 Reg11;
					u32 Reg12;
					u32 Reg13;
					u32 Reg14;
					u32 Reg15;
					u32 StatusFlags;
					u32 MACFlags;
					u32 ClippingFlags;
					u32 Reserved0;
					
					// these three cannot be read or written while VU0 is operating
					u32 R;
					u32 I;
					u32 Q;
					
					// this one is only bits 0-15, the rest are zero
					// cannot be read or written while VU0 is operating
					u32 TPC;
				};
			};
			
		};
		*/

		
		union StatusFlags_t
		{
			u32 Value;
			
			struct
			{
				// bit 0 - zero flag - gets set when any of the Zx, Zy, Zz, Zw flags are set in MAC flag
				u32 ZeroFlag : 1;
				
				// bit 1 - sign flag
				u32 SignFlag : 1;
				
				// bit 2 - underflow flag
				u32 UnderflowFlag : 1;
				
				// bit 3 - overflow flag
				u32 OverflowFlag : 1;
				
				// bit 4 - invalid flag - gets set on either 0/0 or SQRT/RSQRT on a negative number
				u32 InvalidFlag : 1;
				
				// bit 5 - division by zero flag - get set when either DIV/RSQRT does division by zero
				u32 DivideByZeroFlag : 1;
				
				// bit 6 - sticky zero flag - gets set when any of the Zx, Zy, Zz, Zw flags are set in MAC flag
				u32 StickyZeroFlag : 1;
				
				// bit 7 - sticky sign flag
				u32 StickySignFlag : 1;
				
				// bit 8 - sticky underflow flag
				u32 StickyUnderflowFlag : 1;
				
				// bit 9 - sticky overflow flag
				u32 StickyOverflowFlag : 1;
				
				// bit 10 - sticky invalid flag - gets set on either 0/0 or SQRT/RSQRT on a negative number
				u32 StickyInvalidFlag : 1;
				
				// bit 11 - sticky division by zero flag - get set when either DIV/RSQRT does division by zero
				u32 StickyDivideByZeroFlag : 1;
			};
		};
		
		StatusFlags_t StatusFlags;
		
		// vu registers

		// COP2 //
		
		// last 16 registers are control registers
		// #16 - Status flag
		// #17 - MAC flag
		// read only
		// #18 - clipping flag
		// #19 - reserved
		// #20 - R register
		// #21 - I register
		// #22 - Q register
		// #23 - P register
		// #24-#25 - reserved
		// #26 - TPC register
		// read only except while running
		// #27 - CMSAR0 register
		// #28 - FBRST register
		// #29 - VPU-STAT register
		// read only
		// #30 - reserved
		// #31 - CMSAR1 register
		// while VU1 is stopped, writing to here starts it at address that is written
		
		// COP2 registers (for now)
		Reg128 vf [ 32 ];
		Reg32 vi [ 32 ];
		
		// note: these should be mapped to VI registers
		//Reg32 I, P, Q, R;
		
		
		// accumulator
		DoubleLong dACC [ 4 ];
		
		// flags ??
		Reg128 zero_flag, sign_flag, overflow_flag, underflow_flag;
		Reg128 zero_stickyflag, sign_stickyflag, overflow_stickyflag, underflow_stickyflag;
		
		u32 divide_flag, invalid_negative_flag, invalid_zero_flag;
		u32 divide_stickyflag, invalid_negative_stickyflag, invalid_zero_stickyflag;
		
		union ClippingFlag_t
		{
			struct
			{
				u32 x_plus0 : 1;
				u32 x_minus0 : 1;
				u32 y_plus0 : 1;
				u32 y_minus0 : 1;
				u32 z_plus0 : 1;
				u32 z_minus0 : 1;
				
				u32 x_plus1 : 1;
				u32 x_minus1 : 1;
				u32 y_plus1 : 1;
				u32 y_minus1 : 1;
				u32 z_plus1 : 1;
				u32 z_minus1 : 1;
				
				u32 x_plus2 : 1;
				u32 x_minus2 : 1;
				u32 y_plus2 : 1;
				u32 y_minus2 : 1;
				u32 z_plus2 : 1;
				u32 z_minus2 : 1;
				
				u32 x_plus3 : 1;
				u32 x_minus3 : 1;
				u32 y_plus3 : 1;
				u32 y_minus3 : 1;
				u32 z_plus3 : 1;
				u32 z_minus3 : 1;
			};
			
			u32 Value;
		};
		
		ClippingFlag_t ClippingFlag;
		
		u32 PC;
		u32 NextPC;
		u32 LastPC;
		u32 Last_ReadAddress;
		u32 Last_WriteAddress;
		u32 Last_ReadWriteAddress;
		
		u64 CycleCount;
		
		// use this flag to control whether VU is running or not
		// once the VU gets the command to run code, this should get set to 1, and then get cleared to 0 when done
		u32 Running;
		
		// use this flag to stop transfer via VIF, for example, while VU is running
		u32 VifStopped;
		
		// these need to be set by creator of object
		u32 ulVuMem_Start, ulMicroMem_Start;
		u32 ulVuMem_Size, ulMicroMem_Size;
		u32 ulVuMem_Mask, ulMicroMem_Mask;
		
		// cycle that the next event will happen at for this device
		u64 NextEvent_Cycle, NextEvent_Cycle_Vsync;

		// the cycle that device is busy until
		u64 BusyUntil_Cycle;

		u32 Number;

		u64 Read ( u32 Address, u64 Mask );
		void Write ( u32 Address, u64 Data, u64 Mask );
		
		void DMA_Read ( u32* Data, int ByteReadCount );
		void DMA_Write ( u32* Data, int ByteWriteCount );
		
		// this will need to return how many quad words were fully written out of number sent
		u32 DMA_WriteBlock ( u64* Data, u32 QuadwordCount );
		
		// this will need to return how many quadwords were fully read from device out of number requested
		u32 DMA_ReadBlock ( u64* Data, u32 QuadwordCount );
		
		bool DMA_Write_Ready ();
		bool DMA_Read_Ready ();
		
		void Start ( int Number );

		// returns either vblank interrupt signal, gpu interrupt signal, or no interrupt signal
		void Run ();
		void Reset ();
		
		void ProcessBranchDelaySlot ();
		
		
		u32 Read_CFC ( u32 Register );
		void Write_CTC ( u32 Register, u32 Data );
		
		
		union VifCode_t
		{
			struct
			{
				// the immediate value - bits 0-15
				u32 IMMED : 16;
				
				// num field - bits 16-23
				u32 NUM : 8;
				
				// cmd field - bits 24-30
				u32 CMD : 7;
				
				// interrupt bit
				u32 INT : 1;
			};
			
			struct
			{
				u32 padding0 : 24;
				
				u32 vl : 2;
				u32 vn : 2;
				u32 m : 1;
				u32 CMDHIGH : 2;
				u32 padding : 1;
			};
			
			u32 Value;
		};
		
		// ***todo*** also reset these when VU gets reset
		u32 lVifIdx;
		VifCode_t VifCode;
		u32 lVifCodeState;
		u32 lVifCommandSize;
		
		// need this to return the number of quadwords read and update the offset so it points to correct data for next time
		u32 VIF_FIFO_Execute ( u32* Data, u32 SizeInWords32 );
		
		// these need to be set externally for now
		static const int c_iMaxMemSize = 0x4000;
		u32 MemSize;
		u32 MemMask;
		
		// this is the vu program memory
		union
		{
			u8 MicroMem8 [ c_iMaxMemSize ];
			u16 MicroMem16 [ c_iMaxMemSize >> 1 ];
			u32 MicroMem32 [ c_iMaxMemSize >> 2 ];
			u64 MicroMem64 [ c_iMaxMemSize >> 3 ];
		};
		
		// this is the vu data memory
		union
		{
			u8 VuMem8 [ c_iMaxMemSize ];
			u16 VuMem16 [ c_iMaxMemSize >> 1 ];
			u32 VuMem32 [ c_iMaxMemSize >> 2 ];
			u64 VuMem64 [ c_iMaxMemSize >> 3 ];
		};
		
		
		union CYCLE_t
		{
			struct
			{
				u32 CL : 8;
				u32 WL : 8;
				u32 padding0 : 16;
			};
			
			u32 Value;
		};
		
		
		// STAT
		union VifStat_t
		{
			struct
			{
				// VIF Status - bits 0-1 - read only
				// 00: Idle, 01: Waiting for data after VIF Code, 10: Processing VIF Code, 11: Processing/Transferring data after VIF Code
				u32 VPS : 2;
				
				// E-bit/Executing - bit 2 - read only
				// 0: Not executing program, 1: Executing program
				u32 VEW : 1;
				
				// GIF Transfer status - bit 3 - read only
				// 0: Not waiting for GIF, 1: Waiting for GIF due to FLUSH/FLUSHA or DIRECT/DIRECTHL or MSCALF
				// VIF1 only
				u32 VGW : 1;
				
				// zero - bits 4-5
				u32 zero0 : 2;
				
				// MARK detected - bit 6 - read only
				// 0: MARK not detected, 1: MARK detected
				u32 MRK : 1;
				
				// Double Buffer bit - bit 7 - read only
				// 0: TOPS is buffer 0 (BASE), 1: TOPS is buffer 1 (BASE+OFFSET)
				// VIF1 only
				u32 DBF : 1;
				
				// encountered STOP - bit 8 - read only
				// 0: No, 1: stalled due to STOP
				u32 VSS : 1;
				
				// encountered ForceBreak - bit 9 - read only
				// 0: No, 1: stalled due to ForceBreak
				u32 VFS : 1;
				
				// encountered VIF1 interrupt - bit 10 - read only
				// 0: No, 1: stalled due to VIF1 interrupt
				u32 VIS : 1;
				
				// encountered I-bit interrupt - bit 11 - read only
				// 0: No, 1: detected I-bit interrupt
				u32 INT : 1;
				
				// dma tag error - bit 12 - read only
				// 0: No, 1: error
				u32 ER0 : 1;
				
				// vif code error - bit 13 - read only
				// 0: No, 1: error
				u32 ER1 : 1;
				
				// zero - bits 14-22
				u32 zero1 : 9;
				
				// VIF-FIFO transfer direction - bit 23 - read/write
				// 0: memory->VIF, 1: VIF->memory
				// VIF1 only
				u32 FDR : 1;
				
				// Count of Quadwords of data in VIF fifo - bits 24-28 - read only
				// 00000: 0 qw (empty), 00001: 1 qw, ... 10000: 16 qw (full)
				u32 FQC : 5;
			};
		};
		
		
		// vif registers
		union VifRegs_t
		{
			u32 Regs [ c_iNumVuRegs ];
			
			struct
			{
				VifStat_t STAT;	// register #0
				u32 FBRST;
				u32 ERR;
				u32 MARK;
				CYCLE_t CYCLE;
				u32 MODE;
				u32 NUM;
				u32 MASK;
				u32 CODE;
				u32 ITOPS;
				u32 BASE;
				u32 OFST;
				u32 TOPS;
				u32 ITOP;
				u32 TOP;
				u32 RES;
				u32 R0;		// register #16
				u32 R1;
				u32 R2;
				u32 R3;
				u32 C0;		// register #20
				u32 C1;
				u32 C2;
				u32 C3;
			};
		};
		
		VifRegs_t VifRegs;

		
		//////////////////////////////////
		//	Device Specific Parameters	//
		//////////////////////////////////
		
		
		// callback functions //
		typedef void (*cbFunction) ( void );
		
		
		union ProcStatus
		{
			struct
			{
				u8 CheckInterrupt;
				u8 DelaySlot_Valid;
				u8 LoadBuffer_Valid;
				u8 StoreBuffer_Valid;
				u8 EBitDelaySlot_Valid;
			};
			
			u64 Value;
		};

		// processor status - I'll use this to make it run faster
		ProcStatus Status;
		
		
		struct DelaySlot
		{
			union
			{
				struct
				{
					Instruction::Format Instruction;
					u32 Data;
					cbFunction cb;
				};
				
				struct
				{
					u64 Value;
					cbFunction Value2;
				};
			};
			
			//Execute::Callback_Function cb;
		};
		
		DelaySlot DelaySlot0;
		DelaySlot DelaySlot1;

		u32 NextDelaySlotIndex;
		DelaySlot DelaySlots [ 2 ];
		
		
		// we need to know if the GPU is busy or not
		s64 BusyCycles;
		

		
		// constructor
		VU ();
		
		// debug info
		static u32* DebugCpuPC;
		

		

		static const u32 c_InterruptCpuNotifyBit = 0;
		static const u32 c_InterruptBit_VIF = 4;
		static const u32 c_InterruptBit_VU = 6;

		static u32* _Intc_Stat;
		static u32* _Intc_Mask;
		static u32* _R5900_Status_12;
		static u32* _R5900_Cause_13;
		static u64* _ProcStatus;
		
		inline static void ConnectInterrupt ( u32* _IStat, u32* _IMask, u32* _R5900_Status, u32* _R5900_Cause, u64* _ProcStat )
		{
			_Intc_Stat = _IStat;
			_Intc_Mask = _IMask;
			_R5900_Cause_13 = _R5900_Cause;
			_R5900_Status_12 = _R5900_Status;
			_ProcStatus = _ProcStat;
		}
		
		
		
		inline void SetInterrupt_VIF ()
		{
			*_Intc_Stat |= ( 1 << ( c_InterruptBit_VIF + Number ) );
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


		inline void SetInterrupt_VU ()
		{
			*_Intc_Stat |= ( 1 << ( c_InterruptBit_VU + Number ) );
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

		
#ifdef ENABLE_GUI_DEBUGGER
		static WindowClass::Window *DebugWindow [ c_iMaxInstances ];
		static DebugValueList<float> *GPR_ValueList [ c_iMaxInstances ];
		static DebugValueList<u32> *COP0_ValueList [ c_iMaxInstances ];
		static DebugValueList<u32> *COP2_CPCValueList [ c_iMaxInstances ];
		static DebugValueList<u32> *COP2_CPRValueList [ c_iMaxInstances ];
		static Debug_DisassemblyViewer *DisAsm_Window [ c_iMaxInstances ];
		static Debug_BreakpointWindow *Breakpoint_Window [ c_iMaxInstances ];
		static Debug_MemoryViewer *ScratchPad_Viewer [ c_iMaxInstances ];
		static Debug_BreakPoints *Breakpoints [ c_iMaxInstances ];
#endif


		static bool DebugWindow_Enabled [ c_iMaxInstances ];
		static void DebugWindow_Enable ( int Number );
		static void DebugWindow_Disable ( int Number );
		static void DebugWindow_Update ( int Number );

	};
	
	// ***TODO*** I should probably use a class template here instead
	// wrapper for static functions
	class VU0
	{
	public:
	
		static VU *_VU0;
		
		VU VU0;
		
		inline void Start ()
		{
			_VU0 = & VU0;
			
			VU0.Start ( 0 );
			//VU0.Number = 0;
		}
		
		inline void Reset () { VU0.Reset (); }
		
		inline static u64 Read ( u32 Address, u64 Mask ) { _VU0->Read ( Address, Mask ); }
		inline static void Write ( u32 Address, u64 Data, u64 Mask ) { _VU0->Write( Address, Data, Mask ); }
		
		inline static void DMA_WriteBlock ( u64* Data, u32 QuadwordCount ) { _VU0->DMA_WriteBlock ( Data, QuadwordCount ); }
		inline static void DMA_ReadBlock ( u64* Data, u32 QuadwordCount ) { _VU0->DMA_ReadBlock ( Data, QuadwordCount ); }
		
		inline static bool DMA_Write_Ready () { return _VU0->DMA_Write_Ready (); }
		inline static bool DMA_Read_Ready () { return _VU0->DMA_Read_Ready (); }
	};
	
	
	class VU1
	{
	public:
	
		static VU *_VU1;
		
		VU VU1;
		
		inline void Start ()
		{
			_VU1 = & VU1;
			
			VU1.Start ( 1 );
			//VU1.Number = 1;
		}
		
		inline void Reset () { VU1.Reset (); }
		
		inline static u64 Read ( u32 Address, u64 Mask ) { _VU1->Read ( Address, Mask ); }
		inline static void Write ( u32 Address, u64 Data, u64 Mask ) { _VU1->Write( Address, Data, Mask ); }
		
		inline static void DMA_WriteBlock ( u64* Data, u32 QuadwordCount ) { _VU1->DMA_WriteBlock ( Data, QuadwordCount ); }
		inline static void DMA_ReadBlock ( u64* Data, u32 QuadwordCount ) { _VU1->DMA_ReadBlock ( Data, QuadwordCount ); }
		
		inline static bool DMA_Write_Ready () { return _VU1->DMA_Write_Ready (); }
		inline static bool DMA_Read_Ready () { return _VU1->DMA_Read_Ready (); }
	};
}




#endif

