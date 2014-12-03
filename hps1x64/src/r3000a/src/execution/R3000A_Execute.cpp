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


///////////////////////////////////////////////////////////////////////////////
// *** TODO *** Fix LWL/LWR/SWL/SWR to use masked loading/storing
// *** TODO *** make sure COP2 is ready before processing SWC2/LWC2


#include "R3000A_Execute.h"
#include "R3000A_Lookup.h"

#include "R3000ADebugPrint.h"

#include <iostream>
#include <iomanip>



#ifdef _DEBUG_VERSION_

// enable debug
#define INLINE_DEBUG_ENABLE

/*
#define INLINE_DEBUG_MULT
#define INLINE_DEBUG_MULTU
#define INLINE_DEBUG_DIV
#define INLINE_DEBUG_DIVU
#define INLINE_DEBUG_MTHI
#define INLINE_DEBUG_MTLO
#define INLINE_DEBUG_MFHI
#define INLINE_DEBUG_MFLO
*/


//#define INLINE_DEBUG_SWL
//#define INLINE_DEBUG_LWL
//#define INLINE_DEBUG_SWR
//#define INLINE_DEBUG_LWR
//#define INLINE_DEBUG_SYSCALL
//#define INLINE_DEBUG_RFE
//#define INLINE_DEBUG_R3000A

/*
#define INLINE_DEBUG_LWC2
#define INLINE_DEBUG_SWC2
#define INLINE_DEBUG_MTC2
#define INLINE_DEBUG_MFC2
#define INLINE_DEBUG_CTC2
#define INLINE_DEBUG_CFC2
#define INLINE_DEBUG_COP2
*/


//#define COUT_USERMODE_LOAD
//#define COUT_USERMODE_STORE
//#define COUT_FC
//#define COUT_SWC

#define INLINE_DEBUG_TRACE

#endif


using namespace std;

// this area deals with the execution of instructions on the R3000A
using namespace R3000A;
using namespace R3000A::Instruction;

// static vars //
Cpu *Execute::r;


// if a register is loading after load-delay slot, then we need to stall the pipeline until it finishes loading from memory
// if register is loading but is stored to, then we should clear loading flag for register and cancel load
//#define CHECK_LOADING(arg)	/*if ( r->GPRLoading_Bitmap & ( arg ) ) { r->BusyUntil_Cycle = r->Bus->BusyUntil_Cycle; return false; }*/

//#define CHECK_LOADING_COP2(arg)	/*if ( r->COP2.CPRLoading_Bitmap & ( arg ) ) { r->BusyUntil_Cycle = r->Bus->BusyUntil_Cycle; return false; }*/

// we can also cancel the loading of a register if it gets written to before load is complete
//#define CANCEL_LOADING(arg) ( r->GPRLoading_Bitmap &= ~( arg ) )

// if modified register is in delay slot, then kill the load from delay slot
#define CHECK_DELAYSLOT(ModifiedRegister) r->LastModifiedRegister = ModifiedRegister;

#define TRACE_VALUE(ValTr) r->TraceValue = ValTr;

//#define PROCESS_LOADDELAY_BEFORELOAD
#define PROCESS_LOADDELAY_BEFORESTORE

////////////////////////////////////////////////
// R-Type Instructions (non-interrupt)


// regular arithemetic
void Execute::ADDU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDU || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// add without overflow exception: rd = rs + rt
	r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u + r->GPR [ i.Rt ].u;
	
	// if the register that gets modified is in load delay slot, then cancel load
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_ADDU || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SUBU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBU || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// subtract without overflow exception: rd = rs - rt
	r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u - r->GPR [ i.Rt ].u;
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_SUBU || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::AND ( Instruction::Format i )
{
#if defined INLINE_DEBUG_AND || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// logical AND: rd = rs & rt
	r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u & r->GPR [ i.Rt ].u;
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_AND || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::OR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_OR || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// logical OR: rd = rs | rt
	r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u | r->GPR [ i.Rt ].u;
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_OR || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::XOR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_XOR || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// logical XOR: rd = rs ^ rt
	r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u ^ r->GPR [ i.Rt ].u;
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_XOR || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::NOR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_NOR || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// logical NOR: rd = ~(rs | rt)
	r->GPR [ i.Rd ].u = ~( r->GPR [ i.Rs ].u | r->GPR [ i.Rt ].u );
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_NOR || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SLT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SLT || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// set less than signed: rd = rs < rt ? 1 : 0
	r->GPR [ i.Rd ].s = r->GPR [ i.Rs ].s < r->GPR [ i.Rt ].s ? 1 : 0;
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_SLT || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SLTU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SLTU || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// set less than signed: rd = rs < rt ? 1 : 0
	r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u < r->GPR [ i.Rt ].u ? 1 : 0;
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_SLTU || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}


////////////////////////////////////////////
// I-Type Instructions (non-interrupt)



void Execute::ADDIU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDIU || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif

	// *note* immediate value is sign-extended before the addition is performed

	// add immedeate without overflow exception: rt = rs + immediate
	r->GPR [ i.Rt ].s = r->GPR [ i.Rs ].s + i.sImmediate;
	
	CHECK_DELAYSLOT ( i.Rt );
	
#if defined INLINE_DEBUG_ADDIU || defined INLINE_DEBUG_R3000A
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::ANDI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ANDI || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// Logical AND zero-extended immedeate: rt = rs & immediate
	r->GPR [ i.Rt ].u = r->GPR [ i.Rs ].u & i.uImmediate;
	
	CHECK_DELAYSLOT ( i.Rt );
	
#if defined INLINE_DEBUG_ANDI || defined INLINE_DEBUG_R3000A
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::ORI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ORI || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// Logical OR zero-extended immedeate: rt = rs | immediate
	r->GPR [ i.Rt ].u = r->GPR [ i.Rs ].u | i.uImmediate;
	
	CHECK_DELAYSLOT ( i.Rt );
	
#if defined INLINE_DEBUG_ORI || defined INLINE_DEBUG_R3000A
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::XORI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_XORI || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// Logical XOR zero-extended immedeate: rt = rs & immediate
	r->GPR [ i.Rt ].u = r->GPR [ i.Rs ].u ^ i.uImmediate;
	
	CHECK_DELAYSLOT ( i.Rt );
	
#if defined INLINE_DEBUG_XORI || defined INLINE_DEBUG_R3000A
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::SLTI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SLTI || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// Set less than sign-extended immedeate: rt = rs < immediate ? 1 : 0
	r->GPR [ i.Rt ].s = r->GPR [ i.Rs ].s < i.sImmediate ? 1 : 0;
	
	CHECK_DELAYSLOT ( i.Rt );
	
#if defined INLINE_DEBUG_SLTI || defined INLINE_DEBUG_R3000A
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::SLTIU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SLTIU || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// *note* Rs is still sign-extended here, but the comparison is an unsigned one

	// Set if unsigned less than sign-extended immedeate: rt = rs < immediate ? 1 : 0
	r->GPR [ i.Rt ].u = r->GPR [ i.Rs ].u < ((u32) ((s32) i.sImmediate)) ? 1 : 0;
	
	CHECK_DELAYSLOT ( i.Rt );
	
#if defined INLINE_DEBUG_SLTIU || defined INLINE_DEBUG_R3000A
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::LUI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LUI || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif
	
	// Load upper immediate
	r->GPR [ i.Rt ].u = ( i.uImmediate << 16 );
	
	CHECK_DELAYSLOT ( i.Rt );
	
#if defined INLINE_DEBUG_LUI || defined INLINE_DEBUG_R3000A
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}





void Execute::MFHI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MFHI || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: HI = " << r->HiLo.uHi;
#endif
	
	// this instruction interlocks if multiply/divide unit is busy
	if ( r->MulDiv_BusyUntil_Cycle > r->CycleCount )
	{
		// determine when multiply/divide unit is busy until
		r->BusyUntil_Cycle = r->MulDiv_BusyUntil_Cycle;
		
		// wait until the multiply/divide unit is no longer busy
		r->WaitForCpuReady1 ();
	}
	
	// move from Hi register
	r->GPR [ i.Rd ].u = r->HiLo.uHi;
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_MFHI || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::MFLO ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MFLO || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: LO = " << r->HiLo.uLo;
#endif
	
	// this instruction interlocks if multiply/divide unit is busy
	if ( r->MulDiv_BusyUntil_Cycle > r->CycleCount )
	{
		// determine when multiply/divide unit is busy until
		r->BusyUntil_Cycle = r->MulDiv_BusyUntil_Cycle;
		
		// wait until the multiply/divide unit is no longer busy
		r->WaitForCpuReady1 ();
	}
	
	// move from Lo register
	r->GPR [ i.Rd ].u = r->HiLo.uLo;
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_MFLO || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}




void Execute::MTHI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MTHI || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// there is no MTHI/MTLO delay slot
	r->HiLo.uHi = r->GPR [ i.Rs ].u;
}

void Execute::MTLO ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MTLO || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// there is no MTHI/MTLO delay slot
	r->HiLo.uLo = r->GPR [ i.Rs ].u;
}


//////////////////////////////////////////////////////////
// Shift instructions



void Execute::SLL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SLL || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// shift left logical: rd = rt << shift
	r->GPR [ i.Rd ].u = ( r->GPR [ i.Rt ].u << i.Shift );
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_SLL || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SRL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SRL || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// shift right logical: rd = rt >> shift
	r->GPR [ i.Rd ].u = ( r->GPR [ i.Rt ].u >> i.Shift );
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_SRL || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SRA ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SRA || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// shift right arithmetic: rd = rt >> shift
	r->GPR [ i.Rd ].s = ( r->GPR [ i.Rt ].s >> i.Shift );
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_SRA || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SLLV ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SLLV || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// shift left logical variable: rd = rt << rs
	r->GPR [ i.Rd ].u = ( r->GPR [ i.Rt ].u << ( r->GPR [ i.Rs ].u & 0x1f ) );
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_SLLV || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SRLV ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SRLV || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// shift right logical variable: rd = rt >> rs
	r->GPR [ i.Rd ].u = ( r->GPR [ i.Rt ].u >> ( r->GPR [ i.Rs ].u & 0x1f ) );
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_SRLV || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SRAV ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SRAV || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// shift right arithmetic variable: rd = rt >> rs
	r->GPR [ i.Rd ].s = ( r->GPR [ i.Rt ].s >> ( r->GPR [ i.Rs ].u & 0x1f ) );
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_SRAV || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}


/////////////////////////////////////////////////////////////
// Multiply/Divide Instructions


