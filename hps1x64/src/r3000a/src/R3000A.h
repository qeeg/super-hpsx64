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


#ifndef _R3000A_H_

#define _R3000A_H_

#include "Debug.h"


#include <sstream>

#include "WinApiHandler.h"

#include "GenericDataPort.h"

#include "types.h"

#include "GNUAsmUtility_x64.h"

#include "R3000A_Instruction.h"
#include "R3000A_Execute.h"
#include "R3000A_ICache.h"
#include "R3000A_COP2.h"

#include "PS1DataBus.h"


// Windows API specific code is in this header file
#include "DebugValueList.h"
//#include "InfiniteListView.h"
#include "DisassemblyViewer.h"

#include "BreakpointWindow.h"

using namespace std;
using namespace x64Asm::Utilities;
using namespace R3000A::Instruction;

class Debug_MemoryViewer;


namespace Playstation1
{
	class DataBus;
}


namespace R3000A
{

	//class COP2_Device;

	struct EntryLo0_Reg
	{
		union
		{
			unsigned long l;

			struct
			{
				// bit 0 - Global - r/w
				unsigned long G : 1;
				
				// bit 1 - Valid - set to 1 if the TLB entry is enabled - r/w
				unsigned long V : 1;
				
				// bit 2 - Dirty - set to 1 if writable - r/w
				unsigned long D : 1;
				
				// bits 3-5 - TLB page coherency - 2: uncached, 3: Cached, 7: uncached accelerated - r/w
				unsigned long C : 3;
				
				// bits 6-25 - Page frame number (the upper bits of the physical address) - r/w
				unsigned long PFN : 20;
				
				// bits 26-30 - zero
				unsigned long zero : 5;
				
				// bit 31 - Memory type - 0: Main Memory, 1: Scratchpad RAM - r/w - only for EntryLo0
				unsigned long S : 1;
			};

		};
	};
	
	struct EntryLo1_Reg
	{
		union
		{
			unsigned long l;
			
			struct
			{
				// bit 0 - Global - r/w
				unsigned long G : 1;
				
				// bit 1 - Valid - set to 1 if the TLB entry is enabled - r/w
				unsigned long V : 1;
				
				// bit 2 - Dirty - set to 1 if writable - r/w
				unsigned long D : 1;
				
				// bits 3-5 - TLB page coherency - 2: uncached, 3: Cached, 7: uncached accelerated - r/w
				unsigned long C : 3;
				
				// bits 6-25 - Page frame number (the upper bits of the physical address) - r/w
				unsigned long PFN : 20;
			};
		};
	};

	struct Context_Reg
	{
		union
		{
			unsigned long l;
			
			struct
			{
				// bits 0-3 - zero
				unsigned long zero : 4;
				
				// bits 4-22 - Virtual page address that caused TLB miss
				unsigned long BadVPN2 : 19;
				
				// bits 23-31 - Page table address
				unsigned long PTEBase : 9;
			};
		};
	};
	
	struct PageMask_Reg
	{
		union
		{
			unsigned long l;
			
			struct
			{
				// bits 0-12 - zero
				unsigned long zero : 13;
				
				// bits 13-24 - Page size comparison mask - 0: 4KB, 3: 16KB, 15: 64KB, 63: 256KB, 65536: 1MB, 256K-1: 4MB, 1M-1: 16MB - r/w
				// start value:
				unsigned long MASK : 12;
			};
		};
	};

	struct EntryHi_Reg
	{
		union
		{
			unsigned long l;
			
			struct
			{
				// bits 0-7 - ASID - Address space ID - r/w
				// initial value:
				unsigned long ASID : 8;
				
				// bits 8-12 - zero
				unsigned long zero : 5;
				
				// bits 13-31 - VPN2 - Virtual page number divided by two - r/w
				// initial value:
				unsigned long VPN2 : 19;
			};
		};
	};

	struct Status_Reg
	{
		union
		{
			unsigned long Value;
			
			struct
			{
				union
				{
					struct
					{
						// bit 0 - Interrupt enable flag - 0: all interrupts disabled; 1: interrupts enabled
						unsigned char IEc : 1;
						
						// bit 1 - Kernel/User mode - 0: user mode; 1: kernel level priveleges
						// in kernel mode the program can access the entire address space and use priveleged (COP0) instructions
						// in user mode software is restricted to program addresses 0x0000 0000 to 0x7fff ffff
						// also in user mode use of priveleged (COP0) instructions can be denied
						unsigned char KUc : 1;
						
						// bit 2 - "IE previous"
						unsigned char IEp : 1;
						
						// bit 3 - "KU previous"
						unsigned char KUp : 1;
						
						// bit 4 - "IE old"
						unsigned char IEo : 1;
						
						// bit 5 - "KU old"
						unsigned char KUo : 1;
						
