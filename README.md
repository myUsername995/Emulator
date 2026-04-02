# Emulator
An emulator for my 8-bit computer in Lumber Tycoon 2.

# HOW TO USE THE EMULATOR
- q - quit
- c - step the emulator
- go - go until the program reaches some line Eg: -> go 12
- set - set values in memory to some value (below 255) Eg: -> set r0 10 or set m3 255
- load - loads a program from the "emulator" folder Eg: -> load collatz.txt - load division.txt

Note: The instruction addresses are 0-indexed.

# INSTRUCTION LAYOUT
0 1 2 3 4 5 6 7

- 0-3: instruction type
- 4-7: immediate (jumps, LUI, LLi)
- 4-5: ram address (load, store)
- 6: destination register (add, subtract, left shift, right shift, increment)
- 7: source register (add, subtract, left shift, right shift, increment)
- 7: decider (print) -> 0: ram 1: registers

# INSTRUCTIONS
- NOP xx xx - does nothing
- LOD m0 r0 - loads from memory to registers
- STR m0 r0 - stores to memory from registers
- ADD r0 r0 - adds the two registers
- -> 1st parameter: destination register (where the number will be put)
- -> 2nd parameter: source register (doesn't matter for addition)
- SUB r0 r0 - subtracts the two registers
- -> 1st parameter: destination register
- -> 2nd parameter: source register (the second number in the subtraction)
- LSH r0 r0 - divides a register by 2 (parameters: destination, source)
- RSH r0 r0 - multiplies a register by 2 (parameters: destination, source)
- JMP 0 - jumps to some line
- JIN 0 - jumps if the negative flag is set (set by a previous instruction like add, subtract, lsh or rsh)
- JIZ 0 - jumps if the zero flag is set
- JIA 0 - jumps if the above zero flag is set
- OUT r0/m0 - puts a register/memory address into the output register
- HALT xx xx - halts the program's execution
- LUI 15 - loads the upper 4 bits of a byte into the 0th register (also clears the 0th register)
- LLI 15 - loads the lower 4 bits of a byte into the 0th register (doesn't clear the 0th register)
- INC r0 r0 - increments a register (parameters: destination, source)

# NOTES
The ingame computer is a little different than this emulator, here is everything that's different:
- After a print instruction, you should only call print again after atleast 2 instructions, otherwise the display can't keep up
- The print instruction is special because it uses all 8 bits -> 4-5: memory address, 6: register address, 7: "decider" 
(0 to load from memory, 1 to load from registers)
