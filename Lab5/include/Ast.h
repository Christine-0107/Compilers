#ifndef __AST_H__
#define __AST_H__

#include <fstream>
#include <stack>
#include "Operand.h"

class SymbolEntry;
class Unit;
class Function;
class BasicBlock;
class Instruction;
class IRBuilder;
class WhileStmt;

static std::stack<WhileStmt*> nestWhile;

class Node
{
private:
    static int counter;
    int seq;
protected:
    std::vector<Instruction*> true_list;
    std::vector<Instruction*> false_list;
    static IRBuilder *builder;
    void backPatch(std::vector<Instruction*> &list, BasicBlock*bb);
    std::vector<Instruction*> merge(std::vector<Instruction*> &list1, std::vector<Instruction*> &list2);

public:
    Node();
    int getSeq() const {return seq;};
    static void setIRBuilder(IRBuilder*ib) {builder = ib;};
    virtual void output(int level) = 0;
    virtual void typeCheck() = 0;
    virtual void genCode() = 0;
    std::vector<Instruction*>& trueList() {return true_list;}
    std::vector<Instruction*>& falseList() {return false_list;}
};

class ExprNode : public Node
{
protected:
    SymbolEntry *symbolEntry;
    Operand *dst;   // The result of the subtree is stored into dst.
public:
    ExprNode(SymbolEntry *symbolEntry) : symbolEntry(symbolEntry){};
    Operand* getOperand() {return dst;};
    SymbolEntry* getSymPtr() {return symbolEntry;};
};

class BinaryExpr : public ExprNode
{
private:
    int op;
    ExprNode *expr1, *expr2;
public:
    enum {ADD, SUB, MUL, DIV, MOD, AND, OR, 
    LESS, GREATER, LESSEQUAL, GREATEREQUAL, TRUEEQUAL, FALSEEQUAL};
    BinaryExpr(SymbolEntry *se, int op, ExprNode*expr1, ExprNode*expr2) : ExprNode(se), op(op), expr1(expr1), expr2(expr2){dst = new Operand(se);};
    void output(int level);
    void typeCheck();
    void genCode();
};

class UnaryExpr : public ExprNode
{
private:
    int op;
    ExprNode *expr;
public:
    enum {UPLUS, UMINUS, NOT};
    UnaryExpr(SymbolEntry *se, int op, ExprNode*expr) : ExprNode(se), op(op), expr(expr) {dst = new Operand(se);};
    void output(int level);
    void typeCheck();
    void genCode();
};

class Constant : public ExprNode
{
public:
    Constant(SymbolEntry *se) : ExprNode(se){dst = new Operand(se);};
    void output(int level);
    void typeCheck();
    void genCode();
};

class Id : public ExprNode
{
public:
    Id(SymbolEntry *se) : ExprNode(se){SymbolEntry *temp = new TemporarySymbolEntry(se->getType(), SymbolTable::getLabel()); dst = new Operand(temp);};
    Type* getType();
    SymbolEntry* getSymbolEntry() {return symbolEntry;}
    void output(int level);
    void typeCheck();
    void genCode();
};

class StmtNode : public Node
{};

