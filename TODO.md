# Log viewer impl
- What's the most cache friendly way to store, and more importantly, query the log?
- Storage: as is in the file, with another array denoting line ending byte offsets
- Switching between the data and offset arrays is yuck, but that's unavoidable for any sort of filtering?
- It may be worth copying sorted data to a new array, so that it can be read sequentially. Alternative is to make another offset array for each filter.
- Whats worth caching?
- How are parsed items stored? Array of dynamically defined struct based on the pattern? Or just parse the raw data when needed

# Info stripe
Can show:
- Log level
- Match location
- Duration since last match (heat map)
- Parsed values (e.g duration, packet size, etc)

# Log viewer features
- Load from stdin and other sources (socket, pipe, etc)
- X Highlight all matching selected text
- Format parsing, to highlight diff levels, and get timestamp deltas
- Show log level and delta in scrollbar
- Filter out messages not matching a criterion (make them dim, not disappear). Delta only computed on the matching items
- Multiple search, with different highlight colors
- Compute duration between unique start/end matches, show in scroll bar
- Save regexes into a profile
- Quickly toggle visibility of non matching items, mostly useful while scrolling
- X Tail mode
- Interleave mode, with configurable timestamp offsets
- Pad header widths
- Same log filtering options as framework
- Programatic search? I.e. match a certain header text, only if delta since a different match > some amount.
- So, each match becomes a named dataview, and comparisons can then be performed between them.
- Numeric heatmap coloring with configurable range (or min/max of dataset). Combined with filtering, this acts a bit like a graph
- Multiple file comparison, sync scrolling
- Ctrl g jump to timestamp
- Show Time diff between find result items
- Pattern lookup? 5A93 > CID=5 > QID=13, then find any matching QID
- Jump back to last active find item after scrolling away / doing a different find
- Associate name with match. Ie find \"open qid=123 adcresource read\", all future matches of qid=123 show the command invocation
- Parse search terms and then filer on them, like number > 123
