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


// need a define to toggle sse2 on and off for now
// on 64-bit systems, sse2 is supposed to come as standard
//#define _ENABLE_SSE2
//#define _ENABLE_SSE41


#include "VU.h"
#include "VU_Print.h"
#include "VU_Execute.h"


// used for restarting VU Dma transfer - unless its going to check every cycle, needs to alert dma to recheck ability to transfer
#include "PS2_Dma.h"

#include "PS2_GPU.h"


using namespace Playstation2;
using namespace Vu;
//using namespace x64Asm::Utilities;
//using namespace Math::Reciprocal;





// enable debugging

#ifdef _DEBUG_VERSION_

#define INLINE_DEBUG_ENABLE

/*
#define INLINE_DEBUG_READ
#define INLINE_DEBUG_WRITE
#define INLINE_DEBUG_DMA_READ
#define INLINE_DEBUG_DMA_WRITE
#define INLINE_DEBUG_FIFO

#define INLINE_DEBUG_VUCOM
#define INLINE_DEBUG_VURUN
*/

#define INLINE_DEBUG_INVALID

//#define INLINE_DEBUG_EXECUTE

#endif


VU *VU::_VU [ VU::c_iMaxInstances ];


VU *VU0::_VU0;
VU *VU1::_VU1;

int VU::iInstance = 0;


u32* VU::_DebugPC;
u64* VU::_DebugCycleCount;

//u32* VU::_Intc_Master;
u32* VU::_Intc_Stat;
u32* VU::_Intc_Mask;
u32* VU::_R5900_Status_12;
u32* VU::_R5900_Cause_13;
u64* VU::_ProcStatus;



//VU* VU::_VU;


u64* VU::_NextSystemEvent;


// needs to be removed sometime - no longer needed
u32* VU::DebugCpuPC;




bool VU::DebugWindow_Enabled [ VU::c_iMaxInstances ];

#ifdef ENABLE_GUI_DEBUGGER
WindowClass::Window *VU::DebugWindow [ VU::c_iMaxInstances ];
DebugValueList<float> *VU::GPR_ValueList [ VU::c_iMaxInstances ];
DebugValueList<u32> *VU::COP0_ValueList [ VU::c_iMaxInstances ];
DebugValueList<u32> *VU::COP2_CPCValueList [ VU::c_iMaxInstances ];
DebugValueList<u32> *VU::COP2_CPRValueList [ VU::c_iMaxInstances ];
Debug_DisassemblyViewer *VU::DisAsm_Window [ VU::c_iMaxInstances ];
Debug_BreakpointWindow *VU::Breakpoint_Window [ VU::c_iMaxInstances ];
Debug_MemoryViewer *VU::ScratchPad_Viewer [ VU::c_iMaxInstances ];
Debug_BreakPoints *VU::Breakpoints [ VU::c_iMaxInstances ];
#endif



Debug::Log VU::debug;



static const char* VU::VU0RegNames [ 24 ] = { "STAT", "FBRST", "ERR", "MARK", "CYCLE", "MODE", "NUM", "MASK", "CODE", "ITOPS", "RES", "RES", "RES", "ITOP", "RES", "RES",
											"R0", "R1", "R2", "R3", "C0", "C1", "C2", "C3" };
											
static const char* VU::VU1RegNames [ 24 ] = { "STAT", "FBRST", "ERR", "MARK", "CYCLE", "MODE", "NUM", "MASK", "CODE", "ITOPS", "BASE", "OFST", "TOPS", "ITOP", "TOP", "RES",
											"R0", "R1", "R2", "R3", "C0", "C1", "C2", "C3" };




VU::VU ()
{

	cout << "Running VU constructor...\n";
}

void VU::Reset ()
{
	// zero object
	memset ( this, 0, sizeof( VU ) );


	
}



// actually, you need to start objects after everything has been initialized
void VU::Start ( int iNumber )
{
	cout << "Running VU::Start...\n";
	
	
#ifdef INLINE_DEBUG_ENABLE
	if ( !iNumber )
	{
	debug.Create ( "PS2_VU_Log.txt" );
	}
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering VU::Start";
#endif


	cout << "Resetting VU...\n";

	Reset ();

	cout << "Starting VU Lookup object...\n";
	
	Vu::Instruction::Lookup::Start ();
	
	cout << "Starting VU Print object...\n";
	
	Vu::Instruction::Print::Start ();
	
	// only need to start the VU Execute once, or else there are issues with debugging
	if ( !iNumber )
	{
		Vu::Instruction::Execute::Start ();
	}
	
	// set the vu number
	Number = iNumber;
	
	// set as current GPU object
	if ( Number < c_iMaxInstances )
	{
		_VU [ Number ] = this;
		
		if ( !Number )
		{
			ulMicroMem_Start = c_ulMicroMem0_Start;
			ulMicroMem_Size = c_ulMicroMem0_Size;
			ulMicroMem_Mask = c_ulMicroMem0_Mask;
			ulVuMem_Start = c_ulVuMem0_Start;
			ulVuMem_Size = c_ulVuMem0_Size;
			ulVuMem_Mask = c_ulVuMem0_Mask;
		}
		else
		{
			ulMicroMem_Start = c_ulMicroMem1_Start;
			ulMicroMem_Size = c_ulMicroMem1_Size;
			ulMicroMem_Mask = c_ulMicroMem1_Mask;
			ulVuMem_Start = c_ulVuMem1_Start;
			ulVuMem_Size = c_ulVuMem1_Size;
			ulVuMem_Mask = c_ulVuMem1_Mask;
		}
		
#ifdef ENABLE_GUI_DEBUGGER
		cout << "\nVU#" << Number << " breakpoint instance";
		Breakpoints [ Number ] = new Debug_BreakPoints ( NULL, NULL, NULL );
#endif
	}
	

	// update number of object instances
	iInstance++;
	

	cout << "done\n";

#ifdef INLINE_DEBUG
	debug << "->Exiting VU::Start";
#endif

	cout << "Exiting VU::Start...\n";
}







/*
void VU::SetNextEvent ( u64 CycleOffset )
{
	NextEvent_Cycle = CycleOffset + *_DebugCycleCount;
	
	Update_NextEventCycle ();
}


void VU::SetNextEvent_Cycle ( u64 Cycle )
{
	NextEvent_Cycle = Cycle;
	
	Update_NextEventCycle ();
}

void VU::Update_NextEventCycle ()
{
	if ( NextEvent_Cycle > *_DebugCycleCount && ( NextEvent_Cycle < *_NextSystemEvent || *_NextSystemEvent <= *_DebugCycleCount ) ) *_NextSystemEvent = NextEvent_Cycle;
}
*/





