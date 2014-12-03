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



#ifndef _X64ENCODER_H_

#define _X64ENCODER_H_


#include "x64Instructions.h"



#define INVALIDCODEBLOCK		0xffffffff

// some x64 instructions have more than 1 opcode
#define MAKE2OPCODE(opcode1,opcode2)			( ( ( opcode2 ) << 8 ) | ( opcode1 ) )
#define MAKE3OPCODE(opcode1,opcode2,opcode3)	( ( ( opcode3 ) << 16 ) | ( ( opcode2 ) << 8 ) | ( opcode1 ) )
#define GETOPCODE1(multiopcode)					( ( multiopcode ) & 0xff )
#define GETOPCODE2(multiopcode)					( ( ( multiopcode ) >> 8 ) & 0xff )
#define GETOPCODE3(multiopcode)					( ( ( multiopcode ) >> 16 ) & 0xff )


// this not only encodes from a source cpu to a target cpu but also holds the code and runs it
// lNumberOfCodeBlocks must be a power of 2
// the total amount allocated to the x64 code area is lCodeBlockSize*lNumberOfCodeBlocks*2
// lCodeBlockSize is in bytes

class x64Encoder
{

public:

	// need to know what code is where
	// this contains the full source address (no, I'll use address/4 or address/8) and the index of the x64 block in the array of x64 code
	// you look up by using the lower bits of the source address
	long* x64CodeHashTable;
	
	// we also need to know the source cpu address that points to next instruction after block - not next instruction to execute, just the next one
	long* x64CodeSourceNextAddress;

	// this is going to point to the current code area
	char* x64CodeArea;
	
	// this is going to be where all the live cached x64 code is at - no need for this after testing
	char* LiveCodeArea;
	
	// I'll need a work area before copying all dynamic code into actual code block
	char* AlternateStream;

	long lCodeBlockSize_PowerOfTwo, lCodeBlockSize, lCodeBlockSize_Mask;
	long lNumberOfCodeBlocks;
	
	// I need the index for the current code block
	long x64CurrentCodeBlockIndex;
	
	// the total amount of space allocated for code area
	long x64TotalSpaceAllocated;

	// need to know the address where to start putting the next x64 instruction
	long x64NextOffset;
	
	// need to know the address of the start of the current x64 instruction being encoded
	long x64CurrentStartOffset;
	
	// need to know offset first x64 byte of current source cpu instruction being encoded
	long x64CurrentSourceCpuInstructionStartOffset;
	
	// need to know the Source Cpu Address that is currently being worked on
	long x64CurrentSourceAddress;
	
	// need to keep track of the source cpu instruction size
	long x64SourceCpuInstructionSize;

	// need to know if we are currently encoding or if we're ready to execute
	bool isEncoding;
	
	// we also ned to know that it was set up successfully
	bool isReadyForUse;
	
	// the size of code in bytes in alternate stream
	long lAlternateStreamSize;

	// no need for this stuff after testing - modern cpus can handle this stuff without any tricks needed
	// we can get more control over pre-loading of data by separating pre-allocation and instructions into different streams
	// I don't know the effect on performance, so I'll add in some way to control operation of this
//	long PreAllocationBitmap;	// just set the bits for registers, then if a bit is already set copy the streams (PreAlloc then Inst), reset, repeat
//	long PreAllocationStreamSize;
//	long InstructionStreamSize;
//	char* PreAllocationStream;
//	char* InstructionStream;
	
	// constructor
	// CountOfCodeBlocks must be a power of 2, OR ELSE!!!!
	x64Encoder ( long SizePerCodeBlock_PowerOfTwo, long CountOfCodeBlocks );
	
	// destructor
	~x64Encoder ( void );
	
	// flush current code block
	bool FlushCurrentCodeBlock ( void );
	
	// flush dynamic code block - must do this once after creation before you can use it
	bool FlushCodeBlock ( long IndexOfCodeBlock );
	
	// flush instruction cache line for current process - have to use this with dynamic code or else
	bool FlushICache ( long long baseAddress, long NumberOfBytes );
	
	// get/set current instruction offset
	long GetCurrentInstructionOffset ( void );
	void SetCurrentInstructionOffset ( long offset );

	// get the size of a code block for encoder - should help with determining if there is more space available
	long GetCodeBlockSize ( void );
	
	// get the size of the current dynamic code being worked out - should help with determining if there is more space available
	// this will only work when currently working in the alternate stream
	long GetAlternateStreamCurrentSize ( void );
	
	// start writing code to alternate stream
	void SwitchToAlternateStream ( void );
	
	// start writing code to live code area
	void SwitchToLiveStream ( void );
	
	// copy dynamic code from alternate stream to current position in live stream
	void CopyToLiveStream ( void );

	// call this before beginning an x64 code block
	bool StartCodeBlock ( long lCodeBlockIndex );

	// call this when you are done with the code block
	bool EndCodeBlock ( void );

	// call this before starting to encode a single instruction from source cpu
	bool StartInstructionBlock ( void );

	// call this when done encoding a single instructin from source cpu
	bool EndInstructionBlock ( void );

	// takes you to the start of the current source cpu instruction you were encoding - cuz you may need to backtrack
	bool UndoInstructionBlock ( void );
	
	// this returns the amount of space remaining in the current code block
	long x64CurrentCodeBlockSpaceRemaining ( void );
	
	// invalidate a code block that has been overwritten in source cpu memory or is otherwise no longer valid
	bool x64InvalidateCodeBlock ( long lSourceAddress );

	// check if code is already encoded and ready to run
	bool x64IsEncodedAndReady ( long lSourceAddress );

	// executes a code block and returns address of the next instruction
	long long ExecuteCodeBlock ( long lSourceAddress );



	// **** Functions For Encoding of x64 Instructions **** //
	
	// encode an instruction with no register arguments
	bool x64Encode16 ( long x64InstOpcode );
	bool x64Encode32 ( long x64InstOpcode );
	bool x64Encode64 ( long x64InstOpcode );
	
	// encode a move immediate instruction using fewest bytes possible
	bool x64EncodeMovImm64 ( long x64DestRegIndex, long long Immediate64 );
	bool x64EncodeMovImm32 ( long x64DestRegIndex, long Immediate32 );
	bool x64EncodeMovImm16 ( long x64DestRegIndex, short Immediate16 );
	bool x64EncodeMovImm8 ( long x64DestRegIndex, long Immediate8 );

