

#include "VU_Execute.h"
#include "VU_Print.h"
#include "PS2Float.h"
#include "PS2_GPU.h"

//#include <cmath>

using namespace std;
using namespace Vu::Instruction;
using namespace PS2Float;


#ifdef _DEBUG_VERSION_
Debug::Log Execute::debug;
#endif



// enable debugging

#ifdef _DEBUG_VERSION_

#define INLINE_DEBUG_ENABLE

//#define INLINE_DEBUG_VU
#define INLINE_DEBUG_UNIMPLEMENTED

#endif




const char Execute::XyzwLUT [ 4 ] = { 'x', 'y', 'z', 'w' };
const char* Execute::BCType [ 4 ] = { "F", "T", "FL", "TL" };






static void Execute::Start ()
{
	// make sure the lookup object has started (note: this can take a LONG time for R5900 currently)
	Lookup::Start ();
	
#ifdef INLINE_DEBUG_ENABLE
	debug.Create ( "PS2_VU_Execute_Log.txt" );
#endif
}



static void Execute::INVALID ( VU *v, Instruction::Format i )
{
}



//// *** UPPER instructions *** ////


// ABS //

static void Execute::ABS ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ABS || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	if ( i.destx )
	{
		v->vf [ i.Ft ].uw0 = v->vf [ i.Fs ].uw0 & ~0x80000000;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].uw1 = v->vf [ i.Fs ].uw1 & ~0x80000000;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].uw2 = v->vf [ i.Fs ].uw2 & ~0x80000000;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].uw3 = v->vf [ i.Fs ].uw3 & ~0x80000000;
	}
	
	// flags affected: none

#if defined INLINE_DEBUG_ABS || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}





// ADD //

static void Execute::ADD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// fd = fs + ft
	VuUpperOp ( v, i, PS2_Float_Add );
	
	// flags affected: zero, sign, [ overflow, underflow ]

#if defined INLINE_DEBUG_ADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::ADDi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
#endif

	// fd = fs + I
	
	VuUpperOpI ( v, i, PS2_Float_Add );
	
	// flags affected: zero, sign, [ overflow, underflow ]

#if defined INLINE_DEBUG_ADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::ADDq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
#endif

	// fd = fs + Q
	
	VuUpperOpQ ( v, i, PS2_Float_Add );
	
	// flags affected: zero, sign, [ overflow, underflow ]

#if defined INLINE_DEBUG_ADDQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::ADDBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADCBX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// fd = fs + ftbc
	
	VuUpperOpX ( v, i, PS2_Float_Add );

	// flags affected: zero, sign, [ overflow, underflow ]

#if defined INLINE_DEBUG_ADDBCX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::ADDBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDBCY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// fd = fs + ftbc
	
	VuUpperOpY ( v, i, PS2_Float_Add );
	
	// flags affected: zero, sign, [ overflow, underflow ]

#if defined INLINE_DEBUG_ADDBCY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::ADDBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDBCZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// fd = fs + ftbc
	
	VuUpperOpZ ( v, i, PS2_Float_Add );
	
	// flags affected: zero, sign, [ overflow, underflow ]

#if defined INLINE_DEBUG_ADDBCZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::ADDBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDBCW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// fd = fs + ftbc
	VuUpperOpW ( v, i, PS2_Float_Add );
	
	// flags affected: zero, sign, [ overflow, underflow ]

#if defined INLINE_DEBUG_ADDBCW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}




// SUB //

static void Execute::SUB ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// fd = fs - ft
	VuUpperOp ( v, i, PS2_Float_Sub );
	
	// flags affected: zero, sign, [ overflow, underflow ]

#if defined INLINE_DEBUG_SUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::SUBi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
#endif

	VuUpperOpI ( v, i, PS2_Float_Sub );
	
#if defined INLINE_DEBUG_SUBI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::SUBq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
#endif

	VuUpperOpQ ( v, i, PS2_Float_Sub );
	
#if defined INLINE_DEBUG_SUBQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::SUBBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpX ( v, i, PS2_Float_Sub );
	
#if defined INLINE_DEBUG_SUBX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::SUBBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpY ( v, i, PS2_Float_Sub );
	
#if defined INLINE_DEBUG_SUBY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::SUBBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpZ ( v, i, PS2_Float_Sub );
	
#if defined INLINE_DEBUG_SUBZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::SUBBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpW ( v, i, PS2_Float_Sub );
	
#if defined INLINE_DEBUG_SUBW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}




// MADD //

static void Execute::MADD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOp ( v, i, PS2_Float_Madd );
	
#if defined INLINE_DEBUG_MADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MADDi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpI ( v, i, PS2_Float_Madd );
	
#if defined INLINE_DEBUG_MADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MADDq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpQ ( v, i, PS2_Float_Madd );
	
#if defined INLINE_DEBUG_MADDQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MADDBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpX ( v, i, PS2_Float_Madd );
	
#if defined INLINE_DEBUG_MADDX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MADDBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpY ( v, i, PS2_Float_Madd );
	
#if defined INLINE_DEBUG_MADDY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MADDBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpZ ( v, i, PS2_Float_Madd );
	
#if defined INLINE_DEBUG_MADDZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MADDBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpW ( v, i, PS2_Float_Madd );
	
#if defined INLINE_DEBUG_MADDW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}



// MSUB //

static void Execute::MSUB ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOp ( v, i, PS2_Float_Msub );
	
#if defined INLINE_DEBUG_MSUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MSUBi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpI ( v, i, PS2_Float_Msub );
	
#if defined INLINE_DEBUG_MSUBI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MSUBq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpQ ( v, i, PS2_Float_Msub );
	
#if defined INLINE_DEBUG_MSUBQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MSUBBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpX ( v, i, PS2_Float_Msub );
	
#if defined INLINE_DEBUG_MSUBX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MSUBBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpY ( v, i, PS2_Float_Msub );
	
#if defined INLINE_DEBUG_MSUBY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MSUBBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpZ ( v, i, PS2_Float_Msub );
	
#if defined INLINE_DEBUG_MSUBZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MSUBBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpW ( v, i, PS2_Float_Msub );
	
#if defined INLINE_DEBUG_MSUBW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}



// MUL //

static void Execute::MUL ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MUL || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOp ( v, i, PS2_Float_Mul );
	
#if defined INLINE_DEBUG_MUL || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MULi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
#endif

	VuUpperOpI ( v, i, PS2_Float_Mul );
	
#if defined INLINE_DEBUG_MULI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MULq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
#endif

	VuUpperOpQ ( v, i, PS2_Float_Mul );
	
#if defined INLINE_DEBUG_MULQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MULBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpX ( v, i, PS2_Float_Mul );
	
#if defined INLINE_DEBUG_MULX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MULBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpY ( v, i, PS2_Float_Mul );
	
#if defined INLINE_DEBUG_MULY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MULBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpZ ( v, i, PS2_Float_Mul );
	
#if defined INLINE_DEBUG_MULZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MULBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpW ( v, i, PS2_Float_Mul );
	