void Execute::MULT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MULT || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// if rs is between -0x800 and 0x7ff, then multiply takes 6 cycles
	static const int c_iMultiplyCycles_Fast = 6;
	
	// if rs is between 0x800 and 0xfffff or between -0x7ff and -0x100000, then multiply takes 9 cycles
	static const int c_iMultiplyCycles_Med = 9;
	
	// otherwise, multiply takes 13 cycles
	static const int c_iMultiplyCycles_Slow = 13;
	
	// check if mul/div unit is in use
	if ( r->MulDiv_BusyUntil_Cycle > r->CycleCount )
	{
		// determine when multiply/divide unit is busy until
		r->BusyUntil_Cycle = r->MulDiv_BusyUntil_Cycle;
		
		// wait until the multiply/divide unit is no longer busy
		r->WaitForCpuReady1 ();
	}

	// calculate cycles mul/div unit will be busy for
	r->MulDiv_BusyUntil_Cycle = r->CycleCount + c_iMultiplyCycles_Slow;
	if ( r->GPR [ i.Rs ].s < 0x800 && r->GPR [ i.Rs ].s >= -0x800 )
	{
		r->MulDiv_BusyUntil_Cycle = r->CycleCount + c_iMultiplyCycles_Fast;
	}
	else if ( r->GPR [ i.Rs ].s < 0x100000 && r->GPR [ i.Rs ].s >= -0x100000 )
	{
		r->MulDiv_BusyUntil_Cycle = r->CycleCount + c_iMultiplyCycles_Med;
	}
	
	// multiply signed Lo,Hi = rs * rt
	r->HiLo.sValue = ((s64) (r->GPR [ i.Rs ].s)) * ((s64) (r->GPR [ i.Rt ].s));

#if defined INLINE_DEBUG_MULT || defined INLINE_DEBUG_R3000A
	debug << "; Output: LO = " << r->HiLo.uLo << "; HI = " << r->HiLo.uHi;
#endif
}

void Execute::MULTU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MULTU || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// if rs is between 0 and 0x7ff, then multiply takes 6 cycles
	static const int c_iMultiplyCycles_Fast = 6;
	
	// if rs is between 0x800 and 0xfffff, then multiply takes 9 cycles
	static const int c_iMultiplyCycles_Med = 9;
	
	// otherwise, multiply takes 13 cycles
	static const int c_iMultiplyCycles_Slow = 13;
	
	// check if mul/div unit is in use
	if ( r->MulDiv_BusyUntil_Cycle > r->CycleCount )
	{
		// determine when multiply/divide unit is busy until
		r->BusyUntil_Cycle = r->MulDiv_BusyUntil_Cycle;
		
		// wait until the multiply/divide unit is no longer busy
		r->WaitForCpuReady1 ();
	}
	
	// calculate cycles mul/div unit will be busy for
	r->MulDiv_BusyUntil_Cycle = r->CycleCount + c_iMultiplyCycles_Slow;
	if ( r->GPR [ i.Rs ].u < 0x800 )
	{
		r->MulDiv_BusyUntil_Cycle = r->CycleCount + c_iMultiplyCycles_Fast;
	}
	else if ( r->GPR [ i.Rs ].u < 0x100000 )
	{
		r->MulDiv_BusyUntil_Cycle = r->CycleCount + c_iMultiplyCycles_Med;
	}

	// multiply unsigned Lo,Hi = rs * rt
	r->HiLo.uValue = ((u64) (r->GPR [ i.Rs ].u)) * ((u64) (r->GPR [ i.Rt ].u));
	
#if defined INLINE_DEBUG_MULTU || defined INLINE_DEBUG_R3000A
	debug << "; Output: LO = " << r->HiLo.uLo << "; HI = " << r->HiLo.uHi;
#endif
}

void Execute::DIV ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DIV || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// 36 cycles
	static const int c_iDivideCycles = 36;

	// check if mul/div unit is in use
	if ( r->MulDiv_BusyUntil_Cycle > r->CycleCount )
	{
		// determine when multiply/divide unit is busy until
		r->BusyUntil_Cycle = r->MulDiv_BusyUntil_Cycle;
		
		// wait until the multiply/divide unit is no longer busy
		r->WaitForCpuReady1 ();
	}
	
	// mult/div unit is busy now
	r->MulDiv_BusyUntil_Cycle = r->CycleCount + c_iDivideCycles;

	// divide signed: Lo = rs / rt; Hi = rs % rt
	if ( r->GPR [ i.Rt ].u != 0 )
	{
		// if rs = 0x80000000 and rt = -1 then hi = 0 and lo = 0x80000000
		if ( r->GPR [ i.Rs ].u == 0x80000000 && r->GPR [ i.Rt ].s == -1 )
		{
			r->HiLo.uHi = 0;
			r->HiLo.uLo = 0x80000000;
		}
		else
		{
			r->HiLo.sLo = r->GPR [ i.Rs ].s / r->GPR [ i.Rt ].s;
			r->HiLo.sHi = r->GPR [ i.Rs ].s % r->GPR [ i.Rt ].s;
		}
	}
	else
	{
		if ( r->GPR [ i.Rs ].s < 0 )
		{
			r->HiLo.sLo = 1;
		}
		else
		{
			r->HiLo.sLo = -1;
		}
		
		r->HiLo.uHi = r->GPR [ i.Rs ].u;
	}
	
#if defined INLINE_DEBUG_DIV || defined INLINE_DEBUG_R3000A
	debug << "; Output: LO = " << r->HiLo.uLo << "; HI = " << r->HiLo.uHi;
#endif
}

void Execute::DIVU ( Instruction::Format i )
{
	// 36 cycles
	static const int c_iDivideCycles = 36;
	
#if defined INLINE_DEBUG_DIVU || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// check if mul/div unit is in use
	if ( r->MulDiv_BusyUntil_Cycle > r->CycleCount )
	{
		// determine when multiply/divide unit is busy until
		r->BusyUntil_Cycle = r->MulDiv_BusyUntil_Cycle;
		
		// wait until the multiply/divide unit is no longer busy
		r->WaitForCpuReady1 ();
	}
	
	// mult/div unit is busy now
	r->MulDiv_BusyUntil_Cycle = r->CycleCount + c_iDivideCycles;

	// divide unsigned: Lo = rs / rt; Hi = rs % rt
	if ( r->GPR [ i.Rt ].u != 0 )
	{
		r->HiLo.uLo = r->GPR [ i.Rs ].u / r->GPR [ i.Rt ].u;
		r->HiLo.uHi = r->GPR [ i.Rs ].u % r->GPR [ i.Rt ].u;
	}
	else
	{
		r->HiLo.sLo = -1;
		r->HiLo.uHi = r->GPR [ i.Rs ].u;
	}
	
#if defined INLINE_DEBUG_DIVU || defined INLINE_DEBUG_R3000A
	debug << "; Output: LO = " << r->HiLo.uLo << "; HI = " << r->HiLo.uHi;
#endif
}



////////////////////////////////////////////
// Jump/Branch Instructions



void Execute::J ( Instruction::Format i )
{
#if defined INLINE_DEBUG_J || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif
	
	// next instruction is in the branch delay slot
	r->DelaySlot0.Instruction = i;
	r->DelaySlot0.cb = r->_cb_Jump;
	r->Status.DelaySlot_Valid |= 0x1;
}

void Execute::JR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_JR || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// generates an address exception if two low order bits of Rs are not zero


	// check for address exception??
	if ( r->GPR [ i.Rs ].u & 3 )
	{
		// if lower 2-bits of register are not zero, fire address exception
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
		return;
	}
	
	// next instruction is in the branch delay slot
	r->DelaySlot0.Instruction = i;
	r->DelaySlot0.cb = r->_cb_JumpRegister;

	// *** todo *** check if address exception should be generated if lower 2-bits of jump address are not zero
	// will clear out lower two bits of address for now
	r->DelaySlot0.Data = r->GPR [ i.Rs ].u & ~3;
	
	r->Status.DelaySlot_Valid |= 0x1;
}

void Execute::JAL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_JAL || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	
	// next instruction is in the branch delay slot
	r->DelaySlot0.Instruction = i;
	r->DelaySlot0.cb = r->_cb_Jump;
	r->Status.DelaySlot_Valid |= 0x1;

	// *** note *** this is tricky because return address gets stored to r31 after execution of load delay slot but before next instruction
	///////////////////////////////////////////////////////////////
	// Store return address when instruction is executed in r31
	r->GPR [ 31 ].u = r->PC + 8;
	
	CHECK_DELAYSLOT ( 31 );
	
#if defined INLINE_DEBUG_JAL || defined INLINE_DEBUG_R3000A
	debug << "; Output: r31 = " << r->GPR [ 31 ].u;
#endif
}

void Execute::JALR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_JALR || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// JALR rd, rs


	// check for address exception??
	if ( r->GPR [ i.Rs ].u & 3 )
	{
		// if lower 2-bits of register are not zero, fire address exception
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
		
		return;
	}
	
	// next instruction is in the branch delay slot
	r->DelaySlot0.Instruction = i;
	
	// *** todo *** check if address exception should be generated if lower 2-bits of jump address are not zero
	// will clear out lower two bits of address for now
	r->DelaySlot0.Data = r->GPR [ i.Rs ].u & ~3;
	r->DelaySlot0.cb = r->_cb_JumpRegister;
	
	r->Status.DelaySlot_Valid |= 0x1;

	///////////////////////////////////////////////////////////////
	// Store return address when instruction is executed in Rd
	// *note* this must happen AFTER the stuff above
	r->GPR [ i.Rd ].u = r->PC + 8;
	
	CHECK_DELAYSLOT ( i.Rd );
	
#if defined INLINE_DEBUG_JALR || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::BEQ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BEQ || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif
	
	if ( r->GPR [ i.Rs ].u == r->GPR [ i.Rt ].u )
	{
		// next instruction is in the branch delay slot
		r->DelaySlot0.Instruction = i;
		r->DelaySlot0.cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x1;
		
#if defined INLINE_DEBUG_BEQ || defined INLINE_DEBUG_R3000A
		debug << ";  WILL TAKE";
#endif
	}
}

void Execute::BNE ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BNE || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif
	
	if ( r->GPR [ i.Rs ].u != r->GPR [ i.Rt ].u )
	{
		// next instruction is in the branch delay slot
		r->DelaySlot0.Instruction = i;
		r->DelaySlot0.cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x1;
		
#if defined INLINE_DEBUG_BNE || defined INLINE_DEBUG_R3000A
		debug << ";  WILL TAKE";
#endif
	}
}

