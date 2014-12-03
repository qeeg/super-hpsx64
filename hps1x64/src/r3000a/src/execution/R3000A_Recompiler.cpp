

#include "R3000A_Recompiler.h"

using namespace R3000A;


#define _USE_LOAD_CALLBACK
#define _USE_BRANCH_CALLBACK

x64Encoder *Recompiler::e;
ICache_Device *Recompiler::ICache;
Cpu *Recompiler::r;


// constructor
Recompiler::Recompiler ( u32 BlockSize_PowerOfTwo, u32 MaxIStep, Cpu* R3000ACpu )
{
	Reset ();
	
	
	BlockSize = 1 << BlockSize_PowerOfTwo;
	MaxStep = MaxIStep;
	
	
	// create the encoder
	e = new x64Encoder ( BlockSize_PowerOfTwo, c_iNumBlocks );
	
	//ICache = IC;
	r = R3000ACpu;
}

// destructor
Recompiler::~Recompiler ()
{
	delete e;
}


void Recompiler::RecompileInstructions ( u32 BeginAddress )
{
	u32 Address, Block;
	s32 ret, Cycles;
	Instruction::Format inst;
	
	// mask address
	// don't do this
	//StartAddress &= c_iAddress_Mask;
	
	Address = BeginAddress;
	
	// get the block to encode in
	Block = ( BeginAddress >> 2 ) & c_iNumBlocks_Mask;
	
	// set block initially to cache
	DoNotCache [ Block ] = 0;
	
	// start in code block
	e->StartCodeBlock ( Block );
	
	// set the start address for code block
	StartAddress [ Block ] = BeginAddress & c_iAddress_Mask;
	
	// clear cache miss count
	CacheMissCount [ Block ] = 0;
	
	// start cycles at zero
	Cycles = 0;
	
	for ( int i = 0; i < MaxStep; i++, Address += 4 )
	{
		if ( r->ICache.isCacheHit ( Address ) )
		{
			// start encoding a MIPS instruction
			e->StartInstructionBlock ();
			
			// recompile the instruction
			inst.Value = r->ICache.Read ( Address );
			ret = Recompile ( inst, Address );
			
			if ( ret < 0 )
			{
				// there was a problem, and recompiling is done
				
				// need to undo whatever we did for this instruction
				e->UndoInstructionBlock ();
				
				// mark block as unable to recompile if there were no instructions recompiled at all
				if ( !Cycles ) DoNotCache [ Block ] = 1;
				
				// done
				break;
			}
			
			if ( !ret )
			{
				// instruction successfully encoded from MIPS into x64
				e->EndInstructionBlock ();
				
				// add number of cycles encoded
				Cycles += 1;
				
				// update address here since the currently generated instruction is cool
				Address += 4;
				
				// done
				break;
			}
			
			// instruction successfully encoded from MIPS into x64
			e->EndInstructionBlock ();
			
			// add number of cycles encoded
			Cycles += ret;
		}
		else
		{
			// data is not cached
			
			// cache miss //
			CacheMissCount [ Block ]++;
			break;
		}
	}
	
	// *** todo *** put in the end of code block (set next PC, etc) and return
	// *** todo *** will need to make sure there is at least 8 bytes remaining for the footer (actually 7 bytes, but should check for 8)
	e->MovMemImm32 ( &r->NextPC, Address );
	e->x64EncodeReturn ();
	
	// set the end address for code block (inclusive)
	LastAddress [ Block ] = ( Address - 4 ) & c_iAddress_Mask;
	
	// put in max cycles for block
	// code block is not valid if this is zero
	MaxCycles [ Block ] = Cycles;
	
	// done encoding block
	e->EndCodeBlock ();
}

