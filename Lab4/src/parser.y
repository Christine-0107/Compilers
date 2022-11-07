%code top{
    #include <iostream>
    #include <assert.h>
    #include "parser.h"
    extern Ast ast;
    int yylex();
    int yyerror( char const * );
    Type* cur;
}

%code requires {
    #include "Ast.h"
    #include "SymbolTable.h"
    #include "Type.h"
}

%union {
    int itype;
    float ftype;
    char* strtype;
    StmtNode* stmttype;
    ExprNode* exprtype;
    Type* type;
}

%start Program
%token <strtype> ID 
%token <itype> INTEGER 
%token <ftype> FFLOAT
%token INT FLOAT VOID
%token IF ELSE WHILE BREAK CONTINUE CONST RETURN
%token LBRACE RBRACE LPAREN RPAREN LBRACKET RBRACKET SEMICOLON COMMA
%token ASSIGN LESS GREATER GREATEREQUAL LESSEQUAL TRUEEQUAL FALSEEQUAL
%token ADD SUB MUL DIV MOD AND OR NOT

%nterm <stmttype> Stmts Stmt ExpStmt AssignStmt BlockStmt IfStmt WhileStmt BreakStmt ContinueStmt ReturnStmt DeclStmt VarDeclStmt VarDeclList VarDecl ConstDeclStmt ConstDeclList ConstDecl FuncParam FuncParamList FuncDef ArrayLists ArrayDecl ArrayDeclList FuncCallParamList
%nterm <exprtype> Exp AddExp MulExp Cond UnaryExp LOrExp LVal PrimaryExp RelExp EquExp LAndExp
%nterm <type> Type

%precedence THEN
%precedence ELSE
%%
Program
    : Stmts {
        ast.setRoot($1);
    }
    ;
Stmts
    : Stmt {$$=$1;}
    | Stmts Stmt{
        $$ = new SeqNode($1, $2);
    }
    ;
Stmt
    : AssignStmt {$$=$1;}
    | ExpStmt SEMICOLON {$$=$1;}
    | BlockStmt {$$=$1;}
    | IfStmt {$$=$1;}
    | WhileStmt {$$=$1;}
    | BreakStmt {$$=$1;}
    | ContinueStmt {$$=$1;}
    | ReturnStmt {$$=$1;}
    | DeclStmt {$$=$1;}
    | FuncDef {$$=$1;}
    | SEMICOLON {$$ = new NullStmt();}
    ;
LVal
    : ID {
        SymbolEntry *se;
        se = identifiers->lookup($1);
        if(se == nullptr)
        {
            fprintf(stderr, "identifier \"%s\" is undefined\n", (char*)$1);
            delete [](char*)$1;
            assert(se != nullptr);
        }
        $$ = new Id(se);
        delete []$1;
    }
    // 数组左值
    |  ID ArrayLists {
        SymbolEntry *se;
        se = identifiers->lookup($1);
        if(se == nullptr){
            fprintf(stderr, "identifier \"%s\" is undefined\n", (char*)$1);
            delete [](char*)$1;
            assert(se != nullptr);
        }
        Id* newId =new Id(se);
        newId->addArrayList((ArrayNode*)$2);
        $$ = newId;
        delete []$1;
    }
    ;
AssignStmt
    :
    LVal ASSIGN Exp SEMICOLON {
        $$ = new AssignStmt($1, $3);
    }
    ;
ExpStmt
    :
    Exp{
        ExprNode *node = (ExprNode*)$1;
        ExpStmtNode *stmtNode = new ExpStmtNode();
        stmtNode->addExpr(node);
        $$ = stmtNode;
    }
    ;
BlockStmt
    :   LBRACE 
        {identifiers = new SymbolTable(identifiers);} 
        Stmts RBRACE 
        {
            $$ = new CompoundStmt($3);
            SymbolTable *top = identifiers;
            identifiers = identifiers->getPrev();
            delete top;
        }
    |LBRACE RBRACE
    {
        $$=new CompoundStmt(nullptr);
    } 
    ;
WhileStmt
    : WHILE LPAREN Cond RPAREN Stmt {
        $$ = new WhileStmt($3, $5);
    }
    ;

IfStmt
    : IF LPAREN Cond RPAREN Stmt %prec THEN {
        $$ = new IfStmt($3, $5);
    }
    | IF LPAREN Cond RPAREN Stmt ELSE Stmt {
        $$ = new IfElseStmt($3, $5, $7);
    }
    ;
BreakStmt
    :  BREAK SEMICOLON {
        $$ = new BreakStmt();
    }
    ;
ContinueStmt
    :  CONTINUE SEMICOLON {
        $$ = new ContinueStmt();
    }
    ;
