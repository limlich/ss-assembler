#include "assembler.hpp"

#include <iostream>
#include <cstdio>

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
    outFile_.open(outFilename, std::ios_base::binary);
    if (!outFile_.is_open()) {
        inFile.close();
        std::cout << "Cannot open file for writing: " << outFilename << std::endl;
        return AE_FILE;
    }

    lexer_.switch_streams(&inFile);

    int res = AE_OK;

    writeObjHeader();
    initSectionHeaderTable();
    initSymbolTable();
    initStrSection();

    for (pass_ = 0; pass_ < 2; ++pass_) {
        inFile.seekg(0);
        location_.initialize(&inFilename);

        instrNumArgs_ = 0;
        dirArgs_.clear();
        labeled_ = false;
        endDir_ = false;
        sectionName_ = "";
        relSectionName_ = "";
        section_ = nullptr;
        relSection_ = nullptr;
        lc_ = 0;
        
        res = parser_.parse();

        if (res != AE_END) {
            if (res != AE_OK)
                break;
            dir("end"); // implicit .end
        } else
            res = AE_OK;
    }

    location_.initialize();
    lexer_.switch_streams();
    inFile.close();
    outFile_.close();
    if (res != AE_OK)
        std::remove(outFilename.c_str());

    sections_.clear();
    sectionHeaderTable_.clear();
    symbols_.clear();

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
    if (sectionName_.empty()) {
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
    pcRel_ = false;

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

    section_->data.push_back(iInfo.opCode); // InstrDescr
    if (iInfo.numArgs == 0) // instr
        return AE_OK;

    if (iInfo.argAddrModes[0] == REGDIR && iInfo.argAddrModes[1] == 0) { // instr reg
        ubyte regD = *std::get_if<ushort>(&instrArgs_[0].val);
        section_->data.push_back(regD << 4 | 0xF); // RegDescr
        return AE_OK;
    }

    if (iInfo.argAddrModes[0] == REGDIR && iInfo.argAddrModes[1] == REGDIR) { // instr reg, reg
        ubyte regD = *std::get_if<ushort>(&instrArgs_[0].val);
        ubyte regS = *std::get_if<ushort>(&instrArgs_[1].val);
        section_->data.push_back(regD << 4 | regS); // RegDescr
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

    section_->data.push_back(regD << 4 | regS); // RegDescr
    section_->data.push_back(regIndUpdate_ << 4 | addrMode); // AddrMode

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
    pcRel_ = true;
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

    if (dInfo.sectionRequired && sectionName_.empty()) {
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
            Symbol &symbol = getSymbol(symbolName);
            symbol.global = true;
        }
        break;

    case EXTERN:
        for (uint i = 0; i < dirArgs_.size(); ++i) {
            const std::string &symbolName = std::get<std::string>(dirArgs_[i]);
            Symbol &symbol = getSymbol(symbolName);
            symbol.external = true;
        }
        break;

    case SECTION: {
        endSection(); // end previous section

        sectionName_ = SECTION_PREFIX + std::get<std::string>(dirArgs_[0]);
        relSectionName_ = sectionName_ + REL_SUFFIX;

        auto sit = sections_.find(sectionName_);
        if (sit != sections_.end()) {
            error("section with the same name already declared in this file: " + sectionName_);
            return AE_SECTION;
        }

        initRelSection();

        section_ = &sections_[sectionName_];
        relSection_ = &sections_[relSectionName_];

        section_->entry.type = ST_DATA;

        break;
    }

    case WORD:
        lc_ += dirArgs_.size() * 2;
        break;

    case SKIP:
        lc_ += std::get<ushort>(dirArgs_[0]);
        break;

    case EQU: {
        const std::string &symbolName = std::get<std::string>(dirArgs_[0]);
        ushort literal = std::get<ushort>(dirArgs_[1]);
        Symbol &symbol = getSymbol(symbolName);
        if (symbol.defined()) {
            error("symbol already defined: " + symbolName);
            return AE_SYMBOL;
        } else { // symbol definition
            symbol.external = false;
            symbol.entry.type = SYMT_ABS;
            symbol.entry.value = literal;
        }
        break;
    }

    case END:
        endSection();
        endDir_ = true;
        return AE_END;
    }

    return AE_OK;
}
int Assembler::dirSecondPass(const std::string& dirName)
{
    const DirInfo& dInfo = DIRECTIVES.find(dirName)->second;

    // Directives
    switch (dInfo.dir) {
    case GLOBAL:
    case EXTERN:
    case EQU:
        break;
        
    case SECTION:
        endSection();

        sectionName_ = SECTION_PREFIX + std::get<std::string>(dirArgs_[0]);
        relSectionName_ = sectionName_ + REL_SUFFIX;
        section_ = &sections_[sectionName_];
        relSection_ = &sections_[relSectionName_];
        break;

    case WORD:
        for (uint i = 0; i < dirArgs_.size(); ++i) {
            int res = processWord(dirArgs_[i]);
            if (res != AE_OK)
                return res;
        }
        break;

    case SKIP:
        section_->data.resize(section_->data.size() + std::get<ushort>(dirArgs_[0]));
        break;

    case END:
        endSection();
        endSymbolTable();
        endStrSection();
        endSectionHeaderTable();
        writeObjHeader();
        endDir_ = true;
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
        if (sectionName_.empty()) {
            error("label not in any section: " + label);
            return AE_SYMBOL;
        }
        Symbol &symbol = getSymbol(label);
        if (symbol.defined()) {
            error("symbol already defined: " + label);
            return AE_SYMBOL;
        } else { // symbol definition
            symbol.external = false;
            symbol.section = sectionName_;
            symbol.entry.type = SYMT_LABEL;
            symbol.entry.value = (ushort)lc_;
            labeled_ = true;
        }
    } else
        getSymbol(label).entry.sectionEntryId = section_->id;

    return AE_OK;
}

