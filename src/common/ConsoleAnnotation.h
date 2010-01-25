#pragma once
/* v0.1 color&border styles
 */
#define AnnotationShareNameA "Console2_annotationInfo_%d_%d"

#define AnnotationShareName L"Console2_annotationInfo_%d_%d"


/**
 * Annotation Information for each character on the screen.
 * 
 */
struct AnnotationInfo
{
    AnnotationInfo()
    {
      for (int i = 0; i < sizeof(raw)/sizeof(raw[0]); i++)
        raw[i] = 0;
    }
    union{
      struct{
        int bk_color;
        int bk_valid :1;

        int fg_color;
        int fg_valid :1;

        /* bit 0 - left, bit 1 - top, bit 2 - right, bit 3 - bottom */
        int border_visible :4;
        /* 0 - no border, 1 - i pixel */
        int border_style :4;
        int border_color :24;
      };
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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