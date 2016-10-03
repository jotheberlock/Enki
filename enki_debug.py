import gdb

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
