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

// NOTE: Much of the CDVD code is "adapted" from pcsx2 or other programs currently

#include "CDvd.h"
#include "PS1_CD.h"
#include "PS1_Dma.h"
#include <string.h>
#include <fstream>

using namespace Playstation1;
using namespace std;

Debug::Log CDVD::debug;

CDVD *CDVD::_CDVD;


u64* CDVD::_NextSystemEvent;



#ifdef _DEBUG_VERSION_

#define INLINE_DEBUG_ENABLE

//#define INLINE_DEBUG
#define INLINE_DEBUG_READ
#define INLINE_DEBUG_WRITE
#define INLINE_DEBUG_RUN

#endif


u32* CDVD::_DebugPC;
u64* CDVD::_DebugCycleCount;

u32* CDVD::_Intc_Stat;
u32* CDVD::_Intc_Mask;
u32* CDVD::_R3000A_Status_12;
u32* CDVD::_R3000A_Cause_13;
u64* CDVD::_ProcStatus;


static const char CDVD::c_cRegion_JAP [ 8 ] = { 0, 'J', 'j', 'p', 'n', 0, 'J', 0 };
static const char CDVD::c_cRegion_US [ 8 ] = { 0, 'A', 'e', 'n', 'g', 0, 'U', 0 };
static const char CDVD::c_cRegion_EU [ 8 ] = { 0, 'E', 'e', 'n', 'g', 0, 'O', 0 };
static const char CDVD::c_cRegion_H [ 8 ] = { 0, 'H', 'e', 'n', 'g', 0, 'E', 0 };
static const char CDVD::c_cRegion_R [ 8 ] = { 0, 'R', 'e', 'n', 'g', 0, 'A', 0 };
static const char CDVD::c_cRegion_C [ 8 ] = { 0, 'C', 's', 'c', 'h', 0, 'C', 0 };
static const char CDVD::c_cRegion_Korea [ 8 ] = { 0, 'K', 'k', 'o', 'r', 0, 'M', 0 };





CDVD::CDVD ()
{
}


void CDVD::Start ()
{
	cout << "Running CDVD::Start...\n";
	
#ifdef INLINE_DEBUG_ENABLE
	debug.Create( "CDVD_Log.txt" );
#endif

#ifdef INLINE_DEBUG
	debug << "\r\nEntering CDVD::Start";
#endif

	Reset ();
	
	_CDVD = this;
	
	// start region with Japan
	Region = 'J';
	
	SReady = 0x40;
	
	// 0x40 or 0x4e and at which times??
	// note: should probably start with 0x4e
	NReady = 0x4e;
	
	DiskSpeed = 4;
	SectorSize = 2064;
	
	// set the amound of data to read from sector to 2048 by default ??
	SectorReadSize = 2048;

#ifdef INLINE_DEBUG
	debug << "->Exiting CDVD::Start";
#endif
}


void CDVD::Reset ()
{
	// zero object
	memset ( this, 0, sizeof( CDVD ) );
}

// returns interrupt;
void CDVD::Run ()
{
	if ( NextEvent_Cycle != *_DebugCycleCount ) return;
	
	u32 NumberOfReads;
	
#ifdef INLINE_DEBUG_RUN
	debug << "\r\nCDVD::Run " << hex << *_DebugPC << " " << dec << *_DebugCycleCount << "; BusyCycles=" << BusyCycles;
#endif

	switch ( ReadCommand )
	{
		// SYNC (or motor on?)
		case 0:

		// NOP
		case 1:
			// complete
			InterruptReason = 2;
			SetInterrupt_CDVD ();
			
			// the status is "pause" ??
			Status = CDVD_STATUS_PAUSE;
			
			// done
			ReadCommand = -1;
			break;
		
		// STOP
		case 0x3:
			InterruptReason = 2;
			SetInterrupt_CDVD ();
			
			// the status is "stop" ??
			Status = CDVD_STATUS_STOP;
			
			// done
			ReadCommand = -1;
			break;
		
		// pause
		case 0x4:
			// complete
			InterruptReason = 2;
			SetInterrupt_CDVD ();
			
			// the status is "pause"
			Status = CDVD_STATUS_PAUSE;
			
			// done
			ReadCommand = -1;
			break;
			
			
		// cd read
		case 0x6:
#ifdef INLINE_DEBUG_RUN
	debug << "; CDREAD";
	debug << dec << " Sector#" << SeekSectorNum;
	debug << dec << " SectorReadCount=" << SectorReadCount;
	debug << dec << " BA=" << Dma::_DMA->DmaCh [ 3 ].BCR.BA << " BS=" << Dma::_DMA->DmaCh [ 3 ].BCR.BS;
#endif

			if ( Status != CDVD_STATUS_READ )
			{
#ifdef INLINE_DEBUG_RUN
	debug << "; SEEK_DONE";
#endif

				// reading has begun, so status is "read"
				Status = CDVD_STATUS_READ;
				
				// come back when done reading data
				//Set_NextEvent ( c_lCDRead_Cycles * SectorReadCount );
				Set_NextEvent ( ( c_lCDReadx1_Cycles / DiskSpeed ) * SectorReadCount );
				
				if ( SectorReadCount > 8 )
				{
					cout << "\nhps2x64: ***ALERT***: CDVD: SectorReadCount>8.\n";
				}
				
				return;
			}

#ifdef INLINE_DEBUG_RUN
	debug << "; READING_DATA";
#endif

			while ( SectorReadCount )
			{
			
			// read from cd
			CD::_CD->cd_image.ReadNextSector ();
			
			CD::_CD->SectorDataBuffer_Index = CD::_CD->cd_image.GetCurrentBufferIndex ();
			
			// currently need to set this
			// no, no... dma transfer is actually not ready until 0x80 is written to request data
			CD::_CD->isSectorDataReady = true;
			
			// this stuff needs to be set again ?? (fix later)
			// set the sector size for ps1 cd handler
			// need to set the data buffer size again since the ps1 handler resets it for now
			CD::_CD->DataBuffer_Size = _CDVD->SectorReadSize;
			//CD::_CD->ReadMode = _CDVD->SectorReadSize;
			
			
			// also need to trigger read via dma, since it is possible to have dma3 ready BEFORE even starting the read command
			if ( Dma::_DMA->isActive ( 3 ) )
			{
				// set the sector number that is being read
				CD::_CD->DVDSectorNumber = SeekSectorNum;
				
				// PS1 CD DMA is active, so trigger a transfer //
				Dma::_DMA->DMA3_Run ( false );
				
#ifdef INLINE_DEBUG_RUN
			// GetDataBuffer
			u8 *DiskData = CD::_CD->cd_image.GetDataBuffer ( CD::_CD->cd_image.GetCurrentBufferIndex () );
			debug << " (from disk)";
			debug << hex << " Min=" << (u32) DiskData [ 12 ];
			debug << hex << " Sec=" << (u32) DiskData [ 13 ];
			debug << hex << " Frac=" << (u32) DiskData [ 14 ];
#endif

				// if all the data was transferred, then the next transfer is not ready yet
				if ( !CD::_CD->DataBuffer_Size )
				{
					CD::_CD->isSectorDataReady = false;
				}
			}
			
			// dec number of sectors remaining to read
			SectorReadCount--;
			
			// advance sector number
			SeekSectorNum++;
			
			}
			
			// check if there are sectors left to read (if not, signal command complete ??)
			//if ( !SectorReadCount )
			//{
#ifdef INLINE_DEBUG_RUN
	debug << "; COMPLETE";
#endif

				// command complete ??
				//InterruptReason = 2;
				//SetInterrupt_CDVD ();
				
				// no more events for now
				//Set_NextEvent_Value ( -1 );
				
				// data ready
				InterruptReason = 1;
				SetInterrupt_CDVD ();
				//SetInterrupt ();
				
				// when reading is done, the status is "pause"
				Status = CDVD_STATUS_PAUSE;
				
				// ***todo*** looks like there should also be an Interrupt 2 immediately after this
				
				// done reading
				ReadCommand = -1;
				return;
			//}
			
			/*
			if ( SectorReadCount )
			{
				// set next read interval
				Set_NextEvent ( c_lCDRead_Cycles );
			}
			else
			{
				// set timing for command complete interrupt
				//Set_NextEvent ( c_lCDReadComplete_Cycles );
			}
			*/
			
			break;
			
			
		// READ TOC
		case 0x9:
		
			// read from toc
			//CD::_CD->cd_image.ReadNextSector ();
			
			//CD::_CD->SectorDataBuffer_Index = CD::_CD->cd_image.GetCurrentBufferIndex ();
			
			// currently need to set this
			// no, no... dma transfer is actually not ready until 0x80 is written to request data
			CD::_CD->isSectorDataReady = true;
			
			// 1024?? appears to want 2064 bytes when reading TOC for a DVD
			// I'll let 1024 mean it needs TOC info for now
			_CDVD->SectorReadSize = 1024;
			
			// this stuff needs to be set again ?? (fix later)
			// set the sector size for ps1 cd handler
			// need to set the data buffer size again since the ps1 handler resets it for now
			CD::_CD->DataBuffer_Size = 2064;	//_CDVD->SectorReadSize;
			//CD::_CD->ReadMode = _CDVD->SectorReadSize;
			
			
			// also need to trigger read via dma, since it is possible to have dma3 ready BEFORE even starting the read command
			if ( Dma::_DMA->isActive ( 3 ) )
			{
				// set the sector number that is being read
				//CD::_CD->DVDSectorNumber = SeekSectorNum;
				
				// PS1 CD DMA is active, so trigger a transfer //
				Dma::_DMA->DMA3_Run ( false );
				
				// if all the data was transferred, then the next transfer is not ready yet
				if ( !CD::_CD->DataBuffer_Size )
				{
					CD::_CD->isSectorDataReady = false;
				}
			}
			
			
#ifdef INLINE_DEBUG_RUN
	debug << "; COMPLETE";
#endif

				// data ready
				// command complete ??
				// might be int 1 then int 2 ??
				InterruptReason = 2;
				SetInterrupt_CDVD ();
				
				// ***todo*** looks like there should also be an Interrupt 2 immediately after this
				
				// done reading
				ReadCommand = -1;
			
			break;

			
		case 0x20:
#ifdef INLINE_DEBUG_RUN
	debug << "; READ COMPLETE EVENT";
#endif
			// completion event
			// no, this should probably happen immediately
			_CDVD->InterruptReason = 2;
			SetInterrupt_CDVD ();
			
			// done
			ReadCommand = -1;
			break;
	}
	
//#ifdef INLINE_DEBUG_RUN
//	debug << "; Signalling Interrupt";
//#endif

	// time of next event after this one is not known
	//NextEvent_Cycle = 0xffffffffffffffff;
	//Set_NextEvent ( 0xffffffffffffffffULL );

	// signal interrupt
	//SetInterrupt ();
}





