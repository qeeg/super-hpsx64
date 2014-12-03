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



#include "PS2_SIF.h"
#include "PS1_DMA.h"
#include "PS2_DMA.h"

#include "PS1_Intc.h"


//#include "WinApiHandler.h"

//using namespace Playstation2;


//#define ENABLE_SIF_DMA_TIMING
//#define ENABLE_SIF_DMA_SYNC

#ifdef _DEBUG_VERSION_

#define INLINE_DEBUG_ENABLE
//#define INLINE_DEBUG

/*
#define INLINE_DEBUG_WRITE_EE
#define INLINE_DEBUG_READ_EE
#define INLINE_DEBUG_WRITE_IOP
#define INLINE_DEBUG_READ_IOP

#define INLINE_DEBUG_DMA_READ_EE
#define INLINE_DEBUG_DMA_WRITE_EE
#define INLINE_DEBUG_DMA_READ_IOP
#define INLINE_DEBUG_DMA_WRITE_IOP
*/

//#define INLINE_DEBUG_DMA_WRITE_EE_CONTENTS
//#define INLINE_DEBUG_DMA_WRITE_IOP_CONTENTS


//#define INLINE_DEBUG_RUN

#endif



namespace Playstation2
{

u32 *SIF::_DebugPC;
u64 *SIF::_DebugCycleCount;

u32* SIF::_R3000A_Intc_Stat;
u32* SIF::_R3000A_Intc_Mask;
u32* SIF::_R3000A_Status_12;
u32* SIF::_R3000A_Cause_13;
u64* SIF::_R3000A_ProcStatus;

u32* SIF::_R5900_Intc_Stat;
u32* SIF::_R5900_Intc_Mask;
u32* SIF::_R5900_Status_12;
u32* SIF::_R5900_Cause_13;
u64* SIF::_R5900_ProcStatus;


u64* SIF::_NextSystemEvent;


Debug::Log SIF::debug;

SIF *SIF::_SIF;




SIF::SIF ()
{
	cout << "Running SIF constructor...\n";


}


void SIF::Start ()
{
	cout << "Running SIF::Start...\n";

#ifdef INLINE_DEBUG_ENABLE
	debug.Create( "PS2_SIF_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering SIF::Start";
#endif


	Reset ();
	
	_SIF = this;
	
	// ???
	lSBUS_F260 = 0x1d000060;
	
	DebugCount = c_iDebugLines;

	// start the events
	//SetNextEvent ( c_llIntInterval );

	
#ifdef INLINE_DEBUG
	debug << "->Exiting SIF::Start";
#endif
}


void SIF::Reset ()
{
	// zero object
	memset ( this, 0, sizeof( SIF ) );
}





void SIF::Run ()
{


	if ( NextEvent_Cycle != *_DebugCycleCount ) return;

#ifdef INLINE_DEBUG_RUN
	debug << "\r\n\r\nSIF::Run; CycleCount=" << dec << *_DebugCycleCount;
	debug << "; (before) NextEvent_Cycle=" << dec << NextEvent_Cycle;
#endif

	// check if the next transfer is ready
	
	
	// *testing* ?? interrupt ??
	//_SIF->SetInterrupt_IOP_SIF ();
	_SIF->SetInterrupt_EE_SIF ();


	SetNextEvent ( c_llIntInterval );

#ifdef INLINE_DEBUG_RUN
	debug << "; (after) NextEvent_Cycle=" << dec << NextEvent_Cycle;
#endif
}







static u64 SIF::EE_Read ( u32 Address, u64 Mask )
{
#ifdef INLINE_DEBUG_READ_EE
	if ( --_SIF->DebugCount > 0 )
	{
	debug << "\r\nEE::SIF::Read " << hex << *_DebugPC << " " << dec << *_DebugCycleCount << " Address=" << hex << setw ( 8 ) << Address;
	}
#endif

	u32 Output;

	switch ( Address )
	{
		case EESIF_F200:
#ifdef INLINE_DEBUG_READ_EE
	if ( _SIF->DebugCount > 0 ) { debug << "; F200; EE->IOP"; }
#endif

			Output = _SIF->lSBUS_F200;
			break;
			
		case EESIF_F210:
#ifdef INLINE_DEBUG_READ_EE
	if ( _SIF->DebugCount > 0 ) { debug << "; F210; IOP->EE"; }
#endif

			Output = _SIF->lSBUS_F210;
			break;
			
		case EESIF_F220:
#ifdef INLINE_DEBUG_READ_EE
	if ( _SIF->DebugCount > 0 ) { debug << "; F220"; }
#endif

			Output = _SIF->lSBUS_F220;
			break;
			
		case EESIF_F230:
#ifdef INLINE_DEBUG_READ_EE
	if ( _SIF->DebugCount > 0 ) { debug << "; F230"; }
#endif
	
			Output = _SIF->lSBUS_F230;
			break;
			
		case EESIF_CTRL:
#ifdef INLINE_DEBUG_READ_EE
	if ( _SIF->DebugCount > 0 ) { debug << "; F240; CTRL"; }
#endif

			Output = _SIF->lSBUS_F240 | 0xf0000102;
			break;
			
		case EESIF_F260:
#ifdef INLINE_DEBUG_READ_EE
	if ( _SIF->DebugCount > 0 ) { debug << "; F260"; }
#endif

			Output = _SIF->lSBUS_F260;
			break;
			
		default:
#ifdef INLINE_DEBUG_READ_EE
	if ( _SIF->DebugCount > 0 ) { debug << "; Invalid"; }
#endif
		
			// invalid SIO Register
			cout << "\nhps2x64 ALERT: Unknown SIF READ @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << "\n";
			break;
	};
	
#ifdef INLINE_DEBUG_READ_EE
	if ( _SIF->DebugCount > 0 ) { debug << "; Output =" << hex << Output; }
#endif

	return Output;
}




static void SIF::EE_Write ( u32 Address, u64 Data, u64 Mask )
{
#ifdef INLINE_DEBUG_WRITE_EE
	debug << "\r\nEE::SIF::Write " << hex << *_DebugPC << " " << dec << *_DebugCycleCount << " Address=" << hex << setw ( 8 ) << Address << "; Data=" << Data;
	_SIF->DebugCount = c_iDebugLines;
#endif

	
	switch ( Address )
	{
		case EESIF_F200:
#ifdef INLINE_DEBUG_WRITE_EE
			debug << "; F200; EE->IOP";
#endif
			// EE write path, so store data here
			// 0x1000f210 is the IOP write path
			_SIF->lSBUS_F200 = Data;
			break;
			
		case EESIF_F210:
#ifdef INLINE_DEBUG_WRITE_EE
			debug << "; F210; IOP->EE";
#endif

			break;
			
		case EESIF_F220:
#ifdef INLINE_DEBUG_WRITE_EE
			debug << "; F220";
#endif

			// bits get SET from EE side and CLEARED when written from IOP side
			// ??interrupt trigger?? //
			_SIF->lSBUS_F220 |= Data;
			
			// treat bit 16 as an interrupt trigger ??
			if ( _SIF->lSBUS_F220 & 0x10000 )
			{
				//Playstation1::Intc::_INTC->I_STAT_Reg.Unknown0 = 1;
				//Playstation1::Intc::_INTC->UpdateInts ();
			}
			
			break;
			
		case EESIF_F230:
#ifdef INLINE_DEBUG_WRITE_EE
			debug << "; F230";
#endif

			// bits get cleared when written from EE and set when written from IOP
			_SIF->lSBUS_F230 &= ~Data;
			break;
			
		case EESIF_CTRL:
#ifdef INLINE_DEBUG_WRITE_EE
			debug << "; F240; CTRL";
#endif

			// control register //
			
			
			if(!(Data & 0x100))
			{
				_SIF->lSBUS_F240 &= ~0x100;
			}
			else
			{
				_SIF->lSBUS_F240 |= 0x100;
			}
			
			
			break;
			
		case EESIF_F260:
#ifdef INLINE_DEBUG_WRITE_EE
			debug << "; F260";
#endif

			// ??? //
			_SIF->lSBUS_F260 = 0;
			break;

			
		default:
#ifdef INLINE_DEBUG_WRITE_EE
			debug << "; Invalid";
#endif
		
			// invalid SIO Register
			cout << "\nhps2x64 ALERT: Unknown SIF WRITE @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << " Data=" << Data << "\n";
			break;
	};
	
}






static u32 SIF::IOP_Read ( u32 Address )
{
#ifdef INLINE_DEBUG_READ_IOP
	if ( --_SIF->DebugCount > 0 )
	{
	debug << "\r\nIOP::SIF::Read " << hex << *_DebugPC << " " << dec << *_DebugCycleCount << " Address=" << hex << setw ( 8 ) << Address;
	}
#endif

	u32 Output;

	switch ( Address )
	{
		case IOPSIF_F200:
#ifdef INLINE_DEBUG_READ_IOP
	if ( _SIF->DebugCount > 0 ) { debug << "; F200; EE->IOP"; }
#endif

			// incoming from EE
			Output = _SIF->lSBUS_F200;
			break;
			
		case IOPSIF_F210:
#ifdef INLINE_DEBUG_READ_IOP
	if ( _SIF->DebugCount > 0 ) { debug << "; F210; IOP->EE"; }
#endif

			// outgoing from IOP
			Output = _SIF->lSBUS_F210;
			break;
			
		case IOPSIF_F220:
#ifdef INLINE_DEBUG_READ_IOP
	if ( _SIF->DebugCount > 0 ) { debug << "; F220"; }
#endif

			Output = _SIF->lSBUS_F220;
			break;
			
		case IOPSIF_F230:
#ifdef INLINE_DEBUG_READ_IOP
	if ( _SIF->DebugCount > 0 ) { debug << "; F230"; }
#endif

			Output = _SIF->lSBUS_F230;
			break;
			
		case IOPSIF_CTRL:
#ifdef INLINE_DEBUG_READ_IOP
	if ( _SIF->DebugCount > 0 ) { debug << "; F240; CTRL"; }
#endif

			// control register ?? //
			Output = _SIF->lSBUS_F240 | 0xf0000002;
			break;
			
		case IOPSIF_F260:
#ifdef INLINE_DEBUG_READ_IOP
	if ( _SIF->DebugCount > 0 ) { debug << "; F260"; }
#endif

			Output = _SIF->lSBUS_F260;
			break;

			
		default:
#ifdef INLINE_DEBUG_READ_IOP
	if ( _SIF->DebugCount > 0 ) { debug << "; Invalid"; }
#endif
		
			// invalid SIO Register
			cout << "\nhps2x64 ALERT: Unknown SIF READ @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << "\n";
			break;
	};
	
#ifdef INLINE_DEBUG_READ_IOP
	if ( _SIF->DebugCount > 0 ) { debug << "; Output =" << hex << Output; }
#endif

	return Output;
}




static void SIF::IOP_Write ( u32 Address, u32 Data, u32 Mask )
{
#ifdef INLINE_DEBUG_WRITE_IOP
	debug << "\r\nIOP::SIF::Write " << hex << *_DebugPC << " " << dec << *_DebugCycleCount << " Address=" << hex << setw ( 8 ) << Address << "; Data=" << Data;
	_SIF->DebugCount = c_iDebugLines;
#endif

	u32 temp;
	
	switch ( Address )
	{
		case IOPSIF_F200:
#ifdef INLINE_DEBUG_WRITE_IOP
			debug << "; F200; EE->IOP";
#endif

			// incoming from EE
			break;
			
		case IOPSIF_F210:
#ifdef INLINE_DEBUG_WRITE_IOP
			debug << "; F210; IOP->EE";
#endif

			// outgoing from IOP
			_SIF->lSBUS_F210 = Data;
			break;
			
		case IOPSIF_F220:
#ifdef INLINE_DEBUG_WRITE_IOP
			debug << "; F220";
#endif

			// bits cleared when written from IOP
			_SIF->lSBUS_F220 &= ~Data;
			break;
			
		case IOPSIF_F230:
#ifdef INLINE_DEBUG_WRITE_IOP
			debug << "; F230";
#endif

			// bits set when written from IOP
			_SIF->lSBUS_F230 |= Data;
			break;
			
		case IOPSIF_CTRL:
#ifdef INLINE_DEBUG_WRITE_IOP
			debug << "; F240; CTRL";
#endif

			temp = Data & 0xf0;
			if ( Data & 0x20 || Data & 0x80)
			{
				_SIF->lSBUS_F240 &= ~0xf000;
				_SIF->lSBUS_F240 |= 0x2000;
			}

			
			if ( _SIF->lSBUS_F240 & temp )
				_SIF->lSBUS_F240 &= ~temp;
			else
				_SIF->lSBUS_F240 |= temp;
				
			if ( Data & 0x40 )
			{
//#ifdef INLINE_DEBUG_WRITE_IOP
//			debug << "; INT";
//#endif
				// *testing* ?? interrupt ??
				//_SIF->SetInterrupt_IOP_SIF ();
			}
			
#ifdef ENABLE_SIF_DMA_SYNC
			// check if time for transfer
			Check_TransferToIOP ();
			Check_TransferFromIOP ();
#endif
			
#ifdef INLINE_DEBUG_WRITE_IOP
			debug << "; (after) F240=" << hex << _SIF->lSBUS_F240;
#endif
			break;
			
		case IOPSIF_F260:
#ifdef INLINE_DEBUG_WRITE_IOP
			debug << "; F260";
#endif

			_SIF->lSBUS_F260 = 0;
			break;

			
		default:
#ifdef INLINE_DEBUG_WRITE_IOP
			debug << "; Invalid";
#endif
		
			// invalid SIF Register
			cout << "\nhps2x64 ALERT: Unknown SIF WRITE @ Cycle#" << dec << *_DebugCycleCount << " Address=" << hex << Address << " Data=" << Data << "\n";
			break;
	};
	
}




static void SIF::EE_DMA_ReadBlock ()
{
#ifdef INLINE_DEBUG_DMA_READ_EE
	debug << "\r\nEE_DMA_ReadBlock (IOP->EE) " << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif

	// IOP dma channel #9 is trying to pass data to EE dma channel #5
	Playstation1::Dma::_DMA->DMA9_Run ();
}

// returns the amound of data written
// Count is in quadwords
static u32 SIF::EE_DMA_WriteBlock ( u32* Data, u32 Count )
{
#ifdef INLINE_DEBUG_DMA_WRITE_EE
	debug << "\r\nEE_DMA_WriteBlock (EE->IOP) " << hex << *_DebugPC << " " << dec << *_DebugCycleCount << " Count=" << Count;
	debug << " TagForIOP=" << hex << ((u64*) Data) [ 0 ];
#endif

	u32 *SrcDataPtr, *DstDataPtr;
	u32 Data0, Data1, DestAddress, IRQ, ID;
	u32 IOPCount;
	
	// EE dma channel #6 is trying to pass data to IOP dma channel #10
	
	// first check if both PS2 dma channel#6 and IOP dma channel #10 are both enabled
	//if ( Playstation1::Dma::_DMA->DmaCh [ 10 ].TR && Playstation2::Dma::_DMA->DmaCh [ 6 ].STR )
	//{
	//	// both channels are not enabled
	//	return;
	//}
	
	/*
	Data0 = *Data++;
	Data1 = *Data++;
	
#ifdef INLINE_DEBUG_DMA_WRITE_EE
	debug << " Data0=" << hex << Data0 << " Data1=" << Data1;
#endif

	// the first 32-bit value is the address, interrupt, and whether to stop or keep transferring
	// for now, get destination address from Destination DMA tag
	DestAddress = Data0 & 0xffffff;
	Playstation1::Dma::_DMA->DmaCh [ 10 ].MADR = DestAddress;
	
	// check for interrupt
	// the tag is for the dma though, and probably isn't supposed to tell the SIF anything
	if ( Data0 & 0x80000000 )
	{
		// ***todo*** SIF interrupt??
	}
	
	// check for transfer stop
	if ( Data0 & 0x40000000 )
	{
		// ***todo*** needs more testing
	}
	*/
	
	// the second 32-bit value is the amount to transfer
	IOPCount = Data1;
	
	// rest of quadword is ignored
	/*
	Data++;
	Data++;
	*/
	
	// we are now at the data to transfer
	SrcDataPtr = Data;
	
#ifdef INLINE_DEBUG_DMA_WRITE_EE_CONTENTS
	// check transfer contents
	debug << "\r\nData=" << hex;
	for ( int i = 0; i < ( Count << 1 ); i++ )
	{
		debug << " " << ((u64*) Data) [ i ];
	}
#endif
	
	// set the SIF buffer size
	//_SIF->BufferSize = IOPCount;
	
	// set the SIF buffer direction
	//_SIF->BufferDirection = BUFFER_SIF1;
	
	// make a call to transfer the data using PS1 dma
	// the count is the quad word count, so must be multiplied by 4 here
	//Playstation1::Dma::_DMA->DMA10_WriteBlock ( SrcDataPtr, IOPCount );
	Playstation1::Dma::_DMA->DMA10_WriteBlock ( Data, Count << 2 );
	
	// return the amount of data in quadwords that was written
	return Count;

	// for now, get transfer amount from Destination DMA tag
	
	// for now, transfer data into PS1 RAM
	
	// for now, set IOP dma channel 10 with updated destination address
	
	// check for SIF interrupt in Dest DMA tag
	
	// interrupt IOP if needed
}

static void SIF::IOP_DMA_ReadBlock ()
{
#ifdef INLINE_DEBUG_DMA_READ_IOP
	debug << "\r\nIOP_DMA_ReadBlock (EE->IOP) " << hex << *_DebugPC << " " << dec << *_DebugCycleCount;
#endif

	// EE dma channel #6 is trying to pass data to IOP dma channel #10
	
	// trigger the transfer start from PS2 side
	Playstation2::Dma::_DMA->Transfer ( 6 );
}

static void SIF::IOP_DMA_WriteBlock ( u64 EEDMATag, u32* Data, u32 Count )
{
#ifdef INLINE_DEBUG_DMA_WRITE_IOP
	debug << "\r\nIOP_DMA_WriteBlock (IOP->EE) " << hex << *_DebugPC << " " << dec << *_DebugCycleCount << " Count=" << Count << " EETag=" << hex << EEDMATag;
#endif

	// note: "Count" is a count of 32-bit words being transferred

#ifdef INLINE_DEBUG_DMA_WRITE_IOP_CONTENTS
	// check transfer contents
	debug << "\r\nData=" << hex;
	for ( int i = 0; i < Count; i++ )
	{
		debug << " " << ((u32*) Data) [ i ];
	}
#endif

	// round count up to nearest 128-bit boundary and put in QWC
	Count = ( Count >> 2 ) + ( ( Count & 3 ) ? 1 : 0 );

	// IOP dma channel #9 is trying to pass data to EE dma channel #5
	//Playstation2::Dma::_DMA->DMA5_WriteBlock ( EEDMATag, Data, Count );
	Playstation2::Dma::_DMA->DMA5_WriteBlock ( EEDMATag, (u64*) Data, Count );
}


static void SIF::Check_TransferToIOP ()
{
	if ( EE_DMA_Out_Ready () && IOP_DMA_In_Ready () )
	{
		Playstation2::Dma::_DMA->Transfer ( 6 );
	}
}


static void SIF::Check_TransferFromIOP ()
{
	if ( EE_DMA_In_Ready () && IOP_DMA_Out_Ready () )
	{
		Playstation1::Dma::_DMA->DMA9_Run ();
	}
}


static bool SIF::EE_DMA_In_Ready ()
{
	// checks if SIF0 channel#5 is ready on EE
	// maybe incoming dma transfers are not affected by dmac enable
	//return ( Playstation2::Dma::_DMA->DmaCh [ 5 ].CHCR_Reg.STR && ! ( Playstation2::Dma::_DMA->lDMAC_ENABLE & 0x10000 ) );
	return ( Playstation2::Dma::_DMA->DmaCh [ 5 ].CHCR_Reg.STR
			&& ! ( Playstation2::Dma::_DMA->lDMAC_ENABLE & 0x10000 )
#ifdef ENABLE_SIF_DMA_SYNC
			&& ( _SIF->lSBUS_F240 & 0x20 )
#endif
#ifdef ENABLE_SIF_DMA_TIMING
			&& ( *_DebugCycleCount >= _SIF->BusyUntil_Cycle )
#endif
			);
}

static bool SIF::EE_DMA_Out_Ready ()
{
	// checks if SIF1 channel#6 is ready on EE
	//return ( Playstation2::Dma::_DMA->DmaCh [ 6 ].CHCR_Reg.STR && ! ( Playstation2::Dma::_DMA->lDMAC_ENABLE & 0x10000 ) );
	return ( Playstation2::Dma::_DMA->DmaCh [ 6 ].CHCR_Reg.STR
			&& ! ( Playstation2::Dma::_DMA->lDMAC_ENABLE & 0x10000 )
#ifdef ENABLE_SIF_DMA_SYNC
			&& ( _SIF->lSBUS_F240 & 0x40 )
#endif
#ifdef ENABLE_SIF_DMA_TIMING
			&& ( *_DebugCycleCount >= _SIF->BusyUntil_Cycle )
#endif
			);
}


static bool SIF::IOP_DMA_In_Ready ()
{
	// checks if SIF1 channel#10 is ready on IOP
	// note: also need to make sure that bit 6 (0x40) is set in SBUS_F240
	//return Playstation1::Dma::_DMA->DmaCh [ 10 ].CHCR.TR;
	return ( Playstation1::Dma::_DMA->DmaCh [ 10 ].CHCR.TR
#ifdef ENABLE_SIF_DMA_SYNC
			&& ( _SIF->lSBUS_F240 & 0x40 )
#endif
#ifdef ENABLE_SIF_DMA_TIMING
			&& ( *_DebugCycleCount >= _SIF->BusyUntil_Cycle )
#endif
	);
}

static bool SIF::IOP_DMA_Out_Ready ()
{
	// checks if SIF0 channel#9 is ready on IOP
	//return Playstation1::Dma::_DMA->DmaCh [ 9 ].CHCR.TR;
	return ( Playstation1::Dma::_DMA->DmaCh [ 9 ].CHCR.TR
#ifdef ENABLE_SIF_DMA_SYNC
			&& ( _SIF->lSBUS_F240 & 0x20 )
#endif
#ifdef ENABLE_SIF_DMA_TIMING
			&& ( *_DebugCycleCount >= _SIF->BusyUntil_Cycle )
#endif
			);
}





void SIF::Update_NextEventCycle ()
{
	if ( NextEvent_Cycle > *_DebugCycleCount && ( NextEvent_Cycle < *_NextSystemEvent || *_NextSystemEvent <= *_DebugCycleCount ) ) *_NextSystemEvent = NextEvent_Cycle;
}


void SIF::SetNextEvent ( u64 Cycle )
{
	NextEvent_Cycle = Cycle + *_DebugCycleCount;
	
	Update_NextEventCycle ();
}

//void SIF::GetNextEvent ()
//{
//	SetNextEvent ( WaitCycles0 );
//}

}




