import gdb

class Type:

    name = ''
    size = 0

    def __init__(self, nam, siz):
        name = nam
        size = siz

class Local:

    name = ''
    typeid = 0
    offset = 0

    def __init__(self, nam, tid, off):
        name = nam
        typeid = tid
        offset = off
        
class Function:

    name = ''
    address = 0
    size = 0
    locals = []
    
    def __init__(self, nam, addr, siz):
        address = addr
        name = nam
        size = siz

types = {}
functions = []

def load():
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
    
load()
        
class show_locals(gdb.Command):

    def __init__(self):
        super(show_locals, self).__init__("show-locals", gdb.COMMAND_DATA)

    def get_frame_pointer(self):
        return int(gdb.parse_and_eval("$r15"))
        
    def invoke(self, arg, from_tty):
        fp = self.get_frame_pointer()
        print("Hello, world! Frame pointer is {:x} {:d}".format(fp,fp))
        inferior = gdb.selected_inferior()
        bytes = inferior.read_memory(fp, 80)
        
show_locals()
