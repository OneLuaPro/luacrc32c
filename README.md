# luacrc32c
`luacrc32c` is a Lua wrapper for Google's CRC-32C implementation available at https://github.com/google/crc32c. CRC-32C is also known as: 

- CRC-32/ISCSI
- CRC-32/BASE91-C
- CRC-32/CASTAGNOLI
- CRC-32/INTERLAKEN

CRC-32C is specified as the CRC that uses the iSCSI polynomial in [RFC 3720](https://tools.ietf.org/html/rfc3720#section-12.1). The polynomial was introduced by G. Castagnoli, S. Braeuer and M. Herrmann. 

Notes:

- CRC-32C is not CRC‑32 (IEEE 802.3)
- luacrc23c is currently available for the Windows operating system only. It is made for the [OneLuaPro](https://github.com/OneLuaPro) ecosystem.

## API

`luacrc32c` API comprises two functions, which mimic the underlying `crc32c.lib` interface. In addition, the wrapper version can be queried. Note that positive or negative integer numbers are always processed in **little-endian** order.

### luacrc32c.value()

```lua
-- Signature 1
-- elemSize: assumed byte width of integer numbers in dataTable (1, 2, 4, or 8)
-- dataTable: table of positive or negative integer numbers in numerical 1-based order
local crc, errmsg = luacrc32c.value(elemSize, {dataTable})
-- On success: crc = calculated checksum; errmsg = nil
-- On failure: crc = nil; errmsg = detailed error message

-- Signature 2:
-- string: Lua-string
local crc, errmsg = luacrc32c.value(string)
-- On success: crc = calculated checksum; errmsg = nil
-- On failure: crc = nil; errmsg = detailed error message
```

### luacrc32c.extend()

```lua
-- Signature 1
-- elemSize: assumed byte width of integer numbers in dataTable (1, 2, 4, or 8)
-- dataTable: table of positive or negative integer numbers in numerical 1-based order
-- precCrc: CRC of previous data or data block
local crc, errmsg = luacrc32c.extend(elemSize, {dataTable}, prevCrc)
-- On success: crc = calculated checksum; errmsg = nil
-- On failure: crc = nil; errmsg = detailed error message

-- Signature 2:
-- string: Lua-string
-- precCrc: CRC of previous string
local crc, errmsg = luacrc32c.extend(string, prevCrc)
-- On success: crc = calculated checksum; errmsg = nil
-- On failure: crc = nil; errmsg = detailed error message
```

### luacrc32c._VERSION

```lua
> print(luacrc32c._VERSION)
luacrc32c 1.0
```

## Examples

Computed CRC values can be verified with online CRC calculators, e.g. https://crccalc.com/. Pay attention to the byte processing order when comparing CRC values.

### CRC on Strings

This example shows how to use both functions with signature #2 on strings. 

```lua
local luacrc32c = require("luacrc32c")

-- CRC-calculation of a single string
local crc = luacrc32c.value("The quick brown fox jumps over the lazy dog.")
-- Yields: 0x190097b3

-- Successive CRC-calculation of several strings
local prev_crc = luacrc32c.value("The quick brown ")
local crc = luacrc32c.extend("fox jumps over the lazy dog.",prev_crc)
-- Also yields: 0x190097b3
```

### CRC on Numbers

CRC calculation on Lua numbers is tricky:

- Lua does not know about fixed-width integer data types like `int32_t`, `uint32_t` and the like.
- Lua saves integer data internally as `Lua_Integer` which is (in most situations) equivalent to `int64_t`.

That means, an integer like `255` (represented as `0xff` ) is saved internally as `0x00000000000000ff`. However, the CRC checksums of `0xff` and `0x00000000000000ff` are not the same and `libcrc.lib` does not care about data type lengths, it simply processes bytes in the given order. 

That's why it is necessary to tell `luacrc32c` about the assumed nature of numbers in a table to be processed via the argument `elemSize`. `luacrc32c` internally checks the consistency between numbers in the array `{dataTable}` and the actual value of `elemSize` and returns an explanative error message, in case a given number exceeds the selected byte width. This works for both positive and negative integer numbers in `{dataTable}`.

Example from [RFC3720](https://www.rfc-editor.org/rfc/rfc3720.html#appendix-B.4):

```
An iSCSI - SCSI Read (10) Command PDU
Byte:        0  1  2  3
   0:       01 c0 00 00
   4:       00 00 00 00
   8:       00 00 00 00
  12:       00 00 00 00
  16:       14 00 00 00
  20:       00 00 04 00
  24:       00 00 00 14
  28:       00 00 00 18
  32:       28 00 00 00
  36:       00 00 00 00
  40:       02 00 00 00
  44:       00 00 00 00

CRC:       56 3a 96 d9
```

The CRC of this data block can be calculated with `luacrc32c` like so:

```lua
local luacrc32c = require("luacrc32c")
local buf32 = {0xc001, 0, 0, 0, 0x14, 0x040000, 0x14000000, 0x18000000, 0x28, 0, 0x02, 0}
local crc, errmsg = luacrc32c.value(4,buf32)
string.format("CRC = 0x%x   Error Message = %s",crc,tostring(errmsg))
-- Returns: CRC = 0xd9963a56   Error Message = nil
```

If the same call to `luacrc32c.value()` would be made with an erroneous value of `elemSize = 2`, an error will be raised:

```lua
> luacrc32c.value(2,buf32)
nil     Table value exceeds selected 2 byte elemSize.
>
```

## License

See https://github.com/OneLuaPro/luacrc32c/blob/master/LICENSE.txt.
