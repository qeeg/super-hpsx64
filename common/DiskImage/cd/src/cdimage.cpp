

#include "string.h"
#include "cdimage.h"
#include "ConfigFile.h"

using namespace DiskImage;
using namespace x64ThreadSafe::Utilities;

using namespace Utilities::Strings;


CDImage* CDImage::_DISKIMAGE;
WinApi::File CDImage::image;
CDImage::ReadAsync_Params CDImage::_rap_params;
WinApi::File CDImage::sub;
bool CDImage::isSubOpen;
unsigned long CDImage::isDiskOpen;
u32 CDImage::isReadInProgress;
u32 CDImage::isSubReadInProgress;

ifstream* CDImage::CueFile;
ifstream* CDImage::CcdFile;


// need to be able to toggle between reading synchronously and asynchronously
#define DISK_READ_SYNC
//#define DISK_READ_ASYNC


CDImage::CDImage ()
{
	isDiskOpen = false;
	isSubOpen = false;
	
	isReadInProgress = false;
	isSubReadInProgress = false;
	
	ReadIndex = -1;
	WriteIndex = 0;
	
	// set pointer to self for static functions
	_DISKIMAGE =  this;
}


bool CDImage::ParseCueSheet ( string FilePath )
{
	string Line;
	int LineNumber = 1;
	
	int iIndex;
	int iTrack, iMin, iSec, iFrac;
	int iMin_InFile, iSec_InFile, iFrac_InFile, iMin_OnDisk, iSec_OnDisk, iFrac_OnDisk;
	int iPreGap_Min, iPreGap_Sec, iPreGap_Frac;
	
	bool bInsertPreGap;
	
	//int PreGap = 0;
	
	// set number of tracks
	iNumberOfTracks = 0;
	
	// set number of indexes - first entry is for TOC, so start at Entry#1
	iNumberOfIndexes = 1;
	
	// don't insert any stuff for any pregaps yet
	bInsertPreGap = false;
	
	// initialize the pregap
	iPreGap = 0;
	
	CueFile = new ifstream( FilePath.c_str(), ios::binary );
	
	if ( !CueFile->is_open() || CueFile->fail() ) return false;
	
	//cout << "\nCue file is valid. Beginning to parse. FilePath=" << FilePath;
	
	// keep running until we reach end of file
	while ( CueFile->good() )
	{
		// get a line from source file
		//cout << "Getting Line#" << LineNumber << " from the input file.\n";
		getline ( *CueFile, Line );

		// remove extra spaces
		Line = Trim ( Line );
		
		// check whether this is REM, TRACK, INDEX, etc
		if ( Left ( Line, 3 ) == "REM" )
		{
			// this is a comment //
		}
		else if ( Left ( Line, 5 ) == "TRACK" )
		{
			// this is a track number //
			
			// get the track number
			iTrack = CLng ( Split ( Line, " " ) [ 1 ] );
			
			// clear track data
			TrackData [ iTrack ].Min = 0;
			TrackData [ iTrack ].Sec = 0;
			TrackData [ iTrack ].Frac = 0;
			
			// increment number of tracks
			iNumberOfTracks++;
			
			cout << "\nTrack#" << dec << iTrack;
		}
		else if ( Left ( Line, 5 ) == "INDEX" )
		{
			// this is an index number //
			
			// get index number
			iIndex = CLng ( Split ( Line, " " ) [ 1 ] );
			
			// get min:sec:frac
			Line = Split ( Line, " " ) [ 2 ];
			
			// get min
			iMin = CLng ( Split ( Line, ":" ) [ 0 ] );
			
			// get sec
			iSec = CLng ( Split ( Line, ":" ) [ 1 ] );
			
			// get frac
			iFrac = CLng ( Split ( Line, ":" ) [ 2 ] );
			
			// the values in the cue file actually give the location in the .bin file, NOT on the disk
			iMin_InFile = iMin;
			iSec_InFile = iSec;
			iFrac_InFile = iFrac;
			
			// add 2 seconds to the time
			// and add pregap time up until this point
			iSec += 2 + iPreGap;
			if ( iSec >= 60 )
			{
				iSec -= 60;
				iMin++;
			}
			
			// check if pregap data should be inserted
			if ( bInsertPreGap )
			{
				// handling pregap now
				bInsertPreGap = false;
				
				// set this as an all zeroed sector that is NOT in the .bin image
				IndexData [ iNumberOfIndexes ].SectorNumber_InImage = -1;
				
				// put in track number
				IndexData [ iNumberOfIndexes ].Track = iTrack;
				
				// put in the index number (zero)
				IndexData [ iNumberOfIndexes ].Index = 0;
				
				// should probably start at pregap time and count down relatively
				IndexData [ iNumberOfIndexes ].Min = iPreGap_Min;
				IndexData [ iNumberOfIndexes ].Sec = iPreGap_Sec;
				IndexData [ iNumberOfIndexes ].Frac = iPreGap_Frac;
				
				// put in the absolute times
				IndexData [ iNumberOfIndexes ].AMin = iMin;
				IndexData [ iNumberOfIndexes ].ASec = iSec;
				IndexData [ iNumberOfIndexes ].AFrac = iFrac;
				
				// set the absolute sector number
				IndexData [ iNumberOfIndexes ].SectorNumber =  GetSectorNumber ( iMin, iSec, iFrac );
				
				// go to next index
				iNumberOfIndexes++;
				
				// update pregap time HERE
				iPreGap += iPreGap_Sec;
				
				// add new pregap seconds to the time
				iSec += iPreGap_Sec;
				if ( iSec >= 60 )
				{
					iSec -= 60;
					iMin++;
				}
			}
			
			cout << " Min=" << iMin << " Sec=" << iSec << " Frac=" << iFrac;
			
			// put min/sec/frac data in for track
			TrackData [ iTrack ].Min = iMin;
			TrackData [ iTrack ].Sec = iSec;
			TrackData [ iTrack ].Frac = iFrac;
			
			// put in rest of the new data
			
			// if the track number is greater than 1, and the index number is not zero, then put in index zero if not already there
			if ( iTrack > 1 && iIndex != 0 && IndexData [ iNumberOfIndexes - 1 ].Index != 0 )
			{
				u8 TMin, TSec, TFrac;
				
				// put in index zero //
				
				// set the sector to find the data at in the actual disk image
				IndexData [ iNumberOfIndexes ].SectorNumber_InImage = GetSectorNumber ( iMin_InFile, iSec_InFile, iFrac_InFile ) - 150;

				// put in track number
				IndexData [ iNumberOfIndexes ].Track = iTrack;
				
				// put in the index number (assume zero for now)
				IndexData [ iNumberOfIndexes ].Index = 0;
				
				// default of 2 minutes
				IndexData [ iNumberOfIndexes ].Min = 0;
				IndexData [ iNumberOfIndexes ].Sec = 2;
				IndexData [ iNumberOfIndexes ].Frac = 0;
				
				// put in the absolute times
				SplitSectorNumber ( GetSectorNumber ( iMin, iSec, iFrac ) - 150, TMin, TSec, TFrac );
				IndexData [ iNumberOfIndexes ].AMin = TMin;
				IndexData [ iNumberOfIndexes ].ASec = TSec;
				IndexData [ iNumberOfIndexes ].AFrac = TFrac;
				
				// set the absolute sector number
				IndexData [ iNumberOfIndexes ].SectorNumber =  GetSectorNumber ( iMin, iSec, iFrac ) - 150;
				
				// go to next index
				iNumberOfIndexes++;
			}
			
			// set the sector to find the data at in the actual disk image
			IndexData [ iNumberOfIndexes ].SectorNumber_InImage = GetSectorNumber ( iMin_InFile, iSec_InFile, iFrac_InFile );

			// put in track number
			IndexData [ iNumberOfIndexes ].Track = iTrack;
			
			// put in the index number (assume one for now)
			IndexData [ iNumberOfIndexes ].Index = iIndex;
			
			// should probably start at pregap time and count down relatively
			IndexData [ iNumberOfIndexes ].Min = 0;
			IndexData [ iNumberOfIndexes ].Sec = 0;
			IndexData [ iNumberOfIndexes ].Frac = 0;
			
			// put in the absolute times
			IndexData [ iNumberOfIndexes ].AMin = iMin;
			IndexData [ iNumberOfIndexes ].ASec = iSec;
			IndexData [ iNumberOfIndexes ].AFrac = iFrac;
			
			// set the absolute sector number
			IndexData [ iNumberOfIndexes ].SectorNumber =  GetSectorNumber ( iMin, iSec, iFrac );
			
			// go to next index
			iNumberOfIndexes++;
		}
		else if ( Left ( Line, 6 ) == "PREGAP" )
		{
			// this is a pregap //
			
			// insert pregap when processing the next index
			bInsertPreGap = true;
			
			// get min:sec:frac
			Line = Split ( Line, " " ) [ 1 ];
			
			// get min
			iPreGap_Min = CLng ( Split ( Line, ":" ) [ 0 ] );
			
			// get sec
			iPreGap_Sec = CLng ( Split ( Line, ":" ) [ 1 ] );
			
			// get frac
			iPreGap_Frac = CLng ( Split ( Line, ":" ) [ 2 ] );
			
			cout << " PreGap_Min=" << iPreGap_Min << " PreGap_Sec=" << iPreGap_Sec << " PreGap_Frac=" << iPreGap_Frac;
			
			// *** todo *** add pregap into list of tracks, etc
		}
		
	}
	
	if ( !iNumberOfTracks )
	{
		cout << "\nhps1x64 ERROR: CDImage has zero tracks. Error in CUE file.\n";
	}
	
	CueFile->close ();
	return true;
}


