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




#include "R5900_Execute.h"
#include "R5900_Lookup.h"

#include "VU.h"
#include "VU_Execute.h"
#include "PS2Float.h"

#include "R5900_Print.h"

#include "GeneralUtilities.h"

#include <iostream>
#include <iomanip>

using namespace GeneralUtilities;
using namespace Playstation2;
using namespace PS2Float;


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


//#define INLINE_DEBUG_LQ
//#define INLINE_DEBUG_SQ
//#define INLINE_DEBUG_SWL
//#define INLINE_DEBUG_LWL
//#define INLINE_DEBUG_SWR
//#define INLINE_DEBUG_LWR
//#define INLINE_DEBUG_SYSCALL
//#define INLINE_DEBUG_RFE
//#define INLINE_DEBUG_R5900
#define INLINE_DEBUG_INVALID
#define INLINE_DEBUG_UNIMPLEMENTED
//#define INLINE_DEBUG_ERET
//#define INLINE_DEBUG_INTEGER_VECTOR
//#define INLINE_DEBUG_VU0
//#define INLINE_DEBUG_FPU

/*
#define INLINE_DEBUG_PABSH
#define INLINE_DEBUG_PABSW
#define INLINE_DEBUG_PADDB
#define INLINE_DEBUG_PADDH
#define INLINE_DEBUG_PADDW
#define INLINE_DEBUG_PADSBH
#define INLINE_DEBUG_PAND


#define INLINE_DEBUG_PCPYH
#define INLINE_DEBUG_PCPYLD
#define INLINE_DEBUG_PCPYUD

#define INLINE_DEBUG_PDIVBW
#define INLINE_DEBUG_PDIVUW
#define INLINE_DEBUG_PDIVW

#define INLINE_DEBUG_PEXCH
#define INLINE_DEBUG_PEXCW
#define INLINE_DEBUG_PEXEH
#define INLINE_DEBUG_PEXEW
#define INLINE_DEBUG_PEXT5
#define INLINE_DEBUG_PEXTLB
#define INLINE_DEBUG_PEXTLH
#define INLINE_DEBUG_PEXTLW
#define INLINE_DEBUG_PEXTUB
#define INLINE_DEBUG_PEXTUH
#define INLINE_DEBUG_PEXTUW


#define INLINE_DEBUG_PCEQB
#define INLINE_DEBUG_PCEQH
#define INLINE_DEBUG_PCEQW
#define INLINE_DEBUG_PCGTB
#define INLINE_DEBUG_PCGTH
#define INLINE_DEBUG_PCGTW
#define INLINE_DEBUG_PMAXH
#define INLINE_DEBUG_PMAXW
#define INLINE_DEBUG_PMINH
#define INLINE_DEBUG_PMINW
*/

//#define INLINE_DEBUG_MTC0
//#define INLINE_DEBUG_MFC0
//#define INLINE_DEBUG_CTC0
//#define INLINE_DEBUG_CFC0
//#define INLINE_DEBUG_MTC1


//#define INLINE_DEBUG_LWC2
//#define INLINE_DEBUG_SWC2
//#define INLINE_DEBUG_QMTC2_I
//#define INLINE_DEBUG_QMFC2_I
//#define INLINE_DEBUG_QMTC2_NI
//#define INLINE_DEBUG_QMFC2_NI
//#define INLINE_DEBUG_CTC2
//#define INLINE_DEBUG_CFC2
//#define INLINE_DEBUG_COP2


//#define INLINE_DEBUG_DIV_S
//#define INLINE_DEBUG_ADD_S
//#define INLINE_DEBUG_SUB_S
//#define INLINE_DEBUG_MUL_S
//#define INLINE_DEBUG_ADDA_S
//#define INLINE_DEBUG_SUBA_S
//#define INLINE_DEBUG_MULA_S
//#define INLINE_DEBUG_SQRT_S
//#define INLINE_DEBUG_RSQRT_S
//#define INLINE_DEBUG_MADD_S
//#define INLINE_DEBUG_MSUB_S
//#define INLINE_DEBUG_MADDA_S
//#define INLINE_DEBUG_MSUBA_S
//#define INLINE_DEBUG_CVT_S_W
//#define INLINE_DEBUG_CVT_W_S
//#define INLINE_DEBUG_MIN_S
//#define INLINE_DEBUG_MAX_S


//#define COUT_USERMODE_LOAD
//#define COUT_USERMODE_STORE
//#define COUT_FC
//#define COUT_SWC

#define INLINE_DEBUG_TRACE

#endif


using namespace std;

// this area deals with the execution of instructions on the R5900
using namespace R5900;
using namespace R5900::Instruction;



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
//#define PROCESS_LOADDELAY_BEFORESTORE


namespace R5900
{

namespace Instruction
{


// static vars //
Cpu *Execute::r;



static void Execute::Start ()
{
#ifdef INLINE_DEBUG_ENABLE	
	debug.Create ( "R5900_Execute_Log.txt" );
#endif

	cout << "\nRunning R5900::Execute::Start\n";

	// this function currently takes a long time to execute
	Lookup::Start ();
}


// *** R3000A Instructions *** //


////////////////////////////////////////////////
// R-Type Instructions (non-interrupt)


// regular arithemetic
void Execute::ADDU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDU || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// add without overflow exception: rd = rs + rt
	r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rs ].u + r->GPR [ i.Rt ].u );
	
#if defined INLINE_DEBUG_ADDU || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SUBU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBU || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// subtract without overflow exception: rd = rs - rt
	r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rs ].u - r->GPR [ i.Rt ].u );
	
#if defined INLINE_DEBUG_SUBU || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::AND ( Instruction::Format i )
{
#if defined INLINE_DEBUG_AND || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// logical AND: rd = rs & rt
	r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u & r->GPR [ i.Rt ].u;
	
#if defined INLINE_DEBUG_AND || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::OR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_OR || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// logical OR: rd = rs | rt
	r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u | r->GPR [ i.Rt ].u;
	
#if defined INLINE_DEBUG_OR || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::XOR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_XOR || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// logical XOR: rd = rs ^ rt
	r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u ^ r->GPR [ i.Rt ].u;
	
#if defined INLINE_DEBUG_XOR || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::NOR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_NOR || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// logical NOR: rd = ~(rs | rt)
	r->GPR [ i.Rd ].u = ~( r->GPR [ i.Rs ].u | r->GPR [ i.Rt ].u );
	
#if defined INLINE_DEBUG_NOR || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SLT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SLT || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// set less than signed: rd = rs < rt ? 1 : 0
	r->GPR [ i.Rd ].s = r->GPR [ i.Rs ].s < r->GPR [ i.Rt ].s ? 1 : 0;
	
#if defined INLINE_DEBUG_SLT || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SLTU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SLTU || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// set less than signed: rd = rs < rt ? 1 : 0
	r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u < r->GPR [ i.Rt ].u ? 1 : 0;
	
#if defined INLINE_DEBUG_SLTU || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}


////////////////////////////////////////////
// I-Type Instructions (non-interrupt)



void Execute::ADDIU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDIU || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif

	// *note* immediate value is sign-extended before the addition is performed

	// add immedeate without overflow exception: rt = rs + immediate
	r->GPR [ i.Rt ].s = (s32) ( r->GPR [ i.Rs ].s + i.sImmediate );
	
#if defined INLINE_DEBUG_ADDIU || defined INLINE_DEBUG_R5900
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::ANDI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ANDI || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// Logical AND zero-extended immedeate: rt = rs & immediate
	r->GPR [ i.Rt ].u = r->GPR [ i.Rs ].u & ( (u64) i.uImmediate );
	
#if defined INLINE_DEBUG_ANDI || defined INLINE_DEBUG_R5900
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::ORI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ORI || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// Logical OR zero-extended immedeate: rt = rs | immediate
	r->GPR [ i.Rt ].u = r->GPR [ i.Rs ].u | ( (u64) i.uImmediate );
	
#if defined INLINE_DEBUG_ORI || defined INLINE_DEBUG_R5900
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::XORI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_XORI || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// Logical XOR zero-extended immedeate: rt = rs & immediate
	r->GPR [ i.Rt ].u = r->GPR [ i.Rs ].u ^ ( (u64) i.uImmediate );
	
#if defined INLINE_DEBUG_XORI || defined INLINE_DEBUG_R5900
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::SLTI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SLTI || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// Set less than sign-extended immedeate: rt = rs < immediate ? 1 : 0
	r->GPR [ i.Rt ].s = r->GPR [ i.Rs ].s < ( (s64) i.sImmediate ) ? 1 : 0;
	
#if defined INLINE_DEBUG_SLTI || defined INLINE_DEBUG_R5900
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::SLTIU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SLTIU || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// *note* Rs is still sign-extended here, but the comparison is an unsigned one

	// Set if unsigned less than sign-extended immedeate: rt = rs < immediate ? 1 : 0
	r->GPR [ i.Rt ].u = r->GPR [ i.Rs ].u < ((u64) ((s64) i.sImmediate)) ? 1 : 0;
	
#if defined INLINE_DEBUG_SLTIU || defined INLINE_DEBUG_R5900
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::LUI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LUI || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif
	
	// Load upper immediate
	r->GPR [ i.Rt ].s = ( ( (s64) i.sImmediate ) << 16 );
	
#if defined INLINE_DEBUG_LUI || defined INLINE_DEBUG_R5900
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}





void Execute::MFHI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MFHI || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: HI = " << r->HI.u;
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
	r->GPR [ i.Rd ].u = r->HI.u;	//r->HiLo.uHi;
	
#if defined INLINE_DEBUG_MFHI || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::MFLO ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MFLO || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: LO = " << r->LO.u;
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
	r->GPR [ i.Rd ].u = r->LO.u;	//r->HiLo.uLo;
	
#if defined INLINE_DEBUG_MFLO || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}




void Execute::MTHI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MTHI || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// there is no MTHI/MTLO delay slot
	//r->HiLo.uHi = r->GPR [ i.Rs ].u;
	r->HI.u = r->GPR [ i.Rs ].u;
}

void Execute::MTLO ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MTLO || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// there is no MTHI/MTLO delay slot
	//r->HiLo.uLo = r->GPR [ i.Rs ].u;
	r->LO.u = r->GPR [ i.Rs ].u;
}


//////////////////////////////////////////////////////////
// Shift instructions



void Execute::SLL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SLL || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// shift left logical: rd = rt << shift
	//r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rt ].u << i.Shift );
	r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rt ].uw0 << i.Shift );
	
#if defined INLINE_DEBUG_SLL || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SRL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SRL || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// shift right logical: rd = rt >> shift
	//r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rt ].u >> i.Shift );
	r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rt ].uw0 >> i.Shift );
	
#if defined INLINE_DEBUG_SRL || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SRA ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SRA || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// shift right arithmetic: rd = rt >> shift
	//r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rt ].s >> i.Shift );
	r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rt ].sw0 >> i.Shift );
	
#if defined INLINE_DEBUG_SRA || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SLLV ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SLLV || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// shift left logical variable: rd = rt << rs
	//r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rt ].u << ( r->GPR [ i.Rs ].u & 0x1f ) );
	r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rt ].uw0 << ( r->GPR [ i.Rs ].uw0 & 0x1f ) );
	
#if defined INLINE_DEBUG_SLLV || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SRLV ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SRLV || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// shift right logical variable: rd = rt >> rs
	//r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rt ].u >> ( r->GPR [ i.Rs ].u & 0x1f ) );
	r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rt ].uw0 >> ( r->GPR [ i.Rs ].uw0 & 0x1f ) );
	
#if defined INLINE_DEBUG_SRLV || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::SRAV ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SRAV || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// shift right arithmetic variable: rd = rt >> rs
	//r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rt ].s >> ( r->GPR [ i.Rs ].u & 0x1f ) );
	r->GPR [ i.Rd ].s = (s32) ( r->GPR [ i.Rt ].sw0 >> ( r->GPR [ i.Rs ].uw0 & 0x1f ) );
	
#if defined INLINE_DEBUG_SRAV || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}


/////////////////////////////////////////////////////////////
// Multiply/Divide Instructions


void Execute::MULT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MULT || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// *** todo *** Add PS2 form of instruction that writes output to registers
	
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
	//r->HiLo.sValue = ((s64) (r->GPR [ i.Rs ].s)) * ((s64) (r->GPR [ i.Rt ].s));
	s64 temp;
	temp = ( (s64) ( r->GPR [ i.Rs ].sw0 ) ) * ( (s64) ( r->GPR [ i.Rt ].sw0 ) );
	r->LO.s = (s32) temp;
	r->HI.s = (s32) ( temp >> 32 );
	
	// R5900 can additionally write to register
	if ( i.Rd )
	{
		r->GPR [ i.Rd ].sq0 = (s32) temp;
	}

#if defined INLINE_DEBUG_MULT || defined INLINE_DEBUG_R5900
	debug << "; Output: LO=" << r->LO.s << "; HI=" << r->HI.s << "; rd=" << r->GPR [ i.Rd ].s;
#endif
}

void Execute::MULTU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MULTU || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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
	//r->HiLo.uValue = ((u64) (r->GPR [ i.Rs ].u)) * ((u64) (r->GPR [ i.Rt ].u));
	u64 temp;
	temp = ( (u64) ( r->GPR [ i.Rs ].uw0 ) ) * ( (u64) ( r->GPR [ i.Rt ].uw0 ) );
	r->LO.s = (s32) temp;
	r->HI.s = (s32) ( temp >> 32 );
	
	// R5900 can additionally write to register
	if ( i.Rd )
	{
		r->GPR [ i.Rd ].sq0 = (s32) temp;
	}
	
#if defined INLINE_DEBUG_MULTU || defined INLINE_DEBUG_R5900
	debug << "; Output: LO=" << r->LO.s << "; HI=" << r->HI.s << "; rd=" << r->GPR [ i.Rd ].s;
#endif
}

void Execute::DIV ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DIV || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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
	//if ( r->GPR [ i.Rt ].u != 0 )
	if ( r->GPR [ i.Rt ].uw0 != 0 )
	{
		// if rs = 0x80000000 and rt = -1 then hi = 0 and lo = 0x80000000
		if ( r->GPR [ i.Rs ].uw0 == 0x80000000 && r->GPR [ i.Rt ].sw0 == -1 )
		{
			// *todo* check if all this stuff is sign extended during testing
			//r->HiLo.uHi = 0;
			//r->HiLo.uLo = 0x80000000;
			r->HI.s = 0;
			r->LO.s = (s32) 0x80000000;
		}
		else
		{
			// *todo* check during testing if signs are switched on modulus result when sign of dividend and divisor are different
			//r->HiLo.sLo = r->GPR [ i.Rs ].s / r->GPR [ i.Rt ].s;
			//r->HiLo.sHi = r->GPR [ i.Rs ].s % r->GPR [ i.Rt ].s;
			r->LO.s = r->GPR [ i.Rs ].sw0 / r->GPR [ i.Rt ].sw0;
			r->HI.s = r->GPR [ i.Rs ].sw0 % r->GPR [ i.Rt ].sw0;
		}
	}
	else
	{
		if ( r->GPR [ i.Rs ].s < 0 )
		{
			//r->HiLo.sLo = 1;
			r->LO.s = 1;
		}
		else
		{
			//r->HiLo.sLo = -1;
			r->LO.s = -1;
		}
		
		//r->HiLo.uHi = r->GPR [ i.Rs ].u;
		r->HI.s = r->GPR [ i.Rs ].sw0;
	}
	
#if defined INLINE_DEBUG_DIV || defined INLINE_DEBUG_R5900
	debug << "; Output: LO = " << r->LO.s << "; HI = " << r->HI.s;
#endif
}

void Execute::DIVU ( Instruction::Format i )
{
	// 36 cycles
	static const int c_iDivideCycles = 36;
	
#if defined INLINE_DEBUG_DIVU || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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
	//if ( r->GPR [ i.Rt ].u != 0 )
	if ( r->GPR [ i.Rt ].uw0 != 0 )
	{
		//r->HiLo.uLo = r->GPR [ i.Rs ].u / r->GPR [ i.Rt ].u;
		//r->HiLo.uHi = r->GPR [ i.Rs ].u % r->GPR [ i.Rt ].u;
		r->LO.s = (s32) ( r->GPR [ i.Rs ].uw0 / r->GPR [ i.Rt ].uw0 );
		r->HI.s = (s32) ( r->GPR [ i.Rs ].uw0 % r->GPR [ i.Rt ].uw0 );
	}
	else
	{
		//r->HiLo.sLo = -1;
		//r->HiLo.uHi = r->GPR [ i.Rs ].u;
		r->LO.s = -1;
		r->HI.s = r->GPR [ i.Rs ].sw0;
	}
	
#if defined INLINE_DEBUG_DIVU || defined INLINE_DEBUG_R5900
	debug << "; Output: LO = " << r->LO.s << "; HI = " << r->HI.s;
#endif
}



////////////////////////////////////////////
// Jump/Branch Instructions



void Execute::J ( Instruction::Format i )
{
#if defined INLINE_DEBUG_J || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif
	
	// next instruction is in the branch delay slot
	Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
	//r->DelaySlot0.Instruction = i;
	//r->DelaySlot0.cb = r->_cb_Jump;
	d->Instruction = i;
	d->cb = r->_cb_Jump;
	r->Status.DelaySlot_Valid |= 0x2;
}

void Execute::JR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_JR || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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
	Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
	d->Instruction = i;
	d->cb = r->_cb_JumpRegister;

	// *** todo *** check if address exception should be generated if lower 2-bits of jump address are not zero
	// will clear out lower two bits of address for now
	d->Data = r->GPR [ i.Rs ].u & ~3;
	
	r->Status.DelaySlot_Valid |= 0x2;
}

void Execute::JAL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_JAL || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	
	// next instruction is in the branch delay slot
	Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
	d->Instruction = i;
	d->cb = r->_cb_Jump;
	r->Status.DelaySlot_Valid |= 0x2;

	// *** note *** this is tricky because return address gets stored to r31 after execution of load delay slot but before next instruction
	///////////////////////////////////////////////////////////////
	// Store return address when instruction is executed in r31
	r->GPR [ 31 ].u = r->PC + 8;
	
#if defined INLINE_DEBUG_JAL || defined INLINE_DEBUG_R5900
	debug << "; Output: r31 = " << r->GPR [ 31 ].u;
#endif
}

void Execute::JALR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_JALR || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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
	//r->DelaySlot0.Instruction = i;
	
	// *** todo *** check if address exception should be generated if lower 2-bits of jump address are not zero
	// will clear out lower two bits of address for now
	Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
	d->Instruction = i;
	d->Data = r->GPR [ i.Rs ].u & ~3;
	d->cb = r->_cb_JumpRegister;
	
	r->Status.DelaySlot_Valid |= 0x2;

	///////////////////////////////////////////////////////////////
	// Store return address when instruction is executed in Rd
	// *note* this must happen AFTER the stuff above
	r->GPR [ i.Rd ].u = r->PC + 8;
	
#if defined INLINE_DEBUG_JALR || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::BEQ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BEQ || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif
	
	if ( r->GPR [ i.Rs ].u == r->GPR [ i.Rt ].u )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BEQ || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
}

void Execute::BNE ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BNE || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif
	
	if ( r->GPR [ i.Rs ].u != r->GPR [ i.Rt ].u )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BNE || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
}

void Execute::BLEZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BLEZ || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	if ( r->GPR [ i.Rs ].s <= 0 )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BLEZ || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
}

void Execute::BGTZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BGTZ || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	if ( r->GPR [ i.Rs ].s > 0 )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BGTZ || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
}

void Execute::BLTZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BLTZ || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	if ( r->GPR [ i.Rs ].s < 0 )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BLTZ || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
}

void Execute::BGEZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BGEZ || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	if ( r->GPR [ i.Rs ].s >= 0 )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BGEZ || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
}

void Execute::BLTZAL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BLTZAL || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	if ( r->GPR [ i.Rs ].s < 0 )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BLTZAL || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
	
	////////////////////////////////////////////////////////////////////////
	// Store return address when instruction is executed in r31
	// for this instruction this happens whether branch is taken or not
	// *note* this must happen AFTER comparison check
	r->GPR [ 31 ].u = r->PC + 8;
}

void Execute::BGEZAL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BGEZAL || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif
	
	if ( r->GPR [ i.Rs ].s >= 0 )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BGEZAL || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}

	////////////////////////////////////////////////////////////////////////
	// Store return address when instruction is executed in r31
	// for this instruction this happens whether branch is taken or not
	// *note* this must happen AFTER comparison check
	r->GPR [ 31 ].u = r->PC + 8;
}



/*
void Execute::RFE ( Instruction::Format i )
{
#if defined INLINE_DEBUG_RFE || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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

#if defined INLINE_DEBUG_RFE || defined INLINE_DEBUG_R5900
	debug << "\r\n(after) _Intc_Stat=" << hex << *r->_Intc_Stat << " _Intc_Mask=" << *r->_Intc_Mask << " _R3000A_Status=" << r->CPR0.Regs [ 12 ] << " _R3000A_Cause=" << r->CPR0.Regs [ 13 ] << " _ProcStatus=" << r->Status.Value << " CycleCount=" << dec << r->CycleCount;
#endif
}
*/



////////////////////////////////////////////////////////
// Instructions that can cause Synchronous Interrupts //
////////////////////////////////////////////////////////


void Execute::ADD ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ADD || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif
	
	s32 temp;
	
	temp = r->GPR [ i.Rs ].sw0 + r->GPR [ i.Rt ].sw0;
	
	// if the carry outs of bits 30 and 31 differ, then it's signed overflow
	//if ( ( temp < -2147483648LL ) || ( temp > 2147483647LL ) )
	//if( (INT32)( ~( m_r[ INS_RS( m_op ) ] ^ m_r[ INS_RT( m_op ) ] ) & ( m_r[ INS_RS( m_op ) ] ^ result ) ) < 0 )
	//if ( (s32)( ~( r->GPR [ i.Rs ].s ^ r->GPR [ i.Rt ].s ) & ( r->GPR [ i.Rs ].s ^ temp ) ) < 0 )
	//if ( temp < -0x80000000LL || temp > 0x7fffffffLL )
	if ( ( ( ~( r->GPR [ i.Rs ].sw0 ^ r->GPR [ i.Rt ].sw0 ) ) & ( r->GPR [ i.Rs ].sw0 ^ temp ) ) < 0 )
	{
		// overflow
		cout << "\nhps2x64: Execute::ADD generated an overflow exception @ Cycle#" << dec << r->CycleCount << " PC=" << hex << r->PC << "\n";
		r->ProcessSynchronousInterrupt ( Cpu::EXC_OV );
		
#if defined INLINE_DEBUG_ADD || defined INLINE_DEBUG_R5900
		debug << ";  INT";
#endif
	}
	else
	{
		// it's cool - we can do the add and store the result to register
		// sign-extend for R5900
		r->GPR [ i.Rd ].s = temp;
	}
	
#if defined INLINE_DEBUG_ADD || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

void Execute::ADDI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDI || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif

	s32 temp;
	
	//temp = ( (s64) r->GPR [ i.Rs ].sw0 ) + ( (s64) i.sImmediate );
	temp = r->GPR [ i.Rs ].sw0 + ( (s32) i.sImmediate );
	
	// if the carry outs of bits 30 and 31 differ, then it's signed overflow
	//if ( ( temp < -2147483648LL ) || ( temp > 2147483647LL ) )
	//if( (INT32)( ~( m_r[ INS_RS( m_op ) ] ^ m_r[ INS_RT( m_op ) ] ) & ( m_r[ INS_RS( m_op ) ] ^ result ) ) < 0 )
	//if ( (s32)( ~( r->GPR [ i.Rs ].s ^ ( (s32) i.sImmediate ) ) & ( r->GPR [ i.Rs ].s ^ temp ) ) < 0 )
	//if ( temp < -0x80000000LL || temp > 0x7fffffffLL )
	if ( ( ( ~( r->GPR [ i.Rs ].sw0 ^ ( (s32) i.sImmediate ) ) ) & ( r->GPR [ i.Rs ].sw0 ^ temp ) ) < 0 )
	{
		// overflow
		cout << "\nhps2x64: Execute::ADDI generated an overflow exception @ Cycle#" << dec << r->CycleCount << " PC=" << hex << r->PC << "\n";
		r->ProcessSynchronousInterrupt ( Cpu::EXC_OV );
		
#if defined INLINE_DEBUG_ADDI || defined INLINE_DEBUG_R5900
		debug << ";  INT";
#endif
	}
	else
	{
		// it's cool - we can do the addi and store the result to register
		r->GPR [ i.Rt ].s = temp;
	}
	
#if defined INLINE_DEBUG_ADDI || defined INLINE_DEBUG_R5900
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

