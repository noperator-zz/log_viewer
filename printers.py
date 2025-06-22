import gdb

class Widget:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        # return str(self.val)
        return f"Widget(path="

def lookup_type(val):
    # print(f"lookup_type called with {val.type}")
    # if str(val.type) == 'Widget':
    #     return Widget(val)
    return None

def register_printers(objfile):
    gdb.printing.register_pretty_printer(objfile, lookup_type)
