#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <stack>
#include <array>
#ifdef VIRTUOSO_COMPILER
#include <map>
#include <iomanip>
#include <regex>
#include <sstream>
#endif

#include "VirtuosoOpcode.h"

namespace Virtuoso {

#ifdef VIRTUOSO_COMPILER
class Instruction {
   public:
    Instruction(Opcode op, const std::string& filename, int lineNumber, int columnNumber);
    void addLiteralArgument(int value);
    void setOpcode(Opcode value);
    Opcode opcode() const { return m_opcode; }
    void toByteCodes(uint8_t* &pc) const;
    std::vector<uint8_t> toByteList() const;
    void error(std::string msg);
    int lineNumer() const { return m_lineNumber; }
    bool isLabel() const { return m_isLabel; }
    bool isJumpToLabel() const { return m_isJumpToLabel; }
    const std::string& labelName() const { return m_labelName; }
    bool isSubroutine() const { return m_isSubroutine; }
    bool isCall() const { return m_isCall; }
    static Instruction newSubroutine(std::string name, std::string filename, int column_number, int line_number);
    static Instruction newCall(std::string name, std::string filename, int column_number, int line_number);
    static Instruction newLabel(std::string name, std::string filename, int column_number, int line_number);
    static Instruction newJumpToLabel(std::string name, std::string filename, int column_number, int line_number);
    static Instruction newConditionalJumpToLabel(std::string name, std::string filename, int column_number, int line_number);
    void completeLiterals();

   private:
    Opcode m_opcode;
    std::string m_filename;
    int m_lineNumber;
    int m_columnNumber;
    std::string m_labelName;
    bool m_isSubroutine = false;
    bool m_isCall = false;
    bool m_isLabel = false;
    bool m_isJumpToLabel = false;
    std::vector<uint16_t> m_literalArguments;
};

class Program {
   public:
    Program(const std::string& script);

    size_t getByteCodes(std::vector<uint8_t> &byteCodes, std::vector<uint16_t> &subroutines) const;
    std::vector<uint8_t> getByteList() const;
    uint16_t getCRC() const;
    std::string toString() const;

   private:
    enum class BlockType { BEGIN = 0, IF, ELSE, INVALID };
    enum class Mode { NORMAL, GOTO, SUBROUTINE };

    void addLiteral(int literal, const std::string& filename, int lineNumber, int columnNumber);

    void openBlock(BlockType blocktype, const std::string& filename, int line_number, int column_number);
    BlockType getCurrentBlockType() const;
    std::string getCurrentBlockStartLabel() const;
    std::string getCurrentBlockEndLabel() const;
    std::string getNextBlockEndLabel() const;

    void closeBlock(const std::string& filename, int line_number, int column_number);
    void completeJumps();
    void completeCalls();
    void completeLiterals();

    void parseGoto(std::string s, std::string filename, int line_number, int column_number, Mode& mode);
    void parseSubroutine(std::string s, std::string filename, int line_number, int column_number, Mode& mode);
    bool looksLikeLiteral(const std::string& s);
    void parseString(std::string s, std::string filename, int line_number, int column_number, Mode& mode);

    Instruction& findLabel(const std::string& name);