static u32 CDVD::Read ( u32 Address )
{
#ifdef INLINE_DEBUG_READ
	debug << "\r\nCDVD::Read " << hex << *_DebugPC << " " << dec << *_DebugCycleCount << " Address=" << hex << setw ( 8 ) << Address;
#endif

	u32 Output;
	int temp, key;
	
	switch ( Address & 0xff )
	{
		case 0x4:	// NCOMMAND
#ifdef INLINE_DEBUG_READ
	debug << "; NCOMMAND";
#endif

			Output = _CDVD->NCommand;
			
			break;

			
		case 0x5:	// NREADY
#ifdef INLINE_DEBUG_READ
	debug << "; NREADY";
#endif

			Output = _CDVD->NReady;
			break;


		case 0x6:	// ERROR
#ifdef INLINE_DEBUG_READ
	debug << "; ERROR";
#endif

			Output = 0;
			break;

			
		case 0x7:	// BREAK
#ifdef INLINE_DEBUG_READ
	debug << "; BREAK";
#endif

			Output = 0;
			break;
			
		
		// interrupt reason ?? (usually 0x2)
		case 0x8:
#ifdef INLINE_DEBUG_READ
	debug << "; INTERRUPT REASON";
#endif

			Output = _CDVD->InterruptReason;
			break;
			
		
		// STATUS ??
		case 0xa:
#ifdef INLINE_DEBUG_READ
	debug << "; STATUS";
#endif

			Output = _CDVD->Status;
		
			break;

			
		case 0xb:
#ifdef INLINE_DEBUG_READ
	debug << "; TRAY STATE";
#endif

			// 1 means tray open, 0 means tray closed
			// tray closed for now
			Output = 0;
			
			break;
		
		case 0xf:	// DISK TYPE
#ifdef INLINE_DEBUG_READ
	debug << "; DISK TYPE";
#endif

			Output = _CDVD->CurrentDiskType;

			break;

			
		case 0x13:
#ifdef INLINE_DEBUG_READ
	debug << "; Command 0x13->Unknown";
#endif

			Output = 4;
			break;
			
		case 0x16:	// SCOMMAND
#ifdef INLINE_DEBUG_READ
	debug << "; SCOMMAND";
#endif

			Output = _CDVD->SCommand;
			break;

			
		case 0x17:	// SREADY
#ifdef INLINE_DEBUG_READ
	debug << "; SREADY";
#endif
			
			Output = _CDVD->SReady;
			break;

		
		case 0x18:	// SDATAOUT (this looks like it is the result buffer)
#ifdef INLINE_DEBUG_READ
	debug << "; SDATAOUT";
#endif
		
			Output = 0;
			if ( !( _CDVD->SReady & 0x40 ) && ( _CDVD->lResultIndex < _CDVD->lResultCount ) )
			{
				Output = _CDVD->ucResultBuffer [ _CDVD->lResultIndex++ ];
				
				if ( _CDVD->lResultIndex >= _CDVD->lResultCount ) _CDVD->SReady |= 0x40;
			}
			
			break;
			
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
#ifdef INLINE_DEBUG_READ
	debug << "; READ KEY X1";
#endif
		
			key = Address & 0xff;
			temp = key - 0x20;

			Output = _CDVD->DiskKey[temp];
			break;
			
		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B:
		case 0x2C:
#ifdef INLINE_DEBUG_READ
	debug << "; READ KEY X2";
#endif
		
			key = Address & 0xff;
			temp = key - 0x23;

			//CDVD_LOG("cdvdRead%d(Key%d) %x", key, temp, cdvd.Key[temp]);
			Output = _CDVD->DiskKey[temp];
			break;

		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
#ifdef INLINE_DEBUG_READ
	debug << "; READ KEY X3";
#endif
		
			key = Address & 0xff;
			temp = key - 0x26;

			//CDVD_LOG("cdvdRead%d(Key%d) %x", key, temp, cdvd.Key[temp]);
			Output = _CDVD->DiskKey[temp];
			break;

		case 0x38: 		// valid parts of key data (first and last are valid)
#ifdef INLINE_DEBUG_READ
	debug << "; READ KEY 15";
#endif

			//CDVD_LOG("cdvdRead38(KeysValid) %x", cdvd.Key[15]);

			Output = _CDVD->DiskKey[15];
			break;

		case 0x39:	// KEY-XOR
#ifdef INLINE_DEBUG_READ
	debug << "; READ KEY XOR";
#endif

			//CDVD_LOG("cdvdRead39(KeyXor) %x", cdvd.KeyXor);

			Output = _CDVD->KeyXor;
			break;

		case 0x3A: 	// DEC_SET
#ifdef INLINE_DEBUG_READ
	debug << "; READ DEC SET";
#endif

			//CDVD_LOG("cdvdRead3A(DecSet) %x", cdvd.decSet);

			Output = _CDVD->DecSet;
			break;
			
		default:
#ifdef INLINE_DEBUG_READ
	debug << "; UNKNOWN";
#endif

			Output = 0;
			break;
	}

	
#ifdef INLINE_DEBUG_READ
	debug << "; Output =" << hex << Output;
#endif

	return Output;
}


