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



#include "PS1_SPU2.h"
#include "PS1_CD.h"


using namespace Playstation1;




#ifdef _DEBUG_VERSION_

// enable debugging
#define INLINE_DEBUG_ENABLE

/*
#define INLINE_DEBUG_WRITE
#define INLINE_DEBUG_READ
*/

//#define INLINE_DEBUG_REVERB
//#define INLINE_DEBUG_DMA_WRITE
//#define INLINE_DEBUG_DMA_WRITE_RECORD
//#define INLINE_DEBUG_SPU_ERROR_RECORD
//#define INLINE_DEBUG
//#define INLINE_DEBUG_CDSOUND
//#define INLINE_DEBUG_RUN
//#define INLINE_DEBUG_RUN_ATTACK
//#define INLINE_DEBUG_ENVELOPE
//#define INLINE_DEBUG_WRITE_DEFAULT
//#define INLINE_DEBUG_RUN_CHANNELONOFF
//#define INLINE_DEBUG_READ_CHANNELONOFF
//#define INLINE_DEBUG_WRITE_CHANNELONOFF
//#define INLINE_DEBUG_WRITE_IRQA
//#define INLINE_DEBUG_WRITE_CTRL
//#define INLINE_DEBUG_WRITE_STAT
//#define INLINE_DEBUG_WRITE_DATA
//#define INLINE_DEBUG_WRITE_SBA
//#define INLINE_DEBUG_WRITE_LSA_X

#endif



static Debug::Log SPUCore::debug;
static SPUCore *SPUCore::_SPUCore;
static u16 *SPUCore::RAM;


static u32* SPUCore::_DebugPC;
static u64* SPUCore::_DebugCycleCount;

static u32* SPUCore::_Intc_Stat;
static u32* SPUCore::_Intc_Mask;
static u32* SPUCore::_R3000A_Status_12;
static u32* SPUCore::_R3000A_Cause_13;
static u64* SPUCore::_ProcStatus;




u32* SPU2::_DebugPC;
u64* SPU2::_DebugCycleCount;


u64* SPU2::_NextSystemEvent;


u32* SPU2::_Intc_Stat;
u32* SPU2::_Intc_Mask;
u32* SPU2::_R3000A_Status_12;
u32* SPU2::_R3000A_Cause_13;
u64* SPU2::_ProcStatus;


Debug::Log SPU2::debug;

SPU2 *SPU2::_SPU2;

/*
static s16 *SPU::_vLOUT;
static s16 *SPU::_vROUT;
static u16 *SPU::_mBASE;

static u16 *SPU::_dAPF1;
static u16 *SPU::_dAPF2;
static s16 *SPU::_vIIR;
static s16 *SPU::_vCOMB1;
static s16 *SPU::_vCOMB2;
static s16 *SPU::_vCOMB3;
static s16 *SPU::_vCOMB4;
static s16 *SPU::_vWALL;
static s16 *SPU::_vAPF1;
static s16 *SPU::_vAPF2;
static u16 *SPU::_mLSAME;
static u16 *SPU::_mRSAME;
static u16 *SPU::_mLCOMB1;
static u16 *SPU::_mRCOMB1;
static u16 *SPU::_mLCOMB2;
static u16 *SPU::_mRCOMB2;
static u16 *SPU::_dLSAME;
static u16 *SPU::_dRSAME;
static u16 *SPU::_mLDIFF;
static u16 *SPU::_mRDIFF;
static u16 *SPU::_mLCOMB3;
static u16 *SPU::_mRCOMB3;
static u16 *SPU::_mLCOMB4;
static u16 *SPU::_mRCOMB4;
static u16 *SPU::_dLDIFF;
static u16 *SPU::_dRDIFF;
static u16 *SPU::_mLAPF1;
static u16 *SPU::_mRAPF1;
static u16 *SPU::_mLAPF2;
static u16 *SPU::_mRAPF2;
static s16 *SPU::_vLIN;
static s16 *SPU::_vRIN;
*/



//bool SPUCore::DebugWindow_Enabled;
//WindowClass::Window *SPUCore::DebugWindow;
//DebugValueList<u16> *SPUCore::SPUMaster_ValueList;
//DebugValueList<u16> *SPUCore::SPU_ValueList [ 24 ];
//Debug_MemoryViewer *SPUCore::SoundRAM_Viewer;

static u32 SPUCore::Debug_ChannelEnable = 0xffffff;

static HWAVEOUT SPU2::hWaveOut; /* device handle */
static WAVEFORMATEX SPU2::wfx;
static WAVEHDR SPU2::header;
static WAVEHDR SPU2::header0;
static WAVEHDR SPU2::header1;



// *** testing ***
//u64 SPU::hWaveOut_Save;


// 419 registers in this section (0x0-0x346 [0-838])
static const char* SPUCore::RegisterNames1 [ 419 ] = {
	// 0x0
	"Ch0_VOL_L", "Ch0_VOL_R", "Ch0_PITCH", "Ch0_ADSR_0", "Ch0_ADSR_1", "Ch0_ENV_X", "Ch0_CVOL_L", "Ch0_CVOL_R",
	"Ch1_VOL_L", "Ch1_VOL_R", "Ch1_PITCH", "Ch1_ADSR_0", "Ch1_ADSR_1", "Ch1_ENV_X", "Ch1_CVOL_L", "Ch1_CVOL_R",
	"Ch2_VOL_L", "Ch2_VOL_R", "Ch2_PITCH", "Ch2_ADSR_0", "Ch2_ADSR_1", "Ch2_ENV_X", "Ch2_CVOL_L", "Ch2_CVOL_R",
	"Ch3_VOL_L", "Ch3_VOL_R", "Ch3_PITCH", "Ch3_ADSR_0", "Ch3_ADSR_1", "Ch3_ENV_X", "Ch3_CVOL_L", "Ch3_CVOL_R",
	"Ch4_VOL_L", "Ch4_VOL_R", "Ch4_PITCH", "Ch4_ADSR_0", "Ch4_ADSR_1", "Ch4_ENV_X", "Ch4_CVOL_L", "Ch4_CVOL_R",
	"Ch5_VOL_L", "Ch5_VOL_R", "Ch5_PITCH", "Ch5_ADSR_0", "Ch5_ADSR_1", "Ch5_ENV_X", "Ch5_CVOL_L", "Ch5_CVOL_R",
	"Ch6_VOL_L", "Ch6_VOL_R", "Ch6_PITCH", "Ch6_ADSR_0", "Ch6_ADSR_1", "Ch6_ENV_X", "Ch6_CVOL_L", "Ch6_CVOL_R",
	"Ch7_VOL_L", "Ch7_VOL_R", "Ch7_PITCH", "Ch7_ADSR_0", "Ch7_ADSR_1", "Ch7_ENV_X", "Ch7_CVOL_L", "Ch7_CVOL_R",
	"Ch8_VOL_L", "Ch8_VOL_R", "Ch8_PITCH", "Ch8_ADSR_0", "Ch8_ADSR_1", "Ch8_ENV_X", "Ch8_CVOL_L", "Ch8_CVOL_R",
	"Ch9_VOL_L", "Ch9_VOL_R", "Ch9_PITCH", "Ch9_ADSR_0", "Ch9_ADSR_1", "Ch9_ENV_X", "Ch9_CVOL_L", "Ch9_CVOL_R",
	"Ch10_VOL_L", "Ch10_VOL_R", "Ch10_PITCH", "Ch10_ADSR_0", "Ch10_ADSR_1", "Ch10_ENV_X", "Ch10_CVOL_L", "Ch10_CVOL_R",
	"Ch11_VOL_L", "Ch11_VOL_R", "Ch11_PITCH", "Ch11_ADSR_0", "Ch11_ADSR_1", "Ch11_ENV_X", "Ch11_CVOL_L", "Ch11_CVOL_R",
	"Ch12_VOL_L", "Ch12_VOL_R", "Ch12_PITCH", "Ch12_ADSR_0", "Ch12_ADSR_1", "Ch12_ENV_X", "Ch12_CVOL_L", "Ch12_CVOL_R",
	"Ch13_VOL_L", "Ch13_VOL_R", "Ch13_PITCH", "Ch13_ADSR_0", "Ch13_ADSR_1", "Ch13_ENV_X", "Ch13_CVOL_L", "Ch13_CVOL_R",
	"Ch14_VOL_L", "Ch14_VOL_R", "Ch14_PITCH", "Ch14_ADSR_0", "Ch14_ADSR_1", "Ch14_ENV_X", "Ch14_CVOL_L", "Ch14_CVOL_R",
	"Ch15_VOL_L", "Ch15_VOL_R", "Ch15_PITCH", "Ch15_ADSR_0", "Ch15_ADSR_1", "Ch15_ENV_X", "Ch15_CVOL_L", "Ch15_CVOL_R",
	"Ch16_VOL_L", "Ch16_VOL_R", "Ch16_PITCH", "Ch16_ADSR_0", "Ch16_ADSR_1", "Ch16_ENV_X", "Ch16_CVOL_L", "Ch16_CVOL_R",
	"Ch17_VOL_L", "Ch17_VOL_R", "Ch17_PITCH", "Ch17_ADSR_0", "Ch17_ADSR_1", "Ch17_ENV_X", "Ch17_CVOL_L", "Ch17_CVOL_R",
	"Ch18_VOL_L", "Ch18_VOL_R", "Ch18_PITCH", "Ch18_ADSR_0", "Ch18_ADSR_1", "Ch18_ENV_X", "Ch18_CVOL_L", "Ch18_CVOL_R",
	"Ch19_VOL_L", "Ch19_VOL_R", "Ch19_PITCH", "Ch19_ADSR_0", "Ch19_ADSR_1", "Ch19_ENV_X", "Ch19_CVOL_L", "Ch19_CVOL_R",
	"Ch20_VOL_L", "Ch20_VOL_R", "Ch20_PITCH", "Ch20_ADSR_0", "Ch20_ADSR_1", "Ch20_ENV_X", "Ch20_CVOL_L", "Ch20_CVOL_R",
	"Ch21_VOL_L", "Ch21_VOL_R", "Ch21_PITCH", "Ch21_ADSR_0", "Ch21_ADSR_1", "Ch21_ENV_X", "Ch21_CVOL_L", "Ch21_CVOL_R",
	"Ch22_VOL_L", "Ch22_VOL_R", "Ch22_PITCH", "Ch22_ADSR_0", "Ch22_ADSR_1", "Ch22_ENV_X", "Ch22_CVOL_L", "Ch22_CVOL_R",
	"Ch23_VOL_L", "Ch23_VOL_R", "Ch23_PITCH", "Ch23_ADSR_0", "Ch23_ADSR_1", "Ch23_ENV_X", "Ch23_CVOL_L", "Ch23_CVOL_R",
	// 0x180 (384 dec [192x2])
	"PMON_0", "PMON_1", "NON_0", "NON_1", "VMIXL_0", "VMIXL_1", "VMIXEL_0", "VMIXEL_1",
	"VMIXR_0", "VMIXR_1", "VMIXER_0", "VMIXER_1", "MMIX", "CTRL", "IRQA_0", "IRQA_1",
	"KON_0", "KON_1", "KOFF_0", "KOFF_1", "SBA_0", "SBA_1", "DATA", "UNK0",
	"ADMAS?", "UNK1", "UNK2", "UNK3", "UNK4", "UNK5", "UNK6", "UNK7",
	// 0x1c0 (448 dec [224x2])
	"Ch0_SSA_0", "Ch0_SSA_1", "Ch0_LSA_0", "Ch0_LSA_1", "Ch0_NEX_0", "Ch0_NEX_1",
	"Ch1_SSA_0", "Ch1_SSA_1", "Ch1_LSA_0", "Ch1_LSA_1", "Ch1_NEX_0", "Ch1_NEX_1",
	"Ch2_SSA_0", "Ch2_SSA_1", "Ch2_LSA_0", "Ch2_LSA_1", "Ch2_NEX_0", "Ch2_NEX_1",
	"Ch3_SSA_0", "Ch3_SSA_1", "Ch3_LSA_0", "Ch3_LSA_1", "Ch3_NEX_0", "Ch3_NEX_1",
	"Ch4_SSA_0", "Ch4_SSA_1", "Ch4_LSA_0", "Ch4_LSA_1", "Ch4_NEX_0", "Ch4_NEX_1",
	"Ch5_SSA_0", "Ch5_SSA_1", "Ch5_LSA_0", "Ch5_LSA_1", "Ch5_NEX_0", "Ch5_NEX_1",
	"Ch6_SSA_0", "Ch6_SSA_1", "Ch6_LSA_0", "Ch6_LSA_1", "Ch6_NEX_0", "Ch6_NEX_1",
	"Ch7_SSA_0", "Ch7_SSA_1", "Ch7_LSA_0", "Ch7_LSA_1", "Ch7_NEX_0", "Ch7_NEX_1",
	"Ch8_SSA_0", "Ch8_SSA_1", "Ch8_LSA_0", "Ch8_LSA_1", "Ch8_NEX_0", "Ch8_NEX_1",
	"Ch9_SSA_0", "Ch9_SSA_1", "Ch9_LSA_0", "Ch9_LSA_1", "Ch9_NEX_0", "Ch9_NEX_1",
	"Ch10_SSA_0", "Ch10_SSA_1", "Ch10_LSA_0", "Ch10_LSA_1", "Ch10_NEX_0", "Ch10_NEX_1",
	"Ch11_SSA_0", "Ch11_SSA_1", "Ch11_LSA_0", "Ch11_LSA_1", "Ch11_NEX_0", "Ch11_NEX_1",
	"Ch12_SSA_0", "Ch12_SSA_1", "Ch12_LSA_0", "Ch12_LSA_1", "Ch12_NEX_0", "Ch12_NEX_1",
	"Ch13_SSA_0", "Ch13_SSA_1", "Ch13_LSA_0", "Ch13_LSA_1", "Ch13_NEX_0", "Ch13_NEX_1",
	"Ch14_SSA_0", "Ch14_SSA_1", "Ch14_LSA_0", "Ch14_LSA_1", "Ch14_NEX_0", "Ch14_NEX_1",
	"Ch15_SSA_0", "Ch15_SSA_1", "Ch15_LSA_0", "Ch15_LSA_1", "Ch15_NEX_0", "Ch15_NEX_1",
	"Ch16_SSA_0", "Ch16_SSA_1", "Ch16_LSA_0", "Ch16_LSA_1", "Ch16_NEX_0", "Ch16_NEX_1",
	"Ch17_SSA_0", "Ch17_SSA_1", "Ch17_LSA_0", "Ch17_LSA_1", "Ch17_NEX_0", "Ch17_NEX_1",
	"Ch18_SSA_0", "Ch18_SSA_1", "Ch18_LSA_0", "Ch18_LSA_1", "Ch18_NEX_0", "Ch18_NEX_1",
	"Ch19_SSA_0", "Ch19_SSA_1", "Ch19_LSA_0", "Ch19_LSA_1", "Ch19_NEX_0", "Ch19_NEX_1",
	"Ch20_SSA_0", "Ch20_SSA_1", "Ch20_LSA_0", "Ch20_LSA_1", "Ch20_NEX_0", "Ch20_NEX_1",
	"Ch21_SSA_0", "Ch21_SSA_1", "Ch21_LSA_0", "Ch21_LSA_1", "Ch21_NEX_0", "Ch21_NEX_1",
	"Ch22_SSA_0", "Ch22_SSA_1", "Ch22_LSA_0", "Ch22_LSA_1", "Ch22_NEX_0", "Ch22_NEX_1",
	"Ch23_SSA_0", "Ch23_SSA_1", "Ch23_LSA_0", "Ch23_LSA_1", "Ch23_NEX_0", "Ch23_NEX_1",
	// 0x2e0 (736 dec [368x2])
	"RVWAS_0", "RVWAS_1", "dAPF1_0", "dAPF1_1", "dAPF2_0", "dAPF2_1", "mLSAME_0", "mLSAME_1",
	"mRSAME_0", "mRSAME_1", "mLCOMB1_0", "mLCOMB1_1", "mRCOMB1_0", "mRCOMB1_1", "mLCOMB2_0", "mLCOMB2_1",
	"mRCOMB2_0", "mRCOMB2_1", "dLSAME_0", "dLSAME_1", "dRSAME_0", "dRSAME_1", "mLDIFF_0", "mLDIFF_1",
	"mRDIFF_0", "mRDIFF_1", "mLCOMB3_0", "mLCOMB3_1", "mRCOMB3_0", "mRCOMB3_1", "mLCOMB4_0", "mLCOMB4_1",
	"mRCOMB4_0", "mRCOMB4_1", "dLDIFF_0", "dLDIFF_1", "dRDIFF_0", "dRDIFF_1", "mLAPF1_0", "mLAPF1_1",
	"mRAPF1_0", "mRAPF1_1", "mLAPF2_0", "mLAPF2_1", "mRAPF2_0", "mRAPF2_1", "RVWAE_0", "RVWAE_1",
	
	// 0x340 (832 dec [416x2])
	"CON_0", "CON_1", "STAT"
	
	// 0x346-0x400 are unknown/not used??
};




// 20 registers in this section
static const char* SPUCore::RegisterNames2 [ 20 ] = {
	// core0 regs followed by core1 regs again
	// 0x760
	"MVOLL", "MVOLR", "EVOL_L", "EVOL_R", "AVOL_L", "AVOL_R", "BVOL_L", "BVOL_R",
	"MVOLX_L", "MVOLX_R", "vIIR", "vCOMB1", "vCOMB2", "vCOMB3", "vCOMB4", "vWALL",
	"vAPF1", "vAPF2", "vLIN", "vRIN"
	
	// 0x788
	//"MVOLL", "MVOLR", "EVOL_L", "EVOL_R", "AVOL_L", "AVOL_R", "BVOL_L", "BVOL_R",
	//"MVOLX_L", "MVOLX_R", "vIIR", "vCOMB1", "vCOMB2", "vCOMB3", "vCOMB4", "vWALL",
	//"vAPF1", "vAPF2", "vLIN", "vRIN"
	
	// 0x7b0
	//"UNK_A", "UNK_B", "UNK_C", "UNK_D", "UNK_E", "UNK_F", "UNK_G", "UNK_H",

};


// 5 registers in this section
static const char* SPUCore::RegisterNames3 [ 5 ] = {
	// 0x7c0
	"SPDIF_OUT", "SPDIF_IRQINFO", "SPDIF_MODE", "SPDIF_MEDIA", "SPDIF_PROTECT"
};



SPUCore::SPUCore ()
{
	cout << "Running SPUCore constructor...\n";
}

SPUCore::~SPUCore ()
{
	cout << "Running SPUCore destructor...\n";
}


void SPUCore::Start ()
{
	cout << "Running SPU::Start...\n";


#ifdef INLINE_DEBUG
	debug << "\r\nEntering SPU::Start";
#endif

	// can't use this for anything, since there are multiple spu cores
	//_SPUCore = this;
	
	Reset ();
	

#ifdef INLINE_DEBUG
	debug << "->Exiting SPU::Start";
#endif
}


void SPUCore::Reset ()
{
	int i;
	
	// zero object
	memset ( this, 0, sizeof( SPUCore ) );
	
	// pointers for quick access
	/*
	_vLOUT = & ( Regs [ ( vLOUT - SPU_X ) >> 1 ] );
	_vROUT = & ( Regs [ ( vROUT - SPU_X ) >> 1 ] );
	_mBASE = & ( Regs [ ( mBASE - SPU_X ) >> 1 ] );

	_dAPF1 = & ( Regs [ ( dAPF1 - SPU_X ) >> 1 ] );
	_dAPF2 = & ( Regs [ ( dAPF2 - SPU_X ) >> 1 ] );
	_vIIR = & ( Regs [ ( vIIR - SPU_X ) >> 1 ] );
	_vCOMB1 = & ( Regs [ ( vCOMB1 - SPU_X ) >> 1 ] );
	_vCOMB2 = & ( Regs [ ( vCOMB2 - SPU_X ) >> 1 ] );
	_vCOMB3 = & ( Regs [ ( vCOMB3 - SPU_X ) >> 1 ] );
	_vCOMB4 = & ( Regs [ ( vCOMB4 - SPU_X ) >> 1 ] );
	_vWALL = & ( Regs [ ( vWALL - SPU_X ) >> 1 ] );
	_vAPF1 = & ( Regs [ ( vAPF1 - SPU_X ) >> 1 ] );
	_vAPF2 = & ( Regs [ ( vAPF2 - SPU_X ) >> 1 ] );
	_mLSAME = & ( Regs [ ( mLSAME - SPU_X ) >> 1 ] );
	_mRSAME = & ( Regs [ ( mRSAME - SPU_X ) >> 1 ] );
	_mLCOMB1 = & ( Regs [ ( mLCOMB1 - SPU_X ) >> 1 ] );
	_mRCOMB1 = & ( Regs [ ( mRCOMB1 - SPU_X ) >> 1 ] );
	_mLCOMB2 = & ( Regs [ ( mLCOMB2 - SPU_X ) >> 1 ] );
	_mRCOMB2 = & ( Regs [ ( mRCOMB2 - SPU_X ) >> 1 ] );
	_dLSAME = & ( Regs [ ( dLSAME - SPU_X ) >> 1 ] );
	_dRSAME = & ( Regs [ ( dRSAME - SPU_X ) >> 1 ] );
	_mLDIFF = & ( Regs [ ( mLDIFF - SPU_X ) >> 1 ] );
	_mRDIFF = & ( Regs [ ( mRDIFF - SPU_X ) >> 1 ] );
	_mLCOMB3 = & ( Regs [ ( mLCOMB3 - SPU_X ) >> 1 ] );
	_mRCOMB3 = & ( Regs [ ( mRCOMB3 - SPU_X ) >> 1 ] );
	_mLCOMB4 = & ( Regs [ ( mLCOMB4 - SPU_X ) >> 1 ] );
	_mRCOMB4 = & ( Regs [ ( mRCOMB4 - SPU_X ) >> 1 ] );
	_dLDIFF = & ( Regs [ ( dLDIFF - SPU_X ) >> 1 ] );
	_dRDIFF = & ( Regs [ ( dRDIFF - SPU_X ) >> 1 ] );
	_mLAPF1 = & ( Regs [ ( mLAPF1 - SPU_X ) >> 1 ] );
	_mRAPF1 = & ( Regs [ ( mRAPF1 - SPU_X ) >> 1 ] );
	_mLAPF2 = & ( Regs [ ( mLAPF2 - SPU_X ) >> 1 ] );
	_mRAPF2 = & ( Regs [ ( mRAPF2 - SPU_X ) >> 1 ] );
	_vLIN = & ( Regs [ ( vLIN - SPU_X ) >> 1 ] );
	_vRIN = & ( Regs [ ( vRIN - SPU_X ) >> 1 ] );
	*/
	
	// enable audio filter by default
	AudioFilter_Enabled = true;

	
	// zero out registers
	//for ( i = 0; i < 256; i++ ) Regs [ i ] = 0;
	
	//VoiceOn_Bitmap = 0;
	
	s32 LowPassFilterCoefs [ 5 ];
	
	LowPassFilterCoefs [ 0 ] = gx [ 682 ];
	LowPassFilterCoefs [ 1 ] = gx [ 1365 ];
	LowPassFilterCoefs [ 2 ] = gx [ 2048 ];
	LowPassFilterCoefs [ 3 ] = gx [ 2730 ];
	LowPassFilterCoefs [ 4 ] = gx [ 3413 ];
	
	// reset the low pass filters
	LPF_L.Reset ();
	LPF_R.Reset ();
	LPF_L_Reverb.Reset ();
	LPF_R_Reverb.Reset ();
	
	// set lpf coefs
	LPF_L.SetFilter ( LowPassFilterCoefs );
	LPF_R.SetFilter ( LowPassFilterCoefs );
	LPF_L_Reverb.SetFilter ( LowPassFilterCoefs );
	LPF_R_Reverb.SetFilter ( LowPassFilterCoefs );
	
	
	// SPU is not needed to run on EVERY cycle
	//Set_NextEvent ( CPUCycles_PerSPUCycle );
}


u32 SPUCore::Read ( u32 Offset, u32 Mask )
{
#ifdef INLINE_DEBUG_READ
	debug << "\r\nSPU::Read; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << hex << " Mask=" << Mask << " Offset= " << Offset;
	debug << " Core#" << CoreNumber;
#endif

	//u32 lReg;
	u32 Output;
	int Channel;

	// make sure register is in the right range
	if ( Offset >= ( c_iNumberOfRegisters << 1 ) ) return 0;
	
#ifdef INLINE_DEBUG_READ
	// if register is within group1 or 2, then output the name of the register
	if ( Offset < 0x346 )
	{
		debug << "; " << RegisterNames1 [ Offset >> 1 ];
	}
	else if ( Offset >= 0x760 && Offset < 0x788 )
	{
		debug << "; " << RegisterNames2 [ ( Offset - 0x760 ) >> 1 ];
	}
#endif

	
	// Read SPU register value
	//lReg = Offset >> 1;
	
	// perform actions as needed
	switch ( Offset )
	{
		default:
		
			// check if reading from first group of channel registers
			/*
			if ( Offset < 0x180 )
			{
				Channel = ( Offset >> 4 );
				
				switch ( Offset & 0xf )
				{
					// ENV_X ( Channel )
					//case 0xa:
						// ***TODO*** get rid of VOL_ADSR_Value variable
						// for now, return envelope from variable
						//GET_REG16 ( ENVX_CH ( Channel ) ) = VOL_ADSR_Value [ Channel ];
						//break;
				}
			}
			*/
			
			break;
	}
	
	// return value;
	//Output = Regs16 [ lReg ];
	Output = GET_REG16 ( Offset );
	
#ifdef INLINE_DEBUG_READ
	debug << "; Output=" << Output;
#endif

	return Output;
}


