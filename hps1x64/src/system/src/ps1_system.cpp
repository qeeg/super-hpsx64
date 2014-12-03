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



#include "ps1_system.h"

#include "R3000ADebugPrint.h"

#include <sstream>
#include <fstream>
#include "WinApiHandler.h"


//using namespace Playstation1;
using namespace std;

using namespace x64ThreadSafe::Utilities;
using namespace R3000A::Instruction;

#ifdef _DEBUG_VERSION_

// enable inline debugging
#define INLINE_DEBUG_ENABLE
//#define INLINE_DEBUG
//#define INLINE_DEBUG_DEVICE
//#define INLINE_DEBUG_MENU
//#define INLINE_DEBUG_BREAK_VALUECHANGE
//#define INLINE_DEBUG_BREAK_VALUEMATCH

#endif


Debug::Log Playstation1::System::debug;

Playstation1::System *Playstation1::System::_SYSTEM;
u64 *Playstation1::System::_DebugCycleCount;

u32 Playstation1::System::debug_enabled;
WindowClass::Window *Playstation1::System::DebugWindow;
WindowClass::Window *Playstation1::System::FrameBuffer;
WindowClass::Window *Playstation1::System::SPU_DebugWindow;
WindowClass::Window *Playstation1::System::DMA_DebugWindow;
WindowClass::Window *Playstation1::System::COP2_DebugWindow;


Playstation1::System::System ()
{
	cout << "Running SYSTEM constructor...\n";
	
/*
#ifdef INLINE_DEBUG_ENABLE
	// create inline debugger
	debug.Create ( "System_Log.txt" );
#endif

	
	_SYSTEM = this;
	
	// set pointer in databus for debugging
	_BUS.DebugPC = &(_CPU.PC);

#ifdef INLINE_DEBUG
	debug << "Setting pointer to bus for dma\r\n";
#endif

	// connect devices
	//_DMA.Set_Bus ( &_BUS );
	_CPU.ConnectDevices ( &_BUS );
	_BUS.ConnectDevices ( &_DMA, &_CD, &_GPU, &_INTC, &_MDEC, &_PIO, &_SIO, &_SPU, &_TIMERS );
	_DMA.ConnectDevices ( & _BUS, & _GPU );
	_TIMERS.ConnectDevices ( &_GPU );
	
	// connect debug info
	_CPU.ConnectDebugInfo ( & _INTC.I_STAT_Reg.Value, & _INTC.I_MASK_Reg.Value );
	
	_DMA._DebugPC = & (_CPU.PC);
	_GPU._DebugPC = & (_CPU.PC);
	_TIMERS._DebugPC = & (_CPU.PC);
	_INTC._DebugPC = & (_CPU.PC);
	_SPU._DebugPC = & (_CPU.PC);
	_CD._DebugPC = & (_CPU.PC);
	_SIO._DebugPC = & (_CPU.PC);
	_PIO._DebugPC = & (_CPU.PC);
	
	_DMA._DebugCycleCount = & (_CPU.CycleCount);
	_GPU._DebugCycleCount = & (_CPU.CycleCount);
	_TIMERS._DebugCycleCount = & (_CPU.CycleCount);
	_INTC._DebugCycleCount = & (_CPU.CycleCount);
	_SPU._DebugCycleCount = & (_CPU.CycleCount);
	_CD._DebugCycleCount = & (_CPU.CycleCount);
	_SIO._DebugCycleCount = & (_CPU.CycleCount);
	_PIO._DebugCycleCount = & (_CPU.CycleCount);
	
	
	_GPU.DebugCpuPC = & (_CPU.PC);
	
	Reset ();
	*/
}




Playstation1::System::~System ()
{
	// write memory cards
	//_SIO.Store_MemoryCardFile ( "mcd0", 0 );
	//_SIO.Store_MemoryCardFile ( "mcd1", 1 );

	if ( debug_enabled )
	{
		delete DebugWindow;
		delete FrameBuffer;
	}
}

