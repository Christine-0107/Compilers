#include "Ast.h"
#include "SymbolTable.h"
#include <string>
#include "Type.h"

extern FILE *yyout;
int Node::counter = 0;

Node::Node()
{
    seq = counter++;
}

void Ast::output()
{
    fprintf(yyout, "program\n");
    if(root != nullptr)
        root->output(4);
}

Type* ExprNode::getType()
{
    return symbolEntry->getType();
}

void NullStmt::output(int level)
{
    fprintf(yyout, "%*cNullStmt\n", level, ' ');
}

void UnaryExpr::output(int level)
{
    std::string op_str;
    switch(op)
    {
        case NOT:
            op_str = "not";
            break;
        case UMINUS:
            op_str = "uminus";
            break;
        case UPLUS:
            op_str = "uplus";
            break;
    }
    fprintf(yyout, "%*cUnaryExpr\top: %s\n", level, ' ', op_str.c_str());
    expr->output(level + 4);
}

void BinaryExpr::output(int level)
{
    std::string op_str;
    switch(op)
    {
        case ADD:
            op_str = "add";
            break;
        case SUB:
            op_str = "sub";
            break;
        case MUL:
            op_str = "mul";
            break;
        case DIV:
            op_str = "div";
            break;    
        case MOD:
            op_str = "mod";
            break;
        case AND:
            op_str = "and";
            break;
        case OR:
            op_str = "or";
            break;
        case LESS:
            op_str = "less";
            break;
        case GREATER:
            op_str = "greater";
            break;  
        case LESSEQUAL:
            op_str = "lessequal";
            break;              
        case GREATEREQUAL:
            op_str = "greaterequal";
            break;   
        case TRUEEQUAL:
            op_str = "trueequal";
            break;     
        case FALSEEQUAL:
            op_str = "falseequal";
            break;                     
    }
    fprintf(yyout, "%*cBinaryExpr\top: %s\n", level, ' ', op_str.c_str());
    expr1->output(level + 4);
    expr2->output(level + 4);
}

void Constant::output(int level)
{
    std::string type, value;
    type = symbolEntry->getType()->toStr();
    value = symbolEntry->toStr();
    if(type=="int")
    {
        fprintf(yyout, "%*cIntegerLiteral\tvalue: %s\ttype: %s\n", level, ' ',
            value.c_str(), type.c_str());
    }
    else if(type=="float")
    {
        fprintf(yyout, "%*cFloatLiteral\tvalue: %s\ttype: %s\n", level, ' ',
            value.c_str(), type.c_str());
    }
}   

void ExpStmtNode::addExpr(ExprNode *node){
    expr = node;
}

void ExpStmtNode::output(int level){
    fprintf(yyout, "%*cExpStmtNode\n", level, ' ');
    expr->output(level + 4);
}

void ArrayNode::addNode(ExprNode *node){
    exprList.push_back(node);
}

void ArrayNode::output(int level){
    fprintf(yyout, "%*cArrayNode\n", level, ' ');
    for(long unsigned int i=0;i<exprList.size();i++){
        exprList[i]->output(level+4);
    }
}

bool Id::isArray(){
    return getType()->isArray();
}

void Id::output(int level)
{
    std::string name, type;
    int scope;
    name = symbolEntry->toStr();
    type = symbolEntry->getType()->toStr();
    scope = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getScope();
    fprintf(yyout, "%*cId\tname: %s\tscope: %d\ttype: %s\n", level, ' ',
            name.c_str(), scope, type.c_str());
    if(isArray() && arrayList!=nullptr){
        arrayList->output(level+4);
    }
}

void CompoundStmt::output(int level)
{
    fprintf(yyout, "%*cCompoundStmt\n", level, ' ');
    if(stmt!=nullptr)
        stmt->output(level + 4);
    else
        fprintf(yyout, "%*cNullStmt\n", level+4, ' ');
}

void SeqNode::output(int level)
{
    fprintf(yyout, "%*cSequence\n", level, ' ');
    stmt1->output(level + 4);
    stmt2->output(level + 4);
}

void VarDeclStmt::output(int level)
{
    fprintf(yyout, "%*cVarDeclStmt\n", level, ' ');
    varDeclList->output(level + 4);
}

void ArrayDeclNode::addDecl(ArrayDeclNode* next){
    arrayDeclList.push_back(next);
}

void ArrayDeclNode::setLeafNode(ExprNode* leaf){
    leafNode = leaf;
}

bool ArrayDeclNode::isLeaf(){
    return arrayDeclList.empty();
}