#if defined INLINE_DEBUG_MULW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}



// MAX //

static void Execute::MAX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Max ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fx );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Max ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fy );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Max ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fz );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Max ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fw );
	}
	
#if defined INLINE_DEBUG_MAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MAXi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MAXI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f;
#endif

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Max ( v->vf [ i.Fs ].fx, v->vi [ 21 ].f );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Max ( v->vf [ i.Fs ].fy, v->vi [ 21 ].f );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Max ( v->vf [ i.Fs ].fz, v->vi [ 21 ].f );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Max ( v->vf [ i.Fs ].fw, v->vi [ 21 ].f );
	}
	
#if defined INLINE_DEBUG_MAXI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MAXBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MAXX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Max ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fx );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Max ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fx );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Max ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fx );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Max ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fx );
	}
	
#if defined INLINE_DEBUG_MAXX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MAXBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MAXY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Max ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fy );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Max ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fy );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Max ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fy );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Max ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fy );
	}
	
#if defined INLINE_DEBUG_MAXY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MAXBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MAXZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Max ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fz );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Max ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fz );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Max ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fz );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Max ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fz );
	}
	
#if defined INLINE_DEBUG_MAXZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MAXBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MAXW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Max ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fw );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Max ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fw );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Max ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fw );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Max ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fw );
	}
	
#if defined INLINE_DEBUG_MAXW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}



// MINI //

static void Execute::MINI ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MINI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Min ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fx );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Min ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fy );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Min ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fz );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Min ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fw );
	}
	
#if defined INLINE_DEBUG_MINI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MINIi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MINII || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " I=" << hex << v->vi [ 21 ].f;
#endif

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Min ( v->vf [ i.Fs ].fx, v->vi [ 21 ].f );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Min ( v->vf [ i.Fs ].fy, v->vi [ 21 ].f );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Min ( v->vf [ i.Fs ].fz, v->vi [ 21 ].f );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Min ( v->vf [ i.Fs ].fw, v->vi [ 21 ].f );
	}
	
#if defined INLINE_DEBUG_MINII || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MINIBCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MINIX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Min ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fx );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Min ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fx );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Min ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fx );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Min ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fx );
	}
	
#if defined INLINE_DEBUG_MINIX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MINIBCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MINIY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Min ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fy );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Min ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fy );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Min ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fy );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Min ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fy );
	}
	
#if defined INLINE_DEBUG_MINIY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MINIBCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MINIZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Min ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fz );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Min ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fz );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Min ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fz );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Min ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fz );
	}
	
#if defined INLINE_DEBUG_MINIZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}

static void Execute::MINIBCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MINIW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	if ( i.destx )
	{
		v->vf [ i.Fd ].fx = PS2_Float_Min ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fw );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Fd ].fy = PS2_Float_Min ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fw );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Fd ].fz = PS2_Float_Min ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fw );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Fd ].fw = PS2_Float_Min ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fw );
	}
	
#if defined INLINE_DEBUG_MINIW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Fd=" << " vfx=" << hex << v->vf [ i.Fd ].fx << " vfy=" << v->vf [ i.Fd ].fy << " vfz=" << v->vf [ i.Fd ].fz << " vfw=" << v->vf [ i.Fd ].fw;
#endif
}








// ITOF //

static void Execute::ITOF0 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ITOF0 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << dec << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	if ( i.destx )
	{
		v->vf [ i.Ft ].fx = (float) v->vf [ i.Fs ].sw0;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].fy = (float) v->vf [ i.Fs ].sw1;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].fz = (float) v->vf [ i.Fs ].sw2;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].fw = (float) v->vf [ i.Fs ].sw3;
	}
	
	// flags affected: none

#if defined INLINE_DEBUG_ITOF0 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

static void Execute::FTOI0 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FTOI0 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	if ( i.destx )
	{
		v->vf [ i.Ft ].sw0 = (s32) v->vf [ i.Fs ].fx;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].sw1 = (s32) v->vf [ i.Fs ].fy;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].sw2 = (s32) v->vf [ i.Fs ].fz;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].sw3 = (s32) v->vf [ i.Fs ].fw;
	}
	
	// flags affected: none

#if defined INLINE_DEBUG_FTOI0 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << dec << v->vf [ i.Ft ].uw0 << " vfy=" << v->vf [ i.Ft ].uw1 << " vfz=" << v->vf [ i.Ft ].uw2 << " vfw=" << v->vf [ i.Ft ].uw3;
#endif
}

static void Execute::ITOF4 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ITOF4 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << dec << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	static const float c_fMultiplier = 1.0f / 16.0f;

	if ( i.destx )
	{
		v->vf [ i.Ft ].fx = ( (float) v->vf [ i.Fs ].sw0 ) * c_fMultiplier;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].fy = ( (float) v->vf [ i.Fs ].sw1 ) * c_fMultiplier;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].fz = ( (float) v->vf [ i.Fs ].sw2 ) * c_fMultiplier;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].fw = ( (float) v->vf [ i.Fs ].sw3 ) * c_fMultiplier;
	}
	
	// flags affected: none
	
#if defined INLINE_DEBUG_ITOF4 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

static void Execute::FTOI4 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FTOI4 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	static const float c_fMultiplier = 16.0f;
	
	if ( i.destx )
	{
		v->vf [ i.Ft ].sw0 = (s32) ( v->vf [ i.Fs ].fx * c_fMultiplier );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].sw1 = (s32) ( v->vf [ i.Fs ].fy * c_fMultiplier );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].sw2 = (s32) ( v->vf [ i.Fs ].fz * c_fMultiplier );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].sw3 = (s32) ( v->vf [ i.Fs ].fw * c_fMultiplier );
	}
	
	// flags affected: none

#if defined INLINE_DEBUG_FTOI4 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << dec << v->vf [ i.Ft ].uw0 << " vfy=" << v->vf [ i.Ft ].uw1 << " vfz=" << v->vf [ i.Ft ].uw2 << " vfw=" << v->vf [ i.Ft ].uw3;
#endif
}

static void Execute::ITOF12 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ITOF12 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << dec << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	static const float c_fMultiplier = 1.0f / 4096.0f;

	if ( i.destx )
	{
		v->vf [ i.Ft ].fx = ( (float) v->vf [ i.Fs ].sw0 ) * c_fMultiplier;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].fy = ( (float) v->vf [ i.Fs ].sw1 ) * c_fMultiplier;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].fz = ( (float) v->vf [ i.Fs ].sw2 ) * c_fMultiplier;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].fw = ( (float) v->vf [ i.Fs ].sw3 ) * c_fMultiplier;
	}
	
	// flags affected: none
	
#if defined INLINE_DEBUG_ITOF12 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

static void Execute::FTOI12 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FTOI12 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	static const float c_fMultiplier = 4096.0f;
	
	if ( i.destx )
	{
		v->vf [ i.Ft ].sw0 = (s32) ( v->vf [ i.Fs ].fx * c_fMultiplier );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].sw1 = (s32) ( v->vf [ i.Fs ].fy * c_fMultiplier );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].sw2 = (s32) ( v->vf [ i.Fs ].fz * c_fMultiplier );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].sw3 = (s32) ( v->vf [ i.Fs ].fw * c_fMultiplier );
	}
	
	// flags affected: none