void Execute::SUB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SUB || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif
	
	s32 temp;
	
	temp = r->GPR [ i.Rs ].sw0 - r->GPR [ i.Rt ].sw0;
	
	// if the carry outs of bits 30 and 31 differ, then it's signed overflow
	//if ( ( temp < -2147483648LL ) || ( temp > 2147483647LL ) )
	//if( (INT32)( ( m_r[ INS_RS( m_op ) ] ^ m_r[ INS_RT( m_op ) ] ) & ( m_r[ INS_RS( m_op ) ] ^ result ) ) < 0 )
	//if ( (s32) ( ( r->GPR [ i.Rs ].s ^ r->GPR [ i.Rt ].s ) & ( r->GPR [ i.Rs ].s ^ temp ) ) < 0 )
	//if ( temp < -0x80000000LL || temp > 0x7fffffffLL )
	if ( ( ( r->GPR [ i.Rs ].sw0 ^ r->GPR [ i.Rt ].sw0 ) & ( r->GPR [ i.Rs ].sw0 ^ temp ) ) < 0 )
	{
		// overflow
		cout << "\nhps2x64: Execute::SUB generated an overflow exception @ Cycle#" << dec << r->CycleCount << " PC=" << hex << r->PC << "\n";
		r->ProcessSynchronousInterrupt ( Cpu::EXC_OV );
		
#if defined INLINE_DEBUG_SUB || defined INLINE_DEBUG_R5900
		debug << ";  INT";
#endif
	}
	else
	{
		// it's cool - we can do the sub and store the result to register
		r->GPR [ i.Rd ].s = temp;
	}
	
#if defined INLINE_DEBUG_SUB || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}




void Execute::SYSCALL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SYSCALL || defined INLINE_DEBUG_R5900
	debug << "\r\nBefore:" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "\r\n" << hex << "Status=" << r->CPR0.Regs [ 12 ] << " Cause=" << r->CPR0.Regs [ 13 ] << " a0=" << r->GPR [ 4 ].u << " a1=" << r->GPR [ 5 ].u;
	debug << " r1=" << r->GPR [ 1 ].u;
	debug << " r2=" << r->GPR [ 2 ].u;
	debug << " r3=" << r->GPR [ 3 ].u;
#endif
	
	r->ProcessSynchronousInterrupt ( Cpu::EXC_SYSCALL );
	
#if defined INLINE_DEBUG_SYSCALL || defined INLINE_DEBUG_R5900
	debug << "\r\nAfter:" << hex << setw( 8 ) << r->PC << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << i.Value;
	debug << "\r\n" << hex << "Status=" << r->CPR0.Regs [ 12 ] << " Cause=" << r->CPR0.Regs [ 13 ] << " a0=" << r->GPR [ 4 ].u << " a1=" << r->GPR [ 5 ].u;
	debug << " r1=" << r->GPR [ 1 ].u;
	debug << " r2=" << r->GPR [ 2 ].u;
	debug << " r3=" << r->GPR [ 3 ].u;
#endif
}

void Execute::BREAK ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BREAK || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif
	
	cout << "\nhps1x64: Execute::BREAK generated an exception @ Cycle#" << dec << r->CycleCount << " PC=" << hex << r->PC << "\n";
	r->ProcessSynchronousInterrupt ( Cpu::EXC_BP );
	
	// say to stop if we are debugging
	Cpu::DebugStatus.Stop = true;
	Cpu::DebugStatus.Done = true;
}

void Execute::Invalid ( Instruction::Format i )
{
#if defined INLINE_DEBUG_INVALID || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	cout << "\nhps1x64 NOTE: Invalid Instruction @ Cycle#" << dec << r->CycleCount << " PC=" << hex << r->PC << " Instruction=" << i.Value << " LastPC=" << r->LastPC << "\n";
	r->ProcessSynchronousInterrupt ( Cpu::EXC_RI );
}





void Execute::MFC0 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MFC0 || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: CPR[0,rd] = " << r->CPR0.Regs [ i.Rd ];
#endif

	// MFCz rt, rd
	// 1 instruction delay?
	//r->DelaySlot0.Instruction = i;
	//r->DelaySlot0.Data = r->CPR0.Regs [ i.Rd ];
	//r->DelaySlot0.cb = r->_cb_FC;
	//r->Status.DelaySlot_Valid |= 0x1;
	
	// no delay slot on R5900?
	r->GPR [ i.Rt ].sq0 = (s32) r->Read_MFC0 ( i.Rd );	//r->CPR0.Regs [ i.Rd ];
	
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
#if defined INLINE_DEBUG_MTC0 || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// MTCz rt, rd
	// 1 instruction delay?
	//r->DelaySlot0.Instruction = i;
	//r->DelaySlot0.Data = r->GPR [ i.Rt ].u;
	//r->DelaySlot0.cb = r->_cb_MTC0;
	//r->Status.DelaySlot_Valid |= 0x1;
	
	// no delay slot on R5900?
	//r->CPR0.Regs [ i.Rd ] = r->GPR [ i.Rt ].sw0;
	r->Write_MTC0 ( i.Rd, r->GPR [ i.Rt ].sw0 );
	
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




void Execute::MTC2_Callback ( Cpu* r )
{
	//r->COP2.Write_MTC ( r->DelaySlot1.Instruction.Rd, r->DelaySlot1.Data );
	
	// clear delay slot
	r->DelaySlot1.Value = 0;
}



void Execute::CFC2 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_CFC2 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: CPC[2,rd] = " << VU0::_VU0->vi [ i.Rd ].u;
#endif
	
	// CFCz rt, rd
	// no delay slot on R5900?
	//r->GPR [ i.Rt ].sq0 = (s32) ( r->CPC2 [ i.Rd ] & 0xffff );
	//r->GPR [ i.Rt ].uq0 = VU0::_VU0->vi [ i.Rd ].u;
	r->GPR [ i.Rt ].sq0 = (s32) VU0::_VU0->Read_CFC ( i.Rd );
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].u )
#endif
}



void Execute::CTC2 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_CTC2 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// CTCz rt, rd
	// no delay slot on R5900?
	//r->CPC2 [ i.Rd ] = ( r->GPR [ i.Rt ].sw0 & 0xffff );
	//VU0::_VU0->vi [ i.Rd ].u = r->GPR [ i.Rt ].uw0;
	VU0::_VU0->Write_CTC ( i.Rd, r->GPR [ i.Rt ].uw0 );

#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].u )
#endif
}


void Execute::CTC2_Callback ( Cpu* r )
{
	//r->COP2.Write_CTC ( r->DelaySlot1.Instruction.Rd, r->DelaySlot1.Data );
	
	// clear delay slot
	r->DelaySlot1.Value = 0;
}



// Load/Store - will need to use address translation to get physical addresses when needed

//////////////////////////////////////////////////////////////////////////
// store instructions

// store instructions
void Execute::SB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SB || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif
	
	// SB rt, offset(base)

	// step 1: check if storing to data cache
	//u32 StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	u32 StoreAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	// clear top 3 bits since there is no data cache for caching stores
	// don't clear top 3 bits since scratchpad is at 0x70000000
	//StoreAddress &= 0x1fffffff;

	
	// ***todo*** perform store of byte
	r->Bus->Write ( StoreAddress, r->GPR [ i.Rt ].uq0, 0xff );
	
	
	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].u )
#endif
}





void Execute::SH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SH || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif
	
	// SH rt, offset(base)
	
	// step 1: check if storing to data cache
	//u32 StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	u32 StoreAddress = r->GPR [ i.Base ].sw0 + i.sOffset;

	// *** testing *** alert if load is from unaligned address
	if ( StoreAddress & 0x1 )
	{
		cout << "\nhps2x64 ALERT: StoreAddress is unaligned for SH @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADES );
		return;
	}
	
	// clear top 3 bits since there is no data cache for caching stores
	// don't clear top 3 bits since scratchpad is at 0x70000000
	//StoreAddress &= 0x1fffffff;
	
	
	// ***todo*** perform store of halfword
	r->Bus->Write ( StoreAddress, r->GPR [ i.Rt ].uq0, 0xffff );
	
	

	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].u )
#endif
}

void Execute::SW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SW || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif
	
	// SW rt, offset(base)
	u32 StoreAddress;
	
	// check if storing to data cache
	//StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	StoreAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	// *** testing *** alert if load is from unaligned address
	if ( StoreAddress & 0x3 )
	{
		cout << "\nhps1x64 ALERT: StoreAddress is unaligned for SW @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADES );
		return;
	}
	
	// clear top 3 bits since there is no data cache for caching stores
	// don't clear top 3 bits since scratchpad is at 0x70000000
	//StoreAddress &= 0x1fffffff;
	
	
	// ***todo*** perform store of word
	r->Bus->Write ( StoreAddress, r->GPR [ i.Rt ].uq0, 0xffffffff );
	
	

	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].uw0 )
#endif
}



void Execute::SWL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SWL || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif

	static const u32 c_Mask = 0xffffffff;
	u32 Type, Offset;

	// SWL rt, offset(base)
	
	// step 1: check if storing to data cache
	//u32 StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	u32 StoreAddress = r->GPR [ i.Base ].sw0 + i.sOffset;

	// clear top 3 bits since there is no data cache for caching stores
	//StoreAddress &= 0x1fffffff;
	
	
	// ***todo*** perform store SWL
	r->Bus->Write ( StoreAddress & ~3, r->GPR [ i.Rt ].uw0 >> ( ( 3 - ( StoreAddress & 3 ) ) << 3 ), 0xffffffffUL >> ( ( 3 - ( StoreAddress & 3 ) ) << 3 ) );
	

	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].u )
#endif
}

void Execute::SWR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SWR || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif

	static const u32 c_Mask = 0xffffffff;
	u32 Type, Offset;
	
	// SWR rt, offset(base)
	
	// step 1: check if storing to data cache
	//u32 StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	u32 StoreAddress = r->GPR [ i.Base ].sw0 + i.sOffset;

	// clear top 3 bits since there is no data cache for caching stores
	//StoreAddress &= 0x1fffffff;
	
	
	// ***todo*** perform store SWR
	r->Bus->Write ( StoreAddress & ~3, r->GPR [ i.Rt ].uw0 << ( ( StoreAddress & 3 ) << 3 ), 0xffffffffUL << ( ( StoreAddress & 3 ) << 3 ) );
	
	
	
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
#if defined INLINE_DEBUG_LB || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif
	
	// signed load byte from memory
	// LB rt, offset(base)
	
	u32 LoadAddress;
	
	// set load to happen after delay slot
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	
	// ***todo*** perform signed load of byte
	r->GPR [ i.Rt ].sq0 = (s8) r->Bus->Read ( LoadAddress, 0xff );
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}






void Execute::LH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LH || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif
	
	// LH rt, offset(base)
	
	u32 LoadAddress;
	
	// set load to happen after delay slot
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	// *** testing *** alert if load is from unaligned address
	if ( LoadAddress & 0x1 )
	{
		cout << "\nhps1x64 ALERT: LoadAddress is unaligned for LH @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
		return;
	}
	
	
	// ***todo*** perform signed load of halfword
	r->GPR [ i.Rt ].sq0 = (s16) r->Bus->Read ( LoadAddress, 0xffff );
	
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}








void Execute::LW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LW || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif
	
	// LW rt, offset(base)
	
	u32 LoadAddress;
	
	// set load to happen after delay slot
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	// *** testing *** alert if load is from unaligned address
	if ( LoadAddress & 0x3 )
	{
		cout << "\nhps2x64 ALERT: LoadAddress is unaligned for LW @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
		return;
	}
	
	
	// ***todo*** perform signed load of word
	r->GPR [ i.Rt ].sq0 = (s32) r->Bus->Read ( LoadAddress, 0xffffffff );
	
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}

void Execute::LBU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LBU || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif
	
	// LBU rt, offset(base)
	
	u32 LoadAddress;
	
	// set load to happen after delay slot
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	
	// ***todo*** perform load of unsigned byte
	r->GPR [ i.Rt ].uq0 = (u8) r->Bus->Read ( LoadAddress, 0xff );
	
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}

void Execute::LHU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LHU || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif
	
	// LHU rt, offset(base)
	
	u32 LoadAddress;
	
	// set load to happen after delay slot
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	// *** testing *** alert if load is from unaligned address
	if ( LoadAddress & 0x1 )
	{
		cout << "\nhps1x64 ALERT: LoadAddress is unaligned for LHU @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
		return;
	}
	
	
	// ***todo*** perform unsigned load of halfword
	r->GPR [ i.Rt ].uq0 = (u16) r->Bus->Read ( LoadAddress, 0xffff );
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}




// load instructions without load-delay slot
void Execute::LWL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LWL || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif

	// LWL rt, offset(base)
	
	u32 LoadAddress;
	u32 Value, Temp;
	
	// set load to happen after delay slot
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	
	// ***todo*** perform load LWL
	Value = r->Bus->Read ( LoadAddress & ~3, 0xffffffff );
	
	Value <<= ( ( 3 - ( LoadAddress & 3 ) ) << 3 );
	Temp = r->GPR [ i.Rt ].uw0;
	Temp <<= ( ( ( LoadAddress & 3 ) + 1 ) << 3 );
	if ( ( LoadAddress & 3 ) == 3 ) Temp = 0;
	Temp >>= ( ( ( LoadAddress & 3 ) + 1 ) << 3 );
	r->GPR [ i.Rt ].sq0 = (s32) ( Value | Temp );
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}

void Execute::LWR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LWR || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif

	// LWR rt, offset(base)
	
	u32 LoadAddress;
	u32 Value, Temp;
	
	// set load to happen after delay slot
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	
	// ***todo*** perform load LWR
	Value = r->Bus->Read ( LoadAddress & ~3, 0xffffffff );
	
	Value >>= ( ( LoadAddress & 3 ) << 3 );
	Temp = r->GPR [ i.Rt ].uw0;
	Temp >>= ( ( 4 - ( LoadAddress & 3 ) ) << 3 );
	if ( ( LoadAddress & 3 ) == 0 ) Temp = 0;
	Temp <<= ( ( 4 - ( LoadAddress & 3 ) ) << 3 );
	
	// note: LWR is only sign extended when the full memory value is loaded, which is when ( LoadAddress & 3 ) == 0
	if ( LoadAddress & 3 )
	{
		// NOT sign extended //
		r->GPR [ i.Rt ].uw0 = Value | Temp;
	}
	else
	{
		// sign extended //
		r->GPR [ i.Rt ].sq0 = (s32) ( Value | Temp );
	}
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}






//// ***** R5900 INSTRUCTIONS ***** ////

// arithemetic instructions //

static void Execute::DADD ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DADD || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	s64 temp;
	
	temp = r->GPR [ i.Rs ].s + r->GPR [ i.Rt ].s;

	// ***todo*** implement overflow exception
	// note: this is similar to something seen in MAME and adapted from something in pcsx2
	if ( ( ( ~( r->GPR [ i.Rs ].s ^ r->GPR [ i.Rt ].s ) ) & ( r->GPR [ i.Rs ].s ^ temp ) ) < 0 )
	{
		// overflow
		cout << "\nhps2x64: Execute::DADD generated an overflow exception @ Cycle#" << dec << r->CycleCount << " PC=" << hex << r->PC << "\n";
		r->ProcessSynchronousInterrupt ( Cpu::EXC_OV );
		
#if defined INLINE_DEBUG_DADD || defined INLINE_DEBUG_R5900
		debug << ";  INT";
#endif
	}
	else
	{
		// it's cool - we can do the add and store the result to register
		// sign-extend for R5900
		r->GPR [ i.Rd ].s = temp;
	}
	
	// add WITH overflow exception: rd = rs + rt
	//r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u + r->GPR [ i.Rt ].u;
	
#if defined INLINE_DEBUG_DADD || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::DADDI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DADDI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	s64 temp;
	
	// ***todo*** implement overflow exception
	temp = r->GPR [ i.Rs ].s + ( (s64) i.sImmediate );
	
	// ***todo*** implement overflow exception
	// note: this is similar to something seen in MAME and adapted from something in pcsx2
	if ( ( ( ~( r->GPR [ i.Rs ].s ^ ( (s64) i.sImmediate ) ) ) & ( r->GPR [ i.Rs ].s ^ temp ) ) < 0 )
	{
		// overflow
		cout << "\nhps2x64: Execute::DADDI generated an overflow exception @ Cycle#" << dec << r->CycleCount << " PC=" << hex << r->PC << "\n";
		r->ProcessSynchronousInterrupt ( Cpu::EXC_OV );
		
#if defined INLINE_DEBUG_DADDI || defined INLINE_DEBUG_R5900
		debug << ";  INT";
#endif
	}
	else
	{
		// it's cool - we can do the add and store the result to register
		// sign-extend for R5900
		r->GPR [ i.Rt ].s = temp;
	}
	
	// add immedeate WITH overflow exception: rt = rs + immediate
	//r->GPR [ i.Rt ].s = r->GPR [ i.Rs ].s + ( (s64) i.sImmediate );
	
#if defined INLINE_DEBUG_DADD || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

static void Execute::DADDU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DADDU || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// add without overflow exception: rd = rs + rt
	r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u + r->GPR [ i.Rt ].u;
	
#if defined INLINE_DEBUG_DADDU || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::DADDIU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DADDIU || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u;
#endif

	// *note* immediate value is sign-extended before the addition is performed

	// add immedeate without overflow exception: rt = rs + immediate
	r->GPR [ i.Rt ].s = r->GPR [ i.Rs ].s + ( (s64) i.sImmediate );
	
#if defined INLINE_DEBUG_DADDIU || defined INLINE_DEBUG_R5900
	debug << "; Output: rt = " << r->GPR [ i.Rt ].u;
#endif
}

static void Execute::DSUB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DSUB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	s64 temp;

	temp = r->GPR [ i.Rs ].s - r->GPR [ i.Rt ].s;
	
	// ***todo*** implement overflow exception
	if ( ( ( r->GPR [ i.Rs ].s ^ r->GPR [ i.Rt ].s ) & ( r->GPR [ i.Rs ].s ^ temp ) ) < 0 )
	{
		// overflow
		cout << "\nhps2x64: Execute::DSUB generated an overflow exception @ Cycle#" << dec << r->CycleCount << " PC=" << hex << r->PC << "\n";
		r->ProcessSynchronousInterrupt ( Cpu::EXC_OV );
		
#if defined INLINE_DEBUG_DSUB || defined INLINE_DEBUG_R5900
		debug << ";  INT";
#endif
	}
	else
	{
		// it's cool - we can do the sub and store the result to register
		r->GPR [ i.Rd ].s = temp;
	}
	
	// subtract WITH overflow exception: rd = rs - rt
	//r->GPR [ i.Rd ].s = r->GPR [ i.Rs ].u - r->GPR [ i.Rt ].u;
	
#if defined INLINE_DEBUG_DSUB || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::DSUBU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DSUBU || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].u << "; rt = " << r->GPR [ i.Rt ].u;
#endif

	// subtract without overflow exception: rd = rs - rt
	r->GPR [ i.Rd ].s = r->GPR [ i.Rs ].u - r->GPR [ i.Rt ].u;
	
#if defined INLINE_DEBUG_DSUBU || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::DSLL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DSLL || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// shift left logical: rd = rt << shift
	r->GPR [ i.Rd ].u = ( r->GPR [ i.Rt ].u << i.Shift );
	
#if defined INLINE_DEBUG_DSLL || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::DSLL32 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DSLL32 || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// shift left logical: rd = rt << shift
	r->GPR [ i.Rd ].u = ( r->GPR [ i.Rt ].u << ( i.Shift + 32 ) );
	
#if defined INLINE_DEBUG_DSLL32 || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::DSLLV ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DSLLV || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// shift left logical variable: rd = rt << rs
	r->GPR [ i.Rd ].s = r->GPR [ i.Rt ].u << ( r->GPR [ i.Rs ].u & 0x3f );
	
#if defined INLINE_DEBUG_DSLLV || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::DSRA ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DSRA || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// shift right arithmetic: rd = rt >> shift
	r->GPR [ i.Rd ].s = ( r->GPR [ i.Rt ].s >> i.Shift );
	
#if defined INLINE_DEBUG_DSRA || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::DSRA32 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DSRA32 || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// shift right arithmetic: rd = rt >> shift
	r->GPR [ i.Rd ].s = ( r->GPR [ i.Rt ].s >> ( i.Shift + 32 ) );
	
#if defined INLINE_DEBUG_DSRA32 || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::DSRAV ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DSRAV || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// shift right arithmetic variable: rd = rt >> rs
	r->GPR [ i.Rd ].s = ( r->GPR [ i.Rt ].s >> ( r->GPR [ i.Rs ].u & 0x3f ) );
	
#if defined INLINE_DEBUG_DSRAV || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::DSRL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DSRL || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// shift right logical: rd = rt >> shift
	r->GPR [ i.Rd ].s = ( r->GPR [ i.Rt ].u >> i.Shift );
	
#if defined INLINE_DEBUG_DSRL || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::DSRL32 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DSRL32 || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u;
#endif
	
	// shift right logical: rd = rt >> shift
	r->GPR [ i.Rd ].s = ( r->GPR [ i.Rt ].u >> ( i.Shift + 32 ) );
	
#if defined INLINE_DEBUG_DSRL32 || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::DSRLV ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DSRLV || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; rs = " << r->GPR [ i.Rs ].u;
#endif
	
	// shift right logical variable: rd = rt >> rs
	r->GPR [ i.Rd ].s = ( r->GPR [ i.Rt ].u >> ( r->GPR [ i.Rs ].u & 0x3f ) );
	
#if defined INLINE_DEBUG_DSRLV || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}


static void Execute::MULT1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MULT1 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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
	//r->HiLo.sValue = ((s64) (r->GPR [ i.Rs ].s)) * ((s64) (r->GPR [ i.Rt ].s));
	s64 temp;
	temp = ( (s64) ( r->GPR [ i.Rs ].sw0 ) ) * ( (s64) ( r->GPR [ i.Rt ].sw0 ) );
	r->LO.sq1 = (s32) temp;
	r->HI.sq1 = (s32) ( temp >> 32 );
	
	// R5900 can additionally write to register
	if ( i.Rd )
	{
		r->GPR [ i.Rd ].sq0 = (s32) temp;
	}

#if defined INLINE_DEBUG_MULT1 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "; Output: LO1=" << r->LO.sq1 << "; HI1=" << r->HI.sq1 << "; rd=" << r->GPR [ i.Rd ].sq0;
#endif

}

static void Execute::MULTU1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MULTU1 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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
	//r->HiLo.uValue = ((u64) (r->GPR [ i.Rs ].u)) * ((u64) (r->GPR [ i.Rt ].u));
	u64 temp;
	temp = ( (u64) ( r->GPR [ i.Rs ].uw0 ) ) * ( (u64) ( r->GPR [ i.Rt ].uw0 ) );
	r->LO.sq1 = (s32) temp;
	r->HI.sq1 = (s32) ( temp >> 32 );
	
	// R5900 can additionally write to register
	if ( i.Rd )
	{
		r->GPR [ i.Rd ].sq0 = (s32) temp;
	}
	
#if defined INLINE_DEBUG_MULTU1 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "; Output: LO1=" << r->LO.sq1 << "; HI1=" << r->HI.sq1 << "; rd=" << r->GPR [ i.Rd ].sq0;
#endif

}

static void Execute::DIV1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DIV1 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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
	//if ( r->GPR [ i.Rt ].u != 0 )
	if ( r->GPR [ i.Rt ].uw0 != 0 )
	{
		// if rs = 0x80000000 and rt = -1 then hi = 0 and lo = 0x80000000
		if ( r->GPR [ i.Rs ].uw0 == 0x80000000 && r->GPR [ i.Rt ].sw0 == -1 )
		{
			// *todo* check if all this stuff is sign extended during testing
			//r->HiLo.uHi = 0;
			//r->HiLo.uLo = 0x80000000;
			r->HI.sq1 = 0;
			r->LO.sq1 = (s32) 0x80000000;
		}
		else
		{
			// *todo* check during testing if signs are switched on modulus result when sign of dividend and divisor are different
			//r->HiLo.sLo = r->GPR [ i.Rs ].s / r->GPR [ i.Rt ].s;
			//r->HiLo.sHi = r->GPR [ i.Rs ].s % r->GPR [ i.Rt ].s;
			r->LO.sq1 = r->GPR [ i.Rs ].sw0 / r->GPR [ i.Rt ].sw0;
			r->HI.sq1 = r->GPR [ i.Rs ].sw0 % r->GPR [ i.Rt ].sw0;
		}
	}
	else
	{
		if ( r->GPR [ i.Rs ].s < 0 )
		{
			//r->HiLo.sLo = 1;
			r->LO.sq1 = 1;
		}
		else
		{
			//r->HiLo.sLo = -1;
			r->LO.sq1 = -1;
		}
		
		//r->HiLo.uHi = r->GPR [ i.Rs ].u;
		r->HI.sq1 = r->GPR [ i.Rs ].sw0;
	}
	
#if defined INLINE_DEBUG_DIV1 || defined INLINE_DEBUG_R5900
	debug << "; Output: LO1 = " << r->LO.sq1 << "; HI1 = " << r->HI.sq1;
#endif
}