void Recompiler::Invalidate ( u32 Address )
{
	s32 StartBlock, StopBlock;
	
	// mask addresses??
	Address &= c_iAddress_Mask;
	
	// Get block to stop checking address at. 4 bytes per instruction
	StopBlock = ( Address >> 2 ) & c_iNumBlocks_Mask;
	
	// get block to start checking address at
	StartBlock = StopBlock - ( MaxStep - 1 );
	
	if ( StartBlock < 0 ) StartBlock = 0;
	
	for ( u32 i = StartBlock; i <= StopBlock; i++ )
	{
		if ( Address >= StartAddress [ i ] && Address <= LastAddress [ i ] )
		{
			// invalidate any related code cache blocks found
			MaxCycles [ i ] = 0;
			DoNotCache [ i ] = 0;
			CacheMissCount [ i ] = 0;
		}
	}
	
	// don't forget to invalidate the block
	MaxCycles [ StopBlock ] = 0;
	DoNotCache [ StopBlock ] = 0;
	CacheMissCount [ StopBlock ] = 0;
}


// regular arithemetic

// *** todo *** no need to save LastModifiedRegister unless instruction is KNOWN to be in a delay slot on run
long Recompiler::ADDU ( Instruction::Format i, u32 Address )
{
	//r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u + r->GPR [ i.Rt ].u;
	//CHECK_DELAYSLOT ( i.Rd );
	
	int ret = 1;
	if ( i.Rd )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].s );
		e->AddRegMem32 ( 0, &r->GPR [ i.Rt ].u );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].s, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	
	return 1;
	
	//return -1;
}

long Recompiler::SUBU ( Instruction::Format i, u32 Address )
{
	//r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u - r->GPR [ i.Rt ].u;
	//CHECK_DELAYSLOT ( i.Rd );
	
	int ret = 1;
	if ( i.Rd )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
		e->SubRegMem32 ( 0, &r->GPR [ i.Rt ].u );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].u, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	
	return 1;
	
	//return -1;
}

long Recompiler::AND ( Instruction::Format i, u32 Address )
{
	
	int ret = 1;
	if ( i.Rd )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
		e->AndRegMem32 ( 0, &r->GPR [ i.Rt ].u );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].u, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	return 1;
	
	//return -1;
}

long Recompiler::OR ( Instruction::Format i, u32 Address )
{
	
	int ret = 1;
	if ( i.Rd )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
		e->OrRegMem32 ( 0, &r->GPR [ i.Rt ].u );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].u, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	return 1;
	
	//return -1;
}

long Recompiler::XOR ( Instruction::Format i, u32 Address )
{
	
	int ret = 1;
	if ( i.Rd )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
		e->XorRegMem32 ( 0, &r->GPR [ i.Rt ].u );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].u, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	return 1;
	
	//return -1;
}

long Recompiler::NOR ( Instruction::Format i, u32 Address )
{
	
	int ret = 1;
	if ( i.Rd )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
		e->OrRegMem32 ( 0, &r->GPR [ i.Rt ].u );
		e->NotReg32 ( 0 );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].u, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	return 1;
	
	//return -1;
}

long Recompiler::SLT ( Instruction::Format i, u32 Address )
{
	//r->GPR [ i.Rd ].s = r->GPR [ i.Rs ].s < r->GPR [ i.Rt ].s ? 1 : 0;
	//CHECK_DELAYSLOT ( i.Rd );
	
	int ret = 1;
	if ( i.Rd )
	{
		e->XorRegReg32 ( RCX, RCX );
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
		e->CmpRegMem32 ( 0, &r->GPR [ i.Rt ].u );
		e->Set_L ( RCX );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].u, RCX );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	return 1;
	
	//return -1;
}

long Recompiler::SLTU ( Instruction::Format i, u32 Address )
{
	//r->GPR [ i.Rd ].u = r->GPR [ i.Rs ].u < r->GPR [ i.Rt ].u ? 1 : 0;
	//CHECK_DELAYSLOT ( i.Rd );
	
	int ret = 1;
	if ( i.Rd )
	{
		e->XorRegReg32 ( RCX, RCX );
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
		e->CmpRegMem32 ( 0, &r->GPR [ i.Rt ].u );
		e->Set_B ( RCX );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].u, RCX );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	return 1;
	
	//return -1;
}


////////////////////////////////////////////
// I-Type Instructions (non-interrupt)



