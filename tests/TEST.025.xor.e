Uint64 ret
Uint64 left
Uint64 right

ret = 0
left = 1
right = 0

if left xor right
    ret = ret + 1

left = 0
right = 1

if left xor right
    ret = ret + 2

left = 1

if left xor right
    ret = ret + 4

left = 0
right = 0

if left xor right
    ret = ret + 8

return ret
