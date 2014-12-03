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



#include "PS1_Intc.h"

using namespace Playstation1;


#ifdef _DEBUG_VERSION_

// enable debugging
#define INLINE_DEBUG_ENABLE


//#define INLINE_DEBUG_WRITE
//#define INLINE_DEBUG_READ


#endif


u32* Intc::_DebugPC;
u64* Intc::_DebugCycleCount;

u32* Intc::_R3000A_Status_12;
u32* Intc::_R3000A_Cause_13;
u64* Intc::_ProcStatus;


Debug::Log Intc::debug;


Intc *Intc::_INTC;

bool Intc::DebugWindow_Enabled;
WindowClass::Window *Intc::DebugWindow;
DebugValueList<u32> *Intc::ValueList;


Intc::Intc ()
{
	cout << "Running INTC constructor...\n";

/*
#ifdef INLINE_DEBUG_ENABLE
	debug.Create ( "INTC_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering INTC Constructor";
#endif


	// I_STAT
	//Registers [ I_STAT - Regs_Start ].ReadOK = true;
	//Registers [ I_STAT - Regs_Start ].WriteOK = true;
	//Registers [ I_STAT - Regs_Start ].Unknown = false;
	//Registers [ I_STAT - Regs_Start ].Name = "I_STAT: Interrupt Status and Acknowledge Register";
	//Registers [ I_STAT - Regs_Start ].Address = I_STAT;
	//Registers [ I_STAT - Regs_Start ].SizeInBytes = 4;
	//Registers [ I_STAT - Regs_Start ].DataPtr = & (I_STAT_Reg.Value);
	
	// I_MASK
	//Registers [ I_MASK - Regs_Start ].ReadOK = true;
	//Registers [ I_MASK - Regs_Start ].WriteOK = true;
	//Registers [ I_MASK - Regs_Start ].Unknown = false;
	//Registers [ I_MASK - Regs_Start ].Name = "I_MASK: Interrupt Mask Register";
	//Registers [ I_MASK - Regs_Start ].Address = I_MASK;
	//Registers [ I_MASK - Regs_Start ].SizeInBytes = 4;
	//Registers [ I_MASK - Regs_Start ].DataPtr = & (I_MASK_Reg.Value);

#ifdef INLINE_DEBUG
	debug << "->Exiting INTC Constructor";
#endif
*/

}


