#include <fstream>
#include <iostream>
#include <map>
#include <vector>

// Memory segments
#define SP_LOC       0   // Represents SP pointer in RAM, not the value of SP
#define LCL          1
#define ARG          2
#define POINTER_BASE 3   // THIS = RAM[3], THAT = RAM[4]
#define TEMP_BASE    5   // TEMP  = RAM[5-12]
#define R_BASE       13  // R13, R14, R15 = RAM[13-15]
#define STATIC_BASE  16  // STATIC = RAM[16-255]
#define STACK_BASE   256

std::string fileName;

enum CommandType {
  C_ARITHMETIC, C_PUSH, C_POP, C_LABEL, C_GOTO,
  C_IF, C_FUNCTION, C_RETURN, C_CALL, C_UNDEFINED
};

std::map<std::string, CommandType> commandMap = {
  { "add", C_ARITHMETIC },
  { "sub", C_ARITHMETIC },
  { "neg", C_ARITHMETIC },
  { "eq", C_ARITHMETIC },
  { "gt", C_ARITHMETIC },
  { "lt", C_ARITHMETIC },
  { "and", C_ARITHMETIC },
  { "or", C_ARITHMETIC },
  { "not", C_ARITHMETIC },
  { "push", C_PUSH },
  { "pop", C_POP },
};

// "static", "temp", and "constant" segments are handled differently
std::map<std::string, std::string> segmentSymbolMap = {
  { "argument", "ARG" },
  { "local", "LCL" },
  { "this", "THIS" },
  { "that", "THAT" },
  { "pointer", "THIS" }, // pointer 0 is THIS, pointer 1 is THAT
};

class Command {
public:
  CommandType type {C_UNDEFINED};
  std::string commandTypeStr {};
  std::string arg1 {""};
  int arg2 {-1};
  
  Command(std::string strLine) {
    bool commandStrFound = false;
    bool arg1Found = false;
    std::string arg2Str;

    for (char const &c : strLine) {
      // Determine the command string and the command type
      if ((c == ' ' || c == '\r') && !commandStrFound) {
        commandStrFound = true;
        type = commandMap.find(commandTypeStr)->second;
        continue;
      }
      else if (!commandStrFound) {
        commandTypeStr += c;
        continue;
      }

      // Determine the first argument
      if (type == CommandType::C_RETURN) {
        arg1Found = true; // Not used in this case according to spec
        continue;
      }
      else if (type == CommandType::C_ARITHMETIC) {
        arg1 = commandTypeStr;
        arg1Found = true;
        continue;
      } else if (c == ' ' && !arg1Found) {
        arg1Found = true;
        continue;
      } else if (!arg1Found) {
        arg1 += c;
        continue;
      }

      // Determine the second argument
      if ((type == CommandType::C_PUSH  || type == CommandType::C_POP ||
          type == CommandType::C_FUNCTION || type == CommandType::C_CALL) &&
          c != '\n') {
        arg2Str += c;
      }
    }

    if (arg2Str.length() > 0)
      arg2 = std::stoi(arg2Str);
  }
};

/* This function takes a file name with the path included and
returns the file name only, for use with the "static segment. */
std::string getFileNameWithoutExtension(std::string inFileNameWithPath) {
  std::string fileName = inFileNameWithPath.substr(inFileNameWithPath.find_last_of("/\\") + 1);
  std::string::size_type const p(fileName.find_last_of('.'));
  std::string fileNameWithoutExtension = fileName.substr(0, p);
  return fileNameWithoutExtension;
}

/* This function handles the full path name so that you can pass in
paths to the command line and the file will get saved correctly. */
std::string formatOutFileNameWithPath(std::string inFileNameWithPath) {
  std::string outFileName;
  for (char const &c : inFileNameWithPath) {
    if (c  == '.')
      break;
    outFileName += c;
  }

  outFileName += ".asm";
  std::cout << "Writing to " << outFileName << "...\n";
  return outFileName;
}

std::string joinAssembly(std::vector<std::string> instructions) {
  std::string joinedStr;
  for (int i = 0; i < instructions.size(); i++) {
    joinedStr += instructions[i] + "\n";
  }
  return joinedStr;
}

int SP = STACK_BASE; // SP is initialized to point to the stack base

void writeInitialSP(std::ofstream& outFile) {
  std::vector<std::string> instructions = {
    "// Initialize the stack pointer",
    "@" + std::to_string(STACK_BASE),
    "D=A",
    "@SP",
    "M=D"
  };
  outFile << joinAssembly(instructions);
}