void SPUCore::Write ( u32 Offset, u32 Data, u32 Mask )
{
#ifdef INLINE_DEBUG_WRITE
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << hex << " Mask=" << Mask << " Offset= " << Offset << " Data=" << Data;
	debug << " Core#" << CoreNumber;
#endif

	//u32 lReg;
	u16 ModeRate;
	int Channel;

	// make sure register is in the right range
	if ( Offset >= ( c_iNumberOfRegisters << 1 ) ) return 0;
	
#ifdef INLINE_DEBUG_WRITE
	// if register is within group1 or 2, then output the name of the register
	if ( Offset < 0x346 )
	{
		debug << "; " << RegisterNames1 [ Offset >> 1 ];
	}
	else if ( Offset >= 0x760 && Offset < 0x788 )
	{
		debug << "; " << RegisterNames2 [ ( Offset - 0x760 ) >> 1 ];
	}
#endif

	
	// Get the register number from offset
	//lReg = Offset >> 1;
	
	// perform actions as needed
	switch ( Offset )
	{
		case STAT:
			// this one should be read-only
			return;
			break;
			
		// reverb work address START
		case RVWAS_0:
		case RVWAS_1:
		
			//_SPU->Regs [ ( ( RVWA - SPU_X ) >> 1 ) & 0xff ] = (u16)Data;
			GET_REG16 ( Offset ) = (u16) Data;
			
			//_SPU->ReverbWork_Start = ( Data & 0xffff ) << 3;
			ReverbWork_Start = GET_REG32 ( RVWAS_0 );
			
			// mask against size of ram for now
			ReverbWork_Start &= ( c_iRam_Size >> 1 );
			
			// align to sound block boundary
			ReverbWork_Start &= ~7;
			
			//_SPU->ReverbWork_Size = c_iRam_Size - _SPU->ReverbWork_Start;
			ReverbWork_Size = ReverbWork_End - ReverbWork_Start;
			
			//_SPU->Reverb_BufferAddress = _SPU->ReverbWork_Start;
			Reverb_BufferAddress = ReverbWork_Start;
			
			// check if interrupt triggered by reverb work address
			//if ( ( _SPU->Reverb_BufferAddress == ( ( (u32) _SPU->Regs [ ( IRQA - SPU_X ) >> 1 ] ) << 3 ) ) && ( _SPU->Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] & 0x40 ) )
			if ( ( Reverb_BufferAddress == ( GET_REG32 ( IRQA_0 ) ) ) && ( GET_REG16 ( CTRL ) & 0x40 ) )
			{
				// we have reached irq address - trigger interrupt
				SetInterrupt ();
				
				// interrupt
				//_SPU->Regs [ ( 0x1f801dae - SPU_X ) >> 1 ] |= 0x40;
				GET_REG16 ( STAT ) |= 0x40;
			}
			
			break;
			
		// reverb work address END
		case RVWAE_0:
		
			//_SPU->Regs [ ( ( RVWA - SPU_X ) >> 1 ) & 0xff ] = (u16)Data;
			GET_REG16 ( Offset ) = (u16) Data;
			
			//_SPU->ReverbWork_Start = ( Data & 0xffff ) << 3;
			ReverbWork_End = GET_REG32 ( RVWAE_0 );
			
			// mask against size of ram for now
			ReverbWork_End &= ( c_iRam_Size >> 1 );
			
			//_SPU->ReverbWork_Size = c_iRam_Size - _SPU->ReverbWork_Start;
			ReverbWork_Size = ReverbWork_End - ReverbWork_Start;
			
			// make sure the size is positive
			if ( ReverbWork_Size < 0 ) ReverbWork_Size = 0;
			
			//_SPU->Reverb_BufferAddress = _SPU->ReverbWork_Start;
			//Reverb_BufferAddress = ReverbWork_Start;
			break;
			
		case RVWAE_1:
			// the lower part of reverb end address is always zero
			Data = 0;
			break;
			
		////////////////////////////////////////
		// Sound Buffer Address
		case SBA_0:
		case SBA_1:
		
			///////////////////////////////////
			// set next sound buffer address
			//_SPU->Regs [ ( ( Address - SPU_X ) >> 1 ) & 0xff ] = (u16)Data;
			GET_REG16 ( Offset ) = (u16) Data;
			
			//NextSoundBufferAddress = ( Data << 3 ) & c_iRam_Mask;
			NextSoundBufferAddress = GET_REG32 ( SBA_0 ) & ( c_iRam_Mask >> 1 );
			
			// align - this needs to be aligned to a sound block boundary
			NextSoundBufferAddress &= ~7;

			break;
				
		//////////////////////////////////////////
		// Data forwarding register
		case DATA:
		
			// buffer can be written into at any time
			if ( BufferIndex < 32 )
			{
				Buffer [ BufferIndex++ ] = (u16) Data;
			}
			
			break;
				
		case KON_0:
			/////////////////////////////////////////////
			// Key On 0-15
			
			// when keyed on set channel ADSR to attack mode
			for ( Channel = 0; Channel < 16; Channel++ )
			{
				if ( ( 1 << Channel ) & Data )
				{
					Start_SampleDecoding ( Channel );
				}
			}
			
			// clear the end of sample passed flag for keyed on channels
			GET_REG16 ( CON_0 ) &= ~Data;
			break;
			
		case KON_1:
			/////////////////////////////////////////////
			// Key On 16-23
			
			// when keyed on set channel ADSR to attack mode
			for ( Channel = 16; Channel < 24; Channel++ )
			{
				if ( ( 1 << ( Channel - 16 ) ) & Data )
				{
					Start_SampleDecoding ( Channel );
				}
			}
			
			// clear the end of sample passed flag for keyed on channels
			GET_REG16 ( CON_1 ) &= ~Data;
			break;
			
		case KOFF_0:
			/////////////////////////////////////////////
			// Key off 0-15
			
			// on key off we need to change ADSR mode to release mode
			for ( Channel = 0; Channel < 16; Channel++ )
			{
				if ( ( 1 << Channel ) & Data )
				{
					// put channel in adsr release phase unconditionally
					ADSR_Status [ Channel ] = ADSR_RELEASE;
					
					// *** TODO *** remove VOL_ADSR_Value variable
					// start envelope for release mode
					//ModeRate = _SPU->Regs [ ( ADSR_1 >> 1 ) + ( Channel << 3 ) ];
					ModeRate = GET_REG16 ( ADSR1_CH ( Channel ) );
					
					//Start_VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ModeRate & 0x1f ) << ( 2 ), ( ( ( ModeRate >> 5 ) & 1 ) << 1 ) | 0x1 );
					Start_VolumeEnvelope ( GET_REG16s ( ENVX_CH( Channel ) ), Cycles [ Channel ], ( ModeRate & 0x1f ) << ( 2 ), ( ( ( ModeRate >> 5 ) & 1 ) << 1 ) | 0x1 );
					
					// loop address not manually specified
					// can still change loop address after sound starts
					LSA_Manual_Bitmap &= ~( 1 << Channel );
				}
			}
			
			break;
			
		case KOFF_1:
			/////////////////////////////////////////////
			// Key off 16-23
			
			// on key off we need to change ADSR mode to release mode
			for ( Channel = 16; Channel < 24; Channel++ )
			{
				if ( ( 1 << ( Channel - 16 ) ) & Data )
				{
					// put channel in adsr release phase unconditionally
					ADSR_Status [ Channel ] = ADSR_RELEASE;
					
					// *** TODO *** remove VOL_ADSR_Value variable
					// start envelope for release mode
					//ModeRate = _SPU->Regs [ ( ADSR_1 >> 1 ) + ( Channel << 3 ) ];
					ModeRate = GET_REG16 ( ADSR1_CH ( Channel ) );
					
					//Start_VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ModeRate & 0x1f ) << ( 2 ), ( ( ( ModeRate >> 5 ) & 1 ) << 1 ) | 0x1 );
					Start_VolumeEnvelope ( GET_REG16s ( ENVX_CH( Channel ) ), Cycles [ Channel ], ( ModeRate & 0x1f ) << ( 2 ), ( ( ( ModeRate >> 5 ) & 1 ) << 1 ) | 0x1 );
					
					// loop address not manually specified
					// can still change loop address after sound starts
					LSA_Manual_Bitmap &= ~( 1 << Channel );
				}
			}
			
			break;

			
		case CTRL:
			
			// bits 0-5 of CTRL should be applied to bits 0-5 of STAT (***TODO*** supposed to be delayed application)
			// source: Martin Korth PSX Specification
			GET_REG16 ( STAT ) &= ~0x3f;
			GET_REG16 ( STAT ) |= ( Data & 0x3f );
			
			// bit 7 of STAT appears to be bit 5 of CTRL (***TODO*** supposed to be delayed application)
			// source: Martin Korth PSX Specification
			//GET_REG16 ( STAT ) &= ~( 1 << 7 );
			//GET_REG16 ( STAT ) |= ( ( Data << 2 ) & ( 1 << 7 ) );
			// copy bit 5 of ctrl to bit 7 of stat
			//switch ( ( _SPU->Regs [ ( CTRL - SPU_X ) >> 1 ] >> 5 ) & 0x3 )
			switch ( ( Data >> 4 ) & 0x3 )
			{
				// no reads or writes (stop)
				case 0:
				
				// manual write
				case 1:	
				
					//_SPU->Regs [ ( STAT - SPU_X ) >> 1 ] = ( _SPU->Regs [ ( STAT - SPU_X ) >> 1 ] & ~0x0380 ) | ( 0 << 7 );
					GET_REG16 ( STAT ) &= ~0x0380;
					break;
					
				case 2:
					// dma write
					//_SPU->Regs [ ( STAT - SPU_X ) >> 1 ] = ( _SPU->Regs [ ( STAT - SPU_X ) >> 1 ] & ~0x0380 ) | ( 0x3 << 7 );
					GET_REG16 ( STAT ) = ( GET_REG16 ( STAT ) & ~0x0380 ) | ( 0x3 << 7 );
					break;
					
				case 3:
					// dma read
					//_SPU->Regs [ ( STAT - SPU_X ) >> 1 ] = ( _SPU->Regs [ ( STAT - SPU_X ) >> 1 ] & ~0x0380 ) | ( 0x5 << 7 );
					GET_REG16 ( STAT ) = ( GET_REG16 ( STAT ) & ~0x0380 ) | ( 0x5 << 7 );
					break;
			}
			
			// check if disabling/acknowledging interrupt
			if ( ! ( Data & 0x40 ) )
			{
				// clear interrupt
				GET_REG16 ( STAT ) &= ~0x40;
			}
			
			///////////////////////////////////////////////////////////////////////////
			// if DMA field was written as 01 then write SPU buffer into sound RAM
			if ( ( Data & 0x30 ) == 0x10 )
			{
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_CTRL
	debug << "; MANUAL WRITE";
#endif

				///////////////////////////////////////////////////////
				// write SPU buffer into sound ram
				for ( int i = 0; i < BufferIndex; i++ )
				{
					//RAM [ ( ( NextSoundBufferAddress + ( i << 1 ) ) & c_iRam_Mask ) >> 1 ] = Buffer [ i ];
					RAM [ ( NextSoundBufferAddress + i ) & ( c_iRam_Mask >> 1 ) ] = Buffer [ i ];
				}
				
				//////////////////////////////////////////////////////////
				// update next sound buffer address
				NextSoundBufferAddress += ( BufferIndex << 1 );
				
				// align to sound block boundary
				NextSoundBufferAddress &= ~7;
				
				//////////////////////////////////////////////////
				// reset buffer index
				BufferIndex = 0;
				
				////////////////////////////////////////////////////////////
				// save back into the sound buffer address register
				// sound buffer address register does not change
				//_SPU->Regs [ ( ( REG_SoundBufferAddress - SPU_X ) >> 1 ) & 0xff ] = (u16) (_SPU->NextSoundBufferAddress >> 3);
			}
			
			break;
			
			
		case CON_0:
		case CON_1:
			// writes should clear register to zero
			Data = 0;
			break;
			
		default:
			break;
	}
	
	// set value
	//Regs16 [ lReg ] = Data;
	GET_REG16 ( Offset ) = Data;
}



void SPUCore::UpdatePitch ( int Channel, u32 Pitch, u32 Reg_PMON, s32 PreviousSample )
{
	s64 Pitch_Step;
	s64 Pitch_Factor;
	
	Pitch_Step = Pitch;
	
	if ( Reg_PMON & ( 1 << Channel ) & ~1 )
	{
		// pitch modulation is enabled for channel //
		
		Pitch_Factor = ((s64)adpcm_decoder::clamp ( PreviousSample ));
		Pitch_Factor += 0x8000;
		Pitch_Step = ( Pitch_Step << 48 ) >> 48;
		Pitch_Step = ( Pitch_Step * Pitch_Factor ) >> 15;
		Pitch_Step &= 0xffff;
	}
	
	if ( Pitch_Step > 0x3fff ) Pitch_Step = 0x3fff;
	
	//Pitch_Counter += Pitch_Step;
	CurrentSample_Offset [ Channel ] += ( Pitch_Step << 20 );
	CurrentSample_Read [ Channel ] += ( Pitch_Step << 20 );
}

void SPU2::Run ()
{
	if ( NextEvent_Cycle != *_DebugCycleCount ) return;

#ifdef INLINE_DEBUG_RUN2
	debug << "\r\nSPU2::Run Cycle#" << dec << *_DebugCycleCount;
#endif
	
	SPU0.Run ();
	SPU1.Run ();
	
	// update number of spu cycles ran
	CycleCount++;
	
	//NextEvent_Cycle = *_DebugCycleCount + CPUCycles_PerSPUCycle;
	Set_NextEvent ( CPUCycles_PerSPUCycle );
}

