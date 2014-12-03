
#include "R3000A.h"
#include "R3000A_Lookup.h"
#include "x64Encoder.h"


#ifndef _R3000A_RECOMPILER_H_
#define _R3000A_RECOMPILER_H_

namespace R3000A
{
	// will probably create two recompilers for each processor. one for single stepping and one for multi-stepping
	class Recompiler
	{
	public:
		// number of code cache slots total
		static const int c_iNumBlocks = 1024;
		static const u32 c_iNumBlocks_Mask = c_iNumBlocks - 1;
		//static const int c_iBlockSize_Shift = 4;
		//static const int c_iBlockSize_Mask = ( 1 << c_iBlockSize_Shift ) - 1;
		
		static const u32 c_iAddress_Mask = 0x1fffffff;
		
		u32 TotalCodeCache_Size;	// = c_iNumBlocks * ( 1 << c_iBlockSize_Shift );
		
		u32 BlockSize;
		u32 BlockSize_Shift;
		u32 BlockSize_Mask;
		
		u32 MaxStep;
		//u32 MaxStep_Shift;
		//u32 MaxStep_Mask;
		
		static x64Encoder *e;
		static ICache_Device *ICache;
		static Cpu *r;
		
		// constructor
		// block size must be power of two, multiplier shift value
		// so for BlockSize_PowerOfTwo, for a block size of 4 pass 2, block size of 8 pass 3, etc
		// for MaxStep, use 0 for single stepping, 1 for stepping until end of 1 cache block, 2 for stepping until end of 2 cache blocks, etc
		Recompiler ( u32 BlockSize_PowerOfTwo, u32 MaxIStep, Cpu* R3000ACpu );
		
		// destructor
		~Recompiler ();
		
		
		void Reset () { memset ( this, 0, sizeof( Recompiler ) ); }

		
		// accessors
		inline u32 Get_DoNotCache ( u32 Address ) { return DoNotCache [ ( Address >> 2 ) & c_iNumBlocks_Mask ]; }
		inline u32 Get_CacheMissCount ( u32 Address ) { return CacheMissCount [ ( Address >> 2 ) & c_iNumBlocks_Mask ]; }
		inline u32 Get_StartAddress ( u32 Address ) { return StartAddress [ ( Address >> 2 ) & c_iNumBlocks_Mask ]; }
		inline u32 Get_LastAddress ( u32 Address ) { return LastAddress [ ( Address >> 2 ) & c_iNumBlocks_Mask ]; }
		inline u32 Get_MaxCycles ( u32 Address ) { return MaxCycles [ ( Address >> 2 ) & c_iNumBlocks_Mask ]; }
		
		// says if block points to an R3000A instruction that should NOT be EVER cached
		u8 DoNotCache [ c_iNumBlocks ];
		
		// number of times a cache miss was encountered while recompiling for this block
		u32 CacheMissCount [ c_iNumBlocks ];
		
		// code cache block not invalidated
		//u8 isValid [ c_iNumBlocks ];
		
		// start address for instruction encodings (inclusive)
		u32 StartAddress [ c_iNumBlocks ];
		
		// last address that instruction encoding is valid for (inclusive)
		u32 LastAddress [ c_iNumBlocks ];
		
		// max number of cycles that instruction encoding could use up if executed
		// need to know this to single step when there are interrupts in between
		// code block is not valid when this is zero
		u32 MaxCycles [ c_iNumBlocks ];
		
		// recompiler function
		// returns -1 if the instruction cannot be recompiled, otherwise returns the maximum number of cycles the instruction uses
		typedef long (*Function) ( Instruction::Format Instruction, u32 Address );

		// *** todo *** do not recompile more than one instruction if currently in a branch delay slot or load delay slot!!
		void RecompileInstructions ( u32 StartAddress );
		void Invalidate ( u32 Address );
		
		//void Recompile ( u32 Instruction );
		
			static const Function FunctionList [];
			
		// used by object to recompile an instruction into a code block
		// returns -1 if the instruction cannot be recompiled
		// returns 0 if the instruction was recompiled, but MUST start a new block for the next instruction (because it is guaranteed in a delay slot)
		// returns 1 if successful and can continue recompiling
		inline static long Recompile ( Instruction::Format i, u32 Address ) { return FunctionList [ Instruction::Lookup::FindByInstruction ( i.Value ) ] ( i, Address ); }
		
		
		inline static void Run ( u32 Address ) { e->ExecuteCodeBlock ( ( Address >> 2 ) & c_iNumBlocks_Mask ); }

			static long Invalid ( Instruction::Format i, u32 Address );