void VU::Write_CTC ( u32 Register, u32 Data )
{
	// check if INT reg
	if ( Register < 16 )
	{
		vi [ Register ].sLo = (s16) Data;
	}
	else
	{
		// control register //
		
		switch ( Register )
		{
			// FBRST
			case 28:
				
				// check if we need to reset VU0
				if ( Data & 0x2 )
				{
					// reset VU0
					// do this for now
					_VU [ 0 ]->lVifIdx = 0;
					_VU [ 0 ]->lVifCodeState = 0;
					
					// set DBF to zero (for VU double buffering)
					_VU [ 0 ]->VifRegs.STAT.DBF = 0;
				}
				
				// check if we need to reset VU1
				if ( Data & 0x200 )
				{
					// reset VU1
					// do this for now
					_VU [ 1 ]->lVifIdx = 0;
					_VU [ 1 ]->lVifCodeState = 0;
					
					// set DBF to zero (for VU double buffering)
					_VU [ 1 ]->VifRegs.STAT.DBF = 0;
				}
				
				// clear bits 0,1 and 8,9
				Data &= ~0x303;
				
				// write register value
				vi [ Register ].u = Data;
				
				break;
				
			// CMSAR1 - runs vu1 from address on write
			case 31:
				cout << "\nhps2x64: ALERT: writing to CMSAR1!\n";
				vi [ Register ].u = Data;
				break;
				
			default:
				vi [ Register ].u = Data;
				break;
		}
	}
	
	
}

u32 VU::Read_CFC ( u32 Register )
{
	return vi [ Register ].u;
}




u64 VU::Read ( u32 Address, u64 Mask )
{
	//u32 Temp;
	u64 Output = 0;
	u32 lReg;

#ifdef INLINE_DEBUG_READ
	debug << "\r\n\r\nVU::Read; " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << " Mask=" << Mask;
	debug << "; VU#" << Number;
	if ( !Number )
	{
	if ( ( ( ( Address & 0xffff ) - 0x3800 ) >> 4 ) < c_iNumVuRegs )
	{
	// vu0
	debug << "; " << VU0RegNames [ ( ( Address & 0xffff ) - 0x3800 ) >> 4 ];
	}
	}
	else
	{
	if ( ( ( ( Address & 0xffff ) - 0x3c00 ) >> 4 ) < c_iNumVuRegs )
	{
	// vu1
	debug << "; " << VU1RegNames [ ( ( Address & 0xffff ) - 0x3c00 ) >> 4 ];
	}
	}
#endif

	// check if Address is for the registers or the fifo
	if ( ( Address & 0xffff ) < 0x4000 )
	{
		// will set some values here for now
		switch ( Address )
		{
			default:
				break;
		}
		
		lReg = ( Address >> 4 ) & 0x1f;
		
		if ( lReg > c_iNumVuRegs )
		{
			Output = VifRegs.Regs [ lReg ];
		}

		switch ( Address )
		{
			default:
				break;
		}
	}
	
	
#ifdef INLINE_DEBUG_READ
	debug << "; Output=" << hex << Output;
#endif

	return Output;
}


void VU::Write ( u32 Address, u64 Data, u64 Mask )
{
	u32 lReg;
	u32 QWC_Transferred;
	//u32 ulTempArray [ 4 ];
	
#ifdef INLINE_DEBUG_WRITE
	debug << "\r\n\r\nVU::Write; " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Data = " << Data << " Mask=" << Mask;
	debug << "; VU#" << Number;
	if ( !Number )
	{
	if ( ( ( ( Address & 0xffff ) - 0x3800 ) >> 4 ) < c_iNumVuRegs )
	{
	// vu0
	debug << "; " << VU0RegNames [ ( ( Address & 0xffff ) - 0x3800 ) >> 4 ];
	}
	else if ( ( Address & 0xffff ) == 0x4000 )
	{
	debug << "; VIF0FIFO";
	}
	}
	else
	{
	if ( ( ( ( Address & 0xffff ) - 0x3c00 ) >> 4 ) < c_iNumVuRegs )
	{
	// vu1
	debug << "; " << VU1RegNames [ ( ( Address & 0xffff ) - 0x3c00 ) >> 4 ];
	}
	else if ( ( Address & 0xffff ) == 0x5000 )
	{
	debug << "; VIF1FIFO";
	}
	}
#endif

	if ( ( Address & 0xffff ) < 0x4000 )
	{
		// get register number being written to
		lReg = ( Address >> 4 ) & 0x1f;
		
		// perform actions before write
		switch ( lReg )
		{
			// FBRST
			case 0x1:
			
				// when 1 is written, it means to reset VU completely
				// bit 0 - reset VU (when writing to memory mapped register. when writing to VU0 int reg 28, then it is bit 1)
				if ( Data & 0x1 )
				{
					// reset VU
					//Reset ();
					
					// do this for now
					_VU [ 0 ]->lVifIdx = 0;
					_VU [ 0 ]->lVifCodeState = 0;
					
					// set DBF to zero (for VU double buffering)
					_VU [ 0 ]->VifRegs.STAT.DBF = 0;
					
					// clear bit 0
					Data &= 0x1;
				}
				
				break;
				
			default:
				break;
		}

		
		if ( lReg > c_iNumVuRegs )
		{
			VifRegs.Regs [ lReg ] = Data;
		}

		// perform actions after write
		switch ( Address )
		{
			default:
				break;
		}
	}
	else
	{
		QWC_Transferred = VIF_FIFO_Execute ( (u32*) Data, 4 );
		
		if ( QWC_Transferred != 1 )
		{
#ifdef INLINE_DEBUG_WRITE
			debug << " QWC_Transferred=" << dec << QWC_Transferred;
#endif

			cout << "\nhps2x64 ALERT: VU: non-dma transfer did not completely execute\n";
		}
	}
}