static void Execute::DIVU1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DIVU1 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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

	// divide unsigned: Lo = rs / rt; Hi = rs % rt
	//if ( r->GPR [ i.Rt ].u != 0 )
	if ( r->GPR [ i.Rt ].uw0 != 0 )
	{
		//r->HiLo.uLo = r->GPR [ i.Rs ].u / r->GPR [ i.Rt ].u;
		//r->HiLo.uHi = r->GPR [ i.Rs ].u % r->GPR [ i.Rt ].u;
		r->LO.sq1 = (s32) ( r->GPR [ i.Rs ].uw0 / r->GPR [ i.Rt ].uw0 );
		r->HI.sq1 = (s32) ( r->GPR [ i.Rs ].uw0 % r->GPR [ i.Rt ].uw0 );
	}
	else
	{
		//r->HiLo.sLo = -1;
		//r->HiLo.uHi = r->GPR [ i.Rs ].u;
		r->LO.sq1 = -1;
		r->HI.sq1 = r->GPR [ i.Rs ].sw0;
	}
	
#if defined INLINE_DEBUG_DIVU1 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "; Output: LO1 = " << r->LO.sq1 << "; HI1 = " << r->HI.sq1;
#endif
}

static void Execute::MADD ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MADD || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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
	//r->HiLo.sValue = ((s64) (r->GPR [ i.Rs ].s)) * ((s64) (r->GPR [ i.Rt ].s));
	s64 temp, lltemp2;
	temp = ( (s64) ( r->GPR [ i.Rs ].sw0 ) ) * ( (s64) ( r->GPR [ i.Rt ].sw0 ) );
	
	// also add in hi,lo
	lltemp2 = ( (u64) r->LO.uw0 ) | ( ( (u64) r->HI.uw0 ) << 32 );
	temp += lltemp2;
	
	r->LO.sq0 = (s32) temp;
	r->HI.sq0 = (s32) ( temp >> 32 );
	
	// R5900 can additionally write to register
	if ( i.Rd )
	{
		r->GPR [ i.Rd ].sq0 = (s32) temp;
	}
}

static void Execute::MADD1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MADD1 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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
	//r->HiLo.sValue = ((s64) (r->GPR [ i.Rs ].s)) * ((s64) (r->GPR [ i.Rt ].s));
	s64 temp, lltemp2;
	temp = ( (s64) ( r->GPR [ i.Rs ].sw0 ) ) * ( (s64) ( r->GPR [ i.Rt ].sw0 ) );
	
	// also add in hi,lo
	lltemp2 = ( (u64) r->LO.uw2 ) | ( ( (u64) r->HI.uw2 ) << 32 );
	temp += lltemp2;
	
	r->LO.sq1 = (s32) temp;
	r->HI.sq1 = (s32) ( temp >> 32 );
	
	// R5900 can additionally write to register
	if ( i.Rd )
	{
		r->GPR [ i.Rd ].sq0 = (s32) temp;
	}
}

static void Execute::MADDU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDU || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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
	//r->HiLo.uValue = ((u64) (r->GPR [ i.Rs ].u)) * ((u64) (r->GPR [ i.Rt ].u));
	u64 temp, ulltemp2;
	temp = ( (u64) ( r->GPR [ i.Rs ].uw0 ) ) * ( (u64) ( r->GPR [ i.Rt ].uw0 ) );
	
	// also add in hi,lo
	ulltemp2 = ( (u64) r->LO.uw0 ) | ( ( (u64) r->HI.uw0 ) << 32 );
	temp += ulltemp2;
	
	r->LO.sq0 = (s32) temp;
	r->HI.sq0 = (s32) ( temp >> 32 );
	
	// R5900 can additionally write to register
	if ( i.Rd )
	{
		r->GPR [ i.Rd ].sq0 = (s32) temp;
	}
	
#if defined INLINE_DEBUG_MULTU || defined INLINE_DEBUG_R5900
	debug << "; Output: LO=" << r->LO.s << "; HI=" << r->HI.s << "; rd=" << r->GPR [ i.Rd ].s;
#endif
}

static void Execute::MADDU1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDU1 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
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
	//r->HiLo.uValue = ((u64) (r->GPR [ i.Rs ].u)) * ((u64) (r->GPR [ i.Rt ].u));
	u64 temp, ulltemp2;
	temp = ( (u64) ( r->GPR [ i.Rs ].uw0 ) ) * ( (u64) ( r->GPR [ i.Rt ].uw0 ) );
	
	// also add in hi,lo
	ulltemp2 = ( (u64) r->LO.uw2 ) | ( ( (u64) r->HI.uw2 ) << 32 );
	temp += ulltemp2;
	
	r->LO.sq1 = (s32) temp;
	r->HI.sq1 = (s32) ( temp >> 32 );
	
	// R5900 can additionally write to register
	if ( i.Rd )
	{
		r->GPR [ i.Rd ].sq0 = (s32) temp;
	}
	
#if defined INLINE_DEBUG_MULTU || defined INLINE_DEBUG_R5900
	debug << "; Output: LO=" << r->LO.s << "; HI=" << r->HI.s << "; rd=" << r->GPR [ i.Rd ].s;
#endif
}



// Load/Store instructions //

static void Execute::SD ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SD || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->GPR [ i.Rt ].u << "; base = " << r->GPR [ i.Base ].u;
#endif
	
	// SW rt, offset(base)
	u32 StoreAddress;
	
	// check if storing to data cache
	//StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	StoreAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	// *** testing *** alert if load is from unaligned address
	if ( StoreAddress & 0x7 )
	{
		cout << "\nhps1x64 ALERT: StoreAddress is unaligned for SD @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADES );
		return;
	}
	
	// clear top 3 bits since there is no data cache for caching stores
	// don't clear top 3 bits since scratchpad is at 0x70000000
	//StoreAddress &= 0x1fffffff;
	
	
	// ***todo*** perform store of word
	r->Bus->Write ( StoreAddress, r->GPR [ i.Rt ].uq0, 0xffffffffffffffffULL );
	
	

	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->GPR [ i.Rt ].uw0 )
#endif
}

static void Execute::LD ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LD || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif
	
	// LW rt, offset(base)
	
	u32 LoadAddress;
	
	// set load to happen after delay slot
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	// *** testing *** alert if load is from unaligned address
	if ( LoadAddress & 0x7 )
	{
		cout << "\nhps1x64 ALERT: LoadAddress is unaligned for LD @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
		return;
	}
	
	
	// ***todo*** perform signed load of word
	r->GPR [ i.Rt ].uq0 = r->Bus->Read ( LoadAddress, 0xffffffffffffffffULL );
	
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}

static void Execute::LWU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LWU || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif
	
	// LW rt, offset(base)
	
	u32 LoadAddress;
	
	// set load to happen after delay slot
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	// *** testing *** alert if load is from unaligned address
	if ( LoadAddress & 0x3 )
	{
		cout << "\nhps1x64 ALERT: LoadAddress is unaligned for LW @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
		return;
	}
	
	
	// ***todo*** perform signed load of word
	r->GPR [ i.Rt ].uq0 = (u32) r->Bus->Read ( LoadAddress, 0xffffffff );
	
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}

static void Execute::SDL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SDL || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	static const u32 c_Mask = 0xffffffff;
	u64 Type, Offset;

	// SDL rt, offset(base)
	
	// step 1: check if storing to data cache
	//u32 StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	u32 StoreAddress = r->GPR [ i.Base ].sw0 + i.sOffset;

	// clear top 3 bits since there is no data cache for caching stores
	//StoreAddress &= 0x1fffffff;
	
	
	// ***todo*** perform store SDL
	r->Bus->Write ( StoreAddress & ~7, r->GPR [ i.Rt ].uq0 >> ( ( 7 - ( StoreAddress & 7 ) ) << 3 ), 0xffffffffffffffffULL >> ( ( 7 - ( StoreAddress & 7 ) ) << 3 ) );
	

	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;

}

static void Execute::SDR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SDR || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	static const u32 c_Mask = 0xffffffff;
	u32 Type, Offset;
	
	// SDR rt, offset(base)
	
	// step 1: check if storing to data cache
	//u32 StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	u32 StoreAddress = r->GPR [ i.Base ].sw0 + i.sOffset;

	// clear top 3 bits since there is no data cache for caching stores
	//StoreAddress &= 0x1fffffff;
	
	
	// ***todo*** perform store SWR
	r->Bus->Write ( StoreAddress & ~7, r->GPR [ i.Rt ].uq0 << ( ( StoreAddress & 7 ) << 3 ), 0xffffffffffffffffULL << ( ( StoreAddress & 7 ) << 3 ) );
	
	
	
	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
}

static void Execute::LDL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LDL || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// LDL rt, offset(base)
	
	u32 LoadAddress;
	u64 Value, Temp;
	
	// set load to happen after delay slot
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	
	// ***todo*** perform load LDL
	Value = r->Bus->Read ( LoadAddress & ~7, 0xffffffffffffffffULL );
	
	Value <<= ( ( 7 - ( LoadAddress & 7 ) ) << 3 );
	Temp = r->GPR [ i.Rt ].uq0;
	Temp <<= ( ( ( LoadAddress & 7 ) + 1 ) << 3 );
	if ( ( LoadAddress & 7 ) == 7 ) Temp = 0;
	Temp >>= ( ( ( LoadAddress & 7 ) + 1 ) << 3 );
	r->GPR [ i.Rt ].sq0 = Value | Temp;
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}

static void Execute::LDR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LDR || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// LDR rt, offset(base)
	
	u32 LoadAddress;
	u64 Value, Temp;
	
	// set load to happen after delay slot
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	
	// ***todo*** perform load LWR
	Value = r->Bus->Read ( LoadAddress & ~7, 0xffffffffffffffffULL );
	
	Value >>= ( ( LoadAddress & 7 ) << 3 );
	Temp = r->GPR [ i.Rt ].uq0;
	Temp >>= ( ( 8 - ( LoadAddress & 7 ) ) << 3 );
	if ( ( LoadAddress & 7 ) == 0 ) Temp = 0;
	Temp <<= ( ( 8 - ( LoadAddress & 7 ) ) << 3 );
	r->GPR [ i.Rt ].sq0 = Value | Temp;
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
}

static void Execute::LQ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LQ || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// LQ rt, offset(base)
	
	u32 LoadAddress;
	u64* Data;
	
	// set load to happen after delay slot
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	// *** testing *** alert if load is from unaligned address
	// this one actually does NOT have any address errors due to alignment, as it pretends the bottom 4-bits are zero on R5900
	//if ( LoadAddress & 0xf )
	//{
	//	cout << "\nhps2x64 ALERT: LoadAddress is unaligned for LW @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
	//	
	//	// *** testing ***
	//	r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
	//	return;
	//}
	
	// bottom four bits of Address are cleared
	LoadAddress &= ~0xf;
	
	// ***todo*** perform signed load of word
	//Data = (u64*) r->Bus->Read ( LoadAddress, 0 );
	//r->GPR [ i.Rt ].uLo = Data [ 1 ];
	//r->GPR [ i.Rt ].uHi = Data [ 0 ];
	
	
	Data = (u64*) r->Bus->Read ( LoadAddress, 0 );
	
	r->GPR [ i.Rt ].uLo = Data [ 0 ];
	r->GPR [ i.Rt ].uHi = Data [ 1 ];
	
	//r->GPR [ i.Rt ].uw3 = ((u32*)Data) [ 0 ];
	//r->GPR [ i.Rt ].uw2 = ((u32*)Data) [ 1 ];
	//r->GPR [ i.Rt ].uw1 = ((u32*)Data) [ 2 ];
	//r->GPR [ i.Rt ].uw0 = ((u32*)Data) [ 3 ];

	//r->GPR [ i.Rt ].uw0 = ((u32*)Data) [ 0 ];
	//r->GPR [ i.Rt ].uw1 = ((u32*)Data) [ 1 ];
	//r->GPR [ i.Rt ].uw2 = ((u32*)Data) [ 2 ];
	//r->GPR [ i.Rt ].uw3 = ((u32*)Data) [ 3 ];
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;

}

static void Execute::SQ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SQ || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// SQ rt, offset(base)
	u32 StoreAddress;
	
	// check if storing to data cache
	//StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	StoreAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	// *** testing *** alert if load is from unaligned address
	// this one actually does NOT have any address errors due to alignment, as it pretends the bottom 4-bits are zero on R5900
	//if ( StoreAddress & 0xf )
	//{
	//	cout << "\nhps2x64 ALERT: StoreAddress is unaligned for SW @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
	//	
	//	// *** testing ***
	//	r->ProcessSynchronousInterrupt ( Cpu::EXC_ADES );
	//	return;
	//}
	
	// clear top 3 bits since there is no data cache for caching stores
	// don't clear top 3 bits since scratchpad is at 0x70000000
	//StoreAddress &= 0x1fffffff;
	
	// bottom four bits of Address are cleared
	StoreAddress &= ~0xf;
	
	// ***todo*** perform store of word
	// *note* probably want to pass a pointer to hi part, since that is in lower area of memory
	r->Bus->Write ( StoreAddress, & ( r->GPR [ i.Rt ].uw0 ), 0 );
	//r->Bus->Write ( StoreAddress, & ( r->GPR [ i.Rt ].uHi ), 0 );
	
	

	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
}


static void Execute::MOVZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MOVZ || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rd=" << r->GPR [ i.Rd ].uq0 << "; rs=" << r->GPR [ i.Rs ].uq0 << "; rt=" << r->GPR [ i.Rt ].uq0;
#endif

	// movz rd, rs, rt
	// if ( rt == 0 ) rd = rs
	
	if ( !r->GPR [ i.Rt ].uq0 )
	{
		r->GPR [ i.Rd ].uq0 = r->GPR [ i.Rs ].uq0;
	}
	
#if defined INLINE_DEBUG_MOVZ || defined INLINE_DEBUG_R5900
	debug << hex << "; Output: rd=" << r->GPR [ i.Rd ].uq0;
#endif
}

static void Execute::MOVN ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MOVN || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rd=" << r->GPR [ i.Rd ].uq0 << "; rs=" << r->GPR [ i.Rs ].uq0 << "; rt=" << r->GPR [ i.Rt ].uq0;
#endif

	// movn rd, rs, rt
	// if ( rt != 0 ) rd = rs

	if ( r->GPR [ i.Rt ].uq0 )
	{
		r->GPR [ i.Rd ].uq0 = r->GPR [ i.Rs ].uq0;
	}
	
#if defined INLINE_DEBUG_MOVN || defined INLINE_DEBUG_R5900
	debug << hex << "; Output: rd=" << r->GPR [ i.Rd ].uq0;
#endif
}


static void Execute::MFHI1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MFHI1 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: HI1 = " << r->HI.uq1;
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
	r->GPR [ i.Rd ].uq0 = r->HI.uq1;
	
#if defined INLINE_DEBUG_MFHI || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::MTHI1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MTHI1 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	r->HI.uq1 = r->GPR [ i.Rs ].uq0;
}

static void Execute::MFLO1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MFLO1 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: LO1 = " << r->LO.uq1;
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
	r->GPR [ i.Rd ].uq0 = r->LO.uq1;
	
#if defined INLINE_DEBUG_MFLO || defined INLINE_DEBUG_R5900
	debug << "; Output: rd = " << r->GPR [ i.Rd ].u;
#endif
}

static void Execute::MTLO1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MTLO1 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	r->LO.uq1 = r->GPR [ i.Rs ].uq0;
}



static void Execute::MFSA ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MFSA || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: SA = " << r->SA;
#endif

	// MFSA rd
	// only operates in instruction pipeline 0
	r->GPR [ i.Rd ].uq0 = r->SA;

#if defined INLINE_DEBUG_MFSA || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "; Output: rd = " << r->GPR [ i.Rd ].uq0;
#endif
}

static void Execute::MTSA ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MTSA || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].uq0;
#endif

	// MTSA rs
	// only operates in instruction pipeline 0
	r->SA = r->GPR [ i.Rs ].uq0;
	
#if defined INLINE_DEBUG_MTSA || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "; Output: SA = " << r->SA;
#endif
}

static void Execute::MTSAB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MTSAB || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].uq0;
#endif

	// MTSAB rs, immediate
	// only operates in instruction pipeline 0
	// or does this not shift left 3 ??
	//r->SA = ( ( ( r->GPR [ i.Rs ].uw0 ) ^ ( i.uImmediate ) ) & 0xf ) << 3;
	r->SA = ( ( ( r->GPR [ i.Rs ].uw0 ) ^ ( i.uImmediate ) ) & 0xf );
	
#if defined INLINE_DEBUG_MTSAB || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "; Output: SA = " << r->SA;
#endif
}

static void Execute::MTSAH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MTSAH || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rs = " << r->GPR [ i.Rs ].uq0;
#endif

	// MTSAH rs, immediate
	// only operates in instruction pipeline 0
	// or does this shift left only one instead ??
	//r->SA = ( ( ( r->GPR [ i.Rs ].uw0 ) ^ ( i.uImmediate ) ) & 0x7 ) << 4;
	r->SA = ( ( ( r->GPR [ i.Rs ].uw0 ) ^ ( i.uImmediate ) ) & 0x7 ) << 1;
	
#if defined INLINE_DEBUG_MTSAH || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "; Output: SA = " << r->SA;
#endif
}




// Branch instructions //

static void Execute::BEQL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BEQL || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( r->GPR [ i.Rs ].u == r->GPR [ i.Rt ].u )
	{
		// taking branch (after delay slot of course) //
		
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BEQ || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}
}

static void Execute::BNEL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BNEL || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( r->GPR [ i.Rs ].u != r->GPR [ i.Rt ].u )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BNE || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}
}

static void Execute::BGEZL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BGEZL || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( r->GPR [ i.Rs ].s >= 0 )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BGEZ || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}
}

static void Execute::BLEZL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BLEZL || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( r->GPR [ i.Rs ].s <= 0 )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BLEZ || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}
}

static void Execute::BGTZL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BGTZL || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( r->GPR [ i.Rs ].s > 0 )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BGTZ || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}
}

static void Execute::BLTZL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BLTZL || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( r->GPR [ i.Rs ].s < 0 )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BLTZ || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}
}



static void Execute::BLTZALL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BLTZALL || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( r->GPR [ i.Rs ].s < 0 )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BLTZAL || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}
	
	////////////////////////////////////////////////////////////////////////
	// Store return address when instruction is executed in r31
	// for this instruction this happens whether branch is taken or not
	// *note* this must happen AFTER comparison check
	r->GPR [ 31 ].u = r->PC + 8;
}

static void Execute::BGEZALL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BGEZALL || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( r->GPR [ i.Rs ].s >= 0 )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BGEZAL || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}

	////////////////////////////////////////////////////////////////////////
	// Store return address when instruction is executed in r31
	// for this instruction this happens whether branch is taken or not
	// *note* this must happen AFTER comparison check
	r->GPR [ 31 ].u = r->PC + 8;
}




static void Execute::BC0T ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BC0T || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::BC0TL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BC0TL || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::BC0F ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BC0F || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::BC0FL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BC0FL || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::BC1T ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BC1T || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: FCR31 = " << r->CPC1 [ 31 ];
#endif

	if ( r->CPC1 [ 31 ] & 0x00800000 )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BC1T || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
}

static void Execute::BC1TL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BC1TL || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: FCR31 = " << r->CPC1 [ 31 ];
#endif

	if ( r->CPC1 [ 31 ] & 0x00800000 )
	{
		// taking branch (after delay slot of course) //
		
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BC1TL || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}
}

static void Execute::BC1F ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BC1F || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: FCR31 = " << r->CPC1 [ 31 ];
#endif

	if ( ! ( r->CPC1 [ 31 ] & 0x00800000 ) )
	{
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BC1F || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
}

static void Execute::BC1FL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BC1FL || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: FCR31 = " << r->CPC1 [ 31 ];
#endif

	if ( ! ( r->CPC1 [ 31 ] & 0x00800000 ) )
	{
		// taking branch (after delay slot of course) //
		
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BC1FL || defined INLINE_DEBUG_R5900
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}
}

static void Execute::BC2T ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BC2T || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// branch if vu1 is running
	if ( VU1::_VU1->Running )
	{
		// taking branch (after delay slot of course) //
		
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BC2T || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}
}

static void Execute::BC2TL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BC2TL || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// branch if vu1 is running
	if ( VU1::_VU1->Running )
	{
		// taking branch (after delay slot of course) //
		
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BC2TL || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}
}

static void Execute::BC2F ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BC2F || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// branch if vu1 is NOT running
	if ( !VU1::_VU1->Running )
	{
		// taking branch (after delay slot of course) //
		
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BC2F || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}
}

static void Execute::BC2FL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_BC2FL || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// branch if vu1 is NOT running
	if ( !VU1::_VU1->Running )
	{
		// taking branch (after delay slot of course) //
		
		// next instruction is in the branch delay slot
		Cpu::DelaySlot *d = & ( r->DelaySlots [ r->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		d->cb = r->_cb_Branch;
		r->Status.DelaySlot_Valid |= 0x2;
		
#if defined INLINE_DEBUG_BC2FL || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
		debug << ";  WILL TAKE";
#endif
	}
	else
	{
		// NOT branching (and skips next instruction) //
		
		// skip next instruction
		r->NextPC = r->PC + 8;
		
		// *todo* add an additional cycle for the skipped instruction
		//CycleCount++;
	}
}






static void Execute::TGEI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TGEI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// trap if rs >= signed imm
	if ( r->GPR [ i.Rs ].sq0 >= (s64) i.sImmediate )
	{
		r->ProcessSynchronousInterrupt ( Cpu::EXC_TRAP );
	}
}

static void Execute::TGEIU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TGEIU || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// trap if rs >= unsigned imm
	if ( r->GPR [ i.Rs ].uq0 >= (u64) i.uImmediate )
	{
		r->ProcessSynchronousInterrupt ( Cpu::EXC_TRAP );
	}
}

static void Execute::TLTI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TLTI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// trap if rs < signed imm
	if ( r->GPR [ i.Rs ].sq0 < (s64) i.sImmediate )
	{
		r->ProcessSynchronousInterrupt ( Cpu::EXC_TRAP );
	}
}

static void Execute::TLTIU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TLTIU || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// trap if rs < unsigned imm
	if ( r->GPR [ i.Rs ].uq0 < (u64) i.uImmediate )
	{
		r->ProcessSynchronousInterrupt ( Cpu::EXC_TRAP );
	}
}

static void Execute::TEQI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TEQI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// trap if rs == signed imm
	if ( r->GPR [ i.Rs ].sq0 == (s64) i.sImmediate )
	{
		r->ProcessSynchronousInterrupt ( Cpu::EXC_TRAP );
	}
}

static void Execute::TNEI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TNEI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// trap if rs != signed imm
	if ( r->GPR [ i.Rs ].sq0 != (s64) i.sImmediate )
	{
		r->ProcessSynchronousInterrupt ( Cpu::EXC_TRAP );
	}
}


static void Execute::TGE ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TGE || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// trap if rs >= rt
	if ( r->GPR [ i.Rs ].sq0 >= r->GPR [ i.Rt ].sq0 )
	{
		r->ProcessSynchronousInterrupt ( Cpu::EXC_TRAP );
	}
}

static void Execute::TGEU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TGEU || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// trap if rs >= rt
	if ( r->GPR [ i.Rs ].uq0 >= r->GPR [ i.Rt ].uq0 )
	{
		r->ProcessSynchronousInterrupt ( Cpu::EXC_TRAP );
	}
}

static void Execute::TLT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TLT || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// trap if rs < rt
	if ( r->GPR [ i.Rs ].sq0 < r->GPR [ i.Rt ].sq0 )
	{
		r->ProcessSynchronousInterrupt ( Cpu::EXC_TRAP );
	}
}

static void Execute::TLTU ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TLTU || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// trap if rs < rt
	if ( r->GPR [ i.Rs ].uq0 < r->GPR [ i.Rt ].uq0 )
	{
		r->ProcessSynchronousInterrupt ( Cpu::EXC_TRAP );
	}
}

static void Execute::TEQ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TEQ || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// trap if rs == rt
	if ( r->GPR [ i.Rs ].uq0 == r->GPR [ i.Rt ].uq0 )
	{
		r->ProcessSynchronousInterrupt ( Cpu::EXC_TRAP );
	}
}

static void Execute::TNE ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TNE || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// trap if rs != rt
	if ( r->GPR [ i.Rs ].uq0 != r->GPR [ i.Rt ].uq0 )
	{
		r->ProcessSynchronousInterrupt ( Cpu::EXC_TRAP );
	}
}


















// * R5900 Parallel (SIMD) instructions * //


static void Execute::PADSBH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PADSBH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	// first four halfwords subtract, next four add
	r->GPR [ i.Rd ].uh0 = r->GPR [ i.Rs ].uh0 - r->GPR [ i.Rt ].uh0;
	r->GPR [ i.Rd ].uh1 = r->GPR [ i.Rs ].uh1 - r->GPR [ i.Rt ].uh1;
	r->GPR [ i.Rd ].uh2 = r->GPR [ i.Rs ].uh2 - r->GPR [ i.Rt ].uh2;
	r->GPR [ i.Rd ].uh3 = r->GPR [ i.Rs ].uh3 - r->GPR [ i.Rt ].uh3;
	r->GPR [ i.Rd ].uh4 = r->GPR [ i.Rs ].uh4 + r->GPR [ i.Rt ].uh4;
	r->GPR [ i.Rd ].uh5 = r->GPR [ i.Rs ].uh5 + r->GPR [ i.Rt ].uh5;
	r->GPR [ i.Rd ].uh6 = r->GPR [ i.Rs ].uh6 + r->GPR [ i.Rt ].uh6;
	r->GPR [ i.Rd ].uh7 = r->GPR [ i.Rs ].uh7 + r->GPR [ i.Rt ].uh7;
	