long Recompiler::ADDIU ( Instruction::Format i, u32 Address )
{
	//r->GPR [ i.Rt ].s = r->GPR [ i.Rs ].s + i.sImmediate;
	//CHECK_DELAYSLOT ( i.Rt );
	
	int ret = 1;
	if ( i.Rt )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].s );
		e->AddRegImm32 ( 0, i.sImmediate );
		e->MovRegToMem32 ( &r->GPR [ i.Rt ].s, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rt );
	}
	
	if ( !ret )
	{
		cout << "\nError encoding ADDIU instruction.\n";
		return -1;
	}
	
	return 1;
	
	//return -1;
}

long Recompiler::ANDI ( Instruction::Format i, u32 Address )
{
	//r->GPR [ i.Rt ].u = r->GPR [ i.Rs ].u & i.uImmediate;
	//CHECK_DELAYSLOT ( i.Rt );
	
	int ret = 1;
	if ( i.Rt )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].s );
		e->AndRegImm32 ( 0, i.uImmediate );
		e->MovRegToMem32 ( &r->GPR [ i.Rt ].s, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rt );
	}
	
	if ( !ret )
	{
		cout << "\nError encoding ADDIU instruction.\n";
		return -1;
	}
	
	return 1;
	
	//return -1;
}

long Recompiler::ORI ( Instruction::Format i, u32 Address )
{
	//r->GPR [ i.Rt ].u = r->GPR [ i.Rs ].u | i.uImmediate;
	//CHECK_DELAYSLOT ( i.Rt );
	
	int ret = 1;
	if ( i.Rt )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].s );
		e->OrRegImm32 ( 0, i.uImmediate );
		e->MovRegToMem32 ( &r->GPR [ i.Rt ].s, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rt );
	}
	
	if ( !ret )
	{
		cout << "\nError encoding ADDIU instruction.\n";
		return -1;
	}
	
	return 1;
	
	//return -1;
}

long Recompiler::XORI ( Instruction::Format i, u32 Address )
{
	//r->GPR [ i.Rt ].u = r->GPR [ i.Rs ].u ^ i.uImmediate;
	//CHECK_DELAYSLOT ( i.Rt );
	
	int ret = 1;
	if ( i.Rt )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].s );
		e->XorRegImm32 ( 0, i.uImmediate );
		e->MovRegToMem32 ( &r->GPR [ i.Rt ].s, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rt );
	}
	
	if ( !ret )
	{
		cout << "\nError encoding ADDIU instruction.\n";
		return -1;
	}
	
	return 1;
	
	//return -1;
}

long Recompiler::SLTI ( Instruction::Format i, u32 Address )
{
	//r->GPR [ i.Rt ].s = r->GPR [ i.Rs ].s < i.sImmediate ? 1 : 0;
	//CHECK_DELAYSLOT ( i.Rt );
	int ret = 1;
	if ( i.Rt )
	{
		e->XorRegReg32 ( RCX, RCX );
		//e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
		e->CmpMemImm32 ( &r->GPR [ i.Rs ].s, i.sImmediate );
		e->Set_L ( RCX );
		e->MovRegToMem32 ( &r->GPR [ i.Rt ].u, RCX );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rt );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	return 1;
	//return -1;
}

long Recompiler::SLTIU ( Instruction::Format i, u32 Address )
{
	//r->GPR [ i.Rt ].u = r->GPR [ i.Rs ].u < ((u32) ((s32) i.sImmediate)) ? 1 : 0;
	//CHECK_DELAYSLOT ( i.Rt );
	int ret = 1;
	if ( i.Rt )
	{
		e->XorRegReg32 ( RCX, RCX );
		//e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
		e->CmpMemImm32 ( &r->GPR [ i.Rs ].s, i.sImmediate );
		e->Set_B ( RCX );
		e->MovRegToMem32 ( &r->GPR [ i.Rt ].u, RCX );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rt );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	return 1;
	return -1;
}

long Recompiler::LUI ( Instruction::Format i, u32 Address )
{
	//r->GPR [ i.Rt ].u = ( i.uImmediate << 16 );
	//CHECK_DELAYSLOT ( i.Rt );
	
	int ret = 1;
	if ( i.Rt )
	{
		e->MovMemImm32 ( &r->GPR [ i.Rt ].u, ( i.uImmediate << 16 ) );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rt );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding LUI instruction.\n";
		return -1;
	}
	
	return 1;
	
	//return -1;
}







