/*
 * Copyright (c) 2003 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Copyright (c) 1999-2003 Apple Computer, Inc.  All Rights Reserved.
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/* ====================================================================
 * Copyright (c) 1995-1999 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

/* Base64 encoder/decoder. Originally Apache file ap_base64.c
 */

#pragma once
// #include <cstring>
// #include <sstream>

class Base64
{
public:
    static size_t decode(const char *bufcoded, char* bufplain = nullptr, size_t limit = 0)
    {
        static const uint8_t pr2six[256] =
        {
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
            64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
            64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
        };
        const uint8_t* bufin = (const uint8_t*) bufcoded;
        size_t nprbytes;
        if (limit != 0)
        {
            while (limit != 0 && pr2six[*(bufin++)] <= 63)
                limit--;
            nprbytes = (bufin - (const uint8_t*) bufcoded) - ((limit > 0) ? 1 : 0);
        }
        else
        {
            while (pr2six[*(bufin++)] <= 63)
                ;
            nprbytes = (bufin - (const uint8_t*) bufcoded) - 1;
        }
        size_t nbytesdecoded = ((nprbytes + 3) / 4) * 3;
        if (bufplain == nullptr)
            return nbytesdecoded + 1;

        uint8_t* bufout = (uint8_t*) bufplain;
        bufin = (const uint8_t*) bufcoded;

        while (nprbytes > 4)
        {
            *(bufout++) =
                (uint8_t)(pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
            *(bufout++) =
                (uint8_t)(pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
            *(bufout++) =
                (uint8_t)(pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
            bufin += 4;
            nprbytes -= 4;
        }

        if (nprbytes > 1) {
            *(bufout++) =
                (uint8_t)(pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        }
        if (nprbytes > 2) {
            *(bufout++) =
                (uint8_t)(pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        }
        if (nprbytes > 3) {
            *(bufout++) =
                (uint8_t)(pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        }
        *(bufout++) = '\0';
        nbytesdecoded -= (4 - nprbytes) & 3;
        return nbytesdecoded;
    }

    static size_t decodeLength(const char* string)
    {
        return decode(string, nullptr);
    }

    static size_t encode(const char* string, size_t len = 0, char* encoded = nullptr)
    {
        if (len == 0)
            len = strlen(string);
        if (len < 2)
            return 0;
        if (encoded == nullptr)
            return ((len + 2) / 3 * 4) + 1;
        static const char basis_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        char* p = encoded;
        size_t i;
        for (i = 0; i < len - 2; i += 3) {
            *p++ = basis_64[(string[i] >> 2) & 0x3F];
            *p++ = basis_64[((string[i] & 0x3) << 4) |
                            ((int) (string[i + 1] & 0xF0) >> 4)];
            *p++ = basis_64[((string[i + 1] & 0xF) << 2) |
                            ((int) (string[i + 2] & 0xC0) >> 6)];
            *p++ = basis_64[string[i + 2] & 0x3F];
        }
        if (i < len)
        {
            *p++ = basis_64[(string[i] >> 2) & 0x3F];
            if (i == (len - 1)) {
                *p++ = basis_64[((string[i] & 0x3) << 4)];
                *p++ = '=';
            }
            else
            {
                *p++ = basis_64[((string[i] & 0x3) << 4) |
                            ((int) (string[i + 1] & 0xF0) >> 4)];
                *p++ = basis_64[((string[i + 1] & 0xF) << 2)];
            }
            *p++ = '=';
        }
        *p++ = '\0';
        return p - encoded;
    }

    static size_t encodeLength(const char* string, size_t len = 0)
    {
        return encode(string, len);
    }

    // std::string encode(const std::string& s)
    // {
    //     int len = encodeLength(s.c_str());
    //     char* buf = (char*)malloc(len);
    //     memset(buf, 0, len);
    //     encode(s.c_str(), s.length(), buf);
    //     std::string result(buf);
    //     free(buf);
    //     return result;
    // }

    // std::string decode(const std::string& s)
    // {
    //     const char* cstr = s.c_str();
    //     int len = decodeLength(cstr);
    //     char* buf = (char*)malloc(len);
    //     memset(buf, 0, len);
    //     decode(cstr, buf);

    //     std::ostringstream out;
    //     for (int i = 0; i < len; i++)
    //     {
    //         out << (char) buf[i];
    //     }
    //     std::string result = out.str();
    //     free(buf);
    //     return result;
    // }
};
