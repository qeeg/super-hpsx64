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


#ifndef _PS1DATABUS_H_
#define _PS1DATABUS_H_

#include "types.h"


#include "PS1_Dma.h"
#include "PS1_CD.h"
#include "PS1_Gpu.h"
#include "PS1_Intc.h"
#include "PS1_MDEC.h"
#include "PS1_PIO.h"
#include "PS1_SIO.h"
#include "PS1_SPU.h"
#include "PS1_Timer.h"

#ifdef PS2_COMPILE
#include "CDvd.h"
#include "PS1_SPU2.h"
#endif

#include "R3000ADebugPrint.h"

#include "DebugMemoryViewer.h"

namespace Playstation1
{

	class Dma;
	class MDEC;

	class DataBus
	{
	
		static Debug::Log debug;
		
	public:
	
		static DataBus *_BUS;
		
		
		// 0x1f80 0000 is where the hardware mapped registers start at
		static const u32 HWRegisters_Start = 0x1f800000;
		
		
		
		// cycle that the next event will happen at for this device
		// this will not actually be used here
		u64 NextEvent_Cycle;


		// 0x0000 0000 is physical address of where the regular ram starts at
		static const u32 MainMemory_Start = 0x00000000;
		static const u32 MainMemory_Size = 0x200000;	// 2 MB
		static const u32 MainMemory_Mask = MainMemory_Size - 1;
		
		// 0x1fc0 0000 is physical address of where BIOS starts at
		static const u32 BIOS_Start = 0x1fc00000;
		
#ifdef PS2_COMPILE
		// bios for R3000A on a PS2 is 4MB
		static const u32 BIOS_Size = 0x400000;	// 4MB
#else
		static const u32 BIOS_Size = 0x80000;	// 512 KB
#endif

		static const u32 BIOS_Mask = BIOS_Size - 1;
		
		// reading from BIOS takes 24 cycles (8-bit bus), executing an instruction would be an extra cycle
		static const int c_iBIOS_Read_Latency = 24;
		
		// reading from RAM takes 5 cycles, but if executing an instruction, that takes an additional 1 cycle to execute
		static const int c_iRAM_Read_Latency = 5;
		
		// reading from hardware registers takes around 3 cycles or less
		static const int c_iReg_Read_Latency = 3;
		
		// bus registers
		u32 RamSize, CD_Delay, DMA_Delay;
		
		
		// reg cache
		u32 RegCache_0x1f8010 [ 256 >> 2 ];
		
		
		// lookup table for bus
		typedef u32 (*PS1_BusInterface_Read) ( u32 Address );
		typedef void (*PS1_BusInterface_Write) ( u32 Address, u32 Data, u32 Mask );
		static const u32 c_LUT_Bus_Size = 0x400;
		static PS1_BusInterface_Read LUT_BusRead [ c_LUT_Bus_Size ];
		static PS1_BusInterface_Write LUT_BusWrite [ c_LUT_Bus_Size ];
		
		// connect device with bus
		void Init_ConnectDevice ( void );
		void ConnectDevice_Read ( u32 AddressStart, PS1_BusInterface_Read CallbackFunction );
		void ConnectDevice_Write ( u32 AddressStart, PS1_BusInterface_Write CallbackFunction );
		
		// lookup table for hardware registers
		static const u32 c_LUT_Reg_Size = 0x400;
		static PS1_BusInterface_Read LUT_RegRead [ c_LUT_Reg_Size ];
		static PS1_BusInterface_Write LUT_RegWrite [ c_LUT_Reg_Size ];
		
		// connect registers with bus
		void Init_ConnectRegs ( void );
		void ConnectRegs_Read ( u32 AddressStart, PS1_BusInterface_Read CallbackFunction );
		void ConnectRegs_Write ( u32 AddressStart, PS1_BusInterface_Write CallbackFunction );
		
		
		static u32 InvalidAddress_Read ( u32 Address );
		static void InvalidAddress_Write ( u32 Address, u32 Data, u32 Mask );
		
		static u32 RamSize_Read ( u32 Address );
		static void RamSize_Write ( u32 Address, u32 Data, u32 Mask );

		static u32 Memory_Read ( u32 Address );
		static void Memory_Write ( u32 Address, u32 Data, u32 Mask );
		static u32 BIOS_Read ( u32 Address );
		static u32 PIO_Read ( u32 Address );
		static void PIO_Write ( u32 Address, u32 Data, u32 Mask );
		static u32 Device_Read ( u32 Address );
		static void Device_Write ( u32 Address, u32 Data, u32 Mask );
		

#ifdef PS2_COMPILE
		
		static u32 DEV5_Read ( u32 Address );
		static void DEV5_Write ( u32 Address, u32 Data, u32 Mask );
		
		static u32 SBUS_Read ( u32 Address );
		static void SBUS_Write ( u32 Address, u32 Data, u32 Mask );

