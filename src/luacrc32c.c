/*
  --------------------------------------------------------------------------------
  MIT License

  luacrc32c - Copyright (c) 2026 Kritzel Kratzel for OneLuaPro.

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in 
  the Software without restriction, including without limitation the rights to 
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
  the Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all 
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
  --------------------------------------------------------------------------------

  luacrc32c is a Lua interface for Google's CRC32C library available at
  https://github.com/google/crc32c. CRC32C is also known as:
  CRC-32/ISCSI
  CRC-32/BASE91-C
  CRC-32/CASTAGNOLI
  CRC-32/INTERLAKEN
  CRC-32C

  --------------------------------------------------------------------------------
*/

#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <crc32c/crc32c.h>

#define LUACRC32C_VERSION "luacrc32c 1.0"

static inline uint16_t htole16(uint16_t x) {
#if defined(_WIN32)
  return x; 		// Windows is already Little Endian
#else
  return htole16(x); 	// Linux/BSD: real function
#endif
}

static inline uint32_t htole32(uint32_t x) {
#if defined(_WIN32)
  return x; 		// Windows is already Little Endian
#else
  return htole32(x); 	// Linux/BSD: real function
#endif
}

static inline uint64_t htole64(uint64_t x) {
#if defined(_WIN32)
  return x; 		// Windows is already Little Endian
#else
  return htole64(x); 	// Linux/BSD: real function
#endif
}

static int doStuff(lua_State *L, uint32_t crc) {
  if (lua_gettop(L) == 2 && lua_type(L,1) == LUA_TNUMBER && lua_type(L,2) == LUA_TTABLE) {
    // ---> Signature 1: luacrc32c.value(elemSize, {dataTable})
    // Desired elemSize in Bytes
    lua_Integer elemSize = luaL_checkinteger(L, 1);
    
    // Check elemSize as 1, 2, 4, 8
    if (elemSize <= 0 || (elemSize & (elemSize - 1)) != 0 || elemSize > sizeof(lua_Integer)) {
      // (v & (v - 1)) == 0 checks whether elemSize is a power of 2
      lua_pushnil(L);	// no result
      lua_pushstring(L,"elemSize is not 1, 2, 4, or 8");
      return 2;
    }

    // Serialization buffer
    uint8_t raw[sizeof(lua_Integer)];

    // Traverse the full table and calculate CRC successively
    lua_pushnil(L);	// Push first dummy key
    while (lua_next(L,2) != 0) {
      // key at index -2, natural Lua indexing requires key as integer
      if (!lua_isinteger(L,-2)) {
	lua_pushnil(L);	// no result
	lua_pushstring(L,"Table key is not an Integer.");
	return 2;
      }
      // key must be a positive number
      if (lua_tointeger(L,-2) < 1) {
	lua_pushnil(L);	// no result
	lua_pushstring(L,"Table key is not greater than zero.");
	return 2;
      }
      // value at index -1, check if value is integer
      if (!lua_isinteger(L,-1)) {
	lua_pushnil(L);	// no result
	lua_pushstring(L,"Table value is not an Integer.");
	return 2;
      }
      lua_Integer v = lua_tointeger(L,-1);
      // pop the value, keep the key for the next iteration
      lua_pop(L,1);

      // Check and prepare. elemSize counts in Bytes
      if (elemSize == 1) {
	if (v < INT8_MIN || v > UINT8_MAX) {
	  lua_pushnil(L);	// no result
	  lua_pushstring(L,"Table value exceeds selected 1 byte elemSize.");
	  return 2;
	}
	raw[0] = (uint8_t)v;
      }
      else if (elemSize == 2) {
	if (v < INT16_MIN || v > UINT16_MAX) {
	  lua_pushnil(L);	// no result
	  lua_pushstring(L,"Table value exceeds selected 2 byte elemSize.");
	  return 2;
	}
	uint16_t tmp = htole16((uint16_t)v);
	memcpy(raw, &tmp, sizeof(tmp));
      }
      else if (elemSize == 4) {
	if (v < INT32_MIN || v > UINT32_MAX) {
	  lua_pushnil(L);	// no result
	  lua_pushstring(L,"Table value exceeds selected 4 byte elemSize.");
	  return 2;
	}
	uint32_t tmp = htole32((uint32_t)v);
	memcpy(raw, &tmp, sizeof(tmp));
      }
      else if (elemSize == 8) {
	if (v < INT64_MIN || v > UINT64_MAX) {
	  // Note: this bail-out will never be reached, because there is no integer
	  // number larger than UINT64_MAX or smaller than INT64_MIN as long as we
	  // deal with 8 byte signed/unsigend integer datatypes in Lua.
	  //
	  // Lua 5.4.8  Copyright (C) 1994-2025 Lua.org, PUC-Rio
	  // > print(math.mininteger)
	  // -9223372036854775808	<- this is INT64_MIN = -2**63
	  // > print(math.maxinteger)
	  //  9223372036854775807	<- this is INT64_MAX = 2**63 - 1
	  //
	  // 18446744073709551615	<- this is UINT64_MAX = 2**64 - 1
	  lua_pushnil(L);	// no result
	  lua_pushstring(L,"Table value exceeds selected 8 byte elemSize.");
	  return 2;
	}
	uint64_t tmp = htole64((uint64_t)v);
	memcpy(raw, &tmp, sizeof(tmp));
      }
      else {
	lua_pushnil(L);	// no result
	lua_pushstring(L,"Impossible value for elemSize.");
	return 2;
      }

      // Calculate CRC32
      crc = crc32c_extend(crc, raw, (size_t)elemSize);
    }     
  }
  else if (lua_gettop(L) == 1 && lua_type(L,1) == LUA_TSTRING) {
    // ---> Signature 2: luacrc32c.value(string)
    size_t len;
    const char *str = luaL_checklstring(L,1,&len);
    crc = crc32c_extend(crc, (const uint8_t*)str, len);
  }
  else {
    return luaL_error(L,"Wrong signature - neither (elemSize, {dataTable}) nor (string).");
  }
  
  // return result and status
  lua_pushinteger(L,crc);	// Return CRC32
  lua_pushnil(L);		// No error message
  return 2;  
}

