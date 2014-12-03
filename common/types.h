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



#ifndef _TYPES_H_

#define _TYPES_H_


#define ALIGN16		__attribute__ ((aligned (16)))
#define ALIGN32		__attribute__ ((aligned (32)))
#define ALIGN64		__attribute__ ((aligned (64)))


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;

typedef char s8;
typedef short s16;
typedef long s32;
typedef long long s64;

union FloatLong
{
	float f;
	long l;
	unsigned long lu;
};

union DoubleLong
{
	double d;
	long long l;
	unsigned long long lu;
};


struct Reg32
{
	union
	{
		u32 u;
		s32 s;
		
		float f;
		
		u8 Bytes [ 4 ];
		
		struct
		{
			u8 ub8;
			u8 filler0;
			u16 filler1;
		};
		
		struct
		{
			s8 sb8;
			s8 filler2;
			s16 filler3;
		};
		
		struct
		{
			u16 uLo;
			u16 uHi;
		};
		
		struct
		{
			s16 sLo;
			s16 sHi;
		};
		
	};
};


struct Reg64
{
	union
	{
		struct
		{
			u32 uLo;
			u32 uHi;
		};
		
		struct
		{
			s32 sLo;
			s32 sHi;
		};

		u64 uValue;
		s64 sValue;
		
		u32 uArray [ 2 ];
		s32 sArray [ 2 ];
	};
};


struct Reg128
{
	union
	{
		struct
		{
			u64 u;
			u64 filler64_0;
		};
		
		struct
		{
			s64 s;
			s64 filler64_1;
		};
		
		struct
		{
			float fx, fy, fz, fw;
		};
		
		struct
		{
			u32 ux, uy, uz, uw;
		};
		
		struct
		{
			s32 sx, sy, sz, sw;
		};
		
		struct
		{
			u64 uq0, uq1;
		};
		
		struct
		{
			s64 sq0, sq1;
		};
		
		struct
		{
			u32 uw0, uw1, uw2, uw3;
		};
		
		struct
		{
			s32 sw0, sw1, sw2, sw3;
		};
		
		struct
		{
			u16 uh0, uh1, uh2, uh3, uh4, uh5, uh6, uh7;
		};
		
		struct
		{
			s16 sh0, sh1, sh2, sh3, sh4, sh5, sh6, sh7;
		};
		
		struct
		{
			u8 ub0, ub1, ub2, ub3, ub4, ub5, ub6, ub7, ub8, ub9, ub10, ub11, ub12, ub13, ub14, ub15;
		};
		
		struct
		{
			s8 sb0, sb1, sb2, sb3, sb4, sb5, sb6, sb7, sb8, sb9, sb10, sb11, sb12, sb13, sb14, sb15;
		};
		
		struct
		{
			u64 uLo;
			u64 uHi;
		};
		
		struct
		{
			s64 sLo;
			s64 sHi;
		};
		
		u8 vub [ 16 ];
		u16 vuh [ 8 ];
		u32 vuw [ 4 ];
		u64 vuq [ 2 ];
		
		s8 vsb [ 16 ];
		s16 vsh [ 8 ];
		s32 vsw [ 4 ];
		s64 vsq [ 2 ];
		
		
		
	};
};



// this is for any code used from mame/mess
//typedef long long INT64;
//typedef long INT32;
//typedef short INT16;
//typedef char INT8;
//typedef unsigned long long UINT64;
//typedef unsigned long UINT32;
//typedef unsigned short UINT16;
//typedef unsigned char UINT8;


#endif


