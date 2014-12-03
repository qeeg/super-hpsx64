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


#include "PS2_Dma.h"
#include "PS2_GPU.h"
#include "PS2_SIF.h"

#include "VU.h"

//using namespace Playstation2;

//#define ENABLE_SIF_DMA_TIMING
//#define ENABLE_SIF_DMA_SYNC

#define TEST_ASYNC_DMA
#define TEST_ASYNC_DMA_STAGE2


#ifdef _DEBUG_VERSION_

// enable debugging

#define INLINE_DEBUG_ENABLE

/*
#define INLINE_DEBUG_WRITE
#define INLINE_DEBUG_READ
#define INLINE_DEBUG_START
#define INLINE_DEBUG_TRANSFER
#define INLINE_DEBUG_END
#define INLINE_DEBUG_RUN_DMA5
#define INLINE_DEBUG_INT
*/


//#define INLINE_DEBUG_COMPLETE

//#define INLINE_DEBUG_READ_CHCR
//#define INLINE_DEBUG_WRITE_CHCR
//#define INLINE_DEBUG_READ_CTRL
//#define INLINE_DEBUG_WRITE_CTRL
//#define INLINE_DEBUG_READ_INVALID
//#define INLINE_DEBUG_WRITE_INVALID

//#define INLINE_DEBUG_WRITE_DMA2
//#define INLINE_DEBUG_RUN_DMA2
//#define INLINE_DEBUG_RUN_DMA2_CO


//#define INLINE_DEBUG_WRITE_PCR
//#define INLINE_DEBUG_READ_PCR

//#define INLINE_DEBUG

//#define INLINE_DEBUG_RUN_DMA0
//#define INLINE_DEBUG_RUN_DMA1
//#define INLINE_DEBUG_RUN_DMA3
//#define INLINE_DEBUG_RUN_DMA6
//#define INLINE_DEBUG_RUN_DMA4
//#define INLINE_DEBUG_RUN
//#define INLINE_DEBUG_CD
//#define INLINE_DEBUG_SPU
//#define INLINE_DEBUG_ACK


#endif


