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


#ifndef _PS2_TIMER_H_
#define _PS2_TIMER_H_


#include "types.h"

#include "Debug.h"

//#include "PS2_Intc.h"
//#include "PS2_Gpu.h"

#include "DebugValueList.h"


namespace Playstation2
{

	

	class Timer
	{
	
		//Debug::Log debug;
	
	public:
	
		// maximum readable timer value
		static const long c_iMaxTimerValue = 0xffff;
		
		// the internal timer value at overflow (before getting reset to zero)
		static const long c_iTimerValueAtOverflow = 0x10000;
		
		static unsigned long long *_llCycleCount;
		static unsigned long long *_llScanlineStart, *_llNextScanlineStart, *_llHBlankStart;
		static unsigned long *_lScanline, *_lNextScanline, *_lVBlank_Y, *_lRasterYMax;
	
		// timers 0 and 1 get a signal from the GPU for pixel/hblank
		//static GPU *g;
		
		
		static int Count;
		
		
	
		int Index;
		u32 TimerNumber;
		
		union COUNT_Format
		{
			struct
			{
				// this is the current counter value
				// bits 0-15
				u16 Count;
				
				// this is garbage data
				// bits 16-31
				u16 Garbage;
			};
			
			u32 Value;
		};
		
		// the counter value - read only
		COUNT_Format COUNT;

		union MODE_Format
		{
			struct
			{
				// bits 0-1: Clock select
				// 00: Bus clk (147.456+MHZ); 01: 1/16 Bus clk; 10: 1/256 Bus clk; 11: External clk (h-blank)
				u32 ClkSelect : 2;
				
				// bit 2: Enable gate function
				// 0: Gate function disabled; 1: Gate function enabled
				u32 Gate : 1;
				
				// bit 3: select gate
				// 0: h-blank (disabled when ClkSelect is 11); 1: v-blank
				u32 GateSelect : 1;
				
				// bit 4-5: Gate mode
				// 00: Counts while gate signal low; 01: resets and starts counting at gate signal rising edge
				// 10: Resets and starts counting at gate signal falling edge; 11: resets and starts counting at both edges of gate signal
				u32 GateMode : 2;
				
				// bit 6: Zero Return
				// 0: The counter ignores the compare value; 1: counter is reset when it reaches compare value
				u32 CompareEnable : 1;
				
				// bit 7: Counter enable
				// 0: Stops the counter; 1: starts/restarts counter
				u32 CounterEnable : 1;
				
				// bit 8: interrupt on target reached
				// 0: disable interrupt on target reached; 1: interrupt is generated when target is reached
				u32 IrqOnTarget : 1;
				
				// bit 9: interrupt on overflow reached
				// 0: disable interrupt on overflow reached; 1: interrupt is generated when overflow is reached
				u32 IrqOnOverflow : 1;
				
				// bit 10: interrupt on target reached
				// 0: target interrupt not generated; 1: target interrupt generated
				// clear flag by writing 1
				u32 IrqOnTarget_Generated : 1;
				
				// bit 11: interrupt on overflow reached
				// 0: overflow interrupt not generated; 1: overflow interrupt generated
				// clear flag by writing 1
				u32 IrqOnOverflow_Generated : 1;
			};
			
			u32 Value;
		};
		
		// the counter mode - r/w
		MODE_Format MODE;

		
		union COMP_Format
		{
			struct
			{
				// this is the current compare value - bits 0-15
				u16 Compare;
				
				// this is garbage data - bits 16-31
				u16 Garbage;
			};
			
			u32 Value;
		};

		
		// the counter compare value - read and write
		COMP_Format COMP;
		
		
		union HOLD_Format
		{
			struct
			{
				// this is the current compare value - bits 0-15
				u16 Hold;
				
				// this is garbage data - bits 16-31
				u16 Garbage;
			};
			
			u32 Value;
		};
		
		HOLD_Format HOLD;
		
		
		// also need to know more stuff about the timer
		
		// timer status - like free run, wait till next hblank, etc
		//enum { TIMER_FREERUN = 0, TIMER_SYNC_RUN, TIMER_SYNC_WAIT, TIMER_SYNC_STOPPED };
		//u32 TimerStatus;
		
		u64 IRQ_Counter;
		
		// value the timer started at with the cycle number it started at that value
		u64 StartValue;
		u64 StartCycle;
		
		// need to know when the next blanking starts for a timer in sync mode
		//u64 NextBlankingStart_Cycle;
		
		// cycles to add before dividing
		//double dOffsetCycles;
		
		// these two are in 32.32 fixed point
		// these actually need to be double-precision floating point
		//double dCyclesPerTick;
		//double dTicksPerCycle;
		
		// this is when the next known event or interrupt happens
		u64 NextEvent_Cycle;
		
		
		typedef void (*UpdateFn) ( Timer* t );
		typedef void (*GetNextEventFn) ( Timer* t );
		
		// *note* these would have to be reloaded whenever restoring a save state
		UpdateFn cbUpdate;
		GetNextEventFn cbGetNextEvent;
		
		// ***todo*** here would need to set the call back functions above
		void AfterSaveStateLoad ();
		
		// sets the call back functions
		void SetCB ();
		
		u32 EventType;
		enum { TIMER_COMPARE_INT, TIMER_OVERFLOW_INT, TIMER_COMPARE, TIMER_OVERFLOW };
		
