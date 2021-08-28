#include "assembler.hpp"

#include <iostream>
#include <fstream>

Assembler::Assembler() :
    lexer_(), parser_(lexer_, *this)
{}

int Assembler::run(const std::string& inFilename, const std::string& outFilename)
{
    std::ifstream inFile(inFilename);
    if (!inFile.is_open()) {
        std::cout << "Cannot open file: " << inFilename << std::endl;
        return AE_FILE;
    }

    lexer_.switch_streams(&inFile);

    int res = AE_OK;

    for (pass_ = 0; pass_ < 2 && res == AE_OK; ++pass_) {
        inFile.seekg(0);
        location_.initialize(&inFilename);

        instrNumArgs_ = 0;
        dirArgs_.clear();
        labeled_ = false;
        section_ = "";
        sectionData_ = nullptr;
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

    sections_.clear();
    symbols_.clear();
    relEntries_.clear();

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
    labeled_ = false;

    return res;

}
int Assembler::instrFirstPass(const std::string& instrName)
{
    auto instrIt = INSTRUCTIONS.find(instrName);
    if (instrIt == INSTRUCTIONS.end()) {
        syntaxError("unknown instruction: " + instrName);
        return AE_SYNTAX;
    }

    const InstrInfo& iInfo = instrIt->second;

    if (iInfo.numArgs != instrNumArgs_) {
        syntaxError("instruction takes " + std::to_string(iInfo.numArgs)
                    + " operands, but " + std::to_string(instrNumArgs_)+ " were provided");
        return AE_SYNTAX;
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
    const InstrInfo& iInfo = INSTRUCTIONS.find(instrName)->second;

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

    string_ushort_variant *payload = nullptr;

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
        addrMode = AM_REGDIR;
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
        int res = processWord(*payload);
        if (res != AE_OK)
            return res;
    }

    return AE_OK;
}
int Assembler::instrArgImmed(string_ushort_variant arg)
{
    instrArgs_[instrNumArgs_].jmpSyntax = false;
    instrArgs_[instrNumArgs_].addrMode = IMMED;
    instrArgs_[instrNumArgs_].val = arg;
    instrNumArgs_++;
    return AE_OK;
}
int Assembler::instrArgMemDirOrJmpImmed(string_ushort_variant arg, bool jmpSyntax)
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
    auto regIt = REGISTERS.find(reg);
    if (regIt == REGISTERS.end()) {
        syntaxError("invalid register: " + reg);
        return AE_SYNTAX;
    }

    instrArgs_[instrNumArgs_].jmpSyntax = jmpSyntax;
    instrArgs_[instrNumArgs_].addrMode = REGDIR;
    instrArgs_[instrNumArgs_].val = regIt->second;
    instrNumArgs_++;
    return AE_OK;
}
int Assembler::instrArgRegInd(const std::string& reg, bool jmpSyntax)
{
    auto regIt = REGISTERS.find(reg);
    if (regIt == REGISTERS.end()) {
        syntaxError("invalid register: " + reg);
        return AE_SYNTAX;
    }

    instrArgs_[instrNumArgs_].jmpSyntax = jmpSyntax;
    instrArgs_[instrNumArgs_].addrMode = REGIND;
    instrArgs_[instrNumArgs_].val = regIt->second;
    instrNumArgs_++;
    return AE_OK;
}
int Assembler::instrArgRegIndOff(const std::string& reg, string_ushort_variant off, bool jmpSyntax)
{
    auto regIt = REGISTERS.find(reg);
    if (regIt == REGISTERS.end()) {
        syntaxError("invalid register: " + reg);
        return AE_SYNTAX;
    }

    instrArgs_[instrNumArgs_].jmpSyntax = jmpSyntax;
    instrArgs_[instrNumArgs_].addrMode = REGIND_OFFSET;
    instrArgs_[instrNumArgs_].val = regIt->second;
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
    labeled_ = false;

    return res;
}
int Assembler::dirFirstPass(const std::string& dirName)
{
    auto dirIt = DIRECTIVES.find(dirName);
    if (dirIt == DIRECTIVES.end()) {
        syntaxError("unknown directive: " + dirName);
        return AE_SYNTAX;
    }

    const DirInfo& dInfo = dirIt->second;

    if (dInfo.sectionRequired && section_.empty()) {
        syntaxError("directive not in any section: " + dirName);
        return AE_SYNTAX;
    }

    if (!dInfo.labelsAllowed && labeled_) {
        syntaxError("directive doesn't support labels: " + dirName);
        return AE_SYNTAX;
    }

    // Check argument syntax
    switch(dInfo.argFormat) {
    case NONE:
        if (!dirArgs_.empty()) {
            syntaxError("expected directive syntax: ." + dirName);
            return AE_SYNTAX;
        }
        break;
    case SYM:
        if (dirArgs_.size() != 1 || !std::get_if<std::string>(&dirArgs_[0])) {
            syntaxError("expected directive syntax: ." + dirName + " <IDENT>");
            return AE_SYNTAX;
        }
        break;
    case LIT:
        if (dirArgs_.size() != 1 || !std::get_if<ushort>(&dirArgs_[0])) {
            syntaxError("expected directive syntax: ." + dirName + " <LITERAL>");
            return AE_SYNTAX;
        }
        break;
    case SYM_LIT:
        if (dirArgs_.size() != 2 || !std::get_if<std::string>(&dirArgs_[0]) || !std::get_if<ushort>(&dirArgs_[1])) {
            syntaxError("expected directive syntax: ." + dirName + " <IDENT>, <LITERAL>");
            return AE_SYNTAX;
        }
        break;
    case SYM_LIST:
        if (dirArgs_.empty()) {
            syntaxError("expected directive syntax: ." + dirName + " <IDENT list>");
            return AE_SYNTAX;
        }
        for (uint i = 0; i < dirArgs_.size(); ++i) {
            if (!std::get_if<std::string>(&dirArgs_[i])) {
                syntaxError("unexpected " + std::to_string(std::get<ushort>(dirArgs_[i])) + ", expected directive syntax: ." + dirName + " <IDENT list>");
                return AE_SYNTAX;
            }
        }
        break;
    case SYM_LIT_LIST:
        if (dirArgs_.empty()) {
            syntaxError("expected directive syntax: ." + dirName + " <IDENT/LITERAL list>");
            return AE_SYNTAX;
        }
        break;
    }

    // Directives
    switch (dInfo.dir) {
    case GLOBAL:
        for (uint i = 0; i < dirArgs_.size(); ++i) {
            const std::string &symbolName = std::get<std::string>(dirArgs_[i]);
            Symbol &symbol = symbols_[symbolName];
            if (symbol.external) {
                error("symbol already declared as extern: " + symbolName);
                return AE_SYMBOL;
            } else // symbol declaration
                symbol.global = true;
        }
        break;

    case EXTERN:
        for (uint i = 0; i < dirArgs_.size(); ++i) {
            const std::string &symbolName = std::get<std::string>(dirArgs_[i]);
            Symbol &symbol = symbols_[symbolName];
            if (symbol.defined) {
                error("symbol already defined: " + symbolName);
                return AE_SYMBOL;
            } else if (symbol.global) {
                error("symbol already declared as global: " + symbolName);
                return AE_SYMBOL;
            } else // symbol declaration
                symbol.external = true;
        }
        break;

    case SECTION:
        // Reserve space for previous section
        if (!section_.empty())
            sectionData_->reserve(lc_);

        section_ = std::get<std::string>(dirArgs_[0]);
        sectionData_ = &sections_[section_].data;
        lc_ = 0;
        break;

    case WORD:
        lc_ += dirArgs_.size() * 2;
        break;

    case SKIP:
        lc_ += std::get<ushort>(dirArgs_[0]);
        break;

    case EQU: {
        const std::string &symbolName = std::get<std::string>(dirArgs_[0]);
        ushort literal = std::get<ushort>(dirArgs_[1]);
        Symbol &symbol = symbols_[symbolName];
        if (symbol.defined) {
            error("symbol already defined: " + symbolName);
            return AE_SYMBOL;
        } else if (symbol.external) {
            error("symbol already declared as extern: " + symbolName);
            return AE_SYMBOL;
        } else { // symbol definition
            symbol.defined = true;
            symbol.value = literal;
        }
        break;
    }

    case END:
        return AE_END;
    }

    return AE_OK;
}
int Assembler::dirSecondPass(const std::string& dirName)
{
    const DirInfo& dInfo = DIRECTIVES.find(dirName)->second;

    // Directives
    switch (dInfo.dir) {
    case EXTERN:
    case EQU:
        break;

    case GLOBAL:
        for (uint i = 0; i < dirArgs_.size(); ++i) {
            std::string symbolName = std::get<std::string>(dirArgs_[i]);
            Symbol &symbol = symbols_[symbolName];
            if (!symbol.defined) // undefined exported symbol
                warning("undefined global symbol: " + symbolName);
        }
        break;
        
    case SECTION:
        section_ = std::get<std::string>(dirArgs_[0]);
        sectionData_ = &sections_[section_].data;
        break;

    case WORD:
        for (uint i = 0; i < dirArgs_.size(); ++i) {
            int res = processWord(dirArgs_[i]);
            if (res != AE_OK)
                return res;
        }
        break;

    case SKIP:
        sectionData_->resize(sectionData_->size() + std::get<ushort>(dirArgs_[0]));
        break;

    case END:
        return AE_END;
    }

    return AE_OK;
}
int Assembler::dirArg(string_ushort_variant arg)
{
    dirArgs_.push_back(arg);
    return AE_OK;
}