    std::vector<std::string> m_sourceLines;
    std::vector<Instruction> m_instructionList;
    std::map<std::string, uint16_t> m_subroutineAddresses;
    std::map<std::string, Opcode> m_subroutineCommands;
    int m_maxBlock = 0;
    std::stack<int> m_openBlocks;
    std::stack<BlockType> m_openBlockTypes;
};

Instruction::Instruction(Opcode op, const std::string& filename, int lineNumber, int columnNumber)
    : m_opcode(op), m_filename(filename), m_lineNumber(lineNumber), m_columnNumber(columnNumber) {}

void Instruction::addLiteralArgument(int value) {
    m_literalArguments.push_back(value);
    if (m_literalArguments.size() > 126) {
        throw "Too many literals (> 126) in a row: this will overflow the stack.";
    }
}

void Instruction::setOpcode(Opcode value) {
    if (m_opcode != Opcode::QUIT) {
        throw "The opcode has already been set.";
    }
    m_opcode = value;
}

void Instruction::toByteCodes(uint8_t* &pc) const {
    if (m_isLabel || m_isSubroutine) {
        return;
    }
    *pc++ = ((uint8_t)m_opcode);
    if (m_opcode == Opcode::LITERAL || m_opcode == Opcode::JUMP || m_opcode == Opcode::JUMP_Z || m_opcode == Opcode::CALL) {
        if (m_literalArguments.empty()) {
            *pc++ = 0;
            *pc++ = 0;
        } else {
            *pc++ = uint8_t(m_literalArguments[0] % 256);
            *pc++ = uint8_t(m_literalArguments[0] / 256);
        }
    } else if (m_opcode == Opcode::LITERAL8) {
        *pc++ = uint8_t(m_literalArguments[0]);
    } else {
        if (m_opcode == Opcode::LITERAL_N) {
            *pc++ = uint8_t(m_literalArguments.size() * 2);
            for (uint16_t literalArgument : m_literalArguments) {
                *pc++ = uint8_t(literalArgument % 256);
                *pc++ = uint8_t(literalArgument / 256);
            }
        }
        if (m_opcode == Opcode::LITERAL8_N) {
            *pc++ = uint8_t(m_literalArguments.size());
            for (uint16_t literalArgument : m_literalArguments) {
                *pc++ = uint8_t(literalArgument);
            }
        }
    }
}

std::vector<uint8_t> Instruction::toByteList() const {
    std::vector<uint8_t> list;
    if (m_isLabel || m_isSubroutine) {
        return list;
    }
    list.push_back((uint8_t)m_opcode);
    if (m_opcode == Opcode::LITERAL || m_opcode == Opcode::JUMP || m_opcode == Opcode::JUMP_Z || m_opcode == Opcode::CALL) {
        if (m_literalArguments.empty()) {
            list.push_back(0);
            list.push_back(0);
        } else {
            list.push_back(uint8_t(m_literalArguments[0] % 256));
            list.push_back(uint8_t(m_literalArguments[0] / 256));
        }
    } else if (m_opcode == Opcode::LITERAL8) {
        list.push_back(uint8_t(m_literalArguments[0]));
    } else {
        if (m_opcode == Opcode::LITERAL_N) {
            list.push_back(uint8_t(m_literalArguments.size() * 2));
            for (uint16_t literalArgument : m_literalArguments) {
                list.push_back(uint8_t(literalArgument % 256));
                list.push_back(uint8_t(literalArgument / 256));
            }
        }
        if (m_opcode == Opcode::LITERAL8_N) {
            list.push_back(uint8_t(m_literalArguments.size()));
            for (uint16_t literalArgument : m_literalArguments) {
                list.push_back(uint8_t(literalArgument));
            }
        }
    }
    return list;
}

void Instruction::error(std::string msg) {
    throw m_filename + ":" + std::to_string(m_lineNumber) + ":" + std::to_string(m_columnNumber) + ": " + msg;
}

Instruction Instruction::newSubroutine(std::string name, std::string filename, int column_number, int line_number) {
    Instruction bytecodeInstruction(Opcode::QUIT, filename, column_number, line_number);
    bytecodeInstruction.m_isSubroutine = true;
    bytecodeInstruction.m_labelName = name;
    return bytecodeInstruction;
}

Instruction Instruction::newCall(std::string name, std::string filename, int column_number, int line_number) {
    Instruction bytecodeInstruction(Opcode::QUIT, filename, column_number, line_number);
    bytecodeInstruction.m_isCall = true;
    bytecodeInstruction.m_labelName = name;
    return bytecodeInstruction;
}

Instruction Instruction::newLabel(std::string name, std::string filename, int column_number, int line_number) {
    Instruction bytecodeInstruction(Opcode::QUIT, filename, column_number, line_number);
    bytecodeInstruction.m_isLabel = true;
    bytecodeInstruction.m_labelName = name;
    return bytecodeInstruction;
}

Instruction Instruction::newJumpToLabel(std::string name, std::string filename, int column_number, int line_number) {
    Instruction bytecodeInstruction(Opcode::JUMP, filename, column_number, line_number);
    bytecodeInstruction.m_isJumpToLabel = true;
    bytecodeInstruction.m_labelName = name;
    return bytecodeInstruction;
}

Instruction Instruction::newConditionalJumpToLabel(std::string name, std::string filename, int column_number, int line_number) {
    Instruction bytecodeInstruction(Opcode::JUMP_Z, filename, column_number, line_number);
    bytecodeInstruction.m_isJumpToLabel = true;
    bytecodeInstruction.m_labelName = name;
    return bytecodeInstruction;
}

void Instruction::completeLiterals() {
    if (m_opcode != Opcode::LITERAL) {
        return;
    }
    bool flag = false;
    for (uint16_t literalArgument : m_literalArguments) {
        if (literalArgument > 255) {
            flag = true;
            break;
        }
    }
    if (flag && m_literalArguments.size() > 1) {
        m_opcode = Opcode::LITERAL_N;
    } else if (flag && m_literalArguments.size() == 1) {
        m_opcode = Opcode::LITERAL;
    } else if (m_literalArguments.size() > 1) {
        m_opcode = Opcode::LITERAL8_N;
    } else {
        m_opcode = Opcode::LITERAL8;
    }
}

const std::map<std::string, Opcode> dictionary = {{"QUIT", Opcode::QUIT},
                                                  {"LITERAL", Opcode::LITERAL},
                                                  {"LITERAL8", Opcode::LITERAL8},
                                                  {"LITERAL_N", Opcode::LITERAL_N},
                                                  {"LITERAL8_N", Opcode::LITERAL8_N},
                                                  {"RETURN", Opcode::RETURN},
                                                  {"JUMP", Opcode::JUMP},
                                                  {"JUMP_Z", Opcode::JUMP_Z},
                                                  {"DELAY", Opcode::DELAY},
                                                  {"GET_MS", Opcode::GET_MS},
                                                  {"DEPTH", Opcode::DEPTH},
                                                  {"DROP", Opcode::DROP},
                                                  {"DUP", Opcode::DUP},
                                                  {"OVER", Opcode::OVER},
                                                  {"PICK", Opcode::PICK},
                                                  {"SWAP", Opcode::SWAP},
                                                  {"ROT", Opcode::ROT},
                                                  {"ROLL", Opcode::ROLL},
                                                  {"BITWISE_NOT", Opcode::BITWISE_NOT},
                                                  {"BITWISE_AND", Opcode::BITWISE_AND},
                                                  {"BITWISE_OR", Opcode::BITWISE_OR},
                                                  {"BITWISE_XOR", Opcode::BITWISE_XOR},
                                                  {"SHIFT_RIGHT", Opcode::SHIFT_RIGHT},
                                                  {"SHIFT_LEFT", Opcode::SHIFT_LEFT},
                                                  {"LOGICAL_NOT", Opcode::LOGICAL_NOT},
                                                  {"LOGICAL_AND", Opcode::LOGICAL_AND},
                                                  {"LOGICAL_OR", Opcode::LOGICAL_OR},
                                                  {"NEGATE", Opcode::NEGATE},
                                                  {"PLUS", Opcode::PLUS},
                                                  {"MINUS", Opcode::MINUS},
                                                  {"TIMES", Opcode::TIMES},
                                                  {"DIVIDE", Opcode::DIVIDE},
                                                  {"MOD", Opcode::MOD},
                                                  {"POSITIVE", Opcode::POSITIVE},
                                                  {"NEGATIVE", Opcode::NEGATIVE},
                                                  {"NONZERO", Opcode::NONZERO},
                                                  {"EQUALS", Opcode::EQUALS},
                                                  {"NOT_EQUALS", Opcode::NOT_EQUALS},
                                                  {"MIN", Opcode::MIN},
                                                  {"MAX", Opcode::MAX},
                                                  {"LESS_THAN", Opcode::LESS_THAN},
                                                  {"GREATER_THAN", Opcode::GREATER_THAN},
                                                  {"SERVO", Opcode::SERVO},
                                                  {"SERVO_8BIT", Opcode::SERVO_8BIT},
                                                  {"SPEED", Opcode::SPEED},
                                                  {"ACCELERATION", Opcode::ACCELERATION},
                                                  {"GET_POSITION", Opcode::GET_POSITION},
                                                  {"GET_MOVING_STATE", Opcode::GET_MOVING_STATE},
                                                  {"LED_ON", Opcode::LED_ON},
                                                  {"LED_OFF", Opcode::LED_OFF},
                                                  {"PWM", Opcode::PWM},
                                                  {"PEEK", Opcode::PEEK},
                                                  {"POKE", Opcode::POKE},
                                                  {"SERIAL_SEND_BYTE", Opcode::SERIAL_SEND_BYTE},
                                                  {"CALL", Opcode::CALL},
                                                  {"SERVOX", Opcode::SERVOX},
                                                  {"SERIAL_SEND", Opcode::SERIAL_SEND},
                                              };

Program::Program(const std::string& program) {
    const std::regex eol_re("\\n|\\r\\n");
    for (std::sregex_token_iterator line_iter(program.begin(), program.end(), eol_re, -1); line_iter != std::sregex_token_iterator(); ++line_iter) {
        std::string text_line = *line_iter;
        m_sourceLines.push_back(text_line);
    }

    Mode mode = Mode::NORMAL;

    int line_number = 0;
    int column_number = 0;
    const std::regex ws_re("\\s|\\t");
    for (auto text_line : m_sourceLines) {
        column_number = 1;

        // remove comments
        text_line = std::regex_replace(text_line, std::regex("#.*"), "");

        for (std::sregex_token_iterator token_iter(text_line.begin(), text_line.end(), ws_re, -1); token_iter != std::sregex_token_iterator();
             ++token_iter) {
            std::string token = *token_iter;
            if (token.empty()) {
                column_number++;
                continue;
            }
            // To upper case
            std::transform(token.begin(), token.end(), token.begin(), ::toupper);

            if (mode == Mode::NORMAL) {
                parseString(token, "script", line_number, column_number, mode);
            } else if (mode == Mode::GOTO) {
                parseGoto(token, "script", line_number, column_number, mode);
            } else if (mode == Mode::SUBROUTINE) {
                parseSubroutine(token, "script", line_number, column_number, mode);
            }
            column_number += token.size() + 1;
        }
        line_number++;
    }
    if (!m_openBlocks.empty()) {
        const std::string currentBlockStartLabel = getCurrentBlockStartLabel();
        Instruction& bytecodeInstruction = findLabel(currentBlockStartLabel);
        bytecodeInstruction.error("BEGIN block was never closed.");
    }
    completeLiterals();
    completeCalls();
    completeJumps();
}

std::string Program::toString() const {
    if (m_instructionList.empty()) return {};

    std::ostringstream streamWriter;
    int num = 0;
    int num2 = 0;
    Instruction bytecodeInstruction(m_instructionList[num]);

    for (size_t line_number = 0; line_number < m_sourceLines.size(); line_number++) {
        int column_number = 0;
        streamWriter << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << num2 << ": ";
        while (bytecodeInstruction.lineNumer() == line_number) {
            for (uint8_t item : bytecodeInstruction.toByteList()) {
                streamWriter << std::setw(2) << int(item);
                num2++;
                column_number += 2;
            }
            num++;
            if (num >= m_instructionList.size()) break;
            bytecodeInstruction = m_instructionList[num];
        }
        for (int j = 0; j < 20 - column_number; j++) {
            streamWriter << " ";
        }
        streamWriter << " -- " << m_sourceLines[line_number] << std::endl;
    }
    streamWriter << std::endl;
    streamWriter << "Subroutines:" << std::endl;
    streamWriter << "Hex Decimal Address Name" << std::endl;
    std::array<std::string, 128> array;
    for (const auto& subroutineAddress : m_subroutineAddresses) {
        std::string key = subroutineAddress.first;
        if (m_subroutineCommands.at(key) != Opcode::CALL) {
            uint8_t b = (uint8_t)(int(m_subroutineCommands.at(key)) - 128);
            uint16_t num4 = m_subroutineAddresses.at(key);

            std::ostringstream str;
            str << std::uppercase << std::setfill('0') << std::hex << std::setw(2) << int(b) << "  " << std::dec << std::setw(3) << int(b) << "     "
                << std::hex << std::setw(4) << num4 << "    " << key << std::endl;
            array[b] = str.str();
        }
    }
    for (const auto& str : array) {
        streamWriter << str;
    }
    for (const auto& subroutineAddress : m_subroutineAddresses) {
        const std::string key = subroutineAddress.first;
        if (m_subroutineCommands.at(key) == Opcode::CALL) {
            streamWriter << "--  ---     " << std::hex << std::setw(4) << m_subroutineAddresses.at(key) << "    " << key;
        }
    }
    return streamWriter.str();
}

void Program::addLiteral(int literal, const std::string& filename, int lineNumber, int columnNumber) {
    if (m_instructionList.empty() || m_instructionList.back().opcode() != Opcode::LITERAL) {
        m_instructionList.push_back(Instruction(Opcode::LITERAL, filename, lineNumber, columnNumber));
    }
    m_instructionList.back().addLiteralArgument(literal);
}

size_t Program::getByteCodes(std::vector<uint8_t> &byteCodes, std::vector<uint16_t> &subroutines) const {
    uint8_t buffer[256];
    size_t size = 0;
    for (const Instruction& instruction : m_instructionList) {
        uint8_t* pc = buffer;
        instruction.toByteCodes(pc);
        size += pc - buffer;
    }
    byteCodes.resize(size);
    uint8_t* pc = byteCodes.data();
    for (const Instruction& instruction : m_instructionList) {
        instruction.toByteCodes(pc);
    }
    size_t subroutineCount = 0;
    for (const auto& subroutineAddress : m_subroutineAddresses) {
        std::string key = subroutineAddress.first;
        if (m_subroutineCommands.at(key) != Opcode::CALL) {
            subroutineCount += 1;
        }
    }
    subroutines.resize(subroutineCount);
    for (const auto& subroutineAddress : m_subroutineAddresses) {
        std::string key = subroutineAddress.first;
        if (m_subroutineCommands.at(key) != Opcode::CALL) {
            uint8_t b = (uint8_t)(int(m_subroutineCommands.at(key)) - 128);
            auto address = m_subroutineAddresses.at(key);
            subroutines[b] = address;
        }
    }
    return size;
}

std::vector<uint8_t> Program::getByteList() const {
    std::vector<uint8_t> list;
    for (const Instruction& instruction : m_instructionList) {
        list.insert(list.end(), instruction.toByteList().begin(), instruction.toByteList().end());
    }
    return list;
}

void Program::openBlock(BlockType blocktype, const std::string& filename, int line_number, int column_number) {
    m_instructionList.push_back(Instruction::newLabel("block_start_" + std::to_string(m_maxBlock), filename, line_number, column_number));
    m_openBlocks.push(m_maxBlock);
    m_openBlockTypes.push(blocktype);
    m_maxBlock++;
}

Program::BlockType Program::getCurrentBlockType() const { return !m_openBlocks.empty() ? m_openBlockTypes.top() : BlockType::INVALID; }

std::string Program::getCurrentBlockStartLabel() const { return "block_start_" + std::to_string(m_openBlocks.top()); }

std::string Program::getCurrentBlockEndLabel() const { return "block_end_" + std::to_string(m_openBlocks.top()); }

std::string Program::getNextBlockEndLabel() const { return "block_end_" + std::to_string(m_maxBlock); }

Instruction& Program::findLabel(const std::string& name) {
    for (auto& instruction : m_instructionList) {
        if (instruction.isLabel() && instruction.labelName() == name) {
            return instruction;
        }
    }
    throw "Label not found.";
}

void Program::closeBlock(const std::string& filename, int line_number, int column_number) {
    m_instructionList.push_back(Instruction::newLabel("block_end_" + std::to_string(m_openBlocks.top()), filename, line_number, column_number));
    m_openBlocks.pop();
    m_openBlockTypes.pop();
}

void Program::completeJumps() {
    std::map<std::string, int> dictionary;
    int num = 0;
    for (Instruction& instruction : m_instructionList) {
        if (instruction.isLabel()) {
            if (dictionary.find(instruction.labelName()) != dictionary.end()) {
                instruction.error("The label " + instruction.labelName() + " has already been used.");
            }
            dictionary[instruction.labelName()] = num;
        }
        num += instruction.toByteList().size();
    }
    for (Instruction& instruction : m_instructionList) {
        try {
            if (instruction.isJumpToLabel()) {
                instruction.addLiteralArgument(dictionary.at(instruction.labelName()));
            }
        } catch (std::exception& e) {
            instruction.error("The label " + instruction.labelName() + " was not found.");
        }
    }
}

void Program::completeCalls() {
    uint32_t num_subroutines = 128;
    for (Instruction& instruction : m_instructionList) {
        if (instruction.isSubroutine()) {
            if (m_subroutineCommands.find(instruction.labelName()) != m_subroutineCommands.end()) {
                instruction.error("The subroutine " + instruction.labelName() + " has already been defined.");
            }
            m_subroutineCommands[instruction.labelName()] = (num_subroutines >= 256) ? Opcode::CALL : Opcode(num_subroutines);
            num_subroutines++;
            if (num_subroutines > 255) {
                instruction.error("Too many subroutines.  The limit is 255.");
            }
        }
    }
    for (Instruction& instruction : m_instructionList) {
        try {
            if (instruction.isCall()) {
                instruction.setOpcode(m_subroutineCommands[instruction.labelName()]);
            }
        } catch (std::exception& e) {
            instruction.error("Did not understand '" + instruction.labelName() + "'");
        }
    }
    uint16_t address = 0;
    for (const Instruction& instruction : m_instructionList) {
        if (instruction.isSubroutine()) {
            m_subroutineAddresses[instruction.labelName()] = address;
        }
        address += instruction.toByteList().size();
    }
    for (Instruction& instruction : m_instructionList) {
        if (instruction.opcode() == Opcode::CALL) {
            instruction.addLiteralArgument(m_subroutineAddresses[instruction.labelName()]);
        }
    }
}

void Program::completeLiterals() {
    for (Instruction& instruction : m_instructionList) {
        instruction.completeLiterals();
    }
}

uint16_t oneByteCRC(uint8_t v) {
    const uint16_t CRC16_POLY = 40961;
    uint16_t num = v;
    for (int i = 0; i < 8; i++) {
        num = (((num & 1) != 1) ? ((uint16_t)(num >> 1)) : ((uint16_t)((uint32_t)(num >> 1) ^ CRC16_POLY)));
    }
    return num;
}

uint16_t CRC(const std::vector<uint8_t>& message) {
    uint16_t num = 0;
    for (uint16_t num2 = 0; num2 < message.size(); num2 = (uint16_t)(num2 + 1)) {
        num = (uint16_t)((num >> 8) ^ oneByteCRC((uint8_t)((uint8_t)num ^ message[num2])));
    }
    return num;
}

uint16_t Program::getCRC() const {
    std::vector<uint8_t> list;
    std::array<uint16_t, 128> array;
    for (const auto& keyValue : m_subroutineCommands) {
        if (m_subroutineCommands.at(keyValue.first) != Opcode::CALL) {
            array[uint8_t(m_subroutineCommands.at(keyValue.first)) - 128] = m_subroutineAddresses.at(keyValue.first);
        }
    }
    for (uint16_t num : array) {
        list.push_back((uint8_t)(num & 0xFFu));
        list.push_back((uint8_t)(num >> 8));
    }
    list.insert(list.end(), getByteList().begin(), getByteList().end());
    return CRC(list);
}

void Program::parseGoto(std::string s, std::string filename, int line_number, int column_number, Mode& mode) {
    m_instructionList.push_back(Instruction::newJumpToLabel("USER_" + s, filename, line_number, column_number));
    mode = Mode::NORMAL;
}

void Program::parseSubroutine(std::string s, std::string filename, int line_number, int column_number, Mode& mode) {
    if (looksLikeLiteral(s)) {
        throw "The name " + s + " is not valid as a subroutine name (it looks like a number).";
    }
    if (dictionary.find(s) != dictionary.end()) {
        throw "The name " + s + " is not valid as a subroutine name (it is a built-in command).";
    }
    const std::vector<std::string> keywords = {"GOTO", "SUB", "BEGIN", "WHILE", "REPEAT", "IF", "ENDIF", "ELSE"};
    for (std::string keyword : keywords) {
        if (keyword == s) {
            throw "The name " + s + " is not valid as a subroutine name (it is a keyword).";
        }
    }
    m_instructionList.push_back(Instruction::newSubroutine(s, filename, line_number, column_number));
    mode = Mode::NORMAL;
}

bool Program::looksLikeLiteral(const std::string& s) {
    return std::regex_match(s, std::regex("^-?[0-9.]+$")) || std::regex_match(s, std::regex("^0[xX][0-9a-fA-F.]+$"));
}

void Program::parseString(std::string s, std::string filename, int line_number, int column_number, Mode& mode) {
    try {
        if (looksLikeLiteral(s)) {
            int num;
            if (s.rfind("0X", 0) == 0) {
                num = std::stoi(s, nullptr, 16);
                if (num > 65535 || num < 0) {
                    throw "Value " + s + " is not in the allowed range of " + std::to_string(std::numeric_limits<uint16_t>::min()) + " to " +
                        std::to_string(std::numeric_limits<uint16_t>::max()) + ".";
                }
            } else {
                num = std::stoi(s);
                if (num > 32767 || num < -32768) {
                    throw "Value " + s + " is not in the allowed range of " + std::to_string(std::numeric_limits<int16_t>::min()) + " to " +
                        std::to_string(std::numeric_limits<int16_t>::max()) + ".";
                }
            }
            int literal = (int16_t)(long)(num % 65535);
            addLiteral(literal, filename, line_number, column_number);
            return;
        }
    } catch (std::exception& ex) {
        throw "Error parsing " + s + ": " + ex.what();
    }
    if (s == "GOTO") {
        mode = Mode::GOTO;
        return;
    }
    if (s == "SUB") {
        mode = Mode::SUBROUTINE;
        return;
    }
    std::smatch base_match;
    const std::regex base_regex("(.*):$");
    if (std::regex_match(s, base_match, base_regex)) {
        m_instructionList.push_back(Instruction::newLabel("USER_" + base_match[1].str(), filename, line_number, column_number));
        return;
    }
    if (s == "BEGIN") {
        openBlock(BlockType::BEGIN, filename, line_number, column_number);
        return;
    }
    if (s == "WHILE") {
        if (getCurrentBlockType() != BlockType::BEGIN) {
            throw "WHILE must be inside a BEGIN...REPEAT block";
        }
        m_instructionList.push_back(Instruction::newConditionalJumpToLabel(getCurrentBlockEndLabel(), filename, line_number, column_number));
        return;
    }
    if (s == "REPEAT") {
        try {
            if (getCurrentBlockType() != BlockType::BEGIN) {
                throw "REPEAT must end a BEGIN...REPEAT block";
            }
            m_instructionList.push_back(Instruction::newJumpToLabel(getCurrentBlockStartLabel(), filename, line_number, column_number));
            closeBlock(filename, line_number, column_number);
        } catch (std::exception& ex) {
            throw filename + ":" + std::to_string(line_number) + ":" + std::to_string(column_number) + ": Found REPEAT without a corresponding BEGIN";
        }
        return;
    }
    if (s == "IF") {
        openBlock(BlockType::IF, filename, line_number, column_number);
        m_instructionList.push_back(Instruction::newConditionalJumpToLabel(getCurrentBlockEndLabel(), filename, line_number, column_number));
        return;
    }
    if (s == "ENDIF") {
        try {
            if (getCurrentBlockType() != BlockType::IF && getCurrentBlockType() != BlockType::ELSE) {
                throw "ENDIF must end an IF...ENDIF or an IF...ELSE...ENDIF block.";
            }
            closeBlock(filename, line_number, column_number);
        } catch (std::exception& ex) {
            throw filename + ":" + std::to_string(line_number) + ":" + std::to_string(column_number) + ": Found ENDIF without a corresponding IF";
        }
        return;
    }
    if (s == "ELSE") {
        try {
            if (getCurrentBlockType() != BlockType::IF) {
                throw "ELSE must be part of an IF...ELSE...ENDIF block.";
            }
            m_instructionList.push_back(Instruction::newJumpToLabel(getNextBlockEndLabel(), filename, line_number, column_number));
            closeBlock(filename, line_number, column_number);
            openBlock(BlockType::ELSE, filename, line_number, column_number);
        } catch (std::exception& ex) {
            throw filename + ":" + std::to_string(line_number) + ":" + std::to_string(column_number) + ": Found ELSE without a corresponding IF";
        }
        return;
    }
    if (dictionary.find(s) != dictionary.end()) {
        const Opcode opcode = dictionary.at(s);
        switch (opcode) {
            case Opcode::LITERAL:
            case Opcode::LITERAL8:
            case Opcode::LITERAL_N:
            case Opcode::LITERAL8_N:
                throw filename + ":" + std::to_string(line_number) + ":" + std::to_string(column_number) +
                    ": Literal commands may not be used directly in a program.  Integers should be entered directly.";
            case Opcode::JUMP:
            case Opcode::JUMP_Z:
                throw filename + ":" + std::to_string(line_number) + ":" + std::to_string(column_number) +
                    ": Jumps may not be used directly in a program.";
            default:
                ;
        }
        m_instructionList.push_back(Instruction(opcode, filename, line_number, column_number));
    } else {
        m_instructionList.push_back(Instruction::newCall(s, filename, line_number, column_number));
    }
}
#endif

#ifdef VIRTUOSO_INTERPRETER
class Virtuosopreter {
public:
    enum
    {
        ERROR_SERIAL_SIGNAL = 1<<0,
        ERROR_SERIAL_OVERRUN = 1<<1,
        ERROR_SERIAL_BUFFER = 1<<2,
        ERROR_SERIAL_CRC = 1<<3,
        ERROR_SERIAL_PROTOCOL = 1<<4,
        ERROR_SERIAL_TIMEOUT = 1<<5,
        ERROR_SCRIPT_STACK_ERROR = 1<<6,
        ERROR_CALL_STACK_ERROR = 1<<7,
        ERROR_PROGRAM_COUNTER_ERROR = 1<<8
    };
    Virtuosopreter(
            const std::vector<uint8_t>& byteCodes,
            const std::vector<uint16_t>& subroutines) :
        fByteCodes(byteCodes),
        fSubroutines(subroutines)
    {
        reset();
    }

