////////////////////////////////////////////////////////////////////////////////////////
//
//      2021 Georgi Angelov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////////////////

#ifndef _VFS_LFS_H_
#define _VFS_LFS_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "VFS.h"

    //////////////////////////////////////////////////////////////////////////////////////

#ifndef LFS_RAM_LETTER
#define LFS_RAM_LETTER "R:"
#endif

#ifndef LFS_RAM_BLOCK_COUNT
#define LFS_RAM_BLOCK_COUNT 128 /* Size 16k ( 128 x 128) */
#endif

#ifndef LFS_RAM_BLOCK_SIZE
#define LFS_RAM_BLOCK_SIZE 128
#endif

#ifndef LFS_RAM_PROG_SIZE
#define LFS_RAM_PROG_SIZE 16
#endif

#ifndef LFS_RAM_READ_SIZE
#define LFS_RAM_READ_SIZE 16
#endif

#ifndef LFS_RAM_CACHE_SIZE
#define LFS_RAM_CACHE_SIZE 16
#endif

#ifndef LFS_RAM_LOOKAHEAD_SIZE
#define LFS_RAM_LOOKAHEAD_SIZE 16
#endif

#ifndef LFS_RAM_BLOCK_CYCLES
#define LFS_RAM_BLOCK_CYCLES 1000
#endif

    //////////////////////////////////////////////////////////////////////////////////////

#ifndef LFS_ROM_LETTER
#define LFS_ROM_LETTER "F:"
#endif

#ifndef LFS_ROM_BLOCK_COUNT
#define LFS_ROM_BLOCK_COUNT 64 /* Size 256k ( 64 x 4096 )    */
#endif

#ifndef LFS_ROM_BLOCK_SIZE
#define LFS_ROM_BLOCK_SIZE 4096 /* DO NOT EDIT - ERASE PAGE   */
#endif

#ifndef LFS_ROM_PROG_SIZE
#define LFS_ROM_PROG_SIZE 256 /* DO NOT EDIT - WRITE SECTOR */
#endif

#ifndef LFS_ROM_READ_SIZE
#define LFS_ROM_READ_SIZE 256
#endif

#ifndef LFS_ROM_CACHE_SIZE
#define LFS_ROM_CACHE_SIZE 256
#endif

#ifndef LFS_ROM_LOOKAHEAD_SIZE
#define LFS_ROM_LOOKAHEAD_SIZE 64
#endif

#ifndef LFS_ROM_BLOCK_CYCLES
#define LFS_ROM_BLOCK_CYCLES 1000
#endif

    //#define LFS_ROM_PRE_FORMAT /* ONLY FOR TEST */

    //////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
#endif // _VFS_LFS_H_