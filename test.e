import sys

def fun4() Uint64
    sys:write("Fifth!\n")
    
def fun() Uint64
    def fun2() Uint64
        def fun3() Uint64
            sys:write("Fource!\n")
            fun4()
        sys:write("Thrice!\n")
        fun3()
    sys:write("Twice!\n")
    fun2()
    
sys:write("Hello world!\n")
fun()