static uint32_t getCrcFromStack(lua_State *L, int arg) {
  if (!lua_isinteger(L,arg)) {
    lua_pushnil(L);	// no result
    lua_pushstring(L,"CRC value is not an Integer.");
    return 2;
  }
  // read crc value from stack
  uint32_t crc = (uint32_t)luaL_checkinteger (L, arg);
  if (crc < 0 || crc > UINT32_MAX) {
    // given crc out of range
    lua_pushnil(L);	// no result
    lua_pushstring(L,"CRC value is out of range.");
    return 2;      
  }
  // pop crc value from stack
  lua_pop(L,1);
  // return crc
  return crc;
}

static int luacrc32c_value(lua_State *L) {
  // crc start value is always 0 here
  return doStuff(L,0);
}

static int luacrc32c_extend(lua_State *L) {
  // crc start value comes as input from argument list
  if (lua_gettop(L) == 3 && lua_type(L,1) == LUA_TNUMBER && lua_type(L,2) == LUA_TTABLE
      && lua_type(L,3) == LUA_TNUMBER) {
    // ---> Signature 1: luacrc32c.value(elemSize, {dataTable}, crc)
    return doStuff(L,getCrcFromStack(L,3));
  }
  else if (lua_gettop(L) == 2 && lua_type(L,1) == LUA_TSTRING && lua_type(L,2) == LUA_TNUMBER) {
    // ---> Signature 2: luacrc32c.value(string, crc)
    return doStuff(L,getCrcFromStack(L,2));
  }
  else {
    return luaL_error(L,"Wrong signature - neither (elemSize, {dataTable}, crc) nor (string, crc).");
  }
}

static const struct luaL_Reg luacrc32c_metamethods [] = {
  {"__call", luacrc32c_value},
  {"__call", luacrc32c_extend},
  {NULL, NULL}
};

static const struct luaL_Reg luacrc32c_funcs [] = {
  {"value", luacrc32c_value},
  {"extend", luacrc32c_extend},
  {NULL, NULL}
};

LUALIB_API int luaopen_luacrc32c(lua_State *L){
  luaL_newlib(L, luacrc32c_funcs);
  luaL_newlib(L, luacrc32c_metamethods);
  lua_setmetatable(L, -2);
  lua_pushliteral(L,LUACRC32C_VERSION);
  lua_setfield(L,-2,"_VERSION");
  return 1;
}