void SPUCore::Run ()
{
	int Channel;
	
	s64 Sample, PreviousSample, ChSampleL, ChSampleR, SampleL, SampleR, CD_SampleL, CD_SampleR, ReverbSampleL, ReverbSampleR;
	s64 FOutputL, FOutputR, ROutputL, ROutputR;
	
	u32 ChannelOn, ChannelNoise, PitchMod, ReverbOn;
	
	u64 Temp;
	u32 ModeRate;
	
	
	//if ( NextEvent_Cycle != *_DebugCycleCount ) return;
	
	// update number of spu cycles ran
	// I'll do this per core for now
	CycleCount++;
	
	//NextEvent_Cycle = *_DebugCycleCount + CPUCycles_PerSPUCycle;
	//Set_NextEvent ( CPUCycles_PerSPUCycle );
	
//#ifdef INLINE_DEBUG_RUN
//	debug << "\r\nSPU::Run";
//#endif

	// initialize current sample for left and right
	SampleL = 0;
	SampleR = 0;
	ReverbSampleL = 0;
	ReverbSampleR = 0;

	/*
	// check if SPU is on
	if ( ! ( Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] >> 15 ) )
	{
		// SPU is not on

#ifdef INLINE_DEBUG_RUN
	debug << "; Off" << "; Offset = " << ( 0x1f801daa - SPU_X ) << "; SPU_CTRL = " << Regs [ ( 0x1f801daa - SPU_X ) >> 1 ];
#endif

		return 0;
	}
	*/
	
	// SPU is on
	
	// *** TODO *** run SPU and output 1 sample LR
	
	/////////////////////////////////////////////////////////////////////
	// get what channels are enabled
	//ChannelOn = Regs16 [ CON_0 >> 1 ];
	//ChannelOn |= ( (u32) ( Regs16 [ CON_1 >> 1 ] ) ) << 16;
	ChannelOn = GET_REG32 ( CON_0 );
	
	// get what channels are set to noise
	//ChannelNoise = Regs16 [ NON_0 >> 1 ];
	//ChannelNoise |= ( (u32) ( Regs16 [ NON_1 >> 1 ] ) ) << 16;
	ChannelNoise = GET_REG32 ( NON_0 );
	
	// get what channels are using frequency modulation
	//PitchMod = Regs16 [ PMON_0 >> 1 ];
	//PitchMod |= ( (u32) ( Regs16 [ PMON_1 >> 1 ] ) ) << 16;
	PitchMod = GET_REG32 ( PMON_0 );
	
	// get what channels have reverb on
	// SKIP REVERB FOR NOW
	//ReverbOn = Regs16 [ RON_0 >> 1 ];
	//ReverbOn |= ( (u32) ( Regs16 [ RON_1 >> 1 ] ) ) << 16;
	//ReverbOn = GET_REG32 ( RON_0 );

//#ifdef INLINE_DEBUG_RUN
//	debug << "; ChannelOn=" << ChannelOn << " KeyOn=" << KeyOn;
//#endif

	// if spu is enabled, run noise generator
	//if ( Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] >> 15 )
	if ( GET_REG16 ( CTRL ) >> 15 )
	{
		RunNoiseGenerator ();

		//if ( Regs [ ( MVOL_L - SPU_X ) >> 1 ] >> 15 )
		if ( GET_REG16 ( MVOL_L ) >> 15 )
		{
			// ***TODO*** get rid of MVOL_L_Value variable
			
			//MVOL_L_Value = (s64) ( (s16) Regs [ ( CMVOL_L - SPU_X ) >> 1 ] );
			MVOL_L_Value = (s64) ( (s16) GET_REG16 ( CMVOL_L ) );
			
			//VolumeEnvelope ( MVOL_L_Value, MVOL_L_Cycles, GET_REG16 ( MVOL_L ) & 0x7f, ( GET_REG16 ( MVOL_L ) >> 13 ) & 0x3 );
			VolumeEnvelope ( GET_REG16s ( CMVOL_L ), MVOL_L_Cycles, GET_REG16 ( MVOL_L ) & 0x7f, ( GET_REG16 ( MVOL_L ) >> 13 ) & 0x3 );
			
			//GET_REG16 ( CMVOL_L ) = MVOL_L_Value;
		}
		else
		{
			// just set the current master volume L
			//Regs [ ( CMVOL_L - SPU_X ) >> 1 ] = Regs [ ( MVOL_L - SPU_X ) >> 1 ] << 1;
			GET_REG16 ( CMVOL_L ) = GET_REG16 ( MVOL_L ) << 1;
		}
	
		//if ( Regs [ ( MVOL_R - SPU_X ) >> 1 ] >> 15 )
		if ( GET_REG16 ( MVOL_R ) >> 15 )
		{
			// ***TODO*** get rid of MVOL_R_Value variable
			
			//MVOL_R_Value = (s64) ( (s16) Regs [ ( CMVOL_R - SPU_X ) >> 1 ] );
			MVOL_R_Value = (s64) ( (s16) GET_REG16 ( CMVOL_R ) );
			
			//VolumeEnvelope ( MVOL_R_Value, MVOL_R_Cycles, Regs [ ( MVOL_R - SPU_X ) >> 1 ] & 0x7f, ( Regs [ ( MVOL_R - SPU_X ) >> 1 ] >> 13 ) & 0x3 );
			//VolumeEnvelope ( MVOL_R_Value, MVOL_R_Cycles, GET_REG16 ( MVOL_R ) & 0x7f, ( GET_REG16 ( MVOL_R ) >> 13 ) & 0x3 );
			VolumeEnvelope ( GET_REG16s ( CMVOL_R ), MVOL_R_Cycles, GET_REG16 ( MVOL_R ) & 0x7f, ( GET_REG16 ( MVOL_R ) >> 13 ) & 0x3 );
			
			//GET_REG16 ( CMVOL_R ) = MVOL_R_Value;
		}
		else
		{
			// just set the current master volume R
			//Regs [ ( CMVOL_R - SPU_X ) >> 1 ] = Regs [ ( MVOL_R - SPU_X ) >> 1 ] << 1;
			GET_REG16 ( CMVOL_R ) = GET_REG16 ( MVOL_R ) << 1;
		}
		
		// also process audio if SPU is on
		////////////////////////////
		// loop through channels
		for ( Channel = 0; Channel < 24; Channel++ )
		{
	
		// check if SPU is on
		//if ( Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] >> 15 )

			//if ( Regs [ ( VOL_L >> 1 ) + ( Channel << 3 ) ] >> 15 )
			if ( GET_REG16 ( VOLL_CH ( Channel ) ) >> 15 )
			{
				// *** TODO *** VOL_L_Value variable could be removed
				
				// set current volume left
				//VOL_L_Value [ Channel ] = (s64) ( (s16) Regs [ ( CVOL_L_START - SPU_X + ( Channel << 1 ) ) >> 1 ] );
				VOL_L_Value [ Channel ] = (s64) ( (s16) GET_REG16 ( CVOLL_CH ( Channel ) ) );
				
				// perform envelope
				//VolumeEnvelope ( VOL_L_Value [ Channel ], VOL_L_Cycles [ Channel ], Regs [ ( VOL_L >> 1 ) + ( Channel << 3 ) ] & 0x7f, ( Regs [ ( VOL_L >> 1 ) + ( Channel << 3 ) ] >> 13 ) & 0x3 );
				VolumeEnvelope ( GET_REG16s ( CVOLL_CH ( Channel ) ), VOL_L_Cycles [ Channel ], GET_REG16 ( VOLL_CH ( Channel ) ) & 0x7f, ( GET_REG16 ( VOLL_CH ( Channel ) ) >> 13 ) & 0x3 );
				
				// store the new current volume left
				//Regs [ ( CVOL_L_START - SPU_X + ( Channel << 1 ) ) >> 1 ] = VOL_L_Value [ Channel ];
				//GET_REG16 ( CVOLL_CH ( Channel ) ) = VOL_L_Value [ Channel ];
			}
			else
			{
				// just set the current volume L
				//Regs [ ( CVOL_L_START - SPU_X + ( Channel << 1 ) ) >> 1 ] = Regs [ ( VOL_L >> 1 ) + ( Channel << 3 ) ] << 1;
				GET_REG16 ( CVOLL_CH ( Channel ) ) = GET_REG16 ( VOLL_CH ( Channel ) ) << 1;
			}

			//if ( Regs [ ( VOL_R >> 1 ) + ( Channel << 3 ) ] >> 15 )
			if ( GET_REG16 ( VOLR_CH ( Channel ) ) >> 15 )
			{
				// set current volume right
				//VOL_R_Value [ Channel ] = (s64) ( (s16) Regs [ ( CVOL_R_START - SPU_X + ( Channel << 1 ) ) >> 1 ] );
				VOL_R_Value [ Channel ] = (s64) ( (s16) GET_REG16 ( CVOLR_CH ( Channel ) ) );
				
				//VolumeEnvelope ( VOL_R_Value [ Channel ], VOL_R_Cycles [ Channel ], Regs [ ( VOL_R >> 1 ) + ( Channel << 3 ) ] & 0x7f, ( Regs [ ( VOL_R >> 1 ) + ( Channel << 3 ) ] >> 13 ) & 0x3 );
				VolumeEnvelope ( GET_REG16s ( CVOLR_CH ( Channel ) ), VOL_R_Cycles [ Channel ], GET_REG16 ( VOLR_CH ( Channel ) ) & 0x7f, ( GET_REG16 ( VOLR_CH ( Channel ) ) >> 13 ) & 0x3 );
				
				// store the new current volume right
				//Regs [ ( CVOL_R_START - SPU_X + ( Channel << 1 ) ) >> 1 ] = VOL_R_Value [ Channel ];
				//GET_REG16 ( CVOLR_CH ( Channel ) ) = VOL_R_Value [ Channel ];
			}
			else
			{
				// just set the current volume R
				//Regs [ ( CVOL_R_START - SPU_X + ( Channel << 1 ) ) >> 1 ] = Regs [ ( VOL_R >> 1 ) + ( Channel << 3 ) ] << 1;
				GET_REG16 ( CVOLR_CH ( Channel ) ) = GET_REG16 ( VOLR_CH ( Channel ) ) << 1;
			}
			
			
			/////////////////////////////////////////////////////////////////////
			// update ADSR envelope
			
			// check adsr status
			switch ( ADSR_Status [ Channel ] )
			{
				case ADSR_MUTE:
				
					VOL_ADSR_Value [ Channel ] = 0;
					
					//Regs [ ( ( Channel << 4 ) + ENV_X ) >> 1 ] = 0;
					GET_REG16 ( ENVX_CH ( Channel ) ) = 0;
				
					break;
					
				case ADSR_ATTACK:
#ifdef INLINE_DEBUG_RUN_ATTACK
	debug << "\r\nChannel#" << Channel;
#endif
			
#ifdef INLINE_DEBUG_RUN_ATTACK
	//debug << "; Attack; Rate=" << (((double)VOL_ATTACK_Constant [ Channel ])/64536) << "; Rate75=" << (((double)VOL_ATTACK_Constant75 [ Channel ])/64536);
	debug << "; Attack";
#endif

					///////////////////////////////////////////
					// ADSR - Attack Mode
					
					////////////////////////////////////////////////////
					// linear or psx pseudo exponential increase
					
#ifdef INLINE_DEBUG_RUN_ATTACK
	debug << "; (before) VOL_ADSR_Value=" << hex << GET_REG16 ( ENVX_CH ( Channel ) ) << "; Cycles=" << dec << Cycles [ Channel ];
#endif

					//ModeRate = Regs [ ( ADSR_0 >> 1 ) + ( Channel << 3 ) ];
					ModeRate = GET_REG16 ( ADSR0_CH ( Channel ) );
					
					//VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ModeRate >> 8 ) & 0x7f, ( ModeRate >> 15 ) << 1 );
					VolumeEnvelope ( GET_REG16s ( ENVX_CH ( Channel ) ), Cycles [ Channel ], ( ModeRate >> 8 ) & 0x7f, ( ModeRate >> 15 ) << 1 );
					
#ifdef INLINE_DEBUG_RUN_ATTACK
	debug << "; (after) VOL_ADSR_Value=" << hex << GET_REG16 ( ENVX_CH ( Channel ) ) << "; Cycles=" << dec << Cycles [ Channel ] << "; Value=" << hex << ( ( ModeRate >> 8 ) & 0x7f ) << "; flags=" << ( ( ModeRate >> 15 ) << 1 );
#endif

					
					// saturate and switch to decay mode if volume goes above 32767
					//if ( VOL_ADSR_Value [ Channel ] >= 32767 )
					if ( ( (u32) GET_REG16 ( ENVX_CH ( Channel ) ) ) >= 32767 )
					{
#ifdef INLINE_DEBUG_RUN_ATTACK
	debug << "; DECAY_NEXT";
#endif

						//VOL_ADSR_Value [ Channel ] = 32767;
						GET_REG16 ( ENVX_CH ( Channel ) ) = 32767;
						
						ADSR_Status [ Channel ] = ADSR_DECAY;
						
						// start envelope for decay mode
						//ModeRate = Regs [ ( ADSR_0 >> 1 ) + ( Channel << 3 ) ];
						ModeRate = GET_REG16 ( ADSR0_CH ( Channel ) );
						
						//Start_VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ( ModeRate >> 4 ) & 0xf ) << ( 2 ), 0x3 );
						Start_VolumeEnvelope ( GET_REG16s ( ENVX_CH ( Channel ) ), Cycles [ Channel ], ( ( ModeRate >> 4 ) & 0xf ) << ( 2 ), 0x3 );
					}
					
					break;
					
				case ADSR_DECAY:
#ifdef INLINE_DEBUG_RUN_DECAY
	debug << "\r\nChannel#" << Channel;
#endif
			
#ifdef INLINE_DEBUG_RUN_DECAY
	//debug << "; Decay; Rate=" << (((double)VOL_DECAY_Constant [ Channel ])/(1<<30));
	debug << "; Decay";
#endif

					////////////////////////////////////////////
					// ADSR - Decay Mode
				
					////////////////////////////////////////////////
					// Exponential decrease


#ifdef INLINE_DEBUG_RUN_DECAY
	debug << "; (before) VOL_ADSR_Value=" << hex << GET_REG16 ( ENVX_CH ( Channel ) ) << "; Cycles=" << dec << Cycles [ Channel ];
#endif

					//ModeRate = Regs [ ( ADSR_0 >> 1 ) + ( Channel << 3 ) ];
					ModeRate = GET_REG16 ( ADSR0_CH ( Channel ) );
					
					//VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ( ModeRate >> 4 ) & 0xf ) << ( 2 ), 0x3 );
					VolumeEnvelope ( GET_REG16s ( ENVX_CH ( Channel ) ), Cycles [ Channel ], ( ( ModeRate >> 4 ) & 0xf ) << ( 2 ), 0x3 );
					
#ifdef INLINE_DEBUG_RUN_DECAY
	debug << "; (after) VOL_ADSR_Value=" << hex << GET_REG16 ( ENVX_CH ( Channel ) ) << "; Cycles=" << dec << Cycles [ Channel ] << "; Value=" << hex << ( ( ModeRate >> 8 ) & 0x7f ) << "; flags=" << ( ( ModeRate >> 15 ) << 1 );
#endif

					// saturate if volume goes below zero
					// *** TODO *** ADSR volume probably can't go below zero in decay mode since it is always an exponential decrease
					//if ( VOL_ADSR_Value [ Channel ] < 0 )
					if ( ( (s16) GET_REG16 ( ENVX_CH ( Channel ) ) ) < 0 )
					{
						//VOL_ADSR_Value [ Channel ] = 0;
						GET_REG16 ( ENVX_CH ( Channel ) ) = 0;
					}
					
					// switch to sustain mode if we reach sustain level
					//if ( VOL_ADSR_Value [ Channel ] <= VOL_SUSTAIN_Level [ Channel ] )
					if ( ( (u32) GET_REG16 ( ENVX_CH ( Channel ) ) ) <= VOL_SUSTAIN_Level [ Channel ] )
					{
#ifdef INLINE_DEBUG_RUN_DECAY
	debug << "; SUSTAIN_NEXT";
#endif

						//VOL_ADSR_Value [ Channel ] = VOL_SUSTAIN_Level [ Channel ];
						GET_REG16 ( ENVX_CH ( Channel ) ) = VOL_SUSTAIN_Level [ Channel ];
						
						ADSR_Status [ Channel ] = ADSR_SUSTAIN;
						
						// start envelope for sustain mode
						//ModeRate = Regs [ ( ADSR_1 >> 1 ) + ( Channel << 3 ) ];
						ModeRate = GET_REG16 ( ADSR1_CH ( Channel ) );
						
						//Start_VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ModeRate >> 6 ) & 0x7f, ModeRate >> 14 );
						Start_VolumeEnvelope ( GET_REG16s ( ENVX_CH ( Channel ) ), Cycles [ Channel ], ( ModeRate >> 6 ) & 0x7f, ModeRate >> 14 );
					}
					
					break;
					
				case ADSR_SUSTAIN:
#ifdef INLINE_DEBUG_RUN_SUSTAIN
	debug << "\r\nChannel#" << Channel;
#endif
			
#ifdef INLINE_DEBUG_RUN_SUSTAIN
	//debug << "; Sustain; Rate=" << (((double)VOL_SUSTAIN_Constant [ Channel ])/64536) << "; Rate75=" << (((double)VOL_SUSTAIN_Constant75 [ Channel ])/64536);
	debug << "; Sustain";
#endif

					/////////////////////////////////////////////
					// ADSR - Sustain Mode
					
#ifdef INLINE_DEBUG_RUN_SUSTAIN
	debug << "; (before) VOL_ADSR_Value=" << hex << GET_REG16 ( ENVX_CH ( Channel ) ) << "; Cycles=" << dec << Cycles [ Channel ];
#endif
					
					//ModeRate = Regs [ ( ADSR_1 >> 1 ) + ( Channel << 3 ) ];
					ModeRate = GET_REG16 ( ADSR1_CH ( Channel ) );
					
					//VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ModeRate >> 6 ) & 0x7f, ModeRate >> 14 );
					VolumeEnvelope ( GET_REG16s ( ENVX_CH ( Channel ) ), Cycles [ Channel ], ( ModeRate >> 6 ) & 0x7f, ModeRate >> 14 );

#ifdef INLINE_DEBUG_RUN_SUSTAIN
	debug << "; (after) VOL_ADSR_Value=" << hex << GET_REG16 ( ENVX_CH ( Channel ) ) << "; Cycles=" << dec << Cycles [ Channel ] << "; Value=" << hex << ( ( ModeRate >> 8 ) & 0x7f ) << "; flags=" << ( ( ModeRate >> 15 ) << 1 );
#endif

					
					// saturate if volume goes above 32767
					//if ( VOL_ADSR_Value [ Channel ] > 32767 )
					if ( GET_REG16 ( ENVX_CH ( Channel ) ) > 32767 )
					{
						//VOL_ADSR_Value [ Channel ] = 32767;
						GET_REG16 ( ENVX_CH ( Channel ) ) = 32767;
					}
					
					// or below zero
					//if ( VOL_ADSR_Value [ Channel ] < 0 )
					if ( ( (s16) GET_REG16 ( ENVX_CH ( Channel ) ) ) < 0 )
					{
						//VOL_ADSR_Value [ Channel ] = 0;
						GET_REG16 ( ENVX_CH ( Channel ) ) = 0;
					}
					
					// we do not switch to release mode until key off signal is given
					
					break;
					
				case ADSR_RELEASE:
#ifdef INLINE_DEBUG_RUN_RELEASE
	debug << "\r\nChannel#" << Channel;
#endif
			
#ifdef INLINE_DEBUG_RUN_RELEASE
	//debug << "; Release; Rate=" << (((double)VOL_RELEASE_Constant [ Channel ])/(1<<30));
	debug << "; Release";
#endif

					///////////////////////////////////////////////
					// ADSR - Release Mode
				
#ifdef INLINE_DEBUG_RUN_RELEASE
	debug << "; (before) VOL_ADSR_Value=" << hex << GET_REG16 ( ENVX_CH ( Channel ) ) << "; Cycles=" << dec << Cycles [ Channel ];
#endif

					// when at or below zero we turn note off completely and set adsr volume to zero
					if ( ( (s16) GET_REG16 ( ENVX_CH ( Channel ) ) ) <= 0 )
					{
						// ADSR volume is below zero in RELEASE mode //
					
						// saturate to zero
						//VOL_ADSR_Value [ Channel ] = 0;
						GET_REG16 ( ENVX_CH ( Channel ) ) = 0;
						
						// the channel on bit is not really for whether the channel is on or off
						//ChannelOn = ChannelOn & ~( 1 << Channel );
					}
					else
					{
						// RELEASE mode //
						
						// *** note *** it is possible for ADSR volume to go negative for 1T in linear decrement mode //
						
						//ModeRate = Regs [ ( ADSR_1 >> 1 ) + ( Channel << 3 ) ];
						ModeRate = GET_REG16 ( ADSR1_CH ( Channel ) );
						
						//VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ModeRate & 0x1f ) << ( 2 ), ( ( ( ModeRate >> 5 ) & 1 ) << 1 ) | 0x1 );
						VolumeEnvelope ( GET_REG16s ( ENVX_CH ( Channel ) ), Cycles [ Channel ], ( ModeRate & 0x1f ) << ( 2 ), ( ( ( ModeRate >> 5 ) & 1 ) << 1 ) | 0x1 );
					}
					
					// *** testing ***
					//if ( VOL_ADSR_Value [ Channel ] < 0 )
					if ( ( (s16) GET_REG16 ( ENVX_CH ( Channel ) ) ) < 0 )
					{
						// saturate to zero
						//VOL_ADSR_Value [ Channel ] = 0;
						GET_REG16 ( ENVX_CH ( Channel ) ) = 0;
					}


#ifdef INLINE_DEBUG_RUN_RELEASE
	debug << "; (after) VOL_ADSR_Value=" << hex << GET_REG16 ( ENVX_CH ( Channel ) ) << "; Cycles=" << dec << Cycles [ Channel ] << "; Value=" << hex << ( ( ModeRate >> 8 ) & 0x7f ) << "; flags=" << ( ( ModeRate >> 15 ) << 1 );
#endif

					
						
					break;
			}
			
			/////////////////////////////////////////////////////////////////////////////
			// update ADSR Envelope Volume
			//Regs [ ( ( Channel << 4 ) + ENV_X ) >> 1 ] = VOL_ADSR_Value [ Channel ];
			GET_REG16 ( ENVX_CH ( Channel ) ) = VOL_ADSR_Value [ Channel ];
			
			//////////////////////////////////////////////////////////////
			// load sample
			
			// check if channel is set to noise
			if ( ChannelNoise & ( 1 << Channel ) )
			{
				// channel is set to noise //
				
				Sample = NoiseLevel;
			}
			else
			{
				// channel is not set to noise //
				
				u32 SampleNumber = CurrentSample_Read [ Channel ] >> 32;
				
				//Sample = DecodedBlocks [ ( Channel * 28 ) + ( CurrentSample_Offset [ Channel ] >> 32 ) ];
				Sample = DecodedBlocks [ ( Channel << 5 ) + ( SampleNumber & 0x1f ) ];
				
				///////////////////////////////////////////
				// apply sample interpolation
				
				Sample = Calc_sample_gx ( CurrentSample_Read [ Channel ] >> 16, Sample, DecodedBlocks [ ( Channel << 5 ) + ( ( SampleNumber - 1 ) & 0x1f ) ],
				DecodedBlocks [ ( Channel << 5 ) + ( ( SampleNumber - 2 ) & 0x1f ) ], DecodedBlocks [ ( Channel << 5 ) + ( ( SampleNumber - 3 ) & 0x1f ) ] );
				
				
				////////////////////////////////////
				// apply envelope volume
				// this does not apply when set to noise
				//Sample = ( Sample * ( (s64) ( (s16) Regs [ ( ( Channel << 4 ) + ENV_X ) >> 1 ] ) ) ) >> c_iVolumeShift;
				Sample = ( Sample * ( (s64) ( (s16) GET_REG16 ( ENVX_CH(Channel) ) ) ) ) >> c_iVolumeShift;
				
				//UpdatePitch ( Channel, Regs [ ( ( Channel << 4 ) + PITCH ) >> 1 ], PitchMod, PreviousSample );
				UpdatePitch ( Channel, GET_REG16 ( PITCH_CH(Channel) ), PitchMod, PreviousSample );
				
			}
			
			// store previous sample in case next channel uses frequency modulation
			PreviousSample = Sample;
			
			// save current sample for debugging
			Debug_CurrentRawSample [ Channel ] = Sample;

			// copy samples for voice1 and voice3 into buffer //
			
			if ( Channel == 1 )
			{
				RAM [ ( 0x0800 + DecodeBufferOffset ) >> 1 ] = Sample;
			}
			
			if ( Channel == 3 )
			{
				RAM [ ( 0x0c00 + DecodeBufferOffset ) >> 1 ] = Sample;
			}
			
			// check for interrupts
			//if ( ( ( ( (u32) Regs [ ( IRQA - SPU_X ) >> 1 ] ) << 3 ) == ( DecodeBufferOffset + 0x800 ) ) && ( Regs [ ( CTRL - SPU_X ) >> 1 ] & 0x40 ) )
			if ( ( ( GET_REG32 ( IRQA_0 ) << 3 ) == ( DecodeBufferOffset + 0x800 ) ) && ( GET_REG16 ( CTRL ) & 0x40 ) )
			{
				// we have reached irq address - trigger interrupt
				SetInterrupt ();
				
				// interrupt
				//Regs [ ( STAT - SPU_X ) >> 1 ] |= 0x40;
				GET_REG16 ( STAT ) |= 0x40;
			}
			
			if ( ( ( GET_REG32 ( IRQA_0 ) << 3 ) == ( DecodeBufferOffset + 0xc00 ) ) && ( GET_REG16 ( CTRL ) & 0x40 ) )
			{
				// we have reached irq address - trigger interrupt
				SetInterrupt ();
				
				// interrupt
				GET_REG16 ( STAT ) |= 0x40;
			}
			
			//////////////////////////////////////////////////////////////////
			// apply volume processing
			
			
			// apply current channel volume
			//ChSampleL = ( Sample * ((s64) ((s16)Regs [ ( CVOL_L_START - SPU_X + ( Channel << 1 ) ) >> 1 ]) ) ) >> c_iVolumeShift;
			//ChSampleR = ( Sample * ((s64) ((s16)Regs [ ( CVOL_R_START - SPU_X + ( Channel << 1 ) ) >> 1 ]) ) ) >> c_iVolumeShift;
			ChSampleL = ( Sample * ((s64) ((s16) GET_REG16 ( CVOLL_CH ( Channel ) ) ) ) ) >> c_iVolumeShift;
			ChSampleR = ( Sample * ((s64) ((s16) GET_REG16 ( CVOLR_CH ( Channel ) ) ) ) ) >> c_iVolumeShift;

			
			// check if channel is muted for debugging
			if ( !( Debug_ChannelEnable & ( 1 << Channel ) ) )
			{
				ChSampleL = 0;
				ChSampleR = 0;
			}

			
			// check if reverb is on for channel
			if ( ReverbOn & ( 1 << Channel ) )
			{
				ReverbSampleL += ChSampleL;
				ReverbSampleR += ChSampleR;
				
				// multiply by gain??
				//ReverbSampleL *= c_iMixer_Gain;
				//ReverbSampleR *= c_iMixer_Gain;
			}
			
			///////////////////////////////////////////////////////////////////
			// mix sample l/r
			
			SampleL += ChSampleL;
			SampleR += ChSampleR;
			
			
			//////////////////////////////////////////////////////////////////
			// Advance to next sample for channel
			//CurrentSample_Offset [ Channel ] += dSampleDT [ Channel ];
			//CurrentSample_Read [ Channel ] += dSampleDT [ Channel ];
			
			// save for debugging
			Debug_CurrentSample [ Channel ] = ( CurrentBlockAddress [ Channel ] >> 3 );	//CurrentSample_Offset [ Channel ] >> 32;
			Debug_CurrentRate [ Channel ] = dSampleDT [ Channel ] >> 20;
			
			// check if greater than or equal to 28 samples
			if ( CurrentSample_Offset [ Channel ] >= ( 28ULL << 32 ) )
			{
				// subtract 28
				CurrentSample_Offset [ Channel ] -= ( 28ULL << 32 );
				
				
				// check loop/end flag for current block
				/////////////////////////////////////////////////////////////////////////////////////////////
				// Check end flag and loop if needed (also checking to make sure loop bit is set also)
				// the loop/end flag is actually bit 0
				if ( RAM [ CurrentBlockAddress [ Channel ] >> 1 ] & ( 0x1 << 8 ) )
				{
					////////////////////////////////////////////////////////////////////
					// reached loop/end flag
					
					// make sure channel is not set to mute
					if ( ADSR_Status [ Channel ] != ADSR_MUTE )
					{
						// passed end of sample data, even if looping
						ChannelOn |= ( 1 << Channel );
					}

					// check if envelope should be killed
					//if ( KillEnvelope_Bitmap & ( 1 << Channel ) )
					if ( ( !( RAM [ CurrentBlockAddress [ Channel ] >> 1 ] & ( 0x2 << 8 ) ) ) /*|| ( KillEnvelope_Bitmap & ( 1 << Channel ) )*/ )
					{
						// if channel is not already set to mute, then turn off reverb for channel and mark we passed end of sample data
						if ( ADSR_Status [ Channel ] != ADSR_MUTE )
						{
							// turn off reverb for channel
							ReverbOn &= ~( 1 << Channel );
						}
						
						// kill envelope //
						ADSR_Status [ Channel ] = ADSR_MUTE;
						VOL_ADSR_Value [ Channel ] = 0;
					}
					
					// set address of next sample block to be loop start address
					//CurrentBlockAddress [ Channel ] = ( ( ((u32)Regs [ ( ( Channel << 4 ) + LSA_X ) >> 1 ]) << 3 ) & c_iRam_Mask );
					CurrentBlockAddress [ Channel ] = ( ( GET_REG32 ( LSA0_CH (Channel) ) << 3 ) & c_iRam_Mask );
				}
				else
				{
					// did not reach loop/end flag //
					
					// advance to address of next sample block
					CurrentBlockAddress [ Channel ] += 16;
					CurrentBlockAddress [ Channel ] &= c_iRam_Mask;
				}
				
				
				//////////////////////////////////////////////////////////////////////////////
				// Check loop start flag and set loop address if needed
				// LOOP_START is bit 3 actually
				// note: LOOP_START is actually bit 2
				if ( ( RAM [ ( CurrentBlockAddress [ Channel ] & c_iRam_Mask ) >> 1 ] & ( 0x4 << 8 ) ) /* && ! ( LSA_Manual_Bitmap & ( 1 << Channel ) ) */ )
				{
					///////////////////////////////////////////////////
					// we are at loop start address
					// set loop start address
					//Regs [ ( ( Channel << 4 ) + LSA_X ) >> 1 ] = ( CurrentBlockAddress [ Channel ] >> 3 );
					SET_REG32 ( LSA0_CH(Channel), CurrentBlockAddress [ Channel ] >> 3 );
					
					// clear killing of envelope
					//KillEnvelope_Bitmap &= ~( 1 << Channel );
				}
				
				//////////////////////////////////////////////////////////////////////////////
				// check if the IRQ address is in this block and if interrupts are enabled
				//if ( ( ( CurrentBlockAddress [ Channel ] >> 4 ) == ( Regs [ ( IRQA - SPU_X ) >> 1 ] >> 1 ) ) && ( Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] & 0x40 ) )
				if ( ( ( CurrentBlockAddress [ Channel ] >> 4 ) == ( GET_REG32 ( IRQA_0 ) >> 1 ) ) && ( GET_REG16 ( STAT ) & 0x40 ) )
				{
					// we have reached irq address - trigger interrupt
					SetInterrupt ();
					
					// interrupt
					//Regs [ ( 0x1f801dae - SPU_X ) >> 1 ] |= 0x40;
					GET_REG16 ( STAT ) |= 0x40;
				}
				
				// decode the new block
				//SampleDecoder [ Channel ].decode_packet ( (adpcm_packet*) & ( RAM [ CurrentBlockAddress [ Channel ] >> 1 ] ), & ( DecodedBlocks [ Channel * c_iSamplesPerBlock ] ) );
				CurrentSample_Write [ Channel ] += 28;
				SampleDecoder [ Channel ].decode_packet32 ( (adpcm_packet*) & ( RAM [ CurrentBlockAddress [ Channel ] >> 1 ] ), DecodedSamples );
				for ( int i = 0; i < 28; i++ ) DecodedBlocks [ ( Channel << 5 ) + ( ( CurrentSample_Write [ Channel ] + i ) & 0x1f ) ] = DecodedSamples [ i ];
			}
		}
	}
	
	// store to audio buffer l/r
	// check if SPU is muted
	//if ( ! ( Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] & 0x4000 ) )
	if ( ! ( GET_REG16 ( STAT ) & 0x4000 ) )
	{
		SampleL = 0;
		SampleR = 0;
	}
	
	///////////////////////////////////////////////////////////////
	// handle extern audio input if it is enabled
	/*
	if ( REG ( CTRL ) & 0x2 )
	{
		/////////////////////////////////////////////////////////////////
		// Extern audio is enabled
		
		// request external l/r audio sample from device
		
		// apply volume processing for extern audio l/r
		
		// mix into final audio output for this sample l/r
	}
	*/
	
	// load from spu unconditionally //
	// request l/r audio sample from cd device
	s32 TempL, TempR, TempSample;
	TempSample = CD::_CD->Spu_ReadNextSample ();
	TempL = TempSample >> 16;
	TempR = ( TempSample << 16 ) >> 16;
			
	//////////////////////////////////////////////////////////////
	// handle CD audio input if it is enabled and cd is playing
	// note: 0x1 bit should only control output of cd audio, not input of cd audio to SPU
	//if ( REG ( CTRL ) & 0x1 )
	//{
		// cd audio is enabled for output
		//CD_SampleL = 0;
		//CD_SampleR = 0;
		
		//if ( CD::_CD->isPlaying () )
		//{
			/////////////////////////////////////////////////////
			// CD audio is enabled
			s32 tVOL_L, tVOL_R;
			
		
#ifdef INLINE_DEBUG_CDSOUND
	if ( TempL != 0 || TempR != 0 )
	{
		debug << "\r\nMixing CD; SampleL=" << hex << TempL << " SampleR=" << TempR;
	}
#endif

			// apply volume processing for cd audio l/r
			// leave out temporarily for now
			/*
			tVOL_L = (s64) ( (s16) Regs [ ( CDVOL_L - SPU_X ) >> 1 ] );
			tVOL_R = (s64) ( (s16) Regs [ ( CDVOL_R - SPU_X ) >> 1 ] );
			CD_SampleL = ( TempL * tVOL_L ) >> c_iVolumeShift;
			CD_SampleR = ( TempR * tVOL_R ) >> c_iVolumeShift;
			*/
			
			// mix into final audio output for this sample l/r
			
			// sample should also be copied into sound ram for cd audio l/r area
			RAM [ ( 0x0000 + DecodeBufferOffset ) >> 1 ] = TempL;
			RAM [ ( 0x0400 + DecodeBufferOffset ) >> 1 ] = TempR;
			
			// check for interrupts
			if ( GET_REG16 ( CTRL ) & 0x40 )
			{
				if ( ( GET_REG32 ( IRQA_0 ) << 3 ) == ( DecodeBufferOffset + 0x000 ) )
				{
					// we have reached irq address - trigger interrupt
					SetInterrupt ();
					
					// interrupt
					//Regs [ ( STAT - SPU_X ) >> 1 ] |= 0x40;
					GET_REG16 ( STAT ) |= 0x40;
				}
				
				if ( ( GET_REG32 ( IRQA_0 ) << 3 ) == ( DecodeBufferOffset + 0x400 ) )
				{
					// we have reached irq address - trigger interrupt
					SetInterrupt ();
					
					// interrupt
					//Regs [ ( STAT - SPU_X ) >> 1 ] |= 0x40;
					GET_REG16 ( STAT ) |= 0x40;
				}
			}
			
		//}
		
		// check if cd audio output is enabled
		if ( GET_REG16 ( CTRL ) & 0x1 )
		{
			// mix
			SampleL += CD_SampleL;
			SampleR += CD_SampleR;
			
			// multiply by gain??
			//SampleL *= c_iMixer_Gain;
			//SampleR *= c_iMixer_Gain;
			
			// check if reverb is on for cd
			//if ( REG ( CTRL ) & 0x4 )
			if ( GET_REG16 ( CTRL ) & 0x4 )
			{
				// reverb is enabled for cd
				ReverbSampleL += CD_SampleL;
				ReverbSampleR += CD_SampleR;
				
				// multiply by gain??
				//ReverbSampleL *= c_iMixer_Gain;
				//ReverbSampleR *= c_iMixer_Gain;
			}
		}
	//}
	
	///////////////////////////////////////////////////////////
	// Apply FIR filter ??
	
	//SampleL = LPF_L.ApplyFilter ( SampleL );
	//SampleR = LPF_R.ApplyFilter ( SampleR );
	//ReverbSampleL = LPF_L_Reverb.ApplyFilter ( ReverbSampleL );
	//ReverbSampleR = LPF_R_Reverb.ApplyFilter ( ReverbSampleR );
	
	// perform filter for regular audio out
	FOutputL = Calc_sample_filter ( SampleL, Lx1, Lx2, Ly1, Ly2 );
	FOutputR = Calc_sample_filter ( SampleR, Rx1, Rx2, Ry1, Ry2 );
	ROutputL = Calc_sample_filter ( ReverbSampleL, LReverb_x1, LReverb_x2, LReverb_y1, LReverb_y2 );
	ROutputR = Calc_sample_filter ( ReverbSampleR, RReverb_x1, RReverb_x2, RReverb_y1, RReverb_y2 );
	
	// clamp
	
	// put samples in history
	Lx2 = Lx1; Lx1 = SampleL;
	Ly2 = Ly1; Ly1 = FOutputL;
	Rx2 = Rx1; Rx1 = SampleR;
	Ry2 = Ry1; Ry1 = FOutputR;
	LReverb_x2 = LReverb_x1; LReverb_x1 = ReverbSampleL;
	LReverb_y2 = LReverb_y1; LReverb_y1 = ROutputL;
	RReverb_x2 = RReverb_x1; RReverb_x1 = ReverbSampleR;
	RReverb_y2 = RReverb_y1; RReverb_y1 = ROutputR;
	
	// haven't decided which sounds better, so this is optional for now
	if ( AudioFilter_Enabled )
	{
		// set filter outputs
		SampleL = FOutputL;
		SampleR = FOutputR;
		ReverbSampleL = ROutputL;
		ReverbSampleR = ROutputR;
	}
	
	///////////////////////////////////////////////////////////////////
	// apply effect processing
	
	// check that reverb is enabled
	//if ( REG ( CTRL ) & 0x80 )
	//{
		// reverb is enabled //
		// or rather, the output is always enabled //
		
		// determine if we do reverb for left or for right on this cycle
		if ( CycleCount & 1 )
		{
			
			// do reverb @ 22050 hz //
			// *** TODO *** check for interrupt in reverb buffer
			//ProcessReverbR ( ReverbSampleR );
			
		}
		else
		{
			// process reverb
			//ProcessReverbL ( ReverbSampleL );
		}
		
		// mix
		// the mix of reverb output should happen unconditionally...
		SampleL += ReverbL_Output;
		SampleR += ReverbR_Output;
		
		// multiply by gain??
		//SampleL *= c_iMixer_Gain;
		//SampleR *= c_iMixer_Gain;
	//}
	
	
	//////////////////////////////////////
	// ***TODO*** apply master volume
	// still need to fix this so it uses the "current master volume" register
	//SampleL = ( SampleL * ( (s64) ((s16)Regs [ ( CMVOL_L - SPU_X ) >> 1 ]) ) ) >> c_iVolumeShift;
	//SampleR = ( SampleR * ( (s64) ((s16)Regs [ ( CMVOL_R - SPU_X ) >> 1 ]) ) ) >> c_iVolumeShift;
	SampleL = ( SampleL * ( (s64) ((s16) GET_REG16 ( CMVOL_L )) ) ) >> c_iVolumeShift;
	SampleR = ( SampleR * ( (s64) ((s16) GET_REG16 ( CMVOL_R )) ) ) >> c_iVolumeShift;
	
	////////////////////////////////////////////////////////
	// Apply the Program's Global Volume set by user
	// spucore will not be worried about this, just the interface
	//SampleL = ( SampleL * GlobalVolume ) >> c_iVolumeShift;
	//SampleR = ( SampleR * GlobalVolume ) >> c_iVolumeShift;
}






SPU2::SPU2 ()
{
	cout << "Running SPU2 constructor...\n";
}


SPU2::~SPU2 ()
{
	waveOutClose( hWaveOut );
}


void SPU2::Reset ()
{
	// zero object
	memset ( this, 0, sizeof( SPU2 ) );
}



void SPU2::Start ()
{
	cout << "Running SPU::Start...\n";

#ifdef INLINE_DEBUG_ENABLE
	debug.Create ( "SPU2Interface_Log.txt" );
#endif

#ifdef INLINE_DEBUG_ENABLE
	SPUCore::debug.Create ( "SPU2Core_Log.txt" );
#endif


#ifdef INLINE_DEBUG
	debug << "\r\nEntering SPU::Start";
#endif

	
	_SPU2 = this;
	Reset ();
	
	// set ram device pointer for SPUCore(s)
	SPUCore::RAM = RAM;
	SPUCore::_DebugPC = _DebugPC;
	SPUCore::_DebugCycleCount = _DebugCycleCount;

	
	// start the cores
	SPU0.Start ();
	SPU1.Start ();
	
	// set the core numbers
	SPU0.SetCoreNumber ( 0 );
	SPU1.SetCoreNumber ( 1 );
	
	// set the global volume to default
	GlobalVolume = c_iGlobalVolume_Default;
	
	// start the sound buffer out at 1m for now
	PlayBuffer_Size = c_iPlayBuffer_MaxSize;
	NextPlayBuffer_Size = c_iPlayBuffer_MaxSize;

	wfx.nSamplesPerSec = 44100; /* sample rate */
	wfx.wBitsPerSample = 16; /* sample size */
	wfx.nChannels = 2; /* channels*/
	/*
	 * WAVEFORMATEX also has other fields which need filling.
	 * as long as the three fields above are filled this should
	 * work for any PCM (pulse code modulation) format.
	 */
	wfx.cbSize = 0; /* size of _extra_ info */
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

	if( waveOutOpen( &hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL ) != MMSYSERR_NOERROR)
	{
		cout << "\nunable to open WAVE_MAPPER device\n";
		//ExitProcess(1);
	}
	else
	{
		cout << "\naudio device was opened successfully\n";
	}
	
	// disable audio output for now
	AudioOutput_Enabled = 1;
	
	// audio device is done with audio buffer
	//header.dwFlags |= WHDR_DONE;
	header0.dwFlags |= WHDR_DONE;
	header1.dwFlags |= WHDR_DONE;
	
	// *** testing ***
	//hWaveOut_Save = (u64)hWaveOut;
	
	// output size of object
	//cout << " Size of SPU class=" << sizeof ( SPU );

	// SPU is not needed to run on EVERY cycle
	Set_NextEvent ( CPUCycles_PerSPUCycle );
	
#ifdef INLINE_DEBUG
	debug << "->Exiting SPU::Start";
#endif
}



/*
void SPU::Reset ()
{
	int i;
	
	// zero object
	memset ( this, 0, sizeof( SPU ) );
	
	// pointers for quick access
	_vLOUT = & ( Regs [ ( vLOUT - SPU_X ) >> 1 ] );
	_vROUT = & ( Regs [ ( vROUT - SPU_X ) >> 1 ] );
	_mBASE = & ( Regs [ ( mBASE - SPU_X ) >> 1 ] );

	_dAPF1 = & ( Regs [ ( dAPF1 - SPU_X ) >> 1 ] );
	_dAPF2 = & ( Regs [ ( dAPF2 - SPU_X ) >> 1 ] );
	_vIIR = & ( Regs [ ( vIIR - SPU_X ) >> 1 ] );
	_vCOMB1 = & ( Regs [ ( vCOMB1 - SPU_X ) >> 1 ] );
	_vCOMB2 = & ( Regs [ ( vCOMB2 - SPU_X ) >> 1 ] );
	_vCOMB3 = & ( Regs [ ( vCOMB3 - SPU_X ) >> 1 ] );
	_vCOMB4 = & ( Regs [ ( vCOMB4 - SPU_X ) >> 1 ] );
	_vWALL = & ( Regs [ ( vWALL - SPU_X ) >> 1 ] );
	_vAPF1 = & ( Regs [ ( vAPF1 - SPU_X ) >> 1 ] );
	_vAPF2 = & ( Regs [ ( vAPF2 - SPU_X ) >> 1 ] );
	_mLSAME = & ( Regs [ ( mLSAME - SPU_X ) >> 1 ] );
	_mRSAME = & ( Regs [ ( mRSAME - SPU_X ) >> 1 ] );
	_mLCOMB1 = & ( Regs [ ( mLCOMB1 - SPU_X ) >> 1 ] );
	_mRCOMB1 = & ( Regs [ ( mRCOMB1 - SPU_X ) >> 1 ] );
	_mLCOMB2 = & ( Regs [ ( mLCOMB2 - SPU_X ) >> 1 ] );
	_mRCOMB2 = & ( Regs [ ( mRCOMB2 - SPU_X ) >> 1 ] );
	_dLSAME = & ( Regs [ ( dLSAME - SPU_X ) >> 1 ] );
	_dRSAME = & ( Regs [ ( dRSAME - SPU_X ) >> 1 ] );
	_mLDIFF = & ( Regs [ ( mLDIFF - SPU_X ) >> 1 ] );
	_mRDIFF = & ( Regs [ ( mRDIFF - SPU_X ) >> 1 ] );
	_mLCOMB3 = & ( Regs [ ( mLCOMB3 - SPU_X ) >> 1 ] );
	_mRCOMB3 = & ( Regs [ ( mRCOMB3 - SPU_X ) >> 1 ] );
	_mLCOMB4 = & ( Regs [ ( mLCOMB4 - SPU_X ) >> 1 ] );
	_mRCOMB4 = & ( Regs [ ( mRCOMB4 - SPU_X ) >> 1 ] );
	_dLDIFF = & ( Regs [ ( dLDIFF - SPU_X ) >> 1 ] );
	_dRDIFF = & ( Regs [ ( dRDIFF - SPU_X ) >> 1 ] );
	_mLAPF1 = & ( Regs [ ( mLAPF1 - SPU_X ) >> 1 ] );
	_mRAPF1 = & ( Regs [ ( mRAPF1 - SPU_X ) >> 1 ] );
	_mLAPF2 = & ( Regs [ ( mLAPF2 - SPU_X ) >> 1 ] );
	_mRAPF2 = & ( Regs [ ( mRAPF2 - SPU_X ) >> 1 ] );
	_vLIN = & ( Regs [ ( vLIN - SPU_X ) >> 1 ] );
	_vRIN = & ( Regs [ ( vRIN - SPU_X ) >> 1 ] );
	
	
	// enable audio filter by default
	AudioFilter_Enabled = true;

	
	// zero out registers
	//for ( i = 0; i < 256; i++ ) Regs [ i ] = 0;
	
	//VoiceOn_Bitmap = 0;
	
	s32 LowPassFilterCoefs [ 5 ];
	
	LowPassFilterCoefs [ 0 ] = gx [ 682 ];
	LowPassFilterCoefs [ 1 ] = gx [ 1365 ];
	LowPassFilterCoefs [ 2 ] = gx [ 2048 ];
	LowPassFilterCoefs [ 3 ] = gx [ 2730 ];
	LowPassFilterCoefs [ 4 ] = gx [ 3413 ];
	
	// reset the low pass filters
	LPF_L.Reset ();
	LPF_R.Reset ();
	LPF_L_Reverb.Reset ();
	LPF_R_Reverb.Reset ();
	
	// set lpf coefs
	LPF_L.SetFilter ( LowPassFilterCoefs );
	LPF_R.SetFilter ( LowPassFilterCoefs );
	LPF_L_Reverb.SetFilter ( LowPassFilterCoefs );
	LPF_R_Reverb.SetFilter ( LowPassFilterCoefs );
	
	
	// SPU is not needed to run on EVERY cycle
	//WaitCycles = CPUCycles_PerSPUCycle;
	//NextEvent_Cycle = CPUCycles_PerSPUCycle;
	Set_NextEvent ( CPUCycles_PerSPUCycle );
}
*/


