/*
    libcoremeta - A library of useful C functions and ADT's
    Copyright (C) 2000-2006 B. Augestad, boa@metasystems.no 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef RFC1738_H
#define RFC1738_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
#endif

/* @file
 * Implements encode and decode() functions for HTTP URL arguments
 * according to RFC 1738.
 *
 * The short and simple rule is that if a character is A-Za-z0-9
 * it is not encoded, anything else is encoded. The character is
 * encoded as a two digit hex number, prefixed with %. 
 *
 * Issues: This version decodes %00, which maps to '\0'.
 * Don't know if that's a serious issue or not(security).
 */

/* Changed to work on memory buffers instead of just strings */
size_t rfc1738_encode(char* dest, size_t cbdest, const char* src, size_t cbsrc);
size_t rfc1738_decode(char* dest, size_t cbdest, const char* src, size_t cbsrc);

/* Works for zero terminated strings, the result will be zero terminated too .
 * Returns length of dest string, excluding null character.
 */
size_t rfc1738_encode_string(char* dest, size_t cbdest, const char* src);
size_t rfc1738_decode_string(char* dest, size_t cbdest, const char* src);

#ifdef __cplusplus
}
#endif

#endif