		u16 SPU2_Temp [ 2048 ];
		//static u32 SPU2_Read ( u32 Address );
		//static void SPU2_Write ( u32 Address, u32 Data, u32 Mask );

		// reg cache
		u32 RegCache_0x1f8014 [ 256 >> 2 ];
#endif
		
		
		// Need pointer into main memory
		union MemoryPtr_Format
		{
			u8* b8;
			u16* b16;
			u32* b32;
			s8* sb8;
			s16* sb16;
			s32* sb32;
			
			MemoryPtr_Format ( u32* _ptr ) { b32 = _ptr; }
		};
		
		// we need a reference to all the components connected to the data bus?
		//static MemoryPtr_Format MainMemory;
		//static MemoryPtr_Format BIOS;
		
		union _MainMemory
		{
			u8 b8 [ MainMemory_Size ];
			u16 b16 [ MainMemory_Size / sizeof ( u16 ) ];
			u32 b32 [ MainMemory_Size / sizeof ( u32 ) ];
			s8 sb8 [ MainMemory_Size ];
			s16 sb16 [ MainMemory_Size / sizeof ( s16 ) ];
			s32 sb32 [ MainMemory_Size / sizeof ( s32 ) ];
		};
		
		union _BIOS
		{
			u8 b8 [ BIOS_Size ];
			u16 b16 [ BIOS_Size / sizeof ( u16 ) ];
			u32 b32 [ BIOS_Size / sizeof ( u32 ) ];
			s8 sb8 [ BIOS_Size ];
			s16 sb16 [ BIOS_Size / sizeof ( s16 ) ];
			s32 sb32 [ BIOS_Size / sizeof ( s32 ) ];
		};
		
		_MainMemory MainMemory;
		_BIOS BIOS;
		
		
		
		// need a reference to all the components connected to the data bus
		static u32 *DebugPC;	// to match up debug info
		static Dma* DMA_Device;
		static CD* CD_Device;
		static GPU* GPU_Device;
		static Intc* INTC_Device;
		static MDEC* MDEC_Device;
		static PIO* PIO_Device;
		static SIO* SIO_Device;
		static SPU* SPU_Device;
		static Timers* Timers_Device;
		
		//R3000A::Cpu* cpu;
		
		//static const u32 Read_BusyCycles = 1;
		//static const u32 Write_BusyCycles = 1;
		//static const u32 Burst4_BusyCycles = 4;
		
		// says whether dma has accessed bus for a cycle
		// true if we are ok to access the bus
		//bool AccessOK;
		
		u32 BusyCycles;
		
		u64 BusyUntil_Cycle;
		
		// this can be static since it is read right after it is written to
		static u32 Latency;
		
		bool isReady ();
		void ReserveBus ( u64 Cycles );
		void ReserveBus_Latency ();
		
		// constructor - needs to connect everything with the bus
		DataBus ();
		void ConnectDevices ( Dma* _Dma_Device, CD* _CD_Device, GPU* _GPU_Device, Intc* _INTC_Device, MDEC* _MDEC_Device, PIO* _PIO_Device,
							SIO* _SIO_Device, SPU* _SPU_Device, Timers* _Timers_Device );
		
		// destructor
		~DataBus ();
		
		void Reset ();
		
		void Start ();
		
		// request or accept access to the PS1 data bus (32-bit bus)
		// burst transfers possible
		//void Request ( u32 RequestingDeviceIndex, u32 RequestType );
		//bool Accept ( u32 RequestingDeviceIndex );

		// read or write data on PS1 data bus after request accepted
		// returns false when bus could not process request because it was either busy or dma did not release bus
		u32 Read ( u32 Address );
		void Write ( u32 Data, u32 Address, u32 Mask );
		void IRead ( u32* DataOut, u32 Address, u32 ReadType );
		
		void Run ();



#ifdef PS2_COMPILE
		u32 EE_Read ( u32 Address, u32 Mask );
		void EE_Write ( u32 Address, u32 Data, u32 Mask );
#endif
		
		
		
		void* GetPointer ( u32 Address );


		enum { RW_8, RW_16, RW_32, RW_BURST4 };	// values for Read/Write Type
		


		
		/////////////////////////////////////////////
		// For Debugging
		
		void SaveBIOSToFile ( u32 Address, u32 NumberOfInstructions );
		void SaveRAMToFile ( u32 Address, u32 NumberOfInstructions );
		
		
		////////////////////////////////
		// Debug Info
		static u32* _DebugPC;
		static u64* _DebugCycleCount;
		
		// object debug stuff
		// Enable/Disable debug window for object
		// Windows API specific code is in here
		static bool DebugWindow_Enabled;
		static WindowClass::Window *DebugWindow;
		static Debug_MemoryViewer *MemoryViewer;
		static void DebugWindow_Enable ();
		static void DebugWindow_Disable ();
		static void DebugWindow_Update ();
	};
}

#endif



