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


#ifndef _PS2DATABUS_H_
#define _PS2DATABUS_H_

#include "types.h"


#include "PS2_Dma.h"
#include "PS2_Gpu.h"
#include "PS2_Intc.h"
//#include "PS2_MDEC.h"
//#include "PS2_PIO.h"
#include "PS2_SIO.h"
#include "PS2_Timer.h"

//#include "R5900DebugPrint.h"

#ifdef ENABLE_GUI_DEBUGGER
#include "DebugMemoryViewer.h"
#endif

namespace Playstation2
{

	//class Dma;
	//class MDEC;

	class DataBus
	{
	
#ifdef ENABLE_GUI_DEBUGGER
		static Debug::Log debug;
#endif

		
	public:
	
		static DataBus *_BUS;
		
		
		// 0x1f80 0000 is where the hardware mapped registers start at for PS1
		// 0x1000 0000 is where the hardware mapped registers start at for PS2
		static const u32 HWRegisters_Start = 0x10000000;
		
		
		// cycle that the next event will happen at for this device
		// this will not actually be used here
		u64 NextEvent_Cycle;
		
		
		// PS2 Bus speed can be 147.456 MHZ in earlier models, and 149.5 MHZ in later models
		// R5900 runs at twice that speed
		// PS1 Bus speed when used with PS2 is 1/4 of that, unless in PS1 configuration
		static const u64 c_ClockSpeed1 = 147456000;
		static const u64 c_ClockSpeed2 = 149500000;


		// 0x0000 0000 is physical address of where the regular ram starts at
		static const u32 MainMemory_Start = 0x00000000;
		static const u32 MainMemory_Size = 0x2000000;	// 32 MB
		static const u32 MainMemory_Mask = MainMemory_Size - 1;
		
		// 0x1fc0 0000 is physical address of where BIOS starts at
		static const u32 BIOS_Start = 0x1fc00000;
		static const u32 BIOS_Size = 0x400000;	// 4MB
		static const u32 BIOS_Mask = BIOS_Size - 1;

		// 0x70000000 is physical address of where scratch pad starts at
		static const u32 ScratchPad_Start = 0x70000000;
		static const u32 ScratchPad_Size = 0x4000;	// 16 KB
		static const u32 ScratchPad_Mask = ScratchPad_Size - 1;

		// 0x11000000 is physical address of where micromem starts at
		static const u32 MicroMem0_Start = 0x11000000;
		static const u32 MicroMem0_Size = 0x1000;	// 4 KB
		static const u32 MicroMem0_Mask = MicroMem0_Size - 1;
		
		static const u32 VuMem0_Start = 0x11004000;
		static const u32 VuMem0_Size = 0x1000;	// 4 KB
		static const u32 VuMem0_Mask = VuMem0_Size - 1;

		
		static const u32 MicroMem1_Start = 0x11008000;
		static const u32 MicroMem1_Size = 0x4000;	// 16 KB
		static const u32 MicroMem1_Mask = MicroMem1_Size - 1;
		
		static const u32 VuMem1_Start = 0x1100c000;
		static const u32 VuMem1_Size = 0x4000;	// 16 KB
		static const u32 VuMem1_Mask = VuMem1_Size - 1;


		static const u32 DirectCacheMem_Start = 0xffff8000;
		static const u32 DirectCacheMem_Size = 0x8000;	// 32 KB or 16 KB or 8 KB ?? (trying 32KB first)
		static const u32 DirectCacheMem_Mask = DirectCacheMem_Size - 1;
		
		
		// reading from BIOS takes 24 cycles (8-bit bus), executing an instruction would be an extra cycle
		static const int c_iBIOS_Read_Latency = 24;
		
		// reading from RAM takes 5 cycles, but if executing an instruction, that takes an additional 1 cycle to execute
		static const int c_iRAM_Read_Latency = 5;
		
		// reading from hardware registers takes around 3 cycles or less
		static const int c_iReg_Read_Latency = 3;
		
		// bus registers
		//u32 RamSize, CD_Delay, DMA_Delay;
		
		
		
