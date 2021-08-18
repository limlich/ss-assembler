#include "assembler.hpp"

#include <iostream>
#include <fstream>

Assembler::Assembler() :
    lexer_(), parser_(lexer_, *this)
{}

int Assembler::run(const std::string& inFilename, const std::string& outFilename)
{
    std::ifstream inFile(inFilename, std::ios_base::in);
    if (!inFile.is_open()) {
        std::cout << "Cannot open file: " << inFilename << std::endl;
        return AE_FILE;
    }

    sections_.clear();
    symbols_.clear();

    lexer_.switch_streams(&inFile);

    int res = AE_OK;

    for (pass_ = 0; pass_ < 2 && res == AE_OK; ++pass_) {
        inFile.seekg(0);
        location_.initialize(&inFilename);

        instrNumArgs_ = 0;
        dirArgs_.clear();
        labels_.clear();
        section_ = "";
        lc_ = 0;
        
        res = parser_.parse();
        if (res == AE_END)
            res = AE_OK;
    }

    location_.initialize();
    lexer_.switch_streams();
    inFile.close();

    if (res == AE_OK)
        res = writeToFile(outFilename);

    return res;
}

void Assembler::locationAddColumns(yy::location::counter_type count)
{
    location_.step();
    location_.columns(count);
}
void Assembler::locationAddLines(yy::location::counter_type count)
{
    location_.lines(count);
}

