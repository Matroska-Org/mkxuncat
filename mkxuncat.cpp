/****************************************************************************
** mkxuncat : split Matroska files in pieces, see http://www.matroska.org/
**
** Copyright (C) 2005 Steve Lhomme.  All rights reserved.
**
** This file is part of mkxuncat.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id $
	\author Steve Lhomme     <robux4 @ matroska.org>
*/

#include <cstdio>
#include <string>

using namespace std;

#include <ebml/EbmlStream.h>
#include <ebml/EbmlHead.h>
#include <ebml/EbmlContexts.h>
#include <matroska/KaxContexts.h>
#include <matroska/KaxSegment.h>

using namespace LIBEBML_NAMESPACE;
using namespace LIBMATROSKA_NAMESPACE;

#if defined(_WIN32)
#include <WinIOCallback.h>
typedef WinIOCallback IOclass;
#else
#include <ebml/StdIOCallback.h>
typedef StdIOCallback IOclass;
#endif

int main(int argc, const char *argv[])
{
	fprintf( stdout, "mkxuncat v0.1.2 (c) Steve Lhomme - GPL\n" );

	if ( argc != 2 )
	{
		fprintf( stderr, "Usage: mkxuncat <file_to_uncat>\n\tUndo concatenation on the given file\n" );
		return -1;
	}

	// get the file basename/extension
	string basename = argv[1];
	string extension;
	if (basename.rfind( '.' ) != string::npos )
	{
		extension = basename.substr( basename.rfind( '.' )+1 );
		basename = basename.substr( 0, basename.rfind( '.' ) );
	}

	IOclass p_InputFile( argv[1], MODE_READ );
	if ( p_InputFile.getFilePointer() != 0 )
	{
		fprintf( stderr, "Could not open file '%s' for reading\n", argv[1] );
		return -2;
	}

	EbmlStream aStream( p_InputFile );

	unsigned int u_OuputFileIndex = 0;
	char t_IndexBuffer[8];
	int64 i_DataSize, i_SizeToCopy;
	string OutputFilename;
	int i_Level = 0;
	IOclass *p_OutputFile = NULL;
	char *ReadBuff = new char[8*1024];

	EbmlElement * ElementLevel0;

	// find the EBML head in the file
	ElementLevel0 = aStream.FindNextElement( KaxMatroska_Context, i_Level, 0xFFFFFFFFL, false, 1 );
	
	while ( ElementLevel0 != NULL )
	{
		if ( EbmlId(*ElementLevel0) == EbmlHead::ClassInfos.GlobalId )
		{
			delete p_OutputFile;

			OutputFilename = basename + '.' + itoa( u_OuputFileIndex++, t_IndexBuffer, 10 ) + '.' + extension;
			p_OutputFile = new IOclass( OutputFilename.c_str(), MODE_CREATE );
			if ( p_OutputFile->getFilePointer() != 0 )
			{
				fprintf( stderr, "Could not open file '%s' for writing\n", OutputFilename.c_str() );
				return -3;
			}
		}

		p_InputFile.setFilePointer( ElementLevel0->GetElementPosition() );
		i_DataSize = i_SizeToCopy = ElementLevel0->ElementSize(true);
		
		// copy data
		while ( i_SizeToCopy )
		{
			if ( i_SizeToCopy >= 8*1024 )
			{
				p_OutputFile->write( ReadBuff, p_InputFile.read( ReadBuff, 8*1024 ) );
				i_SizeToCopy -= 8*1024;
			}
			else
			{
				p_OutputFile->write( ReadBuff, p_InputFile.read( ReadBuff, i_SizeToCopy ) );
				i_SizeToCopy = 0;
			}
		}

		p_InputFile.setFilePointer( ElementLevel0->GetElementPosition() + i_DataSize );

		ElementLevel0 = aStream.FindNextElement( KaxMatroska_Context, i_Level, 0xFFFFFFFFL, false, 1 );
	}

	delete ReadBuff;

	return 0;
}