/*
void SPU::UpdatePitch ( int Channel, u32 Pitch, u32 Reg_PMON, s32 PreviousSample )
{
	s64 Pitch_Step;
	s64 Pitch_Factor;
	
	Pitch_Step = Pitch;
	
	if ( Reg_PMON & ( 1 << Channel ) & ~1 )
	{
		// pitch modulation is enabled for channel //
		
		Pitch_Factor = ((s64)adpcm_decoder::clamp ( PreviousSample ));
		Pitch_Factor += 0x8000;
		Pitch_Step = ( Pitch_Step << 48 ) >> 48;
		Pitch_Step = ( Pitch_Step * Pitch_Factor ) >> 15;
		Pitch_Step &= 0xffff;
	}
	
	if ( Pitch_Step > 0x3fff ) Pitch_Step = 0x3fff;
	
	//Pitch_Counter += Pitch_Step;
	CurrentSample_Offset [ Channel ] += ( Pitch_Step << 20 );
	CurrentSample_Read [ Channel ] += ( Pitch_Step << 20 );
}
*/




/*
void SPU::Run ()
{
	int Channel;
	
	s64 Sample, PreviousSample, ChSampleL, ChSampleR, SampleL, SampleR, CD_SampleL, CD_SampleR, ReverbSampleL, ReverbSampleR;
	s64 FOutputL, FOutputR, ROutputL, ROutputR;
	
	u32 ChannelOn, ChannelNoise, PitchMod, ReverbOn;
	
	u64 Temp;
	u32 ModeRate;
	
	
	if ( NextEvent_Cycle != *_DebugCycleCount ) return;
	
	// update number of spu cycles ran
	CycleCount++;
	
	//NextEvent_Cycle = *_DebugCycleCount + CPUCycles_PerSPUCycle;
	Set_NextEvent ( CPUCycles_PerSPUCycle );
	
//#ifdef INLINE_DEBUG_RUN
//	debug << "\r\nSPU::Run";
//#endif

	// initialize current sample for left and right
	SampleL = 0;
	SampleR = 0;
	ReverbSampleL = 0;
	ReverbSampleR = 0;

	
	// SPU is on
	
	// *** TODO *** run SPU and output 1 sample LR
	
	/////////////////////////////////////////////////////////////////////
	// get what channels are enabled
	ChannelOn = Regs [ ( ( CON_0 - SPU_X ) >> 1 ) & 0xff ];
	ChannelOn |= ( (u32) ( Regs [ ( ( CON_1 - SPU_X ) >> 1 ) & 0xff ] ) ) << 16;
	
	// get what channels are set to noise
	ChannelNoise = Regs [ ( ( NON_0 - SPU_X ) >> 1 ) & 0xff ];
	ChannelNoise |= ( (u32) ( Regs [ ( ( NON_1 - SPU_X ) >> 1 ) & 0xff ] ) ) << 16;
	
	// get what channels are using frequency modulation
	PitchMod = Regs [ ( PMON_0 - SPU_X ) >> 1 ];
	PitchMod |= ( (u32) ( Regs [ ( PMON_1 - SPU_X ) >> 1 ] ) ) << 16;
	
	// get what channels have reverb on
	ReverbOn = Regs [ ( RON_0 - SPU_X ) >> 1 ];
	ReverbOn |= ( (u32) ( Regs [ ( RON_1 - SPU_X ) >> 1 ] ) ) << 16;

//#ifdef INLINE_DEBUG_RUN
//	debug << "; ChannelOn=" << ChannelOn << " KeyOn=" << KeyOn;
//#endif

	// if spu is enabled, run noise generator
	if ( Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] >> 15 )
	{
		RunNoiseGenerator ();
	}
	
	////////////////////////////
	// loop through channels
	for ( Channel = 0; Channel < 24; Channel++ )
	{
	
		// check if SPU is on
		if ( Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] >> 15 )
		{

			//SweepVolume ( Regs [ ( VOL_L >> 1 ) + ( Channel << 3 ) ], VOL_L_Value [ Channel ], VOL_L_Constant [ Channel ], VOL_L_Constant75 [ Channel ] );
			//SweepVolume ( Regs [ ( VOL_R >> 1 ) + ( Channel << 3 ) ], VOL_R_Value [ Channel ], VOL_R_Constant [ Channel ], VOL_R_Constant75 [ Channel ] );
			//SweepVolume ( Regs [ ( ( MVOL_L - SPU_X ) >> 1 ) & 0xff ], MVOL_L_Value, MVOL_L_Constant, MVOL_L_Constant75 );
			//SweepVolume ( Regs [ ( ( MVOL_R - SPU_X ) >> 1 ) & 0xff ], MVOL_R_Value, MVOL_R_Constant, MVOL_R_Constant75 );
			
			if ( Regs [ ( VOL_L >> 1 ) + ( Channel << 3 ) ] >> 15 )
			{
				// set current volume left
				VOL_L_Value [ Channel ] = (s64) ( (s16) Regs [ ( CVOL_L_START - SPU_X + ( Channel << 1 ) ) >> 1 ] );
				
				// perform envelope
				VolumeEnvelope ( VOL_L_Value [ Channel ], VOL_L_Cycles [ Channel ], Regs [ ( VOL_L >> 1 ) + ( Channel << 3 ) ] & 0x7f, ( Regs [ ( VOL_L >> 1 ) + ( Channel << 3 ) ] >> 13 ) & 0x3 );
				
				// store the new current volume left
				Regs [ ( CVOL_L_START - SPU_X + ( Channel << 1 ) ) >> 1 ] = VOL_L_Value [ Channel ];
			}
			else
			{
				// just set the current volume L
				Regs [ ( CVOL_L_START - SPU_X + ( Channel << 1 ) ) >> 1 ] = Regs [ ( VOL_L >> 1 ) + ( Channel << 3 ) ] << 1;
			}

			if ( Regs [ ( VOL_R >> 1 ) + ( Channel << 3 ) ] >> 15 )
			{
				// set current volume right
				VOL_R_Value [ Channel ] = (s64) ( (s16) Regs [ ( CVOL_R_START - SPU_X + ( Channel << 1 ) ) >> 1 ] );
				
				VolumeEnvelope ( VOL_R_Value [ Channel ], VOL_R_Cycles [ Channel ], Regs [ ( VOL_R >> 1 ) + ( Channel << 3 ) ] & 0x7f, ( Regs [ ( VOL_R >> 1 ) + ( Channel << 3 ) ] >> 13 ) & 0x3 );
				
				// store the new current volume right
				Regs [ ( CVOL_R_START - SPU_X + ( Channel << 1 ) ) >> 1 ] = VOL_R_Value [ Channel ];
			}
			else
			{
				// just set the current volume R
				Regs [ ( CVOL_R_START - SPU_X + ( Channel << 1 ) ) >> 1 ] = Regs [ ( VOL_R >> 1 ) + ( Channel << 3 ) ] << 1;
			}
			
			if ( Regs [ ( MVOL_L - SPU_X ) >> 1 ] >> 15 )
			{
				MVOL_L_Value = (s64) ( (s16) Regs [ ( CMVOL_L - SPU_X ) >> 1 ] );
				
				VolumeEnvelope ( MVOL_L_Value, MVOL_L_Cycles, Regs [ ( MVOL_L - SPU_X ) >> 1 ] & 0x7f, ( Regs [ ( MVOL_L - SPU_X ) >> 1 ] >> 13 ) & 0x3 );
				
				Regs [ ( CMVOL_L - SPU_X ) >> 1 ] = MVOL_L_Value;
			}
			else
			{
				// just set the current master volume L
				Regs [ ( CMVOL_L - SPU_X ) >> 1 ] = Regs [ ( MVOL_L - SPU_X ) >> 1 ] << 1;
			}
		
			if ( Regs [ ( MVOL_R - SPU_X ) >> 1 ] >> 15 )
			{
				MVOL_R_Value = (s64) ( (s16) Regs [ ( CMVOL_R - SPU_X ) >> 1 ] );
				
				VolumeEnvelope ( MVOL_R_Value, MVOL_R_Cycles, Regs [ ( MVOL_R - SPU_X ) >> 1 ] & 0x7f, ( Regs [ ( MVOL_R - SPU_X ) >> 1 ] >> 13 ) & 0x3 );
				
				Regs [ ( CMVOL_R - SPU_X ) >> 1 ] = MVOL_R_Value;
			}
			else
			{
				// just set the current master volume R
				Regs [ ( CMVOL_R - SPU_X ) >> 1 ] = Regs [ ( MVOL_R - SPU_X ) >> 1 ] << 1;
			}
			
			/////////////////////////////////////////////////////////////////////
			// update ADSR envelope
			
			// check adsr status
			switch ( ADSR_Status [ Channel ] )
			{
				case ADSR_MUTE:
				
					VOL_ADSR_Value [ Channel ] = 0;
					Regs [ ( ( Channel << 4 ) + ENV_X ) >> 1 ] = 0;
				
					break;
					
				case ADSR_ATTACK:
#ifdef INLINE_DEBUG_RUN
	debug << "\r\nChannel#" << Channel;
#endif
			
#ifdef INLINE_DEBUG_RUN
	//debug << "; Attack; Rate=" << (((double)VOL_ATTACK_Constant [ Channel ])/64536) << "; Rate75=" << (((double)VOL_ATTACK_Constant75 [ Channel ])/64536);
	debug << "; Attack";
#endif

					///////////////////////////////////////////
					// ADSR - Attack Mode
					
					////////////////////////////////////////////////////
					// linear or psx pseudo exponential increase
					
#ifdef INLINE_DEBUG_RUN
	debug << "; (before) VOL_ADSR_Value=" << hex << VOL_ADSR_Value [ Channel ] << "; Cycles=" << dec << Cycles [ Channel ];
#endif

					ModeRate = Regs [ ( ADSR_0 >> 1 ) + ( Channel << 3 ) ];
					VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ModeRate >> 8 ) & 0x7f, ( ModeRate >> 15 ) << 1 );
					
#ifdef INLINE_DEBUG_RUN
	debug << "; (after) VOL_ADSR_Value=" << hex << VOL_ADSR_Value [ Channel ] << "; Cycles=" << dec << Cycles [ Channel ] << "; Value=" << hex << ( ( ModeRate >> 8 ) & 0x7f ) << "; flags=" << ( ( ModeRate >> 15 ) << 1 );
#endif

					// check if current volume < 0x6000
					
					// saturate and switch to decay mode if volume goes above 32767
					if ( VOL_ADSR_Value [ Channel ] >= 32767 )
					{
#ifdef INLINE_DEBUG_RUN
	debug << "; DECAY_NEXT";
#endif

						VOL_ADSR_Value [ Channel ] = 32767;
						ADSR_Status [ Channel ] = ADSR_DECAY;
						
						// start envelope for decay mode
						ModeRate = Regs [ ( ADSR_0 >> 1 ) + ( Channel << 3 ) ];
						Start_VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ( ModeRate >> 4 ) & 0xf ) << ( 2 ), 0x3 );
					}
					
					break;
					
				case ADSR_DECAY:
#ifdef INLINE_DEBUG_RUN
	debug << "\r\nChannel#" << Channel;
#endif
			
#ifdef INLINE_DEBUG_RUN
	//debug << "; Decay; Rate=" << (((double)VOL_DECAY_Constant [ Channel ])/(1<<30));
	debug << "; Decay";
#endif

					////////////////////////////////////////////
					// ADSR - Decay Mode
				
					////////////////////////////////////////////////
					// Exponential decrease


#ifdef INLINE_DEBUG_RUN
	debug << "; (before) VOL_ADSR_Value=" << hex << VOL_ADSR_Value [ Channel ] << "; Cycles=" << dec << Cycles [ Channel ];
#endif

					ModeRate = Regs [ ( ADSR_0 >> 1 ) + ( Channel << 3 ) ];
					VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ( ModeRate >> 4 ) & 0xf ) << ( 2 ), 0x3 );
					
#ifdef INLINE_DEBUG_RUN
	debug << "; (after) VOL_ADSR_Value=" << hex << VOL_ADSR_Value [ Channel ] << "; Cycles=" << dec << Cycles [ Channel ] << "; Value=" << hex << ( ( ModeRate >> 8 ) & 0x7f ) << "; flags=" << ( ( ModeRate >> 15 ) << 1 );
#endif

					// saturate if volume goes below zero
					// *** TODO *** ADSR volume probably can't go below zero in decay mode since it is always an exponential decrease
					if ( VOL_ADSR_Value [ Channel ] < 0 )
					{
						VOL_ADSR_Value [ Channel ] = 0;
					}
					
					// switch to sustain mode if we reach sustain level
					if ( VOL_ADSR_Value [ Channel ] <= VOL_SUSTAIN_Level [ Channel ] )
					{
#ifdef INLINE_DEBUG_RUN
	debug << "; SUSTAIN_NEXT";
#endif

						VOL_ADSR_Value [ Channel ] = VOL_SUSTAIN_Level [ Channel ];
						ADSR_Status [ Channel ] = ADSR_SUSTAIN;
						
						// start envelope for sustain mode
						ModeRate = Regs [ ( ADSR_1 >> 1 ) + ( Channel << 3 ) ];
						Start_VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ModeRate >> 6 ) & 0x7f, ModeRate >> 14 );
					}
					
					break;
					
				case ADSR_SUSTAIN:
#ifdef INLINE_DEBUG_RUN
	debug << "\r\nChannel#" << Channel;
#endif
			
#ifdef INLINE_DEBUG_RUN
	//debug << "; Sustain; Rate=" << (((double)VOL_SUSTAIN_Constant [ Channel ])/64536) << "; Rate75=" << (((double)VOL_SUSTAIN_Constant75 [ Channel ])/64536);
	debug << "; Sustain";
#endif

					/////////////////////////////////////////////
					// ADSR - Sustain Mode
					
#ifdef INLINE_DEBUG_RUN
	debug << "; (before) VOL_ADSR_Value=" << hex << VOL_ADSR_Value [ Channel ] << "; Cycles=" << dec << Cycles [ Channel ];
#endif
					
					ModeRate = Regs [ ( ADSR_1 >> 1 ) + ( Channel << 3 ) ];
					VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ModeRate >> 6 ) & 0x7f, ModeRate >> 14 );

#ifdef INLINE_DEBUG_RUN
	debug << "; (after) VOL_ADSR_Value=" << hex << VOL_ADSR_Value [ Channel ] << "; Cycles=" << dec << Cycles [ Channel ] << "; Value=" << hex << ( ( ModeRate >> 8 ) & 0x7f ) << "; flags=" << ( ( ModeRate >> 15 ) << 1 );
#endif

					
					// saturate if volume goes above 32767
					if ( VOL_ADSR_Value [ Channel ] > 32767 )
					{
						VOL_ADSR_Value [ Channel ] = 32767;
					}
					
					// or below zero
					if ( VOL_ADSR_Value [ Channel ] < 0 )
					{
						VOL_ADSR_Value [ Channel ] = 0;
					}
					
					// we do not switch to release mode until key off signal is given
					
					break;
					
				case ADSR_RELEASE:
#ifdef INLINE_DEBUG_RUN
	debug << "\r\nChannel#" << Channel;
#endif
			
#ifdef INLINE_DEBUG_RUN
	//debug << "; Release; Rate=" << (((double)VOL_RELEASE_Constant [ Channel ])/(1<<30));
	debug << "; Release";
#endif

					///////////////////////////////////////////////
					// ADSR - Release Mode
				
#ifdef INLINE_DEBUG_RUN
	debug << "; (before) VOL_ADSR_Value=" << hex << VOL_ADSR_Value [ Channel ] << "; Cycles=" << dec << Cycles [ Channel ];
#endif

					// when at or below zero we turn note off completely and set adsr volume to zero
					if ( VOL_ADSR_Value [ Channel ] <= 0 )
					{
						// ADSR volume is below zero in RELEASE mode //
					
						// saturate to zero
						VOL_ADSR_Value [ Channel ] = 0;
						
						// the channel on bit is not really for whether the channel is on or off
						//ChannelOn = ChannelOn & ~( 1 << Channel );
					}
					else
					{
						// RELEASE mode //
						
						// *** note *** it is possible for ADSR volume to go negative for 1T in linear decrement mode //
						
						ModeRate = Regs [ ( ADSR_1 >> 1 ) + ( Channel << 3 ) ];
						VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ModeRate & 0x1f ) << ( 2 ), ( ( ( ModeRate >> 5 ) & 1 ) << 1 ) | 0x1 );
					}
					
					// *** testing ***
					if ( VOL_ADSR_Value [ Channel ] < 0 )
					{
						// saturate to zero
						VOL_ADSR_Value [ Channel ] = 0;
					}


#ifdef INLINE_DEBUG_RUN
	debug << "; (after) VOL_ADSR_Value=" << hex << VOL_ADSR_Value [ Channel ] << "; Cycles=" << dec << Cycles [ Channel ] << "; Value=" << hex << ( ( ModeRate >> 8 ) & 0x7f ) << "; flags=" << ( ( ModeRate >> 15 ) << 1 );
#endif

					// check if linear or exponential (adsr1 bit 5)
					
						
					break;
			}
			
			/////////////////////////////////////////////////////////////////////////////
			// update ADSR Envelope Volume
			Regs [ ( ( Channel << 4 ) + ENV_X ) >> 1 ] = VOL_ADSR_Value [ Channel ];
			
			//////////////////////////////////////////////////////////////
			// load sample
			
			// check if channel is set to noise
			if ( ChannelNoise & ( 1 << Channel ) )
			{
				// channel is set to noise //
				
				Sample = NoiseLevel;
			}
			else
			{
				// channel is not set to noise //
				
				u32 SampleNumber = CurrentSample_Read [ Channel ] >> 32;
				
				//Sample = DecodedBlocks [ ( Channel * 28 ) + ( CurrentSample_Offset [ Channel ] >> 32 ) ];
				Sample = DecodedBlocks [ ( Channel << 5 ) + ( SampleNumber & 0x1f ) ];
				
				///////////////////////////////////////////
				// apply sample interpolation
				
				Sample = Calc_sample_gx ( CurrentSample_Read [ Channel ] >> 16, Sample, DecodedBlocks [ ( Channel << 5 ) + ( ( SampleNumber - 1 ) & 0x1f ) ],
				DecodedBlocks [ ( Channel << 5 ) + ( ( SampleNumber - 2 ) & 0x1f ) ], DecodedBlocks [ ( Channel << 5 ) + ( ( SampleNumber - 3 ) & 0x1f ) ] );
				
				
				////////////////////////////////////
				// apply envelope volume
				// this does not apply when set to noise
				Sample = ( Sample * ( (s64) ( (s16) Regs [ ( ( Channel << 4 ) + ENV_X ) >> 1 ] ) ) ) >> c_iVolumeShift;
				
				UpdatePitch ( Channel, Regs [ ( ( Channel << 4 ) + PITCH ) >> 1 ], PitchMod, PreviousSample );
				
			}
			
			// store previous sample in case next channel uses frequency modulation
			PreviousSample = Sample;
			
			// save current sample for debugging
			Debug_CurrentRawSample [ Channel ] = Sample;

			// copy samples for voice1 and voice3 into buffer //
			
			if ( Channel == 1 )
			{
				RAM [ ( 0x0800 + DecodeBufferOffset ) >> 1 ] = Sample;
			}
			
			if ( Channel == 3 )
			{
				RAM [ ( 0x0c00 + DecodeBufferOffset ) >> 1 ] = Sample;
			}
			
			// check for interrupts
			if ( ( ( ( (u32) Regs [ ( IRQA - SPU_X ) >> 1 ] ) << 3 ) == ( DecodeBufferOffset + 0x800 ) ) && ( Regs [ ( CTRL - SPU_X ) >> 1 ] & 0x40 ) )
			{
				// we have reached irq address - trigger interrupt
				SetInterrupt ();
				
				// interrupt
				Regs [ ( STAT - SPU_X ) >> 1 ] |= 0x40;
			}
			
			if ( ( ( ( (u32) Regs [ ( IRQA - SPU_X ) >> 1 ] ) << 3 ) == ( DecodeBufferOffset + 0xc00 ) ) && ( Regs [ ( CTRL - SPU_X ) >> 1 ] & 0x40 ) )
			{
				// we have reached irq address - trigger interrupt
				SetInterrupt ();
				
				// interrupt
				Regs [ ( STAT - SPU_X ) >> 1 ] |= 0x40;
			}
			
			//////////////////////////////////////////////////////////////////
			// apply volume processing
			
			
			// apply channel volume
			//ChSampleL = ( Sample * ( VOL_L_Value [ Channel ] >> 16 ) ) >> c_iVolumeShift;
			//ChSampleR = ( Sample * ( VOL_R_Value [ Channel ] >> 16 ) ) >> c_iVolumeShift;
			ChSampleL = ( Sample * ((s64) ((s16)Regs [ ( CVOL_L_START - SPU_X + ( Channel << 1 ) ) >> 1 ]) ) ) >> c_iVolumeShift;
			ChSampleR = ( Sample * ((s64) ((s16)Regs [ ( CVOL_R_START - SPU_X + ( Channel << 1 ) ) >> 1 ]) ) ) >> c_iVolumeShift;

			
			// check if channel is muted for debugging
			if ( !( Debug_ChannelEnable & ( 1 << Channel ) ) )
			{
				ChSampleL = 0;
				ChSampleR = 0;
			}

			// update current left/right volume for channel
			//Regs [ 256 + ( Channel << 1 ) + 0 ] = VOL_L_Value [ Channel ] >> 16;
			//Regs [ 256 + ( Channel << 1 ) + 1 ] = VOL_R_Value [ Channel ] >> 16;
			
			// check if reverb is on for channel
			if ( ReverbOn & ( 1 << Channel ) )
			{
				ReverbSampleL += ChSampleL;
				ReverbSampleR += ChSampleR;
				
				// multiply by gain??
				//ReverbSampleL *= c_iMixer_Gain;
				//ReverbSampleR *= c_iMixer_Gain;
			}
			
			///////////////////////////////////////////////////////////////////
			// mix sample l/r
			
			SampleL += ChSampleL;
			SampleR += ChSampleR;
			
			// multiply by gain??
			//SampleL *= c_iMixer_Gain;
			//SampleR *= c_iMixer_Gain;
			
			//////////////////////////////////////////////////////////////////
			// Advance to next sample for channel
			//CurrentSample_Offset [ Channel ] += dSampleDT [ Channel ];
			//CurrentSample_Read [ Channel ] += dSampleDT [ Channel ];
			
			// save for debugging
			Debug_CurrentSample [ Channel ] = ( CurrentBlockAddress [ Channel ] >> 3 );	//CurrentSample_Offset [ Channel ] >> 32;
			Debug_CurrentRate [ Channel ] = dSampleDT [ Channel ] >> 20;
			
			// check if greater than or equal to 28 samples
			if ( CurrentSample_Offset [ Channel ] >= ( 28ULL << 32 ) )
			{
				// subtract 28
				CurrentSample_Offset [ Channel ] -= ( 28ULL << 32 );
				
				
				// check loop/end flag for current block
				/////////////////////////////////////////////////////////////////////////////////////////////
				// Check end flag and loop if needed (also checking to make sure loop bit is set also)
				// the loop/end flag is actually bit 0
				if ( RAM [ CurrentBlockAddress [ Channel ] >> 1 ] & ( 0x1 << 8 ) )
				{
					////////////////////////////////////////////////////////////////////
					// reached loop/end flag
					
					// make sure channel is not set to mute
					if ( ADSR_Status [ Channel ] != ADSR_MUTE )
					{
						// passed end of sample data, even if looping
						ChannelOn |= ( 1 << Channel );
					}

					// check if envelope should be killed
					//if ( KillEnvelope_Bitmap & ( 1 << Channel ) )
					if ( ( !( RAM [ CurrentBlockAddress [ Channel ] >> 1 ] & ( 0x2 << 8 ) ) )  )
					{
						// if channel is not already set to mute, then turn off reverb for channel and mark we passed end of sample data
						if ( ADSR_Status [ Channel ] != ADSR_MUTE )
						{
							// turn off reverb for channel
							ReverbOn &= ~( 1 << Channel );
						}
						
						// kill envelope //
						ADSR_Status [ Channel ] = ADSR_MUTE;
						VOL_ADSR_Value [ Channel ] = 0;
					}
					
					// set address of next sample block to be loop start address
					CurrentBlockAddress [ Channel ] = ( ( ((u32)Regs [ ( ( Channel << 4 ) + LSA_X ) >> 1 ]) << 3 ) & c_iRam_Mask );
				}
				else
				{
					// did not reach loop/end flag //
					
					// advance to address of next sample block
					CurrentBlockAddress [ Channel ] += 16;
					CurrentBlockAddress [ Channel ] &= c_iRam_Mask;
				}
				
				
				//////////////////////////////////////////////////////////////////////////////
				// Check loop start flag and set loop address if needed
				// LOOP_START is bit 3 actually
				// note: LOOP_START is actually bit 2
				if ( ( RAM [ ( CurrentBlockAddress [ Channel ] & c_iRam_Mask ) >> 1 ] & ( 0x4 << 8 ) )  )
				{
					///////////////////////////////////////////////////
					// we are at loop start address
					// set loop start address
					Regs [ ( ( Channel << 4 ) + LSA_X ) >> 1 ] = ( CurrentBlockAddress [ Channel ] >> 3 );
					
					// clear killing of envelope
					//KillEnvelope_Bitmap &= ~( 1 << Channel );
				}
				
				//////////////////////////////////////////////////////////////////////////////
				// check if the IRQ address is in this block and if interrupts are enabled
				if ( ( ( CurrentBlockAddress [ Channel ] >> 4 ) == ( Regs [ ( IRQA - SPU_X ) >> 1 ] >> 1 ) ) && ( Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] & 0x40 ) )
				{
					// we have reached irq address - trigger interrupt
					SetInterrupt ();
					
					// interrupt
					Regs [ ( 0x1f801dae - SPU_X ) >> 1 ] |= 0x40;
				}
				
				// decode the new block
				//SampleDecoder [ Channel ].decode_packet ( (adpcm_packet*) & ( RAM [ CurrentBlockAddress [ Channel ] >> 1 ] ), & ( DecodedBlocks [ Channel * c_iSamplesPerBlock ] ) );
				CurrentSample_Write [ Channel ] += 28;
				SampleDecoder [ Channel ].decode_packet32 ( (adpcm_packet*) & ( RAM [ CurrentBlockAddress [ Channel ] >> 1 ] ), DecodedSamples );
				for ( int i = 0; i < 28; i++ ) DecodedBlocks [ ( Channel << 5 ) + ( ( CurrentSample_Write [ Channel ] + i ) & 0x1f ) ] = DecodedSamples [ i ];
			}
		}
	}
	
	// store to audio buffer l/r
	// check if SPU is muted
	if ( ! ( Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] & 0x4000 ) )
	{
		SampleL = 0;
		SampleR = 0;
	}
	
	
	// load from spu unconditionally //
	// request l/r audio sample from cd device
	s32 TempL, TempR, TempSample;
	TempSample = CD::_CD->Spu_ReadNextSample ();
	TempL = TempSample >> 16;
	TempR = ( TempSample << 16 ) >> 16;
			
	//////////////////////////////////////////////////////////////
	// handle CD audio input if it is enabled and cd is playing
	// note: 0x1 bit should only control output of cd audio, not input of cd audio to SPU
	//if ( REG ( CTRL ) & 0x1 )
	//{
		// cd audio is enabled for output
		//CD_SampleL = 0;
		//CD_SampleR = 0;
		
		//if ( CD::_CD->isPlaying () )
		//{
			/////////////////////////////////////////////////////
			// CD audio is enabled
			s32 tVOL_L, tVOL_R;
			
		
#ifdef INLINE_DEBUG_CDSOUND
	if ( TempL != 0 || TempR != 0 )
	{
		debug << "\r\nMixing CD; SampleL=" << hex << TempL << " SampleR=" << TempR;
	}
#endif

			// apply volume processing for cd audio l/r
			tVOL_L = (s64) ( (s16) Regs [ ( CDVOL_L - SPU_X ) >> 1 ] );
			tVOL_R = (s64) ( (s16) Regs [ ( CDVOL_R - SPU_X ) >> 1 ] );
			CD_SampleL = ( TempL * tVOL_L ) >> c_iVolumeShift;
			CD_SampleR = ( TempR * tVOL_R ) >> c_iVolumeShift;
			
			// store to cd audio buffer l/r
			//Mixer [ ( Mixer_WriteIdx + 0 ) & c_iMixerMask ] += TempL;
			//Mixer [ ( Mixer_WriteIdx + 1 ) & c_iMixerMask ] += TempR;
			
			// mix into final audio output for this sample l/r
			
			// sample should also be copied into sound ram for cd audio l/r area
			RAM [ ( 0x0000 + DecodeBufferOffset ) >> 1 ] = TempL;
			RAM [ ( 0x0400 + DecodeBufferOffset ) >> 1 ] = TempR;
			
			// check for interrupts
			if ( ( ( ( (u32) Regs [ ( IRQA - SPU_X ) >> 1 ] ) << 3 ) == ( DecodeBufferOffset + 0x000 ) ) && ( Regs [ ( CTRL - SPU_X ) >> 1 ] & 0x40 ) )
			{
				// we have reached irq address - trigger interrupt
				SetInterrupt ();
				
				// interrupt
				Regs [ ( STAT - SPU_X ) >> 1 ] |= 0x40;
			}
			
			if ( ( ( ( (u32) Regs [ ( IRQA - SPU_X ) >> 1 ] ) << 3 ) == ( DecodeBufferOffset + 0x400 ) ) && ( Regs [ ( CTRL - SPU_X ) >> 1 ] & 0x40 ) )
			{
				// we have reached irq address - trigger interrupt
				SetInterrupt ();
				
				// interrupt
				Regs [ ( STAT - SPU_X ) >> 1 ] |= 0x40;
			}
			
		//}
		
		// check if cd audio output is enabled
		if ( REG ( CTRL ) & 0x1 )
		{
			// mix
			SampleL += CD_SampleL;
			SampleR += CD_SampleR;
			
			// multiply by gain??
			//SampleL *= c_iMixer_Gain;
			//SampleR *= c_iMixer_Gain;
			
			// check if reverb is on for cd
			if ( REG ( CTRL ) & 0x4 )
			{
				// reverb is enabled for cd
				ReverbSampleL += CD_SampleL;
				ReverbSampleR += CD_SampleR;
				
				// multiply by gain??
				//ReverbSampleL *= c_iMixer_Gain;
				//ReverbSampleR *= c_iMixer_Gain;
			}
		}
	//}
	
	///////////////////////////////////////////////////////////
	// Apply FIR filter ??
	
	//SampleL = LPF_L.ApplyFilter ( SampleL );
	//SampleR = LPF_R.ApplyFilter ( SampleR );
	//ReverbSampleL = LPF_L_Reverb.ApplyFilter ( ReverbSampleL );
	//ReverbSampleR = LPF_R_Reverb.ApplyFilter ( ReverbSampleR );
	
	// perform filter for regular audio out
	FOutputL = Calc_sample_filter ( SampleL, Lx1, Lx2, Ly1, Ly2 );
	FOutputR = Calc_sample_filter ( SampleR, Rx1, Rx2, Ry1, Ry2 );
	ROutputL = Calc_sample_filter ( ReverbSampleL, LReverb_x1, LReverb_x2, LReverb_y1, LReverb_y2 );
	ROutputR = Calc_sample_filter ( ReverbSampleR, RReverb_x1, RReverb_x2, RReverb_y1, RReverb_y2 );
	
	// clamp
	
	// put samples in history
	Lx2 = Lx1; Lx1 = SampleL;
	Ly2 = Ly1; Ly1 = FOutputL;
	Rx2 = Rx1; Rx1 = SampleR;
	Ry2 = Ry1; Ry1 = FOutputR;
	LReverb_x2 = LReverb_x1; LReverb_x1 = ReverbSampleL;
	LReverb_y2 = LReverb_y1; LReverb_y1 = ROutputL;
	RReverb_x2 = RReverb_x1; RReverb_x1 = ReverbSampleR;
	RReverb_y2 = RReverb_y1; RReverb_y1 = ROutputR;
	
	// haven't decided which sounds better, so this is optional for now
	if ( AudioFilter_Enabled )
	{
		// set filter outputs
		SampleL = FOutputL;
		SampleR = FOutputR;
		ReverbSampleL = ROutputL;
		ReverbSampleR = ROutputR;
	}
	
	///////////////////////////////////////////////////////////////////
	// apply effect processing
	
	// check that reverb is enabled
	//if ( REG ( CTRL ) & 0x80 )
	//{
		// reverb is enabled //
		// or rather, the output is always enabled //
		
		// determine if we do reverb for left or for right on this cycle
		if ( CycleCount & 1 )
		{
			
			// do reverb @ 22050 hz //
			// *** TODO *** check for interrupt in reverb buffer
			ProcessReverbR ( ReverbSampleR );
			
		}
		else
		{
			// process reverb
			ProcessReverbL ( ReverbSampleL );
		}
		
		// mix
		// the mix of reverb output should happen unconditionally...
		SampleL += ReverbL_Output;
		SampleR += ReverbR_Output;
		
		// multiply by gain??
		//SampleL *= c_iMixer_Gain;
		//SampleR *= c_iMixer_Gain;
	//}
	
	
	//////////////////////////////////////
	// ***TODO*** apply master volume
	// still need to fix this so it uses the "current master volume" register
	//SampleL = ( SampleL * ( MVOL_L_Value >> 16 ) ) >> c_iVolumeShift;
	//SampleR = ( SampleR * ( MVOL_R_Value >> 16 ) ) >> c_iVolumeShift;
	SampleL = ( SampleL * ( (s64) ((s16)Regs [ ( CMVOL_L - SPU_X ) >> 1 ]) ) ) >> c_iVolumeShift;
	SampleR = ( SampleR * ( (s64) ((s16)Regs [ ( CMVOL_R - SPU_X ) >> 1 ]) ) ) >> c_iVolumeShift;
	
	// update current master volume registers
	//Regs [ ( 0x1f801db8 - SPU_X ) >> 1 ] = MVOL_L_Value >> 16;
	//Regs [ ( 0x1f801dba - SPU_X ) >> 1 ] = MVOL_R_Value >> 16;
	
	////////////////////////////////////////////////////////
	// Apply the Program's Global Volume set by user
	SampleL = ( SampleL * GlobalVolume ) >> c_iVolumeShift;
	SampleR = ( SampleR * GlobalVolume ) >> c_iVolumeShift;

	// mix samples
	Mixer [ ( Mixer_WriteIdx + 0 ) & c_iMixerMask ] = adpcm_decoder::clamp ( SampleL );
	Mixer [ ( Mixer_WriteIdx + 1 ) & c_iMixerMask ] = adpcm_decoder::clamp ( SampleR );
	
	// samples is now in mixer
	Mixer_WriteIdx += 2;
	
	// output one l/r sample
	if ( AudioOutput_Enabled && ( Mixer_WriteIdx - Mixer_ReadIdx ) >= PlayBuffer_Size )
	{
		int testvalue0, testvalue1;
		
		//if ( !hWaveOut ) cout << "\n!hWaveOut; p1\n";
		
		// make sure the read index at write index minus the size of the buffer
		Mixer_ReadIdx = Mixer_WriteIdx - PlayBuffer_Size;
		
		if ( ( Mixer_WriteIdx - Mixer_ReadIdx ) == PlayBuffer_Size )
		{
			// play buffer cannot hold any more data //
			
			
			//testvalue0 = waveOutUnprepareHeader( hWaveOut, &header0, sizeof(WAVEHDR) );
			//testvalue1 = waveOutUnprepareHeader( hWaveOut, &header1, sizeof(WAVEHDR) );
			
			
			//while ( !( ((volatile u32) (header.dwFlags)) & WHDR_DONE ) )
			//while ( waveOutUnprepareHeader( hWaveOut, &header0, sizeof(WAVEHDR) ) == WAVERR_STILLPLAYING )
			//while ( testvalue0 == WAVERR_STILLPLAYING && testvalue1 == WAVERR_STILLPLAYING )
			while ( !( header0.dwFlags & WHDR_DONE ) && !( header1.dwFlags & WHDR_DONE ) )
			{
				//cout << "\nWaiting for samples to finish playing...";
				
				//MsgWaitForMultipleObjectsEx( NULL, NULL, 1, QS_ALLINPUT, MWMO_ALERTABLE );
				
				//testvalue = waveOutUnprepareHeader( hWaveOut, &header0, sizeof(WAVEHDR) );
				testvalue0 = waveOutUnprepareHeader( hWaveOut, &header0, sizeof(WAVEHDR) );
				testvalue1 = waveOutUnprepareHeader( hWaveOut, &header1, sizeof(WAVEHDR) );
				
			}
		}
		
		//testvalue = waveOutUnprepareHeader( hWaveOut, &header0, sizeof(WAVEHDR) );
		//testvalue0 = waveOutUnprepareHeader( hWaveOut, &header0, sizeof(WAVEHDR) );
		//testvalue1 = waveOutUnprepareHeader( hWaveOut, &header1, sizeof(WAVEHDR) );
		
		//if ( !hWaveOut ) cout << "\n!hWaveOut; p4\n";
		
		//if ( header.dwFlags & WHDR_DONE )
		//if ( testvalue0 == MMSYSERR_NOERROR )
		if ( header0.dwFlags & WHDR_DONE )
		{
#ifdef INLINE_DEBUG_CDSOUND
	//debug << "\r\nPlaying; Mixer_WriteIdx=" << dec << Mixer_WriteIdx << " Mixer_ReadIdx=" << dec << Mixer_ReadIdx;
#endif

			ZeroMemory( &header0, sizeof(WAVEHDR) );
			
			//if ( !hWaveOut ) cout << "\n!hWaveOut; p5\n";
			
			// this must be the size in bytes
			header0.dwBufferLength = ( Mixer_WriteIdx - Mixer_ReadIdx ) * 2;	//size;
			
			// copy samples to play into the play buffer
			for ( int i = 0; i < ( Mixer_WriteIdx - Mixer_ReadIdx ); i++ ) PlayBuffer0 [ i ] = Mixer [ ( i + Mixer_ReadIdx ) & c_iMixerMask ];
			
			//if ( !hWaveOut ) cout << "\n!hWaveOut; p6\n";
			
			header0.lpData = (char*) PlayBuffer0;

			if ( AudioOutput_Enabled )
			{
				testvalue0 = waveOutPrepareHeader( hWaveOut, &header0, sizeof(WAVEHDR) );
				
				
				testvalue0 = waveOutWrite( hWaveOut, &header0, sizeof(WAVEHDR) );
				
				
				//cout << "\nSent enabled audio successfully.";
			}
			
			// data in mixer has been played now
			Mixer_ReadIdx = Mixer_WriteIdx;
		}
		//else if ( testvalue1 == MMSYSERR_NOERROR )
		else if ( header1.dwFlags & WHDR_DONE )
		{
			ZeroMemory( &header1, sizeof(WAVEHDR) );
			
			//if ( !hWaveOut ) cout << "\n!hWaveOut; p5\n";
			
			// this must be the size in bytes
			header1.dwBufferLength = ( Mixer_WriteIdx - Mixer_ReadIdx ) * 2;	//size;
			
			// copy samples to play into the play buffer
			for ( int i = 0; i < ( Mixer_WriteIdx - Mixer_ReadIdx ); i++ ) PlayBuffer1 [ i ] = Mixer [ ( i + Mixer_ReadIdx ) & c_iMixerMask ];
			
			//if ( !hWaveOut ) cout << "\n!hWaveOut; p6\n";
			
			header1.lpData = (char*) PlayBuffer1;

			if ( AudioOutput_Enabled )
			{
				testvalue1 = waveOutPrepareHeader( hWaveOut, &header1, sizeof(WAVEHDR) );
				
				
				testvalue1 = waveOutWrite( hWaveOut, &header1, sizeof(WAVEHDR) );
				
				
				//cout << "\nSent enabled audio successfully.";
			}
			
			// data in mixer has been played now
			Mixer_ReadIdx = Mixer_WriteIdx;
		}
		
		// check if the buffer size changed
		if ( PlayBuffer_Size != NextPlayBuffer_Size )
		{
			PlayBuffer_Size = NextPlayBuffer_Size;
		}

	}
	
	///////////////////////////////////////////////////////////////////////////
	// Update Decode Buffer Offset (for CD L/R,Voice1+Voice3 decode area)
	DecodeBufferOffset += 2;
	DecodeBufferOffset &= ( DecodeBufferSize - 1 );
	
	///////////////////////////////////////////////////////
	// update whether decoding in first/second half of buffer
	//Regs [ ( 0x1f801dae - SPU_X ) >> 1 ] &= ~( 0x200 << 2 );
	//Regs [ ( 0x1f801dae - SPU_X ) >> 1 ] |= ( DecodeBufferOffset & 0x200 ) << 2;
	GET_REG16 ( STAT ) &= ~( 0x200 << 2 );
	GET_REG16 ( STAT ) |= ( DecodeBufferOffset & 0x200 ) << 2;
	
	// write back ChannelOn
	//Regs [ ( ( CON_0 - SPU_X ) >> 1 ) & 0xff ] = (u16) ChannelOn;
	//Regs [ ( ( CON_1 - SPU_X ) >> 1 ) & 0xff ] = (u16) ( ChannelOn >> 16 );
	SET_REG32 ( CON_0, ChannelOn );
	
	// write back reverb on
	// SKIP REVERB FOR NOW
	//Regs [ ( RON_0 - SPU_X ) >> 1 ] = (u16) ReverbOn;
	//Regs [ ( RON_1 - SPU_X ) >> 1 ] = (u16) ( ReverbOn >> 16 );
	//SET_REG32( RON_0, ReverbOn );
}
*/