		// lookup table for bus
		typedef u64 (*PS2_BusInterface_Read) ( u32 Address, u64 Mask );
		typedef void (*PS2_BusInterface_Write) ( u32 Address, u64 Data, u64 Mask );
		
		// I'll treat this one extra special
		typedef u64* (*PS2_BusInterface_Read128) ( u32 Address, u64 Mask );
		typedef void (*PS2_BusInterface_Write128) ( u32 Address, u64* Data, u64 Mask );
		
		static const u32 c_LUT_Bus_Size = 0x400;
		static PS2_BusInterface_Read LUT_BusRead [ c_LUT_Bus_Size ];
		static PS2_BusInterface_Write LUT_BusWrite [ c_LUT_Bus_Size ];

		static PS2_BusInterface_Read128 LUT_BusRead128 [ c_LUT_Bus_Size ];
		static PS2_BusInterface_Write128 LUT_BusWrite128 [ c_LUT_Bus_Size ];
		
		// connect device with bus
		void Init_ConnectDevice ( void );
		void ConnectDevice_Read ( u32 AddressStart, PS2_BusInterface_Read CallbackFunction );
		void ConnectDevice_Write ( u32 AddressStart, PS2_BusInterface_Write CallbackFunction );
		
		void ConnectDevice_Read128 ( u32 AddressStart, PS2_BusInterface_Read128 CallbackFunction );
		void ConnectDevice_Write128 ( u32 AddressStart, PS2_BusInterface_Write128 CallbackFunction );
		
		
		// lookup table for hardware registers
		static const u32 c_LUT_Reg_Size = 0x400;
		static PS2_BusInterface_Read LUT_RegRead [ c_LUT_Reg_Size ];
		static PS2_BusInterface_Write LUT_RegWrite [ c_LUT_Reg_Size ];
		
		// connect registers with bus
		void Init_ConnectRegs ( void );
		void ConnectRegs_Read ( u32 AddressStart, PS2_BusInterface_Read CallbackFunction );
		void ConnectRegs_Write ( u32 AddressStart, PS2_BusInterface_Write CallbackFunction );
		
		
		static u64 InvalidAddress_Read ( u32 Address, u64 Mask );
		static void InvalidAddress_Write ( u32 Address, u64 Data, u64 Mask );
		

		static u64 Memory_Read ( u32 Address, u64 Mask );
		static void Memory_Write ( u32 Address, u64 Data, u64 Mask );
		
		static u64 BIOS_Read ( u32 Address, u64 Mask );
		
		static u64 ScratchPad_Read ( u32 Address, u64 Mask );
		static void ScratchPad_Write ( u32 Address, u64 Data, u64 Mask );
		
		static u64 Device_Read ( u32 Address, u64 Mask );
		static void Device_Write ( u32 Address, u64 Data, u64 Mask );
		
		static u64 VuMem_Read ( u32 Address, u64 Mask );
		static void VuMem_Write ( u32 Address, u64 Data, u64 Mask );
		
		static u64 DirectCacheMem_Read ( u32 Address, u64 Mask );
		static void DirectCacheMem_Write ( u32 Address, u64 Data, u64 Mask );
		
		//static u32* RamSize_Read ( u32 Address, u64 Mask );
		//static void RamSize_Write ( u32 Address, u32 Data, u64 Mask );
		//static u32* PIO_Read ( u32 Address );
		//static void PIO_Write ( u32 Address, u32* Data, u32 Mask );
		