	// encode a single register x64 instruction into code block
	bool x64EncodeReg32 ( long x64InstOpcode, long ModRMOpcode, long x64Reg );
	bool x64EncodeReg64 ( long x64InstOpcode, long ModRMOpcode, long x64Reg );
	
	// encode a register-register x64 instruction into code block
	bool x64EncodeRegReg16 ( long x64InstOpcode, long x64DestReg_Reg_Opcode, long x64SourceReg_RM_Base );
	bool x64EncodeRegReg32 ( long x64InstOpcode, long x64DestReg_Reg_Opcode, long x64SourceReg_RM_Base );
	bool x64EncodeRegReg64 ( long x64InstOpcode, long x64DestReg_Reg_Opcode, long x64SourceReg_RM_Base );

	// encode a x64 Imm8 instruction into code block
	bool x64EncodeReg16Imm8 ( long x64InstOpcode, long ModRMOpcode, long x64DestReg_RM_Base, char cImmediate8 );
	bool x64EncodeReg32Imm8 ( long x64InstOpcode, long ModRMOpcode, long x64DestReg_RM_Base, char cImmediate8 );
	bool x64EncodeReg64Imm8 ( long x64InstOpcode, long ModRMOpcode, long x64DestReg_RM_Base, char cImmediate8 );
	
	// encode x64 Imm8 instruction but with memory
	bool x64EncodeMem16Imm8 ( long x64InstOpcode, long ModRMOpcode, long BaseAddressReg, long IndexReg, long Scale, long Offset, char cImmediate8 );
	bool x64EncodeMem32Imm8 ( long x64InstOpcode, long ModRMOpcode, long BaseAddressReg, long IndexReg, long Scale, long Offset, char cImmediate8 );
	bool x64EncodeMem64Imm8 ( long x64InstOpcode, long ModRMOpcode, long BaseAddressReg, long IndexReg, long Scale, long Offset, char cImmediate8 );

	// encode 16-bit immediate x64 instruction
	bool x64EncodeReg16Imm16 ( long x64InstOpcode, long ModRMOpcode, long x64DestReg, short cImmediate16 );

	// encode a reg-immediate 32 bit x64 instruction into code block
	bool x64EncodeReg32Imm32 ( long x64InstOpcode, long ModRMOpcode, long x64DestReg, long cImmediate32 );

	// encode a reg-immediate 64 bit x64 instruction into code block
	bool x64EncodeReg64Imm32 ( long x64InstOpcode, long ModRMOpcode, long x64DestReg, long cImmediate32 );
	
	
	// *** These functions work with memory accesses ***
	// ** NOTE ** Do not use sp/esp/rsp or bp/ebp/rbp with these functions until I read the x64 docs more thoroughly!!!

	// encode an x64 instruction that addresses memory
	bool x64EncodeRegMem16 ( long x64InstOpcode, long x64DestReg, long BaseAddressReg, long IndexReg, long Scale, long Offset );
	bool x64EncodeRegMem32 ( long x64InstOpcode, long x64DestReg, long BaseAddressReg, long IndexReg, long Scale, long Offset );
	bool x64EncodeRegMem64 ( long x64InstOpcode, long x64DestReg, long BaseAddressReg, long IndexReg, long Scale, long Offset );

	bool x64EncodeMemImm16 ( long x64InstOpcode, long Mod, short Imm16, long BaseAddressReg, long IndexReg, long Scale, long Offset );
	bool x64EncodeMemImm32 ( long x64InstOpcode, long Mod, long Imm32, long BaseAddressReg, long IndexReg, long Scale, long Offset );
	bool x64EncodeMemImm64 ( long x64InstOpcode, long Mod, long Imm32, long BaseAddressReg, long IndexReg, long Scale, long Offset );
	
	// these are for encoding just immediate values
	bool x64EncodeImmediate8 ( char Imm8 );
	bool x64EncodeImmediate16 ( short Imm16 );
	bool x64EncodeImmediate32 ( long Imm32 );
	bool x64EncodeImmediate64 ( long long Imm64 );

	// encode a return instruction
	bool x64EncodeReturn ( void );
	
	
	// *** testing *** encode RIP-offset addressing
	bool x64EncodeRipOffset16 ( long x64InstOpcode, long x64DestReg, char* DataAddress );
	bool x64EncodeRipOffset32 ( long x64InstOpcode, long x64DestReg, char* DataAddress );
	bool x64EncodeRipOffset64 ( long x64InstOpcode, long x64DestReg, char* DataAddress );
	bool x64EncodeRipOffsetImm8 ( long x64InstOpcode, long x64DestReg, char* DataAddress, char Imm8 );
	bool x64EncodeRipOffsetImm16 ( long x64InstOpcode, long x64DestReg, char* DataAddress, short Imm16 );
	bool x64EncodeRipOffsetImm32 ( long x64InstOpcode, long x64DestReg, char* DataAddress, long Imm32 );
	bool x64EncodeRipOffsetImm64 ( long x64InstOpcode, long x64DestReg, char* DataAddress, long Imm32 );
	
	bool x64EncodeRipOffset16Imm8 ( long x64InstOpcode, long x64DestReg, char* DataAddress, char Imm8 );
	bool x64EncodeRipOffset32Imm8 ( long x64InstOpcode, long x64DestReg, char* DataAddress, char Imm8 );
	bool x64EncodeRipOffset64Imm8 ( long x64InstOpcode, long x64DestReg, char* DataAddress, char Imm8 );
	
	
	
	// ** Encoding vector/sse instructions ** //

	// encode an avx instruction (64-bit version) with an immediate byte on the end
	bool x64EncodeRegVImm8 ( long L, long w, long pp, long mmmmm, long avxInstOpcode, long REG_R, long vvvv, long RM_B, char cImmediate8 );
	bool x64EncodeRegMemVImm8 ( long L, long w, long pp, long mmmmm, long avxInstOpcode, long REG_R_Dest, long vvvv, long x64RM_B_Base, long x64IndexReg, long Scale, long Offset, char Imm8 );
	
	// encode avx instruction for a single 32-bit load store to/from memory
	bool x64EncodeRegMem32S ( long pp, long mmmmm, long avxInstOpcode, long avxDestSrcReg, long avxBaseReg, long avxIndexReg, long Scale, long Offset );
	bool x64EncodeRegMemV ( long L, long w, long pp, long mmmmm, long avxInstOpcode, long REG_R_Dest, long vvvv, long x64RM_B_Base, long x64IndexReg, long Scale, long Offset );
	bool x64EncodeRegMem256 ( long pp, long mmmmm, long avxInstOpcode, long avxDestSrcReg, long avxBaseReg, long avxIndexReg, long Scale, long Offset );

