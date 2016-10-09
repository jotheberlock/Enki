def zargle() Uint64
    write("Foo\n")

def frobnitz() Uint64
    zargle()
    
def bar() Uint64
    frobnitz()

def foo() Uint64
    bar()
    
foo()
return 0

