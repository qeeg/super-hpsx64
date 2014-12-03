

#include "Debug.h"

using namespace Debug;

Log::Log()
{
}

Log::~Log()
{
	OutputFile.Close ();
}

bool Log::Create ( char* LogFileName )
{
	OutputFileName = LogFileName;
	//OutputFile.open ( LogFileName );
	OutputFile.Create ( LogFileName );
	//OutputFile.close ();
	ss.str("");
	return true;
}

/*
ostream& operator<< (ostream& out, char c );
ostream& operator<< (ostream& out, signed char c );
ostream& operator<< (ostream& out, unsigned char c );
 
ostream& operator<< (ostream& out, const char* s );
ostream& operator<< (ostream& out, const signed char* s );
ostream& operator<< (ostream& out, const unsigned char* s );
*/

Log& Log::operator<< ( char s )
{

	ss << s;
	OutputFile.WriteString ( ss.str().c_str () );
	ss.str ( "" );
	return *this;

}

Log& Log::operator<< ( signed char s )
{

	ss << s;
	OutputFile.WriteString ( ss.str().c_str () );
	ss.str ( "" );
	return *this;

}

Log& Log::operator<< ( unsigned char s )
{

	ss << s;
	OutputFile.WriteString ( ss.str().c_str () );
	ss.str ( "" );
	return *this;

}
 
Log& Log::operator<< ( const char* s )
{

	ss << s;
	OutputFile.WriteString ( ss.str().c_str () );
	ss.str ( "" );
	return *this;

}

Log& Log::operator<< ( const signed char* s )
{

	ss << s;
	OutputFile.WriteString ( ss.str().c_str () );
	ss.str ( "" );
	return *this;

}

Log& Log::operator<< ( const unsigned char* s )
{

	ss << s;
	OutputFile.WriteString ( ss.str().c_str () );
	ss.str ( "" );
	return *this;

}

Log& Log::operator<< ( int s )
{

	ss << s;
	OutputFile.WriteString ( ss.str().c_str () );
	ss.str ( "" );
	return *this;

}

Log& Log::operator<< ( long s )
{

	ss << s;
	OutputFile.WriteString ( ss.str().c_str () );
	ss.str ( "" );
	return *this;

}

Log& Log::operator<< ( long long s )
{

	ss << s;
	OutputFile.WriteString ( ss.str().c_str () );
	ss.str ( "" );
	return *this;

}


Log& Log::operator<< ( unsigned long s )
{

	ss << s;
	OutputFile.WriteString ( ss.str().c_str () );
	ss.str ( "" );
	return *this;

}

Log& Log::operator<< ( unsigned long long s )
{

	ss << s;
	OutputFile.WriteString ( ss.str().c_str () );
	ss.str ( "" );
	return *this;

}

Log& Log::operator<< ( float s )
{

	ss << s;
	OutputFile.WriteString ( ss.str().c_str () );
	ss.str ( "" );
	return *this;

}

Log& Log::operator<< ( double s )
{

	ss << s;
	OutputFile.WriteString ( ss.str().c_str () );
	ss.str ( "" );
	return *this;

}

Log& Log::operator<< ( FunctionIOS s )
{

	ss << s;
	//OutputFile.WriteString ( ss.str().c_str () );
	//ss.str ( "" );
	return *this;

}

Log& Log::operator<< ( FunctionOSTREAM s )
{

	ss << s;
	//OutputFile.WriteString ( ss.str().c_str () );
	//ss.str ( "" );
	return *this;

}

/*
Log& Log::operator<< ( FunctionMANIP s )
{

	ss << s;
	//OutputFile.WriteString ( ss.str().c_str () );
	//ss.str ( "" );
	return *this;

}
*/

Log& Log::operator<< ( _Setw s )
{

	ss << s;
	//OutputFile.WriteString ( ss.str().c_str () );
	//ss.str ( "" );
	return *this;

}