static void CDVD::Write ( u32 Address, u32 Data, u32 Mask )
{
#ifdef INLINE_DEBUG_WRITE
	debug << "\r\nCDVD::Write " << hex << *_DebugPC << " " << dec << *_DebugCycleCount << " Address=" << hex << setw ( 8 ) << Address << "; Data=" << Data;
#endif

	switch ( Address & 0xff )
	{
		case 0x4:
#ifdef INLINE_DEBUG_WRITE
	debug << "; NCOMMAND";
#endif

			_CDVD->NCommand = Data;
			
			_CDVD->Process_NCommand ( Data );
			
			break;
		
		case 0x5:
#ifdef INLINE_DEBUG_WRITE
	debug << "; NPARAM";
#endif

			_CDVD->ucNArgBuffer [ _CDVD->lNArgIndex & c_iNArgBuffer_Mask ] = Data;
			_CDVD->lNArgIndex++;
			
			break;
			
			
		case 0x6:
#ifdef INLINE_DEBUG_WRITE
	debug << "; HOWTO??";
#endif

			// ?? must mean to enable data transfer via DMA
			// ..as in transfer immediately irregardless ??
			// 0x80 = "DataRequest"
			/*
			if ( Data & 0x80 )
			{
				// enable transfer of data via dma
				CD::_CD->isSectorDataReady = true;
				
				// set the data buffer size to allow transfer
				CD::_CD->DataBuffer_Size = _CDVD->SectorReadSize;
				
				// now trigger dma transfer
				if ( Dma::_DMA->isActive ( 3 ) )
				{
#ifdef INLINE_DEBUG_WRITE
	debug << "; DMA3TRANSFER";
#endif

					// PS1 CD DMA is active, so trigger a transfer //
					Dma::_DMA->DMA3_Run ( true );
					
					// if all the data was transferred, then the next transfer is not ready yet
					if ( !CD::_CD->DataBuffer_Size )
					{
						CD::_CD->isSectorDataReady = false;
					}
				}
			}
			*/
			
			break;
			
		case 0x8:
#ifdef INLINE_DEBUG_WRITE
	debug << "; INTERRUPT ACKNOWLEDGE";
#endif

			// if acknowledging an int 1, then re-interrupt with an int 2
			if ( _CDVD->InterruptReason == 1 )
			{
				_CDVD->InterruptReason &= ~Data;
				
				if ( !_CDVD->InterruptReason )
				{
					// set the command to send a complete signal
					// will use 0x20 for now for complete
					//_CDVD->InterruptReason = 2;
					//SetInterrupt_CDVD ();
				}
			}
			else
			{
				_CDVD->InterruptReason &= ~Data;
			}
		
			break;
			
		case 0x16:	// SCOMMAND (looks like it's the command for the arguments sent to SDATAIN)
#ifdef INLINE_DEBUG_WRITE
	debug << "; SCOMMAND";
#endif
		
			_CDVD->SCommand = Data;
			
			_CDVD->Process_SCommand ( Data );
			
			break;
			
		case 0x17:	// SDATAIN ?? (looks like it is specifying parameters)
#ifdef INLINE_DEBUG_WRITE
	debug << "; SPARAM";
#endif

			_CDVD->ucSArgBuffer [ _CDVD->lSArgIndex & c_iSArgBuffer_Mask ] = Data;
			_CDVD->lSArgIndex++;
			break;
			
		case 0x3a:
#ifdef INLINE_DEBUG_WRITE
	debug << "; DECSET";
#endif

			// set the info for decrypting data from disk
			CD::_CD->DecryptSetting = Data;
			
			_CDVD->DecSet = Data;
			break;
			
			
		default:
#ifdef INLINE_DEBUG_WRITE
	debug << "; UNKNOWN";
#endif

			break;
	}
}

void CDVD::DMA_Read ( u32* Data, int ByteReadCount )
{
#ifdef INLINE_DEBUG
	debug << "\r\nCDVD::DMA_Read";
#endif

	Data [ 0 ] = 0;
}

void CDVD::DMA_Write ( u32* Data, int ByteWriteCount )
{
#ifdef INLINE_DEBUG_WRITE
	debug << "\r\nCDVD::DMA_Write; Data = " << Data [ 0 ];
#endif
}




void CDVD::EnqueueResult ( u32 Size )
{
	// set the number of bytes in result
	lResultCount = Size;
	
	// start the reading from beginning of result buffer
	lResultIndex = 0;
	
	// data is ready in response/result buffer
	SReady &= ~0x40;
	
	// ?? Reset argument buffer index ??
	lSArgIndex = 0;
	
	// testing
	//NReady = 0x40;
}


bool CDVD::LoadNVMFile ( string FilePath )
{
	//static const char* c_sClassName = "CDVD";
	//static const char* c_sMethodName = "LoadNVMFile";
	static const char* c_sPrefix = "CDVD::LoadNVMFile: ";
	static const char* c_sErrorString = "CDVD::LoadNVMFile: Error loading NVM File.";
	static const char* c_sSuccessString = "CDVD::LoadNVMFile: Successfully loaded NVM File.";

#ifdef INLINE_DEBUG
	debug << "\r\nEntered function: LoadTestProgram";
#endif

	ifstream InputFile ( FilePath.c_str (), ios::binary );
	
	if ( !InputFile )
	{
#ifdef INLINE_DEBUG
	debug << c_sErrorString << "\n";
#endif

		cout << c_sErrorString << "\n";
		cout << "Path=" << FilePath.c_str () << "\n";
		return false;
	}


#ifdef INLINE_DEBUG
	debug << c_sPrefix << " loading NVM file." << "\n";
#endif

	// write entire program into memory
	//InputFile.read ( (char*) ( _BUS.BIOS.b32 ), BIOS_SIZE_IN_BYTES );
	InputFile.read ( (char*) ( NVM ), c_iNVMSize );
	
	InputFile.close();
	
#ifdef INLINE_DEBUG
	debug << c_sPrefix << " done." < "\n";
#endif
	
	cout << c_sSuccessString << "\n";
	cout << "Path=" << FilePath.c_str () << "\n";
	return true;
}