void Playstation1::System::Reset ()
{
	Debug_BreakPoint_Address = 0xffffffff;
	Debug_RAMDisplayStart = 0;
	
	Debug_CycleBreakPoint_Address = 0xffffffffffffffff;
	Debug_AddressBreakPoint_Address = 0xffffffff;
	
	//_BUS.Reset();
	//_CPU.Reset();
	//_DMA.Reset();
	//_CD.Reset();
	//_GPU.Reset();
	//_INTC.Reset();
	//_MDEC.Reset();
	//_PIO.Reset();
	//_SIO.Reset();
	//_SPU.Reset();
	//_TIMERS.Reset();
}


void Playstation1::System::Start ()
{
#ifdef INLINE_DEBUG_ENABLE
	// create inline debugger
	debug.Create ( "System_Log.txt" );
#endif

	
	_SYSTEM = this;

	
	_SPU._NextSystemEvent = &NextEvent_Cycle;
	_CD._NextSystemEvent = &NextEvent_Cycle;
	_PIO._NextSystemEvent = &NextEvent_Cycle;
	_DMA._NextSystemEvent = &NextEvent_Cycle;
	_TIMERS._NextSystemEvent = &NextEvent_Cycle;
	_GPU._NextSystemEvent = &NextEvent_Cycle;
	_SIO._NextSystemEvent = &NextEvent_Cycle;
	_CPU._NextSystemEvent = &NextEvent_Cycle;
	
#ifdef PS2_COMPILE
	_CDVD._NextSystemEvent = &NextEvent_Cycle;
	_SPU2._NextSystemEvent = &NextEvent_Cycle;
#endif


	// set pointers for debugger
	//_CPU.Breakpoints->RAM = _BUS.MainMemory.b8;
	//_CPU.Breakpoints->BIOS = _BUS.BIOS.b8;
	//_CPU.Breakpoints->DCACHE = _CPU.DCache.b8;
	
	// set pointer in databus for debugging
	_BUS.DebugPC = &(_CPU.PC);

#ifdef INLINE_DEBUG
	debug << "Setting pointer to bus for dma\r\n";
#endif


	// connect devices
	//_DMA.Set_Bus ( &_BUS );
	_CPU.ConnectDevices ( &_BUS, &_SPU.CycleCount );
	_BUS.ConnectDevices ( &_DMA, &_CD, &_GPU, &_INTC, &_MDEC, &_PIO, &_SIO, &_SPU, &_TIMERS );
	_DMA.ConnectDevices ( & _BUS, &_MDEC, & _GPU, &_CD, &_SPU, &_CPU );
	_TIMERS.ConnectDevices ( &_GPU );
	
	// connect debug info
	_CPU.ConnectDebugInfo ( & _INTC.I_STAT_Reg.Value, & _INTC.I_MASK_Reg.Value );
	
	// connect interrupts
	_DMA.ConnectInterrupt ( & _INTC.I_STAT_Reg.Value, & _INTC.I_MASK_Reg.Value, & _CPU.CPR0.Regs [ 12 ], & _CPU.CPR0.Regs [ 13 ], &_CPU.Status.Value );
	_GPU.ConnectInterrupt ( & _INTC.I_STAT_Reg.Value, & _INTC.I_MASK_Reg.Value, & _CPU.CPR0.Regs [ 12 ], & _CPU.CPR0.Regs [ 13 ], &_CPU.Status.Value );
	_TIMERS.ConnectInterrupt ( & _INTC.I_STAT_Reg.Value, & _INTC.I_MASK_Reg.Value, & _CPU.CPR0.Regs [ 12 ], & _CPU.CPR0.Regs [ 13 ], &_CPU.Status.Value );
	_SPU.ConnectInterrupt ( & _INTC.I_STAT_Reg.Value, & _INTC.I_MASK_Reg.Value, & _CPU.CPR0.Regs [ 12 ], & _CPU.CPR0.Regs [ 13 ], &_CPU.Status.Value );
	_CD.ConnectInterrupt ( & _INTC.I_STAT_Reg.Value, & _INTC.I_MASK_Reg.Value, & _CPU.CPR0.Regs [ 12 ], & _CPU.CPR0.Regs [ 13 ], &_CPU.Status.Value );
	_SIO.ConnectInterrupt ( & _INTC.I_STAT_Reg.Value, & _INTC.I_MASK_Reg.Value, & _CPU.CPR0.Regs [ 12 ], & _CPU.CPR0.Regs [ 13 ], &_CPU.Status.Value );
	_PIO.ConnectInterrupt ( & _INTC.I_STAT_Reg.Value, & _INTC.I_MASK_Reg.Value, & _CPU.CPR0.Regs [ 12 ], & _CPU.CPR0.Regs [ 13 ], &_CPU.Status.Value );
	_CPU.ConnectInterrupt ( & _INTC.I_STAT_Reg.Value, & _INTC.I_MASK_Reg.Value, & _CPU.CPR0.Regs [ 13 ] );
	_INTC.ConnectInterrupt ( & _CPU.CPR0.Regs [ 12 ], & _CPU.CPR0.Regs [ 13 ], &_CPU.Status.Value );

#ifdef PS2_COMPILE
	_CDVD.ConnectInterrupt ( & _INTC.I_STAT_Reg.Value, & _INTC.I_MASK_Reg.Value, & _CPU.CPR0.Regs [ 12 ], & _CPU.CPR0.Regs [ 13 ], &_CPU.Status.Value );
	_SPU2.ConnectInterrupt ( & _INTC.I_STAT_Reg.Value, & _INTC.I_MASK_Reg.Value, & _CPU.CPR0.Regs [ 12 ], & _CPU.CPR0.Regs [ 13 ], &_CPU.Status.Value );
#endif

	
	_DMA._DebugPC = & (_CPU.PC);
	_GPU._DebugPC = & (_CPU.PC);
	_TIMERS._DebugPC = & (_CPU.PC);
	_INTC._DebugPC = & (_CPU.PC);
	_SPU._DebugPC = & (_CPU.PC);
	_CD._DebugPC = & (_CPU.PC);
	_SIO._DebugPC = & (_CPU.PC);
	_PIO._DebugPC = & (_CPU.PC);
	_MDEC._DebugPC = & (_CPU.PC);
	_BUS._DebugPC = & (_CPU.PC);
	
#ifdef PS2_COMPILE
	_CDVD._DebugPC = & (_CPU.PC);
	_SPU2._DebugPC = & (_CPU.PC);
#endif


	_DMA._DebugCycleCount = & (_CPU.CycleCount);
	_GPU._DebugCycleCount = & (_CPU.CycleCount);
	_TIMERS._DebugCycleCount = & (_CPU.CycleCount);
	_INTC._DebugCycleCount = & (_CPU.CycleCount);
	_SPU._DebugCycleCount = & (_CPU.CycleCount);
	_CD._DebugCycleCount = & (_CPU.CycleCount);
	_SIO._DebugCycleCount = & (_CPU.CycleCount);
	_PIO._DebugCycleCount = & (_CPU.CycleCount);
	_MDEC._DebugCycleCount = & (_CPU.CycleCount);
	_BUS._DebugCycleCount = & (_CPU.CycleCount);
	_DebugCycleCount = & (_CPU.CycleCount);
	
#ifdef PS2_COMPILE
	_CDVD._DebugCycleCount = & (_CPU.CycleCount);
	_SPU2._DebugCycleCount = & (_CPU.CycleCount);
#endif


	// also set for timer object
	Timer::_DebugPC = & (_CPU.PC);
	Timer::_DebugCycleCount = & (_CPU.CycleCount);
	
	_DMA._Intc_Stat = & _INTC.I_STAT_Reg.Value;
	_DMA._Intc_Mask = & _INTC.I_MASK_Reg.Value;
	_DMA._R3000a_Status = & _CPU.CPR0.Status.Value;
	
	_GPU.DebugCpuPC = & (_CPU.PC);

	//Reset ();

	// Start Objects //
	_TIMERS.Start ();
	_GPU.Start ();
	_CPU.Start ();
	_BUS.Start ();
	_DMA.Start ();
	_INTC.Start ();
	_SPU.Start ();
	_SIO.Start ();
	_PIO.Start ();
	_MDEC.Start ();
	_CD.Start ();
	
#ifdef PS2_COMPILE
	_CDVD.Start ();
	_SPU2.Start ();
#endif

}