int Assembler::label(const std::string& label)
{
    if (pass_ == 0) {
        if (section_.empty()) {
            error("label not in any section: " + label);
            return AE_SYMBOL;
        }
        Symbol &symbol = symbols_[label];
        if (symbol.defined) {
            error("symbol already defined: " + label);
            return AE_SYMBOL;
        } else if (symbol.external) {
            error("symbol already declared as extern: " + label);
            return AE_SYMBOL;
        } else { // symbol definition
            symbol.defined = true;
            symbol.label = true;
            symbol.value = (ushort)lc_;
            symbol.section = section_;
            labeled_ = true;
        }
    }

    return AE_OK;
}

int Assembler::writeToFile(const std::string& outFilename)
{
    std::ofstream outFile(outFilename, std::ios_base::binary);
    if (!outFile.is_open()) {
        std::cout << "Cannot open file for writing: " << outFilename << std::endl;
        return AE_FILE;
    }

    // TODO:

    outFile.close();

    return AE_OK;
}

int Assembler::processWord(string_ushort_variant &arg)
{
    ubyte dataHigh, dataLow;
    ushort *literal = std::get_if<ushort>(&arg);

    if (literal) {
        dataHigh = *literal >> 8;
        dataLow = *literal;
    } else {
        const std::string &symbolName = std::get<std::string>(arg);
        const Symbol &symbol = symbols_[symbolName];
        if (!symbol.defined && !symbol.external) {
            error("undeclared symbol " + symbolName);
            return AE_SYMBOL;
        }

        dataHigh = symbol.value >> 8;
        dataLow = symbol.value;

        // Relocation entries for labels and external symbols
        if (symbol.label || symbol.external)
            relEntries_.emplace_back(&symbol, section_, sectionData_->size());
    }

    sectionData_->push_back(dataHigh);
    sectionData_->push_back(dataLow);

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
void Assembler::warning(const std::string& msg)
{
    parser_.error(getLocation(), "warning, " + msg);
}