bool CDImage::ParseCcdSheet ( string FilePath )
{
	string Line;
	int LineNumber = 1;
	
	int iTrack, iMin, iSec, iFrac, iIndex, iNextIndex, iSector;
	
	// set number of tracks
	iNumberOfTracks = 0;
	
	// set number of indexes
	iNumberOfIndexes = 1;
	
	CcdFile = new ifstream( FilePath.c_str(), ios::binary );
	
	if ( !CcdFile->is_open() || CcdFile->fail() ) return false;
	
	//cout << "\nCcd file is valid. Beginning to parse. FilePath=" << FilePath;
	
	
	// keep running until we reach end of file
	while ( CcdFile->good() )
	{
		// get a line from source file
		//cout << "Getting Line#" << LineNumber << " from the input file.\n";
		getline ( *CcdFile, Line );

		// remove extra spaces
		Line = Trim ( Line );
		
		// check whether this is REM, TRACK, INDEX, etc
		/*
		if ( Left ( Line, 3 ) == "REM" )
		{
			// this is a comment //
		}
		else
		*/
		
		if ( Left ( Line, 6 ) == "[TRACK" )
		{
			// this is a track number //
			
			// get the track number
			iTrack = CLng ( Replace ( Split ( Line, " " ) [ 1 ], "]", "" ) );
			
			// clear track data
			TrackData [ iTrack ].Min = 0;
			TrackData [ iTrack ].Sec = 0;
			TrackData [ iTrack ].Frac = 0;
			
			// did not get index yet
			iNextIndex = 1;
			
			// increment number of tracks
			iNumberOfTracks++;
			
			cout << "\nTrack#" << dec << iTrack;
		}
		else if ( Left ( Line, 5 ) == "INDEX" )
		{
			// this is an index number //
			
			// only get first index listed for now
			// actually, just get the last one for PS1
			//if ( iNextIndex == 1 )
			//{
				// get index#=sector
				Line = Split ( Line, " " ) [ 1 ];
				
				// get index??
				iIndex = CLng ( Split ( Line, "=" ) [ 0 ] ) /*+ 1*/;
				
				// get min:sec:frac
				//Line = Split ( Line, " " ) [ 2 ];
				// get sector
				iSector = CLng ( Split ( Line, "=" ) [ 1 ] );
				
				// get min
				//iMin = CLng ( Split ( Line, ":" ) [ 0 ] );
				iMin = iSector / c_SectorsPerMinute;
				
				// get sec
				//iSec = CLng ( Split ( Line, ":" ) [ 1 ] );
				iSec = ( iSector % c_SectorsPerMinute ) / c_SectorsPerSecond;
				
				// get frac
				//iFrac = CLng ( Split ( Line, ":" ) [ 2 ] );
				iFrac = iSector % c_SectorsPerSecond;
				
				cout << " Index#" << iIndex << " Sector=" << iSector << " Min=" << iMin << " Sec=" << iSec << " Frac=" << iFrac;
				
				// add 2 seconds to the time
				iSec += 2;
				if ( iSec >= 60 )
				{
					iSec -= 60;
					iMin++;
				}
			
				// put min/sec/frac data in for track
				TrackData [ iTrack ].Min = iMin;
				TrackData [ iTrack ].Sec = iSec;
				TrackData [ iTrack ].Frac = iFrac;
				
				// get next index
				iNextIndex++;
				
				// now put in the new stuff //
				
				// if the track number is greater than 1, and the index number is not zero, then put in index zero if not already there
				if ( iTrack > 1 && iIndex != 0 && IndexData [ iNumberOfIndexes - 1 ].Index != 0 )
				{
					u8 TMin, TSec, TFrac;
					
					// put in index zero //
					
					// set the sector to find the data at in the actual disk image
					IndexData [ iNumberOfIndexes ].SectorNumber_InImage = GetSectorNumber ( iMin, iSec, iFrac ) - 150;

					// put in track number
					IndexData [ iNumberOfIndexes ].Track = iTrack;
					
					// put in the index number (assume zero for now)
					IndexData [ iNumberOfIndexes ].Index = 0;
					
					// default of 2 minutes
					IndexData [ iNumberOfIndexes ].Min = 0;
					IndexData [ iNumberOfIndexes ].Sec = 2;
					IndexData [ iNumberOfIndexes ].Frac = 0;
					
					// put in the absolute times
					SplitSectorNumber ( GetSectorNumber ( iMin, iSec, iFrac ) - 150, TMin, TSec, TFrac );
					IndexData [ iNumberOfIndexes ].AMin = TMin;
					IndexData [ iNumberOfIndexes ].ASec = TSec;
					IndexData [ iNumberOfIndexes ].AFrac = TFrac;
					
					// set the absolute sector number
					IndexData [ iNumberOfIndexes ].SectorNumber =  GetSectorNumber ( iMin, iSec, iFrac ) - 150;
					
					// go to next index
					iNumberOfIndexes++;
				}
				
				// put in the index/track
				IndexData [ iNumberOfIndexes ].Index = iIndex;
				IndexData [ iNumberOfIndexes ].Track = iTrack;
				
				// put in the relative time values
				if ( !iIndex )
				{
					// assume two seconds for now
					IndexData [ iNumberOfIndexes ].Min = 0;
					IndexData [ iNumberOfIndexes ].Sec = 2;
					IndexData [ iNumberOfIndexes ].Frac = 0;
				}
				else
				{
					IndexData [ iNumberOfIndexes ].Min = 0;
					IndexData [ iNumberOfIndexes ].Sec = 0;
					IndexData [ iNumberOfIndexes ].Frac = 0;
				}
				
				// put in the absolute time values
				IndexData [ iNumberOfIndexes ].AMin = iMin;
				IndexData [ iNumberOfIndexes ].ASec = iSec;
				IndexData [ iNumberOfIndexes ].AFrac = iFrac;
				
				// put in the sector this is found at in disk image
				IndexData [ iNumberOfIndexes ].SectorNumber_InImage = iSector;
				
				// put in the sector number on the actual physical disk
				IndexData [ iNumberOfIndexes ].SectorNumber = GetSectorNumber ( iMin, iSec, iFrac );
				
				// go to next index
				iNumberOfIndexes++;
				
			//}
			
		}
		
	}
	
	if ( !iNumberOfTracks )
	{
		cout << "\nhps1x64 ERROR: CDImage has zero tracks. Error in CCD file.\n";
	}

	CcdFile->close ();
	return true;
}