#if defined INLINE_DEBUG_PADSBH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PABSH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PABSH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sh0 = ( r->GPR [ i.Rt ].sh0 >= 0 ) ? r->GPR [ i.Rt ].sh0 : -r->GPR [ i.Rt ].sh0;
	r->GPR [ i.Rd ].sh1 = ( r->GPR [ i.Rt ].sh1 >= 0 ) ? r->GPR [ i.Rt ].sh1 : -r->GPR [ i.Rt ].sh1;
	r->GPR [ i.Rd ].sh2 = ( r->GPR [ i.Rt ].sh2 >= 0 ) ? r->GPR [ i.Rt ].sh2 : -r->GPR [ i.Rt ].sh2;
	r->GPR [ i.Rd ].sh3 = ( r->GPR [ i.Rt ].sh3 >= 0 ) ? r->GPR [ i.Rt ].sh3 : -r->GPR [ i.Rt ].sh3;
	r->GPR [ i.Rd ].sh4 = ( r->GPR [ i.Rt ].sh4 >= 0 ) ? r->GPR [ i.Rt ].sh4 : -r->GPR [ i.Rt ].sh4;
	r->GPR [ i.Rd ].sh5 = ( r->GPR [ i.Rt ].sh5 >= 0 ) ? r->GPR [ i.Rt ].sh5 : -r->GPR [ i.Rt ].sh5;
	r->GPR [ i.Rd ].sh6 = ( r->GPR [ i.Rt ].sh6 >= 0 ) ? r->GPR [ i.Rt ].sh6 : -r->GPR [ i.Rt ].sh6;
	r->GPR [ i.Rd ].sh7 = ( r->GPR [ i.Rt ].sh7 >= 0 ) ? r->GPR [ i.Rt ].sh7 : -r->GPR [ i.Rt ].sh7;
	
#if defined INLINE_DEBUG_PABSH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PABSW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PABSW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sw0 = ( r->GPR [ i.Rt ].sw0 >= 0 ) ? r->GPR [ i.Rt ].sw0 : -r->GPR [ i.Rt ].sw0;
	r->GPR [ i.Rd ].sw1 = ( r->GPR [ i.Rt ].sw1 >= 0 ) ? r->GPR [ i.Rt ].sw1 : -r->GPR [ i.Rt ].sw1;
	r->GPR [ i.Rd ].sw2 = ( r->GPR [ i.Rt ].sw2 >= 0 ) ? r->GPR [ i.Rt ].sw2 : -r->GPR [ i.Rt ].sw2;
	r->GPR [ i.Rd ].sw3 = ( r->GPR [ i.Rt ].sw3 >= 0 ) ? r->GPR [ i.Rt ].sw3 : -r->GPR [ i.Rt ].sw3;
	
#if defined INLINE_DEBUG_PABSW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PAND ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PAND || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1 << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uq0 = r->GPR [ i.Rs ].uq0 & r->GPR [ i.Rt ].uq0;
	r->GPR [ i.Rd ].uq1 = r->GPR [ i.Rs ].uq1 & r->GPR [ i.Rt ].uq1;
	
#if defined INLINE_DEBUG_PAND || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PXOR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PXOR || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1 << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uq0 = r->GPR [ i.Rs ].uq0 ^ r->GPR [ i.Rt ].uq0;
	r->GPR [ i.Rd ].uq1 = r->GPR [ i.Rs ].uq1 ^ r->GPR [ i.Rt ].uq1;
	
#if defined INLINE_DEBUG_PXOR || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::POR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_POR || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1 << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uq0 = r->GPR [ i.Rs ].uq0 | r->GPR [ i.Rt ].uq0;
	r->GPR [ i.Rd ].uq1 = r->GPR [ i.Rs ].uq1 | r->GPR [ i.Rt ].uq1;
	
#if defined INLINE_DEBUG_POR || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PNOR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PNOR || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1 << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uq0 = ~( r->GPR [ i.Rs ].uq0 | r->GPR [ i.Rt ].uq0 );
	r->GPR [ i.Rd ].uq1 = ~( r->GPR [ i.Rs ].uq1 | r->GPR [ i.Rt ].uq1 );
	
#if defined INLINE_DEBUG_PNOR || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}


static void Execute::PLZCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PLZCW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
#endif

	if ( r->GPR [ i.Rs ].sw0 < 0 )
	{
		// if negative, it is actually a leading one count
		r->GPR [ i.Rd ].sw0 = CountLeadingZeros32 ( ~r->GPR [ i.Rs ].sw0 ) - 1;
	}
	else
	{
		// is positive, so do a leading zero count //
		r->GPR [ i.Rd ].sw0 = CountLeadingZeros32 ( r->GPR [ i.Rs ].sw0 ) - 1;
	}
	
	if ( r->GPR [ i.Rs ].sw1 < 0 )
	{
		// if negative, it is actually a leading one count
		r->GPR [ i.Rd ].sw1 = CountLeadingZeros32 ( ~r->GPR [ i.Rs ].sw1 ) - 1;
	}
	else
	{
		// is positive, so do a leading zero count //
		r->GPR [ i.Rd ].sw1 = CountLeadingZeros32 ( r->GPR [ i.Rs ].sw1 ) - 1;
	}
	
#if defined INLINE_DEBUG_PLZCW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}


static void Execute::PMFHL_LH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMFHL_LH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " LO=" << r->LO.uq0 << " " << r->LO.uq1;
	debug << hex << " HI=" << r->HI.uq0 << " " << r->HI.uq1;
#endif

	r->GPR [ i.Rd ].uh0 = r->LO.uh0;
	r->GPR [ i.Rd ].uh1 = r->LO.uh2;
	r->GPR [ i.Rd ].uh2 = r->HI.uh0;
	r->GPR [ i.Rd ].uh3 = r->HI.uh2;
	r->GPR [ i.Rd ].uh4 = r->LO.uh4;
	r->GPR [ i.Rd ].uh5 = r->LO.uh6;
	r->GPR [ i.Rd ].uh6 = r->HI.uh4;
	r->GPR [ i.Rd ].uh7 = r->HI.uh6;
	
#if defined INLINE_DEBUG_PMFHL_LH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PMFHL_LW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMFHL_LW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " LO=" << r->LO.uq0 << " " << r->LO.uq1;
	debug << hex << " HI=" << r->HI.uq0 << " " << r->HI.uq1;
#endif

	r->GPR [ i.Rd ].uw0 = r->LO.uw0;
	r->GPR [ i.Rd ].uw1 = r->HI.uw0;
	r->GPR [ i.Rd ].uw2 = r->LO.uw2;
	r->GPR [ i.Rd ].uw3 = r->HI.uw2;
	
#if defined INLINE_DEBUG_PMFHL_LW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PMFHL_UW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMFHL_UW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " LO=" << r->LO.uq0 << " " << r->LO.uq1;
	debug << hex << " HI=" << r->HI.uq0 << " " << r->HI.uq1;
#endif

	r->GPR [ i.Rd ].uw0 = r->LO.uw1;
	r->GPR [ i.Rd ].uw1 = r->HI.uw1;
	r->GPR [ i.Rd ].uw2 = r->LO.uw3;
	r->GPR [ i.Rd ].uw3 = r->HI.uw3;
	
#if defined INLINE_DEBUG_PMFHL_UW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PMTHL_LW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMTHL_LW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
#endif

	r->LO.sw0 = r->GPR [ i.Rs ].sw0;
	r->LO.sw2 = r->GPR [ i.Rs ].sw2;
	r->HI.sw0 = r->GPR [ i.Rs ].sw1;
	r->HI.sw2 = r->GPR [ i.Rs ].sw3;
	
#if defined INLINE_DEBUG_PMTHL_LW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " LO=" << r->LO.uq0 << " " << r->LO.uq1 << " HI=" << r->HI.uq0 << " " << r->HI.uq1;
#endif
}


static void Execute::PMFHL_SH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMFHL_SH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}


static void Execute::PMFHL_SLW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMFHL_SLW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}



static void Execute::PSLLH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSLLH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uh0 = r->GPR [ i.Rt ].uh0 << i.Shift;
	r->GPR [ i.Rd ].uh1 = r->GPR [ i.Rt ].uh1 << i.Shift;
	r->GPR [ i.Rd ].uh2 = r->GPR [ i.Rt ].uh2 << i.Shift;
	r->GPR [ i.Rd ].uh3 = r->GPR [ i.Rt ].uh3 << i.Shift;
	r->GPR [ i.Rd ].uh4 = r->GPR [ i.Rt ].uh4 << i.Shift;
	r->GPR [ i.Rd ].uh5 = r->GPR [ i.Rt ].uh5 << i.Shift;
	r->GPR [ i.Rd ].uh6 = r->GPR [ i.Rt ].uh6 << i.Shift;
	r->GPR [ i.Rd ].uh7 = r->GPR [ i.Rt ].uh7 << i.Shift;
	
#if defined INLINE_DEBUG_PSLLH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PSLLW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSLLW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].ux = r->GPR [ i.Rt ].ux << i.Shift;
	r->GPR [ i.Rd ].uy = r->GPR [ i.Rt ].uy << i.Shift;
	r->GPR [ i.Rd ].uz = r->GPR [ i.Rt ].uz << i.Shift;
	r->GPR [ i.Rd ].uw = r->GPR [ i.Rt ].uw << i.Shift;
	
#if defined INLINE_DEBUG_PSLLW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PSRLH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSRLH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uh0 = r->GPR [ i.Rt ].uh0 >> i.Shift;
	r->GPR [ i.Rd ].uh1 = r->GPR [ i.Rt ].uh1 >> i.Shift;
	r->GPR [ i.Rd ].uh2 = r->GPR [ i.Rt ].uh2 >> i.Shift;
	r->GPR [ i.Rd ].uh3 = r->GPR [ i.Rt ].uh3 >> i.Shift;
	r->GPR [ i.Rd ].uh4 = r->GPR [ i.Rt ].uh4 >> i.Shift;
	r->GPR [ i.Rd ].uh5 = r->GPR [ i.Rt ].uh5 >> i.Shift;
	r->GPR [ i.Rd ].uh6 = r->GPR [ i.Rt ].uh6 >> i.Shift;
	r->GPR [ i.Rd ].uh7 = r->GPR [ i.Rt ].uh7 >> i.Shift;
	
#if defined INLINE_DEBUG_PSRLH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PSRLW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSRLW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].ux = r->GPR [ i.Rt ].ux >> i.Shift;
	r->GPR [ i.Rd ].uy = r->GPR [ i.Rt ].uy >> i.Shift;
	r->GPR [ i.Rd ].uz = r->GPR [ i.Rt ].uz >> i.Shift;
	r->GPR [ i.Rd ].uw = r->GPR [ i.Rt ].uw >> i.Shift;
	
#if defined INLINE_DEBUG_PSRLW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}


static void Execute::PSRAH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSRAH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sh0 = r->GPR [ i.Rt ].sh0 >> i.Shift;
	r->GPR [ i.Rd ].sh1 = r->GPR [ i.Rt ].sh1 >> i.Shift;
	r->GPR [ i.Rd ].sh2 = r->GPR [ i.Rt ].sh2 >> i.Shift;
	r->GPR [ i.Rd ].sh3 = r->GPR [ i.Rt ].sh3 >> i.Shift;
	r->GPR [ i.Rd ].sh4 = r->GPR [ i.Rt ].sh4 >> i.Shift;
	r->GPR [ i.Rd ].sh5 = r->GPR [ i.Rt ].sh5 >> i.Shift;
	r->GPR [ i.Rd ].sh6 = r->GPR [ i.Rt ].sh6 >> i.Shift;
	r->GPR [ i.Rd ].sh7 = r->GPR [ i.Rt ].sh7 >> i.Shift;
	
#if defined INLINE_DEBUG_PSRAH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PSRAW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSRAW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sx = r->GPR [ i.Rt ].sx >> i.Shift;
	r->GPR [ i.Rd ].sy = r->GPR [ i.Rt ].sy >> i.Shift;
	r->GPR [ i.Rd ].sz = r->GPR [ i.Rt ].sz >> i.Shift;
	r->GPR [ i.Rd ].sw = r->GPR [ i.Rt ].sw >> i.Shift;
	
#if defined INLINE_DEBUG_PSRAW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PSLLVW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSLLVW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sq0 = (s32) ( r->GPR [ i.Rt ].uw0 << ( r->GPR [ i.Rs ].uw0 & 0x1f ) );
	r->GPR [ i.Rd ].sq1 = (s32) ( r->GPR [ i.Rt ].uw2 << ( r->GPR [ i.Rs ].uw2 & 0x1f ) );
	
#if defined INLINE_DEBUG_PSLLVW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PSRLVW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSRLVW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sq0 = (s32) ( r->GPR [ i.Rt ].uw0 >> ( r->GPR [ i.Rs ].uw0 & 0x1f ) );
	r->GPR [ i.Rd ].sq1 = (s32) ( r->GPR [ i.Rt ].uw2 >> ( r->GPR [ i.Rs ].uw2 & 0x1f ) );
	
#if defined INLINE_DEBUG_PSRLVW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PSRAVW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSRAVW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sq0 = (s32) ( r->GPR [ i.Rt ].sw0 >> ( r->GPR [ i.Rs ].sw0 & 0x1f ) );
	r->GPR [ i.Rd ].sq1 = (s32) ( r->GPR [ i.Rt ].sw2 >> ( r->GPR [ i.Rs ].sw2 & 0x1f ) );
	
#if defined INLINE_DEBUG_PSRAVW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}


static void Execute::PADDB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PADDB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].ub0 = r->GPR [ i.Rs ].ub0 + r->GPR [ i.Rt ].ub0;
	r->GPR [ i.Rd ].ub1 = r->GPR [ i.Rs ].ub1 + r->GPR [ i.Rt ].ub1;
	r->GPR [ i.Rd ].ub2 = r->GPR [ i.Rs ].ub2 + r->GPR [ i.Rt ].ub2;
	r->GPR [ i.Rd ].ub3 = r->GPR [ i.Rs ].ub3 + r->GPR [ i.Rt ].ub3;
	r->GPR [ i.Rd ].ub4 = r->GPR [ i.Rs ].ub4 + r->GPR [ i.Rt ].ub4;
	r->GPR [ i.Rd ].ub5 = r->GPR [ i.Rs ].ub5 + r->GPR [ i.Rt ].ub5;
	r->GPR [ i.Rd ].ub6 = r->GPR [ i.Rs ].ub6 + r->GPR [ i.Rt ].ub6;
	r->GPR [ i.Rd ].ub7 = r->GPR [ i.Rs ].ub7 + r->GPR [ i.Rt ].ub7;
	r->GPR [ i.Rd ].ub8 = r->GPR [ i.Rs ].ub8 + r->GPR [ i.Rt ].ub8;
	r->GPR [ i.Rd ].ub9 = r->GPR [ i.Rs ].ub9 + r->GPR [ i.Rt ].ub9;
	r->GPR [ i.Rd ].ub10 = r->GPR [ i.Rs ].ub10 + r->GPR [ i.Rt ].ub10;
	r->GPR [ i.Rd ].ub11 = r->GPR [ i.Rs ].ub11 + r->GPR [ i.Rt ].ub11;
	r->GPR [ i.Rd ].ub12 = r->GPR [ i.Rs ].ub12 + r->GPR [ i.Rt ].ub12;
	r->GPR [ i.Rd ].ub13 = r->GPR [ i.Rs ].ub13 + r->GPR [ i.Rt ].ub13;
	r->GPR [ i.Rd ].ub14 = r->GPR [ i.Rs ].ub14 + r->GPR [ i.Rt ].ub14;
	r->GPR [ i.Rd ].ub15 = r->GPR [ i.Rs ].ub15 + r->GPR [ i.Rt ].ub15;
	
#if defined INLINE_DEBUG_PADDB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PADDH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PADDH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uh0 = r->GPR [ i.Rs ].uh0 + r->GPR [ i.Rt ].uh0;
	r->GPR [ i.Rd ].uh1 = r->GPR [ i.Rs ].uh1 + r->GPR [ i.Rt ].uh1;
	r->GPR [ i.Rd ].uh2 = r->GPR [ i.Rs ].uh2 + r->GPR [ i.Rt ].uh2;
	r->GPR [ i.Rd ].uh3 = r->GPR [ i.Rs ].uh3 + r->GPR [ i.Rt ].uh3;
	r->GPR [ i.Rd ].uh4 = r->GPR [ i.Rs ].uh4 + r->GPR [ i.Rt ].uh4;
	r->GPR [ i.Rd ].uh5 = r->GPR [ i.Rs ].uh5 + r->GPR [ i.Rt ].uh5;
	r->GPR [ i.Rd ].uh6 = r->GPR [ i.Rs ].uh6 + r->GPR [ i.Rt ].uh6;
	r->GPR [ i.Rd ].uh7 = r->GPR [ i.Rs ].uh7 + r->GPR [ i.Rt ].uh7;
	
#if defined INLINE_DEBUG_PADDH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PADDW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PADDW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].ux = r->GPR [ i.Rs ].ux + r->GPR [ i.Rt ].ux;
	r->GPR [ i.Rd ].uy = r->GPR [ i.Rs ].uy + r->GPR [ i.Rt ].uy;
	r->GPR [ i.Rd ].uz = r->GPR [ i.Rs ].uz + r->GPR [ i.Rt ].uz;
	r->GPR [ i.Rd ].uw = r->GPR [ i.Rs ].uw + r->GPR [ i.Rt ].uw;
	
#if defined INLINE_DEBUG_PADDW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PSUBB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSUBB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].ub0 = r->GPR [ i.Rs ].ub0 - r->GPR [ i.Rt ].ub0;
	r->GPR [ i.Rd ].ub1 = r->GPR [ i.Rs ].ub1 - r->GPR [ i.Rt ].ub1;
	r->GPR [ i.Rd ].ub2 = r->GPR [ i.Rs ].ub2 - r->GPR [ i.Rt ].ub2;
	r->GPR [ i.Rd ].ub3 = r->GPR [ i.Rs ].ub3 - r->GPR [ i.Rt ].ub3;
	r->GPR [ i.Rd ].ub4 = r->GPR [ i.Rs ].ub4 - r->GPR [ i.Rt ].ub4;
	r->GPR [ i.Rd ].ub5 = r->GPR [ i.Rs ].ub5 - r->GPR [ i.Rt ].ub5;
	r->GPR [ i.Rd ].ub6 = r->GPR [ i.Rs ].ub6 - r->GPR [ i.Rt ].ub6;
	r->GPR [ i.Rd ].ub7 = r->GPR [ i.Rs ].ub7 - r->GPR [ i.Rt ].ub7;
	r->GPR [ i.Rd ].ub8 = r->GPR [ i.Rs ].ub8 - r->GPR [ i.Rt ].ub8;
	r->GPR [ i.Rd ].ub9 = r->GPR [ i.Rs ].ub9 - r->GPR [ i.Rt ].ub9;
	r->GPR [ i.Rd ].ub10 = r->GPR [ i.Rs ].ub10 - r->GPR [ i.Rt ].ub10;
	r->GPR [ i.Rd ].ub11 = r->GPR [ i.Rs ].ub11 - r->GPR [ i.Rt ].ub11;
	r->GPR [ i.Rd ].ub12 = r->GPR [ i.Rs ].ub12 - r->GPR [ i.Rt ].ub12;
	r->GPR [ i.Rd ].ub13 = r->GPR [ i.Rs ].ub13 - r->GPR [ i.Rt ].ub13;
	r->GPR [ i.Rd ].ub14 = r->GPR [ i.Rs ].ub14 - r->GPR [ i.Rt ].ub14;
	r->GPR [ i.Rd ].ub15 = r->GPR [ i.Rs ].ub15 - r->GPR [ i.Rt ].ub15;
	
#if defined INLINE_DEBUG_PSUBB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PSUBH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSUBH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uh0 = r->GPR [ i.Rs ].uh0 - r->GPR [ i.Rt ].uh0;
	r->GPR [ i.Rd ].uh1 = r->GPR [ i.Rs ].uh1 - r->GPR [ i.Rt ].uh1;
	r->GPR [ i.Rd ].uh2 = r->GPR [ i.Rs ].uh2 - r->GPR [ i.Rt ].uh2;
	r->GPR [ i.Rd ].uh3 = r->GPR [ i.Rs ].uh3 - r->GPR [ i.Rt ].uh3;
	r->GPR [ i.Rd ].uh4 = r->GPR [ i.Rs ].uh4 - r->GPR [ i.Rt ].uh4;
	r->GPR [ i.Rd ].uh5 = r->GPR [ i.Rs ].uh5 - r->GPR [ i.Rt ].uh5;
	r->GPR [ i.Rd ].uh6 = r->GPR [ i.Rs ].uh6 - r->GPR [ i.Rt ].uh6;
	r->GPR [ i.Rd ].uh7 = r->GPR [ i.Rs ].uh7 - r->GPR [ i.Rt ].uh7;
	
#if defined INLINE_DEBUG_PSUBH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PSUBW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSUBW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].ux = r->GPR [ i.Rs ].ux - r->GPR [ i.Rt ].ux;
	r->GPR [ i.Rd ].uy = r->GPR [ i.Rs ].uy - r->GPR [ i.Rt ].uy;
	r->GPR [ i.Rd ].uz = r->GPR [ i.Rs ].uz - r->GPR [ i.Rt ].uz;
	r->GPR [ i.Rd ].uw = r->GPR [ i.Rs ].uw - r->GPR [ i.Rt ].uw;
	
#if defined INLINE_DEBUG_PSUBW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}


static void Execute::PADDSB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PADDSB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::PADDSH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PADDSH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::PADDSW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PADDSW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}


static void Execute::PSUBSB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSUBSB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}


static void Execute::PSUBSH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSUBSH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}


static void Execute::PSUBSW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSUBSW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}