/*
u32 SPUCore::Read ( u32 Offset )
{
//#ifdef INLINE_DEBUG_READ
//	debug << "\r\nSPU::Read; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
//#endif

	u32 lReg;

	if ( Offset >= ( c_iNumberOfRegisters << 1 ) ) return 0;
	
	// Read SPU register value
	lReg = Offset >> 1;

	switch ( Offset )
	{
		///////////////////////////////////
		// SPU Control
		case CTRL:
#if defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_CTRL
	debug << "\r\nSPU::Read; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
	debug << "; CTRL=" << hex << setw ( 4 ) << Regs [ lReg ];
#endif

			return Regs [ lReg ];
			break;
	
		////////////////////////////////////
		// SPU Status X
		case STAT_X:
#if defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_STAT_0
	debug << "\r\nSPU::Read; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
	debug << "; STAT_0= " << hex << setw ( 4 ) << Regs [ lReg ];
#endif

			// unknown what this is for - gets loaded with 4 when SPU gets initialized by BIOS
			return Regs [ lReg ];
			break;
			
		/////////////////////////////////////
		// SPU Status
		case STAT:
#if defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_STAT_1
	debug << "\r\nSPU::Read; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
	debug << "; STAT_1= " << hex << setw ( 4 ) << Regs [ lReg ];
#endif
			
			// bit 10 - Rd: 0 - SPU ready to transfer; 1 - SPU not ready
			// bit 11 - Dh: 0 - decoding in first half of buffer; 1 - decoding in second half of buffer
			
			return Regs [ lReg ];
			break;
			
		case KON_0:
#if defined INLINE_DEBUG_READ_CHANNELONOFF || defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_KON_0
	debug << "\r\nSPU::Read; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
	debug << "; KON_0= " << hex << setw ( 4 ) << Regs [ lReg ];
#endif

			// just return the SPU register value
			return Regs [ lReg ];
			
			break;

		case KON_1:
#if defined INLINE_DEBUG_READ_CHANNELONOFF || defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_KON_1
	debug << "\r\nSPU::Read; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
	debug << "; KON_1= " << hex << setw ( 4 ) << Regs [ lReg ];
#endif

			// just return the SPU register value
			return Regs [ lReg ];
			
			break;

		case KOFF_0:
#if defined INLINE_DEBUG_READ_CHANNELONOFF || defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_KOFF_0
	debug << "\r\nSPU::Read; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
	debug << "; KOFF_0= " << hex << setw ( 4 ) << Regs [ lReg ];
#endif

			// just return the SPU register value
			return Regs [ lReg ];
			
			break;
			
		case KOFF_1:
#if defined INLINE_DEBUG_READ_CHANNELONOFF || defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_KOFF_1
	debug << "\r\nSPU::Read; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
	debug << "; KOFF_1= " << hex << setw ( 4 ) << Regs [ lReg ];
#endif

			// just return the SPU register value
			return Regs [ lReg ];
			
			break;

		case CON_0:
#if defined INLINE_DEBUG_READ_CHANNELONOFF || defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_CON_0
	debug << "\r\nSPU::Read; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
	debug << "; CON_0= " << hex << setw ( 4 ) << Regs [ lReg ];
#endif

			// just return the SPU register value
			return Regs [ lReg ];
			
			break;
			
		case CON_1:
#if defined INLINE_DEBUG_READ_CHANNELONOFF || defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_CON_1
	debug << "\r\nSPU::Read; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
	debug << "; CON_1= " << hex << setw ( 4 ) << Regs [ lReg ];
#endif

			// just return the SPU register value
			return Regs [ lReg ];
			
			break;
			
			
		default:
#ifdef INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_CON_DEFAULT
	debug << "\r\nSPU::Read; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
	//debug << "; " << _SPU->RegisterNames [ lReg ] << "=";
	debug << " Reg=";
	debug << hex << setw ( 4 ) << Regs [ lReg ];
#endif

			// just return the SPU register value
			// don't AND with 0xff
			return Regs [ lReg ];
			
			break;
			
	}

	
}
*/


