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


#ifndef _PS1_TIMER_H_
#define _PS1_TIMER_H_


#include "types.h"

#include "Debug.h"

//#include "PS1_Intc.h"
#include "PS1_Gpu.h"

#include "DebugValueList.h"


#ifdef PS2_COMPILE
#include "PS2_SIF.h"
#endif


namespace Playstation1
{

	

	class Timer
	{
	
		//Debug::Log debug;
	
	public:
	
		// timers 0 and 1 get a signal from the GPU for pixel/hblank
		static GPU *g;
		
		
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
				// 0 - timer running; 1 - timer stopped (can only be 1 for timer 2)
				// bit 0
				// from Nocash PSX Specification
				// 0: free run; 1: synchronize via bits 1-2
				u32 RunMode : 1;

				// bits 1-2
				// from Nocash PSX Specification
				// Synchronization Mode
				// counter 0: 0-do not count during hblank; 1-reset counter to 0 at hblank;
				// 2-reset to 0 at hblank and do not count outside of hblank; 3-pause until next hblank, then switch to free run
				// counter 1: like counter 0, but vblank instead of hblank
				// counter 2: 0 or 3-stop counter at current value; 1 or 2-free run
				u32 SyncMode : 2;
				
				// 0 - count to 0xffff (including that value); 1 - count to value in target/compare register (including that value)
				// bit 3
				u32 CountToTarget : 1;
				
				// set along with Iq2 for an IRQ when target is reached in counter
				// bit 4
				// 1: Interrupt when counter equals target
				u32 IrqOnTarget : 1;
				
				// bit 5
				// 1: Interrupt when counter equals 0xffff
				u32 IrqOnOverflow : 1;
				
				// set along with Iq1 for an IRQ when target is reached in counter
				// bit 6
				// 0: Irq only once; 1: Irq repeatedly
				u32 IrqMode_Repeat : 1;

				// bit 7
				// 0: pulse mode; 1: toggle mode
				u32 IrqMode_Toggle : 1;
				
				// bits 8-9
				// counter 0: 0 or 2-system clock; 1 or 3-dot clock
				// counter 1: 0 or 2-system clock; 1 or 3-hblank
				// counter 2: 0 or 1-system clock; 2 or 3-system clock/8
				u32 ClockSource : 2;
				
				// bit 10 - Interrupt Request
				// 0: yes; 1: no
				u32 IrqRequest : 1;
				
				// bit 11 - Target reached
				// 0: no; 1: yes
				u32 TargetReached : 1;
				
				// bit 12 - 0xffff reached
				// 0: no; 1: yes
				u32 OverflowReached : 1;

#ifdef PS2_COMPILE

				// bits 13-14 - Clock div for timers 4 and 5 ??
				// 0: count cycles, 1: count every 8 cycles, 2: count every 16 cycles, 3: count every 256 cycles
				u32 ClockDiv : 2;
				
				// bit 15 - zero
				u32 zero0 : 1;
				
#else

				// bits 13-15 - zero
				u32 zero0 : 3;
				
#endif
				
				// garbage data - bits 16-31
				u32 Garbage0 : 16;
			};
			
		
			struct
			{
				// 0 - timer running; 1 - timer stopped (can only be 1 for timer 2)
				// bit 0
				u32 En : 1;

				// bits 1-2
				u32 Zero2 : 2;
				
				// 0 - count to 0xffff; 1 - count to value in target/compare register
				// bit 3
				u32 Tar : 1;
				
				// set along with Iq2 for an IRQ when target is reached in counter
				// bit 4
				u32 Iq1 : 1;
				
				// bit 5
				u32 Zero1 : 1;
				
				// set along with Iq1 for an IRQ when target is reached in counter
				// bit 6
				u32 Iq2 : 1;

				// bit 7
				u32 Zero0 : 1;
				
				// 0 - System Clock (if Div is also zero); 1 - Pixel Clock (timer 0), Horizontal Retrace (timer 1)
				// bit 8
				u32 Clc : 1;
				