#if defined INLINE_DEBUG_FTOI12 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << dec << v->vf [ i.Ft ].uw0 << " vfy=" << v->vf [ i.Ft ].uw1 << " vfz=" << v->vf [ i.Ft ].uw2 << " vfw=" << v->vf [ i.Ft ].uw3;
#endif
}

static void Execute::ITOF15 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ITOF15 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << dec << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	static const float c_fMultiplier = 1.0f / 32768.0f;

	if ( i.destx )
	{
		v->vf [ i.Ft ].fx = ( (float) v->vf [ i.Fs ].sw0 ) * c_fMultiplier;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].fy = ( (float) v->vf [ i.Fs ].sw1 ) * c_fMultiplier;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].fz = ( (float) v->vf [ i.Fs ].sw2 ) * c_fMultiplier;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].fw = ( (float) v->vf [ i.Fs ].sw3 ) * c_fMultiplier;
	}
	
	// flags affected: none
	
#if defined INLINE_DEBUG_ITOF15 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

static void Execute::FTOI15 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FTOI15 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	static const float c_fMultiplier = 32768.0f;
	
	if ( i.destx )
	{
		v->vf [ i.Ft ].sw0 = (s32) ( v->vf [ i.Fs ].fx * c_fMultiplier );
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].sw1 = (s32) ( v->vf [ i.Fs ].fy * c_fMultiplier );
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].sw2 = (s32) ( v->vf [ i.Fs ].fz * c_fMultiplier );
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].sw3 = (s32) ( v->vf [ i.Fs ].fw * c_fMultiplier );
	}
	
	// flags affected: none

#if defined INLINE_DEBUG_FTOI15 || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << dec << v->vf [ i.Ft ].uw0 << " vfy=" << v->vf [ i.Ft ].uw1 << " vfz=" << v->vf [ i.Ft ].uw2 << " vfw=" << v->vf [ i.Ft ].uw3;
#endif
}





// ADDA //

static void Execute::ADDA ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// fd = fs + ft
	VuUpperOp_A ( v, i, PS2_Float_AddA );
	
#if defined INLINE_DEBUG_ADDA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::ADDAi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
#endif

	VuUpperOpI_A ( v, i, PS2_Float_AddA );
	
#if defined INLINE_DEBUG_ADDAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::ADDAq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
#endif

	VuUpperOpQ_A ( v, i, PS2_Float_AddA );
	
#if defined INLINE_DEBUG_ADDAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::ADDABCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpX_A ( v, i, PS2_Float_AddA );
	
#if defined INLINE_DEBUG_ADDAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::ADDABCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpY_A ( v, i, PS2_Float_AddA );
	
#if defined INLINE_DEBUG_ADDAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::ADDABCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpZ_A ( v, i, PS2_Float_AddA );
	
#if defined INLINE_DEBUG_ADDAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::ADDABCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ADDAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpW_A ( v, i, PS2_Float_AddA );
	
#if defined INLINE_DEBUG_ADDAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}




// SUBA //

static void Execute::SUBA ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOp_A ( v, i, PS2_Float_SubA );
	
#if defined INLINE_DEBUG_SUBA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::SUBAi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
#endif

	VuUpperOpI_A ( v, i, PS2_Float_SubA );
	
#if defined INLINE_DEBUG_SUBAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::SUBAq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
#endif

	VuUpperOpQ_A ( v, i, PS2_Float_SubA );
	
#if defined INLINE_DEBUG_SUBAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::SUBABCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpX_A ( v, i, PS2_Float_SubA );
	
#if defined INLINE_DEBUG_SUBAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::SUBABCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpY_A ( v, i, PS2_Float_SubA );
	
#if defined INLINE_DEBUG_SUBAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::SUBABCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpZ_A ( v, i, PS2_Float_SubA );
	
#if defined INLINE_DEBUG_SUBAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::SUBABCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SUBAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpW_A ( v, i, PS2_Float_SubA );
	
#if defined INLINE_DEBUG_SUBAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}



// MADDA //

static void Execute::MADDA ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOp_A ( v, i, PS2_Float_MaddA );
	
#if defined INLINE_DEBUG_MADDA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MADDAi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpI_A ( v, i, PS2_Float_MaddA );
	
#if defined INLINE_DEBUG_MADDAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MADDAq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpQ_A ( v, i, PS2_Float_MaddA );
	
#if defined INLINE_DEBUG_MADDAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MADDABCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpX_A ( v, i, PS2_Float_MaddA );
	
#if defined INLINE_DEBUG_MADDAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MADDABCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpY_A ( v, i, PS2_Float_MaddA );
	
#if defined INLINE_DEBUG_MADDAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MADDABCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpZ_A ( v, i, PS2_Float_MaddA );
	
#if defined INLINE_DEBUG_MADDAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MADDABCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MADDAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpW_A ( v, i, PS2_Float_MaddA );
	
#if defined INLINE_DEBUG_MADDAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}


// MSUBA //

static void Execute::MSUBA ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOp_A ( v, i, PS2_Float_MsubA );
	
#if defined INLINE_DEBUG_MSUBA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MSUBAi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpI_A ( v, i, PS2_Float_MsubA );
	
#if defined INLINE_DEBUG_MSUBAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MSUBAq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpQ_A ( v, i, PS2_Float_MsubA );
	
#if defined INLINE_DEBUG_MSUBAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MSUBABCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpX_A ( v, i, PS2_Float_MsubA );
	
#if defined INLINE_DEBUG_MSUBAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MSUBABCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpY_A ( v, i, PS2_Float_MsubA );
	
#if defined INLINE_DEBUG_MSUBAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MSUBABCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpZ_A ( v, i, PS2_Float_MsubA );
	
#if defined INLINE_DEBUG_MSUBAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MSUBABCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MSUBAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	VuUpperOpW_A ( v, i, PS2_Float_MsubA );
	
#if defined INLINE_DEBUG_MSUBAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}



// MULA //

static void Execute::MULA ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOp_A ( v, i, PS2_Float_MulA );
	
#if defined INLINE_DEBUG_MULA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MULAi ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
#endif

	VuUpperOpI_A ( v, i, PS2_Float_MulA );
	
#if defined INLINE_DEBUG_MULAI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MULAq ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
#endif

	VuUpperOpQ_A ( v, i, PS2_Float_MulA );
	
#if defined INLINE_DEBUG_MULAQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MULABCX ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpX_A ( v, i, PS2_Float_MulA );
	
#if defined INLINE_DEBUG_MULAX || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MULABCY ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpY_A ( v, i, PS2_Float_MulA );
	
#if defined INLINE_DEBUG_MULAY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MULABCZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpZ_A ( v, i, PS2_Float_MulA );
	