static void Execute::PADDUB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PADDUB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].ub0 = ( ( ( (u64) r->GPR [ i.Rs ].ub0 ) + ( (u64) r->GPR [ i.Rt ].ub0 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub0 + r->GPR [ i.Rt ].ub0 ) );
	r->GPR [ i.Rd ].ub1 = ( ( ( (u64) r->GPR [ i.Rs ].ub1 ) + ( (u64) r->GPR [ i.Rt ].ub1 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub1 + r->GPR [ i.Rt ].ub1 ) );
	r->GPR [ i.Rd ].ub2 = ( ( ( (u64) r->GPR [ i.Rs ].ub2 ) + ( (u64) r->GPR [ i.Rt ].ub2 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub2 + r->GPR [ i.Rt ].ub2 ) );
	r->GPR [ i.Rd ].ub3 = ( ( ( (u64) r->GPR [ i.Rs ].ub3 ) + ( (u64) r->GPR [ i.Rt ].ub3 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub3 + r->GPR [ i.Rt ].ub3 ) );
	r->GPR [ i.Rd ].ub4 = ( ( ( (u64) r->GPR [ i.Rs ].ub4 ) + ( (u64) r->GPR [ i.Rt ].ub4 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub4 + r->GPR [ i.Rt ].ub4 ) );
	r->GPR [ i.Rd ].ub5 = ( ( ( (u64) r->GPR [ i.Rs ].ub5 ) + ( (u64) r->GPR [ i.Rt ].ub5 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub5 + r->GPR [ i.Rt ].ub5 ) );
	r->GPR [ i.Rd ].ub6 = ( ( ( (u64) r->GPR [ i.Rs ].ub6 ) + ( (u64) r->GPR [ i.Rt ].ub6 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub6 + r->GPR [ i.Rt ].ub6 ) );
	r->GPR [ i.Rd ].ub7 = ( ( ( (u64) r->GPR [ i.Rs ].ub7 ) + ( (u64) r->GPR [ i.Rt ].ub7 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub7 + r->GPR [ i.Rt ].ub7 ) );
	r->GPR [ i.Rd ].ub8 = ( ( ( (u64) r->GPR [ i.Rs ].ub8 ) + ( (u64) r->GPR [ i.Rt ].ub8 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub8 + r->GPR [ i.Rt ].ub8 ) );
	r->GPR [ i.Rd ].ub9 = ( ( ( (u64) r->GPR [ i.Rs ].ub9 ) + ( (u64) r->GPR [ i.Rt ].ub9 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub9 + r->GPR [ i.Rt ].ub9 ) );
	r->GPR [ i.Rd ].ub10 = ( ( ( (u64) r->GPR [ i.Rs ].ub10 ) + ( (u64) r->GPR [ i.Rt ].ub10 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub10 + r->GPR [ i.Rt ].ub10 ) );
	r->GPR [ i.Rd ].ub11 = ( ( ( (u64) r->GPR [ i.Rs ].ub11 ) + ( (u64) r->GPR [ i.Rt ].ub11 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub11 + r->GPR [ i.Rt ].ub11 ) );
	r->GPR [ i.Rd ].ub12 = ( ( ( (u64) r->GPR [ i.Rs ].ub12 ) + ( (u64) r->GPR [ i.Rt ].ub12 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub12 + r->GPR [ i.Rt ].ub12 ) );
	r->GPR [ i.Rd ].ub13 = ( ( ( (u64) r->GPR [ i.Rs ].ub13 ) + ( (u64) r->GPR [ i.Rt ].ub13 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub13 + r->GPR [ i.Rt ].ub13 ) );
	r->GPR [ i.Rd ].ub14 = ( ( ( (u64) r->GPR [ i.Rs ].ub14 ) + ( (u64) r->GPR [ i.Rt ].ub14 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub14 + r->GPR [ i.Rt ].ub14 ) );
	r->GPR [ i.Rd ].ub15 = ( ( ( (u64) r->GPR [ i.Rs ].ub15 ) + ( (u64) r->GPR [ i.Rt ].ub15 ) ) > 0xffULL ? 0xff : ( r->GPR [ i.Rs ].ub15 + r->GPR [ i.Rt ].ub15 ) );
	
#if defined INLINE_DEBUG_PADDUB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PADDUH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PADDUH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uh0 = ( ( ( (u64) r->GPR [ i.Rs ].uh0 ) + ( (u64) r->GPR [ i.Rt ].uh0 ) ) > 0xffffULL ? 0xffff : ( r->GPR [ i.Rs ].uh0 + r->GPR [ i.Rt ].uh0 ) );
	r->GPR [ i.Rd ].uh1 = ( ( ( (u64) r->GPR [ i.Rs ].uh1 ) + ( (u64) r->GPR [ i.Rt ].uh1 ) ) > 0xffffULL ? 0xffff : ( r->GPR [ i.Rs ].uh1 + r->GPR [ i.Rt ].uh1 ) );
	r->GPR [ i.Rd ].uh2 = ( ( ( (u64) r->GPR [ i.Rs ].uh2 ) + ( (u64) r->GPR [ i.Rt ].uh2 ) ) > 0xffffULL ? 0xffff : ( r->GPR [ i.Rs ].uh2 + r->GPR [ i.Rt ].uh2 ) );
	r->GPR [ i.Rd ].uh3 = ( ( ( (u64) r->GPR [ i.Rs ].uh3 ) + ( (u64) r->GPR [ i.Rt ].uh3 ) ) > 0xffffULL ? 0xffff : ( r->GPR [ i.Rs ].uh3 + r->GPR [ i.Rt ].uh3 ) );
	r->GPR [ i.Rd ].uh4 = ( ( ( (u64) r->GPR [ i.Rs ].uh4 ) + ( (u64) r->GPR [ i.Rt ].uh4 ) ) > 0xffffULL ? 0xffff : ( r->GPR [ i.Rs ].uh4 + r->GPR [ i.Rt ].uh4 ) );
	r->GPR [ i.Rd ].uh5 = ( ( ( (u64) r->GPR [ i.Rs ].uh5 ) + ( (u64) r->GPR [ i.Rt ].uh5 ) ) > 0xffffULL ? 0xffff : ( r->GPR [ i.Rs ].uh5 + r->GPR [ i.Rt ].uh5 ) );
	r->GPR [ i.Rd ].uh6 = ( ( ( (u64) r->GPR [ i.Rs ].uh6 ) + ( (u64) r->GPR [ i.Rt ].uh6 ) ) > 0xffffULL ? 0xffff : ( r->GPR [ i.Rs ].uh6 + r->GPR [ i.Rt ].uh6 ) );
	r->GPR [ i.Rd ].uh7 = ( ( ( (u64) r->GPR [ i.Rs ].uh7 ) + ( (u64) r->GPR [ i.Rt ].uh7 ) ) > 0xffffULL ? 0xffff : ( r->GPR [ i.Rs ].uh7 + r->GPR [ i.Rt ].uh7 ) );
	
#if defined INLINE_DEBUG_PADDUH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PADDUW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PADDUW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	// rd = rs + rt
	r->GPR [ i.Rd ].uw0 = ( ( ( (u64) r->GPR [ i.Rs ].uw0 ) + ( (u64) r->GPR [ i.Rt ].uw0 ) ) > 0xffffffffULL ? 0xffffffff : ( r->GPR [ i.Rs ].uw0 + r->GPR [ i.Rt ].uw0 ) );
	r->GPR [ i.Rd ].uw1 = ( ( ( (u64) r->GPR [ i.Rs ].uw1 ) + ( (u64) r->GPR [ i.Rt ].uw1 ) ) > 0xffffffffULL ? 0xffffffff : ( r->GPR [ i.Rs ].uw1 + r->GPR [ i.Rt ].uw1 ) );
	r->GPR [ i.Rd ].uw2 = ( ( ( (u64) r->GPR [ i.Rs ].uw2 ) + ( (u64) r->GPR [ i.Rt ].uw2 ) ) > 0xffffffffULL ? 0xffffffff : ( r->GPR [ i.Rs ].uw2 + r->GPR [ i.Rt ].uw2 ) );
	r->GPR [ i.Rd ].uw3 = ( ( ( (u64) r->GPR [ i.Rs ].uw3 ) + ( (u64) r->GPR [ i.Rt ].uw3 ) ) > 0xffffffffULL ? 0xffffffff : ( r->GPR [ i.Rs ].uw3 + r->GPR [ i.Rt ].uw3 ) );
	
#if defined INLINE_DEBUG_PADDUW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}


static void Execute::PSUBUB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSUBUB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::PSUBUH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSUBUH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::PSUBUW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PSUBUW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}



static void Execute::PMAXH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMAXH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sh0 = ( r->GPR [ i.Rs ].sh0 > r->GPR [ i.Rt ].sh0 ) ? r->GPR [ i.Rs ].sh0 : r->GPR [ i.Rt ].sh0;
	r->GPR [ i.Rd ].sh1 = ( r->GPR [ i.Rs ].sh1 > r->GPR [ i.Rt ].sh1 ) ? r->GPR [ i.Rs ].sh1 : r->GPR [ i.Rt ].sh1;
	r->GPR [ i.Rd ].sh2 = ( r->GPR [ i.Rs ].sh2 > r->GPR [ i.Rt ].sh2 ) ? r->GPR [ i.Rs ].sh2 : r->GPR [ i.Rt ].sh2;
	r->GPR [ i.Rd ].sh3 = ( r->GPR [ i.Rs ].sh3 > r->GPR [ i.Rt ].sh3 ) ? r->GPR [ i.Rs ].sh3 : r->GPR [ i.Rt ].sh3;
	r->GPR [ i.Rd ].sh4 = ( r->GPR [ i.Rs ].sh4 > r->GPR [ i.Rt ].sh4 ) ? r->GPR [ i.Rs ].sh4 : r->GPR [ i.Rt ].sh4;
	r->GPR [ i.Rd ].sh5 = ( r->GPR [ i.Rs ].sh5 > r->GPR [ i.Rt ].sh5 ) ? r->GPR [ i.Rs ].sh5 : r->GPR [ i.Rt ].sh5;
	r->GPR [ i.Rd ].sh6 = ( r->GPR [ i.Rs ].sh6 > r->GPR [ i.Rt ].sh6 ) ? r->GPR [ i.Rs ].sh6 : r->GPR [ i.Rt ].sh6;
	r->GPR [ i.Rd ].sh7 = ( r->GPR [ i.Rs ].sh7 > r->GPR [ i.Rt ].sh7 ) ? r->GPR [ i.Rs ].sh7 : r->GPR [ i.Rt ].sh7;
	
#if defined INLINE_DEBUG_PMAXH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PMAXW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMAXW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sw0 = ( r->GPR [ i.Rs ].sw0 > r->GPR [ i.Rt ].sw0 ) ? r->GPR [ i.Rs ].sw0 : r->GPR [ i.Rt ].sw0;
	r->GPR [ i.Rd ].sw1 = ( r->GPR [ i.Rs ].sw1 > r->GPR [ i.Rt ].sw1 ) ? r->GPR [ i.Rs ].sw1 : r->GPR [ i.Rt ].sw1;
	r->GPR [ i.Rd ].sw2 = ( r->GPR [ i.Rs ].sw2 > r->GPR [ i.Rt ].sw2 ) ? r->GPR [ i.Rs ].sw2 : r->GPR [ i.Rt ].sw2;
	r->GPR [ i.Rd ].sw3 = ( r->GPR [ i.Rs ].sw3 > r->GPR [ i.Rt ].sw3 ) ? r->GPR [ i.Rs ].sw3 : r->GPR [ i.Rt ].sw3;
	
#if defined INLINE_DEBUG_PMAXW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PMINH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMINH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sh0 = ( r->GPR [ i.Rs ].sh0 < r->GPR [ i.Rt ].sh0 ) ? r->GPR [ i.Rs ].sh0 : r->GPR [ i.Rt ].sh0;
	r->GPR [ i.Rd ].sh1 = ( r->GPR [ i.Rs ].sh1 < r->GPR [ i.Rt ].sh1 ) ? r->GPR [ i.Rs ].sh1 : r->GPR [ i.Rt ].sh1;
	r->GPR [ i.Rd ].sh2 = ( r->GPR [ i.Rs ].sh2 < r->GPR [ i.Rt ].sh2 ) ? r->GPR [ i.Rs ].sh2 : r->GPR [ i.Rt ].sh2;
	r->GPR [ i.Rd ].sh3 = ( r->GPR [ i.Rs ].sh3 < r->GPR [ i.Rt ].sh3 ) ? r->GPR [ i.Rs ].sh3 : r->GPR [ i.Rt ].sh3;
	r->GPR [ i.Rd ].sh4 = ( r->GPR [ i.Rs ].sh4 < r->GPR [ i.Rt ].sh4 ) ? r->GPR [ i.Rs ].sh4 : r->GPR [ i.Rt ].sh4;
	r->GPR [ i.Rd ].sh5 = ( r->GPR [ i.Rs ].sh5 < r->GPR [ i.Rt ].sh5 ) ? r->GPR [ i.Rs ].sh5 : r->GPR [ i.Rt ].sh5;
	r->GPR [ i.Rd ].sh6 = ( r->GPR [ i.Rs ].sh6 < r->GPR [ i.Rt ].sh6 ) ? r->GPR [ i.Rs ].sh6 : r->GPR [ i.Rt ].sh6;
	r->GPR [ i.Rd ].sh7 = ( r->GPR [ i.Rs ].sh7 < r->GPR [ i.Rt ].sh7 ) ? r->GPR [ i.Rs ].sh7 : r->GPR [ i.Rt ].sh7;
	
#if defined INLINE_DEBUG_PMINH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PMINW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMINW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sw0 = ( r->GPR [ i.Rs ].sw0 < r->GPR [ i.Rt ].sw0 ) ? r->GPR [ i.Rs ].sw0 : r->GPR [ i.Rt ].sw0;
	r->GPR [ i.Rd ].sw1 = ( r->GPR [ i.Rs ].sw1 < r->GPR [ i.Rt ].sw1 ) ? r->GPR [ i.Rs ].sw1 : r->GPR [ i.Rt ].sw1;
	r->GPR [ i.Rd ].sw2 = ( r->GPR [ i.Rs ].sw2 < r->GPR [ i.Rt ].sw2 ) ? r->GPR [ i.Rs ].sw2 : r->GPR [ i.Rt ].sw2;
	r->GPR [ i.Rd ].sw3 = ( r->GPR [ i.Rs ].sw3 < r->GPR [ i.Rt ].sw3 ) ? r->GPR [ i.Rs ].sw3 : r->GPR [ i.Rt ].sw3;
	
#if defined INLINE_DEBUG_PMINW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}




static void Execute::PPACB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PPACB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	// note: must use this order
	r->GPR [ i.Rd ].ub1 = r->GPR [ i.Rt ].ub2;
	r->GPR [ i.Rd ].ub3 = r->GPR [ i.Rt ].ub6;
	r->GPR [ i.Rd ].ub5 = r->GPR [ i.Rt ].ub10;
	r->GPR [ i.Rd ].ub7 = r->GPR [ i.Rt ].ub14;
	r->GPR [ i.Rd ].ub9 = r->GPR [ i.Rs ].ub2;
	r->GPR [ i.Rd ].ub11 = r->GPR [ i.Rs ].ub6;
	r->GPR [ i.Rd ].ub13 = r->GPR [ i.Rs ].ub10;
	r->GPR [ i.Rd ].ub15 = r->GPR [ i.Rs ].ub14;
	
	r->GPR [ i.Rd ].ub2 = r->GPR [ i.Rt ].ub4;
	r->GPR [ i.Rd ].ub6 = r->GPR [ i.Rt ].ub12;
	r->GPR [ i.Rd ].ub10 = r->GPR [ i.Rs ].ub4;
	r->GPR [ i.Rd ].ub14 = r->GPR [ i.Rs ].ub12;
	
	r->GPR [ i.Rd ].ub12 = r->GPR [ i.Rs ].ub8;
	r->GPR [ i.Rd ].ub4 = r->GPR [ i.Rt ].ub8;
	r->GPR [ i.Rd ].ub8 = r->GPR [ i.Rs ].ub0;
	r->GPR [ i.Rd ].ub0 = r->GPR [ i.Rt ].ub0;
	
	
#if defined INLINE_DEBUG_PPACB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PPACH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PPACH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	// note: must use this order
	r->GPR [ i.Rd ].uh7 = r->GPR [ i.Rs ].uh6;
	r->GPR [ i.Rd ].uh3 = r->GPR [ i.Rt ].uh6;
	
	r->GPR [ i.Rd ].uh5 = r->GPR [ i.Rs ].uh2;
	r->GPR [ i.Rd ].uh1 = r->GPR [ i.Rt ].uh2;
	
	r->GPR [ i.Rd ].uh6 = r->GPR [ i.Rs ].uh4;
	r->GPR [ i.Rd ].uh2 = r->GPR [ i.Rt ].uh4;
	
	r->GPR [ i.Rd ].uh4 = r->GPR [ i.Rs ].uh0;
	r->GPR [ i.Rd ].uh0 = r->GPR [ i.Rt ].uh0;
	
#if defined INLINE_DEBUG_PPACH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PPACW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PPACW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	// note: must use this order
	r->GPR [ i.Rd ].uw3 = r->GPR [ i.Rs ].uw2;
	r->GPR [ i.Rd ].uw1 = r->GPR [ i.Rt ].uw2;
	
	r->GPR [ i.Rd ].uw2 = r->GPR [ i.Rs ].uw0;
	r->GPR [ i.Rd ].uw0 = r->GPR [ i.Rt ].uw0;
	
#if defined INLINE_DEBUG_PPACW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}


static void Execute::PEXT5 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PEXT5 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uq0 = ( ( r->GPR [ i.Rt ].uq0 & 0x1f0000001fULL ) << 3 ) | ( ( r->GPR [ i.Rt ].uq0 & 0x3e0000003e0ULL ) << 6 ) | ( ( r->GPR [ i.Rt ].uq0 & 0x7c0000007c00ULL ) << 9 ) | ( ( r->GPR [ i.Rt ].uq0 & 0x800000008000ULL ) << 16 );
	r->GPR [ i.Rd ].uq1 = ( ( r->GPR [ i.Rt ].uq1 & 0x1f0000001fULL ) << 3 ) | ( ( r->GPR [ i.Rt ].uq1 & 0x3e0000003e0ULL ) << 6 ) | ( ( r->GPR [ i.Rt ].uq1 & 0x7c0000007c00ULL ) << 9 ) | ( ( r->GPR [ i.Rt ].uq1 & 0x800000008000ULL ) << 16 );
	
#if defined INLINE_DEBUG_PEXT5 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PPAC5 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PPAC5 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	u64 temp;
	temp = ( ( r->GPR [ i.Rt ].uq0 & 0xf8000000f8ULL ) >> 3 ) | ( ( r->GPR [ i.Rt ].uq0 & 0xf8000000f800ULL ) >> 6 ) | ( ( r->GPR [ i.Rt ].uq0 & 0xf8000000f80000ULL ) >> 9 ) | ( ( r->GPR [ i.Rt ].uq0 & 0x8000000080000000ULL ) >> 16 );
	r->GPR [ i.Rd ].uh0 = (u16) temp;
	r->GPR [ i.Rd ].uh2 = (u16) ( temp >> 32 );
	temp = ( ( r->GPR [ i.Rt ].uq1 & 0xf8000000f8ULL ) >> 3 ) | ( ( r->GPR [ i.Rt ].uq1 & 0xf8000000f800ULL ) >> 6 ) | ( ( r->GPR [ i.Rt ].uq1 & 0xf8000000f80000ULL ) >> 9 ) | ( ( r->GPR [ i.Rt ].uq1 & 0x8000000080000000ULL ) >> 16 );
	r->GPR [ i.Rd ].uh4 = (u16) temp;
	r->GPR [ i.Rd ].uh6 = (u16) ( temp >> 32 );
	
#if defined INLINE_DEBUG_PPAC5 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}


static void Execute::PCGTB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PCGTB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sb0 = ( r->GPR [ i.Rs ].sb0 > r->GPR [ i.Rt ].sb0 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb1 = ( r->GPR [ i.Rs ].sb1 > r->GPR [ i.Rt ].sb1 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb2 = ( r->GPR [ i.Rs ].sb2 > r->GPR [ i.Rt ].sb2 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb3 = ( r->GPR [ i.Rs ].sb3 > r->GPR [ i.Rt ].sb3 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb4 = ( r->GPR [ i.Rs ].sb4 > r->GPR [ i.Rt ].sb4 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb5 = ( r->GPR [ i.Rs ].sb5 > r->GPR [ i.Rt ].sb5 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb6 = ( r->GPR [ i.Rs ].sb6 > r->GPR [ i.Rt ].sb6 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb7 = ( r->GPR [ i.Rs ].sb7 > r->GPR [ i.Rt ].sb7 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb8 = ( r->GPR [ i.Rs ].sb8 > r->GPR [ i.Rt ].sb8 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb9 = ( r->GPR [ i.Rs ].sb9 > r->GPR [ i.Rt ].sb9 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb10 = ( r->GPR [ i.Rs ].sb10 > r->GPR [ i.Rt ].sb10 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb11 = ( r->GPR [ i.Rs ].sb11 > r->GPR [ i.Rt ].sb11 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb12 = ( r->GPR [ i.Rs ].sb12 > r->GPR [ i.Rt ].sb12 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb13 = ( r->GPR [ i.Rs ].sb13 > r->GPR [ i.Rt ].sb13 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb14 = ( r->GPR [ i.Rs ].sb14 > r->GPR [ i.Rt ].sb14 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb15 = ( r->GPR [ i.Rs ].sb15 > r->GPR [ i.Rt ].sb15 ) ? 0xff : 0;
	
#if defined INLINE_DEBUG_PCGTB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PCGTH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PCGTH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sh0 = ( r->GPR [ i.Rs ].sh0 > r->GPR [ i.Rt ].sh0 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh1 = ( r->GPR [ i.Rs ].sh1 > r->GPR [ i.Rt ].sh1 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh2 = ( r->GPR [ i.Rs ].sh2 > r->GPR [ i.Rt ].sh2 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh3 = ( r->GPR [ i.Rs ].sh3 > r->GPR [ i.Rt ].sh3 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh4 = ( r->GPR [ i.Rs ].sh4 > r->GPR [ i.Rt ].sh4 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh5 = ( r->GPR [ i.Rs ].sh5 > r->GPR [ i.Rt ].sh5 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh6 = ( r->GPR [ i.Rs ].sh6 > r->GPR [ i.Rt ].sh6 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh7 = ( r->GPR [ i.Rs ].sh7 > r->GPR [ i.Rt ].sh7 ) ? 0xffff : 0;
	
#if defined INLINE_DEBUG_PCGTH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PCGTW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PCGTW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sw0 = ( r->GPR [ i.Rs ].sw0 > r->GPR [ i.Rt ].sw0 ) ? 0xffffffff : 0;
	r->GPR [ i.Rd ].sw1 = ( r->GPR [ i.Rs ].sw1 > r->GPR [ i.Rt ].sw1 ) ? 0xffffffff : 0;
	r->GPR [ i.Rd ].sw2 = ( r->GPR [ i.Rs ].sw2 > r->GPR [ i.Rt ].sw2 ) ? 0xffffffff : 0;
	r->GPR [ i.Rd ].sw3 = ( r->GPR [ i.Rs ].sw3 > r->GPR [ i.Rt ].sw3 ) ? 0xffffffff : 0;
	
#if defined INLINE_DEBUG_PCGTW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}


static void Execute::PCEQB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PCEQB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sb0 = ( r->GPR [ i.Rs ].sb0 == r->GPR [ i.Rt ].sb0 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb1 = ( r->GPR [ i.Rs ].sb1 == r->GPR [ i.Rt ].sb1 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb2 = ( r->GPR [ i.Rs ].sb2 == r->GPR [ i.Rt ].sb2 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb3 = ( r->GPR [ i.Rs ].sb3 == r->GPR [ i.Rt ].sb3 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb4 = ( r->GPR [ i.Rs ].sb4 == r->GPR [ i.Rt ].sb4 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb5 = ( r->GPR [ i.Rs ].sb5 == r->GPR [ i.Rt ].sb5 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb6 = ( r->GPR [ i.Rs ].sb6 == r->GPR [ i.Rt ].sb6 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb7 = ( r->GPR [ i.Rs ].sb7 == r->GPR [ i.Rt ].sb7 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb8 = ( r->GPR [ i.Rs ].sb8 == r->GPR [ i.Rt ].sb8 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb9 = ( r->GPR [ i.Rs ].sb9 == r->GPR [ i.Rt ].sb9 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb10 = ( r->GPR [ i.Rs ].sb10 == r->GPR [ i.Rt ].sb10 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb11 = ( r->GPR [ i.Rs ].sb11 == r->GPR [ i.Rt ].sb11 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb12 = ( r->GPR [ i.Rs ].sb12 == r->GPR [ i.Rt ].sb12 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb13 = ( r->GPR [ i.Rs ].sb13 == r->GPR [ i.Rt ].sb13 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb14 = ( r->GPR [ i.Rs ].sb14 == r->GPR [ i.Rt ].sb14 ) ? 0xff : 0;
	r->GPR [ i.Rd ].sb15 = ( r->GPR [ i.Rs ].sb15 == r->GPR [ i.Rt ].sb15 ) ? 0xff : 0;
	
#if defined INLINE_DEBUG_PCEQB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PCEQH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PCEQH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sh0 = ( r->GPR [ i.Rs ].sh0 == r->GPR [ i.Rt ].sh0 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh1 = ( r->GPR [ i.Rs ].sh1 == r->GPR [ i.Rt ].sh1 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh2 = ( r->GPR [ i.Rs ].sh2 == r->GPR [ i.Rt ].sh2 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh3 = ( r->GPR [ i.Rs ].sh3 == r->GPR [ i.Rt ].sh3 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh4 = ( r->GPR [ i.Rs ].sh4 == r->GPR [ i.Rt ].sh4 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh5 = ( r->GPR [ i.Rs ].sh5 == r->GPR [ i.Rt ].sh5 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh6 = ( r->GPR [ i.Rs ].sh6 == r->GPR [ i.Rt ].sh6 ) ? 0xffff : 0;
	r->GPR [ i.Rd ].sh7 = ( r->GPR [ i.Rs ].sh7 == r->GPR [ i.Rt ].sh7 ) ? 0xffff : 0;
	
#if defined INLINE_DEBUG_PCEQH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}


static void Execute::PCEQW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PCEQW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sw0 = ( r->GPR [ i.Rs ].sw0 == r->GPR [ i.Rt ].sw0 ) ? 0xffffffff : 0;
	r->GPR [ i.Rd ].sw1 = ( r->GPR [ i.Rs ].sw1 == r->GPR [ i.Rt ].sw1 ) ? 0xffffffff : 0;
	r->GPR [ i.Rd ].sw2 = ( r->GPR [ i.Rs ].sw2 == r->GPR [ i.Rt ].sw2 ) ? 0xffffffff : 0;
	r->GPR [ i.Rd ].sw3 = ( r->GPR [ i.Rs ].sw3 == r->GPR [ i.Rt ].sw3 ) ? 0xffffffff : 0;
	
#if defined INLINE_DEBUG_PCEQW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}




static void Execute::PEXTLB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PEXTLB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	// note: must do these in reverse order or else data can get overwritten
	r->GPR [ i.Rd ].uh7 = ( ( ( (u16) r->GPR [ i.Rs ].ub7 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub7 ) );
	r->GPR [ i.Rd ].uh6 = ( ( ( (u16) r->GPR [ i.Rs ].ub6 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub6 ) );
	r->GPR [ i.Rd ].uh5 = ( ( ( (u16) r->GPR [ i.Rs ].ub5 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub5 ) );
	r->GPR [ i.Rd ].uh4 = ( ( ( (u16) r->GPR [ i.Rs ].ub4 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub4 ) );
	r->GPR [ i.Rd ].uh3 = ( ( ( (u16) r->GPR [ i.Rs ].ub3 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub3 ) );
	r->GPR [ i.Rd ].uh2 = ( ( ( (u16) r->GPR [ i.Rs ].ub2 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub2 ) );
	r->GPR [ i.Rd ].uh1 = ( ( ( (u16) r->GPR [ i.Rs ].ub1 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub1 ) );
	r->GPR [ i.Rd ].uh0 = ( ( ( (u16) r->GPR [ i.Rs ].ub0 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub0 ) );
	
#if defined INLINE_DEBUG_PEXTLB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PEXTLH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PEXTLH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	// note: must do these in reverse order or else data can get overwritten
	r->GPR [ i.Rd ].uw3 = ( ( ( (u32) r->GPR [ i.Rs ].uh3 ) << 16 ) | ( (u32) r->GPR [ i.Rt ].uh3 ) );
	r->GPR [ i.Rd ].uw2 = ( ( ( (u32) r->GPR [ i.Rs ].uh2 ) << 16 ) | ( (u32) r->GPR [ i.Rt ].uh2 ) );
	r->GPR [ i.Rd ].uw1 = ( ( ( (u32) r->GPR [ i.Rs ].uh1 ) << 16 ) | ( (u32) r->GPR [ i.Rt ].uh1 ) );
	r->GPR [ i.Rd ].uw0 = ( ( ( (u32) r->GPR [ i.Rs ].uh0 ) << 16 ) | ( (u32) r->GPR [ i.Rt ].uh0 ) );
	
#if defined INLINE_DEBUG_PEXTLH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PEXTLW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PEXTLW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	// PEXTLW rd, rs, rt
	// note: must do uq1 first or else data can get overwritten
	r->GPR [ i.Rd ].uq1 = ( ( ( (u64) r->GPR [ i.Rs ].uw1 ) << 32 ) | ( (u64) r->GPR [ i.Rt ].uw1 ) );
	r->GPR [ i.Rd ].uq0 = ( ( ( (u64) r->GPR [ i.Rs ].uw0 ) << 32 ) | ( (u64) r->GPR [ i.Rt ].uw0 ) );
	
#if defined INLINE_DEBUG_PEXTLW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PEXTUB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PEXTUB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uh0 = ( ( ( (u16) r->GPR [ i.Rs ].ub8 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub8 ) );
	r->GPR [ i.Rd ].uh1 = ( ( ( (u16) r->GPR [ i.Rs ].ub9 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub9 ) );
	r->GPR [ i.Rd ].uh2 = ( ( ( (u16) r->GPR [ i.Rs ].ub10 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub10 ) );
	r->GPR [ i.Rd ].uh3 = ( ( ( (u16) r->GPR [ i.Rs ].ub11 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub11 ) );
	r->GPR [ i.Rd ].uh4 = ( ( ( (u16) r->GPR [ i.Rs ].ub12 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub12 ) );
	r->GPR [ i.Rd ].uh5 = ( ( ( (u16) r->GPR [ i.Rs ].ub13 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub13 ) );
	r->GPR [ i.Rd ].uh6 = ( ( ( (u16) r->GPR [ i.Rs ].ub14 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub14 ) );
	r->GPR [ i.Rd ].uh7 = ( ( ( (u16) r->GPR [ i.Rs ].ub15 ) << 8 ) | ( (u16) r->GPR [ i.Rt ].ub15 ) );
	
#if defined INLINE_DEBUG_PEXTUB || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PEXTUH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PEXTUH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uw0 = ( ( ( (u32) r->GPR [ i.Rs ].uh4 ) << 16 ) | ( (u32) r->GPR [ i.Rt ].uh4 ) );
	r->GPR [ i.Rd ].uw1 = ( ( ( (u32) r->GPR [ i.Rs ].uh5 ) << 16 ) | ( (u32) r->GPR [ i.Rt ].uh5 ) );
	r->GPR [ i.Rd ].uw2 = ( ( ( (u32) r->GPR [ i.Rs ].uh6 ) << 16 ) | ( (u32) r->GPR [ i.Rt ].uh6 ) );
	r->GPR [ i.Rd ].uw3 = ( ( ( (u32) r->GPR [ i.Rs ].uh7 ) << 16 ) | ( (u32) r->GPR [ i.Rt ].uh7 ) );
	
#if defined INLINE_DEBUG_PEXTUH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PEXTUW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PEXTUW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uq0 = ( ( ( (u64) r->GPR [ i.Rs ].uw2 ) << 32 ) | ( (u64) r->GPR [ i.Rt ].uw2 ) );
	r->GPR [ i.Rd ].uq1 = ( ( ( (u64) r->GPR [ i.Rs ].uw3 ) << 32 ) | ( (u64) r->GPR [ i.Rt ].uw3 ) );
	
#if defined INLINE_DEBUG_PEXTUW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}






static void Execute::PMADDH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMADDH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::PMADDW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMADDW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::PMADDUW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMADDUW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}



static void Execute::PMSUBW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMSUBW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}


static void Execute::PMFLO ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMFLO || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " LO=" << r->LO.uq0 << " " << r->LO.uq1;
#endif

	r->GPR [ i.Rd ].uq0 = r->LO.uq0;
	r->GPR [ i.Rd ].uq1 = r->LO.uq1;
	
#if defined INLINE_DEBUG_PMFLO || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PMFHI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMFHI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " HI=" << r->HI.uq0 << " " << r->HI.uq1;
#endif

	r->GPR [ i.Rd ].uq0 = r->HI.uq0;
	r->GPR [ i.Rd ].uq1 = r->HI.uq1;
	
#if defined INLINE_DEBUG_PMFHI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}


static void Execute::PINTH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PINTH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	u16 temp;

	// note: must use this order/method
	r->GPR [ i.Rd ].uh0 = r->GPR [ i.Rt ].uh0;
	r->GPR [ i.Rd ].uh7 = r->GPR [ i.Rs ].uh7;
	
	temp = r->GPR [ i.Rs ].uh4;
	r->GPR [ i.Rd ].uh4 = r->GPR [ i.Rt ].uh2;
	r->GPR [ i.Rd ].uh2 = r->GPR [ i.Rt ].uh1;
	r->GPR [ i.Rd ].uh1 = temp;
	
	temp = r->GPR [ i.Rs ].uh5;
	r->GPR [ i.Rd ].uh5 = r->GPR [ i.Rs ].uh6;
	r->GPR [ i.Rd ].uh6 = r->GPR [ i.Rt ].uh3;
	r->GPR [ i.Rd ].uh3 = temp;
	
#if defined INLINE_DEBUG_PINTH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PINTEH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PINTEH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	// note: these must be done in reverse order or else
	r->GPR [ i.Rd ].uh7 = r->GPR [ i.Rs ].uh6;
	r->GPR [ i.Rd ].uh6 = r->GPR [ i.Rt ].uh6;
	r->GPR [ i.Rd ].uh5 = r->GPR [ i.Rs ].uh4;
	r->GPR [ i.Rd ].uh4 = r->GPR [ i.Rt ].uh4;
	r->GPR [ i.Rd ].uh3 = r->GPR [ i.Rs ].uh2;
	r->GPR [ i.Rd ].uh2 = r->GPR [ i.Rt ].uh2;
	r->GPR [ i.Rd ].uh1 = r->GPR [ i.Rs ].uh0;
	r->GPR [ i.Rd ].uh0 = r->GPR [ i.Rt ].uh0;
	
#if defined INLINE_DEBUG_PINTEH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}



static void Execute::PMULTH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMULTH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	// rd = 0, 2, 4, 6
	// lo = 0, 1, 4, 5
	// hi = 2, 3, 6, 7
	
	r->GPR [ i.Rd ].sw0 = ( (s32) r->GPR [ i.Rs ].sh0 ) * ( (s32) r->GPR [ i.Rt ].sh0 );
	r->GPR [ i.Rd ].sw1 = ( (s32) r->GPR [ i.Rs ].sh2 ) * ( (s32) r->GPR [ i.Rt ].sh2 );
	r->GPR [ i.Rd ].sw2 = ( (s32) r->GPR [ i.Rs ].sh4 ) * ( (s32) r->GPR [ i.Rt ].sh4 );
	r->GPR [ i.Rd ].sw3 = ( (s32) r->GPR [ i.Rs ].sh6 ) * ( (s32) r->GPR [ i.Rt ].sh6 );
	
	r->LO.sw0 = r->GPR [ i.Rd ].sw0;
	r->LO.sw1 = ( (s32) r->GPR [ i.Rs ].sh1 ) * ( (s32) r->GPR [ i.Rt ].sh1 );
	r->LO.sw2 = r->GPR [ i.Rd ].sw2;
	r->LO.sw3 = ( (s32) r->GPR [ i.Rs ].sh5 ) * ( (s32) r->GPR [ i.Rt ].sh5 );
	
	r->HI.sw0 = r->GPR [ i.Rd ].sw1;
	r->HI.sw1 = ( (s32) r->GPR [ i.Rs ].sh3 ) * ( (s32) r->GPR [ i.Rt ].sh3 );
	r->HI.sw2 = r->GPR [ i.Rd ].sw3;
	r->HI.sw3 = ( (s32) r->GPR [ i.Rs ].sh7 ) * ( (s32) r->GPR [ i.Rt ].sh7 );
	
#if defined INLINE_DEBUG_PMULTH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PMULTW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMULTW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].sq0 = ( (s64) r->GPR [ i.Rs ].sw0 ) * ( (s64) r->GPR [ i.Rt ].sw0 );
	r->GPR [ i.Rd ].sq1 = ( (s64) r->GPR [ i.Rs ].sw2 ) * ( (s64) r->GPR [ i.Rt ].sw2 );
	
	r->LO.sq0 = (s32) ( r->GPR [ i.Rd ].sq0 );
	r->LO.sq1 = (s32) ( r->GPR [ i.Rd ].sq1 );
	r->HI.sq0 = ( r->GPR [ i.Rd ].sq0 >> 32 );
	r->HI.sq1 = ( r->GPR [ i.Rd ].sq1 >> 32 );
	
#if defined INLINE_DEBUG_PMULTW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PMULTUW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMULTUW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->GPR [ i.Rd ].uq0 = ( (u64) r->GPR [ i.Rs ].uw0 ) * ( (u64) r->GPR [ i.Rt ].uw0 );
	r->GPR [ i.Rd ].uq1 = ( (u64) r->GPR [ i.Rs ].uw2 ) * ( (u64) r->GPR [ i.Rt ].uw2 );
	
	r->LO.sq0 = (s32) ( r->GPR [ i.Rd ].sq0 );
	r->LO.sq1 = (s32) ( r->GPR [ i.Rd ].sq1 );
	r->HI.sq0 = ( r->GPR [ i.Rd ].sq0 >> 32 );
	r->HI.sq1 = ( r->GPR [ i.Rd ].sq1 >> 32 );
	
#if defined INLINE_DEBUG_PMULTUW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}


static void Execute::PDIVW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PDIVW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->LO.sq0 = (s32) ( r->GPR [ i.Rs ].sw0 / r->GPR [ i.Rt ].sw0 );
	r->LO.sq1 = (s32) ( r->GPR [ i.Rs ].sw2 / r->GPR [ i.Rt ].sw2 );
	
	r->HI.sq0 = (s32) ( r->GPR [ i.Rs ].sw0 % r->GPR [ i.Rt ].sw0 );
	r->HI.sq1 = (s32) ( r->GPR [ i.Rs ].sw2 % r->GPR [ i.Rt ].sw2 );
	
#if defined INLINE_DEBUG_PDIVW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " LO=" << r->LO.uq0 << " " << r->LO.uq1 << " HI=" << r->HI.uq0 << " " << r->HI.uq1;
#endif
}

static void Execute::PDIVUW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PDIVUW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->LO.sq0 = (s32) ( r->GPR [ i.Rs ].uw0 / r->GPR [ i.Rt ].uw0 );
	r->LO.sq1 = (s32) ( r->GPR [ i.Rs ].uw2 / r->GPR [ i.Rt ].uw2 );
	
	r->HI.sq0 = (s32) ( r->GPR [ i.Rs ].uw0 % r->GPR [ i.Rt ].uw0 );
	r->HI.sq1 = (s32) ( r->GPR [ i.Rs ].uw2 % r->GPR [ i.Rt ].uw2 );
	
#if defined INLINE_DEBUG_PDIVUW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " LO=" << r->LO.uq0 << " " << r->LO.uq1 << " HI=" << r->HI.uq0 << " " << r->HI.uq1;
#endif
}

static void Execute::PDIVBW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PDIVBW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	r->LO.sw0 = ( r->GPR [ i.Rs ].sw0 / r->GPR [ i.Rt ].sh0 );
	r->LO.sw1 = ( r->GPR [ i.Rs ].sw1 / r->GPR [ i.Rt ].sh0 );
	r->LO.sw2 = ( r->GPR [ i.Rs ].sw2 / r->GPR [ i.Rt ].sh0 );
	r->LO.sw3 = ( r->GPR [ i.Rs ].sw3 / r->GPR [ i.Rt ].sh0 );
	
	r->HI.sw0 = (s16) ( r->GPR [ i.Rs ].sw0 % r->GPR [ i.Rt ].sh0 );
	r->HI.sw1 = (s16) ( r->GPR [ i.Rs ].sw1 % r->GPR [ i.Rt ].sh0 );
	r->HI.sw2 = (s16) ( r->GPR [ i.Rs ].sw2 % r->GPR [ i.Rt ].sh0 );
	r->HI.sw3 = (s16) ( r->GPR [ i.Rs ].sw3 % r->GPR [ i.Rt ].sh0 );
	
#if defined INLINE_DEBUG_PDIVBW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " LO=" << r->LO.uq0 << " " << r->LO.uq1 << " HI=" << r->HI.uq0 << " " << r->HI.uq1;
#endif
}




static void Execute::PHMADH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PHMADH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}


static void Execute::PMSUBH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMSUBH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::PHMSBH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PHMSBH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}


static void Execute::PREVH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PREVH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif	

	u16 temp;

	// note: must use this order/method
	temp = r->GPR [ i.Rt ].uh3;
	r->GPR [ i.Rd ].uh3 = r->GPR [ i.Rt ].uh0;
	r->GPR [ i.Rd ].uh0 = temp;
	
	temp = r->GPR [ i.Rt ].uh2;
	r->GPR [ i.Rd ].uh2 = r->GPR [ i.Rt ].uh1;
	r->GPR [ i.Rd ].uh1 = temp;
	
	temp = r->GPR [ i.Rt ].uh7;
	r->GPR [ i.Rd ].uh7 = r->GPR [ i.Rt ].uh4;
	r->GPR [ i.Rd ].uh4 = temp;
	
	temp = r->GPR [ i.Rt ].uh6;
	r->GPR [ i.Rd ].uh6 = r->GPR [ i.Rt ].uh5;
	r->GPR [ i.Rd ].uh5 = temp;
	
#if defined INLINE_DEBUG_PREVH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}




static void Execute::PEXEH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PEXEH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	u16 temp;

	// note: must use this order/method
	r->GPR [ i.Rd ].uh1 = r->GPR [ i.Rt ].uh1;
	r->GPR [ i.Rd ].uh3 = r->GPR [ i.Rt ].uh3;
	r->GPR [ i.Rd ].uh5 = r->GPR [ i.Rt ].uh5;
	r->GPR [ i.Rd ].uh7 = r->GPR [ i.Rt ].uh7;
	
	temp = r->GPR [ i.Rt ].uh2;
	r->GPR [ i.Rd ].uh2 = r->GPR [ i.Rt ].uh0;
	r->GPR [ i.Rd ].uh0 = temp;
	
	temp = r->GPR [ i.Rt ].uh6;
	r->GPR [ i.Rd ].uh6 = r->GPR [ i.Rt ].uh4;
	r->GPR [ i.Rd ].uh4 = temp;
	
#if defined INLINE_DEBUG_PEXEH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PEXEW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PEXEW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	u32 temp;

	// note: must use this order/method
	r->GPR [ i.Rd ].uw1 = r->GPR [ i.Rt ].uw1;
	r->GPR [ i.Rd ].uw3 = r->GPR [ i.Rt ].uw3;
	
	temp = r->GPR [ i.Rt ].uw2;
	r->GPR [ i.Rd ].uw2 = r->GPR [ i.Rt ].uw0;
	r->GPR [ i.Rd ].uw0 = temp;
	
#if defined INLINE_DEBUG_PEXEW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PROT3W ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PROT3W || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	u32 temp;

	// note: must use this order/method
	r->GPR [ i.Rd ].uw3 = r->GPR [ i.Rt ].uw3;
	
	temp = r->GPR [ i.Rt ].uw2;
	r->GPR [ i.Rd ].uw2 = r->GPR [ i.Rt ].uw0;
	r->GPR [ i.Rd ].uw0 = r->GPR [ i.Rt ].uw1;
	r->GPR [ i.Rd ].uw1 = temp;
	
#if defined INLINE_DEBUG_PROT3W || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}



static void Execute::PMTHI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMTHI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif

	r->HI.uq0 = r->GPR [ i.Rd ].uq0;
	r->HI.uq1 = r->GPR [ i.Rd ].uq1;
	
#if defined INLINE_DEBUG_PMTHI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " HI=" << r->HI.uq0 << " " << r->HI.uq1;
#endif
}

static void Execute::PMTLO ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PMTLO || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif

	r->LO.uq0 = r->GPR [ i.Rd ].uq0;
	r->LO.uq1 = r->GPR [ i.Rd ].uq1;
	
#if defined INLINE_DEBUG_PMTLO || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " LO=" << r->LO.uq0 << " " << r->LO.uq1;
#endif
}



static void Execute::PCPYLD ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PCPYLD || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	// PCPYLD rd, rs, rt
	// note: must use this order
	r->GPR [ i.Rd ].uq1 = r->GPR [ i.Rs ].uq0;
	r->GPR [ i.Rd ].uq0 = r->GPR [ i.Rt ].uq0;
	
#if defined INLINE_DEBUG_PCPYLD || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PCPYUD ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PCPYUD || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rs=" << r->GPR [ i.Rs ].uq0 << " " << r->GPR [ i.Rs ].uq1;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	// PCPYUD rd, rs, rt
	// note: must use this order
	r->GPR [ i.Rd ].uq0 = r->GPR [ i.Rs ].uq1;
	r->GPR [ i.Rd ].uq1 = r->GPR [ i.Rt ].uq1;
	
#if defined INLINE_DEBUG_PCPYUD || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PCPYH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PCPYH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	// note: must use this order
	r->GPR [ i.Rd ].uh7 = r->GPR [ i.Rt ].uh4;
	r->GPR [ i.Rd ].uh6 = r->GPR [ i.Rt ].uh4;
	r->GPR [ i.Rd ].uh5 = r->GPR [ i.Rt ].uh4;
	r->GPR [ i.Rd ].uh4 = r->GPR [ i.Rt ].uh4;
	
	r->GPR [ i.Rd ].uh3 = r->GPR [ i.Rt ].uh0;
	r->GPR [ i.Rd ].uh2 = r->GPR [ i.Rt ].uh0;
	r->GPR [ i.Rd ].uh1 = r->GPR [ i.Rt ].uh0;
	r->GPR [ i.Rd ].uh0 = r->GPR [ i.Rt ].uh0;
	
#if defined INLINE_DEBUG_PCPYH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}


static void Execute::PEXCH ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PEXCH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	u16 temp;

	// note: must use this order/method
	r->GPR [ i.Rd ].uh0 = r->GPR [ i.Rt ].uh0;
	r->GPR [ i.Rd ].uh3 = r->GPR [ i.Rt ].uh3;
	
	temp = r->GPR [ i.Rt ].uh2;
	r->GPR [ i.Rd ].uh2 = r->GPR [ i.Rt ].uh1;
	r->GPR [ i.Rd ].uh1 = temp;
	
	r->GPR [ i.Rd ].uh4 = r->GPR [ i.Rt ].uh4;
	r->GPR [ i.Rd ].uh7 = r->GPR [ i.Rt ].uh7;
	
	temp = r->GPR [ i.Rt ].uh6;
	r->GPR [ i.Rd ].uh6 = r->GPR [ i.Rt ].uh5;
	r->GPR [ i.Rd ].uh5 = temp;
	
#if defined INLINE_DEBUG_PEXCH || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::PEXCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PEXCW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Rt=" << r->GPR [ i.Rt ].uq0 << " " << r->GPR [ i.Rt ].uq1;
#endif

	u32 temp;

	// note: must use this order/method
	r->GPR [ i.Rd ].uw0 = r->GPR [ i.Rt ].uw0;
	r->GPR [ i.Rd ].uw3 = r->GPR [ i.Rt ].uw3;
	
	temp = r->GPR [ i.Rt ].uw2;
	r->GPR [ i.Rd ].uw2 = r->GPR [ i.Rt ].uw1;
	r->GPR [ i.Rd ].uw1 = temp;
	
#if defined INLINE_DEBUG_PEXCW || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << hex << " Output:" << " Rd=" << r->GPR [ i.Rd ].uq0 << " " << r->GPR [ i.Rd ].uq1;
#endif
}

static void Execute::QFSRV ( Instruction::Format i )
{
#if defined INLINE_DEBUG_QFSRV || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED || defined INLINE_DEBUG_INTEGER_VECTOR
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif


}





// * R5900 COP0 instructions * //


static void Execute::EI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_EI || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( r->CPR0.Status.EDI || r->CPR0.Status.EXL || r->CPR0.Status.ERL || !r->CPR0.Status.KSU )
	{
		r->CPR0.Status.EIE = 1;
	}
}

static void Execute::DI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DI || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( r->CPR0.Status.EDI || r->CPR0.Status.EXL || r->CPR0.Status.ERL || !r->CPR0.Status.KSU )
	{
		r->CPR0.Status.EIE = 0;
	}

}

static void Execute::CFC0 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_CFC0 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	r->GPR [ i.Rt ].sq0 = (s32) r->CPC0 [ i.Rd ];

}

static void Execute::CTC0 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_CTC0 || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	r->CPC0 [ i.Rd ] = r->GPR [ i.Rt ].sw0;
}




static void Execute::SYNC ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SYNC || defined INLINE_DEBUG_R5900
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}


static void Execute::CACHE ( Instruction::Format i )
{
#if defined INLINE_DEBUG_CACHE || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::PREF ( Instruction::Format i )
{
#if defined INLINE_DEBUG_PREF || defined INLINE_DEBUG_R5900 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::TLBR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TLBR || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::TLBWI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TLBWI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::TLBWR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TLBWR || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::TLBP ( Instruction::Format i )
{
#if defined INLINE_DEBUG_TLBP || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::ERET ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ERET || defined INLINE_DEBUG_R5900
	if ( r->DebugNextERET )
	{
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	}
#endif

	if ( r->CPR0.Status.ERL )
	{
		r->NextPC = r->CPR0.ErrorEPC;
		r->CPR0.Status.ERL = 0;
		
#if defined INLINE_DEBUG_ERET || defined INLINE_DEBUG_R5900
	if ( r->DebugNextERET )
	{
	debug << "; Status.ERL=1;";
	debug << "; ErrorEPC=" << r->CPR0.ErrorEPC;
	debug << "; NextPC=" << r->NextPC;
	debug << "\r\nR29=" << r->GPR [ 29 ].uw0 << " R31=" << r->GPR [ 31 ].uw0 << " 0x3201d0=" << r->Bus->MainMemory.b32 [ 0x3201d0 >> 2 ];
	r->DebugNextERET = 0;
	}
#endif
	}
	else
	{
		r->NextPC = r->CPR0.EPC;
		r->CPR0.Status.EXL = 0;
		
#if defined INLINE_DEBUG_ERET || defined INLINE_DEBUG_R5900
	if ( r->DebugNextERET )
	{
	debug << "; Status.ERL=0;";
	debug << "; EPC=" << r->CPR0.EPC;
	debug << "; NextPC=" << r->NextPC;
	debug << "\r\nR29=" << r->GPR [ 29 ].uw0 << " R31=" << r->GPR [ 31 ].uw0 << " 0x3201d0=" << r->Bus->MainMemory.b32 [ 0x3201d0 >> 2 ];
	r->DebugNextERET = 0;
	}
#endif
	}
	
	// interrupt status changed
	r->UpdateInterrupt ();
	
}

// R5900 does not have RFE instruction //
//static void Execute::RFE ( Instruction::Format i )
//{
//	strInstString << "RFE";
//	AddInstArgs ( strInstString, instruction, FTRFE );
//}


static void Execute::DERET ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DERET || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::WAIT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_WAIT || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}





// * COP1 (floating point) instructions * //


static void Execute::MFC1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MFC1 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU		// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << r->CPR1 [ i.Fs ].u;
#endif

	// no delay slot on R5900?
	r->GPR [ i.Rt ].sq0 = (s32) r->CPR1 [ i.Rd ].s;
	
#if defined INLINE_DEBUG_MFC1 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU		// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Rt=" << r->GPR [ i.Rt ].sq0;
#endif
}

static void Execute::MTC1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MTC1 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU		// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Rt=" << r->GPR [ i.Rt ].sw0;
#endif

	// no delay slot on R5900?
	r->CPR1 [ i.Rd ].s = r->GPR [ i.Rt ].sw0;
	
#if defined INLINE_DEBUG_MTC1 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU		// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fs=" << r->CPR1 [ i.Fs ].u;
#endif
}

static void Execute::CFC1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_CFC1 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " CFs=" << r->CPC1 [ i.Rd ];
#endif

	// no delay slot on R5900?
	//r->GPR [ i.Rt ].sq0 = (s32) r->CPC1 [ i.Rd ];
	r->GPR [ i.Rt ].sq0 = (s32) r->Read_CFC1 ( i.Rd );
	
#if defined INLINE_DEBUG_CFC1 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU
	debug << " Output: Rt=" << r->GPR [ i.Rt ].sq0;
#endif
}

static void Execute::CTC1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_CTC1 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Rt=" << r->GPR [ i.Rt ].sw0;
#endif

	// no delay slot on R5900?
	//r->CPC1 [ i.Rd ] = r->GPR [ i.Rt ].sw0;
	r->Write_CTC1 ( i.Rd, r->GPR [ i.Rt ].sw0 );
	
#if defined INLINE_DEBUG_CTC1 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU
	debug << " Output: CFs=" << r->CPC1 [ i.Rd ];
#endif
}


static void Execute::LWC1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LWC1 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: base = " << r->GPR [ i.Base ].u;
#endif

	// LWC1 rt, offset(base)
	
	u32 LoadAddress;
	
	// set load to happen after delay slot
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	// *** testing *** alert if load is from unaligned address
	if ( LoadAddress & 0x3 )
	{
		cout << "\nhps1x64 ALERT: LoadAddress is unaligned for LW @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
		return;
	}
	
	
	// ***todo*** perform signed load of word
	r->CPR1 [ i.Rt ].u = (s32) r->Bus->Read ( LoadAddress, 0xffffffff );
	
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
	
#if defined INLINE_DEBUG_LWC1 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU
	debug << " Output: C1=" << r->CPR1 [ i.Rt ].u << " " << r->CPR1 [ i.Rt ].f;
#endif
}

static void Execute::SWC1 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SWC1 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "; Input: rt = " << r->CPR1 [ i.Rt ].u << " " << r->CPR1 [ i.Rt ].f << "; base = " << r->GPR [ i.Base ].u;
#endif

	// SWC1 rt, offset(base)
	u32 StoreAddress;
	
	// check if storing to data cache
	//StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	StoreAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
	// *** testing *** alert if load is from unaligned address
	if ( StoreAddress & 0x3 )
	{
		cout << "\nhps2x64 ALERT: StoreAddress is unaligned for SW @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADES );
		return;
	}
	
	// clear top 3 bits since there is no data cache for caching stores
	// don't clear top 3 bits since scratchpad is at 0x70000000
	//StoreAddress &= 0x1fffffff;
	
	
	// ***todo*** perform store of word
	r->Bus->Write ( StoreAddress, r->CPR1 [ i.Rt ].u, 0xffffffff );
	
	

	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
	
#ifdef INLINE_DEBUG_TRACE
	TRACE_VALUE ( r->CPR1 [ i.Rt ].u )
#endif
}





static void Execute::ABS_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ABS_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
#endif

	r->CPR1 [ i.Fd ].u = r->CPR1 [ i.Fs ].u & 0x7fffffff;
	
	// flags affected:
	// clears flags o,u (bits 14,15)
	r->CPC1 [ 31 ] &= ~0x0000c000;

#if defined INLINE_DEBUG_ABS_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
#endif
}


static void Execute::ADD_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ADD_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
#endif

	//u32 OverflowFlag, UnderflowFlag, OverflowSticky, UnderflowSticky, DummyFlag, FlagSet;
	s16 StatusFlag = 0, DummyMACFlag;
	u32 FlagSet;

	// fd = fs + ft
	//r->CPR1 [ i.Fd ].f = r->CPR1 [ i.Fs ].f + r->CPR1 [ i.Ft ].f;
	r->CPR1 [ i.Fd ].f = PS2_Float_Add ( r->CPR1 [ i.Fs ].f, r->CPR1 [ i.Ft ].f, 0, & StatusFlag, & DummyMACFlag );
										
	// clear bits 14, 15 in flag register
	r->CPC1 [ 31 ] &= ~0x0000c000;
	
	// get flags to set
	FlagSet = StatusFlag & 0xc;
	FlagSet = ( FlagSet << 12 ) | ( FlagSet << 1 );

	// overflow flag is bit 15
	// underflow flag is bit 14
	// sticky overflow flag is bit 4
	// sticky underflow flag is bit 3
	r->CPC1 [ 31 ] |= FlagSet;

#if defined INLINE_DEBUG_ADD_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
	debug << " FlagSet=" << hex << FlagSet;
#endif
}


static void Execute::ADDA_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDA_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
#endif

	//u32 OverflowFlag, UnderflowFlag, OverflowSticky, UnderflowSticky, DummyFlag, FlagSet;
	s16 StatusFlag = 0, DummyMACFlag;
	u32 FlagSet;
	
	// ACC = fs + ft
	//r->ACC.f = r->CPR1 [ i.Fs ].f + r->CPR1 [ i.Ft ].f;
	r->dACC.d = PS2_Float_AddA ( r->CPR1 [ i.Fs ].f, r->CPR1 [ i.Ft ].f, 0, & StatusFlag, & DummyMACFlag );
										
	// clear bits 14, 15 in flag register
	r->CPC1 [ 31 ] &= ~0x0000c000;
	
	// get flags to set
	FlagSet = StatusFlag & 0xc;
	FlagSet = ( FlagSet << 12 ) | ( FlagSet << 1 );

	// overflow flag is bit 15
	// underflow flag is bit 14
	// sticky overflow flag is bit 4
	// sticky underflow flag is bit 3
	r->CPC1 [ 31 ] |= FlagSet;

#if defined INLINE_DEBUG_ADDA_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC=" << hex << r->dACC.l << " " << r->dACC.d;
	debug << " FlagSet=" << hex << FlagSet;
#endif
}



static void Execute::CVT_S_W ( Instruction::Format i )
{
#if defined INLINE_DEBUG_CVT_S_W || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
#endif

	// fd = FLOAT ( fs )
	r->CPR1 [ i.Fd ].f = (float) r->CPR1 [ i.Fs ].s;
	
#if defined INLINE_DEBUG_CVT_S_W || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
#endif
}


static void Execute::SUB_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SUB_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
#endif

	//u32 OverflowFlag, UnderflowFlag, OverflowSticky, UnderflowSticky, DummyFlag, FlagSet;
	s16 StatusFlag = 0, DummyMACFlag;
	u32 FlagSet;
	
	// fd = fs - ft
	//r->CPR1 [ i.Fd ].f = r->CPR1 [ i.Fs ].f - r->CPR1 [ i.Ft ].f;
	r->CPR1 [ i.Fd ].f = PS2_Float_Sub ( r->CPR1 [ i.Fs ].f, r->CPR1 [ i.Ft ].f, 0, & StatusFlag, & DummyMACFlag );
										
	// clear bits 14, 15 in flag register
	r->CPC1 [ 31 ] &= ~0x0000c000;
	
	// get flags to set
	FlagSet = StatusFlag & 0xc;
	FlagSet = ( FlagSet << 12 ) | ( FlagSet << 1 );

	// overflow flag is bit 15
	// underflow flag is bit 14
	// sticky overflow flag is bit 4
	// sticky underflow flag is bit 3
	r->CPC1 [ 31 ] |= FlagSet;

#if defined INLINE_DEBUG_SUB_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
	debug << " FlagSet=" << hex << FlagSet;
#endif
}

static void Execute::MUL_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MUL_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
#endif

	//u32 OverflowFlag, UnderflowFlag, OverflowSticky, UnderflowSticky, DummyFlag, FlagSet;
	s16 StatusFlag = 0, DummyMACFlag;
	u32 FlagSet;
	
	// fd = fs * ft
	//r->CPR1 [ i.Fd ].f = r->CPR1 [ i.Fs ].f * r->CPR1 [ i.Ft ].f;
	r->CPR1 [ i.Fd ].f = PS2_Float_Mul ( r->CPR1 [ i.Fs ].f, r->CPR1 [ i.Ft ].f, 0, & StatusFlag, & DummyMACFlag );
										
	// clear bits 14, 15 in flag register
	r->CPC1 [ 31 ] &= ~0x0000c000;
	
	// get flags to set
	FlagSet = StatusFlag & 0xc;
	FlagSet = ( FlagSet << 12 ) | ( FlagSet << 1 );

	// overflow flag is bit 15
	// underflow flag is bit 14
	// sticky overflow flag is bit 4
	// sticky underflow flag is bit 3
	r->CPC1 [ 31 ] |= FlagSet;

#if defined INLINE_DEBUG_MUL_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
	debug << " FlagSet=" << hex << FlagSet;
#endif
}

static void Execute::MULA_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MULA_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
	debug << " ACC=" << hex << r->dACC.l << " " << r->dACC.d;
#endif

	//u32 OverflowFlag, UnderflowFlag, OverflowSticky, UnderflowSticky, DummyFlag, FlagSet;
	s16 StatusFlag = 0, DummyMACFlag;
	u32 FlagSet;
	
	// ACC = fs * ft
	//r->ACC.f = r->CPR1 [ i.Fs ].f * r->CPR1 [ i.Ft ].f;
	r->dACC.d = PS2_Float_MulA ( r->CPR1 [ i.Fs ].f, r->CPR1 [ i.Ft ].f, 0, & StatusFlag, & DummyMACFlag );
										
	// clear bits 14, 15 in flag register
	r->CPC1 [ 31 ] &= ~0x0000c000;
	
	// get flags to set
	FlagSet = StatusFlag & 0xc;
	FlagSet = ( FlagSet << 12 ) | ( FlagSet << 1 );

	// overflow flag is bit 15
	// underflow flag is bit 14
	// sticky overflow flag is bit 4
	// sticky underflow flag is bit 3
	r->CPC1 [ 31 ] |= FlagSet;

#if defined INLINE_DEBUG_MULA_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC=" << hex << r->dACC.l << " " << r->dACC.d;
	debug << " FlagSet=" << hex << FlagSet;
#endif
}


static void Execute::DIV_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_DIV_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
#endif

	s16 StatusFlag = 0;
	u32 FlagSet;

	// fd = fs / ft
	//r->CPR1 [ i.Fd ].f = r->CPR1 [ i.Fs ].f / r->CPR1 [ i.Ft ].f;
	r->CPR1 [ i.Fd ].f = PS2_Float_Div ( r->CPR1 [ i.Fs ].f, r->CPR1 [ i.Ft ].f, & StatusFlag );
	
	// clear non-sticky invalid/divide flags (bits 16,17)
	r->CPC1 [ 31 ] &= ~0x00030000;
	
	// ***TODO*** set flags
	// sticky divide flag is bit 5
	// sticky invalid flag is bit 6
	// divide flag is bit 16
	// invalid flag is bit 17
	
	// swap bits..
	FlagSet = ( StatusFlag & 0x10 ) | ( ( StatusFlag & 0x20 ) >> 2 );
	FlagSet = ( FlagSet << 13 ) | ( FlagSet << 2 );
	
	// set flags
	r->CPC1 [ 31 ] |= FlagSet;

#if defined INLINE_DEBUG_DIV_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
	debug << " FlagSet=" << hex << FlagSet;
#endif
}

static void Execute::SQRT_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SQRT_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
#endif

	s16 StatusFlag = 0;
	u32 FlagSet;

	// fd = sqrt ( ft )
	r->CPR1 [ i.Fd ].f = PS2_Float_Sqrt ( r->CPR1 [ i.Ft ].f, & StatusFlag );
	
	// clear the affected non-sticky flags (bits 16,17)
	r->CPC1 [ 31 ] &= ~0x00030000;
	
	// ***TODO*** set flags
	// get invalid flag
	FlagSet = StatusFlag & 0x10;
	FlagSet = ( FlagSet << 13 ) | ( FlagSet << 2 );
	
	// set the flags
	r->CPC1 [ 31 ] |= FlagSet;
	
#if defined INLINE_DEBUG_SQRT_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
	debug << " FlagSet=" << hex << FlagSet;
#endif
}


static void Execute::MOV_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MOV_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
#endif

	r->CPR1 [ i.Fd ].u = r->CPR1 [ i.Fs ].u;
	
	// flags affected: none

#if defined INLINE_DEBUG_MOV_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
#endif
}

static void Execute::NEG_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_NEG_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
#endif

	r->CPR1 [ i.Fd ].u = r->CPR1 [ i.Fs ].u ^ 0x80000000;
	
	// flags affected:
	// clears flags o,u (bits 14,15)
	r->CPC1 [ 31 ] &= ~0x0000c000;

#if defined INLINE_DEBUG_NEG_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
#endif
}

static void Execute::RSQRT_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_RSQRT_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
#endif

	s16 StatusFlag = 0;
	u32 FlagSet;
	
	r->CPR1 [ i.Fd ].f = PS2_Float_RSqrt ( r->CPR1 [ i.Fs ].f, r->CPR1 [ i.Ft ].f, & StatusFlag );
	
	// ***todo*** flags
	// swap bits..
	FlagSet = ( StatusFlag & 0x10 ) | ( ( StatusFlag & 0x20 ) >> 2 );
	FlagSet = ( FlagSet << 13 ) | ( FlagSet << 2 );
	
	// set flags
	r->CPC1 [ 31 ] |= FlagSet;
	
#if defined INLINE_DEBUG_RSQRT_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
	debug << " FlagSet=" << hex << FlagSet;
#endif
}


static void Execute::SUBA_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBA_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
	debug << " ACC=" << hex << r->dACC.l << " " << r->dACC.d;
#endif

	//u32 OverflowFlag, UnderflowFlag, OverflowSticky, UnderflowSticky, DummyFlag, FlagSet;
	s16 StatusFlag = 0, DummyMACFlag;
	u32 FlagSet;
	
	r->dACC.d = PS2_Float_SubA ( r->CPR1 [ i.Fs ].f, r->CPR1 [ i.Ft ].f, 0, & StatusFlag, & DummyMACFlag );
										
	// clear bits 14, 15 in flag register
	r->CPC1 [ 31 ] &= ~0x0000c000;
	
	// get flags to set
	FlagSet = StatusFlag & 0xc;
	FlagSet = ( FlagSet << 12 ) | ( FlagSet << 1 );

	// overflow flag is bit 15
	// underflow flag is bit 14
	// sticky overflow flag is bit 4
	// sticky underflow flag is bit 3
	r->CPC1 [ 31 ] |= FlagSet;
	
#if defined INLINE_DEBUG_SUBA_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " ACC=" << hex << r->dACC.l << " " << r->dACC.d;
	debug << " FlagSet=" << hex << FlagSet;
#endif
}

static void Execute::MADD_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MADD_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
	debug << " ACC=" << hex << r->dACC.l << " " << r->dACC.d;
#endif

	//u32 OverflowFlag, UnderflowFlag, OverflowSticky, UnderflowSticky, DummyFlag, FlagSet;
	s16 StatusFlag = 0, DummyMACFlag;
	u32 FlagSet;
	
	float fTemp;

	// fd = fs * ft + ACC
	//fTemp = r->CPR1 [ i.Fs ].f * r->CPR1 [ i.Ft ].f;
	//r->CPR1 [ i.Fd ].f = fTemp + r->ACC.f;
	r->CPR1 [ i.Fd ].f = PS2_Float_Madd ( r->dACC.d, r->CPR1 [ i.Fs ].f, r->CPR1 [ i.Ft ].f, 0, & StatusFlag, & DummyMACFlag );
										
	// clear bits 14, 15 in flag register
	r->CPC1 [ 31 ] &= ~0x0000c000;
	
	// get flags to set
	FlagSet = StatusFlag & 0xc;
	FlagSet = ( FlagSet << 12 ) | ( FlagSet << 1 );

	// overflow flag is bit 15
	// underflow flag is bit 14
	// sticky overflow flag is bit 4
	// sticky underflow flag is bit 3
	r->CPC1 [ 31 ] |= FlagSet;

#if defined INLINE_DEBUG_MADD_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
	debug << " FlagSet=" << hex << FlagSet;
#endif
}

static void Execute::MSUB_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUB_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
	debug << " ACC=" << hex << r->dACC.l << " " << r->dACC.d;
#endif

	//u32 OverflowFlag, UnderflowFlag, OverflowSticky, UnderflowSticky, DummyFlag, FlagSet;
	s16 StatusFlag = 0, DummyMACFlag;
	u32 FlagSet;
	
	r->CPR1 [ i.Fd ].f = PS2_Float_Msub ( r->dACC.d, r->CPR1 [ i.Fs ].f, r->CPR1 [ i.Ft ].f, 0, & StatusFlag, & DummyMACFlag );
										
	// clear bits 14, 15 in flag register
	r->CPC1 [ 31 ] &= ~0x0000c000;
	
	// get flags to set
	FlagSet = StatusFlag & 0xc;
	FlagSet = ( FlagSet << 12 ) | ( FlagSet << 1 );

	// overflow flag is bit 15
	// underflow flag is bit 14
	// sticky overflow flag is bit 4
	// sticky underflow flag is bit 3
	r->CPC1 [ 31 ] |= FlagSet;
	
#if defined INLINE_DEBUG_MSUB_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
	debug << " FlagSet=" << hex << FlagSet;
#endif
}

static void Execute::MSUBA_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBA_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
	debug << " ACC=" << hex << r->dACC.l << " " << r->dACC.d;
#endif

	//u32 OverflowFlag, UnderflowFlag, OverflowSticky, UnderflowSticky, DummyFlag, FlagSet;
	s16 StatusFlag = 0, DummyMACFlag;
	u32 FlagSet;
	
	r->dACC.d = PS2_Float_MsubA ( r->dACC.d, r->CPR1 [ i.Fs ].f, r->CPR1 [ i.Ft ].f, 0, & StatusFlag, & DummyMACFlag );
										
	// clear bits 14, 15 in flag register
	r->CPC1 [ 31 ] &= ~0x0000c000;
	
	// get flags to set
	FlagSet = StatusFlag & 0xc;
	FlagSet = ( FlagSet << 12 ) | ( FlagSet << 1 );

	// overflow flag is bit 15
	// underflow flag is bit 14
	// sticky overflow flag is bit 4
	// sticky underflow flag is bit 3
	r->CPC1 [ 31 ] |= FlagSet;
	
#if defined INLINE_DEBUG_MSUBA_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " ACC=" << hex << r->dACC.l << " " << r->dACC.d;
	debug << " FlagSet=" << hex << FlagSet;
#endif
}

static void Execute::MADDA_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDA_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
	debug << " ACC=" << hex << r->dACC.l << " " << r->dACC.d;
#endif

	//u32 OverflowFlag, UnderflowFlag, OverflowSticky, UnderflowSticky, DummyFlag, FlagSet;
	s16 StatusFlag = 0, DummyMACFlag;
	u32 FlagSet;
	
	r->dACC.d = PS2_Float_MaddA ( r->dACC.d, r->CPR1 [ i.Fs ].f, r->CPR1 [ i.Ft ].f, 0, & StatusFlag, & DummyMACFlag );
										
	// clear bits 14, 15 in flag register
	r->CPC1 [ 31 ] &= ~0x0000c000;
	
	// get flags to set
	FlagSet = StatusFlag & 0xc;
	FlagSet = ( FlagSet << 12 ) | ( FlagSet << 1 );

	// overflow flag is bit 15
	// underflow flag is bit 14
	// sticky overflow flag is bit 4
	// sticky underflow flag is bit 3
	r->CPC1 [ 31 ] |= FlagSet;
	
#if defined INLINE_DEBUG_MADDA_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " ACC=" << hex << r->dACC.l << " " << r->dACC.d;
	debug << " FlagSet=" << hex << FlagSet;
#endif
}

static void Execute::CVT_W_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_CVT_W_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
#endif

	// fd = INT ( fs )
	r->CPR1 [ i.Fd ].s = (s32) r->CPR1 [ i.Fs ].f;
	
#if defined INLINE_DEBUG_CVT_W_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
#endif
}

static void Execute::MAX_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MAX_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
#endif

	// fd = MAX ( fs, ft )
	r->CPR1 [ i.Fd ].f = ( ( r->CPR1 [ i.Fs ].f >= r->CPR1 [ i.Ft ].f ) ? r->CPR1 [ i.Fs ].f : r->CPR1 [ i.Ft ].f );

	// flags affected:
	// clears flags o,u (bits 14,15)
	r->CPC1 [ 31 ] &= ~0x0000c000;
	
#if defined INLINE_DEBUG_MAX_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
#endif
}

static void Execute::MIN_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_MIN_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
#endif

	// fd = MIN ( fs, ft )
	r->CPR1 [ i.Fd ].f = ( ( r->CPR1 [ i.Fs ].f <= r->CPR1 [ i.Ft ].f ) ? r->CPR1 [ i.Fs ].f : r->CPR1 [ i.Ft ].f );

	// flags affected:
	// clears flags o,u (bits 14,15)
	r->CPC1 [ 31 ] &= ~0x0000c000;
	
#if defined INLINE_DEBUG_MIN_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << hex << r->CPR1 [ i.Fd ].u << " " << r->CPR1 [ i.Fd ].f;
#endif
}

static void Execute::C_F_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_C_F_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// clears bit 23 in FCR31
	r->CPC1 [ 31 ] &= ~0x00800000;
	
#if defined INLINE_DEBUG_C_F_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: FCR31=" << hex << r->CPC1 [ 31 ];
#endif
}

static void Execute::C_EQ_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_C_EQ_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
#endif

	if ( r->CPR1 [ i.Fs ].f == r->CPR1 [ i.Ft ].f )
	{
#if defined INLINE_DEBUG_C_EQ_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU
	debug << " EQUAL";
#endif

		r->CPC1 [ 31 ] |= 0x00800000;
	}
	else
	{
#if defined INLINE_DEBUG_C_EQ_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU
	debug << " NOTEQUAL";
#endif

		r->CPC1 [ 31 ] &= ~0x00800000;
	}
	
#if defined INLINE_DEBUG_C_EQ_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: FCR31=" << hex << r->CPC1 [ 31 ];
#endif
}

static void Execute::C_LT_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_C_LT_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
#endif

	// Cond = fs < ft
	// ***todo*** handle non-ieee values for this too
	if ( r->CPR1 [ i.Fs ].f < r->CPR1 [ i.Ft ].f )
	{
#if defined INLINE_DEBUG_C_LT_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU
	debug << " LESS";
#endif

		r->CPC1 [ 31 ] |= 0x00800000;
	}
	else
	{
#if defined INLINE_DEBUG_C_LT_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU
	debug << " NOTLESS";
#endif

		r->CPC1 [ 31 ] &= ~0x00800000;
	}
	
#if defined INLINE_DEBUG_C_LT_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: FCR31=" << hex << r->CPC1 [ 31 ];
#endif
}

static void Execute::C_LE_S ( Instruction::Format i )
{
#if defined INLINE_DEBUG_C_LE_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << r->CPR1 [ i.Fs ].u << " " << r->CPR1 [ i.Fs ].f;
	debug << " Ft=" << hex << r->CPR1 [ i.Ft ].u << " " << r->CPR1 [ i.Ft ].f;
#endif

	if ( r->CPR1 [ i.Fs ].f <= r->CPR1 [ i.Ft ].f )
	{
#if defined INLINE_DEBUG_C_LE_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU
	debug << " LESSEQ";
#endif

		r->CPC1 [ 31 ] |= 0x00800000;
	}
	else
	{
#if defined INLINE_DEBUG_C_LE_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU
	debug << " NOTLESSEQ";
#endif

		r->CPC1 [ 31 ] &= ~0x00800000;
	}
	
#if defined INLINE_DEBUG_C_LE_S || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: FCR31=" << hex << r->CPC1 [ 31 ];
#endif
}



// * COP2 (VU0) instrutions * //



// PS2 has LQC2/SQC2 instead of LWC2/SWC2 //
static void Execute::LQC2 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_LQC2 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Base=" << r->GPR [ i.Base ].sw0;
#endif

	// LQC2 ft, offset(base)
	
	u32 LoadAddress;
	u64* Data;
	
	// set load to happen after delay slot
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	LoadAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
#if defined INLINE_DEBUG_LQC2 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " LoadAddress=" << LoadAddress;
#endif

	// *** testing *** alert if load is from unaligned address
	if ( LoadAddress & 0xf )
	{
#if defined INLINE_DEBUG_LQC2 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
		debug << " UNALIGNED";
#endif

		cout << "\nhps2x64 ALERT: LoadAddress is unaligned for LW @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << LoadAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
		return;
	}
	
	// bottom four bits of Address are cleared
	//LoadAddress &= ~0xf;
	
	// ***todo*** perform signed load of word
	//Data = (u64*) r->Bus->Read ( LoadAddress, 0 );
	//r->GPR [ i.Rt ].uLo = Data [ 1 ];
	//r->GPR [ i.Rt ].uHi = Data [ 0 ];
	Data = (u64*) r->Bus->Read ( LoadAddress, 0 );

	VU0::_VU0->vf [ i.Ft ].uq0 = Data [ 0 ];
	VU0::_VU0->vf [ i.Ft ].uq1 = Data [ 1 ];
	
	
	// used for debugging
	r->Last_ReadAddress = LoadAddress;
	r->Last_ReadWriteAddress = LoadAddress;
	
#if defined INLINE_DEBUG_LQC2 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output:" << " ft=" << VU0::_VU0->vf [ i.Ft ].sq0 << " " << VU0::_VU0->vf [ i.Ft ].sq1;
#endif
}

static void Execute::SQC2 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_SQC2 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Base=" << r->GPR [ i.Base ].sw0;
	debug << " ft=" << VU0::_VU0->vf [ i.Ft ].sq0 << " " << VU0::_VU0->vf [ i.Ft ].sq1;
#endif

	// SQC2 ft, offset(base)
	u32 StoreAddress;
	
	// check if storing to data cache
	//StoreAddress = r->GPR [ i.Base ].s + i.sOffset;
	StoreAddress = r->GPR [ i.Base ].sw0 + i.sOffset;
	
#if defined INLINE_DEBUG_SQC2 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " StoreAddress=" << StoreAddress;
#endif

	// *** testing *** alert if load is from unaligned address
	if ( StoreAddress & 0xf )
	{
#if defined INLINE_DEBUG_SQC2 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
		debug << " UNALIGNED";
#endif

		cout << "\nhps2x64 ALERT: StoreAddress is unaligned for SW @ cycle=" << dec << r->CycleCount << " PC=" << hex << r->PC << " Address=" << StoreAddress << "\n";
		
		// *** testing ***
		r->ProcessSynchronousInterrupt ( Cpu::EXC_ADES );
		return;
	}
	
	// clear top 3 bits since there is no data cache for caching stores
	// don't clear top 3 bits since scratchpad is at 0x70000000
	//StoreAddress &= 0x1fffffff;
	
	// bottom four bits of Address are cleared
	//StoreAddress &= ~0xf;
	
	// ***todo*** perform store of word
	// *note* probably want to pass a pointer to hi part, since that is in lower area of memory
	//r->Bus->Write ( StoreAddress, & ( r->CPR2 [ i.Rt ].uHi ), 0 );
	//r->Bus->Write ( StoreAddress, & ( r->CPR2 [ i.Rt ].uw0 ), 0 );
	r->Bus->Write ( StoreAddress, & ( VU0::_VU0->vf [ i.Ft ].uw0 ), 0 );
	

	// used for debugging
	r->Last_WriteAddress = StoreAddress;
	r->Last_ReadWriteAddress = StoreAddress;
}


static void Execute::QMFC2_NI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_QMFC2_NI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Rd/Fs=" << VU0::_VU0->vf [ i.Rd ].sq0 << " " << VU0::_VU0->vf [ i.Rd ].sq1;
#endif

	// QMFC2.NI rt, fd -> fd is actually rd
	// NI -> no interlocking. does not wait for previous VCALLMS to complete

	//r->GPR [ i.Rt ].sq0 = r->CPR2 [ i.Fd ].sq0;
	//r->GPR [ i.Rt ].sq1 = r->CPR2 [ i.Fd ].sq1;
	
	r->GPR [ i.Rt ].sq0 = VU0::_VU0->vf [ i.Rd ].sq0;
	r->GPR [ i.Rt ].sq1 = VU0::_VU0->vf [ i.Rd ].sq1;
	
#if defined INLINE_DEBUG_QMFC2_NI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output:" << " Rt=" << r->GPR [ i.Rt ].sq0 << " " << r->GPR [ i.Rt ].sq1;
#endif
}

static void Execute::QMFC2_I ( Instruction::Format i )
{
#if defined INLINE_DEBUG_QMFC2_I || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Rd/Fs=" << VU0::_VU0->vf [ i.Rd ].sq0 << " " << VU0::_VU0->vf [ i.Rd ].sq1;
#endif

	// QMFC2.I rt, fd
	// I -> interlocking. waits for previous VCALLMS to complete
	// ***todo*** implement interlocking

	//r->GPR [ i.Rt ].sq0 = r->CPR2 [ i.Fd ].sq0;
	//r->GPR [ i.Rt ].sq1 = r->CPR2 [ i.Fd ].sq1;
	
	r->GPR [ i.Rt ].sq0 = VU0::_VU0->vf [ i.Rd ].sq0;
	r->GPR [ i.Rt ].sq1 = VU0::_VU0->vf [ i.Rd ].sq1;
	
#if defined INLINE_DEBUG_QMFC2_I || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output:" << " Rt=" << r->GPR [ i.Rt ].sq0 << " " << r->GPR [ i.Rt ].sq1;
#endif
}

static void Execute::QMTC2_NI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_QMTC2_NI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Rt=" << r->GPR [ i.Rt ].uw0 << " " << r->GPR [ i.Rt ].uw1 << " " << r->GPR [ i.Rt ].uw2 << " " << r->GPR [ i.Rt ].uw3;
#endif

	// QMTC2.NI rt, fd
	// NI -> no interlocking. does not wait for previous VCALLMS to complete

	//r->CPR2 [ i.Rt ].sq0 = r->GPR [ i.Fd ].sq0;
	//r->CPR2 [ i.Rt ].sq1 = r->GPR [ i.Fd ].sq1;
	VU0::_VU0->vf [ i.Rd ].sq0 = r->GPR [ i.Rt ].sq0;
	VU0::_VU0->vf [ i.Rd ].sq1 = r->GPR [ i.Rt ].sq1;
	
#if defined INLINE_DEBUG_QMTC2_NI || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output:" << " Rd/Fs=" << VU0::_VU0->vf [ i.Rd ].sq0 << " " << VU0::_VU0->vf [ i.Rd ].sq1;
#endif
}

static void Execute::QMTC2_I ( Instruction::Format i )
{
#if defined INLINE_DEBUG_QMTC2_I || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 //	 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Rt=" << r->GPR [ i.Rt ].uw0 << " " << r->GPR [ i.Rt ].uw1 << " " << r->GPR [ i.Rt ].uw2 << " " << r->GPR [ i.Rt ].uw3;
#endif

	// QMTC2.I rt, fd
	// I -> interlocking. waits for previous VCALLMS to complete
	// ***todo*** implement interlocking

	//r->CPR2 [ i.Rt ].sq0 = r->GPR [ i.Rd ].sq0;
	//r->CPR2 [ i.Rt ].sq1 = r->GPR [ i.Rd ].sq1;
	VU0::_VU0->vf [ i.Rd ].sq0 = r->GPR [ i.Rt ].sq0;
	VU0::_VU0->vf [ i.Rd ].sq1 = r->GPR [ i.Rt ].sq1;
	
#if defined INLINE_DEBUG_QMTC2_I || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output:" << " Rd/Fs=" << VU0::_VU0->vf [ i.Rd ].sq0 << " " << VU0::_VU0->vf [ i.Rd ].sq1;
#endif
}


static void Execute::COP2 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_COP2 || defined INLINE_DEBUG_R5900 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}




// VABS //

static void Execute::VABS ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VABS || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ABS ( VU0::_VU0, (Vu::Instruction::Format&) i );
}


// VADD //

static void Execute::VADD ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADD || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADD ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VADDi ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADDi || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADDi ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VADDq ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADDq || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADDq ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VADDBCX ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADDBCX || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADDBCX ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VADDBCY ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADDBCY || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADDBCY ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VADDBCZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADDBCZ || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADDBCZ ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VADDBCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADDBCW || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADDBCW ( VU0::_VU0, (Vu::Instruction::Format&) i );
}


// VADDA //

static void Execute::VADDA ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADDA || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADDA ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VADDAi ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADDAi || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADDAi ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VADDAq ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADDAq || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADDAq ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VADDABCX ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADDABCX || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADDABCX ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VADDABCY ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADDABCY || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADDABCY ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VADDABCZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADDABCZ || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADDABCZ ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VADDABCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VADDABCW || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ADDABCW ( VU0::_VU0, (Vu::Instruction::Format&) i );
}





// VSUB //

static void Execute::VSUB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUB || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUB ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSUBi ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUBi || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUBi ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSUBq ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUBq || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUBq ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSUBBCX ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUBBCX || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUBBCX ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSUBBCY ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUBBCY || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUBBCY ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSUBBCZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUBBCZ || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUBBCZ ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSUBBCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUBBCW || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUBBCW ( VU0::_VU0, (Vu::Instruction::Format&) i );
}




// VMADD //

static void Execute::VMADD ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADD || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADD ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMADDi ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADDi || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADDi ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMADDq ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADDq || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADDq ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMADDBCX ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADDBCX || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADDBCX ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMADDBCY ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADDBCY || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADDBCY ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMADDBCZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADDBCZ || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADDBCZ ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMADDBCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADDBCW || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADDBCW ( VU0::_VU0, (Vu::Instruction::Format&) i );
}




// VMSUB //

static void Execute::VMSUB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUB || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUB ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMSUBi ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUBi || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUBi ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMSUBq ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUBq || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUBq ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMSUBBCX ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUBBCX || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUBBCX ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMSUBBCY ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUBBCY || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUBBCY ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMSUBBCZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUBBCZ || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUBBCZ ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMSUBBCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUBBCW || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUBBCW ( VU0::_VU0, (Vu::Instruction::Format&) i );
}




// VMAX //

static void Execute::VMAX ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMAX || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MAX ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMAXi ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMAXi || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MAXi ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMAXBCX ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMAXBCX || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MAXBCX ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMAXBCY ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMAXBCY || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MAXBCY ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMAXBCZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMAXBCZ || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MAXBCZ ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMAXBCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMAXBCW || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MAXBCW ( VU0::_VU0, (Vu::Instruction::Format&) i );
}




// VMINI //

static void Execute::VMINI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMINI || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MINI ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMINIi ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMINi || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MINIi ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMINIBCX ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMINIBCX || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MINIBCX ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMINIBCY ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMINIBCY || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MINIBCY ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMINIBCZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMINIBCZ || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MINIBCZ ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMINIBCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMINIBCW || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MINIBCW ( VU0::_VU0, (Vu::Instruction::Format&) i );
}




// VMUL //

static void Execute::VMUL ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMUL || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MUL ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMULi ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMULi || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MULi ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMULq ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMULq || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MULq ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMULBCX ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMULBCX || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MULBCX ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMULBCY ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMULBCY || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MULBCY ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMULBCZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMULBCZ || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MULBCZ ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMULBCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMULBCW || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MULBCW ( VU0::_VU0, (Vu::Instruction::Format&) i );
}






static void Execute::VOPMSUB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VOPMSUB || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::OPMSUB ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VIADD ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VIADD || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::IADD ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VISUB ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VISUB || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::ISUB ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VIADDI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VIADDI || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::IADDI ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VIAND ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VIAND || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::IAND ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VIOR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VIOR || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::IOR ( VU0::_VU0, (Vu::Instruction::Format&) i );
}