/*
void SPUCore::Write ( u32 Address, u32 Data, u32 Mask )
{
//#ifdef INLINE_DEBUG_WRITE
//	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
//#endif

	u32 Channel;
	u32 Rate;
	
	u16 ModeRate;
	
	//////////////////////////////////////////////////////////////////////
	// *** TODO *** WRITES TO ODD ADDRESSES ARE IGNORED
	if ( Address & 1 ) return;
	
	////////////////////////////////////////////////////////////////////
	// *** TODO *** PROPERLY IMPLEMENT 8-BIT WRITES WITH 16-BIT VALUES
		
	// make sure the address is in the correct range
	//if ( Address < Regs_Start || Address > Regs_End ) return;
	
	if ( Address >= ( SPU_X + ( c_iNumberOfRegisters << 1 ) ) ) return;
	
	// Write SPU register value
	
	
	///////////////////////////////////////////////////////////////////////////////////////////////////
	// ***TODO*** 32-bit writes are probably treated as 16-bit writes on hardware

	////////////////////////////////////////////
	// writes to SPU are 16 bit writes
	Data &= 0xffff;
	
	if ( Mask != 0xffff ) cout << "\nhps1x64 ALERT: SPU::Write Mask=" << hex << Mask;
	
	////////////////////////////////////////////////////////////////////
	// Check if this is the voice data area (0x1f801c00-0x1f801d7f)
	// Voice Data area #1 for PS2 is offsets 0-0x180
	if ( ( ( Address >> 4 ) & 0xff ) <= 0xd7 )
	{
		///////////////////////////////////////////////////////////////////
		// this is the voice data area
		
		Channel = ( ( Address >> 4 ) & 0xff ) - 0xc0;
		
//#ifdef INLINE_DEBUG_WRITE
//	debug << "; Channel#" << Channel;
//#endif

		///////////////////////////////////////////////////////////////
		// Determine what register this is for
		switch ( Address & 0xf )
		{
			case VOL_L:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_VOL_L
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; Channel#" << Channel;
	debug << "; (before)VOL_L=" << _SPU->Regs [ ( ( Channel << 4 ) + VOL_L ) >> 1 ];
#endif
			
				// writing constant volume
				_SPU->Regs [ ( ( Channel << 4 ) + VOL_L ) >> 1 ] = Data;
				
				if ( Data >> 15 )
				{
					Start_VolumeEnvelope ( _SPU->VOL_L_Value [ Channel ], _SPU->VOL_L_Cycles [ Channel ], Data & 0x7f, ( Data >> 13 ) & 0x3 );
				}
				else
				{
					// store the new current volume left
					_SPU->Regs [ ( CVOL_L_START - SPU_X + ( Channel << 1 ) ) >> 1 ] = Data << 1;
				}
	
			
				break;
				
			case VOL_R:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_VOL_R
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; Channel#" << Channel;
	debug << "; (before)VOL_R=" << Regs [ ( ( Channel << 4 ) + VOL_R ) >> 1 ];
#endif
			
				// writing constant volume
				Regs [ ( ( Channel << 4 ) + VOL_R ) >> 1 ] = Data;

				if ( Data >> 15 )
				{
					Start_VolumeEnvelope ( VOL_R_Value [ Channel ], VOL_R_Cycles [ Channel ], Data & 0x7f, ( Data >> 13 ) & 0x3 );
				}
				else
				{
					// store the new current volume left
					Regs [ ( CVOL_R_START - SPU_X + ( Channel << 1 ) ) >> 1 ] = Data << 1;
				}
				
			
				break;
				
			case PITCH:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_PITCH
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; Channel#" << Channel;
	debug << "; (before)PITCH=" << Regs [ ( ( Channel << 4 ) + PITCH ) >> 1 ];
#endif
			
				// only bits 0-13 are used
				Data &= 0x3fff;
				
				/////////////////////////////////////////////////////////////
				// Pitch of the sound
				// frequency = (pitch/4096) * f0
				Regs [ ( ( Channel << 4 ) + PITCH ) >> 1 ] = Data;
				

				// (32.32)dSampleDT = Pitch / 4096
				// for PS2 this would be (32.32)dSampleDT = ( 48 / SampleRateInKHZ ) * ( Pitch / 4096 )
				// where SampleRateInKHZ is 41 for a 41000 Hz sample rate
				// *** testing *** copy pitch over on key on
				//_SPU->dSampleDT [ Channel ] = ( ((u64)Data) << 32 ) >> 12;
				
				break;
				
			case SSA_X:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_SSA_X
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; Channel#" << Channel;
	debug << "; (before)SSA_X=" << Regs [ ( ( Channel << 4 ) + SSA_X ) >> 1 ];
#endif

				// align ??
				//Data = ( Data + 1 ) & ~1;
				Data &= ~1;
			
				////////////////////////////////////////////
				// writing start address for the sound
				Regs [ ( ( Channel << 4 ) + SSA_X ) >> 1 ] = Data;
				
				// *** testing ***
				//_SPU->Regs [ ( ( Channel << 4 ) + LSA_X ) >> 1 ] = Data;
				
				break;
				
			case ADSR_0:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_ADSR_0
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; Channel#" << Channel;
	debug << "; (before)ADSR_0=" << Regs [ ( ( Channel << 4 ) + ADSR_0 ) >> 1 ];
#endif
			
				//////////////////////////////////////////////
				// writing ADSR_0 register
				Regs [ ( ( Channel << 4 ) + ADSR_0 ) >> 1 ] = Data;
				
				// set sustain level (48.16 fixed point)
				// this is now 16.0 fixed point
				VOL_SUSTAIN_Level [ Channel ] = ( ( Data & 0xf ) + 1 ) << ( 11 );
				
				// set decay rate
				Set_ExponentialDecRates ( Channel, ( ( Data >> 4 ) & 0xf ) << 2, (s32*) VOL_DECAY_Constant, (s32*) VOL_DECAY_Constant );
				
				// set attack rate
				// need to know if this is linear or pseudo exponential increase
				if ( Data >> 15 )
				{
					/////////////////////////////////////////////////////////
					// pseudo exponential increase
					Set_ExponentialIncRates ( Channel, ( Data >> 8 ) & 0x7f, (s32*) VOL_ATTACK_Constant, (s32*) VOL_ATTACK_Constant75 );
				}
				else
				{
					/////////////////////////////////////////////////////////////
					// linear increase
					Set_LinearIncRates ( Channel, ( Data >> 8 ) & 0x7f, (s32*) VOL_ATTACK_Constant, (s32*) VOL_ATTACK_Constant75 );
				}
				
				break;
				
			case ADSR_1:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_ADSR_1
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; Channel#" << Channel;
	debug << "; (before)ADSR_1=" << Regs [ ( ( Channel << 4 ) + ADSR_1 ) >> 1 ];
#endif
			
				///////////////////////////////////////////////////
				// writing ADSR_1 register
				Regs [ ( ( Channel << 4 ) + ADSR_1 ) >> 1 ] = Data;
				
				// set release rate
				// check if release rate is linear or exponential
				if ( ( Data >> 5 ) & 1 )
				{
					/////////////////////////////////////////////////
					// exponential decrease
					Set_ExponentialDecRates ( Channel, ( Data & 0x1f ) << 2, (s32*) VOL_RELEASE_Constant, (s32*) VOL_RELEASE_Constant );
				}
				else
				{
					/////////////////////////////////////////////////
					// linear decrease
					Set_LinearDecRates ( Channel, ( Data & 0x1f ) << 2, (s32*) VOL_RELEASE_Constant, (s32*) VOL_RELEASE_Constant );
				}
				
				// set sustain rate
				switch ( ( Data >> 14 ) & 3 )
				{
					case 0:
					
						//////////////////////////////////////////////////////////////////
						// linear increase
						Set_LinearIncRates ( Channel, ( Data >> 6 ) & 0x7f, (s32*) VOL_SUSTAIN_Constant, (s32*) VOL_SUSTAIN_Constant75 );
						
						break;
					
					case 1:
					
						////////////////////////////////////////////////////////////////////
						// linear decrease
						Set_LinearDecRates ( Channel, ( Data >> 6 ) & 0x7f, (s32*) VOL_SUSTAIN_Constant, (s32*) VOL_SUSTAIN_Constant75 );
						
						break;
					
					case 2:
					
						/////////////////////////////////////////////////////////////////////
						// pseudo exponential increase
						Set_ExponentialIncRates ( Channel, ( Data >> 6 ) & 0x7f, (s32*) VOL_SUSTAIN_Constant, (s32*) VOL_SUSTAIN_Constant75 );
						
						break;
					
					case 3:
					
						/////////////////////////////////////////////////////////
						// exponential decrease
						Set_ExponentialDecRates ( Channel, ( Data >> 6 ) & 0x7f, (s32*) VOL_SUSTAIN_Constant, (s32*) VOL_SUSTAIN_Constant75 );
						
						break;
				}
				
				break;
				
			case ENV_X:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_ENV_X
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; Channel#" << Channel;
	debug << "; (before)ENV_X=" << Regs [ ( ( Channel << 4 ) + ENV_X ) >> 1 ];
#endif
			
				////////////////////////////////////////////////
				// writing to current envelope volume register
				
				// *** TODO *** this can actually be set to cause a jump in the value //
				//if ( Data > 0x7fff ) Data = 0x7fff;
				Regs [ ( ( Channel << 4 ) + ENV_X ) >> 1 ] = Data;
				
				// *** testing *** allow for jump in adsr value at any time
				VOL_ADSR_Value [ Channel ] = Data;
				
				break;
				
			case LSA_X:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_LSA_X
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; Channel#" << Channel;
	debug << "; (before)LSA_X=" << Regs [ ( ( Channel << 4 ) + LSA_X ) >> 1 ];
	debug << "; CyclesFromStart=" << dec << ( CycleCount - StartCycle_Channel [ Channel ] );
#endif
			
				// align ??
				//Data = ( Data + 1 ) & ~1;
				Data &= ~1;
				
				///////////////////////////////////////////////////////
				// writing to loop start address for sound
				// this gets set by the sound when it is loaded
				Regs [ ( ( Channel << 4 ) + LSA_X ) >> 1 ] = Data;
				
				// loop start was manually specified - so ignore anything specified in sample
				LSA_Manual_Bitmap |= ( 1 << Channel );
				
				break;
			
		}
	}
	else
	{
	
		/////////////////////////////////////////////////////////////
		// not writing to voice data area
	
		switch ( Address )
		{
			// reverb work address start
			case RVWAS_0:
			
				//ReverbWork_Start = ( Data & 0xffff ) << 3;
				//ReverbWork_Size = c_iRam_Size - _SPU->ReverbWork_Start;
				//Reverb_BufferAddress = _SPU->ReverbWork_Start;
				
				Regs [ RVWAS_0 >> 1 ] = (u16)Data;
				
				// *** TODO ***  make some sort of call to a "UpdateReverbWorkAddress" or something
				
				// check if interrupt triggered by reverb work address
				if ( ( Reverb_BufferAddress == ( ( (u32) Regs [ IRQA_0 >> 1 ] ) << 3 ) ) && ( Regs [ CTRL >> 1 ] & 0x40 ) )
				{
					// we have reached irq address - trigger interrupt
					SetInterrupt ();
					
					// interrupt
					Regs [ STAT >> 1 ] |= 0x40;
				}
				
				break;
			
			case RVWAS_1:
			
				//ReverbWork_Start = ( Data & 0xffff ) << 3;
				//ReverbWork_Size = c_iRam_Size - _SPU->ReverbWork_Start;
				//Reverb_BufferAddress = _SPU->ReverbWork_Start;
				
				Regs [ RVWAS_1 >> 1 ] = (u16)Data;
				
				// *** TODO ***  make some sort of call to a "UpdateReverbWorkAddress" or something
				
				// check if interrupt triggered by reverb work address
				if ( ( Reverb_BufferAddress == ( ( (u32) Regs [ IRQA_0 >> 1 ] ) << 3 ) ) && ( Regs [ CTRL >> 1 ] & 0x40 ) )
				{
					// we have reached irq address - trigger interrupt
					SetInterrupt ();
					
					// interrupt
					Regs [ STAT >> 1 ] |= 0x40;
				}
				
				break;
			
			//////////////////////////////////////////////////////////////////////////////
			// irq address - reading this address in sound buffer causes SPU interrupt
			case IRQA_0:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_IRQA
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)IRQA_0=" << Regs [ IRQA_0 >> 1 ];
#endif

				Regs [ IRQA_0 >> 1 ] = (u16)Data;
				
				break;
			
			case IRQA_1:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_IRQA
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)IRQA_1=" << Regs [ IRQA_1 >> 1 ];
#endif

				Regs [ IRQA_1 >> 1 ] = (u16)Data;
				
				break;
			
			////////////////////////////////////////
			// Sound Buffer Address
			case SBA_0:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_SBA
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)SBA=" << Regs [ SBA_0 >> 1 ];
#endif
			
				///////////////////////////////////
				// set next sound buffer address
				NextSoundBufferAddress = ( Data << 3 ) & c_iRam_Mask;
				Regs [ SBA_0 >> 1 ] = (u16)Data;

				break;

			////////////////////////////////////////
			// Sound Buffer Address
			case SBA_1:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_SBA
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)SBA=" << Regs [ SBA_1 >> 1 ];
#endif
			
				///////////////////////////////////
				// set next sound buffer address
				NextSoundBufferAddress = ( Data << 3 ) & c_iRam_Mask;
				Regs [ SBA_1 >> 1 ] = (u16)Data;

				break;
				
			//////////////////////////////////////////
			// Data forwarding register
			case DATA:	//0x1f801da8
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_DATA
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; DATA; BufferIndex=" << BufferIndex;
#endif
			
				///////////////////////////////////////////////////////
				// Send data to sound buffer and update next address
				//RAM [ ( ( NextSoundBufferAddress & c_iRam_Mask ) >> 1 ) ] = (u16) Data;
				//NextSoundBufferAddress += 2;
				
				///////////////////////////////////////////////////////////////
				// Actually we're supposed to send the data into SPU buffer
				
				// buffer can be written into at any time
				if ( BufferIndex < 32 )
				{
					Buffer [ BufferIndex++ ] = (u16) Data;
				}
				
				break;
				
			///////////////////////////////////
			// SPU Control
			case CTRL:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_CTRL
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)CTRL=" << _SPU->Regs [ CTRL >> 1 ];
#endif

				Regs [ CTRL >> 1 ] = (u16)Data;
				
				// copy bits 0-5 to stat
				Regs [ STAT >> 1 ] = ( Regs [ STAT >> 1 ] & 0xffc0 ) | ( Regs [ CTRL >> 1 ] & 0x3f );
				
				// copy bit 5 of ctrl to bit 7 of stat
				switch ( ( Regs [ CTRL >> 1 ] >> 5 ) & 0x3 )
				{
					case 0:
						// no reads or writes (stop)
						Regs [ STAT >> 1 ] = ( Regs [ STAT >> 1 ] & ~0x0380 ) | ( 0 << 7 );
						break;
						
					case 1:
						// manual write
						Regs [ STAT >> 1 ] = ( Regs [ STAT >> 1 ] & ~0x0380 ) | ( 0 << 7 );
						break;
						
					case 2:
						// dma write
						Regs [ STAT >> 1 ] = Regs [ STAT >> 1 ] & ~0x0380 ) | ( 0x3 << 7 );
						break;
						
					case 3:
						// dma read
						Regs [ STAT >> 1 ] = ( Regs [ STAT >> 1 ] & ~0x0380 ) | ( 0x5 << 7 );
						break;
				}
				
				// check if disabling/acknowledging interrupt
				if ( ! ( Data & 0x40 ) )
				{
					// clear interrupt
					Regs [ STAT >> 1 ] &= ~0x40;
				}
				
				///////////////////////////////////////////////////////////////////////////
				// if DMA field was written as 01 then write SPU buffer into sound RAM
				if ( ( Data & 0x30 ) == 0x10 )
				{
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_CTRL
	debug << "; MANUAL WRITE";
#endif

					///////////////////////////////////////////////////////
					// write SPU buffer into sound ram
					for ( int i = 0; i < BufferIndex; i++ )
					{
						_SPU->RAM [ ( ( _SPU->NextSoundBufferAddress + ( i << 1 ) ) & c_iRam_Mask ) >> 1 ] = Buffer [ i ];
					}
					
					//////////////////////////////////////////////////////////
					// update next sound buffer address
					//_SPU->NextSoundBufferAddress += 64;
					NextSoundBufferAddress += ( BufferIndex << 1 );
					
					//////////////////////////////////////////////////
					// reset buffer index
					BufferIndex = 0;
					
					////////////////////////////////////////////////////////////
					// save back into the sound buffer address register
					// sound buffer address register does not change
					//_SPU->Regs [ ( ( REG_SoundBufferAddress - SPU_X ) >> 1 ) & 0xff ] = (u16) (_SPU->NextSoundBufferAddress >> 3);

					// *** testing ***
					//_SPU->SpuTransfer_Complete ();
				}
				

				break;
		
			////////////////////////////////////
			// SPU Status
			case INIT:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_INIT
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)INIT=" << Regs [ INIT >> 1 ];
#endif
				// unknown what this is for - gets loaded with 4 when SPU gets initialized by BIOS
				Regs [ INIT >> 1 ] = (u16)Data;
				

				break;
				
			/////////////////////////////////////
			// SPU Status
			case STAT:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_STAT
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)STAT=" << _SPU->Regs [ STAT >> 1 ];
#endif
				
				// bit 10 - Rd: 0 - SPU ready to transfer; 1 - SPU not ready
				// bit 11 - Dh: 0 - decoding in first half of buffer; 1 - decoding in second half of buffer
				//_SPU->Regs [ ( ( Address - SPU_X ) >> 1 ) & 0xff ] = (u16)Data;
				
				break;
				
				
			case MVOL_L:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_MVOL_L
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)MVOL_L=" << Regs [ MVOL_L >> 1 ];
#endif
			
				////////////////////////////////////////
				// Master Volume Left
				Regs [ MVOL_L >> 1 ] = (u16) Data;
				
				if ( Data >> 15 )
				{
					Start_VolumeEnvelope ( MVOL_L_Value, MVOL_L_Cycles, Data & 0x7f, ( Data >> 13 ) & 0x3 );
				}
				else
				{
					// store the new current volume left
					Regs [ MVOLX_L >> 1 ] = Data << 1;
				}
				
				break;
				
			case MVOL_R:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_MVOL_R
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)MVOL_R=" << Regs [ MVOL_R >> 1 ];
#endif
			
				////////////////////////////////////////
				// Master Volume Right
				Regs [ MVOL_R >> 1 ] = (u16) Data;
				
				if ( Data >> 15 )
				{
					Start_VolumeEnvelope ( MVOL_R_Value, MVOL_R_Cycles, Data & 0x7f, ( Data >> 13 ) & 0x3 );
				}
				else
				{
					// store the new current volume left
					Regs [ MVOLX_R >> 1 ] = Data << 1;
				}
				
				
				break;
				
			case EVOL_L:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_EVOL_L
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)EVOL_L=" << Regs [ EVOL_L >> 1 ];
#endif
			
				////////////////////////////////////////
				// Effect Volume Left
				Regs [ EVOL_L >> 1 ] = (u16) Data;
				
				break;
				
			case EVOL_R:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_EVOL_R
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)EVOL_R=" << Regs [ EVOL_R >> 1 ];
#endif
			
				////////////////////////////////////////
				// Effect Volume Right
				Regs [ EVOL_R >> 1 ] = (u16) Data;
				
				break;
				
			case KON_0:
#if defined INLINE_DEBUG_WRITE_CHANNELONOFF
			debug << "\r\nKON_0; Data=" << hex << setw ( 4 ) << Data << " (Before) KON_0=" << Regs [ KON_0 >> 1 ] << " KOFF_0=" << Regs [ KOFF_0 >> 1 ] << " CON_0=" << Regs [ CON_0 >> 1 ];
#endif
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_KON_0
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)KON_0=" << Regs [ KON_0 >> 1 ];
#endif
			
				/////////////////////////////////////////////
				// Key On 0-15
				
				// when keyed on set channel ADSR to attack mode
				for ( Channel = 0; Channel < 16; Channel++ )
				{
					if ( ( 1 << Channel ) & Data )
					{
						Start_SampleDecoding ( Channel );
					}
				}
				
				// store to SPU register
				// just set value
				//_SPU->Regs [ ( KON_0 - SPU_X ) >> 1 ] |= (u16) Data;
				Regs [ KON_0 >> 1 ] = (u16) Data;
				
				// clear channel in key off
				// don't touch this here
				//_SPU->Regs [ ( KOFF_0 - SPU_X ) >> 1 ] &= ~((u16) Data);
				
				// logical Or with channel on/off register
				// unknown if channel on/off register is inverted or not
				// actually this has nothing to do with a channel being on or anything like that, so clear the bits
				Regs [ CON_0 >> 1 ] &= ~( (u16) Data );
				
#if defined INLINE_DEBUG_WRITE_CHANNELONOFF
			debug << "; (After) KON_0=" << hex << setw ( 4 ) << _SPU->Regs [ ( KON_0 >> 1 ) ] << " KOFF_0=" << _SPU->Regs [ KOFF_0 >> 1 ] << " CON_0=" << _SPU->Regs [ CON_0 >> 1 ];
#endif
				break;
				
			case KON_1:
#ifdef INLINE_DEBUG_WRITE_CHANNELONOFF
			debug << "\r\nKON_1; Data=" << hex << setw ( 4 ) << Data << " (Before) KON_1=" << Regs [ KON_1 >> 1 ] << " KOFF_1=" << Regs [ KOFF_1 >> 1 ] << " CON_1=" << Regs [ CON_1 >> 1 ];
#endif
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_KON_1
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)KON_1=" << Regs [ KON_1 >> 1 ];
#endif
				/////////////////////////////////////////////
				// Key on 16-23
				
				////////////////////////////////////////////////////////////////////////
				// upper 8 bits of register are zero and ignored
				Data &= 0xff;
				
				// on key on we need to change ADSR mode to attack mode
				for ( Channel = 16; Channel < 24; Channel++ )
				{
					if ( ( 1 << ( Channel - 16 ) ) & Data )
					{
						Start_SampleDecoding ( Channel );
					}
				}
				
				// store to SPU register
				// just set value
				Regs [ KON_1 >> 1 ] = (u16) Data;

				// logical Or with channel on/off register
				// this isn't channel on/off.. clear the bits
				Regs [ CON_1 >> 1 ] &= ~( (u16) Data );
				
#ifdef INLINE_DEBUG_WRITE_CHANNELONOFF
			debug << "; (After) KON_1=" << hex << setw ( 4 ) << _SPU->Regs [ KON_1 >> 1 ] << " KOFF_1=" << _SPU->Regs [ KOFF_1 >> 1 ] << " CON_1=" << _SPU->Regs [ CON_1 >> 1 ];
#endif
				break;
			
				
				
			
				
			case KOFF_0:
#ifdef INLINE_DEBUG_WRITE_CHANNELONOFF
			debug << "\r\nKOFF_0; Data=" << hex << setw ( 4 ) << Data << "(Before) KON_0=" << Regs [ KON_0 >> 1 ] << " KOFF_0=" << Regs [ KOFF_0 >> 1 ] << " CON_0=" << Regs [ CON_0 >> 1 ];
#endif
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_KOFF_0
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)KOFF_0=" << _SPU->Regs [ KOFF_0 >> 1 ];
#endif
			
				/////////////////////////////////////////////
				// Key off 0-15
				
				// on key off we need to change ADSR mode to release mode
				for ( Channel = 0; Channel < 16; Channel++ )
				{
					if ( ( 1 << Channel ) & Data )
					{
						// put channel in adsr release phase unconditionally
						_SPU->ADSR_Status [ Channel ] = ADSR_RELEASE;
						
						// start envelope for release mode
						ModeRate = _SPU->Regs [ ( ADSR_1 >> 1 ) + ( Channel << 3 ) ];
						Start_VolumeEnvelope ( _SPU->VOL_ADSR_Value [ Channel ], _SPU->Cycles [ Channel ], ( ModeRate & 0x1f ) << ( 2 ), ( ( ( ModeRate >> 5 ) & 1 ) << 1 ) | 0x1 );
						
						// loop address not manually specified
						// can still change loop address after sound starts
						_SPU->LSA_Manual_Bitmap &= ~( 1 << Channel );
					}
				}
				
				// store to SPU register
				// just write value
				Regs [ KOFF_0 >> 1 ] = (u16) Data;
				
				// clear channel in key on
				// note: don't touch key on
				//_SPU->Regs [ ( KON_0 - SPU_X ) >> 1 ] &= ~((u16) Data);

				// temp: for now, also make sure channel is on when set to key off
				//_SPU->Regs [ ( CON_0 - SPU_X ) >> 1 ] |= (u16) Data;
				
#ifdef INLINE_DEBUG_WRITE_CHANNELONOFF
			debug << "; (After) KON_0=" << hex << setw ( 4 ) << _SPU->Regs [ ( KON_0 >> 1 ) ] << " KOFF_0=" << _SPU->Regs [ KOFF_0 >> 1 ] << "CON_0=" << _SPU->Regs [ CON_0 >> 1 ];
#endif
				
				break;
				
			case KOFF_1:
#ifdef INLINE_DEBUG_WRITE_CHANNELONOFF || defined INLINE_DEBUG_WRITE
			debug << "\r\nKOFF_1; Data=" << hex << setw ( 4 ) << Data << " (Before) KON_1=" << _SPU->Regs [ ( KON_1 >> 1 ) ] << " KOFF_1=" << _SPU->Regs [ KOFF_1 >> 1 ] << " CON_1=" << _SPU->Regs [ CON_1 >> 1 ];
#endif
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_KOFF_1
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)KOFF_1=" << _SPU->Regs [ KOFF_1 >> 1 ];
#endif
			
				/////////////////////////////////////////////
				// Key off 16-23

				////////////////////////////////////////////////////////////////////////
				// upper 8 bits of register are zero and ignored
				Data &= 0xff;
				
				// on key off we need to change ADSR mode to release mode
				for ( Channel = 16; Channel < 24; Channel++ )
				{
					if ( ( 1 << ( Channel - 16 ) ) & Data )
					{
						// put channel in adsr release phase unconditionally
						_SPU->ADSR_Status [ Channel ] = ADSR_RELEASE;
						
						// start envelope for release mode
						ModeRate = Regs [ ( ADSR_1 >> 1 ) + ( Channel << 3 ) ];
						Start_VolumeEnvelope ( _SPU->VOL_ADSR_Value [ Channel ], _SPU->Cycles [ Channel ], ( ModeRate & 0x1f ) << ( 2 ), ( ( ( ModeRate >> 5 ) & 1 ) << 1 ) | 0x1 );
						
						// loop address not manually specified
						// can still change loop address after sound starts
						_SPU->LSA_Manual_Bitmap &= ~( 1 << Channel );
					}
				}
				
				// store to SPU register (upper 24 bits of register are zero)
				// just write value
				//_SPU->Regs [ ( KOFF_1 - SPU_X ) >> 1 ] |= (u16) Data;
				Regs [ KOFF_1 >> 1 ] = (u16) Data;

#ifdef INLINE_DEBUG_WRITE_CHANNELONOFF
			debug << "; (After) KON_1=" << hex << setw ( 4 ) << _SPU->Regs [ ( KON_1 >> 1 ) ] << " KOFF_1=" << _SPU->Regs [ KOFF_1 >> 1 ] << " CON_1=" << _SPU->Regs [ CON_1 >> 1 ];
#endif
				break;
			
				
			case CON_0:
#ifdef INLINE_DEBUG_WRITE_CHANNELONOFF
			debug << "\r\nCON_0; Data=" << hex << setw ( 4 ) << Data << "(Before) KON_0=" << Regs [ KON_0 >> 1 ] << " KOFF_0=" << Regs [ KOFF_0 >> 1 ] << " CON_0=" << Regs [ CON_0 >> 1 ];
#endif
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_CON_0
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)CON_0=" << Regs [ CON_0 >> 1 ];
#endif

				///////////////////////////////////////////////////////
				// *todo* this register does not get written to
				
				// logical Or with channel on/off register
				// more of a read-only register. modifying it only changes the value momentarily, then it gets set back
				// should set register to zero
				Regs [ CON_0 >> 1 ] = 0;	//(u16) Data;
				
#ifdef INLINE_DEBUG_WRITE_CHANNELONOFF
			debug << "; (After) KON_0=" << hex << setw ( 4 ) << Regs [ KON_0 >> 1 ] << " KOFF_0=" << Regs [ KOFF_0 >> 1 ] << " CON_0=" << Regs [ CON_0 >> 1 ];
#endif
				break;
			
			case CON_1:
#ifdef INLINE_DEBUG_WRITE_CHANNELONOFF
			debug << "\r\nCON_1; Data=" << hex << setw ( 4 ) << Data << " (Before) KON_1=" << Regs [ KON_1 >> 1 ] << " KOFF_1=" << Regs [ KOFF_1 >> 1 ] << " CON_1=" << Regs [ CON_1 >> 1 ];
#endif
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_CON_1
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	debug << "; (before)CON_1=" << Regs [ CON_1 >> 1 ];
#endif

				///////////////////////////////////////////////////////
				// *todo* this register does not get written to
			
				////////////////////////////////////////////////////////////////////////
				// upper 8 bits of register are zero and ignored
				//Data &= 0xff;
				
				// logical Or with channel on/off register
				// more of a read-only register. modifying it only changes the value momentarily, then it gets set back
				// should set register to zero
				Regs [ CON_1 >> 1 ] = 0;	//(u16) Data;
				
#ifdef INLINE_DEBUG_WRITE_CHANNELONOFF
			debug << "; (After) KON_1=" << hex << setw ( 4 ) << Regs [ KON_1 >> 1 ] << " KOFF_1=" << Regs [ KOFF_1 >> 1 ] << " CON_1=" << Regs [ CON_1 >> 1 ];
#endif
				break;
				
			case MVOLX_L:
			
				// current master volume left //
				MVOL_L_Value = ((s32) ((s16) Data)) << 16;
				Regs [ MVOLX_L >> 1 ] = Data;
				
				break;
				
			case MVOLX_R:
			
				// current master volume right //
				MVOL_R_Value = ((s32) ((s16) Data)) << 16;
				Regs [ MVOLX_R >> 1 ] = Data;
			
				break;
				
			default:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_DEFAULT
	debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Writing: " << Data;
	//debug << "; (before)" << _SPU->RegisterNames [ Offset >> 1 ] << "=";
	debug << " Reg=" << hex << Regs [ Offset >> 1 ];
#endif


				/////////////////////////////////////////////
				// by default just store to SPU regs
				_SPU->Regs [ Offset >> 1 ] = (u16) Data;
				break;
		}
	}
}
*/




bool SPUCore::DMA_ReadyForRead ( void )
{
	return true;
}


bool SPUCore::DMA_ReadyForWrite ( void )
{
	return true;
}


void SPUCore::DMA_Read ( u32* Data, int ByteReadCount )
{
	u32 Output;
	
	Output = ((u32*)RAM) [ ( ( NextSoundBufferAddress & c_iRam_Mask ) >> 2 ) ];

	//////////////////////////////////////////////////////////
	// update next sound buffer address
	NextSoundBufferAddress += 4;
	NextSoundBufferAddress &= c_iRam_Mask;
	
	////////////////////////////////////////////////////////////
	// save back into the sound buffer address register
	// this value does not update with transfer
	//Regs [ ( ( REG_SoundBufferAddress - SPU_X ) >> 1 ) & 0xff ] = NextSoundBufferAddress >> 3;
	
	Data [ 0 ] = Output;
}


void SPUCore::DMA_ReadBlock ( u32* Data, u32 BS )
{
	// TODO: check for interrupt (should interrupt on transfers TO and FROM spu ram //
	
	for ( int i = 0; i < BS; i++ )
	{
		Data [ i ] = ((u32*)RAM) [ ( ( NextSoundBufferAddress & c_iRam_Mask ) >> 2 ) ];

		// check if address is set to trigger interrupt
		//if ( ( NextSoundBufferAddress == ( ( (u32) Regs [ ( IRQA - SPU_X ) >> 1 ] ) << 3 ) ) && ( Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] & 0x40 ) )
		if ( ( NextSoundBufferAddress == ( GET_REG32 ( IRQA_0 ) ) ) && ( GET_REG16 ( CTRL ) & 0x40 ) )
		{
			// we have reached irq address - trigger interrupt
			SetInterrupt ();
			
			// interrupt
			//Regs [ ( 0x1f801dae - SPU_X ) >> 1 ] |= 0x40;
			GET_REG16 ( STAT ) |= 0x40;
		}
		
		//////////////////////////////////////////////////////////
		// update next sound buffer address
		NextSoundBufferAddress += 4;
		NextSoundBufferAddress &= c_iRam_Mask;
	}
}


void SPUCore::DMA_Write ( u32* Data, int BlockSizeInWords32 )
{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "\r\nDMA_Write: ";
	debug << "(before) NextSoundBufferAddress=" << hex << NextSoundBufferAddress;
#endif

	///////////////////////////////////////////////////////////////
	// Actually we're supposed to send the data into SPU buffer
	((u32*)Buffer) [ BufferIndex >> 1 ] = Data [ 0 ];
	BufferIndex += 2;
	
	// TODO: check for interrupt (should interrupt on transfers TO and FROM spu ram //

	if ( _SPUCore->BufferIndex >= 32 )
	{
		///////////////////////////////////////////////////////
		// write SPU buffer into sound ram
		for ( int i = 0; i < 16; i++ ) ((u32*)RAM) [ ( ( NextSoundBufferAddress & c_iRam_Mask ) >> 2 ) + i ] = ((u32*)Buffer) [ i ];

		//////////////////////////////////////////////////
		// reset buffer index
		BufferIndex = 0;

		//////////////////////////////////////////////////////////
		// update next sound buffer address
		NextSoundBufferAddress += 64;
		NextSoundBufferAddress &= c_iRam_Mask;

		////////////////////////////////////////////////////////////
		// save back into the sound buffer address register
		//Regs [ ( ( REG_SoundBufferAddress - SPU_X ) >> 1 ) & 0xff ] = NextSoundBufferAddress >> 3;
	}
	
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "; (after) NextSoundBufferAddress=" << NextSoundBufferAddress;
#endif
}


void SPUCore::DMA_Write_Block ( u32* Data, u32 BS )
{
#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "\r\nDMA_Write: ";
	debug << " Cycle#" << dec << *_DebugCycleCount;
	debug << " (before) NextSoundBufferAddress=" << hex << NextSoundBufferAddress << " BS=" << BS;
#endif
	
	// TODO: check for interrupt (should interrupt on transfers TO and FROM spu ram //
	
	//for ( int i = 0; i < 32; i++ )
	for ( int i = 0; i < (BS << 1); i++ )
	{
		// write the data into sound RAM
		RAM [ ( NextSoundBufferAddress + i ) & ( c_iRam_Mask >> 1 ) ] = ((u16*) Data) [ i ];
		
#ifdef INLINE_DEBUG_DMA_WRITE_RECORD
	debug << " " << ((u16*) Data) [ i ];
#endif

		// check for interrupt
		// check if address is set to trigger interrupt
		//if ( ( ( ( NextSoundBufferAddress + ( i << 1 ) ) & c_iRam_Mask ) == ( ( (u32) Regs [ ( IRQA - SPU_X ) >> 1 ] ) << 3 ) ) && ( Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] & 0x40 ) )
		if ( ( ( ( NextSoundBufferAddress + i ) & ( c_iRam_Mask >> 1 ) ) == ( GET_REG32 ( IRQA_0 ) ) ) && ( GET_REG16 ( CTRL ) & 0x40 ) )
		{
			// we have reached irq address - trigger interrupt
			SetInterrupt ();
			
			// interrupt
			//Regs [ ( 0x1f801dae - SPU_X ) >> 1 ] |= 0x40;
			GET_REG16 ( STAT ) |= 0x40;
		}
	}
	
	//////////////////////////////////////////////////
	// reset buffer index
	BufferIndex = 0;

	//////////////////////////////////////////////////////////
	// update next sound buffer address
	//NextSoundBufferAddress += ( BS << 2 );
	NextSoundBufferAddress += ( BS << 1 );
	
	//NextSoundBufferAddress &= c_iRam_Mask;
	NextSoundBufferAddress &= ( c_iRam_Mask >> 1 );
	
	// make sure address is aligned to sound block
	NextSoundBufferAddress &= ~7;

	////////////////////////////////////////////////////////////
	// save back into the sound buffer address register
	// this value does not change with transfer
	//Regs [ ( REG_SoundBufferAddress - SPU_X ) >> 1 ] = NextSoundBufferAddress >> 3;
	
	// *** testing ***
	// transfer is not ultimately complete at this point -> call this when dma_finished is called
	//SpuTransfer_Complete ();

#ifdef INLINE_DEBUG_DMA_WRITE
	debug << "; (after) NextSoundBufferAddress=" << NextSoundBufferAddress;
#endif
}



void SPUCore::RunNoiseGenerator ()
{
	u32 NoiseStep, NoiseShift, ParityBit;
	
	//NoiseStep = ( ( Regs [ ( CTRL - SPU_X ) >> 1 ] >> 8 ) & 0x3 ) + 4;
	NoiseStep = ( ( GET_REG16 ( CTRL ) >> 8 ) & 0x3 ) + 4;
	//NoiseShift = ( ( Regs [ ( CTRL - SPU_X ) >> 1 ] >> 10 ) & 0xf );
	NoiseShift = ( ( GET_REG16 ( CTRL ) >> 10 ) & 0xf );
	
	Timer -= NoiseStep;
	ParityBit = ( ( NoiseLevel >> 15 ) ^ ( NoiseLevel >> 12 ) ^ ( NoiseLevel >> 11 ) ^ ( NoiseLevel >> 10 ) ^ 1 ) & 1;
	if ( Timer < 0 ) NoiseLevel = ( NoiseLevel << 1 ) + ParityBit;
	if ( Timer < 0 ) Timer += ( 0x20000 >> NoiseShift );
	if ( Timer < 0 ) Timer += ( 0x20000 >> NoiseShift );
}