	// encode an avx instruction (64-bit version) that is just register-register
	bool x64EncodeRegRegV ( long L, long w, long pp, long mmmmm, long avxInstOpcode, long REG_R, long vvvv, long RM_B );

	
	// **** x64 Instuctions **** //
	
	// ** General Purpose Register Instructions ** //
	
	
	// * jump/branch instructions * //
	
//	long JmpOffset [ 256 ];
	long BranchOffset [ 256 ];
/*
	long JmpOffsetEndCount;
	long JmpOffset_End [ 512 ];

	long BranchOffsetEndCount;
	long BranchOffset_End [ 512 ];
*/
	// these are used to make a short term hop while encoding x64 instructions
	bool Jmp ( long Offset, unsigned char Label );
	bool Jmp_NE ( long Offset, unsigned char Label );
	bool Jmp_E ( long Offset, unsigned char Label );
	bool Jmp_L ( long Offset, unsigned char Label );
	bool Jmp_LE ( long Offset, unsigned char Label );
	bool Jmp_G ( long Offset, unsigned char Label );
	bool Jmp_GE ( long Offset, unsigned char Label );
	
	// jump short if equal
	bool Jmp8_E ( char Offset, unsigned char Label );
	
	// jump short if not equal
	bool Jmp8_NE ( char Offset, unsigned char Label );
	
	// jump short if unsigned above (carry flag=0 and zero flag=0)
	bool Jmp8_A ( char Offset, unsigned char Label );

	// jump short if unsigned above or equal (carry flag=0)
	bool Jmp8_AE ( char Offset, unsigned char Label );
	
	// jump short if unsigned below (carry flag=1)
	bool Jmp8_B ( char Offset, unsigned char Label );

	// jump short if unsigned below or equal (carry flag=1 or zero flag=1)
	bool Jmp8_BE ( char Offset, unsigned char Label );

	bool Jmp8_G ( char Offset, unsigned char Label );
	bool Jmp8_GE ( char Offset, unsigned char Label );
	bool Jmp8_L ( char Offset, unsigned char Label );
	bool Jmp8_LE ( char Offset, unsigned char Label );
	
/*
	// these are used to jump to the end of code block where cleanup takes place
	bool Jmp_End ( long Offset );
	bool Jmp_NE_End ( long Offset );
	bool Jmp_E_End ( long Offset );
	bool Jmp_L_End ( long Offset );
	bool Jmp_LE_End ( long Offset );
	bool Jmp_G_End ( long Offset );
	bool Jmp_GE_End ( long Offset );
*/
	// these are used to set the target address of jump once you find out what it is
	bool SetJmpTarget ( unsigned char Label );
	
	// set jump target address for a short jump
	bool SetJmpTarget8 ( unsigned char Label );
	
	
	// absolute jump with 32-bit rip-offset
	bool Jmp ( char* Target );
	
	// absolute call with 32-bit rip-offset
	bool Call ( char* Target );

//	bool SetJmpTarget_End ( void );

	// mov
	bool MovRegReg16 ( long DestReg, long SrcReg );
	bool MovRegReg32 ( long DestReg, long SrcReg );
	bool MovRegReg64 ( long DestReg, long SrcReg );
	bool MovRegImm16 ( long DestReg, short Imm16 );
	bool MovRegImm32 ( long DestReg, long Imm32 );
	bool MovRegImm64 ( long DestReg, long long Imm64 );
	bool MovReg64Imm32 ( long DestReg, long Imm32 );
	bool MovRegToMem8 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool MovRegFromMem8 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool MovRegToMem16 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool MovRegFromMem16 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool MovRegToMem32 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool MovRegFromMem32 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool MovRegToMem64 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool MovRegFromMem64 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	
	bool MovMemImm16 ( short Imm16, long AddressReg, long IndexReg, long Scale, long Offset );
	bool MovMemImm32 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );
	bool MovMemImm64 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );

	
	// *** testing ***
	bool MovRegToMem8 ( char* Address, long SrcReg );
	bool MovRegFromMem8 ( long DestReg, char* Address );
	bool MovRegToMem16 ( short* Address, long SrcReg );
	bool MovRegFromMem16 ( long DestReg, short* Address );
	bool MovRegToMem32 ( long* Address, long SrcReg );
	bool MovRegFromMem32 ( long DestReg, long* Address );
	bool MovRegToMem64 ( long long* Address, long SrcReg );
	bool MovRegFromMem64 ( long DestReg, long long* Address );

	bool MovMemImm16 ( short* DestPtr, short Imm16 );
	bool MovMemImm32 ( long* DestPtr, long Imm32 );
	bool MovMemImm64 ( long long* DestPtr, long Imm32 );

	