void writeEndLoop(std::ofstream& outFile) {
  std::vector<std::string> instructions = {
    "// End with an infinite loop",
    "(END)",
    "@END",
    "0;JMP"
  };
  outFile << joinAssembly(instructions);
}

std::string incrementSP = "@SP\nM=M+1\n";
std::string decrementSP = "@SP\nM=M-1\n";
std::string saveStackTopToD = decrementSP + "@SP\n" + "A=M\n" + "D=M\n";

std::string createBinaryOperatorAssembly(std::string assemblyCommand) {
  std::vector<std::string> instructions = {
    saveStackTopToD,
    decrementSP,
    "@SP",
    "A=M",
    assemblyCommand,
    incrementSP
  };
  return joinAssembly(instructions);
}

std::string createUnaryOperatorAssembly(std::string assemblyCommand) {
  std::vector<std::string> instructions = {
    saveStackTopToD,
    "@SP",
    "A=M",
    assemblyCommand,
    incrementSP
  };
  return joinAssembly(instructions);
}

int symbolCounter = 0;
std::string createUniqueCounter() { return std::to_string(symbolCounter++); }

std::string createComparisonAssembly(std::string assemblyCommand) {
  std::string uniqueCounter = createUniqueCounter();
  std::vector<std::string> instructions = {
    saveStackTopToD,
    decrementSP,
    "@SP",
    "A=M",
    "D=D-M",
    "@IF_TRUE:" + uniqueCounter,
    assemblyCommand,
    "@SP",
    "A=M",
    "M=0",
    "@END:" + uniqueCounter,
    "0;JMP",
    "(IF_TRUE:" + uniqueCounter + ")",
    "@SP",
    "A=M",
    "M=-1",
    "(END:" + uniqueCounter + ")",
    incrementSP
  };
  return joinAssembly(instructions);
}

void writeArithmetic(std::ofstream& outFile, Command command) {
  std::string type = command.commandTypeStr;
  if (type == "add") { outFile << createBinaryOperatorAssembly("M=M+D"); }
  if (type == "sub") { outFile << createBinaryOperatorAssembly("M=M-D"); }
  if (type == "and") { outFile << createBinaryOperatorAssembly("M=M&D"); }
  if (type == "or") { outFile << createBinaryOperatorAssembly("M=M|D"); }
  if (type == "eq") { outFile << createComparisonAssembly("D;JEQ"); }
  if (type == "gt") { outFile << createComparisonAssembly("D;JLT"); }
  if (type == "lt") { outFile << createComparisonAssembly("D;JGT"); }
  if (type == "neg") { outFile << createUnaryOperatorAssembly("M=-D"); }
  if (type == "not") { outFile << createUnaryOperatorAssembly("M=!D"); }
  return;
};

std::string createWriteToStackWithBaseAddressAssembly(std::string segment, int index) {
  std::string symbol = segmentSymbolMap.find(segment)->second;
  std::vector<std::string> instructions = {
    "@" + symbol,
    "A=M",
    "D=M",
    "@" + std::to_string(index),
    "D=D+M",
    "A=D",
    "D=M",
    "@SP",
    "A=M",
    "M=D",
    incrementSP
  };
  return joinAssembly(instructions);
}

