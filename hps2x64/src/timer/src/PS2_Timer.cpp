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


//#include "types.h"

#include "PS2_Timer.h"
//#include <math.h>
//#include "Reciprocal.h"

#ifdef _DEBUG_VERSION_

// enable debugging
#define INLINE_DEBUG_ENABLE

/*
#define INLINE_DEBUG_READ
#define INLINE_DEBUG_WRITE
#define INLINE_DEBUG_RUN
*/

//#define INLINE_DEBUG_CALIBRATE
//#define INLINE_DEBUG_EVENT

//#define INLINE_DEBUG_RUN_VBLANK
//#define INLINE_DEBUG

#endif


using namespace Playstation2;
//using namespace Math::Reciprocal;

//#define CONVERT_TO_FIXED1PT63( x )	( ( (u64) ( ( x ) * ( ( (u64) 1 ) << 63 ) ) ) + ( 1 << 8 ) )

//const char* TimerNameList [ 4 ] = { "Timer0_Log.txt", "Timer1_Log.txt", "Timer2_Log.txt", "Timer3_Log.txt" };


static unsigned long long *Timer::_llCycleCount;
static unsigned long long *Timer::_llScanlineStart, *Timer::_llNextScanlineStart, *Timer::_llHBlankStart;
static unsigned long *Timer::_lScanline, *Timer::_lNextScanline, *Timer::_lVBlank_Y, *Timer::_lRasterYMax;

u32* Timer::_DebugPC;
u64* Timer::_DebugCycleCount;

u32* Timers::_DebugPC;
u64* Timers::_DebugCycleCount;


u64* Timers::_NextSystemEvent;

u32* Timers::_Intc_Stat;
u32* Timers::_Intc_Mask;
//u32* Timers::_Intc_Master;
u32* Timers::_R5900_Status_12;
u32* Timers::_R5900_Cause_13;
u64* Timers::_ProcStatus;


int Timer::Count = 0;
Debug::Log Timers::debug;

Timers *Timers::_TIMERS;


//GPU *Timer::g;
//GPU *Timers::g;


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



/*
void Timers::ConnectDevices ( Playstation1::GPU* _g )
{
	g = _g;
	Timer::g = _g;
}
*/


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
	debug.Create ( "PS2_Timers_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering Timers::Start";
#endif


	Reset ();
	
	_TIMERS = this;

	
	cout << "\nSetting timer numbers";
	
	// set the timer numbers
	TheTimers [ 0 ].Set_TimerNumber ( 0, 0 );
	TheTimers [ 1 ].Set_TimerNumber ( 1, 1 );
	TheTimers [ 2 ].Set_TimerNumber ( 2, 2 );
	TheTimers [ 3 ].Set_TimerNumber ( 3, 3 );
	
	
	cout << "\nCalibrating timers";
	
	// calibrate the timers
	CalibrateTimer ( 0 );
	CalibrateTimer ( 1 );
	CalibrateTimer ( 2 );
	CalibrateTimer ( 3 );
	
	cout << "\nClearing timer values";
	
	// set the timer values
	//SetTimerValue ( 0, 0 );
	//SetTimerValue ( 1, 0 );
	//SetTimerValue ( 2, 0 );
	SetValue ( 0, 0 );
	SetValue ( 1, 0 );
	SetValue ( 2, 0 );
	SetValue ( 3, 0 );
	
	// get the next event for each timer (should be none here)
	//GetNextEvent ( 0 );
	//GetNextEvent ( 1 );
	//GetNextEvent ( 2 );
	
	cout << "->Exiting Timers::Start";
	
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

			// check what the event is thats supposed to happen
			switch ( TheTimers [ TimerNumber ].EventType )
			{
				case Timer::TIMER_COMPARE_INT:
				
					// generate a timer interrupt for a compare event
					SetInterrupt ( TimerNumber );
					
					// set the compare interrupt generated flag
					TheTimers [ TimerNumber ].MODE.IrqOnTarget_Generated = 1;
					
					// update the timer
					TheTimers [ TimerNumber ].cbUpdate ( & ( TheTimers [ TimerNumber ] ) );
					
					// get the cycle number for the next event after this one
					TheTimers [ TimerNumber ].cbGetNextEvent ( & ( TheTimers [ TimerNumber ] ) );
					
					break;
					
				case Timer::TIMER_OVERFLOW_INT:
				
					// generate a timer interrupt for an overflow event
					SetInterrupt ( TimerNumber );
					
					// set the overflow interrupt generated flag
					TheTimers [ TimerNumber ].MODE.IrqOnOverflow_Generated = 1;
					
					// update the timer
					TheTimers [ TimerNumber ].cbUpdate ( & ( TheTimers [ TimerNumber ] ) );
					
					// get the cycle number for the next event after this one
					TheTimers [ TimerNumber ].cbGetNextEvent ( & ( TheTimers [ TimerNumber ] ) );
					
					break;
					
			}

			/*
			// make sure value is target - ONLY COMPARE 16-BITS!!
			//if ( ( TimerValue == TimerTarget && TheTimers [ TimerNumber ].MODE.IrqOnTarget ) || ( TimerValue == 0xffff && TheTimers [ TimerNumber ].MODE.IrqOnOverflow ) )
			//{
				// *** TODO *** still need to check which interrupt is enabled.. int on target or int on overflow
				// *** TODO *** also need to check if is a one-shot or repeat interrupt
				
				// check if this is a repeated interrupt or single interrupt on first signal
				if ( TheTimers [ TimerNumber ].MODE.IrqMode_Repeat || ( !TheTimers [ TimerNumber ].MODE.IrqMode_Repeat && !TheTimers [ TimerNumber ].IRQ_Counter ) )
				{
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
	debug << "; INT Timer#" << TimerNumber;
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

			// update the timer - need to do this to get the correct next event cycle distance from the current cycle
			UpdateTimer ( TimerNumber );
			
			// get next event for timer
			Get_NextEvent ( TimerNumber, g->NextEvent_Cycle );
			
		*/
			
#ifdef INLINE_DEBUG_RUN
	debug << "; NextEvent_Cycle=" << dec << TheTimers [ TimerNumber ].NextEvent_Cycle;
#endif

		}
		
	}

}