// VIF Codes are 32-bits. can always do a cast to 64/128-bits as needed
// need this to return the number of quadwords read and update the offset so it points to correct data for next time
u32 VU::VIF_FIFO_Execute ( u32* Data, u32 SizeInWords32 )
{
	u32 ulTemp;
	u32 *DstPtr32;
	
	u32 lWordsToTransfer;
	
	u32 QWC_Transferred;
	
	// need an index for just the data being passed
	u32 lIndex = 0;
	
	u32 lWordsLeft = SizeInWords32;
	
	// also need to subtract lVifIdx & 0x3 from the words left
	// to take over where we left off at
	lWordsLeft -= ( lVifIdx & 0x3 );
	
#ifdef INLINE_DEBUG_VUCOM
	debug << " lVifIdx=" << hex << lVifIdx;
#endif

	// start reading from index offset (may have left off in middle of data QW block)
	Data = & ( Data [ lVifIdx & 0x3 ] );
	
	while ( lWordsLeft )
	{
		// check if vif code processing is in progress
		if ( !lVifCodeState )
		{
			// load vif code
			//VifCode.Value = Data [ lIndex++ ];
			VifCode.Value = *Data++;
			
			// update index
			lVifIdx++;
			
			// vif code is 1 word
			lWordsLeft--;
		}
		
		// perform the action for the vif code
		switch ( VifCode.CMD )
		{
			// NOP
			case 0:
#ifdef INLINE_DEBUG_VUCOM
	debug << " NOP";
#endif

				// check interrupt bit to see if there should be an interrupt
			
				break;
			
			// STCYCL
			case 1:
#ifdef INLINE_DEBUG_VUCOM
	debug << " STCYCL";
#endif

				// set CYCLE register value - first 8 bits are CL, next 8-bits are WL
				// set from IMM
				// Vu0 and Vu1
				VifRegs.CYCLE.Value = VifCode.IMMED;
				break;

			// OFFSET
			case 2:
#ifdef INLINE_DEBUG_VUCOM
	debug << " OFFSET";
#endif

				// set OFFSET register value - first 10 bits of IMM
				// set from IMM
				// Vu1 ONLY
				VifRegs.OFST = VifCode.IMMED;
				
				// set TOPS to BASE
				// *note* should probably use DBF to see what this gets set to
				//VifRegs.TOPS = VifRegs.BASE;
				// set TOPS immediately
				VifRegs.TOPS = VifRegs.BASE + ( VifRegs.OFST * ( (u32) VifRegs.STAT.DBF ) );
				
				// set DBF Flag to zero
				// *note* probably should not do this. probably only reset should set it to zero
				//VifRegs.STAT.DBF = 0;
				
#ifdef INLINE_DEBUG_VUCOM
	debug << hex << " (OFFSET=" << VifRegs.OFST << " TOPS=" << VifRegs.TOPS << ")";
#endif

				break;

			// BASE
			case 3:
#ifdef INLINE_DEBUG_VUCOM
	debug << " BASE";
#endif

				// set BASE register value - first 10 bits of IMM
				// set from IMM
				// Vu1 ONLY
				VifRegs.BASE = VifCode.IMMED;
				
#ifdef INLINE_DEBUG_VUCOM
	debug << hex << " (BASE=" << VifRegs.BASE << ")";
#endif
				break;

			// ITOP
			case 4:
#ifdef INLINE_DEBUG_VUCOM
	debug << " ITOP";
#endif

				// set data pointer (ITOPS register) - first 10 bits of IMM
				// set from IMM
				// Vu0 and Vu1
				VifRegs.ITOPS = VifCode.IMMED;
				break;

			// STMOD
			case 5:
#ifdef INLINE_DEBUG_VUCOM
	debug << " STMOD";
#endif

				// set mode of addition decompression (MODE register) - first 2 bits of IMM
				// set from IMM
				// Vu0 and Vu1
				VifRegs.MODE = VifCode.IMMED;
				break;

			// MSKPATH3
			case 6:
#ifdef INLINE_DEBUG_VUCOM
	debug << " MSKPATH3";
#endif


				// set mask of path3 - bit 15 - 0: enable path3 transfer, 1: disable path3 transfer
				// set from IMM
				// Vu1 ONLY
				break;

			// MARK
			case 7:
#ifdef INLINE_DEBUG_VUCOM
	debug << " MARK";
#endif

				// set mark register
				// set from IMM
				// Vu0 and Vu1
				VifRegs.MARK = VifCode.IMMED;
				break;

			// FLUSHE
			case 16:
#ifdef INLINE_DEBUG_VUCOM
	debug << " FLUSHE";
#endif

				// waits for vu program to finish
				// Vu0 and Vu1
				
				if ( Running )
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					VifStopped = 0;
				}
				
				break;

			// FLUSH
			case 17:
#ifdef INLINE_DEBUG_VUCOM
	debug << " FLUSH";
#endif

				// waits for end of vu program and end of path1/path2 transfer
				// Vu1 ONLY
				// ***todo*** wait for end of path1/path2 transfer
				if ( Running )
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					VifStopped = 0;
				}
				
				break;

			// FLUSHA
			case 19:
#ifdef INLINE_DEBUG_VUCOM
	debug << " FLUSHA";
#endif

				// waits for end of vu program and end of transfer to gif
				// Vu1 ONLY
				// ***todo*** wait for end of transfer to gif
				if ( Running )
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					VifStopped = 0;
				}
				
				break;

			// MSCAL
			case 20:
#ifdef INLINE_DEBUG_VUCOM
	debug << " MSCAL";
#endif

				// waits for end of current vu program and runs new vu program starting at IMM*8
				// Vu0 and Vu1
				
				if ( Running )
				{
					// a VU program is in progress already //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (MSCNT_PENDING)";
#endif

					// there is a pending MSCAL Command -> for now just back up index
					lVifIdx--;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
					// VU program is NOT in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					VifStopped = 0;
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (RUNVU)";
#endif

					// set PC = IMM * 8
					PC = VifCode.IMMED << 3;
					
					// VU is now running
					Running = 1;
					
					// set VBSx in VPU STAT to 1 (running)
					VU0::_VU0->vi [ 29 ].uLo |= ( 1 << ( Number << 3 ) );

					// set TOP to TOPS
					VifRegs.TOP = VifRegs.TOPS;
					
					// set ITOP to ITOPS ??
					VifRegs.ITOP = VifRegs.ITOPS;
					
					// reverse DBF flag
					VifRegs.STAT.DBF ^= 1;
					
					// set TOPS
					VifRegs.TOPS = VifRegs.BASE + ( VifRegs.OFST * ( (u32) VifRegs.STAT.DBF ) );
				}
				
				
				break;

			// MSCALF
			case 21:
#ifdef INLINE_DEBUG_VUCOM
	debug << " MSCALF";
#endif

				// waits for end of current vu program and gif path1/path2 transfer, then runs program starting from IMM*8
				// Vu0 and Vu1
				
				// *** TODO *** wait for end of PATH1/PATH2 transfer
				if ( Running )
				{
					// VU program is in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (MSCNT_PENDING)";
#endif

					// there is a pending MSCALF Command -> for now just back up index
					lVifIdx--;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
					// VU program is NOT in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					VifStopped = 0;
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (RUNVU)";
#endif

					// set PC = IMM * 8
					PC = VifCode.IMMED << 3;
					
					// VU is now running
					Running = 1;
					
					// set VBSx in VPU STAT to 1 (running)
					VU0::_VU0->vi [ 29 ].uLo |= ( 1 << ( Number << 3 ) );
					
					// set TOPS to TOP
					VifRegs.TOP = VifRegs.TOPS;
					
					// set ITOPS to ITOP ??
					VifRegs.ITOP = VifRegs.ITOPS;
					
					// reverse DBF flag
					VifRegs.STAT.DBF ^= 1;
					
					// set TOPS
					VifRegs.TOPS = VifRegs.BASE + ( VifRegs.OFST * ( (u32) VifRegs.STAT.DBF ) );
				}
				
				break;
				
				
			// MSCNT
			case 23:
#ifdef INLINE_DEBUG_VUCOM
	debug << " MSCNT";
#endif

				// waits for end of current vu program and starts next one where it left off
				// Vu0 and Vu1
				
				if ( Running )
				{
					// VU program is in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFSTOP)";
#endif

					VifStopped = 1;
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (MSCNT_PENDING)";
#endif

					// there is a pending MSCNT Command -> for now just back up index
					lVifIdx--;
					
					// just return for now //
					
					// get the number of full quadwords completely processed
					QWC_Transferred = lVifIdx >> 2;
					
					// get start index of remaining data to process in current block
					lVifIdx &= 0x3;
					
					// return the number of full quadwords transferred/processed
					return QWC_Transferred;
				}
				else
				{
					// VU program is NOT in progress //
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (VIFCNT)";
#endif

					VifStopped = 0;
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " (RUNVU)";
#endif

					// VU is now running
					Running = 1;
					
					// set VBSx in VPU STAT to 1 (running)
					VU0::_VU0->vi [ 29 ].uLo |= ( 1 << ( Number << 3 ) );
					
					// set TOP to TOPS
					VifRegs.TOP = VifRegs.TOPS;
					
					// set ITOP to ITOPS ??
					VifRegs.ITOP = VifRegs.ITOPS;
					
					// reverse DBF flag
					VifRegs.STAT.DBF ^= 1;
					
					// set TOPS
					VifRegs.TOPS = VifRegs.BASE + ( VifRegs.OFST * ( (u32) VifRegs.STAT.DBF ) );
				}
				
				break;

			// STMASK
			case 32:
#ifdef INLINE_DEBUG_VUCOM
	debug << " STMASK";
#endif

				// Vu0 and Vu1
				
				if ( !lVifCodeState )
				{
					// there is more incoming data than just the tag
					lVifCodeState++;
				}

				// if processing next element, also check there is data to process
				if ( lWordsLeft )
				{
					// store mask
					//VifRegs.MASK = Data [ lIndex++ ];
					VifRegs.MASK = *Data++;
					
					// update index
					lVifIdx++;
					
					// command size is 1+1 words
					lWordsLeft--;
					
					// command is done
					lVifCodeState = 0;
				}
				
				break;

			// STROW
			case 48:
#ifdef INLINE_DEBUG_VUCOM
	debug << " STROW";
#endif

				static const int c_iSTROW_CommandSize = 5;

				// Vu0 and Vu1
				
				if ( !lVifCodeState )
				{
					// there is more incoming data than just the tag
					lVifCodeState++;
				}

				// if processing next element, also check there is data to process
				if ( lWordsLeft )
				{
					while ( lWordsLeft && ( lVifCodeState < c_iSTROW_CommandSize ) )
					{
						// store ROW (R0-R3)
						// register offset starts at 16, buf lVifCodeState will start at 1
						//VifRegs.Regs [ 15 + lVifCodeState ] = Data [ lIndex++ ];
						VifRegs.Regs [ 15 + lVifCodeState ] = *Data++;
						
						// update index
						lVifIdx++;
						
						// move to next data input element
						lVifCodeState++;
						
						// command size is 1+4 words
						lWordsLeft--;
					}
					
					if ( lVifCodeState >= c_iSTROW_CommandSize )
					{
						// command is done
						lVifCodeState = 0;
					}
				}
				
				break;

			// STCOL
			case 49:
#ifdef INLINE_DEBUG_VUCOM
	debug << " STCOL";
#endif

				static const int c_iSTCOL_CommandSize = 5;

				// Vu0 and Vu1
				
				if ( !lVifCodeState )
				{
					// there is more incoming data than just the tag
					lVifCodeState++;
				}

				// if processing next element, also check there is data to process
				if ( lWordsLeft )
				{
					while ( lWordsLeft && ( lVifCodeState < c_iSTCOL_CommandSize ) )
					{
						// store ROW (R0-R3)
						// register offset starts at 16, buf lVifCodeState will start at 1
						//VifRegs.Regs [ 19 + lVifCodeState ] = Data [ lIndex++ ];
						VifRegs.Regs [ 19 + lVifCodeState ] = *Data++;
						
						// update index
						lVifIdx++;
						
						// move to next data input element
						lVifCodeState++;
						
						// command size is 1+4 words
						lWordsLeft--;
					}
					
					if ( lVifCodeState >= c_iSTCOL_CommandSize )
					{
						// command is done
						lVifCodeState = 0;
					}
				}
				
				break;

			// MPG
			case 74:
#ifdef INLINE_DEBUG_VUCOM
	debug << " MPG";
#endif

				// load following vu program of size NUM*2 into address IMM*8
				// Vu0 and Vu1
				
				if ( !lVifCodeState )
				{
					// set the command size
					lVifCommandSize = 1 + ( VifCode.NUM << 1 );
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " CommandSize32=" << dec << lVifCommandSize;
#endif

					// there is more incoming data than just the tag
					lVifCodeState++;
				}

				// if processing next element, also check there is data to process
				if ( lWordsLeft )
				{
					DstPtr32 = & MicroMem32 [ ( VifCode.IMMED << 1 ) + ( lVifCodeState - 1 ) ];
					while ( lWordsLeft && ( lVifCodeState < lVifCommandSize ) )
					{
						// store ROW (R0-R3)
						// register offset starts at 16, buf lVifCodeState will start at 1
						*DstPtr32++ = *Data++;
						
						// update index
						lVifIdx++;
						
						// move to next data input element
						lVifCodeState++;
						
						// command size is 1+NUM*2 words
						lWordsLeft--;
					}
					
					if ( lVifCodeState >= lVifCommandSize )
					{
						// command is done
						lVifCodeState = 0;
					}
				}

				break;

			// DIRECT
			case 80:
#ifdef INLINE_DEBUG_VUCOM
	debug << " DIRECT";
#endif

				// transfers data to GIF via path2 (correct GIF Tag required)
				// size 1+IMM*4, but IMM is 65536 when it is zero
				// Vu1 ONLY

				if ( !lVifCodeState )
				{
					// set the command size
					lVifCommandSize = 1 + ( ( !VifCode.IMMED ? 65536 : VifCode.IMMED ) << 2 );
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " CommandSize32=" << dec << lVifCommandSize;
#endif

					// there is more incoming data than just the tag
					lVifCodeState++;
				}

				// if processing next element, also check there is data to process
				if ( lWordsLeft )
				{
					while ( ( lWordsLeft > 0 ) && ( lVifCodeState < lVifCommandSize ) )
					{
						// get amount to transfer from this block
						lWordsToTransfer = lVifCommandSize - lVifCodeState;
						if ( lWordsToTransfer > lWordsLeft ) lWordsToTransfer = lWordsLeft;
						
						// ***TODO*** send graphics data to GIF
						// store ROW (R0-R3)
						// register offset starts at 16, buf lVifCodeState will start at 1
						//GPU::Path2_WriteBlock ( (u64*) Data, 1 );
						GPU::Path2_WriteBlock ( (u64*) Data, lWordsToTransfer >> 2 );
						
						//Data += 4;
						Data += lWordsToTransfer;
						
						// update index
						lVifIdx += lWordsToTransfer;
						
						// move to next data input element
						//lVifCodeState++;
						//lVifCodeState += 4;
						lVifCodeState += lWordsToTransfer;
						
						// command size is 1+NUM*2 words
						//lWordsLeft--;
						//lWordsLeft -= 4;
						lWordsLeft -= lWordsToTransfer;
					}
					
					if ( lVifCodeState >= lVifCommandSize )
					{
						// command is done
						lVifCodeState = 0;
					}
				}
				
				break;

			// DIRECTHL
			case 81:
#ifdef INLINE_DEBUG_VUCOM
	debug << " DIRECTHL";