int Assembler::processWord(string_ushort_variant &arg)
{
    ushort value;
    ushort *literal = std::get_if<ushort>(&arg);

    // Relocation entry for labels, external symbols or PC relative addressing
    RelEntry relEntry(pcRel_ ? RT_PC : RT_SYM_16, section_->data.size(), 0);
    bool rel = pcRel_;

    if (literal)
        value = *literal;
    else {
        const std::string &symbolName = std::get<std::string>(arg);
        const Symbol &symbol = getSymbol(symbolName);
        if (!symbol.defined() && !symbol.external) {
            error("undeclared symbol " + symbolName);
            return AE_SYMBOL;
        }

        value = symbol.entry.value;

        if (symbol.label()) {
            relEntry.symbolId = getSectionSymbol(symbol.section).id;
            rel = true;
        } else if (symbol.external) {
            relEntry.symbolId = symbol.id;
            rel = true;
        }
    }

    if (rel) {
        auto const relBegin = (const ubyte*)&relEntry;
        auto const relEnd = relBegin + sizeof(RelEntry);
        relSection_->data.insert(relSection_->data.end(), relBegin, relEnd);
    }

    section_->data.push_back(value);
    section_->data.push_back(value >> 8);

    return AE_OK;
}

Symbol& Assembler::getSymbol(const std::string &symbolName)
{
    auto sit = symbols_.find(symbolName);
    if (sit != symbols_.end())
        return sit->second;

    // new symbol
    Symbol &symbol = symbols_[symbolName];
    symbol.section = sectionName_;

    return symbol;
}

const Symbol& Assembler::getSectionSymbol(const std::string &sectionName)
{
    Symbol &sectionSymbol = getSymbol(sectionName);
    if (!sectionSymbol.section.empty()) {
        // Add section symbol to the symbol table so it has an id
        // for relocation entries
        const Section &section = sections_[sectionName];
        sectionSymbol.section = sectionName;
        sectionSymbol.entry.bind = SYMB_LOCAL;
        sectionSymbol.entry.type = SYMT_SECTION;
        sectionSymbol.entry.nameOffset = section.entry.nameOffset;
        sectionSymbol.entry.sectionEntryId = section.id;
        insertSymbolTableEntry(sectionSymbol);
    }
    return sectionSymbol;
}

void Assembler::writeObjHeader()
{
    outFile_.seekp(0);

    outFile_.write(
        (const char*)&objHeader_,
        sizeof(ObjHeader)
    );
}

void Assembler::initSymbolTable()
{
    // Insert symbol table section so access to it doesn't cause reallocation
    // of section map and invalidate section pointers
    sections_[SYM_TAB_SECTION] = Section(ST_SYM_TAB);
    Symbol invalidSymbol;
    insertSymbolTableEntry(invalidSymbol);
}
   
