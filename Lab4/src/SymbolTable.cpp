#include "SymbolTable.h"
#include "Type.h"
#include <iostream>
#include <sstream>

SymbolEntry::SymbolEntry(Type *type, int kind) 
{
    this->type = type;
    this->kind = kind;
}

ConstantSymbolEntry::ConstantSymbolEntry(Type *type, float value) : SymbolEntry(type, SymbolEntry::CONSTANT)
{
    this->value = value;
}

std::string ConstantSymbolEntry::toStr()
{
    std::ostringstream buffer;
    buffer << value;
    return buffer.str();
}

IdentifierSymbolEntry::IdentifierSymbolEntry(Type *type, std::string name, int scope) : SymbolEntry(type, SymbolEntry::VARIABLE), name(name)
{
    this->scope = scope;
}

std::string IdentifierSymbolEntry::toStr()
{
    return name;
}

TemporarySymbolEntry::TemporarySymbolEntry(Type *type, int label) : SymbolEntry(type, SymbolEntry::TEMPORARY)
{
    this->label = label;
}

std::string TemporarySymbolEntry::toStr()
{
    std::ostringstream buffer;
    buffer << "t" << label;
    return buffer.str();
}

SymbolTable::SymbolTable()
{
    prev = nullptr;
    level = 0;
    Type* func_getint = new FunctionType(TypeSystem::intType, {});
    SymbolEntry *se_getint=new IdentifierSymbolEntry(func_getint,"getint",0);
    this->install("getint",se_getint);

    Type* func_getch = new FunctionType(TypeSystem::intType, {});
    SymbolEntry *se_getch=new IdentifierSymbolEntry(func_getch,"getch",0);
    this->install("getch",se_getch);

    Type* func_getfloat = new FunctionType(TypeSystem::floatType, {});
    SymbolEntry *se_getfloat=new IdentifierSymbolEntry(func_getfloat,"getfloat",0);
    this->install("getfloat",se_getfloat); 

    Type* func_getarray = new FunctionType(TypeSystem::intType, {TypeSystem::intArrayType});
    SymbolEntry *se_getarray=new IdentifierSymbolEntry(func_getarray,"getarray",0);
    this->install("getarray",se_getarray);   

    Type* func_getfarray = new FunctionType(TypeSystem::intType, {TypeSystem::floatArrayType});
    SymbolEntry *se_getfarray=new IdentifierSymbolEntry(func_getfarray,"getfarray",0);
    this->install("getfarray",se_getfarray);      

    Type* func_putint = new FunctionType(TypeSystem::voidType, {TypeSystem::intType});
    SymbolEntry *se_putint=new IdentifierSymbolEntry(func_putint,"putint",0);
    this->install("putint",se_putint);  

    Type* func_putch = new FunctionType(TypeSystem::voidType, {TypeSystem::intType});
    SymbolEntry *se_putch=new IdentifierSymbolEntry(func_putch,"putch",0);
    this->install("putch",se_putch);  

    Type* func_putarray = new FunctionType(TypeSystem::voidType, {TypeSystem::intType,TypeSystem::intArrayType});
    SymbolEntry *se_putarray=new IdentifierSymbolEntry(func_putarray,"putarray",0);
    this->install("putarray",se_putarray);  

    Type* func_putfloat = new FunctionType(TypeSystem::voidType, {TypeSystem::floatType});
    SymbolEntry *se_putfloat=new IdentifierSymbolEntry(func_putfloat,"putfloat",0);
    this->install("putfloat",se_putfloat); 

    Type* func_putfarray = new FunctionType(TypeSystem::voidType, {TypeSystem::intType,TypeSystem::floatArrayType});
    SymbolEntry *se_putfarray=new IdentifierSymbolEntry(func_putfarray,"putfarray",0);
    this->install("putfarray",se_putfarray);  

}

SymbolTable::SymbolTable(SymbolTable *prev)
{
    this->prev = prev;
    this->level = prev->level + 1;
}

/*
    Description: lookup the symbol entry of an identifier in the symbol table
    Parameters: 
        name: identifier name
    Return: pointer to the symbol entry of the identifier

    hint:
    1. The symbol table is a stack. The top of the stack contains symbol entries in the current scope.
    2. Search the entry in the current symbol table at first.
    3. If it's not in the current table, search it in previous ones(along the 'prev' link).
    4. If you find the entry, return it.
    5. If you can't find it in all symbol tables, return nullptr.
*/
SymbolEntry* SymbolTable::lookup(std::string name)
{
    // Todo
    std::map<std::string, SymbolEntry*>::iterator it=symbolTable.find(name);
    if(it!=symbolTable.end())
    {
        return it->second;
    } 
    else if(prev!=nullptr)
    {
        return prev->lookup(name);
    }
    else
    {
        return nullptr;
    }

}

// install the entry into current symbol table.
void SymbolTable::install(std::string name, SymbolEntry* entry)
{
    symbolTable[name] = entry;
}

int SymbolTable::counter = 0;
static SymbolTable t;
SymbolTable *identifiers = &t;
SymbolTable *globals = &t;