class CompoundStmt : public StmtNode
{
private:
    StmtNode *stmt;
public:
    CompoundStmt(StmtNode *stmt) : stmt(stmt) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class SeqNode : public StmtNode
{
private:
    StmtNode *stmt1, *stmt2;
public:
    SeqNode(StmtNode *stmt1, StmtNode *stmt2) : stmt1(stmt1), stmt2(stmt2){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class ExpStmt : public StmtNode
{
private:
    ExprNode *expr;
public:
    ExpStmt(ExprNode *expr) : expr(expr) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class VarDeclNode : public StmtNode
{
private:
    Id *id;
    ExprNode *expr;
public:
    VarDeclNode(Id *id,ExprNode *expr) : id(id),expr(expr){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class VarDeclSeqNode : public StmtNode
{
private:
    std::vector<VarDeclNode*> varDeclList;
public:
    VarDeclSeqNode() {};
    void insertVarDecl(VarDeclNode *varDecl);
    void output(int level);
    void typeCheck();
    void genCode();
};

class VarDeclStmt : public StmtNode
{
private:
    VarDeclSeqNode *varDeclSeqNode;
public:
    VarDeclStmt(VarDeclSeqNode *varDeclSeqNode) :varDeclSeqNode(varDeclSeqNode){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class ConstDeclNode : public StmtNode
{
private:
    Id *id;
    ExprNode *expr;
public:
    ConstDeclNode(Id *id,ExprNode *expr) : id(id),expr(expr){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class ConstDeclSeqNode : public StmtNode
{
private:
    std::vector<ConstDeclNode*> constDeclList;
public:
    ConstDeclSeqNode() {};
    void insertConstDecl(ConstDeclNode *constDecl);
    void output(int level);
    void typeCheck();
    void genCode();
};

class ConstDeclStmt : public StmtNode
{
private:
    ConstDeclSeqNode *constDeclSeqNode;
public:
    ConstDeclStmt(ConstDeclSeqNode *constDeclSeqNode) :constDeclSeqNode(constDeclSeqNode){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class IfStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *thenStmt;
public:
    IfStmt(ExprNode *cond, StmtNode *thenStmt) : cond(cond), thenStmt(thenStmt){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class IfElseStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *thenStmt;
    StmtNode *elseStmt;
public:
    IfElseStmt(ExprNode *cond, StmtNode *thenStmt, StmtNode *elseStmt) : cond(cond), thenStmt(thenStmt), elseStmt(elseStmt) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class WhileStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *whileStmt;
    BasicBlock *cond_bb;
    BasicBlock *stmt_bb;
    BasicBlock *end_bb;
public:
    WhileStmt(ExprNode *cond, StmtNode *whileStmt) : cond(cond), whileStmt(whileStmt) {};
    void output(int level);
    void typeCheck();
    void genCode();
    BasicBlock* getCondBB(){return this->cond_bb;}
    BasicBlock* getStmtBB(){return this->stmt_bb;}
    BasicBlock* getEndBB(){return this->end_bb;}
};

class BreakStmt : public StmtNode
{
public:
    BreakStmt(){};
    void output(int level);
    void typeCheck();
    void genCode();    
};

class ContinueStmt : public StmtNode
{
public:
    ContinueStmt(){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class ReturnStmt : public StmtNode
{
private:
    ExprNode *retValue;
public:
    ReturnStmt(ExprNode*retValue) : retValue(retValue) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class AssignStmt : public StmtNode
{
private:
    ExprNode *lval;
    ExprNode *expr;
public:
    AssignStmt(ExprNode *lval, ExprNode *expr) : lval(lval), expr(expr) {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class FuncParamNode : public StmtNode
{
private:
    Id *id;
public:
    FuncParamNode(Id *id) : id(id) {};
    Id* getId() {return id;};
    Type* getType();
    void output(int level);
    void typeCheck();
    void genCode();
};

class FuncParamSeqNode : public StmtNode
{
private:
    std::vector<FuncParamNode*> paramList;
public:
    FuncParamSeqNode() {};
    void insertParam(FuncParamNode *param);
    std::vector<Type*> getParamsType();
    void output(int level);
    void typeCheck();
    void genCode();
};

class FunctionDef : public StmtNode
{
private:
    SymbolEntry *se;
    FuncParamSeqNode *paramList;
    StmtNode *stmt;
public:
    FunctionDef(SymbolEntry *se, FuncParamSeqNode *paramList, StmtNode *stmt) : se(se), paramList(paramList), stmt(stmt){};
    void output(int level);
    void typeCheck();
    void genCode();
};

class CallParamSeqNode : public StmtNode
{
private:
    std::vector<ExprNode*> paramList;
    std::vector<Operand*> operandList;
public:
    CallParamSeqNode() {};
    void insertParam(ExprNode *param);
    std::vector<Operand*> getParams() {return operandList;}
    std::vector<ExprNode*> getParamList() {return paramList;}
    void output(int level);
    void typeCheck();
    void genCode();
};

class CallFunc : public ExprNode
{
private:
    SymbolEntry *funcSe;
    CallParamSeqNode *callParamSeqNode;
public:
    CallFunc(SymbolEntry *dstSe,SymbolEntry *funcSe,CallParamSeqNode *callParamSeqNode): ExprNode(dstSe), funcSe(funcSe), callParamSeqNode(callParamSeqNode) {dst = new Operand(dstSe);};
    void output(int level);
    void typeCheck();
    void genCode();
};

class NullStmt : public StmtNode
{
public:
    NullStmt() {};
    void output(int level);
    void typeCheck();
    void genCode();
};

class Ast
{
private:
    Node* root;
public:
    Ast() {root = nullptr;}
    void setRoot(Node*n) {root = n;}
    void output();
    void typeCheck();
    void genCode(Unit *unit);
};


#endif
