#ifndef __AST_H__
#define __AST_H__

#include <fstream>
#include <vector>

class SymbolEntry;
class Type;

class Node
{
private:
    static int counter;
    int seq;
public:
    Node();
    int getSeq() const {return seq;};
    virtual void output(int level) = 0;
};

class ExprNode : public Node
{
protected:
    SymbolEntry *symbolEntry;
public:
    ExprNode(SymbolEntry *symbolEntry) : symbolEntry(symbolEntry){};
    Type* getType();
};

class UnaryExpr : public ExprNode
{
private:
    int op;
    ExprNode *expr;
public:
    enum {NOT, UMINUS, UPLUS};
    UnaryExpr(SymbolEntry *se, int op, ExprNode*expr) :ExprNode(se), op(op), expr(expr) {}
    void output(int level);
};

class StmtNode : public Node
{};




class BinaryExpr : public ExprNode
{
private:
    int op;
    ExprNode *expr1, *expr2;
public:
    enum {ADD, SUB, MUL, DIV, MOD, AND, OR, 
    LESS, GREATER, LESSEQUAL, GREATEREQUAL, TRUEEQUAL, FALSEEQUAL};
    BinaryExpr(SymbolEntry *se, int op, ExprNode*expr1, ExprNode*expr2) : ExprNode(se), op(op), expr1(expr1), expr2(expr2){};
    void output(int level);
};

class Constant : public ExprNode
{
public:
    Constant(SymbolEntry *se) : ExprNode(se){};
    void output(int level);
};

class ExpStmtNode : public StmtNode
{
private:
    ExprNode *expr;
public:
    ExpStmtNode(){};
    void addExpr(ExprNode *node);
    void output(int level);
};

class ArrayNode : public StmtNode
{
    private:
        std::vector<ExprNode*> exprList;
    public:
        ArrayNode(){};
        void addNode(ExprNode *node);
        void output(int level);
};

class Id : public ExprNode
{
private:
    ArrayNode* arrayList;
public:
    Id(SymbolEntry *se) : ExprNode(se), arrayList(nullptr){};
    bool isArray();
    void addArrayList(ArrayNode* a){arrayList=a;}
    void output(int level);
};

class CallParamSeqNode : public StmtNode
{
private:
    std::vector<ExprNode*> paramList;
public:
    CallParamSeqNode() {};
    void insertParam(ExprNode *param);
    void output(int level);
};

class CallFuncNode : public ExprNode
{
private:
    Id *funcId;
    CallParamSeqNode *paramList;
public:
    CallFuncNode(SymbolEntry *se,Id *funcId,CallParamSeqNode *paramList): ExprNode(se),funcId(funcId), paramList(paramList) {};
    void output(int level);
};

class CompoundStmt : public StmtNode
{
private:
    StmtNode *stmt;
public:
    CompoundStmt(StmtNode *stmt) : stmt(stmt) {};
    void output(int level);
};

class SeqNode : public StmtNode
{
private:
    StmtNode *stmt1, *stmt2;
public:
    SeqNode(StmtNode *stmt1, StmtNode *stmt2) : stmt1(stmt1), stmt2(stmt2){};
    void output(int level);
};

class ArrayDeclNode : public StmtNode
{
private:
    ExprNode* leafNode; //为空{}
    std::vector<ArrayDeclNode*> arrayDeclList;
public:
    ArrayDeclNode() : leafNode(nullptr){};
    void addDecl(ArrayDeclNode* next);
    void setLeafNode(ExprNode* leaf);
    bool isLeaf();
    void output(int level);
};

class VarDeclNode : public StmtNode
{
private:
    Id *id;
    Node *declNode; //数组为ArrayDeclNode, 其余为ExprNode
    bool isArray;
public:
    VarDeclNode(Id *id, Node *declNode, bool isArray) : id(id), declNode(declNode), isArray(isArray) {};
    void output(int level);
};

class VarDeclSeqNode : public StmtNode
{
private:
    std::vector<VarDeclNode*> varDeclList;
public:
    VarDeclSeqNode() {};
    void insertVarDecl(VarDeclNode *varDecl);
    void output(int level);
};

class VarDeclStmt : public StmtNode
{
private:
    VarDeclSeqNode *varDeclList;
public:
    VarDeclStmt(VarDeclSeqNode *varDeclList) : varDeclList(varDeclList){};
    void output(int level);
};

class ConstDeclNode : public StmtNode
{
private:
    Id *id;
    ExprNode *expr;
public:
    ConstDeclNode(Id *id, ExprNode *expr) : id(id), expr(expr) {};
    void output(int level);
};

class ConstDeclSeqNode : public StmtNode
{
private:
    std::vector<ConstDeclNode*> constDeclList;
public:
    ConstDeclSeqNode() {};
    void insertConstDecl(ConstDeclNode *constDecl);
    void output(int level);
};

class ConstDeclStmt : public StmtNode
{
private:
    ConstDeclSeqNode *constDeclList;
public:
    ConstDeclStmt(ConstDeclSeqNode *constDeclList) : constDeclList(constDeclList){};
    void output(int level);
};

class IfStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *thenStmt;
public:
    IfStmt(ExprNode *cond, StmtNode *thenStmt) : cond(cond), thenStmt(thenStmt){};
    void output(int level);
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
};

class WhileStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *whileStmt;
public:
     WhileStmt(ExprNode *cond, StmtNode *whileStmt) : cond(cond), whileStmt(whileStmt) {};
     void output(int level);
};

class BreakStmt : public StmtNode
{
public:
    BreakStmt() {}
    void output(int level);
};

class ContinueStmt : public StmtNode
{
public:
   ContinueStmt() {}
    void output(int level);
};

class ReturnStmt : public StmtNode
{
private:
    ExprNode *retValue;
public:
    ReturnStmt(ExprNode*retValue) : retValue(retValue) {};
    void output(int level);
};

class AssignStmt : public StmtNode
{
private:
    ExprNode *lval;
    Node *expr;
public:
    AssignStmt(ExprNode *lval, Node *expr) : lval(lval), expr(expr) {};
    void output(int level);
};

class FuncParamNode : public StmtNode
{
private:
    Id *id;
public:
    FuncParamNode(Id *id) : id(id) {};
    Type* getType();
    void output(int level);
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
};

class NullStmt : public StmtNode
{
public:
    NullStmt() {};
    void output(int level);
};

class Ast
{
private:
    Node* root;
public:
    Ast() {root = nullptr;}
    void setRoot(Node*n) {root = n;}
    void output();
};

#endif