				// 0 - System Clock (if Clc is also zero); 1 - 1/8 System Clock (timer 2)
				// bit 9
				u32 Div : 1;
				
				// garbage data - bits 10-31
				u32 Garbage : 22;
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
		
		// also need to know more stuff about the timer
		
		// timer status - like free run, wait till next hblank, etc
		//enum { TIMER_FREERUN = 0, TIMER_SYNC_RUN, TIMER_SYNC_WAIT, TIMER_SYNC_STOPPED };
		//u32 TimerStatus;
		
		u64 IRQ_Counter;
		
		// value the timer started at with the cycle number it started at that value
		u64 StartValue;
		u64 StartCycle;
		
		u32 NextIntType;
		enum { INT_TYPE_TARGET = 0, INT_TYPE_OVERFLOW = 1 };
		
		// need to know when the next blanking starts for a timer in sync mode
		//u64 NextBlankingStart_Cycle;
		
		// cycles to add before dividing
		double dOffsetCycles;
		
		// these two are in 32.32 fixed point
		// these actually need to be double-precision floating point
		double dCyclesPerTick;
		double dTicksPerCycle;
		
		// this is when the next known event or interrupt happens
		u64 NextEvent_Cycle;
		
		
		// constructor - pass timer number 0-3, and ref to parent object
		Timer ();
		
		// reset the timer - use when creating timer or resetting system
		void Reset ();

		// "TimerIndex" just specifies the index of the timer in the array of timers for the collection of timers it is in
		// for standalone PS1, "TimerNumber" is 0 for pixel timer, 1 for HBlank Timer, 2 for Cycle timer
		void Set_TimerNumber ( int TimerIndex, u32 TimerNumber );
		
		// sets/calculates timer interval
		void CalibrateTimer ();
		
		// this updates the timer value
		void UpdateTimer ();
		
		// this gets the next event cycle for the timer, but may possibly only calculate up until "ThroughCycle"
		void Get_NextEvent ( u64 ThroughCycle );
		
		
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
		u32 Get_NextIntTick ( u64 lStartTick );
		
		double Get_OffsetCycles ( u64 lStartCycle );
		u64 Get_FreeRunNextIntCycle ( u32 lStartValue, u64 lStartCycle );
		
		void Update_FreeRunTimer ();
		void UpdateTimer_Wrap ();
	};
	
	
	

	class Timers
	{
	
	public:
		
		static Debug::Log debug;
		
		static Timers *_TIMERS;
		
		//////////////////////////
		//	General Parameters	//
		//////////////////////////
		
		// where the timer registers start at
		static const u32 Regs_Start = 0x1f801100;
		
		// where the timer registers end at
		static const u32 Regs_End = 0x1f801128;
	
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
		
		
#ifdef PS2_COMPILE
		static const u32 c_iNumberOfChannels = 6;
#else
		static const u32 c_iNumberOfChannels = 3;
#endif
		
		Timer TheTimers [ c_iNumberOfChannels ];
		

		// these allow you to read and write registers and allow the device to act on the read/write event
		static u32 Read ( u32 Address );
		static void Write ( u32 Address, u32 Data, u32 Mask );

		
		// update what cycle the next event is at for this device
		void Update_NextEventCycle ();

		
		// this gets the next event cycle for the specified timer
		inline void Get_NextEvent ( int TimerNumber, u64 ThroughCycle ) { TheTimers [ TimerNumber ].Get_NextEvent ( ThroughCycle ); }
		
		
		// this updates the specified timer value
		inline void UpdateTimer ( u32 TimerNumber ) { TheTimers [ TimerNumber ].UpdateTimer (); }
		
		
		inline void CalibrateTimer ( u32 TimerNumber ) { TheTimers [ TimerNumber ].CalibrateTimer (); }
		
		
		inline void SetMode ( u32 TimerNumber, u32 Data ) { TheTimers [ TimerNumber ].SetMode ( Data ); }
		inline void SetValue ( u32 TimerNumber, u32 Data ) { TheTimers [ TimerNumber ].SetValue ( Data ); }
		inline void SetComp ( u32 TimerNumber, u32 Data ) { TheTimers [ TimerNumber ].SetComp ( Data ); }
		
		