//////////////////////////////////////////////////////////
// Shift instructions



long Recompiler::SLL ( Instruction::Format i, u32 Address )
{
	//r->GPR [ i.Rd ].u = ( r->GPR [ i.Rt ].u << i.Shift );
	//CHECK_DELAYSLOT ( i.Rd );
	int ret = 1;
	if ( i.Rd )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rt ].u );
		//e->MovRegImm32 ( RCX, (u32) i.Shift );
		e->ShlRegImm32 ( 0, (u32) i.Shift );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].s, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	
	return 1;
	//return -1;
}

long Recompiler::SRL ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	if ( i.Rd )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rt ].u );
		//e->MovRegImm32 ( RCX, (u32) i.Shift );
		e->ShrRegImm32 ( 0, (u32) i.Shift );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].s, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	
	return 1;
	//return -1;
}

long Recompiler::SRA ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	if ( i.Rd )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rt ].u );
		//e->MovRegImm32 ( RCX, (u32) i.Shift );
		e->SarRegImm32 ( 0, (u32) i.Shift );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].s, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	
	return 1;
	//return -1;
}

long Recompiler::SLLV ( Instruction::Format i, u32 Address )
{
	//r->GPR [ i.Rd ].u = ( r->GPR [ i.Rt ].u << ( r->GPR [ i.Rs ].u & 0x1f ) );
	//CHECK_DELAYSLOT ( i.Rd );
	int ret = 1;
	if ( i.Rd )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rt ].u );
		e->MovRegFromMem32 ( RCX, &r->GPR [ i.Rs ].u );
		e->ShlRegReg32 ( 0 );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].s, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	
	return 1;
	//return -1;
}

long Recompiler::SRLV ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	if ( i.Rd )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rt ].u );
		e->MovRegFromMem32 ( RCX, &r->GPR [ i.Rs ].u );
		e->ShrRegReg32 ( 0 );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].s, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	
	return 1;
	//return -1;
}

long Recompiler::SRAV ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	if ( i.Rd )
	{
		e->MovRegFromMem32 ( 0, &r->GPR [ i.Rt ].u );
		e->MovRegFromMem32 ( RCX, &r->GPR [ i.Rs ].u );
		e->SarRegReg32 ( 0 );
		e->MovRegToMem32 ( &r->GPR [ i.Rd ].s, 0 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding ADDU instruction.\n";
		return -1;
	}
	
	return 1;
	//return -1;
}



////////////////////////////////////////////
// Jump/Branch Instructions



long Recompiler::J ( Instruction::Format i, u32 Address )
{
	//r->DelaySlot0.Instruction = i;
	//r->DelaySlot0.cb = r->_cb_Jump;
	//r->Status.DelaySlot_Valid |= 0x1;
	int ret = 1;
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	ret = e->OrMemImm64 ( &r->Status.Value, 1 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding J instruction.\n";
		return -1;
	}
	return 0;
}

long Recompiler::JR ( Instruction::Format i, u32 Address )
{
	//r->DelaySlot0.Instruction = i;
	//r->DelaySlot0.cb = r->_cb_JumpRegister;
	//r->DelaySlot0.Data = r->GPR [ i.Rs ].u & ~3;
	//r->Status.DelaySlot_Valid |= 0x1;
	int ret = 1;
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
	e->MovRegToMem32 ( &r->DelaySlot0.Data, 0 );
	ret = e->OrMemImm64 ( &r->Status.Value, 1 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding JR instruction.\n";
		return -1;
	}
	return 0;
}

long Recompiler::JAL ( Instruction::Format i, u32 Address )
{
	//r->DelaySlot0.Instruction = i;
	//r->DelaySlot0.cb = r->_cb_Jump;
	//r->Status.DelaySlot_Valid |= 0x1;
	//r->GPR [ 31 ].u = r->PC + 8;
	//CHECK_DELAYSLOT ( 31 );
	int ret = 1;
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	e->OrMemImm64 ( &r->Status.Value, 1 );
	e->MovMemImm32 ( &r->GPR [ 31 ].u, Address + 8 );
	ret = e->MovMemImm32 ( &r->LastModifiedRegister, 31 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding JAL instruction.\n";
		return -1;
	}
	return 0;
	//return -1;
}