ReturnStmt
    :
    RETURN Exp SEMICOLON{
        $$ = new ReturnStmt($2);
    }
    |
    RETURN SEMICOLON{
        $$ = new ReturnStmt(nullptr);
    }
    ;
Exp
    :
    AddExp {$$ = $1;}
    ;
Cond
    :
    LOrExp {$$ = $1;}
    ;
PrimaryExp
    :
    LVal {
        $$ = $1;
    }
    | LPAREN Exp RPAREN{
        $$ = $2;
    }
    | INTEGER {
        //强转
        SymbolEntry *se = new ConstantSymbolEntry(TypeSystem::intType, (int)$1);
        $$ = new Constant(se);
    }
    | FFLOAT {
        SymbolEntry *se = new ConstantSymbolEntry(TypeSystem::floatType, (float)$1);
        $$ = new Constant(se);
    }
    ;

UnaryExp
    :
    PrimaryExp {$$=$1;}
    |
    ID LPAREN FuncCallParamList RPAREN{
        SymbolEntry *se;
        se = identifiers->lookup($1);
        if(se == nullptr){
            //printf(stderr, "identifier \"%s\" is undefined\n", (char*)$1);
            delete [](char*)$1;
            assert(se != nullptr);
        }
        SymbolEntry *tmp = new TemporarySymbolEntry(se->getType(), SymbolTable::getLabel());
        $$ = new CallFuncNode(tmp, new Id(se), (CallParamSeqNode*)$3);
    }
    |
    ADD UnaryExp
    {
        SymbolEntry *se;
        if($2->getType()->isInt())
            se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        else
            se = new TemporarySymbolEntry(TypeSystem::floatType, SymbolTable::getLabel());
        $$ = new UnaryExpr(se, UnaryExpr::UPLUS, $2);
    }
    |
    SUB UnaryExp
    {
        SymbolEntry *se;
        if($2->getType()->isInt())
            se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        else
            se = new TemporarySymbolEntry(TypeSystem::floatType, SymbolTable::getLabel());
        $$ = new UnaryExpr(se, UnaryExpr::UMINUS, $2);
    }
    |
    NOT UnaryExp
    {
        SymbolEntry *se;
        if($2->getType()->isInt())
            se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        else
            se = new TemporarySymbolEntry(TypeSystem::floatType, SymbolTable::getLabel());
        $$ = new UnaryExpr(se, UnaryExpr::NOT, $2);
    }
    ;

MulExp
    :
    UnaryExp {$$ = $1;}
    | 
    MulExp MUL UnaryExp
    {
        SymbolEntry *se;
        if(($1->getType()->isInt())&&($3->getType()->isInt()))
            se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        else
            se = new TemporarySymbolEntry(TypeSystem::floatType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::MUL, $1, $3);
    }
    |
    MulExp DIV UnaryExp
    {
        SymbolEntry *se;
        if(($1->getType()->isInt())&&($3->getType()->isInt()))
            se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        else
            se = new TemporarySymbolEntry(TypeSystem::floatType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::DIV, $1, $3);
    }  
    |
    MulExp MOD UnaryExp
    {
        SymbolEntry *se;
        if(($1->getType()->isInt())&&($3->getType()->isInt()))
            se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        else
            se = new TemporarySymbolEntry(TypeSystem::floatType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::MOD, $1, $3);
    }     
    ;

AddExp
    :
    MulExp {$$ = $1;}
    |
    AddExp ADD MulExp
    {
        SymbolEntry *se;
        if(($1->getType()->isInt())&&($3->getType()->isInt()))
            se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        else
            se = new TemporarySymbolEntry(TypeSystem::floatType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::ADD, $1, $3);
    }
    |
    AddExp SUB MulExp
    {
        SymbolEntry *se;
        if(($1->getType()->isInt())&&($3->getType()->isInt()))
            se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        else
            se = new TemporarySymbolEntry(TypeSystem::floatType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::SUB, $1, $3);
    }
    ;
RelExp
    :
    AddExp {$$ = $1;}
    |
    RelExp LESS AddExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::LESS, $1, $3);
    }
    |
    RelExp GREATER AddExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::GREATER, $1, $3);
    }
    |
    RelExp LESSEQUAL AddExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::LESSEQUAL, $1, $3);
    }
    |
    RelExp GREATEREQUAL AddExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::GREATEREQUAL, $1, $3);
    }  
    ;

EquExp
    :
    RelExp {$$ = $1;}
    |
    EquExp TRUEEQUAL RelExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::TRUEEQUAL, $1, $3);
    } 
    |
    EquExp FALSEEQUAL RelExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::FALSEEQUAL, $1, $3);
    }  
    ; 

LAndExp
    :
    EquExp {$$ = $1;}
    |
    LAndExp AND EquExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::AND, $1, $3);
    }
    ;