		/*
		template<const int c_iTransferWidth>
		class BusInterface_t
		{
			PS2_BusInterface_Read LUT_BusRead [ c_LUT_Bus_Size ];
			PS2_BusInterface_Write LUT_BusWrite [ c_LUT_Bus_Size ];
			
			void Init_ConnectDevice ( void );
			void ConnectDevice_Read ( u32 AddressStart, PS2_BusInterface_Read CallbackFunction );
			void ConnectDevice_Write ( u32 AddressStart, PS2_BusInterface_Write CallbackFunction );
			
			static u32 InvalidAddress_Read ( u32 Address );
			static void InvalidAddress_Write ( u32 Address, u32 Data, u32 Mask );
			
			static u32 Memory_Read ( u32 Address );
			static void Memory_Write ( u32 Address, u32 Data, u32 Mask );
			
			static u32 BIOS_Read ( u32 Address );
			
			static u32 ScratchPad_Read ( u32 Address );
			static void ScratchPad_Write ( u32 Address, u32 Data, u32 Mask );
		};
		*/
		
		
		// Need pointer into main memory
		union MemoryPtr_Format
		{
			u8* b8;
			u16* b16;
			u32* b32;
			u64* b64;
			s8* sb8;
			s16* sb16;
			s32* sb32;
			s64* sb64;
			
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
			u64 b64 [ MainMemory_Size / sizeof ( u64 ) ];
			s8 sb8 [ MainMemory_Size ];
			s16 sb16 [ MainMemory_Size / sizeof ( s16 ) ];
			s32 sb32 [ MainMemory_Size / sizeof ( s32 ) ];
			s64 sb64 [ MainMemory_Size / sizeof ( s64 ) ];
		};
		
		union _BIOS
		{
			u8 b8 [ BIOS_Size ];
			u16 b16 [ BIOS_Size / sizeof ( u16 ) ];
			u32 b32 [ BIOS_Size / sizeof ( u32 ) ];
			u64 b64 [ BIOS_Size / sizeof ( u64 ) ];
			s8 sb8 [ BIOS_Size ];
			s16 sb16 [ BIOS_Size / sizeof ( s16 ) ];
			s32 sb32 [ BIOS_Size / sizeof ( s32 ) ];
			s64 sb64 [ BIOS_Size / sizeof ( s64 ) ];
		};

		union _ScratchPad
		{
			u8 b8 [ ScratchPad_Size ];
			u16 b16 [ ScratchPad_Size / sizeof ( u16 ) ];
			u32 b32 [ ScratchPad_Size / sizeof ( u32 ) ];
			u64 b64 [ ScratchPad_Size / sizeof ( u64 ) ];
			s8 sb8 [ ScratchPad_Size ];
			s16 sb16 [ ScratchPad_Size / sizeof ( s16 ) ];
			s32 sb32 [ ScratchPad_Size / sizeof ( s32 ) ];
			s64 sb64 [ ScratchPad_Size / sizeof ( s64 ) ];
		};
		
		union _MicroMem0
		{
			u8 b8 [ MicroMem0_Size ];
			u16 b16 [ MicroMem0_Size / sizeof ( u16 ) ];
			u32 b32 [ MicroMem0_Size / sizeof ( u32 ) ];
			u64 b64 [ MicroMem0_Size / sizeof ( u64 ) ];
			s8 sb8 [ MicroMem0_Size ];
			s16 sb16 [ MicroMem0_Size / sizeof ( s16 ) ];
			s32 sb32 [ MicroMem0_Size / sizeof ( s32 ) ];
			s64 sb64 [ MicroMem0_Size / sizeof ( s64 ) ];
		};


		union _MicroMem1
		{
			u8 b8 [ MicroMem1_Size ];
			u16 b16 [ MicroMem1_Size / sizeof ( u16 ) ];
			u32 b32 [ MicroMem1_Size / sizeof ( u32 ) ];
			u64 b64 [ MicroMem1_Size / sizeof ( u64 ) ];
			s8 sb8 [ MicroMem1_Size ];
			s16 sb16 [ MicroMem1_Size / sizeof ( s16 ) ];
			s32 sb32 [ MicroMem1_Size / sizeof ( s32 ) ];
			s64 sb64 [ MicroMem1_Size / sizeof ( s64 ) ];
		};

		
		union _DirectCacheMem
		{
			u8 b8 [ DirectCacheMem_Size ];
			u16 b16 [ DirectCacheMem_Size / sizeof ( u16 ) ];
			u32 b32 [ DirectCacheMem_Size / sizeof ( u32 ) ];
			u64 b64 [ DirectCacheMem_Size / sizeof ( u64 ) ];
			s8 sb8 [ DirectCacheMem_Size ];
			s16 sb16 [ DirectCacheMem_Size / sizeof ( s16 ) ];
			s32 sb32 [ DirectCacheMem_Size / sizeof ( s32 ) ];
			s64 sb64 [ DirectCacheMem_Size / sizeof ( s64 ) ];
		};

		
		