#endif

				// transfers data to GIF via path2 (correct GIF Tag required)
				// size 1+IMM*4, but IMM is 65536 when it is zero
				// stalls until PATH3 transfer is complete
				// Vu1 ONLY
				
				if ( !lVifCodeState )
				{
					// set the command size
					lVifCommandSize = 1 + ( ( !VifCode.IMMED ? 65536 : VifCode.IMMED ) << 2 );
					
#ifdef INLINE_DEBUG_VUCOM
	debug << " CommandSize32=" << dec << lVifCommandSize;
#endif

					// there is more incoming data than just the tag
					lVifCodeState++;
				}

				// if processing next element, also check there is data to process
				if ( lWordsLeft )
				{
					while ( ( lWordsLeft > 0 ) && ( lVifCodeState < lVifCommandSize ) )
					{
						// get amount to transfer from this block
						lWordsToTransfer = lVifCommandSize - lVifCodeState;
						if ( lWordsToTransfer > lWordsLeft ) lWordsToTransfer = lWordsLeft;
						
						// ***TODO*** send graphics data to GIF
						// store ROW (R0-R3)
						// register offset starts at 16, buf lVifCodeState will start at 1
						//GPU::Path2_WriteBlock ( (u64*) Data, 1 );
						GPU::Path2_WriteBlock ( (u64*) Data, lWordsToTransfer >> 2 );
						
						//Data += 4;
						Data += lWordsToTransfer;
						
						// update index
						lVifIdx += lWordsToTransfer;
						
						// move to next data input element
						//lVifCodeState++;
						//lVifCodeState += 4;
						lVifCodeState += lWordsToTransfer;
						
						// command size is 1+NUM*2 words
						//lWordsLeft--;
						//lWordsLeft -= 4;
						lWordsLeft -= lWordsToTransfer;
					}
					
					if ( lVifCodeState >= lVifCommandSize )
					{
						// command is done
						lVifCodeState = 0;
					}
				}
				
				break;
				
			default:
			
				if ( ( ( VifCode.Value >> 29 ) & 0x3 ) == 0x3 )
				{
#ifdef INLINE_DEBUG_VUCOM
	debug << " UNPACK";
#endif

					// unpacks data into vu data memory
					// Vu0 and Vu1
					if ( !lVifCodeState )
					{
						// set the command size
						if ( VifRegs.CYCLE.WL <= VifRegs.CYCLE.CL )
						{
							// WL <= CL
							ulTemp = VifCode.NUM;
							//ulTemp = ( ( ( 32 >> VifCode.vl ) * ( VifCode.vn + 1 ) ) * VifCode.NUM );
							
						}
						else
						{
							// WL > CL
							
							// NOTE: either this could cause a crash if WL is zero...
							ulTemp = VifCode.NUM % VifRegs.CYCLE.WL;
							ulTemp = ( ulTemp > VifRegs.CYCLE.CL ) ? VifRegs.CYCLE.CL : ulTemp;
							ulTemp = VifRegs.CYCLE.CL * ( VifCode.NUM / VifRegs.CYCLE.WL ) + ulTemp;
							//ulTemp = ( ( ( 32 >> VifCode.vl ) * ( VifCode.vn + 1 ) ) * ulTemp );
						}
						
						ulTemp = ( ( 32 >> VifCode.vl ) * ( VifCode.vn + 1 ) ) * ulTemp;
						lVifCommandSize = 1 + ( ulTemp >> 5 ) + ( ( ulTemp & 0x1f ) ? 1 : 0 );
						
#ifdef INLINE_DEBUG_VUCOM
	debug << " CommandSize32=" << dec << lVifCommandSize;
#endif

						// alert for values of m, vn, and vl for now
						// m=0, vn=3, vl=0
						cout << "\nhps2x64: VU: m=" << VifCode.m;
						cout << "\nhps2x64: VU: vn=" << VifCode.vn;
						cout << "\nhps2x64: VU: vl=" << VifCode.vl;

						// there is more incoming data than just the tag
						lVifCodeState++;
						
#ifdef INLINE_DEBUG_VUCOM
	debug << hex << " (BASE=" << VifRegs.BASE << " OFFSET=" << VifRegs.OFST << " TOP=" << VifRegs.TOP << " TOPS=" << VifRegs.TOPS << ")";
#endif
					}
					
					if ( lWordsLeft )
					{
						// do this for now
						// add tops register if FLG set
						
						// check if FLG bit is set
						if ( VifCode.IMMED & 0x8000 )
						{
							// ***todo*** update to offset from current data being stored
							DstPtr32 = & VuMem32 [ ( ( VifCode.IMMED & 0x3ff ) + ( VifRegs.TOPS & 0x3ff ) ) << 2 ];
							DstPtr32 += ( lVifCodeState - 1 );
						}
						else
						{
							// ***todo*** update to offset from current data being stored
							DstPtr32 = & VuMem32 [ ( VifCode.IMMED & 0x3ff ) << 2 ];
							DstPtr32 += ( lVifCodeState - 1 );
						}
						
						while ( lWordsLeft && ( lVifCodeState < lVifCommandSize ) )
						{
							// store ROW (R0-R3)
							// register offset starts at 16, buf lVifCodeState will start at 1
							*DstPtr32++ = *Data++;
							
							// update index
							lVifIdx++;
							
							// move to next data input element
							lVifCodeState++;
							
							// command size is 1+NUM*2 words
							lWordsLeft--;
						}
						
						if ( lVifCodeState >= lVifCommandSize )
						{
							// command is done
							lVifCodeState = 0;
						}
					
					}

					break;
				}
				
#ifdef INLINE_DEBUG_VUCOM
	debug << " INVALID";
#endif
#ifdef INLINE_DEBUG_INVALID
	debug << "\r\nVU#" << Number << " Error. Invalid VIF Code=" << hex << VifCode.Value << " Cycle#" << dec << *_DebugCycleCount;
#endif

				cout << "\nhps2x64: VU#" << Number << " Error. Invalid VIF Code=" << hex << VifCode.Value;

				break;

		}
	}
	
	// get the number of full quadwords completely processed
	QWC_Transferred = lVifIdx >> 2;
	
	// get start index of remaining data to process in current block
	lVifIdx &= 0x3;
	
	// if vif code state is zero, then check if the previous code had an interrupt, and trigger if so
	if ( !lVifCodeState )
	{
		// check for interrupt
		if ( VifCode.INT )
		{
			debug << " ***INT***";
			cout << "\n***VUINT***\n";
			
			// ***todo*** need to send interrupt signal and stop transfer to Vif //
			SetInterrupt_VIF ();
			
			// ***todo*** stop vif transfer
		}
	}
	
	// return the number of full quadwords transferred/processed
	return QWC_Transferred;
}






void VU::DMA_Read ( u32* Data, int ByteReadCount )
{
#ifdef INLINE_DEBUG_DMA_READ
	debug << "\r\n\r\nDMA_Read " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; Data = " << hex << Data [ 0 ];
#endif

}