int Assembler::instr(std::string instrName)
{
    if (section_.empty()) {
        error("instruction not in any section");
        return AE_SECTION;
    }

    regIndUpdate_ = REGIND_NONE;

    // push reg -> str reg, [sp]
    // pop reg  -> ldr reg, [sp]
    if (instrNumArgs_ == 1 && instrArgs_[0].addrMode == REGDIR && !instrArgs_[0].jmpSyntax) {
        if (instrName == "push") {
            instrName = "str";
            instrNumArgs_ = 2;
            instrArgs_[1].jmpSyntax = false;
            instrArgs_[1].addrMode = REGIND;
            instrArgs_[1].val = SP_REGISTER;
            regIndUpdate_ = REGIND_PRE_DEC;
        } else if (instrName == "pop") {
            instrName = "ldr";
            instrNumArgs_ = 2;
            instrArgs_[1].jmpSyntax = false;
            instrArgs_[1].addrMode = REGIND;
            instrArgs_[1].val = SP_REGISTER;
            regIndUpdate_ = REGIND_POST_INC;
        }
    }

    int res;
    if (pass_ == 0)
        res = instrFirstPass(instrName);
    else
        res = instrSecondPass(instrName);
    
    instrNumArgs_ = 0;
    labels_.clear();

    return res;

}
int Assembler::instrFirstPass(const std::string& instrName)
{
    auto it = INSTRUCTIONS.find(instrName);
    if (it == INSTRUCTIONS.end()) {
        syntaxError("invalid instruction name: " + instrName);
        return AE_SYNTAX;
    }

    const InstrInfo& iInfo = (*it).second;

    if (iInfo.numArgs != instrNumArgs_) {
        syntaxError("instruction takes " + std::to_string(iInfo.numArgs)
                    + " operands, but " + std::to_string(instrNumArgs_)+ " were provided");
        return AE_SYNTAX;
    }

    // add labels to symbol table
    for (uint i = 0; i < labels_.size(); ++i) {
        if (symbols_.find(labels_[i]) != symbols_.end()) {
            error("symbol already defined: " + labels_[i]);
            return AE_SYMBOL;
        }
        symbols_[labels_[i]] = Symbol(section_, (ushort)lc_, false, false, true);
    }

    lc_++; // InstrDescr

    if (iInfo.numArgs == 0) // instr
        return AE_OK;

    lc_++; // RegDescr

    if (!(iInfo.argAddrModes[0] == REGDIR && iInfo.argAddrModes[1] == 0) // instr reg
        && !(iInfo.argAddrModes[0] == REGDIR && iInfo.argAddrModes[1] == REGDIR)) // instr reg, reg
        lc_++; // AddrMode

    for (ubyte i = 0; i < iInfo.numArgs; ++i) {
        InstrArg& arg = instrArgs_[i];

        if (arg.addrMode == (MEMDIR | IMMED)) { // ambiguous syntax
            if (iInfo.jmpSyntax)
                arg.addrMode = IMMED; // jmp <lit/sym>
            else
                arg.addrMode = MEMDIR; // instr <lit/sym>
            arg.jmpSyntax = iInfo.jmpSyntax;
        } else if (arg.addrMode == REGDIR_OFFSET) // instr %<symbol> | jmp %<symbol>
            arg.jmpSyntax = iInfo.jmpSyntax;

        if (iInfo.jmpSyntax != arg.jmpSyntax) {
            syntaxError(std::string("expected ") + (iInfo.jmpSyntax ? "jump" : "data")
                        + " operand syntax");
            return AE_SYNTAX;
        }

        if (!(iInfo.argAddrModes[i] & arg.addrMode)) {
            syntaxError(std::string("invalid addressing mode for ")
                        + (i == 0 ? "first" : "second") + " operand");
            return AE_SYNTAX;
        }

        switch(arg.addrMode) {
        case IMMED:
        case MEMDIR:
        case REGDIR_OFFSET:
        case REGIND_OFFSET:
            lc_ += 2; // DataHigh + DataLow
            break;
        }
    }

    return AE_OK;
}
int Assembler::instrSecondPass(const std::string& instrName)
{
    auto it = INSTRUCTIONS.find(instrName);
    const InstrInfo& iInfo = (*it).second;

    sectionData_->push_back(iInfo.opCode); // InstrDescr
    if (iInfo.numArgs == 0) // instr
        return AE_OK;

    if (iInfo.argAddrModes[0] == REGDIR && iInfo.argAddrModes[1] == 0) { // instr reg
        ubyte regD = *std::get_if<ushort>(&instrArgs_[0].val);
        sectionData_->push_back(regD << 4 | 0xF); // RegDescr
        return AE_OK;
    }

    if (iInfo.argAddrModes[0] == REGDIR && iInfo.argAddrModes[1] == REGDIR) { // instr reg, reg
        ubyte regD = *std::get_if<ushort>(&instrArgs_[0].val);
        ubyte regS = *std::get_if<ushort>(&instrArgs_[1].val);
        sectionData_->push_back(regD << 4 | regS); // RegDescr
        return AE_OK;
    }

    // instr op | instr regD, op

    InstrArg *op = &instrArgs_[0];
    ubyte regD = 0xFu, regS = 0xFu;
    ubyte addrMode;
    ubyte dataHigh = 0, dataLow = 0;

    if (iInfo.argAddrModes[0] == REGDIR) { // instr regD, op
        regD = *std::get_if<ushort>(&instrArgs_[0].val);
        op = &instrArgs_[1];
    }

    if (op->addrMode == (MEMDIR | IMMED)) { // ambiguous syntax
        if (iInfo.jmpSyntax)
            op->addrMode = IMMED; // jmp <lit/sym>
        else
            op->addrMode = MEMDIR; // instr <lit/sym>
    }

    instr_arg_type *payload = nullptr;

    switch(op->addrMode) {
    case IMMED:
        addrMode = AM_IMMED;
        payload = &op->val;
        break;
    case MEMDIR:
        addrMode = AM_MEMDIR;
        payload = &op->val;
        break;
    case REGDIR:
        regS = *std::get_if<ushort>(&op->val);
        addrMode = AM_MEMDIR;
        break;
    case REGDIR_OFFSET:
        regS = *std::get_if<ushort>(&op->val);
        addrMode = AM_REGDIR_OFFSET;
        payload = &op->off;
        break;
    case REGIND:
        regS = *std::get_if<ushort>(&op->val);
        addrMode = AM_REGIND;
        break;
    case REGIND_OFFSET:
        regS = *std::get_if<ushort>(&op->val);
        addrMode = AM_REGIND_OFFSET;
        payload = &op->off;
        break;
    }

    sectionData_->push_back(regD << 4 | regS); // RegDescr
    sectionData_->push_back(regIndUpdate_ << 4 | addrMode); // AddrMode

    // dataHigh + dataLow
    if (payload) {
        ushort *literal = std::get_if<ushort>(payload);
        if (literal) {
            dataHigh = *literal >> 8;
            dataLow = *literal;
        } else {
            std::string *symbol = std::get_if<std::string>(payload);
            auto it = symbols_.find(*symbol);
            if (it == symbols_.end()) {
                error("undefined symbol " + *symbol);
                return AE_SYMBOL;
            }
            Symbol &sym = (*it).second;
            dataHigh = sym.value >> 8;
            dataLow = sym.value;
            if (sym.rel || sym.imp)
                sym.relEntries.emplace_back(section_, sectionData_->size());
        }
        sectionData_->push_back(dataHigh);
        sectionData_->push_back(dataLow);
    }

    return AE_OK;
}
int Assembler::instrArgImmed(instr_arg_type arg)
{
    instrArgs_[instrNumArgs_].jmpSyntax = false;
    instrArgs_[instrNumArgs_].addrMode = IMMED;
    instrArgs_[instrNumArgs_].val = arg;
    instrNumArgs_++;
    return AE_OK;
}
int Assembler::instrArgMemDirOrJmpImmed(instr_arg_type arg, bool jmpSyntax)
{
    instrArgs_[instrNumArgs_].jmpSyntax = jmpSyntax;
    instrArgs_[instrNumArgs_].addrMode = MEMDIR;
    if (!jmpSyntax) // <literal> | <symbol> (ambiguous syntax, instr info determines addr mode)
        instrArgs_[instrNumArgs_].addrMode |= IMMED;
    instrArgs_[instrNumArgs_].val = arg;
    instrNumArgs_++;
    return AE_OK;
}
int Assembler::instrArgPCRel(const std::string& sym)
{
    instrArgs_[instrNumArgs_].addrMode = REGDIR_OFFSET;
    instrArgs_[instrNumArgs_].val = PC_REGISTER;
    instrArgs_[instrNumArgs_].off = sym;
    instrNumArgs_++;
    return AE_OK;
}
int Assembler::instrArgRegDir(const std::string& reg, bool jmpSyntax)
{
    auto it = REGISTERS.find(reg);
    if (it == REGISTERS.end()) {
        syntaxError("invalid register: " + reg);
        return AE_SYNTAX;
    }

    instrArgs_[instrNumArgs_].jmpSyntax = jmpSyntax;
    instrArgs_[instrNumArgs_].addrMode = REGDIR;
    instrArgs_[instrNumArgs_].val = (*it).second;
    instrNumArgs_++;
    return AE_OK;
}
int Assembler::instrArgRegInd(const std::string& reg, bool jmpSyntax)
{
    auto it = REGISTERS.find(reg);
    if (it == REGISTERS.end()) {
        syntaxError("invalid register: " + reg);
        return AE_SYNTAX;
    }

    instrArgs_[instrNumArgs_].jmpSyntax = jmpSyntax;
    instrArgs_[instrNumArgs_].addrMode = REGIND;
    instrArgs_[instrNumArgs_].val = (*it).second;
    instrNumArgs_++;
    return AE_OK;
}
int Assembler::instrArgRegIndOff(const std::string& reg, instr_arg_type off, bool jmpSyntax)
{
    auto it = REGISTERS.find(reg);
    if (it == REGISTERS.end()) {
        syntaxError("invalid register: " + reg);
        return AE_SYNTAX;
    }

    instrArgs_[instrNumArgs_].jmpSyntax = jmpSyntax;
    instrArgs_[instrNumArgs_].addrMode = REGIND_OFFSET;
    instrArgs_[instrNumArgs_].val = (*it).second;
    instrArgs_[instrNumArgs_].off = off;
    instrNumArgs_++;
    return AE_OK;
}