void Assembler::insertSymbolTableEntry(Symbol &symbol)
{
    Section &symTabSection = sections_[SYM_TAB_SECTION];

    symbol.id = symTabSection.data.size() / sizeof(SymbolEntry);

    auto const entryBegin = (const ubyte*)&symbol.entry;
    auto const entryEnd = entryBegin + sizeof(SymbolEntry);
    symTabSection.data.insert(symTabSection.data.end(), entryBegin, entryEnd);
}

void Assembler::endSymbolTable()
{
    Section &symTabSection = sections_[SYM_TAB_SECTION];
    symTabSection.data.reserve(symbols_.size());

    for (auto& [symbolName, symbol] : symbols_) {
        if (symbol.entry.type == SYMT_SECTION) 
            continue;

        switch (symbol.entry.type) {
        case SYMT_UNDEF: // extern symbol
            if (symbol.external) {
                symbol.entry.bind = SYMB_GLOBAL;
                symbol.global = false;
            } else
                continue; // undefined exported symbol (.global) - ignore
            break;
        case SYMT_ABS:
        case SYMT_LABEL:
            if (symbol.global)
                symbol.entry.bind = SYMB_GLOBAL;
            else
                continue; // local absolute symbols and local labels are not needed in the table
            break;
        case SYMT_SECTION: // already inserted on first rel entry or ignored
            continue;
        }

        symbol.entry.nameOffset = insertStrSectionEntry(symbolName);
        insertSymbolTableEntry(symbol);
    }

    insertSectionTableEntry(SYM_TAB_SECTION, symTabSection);
    writeSection(symTabSection);
}

void Assembler::initRelSection()
{
    // Insert relocation section so access to it doesn't cause reallocation
    // of section map and invalidate section pointers
    sections_[relSectionName_] = Section(ST_REL);
}

void Assembler::initStrSection()
{
    // Insert names section so access to it doesn't cause reallocation
    // of section map and invalidate section pointers
    sections_[STR_SECTION] = Section(ST_STR);
}

std::size_t Assembler::insertStrSectionEntry(const std::string &str)
{
    Section &strSection = sections_[STR_SECTION];
    std::size_t pos = strSection.data.size();

    strSection.data.insert(strSection.data.end(), str.cbegin(), str.cend());
    strSection.data.emplace_back('\0');

    return pos;
}

void Assembler::endStrSection()
{
    Section &strSection = sections_[STR_SECTION];
    insertSectionTableEntry(STR_SECTION, strSection);
    writeSection(strSection);
    objHeader_.strEntryId = strSection.id;
}

void Assembler::initSectionHeaderTable()
{
    // Invalid section
    sectionHeaderTable_.emplace_back(ST_NONE);
}

void Assembler::endSectionHeaderTable()
{
    objHeader_.shtOffset = outFile_.tellp();
    objHeader_.shtSize = sectionHeaderTable_.size();

    outFile_.write(
        (const char*)sectionHeaderTable_.data(),
        sectionHeaderTable_.size() * sizeof(SectionEntry)
    );
}

void Assembler::endSection()
{
    if (sectionName_.empty())
        return;

    if (lc_ == 0 && section_->entry.size == 0)
        return;

    if (pass_ == 0) {
        insertSectionTableEntry(sectionName_, *section_, lc_);
        section_->data.reserve(lc_);
        lc_ = 0;
    } else {
        writeSection(*section_);
        if (!relSection_->data.empty()) {
            insertSectionTableEntry(relSectionName_, *relSection_);
            writeSection(*relSection_);
        }
    }
}

void Assembler::insertSectionTableEntry(const std::string &sectionName, Section &section, ushort size)
{
    section.id = sectionHeaderTable_.size();
    section.entry.size = size ? size : section.data.size();
    section.entry.nameOffset = insertStrSectionEntry(sectionName);
    sectionHeaderTable_.push_back(section.entry);
}

void Assembler::writeSection(Section &section)
{
    sectionHeaderTable_[section.id].dataOffset = section.entry.dataOffset = outFile_.tellp();
    outFile_.write(
        (const char *)section.data.data(),
        section.entry.size
    );

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