void Execute::BLEZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BLEZ || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	if ( r->GPR [ i.Rs ].s <= 0 )
	{
		// next instruction is in the branch delay slot
		r->DelaySlot0.Instruction = i;
		r->DelaySlot0.cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x1;
		
#if defined INLINE_DEBUG_BLEZ || defined INLINE_DEBUG_R3000A
		debug << ";  WILL TAKE";
#endif
	}
}

void Execute::BGTZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BGTZ || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	if ( r->GPR [ i.Rs ].s > 0 )
	{
		// next instruction is in the branch delay slot
		r->DelaySlot0.Instruction = i;
		r->DelaySlot0.cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x1;
		
#if defined INLINE_DEBUG_BGTZ || defined INLINE_DEBUG_R3000A
		debug << ";  WILL TAKE";
#endif
	}
}

void Execute::BLTZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BLTZ || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	if ( r->GPR [ i.Rs ].s < 0 )
	{
		// next instruction is in the branch delay slot
		r->DelaySlot0.Instruction = i;
		r->DelaySlot0.cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x1;
		
#if defined INLINE_DEBUG_BLTZ || defined INLINE_DEBUG_R3000A
		debug << ";  WILL TAKE";
#endif
	}
}

void Execute::BGEZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BGEZ || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	if ( r->GPR [ i.Rs ].s >= 0 )
	{
		// next instruction is in the branch delay slot
		r->DelaySlot0.Instruction = i;
		r->DelaySlot0.cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x1;
		
#if defined INLINE_DEBUG_BGEZ || defined INLINE_DEBUG_R3000A
		debug << ";  WILL TAKE";
#endif
	}
}

void Execute::BLTZAL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BLTZAL || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	if ( r->GPR [ i.Rs ].s < 0 )
	{
		// next instruction is in the branch delay slot
		r->DelaySlot0.Instruction = i;
		r->DelaySlot0.cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x1;
		
#if defined INLINE_DEBUG_BLTZAL || defined INLINE_DEBUG_R3000A
		debug << ";  WILL TAKE";
#endif
	}
	
	////////////////////////////////////////////////////////////////////////
	// Store return address when instruction is executed in r31
	// for this instruction this happens whether branch is taken or not
	// *note* this must happen AFTER comparison check
	r->GPR [ 31 ].u = r->PC + 8;
	
	CHECK_DELAYSLOT ( 31 );
}

void Execute::BGEZAL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BGEZAL || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	if ( r->GPR [ i.Rs ].s >= 0 )
	{
		// next instruction is in the branch delay slot
		r->DelaySlot0.Instruction = i;
		r->DelaySlot0.cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x1;
		
#if defined INLINE_DEBUG_BGEZAL || defined INLINE_DEBUG_R3000A
		debug << ";  WILL TAKE";
#endif
	}

	////////////////////////////////////////////////////////////////////////
	// Store return address when instruction is executed in r31
	// for this instruction this happens whether branch is taken or not
	// *note* this must happen AFTER comparison check
	r->GPR [ 31 ].u = r->PC + 8;
	
	CHECK_DELAYSLOT ( 31 );
}




void Execute::RFE ( Instruction::Format i )
{
#if defined INLINE_DEBUG_RFE || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "\r\nReturning From: ";
	if ( r->CPR0.Cause.ExcCode == 0 ) debug << "ASYNC Interrupt"; else if ( r->CPR0.Cause.ExcCode == 8 ) debug << "Syscall"; else debug << "Other";
	debug << "\r\nReturning To: " << hex << setw ( 8 ) << r->GPR [ 26 ].u << "; EPC=" << r->CPR0.EPC;
	debug << "\r\nUpdateInterrupt;(before) _Intc_Stat=" << hex << *r->_Intc_Stat << " _Intc_Mask=" << *r->_Intc_Mask << " _R3000A_Status=" << r->CPR0.Regs [ 12 ] << " _R3000A_Cause=" << r->CPR0.Regs [ 13 ] << " _ProcStatus=" << r->Status.Value;
#endif
	
	// restore user/kernel status register bits
	// bits 7-8 should stay zero, and bits 5-6 should stay the same
	r->CPR0.Status.b0 = ( r->CPR0.Status.b0 & 0x30 ) | ( ( r->CPR0.Status.b0 >> 2 ) & 0xf );
	
	// check if interrupts should be re-enabled or cleared ?
	
	// whenever interrupt related stuff is messed with, must update the other interrupt stuff
	r->UpdateInterrupt ();

#if defined INLINE_DEBUG_RFE || defined INLINE_DEBUG_R3000A
	debug << "\r\n(after) _Intc_Stat=" << hex << *r->_Intc_Stat << " _Intc_Mask=" << *r->_Intc_Mask << " _R3000A_Status=" << r->CPR0.Regs [ 12 ] << " _R3000A_Cause=" << r->CPR0.Regs [ 13 ] << " _ProcStatus=" << r->Status.Value << " CycleCount=" << dec << r->CycleCount;
#endif
}




////////////////////////////////////////////////////////
// Instructions that can cause Synchronous Interrupts //
////////////////////////////////////////////////////////


void Execute::ADD ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ADD || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif
	
	s32 temp;
	
	//temp = ( (s64) r->GPR [ i.Rs ].s ) + ( (s64) r->GPR [ i.Rt ].s );
	temp = r->GPR [ i.Rs ].s + r->GPR [ i.Rt ].s;
	
	// if the carry outs of bits 30 and 31 differ, then it's signed overflow
	//if ( ( temp < -2147483648LL ) || ( temp > 2147483647LL ) )
	//if( (INT32)( ~( m_r[ INS_RS( m_op ) ] ^ m_r[ INS_RT( m_op ) ] ) & ( m_r[ INS_RS( m_op ) ] ^ result ) ) < 0 )
	//if ( (s32)( ~( r->GPR [ i.Rs ].s ^ r->GPR [ i.Rt ].s ) & ( r->GPR [ i.Rs ].s ^ temp ) ) < 0 )
	//if ( temp < -0x80000000LL || temp > 0x7fffffffLL )
	if ( ( ( ~( r->GPR [ i.Rs ].s ^ r->GPR [ i.Rt ].s ) ) & ( r->GPR [ i.Rs ].s ^ temp ) ) < 0 )
	{
		// overflow
		cout << "\nhps1x64: Execute::ADD generated an overflow exception @ Cycle#" << dec << r->CycleCount << " PC=" << hex << r->PC << "\n";
		r->ProcessSynchronousInterrupt ( Cpu::EXC_OV );
		
#if defined INLINE_DEBUG_ADD || defined INLINE_DEBUG_R3000A
		debug << ";  INT";
#endif
	}
	else
	{
		// it's cool - we can do the add and store the result to register
		r->GPR [ i.Rd ].s = temp;
		
		CHECK_DELAYSLOT ( i.Rd );
	}
	
#if defined INLINE_DEBUG_ADD || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::ADDI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDI || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif

	s32 temp;
	
	//temp = ( (s64) r->GPR [ i.Rs ].s ) + ( (s64) i.sImmediate );
	temp = r->GPR [ i.Rs ].s + ( (s32) i.sImmediate );
	
	// if the carry outs of bits 30 and 31 differ, then it's signed overflow
	//if ( ( temp < -2147483648LL ) || ( temp > 2147483647LL ) )
	//if( (INT32)( ~( m_r[ INS_RS( m_op ) ] ^ m_r[ INS_RT( m_op ) ] ) & ( m_r[ INS_RS( m_op ) ] ^ result ) ) < 0 )
	//if ( (s32)( ~( r->GPR [ i.Rs ].s ^ ( (s32) i.sImmediate ) ) & ( r->GPR [ i.Rs ].s ^ temp ) ) < 0 )
	//if ( temp < -0x80000000LL || temp > 0x7fffffffLL )
	if ( ( ( ~( r->GPR [ i.Rs ].s ^ ( (s32) i.sImmediate ) ) ) & ( r->GPR [ i.Rs ].s ^ temp ) ) < 0 )
	{
		// overflow
		cout << "\nhps1x64: Execute::ADDI generated an overflow exception @ Cycle#" << dec << r->CycleCount << " PC=" << hex << r->PC << "\n";
		r->ProcessSynchronousInterrupt ( Cpu::EXC_OV );
		
#if defined INLINE_DEBUG_ADDI || defined INLINE_DEBUG_R3000A
		debug << ";  INT";
#endif
	}
	else
	{
		// it's cool - we can do the addi and store the result to register
		r->GPR [ i.Rt ].s = temp;
		
		CHECK_DELAYSLOT ( i.Rt );
	}
	
#if defined INLINE_DEBUG_ADDI || defined INLINE_DEBUG_R3000A
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::SUB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SUB || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif
	
	s64 temp;
	
	temp = ( (s64) r->GPR [ i.Rs ].s ) - ( (s64) r->GPR [ i.Rt ].s );
	
	// if the carry outs of bits 30 and 31 differ, then it's signed overflow
	//if ( ( temp < -2147483648LL ) || ( temp > 2147483647LL ) )
	//if( (INT32)( ( m_r[ INS_RS( m_op ) ] ^ m_r[ INS_RT( m_op ) ] ) & ( m_r[ INS_RS( m_op ) ] ^ result ) ) < 0 )
	//if ( (s32) ( ( r->GPR [ i.Rs ].s ^ r->GPR [ i.Rt ].s ) & ( r->GPR [ i.Rs ].s ^ temp ) ) < 0 )
	if ( temp < -0x80000000LL || temp > 0x7fffffffLL )
	{
		// overflow
		cout << "\nhps1x64: Execute::SUB generated an overflow exception @ Cycle#" << dec << r->CycleCount << " PC=" << hex << r->PC << "\n";
		r->ProcessSynchronousInterrupt ( Cpu::EXC_OV );
		
#if defined INLINE_DEBUG_SUB || defined INLINE_DEBUG_R3000A
		debug << ";  INT";
#endif
	}
	else
	{
		// it's cool - we can do the sub and store the result to register
		r->GPR [ i.Rd ].s = temp;
		
		CHECK_DELAYSLOT ( i.Rd );
	}
	
#if defined INLINE_DEBUG_SUB || defined INLINE_DEBUG_R3000A
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}




void Execute::SYSCALL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SYSCALL || defined INLINE_DEBUG_R3000A
	debug << "\r\nBefore:" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "\r\n" << hex << "Status=" << r->CPR0.Regs [ 12 ] << " Cause=" << r->CPR0.Regs [ 13 ] << " a0=" << r->GPR [ 4 ].u;
#endif
	
	r->ProcessSynchronousInterrupt ( Cpu::EXC_SYSCALL );
	
