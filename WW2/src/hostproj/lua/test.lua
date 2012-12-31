local foo = { }
foo[ALLIED] = "" ; foo[AXIS] = ""
unit = getHcUnit("4pdkg1")
foo[ALLIED] = foo[ALLIED] .. unit:name()
