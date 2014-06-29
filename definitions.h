/* ***************************************************************************
 *                                Sunsetter                                  *
 *               (c) Ben Dean-Kawamura, Georg v. Zimmermann                  *
 *  For license terms, see the file COPYING that came with this program.     *
 *                                                                           *
 *  Name: definitions.h                                                      *
 *  Purpose: Has the definitions I think make things easier.                 *
 *                                                                           *
 *************************************************************************** */

#ifndef _DEFINITIONS_
#define _DEFINITIONS_

typedef unsigned char byte;
typedef signed char sbyte;
typedef unsigned short int word;
typedef signed short int sword;
typedef unsigned int duword;
typedef signed int sdword;


#ifdef _WIN32

	typedef unsigned __int64 qword;
	typedef signed __int64 sqword;


	#define strcasecmp(a,b) stricmp(a,b)   // subst
	#define strncasecmp(a,b,c) _strnicmp(a,b,c)

	typedef int socklen_t;

#else
	typedef unsigned long long qword;
	typedef long long sqword;

	#define qword(target) (unsigned long long)(target##ULL)
#endif


#endif
