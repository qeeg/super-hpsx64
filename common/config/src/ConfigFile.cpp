
#include "ConfigFile.h"
#include "StringUtils.h"
#include <cstring>

using namespace Config;
using namespace std;
using namespace Utilities::Strings;



static const char* File::c_sDelimiter = "\n";
static const char* File::c_sAssigner = "=";
static const char* File::c_sVarPrefix = "[";
static const char* File::c_sVarPostfix = "]";


File::File ()
{
	Clear ();
}


void File::Clear ()
{
	memset ( this, 0, sizeof( Config::File ) );
	cout << "\nClearing: Config file size=" << strlen( cData );
	cout << "\nClearing: size of Config::File=" << sizeof( Config::File );
}


bool File::Load ( string NameOfFile )
{
	int size, result;
	ifstream *fFile;
	//FILE* fFile;
	
	//fFile = fopen ( NameOfFile.c_str(), "rb" );
	
	//if ( !fFile ) return false;
	
	fFile = new ifstream( NameOfFile.c_str()/*, ios::binary*/ );
	//result = fread ( cData, 1, size /*c_iConfigFile_MaxSize*/, fFile );
	
	//fseek( fFile, 0, SEEK_END );
	//size = ftell( fFile );
	//fseek( fFile, 0, SEEK_SET );
	fFile->seekg ( 0, std::ifstream::end );
	size = fFile->tellg ();
	fFile->seekg ( 0, std::ifstream::beg );
	
	
	if ( !fFile->is_open() || fFile->fail() ) return false;
	//if ( !result ) return false;
	
	fFile->read ( cData, size );
	result = fFile->gcount();
	
	// terminate string
	cData [ result ] = 0;
	
	// close the file when done
	fFile->close ();
	//fclose ( fFile );
	
	cout << "\nLoading: Config file size=" << strlen( cData );
	
	delete fFile;
	
	return true;
}



void File::Save ( string NameOfFile )
{
	ofstream *fFile;
	fFile = new ofstream( NameOfFile.c_str(), ios::trunc /*ios::binary*/ );
	
	if ( !fFile->is_open() || fFile->fail() ) return false;
	
	//fFile->write ( sData.c_str(), sData.size() );
	fFile->write ( cData, strlen(cData) );
	
	// close the file when done writing to it
	fFile->close ();
	
	cout << "\nSaving: Config file size=" << strlen( cData );
	
	delete fFile;
	
	return true;
}



bool File::Get_Value32 ( string VarName, long& Value )
{
	int x0, x1;
	string sVar;
	
	VarName = c_sVarPrefix + VarName + c_sVarPostfix;
	
	// find where variable is in string
	x0 = InStr ( cData, VarName );
	
	// if it is not there, then return false
	if ( x0 == string::npos ) return false;
	
	// look for assignment operator
	x0++;
	x0 = InStr ( cData, c_sAssigner, x0 );
	
	// look for delimiter
	x0++;
	x1 = InStr ( cData, c_sDelimiter, x0 );
	
	// get the string
	sVar = Mid ( cData, x0, x1 - x0 );
	
	if ( !isNumeric ( sVar ) ) return false;
	
	// convert to number
	Value = CLng ( sVar );
	
	// successful
	return true;
}

bool File::Get_Value64 ( string VarName, long long& Value )
{
	int x0, x1;
	string sVar;
	
	VarName = c_sVarPrefix + VarName + c_sVarPostfix;
	
	// find where variable is in string
	x0 = InStr ( cData, VarName );
	
	// if it is not there, then return false
	if ( x0 == string::npos ) return false;
	
	// look for assignment operator
	x0++;
	x0 = InStr ( cData, c_sAssigner, x0 );
	
	// look for delimiter
	x0++;
	x1 = InStr ( cData, c_sDelimiter, x0 );
	
	// get the string
	sVar = Mid ( cData, x0, x1 - x0 );
	
	if ( !isNumeric ( sVar ) ) return false;
	
	// convert to number
	Value = CLngLng ( sVar );
	
	// successful
	return true;
}


