python
import sys
import os
sys.path.insert(0, os.getcwd())
from printers import register_printers
register_printers(None)
end