#if defined INLINE_DEBUG_MULAZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::MULABCW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MULAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << " Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	VuUpperOpW_A ( v, i, PS2_Float_MulA );
	
#if defined INLINE_DEBUG_MULAW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}







// other upper instructions //

static void Execute::CLIP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_CLIP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << "Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	float fw_plus, fw_minus;
	
	fw_plus = v->vf [ i.Ft ].uw3 & 0x7fffffff;
	fw_minus = v->vf [ i.Ft ].uw3 | 0x80000000;
	
	// read clipping flag
	v->ClippingFlag.Value = v->vi [ 18 ].u;
	
	v->ClippingFlag.Value <<= 6;
	if ( v->vf [ i.Fs ].fx > fw_plus ) v->ClippingFlag.x_plus0 = 1; else if ( v->vf [ i.Fs ].fx < fw_minus ) v->ClippingFlag.x_minus0 = 1;
	if ( v->vf [ i.Fs ].fy > fw_plus ) v->ClippingFlag.y_plus0 = 1; else if ( v->vf [ i.Fs ].fy < fw_minus ) v->ClippingFlag.y_minus0 = 1;
	if ( v->vf [ i.Fs ].fz > fw_plus ) v->ClippingFlag.z_plus0 = 1; else if ( v->vf [ i.Fs ].fz < fw_minus ) v->ClippingFlag.z_minus0 = 1;
	
	// set 24 bits to the clipping flag
	v->vi [ 18 ].u = v->ClippingFlag.Value & 0xffffff;
	
#if defined INLINE_DEBUG_CLIP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " ClippingFlag=" << hex << v->ClippingFlag.Value;
#endif
}



static void Execute::OPMULA ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_OPMULA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << "Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
#endif

	// ACC_x = fs_y * ft_z
	v->dACC [ 0 ].d = PS2_Float_MulA ( v->vf [ i.Fs ].fy,  v->vf [ i.Ft ].fz, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );

	// ACC_y = fs_z * ft_x
	v->dACC [ 1 ].d = PS2_Float_MulA ( v->vf [ i.Fs ].fz,  v->vf [ i.Ft ].fx, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
										
	// ACC_z = fs_x * ft_y
	v->dACC [ 2 ].d = PS2_Float_MulA ( v->vf [ i.Fs ].fx,  v->vf [ i.Ft ].fy, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
										
#if defined INLINE_DEBUG_OPMULA || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif
}

static void Execute::OPMSUB ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_OPMSUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_FPU	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << "Fs= x=" << hex << v->vf [ i.Fs ].fx << " y=" << v->vf [ i.Fs ].fy << " z=" << v->vf [ i.Fs ].fz << " w=" << v->vf [ i.Fs ].fw;
	debug << "Ft= x=" << hex << v->vf [ i.Ft ].fx << " y=" << v->vf [ i.Ft ].fy << " z=" << v->vf [ i.Ft ].fz << " w=" << v->vf [ i.Ft ].fw;
	debug << " ACC= x=" << v->dACC [ 0 ].d << " y=" << v->dACC [ 1 ].d << " z=" << v->dACC [ 2 ].d << " w=" << v->dACC [ 3 ].d;
#endif

	double fd_x, fd_y, fd_z;

	// fd_x = ACC_x - fs_y * ft_z
	v->vf [ i.Fd ].fx = PS2_Float_Msub ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fy,  v->vf [ i.Ft ].fz, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
	
	// fd_y = ACC_y - fs_x * ft_z
	v->vf [ i.Fd ].fy = PS2_Float_Msub ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fx,  v->vf [ i.Ft ].fz, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
	
	// fd_z = ACC_z - fs_x * ft_y
	v->vf [ i.Fd ].fz = PS2_Float_Msub ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fx,  v->vf [ i.Ft ].fy, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
}



static void Execute::NOP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_NOP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionHI ( i.Value ).c_str () << "; " << hex << i.Value;
#endif


}






//// *** LOWER instructions *** ////




// branch/jump instructions //

static void Execute::B ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_B || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		//d->cb = r->_cb_Branch;
		v->Status.DelaySlot_Valid |= 0x2;
}

static void Execute::BAL ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_BAL || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		//d->cb = r->_cb_Branch;
		v->Status.DelaySlot_Valid |= 0x2;
		
		v->vi [ i.it ].uLo = v->PC + 16;
}

static void Execute::IBEQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IBEQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( v->vi [ i.it ].uLo == v->vi [ i.is ].uLo )
	{
		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		//d->cb = r->_cb_Branch;
		v->Status.DelaySlot_Valid |= 0x2;
	}
}

static void Execute::IBNE ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IBNE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( v->vi [ i.it ].uLo != v->vi [ i.is ].uLo )
	{
		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		//d->cb = r->_cb_Branch;
		v->Status.DelaySlot_Valid |= 0x2;
	}
}

static void Execute::IBLTZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IBLTZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( v->vi [ i.is ].sLo < 0 )
	{
		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		//d->cb = r->_cb_Branch;
		v->Status.DelaySlot_Valid |= 0x2;
	}
}

static void Execute::IBGTZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IBGTZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( v->vi [ i.is ].sLo > 0 )
	{
		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		//d->cb = r->_cb_Branch;
		v->Status.DelaySlot_Valid |= 0x2;
	}
}

static void Execute::IBLEZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IBLEZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( v->vi [ i.is ].sLo <= 0 )
	{
		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		//d->cb = r->_cb_Branch;
		v->Status.DelaySlot_Valid |= 0x2;
	}
}

static void Execute::IBGEZ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IBGEZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( v->vi [ i.is ].sLo >= 0 )
	{
		// next instruction is in the branch delay slot
		VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
		d->Instruction = i;
		//d->cb = r->_cb_Branch;
		v->Status.DelaySlot_Valid |= 0x2;
	}
}

static void Execute::JR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_JR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// check for address exception??
	//if ( r->GPR [ i.Rs ].u & 3 )
	//{
	//	// if lower 2-bits of register are not zero, fire address exception
	//	r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
	//	return;
	//}
	
	// next instruction is in the branch delay slot
	VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
	d->Instruction = i;
	//d->cb = r->_cb_JumpRegister;

	// *** todo *** check if address exception should be generated if lower 2-bits of jump address are not zero
	// will clear out lower two bits of address for now
	d->Data = v->vi [ i.is ].uLo & ~3;
	
	v->Status.DelaySlot_Valid |= 0x2;
}

static void Execute::JALR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_JALR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// check for address exception??
	//if ( r->GPR [ i.Rs ].u & 3 )
	//{
	//	// if lower 2-bits of register are not zero, fire address exception
	//	r->ProcessSynchronousInterrupt ( Cpu::EXC_ADEL );
	//	return;
	//}
	
	// next instruction is in the branch delay slot
	VU::DelaySlot *d = & ( v->DelaySlots [ v->NextDelaySlotIndex ^ 1 ] );
	d->Instruction = i;
	//d->cb = r->_cb_JumpRegister;

	// *** todo *** check if address exception should be generated if lower 2-bits of jump address are not zero
	// will clear out lower two bits of address for now
	d->Data = v->vi [ i.is ].uLo & ~3;
	
	v->Status.DelaySlot_Valid |= 0x2;
	
	v->vi [ i.it ].uLo = v->PC + 16;
}