						// bits 6-7 - zero
						unsigned char zero0 : 2;
					};
					
					unsigned char b0;
				};
				
				// bits 8-15 - Interrupt mask - 0: disables interrupts, 1: enables interrupts -  r/w
				// bit 10 - timer/counter - or probably all external interrupts??
				unsigned char IM;

				union
				{
				
					struct
					{
						// bit 16 - Isolate data cache - IsC - r/w
						// if set, makes all loads and stores access only the data cache and never memory
						// if set, partial word stores invalidate the cache entry and not even uncached access is seen on the bus at all
						unsigned short IsC : 1;
						
						// bit 17 - Swap Caches - SwC - r/w
						// if set, reverses the roles of I-Cache and D-Cache, so that software can access and invalidate I-cache entries
						unsigned short SwC : 1;
						
						// bit 18 - PZ - r/w
						// when set, cache parity bits are written as zero and not checked
						unsigned short PZ : 1;
						
						// bit 19 - CM - read only
						// shows the result of the last load operation with the d-cache isolated
						unsigned short CM : 1;
						
						// bit 20 - PE - read only
						// set if a cache parity error has occurred
						unsigned short PE : 1;
						
						// bit 21 - TS - read only
						// set if an address matches two TLB entries simultaneously
						unsigned short TS : 1;
						
						// bit 22 - Controls address of the TLB Refill or general exception vectors - r/w
						// start value: 1
						unsigned short BEV : 1;
						
						// bit 23-24 - zero
						unsigned short zero2 : 2;
						
						// bits 25 - RE - r/w
						// if processor was configured at reset time to have endianness (byte order) reversed, then this bit is set
						unsigned short RE : 1;
						
						// bits 26-27 - zero
						unsigned short zero3 : 2;
						
						// bits 28-31 - Usability of each coprocessor unit - 0: Unusable, 1: Usable - r/w
						unsigned short CU0 : 1;
						unsigned short CU1 : 1;
						unsigned short CU2 : 1;	// this bit MUST be set before you can use COP2 (GTE)
						unsigned short CU3 : 1;
					};
					
					unsigned short h1;
				};
			};
		};
	};

	struct Cause_Reg
	{
		union
		{
			unsigned long l;
			
			struct
			{
				// bits 0-1 - zero
				unsigned long zero0 : 2;
				
				// bits 2-6 - Exception code - 0: Interrupt, 1: TLB Modified, 2: TLB Refill, 3: TLB Refill (store), 4: Address Error, 5: Address Error (store)
				// 6: Bus Error (instruction), 7: Bus Error (data), 8: System Call, 9: Breakpoint, 10: Reserved instruction, 11: Coprocessor Unusable
				// 12: Overflow, 13: Trap
				// r/w
				unsigned long ExcCode : 5;
				
				// bits 7-9 - zero on R5900
				// bit 7 - zero on R3000A
				// on R3000A, just bit 7 is zero
				unsigned long zero1 : 1;
				
				// bits 8-9 - software writable interrupt pending bits - r/w
				// bits 10-15 - hardware interrupt pending bits
				unsigned long IP : 8;
				
				/*
				unsigned long IP1 : 1;
				
				
				// bit 10 - Set when Int[1] interrupt is pending
				unsigned long IP2 : 1;
				
				// bit 11 - Set when Int[0] interrupt is pending
				unsigned long IP3 : 1;
				
				// bits 12-14 - zero
				unsigned long zero2 : 3;
				
				// bit 15 - Set when a timer interrupt is pending
				unsigned long IP7 : 1;
				*/

				// bits 16-27 are zero on R3000A
				// bits 16-18 on R5900 - Exception codes for level 2 exceptions - 0: Reset, 1: NMI, 2: Performance Counter, 3: Debug
				//unsigned long EXC2 : 3;
				// bits 19-27 on R5900 - zero
				//unsigned long zero3 : 9;
				unsigned long zero3 : 12;
				
				// bits 28-29 - Coprocessor number when a coprocessor unusable exception is taken
				unsigned long CE : 2;
				
				// bit 30 is zero on R3000A
				// bit 30 - Set when level 2 exception occurs in a branch delay slot
				//unsigned long BD2 : 1;
				unsigned long zero4 : 1;
				
				// bit 31 - Set when a level 1 exception occurs in a branch delay slot
				unsigned long BD : 1;
			};
		};
	};

	struct PRId_Reg
	{
		union
		{
			unsigned long l;
			
			struct
			{
				// bits 0-7 - Revision number - read only
				// start value: revision number -> set to zero
				unsigned long Rev : 8;
				
				// bits 8-15 - Implementation number - read only
				// start value: 0x2e on R5900
				// start value: 3 on R3000A
				unsigned long Imp : 8;