void CDVD::Process_MechaConCommand ( u8 Command )
{
	//switch ( _CDVD->ucSArgBuffer [ 0 ] )
	switch ( Command )
	{
		case 0x0:
#ifdef INLINE_DEBUG_WRITE
	debug << "; GetVersion";
#endif

			_CDVD->EnqueueResult ( 4 );
			_CDVD->ucResultBuffer[ 0 ] = 0x03;
			_CDVD->ucResultBuffer[ 1 ] = 0x06;
			_CDVD->ucResultBuffer[ 2 ] = 0x02;
			_CDVD->ucResultBuffer[ 3 ] = 0x00;
			break;
			
		case 0x44:
#ifdef INLINE_DEBUG_WRITE
	debug << "; SetConsoleID";
#endif
			// *** TODO ***
			_CDVD->EnqueueResult ( 1 );
			_CDVD->ucResultBuffer[ 0 ] = 0x00;
			break;
			
		case 0x45:
#ifdef INLINE_DEBUG_WRITE
	debug << "; GetConsoleID";
#endif
			// *** TODO ***
			_CDVD->EnqueueResult ( 9 );
			_CDVD->ucResultBuffer[ 0 ] = 0x00;
			_CDVD->ucResultBuffer[ 1 ] = 0x00;
			_CDVD->ucResultBuffer[ 2 ] = 0x00;
			_CDVD->ucResultBuffer[ 3 ] = 0x00;
			_CDVD->ucResultBuffer[ 4 ] = 0x00;
			_CDVD->ucResultBuffer[ 5 ] = 0x00;
			_CDVD->ucResultBuffer[ 6 ] = 0x00;
			_CDVD->ucResultBuffer[ 7 ] = 0x00;
			_CDVD->ucResultBuffer[ 8 ] = 0x00;
			break;
			
		case 0xfd:
#ifdef INLINE_DEBUG_WRITE
	debug << "; GetRenewalDate";
#endif

			_CDVD->EnqueueResult ( 6 );
			_CDVD->ucResultBuffer[ 0 ] = 0x00;
			_CDVD->ucResultBuffer[ 1 ] = 0x04;	// year
			_CDVD->ucResultBuffer[ 2 ] = 0x12;	// day
			_CDVD->ucResultBuffer[ 3 ] = 0x10;	// month
			_CDVD->ucResultBuffer[ 4 ] = 0x01;	// hour
			_CDVD->ucResultBuffer[ 5 ] = 0x30;	// min
			
			break;
			
		default:
#ifdef INLINE_DEBUG_WRITE
	debug << "; Unknown MECHA-CON Command";
#endif

			_CDVD->EnqueueResult ( 1 );
			_CDVD->ucResultBuffer[ 0 ] = 0x80;
			break;
	}
}