void writePushPop(std::ofstream& outFile, CommandType commandType, std::string segment, int index) {
  /* Note: the Stack Pointer (stored at SP in RAM) is the memory address just
   * ~following~ the memory address containing the topmost stack value.
   * So this is why the order of the increment or decrement operators matters.

   * "push segment index" pushes the value of segment[index] onto the stack
   * "push segment index" is RAM[SP++] = segment[index] 
   * "pop segment index" pops the top stack value and stores it in segment[index]
   * "pop segment index" is segment[index] = RAM[--SP]
   */

  std::vector<std::string> instructions;

  // TODO: implement helper for pushSPToStack 

  if (commandType == CommandType::C_PUSH) {
    if (segment == "constant") {
      instructions = {
        "@" + std::to_string(index),
        "D=A",
        "@SP",
        "A=M",
        "M=D",
        incrementSP
      };
      outFile << joinAssembly(instructions);
    }
    else if (segment == "local" || segment == "argument" || segment == "this"  || segment == "that") {
      std::string symbol = segmentSymbolMap.find(segment)->second;
      instructions = {
        "@" + symbol,
        "D=M",
        "@" + std::to_string(index),
        "A=D+A",
        "D=M",
        "@SP",
        "A=M",
        "M=D",
        incrementSP
      };
      outFile << joinAssembly(instructions);
    }
    else if (segment == "pointer") {
      std::string symbol = segmentSymbolMap.find(segment)->second;
      instructions = {
        "@THIS",
        "D=A",
        "@" + std::to_string(index),
        "A=D+A",
        "D=M",
        "@SP",
        "A=M",
        "M=D",
        incrementSP
      };
      outFile << joinAssembly(instructions);
    }
    else if (segment == "temp") {
      instructions = {
        "@" + std::to_string(TEMP_BASE + index),
        "D=M",
        "@SP",
        "A=M",
        "M=D",
        incrementSP
      };
      outFile << joinAssembly(instructions);
    } else if (segment == "static") {
      instructions = {
        "@" + fileName + "." + std::to_string(index),
        "D=M",
        "@SP",
        "A=M",
        "M=D",
        incrementSP
      };
      outFile << joinAssembly(instructions);
    }
    else {
      std::cerr << "Segment type of " + segment + " not handled for push.";
    }
  }
  else if (commandType == CommandType::C_POP) {
    if (segment == "local" || segment == "argument" || segment == "this"  || segment == "that") {
      std::string symbol = segmentSymbolMap.find(segment)->second;
      instructions = {
        // get the address of "segment[index]
        "@" + symbol,
        "D=M",
        // D now holds the address stored in @symbol
        "@" + std::to_string(index),
        "D=D+A",
        "@R13",
        "M=D",
        // D should now be address stored in segment + index
        decrementSP,
        // get the value of SP
        "@SP",
        "A=M",
        "D=M",
        "@R13",
        "A=M",
        "M=D"
      };
      outFile << joinAssembly(instructions);
    }
    else if (segment == "pointer") {
      instructions = {
        // get the address of "segment[index]
        "@THIS",
        "D=A",
        "@" + std::to_string(index),
        "D=D+A",
        "@R13",
        "M=D",
        // D should now be address stored in segment + index
        decrementSP,
        // get the value of SP
        "@SP",
        "A=M",
        "D=M",
        "@R13",
        "A=M",
        "M=D"
      };
      outFile << joinAssembly(instructions);
    }
    else if (segment == "temp") {
      instructions = {
        decrementSP,
        "@SP",
        "A=M",
        "D=M",
        "@" + std::to_string(TEMP_BASE + index),
        "M=D"
      };
      outFile << joinAssembly(instructions);
    }
    else if (segment == "static") {
      instructions = {
        decrementSP,
        "@SP",
        "A=M",
        "D=M",
        "@" + fileName + "." + std::to_string(index),
        "M=D",
      };
      outFile << joinAssembly(instructions);
    }
    else {
      std::cerr << "Segment type of " + segment + " not handled for pop.";
    }
  }
  else {
    std::cerr << "Incorrect command type sent to writePushPop.\n";
  }

  return;
};

void writeComment(std::ofstream& outFile, std::string strLine) {
  outFile << "// " << strLine << "\n";
  return;
};

int main(int argc, char* argv[]) {
  // Initialize the read file
  std::ifstream inFile{ argv[1] };

  if (!inFile) {
    std::cerr << "Could not read file.";
    return 1;
  }

  // Open a new file and clear it if it already exists
  std::string outFileName = formatOutFileNameWithPath(argv[1]);
  std::ofstream outFile{outFileName};
  if (!outFile) {
    std::cerr << "Could not open file for writing.";
    return 1;
  }

  // Save file name without extension for use with "static" segment
  fileName = getFileNameWithoutExtension(argv[1]);
  
  // Initialize the stack pointer
  writeInitialSP(outFile);

  while (inFile) {
    std::string strLine;
    std::getline(inFile, strLine);

    // Ignore comments and empty lines
    if (strLine[0] == '/' || strLine[0] == '\r' ||
        strLine[0] == '\n' || strLine[0] ==  '\0')
      continue;

    // Write copy assembly code ass comment for debugging
    writeComment(outFile, strLine);

    // Parse line
    Command command(strLine);

    // Write the assembly code translations
    switch (command.type) {
      case CommandType::C_ARITHMETIC:
        writeArithmetic(outFile, command);
        break;
      case CommandType::C_PUSH:
      case CommandType::C_POP:
        writePushPop(outFile, command.type, command.arg1, command.arg2);
        break;
      default:
        std::cerr << "Cannot handle this command type.\n";
    }

  }
  writeEndLoop(outFile);
  return 0;
}
