/*
  binary.h - Definitions for binary constants
  Deprecated -- use 0b binary literals instead
  Copyright (c) 2006 David A. Mellis.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef Binary_h
#define Binary_h

/* If supported, 0b binary literals are preferable to these constants.
 * In that case, warn the user about these being deprecated (if possible). */
#if __cplusplus >= 201402L
  /* C++14 introduces binary literals; C++11 introduces [[deprecated()]] */
  #define DEPRECATED(x) [[deprecated("use " #x " instead")]]
#elif __GNUC__ >= 6
  /* GCC 4.3 supports binary literals; GCC 6 supports __deprecated__ on enums*/
  #define DEPRECATED(x) __attribute__ ((__deprecated__ ("use " #x " instead")))
#else
  /* binary literals not supported, or "deprecated" warning not displayable */
  #define DEPRECATED(x)
#endif

#undef DEPRECATED

#endif