namespace Playstation2
{

u32* Dma::_SBUS_F240;

u32* Dma::_DebugPC;
u64* Dma::_DebugCycleCount;
u32* Dma::_R5900_Status;

//u32* Dma::_Intc_Master;
u32* Dma::_Intc_Stat;
u32* Dma::_Intc_Mask;
u32* Dma::_R5900_Cause_13;
u32* Dma::_R5900_Status_12;
u64* Dma::_ProcStatus;


Debug::Log Dma::debug;

Dma *Dma::_DMA;

bool Dma::DebugWindow_Enabled;
WindowClass::Window *Dma::DebugWindow;
DebugValueList<u32> *Dma::DMA_ValueList;



int DmaChannel::Count = 0;


u64* Dma::_NextSystemEvent;


DataBus *Dma::_BUS;
MDEC *Dma::_MDEC;
GPU *Dma::_GPU;
//R5900::Cpu *Dma::_CPU;


const char* Dma::DmaCh_Names [ c_iNumberOfChannels ] = { "VU0/VIF0", "VU1/VIF1", "GPU/GIF", "MDEC/IPU out", "MDEC/IPU in", "SIF0 (from SIF/IOP)", "SIF1 (to SIF/IOP)",
													"SIF2", "SPR out", "SPR in" };


const u64 Dma::c_iDmaSetupTime [ c_iNumberOfChannels ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const u64 Dma::c_iDmaTransferTimePerQwc [ c_iNumberOfChannels ] = {
c_iVU0_TransferTime,
c_iVU1_TransferTime,
c_iGIF_TransferTime,
c_iMDECout_TransferTime,
c_iMDECin_TransferTime,
c_iSIF0_TransferTime,
c_iSIF1_TransferTime,
1,	// sif2
1,	// spr out
1 };	// spr in

//const u64 Dma::c_iDmaTransferTimePerQwc [ c_iNumberOfChannels ] = { 32, 32, 32, 32, 32, 32, 32, 32, 32, 32 };
//const u64 Dma::c_iDmaTransferTimePerQwc [ c_iNumberOfChannels ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const u64 Dma::c_iDeviceBufferSize [ c_iNumberOfChannels ] = { 8, 16, 16, 8, 8, 8, 8, 8, 16, 16 };

static Dma::fnReady Dma::cbReady [ c_iNumberOfChannels ] = {
// channel 0 - VU0/VIF0
VU0::DMA_Write_Ready,
// channel 1 - VU1/VIF1
VU1::DMA_Write_Ready,
// channel 2 - GPU/GIF
NULL,
// channel 3 - MDEC/IPU out
NULL,
// channel 4 - MDEC/IPU in
NULL,
// channel 5 - SIF0 (from SIF/IOP)
SIF::IOP_DMA_Out_Ready,
// channel 6 - SIF1 (to SIF/IOP)
SIF::IOP_DMA_In_Ready,
// channel 7 - SIF2
NULL,
// channel 8 - SPR out
NULL,
// channel 9 - SPR in
NULL
};

static Dma::fnTransfer_FromMemory Dma::cbTransfer_FromMemory [ c_iNumberOfChannels ] = {
// channel 0 - VU0/VIF0
VU0::DMA_WriteBlock,
// channel 1 - VU1/VIF1
VU1::DMA_WriteBlock,
// channel 2 - GPU/GIF
GPU::DMA_WriteBlock,
// channel 3 - MDEC/IPU out
NULL,
// channel 4 - MDEC/IPU in
NULL,
// channel 5 - SIF0 (from SIF/IOP)
NULL,
// channel 6 - SIF1 (to SIF/IOP)
SIF::EE_DMA_WriteBlock,
// channel 7 - SIF2
NULL,
// channel 8 - SPR out
NULL,
// channel 9 - SPR in
NULL
};



Dma::Dma ()
{
	cout << "Running DMA constructor...\n";

/*	
#ifdef INLINE_DEBUG_ENABLE
	debug.Create ( "DMAController_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering DMA controller constructor";
#endif


	// set the current dma object
	_DMA = this;

	Reset ();
	
	
#ifdef INLINE_DEBUG
	debug << "->Exiting DMA controller constructor";
#endif
*/

}


void Dma::Start ()
{
	cout << "Running PS2::DMA::Start...\n";

#ifdef INLINE_DEBUG_ENABLE
	debug.Create ( "PS2_DMA_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering DMA::Start";
#endif

	// set the current dma object
	_DMA = this;

	Reset ();
	
	// none of the dma channels are running yet
	for ( int iChannel = 0; iChannel < c_iNumberOfChannels; iChannel++ )
	{
		QWC_Transferred [ iChannel ] = -1;
	}
	
	// ???
	lDMAC_ENABLE = 0x1201;
	
#ifdef INLINE_DEBUG
	debug << "->Exiting PS2::DMA::Start";
#endif
}



void Dma::Reset ()
{
	// zero object
	memset ( this, 0, sizeof( Dma ) );
	
	// allow all dma channels to run
	//SelectedDMA_Bitmap = 0xffffffff;
	
	// no dma channels are active
	//ActiveChannel = -1;
}


/*
void Dma::ConnectDevices ( DataBus *BUS, MDEC* mdec, GPU *g, CD *cd, SPU *spu, R5900::Cpu *cpu )
{
	_BUS = BUS;
	_MDEC = mdec;
	_GPU = g;
	_CPU = cpu;
}
*/





void Dma::Run ()
{
	//u32 Temp;
	//u32 Data [ 4 ];
	
	// will use this for MDEC for now
	//u32 NumberOfTransfers;

	
#ifdef INLINE_DEBUG_RUN
	debug << "\r\nDma::Run";
	debug << " NextEvent=" << dec << NextEvent_Cycle;
	debug << " CycleCount=" << *_DebugCycleCount;
#endif

	// check if dma is doing anything starting at this particular cycle
	if ( NextEvent_Cycle != *_DebugCycleCount ) return;
	
	
	// check for the channel(s) that needs to be run
	for ( int iChannel = 0; iChannel < c_iNumberOfChannels; iChannel++ )
	{
		if ( NextEvent_Cycle == NextEventCh_Cycle [ iChannel ] )
		{
			// ***todo*** check channel priority
			Transfer ( iChannel );
		}
	}
	
	// get the cycle number of the next event for device
	Update_NextEventCycle ();
	
	/*
	// check if this is for dma 5 or 6
	if ( NextEvent_Cycle == NextEventCh_Cycle [ 5 ] )
	{
		// don't end the transfer, but start the transfer at the source
		//EndTransfer ( 5 );
		Transfer ( 5 );
		Update_NextEventCycle ();
		return;
	}

	if ( NextEvent_Cycle == NextEventCh_Cycle [ 6 ] )
	{
		//EndTransfer ( 6 );
		Transfer ( 6 );
		Update_NextEventCycle ();
		return;
	}
	
	// check if dma even has any channels active first
	if ( ActiveChannel == -1 ) return;
	*/
	
	/*
	// check if the active channel is set to run
	if ( NextEvent_Cycle == NextEventCh_Cycle [ ActiveChannel ] )
	{
#ifdef INLINE_DEBUG_RUN
	debug << " Running";
	debug << " ActiveChannel=" << ActiveChannel << " NextEventCh_Cycle [ ActiveChannel ]=" << NextEventCh_Cycle [ ActiveChannel ];
#endif

		// run the active channel (either 0,1,2,4)
		DMA_Run ( ActiveChannel );
	}
	else
	{
		// if there is an active channel, but it is not set to run on this cycle, then update the next cycle //
		Update_NextEventCycle ();
	}
	*/

}


static u32 Dma::Read ( u32 Address, u64 Mask )
{
#if defined INLINE_DEBUG_READ
	debug << "\r\n\r\nDMA::Read; PC= " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
#endif

	static const u8 c_ucDMAChannel_LUT [ 32 ] = { 0, -1, -1, -1, 1, -1, -1, -1,
													2, -1, -1, -1, 3, 4, -1, -1,
													5, 6, 7, -1, 8, 9, -1, -1,
													-1, -1, -1, -1, -1, -1, -1, -1 };
													
	u32 DmaChannelNum;

	switch ( Address )
	{
		
		case CTRL:
#ifdef INLINE_DEBUG_READ_CTRL
	debug << "\r\n\r\nDMA::Read; PC= " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
#endif
#if defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_CTRL
			debug << "; CTRL = " << hex << _DMA->CTRL_Reg.Value;
#endif

			return _DMA->CTRL_Reg.Value;
			break;
			
		case STAT:
#ifdef INLINE_DEBUG_READ
			debug << "; STAT = " << hex << _DMA->STAT_Reg.Value;
#endif

			return _DMA->STAT_Reg.Value;
			break;


		case PCR:
#ifdef INLINE_DEBUG_READ
			debug << "; PCR = " << hex << _DMA->PCR_Reg.Value;
#endif

			return _DMA->PCR_Reg.Value;
			break;


		case DMA_SQWC:
#if defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_SQWC
			debug << "; SQWC= " << hex << _DMA->SQWC_Reg.Value;
#endif

			return _DMA->SQWC_Reg.Value;
			break;
			
		case DMA_RBOR:
#if defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_RBOR
			debug << "; RBOR= " << hex << _DMA->RBOR_Reg;
#endif

			return _DMA->RBOR_Reg;
			break;

		case DMA_RBSR:
#if defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_RBSR
			debug << "; RBSR= " << hex << _DMA->RBSR_Reg.Value;
#endif

			return _DMA->RBSR_Reg.Value;
			break;

		case DMA_STADR:
#if defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_STADR
			debug << "; STADR= " << hex << _DMA->STADR_Reg;
#endif

			return _DMA->STADR_Reg;
			break;

		case DMA_ENABLER:
#if defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_ENABLER
			debug << "; ENABLER= " << hex << _DMA->lDMAC_ENABLE;
#endif

			// dmac enable r
			return _DMA->lDMAC_ENABLE;
			break;
			
		case DMA_ENABLEW:
#if defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_ENABLEW
			debug << "; ENABLEW= " << hex << _DMA->lDMAC_ENABLE;
#endif

			// ???
			return _DMA->lDMAC_ENABLE;
			break;
			

			
		default:

			// get the dma channel that is being accessed
			DmaChannelNum = c_ucDMAChannel_LUT [ ( Address >> 10 ) & 0x1f ];
			
			if ( Address >= 0x10008000 && Address < 0x1000e000 && DmaChannelNum < c_iNumberOfChannels )
			{

				// get the dma channel number
				//DmaChannelNum = ( ( Address >> 4 ) & 0xf ) - 8;
				

				switch ( ( Address >> 4 ) & 0xf )
				{
					case 0:
#ifdef INLINE_DEBUG_READ_CHCR
	debug << "\r\n\r\nDMA::Read; PC= " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
#endif
#ifdef INLINE_DEBUG_READ || INLINE_DEBUG_READ_CHCR
					debug << "; DMA" << DmaChannelNum << "_CHCR = " << hex << _DMA->DmaCh [ DmaChannelNum ].CHCR_Reg.Value;
#endif

						return _DMA->DmaCh [ DmaChannelNum ].CHCR_Reg.Value;
						break;
					
					case 1:
#ifdef INLINE_DEBUG_READ
					debug << "; DMA" << DmaChannelNum << "_MADR = " << hex << _DMA->DmaCh [ DmaChannelNum ].MADR_Reg.Value;
#endif

						return _DMA->DmaCh [ DmaChannelNum ].MADR_Reg.Value;
						break;
					
					case 2:
#ifdef INLINE_DEBUG_READ
					debug << "; DMA" << DmaChannelNum << "_QWC = " << hex << _DMA->DmaCh [ DmaChannelNum ].QWC_Reg.Value;
#endif

						return _DMA->DmaCh [ DmaChannelNum ].QWC_Reg.Value;
						break;
				
					
					case 3:
#ifdef INLINE_DEBUG_READ
					debug << "; DMA" << DmaChannelNum << "_TADR = " << hex << _DMA->DmaCh [ DmaChannelNum ].TADR_Reg.Value;
#endif

						return _DMA->DmaCh [ DmaChannelNum ].TADR_Reg.Value;
						break;
					
					case 4:
#ifdef INLINE_DEBUG_READ
					debug << "; DMA" << DmaChannelNum << "_ASR0 = " << hex << _DMA->DmaCh [ DmaChannelNum ].ASR0_Reg.Value;
#endif

						return _DMA->DmaCh [ DmaChannelNum ].ASR0_Reg.Value;
						break;
					
					case 5:
#ifdef INLINE_DEBUG_READ
					debug << "; DMA" << DmaChannelNum << "_ASR1 = " << hex << _DMA->DmaCh [ DmaChannelNum ].ASR1_Reg.Value;
#endif

						return _DMA->DmaCh [ DmaChannelNum ].ASR1_Reg.Value;
						break;
					
					
					case 8:
#ifdef INLINE_DEBUG_READ
					debug << "; DMA" << DmaChannelNum << "_SADR = " << hex << _DMA->DmaCh [ DmaChannelNum ].SADR_Reg.Value;
#endif

						return _DMA->DmaCh [ DmaChannelNum ].SADR_Reg.Value;
						break;
					
						
					default:
#ifdef INLINE_DEBUG_READ_INVALID
	debug << "\r\n\r\nDMA::Read; PC= " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
#endif
#if defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_INVALID
						debug << "; Invalid";
#endif
		
						// invalid DMA Register
						cout << "\nhps2x64 ALERT: Unknown DMA READ @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << "\n";
						break;
				}
			}
			else
			{
#ifdef INLINE_DEBUG_READ_INVALID
	debug << "\r\n\r\nDMA::Read; PC= " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address;
#endif
#if defined INLINE_DEBUG_READ || defined INLINE_DEBUG_READ_INVALID
				debug << "; Invalid";
#endif

				cout << "\nhps2x64 WARNING: READ from unknown DMA Register @ Cycle#" << dec << *_DebugCycleCount << " PC=" << hex << *_DebugPC << " Address=" << Address << "\n";
			}

			break;
			
	}
			
	return 0;

}



static void Dma::Write ( u32 Address, u64 Data, u64 Mask )
{
#if defined INLINE_DEBUG_WRITE
	debug << "\r\n\r\nDMA::Write; PC= " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Data = " << Data;
#endif

	static const u8 c_ucDMAChannel_LUT [ 32 ] = { 0, -1, -1, -1, 1, -1, -1, -1,
													2, -1, -1, -1, 3, 4, -1, -1,
													5, 6, 7, -1, 8, 9, -1, -1,
													-1, -1, -1, -1, -1, -1, -1, -1 };

	u32 DmaChannelNum;
	u32 Temp;
	
	switch ( Address )
	{
		case CTRL:
#ifdef INLINE_DEBUG_WRITE_CTRL
	debug << "\r\n\r\nDMA::Write; PC= " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Data = " << Data;
#endif
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_CTRL
			debug << "; (Before) CTRL= " << hex << _DMA->CTRL_Reg.Value;
#endif

			_DMA->CTRL_Reg.Value = Data;
			
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_CTRL
			debug << "; (After) CTRL= " << hex << _DMA->CTRL_Reg.Value;
#endif
			break;

			
		case STAT:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_STAT
			debug << "; (Before) STAT= " << hex << _DMA->STAT_Reg.Value;
#endif

			// the bottom 16-bits get cleared when a one is written to the bit
			_DMA->STAT_Reg.Value &= ~( Data & 0xffff );
			
			// the upper 16-bits get inverted when a one is written to the bit
			// but keep bits 26-28 zero, and bit 31 zero
			_DMA->STAT_Reg.Value ^= ( Data & 0x63ff0000 );
			
			// *** TODO *** INT1 == CIS&CIM || BEIS
			// update interrupts when STAT gets modified
			_DMA->UpdateInterrupt ();

#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_STAT
			debug << "; (After) STAT= " << hex << _DMA->STAT_Reg.Value;
#endif

			break;


		case PCR:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_PCR
			debug << "; (Before) PCR= " << hex << _DMA->PCR_Reg.Value;
#endif

			_DMA->PCR_Reg.Value = Data;

#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_PCR
			debug << "; (After) PCR= " << hex << _DMA->PCR_Reg.Value;
#endif

			break;


		case DMA_SQWC:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_SQWC
			debug << "; (Before) SQWC= " << hex << _DMA->SQWC_Reg.Value;
#endif

			_DMA->SQWC_Reg.Value = Data;
			
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_SQWC
			debug << "; (After) SQWC= " << hex << _DMA->SQWC_Reg.Value;
#endif
			break;
			
		case DMA_RBOR:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_RBOR
			debug << "; (Before) RBOR= " << hex << _DMA->RBOR_Reg;
#endif

			_DMA->RBOR_Reg = Data;
			
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_RBOR
			debug << "; (After) RBOR= " << hex << _DMA->RBOR_Reg;
#endif
			break;

		case DMA_RBSR:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_RBSR
			debug << "; (Before) RBSR= " << hex << _DMA->RBSR_Reg.Value;
#endif

			_DMA->RBSR_Reg.Value = Data;
			
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_RBSR
			debug << "; (After) RBSR= " << hex << _DMA->RBSR_Reg.Value;
#endif
			break;

		case DMA_STADR:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_STADR
			debug << "; (Before) STADR= " << hex << _DMA->STADR_Reg;
#endif

			_DMA->STADR_Reg = Data;
			
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_STADR
			debug << "; (After) STADR= " << hex << _DMA->STADR_Reg;
#endif
			break;
			
			
		case DMA_ENABLER:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_ENABLER
			debug << "; (Before) ENABLER= " << hex << _DMA->lDMAC_ENABLE;
#endif

			// dmac enable r
			// READ ONLY
			
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_ENABLER
			debug << "; (After) ENABLER= " << hex << _DMA->lDMAC_ENABLE;
#endif
			break;
			
		case DMA_ENABLEW:
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_ENABLEW
			debug << "; (Before) ENABLEW= " << hex << _DMA->lDMAC_ENABLE;
#endif

			// check if there is a transition from one to zero
			if ( ( Data ^ 0x10000 ) & _DMA->lDMAC_ENABLE & 0x10000 )
			{
				// transition from zero to one, so store and update transfers
				_DMA->lDMAC_ENABLE = Data;
				_DMA->UpdateTransfer ();
			}
			

			// ???
			_DMA->lDMAC_ENABLE = Data;
			
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_ENABLEW
			debug << "; (After) ENABLEW= " << hex << _DMA->lDMAC_ENABLE;
#endif
			break;
			
			
		default:
		
			// get the dma channel that is being accessed
			DmaChannelNum = c_ucDMAChannel_LUT [ ( Address >> 10 ) & 0x1f ];
			
			if ( Address >= 0x10008000 && Address < 0x1000e000 && DmaChannelNum < c_iNumberOfChannels )
			{
				// get the dma channel number
				//DmaChannelNum = ( ( Address >> 4 ) & 0xf ) - 8;
				

				switch ( ( Address >> 4 ) & 0xf )
				{
					case 0:
#ifdef INLINE_DEBUG_WRITE_CHCR
	debug << "\r\n\r\nDMA::Write; PC= " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Data = " << Data;
#endif
#ifdef INLINE_DEBUG_WRITE
					debug << "; (Before) DMA" << DmaChannelNum << "_CHCR = " << hex << _DMA->DmaCh [ DmaChannelNum ].CHCR_Reg.Value;
#endif

						// must set the full value of CHCR
						_DMA->DmaCh [ DmaChannelNum ].CHCR_Reg.Value = Data;
						
						// check if set to start transfer (check for transition from one to zero)
						// before a dma transfer can start, need CTRL_Reg.DMAE AND (!PCR_Reg.PCE OR PCR.CDE) AND CHCR_Reg.STR
						if ( _DMA->CTRL_Reg.DMAE && ( !_DMA->PCR_Reg.PCE || ( _DMA->PCR_Reg.Value & ( 1 << ( DmaChannelNum + 16 ) ) ) ) && _DMA->DmaCh [ DmaChannelNum ].CHCR_Reg.STR )
						{
							// transfer is set to start //
							
							// start transfer
							_DMA->StartTransfer ( DmaChannelNum );
							_DMA->Transfer ( DmaChannelNum );
						}


#ifdef INLINE_DEBUG_WRITE
					debug << "; (After) DMA" << DmaChannelNum << "_CHCR = " << hex << _DMA->DmaCh [ DmaChannelNum ].CHCR_Reg.Value;
#endif

						break;
						
					
					case 1:
#ifdef INLINE_DEBUG_WRITE
					debug << "; (Before) DMA" << DmaChannelNum << "_MADR = " << hex << _DMA->DmaCh [ DmaChannelNum ].MADR_Reg.Value;
#endif



						_DMA->DmaCh [ DmaChannelNum ].MADR_Reg.Value = Data;
						

#ifdef INLINE_DEBUG_WRITE
					debug << "; (After) DMA" << DmaChannelNum << "_MADR = " << hex << _DMA->DmaCh [ DmaChannelNum ].MADR_Reg.Value;
#endif

						break;
				
					case 2:
#ifdef INLINE_DEBUG_WRITE
					debug << "; (Before) DMA" << DmaChannelNum << "_QWC = " << hex << _DMA->DmaCh [ DmaChannelNum ].QWC_Reg.Value;
#endif


						_DMA->DmaCh [ DmaChannelNum ].QWC_Reg.Value = Data;
						

#ifdef INLINE_DEBUG_WRITE
					debug << "; (After) DMA" << DmaChannelNum << "_QWC = " << hex << _DMA->DmaCh [ DmaChannelNum ].QWC_Reg.Value;
#endif

						break;

						
					case 3:
#ifdef INLINE_DEBUG_WRITE
					debug << "; (Before) DMA" << DmaChannelNum << "_TADR = " << hex << _DMA->DmaCh [ DmaChannelNum ].TADR_Reg.Value;
#endif



						_DMA->DmaCh [ DmaChannelNum ].TADR_Reg.Value = Data;
						

#ifdef INLINE_DEBUG_WRITE
					debug << "; (After) DMA" << DmaChannelNum << "_TADR = " << hex << _DMA->DmaCh [ DmaChannelNum ].TADR_Reg.Value;
#endif

						break;
						
						
					case 4:
#ifdef INLINE_DEBUG_WRITE
					debug << "; (Before) DMA" << DmaChannelNum << "_ASR0 = " << hex << _DMA->DmaCh [ DmaChannelNum ].ASR0_Reg.Value;
#endif



						_DMA->DmaCh [ DmaChannelNum ].ASR0_Reg.Value = Data;
						

#ifdef INLINE_DEBUG_WRITE
					debug << "; (After) DMA" << DmaChannelNum << "_ASR0 = " << hex << _DMA->DmaCh [ DmaChannelNum ].ASR0_Reg.Value;
#endif

						break;
						
						
					case 5:
#ifdef INLINE_DEBUG_WRITE
					debug << "; (Before) DMA" << DmaChannelNum << "_ASR1 = " << hex << _DMA->DmaCh [ DmaChannelNum ].ASR1_Reg.Value;
#endif



						_DMA->DmaCh [ DmaChannelNum ].ASR1_Reg.Value = Data;
						

#ifdef INLINE_DEBUG_WRITE
					debug << "; (After) DMA" << DmaChannelNum << "_ASR1 = " << hex << _DMA->DmaCh [ DmaChannelNum ].ASR1_Reg.Value;
#endif

						break;
						
						
					case 8:
#ifdef INLINE_DEBUG_WRITE
					debug << "; (Before) DMA" << DmaChannelNum << "_SADR = " << hex << _DMA->DmaCh [ DmaChannelNum ].SADR_Reg.Value;
#endif



						_DMA->DmaCh [ DmaChannelNum ].SADR_Reg.Value = Data;
						

#ifdef INLINE_DEBUG_WRITE
					debug << "; (After) DMA" << DmaChannelNum << "_SADR = " << hex << _DMA->DmaCh [ DmaChannelNum ].SADR_Reg.Value;
#endif

						break;
						
						
					default:
#ifdef INLINE_DEBUG_WRITE_INVALID
	debug << "\r\n\r\nDMA::Write; PC= " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Data = " << Data;
#endif
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_INVALID
						debug << "; Invalid";
#endif
		
						// invalid DMA Register
						cout << "\nhps2x64 ALERT: Unknown DMA WRITE @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << " Data=" << Data << "\n";
						break;
					
				}
				
			}
			else
			{
#ifdef INLINE_DEBUG_WRITE_INVALID
	debug << "\r\n\r\nDMA::Write; PC= " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << " Address = " << hex << Address << "; Data = " << Data;
#endif
#if defined INLINE_DEBUG_WRITE || defined INLINE_DEBUG_WRITE_INVALID
				debug << "; Invalid";
#endif

				cout << "\nhps2x64 WARNING: WRITE to unknown DMA Register @ Cycle#" << dec << *_DebugCycleCount << " PC=" << hex << *_DebugPC << " Address=" << Address << "\n";
			}

			break;
	}


}




void Dma::StartTransfer ( int iChannel )
{
#ifdef INLINE_DEBUG_START
	debug << "\r\nDma::StartTransfer; Channel#" << dec << iChannel;
#endif

	// check if channel is already running and was simply suspended
	if ( QWC_Transferred [ iChannel ] >= 0 )
	{
#ifdef INLINE_DEBUG_START
	debug << "\r\nRestarting suspended Dma Channel#" << dec << iChannel;
#endif

		// continue the transfer
		Transfer ( iChannel );
		
		// done here
		return;
	}

	// clear previous tag in CHCR
	// must write tag back anyway when writing to CHCR
	//DmaCh [ iChannel ].CHCR_Reg.Value &= 0xffff;
	
	// reset the QWC that has been transferred in current block
	// -1 means that block still needs to be started
	QWC_Transferred [ iChannel ] = -1;

	switch ( iChannel )
	{
		// SIF0
		case 5:
			
			// starting dma#5 sets bit 13 (0x2000) in SBUS CTRL register
			//DataBus::_BUS->lSBUS_F240 |= 0x2000;
			*_SBUS_F240 |= 0x2000;
			
			break;
			
		// SIF1
		case 6:
		
			// starting dma#6 sets bit 14 (0x4000) in SBUS CTRL register
			//DataBus::_BUS->lSBUS_F240 |= 0x4000;
			*_SBUS_F240 |= 0x4000;
		
			break;
	}
}


void Dma::EndTransfer ( int iChannel )
{
#ifdef INLINE_DEBUG_END
	debug << "\r\nDma::EndTransfer; Channel#" << dec << iChannel;
	debug << "; (before) CHCR=" << hex << DmaCh [ iChannel ].CHCR_Reg.Value << " STAT=" << STAT_Reg.Value;
#endif

	// channel is done with transfer //
	
	// clear STR for channel
	DmaCh [ iChannel ].CHCR_Reg.STR = 0;
	
	// set CIS for channel - (STAT register)
	STAT_Reg.Value |= ( 1 << iChannel );
	
	// check for interrupt
	UpdateInterrupt ();

	switch ( iChannel )
	{
		// SIF0
		case 5:
			
			// ending dma#5 clears bits 5 (0x20) and 13 (0x2000) in SBUS CTRL register
#ifdef ENABLE_SIF_DMA_SYNC
			*_SBUS_F240 &= ~0x2000;
#else
			//DataBus::_BUS->lSBUS_F240 &= ~0x2020;
			*_SBUS_F240 &= ~0x2020;
#endif
			
			break;
			
		// SIF1
		case 6:
		
			// ending dma#6 clears bits 6 (0x40) and 14 (0x4000) in SBUS CTRL register
#ifdef ENABLE_SIF_DMA_SYNC
			*_SBUS_F240 &= ~0x4000;
#else
			//DataBus::_BUS->lSBUS_F240 &= ~0x4040;
			*_SBUS_F240 &= ~0x4040;
#endif
		
			break;
	}
	
#ifdef ENABLE_SIF_DMA_TIMING
	// if channel#5, then check if channel#6 is ready to go since it would have been held up
	if( iChannel == 5 )
	{
		SIF::_SIF->Check_TransferToIOP ();
	}
#endif
	
#ifdef INLINE_DEBUG_END
	debug << "; (after) CHCR=" << hex << DmaCh [ iChannel ].CHCR_Reg.Value << " STAT=" << STAT_Reg.Value;
#endif
}


// need to call this whenever updating interrupt related registers
void Dma::UpdateInterrupt ()
{
#ifdef INLINE_DEBUG_INT
	debug << "; Dma::UpdateInterrupt";
#endif

	// check for interrupt
	if ( STAT_Reg.Value & ( STAT_Reg.Value >> 16 ) & 0x63ff )
	{
#ifdef INLINE_DEBUG_INT
	debug << "; INT";
#endif

		// interrupt (SET INT1 on R5900)
		SetInterrupt ();
	}
	else
	{
#ifdef INLINE_DEBUG_INT
	debug << "; CLEAR_INT";
#endif

		// this should clear the interrupt on INT1 on R5900
		ClearInterrupt ();
	}
}


void Dma::DMA5_WriteBlock ( u64 EEDMATag, u64* Data64, u32 QWC_Count )
{
#ifdef INLINE_DEBUG_RUN_DMA5
	debug << "\r\nDMA5_WriteBlock: DMA5: SIF0 IOP->EE";
	debug << " QWC=" << dec << QWC_Count;
	debug << " Cycle#" << dec << *_DebugCycleCount;
#endif

	// dma transfer has been started //
	
	u32 Temp;
	//u32 Data [ 4 ];
	//u32 NumberOfTransfers;
	
	u32 TransferCount;
	u32 QWC_Remaining;
	
	u32 Data0, Data1, DestAddress, IRQ, ID;
	
	//DMATag EETag;
	//EETag.Value = EEDMATag;
	
	u64 *DstPtr64;
	
	bool TransferInProgress = true;
	
	// this delay should be handled at the source of the transfer
	static u64 CycleDelay = 0;
	
	/*
	if ( CycleDelay > *_DebugCycleCount )
	{
		CycleDelay += c_llSIFDelayPerQWC * (u64) Count;
	}
	else
	{
		CycleDelay = *_DebugCycleCount + (u64) ( c_llSIFDelayPerQWC * Count );
	}
	*/

	//Data0 = *Data++;
	//Data1 = *Data++;
	

	// rest of quadword is ignored
	//Data++;
	//Data++;
	

//#ifdef INLINE_DEBUG_RUN_DMA5
//#endif

	// transfer all the data that was sent
	while ( TransferInProgress )
	{
		// if in destination chain mode, then pull tag and set address first
		if ( DmaCh [ 5 ].CHCR_Reg.MOD == 1 )
		{

			if ( !DmaCh [ 5 ].QWCRemaining )
			{
#ifdef INLINE_DEBUG_RUN_DMA5
	debug << "; QWCRemaining=0 -> Getting TAG";
	debug << "; EETag=" << hex << EEDMATag;
#endif

				// set the tag
				SourceDMATag [ 5 ].Value = EEDMATag;
				
				// subtract from count of data sent
				// the QWC in TAG does not include the tag
				//QWC_Count--;
				
				// set MADR
				//DmaCh [ 5 ].MADR_Reg.Value = EETag.ADDR & DataBus::MainMemory_Mask;
				DmaCh [ 5 ].MADR_Reg.Value = ( SourceDMATag [ 5 ].Value >> 32 );
				
				// set the QWC to transfer
				DmaCh [ 5 ].QWCRemaining = SourceDMATag [ 5 ].QWC;
			
				// also set upper bits to tag upper bits in chcr
				DmaCh [ 5 ].CHCR_Reg.Value = ( DmaCh [ 5 ].CHCR_Reg.Value & 0xffffL ) | ( SourceDMATag [ 5 ].Value & 0xffff0000L );
				
				// have not transferred anything for current tag yet
				DmaCh [ 5 ].QWCTransferred = 0;
			
#ifdef INLINE_DEBUG_RUN_DMA5
	debug << " EETag.QWC=" << dec << SourceDMATag [ 5 ].QWC;
	debug << " Tag.ID=" << SourceDMATag [ 5 ].ID << " Tag.IRQ=" << SourceDMATag [ 5 ].IRQ << " Tag.PCE=" << SourceDMATag [ 5 ].PCE;
	debug << "; Tag.MADR=" << hex << DmaCh [ 5 ].MADR_Reg.Value;
#endif
			}

			// check if there is data remaining in transfer from IOP besides the EE tag
			if ( !QWC_Count )
			{
				// will need to wait for more data
				return;
			}

			//DstPtr64 = & DataBus::_BUS->MainMemory.b64 [ ( ( DmaCh [ 5 ].MADR_Reg.Value >> 3 ) + ( DmaCh [ 5 ].QWCTransferred << 1 ) ) & ~1 ];
			DstPtr64 = GetMemoryPtr ( DmaCh [ 5 ].MADR_Reg.Value + ( DmaCh [ 5 ].QWCTransferred << 4 ) );



			// check the ID
			switch ( SourceDMATag [ 5 ].ID )
			{
				// ID: CNTS
				case 0:
#ifdef INLINE_DEBUG_RUN_DMA5
	debug << "; ID=CNTS";
	debug << "; NOT IMPLEMENTED";
#endif

					// need some type of message here..
					cout << "\nhps2x64: ***ALERT***: DMA Destination Tag CNTS not implemented\n";
					break;
					
				// ID: CNT
				case 1:
#ifdef INLINE_DEBUG_RUN_DMA5
	debug << "; ID=CNT";
#endif

					TransferCount = ( QWC_Count > DmaCh [ 5 ].QWCRemaining ) ? DmaCh [ 5 ].QWCRemaining : QWC_Count;

#ifdef INLINE_DEBUG_RUN_DMA5
	debug << "\r\n***EE SIF0 (IOP->EE) Writing QWC=" << dec << TransferCount << hex << " to MADR=" << ( DmaCh [ 5 ].MADR_Reg.Value + ( DmaCh [ 5 ].QWCTransferred << 4 ) ) << " Cycle#" << dec << *_DebugCycleCount << "\r\n";
#endif

					// transfer the data after the tag
					//for ( int i = 0; i < EETag.QWC; i++ )
					for ( int i = 0; i < TransferCount; i++ )
					{
						// transfer 128-bit quadword
						*DstPtr64++ = *Data64++;
						*DstPtr64++ = *Data64++;
						//*DstPtr++ = *Data++;
						//*DstPtr++ = *Data++;
					}
					
					// update QWC Transferred for tag
					DmaCh [ 5 ].QWCTransferred += TransferCount;
					QWC_Count -= TransferCount;
					
					DmaCh [ 5 ].QWCRemaining -= TransferCount;
					
#ifdef INLINE_DEBUG_RUN_DMA5
	debug << " Transferred=" << dec << TransferCount;
	debug << " Remaining=" << dec << DmaCh [ 5 ].QWCRemaining;
#endif

					// check transfer is complete
					if ( DmaCh [ 5 ].QWCRemaining <= 0 )
					{
						// make sure this value is zero
						DmaCh [ 5 ].QWCRemaining = 0;
						
						// check if IRQ requested
						if ( SourceDMATag [ 5 ].IRQ )
						{
#ifdef ENABLE_SIF_DMA_TIMING
							// actually end the transfer after data has been transferred serially
							// this delay should be handled at the source of the transfer
							SetNextEventCh_Cycle ( CycleDelay, 5 );
							
							// let the SIF know that the EE will be busy for awhile with the data just transferred
							SIF::_SIF->EE_BusyUntil ( CycleDelay );
#else
							// set CIS
							//Stat_Reg.Value |= ( 1 << 5 );
							
							// clear STR
							//DmaCh [ 5 ].CHCR_Reg.STR = 0;
							
							// end transfer
							EndTransfer ( 5 );
							
							// interrupt for sif ??
							//SIF::SetInterrupt_EE_SIF ();
							
							// done
							return;
#endif
						}
						
						// has not transferred new tag yet, so need to return until next transfer comes in to get the new tag
						return;
					}
					
					break;
				
				// ID: END
				case 7:
#ifdef INLINE_DEBUG_RUN_DMA5
	debug << "; ID=END";
	debug << "; NOT IMPLEMENTED";
#endif

					// need some type of message here..
					cout << "\nhps2x64: ***ALERT***: DMA Destination Tag END not implemented\n";
					break;
				
				default:
#ifdef INLINE_DEBUG_RUN_DMA5
	debug << "; DestTag=Unimplemented/Unknown";
#endif

					// need some type of message here..
					cout << "\nhps2x64: ***ALERT***: DMA Destination Tag UNKNOWN/IMPOSSIBLE not implemented\n";
					break;
			}	// end switch ( ID )
		
		}	// end if
		else
		{
			// not in destination chain mode for channel 5??
			cout << "\nhps2x64: DMA: ***ALERT***: DMA Channel#5 not in destination chain mode.\n";
		}
		
	}	// end while ( QWC_Count )
}




void Dma::NormalTransfer_ToMemory ( int iChannel )
{
}

void Dma::NormalTransfer_FromMemory ( int iChannel )
{
	u64 Data0, Data1;
	DMATag SrcDtag;
	DMATag DstDtag;
	
	u32 SrcAddress, DstAddress, TransferCount, TransferIndex;
	u64 *SrcDataPtr, *DstDataPtr;
	
	u32 NextTagAddress, NextDataAddress;
	
	u64 QWC_TransferCount;
	
	static const u64 c_LoopTimeout = 33554432;
	u64 LoopCount;
	
	bool TransferInProgress = true;
	
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; Normal";
#endif

#ifdef INLINE_DEBUG_TRANSFER
	debug << "; FromMemory";
	debug << " MADR=" << hex << DmaCh [ iChannel ].MADR_Reg.Value;
#endif

	for ( LoopCount = 0; LoopCount < c_LoopTimeout; LoopCount++ )
	{
	
#ifdef INLINE_DEBUG_TRANSFER
	debug << " @Cycle#" << dec << *_DebugCycleCount;
#endif

		// check if transfer of block has started yet
		if ( QWC_Transferred [ iChannel ] < 0 )
		{
			// set the amount total in the block to be transferred
			QWC_BlockTotal [ iChannel ] = DmaCh [ iChannel ].QWC_Reg.Value;
			
			// nothing transferred yet
			QWC_Transferred [ iChannel ] = 0;
		}

		
		// check if channel has a ready function to check if its ready
		// if not, then skip
		if ( cbReady [ iChannel ] )
		{
			// check that channel is ready
			if ( !cbReady [ iChannel ] )
			{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; ChannelNOTReady";
#endif

				return;
			}
		}
		
		
		// check if channel has a transfer function
		// if not, then unable to transfer
		if ( !cbTransfer_FromMemory [ iChannel ] )
		{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; ***TransferNOTImplemented***";
#endif

			return;
		}
		
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; TransferingData";
#endif


		// transfer all data at once for now
		
		// get pointer to source data
		//SrcDataPtr = & DataBus::_BUS->MainMemory.b64 [ ( ( DmaCh [ iChannel ].MADR_Reg.Value & DataBus::MainMemory_Mask ) & ~0xf ) >> 3 ];
		SrcDataPtr = GetMemoryPtr ( DmaCh [ iChannel ].MADR_Reg.Value );
		
		// get pointer to where transfer last left off from
		// not sure if dma has an internal counter... will tak a look at this later
		//SrcDataPtr = & ( SrcDataPtr [ QWC_Transferred [ iChannel ] << 1 ] );

		// the amount of data to attemp to transfer depends on the buffer size for the device

#ifdef TEST_ASYNC_DMA
		QWC_TransferCount = ( c_iDeviceBufferSize [ iChannel ] < ( QWC_BlockTotal [ iChannel ] - QWC_Transferred [ iChannel ] ) ) ? c_iDeviceBufferSize [ iChannel ] : ( QWC_BlockTotal [ iChannel ] - QWC_Transferred [ iChannel ] );
		
		// only transfer if there is data to transfer
		if ( QWC_TransferCount )
		{
			QWC_TransferCount = cbTransfer_FromMemory [ iChannel ] ( SrcDataPtr, QWC_TransferCount );
		}
#else
		// perform transfer
		QWC_TransferCount = cbTransfer_FromMemory [ iChannel ] ( SrcDataPtr, QWC_BlockTotal [ iChannel ] - QWC_Transferred [ iChannel ] );
#endif
	
#ifdef INLINE_DEBUG_TRANSFER
		debug << " QWC_TransferCount=" << hex << QWC_TransferCount;
#endif

		// update address for next data to transfer
		//DmaCh [ iChannel ].MADR_Reg.Value += ( (u32) DmaCh [ iChannel ].QWC_Reg.QWC ) << 4;
		DmaCh [ iChannel ].MADR_Reg.Value += QWC_TransferCount << 4;
		
		// update QWC transferred so far
		QWC_Transferred [ iChannel ] += QWC_TransferCount;
		
		// check if all data in the block has transferred
		if ( QWC_Transferred [ iChannel ] >= QWC_BlockTotal [ iChannel ] )
		{
			// all the data has been transferred in block, since this is not a chain transfer we are done
			EndTransfer ( iChannel );
			
			// transfer complete
			QWC_Transferred [ iChannel ] = -1;
			return;
		}
#ifdef TEST_ASYNC_DMA_STAGE2
		else if ( c_iDmaTransferTimePerQwc [ iChannel ] )
		{
			// if transfer is not finished, then schedule transfer to continue later
			SetNextEventCh ( ( c_iDmaTransferTimePerQwc [ iChannel ] * QWC_TransferCount ) + c_iSetupTime, iChannel );
			
			// continue transfer later
			return;
		}
#endif

	} // for LoopCount
	
	// if code ever reaches here, that means there was a timeout
	cout << "\nhps2x64 ERROR: Normal DMA Transfer to Channel#" << iChannel << " TIMED OUT";
}

void Dma::ChainTransfer_ToMemory ( int iChannel )
{
}


// should return the amount of data transferred
u64 Dma::Chain_TransferBlock ( int iChannel )
{
	u64 *TagDataPtr, *SrcDataPtr;
	u32 QWC_TransferCount;
	u64 ullTagToTransfer [ 2 ];

	SrcDataPtr = GetMemoryPtr ( DmaCh [ iChannel ].MADR_Reg.Value );
	
	// check if transfer is just starting
	if ( QWC_Transferred [ iChannel ] < 0 )
	{

		// check if tag should be transferred
		// only transfer tag if transfer count > 0
		//if ( DmaCh [ iChannel ].CHCR_Reg.TTE && TransferCount )
		if ( DmaCh [ iChannel ].CHCR_Reg.TTE && QWC_BlockTotal [ iChannel ] )
		{
			
			// check if channel has a transfer function
			if ( cbTransfer_FromMemory [ iChannel ] )
			{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; TagTransfer";
#endif

				// Tag should be transferred //
				
				// perform transfer
				// *** transfer tag ***
				/*
				if ( !DmaCh [ iChannel ].TADR_Reg.SPR )
				{
					// data source is Main Memory //
					TagDataPtr = & DataBus::_BUS->MainMemory.b64 [ ( ( DmaCh [ iChannel ].TADR_Reg.Value & DataBus::MainMemory_Mask ) & ~0xf ) >> 3 ];
				}
				else
				{
					// data source is Scratchpad //
					TagDataPtr = & DataBus::_BUS->ScratchPad.b64 [ ( ( DmaCh [ iChannel ].TADR_Reg.Value & DataBus::ScratchPad_Mask ) & ~0xf ) >> 3 ];
				}
				*/
				
				TagDataPtr = GetMemoryPtr ( DmaCh [ iChannel ].TADR_Reg.Value );
				
				// ***TODO*** when transferring tag, need to zero bottom 64-bits
				ullTagToTransfer [ 0 ] = 0;
				ullTagToTransfer [ 1 ] = TagDataPtr [ 1 ];
				
				//cbTransfer_FromMemory [ iChannel ] ( SrcDataPtr, 1 );
				QWC_TransferCount = cbTransfer_FromMemory [ iChannel ] ( ullTagToTransfer, 1 );
				
				// update amount transferred
				//QWC_Transferred [ iChannel ] += QWC_TransferCount;
				QWC_Transferred [ iChannel ] = 0;
			}
			else
			{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "\r\n***ChainTransferNotImplemented; DMA Channel Number=" << dec << iChannel << " ***";
#endif

				// no transfer function, no transfer
				return;
			}
			
			// before and after check is needed for transfer of TAG
			// since flush command could be dropped in
			// might need to fix up better later
			if ( cbReady [ iChannel ] )
			{
				// check if channel is ready for transfer
				if ( !( cbReady [ iChannel ] () ) )
				{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; DeviceNotReady";
#endif

					return 0;
				}
			}
			
		}
		else
		{
			// tag should NOT be transferred //
			
			// if transfer is just starting but no tag transferring, then enable transfer of data
			QWC_Transferred [ iChannel ] = 0;
		}
		
#ifdef INLINE_DEBUG_TRANSFER
	debug << " QWC_Transferred=" << hex << QWC_Transferred [ iChannel ];
	debug << " ADDR=" << SourceDMATag [ iChannel ].ADDR;
#endif
	}


//#ifdef INLINE_DEBUG_TRANSFER
//	debug << " SrcDataPtr=" << (u64) SrcDataPtr;
//	debug << " Ptr=" << (u64) & ( DataBus::_BUS->MainMemory.b64 [ ( SourceDMATag [ iChannel ].ADDR & DataBus::MainMemory_Mask ) >> 3 ] );
//#endif
	
	// added - update source data pointer so that it points to the current quadword to transfer next
	SrcDataPtr = & ( SrcDataPtr [ QWC_Transferred [ iChannel ] << 1 ] );

	
#ifdef INLINE_DEBUG_TRANSFER
	debug << "\r\n***DMA#" << iChannel << " " << DmaCh_Names [ iChannel ] << " Transfering QWC=" << hex << SourceDMATag [ iChannel ].QWC << " from ADDR=" << SourceDMATag [ iChannel ].ADDR << " TADR=" << DmaCh [ iChannel ].TADR_Reg.Value << " MADR=" << DmaCh [ iChannel ].MADR_Reg.Value << " EETag=" << SourceDMATag [ iChannel ].Value << " IOPTag=" << *((u64*) SrcDataPtr) << "\r\n";
#endif

	//if ( TransferCount )
	if ( QWC_BlockTotal [ iChannel ] )
	{
		
		// check if channel has a transfer function
		if ( cbTransfer_FromMemory [ iChannel ] )
		{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "\r\nLeft(QWC)=" << dec << ( QWC_BlockTotal [ iChannel ] - QWC_Transferred [ iChannel ] );
#endif

#ifdef TEST_ASYNC_DMA
			QWC_TransferCount = ( c_iDeviceBufferSize [ iChannel ] < ( QWC_BlockTotal [ iChannel ] - QWC_Transferred [ iChannel ] ) ) ? c_iDeviceBufferSize [ iChannel ] : ( QWC_BlockTotal [ iChannel ] - QWC_Transferred [ iChannel ] );
			
			// only transfer if there is data to transfer
			if ( QWC_TransferCount )
			{
				QWC_TransferCount = cbTransfer_FromMemory [ iChannel ] ( SrcDataPtr, QWC_TransferCount );
			}
#else
			// perform transfer
			//QWC_TransferCount = cbTransfer_FromMemory [ iChannel ] ( SrcDataPtr, TransferCount );
			QWC_TransferCount = cbTransfer_FromMemory [ iChannel ] ( SrcDataPtr, QWC_BlockTotal [ iChannel ] - QWC_Transferred [ iChannel ] );
#endif
			
//#ifdef INLINE_DEBUG_TRANSFER
//	debug << " QWC_TransferCount=" << hex << QWC_TransferCount;
//#endif

			// update total data transferred
			QWC_Transferred [ iChannel ] += QWC_TransferCount;
			
#ifdef INLINE_DEBUG_TRANSFER
	debug << "\r\nTransferred(QWC)=" << dec << QWC_TransferCount;
	debug << "\r\nRemaining(QWC)=" << dec << ( QWC_BlockTotal [ iChannel ] - QWC_Transferred [ iChannel ] );
#endif
		}
		else
		{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "\r\n***ChainTransferNotImplemented; DMA Channel Number=" << dec << iChannel << " ***";
#endif

			// no transfer function, no transfer
			return 0;
		}
	}
	
	// return the number of QWs transferred to device
	return QWC_TransferCount;
}


void Dma::ChainTransfer_FromMemory ( int iChannel )
{
	u64 Data0, Data1;
	//DMATag SrcDtag;
	DMATag DstDtag;
	
	u64 ullTagToTransfer [ 2 ];
	
	u32 SrcAddress, DstAddress, TransferCount, TransferIndex;
	u64 *SrcDataPtr, *DstDataPtr, *TagDataPtr;
	
	u32 NextTagAddress, NextDataAddress;
	
	u64 QWC_TransferCount;
	
	bool TransferInProgress = true;
	
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; Chain (Source [FromMemory])";
#endif

				

	while ( TransferInProgress )
	{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; TagSource: MainMemory";
	debug << " @Cycle#" << dec << *_DebugCycleCount;
#endif


		// make sure that device is ready for transfer of the next block
		// check if device is ready for transfer
		// check if channel has a ready function
		if ( cbReady [ iChannel ] )
		{
			// check if channel is ready for transfer
			if ( !( cbReady [ iChannel ] () ) )
			{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; DeviceNotReady";
#endif

				return;
			}
		}
		
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; DeviceIsReady";
#endif

		// check if transfer of block/tag has started yet
		if ( QWC_Transferred [ iChannel ] < 0 )
		{
		
#ifdef INLINE_DEBUG_TRANSFER
	debug << "\r\nDMA#" << iChannel << " Loading Tag from TADR=" << hex << DmaCh [ iChannel ].TADR_Reg.Value << "\r\n";
#endif

			// load 128-bits from the tag address
			TagDataPtr = GetMemoryPtr ( DmaCh [ iChannel ].TADR_Reg.Value );
			Data0 = TagDataPtr [ 0 ];
			Data1 = TagDataPtr [ 1 ];

#ifdef INLINE_DEBUG_TRANSFER
	debug << "; TagData0=" << hex << Data0 << "; TagData1=" << Data1;
#endif

			// looks like the upper 32-bits of Data0 is the address and the lower 32-bits are ID/PCE/FLG etc
			// Data1 appears to be zero and not used anyway
			// this is where I'll need to process the TAG since the address could mean anything
			
			// set the source dma tag
			//SrcDtag.Value = Data0;
			SourceDMATag [ iChannel ].Value = Data0;
			
			// set bits 16-31 of CHCR to the last tag read bits 16-31
			//DmaCh [ iChannel ].CHCR_Reg.Value = ( DmaCh [ iChannel ].CHCR_Reg.Value & 0xffffL ) | ( SrcDtag.Value & 0xffff0000L );
			DmaCh [ iChannel ].CHCR_Reg.Value = ( DmaCh [ iChannel ].CHCR_Reg.Value & 0xffffL ) | ( SourceDMATag [ iChannel ].Value & 0xffff0000L );
			
			// ?? put address into MADR ??
			//DmaCh [ iChannel ].MADR_Reg.Value = SrcDtag.ADDR;
			
			// set the transfer count
			//TransferCount = SrcDtag.QWC;
			//QWC_BlockTotal [ iChannel ] = SrcDtag.QWC;
			QWC_BlockTotal [ iChannel ] = SourceDMATag [ iChannel ].QWC;
			
			// ***todo*** might need to add onto block total if tag is being transferred too
			// instead set to -1 if transferring tag
			
			//if ( ! DmaCh [ iChannel ].CHCR_Reg.TTE )
			//{
			//	// transfer of block/tag is starting now
			//	QWC_Transferred [ iChannel ] = 0;
			//}
			//else
			//{
				QWC_Transferred [ iChannel ] = -1;
			//}
		}
		
		// check ID
		//switch ( SrcDtag.ID )
		switch ( SourceDMATag [ iChannel ].ID )
		{
			// REFE
			// Next Data Address: ADDR
			// Next Tag Address: None (end of transfer)
			case 0:
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; SourceDMATag=REFE";
#endif

				// set MADR ??
				DmaCh [ iChannel ].MADR_Reg.Value = SourceDMATag [ iChannel ].Value >> 32;
				
				QWC_TransferCount = Chain_TransferBlock ( iChannel );


				// ***TODO*** check if MADR gets updated
				
				
				// check if all data has transferred in block
				if ( QWC_Transferred [ iChannel ] >= QWC_BlockTotal [ iChannel ] )
				{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; TRANSFEREND";
#endif

					// this is an REFE tag, so transfer ends here
					EndTransfer ( iChannel );
					
					// update the tag address after transfer - even on REFE tag
					DmaCh [ iChannel ].TADR_Reg.Value += 16;
					
					// start new tag
					QWC_Transferred [ iChannel ] = -1;
					
					// transfer is no longer in progress
					//TransferInProgress = false;
					return;
				}
				
				break;
				

			// CNT
			// Next Data Address: Next to tag
			// Next Tag Address: Next to data
			case 1:
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; SourceDMATag=CNT";
#endif

				// data is Next to tag (set/update MADR ??)
				DmaCh [ iChannel ].MADR_Reg.Value = DmaCh [ iChannel ].TADR_Reg.Value + 16;
				

				QWC_TransferCount = Chain_TransferBlock ( iChannel );
				
				
				// ***TODO*** check if MADR gets updated
				
				
				// check if transfer of block is complete
				if ( QWC_Transferred [ iChannel ] >= QWC_BlockTotal [ iChannel ] )
				{
					// next tag is next to data
					//DmaCh [ iChannel ].TADR_Reg.Value += ( TransferCount << 4 ) + 16;
					DmaCh [ iChannel ].TADR_Reg.Value += ( QWC_BlockTotal [ iChannel ] << 4 ) + 16;
					
					// start new tag
					QWC_Transferred [ iChannel ] = -1;
					
					// if IRQ bit is set, then transfer no longer in progress
					// also check Tag Interrupt Enable (TIE)
					if ( SourceDMATag [ iChannel ].IRQ && DmaCh [ iChannel ].CHCR_Reg.TIE )
					{
						// interrupt??
						EndTransfer ( iChannel );
						
						//TransferInProgress = false;
						return;
					}
				}
				
				
				break;


				
			// NEXT
			// Next Data Address: Next to tag
			// Next Tag Address: ADDR
			case 2:
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; SourceDMATag=NEXT";
#endif

				// data is Next to tag (set/update MADR ??)
				DmaCh [ iChannel ].MADR_Reg.Value = DmaCh [ iChannel ].TADR_Reg.Value + 16;
				
				
				QWC_TransferCount = Chain_TransferBlock ( iChannel );
				
				
				
				// ***TODO*** check if MADR gets updated
				
				
				// check if transfer of block is complete
				if ( QWC_Transferred [ iChannel ] >= QWC_BlockTotal [ iChannel ] )
				{
					// next tag is ADDR
					//DmaCh [ iChannel ].TADR_Reg.Value = SrcDtag.ADDR;
					// need the full value, including SPR bit
					DmaCh [ iChannel ].TADR_Reg.Value = SourceDMATag [ iChannel ].Value >> 32;
					
					// start new tag
					QWC_Transferred [ iChannel ] = -1;
					
					// if IRQ bit is set, then transfer no longer in progress
					if ( SourceDMATag [ iChannel ].IRQ && DmaCh [ iChannel ].CHCR_Reg.TIE )
					{
						// interrupt??
						EndTransfer ( iChannel );
						
						//TransferInProgress = false;
						return;
					}
				}
				
				break;
				
				
			// REF - Data to transfer is at ADDR - next tag is next to the tag
			// Next Data Address: ADDR
			// Next Tag Address: Next to tag
			case 3:
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; SourceDMATag=REF";
#endif

				// set MADR ??
				DmaCh [ iChannel ].MADR_Reg.Value = SourceDMATag [ iChannel ].Value >> 32;

				QWC_TransferCount = Chain_TransferBlock ( iChannel );
				
				
				// ***TODO*** check if MADR gets updated
				
				
				
				// check if transfer of block is complete
				if ( QWC_Transferred [ iChannel ] >= QWC_BlockTotal [ iChannel ] )
				{
					// update the tag address
					DmaCh [ iChannel ].TADR_Reg.Value += 16;
					
					// start new tag
					QWC_Transferred [ iChannel ] = -1;
					
					// if IRQ bit is set, then transfer no longer in progress
					if ( SourceDMATag [ iChannel ].IRQ && DmaCh [ iChannel ].CHCR_Reg.TIE )
					{
						// interrupt??
						EndTransfer ( iChannel );
						
						//TransferInProgress = false;
						return;
					}
				}
				
				break;
			
			
			// CALL
			// transfer QWs after the tag using QWC, pusing the following address onto ASR stack, and use ADDR for next tag
			case 5:
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; SourceDMATag=CALL";
#endif

				// data is Next to tag (set/update MADR ??)
				DmaCh [ iChannel ].MADR_Reg.Value = DmaCh [ iChannel ].TADR_Reg.Value + 16;
				
				
				QWC_TransferCount = Chain_TransferBlock ( iChannel );
				
				// check if transfer of block is complete
				if ( QWC_Transferred [ iChannel ] >= QWC_BlockTotal [ iChannel ] )
				{
					// tag to push onto stack is next to data
					DmaCh [ iChannel ].TADR_Reg.Value += ( QWC_BlockTotal [ iChannel ] << 4 ) + 16;
					
					// push address onto stack
					if ( DmaCh [ iChannel ].CHCR_Reg.ASP < 2 )
					{
						switch ( DmaCh [ iChannel ].CHCR_Reg.ASP )
						{
							case 0:
								DmaCh [ iChannel ].ASR0_Reg.Value = DmaCh [ iChannel ].TADR_Reg.Value;
								break;
								
							case 1:
								DmaCh [ iChannel ].ASR1_Reg.Value = DmaCh [ iChannel ].TADR_Reg.Value;
								break;
						}
						
						// increase the amound pushed onto stack
						DmaCh [ iChannel ].CHCR_Reg.ASP += 1;
					}
					else
					{
						// overflow
						cout << "\nhps2x64: ***ALERT*** DMA ASP overflow on CALL tag.\n";
						
						// probably also means done
						
						// start new tag
						QWC_Transferred [ iChannel ] = -1;
						
						// interrupt??
						EndTransfer ( iChannel );
						
						//TransferInProgress = false;
						return;
					}
					
					// update the tag address
					// next tag is ADDR
					DmaCh [ iChannel ].TADR_Reg.Value = SourceDMATag [ iChannel ].Value >> 32;
					
					// start new tag
					QWC_Transferred [ iChannel ] = -1;
					
					// if IRQ bit is set, then transfer no longer in progress
					if ( SourceDMATag [ iChannel ].IRQ && DmaCh [ iChannel ].CHCR_Reg.TIE )
					{
						// interrupt??
						EndTransfer ( iChannel );
						
						//TransferInProgress = false;
						return;
					}
				}
				
				break;
				
			// RET
			// transfer QWs after tag and reads ASR from stack as the next tag
			case 6:
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; SourceDMATag=RET";
#endif
			
				// data is Next to tag (set/update MADR ??)
				DmaCh [ iChannel ].MADR_Reg.Value = DmaCh [ iChannel ].TADR_Reg.Value + 16;
				
				
				QWC_TransferCount = Chain_TransferBlock ( iChannel );
				
				// check if transfer of block is complete
				if ( QWC_Transferred [ iChannel ] >= QWC_BlockTotal [ iChannel ] )
				{
					// tag to push onto stack is next to data
					//DmaCh [ iChannel ].TADR_Reg.Value += ( QWC_BlockTotal [ iChannel ] << 4 ) + 16;
					
					// pop address from stack
					if ( DmaCh [ iChannel ].CHCR_Reg.ASP > 0 )
					{
						// decrease the amound pushed onto stack
						DmaCh [ iChannel ].CHCR_Reg.ASP -= 1;
						
						// next tag is popped from stack
						switch ( DmaCh [ iChannel ].CHCR_Reg.ASP )
						{
							case 0:
								DmaCh [ iChannel ].TADR_Reg.Value = DmaCh [ iChannel ].ASR0_Reg.Value;
								break;
								
							case 1:
								DmaCh [ iChannel ].TADR_Reg.Value = DmaCh [ iChannel ].ASR1_Reg.Value;
								break;
						}
						
						// start new tag
						QWC_Transferred [ iChannel ] = -1;
						
						// if IRQ bit is set, then transfer no longer in progress
						if ( SourceDMATag [ iChannel ].IRQ && DmaCh [ iChannel ].CHCR_Reg.TIE )
						{
							// interrupt??
							EndTransfer ( iChannel );
							
							//TransferInProgress = false;
							return;
						}
					}
					else
					{
						// underflow - done
						cout << "\nhps2x64: ***ALERT*** DMA ASP underflow on RET tag.\n";
						
						// start new tag
						QWC_Transferred [ iChannel ] = -1;
						
						// interrupt??
						EndTransfer ( iChannel );
						
						//TransferInProgress = false;
						return;
					}
					
				}
				
				break;
			
			// END
			// transfer QWC following tag and end transfer
			case 7:
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; SourceDMATag=END";
#endif

				// data is Next to tag (set/update MADR ??)
				DmaCh [ iChannel ].MADR_Reg.Value = DmaCh [ iChannel ].TADR_Reg.Value + 16;
				
				
				QWC_TransferCount = Chain_TransferBlock ( iChannel );
				
				
				// ***TODO*** check if MADR gets updated
				
				
				// check if transfer of block is complete
				if ( QWC_Transferred [ iChannel ] >= QWC_BlockTotal [ iChannel ] )
				{
					// interrupt??
					EndTransfer ( iChannel );
					
					// start new tag
					QWC_Transferred [ iChannel ] = -1;
					
					//TransferInProgress = false;
					return;
				}
				
				break;

				
			default:
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; TagNotImplemented; Tag=" << hex << SourceDMATag [ iChannel ].ID;
#endif
				
				cout << "\nhps2x64 ALERT: DMA Tag not implemented yet. Tag=" << hex << SourceDMATag [ iChannel ].ID;
				
				// transfer is no longer in progress
				//TransferInProgress = false;
				return;
				
				break;

		}	// switch for chain tag ID
		
#ifdef TEST_ASYNC_DMA_STAGE2
		// block of data has been transferred, so if there delay for data transfer implement here
		if ( c_iDmaTransferTimePerQwc [ iChannel ] )
		{
			// continue transfer after data in device buffer has been processed
			SetNextEventCh ( ( c_iDmaTransferTimePerQwc [ iChannel ] * QWC_TransferCount ) + c_iSetupTime, iChannel );
			return;
		}
#endif
		
	}	// while loop
}

void Dma::InterleaveTransfer_ToMemory ( int iChannel )
{
}

void Dma::InterleaveTransfer_FromMemory ( int iChannel )
{
}


// use this to complete transfer after dmas are restarted after a suspension
void Dma::UpdateTransfer ()
{
	// channel 0 has the highest priority
	if ( DmaCh [ 0 ].CHCR_Reg.STR )
		Transfer ( 0 );
	
	// the other channels come after that
	// skip channel 7
	if ( DmaCh [ 1 ].CHCR_Reg.STR )
		Transfer ( 1 );
		
	if ( DmaCh [ 2 ].CHCR_Reg.STR )
		Transfer ( 2 );
		
	if ( DmaCh [ 3 ].CHCR_Reg.STR )
		Transfer ( 3 );
		
	if ( DmaCh [ 4 ].CHCR_Reg.STR )
		Transfer ( 4 );
		
	if ( DmaCh [ 5 ].CHCR_Reg.STR )
		Transfer ( 5 );
		
	if ( DmaCh [ 6 ].CHCR_Reg.STR )
		Transfer ( 6 );
		
	if ( DmaCh [ 8 ].CHCR_Reg.STR )
		Transfer ( 8 );
		
	if ( DmaCh [ 9 ].CHCR_Reg.STR )
		Transfer ( 9 );
}


void Dma::Transfer ( int iChannel )
{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "\r\nDma::Transfer; Channel#" << dec << iChannel;
	debug << "; (before) CHCR=" << hex << DmaCh [ iChannel ].CHCR_Reg.Value << " STAT=" << STAT_Reg.Value;
#endif

	u64 Data0, Data1;
	
	DMATag DstDtag;
	
	u32 SrcAddress, DstAddress, TransferCount, TransferIndex;
	u64 *SrcDataPtr, *DstDataPtr;
	
	u32 NextTagAddress, NextDataAddress;
	
	bool TransferInProgress = true;
	
	// check if dma transfers are being held
	if ( ( lDMAC_ENABLE & 0x10000 ) /* && ( iChannel != 5 ) */ )
	{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; HOLD";
#endif

		// all dma transfers disabled
		return;
	}
	
	// check if channel STR is 1
	if ( !DmaCh [ iChannel ].CHCR_Reg.STR )
	{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; Channel STR=0; Channel not enabled";
#endif

		return;
	}
	
	// if channel 5/6 (SIF0/SIF1) then make sure IOP is ready for transfer
	switch ( iChannel )
	{
		case 5:
		
			if ( !SIF::_SIF->IOP_DMA_Out_Ready () )
			{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; PS1 SIF0 NOT READY";
#endif

				return;
			}
			
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; PS1 SIF0 READY";
#endif

			break;
			
		case 6:
		
			if ( !SIF::_SIF->IOP_DMA_In_Ready () )
			{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; PS1 SIF1 NOT READY";
#endif

				return;
			}
			
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; PS1 SIF1 READY";
#endif

			break;
	}
	
	
	// check if channel has a dma setup time
	// dma setup times are disabled for now
	if ( c_iDmaSetupTime [ iChannel ] )
	{
		// check if it is not time for DMA transfer to continue
		if ( NextEvent_Cycle != NextEventCh_Cycle [ iChannel ] )
		{
			// set transfer to continue after setup time
			SetNextEventCh ( c_iDmaSetupTime [ iChannel ], iChannel );

			return;
		}
	}
	

#ifdef INLINE_DEBUG_TRANSFER
	debug << "; CHCR=" << hex << DmaCh [ iChannel ].CHCR_Reg.Value;
#endif

	// check if transfer is in chain mode
	switch ( DmaCh [ iChannel ].CHCR_Reg.MOD )
	{
		// Normal transfer //
		case 0:
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; Normal";
#endif

			switch ( iChannel )
			{

				// the channels that are always TO memory are 3, 5, 8
				case 3:
				//case 5:	// will pull this in later
				case 8:
					cout << "\nhps2x64 ALERT: DMA: attempted NORMAL transfer TO memory via DMA Channel#" << dec << iChannel << "\n";
					break;
					
				// the channels that are always FROM memory are 0, 2, 4, 6, 9
				case 0:
				case 2:
				case 4:
				case 6:
				case 9:
				
					// perform NORMAL transfer FROM memory
					NormalTransfer_FromMemory ( iChannel );
				
					break;
					
				// direction of dma transfer only matters for channels 1 and 7
				case 1:
				case 7:
				
					// check if this is going from memory or to memory
					switch ( DmaCh [ iChannel ].CHCR_Reg.DIR )
					{
						// to memory
						case 0:
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; ToMemory";
#endif

							cout << "\nhps2x64 ALERT: DMA: attempted NORMAL transfer to memory via DMA Channel#" << dec << iChannel << "\n";
							break;
							
						// from memory
						case 1:
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; FromMemory";
	debug << " MADR=" << hex << DmaCh [ iChannel ].MADR_Reg.Value;
#endif

							// perform NORMAL transfer FROM memory
							NormalTransfer_FromMemory ( iChannel );

							break;
					}
						
					break;

			}	// switch for channel number
				
				break;	// NORMAL Transfer

				
		// Chain transfer (Source) //
		case 1:
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; Chain (Source [FromMemory])";
#endif


			// check the channel number
			switch ( iChannel )
			{
				case 5:
				
					// pull data from SIF for transfer
					// ***TODO*** this should actually read the data using a consistent method
					SIF::EE_DMA_ReadBlock ();
				
					break;
					
				/*
				case 6:
				
					
					if ( c_ullSIFOverhead )
					{
					
						if ( *_DebugCycleCount == NextEventCh_Cycle [ 6 ] )
						{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "\r\nCH#6 TRANSFER";
#endif

							// pull data from SIF for transfer
							// ***TODO*** this should actually read the data using a consistent method
							ChainTransfer_FromMemory ( 6 );
						}
						else
						{
#ifdef INLINE_DEBUG_TRANSFER
	debug << "\r\nCH#6 OVERHEAD";
#endif

							// need to make the transfer later
							// can't transfer and then wait on the receiving side, have wait, then transfer on the sending side
							// might be that PS1 is clearing out data quickly after starting transfer
							// SIF buffer is 8 qwords, so I'll try waiting 8*128*2=2048 PS1 bus cycles
							//BusyUntil_Cycle [ 5 ] = *_DebugCycleCount + c_ullSIFOverhead;
							
							// repeat transfer after wait (transfer overhead)
							SetNextEventCh( c_ullSIFOverhead, 6 );
						}
					
					}
					else
					{
						ChainTransfer_FromMemory ( 6 );
					}
					
					
					break;
				*/
				
				// the channels that probably have direction controlled on the device side are 0,2,3,
				
				// the channels that are always TO memory are 3, 5, 8
				case 3:
				//case 5:	// will pull this in later
				case 8:
					cout << "\nhps2x64 ALERT: DMA: attempted CHAIN transfer TO memory via DMA Channel#" << dec << iChannel << "\n";
					break;
				
				// the channels that are always FROM memory are 0, 2, 4, 6, 9
				case 0:
				case 2:
				case 4:
				case 6:
				case 9:
				
					// perform CHAIN transfer FROM memory
					ChainTransfer_FromMemory ( iChannel );
				
					break;
					
				// direction of dma transfer only matters for channels 1 and 7
				case 1:
				case 7:
						
					// check if this is going from memory or to memory
					switch ( DmaCh [ iChannel ].CHCR_Reg.DIR )
					{
						// to memory
						case 0:
							cout << "\nhps2x64 ALERT: DMA: attempted CHAIN transfer TO memory via DMA Channel#" << dec << iChannel << "\n";
							break;
						
						// from memory
						case 1:
							ChainTransfer_FromMemory ( iChannel );
							break;
							
					}	// switch for DMA CHAIN transfer direction
					
			}	// switch for channel number
			
			
			break;	// CHAIN transfer
			
		// Interleave transfer (Scratch Pad)
		case 2:
#ifdef INLINE_DEBUG_TRANSFER
	debug << "; Interleave";
#endif

			// Interleave mode transfers data in a more rectangular pattern to/from scratchpad
			cout << "\nhps2x64: ALERT: DMA: Attempting Interleave DMA transfer. DMA Channel#" << dec << iChannel << "\n";

			break;
			
	}	// switch for DMA transfer type (normal, chain, interleave)
}








//const char* DmaChannelLogText [ 7 ] = { "DMA0_Log.txt", "DMA1_Log.txt", "DMA2_Log.txt", "DMA3_Log.txt", "DMA4_Log.txt", "DMA5_Log.txt", "DMA6_Log.txt" };

DmaChannel::DmaChannel ()
{
	// set the dma channel number
	Number = Count++;
	
	Reset ();
}


void DmaChannel::Reset ()
{
	// initialize MADR, BCR, & CHCR
	MADR_Reg.Value = 0;
	QWC_Reg.Value = 0;
	CHCR_Reg.Value = 0;
}


// returns interrupt status
void Dma::DMA_Finished ( int index, bool SuppressDMARestart )
{
#ifdef INLINE_DEBUG_COMPLETE
	debug << "\r\n\r\nDMA" << dec << index << "::Finished; PC= " << hex << setw( 8 ) << *_DebugPC << " " << dec << *_DebugCycleCount << hex << "; Intc_Stat=" << *_Intc_Stat << "; _Intc_Mask=" << *_Intc_Mask << "; _R3000a_Status=" << *_R3000a_Status;
#endif
	
	/*
	u32 ICR_Prev;
	
	EndA = DmaCh [ index ].MADR + DmaCh [ index ].BCR.BS;
	
	// get previous value of ICR
	ICR_Prev = ICR_Reg.Value;
	
	// allow all dma's to operate at next opportunity
	SelectedDMA_Bitmap = 0xffffffff;
	
	// clear bit in bitmap for dma channel
	ChannelEnable_Bitmap &= ~( 1 << index );
	
	// stop the dma channel
	// note: both bits 24 and 28 get reset after the transfer
	DmaCh [ index ].CHCR.Value &= ~( ( 1L << 0x18 ) | ( 1L << 0x1c ) );	//dma->n_channelcontrol &= ~( ( 1L << 0x18 ) | ( 1L << 0x1c ) );
	
	// *** testing *** if the dma is finished, then bcr should be zero
	// note: this actually depends on the dma channel
	// note: only sync mode 1 decrements the upper 16-bits of BCR, all other transfer modes leave it as is
	//DmaCh [ index ].BCR.Value = 0;
	
	// check if dma interrupts are enabled for channel
	if ( ICR_Reg.Value & ( 1 << ( 16 + index ) ) )
	{
		// set interrupt pending for channel
		ICR_Reg.Value |= ( 1 << ( 24 + index ) );
		
		// only allow interrupt pending if the interrupt is enabled
		ICR_Reg.Value &= ( ( ICR_Reg.Value << 8 ) | 0x80ffffff );

		// check if there are any interrupts pending
		if ( ICR_Reg.Value & 0x7f000000 )
		{
			// set interrupt pending flag
			ICR_Reg.Value |= 0x80000000;
		}
		else
		{
			// clear interrupt pending flag
			ICR_Reg.Value &= 0x7fffffff;
			
			
			// *** TESTING ***
			//ClearInterrupt ();
		}
		
		// check if dma interrupts are enabled globally
		// also check that bit 31 transitioned from 0 to 1
		//if ( ( ( ICR_Reg.Value >> 23 ) & 1 ) )
		if ( ( ! ( ICR_Prev & 0x80000000 ) ) && ( ( ICR_Reg.Value & 0x80800000 ) == 0x80800000 ) )
		{
			// check if dma interrupts are enabled for channel
			
			// send interrupt signal
			SetInterrupt ();
		}
	}
	
	// now that the dma channel is finished, check what channel is next and run it immediately
	ActiveChannel = GetActiveChannel ();
	
	if ( !SuppressDMARestart )
	{
		DMA_Run ( ActiveChannel );
	}
	
	// make sure the cycle number for the next dma event is updated
	Update_NextEventCycle ();
	
	// no more events for this particular channel, cuz it is finished
	//SetNextEventCh ( 0, index );
	*/
}



void Dma::SetNextEventCh ( u64 Cycles, u32 Channel )
{
	NextEventCh_Cycle [ Channel ] = Cycles + *_DebugCycleCount;
	
	Update_NextEventCycle ();
}


void Dma::SetNextEventCh_Cycle ( u64 Cycle, u32 Channel )
{
	NextEventCh_Cycle [ Channel ] = Cycle;
	
	//cout << "\nTEST: Channel=" << dec << Channel << " NextEventCh_Cycle [ Channel ]=" << NextEventCh_Cycle [ Channel ] << " Cycle=" << Cycle;
	
	Update_NextEventCycle ();
}

void Dma::Update_NextEventCycle ()
{
	NextEvent_Cycle = -1LL;
	
	for ( int i = 0; i < NumberOfChannels; i++ )
	{
		if ( NextEventCh_Cycle [ i ] > *_DebugCycleCount && ( NextEventCh_Cycle [ i ] < NextEvent_Cycle || NextEvent_Cycle <= *_DebugCycleCount ) )
		{
			// the next event is the next event for device
			NextEvent_Cycle = NextEventCh_Cycle [ i ];
		}
	}

	if ( NextEvent_Cycle > *_DebugCycleCount && ( NextEvent_Cycle < *_NextSystemEvent || *_NextSystemEvent <= *_DebugCycleCount ) ) *_NextSystemEvent = NextEvent_Cycle;
	
	//cout << "\nTEST: dma1 next event cycle=" << dec << NextEventCh_Cycle [ 1 ];
	//cout << "\nTEST: dma next event cycle=" << dec << NextEvent_Cycle;
}



static u64* Dma::GetMemoryPtr ( u32 Address )
{
	if ( Address >> 31 )
	{
		// if SPR bit is set, then it is an SPR Memory address
		return & ( DataBus::_BUS->ScratchPad.b64 [ ( ( Address & DataBus::ScratchPad_Mask ) >> 3 ) & ~1 ] );
	}
	
	// otherwise, it is a main memory address
	return & ( DataBus::_BUS->MainMemory.b64 [ ( ( Address & DataBus::MainMemory_Mask ) >> 3 ) & ~1 ] );
}




static void Dma::DebugWindow_Enable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	static const char* DebugWindow_Caption = "PS2 DMA Debug Window";
	static const int DebugWindow_X = 10;
	static const int DebugWindow_Y = 10;
	static const int DebugWindow_Width = 250;
	static const int DebugWindow_Height = 300;
	
	static const int DMAList_X = 0;
	static const int DMAList_Y = 0;
	static const int DMAList_Width = 220;
	static const int DMAList_Height = 250;
	
	int i;
	stringstream ss;
	
	if ( !DebugWindow_Enabled )
	{
		// create the main debug window
		DebugWindow = new WindowClass::Window ();
		DebugWindow->Create ( DebugWindow_Caption, DebugWindow_X, DebugWindow_Y, DebugWindow_Width, DebugWindow_Height );
		DebugWindow->DisableCloseButton ();
		
		// create "value lists"
		DMA_ValueList = new DebugValueList<u32> ();
		DMA_ValueList->Create ( DebugWindow, DMAList_X, DMAList_Y, DMAList_Width, DMAList_Height );
		
		DMA_ValueList->AddVariable ( "DMA_CTRL", &( _DMA->CTRL_Reg.Value ) );
		DMA_ValueList->AddVariable ( "DMA_STAT", &( _DMA->STAT_Reg.Value ) );
		DMA_ValueList->AddVariable ( "DMA_PCR", &( _DMA->PCR_Reg.Value ) );

		for ( i = 0; i < NumberOfChannels; i++ )
		{
			ss.str ("");
			ss << "DMA" << i << "_MADR";
			DMA_ValueList->AddVariable ( ss.str().c_str(), &( _DMA->DmaCh [ i ].MADR_Reg.Value ) );
			
			ss.str ("");
			ss << "DMA" << i << "_BCR";
			DMA_ValueList->AddVariable ( ss.str().c_str(), &( _DMA->DmaCh [ i ].QWC_Reg.Value ) );
			
			ss.str ("");
			ss << "DMA" << i << "_CHCR";
			DMA_ValueList->AddVariable ( ss.str().c_str(), &( _DMA->DmaCh [ i ].CHCR_Reg.Value ) );
		}
		
		// add start and end addresses for dma transfers
		//DMA_ValueList->AddVariable ( "StartA", &( _DMA->StartA ) );
		//DMA_ValueList->AddVariable ( "EndA", &( _DMA->EndA ) );
		
		// add primitive count and frame count here for now
		//DMA_ValueList->AddVariable ( "PCount", &( _GPU->Primitive_Count ) );
		//DMA_ValueList->AddVariable ( "FCount", &( _GPU->Frame_Count ) );
		
		DebugWindow_Enabled = true;
		
		// update the value lists
		DebugWindow_Update ();
	}

#endif

}

static void Dma::DebugWindow_Disable ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	if ( DebugWindow_Enabled )
	{
		delete DebugWindow;
		delete DMA_ValueList;
	
		// disable debug window
		DebugWindow_Enabled = false;
	}
	
#endif

}

static void Dma::DebugWindow_Update ()
{

#ifndef _CONSOLE_DEBUG_ONLY_

	if ( DebugWindow_Enabled )
	{
		DMA_ValueList->Update();
	}
	
#endif

}


}