static u64 Timers::Read ( u32 Address, u64 Mask )
{
#ifdef INLINE_DEBUG_READ
	debug << "\r\nTimers::Read CycleCount=" << dec << *_DebugCycleCount << " Address=" << hex << Address;
#endif

	u32 TimerNumber, Output;
	
	// get the timer number (fixed for ps2)
	TimerNumber = ( Address >> 11 ) & 3;
	
	// switch fixed for ps2
	switch ( ( Address >> 4 ) & 0xf )
	{
		case 0:	// READ: COUNT
		
			// update timer //
			_TIMERS->UpdateTimer ( TimerNumber );
			
			// get the current value of the timer //
			//_TIMERS->TheTimers [ TimerNumber ].COUNT.Value = _TIMERS->GetTimerValue ( TimerNumber );
			_TIMERS->TheTimers [ TimerNumber ].COUNT.Value = _TIMERS->TheTimers [ TimerNumber ].StartValue;
			
#ifdef INLINE_DEBUG_READ
			//if ( TimerNumber == 3 )
			//{
			debug << "; T" << dec << TimerNumber << "_COUNT = " << hex << _TIMERS->TheTimers [ TimerNumber ].COUNT.Value;
			debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
			debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
			//}
#endif

			return _TIMERS->TheTimers [ TimerNumber ].COUNT.Value;
			break;
			
		case 1:	// READ: MODE
#ifdef INLINE_DEBUG_READ
			//if ( TimerNumber == 3 )
			//{
			debug << "; T" << dec << TimerNumber << "_MODE = " << hex << _TIMERS->TheTimers [ TimerNumber ].MODE.Value;
			debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
			debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
			//}
#endif

			// update timer before reading mode, to update previous flags (reached target/overflow flags, etc.) for reading
			// for PS2, the "reached" flags only update on overflow, so no need to update when reading mode
			//_TIMERS->UpdateTimer ( TimerNumber );
		
			// get the value of MODE register
			//return _TIMERS->TheTimers [ TimerNumber ].MODE.Value;
			Output = _TIMERS->TheTimers [ TimerNumber ].MODE.Value;
			
			// from Nocash PSX Specifications
			// bits 11 and 12 get reset after reading MODE register
			// not for PS2
			//_TIMERS->TheTimers [ TimerNumber ].MODE.Value &= ~( ( 1 << 11 ) | ( 1 << 12 ) );
			
			
			return Output;
			
			break;
			
		case 2:	// READ: COMP
#ifdef INLINE_DEBUG_READ
			//if ( TimerNumber == 3 )
			//{
			debug << "; T" << dec << TimerNumber << "_COMP = " << hex << _TIMERS->TheTimers [ TimerNumber ].COMP.Value;
			debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
			debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
			//}
#endif
		
			// get the value of COMP register
			return _TIMERS->TheTimers [ TimerNumber ].COMP.Value;
			break;
			
		case 3:	// READ: HOLD
#ifdef INLINE_DEBUG_READ
			//if ( TimerNumber == 3 )
			//{
			debug << "; T" << dec << TimerNumber << "_HOLD = " << hex << _TIMERS->TheTimers [ TimerNumber ].HOLD.Value;
			debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
			debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
			//}
#endif
		
			// only for timers 0 and 1
			if ( TimerNumber <= 1 )
			{
				return _TIMERS->TheTimers [ TimerNumber ].HOLD.Value;
			}
			
			// otherwise
			// invalid TIMER Register
			cout << "\nhps2x64 ALERT: Unknown TIMER READ @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << "\n";
			
			break;
			
		default:
#ifdef INLINE_DEBUG_READ
			debug << "; Invalid";
#endif
		
			// invalid TIMER Register
			cout << "\nhps2x64 ALERT: Unknown TIMER READ @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << "\n";
			break;
	}
}