//	bool MovRegToMem32S ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
//	bool MovRegFromMem32S ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );

	// add
	bool AddRegReg16 ( long DestReg, long SrcReg );
	bool AddRegReg32 ( long DestReg, long SrcReg );
	bool AddRegReg64 ( long DestReg, long SrcReg );
	bool AddRegImm16 ( long DestReg, short Imm16 );
	bool AddRegImm32 ( long DestReg, long Imm32 );
	bool AddRegImm64 ( long DestReg, long Imm32 );

	bool AddRegMem16 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AddRegMem32 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AddRegMem64 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AddMemReg16 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AddMemReg32 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AddMemReg64 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AddMemImm16 ( short Imm16, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AddMemImm32 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AddMemImm64 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );

	bool AddRegMem16 ( long DestReg, short* SrcPtr );
	bool AddRegMem32 ( long DestReg, long* SrcPtr );
	bool AddRegMem64 ( long DestReg, long long* SrcPtr );
	bool AddMemReg16 ( short* DestPtr, long SrcReg );
	bool AddMemReg32 ( long* DestPtr, long SrcReg );
	bool AddMemReg64 ( long long* DestPtr, long SrcReg );
	bool AddMemImm16 ( short* DestPtr, short Imm16 );
	bool AddMemImm32 ( long* DestPtr, long Imm32 );
	bool AddMemImm64 ( long long* DestPtr, long Imm32 );

	// and
	bool AndRegReg16 ( long DestReg, long SrcReg );
	bool AndRegReg32 ( long DestReg, long SrcReg );
	bool AndRegReg64 ( long DestReg, long SrcReg );
	bool AndRegImm16 ( long DestReg, short Imm16 );
	bool AndRegImm32 ( long DestReg, long Imm32 );
	bool AndRegImm64 ( long DestReg, long Imm32 );

	bool AndRegMem16 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AndRegMem32 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AndRegMem64 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AndMemReg16 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AndMemReg32 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AndMemReg64 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AndMemImm16 ( short Imm16, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AndMemImm32 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );
	bool AndMemImm64 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );
	
	bool AndRegMem16 ( long DestReg, short* SrcPtr );
	bool AndRegMem32 ( long DestReg, long* SrcPtr );
	bool AndRegMem64 ( long DestReg, long long* SrcPtr );
	bool AndMemReg16 ( short* DestPtr, long SrcReg );
	bool AndMemReg32 ( long* DestPtr, long SrcReg );
	bool AndMemReg64 ( long long* DestPtr, long SrcReg );
	bool AndMemImm16 ( short* DestPtr, short Imm16 );
	bool AndMemImm32 ( long* DestPtr, long Imm32 );
	bool AndMemImm64 ( long long* DestPtr, long Imm32 );
	
	
	// bt - bit test - stores specified bit of value in carry flag
	bool BtRegReg16 ( long Reg, long BitSelectReg );
	bool BtRegReg32 ( long Reg, long BitSelectReg );
	bool BtRegReg64 ( long Reg, long BitSelectReg );
	bool BtRegImm16 ( long Reg, char Imm8 );
	bool BtRegImm32 ( long Reg, char Imm8 );
	bool BtRegImm64 ( long Reg, char Imm8 );
	
	// with memory
	bool BtMemReg16 ( long BitSelectReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtMemReg32 ( long BitSelectReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtMemReg64 ( long BitSelectReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtMemImm16 ( char Imm8, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtMemImm32 ( char Imm8, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtMemImm64 ( char Imm8, long AddressReg, long IndexReg, long Scale, long Offset );

	// btc - bit test and compliment
	bool BtcRegReg16 ( long Reg, long BitSelectReg );
	bool BtcRegReg32 ( long Reg, long BitSelectReg );
	bool BtcRegReg64 ( long Reg, long BitSelectReg );
	bool BtcRegImm16 ( long Reg, char Imm8 );
	bool BtcRegImm32 ( long Reg, char Imm8 );
	bool BtcRegImm64 ( long Reg, char Imm8 );
	
	// with memory
	bool BtcMemReg16 ( long BitSelectReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtcMemReg32 ( long BitSelectReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtcMemReg64 ( long BitSelectReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtcMemImm16 ( char Imm8, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtcMemImm32 ( char Imm8, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtcMemImm64 ( char Imm8, long AddressReg, long IndexReg, long Scale, long Offset );
	
	// btr - bit test and reset
	bool BtrRegReg16 ( long Reg, long BitSelectReg );
	bool BtrRegReg32 ( long Reg, long BitSelectReg );
	bool BtrRegReg64 ( long Reg, long BitSelectReg );
	bool BtrRegImm16 ( long Reg, char Imm8 );
	bool BtrRegImm32 ( long Reg, char Imm8 );
	bool BtrRegImm64 ( long Reg, char Imm8 );
	
	// with memory
	bool BtrMemReg16 ( long BitSelectReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtrMemReg32 ( long BitSelectReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtrMemReg64 ( long BitSelectReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtrMemImm16 ( char Imm8, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtrMemImm32 ( char Imm8, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtrMemImm64 ( char Imm8, long AddressReg, long IndexReg, long Scale, long Offset );
	
	// bts - bit test and set
	bool BtsRegReg16 ( long Reg, long BitSelectReg );
	bool BtsRegReg32 ( long Reg, long BitSelectReg );
	bool BtsRegReg64 ( long Reg, long BitSelectReg );
	bool BtsRegImm16 ( long Reg, char Imm8 );
	bool BtsRegImm32 ( long Reg, char Imm8 );
	bool BtsRegImm64 ( long Reg, char Imm8 );

	// with memory
	bool BtsMemReg16 ( long BitSelectReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtsMemReg32 ( long BitSelectReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtsMemReg64 ( long BitSelectReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtsMemImm16 ( char Imm8, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtsMemImm32 ( char Imm8, long AddressReg, long IndexReg, long Scale, long Offset );
	bool BtsMemImm64 ( char Imm8, long AddressReg, long IndexReg, long Scale, long Offset );
	
	// cbw, cwde, cdqe - sign extention for RAX - from byte to word, word to double word, or double word to quadword respectively
	bool Cbw ( void );
	bool Cwde ( void );
	bool Cdqe ( void );

	// cmov - conditional mov
	bool CmovERegReg16 ( long DestReg, long SrcReg );
	bool CmovNERegReg16 ( long DestReg, long SrcReg );
	bool CmovBRegReg16 ( long DestReg, long SrcReg );
	bool CmovERegReg32 ( long DestReg, long SrcReg );
	bool CmovNERegReg32 ( long DestReg, long SrcReg );
	bool CmovBRegReg32 ( long DestReg, long SrcReg );
	bool CmovERegReg64 ( long DestReg, long SrcReg );
	bool CmovNERegReg64 ( long DestReg, long SrcReg );
	bool CmovBRegReg64 ( long DestReg, long SrcReg );
	
	// with memory
	bool CmovERegMem16 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool CmovNERegMem16 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool CmovERegMem32 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool CmovNERegMem32 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool CmovERegMem64 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool CmovNERegMem64 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	
	// signed conditional mov
	
	bool CmovLRegReg16 ( long DestReg, long SrcReg );
	bool CmovLERegReg16 ( long DestReg, long SrcReg );
	bool CmovGRegReg16 ( long DestReg, long SrcReg );
	bool CmovGERegReg16 ( long DestReg, long SrcReg );
	bool CmovLRegReg32 ( long DestReg, long SrcReg );
	bool CmovLERegReg32 ( long DestReg, long SrcReg );
	bool CmovGRegReg32 ( long DestReg, long SrcReg );
	bool CmovGERegReg32 ( long DestReg, long SrcReg );
	bool CmovLRegReg64 ( long DestReg, long SrcReg );
	bool CmovLERegReg64 ( long DestReg, long SrcReg );
	bool CmovGRegReg64 ( long DestReg, long SrcReg );
	bool CmovGERegReg64 ( long DestReg, long SrcReg );

	// cmp - compare two values
	bool CmpRegReg16 ( long SrcReg1, long SrcReg2 );
	bool CmpRegReg32 ( long SrcReg1, long SrcReg2 );
	bool CmpRegReg64 ( long SrcReg1, long SrcReg2 );
	bool CmpRegImm16 ( long SrcReg, short Imm16 );
	bool CmpRegImm32 ( long SrcReg, long Imm32 );
	bool CmpRegImm64 ( long SrcReg, long Imm32 );

	bool CmpRegMem16 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool CmpRegMem32 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool CmpRegMem64 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool CmpMemReg16 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool CmpMemReg32 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool CmpMemReg64 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool CmpMemImm16 ( short Imm16, long AddressReg, long IndexReg, long Scale, long Offset );
	bool CmpMemImm32 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );
	bool CmpMemImm64 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );

	bool CmpRegMem16 ( long DestReg, short* SrcPtr );
	bool CmpRegMem32 ( long DestReg, long* SrcPtr );
	bool CmpRegMem64 ( long DestReg, long long* SrcPtr );
	bool CmpMemReg16 ( short* DestPtr, long SrcReg );
	bool CmpMemReg32 ( long* DestPtr, long SrcReg );
	bool CmpMemReg64 ( long long* DestPtr, long SrcReg );
	bool CmpMemImm16 ( short* DestPtr, short Imm16 );
	bool CmpMemImm32 ( long* DestPtr, long Imm32 );
	bool CmpMemImm64 ( long long* DestPtr, long Imm32 );

	// div - unsigned divide - divides d:a by register, puts quotient in a, remainder in d
	bool DivRegReg16 ( long SrcReg );
	bool DivRegReg32 ( long SrcReg );
	bool DivRegReg64 ( long SrcReg );

	bool DivRegMem16 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool DivRegMem32 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool DivRegMem64 ( long AddressReg, long IndexReg, long Scale, long Offset );

	// idiv - signed divide
	bool IdivRegReg16 ( long SrcReg );
	bool IdivRegReg32 ( long SrcReg );
	bool IdivRegReg64 ( long SrcReg );

	bool IdivRegMem16 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool IdivRegMem32 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool IdivRegMem64 ( long AddressReg, long IndexReg, long Scale, long Offset );
	
	// imul - signed multiply
	bool ImulRegReg16 ( long SrcReg );
	bool ImulRegReg32 ( long SrcReg );
	bool ImulRegReg64 ( long SrcReg );

	bool ImulRegMem16 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool ImulRegMem32 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool ImulRegMem64 ( long AddressReg, long IndexReg, long Scale, long Offset );

	// lea - can be used as 3 operand add instruction
	bool LeaRegRegReg16 ( long DestReg, long Src1Reg, long Src2Reg );
	bool LeaRegRegReg32 ( long DestReg, long Src1Reg, long Src2Reg );
	bool LeaRegRegReg64 ( long DestReg, long Src1Reg, long Src2Reg );
	bool LeaRegRegImm16 ( long DestReg, long SrcReg, long Imm16 );
	bool LeaRegRegImm32 ( long DestReg, long SrcReg, long Imm32 );
	bool LeaRegRegImm64 ( long DestReg, long SrcReg, long Imm32 );
	
	// movsx - move with sign extension
	bool MovsxReg16Reg8 ( long DestReg, long SrcReg );
	bool MovsxReg32Reg8 ( long DestReg, long SrcReg );
	bool MovsxReg64Reg8 ( long DestReg, long SrcReg );

	bool MovsxReg16Mem8 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool MovsxReg32Mem8 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool MovsxReg64Mem8 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );

	bool MovsxReg32Reg16 ( long DestReg, long SrcReg );
	bool MovsxReg64Reg16 ( long DestReg, long SrcReg );
	bool MovsxdReg64Reg32 ( long DestReg, long SrcReg );

	bool MovsxReg32Mem16 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool MovsxReg64Mem16 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool MovsxdReg64Mem32 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );

	// mul - unsigned multiply
	bool MulRegReg16 ( long SrcReg );
	bool MulRegReg32 ( long SrcReg );
	bool MulRegReg64 ( long SrcReg );
	
	bool MulRegMem16 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool MulRegMem32 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool MulRegMem64 ( long AddressReg, long IndexReg, long Scale, long Offset );

	// neg
	bool NegReg16 ( long DestReg );
	bool NegReg32 ( long DestReg );
	bool NegReg64 ( long DestReg );
	
	bool NegMem16 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool NegMem32 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool NegMem64 ( long AddressReg, long IndexReg, long Scale, long Offset );

	// not 
	bool NotReg16 ( long DestReg );
	bool NotReg32 ( long DestReg );
	bool NotReg64 ( long DestReg );

	bool NotMem16 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool NotMem32 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool NotMem64 ( long AddressReg, long IndexReg, long Scale, long Offset );

	// or
	bool OrRegReg16 ( long DestReg, long SrcReg );
	bool OrRegReg32 ( long DestReg, long SrcReg );
	bool OrRegReg64 ( long DestReg, long SrcReg );
	bool OrRegImm16 ( long DestReg, short Imm16 );
	bool OrRegImm32 ( long DestReg, long Imm32 );
	bool OrRegImm64 ( long DestReg, long Imm32 );

	bool OrRegMem16 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool OrRegMem32 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool OrRegMem64 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool OrMemReg16 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool OrMemReg32 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool OrMemReg64 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool OrMemImm16 ( short Imm16, long AddressReg, long IndexReg, long Scale, long Offset );
	bool OrMemImm32 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );
	bool OrMemImm64 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );

	bool OrRegMem16 ( long DestReg, short* SrcPtr );
	bool OrRegMem32 ( long DestReg, long* SrcPtr );
	bool OrRegMem64 ( long DestReg, long long* SrcPtr );
	bool OrMemReg16 ( short* DestPtr, long SrcReg );
	bool OrMemReg32 ( long* DestPtr, long SrcReg );
	bool OrMemReg64 ( long long* DestPtr, long SrcReg );
	bool OrMemImm16 ( short* DestPtr, short Imm16 );
	bool OrMemImm32 ( long* DestPtr, long Imm32 );
	bool OrMemImm64 ( long long* DestPtr, long Imm32 );
	
	// pop - pop register from stack
	bool PopReg16 ( long DestReg );
	bool PopReg32 ( long DestReg );
	bool PopReg64 ( long DestReg );
	
	// push - push register onto stack
	bool PushReg16 ( long SrcReg );
	bool PushReg32 ( long SrcReg );
	bool PushReg64 ( long SrcReg );

	bool PushImm8 ( char Imm8 );
	bool PushImm16 ( short Imm16 );
	bool PushImm32 ( long Imm32 );

	// ret
	bool Ret ( void );
	
	// set - set byte instructions
	
	// unsigned versions
	
	// seta - set if unsigned above
	bool Set_A ( long DestReg );
	bool Set_A ( long AddressReg, long IndexReg, long Scale, long Offset );
	
	// setae - set if unsigned above or equal
	bool Set_AE ( long DestReg );
	bool Set_AE ( long AddressReg, long IndexReg, long Scale, long Offset );
	
	// setb - set if unsigned below
	bool Set_B ( long DestReg );
	bool Set_B ( long AddressReg, long IndexReg, long Scale, long Offset );
	
	// setbe - set if unsigned below or equal
	bool Set_BE ( long DestReg );
	bool Set_BE ( long AddressReg, long IndexReg, long Scale, long Offset );
	
	// sete - set if equal
	bool Set_E ( long DestReg );
	bool Set_E ( long AddressReg, long IndexReg, long Scale, long Offset );
	
	// setne - set if equal
	bool Set_NE ( long DestReg );
	bool Set_NE ( long AddressReg, long IndexReg, long Scale, long Offset );
	
	// signed versions

	// setg - set if signed greater
	bool Set_G ( long DestReg );
	bool Set_G ( long AddressReg, long IndexReg, long Scale, long Offset );

	// setge - set if signed greater
	bool Set_GE ( long DestReg );
	bool Set_GE ( long AddressReg, long IndexReg, long Scale, long Offset );

	// setl - set if signed greater
	bool Set_L ( long DestReg );
	bool Set_L ( long AddressReg, long IndexReg, long Scale, long Offset );

	// setle - set if signed greater
	bool Set_LE ( long DestReg );
	bool Set_LE ( long AddressReg, long IndexReg, long Scale, long Offset );

	// setb - set if unsigned below
	bool Setb ( long DestReg );
	
	// setl - set if unsigned less than
	bool Setl ( long DestReg );
	bool Setl ( long AddressReg, long IndexReg, long Scale, long Offset );

	// shl - shift left logical - these shift by the value in register c or by an immediate 8-bit value
	bool ShlRegReg16 ( long DestReg );
	bool ShlRegReg32 ( long DestReg );
	bool ShlRegReg64 ( long DestReg );
	bool ShlRegImm16 ( long DestReg, char Imm8 );
	bool ShlRegImm32 ( long DestReg, char Imm8 );
	bool ShlRegImm64 ( long DestReg, char Imm8 );
	
	// with memory
	bool ShlMemReg16 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool ShlMemReg32 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool ShlMemReg64 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool ShlMemImm16 ( long AddressReg, long IndexReg, long Scale, long Offset, char Imm8 );
	bool ShlMemImm32 ( long AddressReg, long IndexReg, long Scale, long Offset, char Imm8 );
	bool ShlMemImm64 ( long AddressReg, long IndexReg, long Scale, long Offset, char Imm8 );

	// sar - shift right arithemetic - these shift by the value in register c or by an immediate 8-bit value
	bool SarRegReg16 ( long DestReg );
	bool SarRegReg32 ( long DestReg );
	bool SarRegReg64 ( long DestReg );
	bool SarRegImm16 ( long DestReg, char Imm8 );
	bool SarRegImm32 ( long DestReg, char Imm8 );
	bool SarRegImm64 ( long DestReg, char Imm8 );

	// with memory
	bool SarMemReg16 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool SarMemReg32 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool SarMemReg64 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool SarMemImm16 ( long AddressReg, long IndexReg, long Scale, long Offset, char Imm8 );
	bool SarMemImm32 ( long AddressReg, long IndexReg, long Scale, long Offset, char Imm8 );
	bool SarMemImm64 ( long AddressReg, long IndexReg, long Scale, long Offset, char Imm8 );

	// shr - shift right logical - these shift by the value in register c or by an immediate 8-bit value
	bool ShrRegReg16 ( long DestReg );
	bool ShrRegReg32 ( long DestReg );
	bool ShrRegReg64 ( long DestReg );
	bool ShrRegImm16 ( long DestReg, char Imm8 );
	bool ShrRegImm32 ( long DestReg, char Imm8 );
	bool ShrRegImm64 ( long DestReg, char Imm8 );

	// with memory
	bool ShrMemReg16 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool ShrMemReg32 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool ShrMemReg64 ( long AddressReg, long IndexReg, long Scale, long Offset );
	bool ShrMemImm16 ( long AddressReg, long IndexReg, long Scale, long Offset, char Imm8 );
	bool ShrMemImm32 ( long AddressReg, long IndexReg, long Scale, long Offset, char Imm8 );
	bool ShrMemImm64 ( long AddressReg, long IndexReg, long Scale, long Offset, char Imm8 );

	// sub
	bool SubRegReg16 ( long DestReg, long SrcReg );
	bool SubRegReg32 ( long DestReg, long SrcReg );
	bool SubRegReg64 ( long DestReg, long SrcReg );
	bool SubRegImm16 ( long DestReg, short Imm16 );
	bool SubRegImm32 ( long DestReg, long Imm32 );
	bool SubRegImm64 ( long DestReg, long Imm32 );

	bool SubRegMem16 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool SubRegMem32 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool SubRegMem64 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool SubMemReg16 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool SubMemReg32 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool SubMemReg64 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool SubMemImm16 ( short Imm16, long AddressReg, long IndexReg, long Scale, long Offset );
	bool SubMemImm32 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );
	bool SubMemImm64 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );

	bool SubRegMem16 ( long DestReg, short* SrcPtr );
	bool SubRegMem32 ( long DestReg, long* SrcPtr );
	bool SubRegMem64 ( long DestReg, long long* SrcPtr );
	bool SubMemReg16 ( short* DestPtr, long SrcReg );
	bool SubMemReg32 ( long* DestPtr, long SrcReg );
	bool SubMemReg64 ( long long* DestPtr, long SrcReg );
	bool SubMemImm16 ( short* DestPtr, short Imm16 );
	bool SubMemImm32 ( long* DestPtr, long Imm32 );
	bool SubMemImm64 ( long long* DestPtr, long Imm32 );
	
	
	// xchg - exchange register with register or memory
	bool XchgRegReg16 ( long DestReg, long SrcReg );
	bool XchgRegReg32 ( long DestReg, long SrcReg );
	bool XchgRegReg64 ( long DestReg, long SrcReg );

	bool XchgRegMem16 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool XchgRegMem32 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool XchgRegMem64 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );

	// xor
	bool XorRegReg16 ( long DestReg, long SrcReg );
	bool XorRegReg32 ( long DestReg, long SrcReg );
	bool XorRegReg64 ( long DestReg, long SrcReg );
	bool XorRegImm16 ( long DestReg, short Imm16 );
	bool XorRegImm32 ( long DestReg, long Imm32 );
	bool XorRegImm64 ( long DestReg, long Imm32 );

	bool XorRegMem16 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool XorRegMem32 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool XorRegMem64 ( long DestReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool XorMemReg16 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool XorMemReg32 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool XorMemReg64 ( long SrcReg, long AddressReg, long IndexReg, long Scale, long Offset );
	bool XorMemImm16 ( short Imm16, long AddressReg, long IndexReg, long Scale, long Offset );
	bool XorMemImm32 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );
	bool XorMemImm64 ( long Imm32, long AddressReg, long IndexReg, long Scale, long Offset );

	bool XorRegMem16 ( long DestReg, short* SrcPtr );
	bool XorRegMem32 ( long DestReg, long* SrcPtr );
	bool XorRegMem64 ( long DestReg, long long* SrcPtr );
	bool XorMemReg16 ( short* DestPtr, long SrcReg );
	bool XorMemReg32 ( long* DestPtr, long SrcReg );
	bool XorMemReg64 ( long long* DestPtr, long SrcReg );
	bool XorMemImm16 ( short* DestPtr, short Imm16 );
	bool XorMemImm32 ( long* DestPtr, long Imm32 );
	bool XorMemImm64 ( long long* DestPtr, long Imm32 );
	
	
	// ** SSE/AVX Register Instructions ** //
	
	// movhlps - combine high quadwords of xmm registers, 3rd operand on bottom
	bool movhlps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	// movlhps - combine low quadwords of xmm registers, 3rd operand on top
	bool movlhps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );

	// pinsr
	bool pinsrb ( long sseDestReg, long sseSrcReg1, long x64SrcReg2, char Imm8 );
	bool pinsrw ( long sseDestReg, long sseSrcReg1, long x64SrcReg2, char Imm8 );
	bool pinsrd ( long sseDestReg, long sseSrcReg1, long x64SrcReg2, char Imm8 );
	bool pinsrq ( long sseDestReg, long sseSrcReg1, long x64SrcReg2, char Imm8 );

	// pextr
	bool pextrb ( long x64DestReg, long sseSrcReg, char Imm8 );
	bool pextrw ( long x64DestReg, long sseSrcReg, char Imm8 );
	bool pextrd ( long x64DestReg, long sseSrcReg, char Imm8 );
	bool pextrq ( long x64DestReg, long sseSrcReg, char Imm8 );
	
	// blendvps
//	bool blendvps ( long sseDestReg, long sseSrcReg1, long x64SrcReg2, long x64SrcReg3 );
	
	// pblendw
	bool pblendw ( long sseDestReg, long sseSrcReg1, long x64SrcReg2, char Imm8 );
	
	// movaps
	bool movaps128 ( long sseDestReg, long sseSrcReg );
	bool movaps_to_mem128 ( long sseSrcReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool movaps_from_mem128 ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );

	// movdqa
	bool movdqa128 ( long sseDestReg, long sseSrcReg );
	bool movdqa_to_mem128 ( long sseSrcReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool movdqa_from_mem128 ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool movdqa256 ( long sseDestReg, long sseSrcReg );
	bool movdqa_to_mem256 ( long sseSrcReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool movdqa_from_mem256 ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	
	// movss - move single 32-bit value from memory or to memory
	bool movss_to_mem128 ( long sseSrcReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool movss_from_mem128 ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );

	// pabs
	bool pabsw ( long sseDestReg, long sseSrcReg );
	bool pabsd ( long sseDestReg, long sseSrcReg );
	
	// packus
	bool packusdw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool packuswb ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );

	// padd
	bool paddb ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool paddw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool paddd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );

	// paddus
	bool paddusb ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool paddusw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );

	// padds - add packed signed values with saturation
	bool paddsb ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool paddsw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	// pand
	bool pand ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	// pandn
	bool pandn ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );

	// todo
	
	// pcmpeq - compare packed integers - sets destination to all 1s if equal, sets to all 0s otherwise
	bool pcmpeqb ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool pcmpeqw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool pcmpeqd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );

	// pcmpgt - compare packed signed integers for greater than
	bool pcmpgtb ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool pcmpgtw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool pcmpgtd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	// pmax - get maximum of signed integers
	bool pmaxsw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool pmaxsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	// pmin - get minimum of signed integers
	bool pminsw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool pminsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	// pmovsxdq - move and sign extend lower 2 32-bit packed integers into 2 64-bit packed integers
	bool pmovsxdq ( long sseDestReg, long sseSrcReg );
	
	// pmuludq
	bool pmuludq ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );

	// por - packed logical or of integers
	bool por ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );

	// pshuf - shuffle packed values
	bool pshufb ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool pshufd ( long sseDestReg, long sseSrcReg, char Imm8 );

	// pshufhw - packed shuffle high words
	bool pshufhw ( long sseDestReg, long sseSrcReg, char Imm8 );

	// pshuflw - packed shuffle low words
	bool pshuflw ( long sseDestReg, long sseSrcReg, char Imm8 );

	// psll - packed shift logical left integers
	bool pslldq ( long sseDestReg, long sseSrcReg, char Imm8 );
	bool psllw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool psllw_imm ( long sseDestReg, long sseSrcReg, char Imm8 );
	bool pslld ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool pslld_imm ( long sseDestReg, long sseSrcReg, char Imm8 );
	
	// psra - packed shift arithemetic right integers
	bool psraw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool psraw_imm ( long sseDestReg, long sseSrcReg, char Imm8 );
	bool psrad ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool psrad_imm ( long sseDestReg, long sseSrcReg, char Imm8 );
	
	// psrl - packed shift logical right integers
	bool psrldq ( long sseDestReg, long sseSrcReg, char Imm8 );
	bool psrlw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool psrlw_imm ( long sseDestReg, long sseSrcReg, char Imm8 );
	bool psrld ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool psrld_imm ( long sseDestReg, long sseSrcReg, char Imm8 );
	
	// psub - subtraction of packed integers
	bool psubb ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool psubw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool psubd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );

	// psubs - subtract packed integers with signed saturation
	bool psubsb ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool psubsw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	// psubus - subtract packed integers with unsigned saturation
	bool psubusb ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool psubusw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	// punpckh - unpack from high - first source on bottom
	bool punpckhbw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool punpckhwd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool punpckhdq ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool punpckhqdq ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );

	// punpckl - unpack from low - first source on bottom
	bool punpcklbw ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool punpcklwd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool punpckldq ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool punpcklqdq ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );

	// pxor
	bool pxor ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	
	//** SSE Floating Point Instructions **//
	
	// extractps
	bool extractps ( long x64DestReg, long sseSrcReg, char Imm8 );
	
	// movmskps
	bool movmskps256 ( long x64DestReg, long sseSrcReg );
	
	// movaps
	bool movaps ( long sseDestReg, long sseSrcReg );
	bool movapd ( long sseDestReg, long sseSrcReg );
	bool movapstomem ( long sseSrcReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool movapsfrommem ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool movapdtomem ( long sseSrcReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool movapdfrommem ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );

	// vbroadcastss
	bool vbroadcastss ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool vbroadcastsd ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	
	// vmaskmov
	bool vmaskmovpstomem ( long sseSrcReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset, long sseMaskReg );
	bool vmaskmovpdtomem ( long sseSrcReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset, long sseMaskReg );

	// blendvp
	bool blendvps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg, long sseSrc3Reg );
	bool blendvpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg, long sseSrc3Reg );
	bool blendps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg, char Imm8 );
	bool blendpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg, char Imm8 );

	bool blendvps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset, long sseSrc3Reg );
	bool blendvpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset, long sseSrc3Reg );
	bool blendps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset, char Imm8 );
	bool blendpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset, char Imm8 );
	
	// addp
	bool addps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool addpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool addss ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool addsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );

	bool addps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool addpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool addss ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool addsd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );

	// andp
	bool andps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool andpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	bool andps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool andpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );

	// andnp
	bool andnps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool andnpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	bool andnps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool andnpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );

	// cmpxxp
	bool cmpps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg, char Imm8 );
	bool cmppd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg, char Imm8 );
	bool cmpss ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg, char Imm8 );
	bool cmpsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg, char Imm8 );
	bool cmpeqps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpeqpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpeqss ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpeqsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpltps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpltpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpltss ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpltsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpgtps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpgtpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpgtss ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpgtsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpleps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmplepd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpless ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmplesd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpgeps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpgepd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpgess ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpgesd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpunordps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpunordpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpunordss ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpunordsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpordps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpordpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpordss ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool cmpordsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	bool cmpps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset, char Imm8 );
	bool cmppd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset, char Imm8 );
	bool cmpss ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset, char Imm8 );
	bool cmpsd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset, char Imm8 );
	bool cmpeqps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpeqpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpeqss ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpeqsd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpltps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpltpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpltss ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpltsd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpgtps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpgtpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpgtss ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpgtsd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpleps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmplepd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpless ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmplesd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpgeps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpgepd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpgess ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpgesd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpunordps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpunordpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpunordss ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpunordsd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpordps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpordpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpordss ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cmpordsd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	
	// cvtp
	bool cvtdq2pd ( long sseDestReg, long sseSrc1Reg );
	bool cvtdq2ps ( long sseDestReg, long sseSrc1Reg );
	bool cvtpd2dq ( long sseDestReg, long sseSrc1Reg );
	bool cvtps2dq ( long sseDestReg, long sseSrc1Reg );
	bool cvtps2pd ( long sseDestReg, long sseSrc1Reg );
	bool cvtpd2ps ( long sseDestReg, long sseSrc1Reg );
	
	bool cvtdq2pd ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cvtdq2ps ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cvtpd2dq ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cvtps2dq ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cvtps2pd ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool cvtpd2ps ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );

	// divp
	bool divps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool divpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool divss ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool divsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	bool divps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool divpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool divss ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool divsd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );

	//maxp
	bool maxps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool maxpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool maxss ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool maxsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	bool maxps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool maxpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool maxss ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool maxsd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );

	// minp
	bool minps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool minpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool minss ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool minsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	bool minps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool minpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool minss ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool minsd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );

	// mulp
	bool mulps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool mulpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool mulss ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool mulsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	bool mulps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool mulpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool mulss ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool mulsd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	
	// orp
	bool orps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool orpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );

	bool orps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool orpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );

	// shufp
	bool shufps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg, char Imm8 );
	bool shufpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg, char Imm8 );

	bool shufps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset, char Imm8 );
	bool shufpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset, char Imm8 );
	
	// rsqrtp - I don't think this is used
	
	// sqrtp
	bool sqrtps ( long sseDestReg, long sseSrc1Reg );
	bool sqrtpd ( long sseDestReg, long sseSrc1Reg );
	bool sqrtss ( long sseDestReg, long sseSrc1Reg );
	bool sqrtsd ( long sseDestReg, long sseSrc1Reg );
	
	bool sqrtps ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool sqrtpd ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool sqrtss ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool sqrtsd ( long sseDestReg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	
	// subp
	bool subps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool subpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool subss ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool subsd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	bool subps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool subpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool subss ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool subsd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	
	// vperm2f128
	bool vperm2f128 ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg, char Imm8 );
	
	// xorp
	bool xorps ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	bool xorpd ( long sseDestReg, long sseSrc1Reg, long sseSrc2Reg );
	
	bool xorps ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	bool xorpd ( long sseDestReg, long sseSrc1Reg, long x64BaseReg, long x64IndexReg, long Scale, long Offset );
	
	
	// **** bookmarking **** //
	
	// if you have a jump and don't know how far you'll need to jump, you'll need to store the jump amount later
	// this will return a bookmark for the current byte and advance to the next byte so you can insert the next instruction
	long x64GetBookmark8 ( void );
	
	// this is where you can write to where you put the bookmark - this will be used for short jumps
	bool x64SetBookmark8 ( long Bookmark, char value );

private:

	bool x64EncodeReg16 ( long x64InstOpcode, long ModRMOpcode, long x64Reg );

	// general encode of sib and immediates and stuff for register-memory instructions
	inline bool x64EncodeMem ( long x64DestReg, long BaseAddressReg, long IndexReg, long Scale, long Offset );
	
	inline bool x64Encode16Bit ( void );
	
	bool x64EncodeRexReg32 ( long DestReg_RM_Base, long SourceReg_Reg_Opcode );
	bool x64EncodeRexReg64 ( long DestReg_RM_Base, long SourceReg_Reg_Opcode );

	// general encoding of opcode(s)
	inline bool x64EncodeOpcode ( long x64InstOpcode );
};




#endif


