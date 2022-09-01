#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <cmath>

enum InstructionType {
  A_INSTRUCTION,
  C_INSTRUCTION,
  L_INSTRUCTION,
  IGNORE
};

std::string cleanLine(std::string strLine)
{
  int strLength = strLine.length();

  std::string returnString;

  for (int i = 0; i < strLength; i++)
  {
    if (!isspace(strLine[i]))
    {
      if (strLine[i] == '/')
        break;
      else
        returnString += strLine[i];
    }
  }

  return returnString;
}

InstructionType getInstructionType(std::string strLine)
{
  char firstChar = strLine[0];

  if (firstChar == '@')
    return A_INSTRUCTION;
  else if (firstChar == '(')
    return L_INSTRUCTION;
  else if (firstChar == 'A'
           || firstChar == 'M'
           || firstChar == 'D'
           || firstChar == '0')
    return C_INSTRUCTION;
  else
    return IGNORE;
}

std::map<std::string, int> symbolTable = {
  {"R0", 0},
  {"R1", 1}, 
  {"R2", 2}, 
  {"R3", 3}, 
  {"R4", 4}, 
  {"R5", 5}, 
  {"R6", 6}, 
  {"R7", 7}, 
  {"R8", 8}, 
  {"R9", 9}, 
  {"R10", 10}, 
  {"R11", 11}, 
  {"R12", 12}, 
  {"R13", 13}, 
  {"R14", 14}, 
  {"R15", 15}, 
  {"SP", 0}, 
  {"LCL", 1}, 
  {"ARG", 2}, 
  {"THIS", 3}, 
  {"THAT", 4}, 
  {"SCREEN", 16384}, 
  {"KBD", 24576}, 
};

// Includes the 7th a-bit in acccccc section
std::map<std::string, std::string> compTable = {
  {"0",   "0101010"}, 
  {"1",   "0111111"}, 
  {"-1",  "0111010"}, 
  {"D",   "0001100"}, 
  {"A",   "0110000"}, 
  {"M",   "1110000"}, 
  {"!D",  "0001101"}, 
  {"!A",  "0110001"}, 
  {"!M",  "1110001"}, 
  {"-D",  "0001111"}, 
  {"-A",  "0110011"}, 
  {"-M",  "1110011"}, 
  {"D+1", "0011111"}, 
  {"A+1", "0110111"}, 
  {"M+1", "1110111"}, 
  {"D-1", "0001110"}, 
  {"A-1", "0110010"}, 
  {"M-1", "1110010"}, 
  {"D+A", "0000010"}, 
  {"D+M", "1000010"}, 
  {"D-A", "0010011"}, 
  {"D-M", "1010011"}, 
  {"A-D", "0000111"}, 
  {"M-D", "1000111"}, 
  {"D&A", "0000000"}, 
  {"D&M", "1000000"}, 
  {"D|A", "0010101"}, 
  {"D|M", "1010101"}, 
};

std::map<std::string, std::string> destTable = {
  {"",    "000"}, 
  {"M",   "001"}, 
  {"D",   "010"}, 
  {"MD",  "011"},
  {"A",   "100"}, 
  {"AM",  "101"}, 
  {"AD",  "110"}, 
  {"AMD", "111"}
};

std::map<std::string, std::string> jumpTable = {
  {"",    "000"}, 
  {"JGT", "001"}, 
  {"JEQ", "010"}, 
  {"JGE", "011"},
  {"JLT", "100"}, 
  {"JNE", "101"}, 
  {"JLE", "110"}, 
  {"JMP", "111"}
};

std::string translateCInstruction(std::string strLine)
{
  std::string dest = "";
  std::string comp = "";
  std::string jump = "";

  std::size_t equalsPos = strLine.find("=");
  std::size_t semicolonPos = strLine.find(";");

  if (semicolonPos != std::string::npos)
    jump = strLine.substr(semicolonPos + 1, 3); // Length of jump instructions is always 3

  if (equalsPos != std::string::npos)
  {
    dest = strLine.substr(0, equalsPos);
    if (semicolonPos != std::string::npos)
      comp = strLine.substr(equalsPos + 1, semicolonPos - equalsPos - 1);
    else
      comp = strLine.substr(equalsPos + 1, strLine.length() - semicolonPos);
  }
  else
    comp = strLine.substr(0, semicolonPos);
  
  std::string binary = "111" + compTable.at(comp) + destTable.at(dest) + jumpTable.at(jump);

  return binary; 
}