#if defined INLINE_DEBUG_SYSCALL || defined INLINE_DEBUG_R3000A
	debug << "\r\nAfter:" << hex << setw( 8 ) << r->PC << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "\r\n" << hex << "Status=" << r->CPR0.Regs [ 12 ] << " Cause=" << r->CPR0.Regs [ 13 ] << " a0=" << r->GPR [ 4 ].u;
#endif
}

void Execute::BREAK ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BREAK || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif
	
	cout << "\nhps1x64: Execute::BREAK generated an exception @ Cycle#" << dec << r->CycleCount << " PC=" << hex << r->PC << "\n";
	r->ProcessSynchronousInterrupt ( Cpu::EXC_BP );
	
	// say to stop if we are debugging
	Cpu::DebugStatus.Stop = true;
	Cpu::DebugStatus.Done = true;
}

void Execute::Invalid ( Instruction::Format i )
{
#if defined INLINE_DEBUG_INVALID || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	cout << "\nhps1x64 NOTE: Invalid Instruction @ Cycle#" << dec << r->CycleCount << " PC=" << hex << r->PC << " Instruction=" << i.Value << " LastPC=" << r->LastPC << "\n";
	r->ProcessSynchronousInterrupt ( Cpu::EXC_RI );
}





void Execute::MFC0 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MFC0 || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: CPR[0,rd] = " << r->CPR0.Regs [ i.Rd ];
#endif

	// MFCz rt, rd
	// 1 instruction delay?
	r->DelaySlot0.Instruction = i;
	r->DelaySlot0.Data = r->CPR0.Regs [ i.Rd ];
	r->DelaySlot0.cb = r->_cb_FC;
	r->Status.DelaySlot_Valid |= 0x1;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->DelaySlot0.Data )
#endif
}


void Execute::FC_Callback ( Cpu* r )
{
	if ( r->DelaySlot1.Instruction.Rt == r->LastModifiedRegister )
	{
#ifdef COUT_FC
		cout << "\nhps1x64 ALERT: Reg#" << dec << r->DelaySlot1.Instruction.Rt << " was modified in MFC/CFC delay slot @ Cycle#" << r->CycleCount << hex << " PC=" << r->PC << "\n";
#endif

	}
	
	r->GPR [ r->DelaySlot1.Instruction.Rt ].u = r->DelaySlot1.Data;
	
	// clear delay slot
	r->DelaySlot1.Value = 0;
}


void Execute::MTC0 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MTC0 || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// MTCz rt, rd
	// 1 instruction delay?
	r->DelaySlot0.Instruction = i;
	r->DelaySlot0.Data = r->GPR [ i.Rt ].u;
	r->DelaySlot0.cb = r->_cb_MTC0;
	r->Status.DelaySlot_Valid |= 0x1;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->DelaySlot0.Data )
#endif
}


void Execute::MTC0_Callback ( Cpu* r )
{
	r->Write_MTC0 ( r->DelaySlot1.Instruction.Rd, r->DelaySlot1.Data );
	
	// clear delay slot
	r->DelaySlot1.Value = 0;
}



void Execute::MFC2 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MFC2 || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: CPR[2,rd] = " << r->COP2.CPR2.Regs [ i.Rd ];
#endif

	/////////////////////////////////////////////////////
	// If COP2 is busy then we need to stall pipeline
	if ( r->CycleCount < r->COP2.BusyUntil_Cycle )
	{
		// COP2 is busy //
		
		// determine what cycle CPU has to wait until before it can continue
		r->BusyUntil_Cycle = r->COP2.BusyUntil_Cycle;
		
		// wait until COP2/CPU is ready
		r->WaitForCpuReady1 ();
	}
	
	// MFCz rt, rd
	// 1 instruction delay
	r->DelaySlot0.Instruction = i;
	r->DelaySlot0.Data = r->COP2.Read_MFC ( i.Rd );
	r->DelaySlot0.cb = r->_cb_FC;
	r->Status.DelaySlot_Valid |= 0x1;
	
	// important note: if this regiter is modified in delay slot, then it IS cancelled
	r->LastModifiedRegister = 255;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->DelaySlot0.Data )
#endif
}




void Execute::MTC2 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MTC2 || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	/////////////////////////////////////////////////////
	// If COP2 is busy then we need to stall pipeline
	if ( r->CycleCount < r->COP2.BusyUntil_Cycle )
	{
		// COP2 is busy //
		
		// determine what cycle CPU has to wait until before it can continue
		r->BusyUntil_Cycle = r->COP2.BusyUntil_Cycle;
		
		// wait until COP2/CPU is ready
		r->WaitForCpuReady1 ();
	}
	
	// MTCz rt, rd
	// 1 instruction delay
	r->DelaySlot0.Instruction = i;
	r->DelaySlot0.Data = r->GPR [ i.Rt ].u;
	r->DelaySlot0.cb = r->_cb_MTC2;
	r->Status.DelaySlot_Valid |= 0x1;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->DelaySlot0.Data )
#endif
}


void Execute::MTC2_Callback ( Cpu* r )
{
	r->COP2.Write_MTC ( r->DelaySlot1.Instruction.Rd, r->DelaySlot1.Data );
	
	// clear delay slot
	r->DelaySlot1.Value = 0;
}



void Execute::CFC2 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_CFC2 || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: CPC[2,rd] = " << r->COP2.CPC2.Regs [ i.Rd ];
#endif
	
	/////////////////////////////////////////////////////
	// If COP2 is busy then we need to stall pipeline
	if ( r->CycleCount < r->COP2.BusyUntil_Cycle )
	{
		// COP2 is busy //
		
		// determine what cycle CPU has to wait until before it can continue
		r->BusyUntil_Cycle = r->COP2.BusyUntil_Cycle;
		
		// wait until COP2/CPU is ready
		r->WaitForCpuReady1 ();
	}
	
	// CFCz rt, rd
	// 1 instruction delay
	r->DelaySlot0.Instruction = i;
	r->DelaySlot0.Data = r->COP2.Read_CFC ( i.Rd );
	r->DelaySlot0.cb = r->_cb_FC;
	r->Status.DelaySlot_Valid |= 0x1;
	
	// important note: if this regiter is modified in delay slot, then it IS cancelled
	r->LastModifiedRegister = 255;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].u )
#endif
}



void Execute::CTC2 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_CTC2 || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	/////////////////////////////////////////////////////
	// If COP2 is busy then we need to stall pipeline
	if ( r->CycleCount < r->COP2.BusyUntil_Cycle )
	{
		// COP2 is busy //
		
		// determine what cycle CPU has to wait until before it can continue
		r->BusyUntil_Cycle = r->COP2.BusyUntil_Cycle;
		
		// wait until COP2/CPU is ready
		r->WaitForCpuReady1 ();
	}
	
	// CTCz rt, rd
	// 1 instruction delay
	r->DelaySlot0.Instruction = i;
	r->DelaySlot0.Data = r->GPR [ i.Rt ].u;
	r->DelaySlot0.cb = r->_cb_CTC2;
	r->Status.DelaySlot_Valid |= 0x1;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].u )
#endif
}


void Execute::CTC2_Callback ( Cpu* r )
{
	r->COP2.Write_CTC ( r->DelaySlot1.Instruction.Rd, r->DelaySlot1.Data );
	
	// clear delay slot
	r->DelaySlot1.Value = 0;
}



// Load/Store - will need to use address translation to get physical addresses when needed

//////////////////////////////////////////////////////////////////////////
// store instructions

// store instructions
void Execute::SB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SB || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif
	
	// SB rt, offset(base)

#ifdef PROCESS_LOADDELAY_BEFORESTORE
	// *note* load delay slot runs before actual value is put in line to be stored
	r->ProcessLoadDelaySlot ();
#endif
	
	// step 1: check if storing to data cache
	u32 StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	
	// clear top 3 bits since there is no data cache for caching stores
	StoreAddress &= 0x1fffffff;
	
	if ( r->CPR0.Status.IsC )	// *todo* - I think we need to check if DCache is isolated too - unsure
	{
#if defined INLINE_DEBUG_SB || defined INLINE_DEBUG_R3000A
		debug << "; IsC";
#endif

		//cout << "\nhps1x64 ALERT: IsC -> Cache is isolated.\n";
		// *** todo *** also this only clears cache line if this writes byte or half word - or maybe all stores invalidate??
		// important note: appears that this clears the instruction cache regardless of whether cache is swapped or not
		r->ICache.InvalidateDirect ( StoreAddress );
		
	}
	else
	{
#ifdef COUT_USERMODE_STORE
		// check if in user mode
		if ( !r->CPR0.Status.KUc )
		{
			// make sure that top 3 bits of address are clear
			if ( StoreAddress & 0xe0000000 )
			{
				cout << "\nhps1x64 ALERT: Invalid store for USER mode. SB. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
			}
		}
#endif
	
		if ( r->isDCache ( StoreAddress ) )
		{
#if defined INLINE_DEBUG_SB || defined INLINE_DEBUG_R3000A
			debug << "; D$";
#endif

			// step 3: if storing to data cache and lines are not reversed, then store to data cache
			r->DCache.b8 [ StoreAddress & ( Cpu::c_ScratchPadRam_Size - 1 ) ] = (u8) r->GPR [ i.Rt ].u;
		}
		else
		{
			// step 4: otherwise, add entry into store buffer. if store buffer is full, then stall pipeline
			if ( r->StoreBuffer.isFullStore() )
			{
#if defined INLINE_DEBUG_SB || defined INLINE_DEBUG_R3000A
				debug << "; STALL->SBUF FULL";
#endif

				// store buffer is full //
				
				// flush store buffer
				r->FlushStoreBuffer ();
				
				// store buffer was full, so add another 3 cycles onto that (special case or something)
				r->Bus->ReserveBus ( 3 );
				
				// determine how long before stores finish
				r->BusyUntil_Cycle = r->Bus->BusyUntil_Cycle;
				
				// wait for stores to finish
				r->WaitForCpuReady1 ();
			}
			
#if defined INLINE_DEBUG_SB || defined INLINE_DEBUG_R3000A
			debug << "; TO SBUF";
#endif

			// add entry into store buffer
			//r->StoreBuffer.Add_Store ( i );
			r->StoreBuffer.Add_Store ( i, r->_cb_SB );
		}
	}
	
	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].u )
#endif
}





void Execute::SH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SH || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif
	
	// SH rt, offset(base)
	
#ifdef PROCESS_LOADDELAY_BEFORESTORE
	// * note * load delay slot runs before actual value is put in line to be stored
	r->ProcessLoadDelaySlot ();
