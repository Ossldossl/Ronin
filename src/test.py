
value = 0
fst_byte = 0xED
snd_byte = 0x95
trd_byte = 0x9C

mask_1 = 0xf
mask_2 = (-0xc0) - 1

value = fst_byte & mask_1
print("1: ", value, bin(value), hex(value))

value <<= 6;
print("2: ", value, bin(value), hex(value))

value += snd_byte & mask_2
print("3: ", value, bin(value), hex(value))

value <<= 6
print("4: ", value, bin(value), hex(value))

value += trd_byte & mask_2
print("5: ", value, bin(value), hex(value))