void VU::DMA_Write ( u32* Data, int ByteWriteCount )
{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "\r\n\r\nDMA_Write " << hex << setw ( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << "; Data = " << hex << Data [ 0 ];
#endif

}


u32 VU::DMA_WriteBlock ( u64* Data, u32 QuadwordCount )
{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "\r\n\r\nDMA_WriteBlock " << hex << setw ( 8 ) << *_DebugPC << " QWC=" << QuadwordCount << " " << dec << *_DebugCycleCount << hex << "; Data= ";
	debug << "; VU#" << Number;
	for ( int i = 0; i < ( QuadwordCount * 2 ); i++ ) debug << " " << Data [ i ];
	debug << "\r\n";
#endif

	u32 QWC_Transferred;
	
	// check if QWC is zero
	if ( !QuadwordCount )
	{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << " QWC_IS_ZERO";
#endif

		return 0;
	}
	
	// send data to VIF FIFO
	QWC_Transferred = VIF_FIFO_Execute ( (u32*) Data, QuadwordCount << 2 );
	
	if ( QWC_Transferred == QuadwordCount )
	{
		// transfer complete //
		lVifIdx = 0;
	}
	
	// return the amount of quadwords processed
	return QWC_Transferred;

	/*
	for ( int i = 0; i < QuadwordCount; i++ )
	{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "\r\n";
	debug << hex << Data [ 0 ] << " " << Data [ 1 ];
#endif

		VIF_FIFO_Execute ( Data [ 0 ], Data [ 1 ] );
		
		Data += 2;
	}
	*/
	
//#ifdef INLINE_DEBUG_DMA_WRITE
//	debug << " VIF#" << Number << " INT";
//#endif
	
	// ***testing*** interrupt??
	//SetInterrupt_VIF ();
}


u32 VU::DMA_ReadBlock ( u64* Data, u32 QuadwordCount )
{
#ifdef INLINE_DEBUG_DMA_READ
	debug << "\r\n\r\nDMA_ReadBlock " << hex << setw ( 8 ) << *_DebugPC << " QWC=" << QuadwordCount << " " << dec << *_DebugCycleCount << hex << "; Data= ";
	debug << "; VU#" << Number;
	for ( int i = 0; i < ( QuadwordCount * 2 ); i++ ) debug << " " << Data [ i ];
	debug << "\r\n";
#endif

}

bool VU::DMA_Read_Ready ()
{
	// do this for now
	// ***todo*** check if read from VU1 is ready
	return !VifStopped;
}

bool VU::DMA_Write_Ready ()
{
	return !VifStopped;
}



void VU::Run ()
{

	Instruction::Format CurInstLO;
	Instruction::Format CurInstHI;
	

	/////////////////////////////////
	// VU components:
	// 1. Instruction Execute Unit
	// 2. Delay Slot Unit
	// 3. Multiply/Divide Unit
	/////////////////////////////////
	
	
	// if VU is not running, update vu cycle and then return
	if ( !Running )
	{
		CycleCount = *_DebugCycleCount;
		return;
	}
	

#ifdef INLINE_DEBUG
	debug << "\r\n->PC = " << hex << setw( 8 ) << PC << dec;
#endif


	//cout << "\nVU -> running=" << Running;
	
	
	/////////////////////////
	// Execute Instruction //
	/////////////////////////
	
	
	// execute instruction
	//NextPC = PC + 4;
	
#ifdef INLINE_DEBUG
	debug << ";Execute";
#endif

	///////////////////////////////////////////////////////////////////////////////////////////
	// R0 is always zero - must be cleared before any instruction is executed, not after
	//GPR [ 0 ].u = 0;
	vi [ 0 ].u = 0;
	
	// f0 always 0, 0, 0, 1 ???
	vf [ 0 ].uLo = 0;
	vf [ 0 ].uHi = 0;
	vf [ 0 ].uw = 0x3f800000;
	
	//cout << "\nVU -> load lo";
	
	// load LO instruction
	//CurInst.Value = Bus->Read ( PC, 0xffffffff );
	CurInstLO.Value = MicroMem32 [ PC >> 2 ];
	
	//cout << "\nVU -> load hi";
	
	// load HI instruction
	//CurInst.Value = Bus->Read ( PC, 0xffffffff );
	CurInstHI.Value = MicroMem32 [ ( PC + 4 ) >> 2 ];
	
	//cout << "\nVU -> execute lo";
	
	// check if E-bit is set (means end of execution after E-bit delay slot)
	if ( CurInstHI.E ) Status.EBitDelaySlot_Valid |= 0x2;
	
	// alert if d or t is set
	if ( CurInstHI.D )
	{
		cout << "\nhps2x64: ALERT: VU#" << Number << " D-bit is set!\n";
	}
	
	if ( CurInstHI.T )
	{
		cout << "\nhps2x64: ALERT: VU#" << Number << " T-bit is set!\n";
	}
	
	
	// execute HI instruction first ??
	
	// check if Immediate or End of execution bit is set
	if ( CurInstHI.I )
	{
		// load immediate regiser with LO instruction
		//I.u = CurInstLO.Value;
		vi [ 21 ].u = CurInstLO.Value;
	}
	else
	{
		// execute LO instruction since it is an instruction rather than an immediate value
		Instruction::Execute::ExecuteInstructionLO ( this, CurInstLO );
	}
	
	//cout << "\nVU -> execute hi";
	
	// execute HI instruction
	Instruction::Execute::ExecuteInstructionHI ( this, CurInstHI );
	
	//cout << "\nVU -> update pc";
	
	// update instruction
	NextPC = PC + 8;
	
	///////////////////////////////////////////////////
	// Check if there is anything else going on
	
	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Check delay slots
	// need to do this before executing instruction because load delay slot might stall pipeline

	// check if anything is going on with the delay slots
	// *note* by doing this before execution of instruction, pipeline can be stalled as needed
	if ( Status.Value )
	{

		/////////////////////////////////////////////
		// check for anything in delay slots
		
#ifdef INLINE_DEBUG
		debug << ";DelaySlotValid";
#endif
		
		//if ( DelaySlot1.Value )
		if ( Status.DelaySlot_Valid & 1 )
		{
#ifdef INLINE_DEBUG
			debug << ";Delay1.Value";
#endif
			
			ProcessBranchDelaySlot ();
			
		}

		///////////////////////////////////
		// move delay slot
		NextDelaySlotIndex ^= 1;
		
		
		// check for end of execution for VU
		if ( Status.EBitDelaySlot_Valid & 1 )
		{
#ifdef INLINE_DEBUG_VURUN
			debug << "\r\nEBitDelaySlot; VUDONE";
			debug << " @Cycle#" << dec << *_DebugCycleCount;
#endif

			Running = 0;
			
			// set VBSx in VPU STAT to be zero (idle)
			VU0::_VU0->vi [ 29 ].uLo &= ~( 1 << ( Number << 3 ) );
			
			// if Vif is stopped for some reason, it can resume transfer now
			VifStopped = 0;
			
			// ***todo*** also need to notify DMA to restart if needed
			Dma::_DMA->Transfer ( Number );
			
#ifdef INLINE_DEBUG_VURUN
			debug << "->DMA Should be restarted";
#endif
		}

		//cout << hex << "\n" << DelaySlot1.Value << " " << DelaySlot1.Value2 << " " << DelaySlot0.Value << " " << DelaySlot0.Value2;
		
		///////////////////////////////////////////////////
		// Advance status bits for checking delay slot
		//Status.DelaySlot_Valid = ( Status.DelaySlot_Valid << 1 ) & 0x3;
		// ***todo*** could possibly combine these shifts into one
		Status.DelaySlot_Valid >>= 1;
		Status.EBitDelaySlot_Valid >>= 1;
		
		//////////////////////////////////////////////
		// check for Asynchronous Interrupts
		// also make sure interrupts are enabled
		//if ( Status.CheckInterrupt )
		//{
		//}
	}


	/////////////////////////////////////
	// Update Program Counter
	LastPC = PC;
	PC = NextPC;
	
	// this counts the bus cycles, not R5900 cycles
	CycleCount++;
}



void VU::ProcessBranchDelaySlot ()
{
	Vu::Instruction::Format i;
	u32 Address;
	
	DelaySlot *d = & ( DelaySlots [ NextDelaySlotIndex ] );
	
	//i = DelaySlot1.Instruction;
	i = d->Instruction;
	
	
	switch ( i.Opcode )
	{
		// B
		case 0x20:
			
		// BAL
		case 0x21:
	
		// IBEQ
		case 0x28:
		
		// IBGEZ
		case 0x2f:
		
		// IBGTZ
		case 0x2d:
		
		// IBLEZ
		case 0x2e:
		
		// IBLTZ
		case 0x2c:
		
		// IBNE
		case 0x29:
		
			NextPC = PC + ( i.Imm11 << 3 );
			break;
			
			
		// JALR
		case 0x25:
		
		// JR
		case 0x24:
		
			NextPC = d->Data;
			break;
	}
	
}



u32* VU::GetMemPtr ( u32 Address32 )
{
	u32 Reg;
	
	if ( !Number )
	{
		if ( Address32 & ( 0x4000 >> 2 ) )
		{
			// get the register number
			Reg = ( Address32 >> 2 ) & 0x1f;
			
			// check if these are float/int/etc registers being accessed
			switch ( ( Address32 >> 6 ) & 0xf )
			{
				// float
				case 0:
				case 1:
					return & VU1::_VU1->vf [ Reg ].uw0;
					break;
					
				// int/control
				case 2:
				case 3:
					return & VU1::_VU1->vi [ Reg ].u;
					break;
					
				default:
					cout << "\nhps2x64: ERROR: VU0: referencing VU1 reg outside of range. Address=" << hex << ( Address32 << 2 );
					break;
			}
		}
		
		return & ( VuMem32 [ Address32 & c_ulVuMem0_Mask ] );
	}
	
	return & ( VuMem32 [ Address32 & c_ulVuMem1_Mask ] );
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void VU::DebugWindow_Enable ( int Number )
{

#ifndef _CONSOLE_DEBUG_ONLY_

	static const char* COP0_Names [] = { "Index", "Random", "EntryLo0", "EntryLo1", "Context", "PageMask", "Wired", "Reserved",
								"BadVAddr", "Count", "EntryHi", "Compare", "Status", "Cause", "EPC", "PRId",
								"Config", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "BadPAddr",
								"Debug", "Perf", "Reserved", "Reserved", "TagLo", "TagHi", "ErrorEPC", "Reserved" };
								
	static const char* DisAsm_Window_ColumnHeadings [] = { "Address", "@", ">", "Instruction", "Inst (hex)" };
								
	static const char* FontName = "Courier New";
	static const int FontSize = 6;
	
	static const char* DebugWindow_Caption = "VU Debug Window";
	static const int DebugWindow_X = 10;
	static const int DebugWindow_Y = 10;
	static const int DebugWindow_Width = 995;
	static const int DebugWindow_Height = 420;
	
	static const int GPRList_X = 0;
	static const int GPRList_Y = 0;
	static const int GPRList_Width = 190;
	static const int GPRList_Height = 380;

	static const int COP1List_X = GPRList_X + GPRList_Width;
	static const int COP1List_Y = 0;
	static const int COP1List_Width = 175;
	static const int COP1List_Height = 300;
	
	static const int COP2_CPCList_X = COP1List_X + COP1List_Width;
	static const int COP2_CPCList_Y = 0;
	static const int COP2_CPCList_Width = 175;
	static const int COP2_CPCList_Height = 300;
	
	static const int COP2_CPRList_X = COP2_CPCList_X + COP2_CPCList_Width;
	static const int COP2_CPRList_Y = 0;
	static const int COP2_CPRList_Width = 175;
	static const int COP2_CPRList_Height = 300;
	
	static const int DisAsm_X = COP2_CPRList_X + COP2_CPRList_Width;
	static const int DisAsm_Y = 0;
	static const int DisAsm_Width = 270;
	static const int DisAsm_Height = 380;
	
	static const int MemoryViewer_Columns = 8;
	static const int MemoryViewer_X = GPRList_X + GPRList_Width;
	static const int MemoryViewer_Y = 300;
	static const int MemoryViewer_Width = 250;
	static const int MemoryViewer_Height = 80;
	
	static const int BkptViewer_X = MemoryViewer_X + MemoryViewer_Width;
	static const int BkptViewer_Y = 300;
	static const int BkptViewer_Width = 275;
	static const int BkptViewer_Height = 80;
	
	int i;
	stringstream ss;
	
#ifdef INLINE_DEBUG
	debug << "\r\nStarting Cpu::DebugWindow_Enable";
#endif
	
	if ( !DebugWindow_Enabled [ Number ] )
	{
		// create the main debug window
		DebugWindow [ Number ] = new WindowClass::Window ();
		DebugWindow [ Number ]->Create ( DebugWindow_Caption, DebugWindow_X, DebugWindow_Y, DebugWindow_Width, DebugWindow_Height );
		DebugWindow [ Number ]->Set_Font ( DebugWindow [ Number ]->CreateFontObject ( FontSize, FontName ) );
		DebugWindow [ Number ]->DisableCloseButton ();
		
		// create "value lists"
		GPR_ValueList [ Number ] = new DebugValueList<float> ();
		COP0_ValueList [ Number ] = new DebugValueList<u32> ();
		COP2_CPCValueList [ Number ] = new DebugValueList<u32> ();
		COP2_CPRValueList [ Number ] = new DebugValueList<u32> ();
		
		// create the value lists
		GPR_ValueList [ Number ]->Create ( DebugWindow [ Number ], GPRList_X, GPRList_Y, GPRList_Width, GPRList_Height );
		COP0_ValueList [ Number ]->Create ( DebugWindow [ Number ], COP1List_X, COP1List_Y, COP1List_Width, COP1List_Height );
		COP2_CPCValueList [ Number ]->Create ( DebugWindow [ Number ], COP2_CPCList_X, COP2_CPCList_Y, COP2_CPCList_Width, COP2_CPCList_Height );
		COP2_CPRValueList [ Number ]->Create ( DebugWindow [ Number ], COP2_CPRList_X, COP2_CPRList_Y, COP2_CPRList_Width, COP2_CPRList_Height );
		
		GPR_ValueList [ Number ]->EnableVariableEdits ();
		COP0_ValueList [ Number ]->EnableVariableEdits ();
		COP2_CPCValueList [ Number ]->EnableVariableEdits ();
		COP2_CPRValueList [ Number ]->EnableVariableEdits ();
	
		// add variables into lists
		for ( i = 0; i < 32; i++ )
		{
			ss.str ("");
			ss << "vf" << dec << i << "_x";
			//GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].uw0) );
			GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].fx) );
			ss.str ("");
			ss << "vf" << dec << i << "_y";
			//GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].uw1) );
			GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].fy) );
			ss.str ("");
			ss << "vf" << dec << i << "_z";
			//GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].uw2) );
			GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].fz) );
			ss.str ("");
			ss << "vf" << dec << i << "_w";
			//GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].uw3) );
			GPR_ValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vf [ i ].fw) );
			
			//COP0_ValueList->AddVariable ( COP0_Names [ i ], &(_CPU->CPR0.Regs [ i ]) );

			if ( i < 16 )
			{
				ss.str("");
				ss << "vi" << dec << i;
				COP2_CPCValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vi [ i ].u) );
				
				//ss.str("");
				//ss << "VI_" << dec << ( ( i << 1 ) + 1 );
				//COP2_CPCValueList->AddVariable ( ss.str().c_str(), &(_CPU->COP2.CPC2.Regs [ ( i << 1 ) + 1 ]) );
			}
			else
			{
				ss.str("");
				ss << "CPC2_" << dec << i;
				COP2_CPRValueList [ Number ]->AddVariable ( ss.str().c_str(), &(_VU [ Number ]->vi [ i ].u) );
				
				//ss.str("");
				//ss << "CPR2_" << dec << ( ( ( i - 16 ) << 1 ) + 1 );
				//COP2_CPRValueList->AddVariable ( ss.str().c_str(), &(_CPU->COP2.CPR2.Regs [ ( ( i - 16 ) << 1 ) + 1 ]) );
			}
		}
		
		
		// also add PC and CycleCount
		COP0_ValueList [ Number ]->AddVariable ( "PC", &(_VU [ Number ]->PC) );
		COP0_ValueList [ Number ]->AddVariable ( "NextPC", &(_VU [ Number ]->NextPC) );
		COP0_ValueList [ Number ]->AddVariable ( "LastPC", &(_VU [ Number ]->LastPC) );
		COP0_ValueList [ Number ]->AddVariable ( "CycleLO", (u32*) &(_VU [ Number ]->CycleCount) );
		
		COP0_ValueList [ Number ]->AddVariable ( "LastReadAddress", &(_VU [ Number ]->Last_ReadAddress) );
		COP0_ValueList [ Number ]->AddVariable ( "LastWriteAddress", &(_VU [ Number ]->Last_WriteAddress) );
		COP0_ValueList [ Number ]->AddVariable ( "LastReadWriteAddress", &(_VU [ Number ]->Last_ReadWriteAddress) );
		
		// need to add in load delay slot values
		//GPR_ValueList->AddVariable ( "D0_INST", &(_VU [ Number ]->DelaySlot0.Instruction.Value) );
		//GPR_ValueList->AddVariable ( "D0_VAL", &(_VU [ Number ]->DelaySlot0.Data) );
		//GPR_ValueList->AddVariable ( "D1_INST", &(_VU [ Number ]->DelaySlot1.Instruction.Value) );
		//GPR_ValueList->AddVariable ( "D1_VAL", &(_VU [ Number ]->DelaySlot1.Data) );
		
		//GPR_ValueList->AddVariable ( "SPUCC", (u32*) _SpuCycleCount );
		//GPR_ValueList->AddVariable ( "Trace", &TraceValue );

		// add some things to the cop0 value list
		COP0_ValueList [ Number ]->AddVariable ( "RUN", &(_VU [ Number ]->Running) );
		COP0_ValueList [ Number ]->AddVariable ( "VifStop", &(_VU [ Number ]->VifStopped) );

		// create the disassembly window
		DisAsm_Window [ Number ] = new Debug_DisassemblyViewer ( Breakpoints [ Number ] );
		DisAsm_Window [ Number ]->Create ( DebugWindow [ Number ], DisAsm_X, DisAsm_Y, DisAsm_Width, DisAsm_Height, Vu::Instruction::Print::PrintInstructionLO, Vu::Instruction::Print::PrintInstructionHI );
		
		DisAsm_Window [ Number ]->Add_MemoryDevice ( "MicroMem", _VU [ Number ]->ulMicroMem_Start, _VU [ Number ]->ulMicroMem_Size, (u8*) _VU [ Number ]->MicroMem8 );
		
		DisAsm_Window [ Number ]->SetProgramCounter ( &_VU [ Number ]->PC );
		
		
		// create window area for breakpoints
		Breakpoint_Window [ Number ] = new Debug_BreakpointWindow ( Breakpoints [ Number ] );
		Breakpoint_Window [ Number ]->Create ( DebugWindow [ Number ], BkptViewer_X, BkptViewer_Y, BkptViewer_Width, BkptViewer_Height );
		
		// create the viewer for D-Cache scratch pad
		ScratchPad_Viewer [ Number ] = new Debug_MemoryViewer ();
		
		ScratchPad_Viewer [ Number ]->Create ( DebugWindow [ Number ], MemoryViewer_X, MemoryViewer_Y, MemoryViewer_Width, MemoryViewer_Height, MemoryViewer_Columns );
		ScratchPad_Viewer [ Number ]->Add_MemoryDevice ( "VuMem", _VU [ Number ]->ulVuMem_Start, _VU [ Number ]->ulVuMem_Size, (u8*) _VU [ Number ]->VuMem8 );
		
		cout << "\nDebug Enable";
		
		// mark debug as enabled now
		DebugWindow_Enabled [ Number ] = true;
		
		cout << "\nUpdate Start";
		
		// update the value lists
		DebugWindow_Update ( Number );
	}
	