			static long J ( Instruction::Format i, u32 Address );
			static long JAL ( Instruction::Format i, u32 Address );
			static long BEQ ( Instruction::Format i, u32 Address );
			static long BNE ( Instruction::Format i, u32 Address );
			static long BLEZ ( Instruction::Format i, u32 Address );
			static long BGTZ ( Instruction::Format i, u32 Address );
			static long ADDI ( Instruction::Format i, u32 Address );
			static long ADDIU ( Instruction::Format i, u32 Address );
			static long SLTI ( Instruction::Format i, u32 Address );
			static long SLTIU ( Instruction::Format i, u32 Address );
			static long ANDI ( Instruction::Format i, u32 Address );
			static long ORI ( Instruction::Format i, u32 Address );
			static long XORI ( Instruction::Format i, u32 Address );
			static long LUI ( Instruction::Format i, u32 Address );
			static long LB ( Instruction::Format i, u32 Address );
			static long LH ( Instruction::Format i, u32 Address );
			static long LWL ( Instruction::Format i, u32 Address );
			static long LW ( Instruction::Format i, u32 Address );
			static long LBU ( Instruction::Format i, u32 Address );
			static long LHU ( Instruction::Format i, u32 Address );
			
			static long LWR ( Instruction::Format i, u32 Address );
			static long SB ( Instruction::Format i, u32 Address );
			static long SH ( Instruction::Format i, u32 Address );
			static long SWL ( Instruction::Format i, u32 Address );
			static long SW ( Instruction::Format i, u32 Address );
			static long SWR ( Instruction::Format i, u32 Address );
			static long LWC2 ( Instruction::Format i, u32 Address );
			static long SWC2 ( Instruction::Format i, u32 Address );
			static long SLL ( Instruction::Format i, u32 Address );
			static long SRL ( Instruction::Format i, u32 Address );
			static long SRA ( Instruction::Format i, u32 Address );
			static long SLLV ( Instruction::Format i, u32 Address );
			static long SRLV ( Instruction::Format i, u32 Address );
			static long SRAV ( Instruction::Format i, u32 Address );
			static long JR ( Instruction::Format i, u32 Address );
			static long JALR ( Instruction::Format i, u32 Address );
			static long SYSCALL ( Instruction::Format i, u32 Address );
			static long BREAK ( Instruction::Format i, u32 Address );
			static long MFHI ( Instruction::Format i, u32 Address );
			static long MTHI ( Instruction::Format i, u32 Address );

			static long MFLO ( Instruction::Format i, u32 Address );
			static long MTLO ( Instruction::Format i, u32 Address );
			static long MULT ( Instruction::Format i, u32 Address );
			static long MULTU ( Instruction::Format i, u32 Address );
			static long DIV ( Instruction::Format i, u32 Address );
			static long DIVU ( Instruction::Format i, u32 Address );
			static long ADD ( Instruction::Format i, u32 Address );
			static long ADDU ( Instruction::Format i, u32 Address );
			static long SUB ( Instruction::Format i, u32 Address );
			static long SUBU ( Instruction::Format i, u32 Address );
			static long AND ( Instruction::Format i, u32 Address );
			static long OR ( Instruction::Format i, u32 Address );
			static long XOR ( Instruction::Format i, u32 Address );
			static long NOR ( Instruction::Format i, u32 Address );
			static long SLT ( Instruction::Format i, u32 Address );
			static long SLTU ( Instruction::Format i, u32 Address );
			static long BLTZ ( Instruction::Format i, u32 Address );
			static long BGEZ ( Instruction::Format i, u32 Address );
			static long BLTZAL ( Instruction::Format i, u32 Address );
			static long BGEZAL ( Instruction::Format i, u32 Address );

			static long MFC0 ( Instruction::Format i, u32 Address );
			static long MTC0 ( Instruction::Format i, u32 Address );
			static long RFE ( Instruction::Format i, u32 Address );
			static long MFC2 ( Instruction::Format i, u32 Address );
			static long CFC2 ( Instruction::Format i, u32 Address );
			static long MTC2 ( Instruction::Format i, u32 Address );
			static long CTC2 ( Instruction::Format i, u32 Address );
			static long COP2 ( Instruction::Format i, u32 Address );
			
			static long RTPS ( Instruction::Format i, u32 Address );
			static long NCLIP ( Instruction::Format i, u32 Address );
			static long OP ( Instruction::Format i, u32 Address );
			static long DPCS ( Instruction::Format i, u32 Address );
			static long INTPL ( Instruction::Format i, u32 Address );
			static long MVMVA ( Instruction::Format i, u32 Address );
			static long NCDS ( Instruction::Format i, u32 Address );
			static long CDP ( Instruction::Format i, u32 Address );
			static long NCDT ( Instruction::Format i, u32 Address );
			static long NCCS ( Instruction::Format i, u32 Address );
			static long CC ( Instruction::Format i, u32 Address );
			static long NCS ( Instruction::Format i, u32 Address );
			static long NCT ( Instruction::Format i, u32 Address );
			static long SQR ( Instruction::Format i, u32 Address );
			static long DCPL ( Instruction::Format i, u32 Address );
			static long DPCT ( Instruction::Format i, u32 Address );
			static long AVSZ3 ( Instruction::Format i, u32 Address );
			static long AVSZ4 ( Instruction::Format i, u32 Address );
			static long RTPT ( Instruction::Format i, u32 Address );
			static long GPF ( Instruction::Format i, u32 Address );
			static long GPL ( Instruction::Format i, u32 Address );
			static long NCCT ( Instruction::Format i, u32 Address );

	};
};

#endif
