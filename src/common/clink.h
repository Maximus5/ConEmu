/* Copyright (c) 2012 Martin Ridgers
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef CLINK_H
#define CLINK_H

#undef CLINK_API
#if defined(CLINK_DLL_BUILD)
#   define CLINK_API            __declspec(dllexport)
#else
#   define CLINK_API            __declspec(dllimport)
#endif

#define PROTOTYPE(x) void x(    \
    const wchar_t* prompt,      \
    wchar_t* result,            \
    unsigned result_size        \
);

PROTOTYPE(CLINK_API call_readline)
typedef PROTOTYPE((*call_readline_t))

#undef PROTOTYPE

#define AS_STR(x)               AS_STR_IMPL(x)
#define AS_STR_IMPL(x)          #x

#define CLINK_DLL_NAME          "clink_dll_" AS_STR(PLATFORM) ".dll"

#endif // CLINK_H
