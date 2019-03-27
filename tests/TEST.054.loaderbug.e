Byte^ header = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam consequat dolor turpis, quis volutpat est hendrerit a. Quisque auctor nibh in arcu hendrerit ornare. Maecenas eleifend tellus purus, quis condimentum libero aliquam in. Sed sollicitudin odio elit, malesuada interdum erat ultrices condimentum. Quisque felis nunc, facilisis id malesuada vitae, interdum a elit. Sed vel nulla quis risus maximus molestie. Duis bibendum, justo ac lobortis sodales, ligula massa cursus leo, sit amet rhoncus dui metus.enki"

struct raw InannaHeader
    Uint8[4] magic

Byte^ ptr = header + 512

InannaHeader^ ih = cast(ptr, InannaHeader^)
Uint ret = ih^.magic[0]
return ret