    /** \brief Reset the controller.
     *
     * Reset the controller.
     */
    virtual void reset()
    {
        fPC = 0;
        fSP = 0;
        fCSP = 0;
        fErrors = 0;
        fRunning = true;
    }

    /** \brief Stops the script.
     *
     * Stops the script, if it is currently running.
     */
    virtual void stopScript()
    {
        fRunning = false;
    }

    /** \brief Starts loaded script at specified \a subroutineNumber location.
     *
     * @param subroutineNumber A subroutine number defined in script's compiled
     * code.
     */
    void restartScript(unsigned subroutineNumber = 0);

    /** \brief Starts loaded script at specified \a subroutineNumber location
     *  after loading \a parameter on to the stack.
     *
     * @param subroutineNumber A subroutine number defined in script's compiled
     * code.
     *
     * @param parameter A number from 0 to 16383.
     *
     * Similar to the \p restartScript function, except it loads the parameter
     * on to the stack before starting the script at the specified subroutine
     * number location.
     */
    void restartScriptWithParameter(unsigned subroutineNumber, unsigned parameter);

    void execute();

    /** \brief Sends the servos and outputs to home position.
     *
     * If the "On startup or error" setting for a servo or output channel is set
     * to "Ignore", the position will be unchanged.
     */
    virtual void goHome()
    {
        // Not supported
    }

