-- luacrc32c test suite
-- From rfc3720 section B.4 (https://www.rfc-editor.org/rfc/rfc3720.html#appendix-B.4)

local luacrc32c = require("luacrc32c")
local crc
print("Testing version:",assert(luacrc32c._VERSION == "luacrc32c 1.0"))

local buf8 = {}
local buf16 = {}
local buf32 = {}
local buf64 = {}
for i=1, 32 do buf8[i] = 0x00 end
for i=1, 16 do buf16[i] = 0x0000 end
for i=1, 8 do buf32[i] = 0x00000000 end
for i=1, 4 do buf64[i] = 0x0000000000000000 end

crc = 0x8a9136aa
print("Testing 32 bytes of zeroes      :",assert( luacrc32c.value(1,buf8) == crc))
print("Testing 16 words of zeroes      :",assert( luacrc32c.value(2,buf16) == crc))
print("Testing 8 double words of zeroes:",assert( luacrc32c.value(4,buf32) == crc))
print("Testing 4 quad words of zeroes  :",assert( luacrc32c.value(8,buf64) == crc))
print()

for i=1, 32 do buf8[i] = 0xff end
for i=1, 16 do buf16[i] = 0xffff end
for i=1, 8 do buf32[i] = 0xffffffff end
for i=1, 4 do buf64[i] = 0xffffffffffffffff end
crc = 0x62a8ab43
print("Testing 32 bytes of ones      :",assert( luacrc32c.value(1,buf8) == crc))
print("Testing 16 words of ones      :",assert( luacrc32c.value(2,buf16) == crc))
print("Testing 8 double words of ones:",assert( luacrc32c.value(4,buf32) == crc))
print("Testing 4 quad words of ones  :",assert( luacrc32c.value(8,buf64) == crc))
print()

-- 32 bytes of incrementing 00..1f: CRC:       4e 79 dd 46
for i=1, 32 do buf8[i] = i-1 end
crc = 0x46dd794e
print("Testing 32 bytes of incrementing 00..1f:",assert( luacrc32c.value(1,buf8) == crc))

-- 32 bytes of decrementing 1f..00: CRC:       5c db 3f 11
for i=1, 32 do buf8[i] = 32-i end
crc = 0x113fdb5c
print("Testing 32 bytes of decrementing 1f..00:",assert( luacrc32c.value(1,buf8) == crc))

-- An iSCSI - SCSI Read (10) Command PDU
-- Byte:        0  1  2  3
--    0:       01 c0 00 00
--    4:       00 00 00 00
--    8:       00 00 00 00
--   12:       00 00 00 00
--   16:       14 00 00 00
--   20:       00 00 04 00
--   24:       00 00 00 14
--   28:       00 00 00 18
--   32:       28 00 00 00
--   36:       00 00 00 00
--   40:       02 00 00 00
--   44:       00 00 00 00

--  CRC:       56 3a 96 d9
local buf32a = {0xc001, 0, 0, 0, 0x14, 0x040000}
local buf32b = {0x14000000, 0x18000000, 0x28, 0, 0x02, 0}
crc = 0xd9963a56
local tmp = assert(luacrc32c.value(4,buf32a))
tmp = assert(luacrc32c.extend(4,buf32b,tmp))
print("Testing .value() & .extend() on iSCSI  :",assert( tmp == crc))
print()

print("Testing empty string                        :",assert( luacrc32c.value("") == 0x0))
print("Testing string in one piece                 :",
      assert( luacrc32c.value("The quick brown fox jumps over the lazy dog.") == 0x190097B3))
local str1 = "The quick brown fox jum"
local str2 = "ps over the lazy dog."
tmp = assert(luacrc32c.value(str1))
tmp = assert(luacrc32c.extend(str2,tmp))
print("Testing string in two pieces                :",assert( tmp == 0x190097B3))
print()


-- boundary test
for _,v in ipairs{1,2,4,8} do
   print(string.format("Testing elemSize = %d bytes:",v))
   -- create hex-number as string then call tonumber()
   local hexString = "0x"
   for _=1,v do
      hexString = hexString .. "ff"
   end
   local maxOkValue =  tonumber(hexString)
   local minOkValue =  -(maxOkValue >> 1) - 1
   local upperNotOkValue = maxOkValue+1
   local lowerNotOkValue = minOkValue-1
   --
   local vals = {upperNotOkValue, maxOkValue, minOkValue, lowerNotOkValue}
   local names = {"upperNotOkValue", "maxOkValue", "minOkValue", "lowerNotOkValue"}
   for key,val in pairs(vals) do
      local retval, errmsg = luacrc32c.value(v,{val})
      local retvalStr
      if retval then
	 retvalStr = string.format("0x%x",retval)
      else
	 retvalStr = "       nil"
      end
      print(string.format("   %-16s = 0x%016x : CRC = %10s : errmsg = %s",
			  names[key],val,retvalStr,errmsg))
   end
   print()
end

print("Testing float number as value (nil + errmsg)        :", luacrc32c.value(1,{2.0}))
print("Testing string as value (nil + errmsg)              :", luacrc32c.value(2,{"foo"}))
print("Testing non-natural key (nil + errmsg)              :", luacrc32c.value(4,{[0]=0x1}))
print("Testing string as key (nil + errmsg)                :", luacrc32c.value(8,{["foo"]=0x1}))



-- https://crccalc.com/
-- crc 0x190097B3