// VCALLMS //

static void Execute::VCALLMS ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VCALLMS || defined INLINE_DEBUG_VU0 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::VCALLMSR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VCALLMSR || defined INLINE_DEBUG_VU0 || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}


// VFTOI //

static void Execute::VFTOI0 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VFTOI0 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::FTOI0 ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VFTOI4 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VFTOI4 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::FTOI4 ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VFTOI12 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VFTOI12 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::FTOI12 ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VFTOI15 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VFTOI15 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::FTOI15 ( VU0::_VU0, (Vu::Instruction::Format&) i );
}


// VITOF //

static void Execute::VITOF0 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VITOF0 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ITOF0 ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VITOF4 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VITOF4 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ITOF4 ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VITOF12 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VITOF12 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ITOF12 ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VITOF15 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VITOF15 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::ITOF15 ( VU0::_VU0, (Vu::Instruction::Format&) i );
}





static void Execute::VMOVE ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMOVE || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::MOVE ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VLQI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VLQI || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::LQI ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VDIV ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VDIV || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::DIV ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMTIR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMTIR || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::MTIR ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VRNEXT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VRNEXT || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::RNEXT ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMR32 ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMR32 || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::MR32 ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSQI ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSQI || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::SQI ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSQRT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSQRT || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::SQRT ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMFIR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMFIR || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::MFIR ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VRGET ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VRGET || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::RGET ( VU0::_VU0, (Vu::Instruction::Format&) i );
}




