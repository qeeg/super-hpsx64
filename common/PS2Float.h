

#include "types.h"
#include <math.h>


#ifndef PSFLOAT_H

#define PSFLOAT_H


#define ENABLE_FLAG_CHECK



namespace PS2Float
{
	static const long long c_llPS2DoubleMax = ( 1151ULL << 52 );
	static const long long c_llPS2DoubleMin = ( 897ULL << 52 );
	//static const long long c_llPS2DoubleMin = ( 1ULL << 63 ) | ( 1151ULL << 52 );
	static const long long c_llDoubleINF = ( 0x7ffULL << 52 );
	static const long long c_llDoubleAbsMask = ( 1ULL << 63 );

	static const long c_lFloatINF = ( 0xff << 23 );
	
	// difference between max ps2 float and next value down
	static const long c_lFloatMaxDiff = 0x73800000;
	
	// these 3 functions are from: http://cottonvibes.blogspot.com/2010/07/testing-for-nan-and-infinity-values.html
	//inline bool isNaN(float x) { return x != x; }
	//inline bool isInf(float x) { return fabs(x) == numeric_limits<float>::infinity(); }
	static inline bool isNaNorInf(float x) { return ((long&)x & 0x7fffffff) >= 0x7f800000; }
	//inline bool isNaN_d (double x) { return x != x; }
	//inline bool isInf_d (double x) { return fabs(x) == numeric_limits<float>::infinity(); }
	static inline bool isNaNorInf_d (double x) { return ((long long&)x & 0x7fffffffffffffffULL) >= 0x7ff0000000000000ULL; }
	
	
	// PS2 floating point ADD
	inline static float PS2_Float_Add ( float fs, float ft, int index, short* StatusFlag, short* MACFlag )		//long* zero, long* sign, long* overflow, long* underflow,
									//long* zero_sticky, long* sign_sticky, long* overflow_sticky, long* underflow_sticky )
	{
		// fd = fs + ft
		DoubleLong Dd, Ds, Dt;
		
		FloatLong Result;
		
		// convert to double
		// note: after conversion, if +/- inf/nan, should stay same
		Ds.d = (double) fs;
		Dt.d = (double) ft;
		
		if ( isNaNorInf_d ( Ds.d ) )
		{
			Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
		}
		
		if ( isNaNorInf_d ( Dt.d ) )
		{
			Dt.l = c_llPS2DoubleMax | ( Dt.l & c_llDoubleAbsMask );
		}
		
		// add the numbers
		Dd.d = Ds.d + Dt.d;
		
		// convert back to float
		// note: for now, this is cool, because...
		// *** todo *** implement proper conversion (max PS2 value of +/- INF does not convert correctly)
		Result.f = (float) Dd.d;
		
		
		// really need the index with x at 3 and w at 0
		//index = 3 - index;
		
#ifdef ENABLE_FLAG_CHECK

		// set sign flags
		//if ( Result.f < 0.0 )
		if ( Result.l >> 31 )
		{
			// set sign flags
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		
		
		

		// check for underflow
		// smallest float value is 2^-126 = 0x00800000
		// value as a double is 1023-126 = 897
		if ( ( Dd.l & ~c_llDoubleAbsMask ) <= c_llPS2DoubleMin && ( Dd.l & ~c_llDoubleAbsMask ) )
		{
			// underflow //
			*StatusFlag |= 0x104;
			*MACFlag |= ( 1 << ( index + 8 ) );
		}
		
		// check for overflow
		if ( ( Dd.l & ~c_llDoubleAbsMask ) >= c_llPS2DoubleMax )
		{
			// overflow //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
		}
		
		// set zero flags
		if ( Result.f == 0.0 )
		{
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
#endif
		
		// done?
		return Result.f;
	}

	// PS2 floating point SUB
	inline static float PS2_Float_Sub ( float fs, float ft, int index, short* StatusFlag, short* MACFlag )		//long* zero, long* sign, long* overflow, long* underflow,
										//long* zero_sticky, long* sign_sticky, long* overflow_sticky, long* underflow_sticky )
	{
		// fd = fs + ft
		DoubleLong Dd, Ds, Dt;
		
		FloatLong Result;
		
		// convert to double
		// note: after conversion, if +/- inf/nan, should stay same
		Ds.d = (double) fs;
		Dt.d = (double) ft;
		
		if ( isNaNorInf_d ( Ds.d ) )
		{
			Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
		}
		
		if ( isNaNorInf_d ( Dt.d ) )
		{
			Dt.l = c_llPS2DoubleMax | ( Dt.l & c_llDoubleAbsMask );
		}
		
		// add the numbers
		Dd.d = Ds.d - Dt.d;
		
		// convert back to float
		// note: for now, this is cool, because...
		// *** todo *** implement proper conversion (max PS2 value of +/- INF does not convert correctly)
		Result.f = (float) Dd.d;
		
		// really need the index with x at 3 and w at 0
		//index = 3 - index;
		
#ifdef ENABLE_FLAG_CHECK

		// set sign flags
		//if ( Result.f < 0.0 )
		if ( Result.l >> 31 )
		{
			// set sign flags
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		
		// check for underflow
		// smallest float value is 2^-126 = 0x00800000
		// value as a double is 1023-126 = 897
		if ( ( Dd.l & ~c_llDoubleAbsMask ) <= c_llPS2DoubleMin && ( Dd.l & ~c_llDoubleAbsMask ) )
		{
			// underflow //
			*StatusFlag |= 0x104;
			*MACFlag |= ( 1 << ( index + 8 ) );
		}
		
		// check for overflow
		if ( ( Dd.l & ~c_llDoubleAbsMask ) >= c_llPS2DoubleMax )
		{
			// overflow //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
		}
		
		// set zero flags
		if ( Result.f == 0.0 )
		{
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
#endif
		
		// done?
		return Result.f;
	}

	// PS2 floating point MUL
	inline static float PS2_Float_Mul ( float fs, float ft, int index, short* StatusFlag, short* MACFlag )		//long* zero, long* sign, long* underflow, long* overflow,
								//long* zero_sticky, long* sign_sticky, long* underflow_sticky, long* overflow_sticky )
	{
		// fd = fs + ft
		DoubleLong Dd, Ds, Dt;
		
		FloatLong Result;
		
		// convert to double
		// note: after conversion, if +/- inf/nan, should stay same
		Ds.d = (double) fs;
		Dt.d = (double) ft;
		
		if ( isNaNorInf_d ( Ds.d ) )
		{
			Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
		}
		
		if ( isNaNorInf_d ( Dt.d ) )
		{
			Dt.l = c_llPS2DoubleMax | ( Dt.l & c_llDoubleAbsMask );
		}
		
		// multiply the numbers
		Dd.d = Ds.d * Dt.d;
		
		// convert back to float
		// note: for now, this is cool, because...
		// *** todo *** implement proper conversion (max PS2 value of +/- INF does not convert correctly)
		Result.f = (float) Dd.d;
		
		
		// really need the index with x at 3 and w at 0
		//index = 3 - index;
		
#ifdef ENABLE_FLAG_CHECK

		// set sign flags
		//if ( Result.f < 0.0 )
		if ( Result.l >> 31 )
		{
			// set sign flags
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		

		// check for underflow
		// smallest float value is 2^-126 = 0x00800000
		// value as a double is 1023-126 = 897
		if ( ( Dd.l & ~c_llDoubleAbsMask ) <= c_llPS2DoubleMin && ( Dd.l & ~c_llDoubleAbsMask ) )
		{
			// underflow //
			*StatusFlag |= 0x104;
			*MACFlag |= ( 1 << ( index + 8 ) );
		}
		
		// check for overflow
		if ( ( Dd.l & ~c_llDoubleAbsMask ) >= c_llPS2DoubleMax )
		{
			// overflow //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
		}
		
		// set zero flags
		if ( Result.f == 0.0 )
		{
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
#endif
		
		// done?
		return Result.f;
	}

	// PS2 floating point MADD
	inline static float PS2_Float_Madd ( double dACC, float fs, float ft, int index, short* StatusFlag, short* MACFlag )		//long* zero, long* sign, long* underflow, long* overflow,
								//long* zero_sticky, long* sign_sticky, long* underflow_sticky, long* overflow_sticky )
	{
		// fd = fs + ft
		DoubleLong Dd, Ds, Dt;
		
		FloatLong Result;
		
		// convert to double
		// note: after conversion, if +/- inf/nan, should stay same
		Ds.d = (double) fs;
		Dt.d = (double) ft;
		
		if ( isNaNorInf_d ( Ds.d ) )
		{
			Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
		}
		
		if ( isNaNorInf_d ( Dt.d ) )
		{
			Dt.l = c_llPS2DoubleMax | ( Dt.l & c_llDoubleAbsMask );
		}
		
		// multiply the numbers
		Dd.d = Ds.d * Dt.d;
		
		// add the numbers
		Dd.d = dACC + Dd.d;
		
		// convert back to float
		// note: for now, this is cool, because...
		// *** todo *** implement proper conversion (max PS2 value of +/- INF does not convert correctly)
		Result.f = (float) Dd.d;
		
		
		// really need the index with x at 3 and w at 0
		//index = 3 - index;
		
#ifdef ENABLE_FLAG_CHECK

		// set sign flags
		//if ( Result.f < 0.0 )
		if ( Result.l >> 31 )
		{
			// set sign flags
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		
		
		
		
		// check for underflow
		// smallest float value is 2^-126 = 0x00800000
		// value as a double is 1023-126 = 897
		if ( ( Dd.l & ~c_llDoubleAbsMask ) <= c_llPS2DoubleMin && ( Dd.l & ~c_llDoubleAbsMask ) )
		{
			// underflow //
			*StatusFlag |= 0x104;
			*MACFlag |= ( 1 << ( index + 8 ) );
		}
		
		// check for overflow
		if ( ( Dd.l & ~c_llDoubleAbsMask ) >= c_llPS2DoubleMax )
		{
			// overflow //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
		}
		
		// set zero flags
		if ( Result.f == 0.0 )
		{
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
#endif
		
		// done?
		return Result.f;
	}
	
	// PS2 floating point MSUB
	inline static float PS2_Float_Msub ( double dACC, float fs, float ft, int index, short* StatusFlag, short* MACFlag )		//long* zero, long* sign, long* underflow, long* overflow,
								//long* zero_sticky, long* sign_sticky, long* underflow_sticky, long* overflow_sticky )
	{
		// fd = fs + ft
		DoubleLong Dd, Ds, Dt;
		
		FloatLong Result;
		
		// convert to double
		// note: after conversion, if +/- inf/nan, should stay same
		Ds.d = (double) fs;
		Dt.d = (double) ft;
		
		if ( isNaNorInf_d ( Ds.d ) )
		{
			Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
		}
		
		if ( isNaNorInf_d ( Dt.d ) )
		{
			Dt.l = c_llPS2DoubleMax | ( Dt.l & c_llDoubleAbsMask );
		}
		
		// multiply the numbers
		Dd.d = Ds.d * Dt.d;
		
		// sub the numbers
		Dd.d = dACC - Dd.d;
		
		// convert back to float
		// note: for now, this is cool, because...
		// *** todo *** implement proper conversion (max PS2 value of +/- INF does not convert correctly)
		Result.f = (float) Dd.d;
		
		
		// really need the index with x at 3 and w at 0
		//index = 3 - index;
		
#ifdef ENABLE_FLAG_CHECK

		// set sign flags
		//if ( Result.f < 0.0 )
		if ( Result.l >> 31 )
		{
			// set sign flags
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		
		
		
		
		// check for underflow
		// smallest float value is 2^-126 = 0x00800000
		// value as a double is 1023-126 = 897
		if ( ( Dd.l & ~c_llDoubleAbsMask ) <= c_llPS2DoubleMin && ( Dd.l & ~c_llDoubleAbsMask ) )
		{
			// underflow //
			*StatusFlag |= 0x104;
			*MACFlag |= ( 1 << ( index + 8 ) );
		}
		
		// check for overflow
		if ( ( Dd.l & ~c_llDoubleAbsMask ) >= c_llPS2DoubleMax )
		{
			// overflow //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
		}
		
		// set zero flags
		if ( Result.f == 0.0 )
		{
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
#endif
		
		// done?
		return Result.f;
	}

	
	inline static float PS2_Float_Max ( float fs, float ft )
	{
		//FloatLong Result;
		float fResult;
		
		if ( isNaNorInf ( fs ) ) (long&) fs = ( ( (long&) fs ) & 0xff800000 );
		if ( isNaNorInf ( ft ) ) (long&) ft = ( ( (long&) ft ) & 0xff800000 );
		
		// get max
		fResult = ( ( fs > ft ) ? fs : ft );
		
		// MAX does NOT affect any flags
		
		// done?
		return fResult;
	}

	inline static float PS2_Float_Min ( float fs, float ft )
	{
		//FloatLong Result;
		float fResult;
		
		if ( isNaNorInf ( fs ) ) (long&) fs = ( ( (long&) fs ) & 0xff800000 );
		if ( isNaNorInf ( ft ) ) (long&) ft = ( ( (long&) ft ) & 0xff800000 );
		
		// get max
		fResult = ( ( fs < ft ) ? fs : ft );
		
		// MIN does NOT affect any flags
		
		// done?
		return fResult;
	}

	

	// PS2 floating point SQRT
	inline static float PS2_Float_Sqrt ( float ft, short* StatusFlag )	//long* invalid_negative, long* invalid_zero,
										//long* divide_sticky, long* invalid_negative_sticky, long* invalid_zero_sticky )
	{
		// Q = sqrt ( ft )
		DoubleLong Dd, Ds, Dt;
		
		FloatLong Result;
		
		// convert to double
		// note: after conversion, if +/- inf/nan, should stay same
		//Ds.d = (double) fs;
		Dt.d = (double) ft;
		
		//if ( isNaNorInf_d ( Ds.d ) )
		//{
		//	Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
		//}
		
		if ( isNaNorInf_d ( Dt.d ) )
		{
			Dt.l = c_llPS2DoubleMax | ( Dt.l & c_llDoubleAbsMask );
		}
		
		// absolute value
		Dt.l &= ~c_llDoubleAbsMask;
		
		// sqrt the numbers
		Dd.d = sqrt ( Dt.d );
		
		// convert back to float
		// note: for now, this is cool, because...
		// *** todo *** implement proper conversion (max PS2 value of +/- INF does not convert correctly)
		Result.f = (float) Dd.d;
		
		// clear affected non-sticky flags
		*StatusFlag &= ~0x30;
		
		// check zero division flag -> set to zero if divide by zero
		// write zero division flag -> set to zero for SQRT
		//*divide = -1;
		
		// write invalid flag (SQRT of negative number or 0/0)
		//*invalid_negative = (long&) ft;
		if ( ft < 0.0f )
		{
			*StatusFlag |= 0x410;
		}
		
		// write zero divide/invalid sticky flags
		// leave divide by zero sticky flag alone, since it did not accumulate
		//*divide_stickyflag &= -1;
		//*invalid_negative_sticky |= (long&) ft;
		
		// invalid zero is ok, since there is no divide here
		// leave sticky flag alone
		//*invalid_zero = 0;
		//*invalid_zero_sticky -> leave alone
		
		// done?
		return Result.f;
	}

	
	// PS2 floating point RSQRT
	inline static float PS2_Float_RSqrt ( float fs, float ft, short* StatusFlag )	//long* divide, long* invalid_negative, long* invalid_zero,
										//long* divide_sticky, long* invalid_negative_sticky, long* invalid_zero_sticky )
	{
		// fd = fs + ft
		DoubleLong Dd, Ds, Dt;
		
		FloatLong Result;
		
		long temp1, temp2;
		
		// convert to double
		// note: after conversion, if +/- inf/nan, should stay same
		Ds.d = (double) fs;
		Dt.d = (double) ft;
		
		if ( isNaNorInf_d ( Ds.d ) )
		{
			Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
		}
		
		if ( isNaNorInf_d ( Dt.d ) )
		{
			Dt.l = c_llPS2DoubleMax | ( Dt.l & c_llDoubleAbsMask );
		}
		
		// absolute value
		Dt.l &= ~c_llDoubleAbsMask;
		
		// RSQRT the numbers
		Dd.d = Ds.d / sqrt ( Dt.d );
		
		// convert back to float
		// note: for now, this is cool, because...
		// *** todo *** implement proper conversion (max PS2 value of +/- INF does not convert correctly)
		Result.f = (float) Dd.d;
		
		// clear affected non-sticky flags
		*StatusFlag &= ~0x30;
		
		// write invalid flag (SQRT of negative number or 0/0)
		//*invalid_negative = (long&) ft;
		//temp1 = (long&) fs;
		//temp2 = (long&) ft;
		//*invalid_zero = temp1 | temp2;
		// *** todo ***
		//*invalid_zero = (long&) fs | (long&) ft;
		if ( ( ft < 0.0f ) || ( fs == 0.0f && ft == 0.0f ) )
		{
			*StatusFlag |= 0x410;
		}
		
		
		// write zero division flag -> set to zero for SQRT
		// write denominator
		//*divide = (long&) ft;
		if ( fs != 0.0f && ft == 0.0f )
		{
			*StatusFlag |= 0x820;
			
			// set result to +max/-max ??
			Result.l |= 0x7fffffff;
		}
		
		
		// write zero/sign sticky flags
		//*divide_sticky &= *divide;
		//*invalid_negative_sticky |= *invalid_negative;
		//*invalid_zero_sticky &= *invalid_zero;
		
		// done?
		return Result.f;
	}


	// PS2 floating point DIV
	inline static float PS2_Float_Div ( float fs, float ft, short* StatusFlag )		//long* divide, long* invalid_negative, long* invalid_zero,
										//long* divide_sticky, long* invalid_negative_sticky, long* invalid_zero_sticky )
	{
		// fd = fs + ft
		DoubleLong Dd, Ds, Dt;
		
		FloatLong Result;
		
		// convert to double
		// note: after conversion, if +/- inf/nan, should stay same
		Ds.d = (double) fs;
		Dt.d = (double) ft;
		
		if ( isNaNorInf_d ( Ds.d ) )
		{
			Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
		}
		
		if ( isNaNorInf_d ( Dt.d ) )
		{
			Dt.l = c_llPS2DoubleMax | ( Dt.l & c_llDoubleAbsMask );
		}
		
		// absolute value
		Dt.l &= ~c_llDoubleAbsMask;
		
		// fd = fs / ft
		Dd.d = Ds.d / Dt.d;
		
		// convert back to float
		// note: for now, this is cool, because...
		// *** todo *** implement proper conversion (max PS2 value of +/- INF does not convert correctly)
		Result.f = (float) Dd.d;
		
		// clear affected non-sticky flags
		*StatusFlag &= ~0x30;
		
		// write zero division flag -> set to zero for SQRT
		// write denominator
		//*divide = (long&) ft;
		if ( ft == 0.0f )
		{
			if ( fs != 0.0f )
			{
				// set divide by zero flag //
				*StatusFlag |= 0x820;
				
				// also set result to +max or -max
				Result.l |= 0x7fffffff;
			}
			else
			{
				// set invalid flag //
				*StatusFlag |= 0x410;
			}
		}
		
		// write invalid flag (SQRT of negative number or 0/0)
		// *** todo ***
		//*invalid_zero = (long&) fs | (long&) ft;
		//if ( fs == 0.0f && ft == 0.0f )
		//{
		//	*StatusFlag |= 0x410;
		//}
		
		// write zero/sign sticky flags
		//*divide_sticky &= *divide;
		//*invalid_negative_sticky |= *invalid_negative;
		//*invalid_zero_sticky &= *invalid_zero;
		
		// done?
		return Result.f;
	}
	
	
	// PS2 floating point ADD
	inline static double PS2_Float_AddA ( float fs, float ft, int index, short* StatusFlag, short* MACFlag )	//long* zero, long* sign, long* overflow, long* underflow,
									//long* zero_sticky, long* sign_sticky, long* overflow_sticky, long* underflow_sticky )
	{
		// fd = fs + ft
		DoubleLong Dd, Ds, Dt;
		
		FloatLong Result;
		
		// convert to double
		// note: after conversion, if +/- inf/nan, should stay same
		Ds.d = (double) fs;
		Dt.d = (double) ft;
		
		if ( isNaNorInf_d ( Ds.d ) )
		{
			Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
		}
		
		if ( isNaNorInf_d ( Dt.d ) )
		{
			Dt.l = c_llPS2DoubleMax | ( Dt.l & c_llDoubleAbsMask );
		}
		
		// add the numbers
		Dd.d = Ds.d + Dt.d;
		
		// convert back to float
		// note: for now, this is cool, because...
		// *** todo *** implement proper conversion (max PS2 value of +/- INF does not convert correctly)
		Result.f = (float) Dd.d;
		
		
		// really need the index with x at 3 and w at 0
		//index = 3 - index;
		
#ifdef ENABLE_FLAG_CHECK

		// set sign flags
		//if ( Result.f < 0.0 )
		if ( Result.l >> 31 )
		{
			// set sign flags
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		
		
		
		
		// check for underflow
		// smallest float value is 2^-126 = 0x00800000
		// value as a double is 1023-126 = 897
		if ( ( Dd.l & ~c_llDoubleAbsMask ) <= c_llPS2DoubleMin && ( Dd.l & ~c_llDoubleAbsMask ) )
		{
			// underflow //
			*StatusFlag |= 0x104;
			*MACFlag |= ( 1 << ( index + 8 ) );
			
			// set result to zero
			Dd.l &= 0x8000000000000000ULL;
		}
		
		// check for overflow
		if ( ( Dd.l & ~c_llDoubleAbsMask ) >= c_llPS2DoubleMax )
		{
			// overflow //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
		}
		
		// set zero flags
		if ( Dd.d == 0.0L )
		{
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
		
#endif

		// done?
		return Dd.d;
	}

	// PS2 floating point SUB
	inline static double PS2_Float_SubA ( float fs, float ft, int index, short* StatusFlag, short* MACFlag )	//long* zero, long* sign, long* overflow, long* underflow,
										//long* zero_sticky, long* sign_sticky, long* overflow_sticky, long* underflow_sticky )
	{
		// fd = fs + ft
		DoubleLong Dd, Ds, Dt;
		
		FloatLong Result;
		
		// convert to double
		// note: after conversion, if +/- inf/nan, should stay same
		Ds.d = (double) fs;
		Dt.d = (double) ft;
		
		if ( isNaNorInf_d ( Ds.d ) )
		{
			Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
		}
		
		if ( isNaNorInf_d ( Dt.d ) )
		{
			Dt.l = c_llPS2DoubleMax | ( Dt.l & c_llDoubleAbsMask );
		}
		
		// add the numbers
		Dd.d = Ds.d - Dt.d;
		
		// convert back to float
		// note: for now, this is cool, because...
		// *** todo *** implement proper conversion (max PS2 value of +/- INF does not convert correctly)
		Result.f = (float) Dd.d;
		
		
		// really need the index with x at 3 and w at 0
		//index = 3 - index;
		
#ifdef ENABLE_FLAG_CHECK
		// set sign flags
		//if ( Result.f < 0.0 )
		if ( Result.l >> 31 )
		{
			// set sign flags
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		
		
		
		// check for underflow
		// smallest float value is 2^-126 = 0x00800000
		// value as a double is 1023-126 = 897
		if ( ( Dd.l & ~c_llDoubleAbsMask ) <= c_llPS2DoubleMin && ( Dd.l & ~c_llDoubleAbsMask ) )
		{
			// underflow //
			*StatusFlag |= 0x104;
			*MACFlag |= ( 1 << ( index + 8 ) );
			
			// set result to zero
			Dd.l &= 0x8000000000000000ULL;
		}
		
		// check for overflow
		if ( ( Dd.l & ~c_llDoubleAbsMask ) >= c_llPS2DoubleMax )
		{
			// overflow //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
		}
		
		// set zero flags
		if ( Dd.d == 0.0L )
		{
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
#endif
		
		// done?
		return Dd.d;
	}

	// PS2 floating point MUL
	inline static double PS2_Float_MulA ( float fs, float ft, int index, short* StatusFlag, short* MACFlag )		//long* zero, long* sign, long* underflow, long* overflow,
								//long* zero_sticky, long* sign_sticky, long* underflow_sticky, long* overflow_sticky )
	{
		// fd = fs + ft
		DoubleLong Dd, Ds, Dt;
		
		FloatLong Result;
		
		// convert to double
		// note: after conversion, if +/- inf/nan, should stay same
		Ds.d = (double) fs;
		Dt.d = (double) ft;
		
		if ( isNaNorInf_d ( Ds.d ) )
		{
			Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
		}
		
		if ( isNaNorInf_d ( Dt.d ) )
		{
			Dt.l = c_llPS2DoubleMax | ( Dt.l & c_llDoubleAbsMask );
		}
		
		// multiply the numbers
		Dd.d = Ds.d * Dt.d;
		
		// convert back to float
		// note: for now, this is cool, because...
		// *** todo *** implement proper conversion (max PS2 value of +/- INF does not convert correctly)
		Result.f = (float) Dd.d;
		
		
		// really need the index with x at 3 and w at 0
		//index = 3 - index;
		
#ifdef ENABLE_FLAG_CHECK
		// set sign flags
		//if ( Result.f < 0.0 )
		if ( Result.l >> 31 )
		{
			// set sign flags
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		
		
		
		// check for underflow
		// smallest float value is 2^-126 = 0x00800000
		// value as a double is 1023-126 = 897
		if ( ( Dd.l & ~c_llDoubleAbsMask ) <= c_llPS2DoubleMin && ( Dd.l & ~c_llDoubleAbsMask ) )
		{
			// underflow //
			*StatusFlag |= 0x104;
			*MACFlag |= ( 1 << ( index + 8 ) );
			
			// set result to zero
			Dd.l &= 0x8000000000000000ULL;
		}
		
		// check for overflow
		if ( ( Dd.l & ~c_llDoubleAbsMask ) >= c_llPS2DoubleMax )
		{
			// overflow //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
		}
		
		// set zero flags
		if ( Dd.d == 0.0L )
		{
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
#endif
		
		// done?
		return Dd.d;
	}

	// PS2 floating point MADD
	inline static double PS2_Float_MaddA ( double dACC, float fs, float ft, int index, short* StatusFlag, short* MACFlag )		//long* zero, long* sign, long* underflow, long* overflow,
								//long* zero_sticky, long* sign_sticky, long* underflow_sticky, long* overflow_sticky )
	{
		// fd = fs + ft
		DoubleLong Dd, Ds, Dt;
		
		FloatLong Result;
		
		// convert to double
		// note: after conversion, if +/- inf/nan, should stay same
		Ds.d = (double) fs;
		Dt.d = (double) ft;
		
		if ( isNaNorInf_d ( Ds.d ) )
		{
			Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
		}
		
		if ( isNaNorInf_d ( Dt.d ) )
		{
			Dt.l = c_llPS2DoubleMax | ( Dt.l & c_llDoubleAbsMask );
		}
		
		// multiply the numbers
		Dd.d = Ds.d * Dt.d;
		
		// add the numbers
		Dd.d = dACC + Dd.d;
		
		// convert back to float
		// note: for now, this is cool, because...
		// *** todo *** implement proper conversion (max PS2 value of +/- INF does not convert correctly)
		Result.f = (float) Dd.d;
		
		
		// really need the index with x at 3 and w at 0
		//index = 3 - index;
		
#ifdef ENABLE_FLAG_CHECK
		// set sign flags
		//if ( Result.f < 0.0 )
		if ( Result.l >> 31 )
		{
			// set sign flags
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
		}
		
		
		
		// check for underflow
		// smallest float value is 2^-126 = 0x00800000
		// value as a double is 1023-126 = 897
		if ( ( Dd.l & ~c_llDoubleAbsMask ) <= c_llPS2DoubleMin && ( Dd.l & ~c_llDoubleAbsMask ) )
		{
			// underflow //
			*StatusFlag |= 0x104;
			*MACFlag |= ( 1 << ( index + 8 ) );
			
			// set result to zero
			Dd.l &= 0x8000000000000000ULL;
		}
		
		// check for overflow
		if ( ( Dd.l & ~c_llDoubleAbsMask ) >= c_llPS2DoubleMax )
		{
			// overflow //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
		}

		// set zero flags
		if ( Dd.d == 0.0L )
		{
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
#endif
		
		// done?
		return Dd.d;
	}
	
	// PS2 floating point MSUB
	inline static double PS2_Float_MsubA ( double dACC, float fs, float ft, int index, short* StatusFlag, short* MACFlag )		//long* zero, long* sign, long* underflow, long* overflow,
								//long* zero_sticky, long* sign_sticky, long* underflow_sticky, long* overflow_sticky )
	{
		// fd = fs + ft
		DoubleLong Dd, Ds, Dt;
		
		FloatLong Result;
		
		// convert to double
		// note: after conversion, if +/- inf/nan, should stay same
		Ds.d = (double) fs;
		Dt.d = (double) ft;
		
		if ( isNaNorInf_d ( Ds.d ) )
		{
			Ds.l = c_llPS2DoubleMax | ( Ds.l & c_llDoubleAbsMask );
		}
		
		if ( isNaNorInf_d ( Dt.d ) )
		{
			Dt.l = c_llPS2DoubleMax | ( Dt.l & c_llDoubleAbsMask );
		}
		
		// multiply the numbers
		Dd.d = Ds.d * Dt.d;
		
		// sub the numbers
		Dd.d = dACC - Dd.d;
		
		// convert back to float
		// note: for now, this is cool, because...
		// *** todo *** implement proper conversion (max PS2 value of +/- INF does not convert correctly)
		Result.f = (float) Dd.d;
		
		
		// really need the index with x at 3 and w at 0
		//index = 3 - index;
		
#ifdef ENABLE_FLAG_CHECK
		// set sign flags
		//if ( Result.f < 0.0 )
		if ( Result.l >> 31 )
		{
			// set sign flags
			*StatusFlag |= 0x82;
			*MACFlag |= ( 1 << ( index + 4 ) );
			
			// set result to zero
			Dd.l &= 0x8000000000000000ULL;
		}
		
		
		
		
		// check for underflow
		// smallest float value is 2^-126 = 0x00800000
		// value as a double is 1023-126 = 897
		if ( ( Dd.l & ~c_llDoubleAbsMask ) <= c_llPS2DoubleMin && ( Dd.l & ~c_llDoubleAbsMask ) )
		{
			// underflow //
			*StatusFlag |= 0x104;
			*MACFlag |= ( 1 << ( index + 8 ) );
			
			// set result to zero
			Dd.l &= 0x8000000000000000ULL;
		}
		
		// check for overflow
		if ( ( Dd.l & ~c_llDoubleAbsMask ) >= c_llPS2DoubleMax )
		{
			// overflow //
			*StatusFlag |= 0x208;
			*MACFlag |= ( 1 << ( index + 12 ) );
		}

		// set zero flags
		if ( Dd.d == 0.0L )
		{
			// set zero flags
			*StatusFlag |= 0x41;
			*MACFlag |= ( 1 << index );
		}
#endif
		
		// done?
		return Dd.d;
	}

}


#endif