void CDVD::Process_SCommand ( u8 Command )
{
	//switch ( Data & 0xff )
	switch ( Command )
	{
		case 0x2:	//READ SUBQ
#ifdef INLINE_DEBUG_WRITE
	debug << "; READ SUBQ";
#endif

			break;
			
		case 0x3:	// MECHA-CON COMMAND ??
#ifdef INLINE_DEBUG_WRITE
	debug << "; MECHA-CON???";
#endif

			_CDVD->Process_MechaConCommand ( _CDVD->ucSArgBuffer [ 0 ] );


			break;
			
		case 0x5:	// TRAY STATE REQUEST
#ifdef INLINE_DEBUG_WRITE
	debug << "; TRAY STATE";
#endif

			// either 1 for open or 0 for closed
			// for now we'll say its closed
			_CDVD->EnqueueResult ( 1 );
			_CDVD->ucResultBuffer[ 0 ] = 0x0;
			
			break;
			
		case 0x6:	// TRAY CTRL
#ifdef INLINE_DEBUG_WRITE
	debug << "; TRAY CTRL";
#endif


			break;
			
		case 0x8:	// READ RTC
#ifdef INLINE_DEBUG_WRITE
	debug << "; READ RTC";
#endif

			static const u64 ullCyclesPerSecond = 36864000ULL;
			static const u64 ullCyclesPerMinute = 36864000ULL * 60ULL;
			static const u64 ullCyclesPerHour = 36864000ULL * 60ULL * 60ULL;

			//SetResultSize(8);
			_CDVD->EnqueueResult ( 8 );
			
			/*
			cdvd.Result[0] = 0;
			cdvd.Result[1] = itob(cdvd.RTC.second); //Seconds
			cdvd.Result[2] = itob(cdvd.RTC.minute); //Minutes
			cdvd.Result[3] = itob(cdvd.RTC.hour); //Hours
			cdvd.Result[4] = 0; //Nothing
			cdvd.Result[5] = itob(cdvd.RTC.day); //Day
			cdvd.Result[6] = itob(cdvd.RTC.month); //Month
			cdvd.Result[7] = itob(cdvd.RTC.year); //Year
			*/
			
			// ***TODO*** Output correct time RTC time value
			_CDVD->ucResultBuffer [ 0 ] = 0;
			
			// seconds
			// Cycle / CyclesPerSecond
			_CDVD->ucResultBuffer [ 1 ] = *_DebugCycleCount / ullCyclesPerSecond;
			
			// minutes
			// Cycles / CyclesPerMinute
			_CDVD->ucResultBuffer [ 2 ] = *_DebugCycleCount / ullCyclesPerMinute;
			
			// hours
			// Cycles / CyclesPerHour
			_CDVD->ucResultBuffer [ 3 ] = *_DebugCycleCount / ullCyclesPerHour;
			
			// zero
			_CDVD->ucResultBuffer [ 4 ] = 0;
			
			// Day
			_CDVD->ucResultBuffer [ 5 ] = 1;
			
			// Month
			_CDVD->ucResultBuffer [ 6 ] = 1;
			
			// year
			_CDVD->ucResultBuffer [ 7 ] = 0x14;
			
			break;
			
		case 0x9:	// WRITE RTC
#ifdef INLINE_DEBUG_WRITE
	debug << "; WRITE RTC";
#endif

			//SetResultSize(1);
			_CDVD->EnqueueResult ( 1 );
			
			//cdvd.Result[0] = 0;
			_CDVD->ucResultBuffer [ 0 ] = 0;
			
			// ***TODO*** write RTC Value
			/*
			cdvd.RTC.pad = 0;
			cdvd.RTC.second = btoi(cdvd.Param[cdvd.ParamP-7]);
			cdvd.RTC.minute = btoi(cdvd.Param[cdvd.ParamP-6]) % 60;
			cdvd.RTC.hour = btoi(cdvd.Param[cdvd.ParamP-5]) % 24;
			cdvd.RTC.day = btoi(cdvd.Param[cdvd.ParamP-3]);
			cdvd.RTC.month = btoi(cdvd.Param[cdvd.ParamP-2] & 0x7f);
			cdvd.RTC.year = btoi(cdvd.Param[cdvd.ParamP-1]);
			*/
			
			break;
		
		// sceCdForbidDVDP (0:1)
		case 0x15:
#ifdef INLINE_DEBUG_WRITE
	debug << "; SCE FORBID DVDP";
#endif

			//SetResultSize(1);
			_CDVD->EnqueueResult ( 1 );
			
			//cdvd.Result[0] = 5;
			_CDVD->ucResultBuffer [ 0 ] = 5;
			
			break;
		
		// CdReadModelNumber (1:9) - from xcdvdman
		case 0x17:
#ifdef INLINE_DEBUG_WRITE
	debug << "; READ MODEL NUMBER";
#endif

			//SetResultSize(9);
			_CDVD->EnqueueResult ( 9 );
			
			//cdvdReadModelNumber(&cdvd.Result[1], cdvd.Param[0]);
			memcpy ( & _CDVD->ucResultBuffer [ 1 ], & ( _CDVD->NVM [ _CDVD->ucSArgBuffer [ 0 ] + nvmlayouts [ c_iBiosVersion ].modelNum ] ), 8 );
			
			break;
			
		// sceCdBootCertify (4:1)//(4:16 in psx?)
		case 0x1a:
#ifdef INLINE_DEBUG_WRITE
	debug << "; SCE BOOT CERTIFY";
#endif

			//on input there are 4 bytes: 1;?10;J;C for 18000; 1;60;E;C for 39002 from ROMVER
			//SetResultSize(1);
			_CDVD->EnqueueResult ( 1 );
			
			//i guess that means okay
			//cdvd.Result[0] = 1;
			_CDVD->ucResultBuffer [ 0 ] = 1;
			
			break;
			
		// sceCdCancelPOffRdy (0:1)
		case 0x1b:
			_CDVD->EnqueueResult ( 1 );
			_CDVD->ucResultBuffer [ 0 ] = 0;
			break;
			
		// sceRemote2Read
		case 0x1e:
#ifdef INLINE_DEBUG_WRITE
	debug << "; Remote2Read";
#endif

			_CDVD->EnqueueResult ( 5 );
			
			_CDVD->ucResultBuffer [ 0 ] = 0x00;
			_CDVD->ucResultBuffer [ 1 ] = 0x14;
			_CDVD->ucResultBuffer [ 2 ] = 0x00;
			_CDVD->ucResultBuffer [ 3 ] = 0x00;
			_CDVD->ucResultBuffer [ 4 ] = 0x00;
			break;
			
		// sceCdReadWakeUpTime
		case 0x22:
#ifdef INLINE_DEBUG_WRITE
	debug << "; CdReadWakeUpTime";
#endif
		
			_CDVD->EnqueueResult ( 10 );
			
			_CDVD->ucResultBuffer [ 0 ] = 0;
			_CDVD->ucResultBuffer [ 1 ] = 0;
			_CDVD->ucResultBuffer [ 2 ] = 0;
			_CDVD->ucResultBuffer [ 3 ] = 0;
			_CDVD->ucResultBuffer [ 4 ] = 0;
			_CDVD->ucResultBuffer [ 5 ] = 0;
			_CDVD->ucResultBuffer [ 6 ] = 0;
			_CDVD->ucResultBuffer [ 7 ] = 0;
			_CDVD->ucResultBuffer [ 8 ] = 0;
			_CDVD->ucResultBuffer [ 9 ] = 0;
			
			break;
			
			
		// sceCdRCBypassCtrl
		case 0x24:
#ifdef INLINE_DEBUG_WRITE
	debug << "; CdRCBypassCtrl";
#endif
		
			_CDVD->EnqueueResult ( 1 );
			
			_CDVD->ucResultBuffer [ 0 ] = 0;
			break;
			
		// region??
		case 0x36:
#ifdef INLINE_DEBUG_WRITE
	debug << "; REGION??";
#endif

			_CDVD->EnqueueResult ( 15 );
			
			// ???
			_CDVD->ucResultBuffer [ 0 ] = 0;
			
			_CDVD->ucResultBuffer [ 1 ] = 1 << 3;
			_CDVD->ucResultBuffer [ 2 ] = 0;
			
			
			// region ??? 8 values ???
			//memcpy ( & _CDVD->ucResultBuffer [ 3 ], & ( _CDVD->NVM [ nvmlayouts [ c_iBiosVersion ].regparams ] ), 8 );
			switch ( _CDVD->Region )
			{
				case 'J':
					memcpy ( & _CDVD->ucResultBuffer [ 3 ], _CDVD->c_cRegion_JAP, 8 );
					break;
					
				case 'A':
					memcpy ( & _CDVD->ucResultBuffer [ 3 ], _CDVD->c_cRegion_US, 8 );
					break;
					
				case 'E':
					memcpy ( & _CDVD->ucResultBuffer [ 3 ], _CDVD->c_cRegion_EU, 8 );
					break;
					
				case 'H':
					memcpy ( & _CDVD->ucResultBuffer [ 3 ], _CDVD->c_cRegion_H, 8 );
					break;
					
				case 'R':
					memcpy ( & _CDVD->ucResultBuffer [ 3 ], _CDVD->c_cRegion_R, 8 );
					break;
					
				case 'C':
					memcpy ( & _CDVD->ucResultBuffer [ 3 ], _CDVD->c_cRegion_C, 8 );
					break;
					
				case 'K':
					memcpy ( & _CDVD->ucResultBuffer [ 3 ], _CDVD->c_cRegion_Korea, 8 );
					break;
			}
			
			//_CDVD->ucResultBuffer [ 3 ] = 0;
			//_CDVD->ucResultBuffer [ 4 ] = 'E';
			//_CDVD->ucResultBuffer [ 5 ] = 'e';
			//_CDVD->ucResultBuffer [ 6 ] = 'n';
			//_CDVD->ucResultBuffer [ 7 ] = 'g';
			//_CDVD->ucResultBuffer [ 8 ] = 0;
			//_CDVD->ucResultBuffer [ 9 ] = 'O';
			//_CDVD->ucResultBuffer [ 10 ] = 0;
			
			
			// ?????
			_CDVD->ucResultBuffer [ 11 ] = 0;
			_CDVD->ucResultBuffer [ 12 ] = 0;
			_CDVD->ucResultBuffer [ 13 ] = 0;
			_CDVD->ucResultBuffer [ 14 ] = 0;
			break;
			
			
		
		case 0x40:	// OPEN CONFIG
#ifdef INLINE_DEBUG_WRITE
	debug << "; OPEN CONFIG";
#endif

			_CDVD->ReadWrite = _CDVD->ucSArgBuffer [ 0 ];
			_CDVD->Offset = _CDVD->ucSArgBuffer [ 1 ];
			_CDVD->NumBlocks = _CDVD->ucSArgBuffer [ 2 ];
			_CDVD->BlockIndex = 0;
			
			_CDVD->ucResultBuffer [ 0 ] = 0;
			
			// data is ready in response buffer
			_CDVD->EnqueueResult ( 1 );
			
			break;
			
		case 0x41:	// READ CONFIG
#ifdef INLINE_DEBUG_WRITE
	debug << "; READ CONFIG";
#endif

			// 16 byte result
			_CDVD->EnqueueResult ( 16 );

			if( _CDVD->ReadWrite != 0 )
			{
				//cout << "\nset to write mode";
				_CDVD->ucResultBuffer[0] = 0x80;
				memset(&_CDVD->ucResultBuffer[1], 0x00, 15);
				return;
			}
			// check if block index is in bounds
			else if( _CDVD->BlockIndex >= _CDVD->NumBlocks )
				return;
			else if(
				((_CDVD->Offset == 0) && (_CDVD->BlockIndex >= 4))||
				((_CDVD->Offset == 1) && (_CDVD->BlockIndex >= 2))||
				((_CDVD->Offset == 2) && (_CDVD->BlockIndex >= 7))
				)
			{
				//cout << "\nconfig#" << _CDVD->Offset << " clearing";
				memset(_CDVD->ucResultBuffer, 0x00, 16);
				return;
			}

			// get config data
			switch (_CDVD->Offset)
			{
				case 0:
					memcpy ( _CDVD->ucResultBuffer, & ( _CDVD->NVM [ ( _CDVD->BlockIndex * 16 ) + nvmlayouts [ c_iBiosVersion ].config0 ] ), 16 );
					//cout << "\nconfig0;offset=" << hex << ( _CDVD->BlockIndex * 16 ) + nvmlayouts [ c_iBiosVersion ].config1;
					_CDVD->BlockIndex++;
					break;
					
				case 2:
					memcpy ( _CDVD->ucResultBuffer, & ( _CDVD->NVM [ ( _CDVD->BlockIndex * 16 ) + nvmlayouts [ c_iBiosVersion ].config2 ] ), 16 );
					//cout << "\nconfig2;offset=" << hex << ( _CDVD->BlockIndex * 16 ) + nvmlayouts [ c_iBiosVersion ].config1;
					_CDVD->BlockIndex++;
					break;
					
				default:
					memcpy ( _CDVD->ucResultBuffer, & ( _CDVD->NVM [ ( _CDVD->BlockIndex * 16 ) + nvmlayouts [ c_iBiosVersion ].config1 ] ), 16 );
					//cout << "\nconfig1;offset=" << hex << ( _CDVD->BlockIndex * 16 ) + nvmlayouts [ c_iBiosVersion ].config1;
					_CDVD->BlockIndex++;
					break;
			}

			break;
			
		case 0x42:	// WRITE CONFIG
#ifdef INLINE_DEBUG_WRITE
	debug << "; WRITE CONFIG";
#endif

			
			break;
			
		case 0x43:	// CLOSE CONFIG
#ifdef INLINE_DEBUG_WRITE
	debug << "; CLOSE CONFIG";
#endif

			_CDVD->ucResultBuffer [ 0 ] = 0;
			_CDVD->EnqueueResult ( 1 );
			break;
			
			
		// secrman: __mechacon_auth_0x80
		case 0x80:
#ifdef INLINE_DEBUG_WRITE
	debug << "; MECHACON AUTH 0x80";
#endif
			_CDVD->mg_datatype = 0;
			_CDVD->ucResultBuffer [ 0 ] = 0;
			_CDVD->EnqueueResult ( 1 );
			break;
			
		case 0x81: // secrman: __mechacon_auth_0x81
#ifdef INLINE_DEBUG_WRITE
	debug << "; MECHACON AUTH 0x81";
#endif
			_CDVD->mg_datatype = 0;
			_CDVD->ucResultBuffer [ 0 ] = 0;
			_CDVD->EnqueueResult ( 1 );
			break;

		case 0x82: // secrman: __mechacon_auth_0x82
#ifdef INLINE_DEBUG_WRITE
	debug << "; MECHACON AUTH 0x82";
#endif
			_CDVD->ucResultBuffer [ 0 ] = 0;
			_CDVD->EnqueueResult ( 1 );
			break;

		case 0x83: // secrman: __mechacon_auth_0x83
#ifdef INLINE_DEBUG_WRITE
	debug << "; MECHACON AUTH 0x83";
#endif
			_CDVD->ucResultBuffer [ 0 ] = 0;
			_CDVD->EnqueueResult ( 1 );
			break;

		case 0x84: // secrman: __mechacon_auth_0x84
#ifdef INLINE_DEBUG_WRITE
	debug << "; MECHACON AUTH 0x84";
#endif
			_CDVD->EnqueueResult (1+8+4);
			_CDVD->ucResultBuffer [0] = 0;

			_CDVD->ucResultBuffer [1] = 0x21;
			_CDVD->ucResultBuffer [2] = 0xdc;
			_CDVD->ucResultBuffer [3] = 0x31;
			_CDVD->ucResultBuffer [4] = 0x96;
			_CDVD->ucResultBuffer [5] = 0xce;
			_CDVD->ucResultBuffer [6] = 0x72;
			_CDVD->ucResultBuffer [7] = 0xe0;
			_CDVD->ucResultBuffer [8] = 0xc8;

			_CDVD->ucResultBuffer [9]  = 0x69;
			_CDVD->ucResultBuffer [10] = 0xda;
			_CDVD->ucResultBuffer [11] = 0x34;
			_CDVD->ucResultBuffer [12] = 0x9b;
			break;

		case 0x85: // secrman: __mechacon_auth_0x85
#ifdef INLINE_DEBUG_WRITE
	debug << "; MECHACON AUTH 0x85";
#endif
			_CDVD->EnqueueResult (1+4+8);
			_CDVD->ucResultBuffer[0] = 0;

			_CDVD->ucResultBuffer [1] = 0xeb;
			_CDVD->ucResultBuffer [2] = 0x01;
			_CDVD->ucResultBuffer [3] = 0xc7;
			_CDVD->ucResultBuffer [4] = 0xa9;

			_CDVD->ucResultBuffer [ 5] = 0x3f;
			_CDVD->ucResultBuffer [ 6] = 0x9c;
			_CDVD->ucResultBuffer [ 7] = 0x5b;
			_CDVD->ucResultBuffer [ 8] = 0x19;
			_CDVD->ucResultBuffer [ 9] = 0x31;
			_CDVD->ucResultBuffer [10] = 0xa0;
			_CDVD->ucResultBuffer [11] = 0xb3;
			_CDVD->ucResultBuffer [12] = 0xa3;
			break;

		case 0x86: // secrman: __mechacon_auth_0x86
#ifdef INLINE_DEBUG_WRITE
	debug << "; MECHACON AUTH 0x86";
#endif
			_CDVD->ucResultBuffer [ 0 ] = 0;
			_CDVD->EnqueueResult ( 1 );
			break;

		case 0x87: // secrman: __mechacon_auth_0x87
#ifdef INLINE_DEBUG_WRITE
	debug << "; MECHACON AUTH 0x87";
#endif
			_CDVD->ucResultBuffer [ 0 ] = 0;
			_CDVD->EnqueueResult ( 1 );
			break;
			
		case 0x88: // secrman: __mechacon_auth_0x88	//for now it is the same; so, fall;)
		case 0x8F: // secrman: __mechacon_auth_0x8F
#ifdef INLINE_DEBUG_WRITE
	debug << "; MECHACON AUTH 0x88/0x8f";
#endif

			//SetResultSize(1);//in:0
			_CDVD->EnqueueResult ( 1 );
			
			//if (cdvd.mg_datatype == 1) // header data
			if ( _CDVD->mg_datatype == 1 )
			{
				// *** TODO ***
				cout << "\nhps1x64: CDVD: TODO: mg_datatype = 1!!!\n";
				/*
				u64* psrc, *pdst;
				int bit_ofs, i;

				if ((cdvd.mg_maxsize != cdvd.mg_size)||(cdvd.mg_size < 0x20) || (cdvd.mg_size != *(u16*)&cdvd.mg_buffer[0x14]))
				{
					fail_pol_cal();
					break;
				}

				std::string zoneStr;
				for (i=0; i<8; i++)
				{
					if (cdvd.mg_buffer[0x1C] & (1<<i)) zoneStr += mg_zones[i];
				}

				Console.WriteLn("[MG] ELF_size=0x%X Hdr_size=0x%X unk=0x%X flags=0x%X count=%d zones=%s",
					*(u32*)&cdvd.mg_buffer[0x10], *(u16*)&cdvd.mg_buffer[0x14], *(u16*)&cdvd.mg_buffer[0x16],
					*(u16*)&cdvd.mg_buffer[0x18], *(u16*)&cdvd.mg_buffer[0x1A],
					zoneStr.c_str()
				);

				bit_ofs = mg_BIToffset(cdvd.mg_buffer);

				psrc = (u64*)&cdvd.mg_buffer[bit_ofs-0x20];

				pdst = (u64*)cdvd.mg_kbit;
				pdst[0] = psrc[0];
				pdst[1] = psrc[1];
				//memcpy(cdvd.mg_kbit, &cdvd.mg_buffer[bit_ofs-0x20], 0x10);

				pdst = (u64*)cdvd.mg_kcon;
				pdst[0] = psrc[2];
				pdst[1] = psrc[3];
				//memcpy(cdvd.mg_kcon, &cdvd.mg_buffer[bit_ofs-0x10], 0x10);

				if ((cdvd.mg_buffer[bit_ofs+5] || cdvd.mg_buffer[bit_ofs+6] || cdvd.mg_buffer[bit_ofs+7]) ||
					(cdvd.mg_buffer[bit_ofs+4] * 16 + bit_ofs + 8 + 16 != *(u16*)&cdvd.mg_buffer[0x14]))
				{
					fail_pol_cal();
					break;
				}
				*/
			}
			
			_CDVD->ucResultBuffer [0] = 0; // 0 complete ; 1 busy ; 0x80 error
			break;
			
		default:
#ifdef INLINE_DEBUG_WRITE
	debug << "; UNKNOWN 0x16 Command";
#endif

			break;
	}
}


