automata.h has been copied from die-xml lib

some comments have been removed and minor fixes have been applied. no refactors have been made at all

the goal for now is just use it to validate json parsing and have a working lib

but I don't like the way it is now. although the current implementation is good to create an automaton dynamically,
it is too error prone to build a static parser
 
next steps are to refactor it to have node builders to build all node transactions in the same place, 
sane key types for fixed node alphabets (eg.: template it and use enums),
polymorphic nodes to work better with grouped and procedural transactions (get rid of that dumb RangeSetter, get rid of defaultTransaction),
also provide a decent Node interface for sane user node types,
plan how to compose automata 
and move all of it to a separate library 
