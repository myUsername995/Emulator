/*
------------------------
HOW TO USE THE EMULATOR
------------------------
q - quit
c - step the emulator
go - go until the program reaches some line Eg -> go 12
set - set values in memory to some value (below 255) Eg: -> set r0 10 - set m3 255
load - loads a program from the "emulator" folder Eg: -> load collatz.txt - load division.txt

Note: The instruction addresses are 0-indexed.

------------------------
INSTRUCTION LAYOUT
------------------------
0 1 2 3 4 5 6 7

0-3: instruction type
4-7: immediate (jumps, LUI, LLi)
4-5: ram address (load, store)
6: destination register (add, subtract, left shift, right shift, increment)
7: source register (add, subtract, left shift, right shift, increment)
7: decider (print) -> 0: ram 1: registers

------------------------
INSTRUCTIONS
------------------------
NOP xx xx - does nothing
LOD m0 r0 - loads from memory to registers
STR m0 r0 - stores to memory from registers
ADD r0 r0 - adds the two registers
-> 1st parameter: destination register (where the number will be put)
-> 2nd parameter: source register (doesn't matter for addition)
SUB r0 r0 - subtracts the two registers
-> 1st parameter: destination register
-> 2nd parameter: source register (the second number in the subtraction)
LSH r0 r0 - divides a register by 2 (parameters: destination, source)
RSH r0 r0 - multiplies a register by 2 (parameters: destination, source)
JMP 0 - jumps to some line
JIN 0 - jumps if the negative flag is set (set by a previous instruction like add, subtract, lsh or rsh)
JIZ 0 - jumps if the zero flag is set
JIA 0 - jumps if the above zero flag is set
OUT r0/m0 - puts a register/memory address into the output register
HALT xx xx - halts the program's execution
LUI 15 - loads the upper 4 bits of a byte into the 0th register (also clears the 0th register)
LLI 15 - loads the lower 4 bits of a byte into the 0th register (doesn't clear the 0th register)
INC r0 r0 - increments a register (parameters: destination, source)

------------------------
NOTES
------------------------
The ingame computer is a little different than this emulator, here is everything that's different:
-After a print instruction, you should only call print again after atleast 2 instructions, otherwise the display can't keep up
-The print instruction is special because it uses all 8 bits -> 4-5: memory address, 6: register address, 7: "decider" 
(0 to load from memory, 1 to load from registers)

*/

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <vector>

constexpr int numMemorySlots = 4;
constexpr int numRegisters = 2;
constexpr int maxInstructions = 16;

enum InstructionType {
    NOP,
    LOAD,
    STORE,
    ADD,
    SUBTRACT,
    LEFT_SHIFT,
    RIGHT_SHIFT,
    JUMP,
    JUMP_IF_NEGATIVE,
    JUMP_IF_ZERO,
    JUMP_IF_ABOVE,
    PRINT,
    HALT,
    LOAD_UPPER_IMMEDIATE,
    LOAD_LOWER_IMMEDIATE,
    INCREMENT,
    UNKNOWN
};

struct State {
    // The state of the computer
    uint8_t registers[numRegisters] = {0};
    uint8_t memory[numMemorySlots] = {0};
    uint8_t output = 0;
    int curInstruction = 0;

    bool overflowFlag = false;
    bool negativeFlag = false;
    bool zeroFlag = false;
    bool aboveFlag = false;

    bool run = true;
    std::vector<std::string> program;
};

// Helpers
InstructionType stringToType(const std::string& str){
    if (str == "NOP") return NOP;
    if (str == "LOD") return LOAD;
    if (str == "STR") return STORE;
    if (str == "ADD") return ADD;
    if (str == "SUB") return SUBTRACT;
    if (str == "LSH") return LEFT_SHIFT;
    if (str == "RSH") return RIGHT_SHIFT;
    if (str == "JMP") return JUMP;
    if (str == "JIN") return JUMP_IF_NEGATIVE;
    if (str == "JIZ") return JUMP_IF_ZERO;
    if (str == "JIA") return JUMP_IF_ABOVE;
    if (str == "OUT") return PRINT;
    if (str == "HLT") return HALT;
    if (str == "LUI") return LOAD_UPPER_IMMEDIATE;
    if (str == "LLI") return LOAD_LOWER_IMMEDIATE;
    if (str == "INC") return INCREMENT;

    return UNKNOWN;
}