std::string translateAInstruction(std::string strLine)
{
  // Get the A-instruction string, ignoring the @ symbol and \r
  std::string aString = strLine.substr(1, strLine.length() - 1);

  std::string binary = "0"; // A-instructions have a leading 0 ...

  int aValue;
  
  try {
    aValue = std::stoi(aString);
  }
  catch (const std::invalid_argument) {
    aValue = symbolTable.find(aString)->second; 
  }

  // ... And are followed by a 15-bit representation of a number
  for(int i = 14; i >= 0; i--)
  {
    if ((aValue - std::pow(2, i)) >= 0)
    {
      binary += "1";
      aValue -= std::pow(2, i); 
    }
    else
      binary += "0";
  }

  return binary;
}

bool isNumber(std::string str) {
  std::string digits = "0123456789";
  for (int i = 0; i < str.length(); i++) {
    if (digits.find(str[i]) == std::string::npos)
      return false;
  }
  return true;
}

std::string parseSymbol(std::string strLine)
{
  return strLine.substr(1, strLine.length() - 1);

  /*if (!isNumber(symbol))
    return symbol;
  else
    return "";
  
   try {
    symbol = std::stoi(symbol);
    int x;
    sscanf(str, "%d", &x);

    if (symbol == "\0") // true in the case of @0 instruction
      return "";
    else
      return symbol;
  }
  catch (const std::invalid_argument) {
    return "";
  }

  return symbol;
  */
}

void buildSymbolTable(std::string inFileName)
{
  std::ifstream inFile{ inFileName };

  if (!inFile)
  {
    std::cerr << "File could not be opened for reading.\n";
    return;
  }

  int nextAvailableAddress = 16;
  int lineNum = 0;

  // Add L-instructions to the symbol table
  while (inFile)
  {
    std::string strLine;
    std::getline(inFile, strLine);
    strLine = cleanLine(strLine);

    InstructionType instructionType = getInstructionType(strLine);
    
    if (instructionType == IGNORE)
      continue;
    else if (instructionType == L_INSTRUCTION)
    {
      // Get just the symbol name inside the parens
      std::string symbol = strLine.substr(1, strLine.length() - 2);
      symbolTable.insert(std::pair<std::string, int>(symbol, lineNum));
    }
    else
      lineNum++;
  }

  // Go back to the beginning of the file
  inFile.clear();
  inFile.seekg(0);

  // Add named variables for the symbol table 
  while (inFile)
  {
    std::string strLine;
    std::getline(inFile, strLine);
    strLine = cleanLine(strLine);

    InstructionType instructionType = getInstructionType(strLine);
    
    if (instructionType == A_INSTRUCTION)
    {
      // Get just the symbol name inside the parens
      std::string symbol = parseSymbol(strLine);

      if (!isNumber(symbol) && symbolTable.find(symbol) == symbolTable.end())
      {
        symbolTable.insert(std::pair<std::string, int>(symbol, nextAvailableAddress));
        nextAvailableAddress++;
      }
    }
  }
}

int main(int argc, char* argv[])
{
  std::ifstream inFile{ argv[1] };

  if (!inFile)
  {
    std::cerr << "File could not be opened for reading.\n";
    return 1;
  }

  std::ofstream outFile{ argv[2] };

  if (!outFile)
  {
    std::cerr << "File could not be opened for writing.\n";
    return 1;
  }

  buildSymbolTable(argv[1]);

  while (inFile)
  {
    std::string strLine;
    std::getline(inFile, strLine);
    strLine = cleanLine(strLine);

    InstructionType instructionType = getInstructionType(strLine);

    if (instructionType == IGNORE || instructionType == L_INSTRUCTION)
      continue;

    std::cout << strLine << "\n";

    if (instructionType == A_INSTRUCTION)
    {
      std::cout << translateAInstruction(strLine) << '\n';
      outFile << translateAInstruction(strLine) <<  '\n';
    }

    if (instructionType == C_INSTRUCTION)
    {
      std::cout << translateCInstruction(strLine) << '\n';
      outFile << translateCInstruction(strLine) << '\n';
    }
  }

  return 0;
}