#endif

}

static void VU::DebugWindow_Disable ( int Number )
{

#ifndef _CONSOLE_DEBUG_ONLY_

	if ( DebugWindow_Enabled [ Number ] )
	{
		delete DebugWindow [ Number ];
		delete GPR_ValueList [ Number ];
		delete COP0_ValueList [ Number ];
		delete COP2_CPCValueList [ Number ];
		delete COP2_CPRValueList [ Number ];

		delete DisAsm_Window [ Number ];
		
		delete Breakpoint_Window [ Number ];
		delete ScratchPad_Viewer [ Number ];
		
	
		// disable debug window
		DebugWindow_Enabled [ Number ] = false;
	}
	
#endif

}

static void VU::DebugWindow_Update ( int Number )
{

#ifndef _CONSOLE_DEBUG_ONLY_

	if ( DebugWindow_Enabled [ Number ] )
	{
		GPR_ValueList [ Number ]->Update();
		COP0_ValueList [ Number ]->Update();
		COP2_CPCValueList [ Number ]->Update();
		COP2_CPRValueList [ Number ]->Update();
		DisAsm_Window [ Number ]->GoTo_Address ( _VU [ Number ]->PC );
		DisAsm_Window [ Number ]->Update ();
		Breakpoint_Window [ Number ]->Update ();
		ScratchPad_Viewer [ Number ]->Update ();
	}
	
#endif

}