// VSUBA //

static void Execute::VSUBA ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUBA || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUBA ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSUBAi ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUBAi || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUBAi ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSUBAq ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUBAq || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUBAq ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSUBABCX ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUBABCX || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUBABCX ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSUBABCY ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUBABCY || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUBABCY ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSUBABCZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUBABCZ || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUBABCZ ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VSUBABCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSUBABCW || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::SUBABCW ( VU0::_VU0, (Vu::Instruction::Format&) i );
}



// VMADDA //

static void Execute::VMADDA ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADDA || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADDA ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMADDAi ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADDAi || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADDAi ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMADDAq ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADDAq || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADDAq ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMADDABCX ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADDABCX || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADDABCX ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMADDABCY ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADDABCY || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADDABCY ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMADDABCZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADDABCZ || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADDABCZ ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMADDABCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMADDABCW || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MADDABCW ( VU0::_VU0, (Vu::Instruction::Format&) i );
}




// VMSUBA //

static void Execute::VMSUBA ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUBA || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUBA ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMSUBAi ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUBAi || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUBAi ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMSUBAq ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUBAq || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUBAq ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMSUBABCX ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUBABCX || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUBABCX ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMSUBABCY ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUBABCY || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUBABCY ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMSUBABCZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUBABCZ || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUBABCZ ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMSUBABCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMSUBABCW || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MSUBABCW ( VU0::_VU0, (Vu::Instruction::Format&) i );
}