				// bits 16-31 - reserved
				unsigned long reserved0 : 16;
			};
		};
	};

	struct Config_Reg
	{
		union
		{
			unsigned long l;
			
			struct
			{
				// bits 0-2 - kseg0 cache mode - 0: cached w/o write-back or -allocate, 2: Uncached, 3: Cached, 7: Uncached accelerated
				// start value:
				unsigned long K0 : 3;
				
				// bits 3-5 - zero
				unsigned long zero0 : 3;
				
				// bits 6-8 - Data cache size - read only - 0: 4KB, 1: 8KB, 2: 16KB
				// start value: 1
				unsigned long DC : 3;
				
				// bits 9-11 - Instruction cache size - read only - 0: 4KB, 1: 8KB, 2: 16KB
				// start value: 2
				unsigned long IC : 3;
				
				// bit 12 - Setting this bit to 1 enables branch prediction
				// start value: 0
				unsigned long BPE : 1;
				
				// bit 13 - Setting this bit to 1 enables non-blocking load
				// start value: 0
				unsigned long NBE : 1;
				
				// bits 14-15 - zero
				unsigned long zero1 : 2;
				
				// bit 16 - Setting this bit to 1 enables the data cache
				// start value: 0
				unsigned long DCE : 1;
				
				// bit 17 - Setting this bit to 1 enables the instruction cache
				// start value: 0
				unsigned long ICE : 1;
				
				// bit 18 - Setting this bit to 1 enables the pipeline parallel issue
				// start value: 0
				unsigned long DIE : 1;
				
				// bits 19-27 - zero
				unsigned long zero2 : 9;
				
				// bits 28-30 - Bus clock ratio - 0: processor clock frequency divided by 2
				// start value: 0
				unsigned long EC : 3;
			};
		};
	};

	struct CPR0_Regs
	{
	
		union
		{

			// there are 32 COP0 control registers
			unsigned long Regs [ 32 ];
			
			struct
			{
			
				// Register #0 - "Index"
				// Index that specifies TLB entry for reading or writing - MMU
				// Index - bits 0-5 - r/w
				// start value:
				unsigned long Index;
				
				// Register #1 - "Random" - Read Only
				// Desc: Index that specifies TLB entry for the TLBWR instruction
				// Purpose: MMU
				// Random - bits 0-5 - read only
				// start value:
				unsigned long Random;
				
				// Register #2 - "EntryLo0" - Lower part of the TLB entry - MMU
				EntryLo0_Reg EntryLo0;
				
				// Register #3 - "EntryLo1" - Lower part of the TLB entry - MMU
				// On R3000A this is actually "BPC" - rw - breakpoint on execute
				EntryLo1_Reg EntryLo1;
				
				// Register #4 - "Context" - TLB miss handling information
				Context_Reg Context;
				
				// Register #5 - "PageMask" - Page size comparison mask
				// On R3000A this is actually "BDA" - rw - breakpoint on data access
				PageMask_Reg PageMask;
				
				// Register #6 - "Wired" - The number of wired TLB entries
				// bits 0-5 - Wired
				// On R3000A this may be "PIDMASK"
				unsigned long Wired;
				
				// Register #7 - Reserved
				// On R3000A this is DCIC - rw - breakpoint control
				unsigned long Reserved0;
				
				// Register #8 - "BadVAddr" - Virtual address that causes an error - bits 0-31
				unsigned long BadVAddr;
				
				// Register #9 - "Count" - Timer count value - incremented every clock cycle - r/w
				// On R3000A this is "BDAM" - rw - data access breakpoint mask
				unsigned long Count;
				
				// Register #10 - "EntryHi" - upper parts of a TLB entry
				EntryHi_Reg EntryHi;
				
				// Register #11 - "Compare" - Timer stable value - when the "Count" register reaches this value an interrupt occurs - r/w
				// On R3000A this is "BPCM" - rw - execute breakpoint mask
				unsigned long Compare;
				
				// Register #12 - "Status" - COP0 Status
				Status_Reg Status;
				
				// Register #13 - "Cause" - Cause of the most recent exception
				Cause_Reg Cause;
				
				// Register #14 - "EPC" - address that is to resume after an exception has been serviced
				unsigned long EPC;
				
				// Register #15 - "PRId" - Processor revision
				// Imp is 3 on R3000A
				PRId_Reg PRId;

				// Register #16 - "Config" - Processor configuration
				// On R3000A this may be "ERREG"
				Config_Reg Config;
				
				// Registers #17-#22 - Reserved
				unsigned long Reserved1;
				unsigned long Reserved2;
				unsigned long Reserved3;
				unsigned long Reserved4;
				unsigned long Reserved5;
				unsigned long Reserved6;
				
				// Register #23 - "BadPAddr" - Physical address that caused an error - lower 4 bits are always zero
				// this may or may not be used on R3000A, need to check this
				unsigned long BadPAddr;
			};
			
		};

	};