		// constructor - pass timer number 0-3, and ref to parent object
		Timer ();
		
		// reset the timer - use when creating timer or resetting system
		void Reset ();

		// "TimerIndex" just specifies the index of the timer in the array of timers for the collection of timers it is in
		// for standalone PS1, "TimerNumber" is 0 for pixel timer, 1 for HBlank Timer, 2 for Cycle timer
		void Set_TimerNumber ( int TimerIndex, u32 TimerNumber );
		
		// sets/calculates timer interval
		void CalibrateTimer ();
		
		inline static u64 Get_NextIntCycle_Clock ( Timer* t, u64 llStartCycle, long lStartTick, long lNextIntTick );
		
		// this updates the timer value
		template<const int MODE_CounterEnable, const int MODE_ClkSelect, const int MODE_Gate, const int MODE_GateSelect, const int MODE_GateMode>
		static void UpdateTimer ( Timer *t );
		
		// this gets the next event cycle for the timer, but may possibly only calculate up until "ThroughCycle"
		template<const int MODE_CounterEnable, const int MODE_ClkSelect, const int MODE_Gate, const int MODE_GateSelect, const int MODE_GateMode>
		static void Get_NextEvent ( Timer *t );
		
		inline long GetValue () { cbUpdate (this); return StartValue; }
		inline long GetMode () { return MODE.Value; }
		inline long GetComp () { return COMP.Value; }
		
		void SetMode ( u32 Data );
		void SetValue ( u32 Data );
		void SetComp ( u32 Data );
		
		
		////////////////////////////////
		// Debug Info
		static u32* _DebugPC;
		static u64* _DebugCycleCount;

		
	private:
		// update what cycle the next event is at for this device
		void SetNextEventCh_Cycle ( u64 Cycle );
		void SetNextEventCh ( u64 Cycle );
		
		// gets the next tick at which interrupt occurs for specified timer (does not wrap tick value)
		//u32 Get_NextIntTick ( u32 lStartTick );
		
		//double Get_OffsetCycles ( u64 lStartCycle );
		//u64 Get_FreeRunNextIntCycle ( u32 lStartValue, u64 lStartCycle );
		
		//void Update_FreeRunTimer ();
		//void UpdateTimer_Wrap ();
		
		template<const int MODE_CounterEnable, const int MODE_ClkSelect, const int MODE_Gate, const int MODE_GateSelect, const int MODE_GateMode>
		inline void CalibrateTimer5 ();
		template<const int MODE_CounterEnable, const int MODE_ClkSelect, const int MODE_Gate, const int MODE_GateSelect>
		inline void CalibrateTimer4 ();
		template<const int MODE_CounterEnable, const int MODE_ClkSelect, const int MODE_Gate>
		inline void CalibrateTimer3 ();
		template<const int MODE_CounterEnable, const int MODE_ClkSelect>
		inline void CalibrateTimer2 ();
		template<const int MODE_CounterEnable>
		inline void CalibrateTimer1 ();
	};
	
	
	

	class Timers
	{
	
		static Debug::Log debug;
	
	public:
		
		static Timers *_TIMERS;
		
		//////////////////////////
		//	General Parameters	//
		//////////////////////////
		
		// where the timer registers start at
		static const u32 Regs_Start = 0x10000000;
		
		// where the timer registers end at
		static const u32 Regs_End = 0x10001830;
	
		// distance between numbered groups of registers
		static const u32 Reg_Size = 0x10;
		
		// cycle that the next event will happen at for this device
		u64 NextEvent_Cycle;



		
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
		
		
		static const u32 c_iNumberOfChannels = 4;
		
		Timer TheTimers [ c_iNumberOfChannels ];
		

		// these allow you to read and write registers and allow the device to act on the read/write event
		static u64 Read ( u32 Address, u64 Mask );
		static void Write ( u32 Address, u64 Data, u64 Mask );

		
		// update what cycle the next event is at for this device
		void Update_NextEventCycle ();

		
		// this gets the next event cycle for the specified timer
		inline void Get_NextEvent ( int TimerNumber ) { TheTimers [ TimerNumber ].cbGetNextEvent ( & ( TheTimers [ TimerNumber ] ) ); }
		
		
		// this updates the specified timer value
		inline void UpdateTimer ( u32 TimerNumber ) { TheTimers [ TimerNumber ].cbUpdate ( & ( TheTimers [ TimerNumber ] ) ); }
		
		
		inline void CalibrateTimer ( u32 TimerNumber ) { TheTimers [ TimerNumber ].CalibrateTimer (); }
		
		
		inline long GetValue ( u32 TimerNumber ) { return TheTimers [ TimerNumber ].GetValue (); }
		inline long GetMode ( u32 TimerNumber ) { return TheTimers [ TimerNumber ].GetMode (); }
		inline long GetComp ( u32 TimerNumber ) { return TheTimers [ TimerNumber ].GetComp (); }
		
		inline void SetMode ( u32 TimerNumber, u32 Data ) { TheTimers [ TimerNumber ].SetMode ( Data ); }
		inline void SetValue ( u32 TimerNumber, u32 Data ) { TheTimers [ TimerNumber ].SetValue ( Data ); }
		inline void SetComp ( u32 TimerNumber, u32 Data ) { TheTimers [ TimerNumber ].SetComp ( Data ); }
		
		

		//////////////////////////////////
		//	Device Specific Parameters	//
		//////////////////////////////////

