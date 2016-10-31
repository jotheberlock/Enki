import gdb
import struct
import traceback

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
little_endian = True
instruction_register = '$rip'
frame_register = '$r15'
sf_bit = True

def lookupFunction(addr):
    global functions
    for function in functions:
        if addr >= function.address and addr < (function.address+function.size):
            return function
    raise LookupError("Function not found")
    
def load():
    global functions
    global types
    global little_endian
    global instruction_register
    global frame_register
    global sf_bit
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
        elif line[0] == 'endian' and line[1] == 'big':
            little_endian = False
        elif line[0] == 'arch':
            if line[1] == '1\n':
                pass # x86-64
            elif line[1] == '2\n':
                # ARM32
                instruction_register = '$pc'
                frame_register = '$ip'
                sf_bit = False
            else:
                print('Unknown architecture ['+line[1]+']!')
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
        global frame_register
        return int(str(gdb.parse_and_eval(frame_register)).split(' ')[0],0)

    def get_ip(self):
        global instruction_register
        return int(str(gdb.parse_and_eval(instruction_register)).split(' ')[0],0)

    def get_static_link(self, bytes):
        global little_endian
        global sf_bit
        if little_endian == True:
            endianchar = '<'
        else:
            endianchar = '>'
        if sf_bit:
            format_str = endianchar+'Q'
            offset = 16
        else:
            format_str = endianchar+'L'
            offset = 8
        val = struct.unpack_from(format_str, bytes, offset)
        val = val[0]
        return int(val)

    def get_ip_from_frame(self, frameptr):
        global little_endian
        global sf_bit
        if little_endian == True:
            endianchar = '<'
        else:
            endianchar = '>'
        if sf_bit:
            offset = 8
            size = 8
            format_str = endianchar+'Q'   
        else:
            offset = 4
            size = 4
            format_str = endianchar+'L'
        frame_bytes = gdb.selected_inferior().read_memory(frameptr+offset, size)
        val = struct.unpack_from(format_str, frame_bytes, 0)
        val = val[0]
        return int(val)
    
    def display_local(self, local, bytes):
        global little_endian
        typename = '<unknown>'
        size = 8
        try:
            typeinfo = types[local.typeid]
            typename = typeinfo.name
            size = typeinfo.size
        except:
            pass

        if little_endian == True:
            endianchar = '<'
        else:
            endianchar = '>'
            
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

        format_str = endianchar+sizechar
        
        val = struct.unpack_from(format_str, bytes, local.offset)
        val = val[0]
        sval = struct.unpack_from(format_str, bytes, local.offset)
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
        static_link = self.get_static_link(bytes)
        parent_ip = self.get_ip_from_frame(static_link)
        try:
            parent_fun = lookupFunction(parent_ip)
        except LookupError:
            print("Can't find parent function matching {:x} ".format(parent_ip))
            return
        print('Static link parent function is '+parent_fun.name)
        
    def invoke(self, arg, from_tty):
        try:
            ip = self.get_ip()
            fp = self.get_frame_pointer()
            self.display_function(ip, fp)
        except:
            traceback.print_exc()
            
show_locals()