// this function is not used anymore
/*
static void SPUCore::SetSweepVars ( u16 flags, u32 Channel, s32* Rates, s32* Rates75 )
{
	u32 Rate;
	
	// also set constants if volume is sweeping
	if ( flags >> 15 )
	{
		Rate = ( flags & 0x7f );

		// writing sweep value - check mode
		switch ( ( flags >> 13 ) & 0x3 )
		{
			case 0:
			
				//////////////////////////////////////////
				// writing Linear increment mode

				// to keep this simple this needs to be an inline function - just sets correct constants for linear increment mode
				Set_LinearIncRates ( Channel, Rate, Rates, Rates75 );
				
				break;
				
			case 1:
			
				//////////////////////////////////////////
				// writing Linear decrement mode
				
				Set_LinearDecRates ( Channel, Rate, Rates, Rates75 );
				
				break;
				
			case 2:
			
				///////////////////////////////////////////////////////////////
				// writing psx pseudo inverse exponential increment mode
				
				Set_ExponentialIncRates ( Channel, Rate, Rates, Rates75 );
				
				break;
				
			case 3:
				
				///////////////////////////////////////////////////////////////
				// writing exponential decrement mode
				
				Set_ExponentialDecRates ( Channel, Rate, Rates, Rates75 );
				
				break;
				
		}
	}
}
*/



// this function is not used anymore
/*
static void SPUCore::SweepVolume ( u16 flags, s64& CurrentVolume, u32 VolConstant, u32 VolConstant75 )
{
	u64 Temp;
	
	//////////////////////////////////////////////
	// check if volume should sweep for VOL_L
	if ( flags >> 15 )
	{
//#ifdef INLINE_DEBUG_RUN
//	debug << "; Sweep";
//#endif

		////////////////////////////////////////////////////
		// Volume should sweep
		
		// check if linear sweep or exponential sweep for L
		if ( ( ( flags >> 13 ) & 3 ) != 3 )
		{
//#ifdef INLINE_DEBUG_RUN
//	debug << "; Linear/Pseudo";
//#endif

			////////////////////////////////////////////////////
			// linear or psx pseudo exponential sweep
			
			// check if current volume < 0x6000
			if ( CurrentVolume < ( 0x6000 << 16 ) )
			{
				CurrentVolume += VolConstant;
			}
			else
			{
				CurrentVolume += VolConstant75;
			}
		}
		else
		{
//#ifdef INLINE_DEBUG_RUN
//	debug << "; Exponential";
//#endif

			////////////////////////////////////////////////////
			// exponential decrement sweep
			Temp = (u64) (CurrentVolume);
			CurrentVolume = (u32) ( ( ( Temp >> 15 ) * VolConstant ) >> 15 );
		}
		
		// clamp volume
		if ( CurrentVolume > ( 32767 << 16 ) )
		{
			CurrentVolume = ( 32767 << 16 );
		}
		
		if ( CurrentVolume < 0 )
		{
			CurrentVolume = 0;
		}
		
	}
	else
	{
//#ifdef INLINE_DEBUG_RUN
//	debug << "; Constant";
//#endif

		///////////////////////////////////////////
		// Volume is constant
		CurrentVolume = ( (u32) flags ) << 17;
		
		// sign extend to 64-bits
		CurrentVolume = ( CurrentVolume << 32 ) >> 32;
	}
}
*/


static s64 SPUCore::Get_VolumeStep ( s16& Level, u32& Cycles, u32 Value, u32 flags )
{
	s32 ShiftValue, StepValue;
	s64 Step;
	
	ShiftValue = ( Value >> 2 ) & 0xf;
	StepValue = Value & 0x3;
	
	// check if increase or decrease
	if ( ! ( flags & 0x1 ) )
	{
		// increase //
		//StepValue = StepValues_Inc [ StepValue ];
		StepValue = 7 - StepValue;
	}
	else
	{
		// decrease //
		//StepValue = StepValues_Dec [ StepValue ];
		StepValue -= 8;
	}
	
	Cycles = 1 << ( ( ( ShiftValue - 11 ) < 0 ) ? 0 : ( ShiftValue - 11 ) );
	Step = StepValue << ( ( ( 11 - ShiftValue ) < 0 ) ? 0 : ( 11 - ShiftValue ) );
	
	// check if exponential AND increase
	if ( ( ( flags & 0x3 ) == 2 ) && ( Level > 0x6000 ) )
	{
		Cycles <<= 2;
	}
	
	// check if exponential AND decrease
	if ( ( flags & 0x3 ) == 3 )
	{
		Step = ( Step * Level ) >> 15;
	}
	
	return Step;
}


static void SPUCore::Start_VolumeEnvelope ( s16& Level, u32& Cycles, u32 Value, u32 flags, bool InitLevel )
{
	//static const s32 StepValues_Inc [] = { 7, 6, 5, 4 };
	//static const s32 StepValues_Dec [] = { -8, -7, -6, -5 };
	
	s32 ShiftValue, StepValue;
	s64 Step;
	
	ShiftValue = ( Value >> 2 ) & 0xf;
	StepValue = Value & 0x3;
	
	// check if increase or decrease
	if ( ! ( flags & 0x1 ) )
	{
		// increase //
		//StepValue = StepValues_Inc [ StepValue ];
		StepValue = 7 - StepValue;
	}
	else
	{
		// decrease //
		//StepValue = StepValues_Dec [ StepValue ];
		StepValue -= 8;
	}
	
	Cycles = 1 << ( ( ( ShiftValue - 11 ) < 0 ) ? 0 : ( ShiftValue - 11 ) );
	Step = StepValue << ( ( ( 11 - ShiftValue ) < 0 ) ? 0 : ( 11 - ShiftValue ) );
	
	// check if exponential AND increase
	if ( ( ( flags & 0x3 ) == 2 ) && ( Level > 0x6000 ) )
	{
		Cycles <<= 2;
	}
	
	// check if exponential AND decrease
	if ( ( flags & 0x3 ) == 3 )
	{
		Step = ( Step * Level ) >> 15;
	}
	
	if ( InitLevel )
	{
		Level = Step;
	}
}

// probably should start cycles at 1 and then let it set it later
// does not saturate Level since it can go to -1 for a cycle
static void SPUCore::VolumeEnvelope ( s16& Level, u32& Cycles, u32 Value, u32 flags )
{
	//static const s32 StepValues_Inc [] = { 7, 6, 5, 4 };
	//static const s32 StepValues_Dec [] = { -8, -7, -6, -5 };
	
	s32 ShiftValue, StepValue;
	s32 Step;
	
	Cycles--;
	
	if ( Cycles ) return;
	
	ShiftValue = ( Value >> 2 ) & 0x1f;
	StepValue = Value & 0x3;
	
#ifdef INLINE_DEBUG_ENVELOPE
	debug << "; ShiftValue=" << hex << ShiftValue << "; StepValue=" << StepValue;
#endif

	// check if increase or decrease
	if ( ! ( flags & 0x1 ) )
	{
		// increase //
		//StepValue = StepValues_Inc [ StepValue ];
		StepValue = 7 - StepValue;
	}
	else
	{
		// decrease //
		//StepValue = StepValues_Dec [ StepValue ];
		StepValue -= 8;
	}
	
#ifdef INLINE_DEBUG_ENVELOPE
	debug << "; StepValue=" << StepValue;
#endif

	Cycles = 1 << ( ( ( ShiftValue - 11 ) < 0 ) ? 0 : ( ShiftValue - 11 ) );
	Step = StepValue << ( ( ( 11 - ShiftValue ) < 0 ) ? 0 : ( 11 - ShiftValue ) );
	
#ifdef INLINE_DEBUG_ENVELOPE
	debug << "; Cycles=" << dec << Cycles;
#endif

	// check if exponential AND increase
	if ( ( ( flags & 0x3 ) == 2 ) && ( Level > 0x6000 ) )
	{
		Cycles <<= 2;
	}
	
	// check if exponential AND decrease
	if ( ( flags & 0x3 ) == 3 )
	{
		Step = ( Step * Level ) >> 15;
	}
	
#ifdef INLINE_DEBUG_ENVELOPE
	debug << "; Step=" << hex << Step << "; (before) Level=" << Level;
#endif

	Level += Step;
	
	// clamp level to signed 16-bits
	if ( Level > 0x7fffLL ) { Level = 0x7fffLL; } else if ( Level < -0x8000LL ) { Level = -0x8000LL; }

#ifdef INLINE_DEBUG_ENVELOPE
	debug << "; (after) Level=" << Level;
#endif
}



void SPUCore::ProcessReverbR ( s64 RightInput )
{
	// disable reverb to get PS2 SPU started
	
	s64 Rin, Rout;
	s64 t_mLDIFF, t_mRDIFF;
	s64 t_mRSAME, t_mRAPF1, t_mRAPF2;
	
	//s64 s_dRSAME = ReadReverbBuffer ( ((u32) *_dRSAME) << 2 );
	//s64 s_mRSAME = ReadReverbBuffer ( ( ((u32) *_mRSAME) << 2 ) - 1 );
	s64 s_dRSAME = ReadReverbBuffer ( GET_REG32( dRSAME_0 ) );
	s64 s_mRSAME = ReadReverbBuffer ( ( GET_REG32 ( mRSAME_0 ) ) - 1 );

	//s64 s_dLDIFF = ReadReverbBuffer ( ((u32) *_dLDIFF) << 2 );
	s64 s_dLDIFF = ReadReverbBuffer ( GET_REG32 ( dLDIFF_0 ) );
	
	//s64 s_dRDIFF = ReadReverbBuffer ( ((u32) *_dRDIFF) << 2 );
	//s64 s_mRDIFF = ReadReverbBuffer ( ( ((u32) *_mRDIFF) << 2 ) - 1 );
	s64 s_dRDIFF = ReadReverbBuffer ( GET_REG32 ( dRDIFF_0 ) );
	s64 s_mRDIFF = ReadReverbBuffer ( ( GET_REG32 ( mRDIFF_0 ) ) - 1 );

	//s64 s_mRCOMB1 = ReadReverbBuffer ( ((u32) *_mRCOMB1) << 2 );
	//s64 s_mRCOMB2 = ReadReverbBuffer ( ((u32) *_mRCOMB2) << 2 );
	//s64 s_mRCOMB3 = ReadReverbBuffer ( ((u32) *_mRCOMB3) << 2 );
	//s64 s_mRCOMB4 = ReadReverbBuffer ( ((u32) *_mRCOMB4) << 2 );
	s64 s_mRCOMB1 = ReadReverbBuffer ( GET_REG32 ( mRCOMB1_0 ) );
	s64 s_mRCOMB2 = ReadReverbBuffer ( GET_REG32 ( mRCOMB2_0 ) );
	s64 s_mRCOMB3 = ReadReverbBuffer ( GET_REG32 ( mRCOMB3_0 ) );
	s64 s_mRCOMB4 = ReadReverbBuffer ( GET_REG32 ( mRCOMB4_0 ) );
	
	//s64 s_mRAPF1 = ReadReverbBuffer ( ((u32) *_mRAPF1) << 2 );
	//s64 s_mRAPF1_dAPF1 = ReadReverbBuffer ( ( ((u32) *_mRAPF1) - ((u32) *_dAPF1) ) << 2 );
	s64 s_mRAPF1 = ReadReverbBuffer ( GET_REG32 ( mRAPF1_0 ) );
	s64 s_mRAPF1_dAPF1 = ReadReverbBuffer ( ( GET_REG32 ( mRAPF1_0 ) - GET_REG32 ( dAPF1_0 ) ) );
	
	//s64 s_mRAPF2 = ReadReverbBuffer ( ((u32) *_mRAPF2) << 2 );
	//s64 s_mRAPF2_dAPF2 = ReadReverbBuffer ( ( ((u32) *_mRAPF2) - ((u32) *_dAPF2) ) << 2 );
	s64 s_mRAPF2 = ReadReverbBuffer ( GET_REG32 ( mRAPF2_0 ) );
	s64 s_mRAPF2_dAPF2 = ReadReverbBuffer ( ( GET_REG32 ( mRAPF2_0 ) - GET_REG32 ( dAPF2_0 ) ) );
	
	// input from mixer //
	//Rin = ( RightInput * ( (s64) *_vRIN ) ) >> 15;
	Rin = ( RightInput * ( (s64) GET_REG16 ( vRIN ) ) ) >> 15;
	
	// same side reflection //
	//[mRSAME] = (Rin + [dRSAME]*vWALL - [mRSAME-2])*vIIR + [mRSAME-2]  ;R-to-R
	//t_mRSAME = ( ( ( Rin + ( ( s_dRSAME * ( (s64) *_vWALL ) ) >> 15 ) - s_mRSAME ) * ( (s64) *_vIIR ) ) >> 15 ) + s_mRSAME;
	t_mRSAME = ( ( ( Rin + ( ( s_dRSAME * ( (s64) GET_REG16 ( vWALL ) ) ) >> 15 ) - s_mRSAME ) * ( (s64) GET_REG16 ( vIIR ) ) ) >> 15 ) + s_mRSAME;
	
	// Different Side Reflection //
	//[mRDIFF] = (Rin + [dLDIFF]*vWALL - [mRDIFF-2])*vIIR + [mRDIFF-2]  ;L-to-R
	//t_mRDIFF = ( ( ( Rin + ( ( s_dLDIFF * ( (s64) *_vWALL ) ) >> 15 ) - s_mRDIFF ) * ( (s64) *_vIIR ) ) >> 15 ) + s_mRDIFF;
	t_mRDIFF = ( ( ( Rin + ( ( s_dLDIFF * ( (s64) GET_REG16 ( vWALL ) ) ) >> 15 ) - s_mRDIFF ) * ( (s64) GET_REG16 ( vIIR ) ) ) >> 15 ) + s_mRDIFF;
	
	// Early Echo (Comb Filter, with input from buffer) //
	//Rout=vCOMB1*[mRCOMB1]+vCOMB2*[mRCOMB2]+vCOMB3*[mRCOMB3]+vCOMB4*[mRCOMB4]
	//Rout = ( ( ( (s64) *_vCOMB1 ) * s_mRCOMB1 ) + ( ( (s64) *_vCOMB2 ) * s_mRCOMB2 ) + ( ( (s64) *_vCOMB3 ) * s_mRCOMB3 ) + ( ( (s64) *_vCOMB4 ) * s_mRCOMB4 ) ) >> 15;
	Rout = ( ( ( (s64) GET_REG16 ( vCOMB1 ) ) * s_mRCOMB1 ) + ( ( (s64) GET_REG16 ( vCOMB2 ) ) * s_mRCOMB2 ) + ( ( (s64) GET_REG16 ( vCOMB3 ) ) * s_mRCOMB3 ) + ( ( (s64) GET_REG16 ( vCOMB4 ) ) * s_mRCOMB4 ) ) >> 15;
	
	// Late Reverb APF1 (All Pass Filter 1, with input from COMB) //
	//[mRAPF1]=Rout-vAPF1*[mRAPF1-dAPF1], Rout=[mRAPF1-dAPF1]+[mRAPF1]*vAPF1
	//t_mRAPF1 = Rout - ( ( ( (s64) *_vAPF1 ) * s_mRAPF1_dAPF1 ) >> 15 );
	//Rout = s_mRAPF1_dAPF1 + ( ( s_mRAPF1 * ( (s64) *_vAPF1 ) ) >> 15 );
	t_mRAPF1 = Rout - ( ( ( (s64) GET_REG16 ( vAPF1 ) ) * s_mRAPF1_dAPF1 ) >> 15 );
	Rout = s_mRAPF1_dAPF1 + ( ( s_mRAPF1 * ( (s64) GET_REG16 ( vAPF1 ) ) ) >> 15 );
	
	// Late Reverb APF2 (All Pass Filter 2, with input from APF1) //
	// [mRAPF2]=Rout-vAPF2*[mRAPF2-dAPF2], Rout=[mRAPF2-dAPF2]+[mRAPF2]*vAPF2
	//t_mRAPF2 = Rout - ( ( ( (s64) *_vAPF2 ) * s_mRAPF2_dAPF2 ) >> 15 );
	//Rout = s_mRAPF2_dAPF2 + ( ( s_mRAPF2 * ( (s64) *_vAPF2 ) ) >> 15 );
	t_mRAPF2 = Rout - ( ( ( (s64) GET_REG16 ( vAPF2 ) ) * s_mRAPF2_dAPF2 ) >> 15 );
	Rout = s_mRAPF2_dAPF2 + ( ( s_mRAPF2 * ( (s64) GET_REG16 ( vAPF2 ) ) ) >> 15 );
	
	// Output to Mixer (Output volume multiplied with input from APF2) //
	// RightOutput = Rout*vROUT
	//ReverbR_Output = ( Rout * ( (s64) *_vROUT ) ) >> 15;
	ReverbR_Output = ( Rout * ( (s64) GET_REG16 ( vROUT ) ) ) >> 15;
	
	// only write to the reverb buffer if reverb is enabled
	//if ( REG ( CTRL ) & 0x80 )
	if ( GET_REG16 ( CTRL ) & 0x80 )
	{
		// *** TODO *** for PS2, there is no shift of the address as it is the actual address stored
		//WriteReverbBuffer ( ((u32) *_mRSAME) << 2, t_mRSAME );
		//WriteReverbBuffer ( ((u32) *_mRDIFF) << 2, t_mRDIFF );
		//WriteReverbBuffer ( ((u32) *_mRAPF1) << 2, t_mRAPF1 );
		//WriteReverbBuffer ( ((u32) *_mRAPF2) << 2, t_mRAPF2 );
		WriteReverbBuffer ( GET_REG32 ( mRSAME_0 ), t_mRSAME );
		WriteReverbBuffer ( GET_REG32 ( mRDIFF_0 ), t_mRDIFF );
		WriteReverbBuffer ( GET_REG32 ( mRAPF1_0 ), t_mRAPF1 );
		WriteReverbBuffer ( GET_REG32 ( mRAPF2_0 ), t_mRAPF2 );
	}
	
	// update reverb buffer address
	// this should actually happen at 22050 hz unconditionally
	UpdateReverbBuffer ();
}


void SPUCore::ProcessReverbL ( s64 LeftInput )
{
	// disable reverb for now just to get PS2 SPU started
	
	s64 Lin, Lout;
	
	// outputs
	s64 t_mLDIFF, t_mRDIFF;
	s64 t_mLSAME, t_mLAPF1, t_mLAPF2;
	
	// inputs
	//s64 s_dLSAME = ReadReverbBuffer ( ((u32) *_dLSAME) << 2 );
	//s64 s_mLSAME = ReadReverbBuffer ( ( ((u32) *_mLSAME) << 2 ) - 1 );
	s64 s_dLSAME = ReadReverbBuffer ( GET_REG32 ( dLSAME_0 ) );
	s64 s_mLSAME = ReadReverbBuffer ( ( GET_REG32 ( mLSAME_0 ) ) - 1 );
	
	//s64 s_dLDIFF = ReadReverbBuffer ( ((u32) *_dLDIFF) << 2 );
	//s64 s_mLDIFF = ReadReverbBuffer ( ( ((u32) *_mLDIFF) << 2 ) - 1 );
	s64 s_dLDIFF = ReadReverbBuffer ( GET_REG32 ( dLDIFF_0 ) );
	s64 s_mLDIFF = ReadReverbBuffer ( ( GET_REG32 ( mLDIFF_0 ) ) - 1 );
	
	//s64 s_dRDIFF = ReadReverbBuffer ( ((u32) *_dRDIFF) << 2 );
	s64 s_dRDIFF = ReadReverbBuffer ( GET_REG32 ( dRDIFF_0 ) );
	
	//s64 s_mLCOMB1 = ReadReverbBuffer ( ((u32) *_mLCOMB1) << 2 );
	//s64 s_mLCOMB2 = ReadReverbBuffer ( ((u32) *_mLCOMB2) << 2 );
	//s64 s_mLCOMB3 = ReadReverbBuffer ( ((u32) *_mLCOMB3) << 2 );
	//s64 s_mLCOMB4 = ReadReverbBuffer ( ((u32) *_mLCOMB4) << 2 );
	s64 s_mLCOMB1 = ReadReverbBuffer ( GET_REG32 ( mLCOMB1_0 ) );
	s64 s_mLCOMB2 = ReadReverbBuffer ( GET_REG32 ( mLCOMB2_0 ) );
	s64 s_mLCOMB3 = ReadReverbBuffer ( GET_REG32 ( mLCOMB3_0 ) );
	s64 s_mLCOMB4 = ReadReverbBuffer ( GET_REG32 ( mLCOMB4_0 ) );
	
	//s64 s_mLAPF1 = ReadReverbBuffer ( ((u32) *_mLAPF1) << 2 );
	//s64 s_mLAPF1_dAPF1 = ReadReverbBuffer ( ( ((u32) *_mLAPF1) - ((u32) *_dAPF1) ) << 2 );
	s64 s_mLAPF1 = ReadReverbBuffer ( GET_REG32 ( mLAPF1_0 ) );
	s64 s_mLAPF1_dAPF1 = ReadReverbBuffer ( ( GET_REG32 ( mLAPF1_0 ) - GET_REG32 ( dAPF1_0 ) ) );
	
	//s64 s_mLAPF2 = ReadReverbBuffer ( ((u32) *_mLAPF2) << 2 );
	//s64 s_mLAPF2_dAPF2 = ReadReverbBuffer ( ( ((u32) *_mLAPF2) - ((u32) *_dAPF2) ) << 2 );
	s64 s_mLAPF2 = ReadReverbBuffer ( GET_REG32 ( mLAPF2_0 ) );
	s64 s_mLAPF2_dAPF2 = ReadReverbBuffer ( ( GET_REG32 ( mLAPF2_0 ) - GET_REG32 ( dAPF2_0 ) ) );
	
	// input from mixer //
	//Lin = ( LeftInput * ( (s64) *_vLIN ) ) >> 15;
	Lin = ( LeftInput * ( (s64) GET_REG16 ( vLIN ) ) ) >> 15;
	
	// same side reflection //
	//[mLSAME] = (Lin + [dLSAME]*vWALL - [mLSAME-2])*vIIR + [mLSAME-2]  ;L-to-L
	//t_mLSAME = ( ( ( Lin + ( ( s_dLSAME * ( (s64) *_vWALL ) ) >> 15 ) - s_mLSAME ) * ( (s64) *_vIIR ) ) >> 15 ) + s_mLSAME;
	t_mLSAME = ( ( ( Lin + ( ( s_dLSAME * ( (s64) GET_REG16 ( vWALL ) ) ) >> 15 ) - s_mLSAME ) * ( (s64) GET_REG16 ( vIIR ) ) ) >> 15 ) + s_mLSAME;
	
	// Different Side Reflection //
	//[mLDIFF] = (Lin + [dRDIFF]*vWALL - [mLDIFF-2])*vIIR + [mLDIFF-2]  ;R-to-L
	//t_mLDIFF = ( ( ( Lin + ( ( s_dRDIFF * ( (s64) *_vWALL ) ) >> 15 ) - s_mLDIFF ) * ( (s64) *_vIIR ) ) >> 15 ) + s_mLDIFF;
	t_mLDIFF = ( ( ( Lin + ( ( s_dRDIFF * ( (s64) GET_REG16 ( vWALL ) ) ) >> 15 ) - s_mLDIFF ) * ( (s64) GET_REG16 ( vIIR ) ) ) >> 15 ) + s_mLDIFF;
	
	// Early Echo (Comb Filter, with input from buffer) //
	//Lout=vCOMB1*[mLCOMB1]+vCOMB2*[mLCOMB2]+vCOMB3*[mLCOMB3]+vCOMB4*[mLCOMB4]
	//Lout = ( ( (s64) *_vCOMB1 ) * s_mLCOMB1 + ( (s64) *_vCOMB2 ) * s_mLCOMB2 + ( (s64) *_vCOMB3 ) * s_mLCOMB3 + ( (s64) *_vCOMB4 ) * s_mLCOMB4 ) >> 15;
	Lout = ( ( (s64) GET_REG16 ( vCOMB1 ) ) * s_mLCOMB1 + ( (s64) GET_REG16 ( vCOMB2 ) ) * s_mLCOMB2 + ( (s64) GET_REG16 ( vCOMB3 ) ) * s_mLCOMB3 + ( (s64) GET_REG16 ( vCOMB4 ) ) * s_mLCOMB4 ) >> 15;
	
	// Late Reverb APF1 (All Pass Filter 1, with input from COMB) //
	//[mLAPF1]=Lout-vAPF1*[mLAPF1-dAPF1], Lout=[mLAPF1-dAPF1]+[mLAPF1]*vAPF1
	//t_mLAPF1 = Lout - ( ( ( (s64) *_vAPF1 ) * s_mLAPF1_dAPF1 ) >> 15 );
	//Lout = s_mLAPF1_dAPF1 + ( ( s_mLAPF1 * ( (s64) *_vAPF1 ) ) >> 15 );
	t_mLAPF1 = Lout - ( ( ( (s64) GET_REG16 ( vAPF1 ) ) * s_mLAPF1_dAPF1 ) >> 15 );
	Lout = s_mLAPF1_dAPF1 + ( ( s_mLAPF1 * ( (s64) GET_REG16 ( vAPF1 ) ) ) >> 15 );
	
	// Late Reverb APF2 (All Pass Filter 2, with input from APF1) //
	// [mLAPF2]=Lout-vAPF2*[mLAPF2-dAPF2], Lout=[mLAPF2-dAPF2]+[mLAPF2]*vAPF2
	//t_mLAPF2 = Lout - ( ( ( (s64) *_vAPF2 ) * s_mLAPF2_dAPF2 ) >> 15 );
	//Lout = s_mLAPF2_dAPF2 + ( ( s_mLAPF2 * ( (s64) *_vAPF2 ) ) >> 15 );
	t_mLAPF2 = Lout - ( ( ( (s64) GET_REG16 ( vAPF2 ) ) * s_mLAPF2_dAPF2 ) >> 15 );
	Lout = s_mLAPF2_dAPF2 + ( ( s_mLAPF2 * ( (s64) GET_REG16 ( vAPF2 ) ) ) >> 15 );
	
	// Output to Mixer (Output volume multiplied with input from APF2) //
	// LeftOutput = Lout*vLOUT
	//ReverbL_Output = ( Lout * ( (s64) *_vLOUT ) ) >> 15;
	ReverbL_Output = ( Lout * ( (s64) GET_REG16 ( vLOUT ) ) ) >> 15;
	
	// only write to the reverb buffer if reverb is enabled
	//if ( REG ( CTRL ) & 0x80 )
	if ( GET_REG16 ( CTRL ) & 0x80 )
	{
		// write back to reverb buffer
		// *** TODO *** for PS2, there is no shift of the address as it is the actual address stored
		//WriteReverbBuffer ( ((u32) *_mLSAME) << 2, t_mLSAME );
		//WriteReverbBuffer ( ((u32) *_mLDIFF) << 2, t_mLDIFF );
		//WriteReverbBuffer ( ((u32) *_mLAPF1) << 2, t_mLAPF1 );
		//WriteReverbBuffer ( ((u32) *_mLAPF2) << 2, t_mLAPF2 );
		WriteReverbBuffer ( GET_REG32 ( mLSAME_0 ), t_mLSAME );
		WriteReverbBuffer ( GET_REG32 ( mLDIFF_0 ), t_mLDIFF );
		WriteReverbBuffer ( GET_REG32 ( mLAPF1_0 ), t_mLAPF1 );
		WriteReverbBuffer ( GET_REG32 ( mLAPF2_0 ), t_mLAPF2 );
	}
}