int Assembler::dir(const std::string& dirName)
{
    int res;
    if (pass_ == 0)
        res = dirFirstPass(dirName);
    else
        res = dirSecondPass(dirName);

    dirArgs_.clear();
    labels_.clear();

    return res;
}
int Assembler::dirFirstPass(const std::string& dirName)
{
    // TODO:

    return AE_OK;
}
int Assembler::dirSecondPass(const std::string& dirName)
{
    // TODO:

    return AE_OK;
}
int Assembler::dirArg(dir_arg_type arg)
{
    dirArgs_.push_back(arg);
    return AE_OK;
}

int Assembler::label(const std::string& label)
{
    if (pass_ == 0)
        labels_.push_back(label);
    return AE_OK;
}

int Assembler::writeToFile(const std::string& outFilename)
{
    std::ifstream outFile(outFilename, std::ios_base::out);
    if (!outFile.is_open()) {
        std::cout << "Cannot open file for writing: " << outFilename << std::endl;
        return AE_FILE;
    }

    // TODO:

    outFile.close();

    return AE_OK;
}

void Assembler::syntaxError(const std::string& msg)
{
    parser_.error(getLocation(), "syntax error, " + msg);
}
void Assembler::error(const std::string& msg)
{
    parser_.error(getLocation(), "error, " + msg);
}