#endif
	
	// step 1: check if storing to data cache
	u32 StoreAddress = r->GPR [ i.Base ].s + i.sOffset;

	// *** testing *** alert if load is from unaligned address
	if ( StoreAddress & 0x1 )
	{
		cout << "\nhps1x64 ALERT: StoreAddress is unaligned for SH @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADES );
		return;
	}
	
	// clear top 3 bits since there is no data cache for caching stores
	StoreAddress &= 0x1fffffff;
	
	
	if ( r->CPR0.Status.IsC )	// *todo* - I think we need to check if DCache is isolated too - unsure
	{
#if defined INLINE_DEBUG_SH || defined INLINE_DEBUG_R3000A
		debug << "; IsC";
#endif

		//cout << "\nhps1x64 ALERT: IsC -> Cache is isolated.\n";
		// *** todo *** also this only clears cache line if this writes byte or half word - or maybe all stores invalidate??
		r->ICache.InvalidateDirect ( StoreAddress );
		
	}
	else
	{
#ifdef COUT_USERMODE_STORE
		// check if in user mode
		if ( !r->CPR0.Status.KUc )
		{
			// make sure that top 3 bits of address are clear
			if ( StoreAddress & 0xe0000000 )
			{
				cout << "\nhps1x64 ALERT: Invalid store for USER mode. SH. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
			}
		}
#endif
	
		if ( r->isDCache ( StoreAddress ) )
		{
#if defined INLINE_DEBUG_SH || defined INLINE_DEBUG_R3000A
			debug << "; D$";
#endif

			// step 3: if storing to data cache and lines are not reversed, then store to data cache
			r->DCache.b16 [ ( StoreAddress & ( Cpu::c_ScratchPadRam_Size - 1 ) ) >> 1 ] = (u16) r->GPR [ i.Rt ].u;
		}
		else
		{
			// step 4: otherwise, add entry into store buffer. if store buffer is full, then stall pipeline
			if ( r->StoreBuffer.isFullStore() )
			{
#if defined INLINE_DEBUG_SH || defined INLINE_DEBUG_R3000A
				debug << "; STALL->SBUF FULL";
#endif

				// store buffer is full //
				
				// flush store buffer
				r->FlushStoreBuffer ();
				
				// store buffer was full, so add another 3 cycles onto that (special case or something)
				r->Bus->ReserveBus ( 3 );
				
				// determine how long before stores finish
				r->BusyUntil_Cycle = r->Bus->BusyUntil_Cycle;
				
				// wait for stores to finish
				r->WaitForCpuReady1 ();
			}
			
#if defined INLINE_DEBUG_SH || defined INLINE_DEBUG_R3000A
			debug << "; TO SBUF";
#endif

			// add entry into store buffer
			//r->StoreBuffer.Add_Store ( i );
			r->StoreBuffer.Add_Store ( i, r->_cb_SH );
		}
	}

	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].u )
#endif
}

void Execute::SW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SW || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif
	
	// SW rt, offset(base)
	u32 StoreAddress;
	
#ifdef PROCESS_LOADDELAY_BEFORESTORE
	// * note * load delay slot runs before actual value is put in line to be stored
	r->ProcessLoadDelaySlot ();
#endif
	
	// check if storing to data cache
	StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	
	// *** testing *** alert if load is from unaligned address
	if ( StoreAddress & 0x3 )
	{
		cout << "\nhps1x64 ALERT: StoreAddress is unaligned for SW @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADES );
		return;
	}
	
	// clear top 3 bits since there is no data cache for caching stores
	StoreAddress &= 0x1fffffff;
	
	
	if ( r->CPR0.Status.IsC )	// *todo* - I think we need to check if DCache is isolated too - unsure
	{
#if defined INLINE_DEBUG_SW || defined INLINE_DEBUG_R3000A
		debug << "; IsC";
#endif

		//cout << "\nhps1x64 ALERT: IsC -> Cache is isolated.\n";
		// *** todo *** also this only clears cache line if this writes byte or half word - or maybe all stores invalidate??
		r->ICache.InvalidateDirect ( StoreAddress );
	}
	else
	{
#ifdef COUT_USERMODE_STORE
		// check if in user mode
		if ( !r->CPR0.Status.KUc )
		{
			// make sure that top 3 bits of address are clear
			if ( StoreAddress & 0xe0000000 )
			{
				cout << "\nhps1x64 ALERT: Invalid store for USER mode. SW. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
			}
		}
#endif
	
		if ( r->isDCache ( StoreAddress ) )
		{
#if defined INLINE_DEBUG_SW || defined INLINE_DEBUG_R3000A
			debug << "; D$";
#endif

			// step 3: if storing to data cache and lines are not reversed, then store to data cache
			r->DCache.b32 [ ( StoreAddress & ( Cpu::c_ScratchPadRam_Size - 1 ) ) >> 2 ] = r->GPR [ i.Rt ].u;
		}
		else
		{
			// step 4: otherwise, add entry into store buffer. if store buffer is full, then stall pipeline
			if ( r->StoreBuffer.isFullStore() )
			{
#if defined INLINE_DEBUG_SW || defined INLINE_DEBUG_R3000A
				debug << "; STALL->SBUF FULL";
#endif

				// store buffer is full //
				
				// flush store buffer
				r->FlushStoreBuffer ();
				
				// store buffer was full, so add another 3 cycles onto that (special case or something)
				r->Bus->ReserveBus ( 3 );
				
				// determine how long before stores finish
				r->BusyUntil_Cycle = r->Bus->BusyUntil_Cycle;
				
				// wait for stores to finish
				r->WaitForCpuReady1 ();
			}
			
#if defined INLINE_DEBUG_SW || defined INLINE_DEBUG_R3000A
			debug << "; TO SBUF";
#endif

			// add entry into store buffer
			//r->StoreBuffer.Add_Store ( i );
			r->StoreBuffer.Add_Store ( i, r->_cb_SW );
		}
	}

	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].u )
#endif
}


void Execute::SWC2 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SWC2 || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: CPR[2,rt] = " << r->COP2.CPR2.Regs [ i.Rt ] << "; base = " << r->GPR [ i.Base ].u;
#endif
	
	// SWC2 rt, offset(base)
	
	// *** TODO *** WAIT FOR COP2 TO BE READY AND IDLE
	
#ifdef PROCESS_LOADDELAY_BEFORESTORE
	// * note * load delay slot runs before actual value is put in line to be stored
	r->ProcessLoadDelaySlot ();
#endif
	
	u32 StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	
	// *** testing *** alert if load is from unaligned address
	if ( StoreAddress & 0x3 )
	{
		cout << "\nhps1x64 ALERT: StoreAddress is unaligned for SWC2 @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADES );
		return;
	}
	
	
	// *** TODO *** this is where it should be made sure that COP2 is ready and idle
	
	
	// clear top 3 bits since there is no data cache for caching stores
	StoreAddress &= 0x1fffffff;

	// step 2: if storing to data cache, check if I$ and D$ are reversed. If so, invalidate cache line
	// * note * - this is probably incorrect but will use as a placeholder
	if ( r->CPR0.Status.IsC )	// *todo* - I think we need to check if DCache is isolated too
	{
#if defined INLINE_DEBUG_SWC2 || defined INLINE_DEBUG_R3000A
	debug << " IsC";
#endif

		//cout << "\nhps1x64 ALERT: IsC -> Cache is isolated.\n";
		r->ICache.InvalidateDirect ( StoreAddress );
	}
	else
	{
#ifdef COUT_USERMODE_STORE
		// check if in user mode
		if ( !r->CPR0.Status.KUc )
		{
			// make sure that top 3 bits of address are clear
			if ( StoreAddress & 0xe0000000 )
			{
				cout << "\nhps1x64 ALERT: Invalid store for USER mode. SWC2. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
			}
		}
#endif
		
		// step 1: check if storing to data cache
		if ( r->isDCache ( StoreAddress ) )
		{
#if defined INLINE_DEBUG_SWC2 || defined INLINE_DEBUG_R3000A
	debug << " DCache";
#endif

			// step 3: if storing to data cache and lines are not reversed, then store to data cache
			r->DCache.b32 [ ( StoreAddress & ( Cpu::c_ScratchPadRam_Size - 1 ) ) >> 2 ] = r->COP2.Read_MFC ( i.Rt ) /*r->COP2.CPR2.Regs [ i.Rt ]*/;
		}
		else
		{
			// step 4: otherwise, add entry into store buffer. if store buffer is full, then stall pipeline
			if ( r->StoreBuffer.isFullStore() )
			{
#if defined INLINE_DEBUG_SWC2 || defined INLINE_DEBUG_R3000A
			debug << "; STALL->SBUF FULL";
#endif

				// store buffer is full //
				
				// flush store buffer
				r->FlushStoreBuffer ();
				
				// store buffer was full, so add another 3 cycles onto that (special case or something)
				r->Bus->ReserveBus ( 3 );
				
				// determine how long before stores finish
				r->BusyUntil_Cycle = r->Bus->BusyUntil_Cycle;
				
				// wait for stores to finish
				r->WaitForCpuReady1 ();
			}
			
#if defined INLINE_DEBUG_SWC2 || defined INLINE_DEBUG_R3000A
	debug << " STORE";
#endif


			// add entry into store buffer
			//r->StoreBuffer.Add_StoreFromCOP2 ( i );
			r->StoreBuffer.Add_StoreFromCOP2 ( i, r->_cb_SW );
		}
	}

	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->COP2.Read_MFC ( i.Rt ) )
#endif
}



void Execute::SWL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SWL || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif

	static const u32 c_Mask = 0xffffffff;
	u32 Type, Offset;

	// SWL rt, offset(base)
	
#ifdef PROCESS_LOADDELAY_BEFORESTORE
	// * note * load delay slot runs before actual value is put in line to be stored
	r->ProcessLoadDelaySlot ();