bool File::Get_String ( string VarName, string& Value )
{
	int x0, x1;
	string sVar;
	
	VarName = c_sVarPrefix + VarName + c_sVarPostfix;
	
	// find where variable is in string
	x0 = InStr ( cData, VarName );
	
	// if it is not there, then return false
	if ( x0 == string::npos ) return false;
	
	// look for assignment operator
	x0++;
	x0 = InStr ( cData, c_sAssigner, x0 );
	
	// look for delimiter
	x0++;
	x1 = InStr ( cData, c_sDelimiter, x0 );
	
	// get the string
	sVar = Mid ( cData, x0, x1 - x0 );
	
	//if ( !isNumeric ( sVar ) ) return false;
	
	// convert to string
	Value = sVar;
	
	// successful
	return true;
}


void File::Set_Value32 ( string VarName, long Value )
{
	VarName = c_sVarPrefix + VarName + c_sVarPostfix + c_sAssigner + CStr ( Value ) + c_sDelimiter;
	strcat ( cData, VarName.c_str () );
}

void File::Set_Value64 ( string VarName, long long Value )
{
	VarName = c_sVarPrefix + VarName + c_sVarPostfix + c_sAssigner + CStr ( Value ) + c_sDelimiter;
	strcat ( cData, VarName.c_str () );
}

void File::Set_String ( string VarName, string Value )
{
	VarName = c_sVarPrefix + VarName + c_sVarPostfix + c_sAssigner + Value + c_sDelimiter;
	strcat ( cData, VarName.c_str () );
}



static bool PSXDiskUtility::GetPSXIDString ( char* Output, char* PSXFileName, int DiskSectorSize )
{
	static const int c_iOutputStringSize = 11;

	int size, result;
	ifstream *fFile;
	char* cData = new char [ DiskSectorSize ];
	
	fFile = new ifstream( PSXFileName, ios_base::in | ios_base::binary );
	
	if ( !fFile->is_open() || fFile->fail() )
	{
		cout << "\n***ERROR*** GetPSIDString: Problem opening file: " << PSXFileName;
		
		delete fFile;
		
		return false;
	}
	
	fFile->seekg ( 0, std::ifstream::end );
	size = fFile->tellg ();
	fFile->seekg ( 0, std::ifstream::beg );
	
	do
	{
	
		fFile->read ( cData, DiskSectorSize );
		
		// find string with data
		for ( int i = 0; ( i + 12 ) < DiskSectorSize; i++ )
		{
			// SLXX_YYY.ZZ;1
			// 0 -> S, 1 -> L, 4 -> _, 8 -> ., 11 -> ;, 12 -> 1
			if ( cData [ i ] == 'S' && cData [ i + 1 ] == 'L' && cData [ i + 4 ] == '_' && cData [ i + 8 ] == '.' && cData [ i + 11 ] == ';' && cData [ i + 12 ] == '1' )
			{
				cout << "\nDisk ID=" << cData [ i ] << cData [ i + 1 ] << cData [ i + 2 ] << cData [ i + 3 ] << cData [ i + 4 ] << cData [ i + 5 ] << cData [ i + 6 ] << cData [ i + 7 ] << cData [ i + 8 ] << cData [ i + 9 ] << cData [ i + 10 ] << cData [ i + 11 ] << cData [ i + 12 ];
				//cout << "\nIndex=" << dec << i;
				
				cout << "\nid";
				
				for ( int j = 0; j < c_iOutputStringSize; j++ )
				{
					Output [ j ] = cData [ j + i ];
				}
				
				cout << "\ncopied";
				
				Output [ c_iOutputStringSize ] = 0;
				
				cout << "\nzero";
				
				fFile->close ();
				
				cout << "\nclosed";
				
				// done
				delete cData;
				
				delete fFile;
				
				return true;
			}
		}
		
	} while ( !fFile->eof () );
	
	fFile->close ();
	
	delete cData;
	
	delete fFile;
	
	return true;
}