    /** \brief Sets the \a target of the servo on \a channelNumber.
     *
     * @param channelNumber A servo number from 0 to 127.
     *
     * @param target A number from 0 to 16383.
     *
     * If the channel is configured as a servo, then the target represents the
     * pulse width to transmit in units of quarter-microseconds. A \a target
     * value of 0 tells the Virtuoso to stop sending pulses to the servo.
     */
    virtual void setTarget(unsigned channel, unsigned pulseQuarterMS)
    {
        printf("SERVO{%u}: %u => %u.%u\n", channel, pulseQuarterMS, pulseQuarterMS >> 2, (pulseQuarterMS&3)*25);
    }

    /** \brief Sets the \a target of the servo on \a channelNumber.
     *
     * @param channelNumber A servo number from 0 to 127.
     *
     * @param target A number from 0 to 1000.
     */
    virtual void setTarget(unsigned channel, unsigned permill, unsigned duration, unsigned easing)
    {
        printf("SERVOX{%u,%u,%u}\n", channel, permill, duration, easing);
    }

    /** \brief Sets the \a speed limit of \a channelNumber.
     *
     * @param channelNumber A servo number from 0 to 127.
     *
     * @param speed A number from 0 to 16383.
     *
     * Limits the speed a servo channel’s output value changes.
     */
    virtual void setSpeed(unsigned channel, unsigned speed)
    {
        printf("SPEED{%u}: %u\n", channel, speed);
    }