int CDImage::FindTrack ( int AMin, int ASec, int AFrac )
{
	int i;
	u32 SectorNumber;
	
	SectorNumber = GetSectorNumber ( AMin, ASec, AFrac );
	i = GetIndexData_Index ( SectorNumber );
	return IndexData [ i ].Track;
	
	/*
	for ( i = 1; i <= iNumberOfTracks; i++ )
	{
		if ( AMin <= TrackData [ i ].Min )
		{
			if ( AMin < TrackData [ i ].Min ) break;
			
			// TrackData [ i ].Min == AMin //
			if ( ASec <= TrackData [ i ].Sec )
			{
				if ( ASec < TrackData [ i ].Sec ) break;
				
				// TrackData [ i ].Sec == ASec //
				if ( AFrac < TrackData [ i ].Frac ) break;
			}
		}
	}
	
	return i - 1;
	*/
}



int CDImage::FindTrack ( u32 SectorNumber )
{
	int i;
	//u32 SectorNumber;
	
	//SectorNumber = GetSectorNumber ( AMin, ASec, AFrac );
	i = GetIndexData_Index ( SectorNumber );
	return IndexData [ i ].Track;
}



// sets all -1 on error
void CDImage::GetTrackStart ( int TrackNumber, unsigned char & AMin, unsigned char & ASec, unsigned char & AFrac )
{
	int i;
	
	for ( i = iNumberOfIndexes; i >= 0; i-- )
	{
		if ( TrackNumber == IndexData [ i ].Track && IndexData [ i ].Index == 1 )
		{
			AMin = IndexData [ i ].AMin;
			ASec = IndexData [ i ].ASec;
			AFrac = IndexData [ i ].AFrac;
			return;
		}
	}
	
	AMin = -1;
	ASec = -1;
	AFrac = -1;
	
	//Min = TrackData [ TrackNumber ].Min;
	//Sec = TrackData [ TrackNumber ].Sec;
	//Frac = TrackData [ TrackNumber ].Frac;
}


bool CDImage::OpenDiskImage ( string DiskImagePath, u32 DiskSectorSize )
{
	//cout << "\nCalling CDImage::OpenDiskImage; DiskImagePath=" << DiskImagePath.c_str();
	
	bool bDiskOpenedSuccessfully;
	
	string CueFile_Path;
	string CcdFile_Path;
	string SubFile_Path;
	
	bool ret;
	
	_DISKIMAGE = this;
	
	// set the sector size
	SectorSize = DiskSectorSize;
	
	// get path to sub file
	SubFile_Path = GetPath ( DiskImagePath ) + GetFile ( DiskImagePath ) + ".sub";
	
	// copy path
	strcpy ( ImagePath, DiskImagePath.c_str() );
	strcpy ( SubPath, SubFile_Path.c_str () );
	
	//cout << "\nImagePath=" << ImagePath;
	
	//asm volatile ( "mfence" );
	
	isDiskOpen = false;
	//Lock_Exchange32 ( (long&)isDiskOpen, false );
	
	// file cannot be accessed while it is opening, and we also need to send data to other thread
	//Lock_Exchange32 ( (long&)isReadInProgress, true );
	isReadInProgress = true;
	
	// first load the disk serial
	// ***todo*** sector size should be variable since this should work for DVDs too
	// assume this is a cd for now, will add disk check later
	ret = Config::PSXDiskUtility::GetPSXIDString ( DiskSerial, DiskImagePath.c_str (), SectorSize );
	
	if ( !ret )
	{
		cout << "\n***ERROR*** DiskImage: There was a problem obtaining disk serial. May not be a PSX disk.";
	}
	else
	{
		cout << "\nDiskImage: Disk Serial=" << DiskSerial;
	}
	
	// disk image probably needs to be opened on gui thread or else the handle is invalid
	//image = fopen64 ( DiskImagePath, "r" );
	//image = fopen ( DiskImagePath, "rb" );
	//if ( !image.Create ( DiskImagePath, OPEN_EXISTING, GENERIC_READ, NULL ) ) return false;
	//if ( !image.CreateAsync ( DiskImagePath ) ) return false;
	//bDiskOpenedSuccessfully = WindowClass::Window::RemoteCall ( (WindowClass::Window::RemoteFunction) _RemoteCall_OpenDiskImage, (void*) NULL, true );
	bDiskOpenedSuccessfully = _RemoteCall_OpenDiskImage ( DiskImagePath );
	
	// now try to open file with subchannel data
	//WindowClass::Window::RemoteCall ( (WindowClass::Window::RemoteFunction) _RemoteCall_OpenSubImage, (void*) NULL, true );
	
	/*
	// get just path and file name and add cue extension
	CueFile_Path = GetPath ( DiskImagePath ) + GetFile ( DiskImagePath ) + ".cue";
	
	if ( !ParseCueSheet ( CueFile_Path.c_str() ) )
	{
		cout << "\nCue file not found or other error. Should be: " << CueFile_Path.c_str();
		
		// get just path and file name and add ccd extension
		CcdFile_Path = GetPath ( DiskImagePath ) + GetFile ( DiskImagePath ) + ".ccd";
		
		if ( !ParseCcdSheet ( CcdFile_Path.c_str() ) )
		{
			cout << "\nCcd file not found or other error. Should be: " << CcdFile_Path.c_str();
			cout << "\nAssuming single data track.\n";
			
			// since there was an error, assume a single data track
			// single track starts at two seconds
			iNumberOfTracks = 1;
			TrackData [ 1 ].Min = 0;
			TrackData [ 1 ].Sec = 2;
			TrackData [ 1 ].Frac = 0;
		}
		else
		{
			cout << "\nCcd file found.";
		}
	}
	else
	{
		cout << "\nCue file found.";
	}
	*/

	//cout << "\nExiting CDImage::OpenDiskImage";
	
	// disk image was opened successfully
	//isDiskOpen = true;
	return bDiskOpenedSuccessfully;
}

bool CDImage::CloseDiskImage ()
{
	// close disk image and make sure it closed successfully
	if ( isDiskOpen )
	{
		//if ( !fclose ( image ) ) return true;
		if ( !image.Close() ) return false;
	}

	// check if .sub file was also open //
	if ( isSubOpen )
	{
		// .sub file is open, so close it too //
		if ( !sub.Close() ) return false;
	}

	// disk image is no longer open since it is being closed
	isDiskOpen = false;
	isSubOpen = false;
	
	//return false;
	return true;
}


