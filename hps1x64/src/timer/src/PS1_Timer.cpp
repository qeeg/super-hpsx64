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


#include "types.h"

#include "PS1_Timer.h"
#include <math.h>
#include "Reciprocal.h"


//#define EXCLUDE_TARGET


#ifdef _DEBUG_VERSION_

// enable debugging
#define INLINE_DEBUG_ENABLE

/*
#define INLINE_DEBUG_READ
#define INLINE_DEBUG_WRITE
#define INLINE_DEBUG_CALIBRATE
#define INLINE_DEBUG_EVENT
#define INLINE_DEBUG_RUN
*/

//#define INLINE_DEBUG_READ_MODE

//#define INLINE_DEBUG_RUN_VBLANK
//#define INLINE_DEBUG

#endif


using namespace Playstation1;
using namespace Math::Reciprocal;

#define CONVERT_TO_FIXED1PT63( x )	( ( (u64) ( ( x ) * ( ( (u64) 1 ) << 63 ) ) ) + ( 1 << 8 ) )

//const char* TimerNameList [ 4 ] = { "Timer0_Log.txt", "Timer1_Log.txt", "Timer2_Log.txt", "Timer3_Log.txt" };



u32* Timer::_DebugPC;
u64* Timer::_DebugCycleCount;

u32* Timers::_DebugPC;
u64* Timers::_DebugCycleCount;


u64* Timers::_NextSystemEvent;

u32* Timers::_Intc_Stat;
u32* Timers::_Intc_Mask;
//u32* Timers::_Intc_Master;
u32* Timers::_R3000A_Status_12;
u32* Timers::_R3000A_Cause_13;
u64* Timers::_ProcStatus;


int Timer::Count = 0;
Debug::Log Timers::debug;

Timers *Timers::_TIMERS;


GPU *Timer::g;
GPU *Timers::g;


bool Timers::DebugWindow_Enabled;
WindowClass::Window *Timers::DebugWindow;
DebugValueList<u32> *Timers::Timer_ValueList;


Timer::Timer ()
{
/*
	debug.Create ( TimerNameList [ Index ] );

#ifdef INLINE_DEBUG
	debug << "\r\nEntering Timer" << Index << " constructor";
#endif
*/

	Reset ();
	
	//Number = Count++;
	
/*	
#ifdef INLINE_DEBUG
	debug << "->Exiting Timer" << Number << " constructor";
#endif
*/
}

void Timer::Reset ()
{
/*
#ifdef INLINE_DEBUG
	debug << "\r\nEntering Timer" << Number << "::Reset";
#endif
*/

	// zero object
	memset ( this, 0, sizeof( Timer ) );
	
/*	
#ifdef INLINE_DEBUG
	debug << "->Exiting Timer" << Number << "::Reset";
#endif
*/
}


void Timer::Set_TimerNumber ( int lTimerIndex, u32 lTimerNumber )
{
	Index = lTimerIndex;
	TimerNumber = lTimerNumber;
}




void Timers::ConnectDevices ( Playstation1::GPU* _g )
{
	g = _g;
	Timer::g = _g;
}


Timers::Timers ()
{
	cout << "Running Timers constructor...\n";

/*
#ifdef INLINE_DEBUG_ENABLE
	debug.Create ( "Timers_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering Timers constructor";
#endif


	Reset ();
	
	_TIMERS = this;
	
#ifdef INLINE_DEBUG
	debug << "->Exiting Timers constructor";
#endif
*/

}