#endif
	
	// step 1: check if storing to data cache
	u32 StoreAddress = r->GPR [ i.Base ].s + i.sOffset;

	// clear top 3 bits since there is no data cache for caching stores
	StoreAddress &= 0x1fffffff;
	
	if ( r->CPR0.Status.IsC )	// *todo* - I think we need to check if DCache is isolated too - unsure
	{
#if defined INLINE_DEBUG_SWL || defined INLINE_DEBUG_R3000A
		debug << "; IsC";
#endif

		//cout << "\nhps1x64 ALERT: IsC -> Cache is isolated.\n";
		// *** todo *** also this only clears cache line if this writes byte or half word - or maybe all stores invalidate??
		r->ICache.InvalidateDirect ( StoreAddress );
	}
	else
	{
#ifdef COUT_USERMODE_STORE
		// check if in user mode
		if ( !r->CPR0.Status.KUc )
		{
			// make sure that top 3 bits of address are clear
			if ( StoreAddress & 0xe0000000 )
			{
				cout << "\nhps1x64 ALERT: Invalid store for USER mode. SWL. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
			}
		}
#endif
	
		if ( r->isDCache ( StoreAddress ) )
		{
#if defined INLINE_DEBUG_SWL || defined INLINE_DEBUG_R3000A
			debug << "; D$";
#endif

			// step 3: if storing to data cache and lines are not reversed, then store to data cache
			// store left stores 4-offset number of bytes
			// shift register right by 3-type bytes, and mask is 0xffffffff shifted right by the same amount
			Type = 3 - ( StoreAddress & 3 );
			Offset = ( StoreAddress & ( Cpu::c_ScratchPadRam_Size - 1 ) ) >> 2;
			r->DCache.b32 [ Offset ] = ( r->GPR [ i.Rt ].u >> ( Type << 3 ) ) | ( r->DCache.b32 [ Offset ] & ~( c_Mask >> ( Type << 3 ) ) );
		}
		else
		{
			// step 4: otherwise, add entry into store buffer. if store buffer is full, then stall pipeline
			if ( r->StoreBuffer.isFullStore() )
			{
#if defined INLINE_DEBUG_SWL || defined INLINE_DEBUG_R3000A
				debug << "; STALL->SBUF FULL";
#endif

				// store buffer is full //
				
				// flush store buffer
				r->FlushStoreBuffer ();
				
				// store buffer was full, so add another 3 cycles onto that (special case or something)
				r->Bus->ReserveBus ( 3 );
				
				// determine how long before stores finish
				r->BusyUntil_Cycle = r->Bus->BusyUntil_Cycle;
				
				// wait for stores to finish
				r->WaitForCpuReady1 ();
			}
			
#if defined INLINE_DEBUG_SWL || defined INLINE_DEBUG_R3000A
			debug << "; TO SBUF";
#endif

			// add entry into store buffer
			//r->StoreBuffer.Add_Store ( i );
			r->StoreBuffer.Add_Store ( i, r->_cb_SWL );
		}
	}

	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].u )
#endif
}

void Execute::SWR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SWR || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif

	static const u32 c_Mask = 0xffffffff;
	u32 Type, Offset;
	
	// SWR rt, offset(base)
	
#ifdef PROCESS_LOADDELAY_BEFORESTORE
	// * note * load delay slot runs before actual value is put in line to be stored
	r->ProcessLoadDelaySlot ();
#endif
	
	// step 1: check if storing to data cache
	u32 StoreAddress = r->GPR [ i.Base ].s + i.sOffset;

	// clear top 3 bits since there is no data cache for caching stores
	StoreAddress &= 0x1fffffff;
	
	if ( r->CPR0.Status.IsC )	// *todo* - I think we need to check if DCache is isolated too - unsure
	{
#if defined INLINE_DEBUG_SWR || defined INLINE_DEBUG_R3000A
		debug << "; IsC";
#endif

		//cout << "\nhps1x64 ALERT: IsC -> Cache is isolated.\n";
		// *** todo *** also this only clears cache line if this writes byte or half word - or maybe all stores invalidate??
		r->ICache.InvalidateDirect ( StoreAddress );
	}
	else
	{
#ifdef COUT_USERMODE_STORE
		// check if in user mode
		if ( !r->CPR0.Status.KUc )
		{
			// make sure that top 3 bits of address are clear
			if ( StoreAddress & 0xe0000000 )
			{
				cout << "\nhps1x64 ALERT: Invalid store for USER mode. SWR. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
			}
		}
#endif
	
		if ( r->isDCache ( StoreAddress ) )
		{
#if defined INLINE_DEBUG_SWR || defined INLINE_DEBUG_R3000A
			debug << "; D$";
#endif

			// step 3: if storing to data cache and lines are not reversed, then store to data cache
			// store right
			// mask and register are shifted left by type bytes
			Type = StoreAddress & 3;
			Offset = ( StoreAddress & ( Cpu::c_ScratchPadRam_Size - 1 ) ) >> 2;
			r->DCache.b32 [ Offset ] = ( r->GPR [ i.Rt ].u << ( Type << 3 ) ) | ( r->DCache.b32 [ Offset ] & ~( c_Mask << ( Type << 3 ) ) );
		}
		else
		{
			// step 4: otherwise, add entry into store buffer. if store buffer is full, then stall pipeline
			if ( r->StoreBuffer.isFullStore() )
			{
#if defined INLINE_DEBUG_SWR || defined INLINE_DEBUG_R3000A
				debug << "; STALL->SBUF FULL";
#endif

				// store buffer is full //
				
				// flush store buffer
				r->FlushStoreBuffer ();
				
				// store buffer was full, so add another 3 cycles onto that (special case or something)
				r->Bus->ReserveBus ( 3 );
				
				// determine how long before stores finish
				r->BusyUntil_Cycle = r->Bus->BusyUntil_Cycle;
				
				// wait for stores to finish
				r->WaitForCpuReady1 ();
			}
			
#if defined INLINE_DEBUG_SWR || defined INLINE_DEBUG_R3000A
			debug << "; TO SBUF";
#endif

			// add entry into store buffer
			//r->StoreBuffer.Add_Store ( i );
			r->StoreBuffer.Add_Store ( i, r->_cb_SWR );
		}
	}
	
	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].u )
#endif
}



/////////////////////////////////////////////////
// load instructions

// load instructions with delay slot
void Execute::LB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LB || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif
	
	// signed load byte from memory
	// LB rt, offset(base)
	
	u32 LoadAddress;
	
#ifdef PROCESS_LOADDELAY_BEFORELOAD
	// ***testing*** load delay slot probably runs before actual value is put in line to be loaded
	r->ProcessLoadDelaySlot ();
#endif
	
	// set load to happen after delay slot
	LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	
#ifdef COUT_USERMODE_LOAD
	// check if in user mode
	if ( !r->CPR0.Status.KUc )
	{
		// make sure that top 3 bits of address are clear
		if ( LoadAddress & 0xe0000000 )
		{
			cout << "\nhps1x64 ALERT: Invalid store for USER mode. LB. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		}
	}
#endif
	
	r->DelaySlot0.Instruction = i;
	r->DelaySlot0.Data = LoadAddress;
	//r->DelaySlot0.cb = LB_DelaySlot_Callback_Bus;
	r->Status.DelaySlot_Valid |= 0x1;
	
	// clear the last modified register so we can see what register was modified in load delay slot
	r->LastModifiedRegister = 255;
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}






void Execute::LH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LH || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif
	
	// LH rt, offset(base)
	
	u32 LoadAddress;
	
#ifdef PROCESS_LOADDELAY_BEFORELOAD
	// ***testing*** load delay slot probably runs before actual value is put in line to be loaded
	r->ProcessLoadDelaySlot ();
#endif
	
	// set load to happen after delay slot
	LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	
#ifdef COUT_USERMODE_LOAD
	// check if in user mode
	if ( !r->CPR0.Status.KUc )
	{
		// make sure that top 3 bits of address are clear
		if ( LoadAddress & 0xe0000000 )
		{
			cout << "\nhps1x64 ALERT: Invalid store for USER mode. LH. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		}
	}
#endif
	
	// *** testing *** alert if load is from unaligned address
	if ( LoadAddress & 0x1 )
	{
		cout << "\nhps1x64 ALERT: LoadAddress is unaligned for LH @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
	}
	else
	{
		r->DelaySlot0.Instruction = i;
		r->DelaySlot0.Data = LoadAddress;
		r->Status.DelaySlot_Valid |= 0x1;
		
		// clear the last modified register so we can see what register was modified in load delay slot
		r->LastModifiedRegister = 255;
		
		// used for debugging
		r->Last_ReadAddress = LoadAddress;
		r->Last_ReadWriteAddress = LoadAddress;
	}
}








void Execute::LW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LW || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif
	
	// LW rt, offset(base)
	
	u32 LoadAddress;
	
#ifdef PROCESS_LOADDELAY_BEFORELOAD
	// ***testing*** load delay slot probably runs before actual value is put in line to be loaded
	r->ProcessLoadDelaySlot ();
#endif
	
	// set load to happen after delay slot
	LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	
#ifdef COUT_USERMODE_LOAD
	// check if in user mode
	if ( !r->CPR0.Status.KUc )
	{
		// make sure that top 3 bits of address are clear
		if ( LoadAddress & 0xe0000000 )
		{
			cout << "\nhps1x64 ALERT: Invalid store for USER mode. LW. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		}
	}
#endif
	
	// *** testing *** alert if load is from unaligned address
	if ( LoadAddress & 0x3 )
	{
		cout << "\nhps1x64 ALERT: LoadAddress is unaligned for LW @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
	}
	else
	{
		r->DelaySlot0.Instruction = i;
		r->DelaySlot0.Data = LoadAddress;
		r->Status.DelaySlot_Valid |= 0x1;
		
		// clear the last modified register so we can see what register was modified in load delay slot
		r->LastModifiedRegister = 255;
		
		// used for debugging
		r->Last_ReadAddress = LoadAddress;
		r->Last_ReadWriteAddress = LoadAddress;
	}
}

void Execute::LBU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LBU || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif
	
	// LBU rt, offset(base)
	
	u32 LoadAddress;
	
#ifdef PROCESS_LOADDELAY_BEFORELOAD
	// ***testing*** load delay slot probably runs before actual value is put in line to be loaded
	r->ProcessLoadDelaySlot ();
#endif
	
	// set load to happen after delay slot
	LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	
#ifdef COUT_USERMODE_LOAD
	// check if in user mode
	if ( !r->CPR0.Status.KUc )
	{
		// make sure that top 3 bits of address are clear
		if ( LoadAddress & 0xe0000000 )
		{
			cout << "\nhps1x64 ALERT: Invalid store for USER mode. LBU. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		}
	}
#endif
	
	r->DelaySlot0.Instruction = i;
	r->DelaySlot0.Data = LoadAddress;
	r->Status.DelaySlot_Valid |= 0x1;
	
	// clear the last modified register so we can see what register was modified in load delay slot
	r->LastModifiedRegister = 255;
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}

void Execute::LHU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LHU || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif
	
	// LHU rt, offset(base)
	
	u32 LoadAddress;
	
#ifdef PROCESS_LOADDELAY_BEFORELOAD
	// ***testing*** load delay slot probably runs before actual value is put in line to be loaded
	r->ProcessLoadDelaySlot ();
#endif
	
	// set load to happen after delay slot
	LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	