// FC/FM/FS instructions //

static void Execute::FCEQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FCEQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->vi [ 1 ].u = ( ( ( v->vi [ 18 ].u & 0xffffff ) == i.Imm24 ) ? 1 : 0 );
}

static void Execute::FCAND ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FCAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->vi [ 1 ].u = ( ( v->vi [ 18 ].u & i.Imm24 ) ? 1 : 0 );
}

static void Execute::FCOR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FCOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->vi [ 1 ].u = ( ( ( ( v->vi [ 18 ].u & 0xffffff ) | i.Imm24 ) == 0xffffff ) ? 1 : 0 );
}

static void Execute::FCGET ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FCGET || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->vi [ i.it ].uLo = v->vi [ 18 ].u & 0xfff;
}

static void Execute::FCSET ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FCSET || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->vi [ 18 ].u = i.Imm24;
}

static void Execute::FMEQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FMEQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->vi [ i.it ].u = ~( v->vi [ i.is ].u ^ v->vi [ 17 ].u );
}

static void Execute::FMAND ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FMAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->vi [ i.it ].u = v->vi [ i.is ].u & v->vi [ 17 ].u;
}

static void Execute::FMOR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FMOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->vi [ i.it ].u = v->vi [ i.is ].u | v->vi [ 17 ].u;
}

static void Execute::FSEQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FSEQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->vi [ i.it ].u = ~( v->vi [ 16 ].u ^ ( ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) & 0xfff ) );
}

static void Execute::FSSET ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FSSET || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->vi [ 16 ].u = ( v->vi [ 16 ].u & 0x3f ) | ( ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) & 0xfc0 );
}

static void Execute::FSAND ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FSAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->vi [ i.it ].u = v->vi [ 16 ].u & ( ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) & 0xfff );
}

static void Execute::FSOR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_FSOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	v->vi [ i.it ].u = v->vi [ 16 ].u | ( ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) ) & 0xfff );
}



// Integer math //


static void Execute::IADD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo << " it=" << v->vf [ i.it ].uLo;
#endif

	// id = is + it
	v->vi [ i.id ].u = v->vi [ i.is ].uLo + v->vi [ i.it ].uLo;
	
#if defined INLINE_DEBUG_IADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: id=" << hex << v->vi [ i.id ].uLo;
#endif
}

static void Execute::IADDI ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo;
#endif

	// it = is + Imm5
	v->vi [ i.it ].u = v->vi [ i.is ].uLo + ( (s32) i.Imm5 );
	
#if defined INLINE_DEBUG_IADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: it=" << hex << v->vi [ i.it ].uLo;
#endif
}

static void Execute::IADDIU ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo;
#endif

	// it = is + Imm15
	v->vi [ i.it ].u = v->vi [ i.is ].uLo + ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) );
	
#if defined INLINE_DEBUG_IADDI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: it=" << hex << v->vi [ i.it ].uLo;
#endif
}

static void Execute::ISUB ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ISUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif

	// id = is - it
	v->vi [ i.id ].u = v->vi [ i.is ].uLo - v->vi [ i.it ].uLo;
	
#if defined INLINE_DEBUG_ISUB || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: id=" << hex << v->vi [ i.id ].uLo;
#endif
}

static void Execute::ISUBIU ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ISUBIU || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo;
#endif

	// it = is - Imm15
	v->vi [ i.it ].u = v->vi [ i.is ].uLo - ( ( i.Imm15_1 << 11 ) | ( i.Imm15_0 ) );
	
#if defined INLINE_DEBUG_ISUBIU || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: it=" << hex << v->vi [ i.it ].uLo;
#endif
}


static void Execute::IAND ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif

	// id = is & it
	v->vi [ i.id ].u = v->vi [ i.is ].uLo & v->vi [ i.it ].uLo;
	
#if defined INLINE_DEBUG_IAND || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: id=" << hex << v->vi [ i.id ].uLo;
#endif
}

static void Execute::IOR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_IOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " is=" << hex << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif

	// id = is | it
	v->vi [ i.id ].u = v->vi [ i.is ].uLo | v->vi [ i.it ].uLo;
	
#if defined INLINE_DEBUG_IOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: id=" << hex << v->vi [ i.id ].uLo;
#endif
}




// Move instructions //

static void Execute::MFP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MOVE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	// dest ft = P
	
	if ( i.destx )
	{
		v->vf [ i.Ft ].uw0 = v->vi [ 23 ].u;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].uw1 = v->vi [ 23 ].u;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].uw2 = v->vi [ 23 ].u;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].uw3 = v->vi [ 23 ].u;
	}
	
	// flags affected: none

#if defined INLINE_DEBUG_MOVE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

static void Execute::MOVE ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MOVE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	// dest ft = fs
	
	if ( i.destx )
	{
		v->vf [ i.Ft ].uw0 = v->vf [ i.Fs ].uw0;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].uw1 = v->vf [ i.Fs ].uw1;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].uw2 = v->vf [ i.Fs ].uw2;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].uw3 = v->vf [ i.Fs ].uw3;
	}
	
	// flags affected: none

#if defined INLINE_DEBUG_MOVE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

static void Execute::MTIR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MFIR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " Fs=" << hex << v->vf [ i.Fs ].vuw [ i.fsf ];
#endif

	// fsf it = fs
	
	v->vi [ i.it ].uLo = (u16) v->vf [ i.Fs ].vuw [ i.fsf ];
	
	// flags affected: none

#if defined INLINE_DEBUG_MFIR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: it=" << hex << v->vi [ i.it ].uLo;
#endif
}

static void Execute::MR32 ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MOVE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vfx=" << hex << v->vf [ i.Fs ].fx << " vfy=" << v->vf [ i.Fs ].fy << " vfz=" << v->vf [ i.Fs ].fz << " vfw=" << v->vf [ i.Fs ].fw;
#endif

	// dest ft = fs
	
	u32 temp;
	
	// must do this or data can get overwritten
	temp = v->vf [ i.Fs ].ux;
	
	if ( i.destx )
	{
		v->vf [ i.Ft ].ux = v->vf [ i.Fs ].uy;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].uy = v->vf [ i.Fs ].uz;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].uz = v->vf [ i.Fs ].uw;
	}
	
	if ( i.destw )
	{
		// note: must do it this way for correct operation
		//v->vf [ i.Ft ].uw = v->vf [ i.Fs ].ux;
		v->vf [ i.Ft ].uw = temp;
	}
	
	// flags affected: none

#if defined INLINE_DEBUG_MOVE || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].fx << " vfy=" << v->vf [ i.Ft ].fy << " vfz=" << v->vf [ i.Ft ].fz << " vfw=" << v->vf [ i.Ft ].fw;
#endif
}