     /** \brief Sets the \a acceleration limit of \a channelNumber.
     *
     * @param channelNumber A servo number from 0 to 127.
     *
     * @param acceleration A number from 0 to 16383.
     *
     * Limits the acceleration a servo channel’s output value changes.
     */
    virtual void setAccelleration(unsigned channel, unsigned accelleration)
    {
        printf("ACCELERATION{%u}: %u\n", channel, accelleration);
    }

    virtual void delayMS(unsigned ms)
    {
        printf("DELAY: %u\n", ms);
    }

    /** \brief Gets the position of \a channelNumber.
     *
     * @param channelNumber A servo number from 0 to 127.
     *
     * @return two-byte position value
     *
     * If channel is configured as a servo, then the position value represents
     * the current pulse width transmitted on the channel in units of
     * quarter-microseconds.
     */
    virtual uint16_t getPosition(unsigned channel)
    {
        printf("GET POSITION: %u\n", channel);
        return 499;
    }

    /** \brief Gets the moving state for all configured servo channels.
     *
     * @return 1 if at least one servo limited by speed or acceleration is still
     * moving, 0 if not.
     *
     * Determines if the servo outputs have reached their targets or are still
     * changing and will return 1 as as long as there is at least one servo that
     * is limited by a speed or acceleration setting.
     */
    virtual bool getMovingState(unsigned channel)
    {
        printf("GET MOVING STATE: %u\n", channel);
        return false;
    }