long Recompiler::JALR ( Instruction::Format i, u32 Address )
{
	//r->DelaySlot0.Instruction = i;
	//r->DelaySlot0.Data = r->GPR [ i.Rs ].u & ~3;
	//r->DelaySlot0.cb = r->_cb_JumpRegister;
	//r->Status.DelaySlot_Valid |= 0x1;
	//r->GPR [ i.Rd ].u = r->PC + 8;
	//CHECK_DELAYSLOT ( i.Rd );
	int ret = 1;
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
	e->MovRegToMem32 ( &r->DelaySlot0.Data, 0 );
	ret = e->OrMemImm64 ( &r->Status.Value, 1 );
	if ( i.Rd )
	{
		e->MovMemImm32 ( &r->GPR [ i.Rd ].u, Address + 8 );
		ret = e->MovMemImm32 ( &r->LastModifiedRegister, i.Rd );
	}
	
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding JALR instruction.\n";
		return -1;
	}
	return 0;
	//return -1;
}

long Recompiler::BEQ ( Instruction::Format i, u32 Address )
{
	//if ( r->GPR [ i.Rs ].u == r->GPR [ i.Rt ].u )
	//{
	//	r->DelaySlot0.Instruction = i;
	//	r->DelaySlot0.cb = r->_cb_Branch;
	//	r->Status.DelaySlot_Valid |= 0x1;
	//}
	int ret = 1;
	e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
	e->CmpRegMem32 ( 0, &r->GPR [ i.Rt ].u );
	e->Jmp8_NE ( 0, 0 );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	ret = e->OrMemImm64 ( &r->Status.Value, 1 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding BEQ instruction.\n";
		return -1;
	}
	e->SetJmpTarget8 ( 0 );
	return 0;
	//return -1;
}

long Recompiler::BNE ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
	e->CmpRegMem32 ( 0, &r->GPR [ i.Rt ].u );
	e->Jmp8_E ( 0, 0 );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	ret = e->OrMemImm64 ( &r->Status.Value, 1 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding BEQ instruction.\n";
		return -1;
	}
	e->SetJmpTarget8 ( 0 );
	return 0;
	//return -1;
}

long Recompiler::BLEZ ( Instruction::Format i, u32 Address )
{
	//if ( r->GPR [ i.Rs ].s <= 0 )
	//{
	//	// next instruction is in the branch delay slot
	//	r->DelaySlot0.Instruction = i;
	//	r->DelaySlot0.cb = r->_cb_Branch;
	//	r->Status.DelaySlot_Valid |= 0x1;
	//}
	int ret = 1;
	//e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
	e->CmpMemImm32 ( &r->GPR [ i.Rs ].u, 0 );
	e->Jmp8_G ( 0, 0 );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	ret = e->OrMemImm64 ( &r->Status.Value, 1 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding BEQ instruction.\n";
		return -1;
	}
	e->SetJmpTarget8 ( 0 );
	return 0;
	//return -1;
}

long Recompiler::BGTZ ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	//e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
	e->CmpMemImm32 ( &r->GPR [ i.Rs ].u, 0 );
	e->Jmp8_LE ( 0, 0 );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	ret = e->OrMemImm64 ( &r->Status.Value, 1 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding BEQ instruction.\n";
		return -1;
	}
	e->SetJmpTarget8 ( 0 );
	return 0;
	//return -1;
}

long Recompiler::BLTZ ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	//e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
	e->CmpMemImm32 ( &r->GPR [ i.Rs ].u, 0 );
	e->Jmp8_GE ( 0, 0 );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	ret = e->OrMemImm64 ( &r->Status.Value, 1 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding BEQ instruction.\n";
		return -1;
	}
	e->SetJmpTarget8 ( 0 );
	return 0;
	//return -1;
}

