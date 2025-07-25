import gdb
import gdb.printing

class DynarrayPrinter:
    """Pretty printer for dynarray<T>"""
    def __init__(self, val):
        self.val = val
        self.data = val['data_']
        self.size = int(val['size_'])
        self.capacity = int(val['capacity_'])

    def to_string(self):
        return f"dynarray(size={self.size}, capacity={self.capacity}, data_={self.data})"

    def children(self):
        for i in range(self.size):
            yield f"[{i}]", (self.data + i).dereference()

    def display_hint(self):
        return 'array'

def build_dynarray_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("dynarray")
    pp.add_printer("dynarray", "^dynarray<.*>$", DynarrayPrinter)
    return pp

def register_printers(objfile):
    gdb.printing.register_pretty_printer(objfile, build_dynarray_printer())