    /** \brief Gets if the script is running or stopped.
     *
     * @return 1 if script is stopped, 0 if running.
     */
    bool getScriptStatus()
    {
        return !fRunning;
    }

    /** \brief Gets the error register.
     *
     * @return Two-byte error code.
     */
    uint16_t getErrors()
    {
        return fErrors;
    }

    virtual void ledState(bool on)
    {
        printf("LED: %s\n", (on ? "ON" : "OFF"));
    }

    virtual void serialSendByte(uint8_t byte)
    {
        printf("SEND BYTE: 0x%02X\n", byte);
    }

private:
    void push(int16_t val)
    {
        if (fSP >= fStack.size())
        {
            fErrors |= ERROR_SCRIPT_STACK_ERROR;
            return;
        }
        fStack[fSP++] = val;
    }

    int16_t pop()
    {
        if (fSP == 0)
        {
            fErrors |= ERROR_SCRIPT_STACK_ERROR;
            return 0;
        }
        return fStack[--fSP];
    }

    void pushCallstack(uint16_t val)
    {
        if (fCSP >= fCallStack.size())
        {
            fErrors |= ERROR_CALL_STACK_ERROR;
            return;
        }
        fCallStack[fCSP++] = val;
    }

    uint16_t popCallstack()
    {
        if (fCSP == 0)
        {
            fErrors |= ERROR_CALL_STACK_ERROR;
            return 0;
        }
        return fCallStack[--fCSP];
    }