		_MainMemory MainMemory;
		_BIOS BIOS;
		_ScratchPad ScratchPad;
		
		// micromem is the vu program code memory, where vumem is the vu data memory
		//_MicroMem0 MicroMem0;
		//_MicroMem0 VuMem0;
		//_MicroMem1 MicroMem1;
		//_MicroMem1 VuMem1;
		
		static u64 *MicroMem0;
		static u64 *VuMem0;
		static u64 *MicroMem1;
		static u64 *VuMem1;
		
		// temp buffer used for 128-bit reads when needed
		u32 TempBuffer [ 4 ];
		
		_DirectCacheMem DirectCacheMem;
		
		
		// put sbus/mch data here for now
		u32 lSBUS_F200, lSBUS_F210, lSBUS_F220, lSBUS_F230, lSBUS_F240, lSBUS_F260;
		u32 lMCH_RICM, lMCH_DRD, lMCH_F480, lMCH_F490;
		u32 RDRAM_SDEVID;
		
		static const u32 c_MCM_RICM = 0x1000f430;
		static const u32 c_MCM_DRD = 0x1000f440;
		
		// ??? PS1 CTRL ???
		u32 lPS1_CTRL_3210;
		u32 lINTC;
		
		u32 lREG_1a6;
		
		// test dmac enable
		u32 lDMAC_ENABLE;
		
		// need a reference to all the components connected to the data bus
		//static u32 *DebugPC;	// to match up debug info
		
		//static Dma* DMA_Device;
		//static GPU* GPU_Device;
		//static Intc* INTC_Device;
		//static MDEC* MDEC_Device;
		//static PIO* PIO_Device;
		//static SIO* SIO_Device;
		//static Timers* Timers_Device;
		
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
		
		u64 Dummy128 [ 2 ];
		
		bool isReady ();
		void ReserveBus ( u64 Cycles );
		void ReserveBus_Latency ();
		
		// constructor - needs to connect everything with the bus
		DataBus ();
		void ConnectDevices (
			//Dma* _Dma_Device,
			//CD* _CD_Device,
			//GPU* _GPU_Device,
			//Intc* _INTC_Device,
			//MDEC* _MDEC_Device,
			//PIO* _PIO_Device,
			//SIO* _SIO_Device,
			//SPU* _SPU_Device,
			//Timers* _Timers_Device
		);
		
		// destructor
		~DataBus ();
		
		void Reset ();
		
		void Start ();
		
		
		inline static void Connect_VuMem ( u64* _MicroMem0, u64* _VuMem0, u64* _MicroMem1, u64* _VuMem1 )
		{
			MicroMem0 = _MicroMem0;
			VuMem0 = _VuMem0;
			MicroMem1 = _MicroMem1;
			VuMem1 = _VuMem1;
		}
		
		// request or accept access to the PS1 data bus (32-bit bus)
		// burst transfers possible
		//void Request ( u32 RequestingDeviceIndex, u32 RequestType );
		//bool Accept ( u32 RequestingDeviceIndex );

		// read or write data on PS1 data bus after request accepted
		// returns false when bus could not process request because it was either busy or dma did not release bus
		u64 Read ( u32 Address, u64 Mask );
		void Write ( u32 Address, u64 Data, u64 Mask );
		
		//u64* Read128 ( u32 Address, u64 Mask );
		//void Write128 ( u32 Address, u64* Data, u64 Mask );
		
		
		
		void IRead ( u32* DataOut, u32 Address, u32 ReadType );
		
		void Run ();
		
		static u64 EndianTemp [ 2 ];
		
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
		
#ifdef ENABLE_GUI_DEBUGGER
		// object debug stuff
		// Enable/Disable debug window for object
		// Windows API specific code is in here
		static bool DebugWindow_Enabled;
		static WindowClass::Window *DebugWindow;
		static Debug_MemoryViewer *MemoryViewer;
		static void DebugWindow_Enable ();
		static void DebugWindow_Disable ();
		static void DebugWindow_Update ();
#endif
	};
}

#endif