// Evaluate a single line
void evaluateNextLine(State& state){
    std::string line = state.program[state.curInstruction];
    std::stringstream lineStr(line);
    std::string command, word1, word2;
    lineStr >> command >> word1 >> word2;

    InstructionType type = stringToType(command);

    int number1 = -1;
    int number2 = -1;

    if (word1.size() == 2) number1 = std::stoi(std::string(1, word1[1]));
    if (word2.size() == 2) number2 = std::stoi(std::string(1, word2[1]));

    auto setFlags = [](State& state, int result){
        state.overflowFlag = false;
        state.negativeFlag = false;
        state.zeroFlag = false;
        state.aboveFlag = false;

        if (result > 255) state.overflowFlag = true;
        if (result < 0) state.negativeFlag = true;
        if (result == 0) state.zeroFlag = true;
        if (result > 0) state.aboveFlag = true;
    };

    switch (type){
        case NOP: {
            break;
        }
        case LOAD: {
            if (word1.size() != 2 || word1[0] != 'm' || word2.size() != 2 || word2[0] != 'r'){
                std::cerr << "Invalid syntax for load instruction." << std::endl;
            }

            int memoryIndex = number1;
            int registerIndex = number2;

            if (memoryIndex >= numMemorySlots || registerIndex >= numRegisters){
                std::cerr << "Invalid register or memory index for load instruction." << std::endl;
                break;
            }

            state.registers[registerIndex] = state.memory[memoryIndex];
            state.curInstruction++;
            break;
        }
        case STORE: {
            if (word1.size() != 2 || word1[0] != 'm' || word2.size() != 2 || word2[0] != 'r'){
                std::cerr << "Invalid syntax for store instruction." << std::endl;
            }

            int memoryIndex = number1;
            int registerIndex = number2;

            if (memoryIndex >= numMemorySlots || registerIndex >= numRegisters){
                std::cerr << "Invalid register or memory index for store instruction." << std::endl;
                break;
            }

            state.memory[memoryIndex] = state.registers[registerIndex];
            state.curInstruction++;
            break;
        }
        case ADD: {
            if (word1.size() != 2 || word1[0] != 'r' || word2.size() != 2 || word2[0] != 'r'){
                std::cerr << "Invalid syntax for add instruction." << std::endl;
            }

            int baseRegister = number2;
            int addRegister = !number2;
            int destinationRegister = number1;

            if (number1 >= numRegisters || number2 >= numRegisters){
                std::cerr << "Invalid register or memory index for add instruction." << std::endl;
                break;
            }

            int result = state.registers[baseRegister] + state.registers[addRegister];
            setFlags(state, result);

            state.registers[destinationRegister] = static_cast<uint8_t>(result);
            state.curInstruction++;
            break;
        }
        case SUBTRACT: {
            if (word1.size() != 2 || word1[0] != 'r' || word2.size() != 2 || word2[0] != 'r'){
                std::cerr << "Invalid syntax for subtract instruction." << std::endl;
            }

            // The source register parameter specifies which register has to be the second number in the subtraction
            int baseRegister = !number2;
            int subRegister = number2;
            int destinationRegister = number1;

            if (number1 >= numRegisters || number2 >= numRegisters){
                std::cerr << "Invalid register or memory index for subtract instruction." << std::endl;
                break;
            }

            int result = state.registers[baseRegister] - state.registers[subRegister];
            setFlags(state, result);

            state.registers[destinationRegister] = static_cast<uint8_t>(result);
            state.curInstruction++;
            break;
        }
        case LEFT_SHIFT: {
            if (word1.size() != 2 || word1[0] != 'r' || word2.size() != 2 || word2[0] != 'r'){
                std::cerr << "Invalid syntax for left shift instruction." << std::endl;
            }

            int sourceRegister = number2;
            int destinationRegister = number1;

            if (sourceRegister >= numRegisters || destinationRegister >= numRegisters){
                std::cerr << "Invalid register or memory index for left shift instruction." << std::endl;
                break;
            }

            int result = state.registers[sourceRegister] / 2;
            setFlags(state, result);

            state.registers[destinationRegister] = static_cast<uint8_t>(result);
            state.curInstruction++;
            break;
        }
        case RIGHT_SHIFT: {
            if (word1.size() != 2 || word1[0] != 'r' || word2.size() != 2 || word2[0] != 'r'){
                std::cerr << "Invalid syntax for right shift instruction." << std::endl;
            }

            int sourceRegister = number2;
            int destinationRegister = number1;

            if (sourceRegister >= numRegisters || destinationRegister >= numRegisters){
                std::cerr << "Invalid register or memory index for right shift instruction." << std::endl;
                break;
            }

            int result = state.registers[sourceRegister] * 2;
            setFlags(state, result);

            state.registers[destinationRegister] = static_cast<uint8_t>(result);
            state.curInstruction++;
            break;
        }
        case JUMP: {
            int immediate = std::stoi(word1);

            if (immediate < 0 || immediate >= maxInstructions){
                std::cerr << "Invalid immediate for jump instruction." << std::endl;
                break;
            }

            state.curInstruction = immediate;
            break;
        }
        case JUMP_IF_NEGATIVE: {
            int immediate = std::stoi(word1);

            if (immediate < 0 || immediate >= maxInstructions){
                std::cerr << "Invalid immediate for jump_if_negative instruction." << std::endl;
                break;
            }

            if (state.negativeFlag){
                state.curInstruction = immediate;
            }
            else {            
                state.curInstruction++;
            }
            break;
        }
        case JUMP_IF_ZERO: {
            int immediate = std::stoi(word1);

            if (immediate < 0 || immediate >= maxInstructions){
                std::cerr << "Invalid immediate for jump_if_zero instruction." << std::endl;
                break;
            }

            if (state.zeroFlag){
                state.curInstruction = immediate;
            }
            else {
                state.curInstruction++;
            }
            break;
        }
        case JUMP_IF_ABOVE: {
            int immediate = std::stoi(word1);

            if (immediate < 0 || immediate >= maxInstructions){
                std::cerr << "Invalid immediate for jump_if_above instruction." << std::endl;
                break;
            }

            if (state.aboveFlag){
                state.curInstruction = immediate;
            }
            else {
                state.curInstruction++;
            }
            break;
        }
        case PRINT: {
            // Print a memory slot
            if (word1[0] == 'm' && number1 >= 0 && number1 < numMemorySlots){
                state.output = state.memory[number1];
            }
            // Print a register
            if (word1[0] == 'r' && number1 >= 0 && number1 < numRegisters){
                state.output = state.registers[number1];
            }
            state.curInstruction++;
            break;
        }
        case HALT: {
            state.run = false;
            break;
        }
        // Load the upper 4 bits
        case LOAD_UPPER_IMMEDIATE: {
            uint8_t upperNum = std::stoi(word1);
            upperNum = (upperNum & 0x0F) << 4;

            state.registers[0] = upperNum;
            state.curInstruction++;
            break;
        }
        // Load the lower 4 bits
        case LOAD_LOWER_IMMEDIATE: {
            uint8_t lowerNum = std::stoi(word1);
            lowerNum = lowerNum & 0x0F;

            state.registers[0] |= lowerNum;
            state.curInstruction++;
            break;
        }
        case INCREMENT: {
            if (word1.size() != 2 || word1[0] != 'r' || word2.size() != 2 || word2[0] != 'r'){
                std::cerr << "Invalid syntax for increment instruction." << std::endl;
            }

            int sourceRegister = number2;
            int destinationRegister = number1;

            if (sourceRegister >= numRegisters || destinationRegister >= numRegisters){
                std::cerr << "Invalid register or memory index for increment instruction." << std::endl;
                break;
            }
            setFlags(state, state.registers[sourceRegister] + 1);

            state.registers[destinationRegister] = state.registers[sourceRegister] + 1;
            state.curInstruction++;
            break;
        }
    }
}