static void Execute::MFIR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_MFIR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vi=" << hex << v->vi [ i.is ].uLo;
#endif

	// dest ft = is
	
	if ( i.destx )
	{
		v->vf [ i.Ft ].sw0 = (s32) v->vi [ i.is ].sLo;
	}
	
	if ( i.desty )
	{
		v->vf [ i.Ft ].sw1 = (s32) v->vi [ i.is ].sLo;
	}
	
	if ( i.destz )
	{
		v->vf [ i.Ft ].sw2 = (s32) v->vi [ i.is ].sLo;
	}
	
	if ( i.destw )
	{
		v->vf [ i.Ft ].sw3 = (s32) v->vi [ i.is ].sLo;
	}
	
	// flags affected: none

#if defined INLINE_DEBUG_MFIR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output: Ft=" << " vfx=" << hex << v->vf [ i.Ft ].sw0 << " vfy=" << v->vf [ i.Ft ].sw1 << " vfz=" << v->vf [ i.Ft ].sw2 << " vfw=" << v->vf [ i.Ft ].sw3;
#endif
}



// Random Number instructions //

static void Execute::RGET ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_RGET || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::RNEXT ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_RNEXT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::RINIT ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_RINIT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::RXOR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_RXOR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}




// Load/Store instructions //

static void Execute::SQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " it=" << v->vi [ i.it ].uLo << " fs=" << v->vf [ i.Fs ].uw0 << " " << v->vf [ i.Fs ].uw1 << " " << v->vf [ i.Fs ].uw2 << " " << v->vf [ i.Fs ].uw3;
#endif

	// SQ fsdest, Imm11(it)
	// do Imm11 x16
	
	u32 StoreAddress;
	u32* pVuMem32;
	
	StoreAddress = ( v->vi [ i.it ].uLo + i.Imm11 ) << 2;
	
	pVuMem32 = v->GetMemPtr ( StoreAddress );
	
	if ( i.destx ) pVuMem32 [ 0 ] = v->vf [ i.Fs ].uw0;
	if ( i.desty ) pVuMem32 [ 1 ] = v->vf [ i.Fs ].uw1;
	if ( i.destz ) pVuMem32 [ 2 ] = v->vf [ i.Fs ].uw2;
	if ( i.destw ) pVuMem32 [ 3 ] = v->vf [ i.Fs ].uw3;
}

static void Execute::LQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_LQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " is=" << v->vi [ i.is ].uLo << " ft=" << v->vf [ i.Ft ].uw0 << " " << v->vf [ i.Ft ].uw1 << " " << v->vf [ i.Ft ].uw2 << " " << v->vf [ i.Ft ].uw3;
#endif

	// LQ ftdest, Imm11(is)
	// do Imm11 x16
	
	u32 LoadAddress;
	u32* pVuMem32;
	
	LoadAddress = ( v->vi [ i.is ].uLo + i.Imm11 ) << 2;
	
	pVuMem32 = v->GetMemPtr ( LoadAddress );
	
	if ( i.destx ) v->vf [ i.Ft ].uw0 = pVuMem32 [ 0 ];
	if ( i.desty ) v->vf [ i.Ft ].uw1 = pVuMem32 [ 1 ];
	if ( i.destz ) v->vf [ i.Ft ].uw2 = pVuMem32 [ 2 ];
	if ( i.destw ) v->vf [ i.Ft ].uw3 = pVuMem32 [ 3 ];
}

static void Execute::SQD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SQD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " it=" << v->vi [ i.it ].uLo << " fs=" << v->vf [ i.Fs ].uw0 << " " << v->vf [ i.Fs ].uw1 << " " << v->vf [ i.Fs ].uw2 << " " << v->vf [ i.Fs ].uw3;
#endif

	// SQD fsdest, (--it)
	// do Imm11 x16
	
	u32 StoreAddress;
	u32* pVuMem32;
	
	// pre-decrement
	v->vi [ i.it ].uLo--;
	
	StoreAddress = v->vi [ i.it ].uLo << 2;
	
	pVuMem32 = v->GetMemPtr ( StoreAddress );
	
	if ( i.destx ) pVuMem32 [ 0 ] = v->vf [ i.Fs ].uw0;
	if ( i.desty ) pVuMem32 [ 1 ] = v->vf [ i.Fs ].uw1;
	if ( i.destz ) pVuMem32 [ 2 ] = v->vf [ i.Fs ].uw2;
	if ( i.destw ) pVuMem32 [ 3 ] = v->vf [ i.Fs ].uw3;
}


static void Execute::SQI ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SQI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " it=" << v->vi [ i.it ].uLo << " fs=" << v->vf [ i.Fs ].uw0 << " " << v->vf [ i.Fs ].uw1 << " " << v->vf [ i.Fs ].uw2 << " " << v->vf [ i.Fs ].uw3;
#endif

	// SQD fsdest, (it++)
	// do Imm11 x16
	
	u32 StoreAddress;
	u32* pVuMem32;
	
	StoreAddress = v->vi [ i.it ].uLo << 2;
	
	pVuMem32 = v->GetMemPtr ( StoreAddress );
	
	if ( i.destx ) pVuMem32 [ 0 ] = v->vf [ i.Fs ].uw0;
	if ( i.desty ) pVuMem32 [ 1 ] = v->vf [ i.Fs ].uw1;
	if ( i.destz ) pVuMem32 [ 2 ] = v->vf [ i.Fs ].uw2;
	if ( i.destw ) pVuMem32 [ 3 ] = v->vf [ i.Fs ].uw3;
	
	// post-increment
	v->vi [ i.it ].uLo++;
}

static void Execute::LQD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_LQD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " is=" << v->vi [ i.is ].uLo;
#endif

	// LQD ftdest, (--is)
	// do Imm11 x16
	
	u32 LoadAddress;
	u32* pVuMem32;
	
	// pre-decrement
	v->vi [ i.is ].uLo--;
	
	LoadAddress = v->vi [ i.is ].uLo << 2;
	
	pVuMem32 = v->GetMemPtr ( LoadAddress );
	
	if ( i.destx ) v->vf [ i.Ft ].uw0 = pVuMem32 [ 0 ];
	if ( i.desty ) v->vf [ i.Ft ].uw1 = pVuMem32 [ 1 ];
	if ( i.destz ) v->vf [ i.Ft ].uw2 = pVuMem32 [ 2 ];
	if ( i.destw ) v->vf [ i.Ft ].uw3 = pVuMem32 [ 3 ];
	
#if defined INLINE_DEBUG_LQD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " Output:" << " ft=" << v->vf [ i.Ft ].uw0 << " " << v->vf [ i.Ft ].uw1 << " " << v->vf [ i.Ft ].uw2 << " " << v->vf [ i.Ft ].uw3;
#endif
}