void Playstation1::System::Test ( void )
{
	// transfer data from 0x1efb28 into frame buffer at x=0 y=400 96x84
	u32 Source, DestX, DestY, x, y, Index;
	
	//Source = 0x1efb28;
	
	// output contents of NVM file
	Index = 0;
	for ( y = 0; y < 32; y++ )
	{
		cout << "\n";
		for ( x = 0; x < 32; x++ )
		{
#ifdef PS2_COMPILE
			cout << hex << setw ( 2 ) << setfill ( '0' ) << (u32) (((u8*)_CDVD.NVM) [ Index++ ]);
#endif
		}
	}
	
	
	/*
	for ( DestY = 0; DestY < 180; DestY++ )
	{
		for ( DestX = 0; DestX < 640; DestX++ )
		{
			//_GPU.VRAM [ ( DestX + 0 ) + ( ( DestY + 400 ) << 10 ) ] = _BUS.MainMemory.b16 [ Source >> 1 ];
			_GPU.VRAM [ ( DestX + 0 ) + ( ( DestY + 0 ) << 10 ) ] = 0;
			
			Source += 2;
		}
	}
	
	// update the frame buffer window
	_GPU.DebugWindow_Update ();
	*/
}


void Playstation1::System::GetNextEventCycle ( void )
{
	// initialize the next event cycle (this way I can later separate the components more later)
	NextEvent_Cycle = -1LL;
	
	if ( _CD.NextEvent_Cycle > *_DebugCycleCount && ( _CD.NextEvent_Cycle < NextEvent_Cycle || NextEvent_Cycle <= *_DebugCycleCount ) ) NextEvent_Cycle = _CD.NextEvent_Cycle;
	if ( _SPU.NextEvent_Cycle > *_DebugCycleCount && ( _SPU.NextEvent_Cycle < NextEvent_Cycle || NextEvent_Cycle <= *_DebugCycleCount ) ) NextEvent_Cycle = _SPU.NextEvent_Cycle;
	if ( _PIO.NextEvent_Cycle > *_DebugCycleCount && ( _PIO.NextEvent_Cycle < NextEvent_Cycle || NextEvent_Cycle <= *_DebugCycleCount ) ) NextEvent_Cycle = _PIO.NextEvent_Cycle;
	if ( _DMA.NextEvent_Cycle > *_DebugCycleCount && ( _DMA.NextEvent_Cycle < NextEvent_Cycle || NextEvent_Cycle <= *_DebugCycleCount ) ) NextEvent_Cycle = _DMA.NextEvent_Cycle;
	if ( _TIMERS.NextEvent_Cycle > *_DebugCycleCount && ( _TIMERS.NextEvent_Cycle < NextEvent_Cycle || NextEvent_Cycle <= *_DebugCycleCount ) ) NextEvent_Cycle = _TIMERS.NextEvent_Cycle;
	if ( _GPU.NextEvent_Cycle > *_DebugCycleCount && ( _GPU.NextEvent_Cycle < NextEvent_Cycle || NextEvent_Cycle <= *_DebugCycleCount ) ) NextEvent_Cycle = _GPU.NextEvent_Cycle;
	if ( _SIO.NextEvent_Cycle > *_DebugCycleCount && ( _SIO.NextEvent_Cycle < NextEvent_Cycle || NextEvent_Cycle <= *_DebugCycleCount ) ) NextEvent_Cycle = _SIO.NextEvent_Cycle;
	
#ifdef PS2_COMPILE
	if ( _CDVD.NextEvent_Cycle > *_DebugCycleCount && ( _CDVD.NextEvent_Cycle < NextEvent_Cycle || NextEvent_Cycle <= *_DebugCycleCount ) ) NextEvent_Cycle = _CDVD.NextEvent_Cycle;
	if ( _SPU2.NextEvent_Cycle > *_DebugCycleCount && ( _SPU2.NextEvent_Cycle < NextEvent_Cycle || NextEvent_Cycle <= *_DebugCycleCount ) ) NextEvent_Cycle = _SPU2.NextEvent_Cycle;
#endif
}