// Load a file into an array of instructions
void loadFile(State& state, const std::string& path){
    std::ifstream inFile(path);
    if (!inFile){
        std::cerr << "Invalid path: " << path << std::endl;
        return;
    }

    std::string line;
    int lineIndex = 0;
    while (std::getline(inFile, line)){
        state.program.push_back(line);
    }
}

void printState(State& state){
    // Move cursor to top-left and clear screen
    std::cout << "\033[H\033[J";

    std::cout << "R0: " << int(state.registers[0]) << std::endl;
    std::cout << "R1: " << int(state.registers[1]) << std::endl;
    std::cout << std::endl;

    std::cout << "M0: " << int(state.memory[0]) << std::endl;
    std::cout << "M1: " << int(state.memory[1]) << std::endl;
    std::cout << "M2: " << int(state.memory[2]) << std::endl;
    std::cout << "M3: " << int(state.memory[3]) << std::endl;
    std::cout << std::endl;

    std::cout << "Overflow: " << state.overflowFlag << std::endl;
    std::cout << "Negative: " << state.negativeFlag << std::endl;
    std::cout << "Zero: " << state.zeroFlag << std::endl;
    std::cout << "Above zero: " << state.aboveFlag << std::endl;
    std::cout << std::endl;

    std::cout << "Output: " << int(state.output) << std::endl;
    if (state.program.size() > state.curInstruction){
        std::cout << "Current instruction: " << state.program[state.curInstruction] << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Enter command: ";

    std::cout.flush(); // ensure immediate update
}

void handleInput(const std::string& input, State& state){
    std::stringstream inputStr(input);
    std::string command;

    inputStr >> command;

    // Quit the emulator
    if (command == "q"){
        state.run = false;
        return;
    }
    // Load a file
    else if (command == "load"){
        std::string path;
        inputStr >> path;

        loadFile(state, "emulator\\" + path);
    }
    // Step the emulator
    else if (command == "c"){
        evaluateNextLine(state);
    }
    // Go until we get to a certain line
    else if (command == "go"){
        std::string curLineStr;
        inputStr >> curLineStr;
        int curLine = std::stoi(curLineStr);

        while (state.curInstruction != curLine && state.run){
            evaluateNextLine(state);
        }
    }
    else if (command == "set"){
        std::string memory, valueStr;
        if (!(inputStr >> memory) || !(inputStr >> valueStr) || memory.size() != 2) {
            std::cout << "Invalid set command." << std::endl;
        }
        else {
            if (memory[0] == 'r'){
                int index = std::stoi(std::string(1, memory[1]));
                int value = std::stoi(valueStr);
                if (index < 0 || index > numRegisters){
                    std::cout << "Invalid register." << std::endl;
                }
                else if (value < 0 || value > 255){
                    std::cout << "Invalid value." << std::endl;
                }
                else {
                    state.registers[index] = static_cast<uint8_t>(value);
                }
            }
            if (memory[0] == 'm'){
                int index = std::stoi(std::string(1, memory[1]));
                int value = std::stoi(valueStr);
                if (index < 0 || index > numMemorySlots){
                    std::cout << "Invalid memory slot." << std::endl;
                }
                else if (value < 0 || value > 255){
                    std::cout << "Invalid value." << std::endl;
                }
                else {
                    state.memory[index] = static_cast<uint8_t>(value);
                }
            }
        }
    }
}

int main(int argc, char* argv[]){
    State currentState;

    while (currentState.run){
        printState(currentState);

        std::string line;
        std::getline(std::cin, line);
        
        std::cout << std::endl;

        handleInput(line, currentState);
    }

    return 0;
}