#ifdef COUT_USERMODE_LOAD
	// check if in user mode
	if ( !r->CPR0.Status.KUc )
	{
		// make sure that top 3 bits of address are clear
		if ( LoadAddress & 0xe0000000 )
		{
			cout << "\nhps1x64 ALERT: Invalid store for USER mode. LHU. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		}
	}
#endif
	
	// *** testing *** alert if load is from unaligned address
	if ( LoadAddress & 0x1 )
	{
		cout << "\nhps1x64 ALERT: LoadAddress is unaligned for LHU @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
	}
	else
	{
		r->DelaySlot0.Instruction = i;
		r->DelaySlot0.Data = LoadAddress;
		r->Status.DelaySlot_Valid |= 0x1;
		
		// clear the last modified register so we can see what register was modified in load delay slot
		r->LastModifiedRegister = 255;
		
		// used for debugging
		r->Last_ReadAddress = LoadAddress;
		r->Last_ReadWriteAddress = LoadAddress;
	}
}

void Execute::LWC2 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LWC2 || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif
	
	// LWC2 rt, offset(base)
	
	// *** TODO *** WAIT FOR COP2 TO BE READY AND IDLE
	
	u32 LoadAddress;
	
#ifdef PROCESS_LOADDELAY_BEFORELOAD
	// ***testing*** load delay slot probably runs before actual value is put in line to be loaded
	r->ProcessLoadDelaySlot ();
#endif
	
	// add entry into load delay slot
	LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	
#ifdef COUT_USERMODE_LOAD
	// check if in user mode
	if ( !r->CPR0.Status.KUc )
	{
		// make sure that top 3 bits of address are clear
		if ( LoadAddress & 0xe0000000 )
		{
			cout << "\nhps1x64 ALERT: Invalid store for USER mode. LWC2. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		}
	}
#endif
	
	// *** testing *** alert if load is from unaligned address
	if ( LoadAddress & 0x3 )
	{
		cout << "\nhps1x64 ALERT: LoadAddress is unaligned for LWC2 @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
	}
	else
	{
		r->DelaySlot0.Instruction = i;
		r->DelaySlot0.Data = LoadAddress;
		r->Status.DelaySlot_Valid |= 0x1;
		
		// used for debugging
		r->Last_ReadAddress = LoadAddress;
		r->Last_ReadWriteAddress = LoadAddress;
	}
}




// load instructions without load-delay slot
void Execute::LWL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LWL || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif

	// LWL rt, offset(base)
	
	u32 LoadAddress;
	
#if defined INLINE_DEBUG_LWL || defined INLINE_DEBUG_R3000A
			debug << "; CHECK";
#endif

#ifdef PROCESS_LOADDELAY_BEFORELOAD
	// ***testing*** load delay slot probably runs before actual value is put in line to be loaded
	r->ProcessLoadDelaySlot ();
#endif
	
#if defined INLINE_DEBUG_LWL || defined INLINE_DEBUG_R3000A
			debug << "; CHECKOK";
#endif

	// set load to happen after delay slot
	LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	
#ifdef COUT_USERMODE_LOAD
	// check if in user mode
	if ( !r->CPR0.Status.KUc )
	{
		// make sure that top 3 bits of address are clear
		if ( LoadAddress & 0xe0000000 )
		{
			cout << "\nhps1x64 ALERT: Invalid store for USER mode. LWL. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		}
	}
#endif
	
	r->DelaySlot0.Instruction = i;
	r->DelaySlot0.Data = LoadAddress;
	r->Status.DelaySlot_Valid |= 0x1;
	
	// clear the last modified register so we can see what register was modified in load delay slot
	r->LastModifiedRegister = 255;
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}

void Execute::LWR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LWR || defined INLINE_DEBUG_R3000A
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif

	// LWR rt, offset(base)
	
	u32 LoadAddress;
	
#ifdef PROCESS_LOADDELAY_BEFORELOAD
	// ***testing*** load delay slot probably runs before actual value is put in line to be loaded
	r->ProcessLoadDelaySlot ();
#endif
	
	// set load to happen after delay slot
	LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	
#ifdef COUT_USERMODE_LOAD
	// check if in user mode
	if ( !r->CPR0.Status.KUc )
	{
		// make sure that top 3 bits of address are clear
		if ( LoadAddress & 0xe0000000 )
		{
			cout << "\nhps1x64 ALERT: Invalid store for USER mode. LWR. Cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		}
	}
#endif
	
	r->DelaySlot0.Instruction = i;
	r->DelaySlot0.Data = LoadAddress;
	r->Status.DelaySlot_Valid |= 0x1;
	
	// clear the last modified register so we can see what register was modified in load delay slot
	r->LastModifiedRegister = 255;
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}




















///////////////////////////
// GTE instructions

void Execute::COP2 ( Instruction::Format i ) {}

void Execute::RTPS ( Instruction::Format i )
{
#if defined INLINE_DEBUG_RTPS || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.RTPS ( r, i );
}

void Execute::NCLIP ( Instruction::Format i )
{
#if defined INLINE_DEBUG_NCLIP || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.NCLIP ( r, i );
}

void Execute::OP ( Instruction::Format i )
{
#if defined INLINE_DEBUG_OP || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.OP ( r, i );
}

void Execute::DPCS ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DPCS || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.DPCS ( r, i );
}

void Execute::INTPL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_INTPL || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.INTPL ( r, i );
}

void Execute::MVMVA ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MVMVA || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.MVMVA ( r, i );
}

void Execute::NCDS ( Instruction::Format i )
{
#if defined INLINE_DEBUG_NCDS || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.NCDS ( r, i );
}

void Execute::CDP ( Instruction::Format i )
{
#if defined INLINE_DEBUG_CDP || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.CDP ( r, i );
}

void Execute::NCDT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_NCDT || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.NCDT ( r, i );
}

void Execute::NCCS ( Instruction::Format i )
{
#if defined INLINE_DEBUG_NCCS || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.NCCS ( r, i );
}

void Execute::CC ( Instruction::Format i )
{
#if defined INLINE_DEBUG_CC || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.CC ( r, i );
}

void Execute::NCS ( Instruction::Format i )
{
#if defined INLINE_DEBUG_NCS || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.NCS ( r, i );
}

void Execute::NCT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_NCT || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.NCT ( r, i );
}

void Execute::SQR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SQR || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.SQR ( r, i );
}

void Execute::DCPL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DCPL || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.DCPL ( r, i );
}

void Execute::DPCT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DPCT || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.DPCT ( r, i );
}

void Execute::AVSZ3 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_AVSZ3 || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.AVSZ3 ( r, i );
}

void Execute::AVSZ4 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_AVSZ4 || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.AVSZ4 ( r, i );
}

void Execute::RTPT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_RTPT || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.RTPT ( r, i );
}

void Execute::GPF ( Instruction::Format i )
{
#if defined INLINE_DEBUG_GPF || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.GPF ( r, i );
}

void Execute::GPL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_GPL || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.GPL ( r, i );
}

void Execute::NCCT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_NCCT || defined INLINE_DEBUG_COP2
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << hex << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
#endif

	// ***testing*** load delay slot probably runs before COP2 starts executing an instruction
	r->ProcessLoadDelaySlot ();
	
	r->COP2.NCCT ( r, i );
}



static const Execute::Function Execute::FunctionList []
{
	// instructions on both R3000A and R5900
	Execute::Invalid,
	Execute::J, Execute::JAL, Execute::JR, Execute::JALR, Execute::BEQ, Execute::BNE, Execute::BGTZ, Execute::BGEZ,
	Execute::BLTZ, Execute::BLEZ, Execute::BGEZAL, Execute::BLTZAL, Execute::ADD, Execute::ADDI, Execute::ADDU, Execute::ADDIU,
	Execute::SUB, Execute::SUBU, Execute::MULT, Execute::MULTU, Execute::DIV, Execute::DIVU, Execute::AND, Execute::ANDI,
	Execute::OR, Execute::ORI, Execute::XOR, Execute::XORI, Execute::NOR, Execute::LUI, Execute::SLL, Execute::SRL,
	Execute::SRA, Execute::SLLV, Execute::SRLV, Execute::SRAV, Execute::SLT, Execute::SLTI, Execute::SLTU, Execute::SLTIU,
	Execute::LB, Execute::LBU, Execute::LH, Execute::LHU, Execute::LW, Execute::LWL, Execute::LWR, Execute::SB,
	Execute::SH, Execute::SW, Execute::SWL, Execute::SWR, Execute::MFHI, Execute::MTHI, Execute::MFLO, Execute::MTLO,
	Execute::MFC0, Execute::MTC0, Execute::CFC2, Execute::CTC2, Execute::SYSCALL, Execute::BREAK,
	
	// instructions on R3000A ONLY
	Execute::MFC2, Execute::MTC2, Execute::LWC2, Execute::SWC2, Execute::RFE,
	Execute::RTPS, Execute::RTPT, Execute::CC, Execute::CDP, Execute::DCPL, Execute::DPCS, Execute::DPCT, Execute::NCS,
	Execute::NCT, Execute::NCDS, Execute::NCDT, Execute::NCCS, Execute::NCCT, Execute::GPF, Execute::GPL, Execute::AVSZ3,
	Execute::AVSZ4, Execute::SQR, Execute::OP, Execute::NCLIP, Execute::INTPL, Execute::MVMVA
};


Debug::Log Execute::debug;