		// the timer count - read only
		static const u32 COUNT_Base = 0x10000000;
		
		
		// the timer mode - setting this to zero resets a timer - read and write
		static const u32 MODE_Base = 0x10000010;
		
		
		// the timer compare/target value - read and write
		static const u32 COMP_Base = 0x10000020;

		// the timer hold value
		static const u32 HOLD_Base = 0x10000030;
		
		
		static const u32 TIMER0_COUNT = 0x10000000;
		static const u32 TIMER0_MODE = 0x10000010;
		static const u32 TIMER0_COMP = 0x10000020;
		static const u32 TIMER0_HOLD = 0x10000030;

		static const u32 TIMER1_COUNT = 0x10000800;
		static const u32 TIMER1_MODE = 0x10000810;
		static const u32 TIMER1_COMP = 0x10000820;
		static const u32 TIMER1_HOLD = 0x10000830;

		static const u32 TIMER2_COUNT = 0x10001000;
		static const u32 TIMER2_MODE = 0x10001010;
		static const u32 TIMER2_COMP = 0x10001020;

		static const u32 TIMER3_COUNT = 0x10001800;
		static const u32 TIMER3_MODE = 0x10001810;
		static const u32 TIMER3_COMP = 0x10001820;
		
		
		//////////////////////////////////////////
		// Timers are synchronized to:			//
		// Timer X - System/Horizontal Retrace	//
		//////////////////////////////////////////

		// timers 0 and 1 get a signal from the GPU for pixel/hblank
		//static GPU *g;
		
		
		
		
		// Constructor
		Timers ();
		
		void Reset ();
		
		void Start ();
		
		// *** IMPORTANT *** this MUST be called after loading a save state due to new timer structure
		// re-calibrate all timers - call this after loading in a save state, since the timers have pointers to some timer functions
		void ReCalibrateAll ();
		
		inline void ConnectExternalSignal_GPU ( u64 *ScanlineStart, u64 *NextScanlineStart, u64 *HBlankStart, u32 *Scanline, u32 *NextScanline, u32 *VBlank_Y, u32 *RasterYMax )
		{
			Timer::_llScanlineStart = ScanlineStart;
			Timer::_llNextScanlineStart = NextScanlineStart;
			Timer::_llHBlankStart = HBlankStart;
			Timer::_lScanline = Scanline;
			Timer::_lNextScanline = NextScanline;
			Timer::_lVBlank_Y = VBlank_Y;
			Timer::_lRasterYMax = RasterYMax;
		}
		
		inline void ConnectExternalSignal_Clock ( u64* CycleCount )
		{
			Timer::_llCycleCount = CycleCount;
			Timer::_DebugCycleCount = CycleCount;
		}
		
		//void ConnectDevices ( GPU *_g );

		// run for a clock cycle
		// returns interrupt status - whether to interrupt or not
		// needs GPU Status READ register
		// returns interrupt status for INTC
		void Run ();
		
		static u64* _NextSystemEvent;
		
		static const u32 c_InterruptBit = 9;
		
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
		