long Recompiler::BGEZ ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	//e->MovRegFromMem32 ( 0, &r->GPR [ i.Rs ].u );
	e->CmpMemImm32 ( &r->GPR [ i.Rs ].u, 0 );
	e->Jmp8_L ( 0, 0 );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	ret = e->OrMemImm64 ( &r->Status.Value, 1 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding BEQ instruction.\n";
		return -1;
	}
	e->SetJmpTarget8 ( 0 );
	return 0;
	//return -1;
}

long Recompiler::BLTZAL ( Instruction::Format i, u32 Address )
{
	//if ( r->GPR [ i.Rs ].s < 0 )
	//{
	//	r->DelaySlot0.Instruction = i;
	//	r->DelaySlot0.cb = r->_cb_Branch;
	//	r->Status.DelaySlot_Valid |= 0x1;
	//}
	//r->GPR [ 31 ].u = r->PC + 8;
	//CHECK_DELAYSLOT ( 31 );
	int ret = 1;
	e->CmpMemImm32 ( &r->GPR [ i.Rs ].u, 0 );
	e->Jmp8_GE ( 0, 0 );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	e->OrMemImm64 ( &r->Status.Value, 1 );
	e->SetJmpTarget8 ( 0 );
	e->MovMemImm32 ( &r->GPR [ 31 ].u, Address + 8 );
	ret = e->MovMemImm32 ( &r->LastModifiedRegister, 31 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding BEQ instruction.\n";
		return -1;
	}
	return 0;
	//return -1;
}

long Recompiler::BGEZAL ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	e->CmpMemImm32 ( &r->GPR [ i.Rs ].u, 0 );
	e->Jmp8_L ( 0, 0 );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	e->OrMemImm64 ( &r->Status.Value, 1 );
	e->SetJmpTarget8 ( 0 );
	e->MovMemImm32 ( &r->GPR [ 31 ].u, Address + 8 );
	ret = e->MovMemImm32 ( &r->LastModifiedRegister, 31 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding BEQ instruction.\n";
		return -1;
	}
	return 0;
	//return -1;
}


/////////////////////////////////////////////////////////////
// Multiply/Divide Instructions

// *** todo *** don't forget to fix CPU wait function to include storing from store buffer
long Recompiler::MULT ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::MULTU ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::DIV ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::DIVU ( Instruction::Format i, u32 Address )
{
	return -1;
}



long Recompiler::MFHI ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::MFLO ( Instruction::Format i, u32 Address )
{
	return -1;
}




long Recompiler::MTHI ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::MTLO ( Instruction::Format i, u32 Address )
{
	return -1;
}




long Recompiler::RFE ( Instruction::Format i, u32 Address )
{
	return -1;
}




////////////////////////////////////////////////////////
// Instructions that can cause Synchronous Interrupts //
////////////////////////////////////////////////////////


long Recompiler::ADD ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::ADDI ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::SUB ( Instruction::Format i, u32 Address )
{
	return -1;
}




long Recompiler::SYSCALL ( Instruction::Format i, u32 Address )
{
	e->Jmp ( (char*) &Instruction::Execute::SYSCALL );
	return 0;
	//return -1;
}

long Recompiler::BREAK ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::Invalid ( Instruction::Format i, u32 Address )
{
	return -1;
}





long Recompiler::MFC0 ( Instruction::Format i, u32 Address )
{
	return -1;
}




long Recompiler::MTC0 ( Instruction::Format i, u32 Address )
{
	return -1;
}




long Recompiler::MFC2 ( Instruction::Format i, u32 Address )
{
	return -1;
}




long Recompiler::MTC2 ( Instruction::Format i, u32 Address )
{
	return -1;
}




long Recompiler::CFC2 ( Instruction::Format i, u32 Address )
{
	return -1;
}



long Recompiler::CTC2 ( Instruction::Format i, u32 Address )
{
	return -1;
}




// Load/Store - will need to use address translation to get physical addresses when needed

//////////////////////////////////////////////////////////////////////////
// store instructions

// store instructions
long Recompiler::SB ( Instruction::Format i, u32 Address )
{
	return -1;
}





long Recompiler::SH ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::SW ( Instruction::Format i, u32 Address )
{
	return -1;
}


long Recompiler::SWC2 ( Instruction::Format i, u32 Address )
{
	return -1;
}