// gets address in reverb work area at offset address
s64 SPUCore::ReadReverbBuffer ( u32 Address )
{
#ifdef INLINE_DEBUG_READREVERB
	debug << "\r\nSPU::ReadReverbBuffer " << "Address=" << hex << Address;
#endif

	s16 Value;
	u32 BeforeAddress;

	// address will be coming straight from register
	// that won't work because of the offsets
	//Address <<= 1;

	Address += Reverb_BufferAddress;
	
#ifdef INLINE_DEBUG_READREVERB
	debug << " " << hex << Address;
	if ( Address < ReverbWork_Start )
	{
		cout << "\nSPU::ReadReverbBuffer; (before) Address<ReverbWork_Start; Address=" << hex << Address << " ReverbWork_Start=" << ReverbWork_Start << dec << " ReverbWork_Size=" << ReverbWork_Size;
	}
	BeforeAddress = Address;
#endif


	//if ( Address >= c_iRam_Size ) Address = ReverbWork_Start + ( Address & c_iRam_Mask );
	if ( Address >= ReverbWork_End ) Address = ReverbWork_Start + ( Address - ReverbWork_End );
	
	
#ifdef INLINE_DEBUG_READREVERB
	debug << " " << hex << Address;
	if ( Address < ReverbWork_Start )
	{
		cout << "\nSPU::ReadReverbBuffer; (after) Address<ReverbWork_Start; (before) Address=" << hex << BeforeAddress << " (after) Address=" << Address << " ReverbWork_Start=" << ReverbWork_Start << dec << " ReverbWork_Size=" << ReverbWork_Size;
	}
#endif

	//Value = RAM [ Address >> 1 ];
	Value = RAM [ Address ];
	
#ifdef INLINE_DEBUG_READREVERB
	debug << " Value=" << hex << Value;
#endif

	// address is ready for use with shift right by 1
	 return Value;
}

void SPUCore::WriteReverbBuffer ( u32 Address, s64 Value )
{
#ifdef INLINE_DEBUG_WRITEREVERB
	debug << "\r\nSPU::WriteReverbBuffer; Reverb_BufferAddress=" << hex << Reverb_BufferAddress << " Value=" << Value << " Address=" << Address;
#endif


	// address will be coming straight from register
	// that won't work because of the offsets
	//Address <<= 1;
	
	Address += Reverb_BufferAddress;
	
#ifdef INLINE_DEBUG_WRITEREVERB
	u32 BeforeAddress;
	debug << " " << Address;
	if ( Address < ReverbWork_Start )
	{
		cout << "\nSPU::WriteReverbBuffer; (before) Address<ReverbWork_Start; Address=" << hex << Address << " ReverbWork_Start=" << ReverbWork_Start << dec << " ReverbWork_Size=" << ReverbWork_Size;
	}
	BeforeAddress = Address;
#endif

	
	//if ( Address >= c_iRam_Size ) Address = ReverbWork_Start + ( Address & c_iRam_Mask );
	if ( Address >= ReverbWork_End ) Address = ReverbWork_Start + ( Address - ReverbWork_End );
	
	// just in case
	Address &= ( c_iRam_Mask >> 1 );
	
#ifdef INLINE_DEBUG_WRITEREVERB
	debug << " " << Address;
	if ( Address < ReverbWork_Start )
	{
		cout << "\nSPU::WriteReverbBuffer; (after) Address<ReverbWork_Start; (before) Address=" << hex << BeforeAddress << " (after) Address=" << Address << " ReverbWork_Start=" << ReverbWork_Start << dec << " ReverbWork_Size=" << ReverbWork_Size;
	}
#endif

	// address is ready for use with shift right by 1
	//RAM [ Address >> 1 ] = adpcm_decoder::clamp ( Value );
	RAM [ Address ] = adpcm_decoder::clamp ( Value );
}

void SPUCore::UpdateReverbBuffer ()
{
#ifdef INLINE_DEBUG_UPDATEREVERB
	debug << "\r\nSPU::UpdateReverbBuffer " << " ReverbWork_Start=" << hex << ReverbWork_Start << " Reverb_BufferAddress=" << Reverb_BufferAddress;
#endif

	//Reverb_BufferAddress += 2;
	Reverb_BufferAddress += 1;
	
	//if ( Reverb_BufferAddress >= c_iRam_Size ) Reverb_BufferAddress = ReverbWork_Start;
	if ( Reverb_BufferAddress >= ReverbWork_End ) Reverb_BufferAddress = ReverbWork_Start;
	
	// check if address in reverb buffer is set to trigger interrupt
	//if ( ( Reverb_BufferAddress == ( ( (u32) Regs [ ( IRQA - SPU_X ) >> 1 ] ) << 3 ) ) && ( Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] & 0x40 ) )
	if ( Reverb_BufferAddress == ( GET_REG32 ( IRQA_0 ) ) && ( GET_REG16 ( CTRL ) & 0x40 ) )
	{
		// we have reached irq address - trigger interrupt
		SetInterrupt ();
		
		// interrupt
		//Regs [ ( 0x1f801dae - SPU_X ) >> 1 ] |= 0x40;
		GET_REG16 ( STAT ) |= 0x40;
	}
	
#ifdef INLINE_DEBUG_UPDATEREVERB
	debug << " " << Reverb_BufferAddress;
#endif
}




void SPUCore::SpuTransfer_Complete ()
{
	//Regs [ ( STAT - SPU_X ) >> 1 ] &= ~( 0xf << 7 );
	//GET_REG16 ( CTRL ) &= ~0x30;
	
	// update STAT //
	//GET_REG16 ( STAT ) &= ~( 0x3f | 0x0380 );
	//GET_REG16 ( STAT ) |= ( GET_REG16 ( CTRL ) & 0x3f );
}


// Sample0 is the sample you are on, then Sample1 is previous sample, then Sample2 is the next previous sample, etc.
static s32 SPUCore::Calc_sample_gx ( u32 SampleOffset_Fixed16, s32 Sample0, s32 Sample1, s32 Sample2, s32 Sample3 )
{
	u32 i;
	s32 Output;
	
	i = ( SampleOffset_Fixed16 >> 8 ) & 0xff;
	
	Output = ( ( Sample0 * ((s32) gx [ i ]) ) + ( Sample1 * ((s32) gx [0x100 + i]) ) + ( Sample2 * ((s32) gx [0x1ff - i]) ) + ( Sample3 * ((s32) gx [0xff - i]) ) ) >> 15;
	
	adpcm_decoder::clamp ( Output );
	
	return Output;
}


static s32 SPUCore::Calc_sample_filter ( s32 x0, s32 x1, s32 x2, s32 y1, s32 y2 )
{
	s32 Output;
	
	Output = ( ( _b0 * x0 ) + ( _b1 * x1 ) + ( _b2 * x2 ) - ( _a1 * y1 ) - ( _a2 * y2 ) ) >> _N;
	
	return Output;
}



void SPUCore::Start_SampleDecoding ( u32 Channel )
{
	u32 ModeRate;
	
	// clear sample history for channel
	//History [ Channel ].Value0 = 0;
	//History [ Channel ].Value1 = 0;
	
	// set the cycle sample was keyed on at
	StartCycle_Channel [ Channel ] = CycleCount;
	
	// starts adsr in attack phase
	ADSR_Status [ Channel ] = ADSR_ATTACK;
	
	// appears to copy voice start address to voice repeat address unconditionally
	// *** testing *** maybe not
	// *note* the PS1 does NOT copy voice start address to voice repeat address when a voice is keyed on
	if ( !( LSA_Manual_Bitmap & ( 1 << Channel ) ) )
	{
		//Regs [ ( ( Channel << 4 ) + LSA_X ) >> 1 ] = Regs [ ( ( Channel << 4 ) + SSA_X ) >> 1 ];
	}
	
		/*
		// check for loop
		if ( ! ( RAM [ CurrentBlockAddress [ Channel ] >> 1 ] & ( 0x2 << 8 ) ) )
		{
			// the loop is still there, just kill the envelope //
			KillEnvelope_Bitmap |= ( 1 << Channel );
		}
		
		// check if this is loop start
		if ( RAM [ ( CurrentBlockAddress [ Channel ] & c_iRam_Mask ) >> 1 ] & ( 0x4 << 8 ) )
		{
			// don't kill the envelope yet
			KillEnvelope_Bitmap &= ~( 1 << Channel );
		}
		*/
				
	// automatically initializes adsr volume to zero
	// *** testing *** try it the other way
	//Regs [ ( ( Channel << 4 ) + ENV_X ) >> 1 ] = 0;
	SET_REG16 ( ENVX_CH( Channel ), 0 );
	VOL_ADSR_Value [ Channel ] = 0;
	
	// loop address not manually specified
	// can still change loop address after sound starts
	//LSA_Manual_Bitmap &= ~( 1 << Channel );
	
	// start envelope
	//ModeRate = Regs [ ( ADSR_0 >> 1 ) + ( Channel << 3 ) ];
	ModeRate = GET_REG16 ( ADSR0_CH ( Channel ) );
	
	//Start_VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ModeRate >> 8 ) & 0x7f, ( ModeRate >> 15 ) << 1 );
	//Start_VolumeEnvelope ( VOL_ADSR_Value [ Channel ], Cycles [ Channel ], ( ModeRate >> 8 ) & 0x7f, ( ModeRate >> 15 ) << 1, true );
	Start_VolumeEnvelope ( GET_REG16s ( ENVX_CH ( Channel ) ), Cycles [ Channel ], ( ModeRate >> 8 ) & 0x7f, ( ModeRate >> 15 ) << 1, true );
	
	//Regs [ ( ( Channel << 4 ) + ENV_X ) >> 1 ] = VOL_ADSR_Value [ Channel ];
	//SET_REG16 ( ENVX_CH( Channel ), VOL_ADSR_Value [ Channel ] );
	
	// start sweep for other volumes too
	/*
	Start_VolumeEnvelope ( VOL_L_Value [ Channel ], VOL_L_Cycles [ Channel ], Regs [ ( VOL_L >> 1 ) + ( Channel << 3 ) ] & 0x7f, ( Regs [ ( VOL_L >> 1 ) + ( Channel << 3 ) ] >> 13 ) & 0x3 );
	Start_VolumeEnvelope ( VOL_R_Value [ Channel ], VOL_R_Cycles [ Channel ], Regs [ ( VOL_R >> 1 ) + ( Channel << 3 ) ] & 0x7f, ( Regs [ ( VOL_R >> 1 ) + ( Channel << 3 ) ] >> 13 ) & 0x3 );
	Start_VolumeEnvelope ( MVOL_L_Value, MVOL_L_Cycles, Regs [ ( MVOL_L - SPU_X ) >> 1 ] & 0x7f, ( Regs [ ( MVOL_L - SPU_X ) >> 1 ] >> 13 ) & 0x3 );
	Start_VolumeEnvelope ( MVOL_R_Value, MVOL_R_Cycles, Regs [ ( MVOL_R - SPU_X ) >> 1 ] & 0x7f, ( Regs [ ( MVOL_R - SPU_X ) >> 1 ] >> 13 ) & 0x3 );
	*/
	
	// copy the pitch over
	//_SPU->dSampleDT [ Channel ] = ( ((u64) Regs [ ( ( Channel << 4 ) + PITCH ) >> 1 ]) << 32 ) >> 12;
	dSampleDT [ Channel ] = ( ( (u64) GET_REG16 ( PITCH_CH( Channel ) ) ) << 32 ) >> 12;


	// prepare for playback //
	
	// clear current sample
	CurrentSample_Offset [ Channel ] = 0;
	CurrentSample_Read [ Channel ] = 0;
	CurrentSample_Write [ Channel ] = 0;
	
	// set start address
	//CurrentBlockAddress [ Channel ] = ( (u32) Regs [ ( ( Channel << 4 ) + SSA_X ) >> 1 ] ) << 3;
	CurrentBlockAddress [ Channel ] = ( GET_REG32 ( SSA0_CH ( Channel ) ) ) << 3;
	
	////////////////////////////////////////////////////////
	// check if the IRQ address is in this block
	// and check if interrupts are enabled
	//if ( ( ( CurrentBlockAddress [ Channel ] >> 4 ) == ( Regs [ ( IRQA - SPU_X ) >> 1 ] >> 1 ) ) && ( Regs [ ( 0x1f801daa - SPU_X ) >> 1 ] & 0x40 ) )
	if ( ( ( CurrentBlockAddress [ Channel ] >> 4 ) == ( GET_REG32 ( IRQA_0 ) >> 1 ) ) && ( GET_REG16 ( CTRL ) & 0x40 ) )
	{
		// we have reached irq address - trigger interrupt
		SetInterrupt ();
		
		// interrupt
		//Regs [ ( 0x1f801dae - SPU_X ) >> 1 ] |= 0x40;
		GET_REG16 ( STAT ) |= 0x40;
	}
	
	//////////////////////////////////////////////////////////////////////////////
	// Check loop start flag and set loop address if needed
	// LOOP_START is bit 3 actually
	// note: LOOP_START is actually bit 2
	if ( ( RAM [ ( CurrentBlockAddress [ Channel ] & c_iRam_Mask ) >> 1 ] & ( 0x4 << 8 ) ) && ( ! ( LSA_Manual_Bitmap & ( 1 << Channel ) ) ) )
	{
#ifdef INLINE_DEBUG_WRITE_KON_0
	debug << "; Channel=" << Channel << "; LOOP_AT_START";
#endif

		///////////////////////////////////////////////////
		// we are at loop start address
		// set loop start address
		//Regs [ ( ( Channel << 4 ) + LSA_X ) >> 1 ] = ( CurrentBlockAddress [ Channel ] >> 3 );
		SET_REG32 ( LSA0_CH( Channel ), ( CurrentBlockAddress [ Channel ] >> 3 ) );
	}
	
	// decode the new block
	
	// clear the samples first because of interpolation algorithm
	for ( int i = 0; i < 32; i++ ) DecodedBlocks [ ( Channel << 5 ) + i ] = 0;
	
#ifdef INLINE_DEBUG_SPU_ERROR_RECORD
	u32 filter = ( RAM [ CurrentBlockAddress [ Channel ] >> 1 ] >> 12 );
	if ( filter > 4 ) 
	{
		debug << "\r\nhpsx64 ALERT: SPU: Filter value is greater than 4 (invalid): filter=" << dec << filter << hex << " SPUAddress=" << CurrentBlockAddress [ Channel ] << " shifted=" << ( CurrentBlockAddress [ Channel ] >> 3 );
		debug << " Channel=" << dec << Channel << " LSA_X=" << hex << Regs [ ( ( Channel << 4 ) + LSA_X ) >> 1 ] << " SSA_X=" << Regs [ ( ( Channel << 4 ) + SSA_X ) >> 1 ];
	}
#endif
	
	// now decode the sample packet into buffer
	//SampleDecoder [ Channel ].decode_packet ( (adpcm_packet*) & ( RAM [ CurrentBlockAddress [ Channel ] >> 1 ] ), & ( DecodedBlocks [ ( Channel << 5 ) + ( ( CurrentSample_Read [ Channel ] >> 32 ) & 0x1f ) ] ) );
	SampleDecoder [ Channel ].decode_packet32 ( (adpcm_packet*) & ( RAM [ CurrentBlockAddress [ Channel ] >> 1 ] ), DecodedSamples );
	for ( int i = 0; i < 28; i++ ) DecodedBlocks [ ( Channel << 5 ) + ( ( CurrentSample_Write [ Channel ] + i ) & 0x1f ) ] = DecodedSamples [ i ];
	
	// clear killing of envelope
	// let's actually try killing the envelope
	//KillEnvelope_Bitmap &= ~( 1 << Channel );
	//KillEnvelope_Bitmap |= ( 1 << Channel );
	
	// check for loop
	//if ( ! ( RAM [ CurrentBlockAddress [ Channel ] >> 1 ] & ( 0x2 << 8 ) ) )
	//{
	//	// the loop is still there, just kill the envelope //
	//	KillEnvelope_Bitmap |= ( 1 << Channel );
	//}
	
	// *** testing *** try resetting decoder
	//SampleDecoder [ Channel ].reset ();
}





static u32 SPU2::Read ( u32 Address )
{
	u32 Offset;
	u32 Output;
	u32 Mask = 0xffff;
	
	// get the physical address of device
	Address &= 0x1fffffff;
	
	// get offset
	Offset = Address - 0x1f900000;
	
	// check if address is for first group of core0 registers
	if ( Offset < 0x400 )
	{
		Output = _SPU2->SPU0.Read ( Offset, Mask );
	}
	// check if address is for first group of core1 registers
	else if ( Offset < 0x760 )
	{
		Output = _SPU2->SPU1.Read ( Offset - 0x400, Mask );
	}
	// check if address is for second group of core0 registers
	else if ( Offset < 0x788 )
	{
		Output = _SPU2->SPU0.Read ( Offset, Mask );
	}
	// check if address is for second group of core1 registers
	else if ( Offset < 0x7b0 )
	{
		// 20 registers in second group per core = 40 bytes
		Output = _SPU2->SPU1.Read ( Offset - 0x28, Mask );
	}
	else if ( Offset < 0x800 )
	{
		// doesn't fit any of the other criteria, so this is in the area that applies to both cores
		
		if ( Offset >= 0x7c0 && Offset < 0x7ca )
		{
#ifdef INLINE_DEBUG_READ
			SPUCore::debug << "\r\nSPU::Read; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << hex << " Mask=" << Mask << " Offset= " << Offset;
			//SPUCore::debug << " Core#" << CoreNumber;
			SPUCore::debug << "; " << SPUCore::RegisterNames3 [ ( Offset - 0x7c0 ) >> 1 ];
#endif

		}
		
		// handle any processing
		switch ( Offset )
		{
			default:
				break;
		}
		
		// read value
		Output = _SPU2->Regs16 [ ( ( Offset - 0x7b0 ) >> 1 ) & c_iNumberOfRegisters_Mask ];
	}
	else
	{
		cout << "\nhps1x64: SPU2 READ from Address >=0x800; Address=" << hex << Address;
	}
	
	return Output;
}

static void SPU2::Write ( u32 Address, u32 Data, u32 Mask )
{
	u32 Offset;
	
	// get the physical address of device
	Address &= 0x1fffffff;
	
	// get offset
	Offset = Address - 0x1f900000;
	
	// check if address is for first group of core0 registers
	if ( Offset < 0x400 )
	{
		_SPU2->SPU0.Write ( Offset, Data, Mask );
	}
	// check if address is for first group of core1 registers
	else if ( Offset < 0x760 )
	{
		_SPU2->SPU1.Write ( Offset - 0x400, Data, Mask );
	}
	// check if address is for second group of core0 registers
	else if ( Offset < 0x788 )
	{
		_SPU2->SPU0.Write ( Offset, Data, Mask );
	}
	// check if address is for second group of core1 registers
	else if ( Offset < 0x7b0 )
	{
		// 20 registers in second group per core = 40 bytes
		_SPU2->SPU1.Write ( Offset - 0x28, Data, Mask );
	}
	else if ( Offset < 0x800 )
	{
		// doesn't fit any of the other criteria, so this is in the area that applies to both cores
		
		if ( Offset >= 0x7c0 && Offset < 0x7ca )
		{
#ifdef INLINE_DEBUG_WRITE
	SPUCore::debug << "\r\nSPU::Write; " << hex << setw(8) << *_DebugPC << " " << dec << *_DebugCycleCount << hex << " Mask=" << Mask << " Offset= " << Offset << " Data=" << Data;
	//SPUCore::debug << " Core#" << CoreNumber;
	SPUCore::debug << "; " << SPUCore::RegisterNames3 [ ( Offset - 0x7c0 ) >> 1 ];
#endif

		}


		// handle any processing
		switch ( Offset )
		{
			default:
				break;
		}
		
		// write value
		_SPU2->Regs16 [ ( ( Offset - 0x7b0 ) >> 1 ) & c_iNumberOfRegisters_Mask ] = Data;
	}
	else
	{
		cout << "\nhps1x64: SPU2 WRITE to Address >=0x800; Address=" << hex << Address << " Data=" << Data;
	}
}







static void SPUCore::DebugWindow_Enable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	static const int SPUList_CountX = 9;
	
	static const int SPUMasterList_X = 0;
	static const int SPUMasterList_Y = 0;
	static const int SPUMasterList_Width = 90;
	static const int SPUMasterList_Height = 120;
	
	static const int SPUList_X = 0;
	static const int SPUList_Y = 0;
	static const int SPUList_Width = SPUMasterList_Width;
	static const int SPUList_Height = SPUMasterList_Height;
	
	static const char* DebugWindow_Caption = "SPU1 Debug Window";
	static const int DebugWindow_X = 10;
	static const int DebugWindow_Y = 10;
	static const int DebugWindow_Width = ( SPUList_Width * SPUList_CountX ) + 10;
	static const int DebugWindow_Height = ( ( ( ( c_iNumberOfChannels + 1 ) / SPUList_CountX ) + 1 ) * SPUList_Height ) + 20;
	
	static const int MemoryViewer_X = SPUList_Width * 7;
	static const int MemoryViewer_Y = SPUList_Height * 2;
	static const int MemoryViewer_Width = SPUList_Width * 2;
	static const int MemoryViewer_Height = SPUList_Height;
	static const int MemoryViewer_Columns = 4;

	
	
	int i;
	
	/*
	stringstream ss;
	
	if ( !DebugWindow_Enabled )
	{
		// disable debug window for SPU2 for now to get it started
		// create the main debug window
		DebugWindow = new WindowClass::Window ();
		DebugWindow->Create ( DebugWindow_Caption, DebugWindow_X, DebugWindow_Y, DebugWindow_Width, DebugWindow_Height );
		DebugWindow->DisableCloseButton ();
		
		// create "value lists"
		SPUMaster_ValueList = new DebugValueList<u16> ();
		for ( i = 0; i < NumberOfChannels; i++ ) SPU_ValueList [ i ] = new DebugValueList<u16> ();
		
		// create the value lists
		SPUMaster_ValueList->Create ( DebugWindow, SPUMasterList_X, SPUMasterList_Y, SPUMasterList_Width, SPUMasterList_Height, true, false );
		
		for ( i = 0; i < NumberOfChannels; i++ )
		{
			SPU_ValueList [ i ]->Create ( DebugWindow, SPUList_X + ( ( ( i + 1 ) % SPUList_CountX ) * SPUList_Width ), SPUList_Y + ( ( ( i + 1 ) / SPUList_CountX ) * SPUList_Height ), SPUList_Width, SPUList_Height, true, false );
		}
		

		SPUMaster_ValueList->AddVariable ( "CON_0", & (_SPU->Regs [ ( SPU::CON_0 - SPU::SPU_X ) >> 1 ]) );
		SPUMaster_ValueList->AddVariable ( "CON_1", & (_SPU->Regs [ ( SPU::CON_1 - SPU::SPU_X ) >> 1 ]) );
		SPUMaster_ValueList->AddVariable ( "KON_0", & (_SPU->Regs [ ( SPU::KON_0 - SPU::SPU_X ) >> 1 ]) );
		SPUMaster_ValueList->AddVariable ( "KON_1", & (_SPU->Regs [ ( SPU::KON_1 - SPU::SPU_X ) >> 1 ]) );
		SPUMaster_ValueList->AddVariable ( "KOFF_0", & (_SPU->Regs [ ( SPU::KOFF_0 - SPU::SPU_X ) >> 1 ]) );
		SPUMaster_ValueList->AddVariable ( "KOFF_1", & (_SPU->Regs [ ( SPU::KOFF_1 - SPU::SPU_X ) >> 1 ]) );
		SPUMaster_ValueList->AddVariable ( "MVOL_L", & (_SPU->Regs [ ( SPU::MVOL_L - SPU::SPU_X ) >> 1 ]) );
		SPUMaster_ValueList->AddVariable ( "MVOL_R", & (_SPU->Regs [ ( SPU::MVOL_R - SPU::SPU_X ) >> 1 ]) );
		SPUMaster_ValueList->AddVariable ( "EVOL_L", & (_SPU->Regs [ ( SPU::EVOL_L - SPU::SPU_X ) >> 1 ]) );
		SPUMaster_ValueList->AddVariable ( "EVOL_R", & (_SPU->Regs [ ( SPU::EVOL_R - SPU::SPU_X ) >> 1 ]) );
		SPUMaster_ValueList->AddVariable ( "SPU_CTRL", & (_SPU->Regs [ ( 0x1f801daa - SPU::SPU_X ) >> 1 ]) );
		SPUMaster_ValueList->AddVariable ( "SPU_STAT", & (_SPU->Regs [ ( 0x1f801dae - SPU::SPU_X ) >> 1 ]) );
		
		// add variables into lists
		for ( i = 0; i < NumberOfChannels; i++ )
		{
			static const char* c_sChannelStr = "C";
			
			ss.str ("");
			ss << c_sChannelStr << dec << i << "_VOLL";
			SPU_ValueList [ i ]->AddVariable ( ss.str().c_str(), &(_SPU->Regs [ ( ( i << 4 ) + SPU::VOL_L ) >> 1 ]) );
			
			ss.str ("");
			ss << c_sChannelStr << i << "_VOLR";
			SPU_ValueList [ i ]->AddVariable ( ss.str().c_str(), &(_SPU->Regs [ ( ( i << 4 ) + SPU::VOL_L ) >> 1 ]) );
			
			ss.str ("");
			ss << c_sChannelStr << i << "_PITCH";
			SPU_ValueList [ i ]->AddVariable ( ss.str().c_str(), &(_SPU->Regs [ ( ( i << 4 ) + SPU::PITCH ) >> 1 ]) );
			
			ss.str ("");
			ss << c_sChannelStr << i << "_SSAX";
			SPU_ValueList [ i ]->AddVariable ( ss.str().c_str(), &(_SPU->Regs [ ( ( i << 4 ) + SPU::SSA_X ) >> 1 ]) );
			
			ss.str ("");
			ss << c_sChannelStr << i << "_ADSR0";
			SPU_ValueList [ i ]->AddVariable ( ss.str().c_str(), &(_SPU->Regs [ ( ( i << 4 ) + SPU::ADSR_0 ) >> 1 ]) );
			
			ss.str ("");
			ss << c_sChannelStr << i << "_ADSR1";
			SPU_ValueList [ i ]->AddVariable ( ss.str().c_str(), &(_SPU->Regs [ ( ( i << 4 ) + SPU::ADSR_1 ) >> 1 ]) );
			
			ss.str ("");
			ss << c_sChannelStr << i << "_ENVX";
			SPU_ValueList [ i ]->AddVariable ( ss.str().c_str(), &(_SPU->Regs [ ( ( i << 4 ) + SPU::ENV_X ) >> 1 ]) );
			
			ss.str ("");
			ss << c_sChannelStr << i << "_LSAX";
			SPU_ValueList [ i ]->AddVariable ( ss.str().c_str(), &(_SPU->Regs [ ( ( i << 4 ) + SPU::LSA_X ) >> 1 ]) );
			
			ss.str ("");
			ss << c_sChannelStr << i << "MADSR";
			SPU_ValueList [ i ]->AddVariable ( ss.str().c_str(), &(_SPU->ADSR_Status [ i ]) );
			
			ss.str ("");
			ss << c_sChannelStr << i << "RAW";
			SPU_ValueList [ i ]->AddVariable ( ss.str().c_str(), &(_SPU->Debug_CurrentRawSample [ i ]) );
			
			ss.str ("");
			ss << c_sChannelStr << i << "SMP";
			SPU_ValueList [ i ]->AddVariable ( ss.str().c_str(), &(_SPU->Debug_CurrentSample [ i ]) );
			
			//ss.str ("");
			//ss << c_sChannelStr << i << "RATE";
			//SPU_ValueList [ i ]->AddVariable ( ss.str().c_str(), &(_SPU->Debug_CurrentRate [ i ]) );
		}
		
		// create the viewer for D-Cache scratch pad
		SoundRAM_Viewer = new Debug_MemoryViewer ();
		
		SoundRAM_Viewer->Create ( DebugWindow, MemoryViewer_X, MemoryViewer_Y, MemoryViewer_Width, MemoryViewer_Height, MemoryViewer_Columns );
		SoundRAM_Viewer->Add_MemoryDevice ( "SoundRAM", 0, c_iRam_Size, (char*) _SPU->RAM );
		
		// mark debug as enabled now
		DebugWindow_Enabled = true;
		
		// update the value lists
		DebugWindow_Update ();
	}
	*/
	
#endif

}

static void SPUCore::DebugWindow_Disable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	int i;
	
	// this will be part of the interface
	/*
	if ( DebugWindow_Enabled )
	{
		delete DebugWindow;
		delete SPUMaster_ValueList;
		for ( i = 0; i < NumberOfChannels; i++ ) delete SPU_ValueList [ i ];
	
		// disable debug window
		DebugWindow_Enabled = false;
	}
	*/
	
#endif

}

static void SPUCore::DebugWindow_Update ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	int i;
	
	// this will be part of the interface
	/*
	if ( DebugWindow_Enabled )
	{
		SPUMaster_ValueList->Update();
		for ( i = 0; i < NumberOfChannels; i++ ) SPU_ValueList [ i ]->Update();
	}
	*/
	
#endif

}