	struct CPR2_ControlRegs
	{
	
		union
		{

			// there are 32 COP2 control registers
			unsigned long Regs [ 32 ];
			
		};
		
	};

	
	struct CPR2_DataRegs
	{
	
		union
		{

			// there are 32 COP2 data registers
			unsigned long Regs [ 32 ];
			
		};
		
	};


	
	
	union ProcStatus
	{
		struct
		{
			union
			{
				struct
				{
					union
					{
						struct
						{
							union
							{
								struct
								{
									u8 Slots_Valid : 2;
									u8 ClearBit0 : 1;
									u8 AsynchronousInterrupt : 1;
									//u8 MTSlots_Valid : 3;
									u8 ClearBit1 : 4;	//1;
								};	// end struct
								
								// use this to check if we need to worry about any delay slots
								u8 DelaySlot_Valid;
								
							};	// end union
							
							union
							{
								struct
								{
									u8 LoadBuffer_Valid : 4;
									u8 StoreBuffer_Valid : 4;
								};	// end struct
								
								// use this to quickly check for loads/stores
								u8 LoadStore_Valid;
								
							};	// end union
						};
						
						u16 ComponentsBusy;
					};
					
					u16 isPipelineStall : 1;				// bit 16
					u16 isSynchronousInterrupt : 1;			// bit 17
					u16 isMultiplyDivideBusy : 1;			// bit 18
					
					// might want to run this on a separate thread, or might not
					u16 isCOP2Busy : 1;						// bit 19

					u16 isExternalInterrupt : 1;
					
					// need to know when processor is waiting for next instruction and/or reloading cache from memory
					u16 isICacheMiss : 1;
					u16 isRequestingInstructionLoad : 1;
					u16 isRequestedInstructionLoaded : 1;
					
				};	// end struct
				
				u32 isSomethingBusy;
				
			};	// end union
			
			union
			{
				struct
				{
					u32 BDSlotIndex : 1;	// branch delay slot
					u32 LDSlotIndex : 1;	// load delay slot
					u32 MTSlotIndex : 2;
					u32 LoadBufferSlotIndex : 2;
					u32 StoreBufferSlotIndex : 2;
					
					// this could potentially be run on a separate thread
					//u32 MultiplyDivideCycle : 6;
					
					// might want to run this on separate thread
					//u32 COP2Cycle : 6;
					
					u32 ExceptionType : 4;	// exception type for synchronous interrupt
					
					u32 Stop : 1;	// says to stop if debugging since we hit break point
				};
				
				u32 Other;
			};	// end union
		};	// end struct
		
		u64 Value;
	};// __attribute__((aligned(4))) __attribute__((packed));	// end union

	
	class Cpu
	{
	
		static Debug::Log debugBios;

	public:
	
		static Cpu *_CPU;
		
		static const long InstructionSize = 4;
		
		// processor clock speed in Hertz (full cycles)
		// but in PS2 configuration, runs instead at 1/4 the PS2 bus speed
		// for ps1, must be 16934400 (16MHZ) clock times two
		// for ps2, must be 18432000(earlier)/18687500(later) (~18MHZ) clock times two
#ifdef PS2_COMPILE
		static const long ClockSpeed = 36864000;
		static const long ClockSpeed2 = 37375000;
#else
		static const long ClockSpeed = 33868800;
#endif
		
#ifdef PS2_COMPILE
		// on a PS2 R3000A IOP this value is NOT the same as on a PS1 R3000A
		// or possibly this value could be changed by the R5900 to boot the R3000A into PS1 mode? Or that other register? hmm...
		static const u32 c_ProcessorRevisionNumber = 0x1f;
#else
		static const u32 c_ProcessorRevisionNumber = 0x2;
#endif

		static const u32 c_ProcessorImplementationNumber = 0x00;

		
		static const u32 c_GeneralInterruptVector = 0x80000080;
		static const u32 c_BootInterruptVector = 0xbfc00180;
		
		static const u32 c_ScratchPadRam_Size = 0x400;
		static const u32 c_ScratchPadRam_Mask = c_ScratchPadRam_Size - 1;
		static const u32 c_ScratchPadRam_Addr = 0x1f800000;

		// 1KB of D-Cache - not like a regular D-Cache, but more like a RAM unless you reverse I$ and D$
		static const u32 DCache_Base = 0x1f800000;
		static const u32 DCache_Size = 0x400;
		
		
		///////////////////////////////////
		// Required Functions For Object //
		///////////////////////////////////

		// enabled if the data on the output lines are valid
		// gets cleared by the bus when the request is accepted
		//Data::InOut::Port Output;

		// enabled if the data on the input lines are valid
		// set by the bus when the requested data is on the lines
		// cleared by the processor once the data has been accepted
		//Data::InOut::Port Input;
		
