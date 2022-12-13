#include "Unit.h"

void Unit::insertFunc(Function *f)
{
    func_list.push_back(f);
}

void Unit::insertId(SymbolEntry *id)
{
    for(auto i=id_list.begin();i!=id_list.end();++i)
    {
        if(((IdentifierSymbolEntry*)id)->getName()==((IdentifierSymbolEntry*)(*i))->getName())
            return;
    }
    id_list.push_back(id);
}

void Unit::removeFunc(Function *func)
{
    func_list.erase(std::find(func_list.begin(), func_list.end(), func));
}

void Unit::output() const
{
    for(auto &id : id_list)
        ((IdentifierSymbolEntry*)id)->output();
    for (auto &func : func_list)
        func->output();
}

Unit::~Unit()
{
    for(auto &id : id_list)
        delete id;
    for(auto &func:func_list)
        delete func;
}