LOrExp
    :
    LAndExp {$$ = $1;}
    |
    LOrExp OR LAndExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::OR, $1, $3);
    }
    ;
Type
    : INT {
        $$ = TypeSystem::intType;
        cur=TypeSystem::intType;
    }
    | FLOAT {
        $$ = TypeSystem::floatType;
        cur=TypeSystem::floatType;
    }
    | VOID {
        $$ = TypeSystem::voidType;
        cur=TypeSystem::voidType;
    }
    ;
VarDeclStmt
    :
    Type VarDeclList SEMICOLON 
    {
        VarDeclStmt *varDeclStmt=new VarDeclStmt((VarDeclSeqNode*)$2);
        $$ = varDeclStmt;
    }
    ;
VarDeclList
    :
    VarDeclList COMMA VarDecl 
    {
        ((VarDeclSeqNode*)$1)->insertVarDecl((VarDeclNode*)$3);
        $$=$1;
    }
    |
    VarDecl 
    {
        VarDeclSeqNode *varDeclSeqNode=new VarDeclSeqNode();
        varDeclSeqNode->insertVarDecl((VarDeclNode*)$1);
        $$=varDeclSeqNode;
    }
    ;
VarDecl
    :ID
    {
        SymbolEntry *se=new IdentifierSymbolEntry(cur, $1, identifiers->getLevel());
        identifiers->install($1, se);
        $$ = new VarDeclNode(new Id(se),nullptr,false);
    }
    |ID  ASSIGN Exp
    {
        SymbolEntry *se= new IdentifierSymbolEntry(cur, $1, identifiers->getLevel());
        identifiers->install($1, se);
        $$ = new VarDeclNode(new Id(se),$3,false);
    }
    |ID  ArrayLists{
        Type* type;
        if(cur->isInt()){
            type=new IntArrayType();
        }
        else if(cur->isFloat()){
            type=new FloatArrayType();
        }
        SymbolEntry *se= new IdentifierSymbolEntry(type, $1, identifiers->getLevel());
        identifiers->install($1, se);
        Id *id = new Id(se);
        id->addArrayList((ArrayNode*)$2);
        $$ = new VarDeclNode(id, nullptr, true);
    }
    |ID  ArrayLists ASSIGN ArrayDecl{
        Type* type;
        if(cur->isInt()){
            type=new IntArrayType();
        }
        else if(cur->isFloat()){
            type=new FloatArrayType();
        }
        SymbolEntry *se= new IdentifierSymbolEntry(type, $1, identifiers->getLevel());
        identifiers->install($1, se);
        Id *id = new Id(se);
        id->addArrayList((ArrayNode*)$2);
        $$ = new VarDeclNode(id, (Node*)$4, true);
    } 
    ;
// 数组声明
ArrayDecl
    :
    Exp{
        ArrayDeclNode* node=new ArrayDeclNode();
        node->setLeafNode((ExprNode*)$1);
        $$ = node;
    }
    |LBRACE ArrayDeclList RBRACE{
        $$ =$2;
    }
    |LBRACE RBRACE{
        $$ = new ArrayDeclNode();
    }
    ;
// 数组声明列表
ArrayDeclList
    :
    ArrayDeclList COMMA ArrayDecl{
        ArrayDeclNode* node = (ArrayDeclNode*)$1;
        node->addDecl((ArrayDeclNode*)$3);
        $$ = node;
    }
    |ArrayDecl{
        ArrayDeclNode* node=new ArrayDeclNode();
        node->addDecl((ArrayDeclNode*)$1);
        $$ = node;
    }
    ;


ConstDeclStmt
    :
    CONST Type ConstDeclList SEMICOLON 
    {
        ConstDeclStmt *constDeclStmt=new ConstDeclStmt((ConstDeclSeqNode*)$3);
        $$ = constDeclStmt;
    }
    ;
ConstDeclList
    :
    ConstDeclList COMMA ConstDecl 
    {
        ((ConstDeclSeqNode*)$1)->insertConstDecl((ConstDeclNode*)$3);
        $$=$1;
    }
    |
    ConstDecl 
    {
        ConstDeclSeqNode *constDeclSeqNode=new ConstDeclSeqNode();
        constDeclSeqNode->insertConstDecl((ConstDeclNode*)$1);
        $$=constDeclSeqNode;
    }
    ;
ConstDecl
    :ID  ASSIGN Exp
    {
        SymbolEntry *se= new IdentifierSymbolEntry(cur, $1, identifiers->getLevel());
        identifiers->install($1, se);
        $$ = new ConstDeclNode(new Id(se),$3);
    }
    |
    ID  ArrayLists ASSIGN ArrayDecl{
        Type* type;
        if(cur->isInt()){
            type=new ConstIntArrayType();
        }
        else if(cur->isFloat()){
            type=new ConstFloatArrayType();
        }
        SymbolEntry *se= new IdentifierSymbolEntry(type, $1, identifiers->getLevel());
        identifiers->install($1, se);
        Id *id = new Id(se);
        id->addArrayList((ArrayNode*)$2);
        $$ = new VarDeclNode(id, (Node*)$4, true);
    } 
    ;