long Recompiler::SWL ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::SWR ( Instruction::Format i, u32 Address )
{
	return -1;
}



/////////////////////////////////////////////////
// load instructions

// load instructions with delay slot
// *** todo *** it is also possible to this and just process load after load delay slot has executed - would still need previous load address before delay slot
// *** todo *** could also skip delay slot zero and put straight into delay slot 1 after next instruction, or just process load delay slot after next instruction
long Recompiler::LB ( Instruction::Format i, u32 Address )
{
	//LoadAddress = r->GPR [ i.Base ].s + i.sOffset;
	//r->DelaySlot0.Instruction = i;
	//r->DelaySlot0.Data = LoadAddress;
	//r->DelaySlot0.cb = LB_DelaySlot_Callback_Bus;
	//r->Status.DelaySlot_Valid |= 0x1;
	//r->LastModifiedRegister = 255;
	int ret = 1;
	e->MovRegImm32 ( 0, i.sOffset );
	e->AddRegMem32 ( 0, &r->GPR [ i.Base ].u );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	e->MovRegToMem32 ( &r->DelaySlot0.Data, 0 );
	e->OrMemImm64 ( &r->Status.Value, 1 );
	ret = e->MovMemImm32 ( &r->LastModifiedRegister, 255 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding LB instruction.\n";
		return -1;
	}
	return 0;
	//return -1;
}






long Recompiler::LH ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	e->MovRegImm32 ( 0, i.sOffset );
	e->AddRegMem32 ( 0, &r->GPR [ i.Base ].u );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	e->MovRegToMem32 ( &r->DelaySlot0.Data, 0 );
	e->OrMemImm64 ( &r->Status.Value, 1 );
	ret = e->MovMemImm32 ( &r->LastModifiedRegister, 255 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding LB instruction.\n";
		return -1;
	}
	return 0;
	//return -1;
}








long Recompiler::LW ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	e->MovRegImm32 ( 0, i.sOffset );
	e->AddRegMem32 ( 0, &r->GPR [ i.Base ].u );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	e->MovRegToMem32 ( &r->DelaySlot0.Data, 0 );
	e->OrMemImm64 ( &r->Status.Value, 1 );
	ret = e->MovMemImm32 ( &r->LastModifiedRegister, 255 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding LB instruction.\n";
		return -1;
	}
	return 0;
	//return -1;
}

long Recompiler::LBU ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	e->MovRegImm32 ( 0, i.sOffset );
	e->AddRegMem32 ( 0, &r->GPR [ i.Base ].u );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	e->MovRegToMem32 ( &r->DelaySlot0.Data, 0 );
	e->OrMemImm64 ( &r->Status.Value, 1 );
	ret = e->MovMemImm32 ( &r->LastModifiedRegister, 255 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding LB instruction.\n";
		return -1;
	}
	return 0;
	//return -1;
}

long Recompiler::LHU ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	e->MovRegImm32 ( 0, i.sOffset );
	e->AddRegMem32 ( 0, &r->GPR [ i.Base ].u );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	e->MovRegToMem32 ( &r->DelaySlot0.Data, 0 );
	e->OrMemImm64 ( &r->Status.Value, 1 );
	ret = e->MovMemImm32 ( &r->LastModifiedRegister, 255 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding LB instruction.\n";
		return -1;
	}
	return 0;
	//return -1;
}

long Recompiler::LWC2 ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	e->MovRegImm32 ( 0, i.sOffset );
	e->AddRegMem32 ( 0, &r->GPR [ i.Base ].u );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	e->MovRegToMem32 ( &r->DelaySlot0.Data, 0 );
	e->OrMemImm64 ( &r->Status.Value, 1 );
	ret = e->MovMemImm32 ( &r->LastModifiedRegister, 255 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding LB instruction.\n";
		return -1;
	}
	return 0;
	//return -1;
}




// load instructions without load-delay slot
long Recompiler::LWL ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	e->MovRegImm32 ( 0, i.sOffset );
	e->AddRegMem32 ( 0, &r->GPR [ i.Base ].u );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	e->MovRegToMem32 ( &r->DelaySlot0.Data, 0 );
	e->OrMemImm64 ( &r->Status.Value, 1 );
	ret = e->MovMemImm32 ( &r->LastModifiedRegister, 255 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding LB instruction.\n";
		return -1;
	}
	return 0;
	//return -1;
}