		//////////////////////////////////
		//	Device Specific Parameters	//
		//////////////////////////////////

		// the timer count - read only
		static const u32 COUNT_Base = 0x1f801100;
		
		
		// the timer mode - setting this to zero resets a timer - read and write
		static const u32 MODE_Base = 0x1f801104;
		
		
		// the timer compare/target value - read and write
		static const u32 COMP_Base = 0x1f801108;
		
		
		static const u32 TIMER0_COUNT = 0x1f801100;
		static const u32 TIMER0_MODE = 0x1f801104;
		static const u32 TIMER0_COMP = 0x1f801108;

		static const u32 TIMER1_COUNT = 0x1f801110;
		static const u32 TIMER1_MODE = 0x1f801114;
		static const u32 TIMER1_COMP = 0x1f801118;

		static const u32 TIMER2_COUNT = 0x1f801120;
		static const u32 TIMER2_MODE = 0x1f801124;
		static const u32 TIMER2_COMP = 0x1f801128;
		
#ifdef PS2_COMPILE

		static const u32 TIMER3_COUNT = 0x1f801480;
		static const u32 TIMER3_MODE = 0x1f801484;
		static const u32 TIMER3_COMP = 0x1f801488;

		static const u32 TIMER4_COUNT = 0x1f801490;
		static const u32 TIMER4_MODE = 0x1f801494;
		static const u32 TIMER4_COMP = 0x1f801498;

		static const u32 TIMER5_COUNT = 0x1f8014a0;
		static const u32 TIMER5_MODE = 0x1f8014a4;
		static const u32 TIMER5_COMP = 0x1f8014a8;
		
#endif
		
		//////////////////////////////////////////
		// Timers are synchronized to:			//
		// Timer 0 - Pixel clock				//
		// Timer 1 - Horizontal Retrace			//
		// Timer 2 - 1/8 System Clock			//
		// Timer 3 - Vertical Retrace			//
		//////////////////////////////////////////

		// timers 0 and 1 get a signal from the GPU for pixel/hblank
		static GPU *g;
		
		
		
		
		// Constructor
		Timers ();
		
		void Reset ();
		
		void Start ();
		
		void ConnectDevices ( GPU *_g );

		// run for a clock cycle
		// returns interrupt status - whether to interrupt or not
		// needs GPU Status READ register
		// returns interrupt status for INTC
		void Run ();
		
		static u64* _NextSystemEvent;
		
		static const u32 c_InterruptBit = 4;
		static const u32 c_InterruptBit2 = 14;
		
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
		
		inline void SetInterrupt ( const u32 TimerNumber )
		{
#ifdef PS2_COMPILE
			if ( TimerNumber < 3 )
			{
#endif

			*_Intc_Stat |= ( 1 << ( c_InterruptBit + TimerNumber ) );
			
#ifdef PS2_COMPILE
			}
			else
			{
				*_Intc_Stat |= ( 1 << ( c_InterruptBit2 + ( TimerNumber - 3 ) ) );
			}
#endif

			if ( *_Intc_Stat & *_Intc_Mask ) *_R3000A_Cause_13 |= ( 1 << 10 );
			
			if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 );
			
#ifdef PS2_COMPILE
			if ( *_Intc_Stat & Playstation2::SIF::_SIF->lSBUS_F230 )
			{
				// ??
				Playstation2::SIF::SetInterrupt_EE_SIF ();
			}
#endif
		}
		
		inline void ClearInterrupt ( const u32 TimerNumber )
		{
			//*_Intc_Master &= ~( 1 << ( c_InterruptBit + TimerNumber ) );
			*_Intc_Stat &= ~( 1 << ( c_InterruptBit + TimerNumber ) );
			if ( ! ( *_Intc_Stat & *_Intc_Mask ) ) *_R3000A_Cause_13 &= ~( 1 << 10 );
			
			if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 );
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
	
};


#endif