		inline void SetInterrupt ( const u32 TimerNumber )
		{
			*_Intc_Stat |= ( 1 << ( c_InterruptBit + TimerNumber ) );
			if ( *_Intc_Stat & *_Intc_Mask ) *_R5900_Cause_13 |= ( 1 << 10 );
			
			if ( ( *_R5900_Cause_13 & *_R5900_Status_12 & 0xff00 ) && ( *_R5900_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 0 );
		}
		
		// goes through INTC which is a latch, so no clearing of interrupts from here
		inline void ClearInterrupt ( const u32 TimerNumber )
		{
			//*_Intc_Stat &= ~( 1 << ( c_InterruptBit + TimerNumber ) );
			//if ( ! ( *_Intc_Stat & *_Intc_Mask ) ) *_R5900_Cause_13 &= ~( 1 << 10 );
			
			//if ( ( *_R5900_Cause_13 & *_R5900_Status_12 & 0xff00 ) && ( *_R5900_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 );
		}

		
		////////////////////////////////
		// Debug Info
		static u32* _DebugPC;
		static u64* _DebugCycleCount;

		// object debug stuff
		// Enable/Disable debug window for object
		// Windows API specific code is in here
		static bool DebugWindow_Enabled;
		static WindowClass::Window *DebugWindow;
		static DebugValueList<u32> *Timer_ValueList;
		static void DebugWindow_Enable ();
		static void DebugWindow_Disable ();
		static void DebugWindow_Update ();
	};



// get cycle that next interrupt would happen at if the timer is counting in clock cycles
inline static u64 Timer::Get_NextIntCycle_Clock ( Timer* t, u64 llStartCycle, long lStartTick, long lNextIntTick )
{
	long lTicksToGo;
	u64 llCycleOffset, llCyclesToGo, llIntCycle;
	
	lTicksToGo = lNextIntTick - lStartTick;
	llCycleOffset = llStartCycle & ( ( 1 << ( t->MODE.ClkSelect << 2 ) ) - 1 );
	llCyclesToGo = ( lTicksToGo << ( t->MODE.ClkSelect << 2 ) ) - llCycleOffset;
	llIntCycle = llStartCycle + llCyclesToGo;
	
	return llIntCycle;
}





// gets the next event for timer.. checks up to "ThroughCycle"
// *important* don't forget to re-check for the next event whenever timer/mode/comp gets set/updated
template<const int MODE_CounterEnable, const int MODE_ClkSelect, const int MODE_Gate, const int MODE_GateSelect, const int MODE_GateMode>
static void Timer::Get_NextEvent ( Timer* t )
{
	long CompareValue;
	long MaxTick, IntTick, NextIntTick, TicksToGo, StartTick;
	long long llCycleOffset, llCyclesToGo, llIntCycle, llStartCycle;
	
	// event is only hit if interrupt is enabled or timer is enabled
	if ( ( !t->MODE.IrqOnTarget && !t->MODE.IrqOnOverflow ) || !MODE_CounterEnable )
	{
		// no events for timer needed //
		t->SetNextEventCh_Cycle ( -1LL );
		return;
	}
	
	// init next int tick
	IntTick = -1;
	
	// get compare value
	CompareValue = t->COMP.Value;
	
	// get maximum tick value
	if ( t->MODE.CompareEnable )
	{
		MaxTick = t->COMP.Value;
	}
	else
	{
		MaxTick = 0xffff;
	}
	
	// check if next tick for interrupt is the compare value //
	
	if ( t->MODE.IrqOnTarget )
	{
		if ( ( t->StartValue < CompareValue ) || !t->MODE.IrqOnOverflow )
		{
			// next timer interrupt is on compare value
			IntTick = CompareValue;
			
			//if ( t->StartValue >= CompareValue )
			//{
			//	// timer needs to wrap around first //
			//	NextIntTick += 0x10000;
			//}
		}
		
	}
	
	// check if next tick for interrupt is on overflow
	if ( t->MODE.IrqOnOverflow )
	{
		// first check if overflow can be reached
		if ( MaxTick == 0xffff )
		{
			// overflow can be reached //
			
			// check if timer value is greater or equal to compare value or there is no compare interrupt
			if ( ( t->StartValue >= CompareValue ) || !t->MODE.IrqOnTarget )
			{
				// next interrupt occurs at overflow //
				IntTick = c_iTimerValueAtOverflow;
			}
		}
	}
	
	// double check that there is an interrupt in the future
	if ( IntTick < 0 )
	{
		// no events for timer needed //
		t->SetNextEventCh_Cycle ( -1LL );
		return;
	}
	
	// get current tick
	StartTick = t->StartValue;
	
	if ( StartTick < IntTick ) NextIntTick = IntTick; else NextIntTick = IntTick + 0x10000;
	
	// get number of ticks to go until event
	TicksToGo = NextIntTick - StartTick;
	
	// set start cycle
	llStartCycle = t->StartCycle;
	
	// check if counting clock cycles or hblanks
	if ( MODE_ClkSelect <= 2 )
	{
		// counting clock cycles //
		
		// check if gate is enabled
		if ( MODE_Gate )
		{
			// gate enabled //
			
			// check if gate is hblank or vblank
			if ( !MODE_GateSelect )
			{
				// gate hblank //
				
				// check gate mode
				switch ( MODE_GateMode )
				{
					case 0:
						// counts while not in blank //
						
						// check if start cycle is before hblank
						if ( llStartCycle < *_llHBlankStart )
						{
							// check if interrupt happens before hblank //
							
							// check if counting 1/16, 1/256, etc
							llIntCycle = Get_NextIntCycle_Clock ( t, llStartCycle, StartTick, NextIntTick );
							
							if ( llIntCycle <= *_llHBlankStart )
							{
								// set interrupt event
								t->SetNextEventCh_Cycle ( llIntCycle );
								return;
							}
						}
						
						break;
						
					case 1:
					
						// resets and starts counting at start of blank //
						
						// check if interrupt occurs before hblank //
						if ( llStartCycle < *_llHBlankStart )
						{
							// count cycles up until hblank //
							
							// check if counting 1/16, 1/256, etc
							llIntCycle = Get_NextIntCycle_Clock ( t, llStartCycle, StartTick, NextIntTick );
							
							if ( llIntCycle < *_llHBlankStart )
							{
								// set interrupt event
								t->SetNextEventCh_Cycle ( llIntCycle );
								return;
							}
							
							llStartCycle = *_llHBlankStart;
							StartTick = 0;
							
							// start tick changed, so recalculate NextIntTick
							if ( StartTick < IntTick ) NextIntTick = IntTick; else NextIntTick = IntTick + 0x10000;
						}
						
						// check if interrupt occurs after hblank //
						llIntCycle = Get_NextIntCycle_Clock ( t, llStartCycle, StartTick, NextIntTick );
						
						// if interrupt is not on scanline or at very beginning of next, then disregard
						if ( llIntCycle <= *_llNextScanlineStart )
						{
							// set interrupt event
							t->SetNextEventCh_Cycle ( llIntCycle );
						}
						
						break;
						
					case 2:
					
						// resets and starts counting on transition out of blank //
						
						// get cycle that interrupt occurs at
						llIntCycle = Get_NextIntCycle_Clock ( t, llStartCycle, StartTick, NextIntTick );
						
						if ( llIntCycle < *_llNextScanlineStart )
						{
							// set interrupt event
							t->SetNextEventCh_Cycle ( llIntCycle );
						}
						
						break;
						
					case 3:
						// resets and starts counting at start of blank and on transition out of blank //
						
						// check if interrupt occurs before hblank //
						if ( llStartCycle < *_llHBlankStart )
						{
							// count cycles up until hblank //
							
							// check if counting 1/16, 1/256, etc
							llIntCycle = Get_NextIntCycle_Clock ( t, llStartCycle, StartTick, NextIntTick );
							
							if ( llIntCycle < *_llHBlankStart )
							{
								// set interrupt event
								t->SetNextEventCh_Cycle ( llIntCycle );
								return;
							}
							
							llStartCycle = *_llHBlankStart;
							StartTick = 0;
							
							// start tick changed, so recalculate NextIntTick
							if ( StartTick < IntTick ) NextIntTick = IntTick; else NextIntTick = IntTick + 0x10000;
						}
						
						// check if interrupt occurs after hblank //
						llIntCycle = Get_NextIntCycle_Clock ( t, llStartCycle, StartTick, NextIntTick );
						
						// if interrupt is not on scanline or at very beginning of next, then disregard.. no, its cool
						if ( llIntCycle < *_llNextScanlineStart )
						{
							// set interrupt event
							t->SetNextEventCh_Cycle ( llIntCycle );
						}
						
						break;
				}
			}
			else
			{
				// gate vblank //
				
				// check gate mode
				switch ( MODE_GateMode )
				{
					case 0:
						// counts while not in blank //
						
						// check if scanline is NOT in vblank
						if ( *_lScanline < *_lVBlank_Y )
						{
							// get cycle that interrupt occurs at
							llIntCycle = Get_NextIntCycle_Clock ( t, llStartCycle, StartTick, NextIntTick );
							
							if ( llIntCycle <= *_llNextScanlineStart )
							{
								// set interrupt event
								t->SetNextEventCh_Cycle ( llIntCycle );
							}
						}
						
						break;
						
					case 1:
						// resets and starts counting at start of blank //
						
						// get cycle that interrupt occurs at
						llIntCycle = Get_NextIntCycle_Clock ( t, llStartCycle, StartTick, NextIntTick );
						
						// check if current scanline is next to blank
						if ( ( *_lScanline + 2 ) >= *_lVBlank_Y )
						{
							// counter resets at start of next scanline
							if ( llIntCycle < *_llNextScanlineStart )
							{
								// set interrupt event
								t->SetNextEventCh_Cycle ( llIntCycle );
							}
						}
						else
						{
							// counter does not reset at start of next scanline
							if ( llIntCycle <= *_llNextScanlineStart )
							{
								// set interrupt event
								t->SetNextEventCh_Cycle ( llIntCycle );
							}
						}
						
						break;
						
					case 2:
						// resets and starts counting on transition out of blank //
						
						// get cycle that interrupt occurs at
						llIntCycle = Get_NextIntCycle_Clock ( t, llStartCycle, StartTick, NextIntTick );
						
						// check if current scanline is next to last
						if ( ( *_lScanline + 2 ) >= *_lRasterYMax )
						{
							// check if current cycle is at next scanline start
							if ( llIntCycle < *_llNextScanlineStart )
							{
								// set interrupt event
								t->SetNextEventCh_Cycle ( llIntCycle );
							}
						}
						else
						{
							if ( llIntCycle <= *_llNextScanlineStart )
							{
								// set interrupt event
								t->SetNextEventCh_Cycle ( llIntCycle );
							}
						}
						
						break;
						
					case 3:
						// resets and starts counting at start of blank and on transition out of blank //
						
						// check if current scanline is next to blank
						if ( ( *_lScanline + 2 ) >= *_lVBlank_Y )
						{
							// check if current cycle is at next scanline start
							if ( *_llCycleCount >= *_llNextScanlineStart )
							{
								// reset counter
								t->StartValue = 0;
								t->StartCycle = *_llCycleCount;
							}
						}
						
						// check if current scanline is next to last
						if ( ( *_lScanline + 2 ) >= *_lRasterYMax )
						{
							// check if current cycle is at next scanline start
							if ( *_llCycleCount >= *_llNextScanlineStart )
							{
								// reset counter
								t->StartValue = 0;
								t->StartCycle = *_llCycleCount;
							}
						}
						
						
						break;
				}
			}
		}
		else
		{
			// gate is disabled //
			
			// get cycle that interrupt occurs at
			llIntCycle = Get_NextIntCycle_Clock ( t, llStartCycle, StartTick, NextIntTick );
			
			if ( llIntCycle <= *_llNextScanlineStart )
			{
				// set interrupt event
				t->SetNextEventCh_Cycle ( llIntCycle );
			}
			
		}
	}
	else
	{
		// counting hblanks //
		
		// check that interrupt is possible on scanline
		if ( ( StartTick + 1 ) != NextIntTick )
		{
			// interrupt not possible on scanline //
			t->SetNextEventCh_Cycle ( -1LL );
			return;
		}
		
		// check if gate function is enabled for vblank (hblank is disabled since we are counting hblanks)
		// check if gate is enabled
		if ( MODE_Gate && MODE_GateSelect )
		{
			// gate enabled (vblank) //
			
			// gate vblank //
			
			// check gate mode
			switch ( MODE_GateMode )
			{
				case 0:
					// counts while not in vblank //
					
					// check if scanline is NOT in vblank
					if ( *_lScanline < *_lVBlank_Y )
					{
						// count cycles
						if ( llStartCycle < *_llHBlankStart )
						{
							// interrupt happens at hblank on scanline
							t->SetNextEventCh_Cycle ( *_llHBlankStart );
							//t->StartCycle++;
						}
					}
					
					// update cycle
					//t->StartCycle = *_llCycleCount;
					
					break;
					
				case 1:
					// resets and starts counting at start of blank //
				case 2:
					// resets and starts counting on transition out of blank //
				case 3:
					// resets and starts counting at start of blank and on transition out of blank //
					
					// count cycles
					if ( llStartCycle < *_llHBlankStart )
					{
						// interrupt happens at hblank on scanline
						t->SetNextEventCh_Cycle ( *_llHBlankStart );
						//t->StartCycle++;
					}
					
					//t->StartCycle = *_llCycleCount;
					
					break;
			}
		}
		else
		{
			// gate is disabled //
			
			// interrupt happens at hblank
			if ( llStartCycle < *_llHBlankStart )
			{
				// interrupt happens at hblank on scanline
				t->SetNextEventCh_Cycle ( *_llHBlankStart );
				//t->StartCycle++;
			}
			
			// update timer cycle
			//t->StartCycle = *_llCycleCount;
		}
	}
	
	// unable to find next cycle timer should interrupt at for now
	//SetNextEventCh_Cycle ( 0 );
}











// the only way to properly simulate a PS2 timer it appears at this point is to update it per scanline... scanline start is probably best
// update per scanline
// needs MODE_CounterEnable, MODE_ClkSelect, MODE_Gate, MODE_GateSelect, MODE_GateMode
template<const int MODE_CounterEnable, const int MODE_ClkSelect, const int MODE_Gate, const int MODE_GateSelect, const int MODE_GateMode>
static void Timer::UpdateTimer ( Timer* t )
{
	long CompareValue;
	
	// if timer is not enabled, then set cycle and done
	if ( !MODE_CounterEnable )
	{
		t->StartCycle = *_llCycleCount;
		return;
	}
	
	// check if counting clock cycles or hblanks
	if ( MODE_ClkSelect <= 2 )
	{
		// counting clock cycles //
		
		// check if gate is enabled
		if ( MODE_Gate )
		{
			// gate enabled //
			
			// check if gate is hblank or vblank
			if ( !MODE_GateSelect )
			{
				// gate hblank //
				
				// check gate mode
				switch ( MODE_GateMode )
				{
					case 0:
						// counts while not in blank //
						
						// check if start cycle is before hblank
						if ( t->StartCycle < *_llHBlankStart )
						{
							// count cycles up until hblank //
							
							// check if counting 1/16, 1/256, etc
							// update timer
							t->StartValue += ( *_llHBlankStart - t->StartCycle + ( t->StartCycle & ( ( 1 << ( MODE_ClkSelect << 2 ) ) - 1 ) ) ) >> ( MODE_ClkSelect << 2 );
						}
						
						// update cycle
						t->StartCycle = *_llCycleCount;
						
						break;
						
					case 1:
						// resets and starts counting at start of blank //
						
						// check if current cycle is at or after hblank
						if ( *_llCycleCount >= *_llHBlankStart )
						{
							t->StartValue = 0;
							t->StartCycle = *_llHBlankStart;
						}
						
						// count cycles //
						t->StartValue += ( *_llCycleCount - t->StartCycle + ( t->StartCycle & ( ( 1 << ( MODE_ClkSelect << 2 ) ) - 1 ) ) ) >> ( MODE_ClkSelect << 2 );
						
						// update cycle
						t->StartCycle = *_llCycleCount;
						
						break;
						
					case 2:
					
						// resets and starts counting on transition out of blank //
						
						// check if current cycle is at or after the start of next scanline
						if ( *_llCycleCount >= *_llNextScanlineStart )
						{
							// reset counter
							t->StartValue = 0;
							t->StartCycle = *_llCycleCount;
						}
						else
						{
							// count cycles
							t->StartValue += ( *_llCycleCount - t->StartCycle + ( t->StartCycle & ( ( 1 << ( MODE_ClkSelect << 2 ) ) - 1 ) ) ) >> ( MODE_ClkSelect << 2 );
						}
						
						break;
						
					case 3:
						// resets and starts counting at start of blank and on transition out of blank //
						
						// check if current cycle is at or after the start of hblank
						if ( *_llCycleCount >= *_llHBlankStart )
						{
							t->StartValue = 0;
							t->StartCycle = *_llHBlankStart;
						}
						
						// check if current cycle is at or after the start of next scanline
						if ( *_llCycleCount >= *_llNextScanlineStart )
						{
							// reset counter
							t->StartValue = 0;
							t->StartCycle = *_llCycleCount;
						}
						else
						{
							// count cycles
							t->StartValue += ( *_llCycleCount - t->StartCycle + ( t->StartCycle & ( ( 1 << ( MODE_ClkSelect << 2 ) ) - 1 ) ) ) >> ( MODE_ClkSelect << 2 );
							t->StartCycle = *_llCycleCount;
						}
						
						break;
				}
			}
			else
			{
				// gate vblank //
				
				// check gate mode
				switch ( MODE_GateMode )
				{
					case 0:
						// counts while not in blank //
						
						// check if scanline is NOT in vblank
						if ( *_lScanline < *_lVBlank_Y )
						{
							// count cycles
							t->StartValue += ( *_llCycleCount - t->StartCycle + ( t->StartCycle & ( ( 1 << ( MODE_ClkSelect << 2 ) ) - 1 ) ) ) >> ( MODE_ClkSelect << 2 );
						}
						
						// update cycle
						t->StartCycle = *_llCycleCount;
						
						break;
						
					case 1:
						// resets and starts counting at start of blank //
						
						// check if current scanline is next to blank
						if ( ( *_lScanline + 2 ) >= *_lVBlank_Y )
						{
							// check if current cycle is at next scanline start
							if ( *_llCycleCount >= *_llNextScanlineStart )
							{
								// reset counter
								t->StartValue = 0;
								t->StartCycle = *_llCycleCount;
							}
						}
						
						// count cycles
						t->StartValue += ( *_llCycleCount - t->StartCycle + ( t->StartCycle & ( ( 1 << ( MODE_ClkSelect << 2 ) ) - 1 ) ) ) >> ( MODE_ClkSelect << 2 );
						t->StartCycle = *_llCycleCount;
							
						break;
						
					case 2:
						// resets and starts counting on transition out of blank //
						
						// check if current scanline is next to last
						if ( ( *_lScanline + 2 ) >= *_lRasterYMax )
						{
							// check if current cycle is at next scanline start
							if ( *_llCycleCount >= *_llNextScanlineStart )
							{
								// reset counter
								t->StartValue = 0;
								t->StartCycle = *_llCycleCount;
							}
						}
						
						// count cycles
						t->StartValue += ( *_llCycleCount - t->StartCycle + ( t->StartCycle & ( ( 1 << ( MODE_ClkSelect << 2 ) ) - 1 ) ) ) >> ( MODE_ClkSelect << 2 );
						t->StartCycle = *_llCycleCount;
						
						break;
						
					case 3:
						// resets and starts counting at start of blank and on transition out of blank //
						
						// check if current scanline is next to blank
						if ( ( *_lScanline + 2 ) >= *_lVBlank_Y )
						{
							// check if current cycle is at next scanline start
							if ( *_llCycleCount >= *_llNextScanlineStart )
							{
								// reset counter
								t->StartValue = 0;
								t->StartCycle = *_llCycleCount;
							}
						}
						
						// check if current scanline is next to last
						if ( ( *_lScanline + 2 ) >= *_lRasterYMax )
						{
							// check if current cycle is at next scanline start
							if ( *_llCycleCount >= *_llNextScanlineStart )
							{
								// reset counter
								t->StartValue = 0;
								t->StartCycle = *_llCycleCount;
							}
						}
						
						// count cycles
						t->StartValue += ( *_llCycleCount - t->StartCycle + ( t->StartCycle & ( ( 1 << ( MODE_ClkSelect << 2 ) ) - 1 ) ) ) >> ( MODE_ClkSelect << 2 );
						t->StartCycle = *_llCycleCount;
						
						break;
				}
			}
		}
		else
		{
			// gate is disabled //
			
			// update timer value
			t->StartValue += ( *_llCycleCount - t->StartCycle + ( t->StartCycle & ( ( 1 << ( MODE_ClkSelect << 2 ) ) - 1 ) ) ) >> ( MODE_ClkSelect << 2 );
			
			// update timer cycle
			t->StartCycle = *_llCycleCount;
		}
		
		// get compare value
		if ( t->MODE.CompareEnable )
		{
			CompareValue = t->COMP.Value;
		}
		else
		{
			CompareValue = 0xffff;
		}
		
		// wrap timer //
		if ( t->StartValue > CompareValue )
		{
			t->StartValue -= ( CompareValue + 1 );
			
			// check if that was not enough wrapping
			if ( t->StartValue > CompareValue )
			{
				t->StartValue %= ( CompareValue + 1 );
			}
		}
	}
	else
	{
		// counting hblanks //
		
		// check if gate function is enabled for vblank (hblank is disabled since we are counting hblanks)
		// check if gate is enabled
		if ( MODE_Gate && MODE_GateSelect )
		{
			// gate enabled (vblank) //
			
			// gate vblank //
			
			// check gate mode
			switch ( MODE_GateMode )
			{
				case 0:
					// counts while not in vblank //
					
					// check if scanline is NOT in vblank
					if ( *_lScanline < *_lVBlank_Y )
					{
						// count cycles
						if ( t->StartCycle < *_llHBlankStart )
						{
							if ( *_llCycleCount >= *_llHBlankStart )
							{
								// passed hblank
								t->StartValue++;
							}
						}
					}
					
					// update cycle
					t->StartCycle = *_llCycleCount;
					
					break;
					
				case 1:
					// resets and starts counting at start of blank //
					
					// check if current scanline is next to blank
					if ( ( *_lScanline + 2 ) >= *_lVBlank_Y )
					{
						// check if current cycle is at next scanline start
						if ( *_llCycleCount >= *_llNextScanlineStart )
						{
							// reset counter
							t->StartValue = 0;
							t->StartCycle = *_llCycleCount;
						}
					}
					
					// count cycles
					if ( t->StartCycle < *_llHBlankStart )
					{
						if ( *_llCycleCount >= *_llHBlankStart )
						{
							// passed hblank
							t->StartValue++;
						}
					}
					
					t->StartCycle = *_llCycleCount;
					
					break;
					
				case 2:
					// resets and starts counting on transition out of blank //
					
					// check if current scanline is next to last
					if ( ( *_lScanline + 2 ) >= *_lRasterYMax )
					{
						// check if current cycle is at next scanline start
						if ( *_llCycleCount >= *_llNextScanlineStart )
						{
							// reset counter
							t->StartValue = 0;
							t->StartCycle = *_llCycleCount;
						}
					}
					
					// count cycles
					if ( t->StartCycle < *_llHBlankStart )
					{
						if ( *_llCycleCount >= *_llHBlankStart )
						{
							// passed hblank
							t->StartValue++;
						}
					}
					
					t->StartCycle = *_llCycleCount;
					
					break;
					
				case 3:
					// resets and starts counting at start of blank and on transition out of blank //
					
					// check if current scanline is next to blank
					if ( ( *_lScanline + 2 ) >= *_lVBlank_Y )
					{
						// check if current cycle is at next scanline start
						if ( *_llCycleCount >= *_llNextScanlineStart )
						{
							// reset counter
							t->StartValue = 0;
							t->StartCycle = *_llCycleCount;
						}
					}
					
					// check if current scanline is next to last
					if ( ( *_lScanline + 2 ) >= *_lRasterYMax )
					{
						// check if current cycle is at next scanline start
						if ( *_llCycleCount >= *_llNextScanlineStart )
						{
							// reset counter
							t->StartValue = 0;
							t->StartCycle = *_llCycleCount;
						}
					}
					
					// count cycles
					if ( t->StartCycle < *_llHBlankStart )
					{
						if ( *_llCycleCount >= *_llHBlankStart )
						{
							// passed hblank
							t->StartValue++;
						}
					}
					
					t->StartCycle = *_llCycleCount;
					
					break;
			}
		}
		else
		{
			// gate is disabled //
			
			// update timer value
			if ( t->StartCycle < *_llHBlankStart )
			{
				if ( *_llCycleCount >= *_llHBlankStart )
				{
					// passed hblank
					t->StartValue++;
				}
			}
			
			// update timer cycle
			t->StartCycle = *_llCycleCount;
		}
		
		
		// get compare value
		if ( t->MODE.CompareEnable )
		{
			CompareValue = t->COMP.Value;
		}
		else
		{
			CompareValue = 0xffff;
		}
		
		// wrap timer //
		if ( t->StartValue > CompareValue )
		{
			t->StartValue -= ( CompareValue + 1 );
		}
	}
	
	// timer has been updated at this point //

}


template<const int MODE_CounterEnable, const int MODE_ClkSelect, const int MODE_Gate, const int MODE_GateSelect, const int MODE_GateMode>
inline void Timer::CalibrateTimer5 ()
{
	cbUpdate = UpdateTimer <MODE_CounterEnable,MODE_ClkSelect,MODE_Gate,MODE_GateSelect,MODE_GateMode>;
	cbGetNextEvent = Get_NextEvent <MODE_CounterEnable,MODE_ClkSelect,MODE_Gate,MODE_GateSelect,MODE_GateMode>;
}


template<const int MODE_CounterEnable, const int MODE_ClkSelect, const int MODE_Gate, const int MODE_GateSelect>
inline void Timer::CalibrateTimer4 ()
{
	switch ( MODE.ClkSelect )
	{
		case 0:
			CalibrateTimer5 <MODE_CounterEnable,MODE_ClkSelect,MODE_Gate,MODE_GateSelect,0> ();
			break;
			
		case 1:
			CalibrateTimer5 <MODE_CounterEnable,MODE_ClkSelect,MODE_Gate,MODE_GateSelect,1> ();
			break;
			
		case 2:
			CalibrateTimer5 <MODE_CounterEnable,MODE_ClkSelect,MODE_Gate,MODE_GateSelect,2> ();
			break;
			
		case 3:
			CalibrateTimer5 <MODE_CounterEnable,MODE_ClkSelect,MODE_Gate,MODE_GateSelect,3> ();
			break;
	}
}


template<const int MODE_CounterEnable, const int MODE_ClkSelect, const int MODE_Gate>
inline void Timer::CalibrateTimer3 ()
{
	switch ( MODE.GateSelect )
	{
		case 0:
			CalibrateTimer4 <MODE_CounterEnable,MODE_ClkSelect,MODE_Gate,0> ();
			break;
			
		case 1:
			CalibrateTimer4 <MODE_CounterEnable,MODE_ClkSelect,MODE_Gate,1> ();
			break;
	}
}


template<const int MODE_CounterEnable, const int MODE_ClkSelect>
inline void Timer::CalibrateTimer2 ()
{
	switch ( MODE.Gate )
	{
		case 0:
			CalibrateTimer3 <MODE_CounterEnable,MODE_ClkSelect,0> ();
			break;
			
		case 1:
			CalibrateTimer3 <MODE_CounterEnable,MODE_ClkSelect,1> ();
			break;
	}
}

template<const int MODE_CounterEnable>
inline void Timer::CalibrateTimer1 ()
{
	switch ( MODE.ClkSelect )
	{
		case 0:
			CalibrateTimer2 <MODE_CounterEnable,0> ();
			break;
			
		case 1:
			CalibrateTimer2 <MODE_CounterEnable,1> ();
			break;
			
		case 2:
			CalibrateTimer2 <MODE_CounterEnable,2> ();
			break;
			
		case 3:
			CalibrateTimer2 <MODE_CounterEnable,3> ();
			break;
	}
}



	
};



#endif