long Recompiler::LWR ( Instruction::Format i, u32 Address )
{
	int ret = 1;
	e->MovRegImm32 ( 0, i.sOffset );
	e->AddRegMem32 ( 0, &r->GPR [ i.Base ].u );
	e->MovMemImm32 ( &r->DelaySlot0.Instruction.Value, i.Value );
	e->MovRegToMem32 ( &r->DelaySlot0.Data, 0 );
	e->OrMemImm64 ( &r->Status.Value, 1 );
	ret = e->MovMemImm32 ( &r->LastModifiedRegister, 255 );
	if ( !ret )
	{
		cout << "\nx64 Recompiler: Error encoding LB instruction.\n";
		return -1;
	}
	return 0;
	//return -1;
}




















///////////////////////////
// GTE instructions

long Recompiler::COP2 ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::RTPS ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::NCLIP ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::OP ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::DPCS ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::INTPL ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::MVMVA ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::NCDS ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::CDP ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::NCDT ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::NCCS ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::CC ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::NCS ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::NCT ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::SQR ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::DCPL ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::DPCT ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::AVSZ3 ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::AVSZ4 ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::RTPT ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::GPF ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::GPL ( Instruction::Format i, u32 Address )
{
	return -1;
}

long Recompiler::NCCT ( Instruction::Format i, u32 Address )
{
	return -1;
}



static const Recompiler::Function Recompiler::FunctionList []
{
	// instructions on both R3000A and R5900
	Recompiler::Invalid,
	Recompiler::J, Recompiler::JAL, Recompiler::JR, Recompiler::JALR, Recompiler::BEQ, Recompiler::BNE, Recompiler::BGTZ, Recompiler::BGEZ,
	Recompiler::BLTZ, Recompiler::BLEZ, Recompiler::BGEZAL, Recompiler::BLTZAL, Recompiler::ADD, Recompiler::ADDI, Recompiler::ADDU, Recompiler::ADDIU,
	Recompiler::SUB, Recompiler::SUBU, Recompiler::MULT, Recompiler::MULTU, Recompiler::DIV, Recompiler::DIVU, Recompiler::AND, Recompiler::ANDI,
	Recompiler::OR, Recompiler::ORI, Recompiler::XOR, Recompiler::XORI, Recompiler::NOR, Recompiler::LUI, Recompiler::SLL, Recompiler::SRL,
	Recompiler::SRA, Recompiler::SLLV, Recompiler::SRLV, Recompiler::SRAV, Recompiler::SLT, Recompiler::SLTI, Recompiler::SLTU, Recompiler::SLTIU,
	Recompiler::LB, Recompiler::LBU, Recompiler::LH, Recompiler::LHU, Recompiler::LW, Recompiler::LWL, Recompiler::LWR, Recompiler::SB,
	Recompiler::SH, Recompiler::SW, Recompiler::SWL, Recompiler::SWR, Recompiler::MFHI, Recompiler::MTHI, Recompiler::MFLO, Recompiler::MTLO,
	Recompiler::MFC0, Recompiler::MTC0, Recompiler::CFC2, Recompiler::CTC2, Recompiler::SYSCALL, Recompiler::BREAK,
	
	// instructions on R3000A ONLY
	Recompiler::MFC2, Recompiler::MTC2, Recompiler::LWC2, Recompiler::SWC2, Recompiler::RFE,
	Recompiler::RTPS, Recompiler::RTPT, Recompiler::CC, Recompiler::CDP, Recompiler::DCPL, Recompiler::DPCS, Recompiler::DPCT, Recompiler::NCS,
	Recompiler::NCT, Recompiler::NCDS, Recompiler::NCDT, Recompiler::NCCS, Recompiler::NCCT, Recompiler::GPF, Recompiler::GPL, Recompiler::AVSZ3,
	Recompiler::AVSZ4, Recompiler::SQR, Recompiler::OP, Recompiler::NCLIP, Recompiler::INTPL, Recompiler::MVMVA
};
