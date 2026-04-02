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
std::string errorMessage = "";

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

enum ParameterType {
    REGISTER,
    MEMORY,
    IMMEDIATE
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

struct Parameter {
    ParameterType type;
    int value;
    bool valid = true;
};

#include <windows.h>

void enableVTMode() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

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

int stringToNum(const std::string& str, bool& isNumber){
    try {
        int value = std::stoi(str);
        isNumber = true;
        return value;
    }
    catch (...){
        isNumber = false;
        return 0;
    }
}

// Evaluate a single line
void evaluateNextLine(State& state){
    if (state.program.size() <= state.curInstruction){
        errorMessage = "Can't execute a line that doesn't exist.\n";
        return;
    }

    auto setParameterStruct = [](Parameter& param, const std::string& str, const std::string& command){
        // Not really an error, could be that the current instruction doesn't need a parameter
        if (str.size() == 0){
            param.valid = false;
            return;
        }

        if (str[0] == 'm') param.type = MEMORY;
        else if (str[0] == 'r') param.type = REGISTER;
        else param.type = IMMEDIATE;

        switch (param.type){
            case REGISTER:
            case MEMORY: {
                if (str.size() != 2){
                    param.valid = false;
                    errorMessage += "Invalid string length for parameter.\n";
                    break;
                }
                bool isNum;
                param.value = stringToNum(std::string(1, str[1]), isNum);

                if (!isNum){
                    errorMessage += "The memory address inputted is not a number\n";
                    param.valid = false;
                    break;
                }
                break;
            }
            case IMMEDIATE: {
                bool isNum;
                param.value = stringToNum(str, isNum);

                if (!isNum){
                    errorMessage += "The memory address inputted is not a number\n";
                    param.valid = false;
                    break;
                }
                break;
            }
        }
    };

    auto printErrors = [](Parameter& param, const std::string& command){
        if (param.type == REGISTER){
            if (param.value < 0 || param.value >= numRegisters){
                errorMessage += "Invalid register for " + command + " instruction.\n";
            }
        }
        else if (param.type == MEMORY){
            if (param.value < 0 || param.value >= numMemorySlots){
                errorMessage += "Invalid memory address for " + command + " instruction.\n";
            }
        }
        // For immediate values, the range of valid values may be defined by each instruction
    };

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

    std::string line = state.program[state.curInstruction++];
    std::stringstream lineStr(line);
    std::string command(""), param1Str(""), param2Str("");
    lineStr >> command >> param1Str >> param2Str;

    InstructionType type = stringToType(command);
    if (type == UNKNOWN){
        errorMessage += "Invalid instruction\n";
        return;
    }

    Parameter param1, param2;
    setParameterStruct(param1, param1Str, command);
    setParameterStruct(param2, param2Str, command);

    printErrors(param1, command);
    printErrors(param2, command);

    switch (type){
        case NOP: {
            break;
        }
        case LOAD: {
            if (!param1.valid || !param2.valid) break;
            if (param1.type != MEMORY || param2.type != REGISTER){
                errorMessage += "Invalid parameter types for load instruction.\n";
                break;
            }

            int memoryIndex = param1.value;
            int registerIndex = param2.value;

            state.registers[registerIndex] = state.memory[memoryIndex];
            break;
        }
        case STORE: {
            if (!param1.valid || !param2.valid) break;
            if (param1.type != MEMORY || param2.type != REGISTER){
                errorMessage += "Invalid parameter types for store instruction.\n";
                break;
            }

            int memoryIndex = param1.value;
            int registerIndex = param2.value;

            state.memory[memoryIndex] = state.registers[registerIndex];
            break;
        }
        case ADD: {
            if (!param1.valid || !param2.valid) break;
            if (param1.type != REGISTER || param2.type != REGISTER){
                errorMessage += "Invalid parameter types for add instruction.\n";
                break;
            }

            int baseRegister = param2.valid;
            int addRegister = !param2.valid;
            int destinationRegister = param1.valid;

            int result = state.registers[baseRegister] + state.registers[addRegister];
            setFlags(state, result);

            state.registers[destinationRegister] = static_cast<uint8_t>(result);
            break;
        }
        case SUBTRACT: {
            if (!param1.valid || !param2.valid) break;
            if (param1.type != REGISTER || param2.type != REGISTER){
                errorMessage += "Invalid parameter types for subtract instruction.\n";
                break;
            }

            // The source register parameter specifies which register has to be the second number in the subtraction
            int baseRegister = !param2.value;
            int subRegister = param2.value;
            int destinationRegister = param1.value;

            int result = state.registers[baseRegister] - state.registers[subRegister];
            setFlags(state, result);

            state.registers[destinationRegister] = static_cast<uint8_t>(result);
            break;
        }
        case LEFT_SHIFT: {
            if (!param1.valid || !param2.valid) break;
            if (param1.type != REGISTER || param2.type != REGISTER){
                errorMessage += "Invalid parameter types for left shift instruction.\n";
                break;
            }

            int sourceRegister = param2.value;
            int destinationRegister = param1.value;

            int result = state.registers[sourceRegister] / 2;
            setFlags(state, result);

            state.registers[destinationRegister] = static_cast<uint8_t>(result);
            break;
        }
        case RIGHT_SHIFT: {
            if (!param1.valid || !param2.valid) break;
            if (param1.type != REGISTER || param2.type != REGISTER){
                errorMessage += "Invalid parameter types for right shift instruction.\n";
                break;
            }

            int sourceRegister = param2.value;
            int destinationRegister = param1.value;

            int result = state.registers[sourceRegister] * 2;
            setFlags(state, result);

            state.registers[destinationRegister] = static_cast<uint8_t>(result);
            break;
        }
        case JUMP: {
            if (!param1.valid){
                break;
            }
            else if (param2Str.size() > 0){
                errorMessage += "Too many parameters for jump instruction.\n";
                break;
            }
            else if (param1.type != IMMEDIATE){
                errorMessage += "Invalid parameter type for jump instruction.\n";
            }
            else if (param1.value < 0 || param1.value >= maxInstructions){
                errorMessage += "Invalid immediate for jump instruction.\n";
                break;
            }

            state.curInstruction = param1.value;
            break;
        }
        case JUMP_IF_NEGATIVE: {
            if (!param1.valid){
                break;
            }
            else if (param2Str.size() > 0){
                errorMessage += "Too many parameters for jump_if_negative instruction.\n";
                break;
            }
            else if (param1.type != IMMEDIATE){
                errorMessage += "Invalid parameter type for jump_if_negative instruction.\n";
            }
            else if (param1.value < 0 || param1.value >= maxInstructions){
                errorMessage += "Invalid immediate for jump_if_negative instruction.\n";
                break;
            }

            if (state.negativeFlag){
                state.curInstruction = param1.value;
            }
            break;
        }
        case JUMP_IF_ZERO: {
            if (!param1.valid){
                break;
            }
            else if (param2Str.size() > 0){
                errorMessage += "Too many parameters for jump_if_zero instruction.\n";
                break;
            }
            else if (param1.type != IMMEDIATE){
                errorMessage += "Invalid parameter type for jump_if_zero instruction.\n";
            }
            else if (param1.value < 0 || param1.value >= maxInstructions){
                errorMessage += "Invalid immediate for jump_if_zero instruction.\n";
                break;
            }

            if (state.zeroFlag){
                state.curInstruction = param1.value;
            }
            break;
        }
        case JUMP_IF_ABOVE: {
            if (!param1.valid){
                break;
            }
            else if (param2Str.size() > 0){
                errorMessage += "Too many parameters for jump_if_abv instruction.\n";
                break;
            }
            else if (param1.type != IMMEDIATE){
                errorMessage += "Invalid parameter type for jump_if_abv instruction.\n";
            }
            else if (param1.value < 0 || param1.value >= maxInstructions){
                errorMessage += "Invalid immediate for jump_if_abv instruction.\n";
                break;
            }

            if (state.aboveFlag){
                state.curInstruction = param1.value;
            }
            break;
        }
        case PRINT: {
            if (!param1.valid) break;
            if (param2Str.size() > 0){
                errorMessage += "Too many parameters for print instruction.\n";
                break;
            }

            // Print a memory slot
            if (param1.type == MEMORY){
                state.output = state.memory[param1.value];
            }
            // Print a register
            else if (param1.type == REGISTER){
                state.output = state.registers[param1.value];
            }
            else {
                errorMessage += "Invalid parameter type for print instruction.\n";
            }
            break;
        }
        case HALT: {
            state.run = false;
            break;
        }
        // Load the upper 4 bits
        case LOAD_UPPER_IMMEDIATE: {
            if (!param1.valid) break;
            else if (param2Str.size() > 0){
                errorMessage += "Too many parameters for LUI instruction.\n";
                break;
            }
            else if (param1.type != IMMEDIATE){
                errorMessage += "Invalid parameter type for LUI instruction.\n";
                break;
            }
            else if (param1.value < 0 || param1.value >= 16){
                errorMessage += "Invalid immediate for LUI instruction (Range: 0 - 16).\n";
                break;
            }

            state.registers[0] = static_cast<uint8_t>(param1.value << 4);
            break;
        }
        // Load the lower 4 bits
        case LOAD_LOWER_IMMEDIATE: {
            if (!param1.valid) break;
            else if (param2Str.size() > 0){
                errorMessage += "Too many parameters for LLI instruction.\n";
                break;
            }
            else if (param1.type != IMMEDIATE){
                errorMessage += "Invalid parameter type for LLI instruction.\n";
                break;
            }
            else if (param1.value < 0 || param1.value >= 16){
                errorMessage += "Invalid immediate for LLI instruction (Range: 0 - 16).\n";
                break;
            }

            state.registers[0] |= static_cast<uint8_t>(param1.value);
            break;
        }
        case INCREMENT: {
            if (!param1.valid || !param2.valid) break;
            if (param1.type != REGISTER || param2.type != REGISTER){
                errorMessage += "Invalid parameter types for increment instruction.\n";
                break;
            }

            int sourceRegister = param1.value;
            int destinationRegister = param2.value;
            setFlags(state, state.registers[sourceRegister] + 1);

            state.registers[destinationRegister] = state.registers[sourceRegister] + 1;
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
    int curInstruction = errorMessage.size() > 0 ? state.curInstruction-1 : state.curInstruction;
    if (state.program.size() > state.curInstruction){
        std::cout << "Current instruction: " << state.program[curInstruction] << " (" << curInstruction << ") " << std::endl;
    }
    std::cout << "Errors: \n" << errorMessage << std::endl;

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
    enableVTMode();

    while (currentState.run){
        printState(currentState);

        // If we encounter an error, then quit the program
        if (errorMessage.size() > 0){
            currentState = State{};
        }
        errorMessage.clear();

        std::string line;
        std::getline(std::cin, line);
        
        std::cout << std::endl;

        handleInput(line, currentState);
    }

    return 0;
}