/*
Execute::Function Execute::LookupTable [ 64 * 32 * 32 * 64 ];


// in format: instruction name, opcode, rs, funct, rt
R3000A::Instruction::Entry<R3000A::Instruction::Execute::Execute::Function> Execute::Entries [] = {
{ "BLTZ", 0x1, DOES_NOT_MATTER, DOES_NOT_MATTER, 0x0, Execute::BLTZ },
{ "BGEZ", 0x1, DOES_NOT_MATTER, DOES_NOT_MATTER, 0x1, Execute::BGEZ },
{ "BLTZAL", 0x1, DOES_NOT_MATTER, DOES_NOT_MATTER, 0x10, Execute::BLTZAL },
{ "BGEZAL", 0x1, DOES_NOT_MATTER, DOES_NOT_MATTER, 0x11, Execute::BGEZAL },
{ "BEQ", 0x4, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::BEQ },
{ "BNE", 0x5, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::BNE },
{ "BLEZ", 0x6, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::BLEZ },
{ "BGTZ", 0x7, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::BGTZ },
{ "J", 0x2, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::J },
{ "JAL", 0x3, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::JAL },
{ "JR", 0x0, DOES_NOT_MATTER, 0x8, DOES_NOT_MATTER, Execute::JR },
{ "JALR", 0x0, DOES_NOT_MATTER, 0x9, DOES_NOT_MATTER, Execute::JALR },
{ "LB", 0x20, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::LB },
{ "LH", 0x21, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::LH },
{ "LWL", 0x22, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::LWL },
{ "LW", 0x23, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::LW },
{ "LBU", 0x24, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::LBU },
{ "LHU", 0x25, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::LHU },
{ "LWR", 0x26, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::LWR },
{ "SB", 0x28, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::SB },
{ "SH", 0x29, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::SH },
{ "SWL", 0x2a, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::SWL },
{ "SW", 0x2b, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::SW },
{ "SWR", 0x2e, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::SWR },
{ "LWC2", 0x32, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::LWC2 },
{ "SWC2", 0x3a, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::SWC2 },
{ "ADDI", 0x8, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::ADDI },
{ "ADDIU", 0x9, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::ADDIU },
{ "SLTI", 0xa, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::SLTI },
{ "SLTIU", 0xb, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::SLTIU },
{ "ANDI", 0xc, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::ANDI },
{ "ORI", 0xd, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::ORI },
{ "XORI", 0xe, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::XORI },
{ "LUI", 0xf, DOES_NOT_MATTER, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::LUI },
{ "SLL", 0x0, DOES_NOT_MATTER, 0x0, DOES_NOT_MATTER, Execute::SLL },
{ "SRL", 0x0, DOES_NOT_MATTER, 0x2, DOES_NOT_MATTER, Execute::SRL },
{ "SRA", 0x0, DOES_NOT_MATTER, 0x3, DOES_NOT_MATTER, Execute::SRA },
{ "SLLV", 0x0, DOES_NOT_MATTER, 0x4, DOES_NOT_MATTER, Execute::SLLV },
{ "SRLV", 0x0, DOES_NOT_MATTER, 0x6, DOES_NOT_MATTER, Execute::SRLV },
{ "SRAV", 0x0, DOES_NOT_MATTER, 0x7, DOES_NOT_MATTER, Execute::SRAV },
{ "SYSCALL", 0x0, DOES_NOT_MATTER, 0xc, DOES_NOT_MATTER, Execute::SYSCALL },
{ "BREAK", 0x0, DOES_NOT_MATTER, 0xd, DOES_NOT_MATTER, Execute::BREAK },
{ "MFHI", 0x0, DOES_NOT_MATTER, 0x10, DOES_NOT_MATTER, Execute::MFHI },
{ "MTHI", 0x0, DOES_NOT_MATTER, 0x11, DOES_NOT_MATTER, Execute::MTHI },
{ "MFLO", 0x0, DOES_NOT_MATTER, 0x12, DOES_NOT_MATTER, Execute::MFLO },
{ "MTLO", 0x0, DOES_NOT_MATTER, 0x13, DOES_NOT_MATTER, Execute::MTLO },
{ "MULT", 0x0, DOES_NOT_MATTER, 0x18, DOES_NOT_MATTER, Execute::MULT },
{ "MULTU", 0x0, DOES_NOT_MATTER, 0x19, DOES_NOT_MATTER, Execute::MULTU },
{ "DIV", 0x0, DOES_NOT_MATTER, 0x1a, DOES_NOT_MATTER, Execute::DIV },
{ "DIVU", 0x0, DOES_NOT_MATTER, 0x1b, DOES_NOT_MATTER, Execute::DIVU },
{ "ADD", 0x0, DOES_NOT_MATTER, 0x20, DOES_NOT_MATTER, Execute::ADD },
{ "ADDU", 0x0, DOES_NOT_MATTER, 0x21, DOES_NOT_MATTER, Execute::ADDU },
{ "SUB", 0x0, DOES_NOT_MATTER, 0x22, DOES_NOT_MATTER, Execute::SUB },
{ "SUBU", 0x0, DOES_NOT_MATTER, 0x23, DOES_NOT_MATTER, Execute::SUBU },
{ "AND", 0x0, DOES_NOT_MATTER, 0x24, DOES_NOT_MATTER, Execute::AND },
{ "OR", 0x0, DOES_NOT_MATTER, 0x25, DOES_NOT_MATTER, Execute::OR },
{ "XOR", 0x0, DOES_NOT_MATTER, 0x26, DOES_NOT_MATTER, Execute::XOR },
{ "NOR", 0x0, DOES_NOT_MATTER, 0x27, DOES_NOT_MATTER, Execute::NOR },
{ "SLT", 0x0, DOES_NOT_MATTER, 0x2a, DOES_NOT_MATTER, Execute::SLT },
{ "SLTU", 0x0, DOES_NOT_MATTER, 0x2b, DOES_NOT_MATTER, Execute::SLTU },
{ "MFC0", 0x10, 0x0, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::MFC0 },
{ "MTC0", 0x10, 0x4, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::MTC0 },
{ "RFE", 0x10, 0x10, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x11, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x12, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x13, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x14, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x15, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x16, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x17, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x18, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x19, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x1a, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x1b, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x1c, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x1d, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x1e, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "RFE", 0x10, 0x1f, 0x10, DOES_NOT_MATTER, Execute::RFE },
{ "MFC2", 0x12, 0x0, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::MFC2 },
{ "CFC2", 0x12, 0x2, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::CFC2 },
{ "MTC2", 0x12, 0x4, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::MTC2 },
{ "CTC2", 0x12, 0x6, DOES_NOT_MATTER, DOES_NOT_MATTER, Execute::CTC2 },

// *** COP 2 Instructions ***

{ "RTPS", 0x12, DOES_NOT_MATTER, 0x1, DOES_NOT_MATTER, Execute::RTPS },
{ "NCLIP", 0x12, DOES_NOT_MATTER, 0x6, DOES_NOT_MATTER, Execute::NCLIP },
{ "OP", 0x12, DOES_NOT_MATTER, 0xc, DOES_NOT_MATTER, Execute::OP },
{ "DPCS", 0x12, DOES_NOT_MATTER, 0x10, DOES_NOT_MATTER, Execute::DPCS },
{ "INTPL", 0x12, DOES_NOT_MATTER, 0x11, DOES_NOT_MATTER, Execute::INTPL },
{ "MVMVA", 0x12, DOES_NOT_MATTER, 0x12, DOES_NOT_MATTER, Execute::MVMVA },
{ "NCDS", 0x12, DOES_NOT_MATTER, 0x13, DOES_NOT_MATTER, Execute::NCDS },
{ "CDP", 0x12, DOES_NOT_MATTER, 0x14, DOES_NOT_MATTER, Execute::CDP },
{ "NCDT", 0x12, DOES_NOT_MATTER, 0x16, DOES_NOT_MATTER, Execute::NCDT },
{ "NCCS", 0x12, DOES_NOT_MATTER, 0x1b, DOES_NOT_MATTER, Execute::NCCS },
{ "CC", 0x12, DOES_NOT_MATTER, 0x1c, DOES_NOT_MATTER, Execute::CC },
{ "NCS", 0x12, DOES_NOT_MATTER, 0x1e, DOES_NOT_MATTER, Execute::NCS },
{ "NCT", 0x12, DOES_NOT_MATTER, 0x20, DOES_NOT_MATTER, Execute::NCT },
{ "SQR", 0x12, DOES_NOT_MATTER, 0x28, DOES_NOT_MATTER, Execute::SQR },
{ "DCPL", 0x12, DOES_NOT_MATTER, 0x29, DOES_NOT_MATTER, Execute::DCPL },
{ "DPCT", 0x12, DOES_NOT_MATTER, 0x2a, DOES_NOT_MATTER, Execute::DPCT },
{ "AVSZ3", 0x12, DOES_NOT_MATTER, 0x2d, DOES_NOT_MATTER, Execute::AVSZ3 },
{ "AVSZ4", 0x12, DOES_NOT_MATTER, 0x2e, DOES_NOT_MATTER, Execute::AVSZ4 },
{ "RTPT", 0x12, DOES_NOT_MATTER, 0x30, DOES_NOT_MATTER, Execute::RTPT },
{ "GPF", 0x12, DOES_NOT_MATTER, 0x3d, DOES_NOT_MATTER, Execute::GPF },
{ "GPL", 0x12, DOES_NOT_MATTER, 0x3e, DOES_NOT_MATTER, Execute::GPL },
{ "NCCT", 0x12, DOES_NOT_MATTER, 0x3f, DOES_NOT_MATTER, Execute::NCCT }
};
*/







static void Execute::Start ()
{
#ifdef INLINE_DEBUG_ENABLE	
	debug.Create ( "R3000A_Execute_Log.txt" );
#endif

	Lookup::Start ();
}

/*	
	u32 Opcode, Rs, Funct, Rt, Index, ElementsInExecute, ElementsInBranchLoad1;
	Instruction::Format i;

#ifdef INLINE_DEBUG
	debug << "Running constructor for Execute class.\r\n";
#endif

	cout << "Running constructor for Execute class.\n";

#ifdef INLINE_DEBUG_ENABLE	
	debug.Create ( "R3000A_Execute_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "Running constructor for Execute class.\r\n";
#endif

	
	ElementsInExecute = (sizeof(Entries) / sizeof(Entries[0]));

	for ( Opcode = 0; Opcode < 64; Opcode++ )
	{
	
		for ( Rs = 0; Rs < 32; Rs++ )
		{
		
			for ( Funct = 0; Funct < 64; Funct++ )
			{
			
				for ( Rt = 0; Rt < 32; Rt++ )
				{
					i.Opcode = Opcode;
					i.Rs = Rs;
					i.Funct = Funct;
					i.Rt = Rt;
				
					// initialize entry in LUT to Invalid instruction
					LookupTable [ ( ( i.Value >> 16 ) | ( i.Value << 16 ) ) & 0x3fffff ] = &Execute::Invalid;
					
					// lookup entry in list of instructions
					for ( Index = 0; Index < ElementsInExecute; Index++ )
					{
						// check if we have found the instruction to insert into current position of LUT
						if ( ( Entries [ Index ].Opcode == Opcode || Entries [ Index ].Opcode == DOES_NOT_MATTER )
						&& ( Entries [ Index ].Rs == Rs || Entries [ Index ].Rs == DOES_NOT_MATTER )
						&& ( Entries [ Index ].Funct == Funct || Entries [ Index ].Funct == DOES_NOT_MATTER )
						&& ( Entries [ Index ].Rt == Rt || Entries [ Index ].Rt == DOES_NOT_MATTER ) )
						{
							// enter function for entry into LUT
							LookupTable [ ( ( i.Value >> 16 ) | ( i.Value << 16 ) ) & 0x3fffff ] = Entries [ Index ].FunctionToCall;

							
							break;
						}

					}
				
				}
				
			}
			
		}
		
	}
}

*/

// generates the lookup table
Execute::Execute ( Cpu* pCpu )
{
	r = pCpu;
}