static void Execute::LQI ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_LQI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " is=" << v->vi [ i.is ].uLo;
#endif

	// LQI ftdest, (is++)
	// do Imm11 x16
	
	u32 LoadAddress;
	u32* pVuMem32;
	
	LoadAddress = v->vi [ i.is ].uLo << 2;
	
	pVuMem32 = v->GetMemPtr ( LoadAddress );
	
	if ( i.destx ) v->vf [ i.Ft ].uw0 = pVuMem32 [ 0 ];
	if ( i.desty ) v->vf [ i.Ft ].uw1 = pVuMem32 [ 1 ];
	if ( i.destz ) v->vf [ i.Ft ].uw2 = pVuMem32 [ 2 ];
	if ( i.destw ) v->vf [ i.Ft ].uw3 = pVuMem32 [ 3 ];
	
	// post-increment
	v->vi [ i.is ].uLo++;
	
#if defined INLINE_DEBUG_LQI || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << hex << " Output:" << " ft=" << v->vf [ i.Ft ].uw0 << " " << v->vf [ i.Ft ].uw1 << " " << v->vf [ i.Ft ].uw2 << " " << v->vf [ i.Ft ].uw3;
#endif
}



static void Execute::ILWR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ILWR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Base=" << v->vi [ i.is ].uLo;
#endif

	// ILWR itdest, (is)
	// do Imm11 x16
	
	
	u32 LoadAddress;
	u32* pVuMem32;
	
	LoadAddress = v->vi [ i.is ].uLo << 2;
	
	pVuMem32 = v->GetMemPtr ( LoadAddress );
	
	if ( i.destx ) v->vi [ i.it ].uLo = pVuMem32 [ 0 ];
	if ( i.desty ) v->vi [ i.it ].uLo = pVuMem32 [ 1 ];
	if ( i.destz ) v->vi [ i.it ].uLo = pVuMem32 [ 2 ];
	if ( i.destw ) v->vi [ i.it ].uLo = pVuMem32 [ 3 ];
	
#if defined INLINE_DEBUG_ILWR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output:" << " it=" << v->vi [ i.it ].uLo;
#endif
}

static void Execute::ISWR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ISWR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Base=" << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif

	// ISWR itdest, (is)
	// do Imm11 x16
	
	u32 StoreAddress;
	u32* pVuMem32;
	
	StoreAddress = v->vi [ i.is ].uLo << 2;
	
	pVuMem32 = v->GetMemPtr ( StoreAddress );
	
	if ( i.destx ) pVuMem32 [ 0 ] = v->vi [ i.it ].uLo;
	if ( i.desty ) pVuMem32 [ 1 ] = v->vi [ i.it ].uLo;
	if ( i.destz ) pVuMem32 [ 2 ] = v->vi [ i.it ].uLo;
	if ( i.destw ) pVuMem32 [ 3 ] = v->vi [ i.it ].uLo;
}

static void Execute::ILW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ILW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Base=" << v->vi [ i.is ].uLo;
#endif

	// ILW itdest, Imm11(is)
	// do Imm11 x16
	
	u32 LoadAddress;
	u32* pVuMem32;
	
	LoadAddress = ( v->vi [ i.is ].uLo + i.Imm11 ) << 2;
	
	pVuMem32 = v->GetMemPtr ( LoadAddress );
	
	if ( i.destx ) v->vi [ i.it ].uLo = pVuMem32 [ 0 ];
	if ( i.desty ) v->vi [ i.it ].uLo = pVuMem32 [ 1 ];
	if ( i.destz ) v->vi [ i.it ].uLo = pVuMem32 [ 2 ];
	if ( i.destw ) v->vi [ i.it ].uLo = pVuMem32 [ 3 ];
	
#if defined INLINE_DEBUG_ILW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << " Output:" << " it=" << v->vi [ i.it ].uLo;
#endif
}

static void Execute::ISW ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ISW || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << hex << " Base=" << v->vi [ i.is ].uLo << " it=" << v->vi [ i.it ].uLo;
#endif

	// ISW itdest, Imm11(is)
	// do Imm11 x16
	
	u32 StoreAddress;
	u32* pVuMem32;
	
	StoreAddress = ( v->vi [ i.is ].uLo + i.Imm11 ) << 2;
	
	pVuMem32 = v->GetMemPtr ( StoreAddress );
	
	if ( i.destx ) pVuMem32 [ 0 ] = v->vi [ i.it ].uLo;
	if ( i.desty ) pVuMem32 [ 1 ] = v->vi [ i.it ].uLo;
	if ( i.destz ) pVuMem32 [ 2 ] = v->vi [ i.it ].uLo;
	if ( i.destw ) pVuMem32 [ 3 ] = v->vi [ i.it ].uLo;
}







// X instructions //

static void Execute::XGKICK ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_XGKICK || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << " vi=" << hex << v->vi [ i.is ].uLo;
#endif

	// looks like this is only supposed to write one 128-bit value to PATH1
	GPU::Path1_WriteBlock ( & v->VuMem64 [ v->vi [ i.is ].uLo << 1 ] );
}


static void Execute::XTOP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_XTOP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	// it = TOP
	// this instruction is VU1 only
	v->vi [ i.it ].uLo = v->VifRegs.TOP & 0x3ff;

}

static void Execute::XITOP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_XITOP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

	if ( v->Number )
	{
		v->vi [ i.it ].uLo = v->VifRegs.ITOP & 0xff;
	}
	else
	{
		v->vi [ i.it ].uLo = v->VifRegs.ITOP & 0x3ff;
	}
}





// WAIT instructions //

static void Execute::WAITP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_WAITP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::WAITQ ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_WAITQ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}



// lower float math //

static void Execute::DIV ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_DIV || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << dec << " fs=" << ( (float&) v->vf [ i.Fs ].vuw [ i.fsf ] );
	debug << dec << " ft=" << ( (float&) v->vf [ i.Ft ].vuw [ i.ftf ] );
#endif

	float fs = (float&) v->vf [ i.Fs ].vuw [ i.fsf ];
	float ft = (float&) v->vf [ i.Ft ].vuw [ i.ftf ];
	
	//v->Q.f = PS2_Float_Div ( fs, ft, &v->divide_flag, &v->invalid_negative_flag, &v->invalid_zero_flag, &v->divide_stickyflag, &v->invalid_negative_stickyflag, &v->invalid_zero_stickyflag );
	v->vi [ 22 ].f = PS2_Float_Div ( fs, ft, & v->vi [ 16 ].sLo );
	
#if defined INLINE_DEBUG_DIV || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << dec << " Q=" << v->vi [ 22 ].f;
#endif
}

static void Execute::RSQRT ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_RSQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << dec << " fs=" << ( (float&) v->vf [ i.Fs ].vuw [ i.fsf ] );
	debug << dec << " ft=" << ( (float&) v->vf [ i.Ft ].vuw [ i.ftf ] );
#endif

	float fs = (float&) v->vf [ i.Fs ].vuw [ i.fsf ];
	float ft = (float&) v->vf [ i.Ft ].vuw [ i.ftf ];
	
	//v->Q.f = PS2_Float_RSqrt ( fs, ft, &v->divide_flag, &v->invalid_negative_flag, &v->invalid_zero_flag, &v->divide_stickyflag, &v->invalid_negative_stickyflag, &v->invalid_zero_stickyflag );
	v->vi [ 22 ].f = PS2_Float_RSqrt ( fs, ft, & v->vi [ 16 ].sLo );
	
