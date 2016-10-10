#pragma once
//////////////////////////////////////////////////////////////////////
//	CFilename Class
//
//	(c) 2003 IDV, Inc.
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization and may
//	not be copied or disclosed except in accordance with the terms of
//	that agreement.
//
//      Copyright (c) 2001-2003 IDV, Inc.
//      All Rights Reserved.
//
//		IDV, Inc.
//		1233 Washington St. Suite 610
//		Columbia, SC 29201
//		Voice: (803) 799-1699
//		Fax:   (803) 931-0320
//		Web:   http://www.idvinc.com
//

#include <string>

//////////////////////////////////////////////////////////////////////
//	CFilename Class

class CFilename : public std::string
{
public:
        CFilename( )                                            { }
        CFilename(const char* pFilename) : std::string(pFilename)    { }
        CFilename(std::string strFilename) :std::string(strFilename) { }

		CFilename GetExtension();      // if filename is "/idv/code/file.cpp", it returns "cpp"
		CFilename GetPath();           // if filename is "/idv/code/file.cpp", it returns "/idv/code"
		CFilename NoExtension();       // if filename is "/idv/code/file.cpp", it returns "/idv/code/file"
		CFilename NoPath();            // if filename is "/idv/code/file.cpp", it returns "file.cpp"
};


//////////////////////////////////////////////////////////////////////
//	CFilename::GetExtension

inline CFilename CFilename::GetExtension()
{
	std::string strExtension;

	int nLength = length( );
	for (int i = nLength - 1; i > 0 && (*this)[i] != '/'; i--)
		if ((*this)[i] == '.')
		{
			strExtension = std::string(c_str( ) + i + 1);
			break;
		}

	return strExtension;
}


//////////////////////////////////////////////////////////////////////
//	CFilename::GetPath

inline CFilename CFilename::GetPath()
{
	char szPath[1024];

	szPath[0] = '\0';
	int nLength = length( );
	for (int i = nLength - 1; i >= 0; i--)
		if ((*this)[i] == '/' ||
			(*this)[i] == '\\')
		{
			int j;
			for (j = 0; j < i + 1; j++)
				szPath[j] = (*this)[j];
			szPath[j] = '\0';
			break;
		}

	return CFilename(szPath);
}


//////////////////////////////////////////////////////////////////////
//	CFilename::NoExtension

inline CFilename CFilename::NoExtension()
{
	char szExtension[1024];

	strcpy(szExtension, c_str( ));
	int nLength = length( );
	for (int i = nLength - 1; i >= 0 && (*this)[i] != '/'; i--)
		if ((*this)[i] == '.')
		{
			int j;
			for (j = 0; j < i; j++)
				szExtension[j] = (*this)[j];
			szExtension[j] = '\0';
			break;
		}

	return szExtension;
}


//////////////////////////////////////////////////////////////////////
//	CFilename::NoPath

inline CFilename CFilename::NoPath()
{
	char szPath[1024];

	strcpy(szPath, c_str( ));
	int nLength = length( );
	for (int i = nLength - 1; i >= 0; i--)
		if ((*this)[i] == '/' || (*this)[i] == '\\')
		{
			int j,k;
			for (j = i + 1, k = 0; j < nLength; j++, k++)
				szPath[k] = (*this)[j];
			szPath[k] = '\0';
			break;
		}

	return std::string(szPath);
}