		// need a connection to the data bus
		// this is how processor communicates with the outside world
		// *** todo *** actually does not need a reference to data bus - just use shared memory
		static Playstation1::DataBus *Bus;
		
		
		
		
		/////////////////////////////
		// Constant CPU Parameters //
		/////////////////////////////
		
		static const u32 ICacheMissCycles = 5;
		
		static const u32 MultiplyCycles = 8;
		static const u32 DivideCycles = 35;
		
		static const u32 c_InstructionLoad_Cycles = 0;
		
		// the number of cycles it takes to store/load a value
		static const u32 c_CycleTime_Store = 1;
		static const u32 c_CycleTime_Load = 5;
		
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Can only modify some parts of COP0 registers
		// AND mask with register and AND inverted mask with value to store, then OR both and store back to register
		// set the bits in mask that can't be written by software
		static const u32 StatusReg_WriteMask = 0x0db800c0;	// software can't modify bits 6-7,19-21,23-24,26-27
		static const u32 CauseReg_WriteMask = 0xfffffcff;	// software can only modify bits 8 and 9
		
		// busy until cycles
		u64 BusyUntil_Cycle;
		u64 MulDiv_BusyUntil_Cycle;

		// used for debugging
		u32 Last_LoadStore_Address;
		
		// this will be used as the icache
		ICache_Device ICache;
		
		// need to delay a certain number of cycles during cache miss before execution can continue
		u32 ICacheMiss_BusyCycles;
		u64 ICacheMiss_BusyUntil_Cycle;
		
		u32 BusyCycles;
		
		// the current instruction being executed
		Instruction::Format CurInst;
		
		// determines if the current instruction has been successfully executed yet
		bool CurInstExecuted;
		
		// if DMA sets invalidate i-cache status flag, then this is the address to invalidate
		volatile u32 ICacheInvalidate_Address;
		
		
		union DCache_Format
		{
			u32 b32 [ DCache_Size/4 ];
			u16 b16 [ DCache_Size/2 ];
			u8 b8 [ DCache_Size ];
		};
		
		DCache_Format DCache;
		
		// used for testing execution unit
		// executes an instruction directly from memory and updates program counter all in same cycle
		// ignores TLB system and ignores cycle delays
		void TestExecute ( u32 NumberOfCyclesToExecute );

		// memory used for testing the operation of the CPU
		//static const u32 TestMemory_Base = 0x00000000;
		//static const u32 TestMemory_Size = 2097152;
		//u32 TestMemory [ TestMemory_Size/4 ];

		
		// sets the program counter
		// used only for testing
		void SetPC ( u32 Value );
		
		void Start ();
		
		// runs the processor for one cycle - need to pass external interrupt signal - should be 1 or 0
		void Run ();

		// resets the data for the processor
		void Reset ();
		
		// dma needs to do this when loading in data to memory
		void InvalidateCache ( u32 Address );
		
		// translates virtual address in CPU into physical address for external bus
		static inline u32 GetPhysicalAddress ( u32 VirtualAddress ) { return VirtualAddress & 0x1fffffff; }

		static u32* _Debug_IntcStat;
		static u32* _Debug_IntcMask;
		// connect devices for using/debugging to the processor
		static void ConnectDebugInfo ( u32* _IntcStat, u32* _IntcMask )
		{
			_Debug_IntcStat = _IntcStat;
			_Debug_IntcMask = _IntcMask;
		}
		
		
		// processor status - I'll use this to make it run faster
		volatile ProcStatus Status;
		

		//////////////////////////////////
		// Functions Specific To Object //
		//////////////////////////////////

		bool debug_enabled;


		

		// bits are set according to which interrupts are pending
		u32 ExternalInterruptsPending;
		
		// this is the reason for interrupt/exception
		u32 ExceptionCode;
		enum { EXC_INT,	// interrupt
				EXC_MOD,	// tlb modification
				EXC_TLBL,	// tlb load
				EXC_TLBS,	// tlb store
				
				// the address errors occur when trying to read outside of kuseg in user mode and when address is misaligned
				EXC_ADEL,	// address error - load/I-fetch
				EXC_ADES,	// address error - store
				
				EXC_IBE,	// bus error on instruction fetch
				EXC_DBE,	// bus error on data load
				EXC_SYSCALL,	// generated unconditionally by syscall instruction
				EXC_BP,			// breakpoint - break instruction
				EXC_RI,			// reserved instruction
				EXC_CPU,		// coprocessor unusable
				EXC_OV,			// arithemetic overflow
				EXC_TRAP,		// placeholder for R5900
				EXC_Unknown };	// will use this to see when exception was created by software
				