// returns false if seek failed
// time values are just regular numbers, NOT bcd
// use for seeking via Min,Sec,Frac
bool CDImage::SeekTime ( s32 Minutes, s32 Seconds, s32 Sector )
{
	int i;
	u32 SectorNumber;
	
	// you can't read data from the disk if you are already reading
	// so we'll just wait until it is done reading
	// important note: should make sure all reads are complete before doing seek, since Read/Write indexes could possibly get overwritted otherwise
	WaitForAllReadsComplete ();

	// get what should be the offset into the sector on the disk
	//SectorOffset = ( Minutes * c_SectorsPerMinute ) + ( Seconds * c_SectorsPerSecond ) + Sector;
	SectorNumber = GetSectorNumber ( Minutes, Seconds, Sector );
	
	// this should no longer chop off the first two seconds
	/*
	// CD disk images start at 2 seconds, though, regardless of whether it is img or bin file
	SectorOffset -= c_SectorsInFirstTwoSeconds;

	// seek to anything under two seconds and it fails
	if ( SectorOffset < 0 ) return false;
	*/

	// reset read and write index
	//ReadIndex = -1;
	//WriteIndex = 0;
	// *** testing ***
	isReadingFirstSector = true;
	Next_ReadIndex = WriteIndex;
	Next_CurrentSector = SectorNumber;
	
	// the next sector to read needs to be available to multiple threads from here
	//NextSector = SectorOffset;
	NextSector = SectorNumber;
	
	// get the sector at which the next track starts at (used for auto pause, etc)
	i = GetIndexData_Index ( SectorNumber );
	
	// get current track
	CurrentTrack = IndexData [ i ].Track;
	
	// only want to know that the track is not equal to the current one
	if ( IndexData [ i + 1 ].Track != CurrentTrack )
	{
		NextTrack = IndexData [ i + 1 ].Track;
		NextTrack_Sector = IndexData [ i + 1 ].SectorNumber;
	}
	else
	{
		NextTrack = IndexData [ i + 2 ].Track;
		NextTrack_Sector = IndexData [ i + 2 ].SectorNumber;
	}

	
	// also set the same for the current sector and read index
	ReadIndex = Next_ReadIndex;
	CurrentSector = Next_CurrentSector;
	
	// is now at the new location after performing the seek, so must update disk location immediately //
	
	// set the min, sec, frac
	SplitSectorNumber ( CurrentSector, Current_AMin, Current_ASec, Current_AFrac );
	
	// update the simulated subq data
	UpdateSubQ_Data ();
	
	
	return true;
}


// use for seeking by sector number
void CDImage::SeekSector ( u64 Sector )
{
	u32 SectorOffset, SectorNumber, i;
	
	// you can't read data from the disk if you are already reading
	// so we'll just wait until it is done reading
	// important note: should make sure all reads are complete before doing seek, since Read/Write indexes could possibly get overwritted otherwise
	WaitForAllReadsComplete ();
	
	SectorOffset = Sector;
	SectorNumber = Sector;
	
	// reset read and write index
	//ReadIndex = -1;
	//WriteIndex = 0;
	
	// the next sector to read needs to be available to multiple threads from here
	NextSector = SectorOffset;
	//Lock_Exchange64 ( (long long&) NextSector, SectorOffset );
	
	// *** testing ***
	isReadingFirstSector = true;
	Next_ReadIndex = WriteIndex;
	Next_CurrentSector = SectorNumber;
	
	// the next sector to read needs to be available to multiple threads from here
	//NextSector = SectorOffset;
	//NextSector = SectorNumber;
	
	// get the sector at which the next track starts at (used for auto pause, etc)
	i = GetIndexData_Index ( SectorNumber );
	
	// get current track
	CurrentTrack = IndexData [ i ].Track;
	
	// only want to know that the track is not equal to the current one
	if ( IndexData [ i + 1 ].Track != CurrentTrack )
	{
		NextTrack = IndexData [ i + 1 ].Track;
		NextTrack_Sector = IndexData [ i + 1 ].SectorNumber;
	}
	else
	{
		NextTrack = IndexData [ i + 2 ].Track;
		NextTrack_Sector = IndexData [ i + 2 ].SectorNumber;
	}

	
	// also set the same for the current sector and read index
	ReadIndex = Next_ReadIndex;
	CurrentSector = Next_CurrentSector;
	
	// is now at the new location after performing the seek, so must update disk location immediately //
	
	// set the min, sec, frac
	SplitSectorNumber ( CurrentSector, Current_AMin, Current_ASec, Current_AFrac );
	
	// update the simulated subq data
	UpdateSubQ_Data ();
	
	
	//return true;
	//fseeko64 ( image, ( Sector ) * c_SectorSize, SEEK_SET );
	//fseek ( image, ( Sector ) * c_SectorSize, SEEK_SET );
	//image.Seek ( ( Sector ) * c_SectorSize );
	
	// reset read and write index
	// don't set next sector or read/write index here since this function is used by object
	//ReadIndex = 0;
	//WriteIndex = 0;
}

/*
int CDImage::ReadData ( u8* Data )
{
}
*/

// only call this after doing a seek first
void CDImage::StartReading ()
{
	//cout << "\nCalled: CDImage::StartReading";

	// just pre-read some sectors
	//NumberOfSectorsToWrite = c_SectorReadCount;
	
	// you can't read data from the disk if you are already reading
	// so we'll just wait until it is done reading
	WaitForAllReadsComplete ();
	
	/*
	while ( isReadInProgress )
	{
		//cout << "\ncdimage: Waiting for read to finish1\n";
		WindowClass::DoEvents ();
	}
	
	// check if .sub file is open //
	if ( isSubOpen )
	{
		// .sub file was open, so wait until read is done from there too //
		while ( isSubReadInProgress )
		{
			WindowClass::DoEvents ();
		}
	}
	*/
	
	// read is in progress
	isReadInProgress = true;
	//Lock_Exchange32 ( (long&)isReadInProgress, true );
	
	// if .sub file is open, then will be reading from it too //
	if ( isSubOpen )
	{
		//Lock_Exchange32 ( (long&)isSubReadInProgress, true );
		isSubReadInProgress = true;
	}
	
	// seek to sector?? or I think this happens asynchronously??
	
	// schedule read to happen asynchronously and return the pointer into the buffer for reading data
	// schedule read to happen asynchronously
	// this has to be called on the gui thread since it does the alertable wait stait thing it needs for asynchronous reading
	// I would use fread for portability, but this is not working properly for some reason in MinGW64
	// *** TODO ***
	WindowClass::Window::RemoteCall ( (WindowClass::Window::RemoteFunction) _RemoteCall_ReadAsync, (void*) NULL, false );
	
	// *** TESTING ***
	WaitForAllReadsComplete ();
	
	// didn't read the data yet, but did read the subchannel info
	if ( isSubOpen )
	{
		//CurrentSubBuffer = & ( SubBuffer [ ( /*ReadIndex*/ 0 & c_BufferSectorMask ) * c_SubChannelSizePerSector ] );
		CurrentSubBuffer = & ( SubBuffer [ ( Next_ReadIndex & c_BufferSectorMask ) * c_SubChannelSizePerSector ] );
		
		//cout << "\n\nStart: AMin=" << hex << (u32)((CDImage::Sector::SubQ*)CurrentSubBuffer)->AbsoluteAddress [ 0 ] << " ASec=" << (u32)((CDImage::Sector::SubQ*)CurrentSubBuffer)->AbsoluteAddress [ 1 ] << " AFrac=" << (u32)((CDImage::Sector::SubQ*)CurrentSubBuffer)->AbsoluteAddress [ 2 ];
		//cout << "\nMin=" << hex << (u32)((CDImage::Sector::SubQ*)CurrentSubBuffer)->TrackRelativeAddress [ 0 ] << " Sec=" << (u32)((CDImage::Sector::SubQ*)CurrentSubBuffer)->TrackRelativeAddress [ 1 ] << " Frac=" << (u32)((CDImage::Sector::SubQ*)CurrentSubBuffer)->TrackRelativeAddress [ 2 ];
	}
	
	//cout << "\nExiting: CDImage::StartReading";
}


