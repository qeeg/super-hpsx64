

#include <sstream>
#include <string>


#include "types.h"
#include "MipsOpcode.h"
#include "VU.h"
#include "VU_Instruction.h"
#include "VU_Lookup.h"

using namespace std;
using namespace Vu::Instruction;
using namespace Playstation2;


#ifndef _VU_EXECUTE_H_
#define _VU_EXECUTE_H_


namespace Vu
{

	namespace Instruction
	{
	
		class Execute
		{
		public:
		
			static Debug::Log debug;

			
			//VU *v;
			
			
			
			typedef void (*Function) ( VU *v, Instruction::Format i );
			

			// upper instruction type op
			typedef float (*OpType0) ( float fs, float ft, int index, short* StatusFlag, short* MACFlag );
			
			// upper instruction with accumulator type op
			typedef float (*OpType1) ( double dACC, float fs, float ft, int index, short* StatusFlag, short* MACFlag );
			
			// upper instruction type op store to ACC
			typedef double (*OpType0A) ( float fs, float ft, int index, short* StatusFlag, short* MACFlag );
										
			// upper instruction with accumulator type op w/ store to ACC
			typedef double (*OpType1A) ( double dACC, float fs, float ft, int index, short* StatusFlag, short* MACFlag );
										
			// lower floating point instruction (sqrt,rsqrt,div,etc) type op
			typedef float (*OpType2) ( float fs, float ft, short* StatusFlag );
			
			
			inline static void VuUpperOp ( VU *v, Instruction::Format i, OpType0 OpFunc0 )
			{
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fx, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fy, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fz, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fw, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpI ( VU *v, Instruction::Format i, OpType0 OpFunc0 )
			{
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->vf [ i.Fs ].fx, v->vi [ 21 ].f, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->vf [ i.Fs ].fy, v->vi [ 21 ].f, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->vf [ i.Fs ].fz, v->vi [ 21 ].f, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->vf [ i.Fs ].fw, v->vi [ 21 ].f, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpQ ( VU *v, Instruction::Format i, OpType0 OpFunc0 )
			{
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->vf [ i.Fs ].fx, v->vi [ 22 ].f, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->vf [ i.Fs ].fy, v->vi [ 22 ].f, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->vf [ i.Fs ].fz, v->vi [ 22 ].f, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->vf [ i.Fs ].fw, v->vi [ 22 ].f, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpX ( VU *v, Instruction::Format i, OpType0 OpFunc0 )
			{
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fx, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fx, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fx, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fx, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpY ( VU *v, Instruction::Format i, OpType0 OpFunc0 )
			{
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fy, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fy, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fy, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fy, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpZ ( VU *v, Instruction::Format i, OpType0 OpFunc0 )
			{
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fz, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fz, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fz, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fz, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpW ( VU *v, Instruction::Format i, OpType0 OpFunc0 )
			{
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fw, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fw, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fw, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fw, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

// -------------------------------------------

			inline static void VuUpperOp_A ( VU *v, Instruction::Format i, OpType0A OpFunc0 )
			{
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->vf [ i.Fs ].fx,  v->vf [ i.Ft ].fx, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->vf [ i.Fs ].fy,  v->vf [ i.Ft ].fy, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->vf [ i.Fs ].fz,  v->vf [ i.Ft ].fz, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->vf [ i.Fs ].fw,  v->vf [ i.Ft ].fw, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
			}

			inline static void VuUpperOpI_A ( VU *v, Instruction::Format i, OpType0A OpFunc0 )
			{
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->vf [ i.Fs ].fx, v->vi [ 21 ].f, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->vf [ i.Fs ].fy, v->vi [ 21 ].f, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->vf [ i.Fs ].fz, v->vi [ 21 ].f, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->vf [ i.Fs ].fw, v->vi [ 21 ].f, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
			}

			inline static void VuUpperOpQ_A ( VU *v, Instruction::Format i, OpType0A OpFunc0 )
			{
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->vf [ i.Fs ].fx, v->vi [ 22 ].f, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->vf [ i.Fs ].fy, v->vi [ 22 ].f, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->vf [ i.Fs ].fz, v->vi [ 22 ].f, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->vf [ i.Fs ].fw, v->vi [ 22 ].f, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
			}

			inline static void VuUpperOpX_A ( VU *v, Instruction::Format i, OpType0A OpFunc0 )
			{
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->vf [ i.Fs ].fx,  v->vf [ i.Ft ].fx, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->vf [ i.Fs ].fy,  v->vf [ i.Ft ].fx, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->vf [ i.Fs ].fz,  v->vf [ i.Ft ].fx, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->vf [ i.Fs ].fw,  v->vf [ i.Ft ].fx, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
			}

			inline static void VuUpperOpY_A ( VU *v, Instruction::Format i, OpType0A OpFunc0 )
			{
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->vf [ i.Fs ].fx,  v->vf [ i.Ft ].fy, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->vf [ i.Fs ].fy,  v->vf [ i.Ft ].fy, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->vf [ i.Fs ].fz,  v->vf [ i.Ft ].fy, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->vf [ i.Fs ].fw,  v->vf [ i.Ft ].fy, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
			}

			inline static void VuUpperOpZ_A ( VU *v, Instruction::Format i, OpType0A OpFunc0 )
			{
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->vf [ i.Fs ].fx,  v->vf [ i.Ft ].fz, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->vf [ i.Fs ].fy,  v->vf [ i.Ft ].fz, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->vf [ i.Fs ].fz,  v->vf [ i.Ft ].fz, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->vf [ i.Fs ].fw,  v->vf [ i.Ft ].fz, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
			}

			inline static void VuUpperOpW_A ( VU *v, Instruction::Format i, OpType0A OpFunc0 )
			{
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->vf [ i.Fs ].fx,  v->vf [ i.Ft ].fw, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->vf [ i.Fs ].fy,  v->vf [ i.Ft ].fw, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->vf [ i.Fs ].fz,  v->vf [ i.Ft ].fw, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->vf [ i.Fs ].fw,  v->vf [ i.Ft ].fw, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
												
				}
			}



			static const Function FunctionList [];
			
			inline static void ExecuteInstructionLO ( VU *v, Instruction::Format i ) { FunctionList [ Lookup::FindByInstructionLO ( i.Value ) ] ( v, i ); }
			inline static void ExecuteInstructionHI ( VU *v, Instruction::Format i ) { FunctionList [ Lookup::FindByInstructionHI ( i.Value ) ] ( v, i ); }
			
			
			
			static const char XyzwLUT [ 4 ];	// = { 'x', 'y', 'z', 'w' };
			static const char* BCType [ 4 ];// = { "F", "T", "FL", "TL" };

			
			
			// constructor
			//Execute ( Playstation2::VU *vv ) { v = vv; }
			
			// creates lookup table from list of entries
			static void Start ();
		
			

			static void INVALID ( VU *v, Instruction::Format i );
			

			static void ADDBCX ( VU *v, Instruction::Format i );
			static void ADDBCY ( VU *v, Instruction::Format i );
			static void ADDBCZ ( VU *v, Instruction::Format i );
			static void ADDBCW ( VU *v, Instruction::Format i );
			
			static void SUBBCX ( VU *v, Instruction::Format i );
			static void SUBBCY ( VU *v, Instruction::Format i );
			static void SUBBCZ ( VU *v, Instruction::Format i );
			static void SUBBCW ( VU *v, Instruction::Format i );
			
			static void MADDBCX ( VU *v, Instruction::Format i );
			static void MADDBCY ( VU *v, Instruction::Format i );
			static void MADDBCZ ( VU *v, Instruction::Format i );
			static void MADDBCW ( VU *v, Instruction::Format i );
			
			static void MSUBBCX ( VU *v, Instruction::Format i );
			static void MSUBBCY ( VU *v, Instruction::Format i );
			static void MSUBBCZ ( VU *v, Instruction::Format i );
			static void MSUBBCW ( VU *v, Instruction::Format i );
			
			static void MAXBCX ( VU *v, Instruction::Format i );
			static void MAXBCY ( VU *v, Instruction::Format i );
			static void MAXBCZ ( VU *v, Instruction::Format i );
			static void MAXBCW ( VU *v, Instruction::Format i );
			
			static void MINIBCX ( VU *v, Instruction::Format i );
			static void MINIBCY ( VU *v, Instruction::Format i );
			static void MINIBCZ ( VU *v, Instruction::Format i );
			static void MINIBCW ( VU *v, Instruction::Format i );
			
			static void MULBCX ( VU *v, Instruction::Format i );
			static void MULBCY ( VU *v, Instruction::Format i );
			static void MULBCZ ( VU *v, Instruction::Format i );
			static void MULBCW ( VU *v, Instruction::Format i );
			
			static void MULq ( VU *v, Instruction::Format i );
			
			static void MAXi ( VU *v, Instruction::Format i );
			static void MULi ( VU *v, Instruction::Format i );
			static void MINIi ( VU *v, Instruction::Format i );
			static void ADDq ( VU *v, Instruction::Format i );
			static void MADDq ( VU *v, Instruction::Format i );
			static void ADDi ( VU *v, Instruction::Format i );
			static void MADDi ( VU *v, Instruction::Format i );
			static void OPMSUB ( VU *v, Instruction::Format i );
			static void SUBq ( VU *v, Instruction::Format i );
			static void MSUBq ( VU *v, Instruction::Format i );
			static void SUBi ( VU *v, Instruction::Format i );
			static void MSUBi ( VU *v, Instruction::Format i );
			static void ADD ( VU *v, Instruction::Format i );
			
			//static void ADDi ( VU *v, Instruction::Format i );
			//static void ADDq ( VU *v, Instruction::Format i );
			//static void ADDAi ( VU *v, Instruction::Format i );
			//static void ADDAq ( VU *v, Instruction::Format i );
			
			static void ADDABCX ( VU *v, Instruction::Format i );
			static void ADDABCY ( VU *v, Instruction::Format i );
			static void ADDABCZ ( VU *v, Instruction::Format i );
			static void ADDABCW ( VU *v, Instruction::Format i );
			
			static void MADD ( VU *v, Instruction::Format i );
			
			static void MUL ( VU *v, Instruction::Format i );
			static void MAX ( VU *v, Instruction::Format i );
			static void SUB ( VU *v, Instruction::Format i );
			static void MSUB ( VU *v, Instruction::Format i );
			static void OPMSUM ( VU *v, Instruction::Format i );
			static void MINI ( VU *v, Instruction::Format i );
			static void IADD ( VU *v, Instruction::Format i );
			static void ISUB ( VU *v, Instruction::Format i );
			static void IADDI ( VU *v, Instruction::Format i );
			static void IAND ( VU *v, Instruction::Format i );
			static void IOR ( VU *v, Instruction::Format i );
			//static void CALLMS ( VU *v, Instruction::Format i );
			//static void CALLMSR ( VU *v, Instruction::Format i );
			static void ITOF0 ( VU *v, Instruction::Format i );
			static void FTOI0 ( VU *v, Instruction::Format i );
			static void MULAq ( VU *v, Instruction::Format i );
			static void ADDAq ( VU *v, Instruction::Format i );
			static void SUBAq ( VU *v, Instruction::Format i );
			static void ADDA ( VU *v, Instruction::Format i );
			static void SUBA ( VU *v, Instruction::Format i );
			static void MOVE ( VU *v, Instruction::Format i );
			static void LQI ( VU *v, Instruction::Format i );
			static void DIV ( VU *v, Instruction::Format i );
			static void MTIR ( VU *v, Instruction::Format i );
			//static void RNEXT ( VU *v, Instruction::Format i );
			static void ITOF4 ( VU *v, Instruction::Format i );
			static void FTOI4 ( VU *v, Instruction::Format i );
			static void ABS ( VU *v, Instruction::Format i );
			static void MADDAq ( VU *v, Instruction::Format i );
			static void MSUBAq ( VU *v, Instruction::Format i );
			static void MADDA ( VU *v, Instruction::Format i );
			static void MSUBA ( VU *v, Instruction::Format i );
			//static void MR32 ( VU *v, Instruction::Format i );
			//static void SQI ( VU *v, Instruction::Format i );
			//static void SQRT ( VU *v, Instruction::Format i );
			//static void MFIR ( VU *v, Instruction::Format i );
			//static void RGET ( VU *v, Instruction::Format i );
			
			//static void ADDABCX ( VU *v, Instruction::Format i );
			//static void ADDABCY ( VU *v, Instruction::Format i );
			//static void ADDABCZ ( VU *v, Instruction::Format i );
			//static void ADDABCW ( VU *v, Instruction::Format i );
			
			static void SUBABCX ( VU *v, Instruction::Format i );
			static void SUBABCY ( VU *v, Instruction::Format i );
			static void SUBABCZ ( VU *v, Instruction::Format i );
			static void SUBABCW ( VU *v, Instruction::Format i );
			
			static void MADDABCX ( VU *v, Instruction::Format i );
			static void MADDABCY ( VU *v, Instruction::Format i );
			static void MADDABCZ ( VU *v, Instruction::Format i );
			static void MADDABCW ( VU *v, Instruction::Format i );
			
			static void MSUBABCX ( VU *v, Instruction::Format i );
			static void MSUBABCY ( VU *v, Instruction::Format i );
			static void MSUBABCZ ( VU *v, Instruction::Format i );
			static void MSUBABCW ( VU *v, Instruction::Format i );
			
			static void ITOF12 ( VU *v, Instruction::Format i );
			static void FTOI12 ( VU *v, Instruction::Format i );
			
			static void MULABCX ( VU *v, Instruction::Format i );
			static void MULABCY ( VU *v, Instruction::Format i );
			static void MULABCZ ( VU *v, Instruction::Format i );
			static void MULABCW ( VU *v, Instruction::Format i );
			
			static void MULAi ( VU *v, Instruction::Format i );
			static void ADDAi ( VU *v, Instruction::Format i );
			static void SUBAi ( VU *v, Instruction::Format i );
			static void MULA ( VU *v, Instruction::Format i );
			static void OPMULA ( VU *v, Instruction::Format i );
			//static void LQD ( VU *v, Instruction::Format i );
			//static void RSQRT ( VU *v, Instruction::Format i );
			//static void ILWR ( VU *v, Instruction::Format i );
			//static void RINIT ( VU *v, Instruction::Format i );
			static void ITOF15 ( VU *v, Instruction::Format i );
			static void FTOI15 ( VU *v, Instruction::Format i );
			static void CLIP ( VU *v, Instruction::Format i );
			static void MADDAi ( VU *v, Instruction::Format i );
			static void MSUBAi ( VU *v, Instruction::Format i );
			static void NOP ( VU *v, Instruction::Format i );
			//static void SQD ( VU *v, Instruction::Format i );


			// lower instructions

			
			static void LQ ( VU *v, Instruction::Format i );
			static void SQ ( VU *v, Instruction::Format i );
			static void ILW ( VU *v, Instruction::Format i );
			static void ISW ( VU *v, Instruction::Format i );
			static void IADDIU ( VU *v, Instruction::Format i );
			static void ISUBIU ( VU *v, Instruction::Format i );
			static void FCEQ ( VU *v, Instruction::Format i );
			static void FCSET ( VU *v, Instruction::Format i );
			static void FCAND ( VU *v, Instruction::Format i );
			static void FCOR ( VU *v, Instruction::Format i );
			static void FSEQ ( VU *v, Instruction::Format i );
			static void FSSET ( VU *v, Instruction::Format i );
			static void FSAND ( VU *v, Instruction::Format i );
			static void FSOR ( VU *v, Instruction::Format i );
			static void FMEQ ( VU *v, Instruction::Format i );
			static void FMAND ( VU *v, Instruction::Format i );
			static void FMOR ( VU *v, Instruction::Format i );
			static void FCGET ( VU *v, Instruction::Format i );
			static void B ( VU *v, Instruction::Format i );
			static void BAL ( VU *v, Instruction::Format i );
			static void JR ( VU *v, Instruction::Format i );
			static void JALR ( VU *v, Instruction::Format i );
			static void IBEQ ( VU *v, Instruction::Format i );
			static void IBNE ( VU *v, Instruction::Format i );
			static void IBLTZ ( VU *v, Instruction::Format i );
			static void IBGTZ ( VU *v, Instruction::Format i );
			static void IBLEZ ( VU *v, Instruction::Format i );
			static void IBGEZ ( VU *v, Instruction::Format i );
			
			//static void LowerOp ( VU *v, Instruction::Format i );
			//static void Lower60 ( VU *v, Instruction::Format i );
			//static void Lower61 ( VU *v, Instruction::Format i );
			//static void Lower62 ( VU *v, Instruction::Format i );
			//static void Lower63 ( VU *v, Instruction::Format i );
			
			//static void DIV ( VU *v, Instruction::Format i );
			//static void EATANxy ( VU *v, Instruction::Format i );
			//static void EATANxz ( VU *v, Instruction::Format i );
			//static void EATAN ( VU *v, Instruction::Format i );
			//static void IADD ( VU *v, Instruction::Format i );
			//static void ISUB ( VU *v, Instruction::Format i );
			//static void IADDI ( VU *v, Instruction::Format i );
			//static void IAND ( VU *v, Instruction::Format i );
			//static void IOR ( VU *v, Instruction::Format i );
			//static void MOVE ( VU *v, Instruction::Format i );
			//static void LQI ( VU *v, Instruction::Format i );
			//static void DIV ( VU *v, Instruction::Format i );
			//static void MTIR ( VU *v, Instruction::Format i );
			static void RNEXT ( VU *v, Instruction::Format i );
			static void MFP ( VU *v, Instruction::Format i );
			static void XTOP ( VU *v, Instruction::Format i );
			static void XGKICK ( VU *v, Instruction::Format i );

			static void MR32 ( VU *v, Instruction::Format i );
			static void SQI ( VU *v, Instruction::Format i );
			static void SQRT ( VU *v, Instruction::Format i );
			static void MFIR ( VU *v, Instruction::Format i );
			static void RGET ( VU *v, Instruction::Format i );
			
			static void XITOP ( VU *v, Instruction::Format i );
			static void ESADD ( VU *v, Instruction::Format i );
			static void EATANxy ( VU *v, Instruction::Format i );
			static void ESQRT ( VU *v, Instruction::Format i );
			static void ESIN ( VU *v, Instruction::Format i );
			static void ERSADD ( VU *v, Instruction::Format i );
			static void EATANxz ( VU *v, Instruction::Format i );
			static void ERSQRT ( VU *v, Instruction::Format i );
			static void EATAN ( VU *v, Instruction::Format i );
			static void LQD ( VU *v, Instruction::Format i );
			static void RSQRT ( VU *v, Instruction::Format i );
			static void ILWR ( VU *v, Instruction::Format i );
			static void RINIT ( VU *v, Instruction::Format i );
			static void ELENG ( VU *v, Instruction::Format i );
			static void ESUM ( VU *v, Instruction::Format i );
			static void ERCPR ( VU *v, Instruction::Format i );
			static void EEXP ( VU *v, Instruction::Format i );
			static void SQD ( VU *v, Instruction::Format i );
			static void WAITQ ( VU *v, Instruction::Format i );
			static void ISWR ( VU *v, Instruction::Format i );
			static void RXOR ( VU *v, Instruction::Format i );
			static void ERLENG ( VU *v, Instruction::Format i );
			static void WAITP ( VU *v, Instruction::Format i );
			
			
			inline static void ClearFlags ( VU *v )
			{
				// clear affected non-sticky status flags (O,U,S,Z)
				// note: status flag should only get cleared before the full SIMD instruction has executed
				v->vi [ 16 ].uLo &= ~0xf;
				
				// clear MAC flag - this should be the correct operation
				// note: this must only be cleared before execution of instruction
				v->vi [ 17 ].uLo = 0;
			}
			
			
			inline static void VuUpperOp ( VU *v, Instruction::Format i, OpType1 OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fx, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fy, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz,  v->vf [ i.Ft ].fz, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw,  v->vf [ i.Ft ].fw, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpI ( VU *v, Instruction::Format i, OpType1 OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx, v->vi [ 21 ].f, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy, v->vi [ 21 ].f, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz, v->vi [ 21 ].f, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw, v->vi [ 21 ].f, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpQ ( VU *v, Instruction::Format i, OpType1 OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx, v->vi [ 22 ].f, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy, v->vi [ 22 ].f, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz, v->vi [ 22 ].f, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw, v->vi [ 22 ].f, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpX ( VU *v, Instruction::Format i, OpType1 OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fx, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fx, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fx, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fx, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpY ( VU *v, Instruction::Format i, OpType1 OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fy, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fy, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fy, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fy, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpZ ( VU *v, Instruction::Format i, OpType1 OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fz, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fz, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fz, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fz, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpW ( VU *v, Instruction::Format i, OpType1 OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->vf [ i.Fd ].fx = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fw, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->vf [ i.Fd ].fy = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fw, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->vf [ i.Fd ].fz = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz,  v->vf [ i.Ft ].fw, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->vf [ i.Fd ].fw = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw,  v->vf [ i.Ft ].fw, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}
			
// ------------------------------------------------------

			inline static void VuUpperOp_A ( VU *v, Instruction::Format i, OpType1A OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx,  v->vf [ i.Ft ].fx, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy,  v->vf [ i.Ft ].fy, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz,  v->vf [ i.Ft ].fz, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw,  v->vf [ i.Ft ].fw, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpI_A ( VU *v, Instruction::Format i, OpType1A OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx, v->vi [ 21 ].f, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy, v->vi [ 21 ].f, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz, v->vi [ 21 ].f, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw, v->vi [ 21 ].f, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpQ_A ( VU *v, Instruction::Format i, OpType1A OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx, v->vi [ 22 ].f, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy, v->vi [ 22 ].f, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz, v->vi [ 22 ].f, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw, v->vi [ 22 ].f, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpX_A ( VU *v, Instruction::Format i, OpType1A OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fx, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fx, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fx, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fx, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpY_A ( VU *v, Instruction::Format i, OpType1A OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fy, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fy, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fy, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fy, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpZ_A ( VU *v, Instruction::Format i, OpType1A OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fz, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fz, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fz, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fz, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			inline static void VuUpperOpW_A ( VU *v, Instruction::Format i, OpType1A OpFunc0 )
			{
				// clear flags
				ClearFlags ( v );
				
				if ( i.destx )
				{
					v->dACC [ 0 ].d = OpFunc0 ( v->dACC [ 0 ].d, v->vf [ i.Fs ].fx, v->vf [ i.Ft ].fw, 3, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.desty )
				{
					v->dACC [ 1 ].d = OpFunc0 ( v->dACC [ 1 ].d, v->vf [ i.Fs ].fy, v->vf [ i.Ft ].fw, 2, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destz )
				{
					v->dACC [ 2 ].d = OpFunc0 ( v->dACC [ 2 ].d, v->vf [ i.Fs ].fz, v->vf [ i.Ft ].fw, 1, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
				
				if ( i.destw )
				{
					v->dACC [ 3 ].d = OpFunc0 ( v->dACC [ 3 ].d, v->vf [ i.Fs ].fw, v->vf [ i.Ft ].fw, 0, & v->vi [ 16 ].sLo, & v->vi [ 17 ].sLo );
				}
			}

			
		};
		
	};
	
};


#endif