// VMULA //

static void Execute::VMULA ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMULA || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MULA ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMULAi ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMULAi || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MULAi ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMULAq ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMULAq || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MULAq ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMULABCX ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMULABCX || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MULABCX ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMULABCY ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMULABCY || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MULABCY ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMULABCZ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMULABCZ || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MULABCZ ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VMULABCW ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VMULABCW || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::MULABCW ( VU0::_VU0, (Vu::Instruction::Format&) i );
}





static void Execute::VOPMULA ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VOPMULA || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::OPMULA ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VLQD ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VLQD || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::LQD ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VRSQRT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VRSQRT || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::RSQRT ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VILWR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VILWR || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::ILWR ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VRINIT ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VRINIT || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::RINIT ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VCLIP ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VCLIP || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	Vu::Instruction::Execute::CLIP ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VNOP ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VNOP || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::VSQD ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VSQD || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::SQD ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VWAITQ ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VWAITQ || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::WAITQ ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VISWR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VISWR || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::ISWR ( VU0::_VU0, (Vu::Instruction::Format&) i );
}

static void Execute::VRXOR ( Instruction::Format i )
{
#if defined INLINE_DEBUG_VRXOR || defined INLINE_DEBUG_VU0 // || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << setw( 8 ) << r->PC << " " << dec << r->CycleCount << " " << Print::PrintInstruction ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	i.Value = 0x80000000 | ( i.Value & 0x01ffffff );
	Vu::Instruction::Execute::RXOR ( VU0::_VU0, (Vu::Instruction::Format&) i );
}






static const Execute::Function Execute::FunctionList []
{
	// instructions on both R3000A and R5900
	// 1 + 56 + 6 = 63 instructions //
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
	//Execute::MFC2, Execute::MTC2, Execute::LWC2, Execute::SWC2, Execute::RFE,
	//Execute::RTPS, Execute::RTPT, Execute::CC, Execute::CDP, Execute::DCPL, Execute::DPCS, Execute::DPCT, Execute::NCS,
	//Execute::NCT, Execute::NCDS, Execute::NCDT, Execute::NCCS, Execute::NCCT, Execute::GPF, Execute::GPL, Execute::AVSZ3,
	//Execute::AVSZ4, Execute::SQR, Execute::OP, Execute::NCLIP, Execute::INTPL, Execute::MVMVA
	
	// instructions on R5900 ONLY
	// (24*8) + 4 + 6 = 192 + 10 = 202 instructions //
	Execute::BEQL, Execute::BNEL, Execute::BGEZL, Execute::BGTZL, Execute::BLEZL, Execute::BLTZL, Execute::BGEZALL, Execute::BLTZALL,
	Execute::DADD, Execute::DADDI, Execute::DADDU, Execute::DADDIU, Execute::DSUB, Execute::DSUBU, Execute::DSLL, Execute::DSLL32,
	Execute::DSLLV, Execute::DSRA, Execute::DSRA32, Execute::DSRAV, Execute::DSRL, Execute::DSRL32, Execute::DSRLV, Execute::LD,
	Execute::LDL, Execute::LDR, Execute::LWU, Execute::LQ, Execute::PREF, Execute::SD, Execute::SDL, Execute::SDR,
	Execute::SQ, Execute::TEQ, Execute::TEQI, Execute::TNE, Execute::TNEI, Execute::TGE, Execute::TGEI, Execute::TGEU,
	Execute::TGEIU, Execute::TLT, Execute::TLTI, Execute::TLTU, Execute::TLTIU, Execute::MOVN, Execute::MOVZ, Execute::MULT1,
	Execute::MULTU1, Execute::DIV1, Execute::DIVU1, Execute::MADD, Execute::MADD1, Execute::MADDU, Execute::MADDU1, Execute::MFHI1,
	Execute::MTHI1, Execute::MFLO1, Execute::MTLO1, Execute::MFSA, Execute::MTSA, Execute::MTSAB, Execute::MTSAH,
	Execute::PABSH, Execute::PABSW, Execute::PADDB, Execute::PADDH, Execute::PADDW, Execute::PADDSB, Execute::PADDSH, Execute::PADDSW,
	Execute::PADDUB, Execute::PADDUH, Execute::PADDUW, Execute::PADSBH, Execute::PAND, Execute::POR, Execute::PXOR, Execute::PNOR,
	Execute::PCEQB, Execute::PCEQH, Execute::PCEQW, Execute::PCGTB, Execute::PCGTH, Execute::PCGTW, Execute::PCPYH, Execute::PCPYLD,
	Execute::PCPYUD, Execute::PDIVBW, Execute::PDIVUW, Execute::PDIVW, Execute::PEXCH, Execute::PEXCW, Execute::PEXEH, Execute::PEXEW,
	Execute::PEXT5, Execute::PEXTLB, Execute::PEXTLH, Execute::PEXTLW, Execute::PEXTUB, Execute::PEXTUH, Execute::PEXTUW, Execute::PHMADH,
	Execute::PHMSBH, Execute::PINTEH, Execute::PINTH, Execute::PLZCW, Execute::PMADDH, Execute::PMADDW, Execute::PMADDUW, Execute::PMAXH,
	Execute::PMAXW, Execute::PMINH, Execute::PMINW, Execute::PMFHI, Execute::PMFLO, Execute::PMTHI, Execute::PMTLO, Execute::PMFHL_LH,
	Execute::PMFHL_SH, Execute::PMFHL_LW, Execute::PMFHL_UW, Execute::PMFHL_SLW, Execute::PMTHL_LW, Execute::PMSUBH, Execute::PMSUBW, Execute::PMULTH,
	Execute::PMULTW, Execute::PMULTUW, Execute::PPAC5, Execute::PPACB, Execute::PPACH, Execute::PPACW, Execute::PREVH, Execute::PROT3W,
	Execute::PSLLH, Execute::PSLLVW, Execute::PSLLW, Execute::PSRAH, Execute::PSRAW, Execute::PSRAVW, Execute::PSRLH, Execute::PSRLW,
	Execute::PSRLVW, Execute::PSUBB, Execute::PSUBH, Execute::PSUBW, Execute::PSUBSB, Execute::PSUBSH, Execute::PSUBSW, Execute::PSUBUB,
	Execute::PSUBUH, Execute::PSUBUW,
	Execute::QFSRV, Execute::SYNC,
	
	Execute::DI, Execute::EI, Execute::ERET, Execute::CACHE, Execute::TLBP, Execute::TLBR, Execute::TLBWI, Execute::TLBWR,
	Execute::CFC0, Execute::CTC0,
	
	Execute::BC0T, Execute::BC0TL, Execute::BC0F, Execute::BC0FL, Execute::BC1T, Execute::BC1TL, Execute::BC1F, Execute::BC1FL,
	Execute::BC2T, Execute::BC2TL, Execute::BC2F, Execute::BC2FL,
	
	Execute::LWC1, Execute::SWC1, Execute::MFC1, Execute::MTC1, Execute::CFC1, Execute::CTC1,
	Execute::ABS_S, Execute::ADD_S, Execute::ADDA_S, Execute::C_EQ_S, Execute::C_F_S, Execute::C_LE_S, Execute::C_LT_S, Execute::CVT_S_W,
	Execute::CVT_W_S, Execute::DIV_S, Execute::MADD_S, Execute::MADDA_S, Execute::MAX_S, Execute::MIN_S, Execute::MOV_S, Execute::MSUB_S,
	Execute::MSUBA_S, Execute::MUL_S, Execute::MULA_S, Execute::NEG_S, Execute::RSQRT_S, Execute::SQRT_S, Execute::SUB_S, Execute::SUBA_S,
	
	// VU macro mode instructions
	Execute::QMFC2_NI, Execute::QMFC2_I, Execute::QMTC2_NI, Execute::QMTC2_I, Execute::LQC2, Execute::SQC2,
	
	Execute::VABS,
	Execute::VADD, Execute::VADDi, Execute::VADDq, Execute::VADDBCX, Execute::VADDBCY, Execute::VADDBCZ, Execute::VADDBCW,
	Execute::VADDA, Execute::VADDAi, Execute::VADDAq, Execute::VADDABCX, Execute::VADDABCY, Execute::VADDABCZ, Execute::VADDABCW,
	Execute::VCALLMS, Execute::VCALLMSR, Execute::VCLIP, Execute::VDIV,
	Execute::VFTOI0, Execute::VFTOI4, Execute::VFTOI12, Execute::VFTOI15,
	Execute::VIADD, Execute::VIADDI, Execute::VIAND, Execute::VILWR, Execute::VIOR, Execute::VISUB, Execute::VISWR,
	Execute::VITOF0, Execute::VITOF4, Execute::VITOF12, Execute::VITOF15,
	Execute::VLQD, Execute::VLQI,
	
	Execute::VMADD, Execute::VMADDi, Execute::VMADDq, Execute::VMADDBCX, Execute::VMADDBCY, Execute::VMADDBCZ, Execute::VMADDBCW,
	Execute::VMADDA, Execute::VMADDAi, Execute::VMADDAq, Execute::VMADDABCX, Execute::VMADDABCY, Execute::VMADDABCZ, Execute::VMADDABCW,
	Execute::VMAX, Execute::VMAXi, Execute::VMAXBCX, Execute::VMAXBCY, Execute::VMAXBCZ, Execute::VMAXBCW,
	Execute::VMFIR,
	Execute::VMINI, Execute::VMINIi, Execute::VMINIBCX, Execute::VMINIBCY, Execute::VMINIBCZ, Execute::VMINIBCW,
	Execute::VMOVE, Execute::VMR32,
	
	Execute::VMSUB, Execute::VMSUBi, Execute::VMSUBq, Execute::VMSUBBCX, Execute::VMSUBBCY, Execute::VMSUBBCZ, Execute::VMSUBBCW,
	Execute::VMSUBA, Execute::VMSUBAi, Execute::VMSUBAq, Execute::VMSUBABCX, Execute::VMSUBABCY, Execute::VMSUBABCZ, Execute::VMSUBABCW,
	Execute::VMTIR,
	Execute::VMUL, Execute::VMULi, Execute::VMULq, Execute::VMULBCX, Execute::VMULBCY, Execute::VMULBCZ, Execute::VMULBCW,
	Execute::VMULA, Execute::VMULAi, Execute::VMULAq, Execute::VMULABCX, Execute::VMULABCY, Execute::VMULABCZ, Execute::VMULABCW,
	Execute::VNOP, Execute::VOPMSUB, Execute::VOPMULA, Execute::VRGET, Execute::VRINIT, Execute::VRNEXT, Execute::VRSQRT, Execute::VRXOR,
	Execute::VSQD, Execute::VSQI, Execute::VSQRT,
	Execute::VSUB, Execute::VSUBi, Execute::VSUBq, Execute::VSUBBCX, Execute::VSUBBCY, Execute::VSUBBCZ, Execute::VSUBBCW,
	Execute::VSUBA, Execute::VSUBAi, Execute::VSUBAq, Execute::VSUBABCX, Execute::VSUBABCY, Execute::VSUBABCZ, Execute::VSUBABCW,
	Execute::VWAITQ,
	Execute::COP2
};





#ifdef _DEBUG_VERSION_
Debug::Log Execute::debug;
#endif





// generates the lookup table
Execute::Execute ( Cpu* pCpu )
{
	r = pCpu;
}



}

}