// returns a pointer to the buffer with sector data that will be read
// must call isSectorReadComplete () to make sure the data is ready before reading
u8* CDImage::ReadNextSector ()
{
	u8* BufferToReadDataFrom;
	
	// make sure that at least the first block has been read before proceeding
	//while ( WriteIndex < c_SectorReadCount )
	//WaitForAllReadsComplete ();
	
	/*
	while ( isReadInProgress )
	{
		WindowClass::DoEvents ();
	}
	
	if ( isSubOpen )
	{
		// .sub file was open, so wait until read is done from there too //
		while ( isSubReadInProgress )
		{
			//cout << "\ncdimage: Waiting for sub read to finish...\n";
			WindowClass::DoEvents ();
		}
	}
	*/
	
	
	if ( isReadingFirstSector )
	{
		// jump to where the first sector should be read from
		ReadIndex = Next_ReadIndex;
		
		// set the current sector
		CurrentSector = Next_CurrentSector;
		
		// no longer reading the first sector
		isReadingFirstSector = false;
	}
	else
	{
		// update read index first thing
		ReadIndex++;
		
		// update the current sector number
		CurrentSector++;
	}
	
	// set the min, sec, frac
	SplitSectorNumber ( CurrentSector, Current_AMin, Current_ASec, Current_AFrac );
	
	// update the simulated subq data
	UpdateSubQ_Data ();
	
	//if ( ReadIndex & c_SectorReadCountMask )
	if ( ReadIndex < ( WriteIndex - c_SectorReadCount ) )
	{
		// get buffer to read sector data from when it becomes ready
		//BufferToReadDataFrom = & ( Buffer [ ( ReadIndex & c_BufferSectorMask ) * c_SectorSize ] );
		BufferToReadDataFrom = & ( Buffer [ ( ReadIndex & c_BufferSectorMask ) * SectorSize ] );

		// also need buffer for subchannel data if .sub file is open //
		if ( isSubOpen )
		{
			CurrentSubBuffer = & ( SubBuffer [ ( ReadIndex & c_BufferSectorMask ) * c_SubChannelSizePerSector ] );
		}
		
		// return pointer into buffer for reading data
		// but since read is asynchronous, must call 
		return BufferToReadDataFrom;
		//return ReadIndex;
	}
	
	///////////////////////////////////////////////////////////////////
	// Time to load in the other half of read buffer
	
	// you can't read data from the file if you are already reading from it
	// so we'll just wait until it is done reading from the file
	WaitForAllReadsComplete ();
	
	/*
	while ( isReadInProgress )
	{
		//cout << "\ncdimage: Waiting for read to finish...\n";
		WindowClass::DoEvents ();
	}
	
	// check if .sub file is open //
	if ( isSubOpen )
	{
		// .sub file was open, so wait until read is done from there too //
		while ( isSubReadInProgress )
		{
			//cout << "\ncdimage: Waiting for sub read to finish...\n";
			WindowClass::DoEvents ();
		}
	}
	*/
	
	
	// just pre-read some sectors
	//NumberOfSectorsToWrite = c_SectorReadCount;
	
	// read is in progress - do this before starting the read
	isReadInProgress = true;
	//Lock_Exchange32 ( (long&)isReadInProgress, true );
	
	// if .sub file is open, then will be reading from it too //
	if ( isSubOpen )
	{
		//Lock_Exchange32 ( (long&)isSubReadInProgress, true );
		isSubReadInProgress = true;
	}
	
	// seek to sector?? or I think this happens asynchronously??
	
	// schedule read to happen asynchronously and return the pointer into the buffer for reading data
	// schedule read to happen asynchronously
	// this has to be called on the gui thread
	// *** TODO ***
	WindowClass::Window::RemoteCall ( (WindowClass::Window::RemoteFunction) _RemoteCall_ReadAsync, (void*) NULL, false );
	
	// *** TESTING ***
	//WaitForAllReadsComplete ();
	
	// get buffer to read sector data from when it becomes ready
	//BufferToReadDataFrom = & ( Buffer [ ( ReadIndex & c_BufferSectorMask ) * c_SectorSize ] );
	BufferToReadDataFrom = & ( Buffer [ ( ReadIndex & c_BufferSectorMask ) * SectorSize ] );
	
	// if .sub file is open, then will need buffer to read sub data from //
	if ( isSubOpen )
	{
		CurrentSubBuffer = & ( SubBuffer [ ( ReadIndex & c_BufferSectorMask ) * c_SubChannelSizePerSector ] );
	}

	// update read index
	// no, this should be done first thing
	//Lock_ExchangeAdd64 ( (long long&)ReadIndex, 1 );
	//ReadIndex++;
		
	
	// return pointer into buffer for reading data
	// but since read is asynchronous, must call 
	return BufferToReadDataFrom;
	//return ReadIndex;
}


// marks the current sector as being the first sector
void CDImage::SetFirstSector ()
{
	Next_ReadIndex = ReadIndex;
	
	// also set the next current sector number
	Next_CurrentSector = CurrentSector;
	
	isReadingFirstSector = true;
}


u64 CDImage::GetCurrentBufferIndex ()
{
	return ReadIndex;
}

u8* CDImage::GetDataBuffer ( u64 Index )
{
	//return (& ( Buffer [ ( Index & c_BufferSectorMask ) * c_SectorSize ] ));
	return (& ( Buffer [ ( Index & c_BufferSectorMask ) * SectorSize ] ));
}

u8* CDImage::GetCurrentDataBuffer ()
{
	//return (& ( Buffer [ ( ReadIndex & c_BufferSectorMask ) * c_SectorSize ] ));
	return (& ( Buffer [ ( ReadIndex & c_BufferSectorMask ) * SectorSize ] ));
}

u8* CDImage::GetCurrentSubBuffer ()
{
	if ( isReadingFirstSector )
	{
		return (& ( SubBuffer [ ( Next_ReadIndex & c_BufferSectorMask ) * c_SubChannelSizePerSector ] ));
	}
	else
	{
		return (& ( SubBuffer [ ( ReadIndex & c_BufferSectorMask ) * c_SubChannelSizePerSector ] ));
	}
	
	return (& ( SubBuffer [ ( ReadIndex & c_BufferSectorMask ) * c_SubChannelSizePerSector ] ));
}


static u32 CDImage::ConvertBCDToDec ( u8 BCD )
{
	//if ( BCD > 0x99 ) BCD -= 0x80;
	return ( BCD & 0xf ) + ( ( BCD >> 4 ) * 10 );
}


static u8 CDImage::ConvertDecToBCD8 ( u32 Dec )
{
	return ( Dec % 10 ) | ( ( ( Dec / 10 ) % 10 ) << 4 );
}


