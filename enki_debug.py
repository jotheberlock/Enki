import gdb
import struct

class Type:

    def __init__(self, nam, siz):
        self.name = nam
        self.size = siz

class Local:

    def __init__(self, nam, tid, off):
        self.name = nam
        self.typeid = tid
        self.offset = off
        
class Function:
    
    def __init__(self, nam, addr, siz):
        self.address = addr
        self.name = nam
        self.size = siz
        self.locals = []
        
    def stackSize(self):
        largest = 0
        for local in self.locals:
            try:
                local_type = types[local.typeid]
                siz = local.offset + local_type.size
            except KeyError:
                print('Type '+str(local.typeid)+' unknown!')
                siz = local.offset + 8
            if siz > largest:
                largest = siz
        return largest
    
types = {}
functions = []

def lookupFunction(addr):
    global functions
    for function in functions:
        if addr >= function.address and addr < (function.address+function.size):
            return function
    raise LookupError("Function not found")
    
def load():
    global functions
    global types
    file = open('debug.txt')
    lines = file.readlines()
    current_function = None
    for line in lines:
        line = line.split(' ')
        if line[0] == 'function':
            if current_function is not None:
                functions.append(current_function)
            current_function = Function(line[1], int(line[2]), int(line[3]))
        elif line[0] == 'local':
            current_function.locals.append(Local(line[1], int(line[2]), int(line[3])))
        elif line[0] == 'type':
            types[int(line[1])] = Type(line[2], int(line[3]))
    if current_function is not None:
        functions.append(current_function)

try:
    load()
except:
    print('Failed to load debug.txt')
    
class show_locals(gdb.Command):

    def __init__(self):
        super(show_locals, self).__init__("show-locals", gdb.COMMAND_DATA)

    def get_frame_pointer(self):
        return int(gdb.parse_and_eval("$r15"))

    def get_ip(self):
        return int(gdb.parse_and_eval("$rip"))
    
    def display_local(self, local, bytes):
        typename = '<unknown>'
        size = 8
        try:
            typeinfo = types[local.typeid]
            typename = typeinfo.name
            size = typeinfo.size
        except:
            pass
        
        if size == 64:
            sizechar = 'Q'
            ssizechar = 'Q'
        elif size == 32:
            sizechar = 'L'
            ssizechar = 'l'
        elif size == 16:
            sizechar = 'H'
            ssizechar = 'h'
        elif size == 8:
            sizechar = 'B'
            ssizechar = 'b'
        else:
            sizechar = 'B'
            ssizechar = 'b'

        val = struct.unpack_from(sizechar, bytes, local.offset)
        val = val[0]
        sval = struct.unpack_from(ssizechar, bytes, local.offset)
        sval = sval[0]
        
        print('{:<10} {:<10} {:<20} {:<20} {:<16x} {:<20}'.format(local.offset,typename, local.name, val, val, sval))

    def display_function(self, ip, fp):
        try:
            fun = lookupFunction(ip)
        except LookupError:
            print("Can't find function matching {:x} ".format(ip))
            return
        print('Function is '+fun.name+' stack size '+str(fun.stackSize())+' frame pointer {:x}'.format(fp))
        print('Offset     Type       Name                 Decimal              Hex              Signed')
        inferior = gdb.selected_inferior()
        bytes = inferior.read_memory(fp, fun.stackSize())
        for local in fun.locals:
            self.display_local(local, bytes)
        
    def invoke(self, arg, from_tty):
        ip = self.get_ip()
        fp = self.get_frame_pointer()
        self.display_function(ip, fp)

show_locals()
