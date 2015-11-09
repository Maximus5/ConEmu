#pragma once
/*
 * v0.1 color&border styles
 * v0.2 interface formal description, unified with ConEmu
 * v0.3 extra information
 */

/**
 * Extended console AnnotationInfo (aka truemod) interprocess interactions agreements:
 *
 *  - Shared buffer is always created by a Host (console manager) process.
 *
 *  - Hosted (child) process should check for Share Name availability and
 *    rollback to basic console behaviour in case Share Name is not available.
 *
 *  - It is responsibility of a Host process to ensure Share is created before
 *    child process is actually requires it
 *
 *  - It is responsibility of a Host process to detect and create additional
 *    shares in case child process creates extra consoles on its own.
 *
 */

/**
 * Share name to search for
 * Two replacements:
 *   %x sizeof(AnnotationInfo) - compatibility/versioning field.
 *   %x console window handle
 */
#define AnnotationShareNameA "Console_annotationInfo_%x_%x"
#define AnnotationShareName L"Console_annotationInfo_%x_%x"

/**
 * Header structure, located at offset <0> within shared annotation buffer.
 */
struct AnnotationHeader
{
	/**
	 * Size of this header. annotation buffer at offset <struct_size>
	 * contains <AnnotationInfo> structures, one after another
	 */
	int struct_size;
	/**
	 * Number of allocated <AnnotationInfo> elements within shared buffer.
	 * Clients should not access buffer outside of this size.
	 */
	int bufferSize;
	/**
	 * Real console is in update phase (text and attributes).
	 * Clients must set this field to TRUE before WriteConsoleOutput and
	 * writing to annotation buffer.
	 * When modifications are completes - clients must
	 * increment modifiedCounter and set this field to FALSE.
	 */
	int locked;
	/**
	 * Flush counter of annotation buffer.
	 * Clients must increment this value, when it completes buffer modification.
	 * Host may use it to understand when to update annotation buffer.
	 */
	unsigned int flushCounter;
	/**
	 * Dimension of the buffer.
	 * Clients must update this when they commit changes to buffer
	 * to avoid color breaks on console resize.
	 */
	int width, height;
	/**
	 * First processed line of the buffer.
	 * Default is ‘-1’, that means the buffer addressed **only**
	 * the lowest part of the console (Far Manager compatible).
	 */
	int lastDirtyLine;
};

/**
 * Annotation Information for each character on the screen.
 *
 */
struct AnnotationInfo
{
	//AnnotationInfo()
	//{
	//  for (int i = 0; i < sizeof(raw)/sizeof(raw[0]); i++)
	//    raw[i] = 0;
	//}
	union
	{
		struct
		{
			/**
			 * Background color
			 * 24bit
			 */
			unsigned int bk_color :24;
			/**
			 * Foreground color
			 * 48bit
			 */
			unsigned int fg_color :24;
			/**
			 * Validity indicators;
			 * 50bit
			 */
			int bk_valid :1;
			int fg_valid :1;

			/**
			 * Custom border over the character position.
			 *
			 * bit 0 - left, bit 1 - top, bit 2 - right, bit 3 - bottom
			 * 54bit
			 */
			int border_visible :4;
			/**
			 * When border within character has angles (f.e. left+top),
			 * console server may choose to draw it in some nice way (rounded, etc)
			 *
			 * 0 - no border, 1 - 1 pixel
			 * 86bit
			 */
			unsigned int border_style :8;
			unsigned int border_color :24;

			/**
			 * Extra character style attributes. See AI_STYLE_* defines
			 * 102bit
			 */
			unsigned int style :16;

			#define AI_STYLE_BOLD          0x0001
			#define AI_STYLE_ITALIC        0x0002
			#define AI_STYLE_UNDERLINE     0x0004
			#define AI_STYLE_STRIKEOUT     0x0008
			#define AI_STYLE_SUPERSCRIPT   0x0010
			#define AI_STYLE_SUBSCRIPT     0x0020
			#define AI_STYLE_SHADOW        0x0040
			#define AI_STYLE_SMALL_CAPS    0x0080
			#define AI_STYLE_ALL_CAPS      0x0100
			#define AI_STYLE_TAB_LEADING   0x0200 // To be able to know if application wrote \t in the cell
			#define AI_STYLE_TAB_TRAILING  0x0400 // Marks end cell of \t spaces autoexpansion (may be the same as AI_STYLE_TAB_LEADING)
			#define AI_STYLE_ACHAR         0x1000 // If client filled the wchr field (MULTIBYTE)
			#define AI_STYLE_WCHAR         0x2000 // If client filled the wchr field (CP1200)

			/**
			 * Alignment field
			 * 128bit
			 */
			unsigned int reserved_1 :26;

			/**
			 * ConEmu internal ID for console row
			 * It must be set by client at the first cell of row
			 * Use functions from ConsoleMixAttr.h
			 * 144bit
			 */
			unsigned int row_id :16;

			/**
			 * It's easier to read all data from one place,
			 * so let wchar_t be here togather with colors
			 * 160bit
			 */
			unsigned int wchr :16;

			/**
			 * Total allowed 256bit
			 */
		};

		// AnnotationInfo size is 8*DWORD
		int raw[8];
	};
};

/*
Copyright (c) 2010 Igor Russkih
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
Copyright (c) 2015 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