void Intc::Start ()
{
	cout << "Running INTC constructor...\n";
	
#ifdef INLINE_DEBUG_ENABLE
	debug.Create ( "INTC_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering INTC Constructor";
#endif

	Reset ();
	
	_INTC = this;

#ifdef INLINE_DEBUG
	debug << "->Exiting INTC Constructor";
#endif
}


void Intc::Reset ()
{
#ifdef INLINE_DEBUG
	debug << "\r\nEntering Intc::Reset";
#endif

	// zero object
	memset ( this, 0, sizeof( Intc ) );
	

#ifdef INLINE_DEBUG
	debug << "->Exiting Intc::Reset";
#endif
}

static u32 Intc::Read ( u32 Address )
{
#ifdef INLINE_DEBUG_READ
	debug << "\r\nIntc::Read; " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
#endif

	u32 Output;

	// Read interrupt controller register value
	switch ( Address )
	{
		case I_STAT:
#ifdef INLINE_DEBUG_READ
			debug << "; I_STAT = " << hex << _INTC->I_STAT_Reg.Value;
#endif

			Output = _INTC->I_STAT_Reg.Value;
			break;
			
		case I_MASK:
#ifdef INLINE_DEBUG_READ
			debug << "; I_MASK = " << hex << _INTC->I_MASK_Reg.Value;
#endif

			Output = _INTC->I_MASK_Reg.Value;
			break;

#ifdef PS2_COMPILE

		case I_CTRL:
#ifdef INLINE_DEBUG_READ
			debug << "; I_CTRL = " << hex << _INTC->I_CTRL_Reg;
#endif

			Output = _INTC->I_CTRL_Reg;
			_INTC->I_CTRL_Reg = 0;
			break;
			
#endif
			
		default:
			cout << "hps1x64 WARNING: READ from unknown INTC Register @ Cycle#" << dec << *_DebugCycleCount << " PC=" << hex << *_DebugPC << " Address=" << Address;
			break;
	}
	
	return Output;
	
}



static void Intc::Write ( u32 Address, u32 Data, u32 Mask )
{
#ifdef INLINE_DEBUG_WRITE
	debug << "\r\nIntc::Write; " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
#endif

	// make sure the address is in the correct range
	//if ( Address < Regs_Start || Address > Regs_End ) return;
	
	// handle writing of value to interrupt controller register
	switch ( Address )
	{
		case I_STAT:
#ifdef INLINE_DEBUG_WRITE
			debug << "; (Before) I_STAT = " << hex << _INTC->I_STAT_Reg.Value;
#endif
		
			// logical AND with value
			//I_STAT_Reg.Value &= Data;
			//I_STAT_Reg.Value = ( I_STAT_Reg.Value & Data & I_MASK_Reg.Value );	//n_irqdata = ( n_irqdata & ~mem_mask ) | ( n_irqdata & n_irqmask & data );
			_INTC->I_STAT_Reg.Value &= Data;	// ( I_STAT_Reg.Value & Data & I_MASK_Reg.Value );
			
			_INTC->UpdateInts ();
			
#ifdef INLINE_DEBUG_WRITE
			debug << "; (After) I_STAT = " << hex << _INTC->I_STAT_Reg.Value;
#endif

			break;
		
		case I_MASK:
#ifdef INLINE_DEBUG_WRITE
			debug << "; (Before) I_MASK = " << hex << _INTC->I_MASK_Reg.Value;
#endif
		
			// set value
			_INTC->I_MASK_Reg.Value = Data;	//n_irqmask = ( n_irqmask & ~mem_mask ) | data;
			
			_INTC->UpdateInts ();
			
#ifdef INLINE_DEBUG_WRITE
			debug << "; (After) I_MASK = " << hex << _INTC->I_MASK_Reg.Value;
#endif

			break;


#ifdef PS2_COMPILE

		case I_CTRL:
#ifdef INLINE_DEBUG_WRITE
			debug << "; (Before) I_CTRL = " << hex << _INTC->I_CTRL_Reg;
#endif
		
			// set value
			_INTC->I_CTRL_Reg = Data;
			
			//_INTC->UpdateInts ();
			
#ifdef INLINE_DEBUG_WRITE
			debug << "; (After) I_CTRL = " << hex << _INTC->I_CTRL_Reg;
#endif

			break;
			
#endif
			
			
		default:
			cout << "hps1x64 WARNING: WRITE to unknown INTC Register @ Cycle#" << dec << *_DebugCycleCount << " PC=" << hex << *_DebugPC << " Address=" << Address;
			break;

	}
	
}

void Intc::SetInterrupt ( u32 Interrupt )
{
	I_STAT_Reg.Value |= Interrupt;
}

void Intc::ClearInterrupt ( u32 Interrupt )
{
	I_STAT_Reg.Value &= ~Interrupt;
}


// *** TODO *** need to make this more of an "UPDATE" function that runs less often
u32 Intc::Run ( u32 Interrupt )
{
	u32 SignalingInts, Temp;

	/*
	// *** testing *** vblank interrupts look like they are queued
	// check what interrupts are already signalling
	if ( I_STAT_Reg.Value & Interrupt & 1 )
	{
		QueuedInterrupts = 1;
	}
	
	// check if queued vblank interrupt is cleared
	if ( !( I_STAT_Reg.Value & 1 ) && ( QueuedInterrupts ) )
	{
		I_STAT_Reg.Value |= 1;
		QueuedInterrupts = 0;
	}
	*/

	I_STAT_Reg.Value |= Interrupt;

	if ( I_STAT_Reg.Value & I_MASK_Reg.Value ) return ( 1 << 10 );

	return 0;
}


void Intc::UpdateInts ()
{
	// need to get all the interrupt statuses
	
	
	// if interrupts are still set, then set them back when cleared
	if ( _INTC->I_STAT_Reg.Value & _INTC->I_MASK_Reg.Value )
	{
		*_R3000A_Cause_13 |= ( 1 << 10 );
	}
	else
	{
		*_R3000A_Cause_13 &= ~( 1 << 10 );
	}
	
	if ( ( *_R3000A_Cause_13 & *_R3000A_Status_12 & 0xff00 ) && ( *_R3000A_Status_12 & 1 ) ) *_ProcStatus |= ( 1 << 20 );
}



static void Intc::DebugWindow_Enable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	static const char* DebugWindow_Caption = "PS1 INTC Debug Window";
	static const int DebugWindow_X = 10;
	static const int DebugWindow_Y = 10;
	static const int DebugWindow_Width = 200;
	static const int DebugWindow_Height = 200;
	
	static const int List_X = 0;
	static const int List_Y = 0;
	static const int List_Width = 150;
	static const int List_Height = 180;
	
	int i;
	stringstream ss;
	
	if ( !DebugWindow_Enabled )
	{
		// create the main debug window
		DebugWindow = new WindowClass::Window ();
		DebugWindow->Create ( DebugWindow_Caption, DebugWindow_X, DebugWindow_Y, DebugWindow_Width, DebugWindow_Height );
		DebugWindow->DisableCloseButton ();
		
		// create "value lists"
		ValueList = new DebugValueList<u32> ();
		ValueList->Create ( DebugWindow, List_X, List_Y, List_Width, List_Height, true, false );
		
		ValueList->AddVariable ( "IMASK", & _INTC->I_MASK_Reg.Value );
		ValueList->AddVariable ( "ISTAT", & _INTC->I_STAT_Reg.Value );
		
#ifdef PS2_COMPILE
		ValueList->AddVariable ( "ICTRL", & _INTC->I_CTRL_Reg );
#endif

		// mark debug as enabled now
		DebugWindow_Enabled = true;
		
		// update the value lists
		DebugWindow_Update ();
	}
	
#endif

}

static void Intc::DebugWindow_Disable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	int i;
	
	if ( DebugWindow_Enabled )
	{
		delete DebugWindow;
		delete ValueList;
	
		// disable debug window
		DebugWindow_Enabled = false;
	}
	
#endif

}

static void Intc::DebugWindow_Update ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	int i;
	
	if ( DebugWindow_Enabled )
	{
		ValueList->Update();
	}
	
#endif

}