void Playstation1::System::RunDevices ()
{
#ifdef INLINE_DEBUG_DEVICE
	debug << "\r\nPlaystation1::System::RunDevices; CycleCount=" << _CPU.CycleCount;
#endif

#ifdef INLINE_DEBUG_DEVICE
	debug << "; TIMERS";
#endif

		// *important* must do timer interrupt events first
		// this is because the GPU updates the timers, but you must catch the timer transition so you don't miss it
		// would probably make more sense for the timers to update themselves every so often, though
		_TIMERS.Run ();
		
#ifdef INLINE_DEBUG_DEVICE
	debug << "; GPU";
#endif

		_GPU.Run ();

#ifdef INLINE_DEBUG_DEVICE
	debug << "; CD";
#endif

		_CD.Run ();

#ifdef INLINE_DEBUG_DEVICE
	debug << "; SPU";
#endif

		_SPU.Run ();

#ifdef INLINE_DEBUG_DEVICE
	debug << "; PIO";
#endif

		_PIO.Run ();
	
#ifdef INLINE_DEBUG_DEVICE
	debug << "; DMA";
#endif

		_DMA.Run ();
	
#ifdef INLINE_DEBUG_DEVICE
	debug << "; SIO";
#endif

		_SIO.Run ();


#ifdef PS2_COMPILE

#ifdef INLINE_DEBUG_DEVICE
	debug << "; CDVD";
#endif

		_CDVD.Run ();

#ifdef INLINE_DEBUG_DEVICE
	debug << "; SPU2";
#endif

		_SPU2.Run ();
		
#endif


		// the cycle number for the next event
		GetNextEventCycle ();
}