bool CDImage::_RemoteCall_OpenDiskImage ( string FullImagePath )
{
	// this will be called from the PS1 thread now, so it does not have to be called on another thread - can return whether successful or not
	
	//cout << "\nCalled CDImage::_RemoteCall_OpenDiskImage";
	
	bool bRet;
	
	string TempString;
	string FileName, Extension, Path;
	
	// first load the image //
	
	// get just the file name without path or extension
	Path = GetPath ( FullImagePath );
	FileName = GetFile ( FullImagePath );
	Extension = GetExtension ( FullImagePath );
	
	//cout << "\nDEBUG: Path= '" << Path.c_str() << "'";
	cout << "\nDEBUG: FileName= '" << FileName.c_str() << "'";
	cout << "\nDEBUG: Extension= '" << Extension.c_str () << "'";
	
#ifdef DISK_READ_SYNC
	bRet = _DISKIMAGE->image.CreateSync ( Path + FileName + ".bin" );
#else
	bRet = _DISKIMAGE->image.CreateAsync ( Path + FileName + ".bin" );
#endif

	// try each image type that is supported one by one (.bin,.img,.iso)
	if ( bRet )
	{
		cout << "\nINFO: Found image file. Image is in .bin format which is supported.";
	}
	else
	{
#ifdef DISK_READ_SYNC
		bRet = _DISKIMAGE->image.CreateSync ( Path + FileName + ".img" );
#else
		bRet = _DISKIMAGE->image.CreateAsync ( Path + FileName + ".img" );
#endif

		if ( bRet )
		{
			cout << "\nINFO: Found image file. Image is in .img format which is supported.";
		}
		else
		{
#ifdef DISK_READ_SYNC
			bRet = _DISKIMAGE->image.CreateSync ( Path + FileName + ".iso" );
#else
			bRet = _DISKIMAGE->image.CreateAsync ( Path + FileName + ".iso" );
#endif

			if ( bRet )
			{
				cout << "\nINFO: Found image file. Image is in .iso format which is supported.";
			}
			else
			{
				cout << "\nERROR: Unable to open cd image file. Only .bin, .img, .iso image formats are supported.";
				return false;
			}
		}
	}
	
	/*
	if ( !_DISKIMAGE->image.CreateAsync ( _DISKIMAGE->ImagePath ) ) 
	{
		// disk was not opened
		cout << "\nProblem opening disk; Path=" << _DISKIMAGE->ImagePath;
		return false;
	}
	*/
	
	// sub file starts out as not opened
	isSubOpen = false;

	// disable sub files for now //
#ifdef ENABLE_SUB_FILES

#ifdef DISK_READ_SYNC	
	bRet = _DISKIMAGE->sub.CreateSync ( Path + FileName + ".sub" );
#else
	bRet = _DISKIMAGE->sub.CreateAsync ( Path + FileName + ".sub" );
#endif
	
	// now look for a .sub file //
	if ( !bRet )
	{
		// .sub file was not opened
		cout << "\nERROR: Problem opening .sub file.";
		isSubOpen = false;
	}
	else
	{
		// .sub image was opened successfully
		cout << "\nINFO: .sub file opened successfully.";
		isSubOpen = true;
		_DISKIMAGE->isSubReadInProgress = false;
	}

#endif

	/*
	if ( !_DISKIMAGE->sub.CreateAsync ( _DISKIMAGE->SubPath ) ) 
	{
		// .sub file was not opened
		cout << "\nCDImage::_RemoteCall_OpenDiskImage; Problem opening .sub file.";
		isSubOpen = false;
	}
	else
	{
		// .sub image was opened successfully
		cout << "\n.sub file opened successfully.";
		isSubOpen = true;
		_DISKIMAGE->isSubReadInProgress = false;
	}
	*/
	
	// set track zero to zero
	TrackData [ 0 ].Min = 0;
	TrackData [ 0 ].Sec = 0;
	TrackData [ 0 ].Frac = 0;
	
	// put in the new stuff too //
	IndexData [ 0 ].Track = 0xff;
	IndexData [ 0 ].Index = 1;
	
	IndexData [ 0 ].Min = 0;
	IndexData [ 0 ].Sec = 0;
	IndexData [ 0 ].Frac = 0;
	
	IndexData [ 0 ].AMin = 0;
	IndexData [ 0 ].ASec = 0;
	IndexData [ 0 ].AFrac = 0;
	
	IndexData [ 0 ].SectorNumber = 0;
	IndexData [ 0 ].SectorNumber_InImage = -1;
	
	// initialize pregap
	iPreGap = 0;
	
	// also check for .cue or .ccd file //
	
	if ( ParseCueSheet ( Path + FileName + ".cue" ) )
	{
		cout << "\nNOTE: Found .cue file for disk image. Parsed successfully.";
	}
	else if ( ParseCcdSheet ( Path + FileName + ".ccd" ) )
	{
		cout << "\nNOTE: Found .ccd file for disk image. Parsed successfully.";
	}
	else
	{
		cout << "\nERROR: Unable to find .cue or .ccd file for disk image.";
		cout << "\nERROR: .cue or .ccd file should be in the same folder as the disk image.";
		cout << "\nWARNING: continuing without use of .cue or .ccd file.";
		
		// since there was an error, assume a single data track
		// single track starts at two seconds
		iNumberOfTracks = 1;
		TrackData [ 1 ].Min = 0;
		TrackData [ 1 ].Sec = 2;
		TrackData [ 1 ].Frac = 0;
		
		// put in the new stuff
		iNumberOfIndexes = 2;
		
		IndexData [ 1 ].Track = 1;
		IndexData [ 1 ].Index = 1;
		
		IndexData [ 1 ].Min = 0;
		IndexData [ 1 ].Sec = 0;
		IndexData [ 1 ].Frac = 0;
		
		IndexData [ 1 ].AMin = 0;
		IndexData [ 1 ].ASec = 2;
		IndexData [ 1 ].AFrac = 0;
		
		IndexData [ 1 ].SectorNumber = 150;
		IndexData [ 1 ].SectorNumber_InImage = 0;
	}
	
	if ( iNumberOfIndexes < 2 )
	{
		// since there was an error, assume a single data track
		// single track starts at two seconds
		iNumberOfTracks = 1;
		TrackData [ 1 ].Min = 0;
		TrackData [ 1 ].Sec = 2;
		TrackData [ 1 ].Frac = 0;
		
		// put in the new stuff
		iNumberOfIndexes = 2;
		
		IndexData [ 1 ].Track = 1;
		IndexData [ 1 ].Index = 1;
		
		IndexData [ 1 ].Min = 0;
		IndexData [ 1 ].Sec = 0;
		IndexData [ 1 ].Frac = 0;
		
		IndexData [ 1 ].AMin = 0;
		IndexData [ 1 ].ASec = 2;
		IndexData [ 1 ].AFrac = 0;
		
		IndexData [ 1 ].SectorNumber = 150;
		IndexData [ 1 ].SectorNumber_InImage = 0;
	}
	
	// get size of file to get end of last track
	SizeOfImage = _DISKIMAGE->image.Size ();
	//SectorsInImage = SizeOfImage / c_SectorSize;
	SectorsInImage = SizeOfImage / SectorSize;
	
	// add in the two seconds of sectors at the beginning = 150 sectors
	SectorsInImage += c_SectorsInFirstTwoSeconds;
	
	// also add in any pregap time
	SectorsInImage += ( iPreGap * 75 );
	
	// get the total length of the disk
	SplitSectorNumber ( SectorsInImage, _DISKIMAGE->TrackData [ iNumberOfTracks + 1 ].Min, _DISKIMAGE->TrackData [ iNumberOfTracks + 1 ].Sec, _DISKIMAGE->TrackData [ iNumberOfTracks + 1 ].Frac );
	
	// also put this into the new index data
	SplitSectorNumber ( SectorsInImage, _DISKIMAGE->IndexData [ iNumberOfIndexes ].AMin, _DISKIMAGE->IndexData [ iNumberOfIndexes ].ASec, _DISKIMAGE->IndexData [ iNumberOfIndexes ].AFrac );
	
	// also set the number of sectors in disk (can be used later for a number of things)
	_DISKIMAGE->IndexData [ iNumberOfIndexes ].SectorNumber = SectorsInImage;
	_DISKIMAGE->IndexData [ iNumberOfIndexes ].SectorNumber_InImage = -1;
	
	// set track number for lead out area
	_DISKIMAGE->IndexData [ iNumberOfIndexes ].Track = 0xaa;
	
	// save the last sector number
	LastSector_Number = SectorsInImage;
	
	cout << "\nNOTE: Size of disk image is: " << dec << SizeOfImage;
	cout << "\nNOTE: End of last track is at: Min=" << dec << (u32)_DISKIMAGE->TrackData [ iNumberOfTracks + 1 ].Min << " Sec=" << (u32)_DISKIMAGE->TrackData [ iNumberOfTracks + 1 ].Sec << " Frac=" << (u32)_DISKIMAGE->TrackData [ iNumberOfTracks + 1 ].Frac;
	
	// disk is now open
	_DISKIMAGE->isDiskOpen = true;
	//Lock_Exchange32 ( (long&)_DISKIMAGE->isDiskOpen, true );
	
	// alert other thread that read is done and disk is open
	//Lock_Exchange32 ( (long&) _DISKIMAGE->isReadInProgress, false );
	_DISKIMAGE->isReadInProgress = false;
	
	//cout << "\nExiting CDImage::_RemoteCall_OpenDiskImage; Path=" << _DISKIMAGE->ImagePath;
	
	// disk was opened successfully
	return true;
}