			// interrupt mask register $1f801074
			// bit 3 - counter 3 - vsync or vblank?
			// bit 4 - counter 0 - system clock
			// bit 5 - counter 1 - horizontal retrace
			// bit 6 - counter 2 - pixel
				

		
		Reg32 GPR [ 32 ];
		
		// need a bitmap to determine which GPR registers are loading from memory so we know if to stall pipeline or not
		u32 GPRLoading_Bitmap;

		//Reg32 Hi, Lo;
		Reg64 HiLo;

//		u32 CPR0 [ 32 ];	// COP0 control registers
		CPR0_Regs CPR0;		// COP0 control registers
		
		
		COP2_Device COP2;

		// the program counter
		u32 PC;
		u32 NextPC;
		
		// need the address of the last instruction executed to handle asyncronous interrupts with branches in branch delay slots
		u32 LastPC;

		// will count cycles for troubleshooting to see what order things happen in
		volatile u64 CycleCount;

//		volatile long CycleCount;
//		long CycleTarget;	// this is the target number of cycles for the processor to execute
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////
		// These are the ones being used to control branch delay slots and load delay slots and crazy stuff //
		//////////////////////////////////////////////////////////////////////////////////////////////////////

		// callback functions //
		typedef void (*cbFunction) ( void );
		
		// delayed-store call back functions
		static void _cb_NONE ();
		static void _cb_SB ();
		static void _cb_SH ();
		static void _cb_SW ();
		static void _cb_SBU ();
		static void _cb_SHU ();
		static void _cb_SWL ();
		static void _cb_SWR ();
		static void _cb_SWC2 ();
		
		// branch delay slot callback functions
		static void _cb_Branch ();
		static void _cb_Jump ();
		static void _cb_JumpRegister ();
		
		
		// load delay slot callback functions
		static void _cb_LB ();
		static void _cb_LBU ();
		static void _cb_LH ();
		static void _cb_LHU ();
		static void _cb_LW ();
		static void _cb_LWL ();
		static void _cb_LWR ();
		static void _cb_LWC2 ();
		
		// co-processor delay slots
		static void _cb_FC ();
		static void _cb_MTC0 ();
		static void _cb_MTC2 ();
		static void _cb_CTC2 ();
		
		
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

		
		
		
		// invalid counts are negative
		s32 LoadStoreCount;
		
		// the next index to load/store from bus
		s32 NextIndex;
		

		class Buffer
		{
		public:
			struct Buffer_Format
			{
				//bool isValid;	// is this entry valid
				Instruction::Format Inst;	// the load instruction executed
				u32 Address;	// the address to load from
				u32 Value;		// the value that is being stored
				Cpu::cbFunction cb;
			};
			
			static Cpu* r;
			
			
			
			u64 ReadIndex;
			u64 WriteIndex;
			
			// circular buffer
			Buffer_Format Buf [ 4 ];
			
			Buffer ();
			
		
			inline void ConnectDevices ( Cpu* c ) { r = c; }

			
			
			
			// need the load address since it is calculated before the delay slot
			/*
			inline void Add_Load ( Instruction::Format i, u32 LoadAddress ) //, Execute::Callback_Function cb )
			{
				// add entry into load buffer
				r->Status.LoadBuffer_Valid |= ( 1 << WriteIndex );	//Buf [ WriteIndex ].isValid = true;
				Buf [ WriteIndex ].Inst = i;
				Buf [ WriteIndex ].Address = LoadAddress;
				Buf [ WriteIndex ].Index = r->LoadStoreCount++;
				//Buf [ WriteIndex ].cb = cb;
				
				// make sure that LoadStoreCount is not negative
				r->LoadStoreCount &= 0x7fffffff;
				
				// unsure if I want to mark register as loading here, or if I should do that elsewhere
				
				// update write index
				WriteIndex = ( WriteIndex + 1 ) & 3;
			}
			*/

			// store address and value to store come from the instruction because there is no delay slot
			inline void Add_Store ( Instruction::Format i, cbFunction cb )
			{
				// add entry into Store buffer
				u32 StoreAddress = (u32) ( r->GPR [ i.Base ].s + i.sOffset );
				
				u32 ValueToStore = r->GPR [ i.Rt ].u;
				r->Status.StoreBuffer_Valid |= ( 1 << WriteIndex );		//Buf [ WriteIndex ].isValid = true;
				Buf [ WriteIndex ].Inst = i;
				Buf [ WriteIndex ].Address = StoreAddress;
				Buf [ WriteIndex ].Value = ValueToStore;
				Buf [ WriteIndex ].cb = cb;
				
				// update write index
				WriteIndex = ( WriteIndex + 1 ) & 3;
			}

