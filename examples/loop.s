LOAD_CONST $MSG "Help, I'm stuck in a loop!"
LOAD_CONST $TARG 0xA
LOAD_CONST $ACC 0x0

LOOP_START: PUTS $MSG

INCR $ACC
WORD_EQ $R0 $ACC $TARG
JMP_IF #LOOP_END $R0

JMP #LOOP_START

LOOP_END: LOAD_CONST $MSG "All done."
PUTS $MSG