void ArrayDeclNode::output(int level)
{
    fprintf(yyout, "%*cArrayDeclNode\n", level, ' ');
    for(long unsigned int i=0;i<arrayDeclList.size();i++){
        arrayDeclList[i]->output(level+4);
    }
    if(leafNode!=nullptr){
        leafNode->output(level+4);
    }
}

void VarDeclNode::output(int level)
{
    fprintf(yyout, "%*cVarDeclNode\n", level, ' ');
    id->output(level+4);
    if(declNode!=nullptr)
        declNode->output(level+4);
}

void VarDeclSeqNode::insertVarDecl(VarDeclNode *varDecl)
{
    varDeclList.push_back(varDecl);
}

void VarDeclSeqNode::output(int level)
{
    fprintf(yyout, "%*cVarDeclSeqNode\n", level, ' ');
    std::vector<VarDeclNode*>::iterator it;
    for(it=varDeclList.begin();it!=varDeclList.end();++it)
    {
        (*it)->output(level+4);
    }
}

void ConstDeclStmt::output(int level)
{
    fprintf(yyout, "%*cConstDeclStmt\n", level, ' ');
    constDeclList->output(level + 4);
}

void ConstDeclNode::output(int level)
{
    fprintf(yyout, "%*cConstDeclNode\n", level, ' ');
    id->output(level+4);
    if(expr!=nullptr)
        expr->output(level+4);
}

void ConstDeclSeqNode::insertConstDecl(ConstDeclNode *constDecl)
{
    constDeclList.push_back(constDecl);
}

void ConstDeclSeqNode::output(int level)
{
    fprintf(yyout, "%*cConstDeclSeqNode\n", level, ' ');
    std::vector<ConstDeclNode*>::iterator it;
    for(it=constDeclList.begin();it!=constDeclList.end();++it)
    {
        (*it)->output(level+4);
    }
}

void IfStmt::output(int level)
{
    fprintf(yyout, "%*cIfStmt\n", level, ' ');
    cond->output(level + 4);
    thenStmt->output(level + 4);
}

void IfElseStmt::output(int level)
{
    fprintf(yyout, "%*cIfElseStmt\n", level, ' ');
    cond->output(level + 4);
    thenStmt->output(level + 4);
    elseStmt->output(level + 4);
}

void WhileStmt::output(int level)
{
    fprintf(yyout, "%*cWhileStmt\n", level, ' ');
    cond->output(level + 4);
    whileStmt->output(level + 4);
}

void BreakStmt::output(int level)
{
    fprintf(yyout, "%*cBreakStmt\n", level, ' ');
}

void ContinueStmt::output(int level)
{
    fprintf(yyout, "%*cContinueStmt\n", level, ' ');
}

void ReturnStmt::output(int level)
{
    fprintf(yyout, "%*cReturnStmt\n", level, ' ');
    if(retValue!=nullptr)
        retValue->output(level + 4);
    else
        fprintf(yyout, "%*cvoid\n", level+4, ' ');
}

void AssignStmt::output(int level)
{
    fprintf(yyout, "%*cAssignStmt\n", level, ' ');
    lval->output(level + 4);
    expr->output(level + 4);
}

void FuncParamNode::output(int level)
{
    fprintf(yyout, "%*cFuncParamNode\n", level, ' ');
    if(id!=nullptr)
        id->output(level+4);
}

void FuncParamSeqNode::insertParam(FuncParamNode *param)
{
    paramList.push_back(param);
}

void FuncParamSeqNode::output(int level)
{
    fprintf(yyout, "%*cFuncParamSeqNode\n", level, ' ');
    std::vector<FuncParamNode*>::iterator it;
    for(it=paramList.begin();it!=paramList.end();++it)
    {
        (*it)->output(level+4);
    }
}
void FunctionDef::output(int level)
{
    std::string name, type;
    name = se->toStr();
    type = se->getType()->toStr();
    fprintf(yyout, "%*cFunctionDefine function name: %s, type: %s\n", level, ' ', 
            name.c_str(), type.c_str());
    paramList->output(level+4);
    stmt->output(level + 4);
}

void CallParamSeqNode::insertParam(ExprNode *param)
{
    paramList.push_back(param);
}

void CallParamSeqNode::output(int level)
{
    fprintf(yyout, "%*cCallParamSeqNode\n", level, ' ');
    std::vector<ExprNode*>::iterator it;
    for(it=paramList.begin();it!=paramList.end();++it)
    {
        (*it)->output(level+4);
    }
}

void CallFuncNode ::output(int level)
{
    fprintf(yyout, "%*cCallFuncNode\n", level, ' ');
    funcId->output(level+4);
    if(paramList!=nullptr){
        paramList->output(level+4);
    }
    else{
        fprintf(yyout, "%*cCallFuncNode NULL\n", level+4, ' ');
    }
}