static void CDImage::_RemoteCall_ReadAsync ()
{
	//cout << "\nCalled CDImage::_RemoteCall_ReadAsync";
	
	int Ret;
	u32 SectorNumber;
	
	// make sure disk is not already reading
	// no, don't do this since it would freeze
	//while ( _DISKIMAGE->isReadInProgress );
	
#ifdef DISK_READ_SYNC

	SectorNumber = _DISKIMAGE->GetSectorNumber_InImage ( _DISKIMAGE->NextSector );

	if ( SectorNumber == -1 )
	{
		// zero sector
		//for ( int i = 0; i < ( c_SectorSize * c_SectorReadCount ); i++ )
		for ( int i = 0; i < ( _DISKIMAGE->SectorSize * c_SectorReadCount ); i++ )
		{
			//_DISKIMAGE->Buffer [ i + ( ( _DISKIMAGE->WriteIndex & c_BufferSectorMask ) * c_SectorSize ) ] = 0;
			_DISKIMAGE->Buffer [ i + ( ( _DISKIMAGE->WriteIndex & c_BufferSectorMask ) * _DISKIMAGE->SectorSize ) ] = 0;
		}
	}
	else
	{
		// load sector from image
		//Ret = _DISKIMAGE->image.ReadSync ( & ( _DISKIMAGE->Buffer [ ( _DISKIMAGE->WriteIndex & c_BufferSectorMask ) * c_SectorSize ] ), c_SectorSize * c_SectorReadCount, _DISKIMAGE->NextSector * c_SectorSize );
		//Ret = _DISKIMAGE->image.ReadSync ( & ( _DISKIMAGE->Buffer [ ( _DISKIMAGE->WriteIndex & c_BufferSectorMask ) * c_SectorSize ] ), c_SectorSize * c_SectorReadCount, SectorNumber * c_SectorSize );
		Ret = _DISKIMAGE->image.ReadSync ( & ( _DISKIMAGE->Buffer [ ( _DISKIMAGE->WriteIndex & c_BufferSectorMask ) * _DISKIMAGE->SectorSize ] ), _DISKIMAGE->SectorSize * c_SectorReadCount, SectorNumber * _DISKIMAGE->SectorSize );
	}
	
	// if .sub file is open, then also read subchannel data //
	if ( isSubOpen )
	{
		//Ret = _DISKIMAGE->sub.ReadSync ( & ( _DISKIMAGE->SubBuffer [ ( _DISKIMAGE->WriteIndex & c_BufferSectorMask ) * c_SubChannelSizePerSector ] ), c_SubChannelSizePerSector * c_SectorReadCount, _DISKIMAGE->NextSector * c_SubChannelSizePerSector );
		Ret = _DISKIMAGE->sub.ReadSync ( & ( _DISKIMAGE->SubBuffer [ ( _DISKIMAGE->WriteIndex & c_BufferSectorMask ) * c_SubChannelSizePerSector ] ), c_SubChannelSizePerSector * c_SectorReadCount, SectorNumber * c_SubChannelSizePerSector );
	}
	
	// update write index
	//Lock_ExchangeAdd64 ( (long long&)_DISKIMAGE->WriteIndex, c_SectorReadCount );
	_DISKIMAGE->WriteIndex += c_SectorReadCount;
	
	// disk is done reading
	//isReadInProgress = false;
	//Lock_Exchange32 ( (long&)_DISKIMAGE->isReadInProgress, false );
	_DISKIMAGE->isReadInProgress = false;
	
	// disk is done reading
	//Lock_Exchange32 ( (long&)_DISKIMAGE->isSubReadInProgress, false );
	_DISKIMAGE->isSubReadInProgress = false;
	
#else

	//_DISKIMAGE->image.ReadAsync ( _params.DataOut, _params.BytesToRead, _params.SeekPosition, _params.Callback_Function );
	//Ret = _DISKIMAGE->image.ReadAsync ( & ( _DISKIMAGE->Buffer [ ( _DISKIMAGE->WriteIndex & c_BufferSectorMask ) * c_SectorSize ] ), c_SectorSize * c_SectorReadCount, _DISKIMAGE->NextSector * c_SectorSize, DiskRead_Callback );
	Ret = _DISKIMAGE->image.ReadAsync ( & ( _DISKIMAGE->Buffer [ ( _DISKIMAGE->WriteIndex & c_BufferSectorMask ) * _DISKIMAGE->SectorSize ] ), _DISKIMAGE->SectorSize * c_SectorReadCount, _DISKIMAGE->NextSector * _DISKIMAGE->SectorSize, DiskRead_Callback );
	
	// if .sub file is open, then also read subchannel data //
	if ( isSubOpen )
	{
		Ret = _DISKIMAGE->sub.ReadAsync ( & ( _DISKIMAGE->SubBuffer [ ( _DISKIMAGE->WriteIndex & c_BufferSectorMask ) * c_SubChannelSizePerSector ] ), c_SubChannelSizePerSector * c_SectorReadCount, _DISKIMAGE->NextSector * c_SubChannelSizePerSector, SubRead_Callback );
	}
	
#endif

	// can update the next sector here
	// value no longer needs to be made available to the calling thread
	//Lock_ExchangeAdd64 ( (long long&)_DISKIMAGE->NextSector, c_SectorReadCount );
	_DISKIMAGE->NextSector += c_SectorReadCount;
	
	//cout << "\nExiting CDImage::_RemoteCall_ReadAsync; ReadAsync=" << Ret << "; LastError=" << GetLastError ();
}


static void CDImage::DiskRead_Callback ()
{
	//cout << "\nCalled CDImage::DiskRead_Callback";
	
	// update the sector offset for next sectors to be read
	// this is now done right after sending the asynchronous read
	//SectorOffset += _DISKIMAGE->NumberOfSectorsToWrite;
	//Lock_ExchangeAdd64 ( (long long&)_DISKIMAGE->NextSector, _DISKIMAGE->NumberOfSectorsToWrite );
	
	//asm volatile ( "sfence" );
	
	// output debug info
	//cout << "\nSector Read: " << dec << _DISKIMAGE->NextSector << "; debug data: " << hex << (u32) _DISKIMAGE->Buffer [ 2352 + 24 + 0 ] << " " << (u32) _DISKIMAGE->Buffer [ 2352 + 24 + 1 ] << " ";
	//cout << (u32) _DISKIMAGE->Buffer [ 2352 + 24 + 2 ] << " " << (u32) _DISKIMAGE->Buffer [ 2352 + 24 + 3 ] << " " << (u32) _DISKIMAGE->Buffer [ 2352 + 24 + 4 ] << " " << (u32) _DISKIMAGE->Buffer [ 2352 + 24 + 5 ];
	
	// update write index
	//Lock_ExchangeAdd64 ( (long long&)_DISKIMAGE->WriteIndex, c_SectorReadCount );
	_DISKIMAGE->WriteIndex += c_SectorReadCount;
	
	// disk is done reading
	//isReadInProgress = false;
	//Lock_Exchange32 ( (long&)_DISKIMAGE->isReadInProgress, false );
	_DISKIMAGE->isReadInProgress = false;
	
	//cout << "\nExiting CDImage::DiskRead_Callback";
}