    unsigned fPC = 0;
    unsigned fSP = 0;
    unsigned fCSP = 0;
    bool fRunning = false;
    std::array<int16_t, 126> fStack;
    std::array<uint16_t, 126> fCallStack;
    const std::vector<uint8_t>& fByteCodes;
    const std::vector<uint16_t>& fSubroutines;
    uint8_t fErrors = 0;
};

void Virtuosopreter::restartScript(unsigned subroutineNumber)
{
    reset();
    fPC = fSubroutines[subroutineNumber & 0x7F];
    fRunning = true;
}

void Virtuosopreter::restartScriptWithParameter(unsigned subroutineNumber, unsigned parameter)
{
    reset();
    push(parameter);
    fPC = fSubroutines[subroutineNumber & 0x7F];
    fRunning = true;
}

void Virtuosopreter::execute()
{
    while (!fErrors && fRunning)
    {
        Opcode opcode = (Opcode)fByteCodes[fPC++];
        // printf("%04X:%d\n", fPC, int(opcode));
        switch (opcode)
        {
            case Opcode::LITERAL:
            {
                push((fByteCodes[fPC+1] << 8) | fByteCodes[fPC]);
                fPC += 2;
                break;
            }
            case Opcode::LITERAL8:
            {
                push(fByteCodes[fPC++]);
                break;
            }
            case Opcode::LITERAL_N:
            {
                auto count = fByteCodes[fPC++] / 2;
                while (count > 0)
                {
                    push((fByteCodes[fPC+1] << 8) | fByteCodes[fPC]);
                    count--;
                    fPC += 2;
                }
                break;
            }
            case Opcode::LITERAL8_N:
            {
                auto count = fByteCodes[fPC++];
                while (count > 0)
                {
                    push(fByteCodes[fPC++]);
                    count--;
                }
                break;
            }
            case Opcode::RETURN:
            {
                fPC = popCallstack();
                break;
            }
            case Opcode::DELAY:
            {
                uint16_t timeMS = pop();
                delayMS(timeMS);
                break;
            }
            case Opcode::SERVO:
            {
                uint16_t servoNum = pop();
                uint16_t pulseValQuarterMS = pop();
                setTarget(servoNum, pulseValQuarterMS);
                break;
            }
            case Opcode::SERVOX:
            {
                uint16_t servoNum = pop();
                uint16_t targetPermill = pop();
                uint16_t targetDuration = pop();
                uint16_t targetEasing = pop();
                setTarget(servoNum, targetPermill, targetDuration, targetEasing);
                break;
            }
            case Opcode::QUIT:
            {
                goto done;
            }
            case Opcode::DROP:
            {
                pop();
                break;
            }
            case Opcode::DEPTH:
            {
                push(fSP);
                break;
            }
            case Opcode::DUP:
            {
                auto val = pop();
                push(val);
                push(val);
                break;
            }
            case Opcode::SWAP:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(rhs);
                push(lhs);
            }

            // Unary
            case Opcode::BITWISE_NOT:
            {
                auto val = pop();
                push(~val);
                break;
            }
            case Opcode::LOGICAL_NOT:
            {
                auto val = pop();
                push(!val);
                break;
            }
            case Opcode::NEGATE:
            {
                auto val = pop();
                push(-val);
                break;
            }
            case Opcode::POSITIVE:
            {
                auto val = pop();
                push(val > 0);
                break;
            }
            case Opcode::NEGATIVE:
            {
                auto val = pop();
                push(val < 0);
                break;
            }
            case Opcode::NONZERO:
            {
                auto val = pop();
                push(val != 0);
                break;
            }

            // Binary
            case Opcode::BITWISE_AND:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs & rhs);
                break;
            }
            case Opcode::BITWISE_OR:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs | rhs);
                break;
            }
            case Opcode::BITWISE_XOR:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs ^ rhs);
                break;
            }
            case Opcode::SHIFT_RIGHT:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs << rhs);
                break;
            }
            case Opcode::SHIFT_LEFT:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs >> rhs);
                break;
            }
            case Opcode::LOGICAL_AND:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs && rhs);
                break;
            }
            case Opcode::LOGICAL_OR:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs || rhs);
                break;
            }
            case Opcode::PLUS:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs + rhs);
                break;
            }
            case Opcode::MINUS:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs - rhs);
                break;
            }
            case Opcode::TIMES:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs * rhs);
                break;
            }
            case Opcode::DIVIDE:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs / rhs);
                break;
            }
            case Opcode::MOD:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs % rhs);
                break;
            }
            case Opcode::EQUALS:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs == rhs);
                break;
            }
            case Opcode::NOT_EQUALS:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs != rhs);
                break;
            }
            case Opcode::LESS_THAN:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs < rhs);
                break;
            }
            case Opcode::GREATER_THAN:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs > rhs);
                break;
            }
            case Opcode::MIN:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs < rhs ? lhs : rhs);
                break;
            }
            case Opcode::MAX:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs < rhs ? rhs : lhs);
                break;
            }
            case Opcode::LED_ON:
            {
                ledState(true);
                break;
            }
            case Opcode::LED_OFF:
            {
                ledState(false);
                break;
            }
            case Opcode::SERIAL_SEND_BYTE:
            {
                uint8_t byte = pop() & 0xFF;
                serialSendByte(byte);
                break;
            }
            case Opcode::SERIAL_SEND:
            {
                uint8_t byte;
                while ((byte = (pop() & 0xFF)) != 0)
                {
                    serialSendByte(byte);
                }
                break;
            }
            case Opcode::GET_POSITION:
            {
                auto channel = pop();
                push(getPosition(channel));
                break;
            }
            case Opcode::GET_MOVING_STATE:
            {
                auto channel = pop();
                push(getMovingState(channel));
                break;
            }
            case Opcode::ACCELERATION:
            {
                auto accelleration = pop();
                auto channel = pop();
                setAccelleration(channel, accelleration);
                break;
            }
            case Opcode::SPEED:
            {
                auto speed = pop();
                auto channel = pop();
                setSpeed(channel, speed);
                break;
            }
            case Opcode::JUMP:
            {
                fPC = (fByteCodes[fPC+1] << 8) | fByteCodes[fPC];
                break;
            }
            case Opcode::JUMP_Z:
            {
                fPC = !pop() ? (fByteCodes[fPC+1] << 8) | fByteCodes[fPC] : fPC + 2;
                break;
            }
            case Opcode::CALL:
            {
                uint16_t address = (fByteCodes[fPC+1] << 8) | fByteCodes[fPC];
                pushCallstack(fPC + 2);
                fPC = address;
            }
            case Opcode::GET_MS:
            case Opcode::OVER:
            case Opcode::PICK:
            case Opcode::ROT:
            case Opcode::ROLL:
            case Opcode::SERVO_8BIT:
            case Opcode::PWM:
            case Opcode::PEEK:
            case Opcode::POKE:
                printf("UNSUPPORTED OPCODE : %d\n", int(opcode));
                fErrors |= ERROR_PROGRAM_COUNTER_ERROR;
                break;
            default:
                if ((uint8_t(opcode) & 0x80) != 0)
                {
                    printf("CALL SUB: %d\n", uint8_t(opcode) & 0x7F);
                    // Call subroutine
                    pushCallstack(fPC);
                    fPC = fSubroutines[uint8_t(opcode) & 0x7F];
                }
                else
                {
                    printf("ERROR\n");
                    fErrors |= ERROR_PROGRAM_COUNTER_ERROR;
                }
                break;
        }
    }
done:
    return;
}

#endif

}  // namespace Virtuoso