void CDVD::Process_NCommand ( u8 Command )
{
	s32 PrevSector, SeekDelta;
	
	//switch ( Data & 0xff )
	switch ( Command )
	{
		// SYNC (or motor on?)
		case 0:

		// NOP
		case 1:
#ifdef INLINE_DEBUG_WRITE
	debug << "; SYNC/NOP";
#endif

			// set the read command as what the command was
			_CDVD->ReadCommand = Command;
			
			_CDVD->Set_NextEvent ( c_lCDSeek_Cycles );
		
			break;
			
		// STOP
		case 0x3:
		
			_CDVD->ReadCommand = Command;
			
			_CDVD->Set_NextEvent ( c_lCDStop_Cycles );
			
			break;
			
		// PAUSE
		case 0x4:
#ifdef INLINE_DEBUG_WRITE
	debug << "; PAUSE";
#endif

			// set the read command as pause
			_CDVD->ReadCommand = 0x4;
			
			// ***todo*** pause might take longer
			_CDVD->Set_NextEvent ( c_lCDSeek_Cycles );
			
			// reset input fifo for N Commands
			_CDVD->lNArgIndex = 0;
			break;
			
		// CD READ (reads data from CD)
		case 0x6:
#ifdef INLINE_DEBUG_WRITE
	debug << "; CD READ";
#endif

			PrevSector = SeekSectorNum;
			
			SeekSectorNum = ( (u32*) _CDVD->ucNArgBuffer ) [ 0 ];
			SectorReadCount = ( (u32*) _CDVD->ucNArgBuffer ) [ 1 ];
			
			// get the read speed
			DiskSpeed = _CDVD->ucNArgBuffer [ 9 ];
			
			switch ( DiskSpeed )
			{
				case 0x3:
					// means x4
					DiskSpeed = 4;
					break;
					
				case 0x85:
					// means x24
					DiskSpeed = 24;
					break;
					
				default:
					// unknown
					cout << "\nhps2x64: CDVD: ***ALERT***: Unknown disk speed set=" << hex << DiskSpeed << "\n";
					DiskSpeed = 12;
					break;
			}
			
			// get difference from last read sector
			SeekDelta = PrevSector - SeekSectorNum;
			
			// get absolute value
			SeekDelta = ( SeekDelta < 0 ) ? -SeekDelta : SeekDelta;
			
			
			// byte 8 is retry count ??
			// byte 9 spndlctrl ??
			
			// byte 10 is the sector read size (0: 2048, 1: 2328, 2: 2340)
			switch ( _CDVD->ucNArgBuffer [ 10 ] )
			{
				case 0:
				
					_CDVD->SectorReadSize = 2048;
					
					// ???
					// if CD then read offset would be 24
					if ( _CDVD->CurrentDiskType == CDVD_TYPE_PS2DVD )
					{
						CD::_CD->ReadMode_Offset = 0;
					}
					else
					{
						CD::_CD->ReadMode_Offset = 24;
					}
					break;
					
				case 1:
					_CDVD->SectorReadSize = 2328;
					CD::_CD->ReadMode_Offset = 24;
					break;
					
				case 2:
					_CDVD->SectorReadSize = 2340;
					CD::_CD->ReadMode_Offset = 12;
					break;
					
				default:
					_CDVD->SectorReadSize = 2048;
					CD::_CD->ReadMode_Offset = 24;
					break;
			}
			
			// set the sector size for ps1 cd handler
			CD::_CD->DataBuffer_Size = _CDVD->SectorReadSize;
			CD::_CD->ReadMode = _CDVD->SectorReadSize;
			
			// reset the index on ps1 side
			CD::_CD->DataBuffer_Index = 0;
			
			// seek to sector
			CD::_CD->cd_image.SeekSector ( _CDVD->SeekSectorNum + DiskImage::CDImage::c_SectorsInFirstTwoSeconds );
			CD::_CD->cd_image.StartReading ();
			
			// set the read command as cd read
			_CDVD->ReadCommand = 0x6;
			
			// status is now "seek" ??
			Status = CDVD_STATUS_SEEK;
			
			// check if this will be a short seek, longer seek, or longer running seek
			if ( SeekDelta <= c_lNoSeekDelta )
			{
				// virtually no seek time
				Set_NextEvent ( 8 );
			}
			else if ( SeekDelta <= c_lFastCDSeekDelta )
			{
				// fast seek
				Set_NextEvent ( c_llCDFastSeekInCycles );
			}
			else
			{
				// long running seek
				Set_NextEvent ( c_llCDSeekInCycles );
			}
			
			//if ( _CDVD->SeekSectorNum < 64 )
			//{
			//	// ***todo*** schedule seek and read to occur
			//	_CDVD->Set_NextEvent ( c_lCDSeek_Cycles );
			//}
			//else
			//{
			//	_CDVD->Set_NextEvent ( 4000000 );
			//}
			
			// command done
			// reset input fifo for N Commands
			_CDVD->lNArgIndex = 0;
			
			// testing //
			//_CDVD->NReady = 0x40;
			
			break;
			
		
		// DVD READ
		case 0x8:
#ifdef INLINE_DEBUG_WRITE
	debug << "; DVD READ";
#endif
		
			_CDVD->SeekSectorNum = ( (u32*) _CDVD->ucNArgBuffer ) [ 0 ];
			_CDVD->SectorReadCount = ( (u32*) _CDVD->ucNArgBuffer ) [ 1 ];
			
			// byte 8 is retry count ??
			// byte 9 spndlctrl ??
			
			// sector size is always 2064 for DVD read
			_CDVD->SectorReadSize = 2064;
			CD::_CD->ReadMode_Offset = 0;
			
			
			// set the sector size for ps1 cd handler
			CD::_CD->DataBuffer_Size = _CDVD->SectorReadSize;
			CD::_CD->ReadMode = _CDVD->SectorReadSize;
			
			// reset the index on ps1 side
			CD::_CD->DataBuffer_Index = 0;
			
			// seek to sector
			CD::_CD->cd_image.SeekSector ( _CDVD->SeekSectorNum + DiskImage::CDImage::c_SectorsInFirstTwoSeconds );
			CD::_CD->cd_image.StartReading ();
			
			// set the read command as cd read
			_CDVD->ReadCommand = 0x6;
			
			// status is now "seek" ??
			Status = CDVD_STATUS_SEEK;
			
			// ***todo*** schedule seek and read to occur
			_CDVD->Set_NextEvent ( c_lCDSeek_Cycles );
			
			// command done
			// reset input fifo for N Commands
			_CDVD->lNArgIndex = 0;
			
			// testing //
			//_CDVD->NReady = 0x40;
			
			break;
			
			
		// READ TOC
		case 0x9:
#ifdef INLINE_DEBUG_WRITE
	debug << "; READ TOC";
#endif
		
			// set the read command as read toc
			_CDVD->ReadCommand = 0x9;
			
			// ***todo*** read toc might take longer
			_CDVD->Set_NextEvent ( c_lCDSeek_Cycles );
			
			// reset input fifo for N Commands
			_CDVD->lNArgIndex = 0;
			break;
			
		// READ KEY
		case 0xc:
#ifdef INLINE_DEBUG_WRITE
	debug << "; READ KEY";
#endif

			u8  arg0 = _CDVD->ucNArgBuffer [0];
			u16 arg1 = ( (u32) _CDVD->ucNArgBuffer [1] ) | (( (u32) _CDVD->ucNArgBuffer[2] )<<8);
			u32 arg2 = ( (u32) _CDVD->ucNArgBuffer[3] ) | (( (u32) _CDVD->ucNArgBuffer[4] )<<8) | (( (u32) _CDVD->ucNArgBuffer[5] )<<16) | (( (u32) _CDVD->ucNArgBuffer[6] )<<24);
			
			char* DiskSerial = CD::_CD->cd_image.DiskSerial;
			
			// SLXX_YYY.ZZ
			s32 numbers=0, letters=0;
			u32 key_0_3;
			u8 key_4, key_14;
			
			char NumbersStr [ 16 ];
			
			// ***todo*** read the cd key
			
			// clear key
			memset ( _CDVD->DiskKey, 0, 16 );
			
			// put numbers into a string and convert to a number
			NumbersStr [ 0 ] = DiskSerial [ 5 ];
			NumbersStr [ 1 ] = DiskSerial [ 6 ];
			NumbersStr [ 2 ] = DiskSerial [ 7 ];
			NumbersStr [ 3 ] = DiskSerial [ 9 ];
			NumbersStr [ 4 ] = DiskSerial [ 10 ];
			NumbersStr [ 5 ] = 0;
			
			// convert to a number
			numbers = Utilities::Strings::CLng ( NumbersStr );
			
			// combine the letters
			letters = (((s32)DiskSerial[3]&0x7F)<< 0) | (((s32)DiskSerial[2]&0x7F)<< 7) | (((s32)DiskSerial[1]&0x7F)<<14) | (((s32)DiskSerial[0]&0x7F)<<21);
			
			// calculate magic numbers
			key_0_3 = ((numbers & 0x1FC00) >> 10) | ((0x01FFFFFF & letters) <<  7);	// numbers = 7F  letters = FFFFFF80
			key_4   = ((numbers & 0x0001F) <<  3) | ((0x0E000000 & letters) >> 25);	// numbers = F8  letters = 07
			key_14  = ((numbers & 0x003E0) >>  2) | 0x04;							// numbers = F8  extra   = 04  unused = 03

			// store key values
			_CDVD->DiskKey [ 0] = (key_0_3&0x000000FF)>> 0;
			_CDVD->DiskKey [ 1] = (key_0_3&0x0000FF00)>> 8;
			_CDVD->DiskKey [ 2] = (key_0_3&0x00FF0000)>>16;
			_CDVD->DiskKey [ 3] = (key_0_3&0xFF000000)>>24;
			_CDVD->DiskKey [ 4] = key_4;
			
			switch (arg2)
			{
				case 75:
					_CDVD->DiskKey [14] = key_14;
					_CDVD->DiskKey [15] = 0x05;
					break;

		//      case 3075:
		//          key[15] = 0x01;
		//          break;

				case 4246:
					// 0x0001F2F707 = sector 0x0001F2F7  dec 0x07
					_CDVD->DiskKey [ 0] = 0x07;
					_CDVD->DiskKey [ 1] = 0xF7;
					_CDVD->DiskKey [ 2] = 0xF2;
					_CDVD->DiskKey [ 3] = 0x01;
					_CDVD->DiskKey [ 4] = 0x00;
					_CDVD->DiskKey [15] = 0x01;
					break;

				default:
					_CDVD->DiskKey [15] = 0x01;
					break;
			}
			
			// send CD interrupt
			SetInterrupt_CDVD ();
			
			// set the reason for the interrupt
			// 0x1 means "data ready"
			// 0x2 means "command complete"
			// 0x4 means "acknowledge" (or maybe 0x3??)
			// 0x8 means "end of data" ?? (or maybe 0x4??)
			_CDVD->InterruptReason = 0x2;
			
			// set xor key
			CD::_CD->XorKey = _CDVD->DiskKey [ 4 ];
			
			// command done
			// reset input fifo for N Commands
			_CDVD->lNArgIndex = 0;
			
			break;
	}
}