void Playstation1::System::Run_Cycles ( u64 Cycles )
{
	Cycles += *_DebugCycleCount;
	
	do
	{
		Run ();
	}
	while ( *_DebugCycleCount < Cycles );
}


void Playstation1::System::Run ()
{
#ifdef INLINE_DEBUG
	debug << "\r\nPlaystation1::System::Run; CycleCount=" << *_DebugCycleCount;
#endif

	// only run components when they are busy with something
	if ( NextEvent_Cycle == *_DebugCycleCount )
	{
		RunDevices ();
	}


#ifdef INLINE_DEBUG
	debug << "; CPU";
#endif

	_CPU.Run ();
}



///////////////////////////////////////////////////////
// Loads a test program into bios
// returns true on success, false otherwise
bool Playstation1::System::LoadTestProgramIntoBios ( char* FilePath )
{
	//static const long BIOS_SIZE_IN_BYTES = 524288;

#ifdef INLINE_DEBUG
	debug << "\r\nEntered function: LoadTestProgram";
#endif

	u32 Code;
	u32 Address = 0;
	ifstream InputFile ( FilePath, ios::binary );
	
	if ( !InputFile )
	{
#ifdef INLINE_DEBUG
	debug << "->Error opening input file into memory/bios";
#endif

		cout << "Error opening test R3000A code.\n";
		return false;
	}


#ifdef INLINE_DEBUG
	debug << "; Reading input file into memory";
#endif

	// write entire program into memory
	//InputFile.read ( (char*) ( _BUS.BIOS.b32 ), BIOS_SIZE_IN_BYTES );
	InputFile.read ( (char*) ( _BUS.BIOS.b32 ), _BUS.BIOS_Size );
	
	InputFile.close();
	
#ifdef INLINE_DEBUG
	debug << "->Leaving function: LoadTestProgram";
#endif
	
	return true;
}




