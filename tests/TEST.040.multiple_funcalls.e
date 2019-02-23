def zargle() Uint
    write("Foo\n")

def frobnitz() Uint
    zargle()
    
def bar() Uint
    frobnitz()

def foo() Uint
    bar()
    
foo()
return 0