#if defined INLINE_DEBUG_RSQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << dec << " Q=" << v->vi [ 22 ].f;
#endif
}

static void Execute::SQRT ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_SQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
	debug << dec << " fs=" << ( (float&) v->vf [ i.Fs ].vuw [ i.fsf ] );
	debug << dec << " ft=" << ( (float&) v->vf [ i.Ft ].vuw [ i.ftf ] );
#endif

	float ft = (float&) v->vf [ i.Ft ].vuw [ i.ftf ];
	
	//v->Q.f = PS2_Float_Sqrt ( ft, &v->divide_flag, &v->invalid_negative_flag, &v->invalid_zero_flag, &v->divide_stickyflag, &v->invalid_negative_stickyflag, &v->invalid_zero_stickyflag );
	v->vi [ 22 ].f = PS2_Float_Sqrt ( ft, & v->vi [ 16 ].sLo );
	
#if defined INLINE_DEBUG_SQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT	// || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << dec << " Q=" << v->vi [ 22 ].f;
#endif
}




// External unit //

static void Execute::EATAN ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_EATAN || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::EATANxy ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_EATANXY || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::EATANxz ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_EATANXZ || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::ERSQRT ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ERSQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::ERCPR ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ERCPR || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::EEXP ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_EEXP || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::ESIN ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ESIN || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::ESQRT ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ESQRT || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::ESADD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ESADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}

static void Execute::ERSADD ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ERSADD || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}


static void Execute::ESUM ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ESUM || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}


static void Execute::ELENG ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ELENG || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}


static void Execute::ERLENG ( VU *v, Instruction::Format i )
{
#if defined INLINE_DEBUG_ERLENG || defined INLINE_DEBUG_VU || defined INLINE_DEBUG_INT || defined INLINE_DEBUG_UNIMPLEMENTED
	debug << "\r\n" << hex << "VU#" << v->Number << " " << setw( 8 ) << v->PC << " " << dec << v->CycleCount << " " << Print::PrintInstructionLO ( i.Value ).c_str () << "; " << hex << i.Value;
#endif

}










static const Execute::Function Execute::FunctionList []
{
	Execute::INVALID,
	
	// VU macro mode instructions //
	
	//Execute::COP2
	//Execute::QMFC2_NI, Execute::QMFC2_I, Execute::QMTC2_NI, Execute::QMTC2_I, Execute::LQC2, Execute::SQC2,
	//Execute::CALLMS, Execute::CALLMSR,
	
	// upper instructions //
	
	Execute::ABS,
	Execute::ADD, Execute::ADDi, Execute::ADDq, Execute::ADDBCX, Execute::ADDBCY, Execute::ADDBCZ, Execute::ADDBCW,
	Execute::ADDA, Execute::ADDAi, Execute::ADDAq, Execute::ADDABCX, Execute::ADDABCY, Execute::ADDABCZ, Execute::ADDABCW,
	Execute::CLIP,
	Execute::FTOI0, Execute::FTOI4, Execute::FTOI12, Execute::FTOI15,
	Execute::ITOF0, Execute::ITOF4, Execute::ITOF12, Execute::ITOF15,
	
	Execute::MADD, Execute::MADDi, Execute::MADDq, Execute::MADDBCX, Execute::MADDBCY, Execute::MADDBCZ, Execute::MADDBCW,
	Execute::MADDA, Execute::MADDAi, Execute::MADDAq, Execute::MADDABCX, Execute::MADDABCY, Execute::MADDABCZ, Execute::MADDABCW,
	Execute::MAX, Execute::MAXi, Execute::MAXBCX, Execute::MAXBCY, Execute::MAXBCZ, Execute::MAXBCW,
	Execute::MINI, Execute::MINIi, Execute::MINIBCX, Execute::MINIBCY, Execute::MINIBCZ, Execute::MINIBCW,
	
	Execute::MSUB, Execute::MSUBi, Execute::MSUBq, Execute::MSUBBCX, Execute::MSUBBCY, Execute::MSUBBCZ, Execute::MSUBBCW,
	Execute::MSUBA, Execute::MSUBAi, Execute::MSUBAq, Execute::MSUBABCX, Execute::MSUBABCY, Execute::MSUBABCZ, Execute::MSUBABCW,
	Execute::MUL, Execute::MULi, Execute::MULq, Execute::MULBCX, Execute::MULBCY, Execute::MULBCZ, Execute::MULBCW,
	Execute::MULA, Execute::MULAi, Execute::MULAq, Execute::MULABCX, Execute::MULABCY, Execute::MULABCZ, Execute::MULABCW,
	Execute::NOP, Execute::OPMSUB, Execute::OPMULA,
	Execute::SUB, Execute::SUBi, Execute::SUBq, Execute::SUBBCX, Execute::SUBBCY, Execute::SUBBCZ, Execute::SUBBCW,
	Execute::SUBA, Execute::SUBAi, Execute::SUBAq, Execute::SUBABCX, Execute::SUBABCY, Execute::SUBABCZ, Execute::SUBABCW,
	
	// lower instructions //
	
	Execute::DIV,
	Execute::IADD, Execute::IADDI, Execute::IAND,
	Execute::ILWR,
	Execute::IOR, Execute::ISUB,
	Execute::ISWR,
	Execute::LQD, Execute::LQI,
	Execute::MFIR, Execute::MOVE, Execute::MR32, Execute::MTIR,
	Execute::RGET, Execute::RINIT, Execute::RNEXT,
	Execute::RSQRT,
	Execute::RXOR,
	Execute::SQD, Execute::SQI,
	Execute::SQRT,
	Execute::WAITQ,

	// instructions not in macro mode //
	
	Execute::B, Execute::BAL,
	Execute::FCAND, Execute::FCEQ, Execute::FCGET, Execute::FCOR, Execute::FCSET,
	Execute::FMAND, Execute::FMEQ, Execute::FMOR,
	Execute::FSAND, Execute::FSEQ, Execute::FSOR, Execute::FSSET,
	Execute::IADDIU,
	Execute::IBEQ, Execute::IBGEZ, Execute::IBGTZ, Execute::IBLEZ, Execute::IBLTZ, Execute::IBNE,
	Execute::ILW,
	Execute::ISUBIU, Execute::ISW,
	Execute::JALR, Execute::JR,
	Execute::LQ,
	Execute::MFP,
	Execute::SQ,
	Execute::WAITP,
	Execute::XGKICK, Execute::XITOP, Execute::XTOP,

	// External Unit //

	Execute::EATAN, Execute::EATANxy, Execute::EATANxz, Execute::EEXP, Execute::ELENG, Execute::ERCPR, Execute::ERLENG, Execute::ERSADD,
	Execute::ERSQRT, Execute::ESADD, Execute::ESIN, Execute::ESQRT, Execute::ESUM
};