DeclStmt
    :
    VarDeclStmt {$$ = $1;}
    |
    ConstDeclStmt {$$ = $1;}
    ;
FuncParam
    :
    Type ID
    {
        SymbolEntry *se = new IdentifierSymbolEntry($1, $2, identifiers->getLevel());
        identifiers->install($2,se);
        FuncParamNode *param=new FuncParamNode(new Id(se));
        $$ = param;
    }
    | 
    // 数组类型，形如int a[]
    Type ID LBRACKET RBRACKET
    {
        Type *type;
        if($1==TypeSystem::intType){
            type = new IntArrayType();
        }
        else if($1==TypeSystem::floatType){
            type = new FloatArrayType();
        }
        SymbolEntry *se = new IdentifierSymbolEntry(type, $2, identifiers->getLevel());
        identifiers->install($2, se);
        Id* id = new Id(se);
        $$ = new FuncParamNode(id);
    }
    |
    // 形如int a[][5]
    Type ID LBRACKET RBRACKET ArrayLists
    {
        Type *type;
        if($1==TypeSystem::intType){
            type = new IntArrayType();
        }
        else if($1==TypeSystem::floatType){
            type = new FloatArrayType();
        }
        SymbolEntry *se = new IdentifierSymbolEntry(type, $2, identifiers->getLevel());
        identifiers->install($2, se);
        Id* id = new Id(se);
        id->addArrayList((ArrayNode*)$5);
        $$ = new FuncParamNode(id);
    }
    |
    // 形如int a[3][4]
    Type ID ArrayLists{
        Type *type;
        if($1==TypeSystem::intType){
            type = new IntArrayType();
        }
        else if($1==TypeSystem::floatType){
            type = new FloatArrayType();
        }
        SymbolEntry *se = new IdentifierSymbolEntry(type, $2, identifiers->getLevel());
        identifiers->install($2, se);
        Id* id = new Id(se);
        id->addArrayList((ArrayNode*)$3);
        $$ = new FuncParamNode(id);
    }
    |
    %empty 
    {
        FuncParamNode *param=new FuncParamNode(nullptr);
        $$ = param;
    }
    ;
FuncParamList
    :
    FuncParamList COMMA FuncParam
    {
        ((FuncParamSeqNode*)$1)->insertParam((FuncParamNode*)$3);
        $$ = $1;
    }
    |
    FuncParam
    {
        FuncParamSeqNode *paramList=new FuncParamSeqNode();
        paramList->insertParam((FuncParamNode*)$1);
        $$ = paramList;
    }
    ;
FuncCallParamList
    :
    FuncCallParamList COMMA Exp{
        CallParamSeqNode* node=(CallParamSeqNode*)$1;
        node->insertParam($3);
        $$ = node;
    }
    |
    Exp{
        CallParamSeqNode* node=new CallParamSeqNode();
        node->insertParam($1);
        $$ = node;
    }
    |
    %empty{
        $$ = nullptr;
    }
    ;
FuncDef
    :
    Type ID {
        Type *funcType;
        funcType = new FunctionType($1,{});
        SymbolEntry *se = new IdentifierSymbolEntry(funcType, $2, identifiers->getLevel());
        identifiers->install($2, se);
        identifiers = new SymbolTable(identifiers);
    }
    LPAREN 
    FuncParamList RPAREN BlockStmt
    {
        SymbolEntry *se;
        se = identifiers->lookup($2); 
        assert(se != nullptr);
        FuncParamSeqNode *paramList=((FuncParamSeqNode*)$5);
        FunctionType *funcType=(FunctionType*)se->getType();
        funcType->setParamsType(paramList->getParamsType());
        $$ = new FunctionDef(se, paramList, $7);
        SymbolTable *top = identifiers;
        identifiers = identifiers->getPrev();
        delete top;
        delete []$2;
    }
    ;

// TODO: 数组支持
// 数组的变量下标表示
ArrayLists 
    :
    ArrayLists LBRACKET Exp RBRACKET 
    {
        ArrayNode* arrayNode = (ArrayNode*)$1;
        arrayNode->addNode($3);
        $$ = arrayNode;
    }
    |
    LBRACKET Exp RBRACKET 
    {
        ArrayNode* arrayNode = new ArrayNode();
        arrayNode->addNode($2);
        $$ = arrayNode;
    }
    ;

 
%%

int yyerror(char const* message)
{
    std::cerr<<message<<std::endl;
    return -1;
}