			inline void Add_StoreFromCOP2 ( Instruction::Format i, cbFunction cb )
			{
				// add entry into Store buffer
				u32 StoreAddress = (u32) ( r->GPR [ i.Base ].s + i.sOffset );
				
				u32 ValueToStore = r->COP2.Read_MFC ( i.Rt );	//r->COP2.CPR2.Regs [ i.Rt ];
				r->Status.StoreBuffer_Valid |= ( 1 << WriteIndex );		//Buf [ WriteIndex ].isValid = true;
				Buf [ WriteIndex ].Inst = i;
				Buf [ WriteIndex ].Address = StoreAddress;
				Buf [ WriteIndex ].Value = ValueToStore;
				Buf [ WriteIndex ].cb = cb;
				
				// update write index
				WriteIndex = ( WriteIndex + 1 ) & 3;
			}
			
			// check if there is an element in the buffer at read position
			//inline bool isValidLoad () { return ( r->Status.LoadBuffer_Valid & ( 1 << ReadIndex ) ); }
			inline bool isValidStore () { return ( r->Status.StoreBuffer_Valid & ( 1 << ReadIndex ) ); }
			
			// check if the next element in the buffer at read position is the next load/store to process
			//inline bool isNextLoad () { return ( this->isValidLoad() && ( ( Buf [ ReadIndex ].Index & 0x7fffffff ) == r->NextIndex ) ); }
			//inline bool isNextStore () { return ( this->isValidStore() && ( ( Buf [ ReadIndex ].Index & 0x7fffffff ) == r->NextIndex ) ); }
			
			// invalidate item at current read position
			//inline void InvalidateLoad () { r->Status.LoadBuffer_Valid &= ~( 1 << ReadIndex ); /*Buf [ ReadIndex ].Inst.Value = 0; Buf [ ReadIndex ].Address = 0; Buf [ ReadIndex ].Value = 0; Buf [ ReadIndex ].Index = 0;*/ }
			inline void InvalidateStore () { r->Status.StoreBuffer_Valid &= ~( 1 << ReadIndex ); /*Buf [ ReadIndex ].Inst.Value = 0; Buf [ ReadIndex ].Address = 0; Buf [ ReadIndex ].Value = 0; Buf [ ReadIndex ].Index = 0;*/ }
			
			// advance to next item in buffer
			inline void Advance ()
			{
				ReadIndex = ( ReadIndex + 1 ) & 0x3;
			}
			
			//inline bool isFullLoad () { return ( r->Status.LoadBuffer_Valid == 0xf ); }
			inline bool isFullStore () { return ( r->Status.StoreBuffer_Valid == 0xf ); }
			
			//inline int LoadCount () { return POPCNT ( r->Status.LoadBuffer_Valid & 0xf ); }
			inline int StoreCount () { return POPCNT ( r->Status.StoreBuffer_Valid & 0xf0 ); }
			
			inline Instruction::Format Get_Inst () { return Buf [ ReadIndex ].Inst; }
			inline u32 Get_Address () { return Buf [ ReadIndex ].Address; }
			inline u32 Get_Value () { return Buf [ ReadIndex ].Value; }
			inline cbFunction Get_CB () { return Buf [ ReadIndex ].cb; }
			
		};
		
		// can also use this for the load and store buffers
		Buffer StoreBuffer;
		//Buffer LoadBuffer;
		
		
		////////////////////////////////////////////
		// These can be used for debugging
		u32 Last_ReadAddress;
		u32 Last_WriteAddress;
		u32 Last_ReadWriteAddress;
		
		// looks like I'll need this
		u32 LastModifiedRegister;
		
		
		
		/////////////////////
		// Pipeline Stalls //
		/////////////////////
		
		//u32 isPipelineStall;
		bool isWriteBusy;
		bool isReadBusy;
		bool isICacheMiss;
		
		//bool isMultiplyDivideBusy;
		u32 MultiplyDivide_BusyCycles;
		//u32 MultiplyDivideBusy_NumCycles;
		//Instruction::Format MultiplyDivideBusy_Inst;
		
		//bool isCOP2Busy;
		//u32 COP2Busy_Cycles;
		//Instruction::Format COP2Busy_Inst;
		
		
		static u32 TraceValue;
		
		
		volatile long Stop;
		
		// constructor - cpu needs a data bus to operate firstly so it can even read instructions!!
		Cpu ();
		
		// destructor
		~Cpu ( void );

		void Write_MTC0 ( u32 Register, u32 Value );

		void SkipIdleCycles ();
		void SkipIdleCpuCycles ();
		void WaitForBusReady1 ();
		void WaitForCpuReady1 ();
		
		void FlushStoreBuffer ();

		static u64* _SpuCycleCount;
		
		void ConnectDevices ( Playstation1::DataBus* db, u64* _SpuCC );
		
		
		////////////////
		// Interrupts //
		////////////////
		
		//bool isSynchronousInterrupt;
		u32 isAsynchronousInterrupt;
		
