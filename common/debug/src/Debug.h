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


#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "WinFile.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

using namespace std;

using namespace WinApi;

namespace Debug
{
	class Log
	{
	public:
		typedef std::ios_base& (*FunctionIOS) ( std::ios_base& );
		typedef ostream& (*FunctionOSTREAM) ( ostream& );
		//typedef smanip (*FunctionMANIP) ( int );
	
		string OutputFileName;
		//ofstream OutputFile;
		stringstream ss;
		
		File OutputFile;
		
		// construtor
		Log();

		// destructor
		~Log();
		
		bool Create ( char* LogFileName );
		
	/*
		ostream& operator<< (ostream& out, char c );
		ostream& operator<< (ostream& out, signed char c );
		ostream& operator<< (ostream& out, unsigned char c );
		 
		ostream& operator<< (ostream& out, const char* s );
		ostream& operator<< (ostream& out, const signed char* s );
		ostream& operator<< (ostream& out, const unsigned char* s );
	*/
		
		Log& operator<< ( char s );
		Log& operator<< ( signed char s );
		Log& operator<< ( unsigned char s );
		Log& operator<< ( const char* s );
		Log& operator<< ( const signed char* s );
		Log& operator<< ( const unsigned char* s );
		Log& operator<< ( int s );
		Log& operator<< ( long s );
		Log& operator<< ( long long s );
		Log& operator<< ( unsigned long s );
		Log& operator<< ( unsigned long long s );
		Log& operator<< ( float s );
		Log& operator<< ( double s );
		Log& operator<< ( FunctionIOS s );
		Log& operator<< ( FunctionOSTREAM s );
		Log& operator<< ( _Setw s );
		
	};


}





#endif


