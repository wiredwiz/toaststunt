/******************************************************************************
 * Copyright (c) 2002, Mathieu Fenniak
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

#include "utf8.h"
#include "storage.h"
#include "log.h"

#define ONEMASK ((size_t)(-1) / 0xFF)
    
    size_t
utf8_strlen(const char * _s)
{
	const char * s;
	size_t count = 0;
	size_t u;
	unsigned char b;

	/* Handle any initial misaligned bytes. */
	for (s = _s; (uintptr_t)(s) & (sizeof(size_t) - 1); s++) {
		b = *s;

		/* Exit if we hit a zero byte. */
		if (b == '\0')
			goto done;

		/* Is this byte NOT the first byte of a character? */
		count += (b >> 7) & ((~b) >> 6);
	}

	/* Handle complete blocks. */
	for (; ; s += sizeof(size_t)) {
		/* Prefetch 256 bytes ahead. */
		__builtin_prefetch(&s[256], 0, 0);

		/* Grab 4 or 8 bytes of UTF-8 data. */
		u = *(size_t *)(s);

		/* Exit the loop if there are any zero bytes. */
		if ((u - ONEMASK) & (~u) & (ONEMASK * 0x80))
			break;

		/* Count bytes which are NOT the first byte of a character. */
		u = ((u & (ONEMASK * 0x80)) >> 7) & ((~u) >> 6);
		count += (u * ONEMASK) >> ((sizeof(size_t) - 1) * 8);
	}

	/* Take care of any left-over bytes. */
	for (; ; s++) {
		b = *s;

		/* Exit if we hit a zero byte. */
		if (b == '\0')
			break;

		/* Is this byte NOT the first byte of a character? */
		count += (b >> 7) & ((~b) >> 6);
	}

done:
	return ((s - _s) - count);
}



const char* utf8_index(const char* str, Num index)
{
    return utf8_substr(str, index, index);
}

const char* utf8_substr(const char* str, Num lower, Num upper)
{
    char* p = (char*)str;
    char* start = NULL;
    char* newstr = NULL;
    int n = 0;
    int state = 0;
    int numbytes = 0;

    /* MOO indexes are one based, C indexes are 0 based.. convert.
     */
    lower--; upper--;
    
    /* to ensure that this code doesn't miss the boundry of the string,
     * we have to iterate more slowly than we could if we just jumped
     * ahead when we found the number of bytes in a character.
     */
    while (*p != '\0' && n < lower)
    {
	if (state == 0)
	{
	    state = utf8_numbytes(*p) - 1;
	    
	    if (state == 0)
	    {
		n++;
	    }
	    else if (state == -2) /* -1 error val of utf8_numbytes minus one */
	    {
		/* just ignore this weird character */
		state = 0;
	    }
	}
	else
	{
	    /* There is no check here for the validity of the UTF-8 bytes
	     * because I can't think of an appropriate failsafe behaviour
	     * anyways.
	     */

	    if (0 == --state)
	    {
		n++;
	    }
	}

	p++;
    }

    if (*p == '\0')
    {
	return str_dup("");
    }

    start = p;
    state = 0;
    
    while (*p != '\0' && n <= upper)
    {
	if (state == 0)
	{
	    state = utf8_numbytes(*p) - 1;
	    
	    if (state == 0)
	    {
		n++;
	    }
	    else if (state == -2) /* -1 error val of utf8_numbytes minus one */
	    {
		/* just ignore this weird character */
		state = 0;
	    }
	}
	else
	{
	    /* There is no check here for the validity of the UTF-8 bytes
	     * because I can't think of an appropriate failsafe behaviour
	     * anyways.
	     */

	    if (0 == --state)
	    {
		n++;
	    }
	}

	p++;
	numbytes++;
    }

    if (numbytes == -1)
    {
	return NULL;
    }

    newstr = (char*)mymalloc(numbytes + 1, M_STRING);
    memcpy(newstr, start, numbytes);
    newstr[numbytes] = '\0';

    return newstr;
}

const char* utf8_copyandset(const char* lhs, Num index, const char* rhs)
{    
    return utf8_strrangeset(lhs, index, index, rhs);
}

const char* utf8_strrangeset(const char* lhs, Num from, Num to, const char* rhs)
{
    char* p = (char*)lhs;
    char* newstr = NULL;
    int n = 0;
    int state = 0;
    int startbytes = 0;

    /* MOO indexes are one based, C indexes are 0 based.. convert.
     */
    from--; to--;

    /* to ensure that this code doesn't miss the boundry of the string,
     * we have to iterate more slowly than we could if we just jumped
     * ahead when we found the number of bytes in a character.
     */
    while (*p != '\0' && n <= to)
    {
	if (state == 0)
	{
	    state = utf8_numbytes(*p) - 1;
	    
	    if (state == 0)
	    {
		n++;
	    }
	    else if (state == -2) /* -1 error val of utf8_numbytes minus one */
	    {
		/* just ignore this weird character */
		state = 0;
	    }
	}
	else
	{
	    /* There is no check here for the validity of the UTF-8 bytes
	     * because I can't think of an appropriate failsafe behaviour
	     * anyways.
	     */

	    if (0 == --state)
	    {
		n++;
	    }
	}

	p++;

	if (n <= from)
	{
	    startbytes++;
	}
    }

    newstr = (char*)mymalloc(startbytes + strlen(rhs) + strlen(p) + 1, M_STRING);
    memcpy(newstr, lhs, startbytes);
    memcpy(newstr + startbytes, rhs, strlen(rhs));
    if (*p != '\0')
	memcpy(newstr + startbytes + strlen(rhs), p, strlen(p));
    newstr[startbytes + strlen(rhs) + strlen(p)] = '\0';

    return newstr;
}