		// signals an asynchronous interrupt to occur at the next possible moment
		void SignalAsynchronousInterrupt ( u32 InterruptingDeviceIndex );

		inline void UpdateInterrupt ()
		{
#ifdef INLINE_DEBUG_UPDATE_INT
	debug << "\r\nUpdateInterrupt;(before) _Intc_Stat=" << hex << *_Intc_Stat << " _Intc_Mask=" << *_Intc_Mask << " _R3000A_Status=" << CPR0.Regs [ 12 ] << " _R3000A_Cause=" << CPR0.Regs [ 13 ] << " _ProcStatus=" << Status.Value;
#endif

			if ( *_Intc_Stat & *_Intc_Mask ) CPR0.Regs [ 13 ] |= ( 1 << 10 ); else CPR0.Regs [ 13 ] &= ~( 1 << 10 );
			if ( ( CPR0.Regs [ 13 ] & CPR0.Regs [ 12 ] & 0xff00 ) && ( CPR0.Regs [ 12 ] & 1 ) ) Status.Value |= ( 1 << 20 ); else Status.Value &= ~( 1 << 20 );
			
#ifdef INLINE_DEBUG_UPDATE_INT
	debug << "\r\n(after) _Intc_Stat=" << hex << *_Intc_Stat << " _Intc_Mask=" << *_Intc_Mask << " _R3000A_Status=" << CPR0.Regs [ 12 ] << " _R3000A_Cause=" << CPR0.Regs [ 13 ] << " _ProcStatus=" << Status.Value;
#endif
		}


		// checks if an address points to DCache or to memory
		inline static bool isDCache ( u32 Address )
		{
			return ( ( Address - DCache_Base ) < DCache_Size );
		}

		
		// checks if a memory address is cached
		bool isCached ( u32 Address );
		
		// checks if a memory address is not cached
		bool isNotCached ( u32 Address );
		
		
		void ProcessSynchronousInterrupt ( u32 ExceptionType );


		static u32* _Intc_Stat;
		static u32* _Intc_Mask;
		static u32* _R3000A_Status;
		
		inline void ConnectInterrupt ( u32* _IStat, u32* _IMask, u32* _R3000A_Stat )
		{
			_Intc_Stat = _IStat;
			_Intc_Mask = _IMask;
			_R3000A_Status = _R3000A_Stat;
		}

		
		//////////////////////////////////////////
		// Debug Status

		union _DebugStatus
		{
			struct
			{
				u32 Stop : 1;
				u32 Step : 1;
				u32 Done : 1;
				u32 OutputCode : 1;
				u32 SaveBIOSToFile : 1;
				u32 SaveRAMToFile : 1;
				u32 SetBreakPoint : 1;
				u32 SetMemoryStart : 1;
			};
			
			u32 Value;
		};

		static volatile _DebugStatus DebugStatus;

		static const u32 CallStack_Size = 0x8;
		u32 CallStackDepth;
		u32 Debug_CallStack_Address [ 8 ];
		u32 Debug_CallStack_Function [ 8 ];
		u32 Debug_CallStack_ReturnAddress [ 8 ];
		

		void ProcessLoadDelaySlot ();
		
		static volatile u64* volatile _NextSystemEvent;
		
		// Enable/Disable debug window for object
		// Windows API specific code is in here
		static bool DebugWindow_Enabled;
		static WindowClass::Window *DebugWindow;
		static DebugValueList<u32> *GPR_ValueList;
		static DebugValueList<u32> *COP0_ValueList;
		static DebugValueList<u32> *COP2_CPCValueList;
		static DebugValueList<u32> *COP2_CPRValueList;
		static Debug_DisassemblyViewer *DisAsm_Window;
		static Debug_BreakpointWindow *Breakpoint_Window;
		static Debug_MemoryViewer *ScratchPad_Viewer;
		static Debug_BreakPoints *Breakpoints;
		static void DebugWindow_Enable ();
		static void DebugWindow_Disable ();
		static void DebugWindow_Update ();
		
		//static const u32 DisAsm_Window_Rows = 0x3ffffff;
		//static string DisAsm_Window_GetText ( int row, int col );

	private:
	
		////////////////////////////////////////////////
		// Debug Output
		static Debug::Log debug;
		
		
		// multi-threading stuff
		// volatile u64 Ran -> this will be defined in the system object to say what devices have ran for cycle
		volatile u32 Running;
		
	
		// gnu stuff keeps ignoring my solid code, have to try this way
		bool ProcessExternalLoad ();
		void ProcessExternalStore ();
		void ProcessAsynchronousInterrupt ();
		
		// process the CPU events required on every cycle (Multiply/Divider,COP2)
		void ProcessRequiredCPUEvents ();
		
		void ProcessBranchDelaySlot ();

		

	};

}




#endif