void Timers::Start ()
{
	cout << "Running Timers::Start...\n";

#ifdef INLINE_DEBUG_ENABLE
	debug.Create ( "Timers_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering Timers::Start";
#endif


	Reset ();
	
	_TIMERS = this;
	
	
	// set the timer numbers
	TheTimers [ 0 ].Set_TimerNumber ( 0, 0 );
	TheTimers [ 1 ].Set_TimerNumber ( 1, 1 );
	TheTimers [ 2 ].Set_TimerNumber ( 2, 2 );


#ifdef PS2_COMPILE
	TheTimers [ 3 ].Set_TimerNumber ( 3, 3 );
	TheTimers [ 4 ].Set_TimerNumber ( 4, 4 );
	TheTimers [ 5 ].Set_TimerNumber ( 5, 5 );
#endif
	
	
	// calibrate the timers
	CalibrateTimer ( 0 );
	CalibrateTimer ( 1 );
	CalibrateTimer ( 2 );
	
#ifdef PS2_COMPILE
	CalibrateTimer ( 3 );
	CalibrateTimer ( 4 );
	CalibrateTimer ( 5 );
#endif
	
	// set the timer values
	//SetTimerValue ( 0, 0 );
	//SetTimerValue ( 1, 0 );
	//SetTimerValue ( 2, 0 );
	SetValue ( 0, 0 );
	SetValue ( 1, 0 );
	SetValue ( 2, 0 );
	
#ifdef PS2_COMPILE
	SetValue ( 3, 0 );
	SetValue ( 4, 0 );
	SetValue ( 5, 0 );
#endif


	// get the next event for each timer (should be none here)
	//GetNextEvent ( 0 );
	//GetNextEvent ( 1 );
	//GetNextEvent ( 2 );
	
#ifdef INLINE_DEBUG
	debug << "->Exiting Timers::Start";
#endif
}


void Timers::Reset ()
{
	// zero object
	memset ( this, 0, sizeof( Timers ) );
}


void Timers::Run ()
{
	// *** TODO *** need to account for when irq is set on both target and 0xffff/Overflow
	// *** TODO *** one shot mode would only trigger the first one in that case also

	//u32 TimerTarget;
	//u32 TimerValue;
	
	u32 TimerNumber;

	if ( NextEvent_Cycle != *_DebugCycleCount ) return;
	
	// event triggered //
#ifdef INLINE_DEBUG_RUN
	debug << "\r\nTimers::Run";
#endif
	
	for ( TimerNumber = 0; TimerNumber < c_iNumberOfChannels; TimerNumber++ )
	{

		// check if it is a timer event
		if ( TheTimers [ TimerNumber ].NextEvent_Cycle == NextEvent_Cycle )
		{
#ifdef INLINE_DEBUG_RUN
	debug << "\r\nInterrupt; Timer#" << TimerNumber << " Cycle#" << dec << *_DebugCycleCount;
#endif

			// make sure value is target - ONLY COMPARE 16-BITS!!
			//if ( ( TimerValue == TimerTarget && TheTimers [ TimerNumber ].MODE.IrqOnTarget ) || ( TimerValue == 0xffff && TheTimers [ TimerNumber ].MODE.IrqOnOverflow ) )
			//{
				// *** TODO *** still need to check which interrupt is enabled.. int on target or int on overflow
				// *** TODO *** also need to check if is a one-shot or repeat interrupt
				
				// check if this is a repeated interrupt or single interrupt on first signal
				if ( TheTimers [ TimerNumber ].MODE.IrqMode_Repeat || ( !TheTimers [ TimerNumber ].MODE.IrqMode_Repeat && !TheTimers [ TimerNumber ].IRQ_Counter ) )
				{
					// update timer to see what its value is
					//UpdateTimer ( TimerNumber );
				
					// if timer equals target, then set target reached flag
					if ( TheTimers [ TimerNumber ].NextIntType == Timer::INT_TYPE_TARGET )
					{
						TheTimers [ TimerNumber ].MODE.TargetReached = 1;
					}
					else
					{
						TheTimers [ TimerNumber ].MODE.OverflowReached = 1;
						//TheTimers [ TimerNumber ].MODE.TargetReached = 1;
					}
				
					// check if timer is in pulse mode or toggle mode
					if ( TheTimers [ TimerNumber ].MODE.IrqMode_Toggle )
					{
						// timer is in toggle mode //
						
#ifdef INLINE_DEBUG_RUN
	debug << "; TOGGLE";
#endif

						// toggle irq
						TheTimers [ TimerNumber ].MODE.IrqRequest ^= 1;
					}
					else
					{
						// timer is in pulse mode //
						
#ifdef INLINE_DEBUG_RUN
	debug << "; PULSE";
#endif

						// clear irq temporarily
						TheTimers [ TimerNumber ].MODE.IrqRequest = 0;
					}
					
					// check if irq is requested (IrqRequest==0)
					if ( !TheTimers [ TimerNumber ].MODE.IrqRequest )
					{
						// IRQ requested //
						
#ifdef INLINE_DEBUG_RUN
	UpdateTimer ( TimerNumber );
	debug << "; INT Timer#" << TimerNumber;
	debug << "; TimerValue@INT=" << hex << TheTimers [ TimerNumber ].StartValue;
#endif

						// generate a timer0 interrupt
						SetInterrupt ( TimerNumber );
						
						// update count of interrupts for timer
						TheTimers [ TimerNumber ].IRQ_Counter++;
					}
					
					// if timer is in pulse mode, then set irq request back for now
					if ( !TheTimers [ TimerNumber ].MODE.IrqMode_Toggle )
					{
						// timer is in pulse mode //
						
						TheTimers [ TimerNumber ].MODE.IrqRequest = 1;
					}
				}
			//}
/*
#ifdef INLINE_DEBUG_RUN
			else
			{
				// there is an event, but there is not interrupt
				debug << " NoInterrupt TimerValue=" << dec << ( TheTimers [ TimerNumber ].COUNT.Count & 0xffff ) << " TimerTarget=" << TimerTarget;
			}
#endif
*/

			// update the timer - need to do this to get the correct next event cycle distance from the current cycle
			UpdateTimer ( TimerNumber );
			
			// get next event for timer
			Get_NextEvent ( TimerNumber, g->NextEvent_Cycle );
			
#ifdef INLINE_DEBUG_RUN
	debug << "; NextEvent_Cycle=" << dec << TheTimers [ TimerNumber ].NextEvent_Cycle;
#endif

		}
		
	}

}




static u32 Timers::Read ( u32 Address )
{
#ifdef INLINE_DEBUG_READ
	debug << "\r\nTimers::Read CycleCount=" << dec << *_DebugCycleCount << " Address=" << hex << Address;
#endif

	u32 TimerNumber, Output;
	
	// get the timer number
	TimerNumber = ( Address >> 4 ) & 0xf;	//0x3;
	
#ifdef PS2_COMPILE
	if ( ( Address >= 0x1f801480 && Address < 0x1f8014b0 ) )
	{
		TimerNumber -= 5;
	}
#endif
	
#ifdef PS2_COMPILE
	if ( ( Address >= 0x1f801100 && Address < 0x1f801130 ) || ( Address >= 0x1f801480 && Address < 0x1f8014b0 ) )
#else
	if ( ( Address >= 0x1f801100 && Address < 0x1f801130 ) )
#endif
	{
		switch ( Address & 0xf )
		{
			case 0:	// READ: COUNT
			
				// update timer //
				_TIMERS->UpdateTimer ( TimerNumber );
				
				// get the current value of the timer //
				//_TIMERS->TheTimers [ TimerNumber ].COUNT.Value = _TIMERS->GetTimerValue ( TimerNumber );
				_TIMERS->TheTimers [ TimerNumber ].COUNT.Value = _TIMERS->TheTimers [ TimerNumber ].StartValue;
				
#ifdef INLINE_DEBUG_READ
				debug << "; T" << dec << TimerNumber << "_COUNT = " << hex << _TIMERS->TheTimers [ TimerNumber ].COUNT.Value;
				debug << dec << " TicksPerCycle=" << _TIMERS->TheTimers [ TimerNumber ].dTicksPerCycle;
				debug << dec << " CyclePerTick=" << _TIMERS->TheTimers [ TimerNumber ].dCyclesPerTick;
				debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
				debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
				debug << dec << " OffsetCycles=" << _TIMERS->TheTimers [ TimerNumber ].dOffsetCycles;
#endif

				return _TIMERS->TheTimers [ TimerNumber ].COUNT.Value;
				break;
			
			case 4:	// READ: MODE
#if defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_MODE
				debug << "; T" << dec << TimerNumber << "_MODE = " << hex << _TIMERS->TheTimers [ TimerNumber ].MODE.Value;
				debug << dec << " TicksPerCycle=" << _TIMERS->TheTimers [ TimerNumber ].dTicksPerCycle;
				debug << dec << " CyclePerTick=" << _TIMERS->TheTimers [ TimerNumber ].dCyclesPerTick;
				debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
				debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
				debug << dec << " OffsetCycles=" << _TIMERS->TheTimers [ TimerNumber ].dOffsetCycles;
#endif

				// update timer before reading mode, to update previous flags (reached target/overflow flags, etc.) for reading
				_TIMERS->UpdateTimer ( TimerNumber );
			
				// get the value of MODE register
				//return _TIMERS->TheTimers [ TimerNumber ].MODE.Value;
				Output = _TIMERS->TheTimers [ TimerNumber ].MODE.Value;
				
				// from Nocash PSX Specifications
				// bits 11 and 12 get reset after reading MODE register
				_TIMERS->TheTimers [ TimerNumber ].MODE.Value &= ~( ( 1 << 11 ) | ( 1 << 12 ) );
				
				return Output;
				
				break;
			
			case 8:	// READ: COMP
#ifdef INLINE_DEBUG_READ
				debug << "; T" << dec << TimerNumber << "_COMP = " << hex << _TIMERS->TheTimers [ TimerNumber ].COMP.Value;
				debug << dec << " TicksPerCycle=" << _TIMERS->TheTimers [ TimerNumber ].dTicksPerCycle;
				debug << dec << " CyclePerTick=" << _TIMERS->TheTimers [ TimerNumber ].dCyclesPerTick;
				debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
				debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
				debug << dec << " OffsetCycles=" << _TIMERS->TheTimers [ TimerNumber ].dOffsetCycles;
#endif
			
				// get the value of COMP register
				return _TIMERS->TheTimers [ TimerNumber ].COMP.Value;
				break;
			
			default:
#ifdef INLINE_DEBUG_READ
				debug << "; Invalid";
#endif
			
				// invalid TIMER Register
				cout << "\nhps1x64 ALERT: Unknown TIMER READ @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << "\n";
				break;
		}
	}
	
}


static void Timers::Write ( u32 Address, u32 Data, u32 Mask )
{
#ifdef INLINE_DEBUG_WRITE
	debug << "\r\nTimers::Write CycleCount=" << dec << *_DebugCycleCount << " Address=" << hex << Address << " Data=" << Data;
#endif

	u32 TimerNumber;

	// *** testing *** check if mask is a word write
	if ( Mask != 0xffffffff && Mask != 0xffff )
	{
		cout << "\nhps1x64 ALERT: Timers::Write Mask=" << hex << Mask;
	}
	
	
	// get the timer number
	TimerNumber = ( Address >> 4 ) & 0xf;	//0x3;
	

#ifdef PS2_COMPILE
	if ( TimerNumber < 3 )
	{
#endif
	// *** TESTING ***
	// only need 16-bits
	// note: PS1 timers are "16-bit", but PS2 timers on IOP are actually 32-bit
	Data &= 0xffff;
#ifdef PS2_COMPILE
	}
#endif

	
	
#ifdef PS2_COMPILE
	if ( ( Address >= 0x1f801480 && Address < 0x1f8014b0 ) )
	{
		TimerNumber -= 5;
	}
#endif
	
#ifdef PS2_COMPILE
	if ( ( Address >= 0x1f801100 && Address < 0x1f801130 ) || ( Address >= 0x1f801480 && Address < 0x1f8014b0 ) )
#else
	if ( ( Address >= 0x1f801100 && Address < 0x1f801130 ) )
#endif
	{
		switch ( Address & 0xf )
		{
			case 0:	// WRITE: COUNT
			
				// write the new timer value //
				_TIMERS->SetValue ( TimerNumber, Data );
				
#ifdef INLINE_DEBUG_WRITE
				debug << "; T" << dec << TimerNumber << "_COUNT = " << hex << _TIMERS->TheTimers [ TimerNumber ].COUNT.Value;
				debug << dec << " TicksPerCycle=" << _TIMERS->TheTimers [ TimerNumber ].dTicksPerCycle;
				debug << dec << " CyclePerTick=" << _TIMERS->TheTimers [ TimerNumber ].dCyclesPerTick;
				debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
				debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
				debug << dec << " OffsetCycles=" << _TIMERS->TheTimers [ TimerNumber ].dOffsetCycles;
#endif

				break;
			
			case 4:	// WRITE: MODE
			
				// write new mode value
				_TIMERS->SetMode ( TimerNumber, Data );

				
#ifdef INLINE_DEBUG_WRITE
				debug << "; T" << dec << TimerNumber << "_MODE = " << hex << _TIMERS->TheTimers [ TimerNumber ].MODE.Value;
				//debug << "; T" << dec << TimerNumber << "_COUNT = " << hex << _TIMERS->TheTimers [ TimerNumber ].COUNT.Value;
				debug << dec << " TicksPerCycle=" << _TIMERS->TheTimers [ TimerNumber ].dTicksPerCycle;
				debug << dec << " CyclePerTick=" << _TIMERS->TheTimers [ TimerNumber ].dCyclesPerTick;
				debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
				debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
				debug << dec << " OffsetCycles=" << _TIMERS->TheTimers [ TimerNumber ].dOffsetCycles;
#endif

				break;
				
			case 8:	// WRITE: COMP
			
				_TIMERS->SetComp ( TimerNumber, Data );
				
#ifdef INLINE_DEBUG_WRITE
				debug << "; T" << dec << TimerNumber << "_COMP = " << hex << _TIMERS->TheTimers [ TimerNumber ].COMP.Value;
				debug << dec << " TicksPerCycle=" << _TIMERS->TheTimers [ TimerNumber ].dTicksPerCycle;
				debug << dec << " CyclePerTick=" << _TIMERS->TheTimers [ TimerNumber ].dCyclesPerTick;
				debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
				debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
				debug << dec << " OffsetCycles=" << _TIMERS->TheTimers [ TimerNumber ].dOffsetCycles;
#endif

				break;
			
			default:
#ifdef INLINE_DEBUG_WRITE
				debug << "; Invalid";
#endif
			
				// invalid TIMER Register
				cout << "\nhps1x64 ALERT: Unknown TIMER WRITE @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << " Data=" << Data << "\n";
				break;

		}
	}
	
}





// update what cycle the next event is at for this device
void Timer::SetNextEventCh_Cycle ( u64 Cycle )
{
	NextEvent_Cycle = Cycle;
	
	Timers::_TIMERS->Update_NextEventCycle ();
}

void Timer::SetNextEventCh ( u64 Cycle )
{
	NextEvent_Cycle = Cycle + *_DebugCycleCount;
	
	Timers::_TIMERS->Update_NextEventCycle ();
}


void Timers::Update_NextEventCycle ()
{
	// first need to initialize the next event cycle to an actual cycle number that currently exists
	//NextEvent_Cycle = TheTimers [ 0 ].NextEvent_Cycle;
	NextEvent_Cycle = -1LL;
	
	for ( int i = 0; i < c_iNumberOfChannels; i++ )
	{
		if ( TheTimers [ i ].NextEvent_Cycle > *_DebugCycleCount && ( TheTimers [ i ].NextEvent_Cycle < NextEvent_Cycle || NextEvent_Cycle <= *_DebugCycleCount ) )
		{
			// the next event is the next event for device
			NextEvent_Cycle = TheTimers [ i ].NextEvent_Cycle;
		}
	}

	if ( NextEvent_Cycle > *_DebugCycleCount && ( NextEvent_Cycle < *_NextSystemEvent || *_NextSystemEvent <= *_DebugCycleCount ) ) *_NextSystemEvent = NextEvent_Cycle;
}





// get number of ticks until you reach the next tick
u32 Timer::Get_NextIntTick ( u64 lStartTick )
{
	u64 lIntTick_Target, lIntTick_Overflow, lIntTick, lCompTick;
	u64 lCompare, lOverflow;
	
	// get the start tick //
	//lStartTick = StartValue & 0xffff;
	
	// get compare value //
	lCompare = (u32) COMP.Compare;
	lOverflow = 0x10000ULL;
	
#ifdef PS2_COMPILE
	if ( TimerNumber >= 3 )
	{
		// get compare value //
		lCompare = (u32) COMP.Value;
		lOverflow = 0x100000000ULL;
	}
#endif
	
	// get the compare tick //
	
	// check if counting to target //
	if ( MODE.CountToTarget )
	{
		// count to target //
		//lCompTick = ( (u32) COMP.Compare );
		lCompTick = lCompare;
	}
	else
	{
		// count to overflow //
		//lCompTick = ( (u32) 0xffff );
		lCompTick = lOverflow;
	}
	
	// get the next int tick //
	
	// set int tick to max
	//lIntTick_Target = 0xffffffff;
	lIntTick_Target = 0x100000000ULL;
	
	// check if irq is on target
	if ( MODE.IrqOnTarget )
	{
		// counter can always reach target //
		//lIntTick_Target = ( (u32) COMP.Compare );
		lIntTick_Target = lCompare;
		
		// check if start tick is on or after int tick, then add compare value to the end tick plus one
		if ( lStartTick >= lIntTick_Target )
		{
			lIntTick_Target += lCompTick + 1;
		}
	}
	
	// set int tick to max
	//lIntTick_Overflow = 0xffffffff;
	lIntTick_Overflow = 0x100000000ULL;
	
	// check if irq is on overflow
	if ( MODE.IrqOnOverflow )
	{
		// make sure counter can reach overflow
		//if ( lCompTick == 0xffff )
		if ( lCompTick == lOverflow )
		{
			lIntTick_Overflow = lCompTick;
			
			// check if start tick is on or after end tick, then add compare value to the end tick plus one
			if ( lStartTick >= lIntTick_Overflow )
			{
				lIntTick_Overflow += lCompTick + 1;
			}
		}
	}
	
	// check which int tick comes next //
	if ( lIntTick_Target < lIntTick_Overflow )
	{
		NextIntType = INT_TYPE_TARGET;
		lIntTick = lIntTick_Target;
	}
	else
	{
		NextIntType = INT_TYPE_OVERFLOW;
		lIntTick = lIntTick_Overflow;
	}
	
	return lIntTick;
}


double Timer::Get_OffsetCycles ( u64 lStartCycle )
{
	double dOffsetCycles;

	// calculate the offset cycles //
	
	dOffsetCycles = 0.0L;
	
#ifdef PS2_COMPILE
	if ( TimerNumber > 3 )
	{
		switch ( MODE.ClockDiv )
		{
			case 0:
				// cycle counter ?? //
				dOffsetCycles = 0.0L;
				break;
				
			case 1:
				// 1/8 cycle counter ?? //
				dOffsetCycles = ( (double) ( lStartCycle & ( 8 - 1 ) ) );
				break;
			
			case 2:
				// 1/16 cycle counter ?? //
				dOffsetCycles = ( (double) ( lStartCycle & ( 16 - 1 ) ) );
				break;
				
			case 3:
				// 1/256 cycle counter ?? //
				dOffsetCycles = ( (double) ( lStartCycle & ( 256 - 1 ) ) );
				break;
		}
	}
#endif

	if ( MODE.ClockSource & 1 )
	{
		// NOT cycle counter //
		

		// get offset cycles (since gpu runs at a different frequency than CPU) //
		// check if timer 0 or 1 //
		switch ( TimerNumber )
		{
			case 0:
				// pixel counter //
				dOffsetCycles = g->GetCycles_SinceLastPixel ( (double) lStartCycle );
				break;
			
			case 1:
				// hblank counter //
				dOffsetCycles = g->GetCycles_SinceLastHBlank ( (double) lStartCycle );
				break;
				
			case 2:
				// timer 2 - cycle counter //
				break;
				
#ifdef PS2_COMPILE
			case 3:
				// hblank counter //
				dOffsetCycles = g->GetCycles_SinceLastHBlank ( (double) lStartCycle );
				break;
#endif

		}
	}
	
	if ( MODE.ClockSource & 2 )
	{
		// check if timer 2 //
		if ( TimerNumber == 2 )
		{
			// one-eigth cycle timer //
			
			// get offset cycles //
			dOffsetCycles = ( (double) ( lStartCycle & 7 ) );
		}
	}
	
	return dOffsetCycles;
}


u64 Timer::Get_FreeRunNextIntCycle ( u32 lStartValue, u64 lStartCycle )
{
	double dOffsetCycles;
	u32 lIntValue;
	u64 lIntCycle;
	
	// get next int tick //
	lIntValue = Get_NextIntTick ( lStartValue );
	
	// calculate the offset cycles //
	dOffsetCycles = Get_OffsetCycles ( lStartCycle );
	
	// get cycle at which the next interrupt is at for timer //
	
	// set the cycle next interrupt will take place at
	lIntCycle = lStartCycle + (u64) CEILD ( ( ( (double) ( lIntValue - lStartValue ) ) * dCyclesPerTick ) - dOffsetCycles );
	
	return lIntCycle;
}


// gets the next event for timer.. checks up to "ThroughCycle"
// *important* don't forget to re-check for the next event whenever timer/mode/comp gets set/updated
void Timer::Get_NextEvent ( u64 ThroughCycle )
{
	bool bBlank;
	double dOffsetCycles;
	u64 lStartCycle, lEndCycle, lCurrentCycle, lBlankCycle;
	u64 lIntCycle;
	u32 lStartValue, lEndValue;
	u32 lIntValue;
	
	// set the final cycle //
	lCurrentCycle = ThroughCycle;
	
	// set start cycle //
	lStartCycle = StartCycle;

#ifdef PS2_COMPILE
	if ( TimerNumber < 3 )
	{
#endif
	// get start tick //
	lStartValue = StartValue & 0xffffULL;
	
#ifdef PS2_COMPILE
	}
	else
	{
		lStartValue = StartValue & 0xffffffffULL;
	}
#endif
	
	// if in free run mode, or this is timer 2, then can calculate like free run
	if ( ( !MODE.RunMode ) || ( TimerNumber == 2 )
#ifdef PS2_COMPILE
		|| ( TimerNumber > 3 )
#endif
	)
	{
		// timer is in free run //
	
		// get the cycle of the next interrupt for free run timer //
		lIntCycle = Get_FreeRunNextIntCycle ( lStartValue, lStartCycle );
		
		// set the cycle for the next event for timer
		SetNextEventCh_Cycle ( lIntCycle );
		
		return;
	}
	else
	{
		// Timer 0 or 1 in sync mode //
		
		// note: don't wrap around timer so you can see where it hits a particular tick //
		
		while ( lStartCycle < lCurrentCycle )
		{
		
			// check if in blanking area //
			
			// check if timer 0 or 1 //
			if ( !TimerNumber )
			{
				// pixel counter //
				bBlank = g->isHBlank ( (double) lStartCycle );
			}
			else
			{
				// hblank counter //
				bBlank = g->isVBlank ( (double) lStartCycle );
			}
		
			// check if in blank //
			if ( bBlank )
			{
				// Blank //
				
				// get next end cycle //
				
				// check if this is timer 0 or 1
				if ( !TimerNumber )
				{
					// Timer 0 //
					
					// jump to next scanline
					lEndCycle = lStartCycle + (u64) CEILD ( g->GetCycles_ToNextScanlineStart ( (double) lStartCycle ) );
				}
				else
				{
					// Timer 1 //
					
					// jump to next field start
					lEndCycle = lStartCycle + (u64) CEILD ( g->GetCycles_ToNextFieldStart ( (double) lStartCycle ) );
				}
				
				// check the sync mode //
				// but here, we were already in blanking area, so count for modes except zero
				// both sync modes 0 and 3 are paused during blank, so only count for modes 1 and 2
				if ( MODE.SyncMode == 1 || MODE.SyncMode == 2 )
				{
					// calculate the offset cycles //
					dOffsetCycles = Get_OffsetCycles ( lStartCycle );
					
					// get next clock tick //
					lEndValue = lStartValue + (u64) ( ( ((double) ( lEndCycle - lStartCycle )) + dOffsetCycles ) * dTicksPerCycle );
					
					// get next int tick //
					// have to get this again each loop
					lIntValue = Get_NextIntTick ( lStartValue );
					
					// check if the next end tick is past the next int tick
					// must include the int value when comparing
					if ( lEndValue >= lIntValue )
					{
						// the interrupt is somewhere in here //
						lIntCycle = lStartCycle + (u64) CEILD ( ( ( (double) ( lIntValue - lStartValue ) ) * dCyclesPerTick ) - dOffsetCycles );
						
						// set cycle for next interrupt
						SetNextEventCh_Cycle ( lIntCycle );
						
						// done
						return;
					}
					
					// set the next tick
					lStartValue = lEndValue;
				}
			}
			else
			{
				// not in Blank //
				
				// get blank cycle and offset cycles //
				
				// check if this is timer 0 or 1
				if ( !TimerNumber )
				{
					// Timer 0 //
					// get the cycle at the next hblank
					lBlankCycle = lStartCycle + (u64) CEILD ( g->GetCycles_ToNextHBlank ( (double) lStartCycle ) );
				}
				else
				{
					// Timer 1 //
					// get the cycle at the next vblank
					lBlankCycle = lStartCycle + (u64) CEILD ( g->GetCycles_ToNextVBlank ( (double) lStartCycle ) );
				}
				
				// set as the end cycle //
				lEndCycle = lBlankCycle;
				
				// if not mode 3 and not mode 2, then update clock //
				// sync modes 2 and 3 are paused outside of blank //
				if ( MODE.SyncMode < 2 )
				{
					// calculate the offset cycles //
					dOffsetCycles = Get_OffsetCycles ( lStartCycle );
					
					// get next clock tick //
					lEndValue = lStartValue + (u64) ( ( ((double) ( lEndCycle - lStartCycle )) + dOffsetCycles ) * dTicksPerCycle );
					
					// get next int tick //
					// have to get this again each loop
					lIntValue = Get_NextIntTick ( lStartValue );
					
					// check if the next end tick is past the next int tick
					// must include the int value when comparing
					if ( lEndValue >= lIntValue )
					{
						// the interrupt is somewhere in here //
						lIntCycle = lStartCycle + (u64) CEILD ( ( ( (double) ( lIntValue - lStartValue ) ) * dCyclesPerTick ) - dOffsetCycles );
						
						// set cycle of next interrupt for timer
						SetNextEventCh_Cycle ( lIntCycle );
						
						// done
						return;
					}
					
					// set the next tick
					lStartValue = lEndValue;
				}
				
				// if the blanking area was just hit, then process //
				// when just reached blank, so for mode 0 do nothing, for mode 1 reset to zero, mode 2 reset to zero, mode 3 switch to free run
				if ( lEndCycle == lBlankCycle )
				{
					// just reached blanking area //
					
					switch ( MODE.SyncMode )
					{
						case 0:
							// pause counter during blank //
							
							// just reached blank and will pause in the code, so do nothing here //
							break;
							
						case 1:
						case 2:
						
							// reset counter to zero at blank //
							lStartValue = 0;
							break;
							
						case 3:
						
							// switch to free run at blank //
							//MODE.RunMode = 0;
							
							// update the start cycle for timer
							lStartCycle = lEndCycle;
							
							// calculate the rest of the cycles //
							lIntCycle = Get_FreeRunNextIntCycle ( lStartValue, lStartCycle );
							
							// set cycle of next interrupt for timer
							SetNextEventCh_Cycle ( lIntCycle );
							
							// done
							return;
							
							break;
					}
				
				}
			}
			
			// update cycle we are at //
			lStartCycle = lEndCycle;
		}
	}
	
	// unable to find next cycle timer should interrupt at for now
	SetNextEventCh_Cycle ( 0 );
}





void Timer::UpdateTimer_Wrap ()
{
	// check if reached/exceeded target //
	
#ifdef PS2_COMPILE
	if ( TimerNumber < 3 )
	{
#endif
	// check if counting to target //
	if ( MODE.CountToTarget )
	{
		if ( StartValue >= ( (u32) COMP.Compare ) )
		{
			// target was reached
			MODE.TargetReached = 1;
		}
		
		// this one is for strictly greater than
		if ( StartValue
#ifndef EXCLUDE_TARGET
		>
#else
		// if not including target then use this
		>=
#endif
		( (u32) COMP.Compare ) )
		{
			// only count to target, including target //
			StartValue %= ( ( (u32) COMP.Compare )
#ifndef EXCLUDE_TARGET
			// or maybe not including target ???
			+ 1
#endif
			);
		}
	}
#ifdef PS2_COMPILE
	}
	else
	{
		if ( MODE.CountToTarget )
		{
			if ( StartValue >= ( (u32) COMP.Value ) )
			{
				// target was reached
				MODE.TargetReached = 1;
			}
			
			// this one is for strictly greater than
			if ( StartValue
#ifndef EXCLUDE_TARGET
			>
#else
			>=
#endif
			( (u32) COMP.Value ) )
			{
				// only count to target, including target //
				StartValue %= ( ( (u32) COMP.Value )
#ifndef EXCLUDE_TARGET
				// or maybe not including target ???
				+ 1
#endif
				);
			}
		}
	}
#endif

	
	// check if overflow was reached //
	// ***todo*** might be a strictly greater than comparison here, I'll have to check this
#ifdef PS2_COMPILE
	if ( TimerNumber < 3 )
	{
#endif
	//if ( StartValue >= 0xffff )
	if ( StartValue > 0xffff )
	{
		
		// this one is for strictly greater than
		/*
		if ( StartValue > 0xffff )
		{
			// only count to overflow, including overflow //
			// *note* this should just be a bitwise AND
			StartValue %= ( 0xffff + 1 );
		}
		*/


		MODE.OverflowReached = 1;
		
		// wrap timer that counts to overflow (including 0xffff)
		StartValue &= 0xffff;
	}
#ifdef PS2_COMPILE
	}
	else
	{
		if ( StartValue > 0xffffffff )
		{
			MODE.OverflowReached = 1;
			
			StartValue &= 0xffffffff;
		}
	}
#endif
}


void Timer::Update_FreeRunTimer ()
{
	double dOffsetCycles;
	u64 lStartCycle, lEndCycle, lCurrentCycle, lBlankCycle;
	
	// timer is in free run //
	
	// get start cycle //
	lStartCycle = StartCycle;
	
	// get end cycle //
	lEndCycle = *_DebugCycleCount;
	
	
	// calculate the offset cycles //
	dOffsetCycles = Get_OffsetCycles ( lStartCycle );
	
	// update ticks //
	
	// need to add the offset cycles since hblank for example happens every scanline, but only at a certain point on the scanline
	StartValue += (u64) ( ( ( (double) ( lEndCycle - lStartCycle ) ) + dOffsetCycles ) * dTicksPerCycle );
	
	StartCycle = lEndCycle;
}


// the only way to properly simulate a PS1 timer it appears at this point is to update it per scanline... hblank is probably best
// update per scanline
void Timer::UpdateTimer ()
{
	bool bBlank;
	double dOffsetCycles;
	u64 lStartCycle, lEndCycle, lCurrentCycle, lBlankCycle;
	
	// note: timer should be updated before reading value/mode. Should also be updated periodically to maintain a good speed. per scanline or per frame is probably good.
	
	// if in free run mode, or this is timer 2, then can calculate like free run
	if ( ( !MODE.RunMode ) || ( TimerNumber == 2 )
#ifdef PS2_COMPILE
		|| ( TimerNumber > 3 )
#endif
	)
	{
		// timer is in free run //
		Update_FreeRunTimer ();
	}
	else
	{
		// Timer 0 or 1 in sync mode //
		
		// get the current cycle //
		lCurrentCycle = *_DebugCycleCount;
		
		// get the start cycle //
		lStartCycle = StartCycle;
		
		while ( lStartCycle < lCurrentCycle )
		{
		
			// check if in blanking area //
			
			// check if timer 0 or 1 //
			if ( !TimerNumber )
			{
				// pixel counter //
				bBlank = g->isHBlank ( (double) lStartCycle );
			}
			else
			{
				// hblank counter //
				bBlank = g->isVBlank ( (double) lStartCycle );
			}
		
			// check if in blank //
			if ( bBlank )
			{
				// Blank //
				
				// get next end cycle //
				
				// check if this is timer 0 or 1
				if ( !TimerNumber )
				{
					// Timer 0 //
					
					// jump to next scanline
					lEndCycle = lStartCycle + (u64) CEILD ( g->GetCycles_ToNextScanlineStart ( (double) lStartCycle ) );
				}
				else
				{
					// Timer 1 //
					
					// jump to next field start
					lEndCycle = lStartCycle + (u64) CEILD ( g->GetCycles_ToNextFieldStart ( (double) lStartCycle ) );
				}
				
				
				// check if passed the current cycle
				if ( lEndCycle > lCurrentCycle )
				{
					// do not pass the current cycle //
					lEndCycle = lCurrentCycle;
				}
				
				// check the sync mode //
				// but here, we were already in blanking area, so count for modes except zero
				// both sync modes 0 and 3 are paused during hblank, so only count for modes 1 and 2
				if ( MODE.SyncMode == 1 || MODE.SyncMode == 2 )
				{
					// calculate the offset cycles //
					dOffsetCycles = Get_OffsetCycles ( lStartCycle );
					
					// update clock ticks //
					StartValue += (u64) ( ( ((double) ( lEndCycle - lStartCycle )) + dOffsetCycles ) * dTicksPerCycle );
				}
			}
			else
			{
				// not in Blank //
				
				// get blank cycle and offset cycles //
				
				// check if this is timer 0 or 1
				if ( !TimerNumber )
				{
					// Timer 0 //
					// get the cycle at the next hblank
					lBlankCycle = lStartCycle + (u64) CEILD ( g->GetCycles_ToNextHBlank ( (double) lStartCycle ) );
				}
				else
				{
					// Timer 1 //
					// get the cycle at the next vblank
					lBlankCycle = lStartCycle + (u64) CEILD ( g->GetCycles_ToNextVBlank ( (double) lStartCycle ) );
				}
				
				// set as the end cycle //
				lEndCycle = lBlankCycle;
				
				// check if passed the current cycle
				if ( lEndCycle > lCurrentCycle )
				{
					// do not pass the current cycle //
					lEndCycle = lCurrentCycle;
				}
				
				// if not mode 3 and not mode 2, then update clock //
				// sync modes 2 and 3 are paused outside of blank //
				if ( MODE.SyncMode < 2 )
				{
					// get the offset cycles //
					dOffsetCycles = Get_OffsetCycles ( lStartCycle );
					
					// update clock ticks //
					StartValue += (u64) ( ( ((double) ( lEndCycle - lStartCycle )) + dOffsetCycles ) * dTicksPerCycle );
				}
				
				// if the blanking area was just hit, then process //
				// when just reached blank, so for mode 0 do nothing, for mode 1 reset to zero, mode 2 reset to zero, mode 3 switch to free run
				if ( lEndCycle == lBlankCycle )
				{
					switch ( MODE.SyncMode )
					{
						case 0:
							// pause counter during blank //
							
							// just reached blank and will pause in the code, so do nothing here //
							break;
							
						case 1:
						case 2:
						
							// reset counter to zero at blank //
							StartValue = 0;
							break;
							
						case 3:
						
							// switch to free run at blank //
							MODE.RunMode = 0;
							
							// update the start cycle for timer
							StartCycle = lEndCycle;
							
							// calculate the rest of the cycles //
							Update_FreeRunTimer ();
							
							// set the end cycle //
							lEndCycle = StartCycle;
							
							break;
					}
				
				}
			}
			
			// update cycle we are at //
			lStartCycle = lEndCycle;
		}
		
		// set the start cycle as the current cycle //
		StartCycle = *_DebugCycleCount;
	}
	
	// wrap timer value around and update associated flags
	UpdateTimer_Wrap ();
}



// updates ticks per cycle and offset cycles from last tick
void Timer::CalibrateTimer ()
{
#ifdef INLINE_DEBUG_CALIBRATE
	Timers::debug << "\r\nTimers::CalibrateTimer Timer#" << dec << TimerNumber;
#endif

	// check timer number
	switch ( TimerNumber )
	{
		case 0:
		
			// pixel clock //
			
			// check the clock source
			if ( MODE.ClockSource & 1 )
			{
				// dot clock //
				
				// get cycles since the last pixel and put into offset
				// set offset
				dOffsetCycles = g->GetCycles_SinceLastPixel ();
				
				// set cycles per pixel
				dCyclesPerTick = g->dCyclesPerPixel;
				//dTicksPerCycle = 1.0L / g->dCyclesPerPixel;
				dTicksPerCycle = g->dPixelsPerCycle;
			}
			else
			{
				// system clock //
				
				// get cycles since the last pixel and put into offset
				// set offset
				dOffsetCycles = 0.0L;
				
				// set cycles per pixel
				dCyclesPerTick = 1.0L;
				dTicksPerCycle = 1.0L;
			}
			
			break;
			
		case 1:
#ifdef PS2_COMPILE
		case 3:
#endif
		
			// hblank clock //
			
			// check the clock source
			if ( MODE.ClockSource & 1 )
			{
				// hblank clock //
				
				// get cycles since the last hblank and put into offset
				// set offset
				dOffsetCycles = g->GetCycles_SinceLastHBlank ();
				
				// set cycles per hblank
				dCyclesPerTick = g->dCyclesPerScanline;
				//dTicksPerCycle = 1.0L / g->dCyclesPerScanline;
				dTicksPerCycle = g->dScanlinesPerCycle;
			}
			else
			{
				// system clock //
				
				// get cycles since the last pixel and put into offset
				// set offset
				dOffsetCycles = 0.0L;
				
				// set cycles per pixel
				dCyclesPerTick = 1.0L;
				dTicksPerCycle = 1.0L;
			}
			
			
			break;
			
		case 2:
		
			// 1/8 system clock //
			
			// check if timer is in sync mode 0 or 3
			if ( MODE.RunMode && ( MODE.SyncMode == 0 || MODE.SyncMode == 3 ) )
			{
				// timer 2 is stopped //
				dTicksPerCycle = 0.0L;
				dCyclesPerTick = 0.0L;
				dOffsetCycles = 0.0L;
			}
			else
			{
				// timer 2 is in free run //
				
				// check the clock source
				if ( MODE.ClockSource & 2 )
				{
					// 1/8 system clock //
					
					dTicksPerCycle = 1.0L / 8.0L;
					dCyclesPerTick = 8.0L;
					
					// get cycles since last tick
					dOffsetCycles = (double) ( *_DebugCycleCount & 7 );
				}
				else
				{
					// system clock //
					
					// get cycles since the last pixel and put into offset
					// set offset
					dOffsetCycles = 0.0L;
					
					// set cycles per pixel
					dCyclesPerTick = 1.0L;
					dTicksPerCycle = 1.0L;
				}
			}
			
			break;
			
#ifdef PS2_COMPILE
		case 4:
		case 5:
			
			// check if timer is in sync mode 0 or 3
			if ( MODE.RunMode && ( MODE.SyncMode == 0 || MODE.SyncMode == 3 ) )
			{
				// timer 4 or 5 is stopped //
				dTicksPerCycle = 0.0L;
				dCyclesPerTick = 0.0L;
				dOffsetCycles = 0.0L;
			}
			else
			{
				switch ( MODE.ClockDiv )
				{
					case 0:
						// cycle counter //
						dCyclesPerTick = 1.0L;
						dTicksPerCycle = 1.0L;
						break;
						
					case 1:
						// 1/8 cycle counter //
						dTicksPerCycle = 1.0L / 8.0L;
						dCyclesPerTick = 8.0L;
						break;
						
					case 2:
						// 1/16 cycle counter //
						dTicksPerCycle = 1.0L / 16.0L;
						dCyclesPerTick = 16.0L;
						break;
						
					case 3:
						// 1/256 cycle counter //
						dTicksPerCycle = 1.0L / 256.0L;
						dCyclesPerTick = 256.0L;
						break;
				}
			}
#endif
	}

	// if calibration changed, then next event for timer might change
	// *note* don't make call from here
	//GetNextEvent ( TimerNumber );
}





void Timer::SetMode( u32 Data )
{
	//u64 ThroughCycle;
	
	// update timer using previous MODE value
	UpdateTimer ();

	// write new mode value
	MODE.Value = Data;
	
	// from Nocash PSX Specifications
	// writing to mode register sets bit 10 (InterruptRequest=No=1)
	MODE.Value |= ( 1 << 10 );
	

	// from Nocash PSX Specifications
	// writing to the mode register clears timer to zero
	COUNT.Value = 0;
	//SetTimerValue ( TimerNumber, 0 );
	StartValue = 0;
	StartCycle = *_DebugCycleCount;
	
	// reset irq counter
	IRQ_Counter = 0;
	
	// check for timer events up until next scanline transition
	//ThroughCycle = ( *_DebugCycleCount ) + (u64) CEILD ( g->GetCycles_ToNextScanlineStart ( (double) ( *_DebugCycleCount ) ) );
	
	// calibrate timer
	CalibrateTimer ();
	
	// get next interrupt event for timer
	Get_NextEvent ( g->NextEvent_Cycle );
}


void Timer::SetValue ( u32 Data )
{
	//u64 ThroughCycle;
	
	// write new timer value
	COUNT.Value = Data;

	StartValue = COUNT.Value;
	StartCycle = *_DebugCycleCount;
	
	// check for timer events up until next scanline transition
	//ThroughCycle = ( *_DebugCycleCount ) + (u64) CEILD ( g->GetCycles_ToNextScanlineStart ( (double) ( *_DebugCycleCount ) ) );
	
	// get next interrupt event for timer
	Get_NextEvent ( g->NextEvent_Cycle );

	// set timer start value
	// recalibrate timer
	//SetTimerValue ( TimerNumber, Data );
	
	// if the set value is equal or greater than compare value, alert //
	
	if ( Data > COMP.Compare )
	{
		cout << "\nhps1x64 ALERT: TIMER#" << TimerNumber << " is being manually set greater than compare value.\n";
	}
	
	//if ( Data == COMP.Compare )
	//{
	//	cout << "\nhps1x64 ALERT: TIMER#" << TimerNumber << " is being manually set equal to compare value.\n";
	//}
}


void Timer::SetComp ( u32 Data )
{
	//u64 ThroughCycle;
	
	// update timer using previous compare value
	UpdateTimer ();

	// write new compare value
	COMP.Value = Data;
	
	// *** todo *** is it needed to recalibrate timer and where interrupt should occur??
	
	// check for timer events up until next scanline transition
	//ThroughCycle = ( *_DebugCycleCount ) + (u64) CEILD ( g->GetCycles_ToNextScanlineStart ( (double) ( *_DebugCycleCount ) ) );
	
	// get next interrupt event for timer
	Get_NextEvent ( g->NextEvent_Cycle );
}










////////////// Debugging ///////////////////////////




static void Timers::DebugWindow_Enable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	static const char* DebugWindow_Caption = "PS1 Timer Debug Window";
	static const int DebugWindow_X = 10;
	static const int DebugWindow_Y = 10;
	static const int DebugWindow_Width = 200;
	static const int DebugWindow_Height = 200;
	
	static const int TimerList_X = 0;
	static const int TimerList_Y = 0;
	static const int TimerList_Width = 150;
	static const int TimerList_Height = 180;
	
	int i;
	stringstream ss;
	
	if ( !DebugWindow_Enabled )
	{
		// create the main debug window
		DebugWindow = new WindowClass::Window ();
		DebugWindow->Create ( DebugWindow_Caption, DebugWindow_X, DebugWindow_Y, DebugWindow_Width, DebugWindow_Height );
		DebugWindow->DisableCloseButton ();
		
		// create "value lists"
		Timer_ValueList = new DebugValueList<u32> ();
		Timer_ValueList->Create ( DebugWindow, TimerList_X, TimerList_Y, TimerList_Width, TimerList_Height, true, false );
		
		
		Timer_ValueList->AddVariable ( "T0_COUNT", & ( _TIMERS->TheTimers [ 0 ].COUNT.Value ) );
		Timer_ValueList->AddVariable ( "T0_COUNT2", (u32*) & ( _TIMERS->TheTimers [ 0 ].StartValue ) );
		Timer_ValueList->AddVariable ( "T0_MODE", & ( _TIMERS->TheTimers [ 0 ].MODE.Value ) );
		Timer_ValueList->AddVariable ( "T0_COMP", & ( _TIMERS->TheTimers [ 0 ].COMP.Value ) );
		Timer_ValueList->AddVariable ( "T1_COUNT", & ( _TIMERS->TheTimers [ 1 ].COUNT.Value ) );
		Timer_ValueList->AddVariable ( "T1_COUNT2", (u32*) & ( _TIMERS->TheTimers [ 1 ].StartValue ) );
		Timer_ValueList->AddVariable ( "T1_MODE", & ( _TIMERS->TheTimers [ 1 ].MODE.Value ) );
		Timer_ValueList->AddVariable ( "T1_COMP", & ( _TIMERS->TheTimers [ 1 ].COMP.Value ) );
		Timer_ValueList->AddVariable ( "T2_COUNT", & ( _TIMERS->TheTimers [ 2 ].COUNT.Value ) );
		Timer_ValueList->AddVariable ( "T2_COUNT2", (u32*) & ( _TIMERS->TheTimers [ 2 ].StartValue ) );
		Timer_ValueList->AddVariable ( "T2_MODE", & ( _TIMERS->TheTimers [ 2 ].MODE.Value ) );
		Timer_ValueList->AddVariable ( "T2_COMP", & ( _TIMERS->TheTimers [ 2 ].COMP.Value ) );
		
#ifdef PS2_COMPILE
		Timer_ValueList->AddVariable ( "T3_COUNT", & ( _TIMERS->TheTimers [ 3 ].COUNT.Value ) );
		Timer_ValueList->AddVariable ( "T3_COUNT2", (u32*) & ( _TIMERS->TheTimers [ 3 ].StartValue ) );
		Timer_ValueList->AddVariable ( "T3_MODE", & ( _TIMERS->TheTimers [ 3 ].MODE.Value ) );
		Timer_ValueList->AddVariable ( "T3_COMP", & ( _TIMERS->TheTimers [ 3 ].COMP.Value ) );
		Timer_ValueList->AddVariable ( "T4_COUNT", & ( _TIMERS->TheTimers [ 4 ].COUNT.Value ) );
		Timer_ValueList->AddVariable ( "T4_COUNT2", (u32*) & ( _TIMERS->TheTimers [ 4 ].StartValue ) );
		Timer_ValueList->AddVariable ( "T4_MODE", & ( _TIMERS->TheTimers [ 4 ].MODE.Value ) );
		Timer_ValueList->AddVariable ( "T4_COMP", & ( _TIMERS->TheTimers [ 4 ].COMP.Value ) );
		Timer_ValueList->AddVariable ( "T5_COUNT", & ( _TIMERS->TheTimers [ 5 ].COUNT.Value ) );
		Timer_ValueList->AddVariable ( "T5_COUNT2", (u32*) & ( _TIMERS->TheTimers [ 5 ].StartValue ) );
		Timer_ValueList->AddVariable ( "T5_MODE", & ( _TIMERS->TheTimers [ 5 ].MODE.Value ) );
		Timer_ValueList->AddVariable ( "T5_COMP", & ( _TIMERS->TheTimers [ 5 ].COMP.Value ) );
#endif
		
		// mark debug as enabled now
		DebugWindow_Enabled = true;
		
		// update the value lists
		DebugWindow_Update ();
	}
	
#endif

}

static void Timers::DebugWindow_Disable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	int i;
	
	if ( DebugWindow_Enabled )
	{
		delete DebugWindow;
		delete Timer_ValueList;
	
		// disable debug window
		DebugWindow_Enabled = false;
	}
	
#endif

}

static void Timers::DebugWindow_Update ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	int i;
	
	if ( DebugWindow_Enabled )
	{
		Timer_ValueList->Update();
	}
	
#endif

}