static void Timers::Write ( u32 Address, u64 Data, u64 Mask )
{
#ifdef INLINE_DEBUG_WRITE
	debug << "\r\nTimers::Write CycleCount=" << dec << *_DebugCycleCount << " Address=" << hex << Address << " Data=" << Data;
#endif

	u32 TimerNumber;

	// *** testing *** check if mask is a word write
	//if ( Mask != 0xffffffff && Mask != 0xffff )
	//{
	//	cout << "\nhps2x64 ALERT: Timers::Write Mask=" << hex << Mask;
	//}
	
	// *** TESTING ***
	// only need 16-bits
	Data &= 0xffff;
	
	// get the timer number (fixed for PS2)
	TimerNumber = ( Address >> 11 ) & 3;
	
	switch ( ( Address >> 4 ) & 0xf )
	{
		case 0:	// WRITE: COUNT
		
			// write the new timer value //
			_TIMERS->SetValue ( TimerNumber, Data );
			
#ifdef INLINE_DEBUG_WRITE
			debug << "; T" << dec << TimerNumber << "_COUNT = " << hex << _TIMERS->TheTimers [ TimerNumber ].COUNT.Value;
			debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
			debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
#endif

			break;
			
		case 1:	// WRITE: MODE
		
			// write new mode value
			_TIMERS->SetMode ( TimerNumber, Data );

			// clear irq on compare/overflow reached flags when a 1 is written to them (bits 10 and 11)
			_TIMERS->TheTimers [ TimerNumber ].MODE.Value &= ~( Data & ( 3 << 10 ) );
			
#ifdef INLINE_DEBUG_WRITE
			debug << "; T" << dec << TimerNumber << "_MODE = " << hex << _TIMERS->TheTimers [ TimerNumber ].MODE.Value;
			debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
			debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
#endif

			break;
			
		case 2:	// WRITE: COMP
		
			_TIMERS->SetComp ( TimerNumber, Data );
			
#ifdef INLINE_DEBUG_WRITE
			debug << "; T" << dec << TimerNumber << "_COMP = " << hex << _TIMERS->TheTimers [ TimerNumber ].COMP.Value;
			debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
			debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
#endif

			break;
			
			
		case 3:	// WRITE: HOLD
		
			// only for timers 0 and 1
			if ( TimerNumber <= 1 )
			{
				_TIMERS->TheTimers [ TimerNumber ].HOLD.Value = Data;
			}
			else
			{
				// otherwise
				// invalid TIMER Register
				cout << "\nhps2x64 ALERT: Unknown TIMER WRITE @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << " Data=" << Data << "\n";
			}
			
#ifdef INLINE_DEBUG_WRITE
			debug << "; T" << dec << TimerNumber << "_COMP = " << hex << _TIMERS->TheTimers [ TimerNumber ].HOLD.Value;
			debug << dec << " StartValue=" << _TIMERS->TheTimers [ TimerNumber ].StartValue;
			debug << dec << " StartCycle=" << _TIMERS->TheTimers [ TimerNumber ].StartCycle;
#endif

			break;
			
		default:
#ifdef INLINE_DEBUG_WRITE
			debug << "; Invalid";
#endif
		
			// invalid TIMER Register
			cout << "\nhps2x64 ALERT: Unknown TIMER WRITE @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << " Data=" << Data << "\n";
			break;

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




/*
// get number of ticks until you reach the next tick
u32 Timer::Get_NextIntTick ( u32 lStartTick )
{
	u32 lIntTick_Target, lIntTick_Overflow, lIntTick, lCompTick;
	u32 lCompare;
	
	// get the start tick //
	//lStartTick = StartValue & 0xffff;
	
	// get compare value //
	lCompare = (u32) COMP.Compare;
	
	// get the compare tick //
	
	// check if counting to target //
	if ( MODE.CountToTarget )
	{
		// count to target //
		lCompTick = ( (u32) COMP.Compare );
	}
	else
	{
		// count to overflow //
		lCompTick = ( (u32) 0xffff );
	}
	
	// get the next int tick //
	
	// set int tick to max
	lIntTick_Target = 0xffffffff;
	
	// check if irq is on target
	if ( MODE.IrqOnTarget )
	{
		// counter can always reach target //
		lIntTick_Target = ( (u32) COMP.Compare );
		
		// check if start tick is on or after int tick, then add compare value to the end tick plus one
		if ( lStartTick >= lIntTick_Target )
		{
			lIntTick_Target += lCompTick + 1;
		}
	}
	
	// set int tick to max
	lIntTick_Overflow = 0xffffffff;
	
	// check if irq is on overflow
	if ( MODE.IrqOnOverflow )
	{
		// make sure counter can reach overflow
		if ( lCompTick == 0xffff )
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
	if ( lIntTick_Target < lIntTick_Overflow ) lIntTick = lIntTick_Target; else lIntTick = lIntTick_Overflow;
	
	return lIntTick;
}


double Timer::Get_OffsetCycles ( u64 lStartCycle )
{
	double dOffsetCycles;

	// calculate the offset cycles //
	dOffsetCycles = 0.0L;
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
*/



// updates ticks per cycle and offset cycles from last tick
void Timer::CalibrateTimer ()
{
#ifdef INLINE_DEBUG_CALIBRATE
	debug << "\r\nTimers::CalibrateTimer Timer#" << dec << TimerNumber;
#endif

	switch ( MODE.CounterEnable )
	{
		case 0:
			CalibrateTimer1 <0> ();
			break;
			
		case 1:
			CalibrateTimer1 <1> ();
			break;
		
	}

}





void Timer::SetMode( u32 Data )
{
	//u64 ThroughCycle;
	
	// update timer using previous MODE value
	//UpdateTimer ();
	cbUpdate ( this );

	// write new mode value
	MODE.Value = Data;
	
	// from Nocash PSX Specifications
	// writing to mode register sets bit 10 (InterruptRequest=No=1)
	//MODE.Value |= ( 1 << 10 );
	

	// from Nocash PSX Specifications
	// writing to the mode register clears timer to zero
	COUNT.Value = 0;
	//SetTimerValue ( TimerNumber, 0 );
	StartValue = 0;
	StartCycle = *_DebugCycleCount;
	
	// reset irq counter
	IRQ_Counter = 0;
	
	
	// calibrate timer
	CalibrateTimer ();
	
	// get next interrupt event for timer
	//Get_NextEvent ( g->NextEvent_Cycle );
	cbGetNextEvent ( this );
}


void Timer::SetValue ( u32 Data )
{
	//u64 ThroughCycle;
	
	
	// write new timer value
	COUNT.Value = Data;

	
	StartValue = COUNT.Value;
	StartCycle = *_DebugCycleCount;
	
	
	// get next interrupt event for timer
	//Get_NextEvent ( g->NextEvent_Cycle );
	cbGetNextEvent ( this );

	// set timer start value
	// recalibrate timer
	//SetTimerValue ( TimerNumber, Data );
	
	// if the set value is equal or greater than compare value, alert //
	
	if ( Data > COMP.Compare )
	{
		cout << "\nhps2x64 ALERT: TIMER#" << TimerNumber << " is being manually set greater than compare value.\n";
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
	//UpdateTimer ();
	cbUpdate ( this );

	// write new compare value
	COMP.Value = Data;
	
	// *** todo *** is it needed to recalibrate timer and where interrupt should occur??
	
	// get next interrupt event for timer
	//Get_NextEvent ( g->NextEvent_Cycle );
	cbGetNextEvent ( this );
}




void Timers::ReCalibrateAll ()
{
	for ( int i = 0; i < c_iNumberOfChannels; i++ )
	{
		CalibrateTimer ( i );
	}
}






////////////// Debugging ///////////////////////////




static void Timers::DebugWindow_Enable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	static const char* DebugWindow_Caption = "PS2 Timer Debug Window";
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
		Timer_ValueList->AddVariable ( "T3_COUNT", & ( _TIMERS->TheTimers [ 3 ].COUNT.Value ) );
		Timer_ValueList->AddVariable ( "T3_COUNT2", (u32*) & ( _TIMERS->TheTimers [ 3 ].StartValue ) );
		Timer_ValueList->AddVariable ( "T3_MODE", & ( _TIMERS->TheTimers [ 3 ].MODE.Value ) );
		Timer_ValueList->AddVariable ( "T3_COMP", & ( _TIMERS->TheTimers [ 3 ].COMP.Value ) );
		
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