Num utf8_strindex(const char* big, const char* small, int case_matters)
{
    char *p = (char*)big;
    int state = 0;
    int n = 0;

    while (*p != '\0')
    {
	if (state == 0)
	{
	    int match = 1;
	    char* p2 = p;
	    char* j = (char*)small;

	    while (*j != '\0' && *p2 != '\0')
	    {
		if (*j != *p2)
		{
		    /* ASCII latin characters will have a check if case doesn't matter. */
		    if (!case_matters && tolower(*j) == tolower(*p2))
		    {
			/* They're the same if you don't account for case. Match continues. */
		    }
		    else
		    {
			match = 0;
			break;
		    }
		}

		p2++;
		j++;
	    }
	    if (*p2 == '\0' && *j != '\0') /* if one hit '\0' and the other didn't, match would be 1 with no match. */
	    {
		match = 0;
	    }
	    if (match)
	    {
		return n + 1;
	    }
						
	    state = utf8_numbytes(*p) - 1;
	    
	    if (state == 0)
	    {
		n++;
	    }
	    else if (state == -2) /* -1 error val of utf8_numbytes minus one */
	    {
		/* just ignore this weird character */
		state = 0;
	    }
	}
	else
	{
	    /* There is no check here for the validity of the UTF-8 bytes
	     * because I can't think of an appropriate failsafe behaviour
	     * anyways.
	     */

	    if (0 == --state)
	    {
		n++;
	    }
	}

	p++;
    }

    return 0;
}

Num utf8_strrindex(const char* big, const char* small, int case_matters)
{
    char *p = (char*)(big + strlen(big) - 1);
    int n = 1;

    while (p >= big)
    {
	if ((*p & 0x80) == 0x80)
	{
	    /* second or more byte of UTF-8 char.
	     * Ignore.
	     */
	}
	else
	{
	    int match = 1;
	    char* p2 = p;
	    char* j = (char*)small;

	    while (*j != '\0' && *p2 != '\0')
	    {
		if (*j != *p2)
		{
		    /* ASCII latin characters will have a check if case doesn't matter. */
		    if (!case_matters && tolower(*j) == tolower(*p2))
		    {
			/* They're the same if you don't account for case. Match continues. */
		    }
		    else
		    {
			match = 0;
			break;
		    }
		}

		p2++;
		j++;
	    }
	    if (*p2 == '\0' && *j != '\0') /* if one hit '\0' and the other didn't, match would be 1 with no match. */
	    {
		match = 0;
	    }
	    if (match) 
	    {
		char* bigcopy = str_dup(big);
		bigcopy[p - big] = '\0';
		n = utf8_strlen(bigcopy);
		bigcopy[p - big] = *p; // not required, but removes the null character so the length is right again.
		free_str(bigcopy);
		return n + 1; // Convert to 1-based index.
	    }
	
	}
	p--;
    }

    return 0;
}

int utf8_numbytes(char c)
{
    if (0 == (0x80 & c))
    {
	/* ascii */
	return 1;
    }
    else if (0xC0 == (0xE0 & c))
    {
	/* 2 byte */
	return 2;
    }
    else if (0xE0 == (0xF0 & c))
    {
	/* 3 byte */
	return 3;
    }
    else if (0xF0 == (0xF8 & c))
    {
	/* 4 byte */
	return 4;
    }
    else if (0xF8 == (0xFC & c))
    {
	/* 5 byte */
	return 5;
    }
    else if (0xFC == (0xFE & c))
    {
	/* 6 byte */
	return 6;
    }

    return -1;
}

int utf8_convert_index(int nRealIdx, const char* pStr)
{
    char* p = (char*)pStr;
    int nRealCnt = 1;
    int nFakeCnt = 1;

    int state = 0;

    while (*p != '\0' && nRealCnt < nRealIdx)
    {
        if (state == 0)
        {
            state = utf8_numbytes(*p) - 1;
            if (state == 0)
            {
                nFakeCnt++;
            }
            else if (state == -2)
            {
                /* ignore weird character */
                state = 0;
            }
        }
        else
        {
            if (0 == --state)
            {
                nFakeCnt++;
            }
        }

        p++;
        nRealCnt++;
    }

    return nFakeCnt;
}