static void CDImage::SubRead_Callback ()
{
	// update write index
	//Lock_ExchangeAdd64 ( (long long&)_DISKIMAGE->WriteIndex, c_SectorReadCount );
	
	// disk is done reading
	//Lock_Exchange32 ( (long&)_DISKIMAGE->isSubReadInProgress, false );
	_DISKIMAGE->isSubReadInProgress = false;
	
	//cout << "\nExiting CDImage::DiskRead_Callback";
}

void CDImage::WaitForSectorReadComplete ()
{
	while ( isReadInProgress )
	{
		//cout << "\ncdimage: Waiting for read to finish...\n";
		WindowClass::DoEvents ();
	}
	
	// check if .sub file is open //
	if ( isSubOpen )
	{
		// .sub file was open, so wait until read is done from there too //
		while ( isSubReadInProgress )
		{
			//cout << "\ncdimage: Waiting for sub read to finish...\n";
			WindowClass::DoEvents ();
		}
	}
	
	while ( ReadIndex >= WriteIndex )
	{
		WindowClass::DoEvents ();
		//cout << "\nWaiting for sector read complete...\n";
	}
}


void CDImage::WaitForAllReadsComplete ()
{
	while ( isReadInProgress )
	{
		//cout << "\ncdimage: Waiting for read to finish...\n";
		WindowClass::DoEvents ();
	}
	
	// check if .sub file is open //
	if ( isSubOpen )
	{
		// .sub file was open, so wait until read is done from there too //
		while ( isSubReadInProgress )
		{
			//cout << "\ncdimage: Waiting for sub read to finish...\n";
			WindowClass::DoEvents ();
		}
	}
}


static unsigned long CDImage::GetSectorNumber ( u32 Min, u32 Sec, u32 Frac )
{
	return ( Min * c_SectorsPerMinute ) + ( Sec * c_SectorsPerSecond ) + Frac;
}

static void CDImage::SplitSectorNumber ( unsigned long SectorNumber, u8& Min, u8& Sec, u8& Frac )
{
	// get min
	//iMin = CLng ( Split ( Line, ":" ) [ 0 ] );
	Min = SectorNumber / c_SectorsPerMinute;
	
	// get sec
	//iSec = CLng ( Split ( Line, ":" ) [ 1 ] );
	Sec = ( SectorNumber % c_SectorsPerMinute ) / c_SectorsPerSecond;
	
	// get frac
	//iFrac = CLng ( Split ( Line, ":" ) [ 2 ] );
	Frac = SectorNumber % c_SectorsPerSecond;
}


void CDImage::Output_IndexData ()
{
	cout << "\nIndex Output:";
	
	for ( int i = 0; i < iNumberOfIndexes; i++ )
	{
		cout << "\nEntry#" << dec << i;
		cout << " Track=" << (u32) IndexData [ i ].Track << " Index=" << (u32) IndexData [ i ].Index;
		cout << " AMin=" << (u32) IndexData [ i ].AMin << " ASec=" << (u32) IndexData [ i ].ASec << " AFrac=" << (u32) IndexData [ i ].AFrac;
		cout << " Min=" << (u32) IndexData [ i ].Min << " Sec=" << (u32) IndexData [ i ].Sec << " Frac=" << (u32) IndexData [ i ].Frac;
		cout << " SectorNumber=" << (u32) IndexData [ i ].SectorNumber << " SectorNumber_InImage=" << (u32) IndexData [ i ].SectorNumber_InImage;
	}
	
	// also output the end of the disk
	cout << "\n\nEnd of disk: " << " AMin=" << (u32) IndexData [ iNumberOfIndexes ].AMin << " ASec=" << (u32) IndexData [ iNumberOfIndexes ].ASec << " AFrac=" << (u32) IndexData [ iNumberOfIndexes ].AFrac;
}



void CDImage::Output_SubQData ( u32 AMin, u32 ASec, u32 AFrac )
{
	UpdateSubQ_Data ( AMin, ASec, AFrac );
	
	cout << "\n\nSubQ Data for AMin=" << dec << AMin << " ASec=" << ASec << " AFrac=" << AFrac;
	cout << "\nTrack=" << (u32)SubQ_Track << " Index=" << (u32)SubQ_Index << " Min=" << (u32)SubQ_Min << " Sec=" << (u32)SubQ_Sec << " Frac=" << (u32)SubQ_Frac;
	cout << "\nSectorNumber=" << GetSectorNumber ( AMin, ASec, AFrac ) << " SectorNumber_InImage=" << GetSectorNumber_InImage ( AMin, ASec, AFrac );
}


// probably returns -1 on error
u32 CDImage::GetIndexData_Index ( u32 SectorNumber )
{
	int i;
	
	// looking backwards keeps it simple
	for ( i = iNumberOfIndexes; i >= 0; i-- )
	{
		if ( SectorNumber >= IndexData [ i ].SectorNumber ) break;
	}
	
	return i;
}


u32 CDImage::GetSectorNumber_InImage ( u32 AMin, u32 ASec, u32 AFrac )
{
	return GetSectorNumber_InImage( GetSectorNumber ( AMin, ASec, AFrac ) );
}


u32 CDImage::GetSectorNumber_InImage ( u32 SectorNumber )
{
	int i;
	u32 SectorOffset;
	
	//SectorNumber = GetSectorNumber ( AMin, ASec, AFrac );
	i = GetIndexData_Index ( SectorNumber );
	
	SectorOffset = SectorNumber - IndexData [ i ].SectorNumber;
	
	// if sector is not in image file, then return -1
	if ( IndexData [ i ].SectorNumber_InImage == -1 ) return -1;
	
	return ( IndexData [ i ].SectorNumber_InImage + SectorOffset );
}



void CDImage::UpdateSubQ_Data ( u32 AMin, u32 ASec, u32 AFrac )
{
	UpdateSubQ_Data ( GetSectorNumber ( AMin, ASec, AFrac ) );
}



void CDImage::UpdateSubQ_Data ()
{
	UpdateSubQ_Data ( CurrentSector );
}




void CDImage::UpdateSubQ_Data ( u32 SectorNumber )
{
	u32 SectorNumber_Relative, SectorNumber_Start_Relative;
	int i;
	
	i = GetIndexData_Index ( SectorNumber );
	
	// looking backwards keeps it simple
	//for ( int i = iNumberOfSectors; i >= 0; i-- )
	//{
		//if ( SectorNumber >= IndexData [ i ].SectorNumber )
		//{
			// found start of the index we are looking for
			
			// set index/track
			SubQ_Index = IndexData [ i ].Index;
			SubQ_Track = IndexData [ i ].Track;
			
			// set absolute time
			//SubQ_AMin = AMin;
			//SubQ_ASec = ASec;
			//SubQ_AFrac = AFrac;
			SplitSectorNumber ( SectorNumber, SubQ_AMin, SubQ_ASec, SubQ_AFrac );
			
			// set relative time //
			
			// check if index 0
			if ( !SubQ_Index )
			{
				// count down to index 1 //
				
				SectorNumber_Start_Relative = 150;
				
				// get sector distance from start of sector
				SectorNumber_Relative = SectorNumber - IndexData [ i ].SectorNumber;
				
				// get relative sector number for index 0
				SectorNumber_Relative = SectorNumber_Start_Relative - SectorNumber_Relative;
			}
			else
			{
				SectorNumber_Start_Relative = 0;
				
				// get sector distance from start of sector
				SectorNumber_Relative = SectorNumber - IndexData [ i ].SectorNumber;
			}
			
			// set the relative time
			SplitSectorNumber ( SectorNumber_Relative, SubQ_Min, SubQ_Sec, SubQ_Frac );
			
			// done
			//return;
		//}
	//}
}





