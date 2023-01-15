%code top{
    #include <iostream>
    #include <assert.h>
    #include "parser.h"
    extern Ast ast;
    int yylex();
    int yyerror( char const * );
    Type *cur=nullptr;
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
%token IF ELSE CONST WHILE BREAK CONTINUE
%token INT FLOAT VOID 
%token LPAREN RPAREN LBRACE RBRACE SEMICOLON COMMA LBRACKET RBRACKET
%token ASSIGN LESS GREATER GREATEREQUAL LESSEQUAL TRUEEQUAL FALSEEQUAL
%token ADD SUB MUL DIV MOD 
%token OR AND NOT
%token RETURN

%nterm <stmttype> Stmts Stmt AssignStmt BlockStmt ExpStmt IfStmt WhileStmt BreakStmt ContinueStmt ReturnStmt FuncParam FuncParamList FuncDef CallParamList DeclStmt VarDeclStmt VarDeclList VarDecl ConstDeclStmt ConstDeclList ConstDecl ArrayLists ArrayDecl ArrayDeclList
%nterm <exprtype> Exp AddExp MulExp EquExp Cond LOrExp PrimaryExp UnaryExp LVal RelExp LAndExp
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
    | BlockStmt {$$=$1;}
    | ExpStmt {$$ = $1;}
    | IfStmt {$$=$1;}
    | WhileStmt {$$=$1;}
    | BreakStmt {$$=$1;}
    | ContinueStmt {$$=$1;}
    | ReturnStmt {$$=$1;}
    | DeclStmt {$$=$1;}
    | FuncDef {$$=$1;}
    | SEMICOLON {$$=new NullStmt();}
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

ExpStmt
    :
    Exp SEMICOLON
    {
        ExpStmt *tmp=new ExpStmt((ExprNode*)$1);
        $$ = tmp;
    }
IfStmt
    : IF LPAREN Cond RPAREN Stmt %prec THEN {
        $$ = new IfStmt($3, $5);
    }
    | IF LPAREN Cond RPAREN Stmt ELSE Stmt {
        $$ = new IfElseStmt($3, $5, $7);
    }
    ;
WhileStmt
    : WHILE LPAREN Cond RPAREN Stmt {
        $$ = new WhileStmt($3, $5);
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
    LVal 
    {
        $$ = $1;
    }
    |
    LPAREN Cond RPAREN
    {
        $$ = $2;
    }
    | INTEGER 
    {
        SymbolEntry *se = new ConstantSymbolEntry(TypeSystem::intType, $1);
        $$ = new Constant(se);
    }
    | FFLOAT
    {
        SymbolEntry *se = new ConstantSymbolEntry(TypeSystem::floatType, $1);
        $$ = new Constant(se);
    }
    ;
UnaryExp
    :
    PrimaryExp
    {
        $$ = $1;
    }
    | 
    ID LPAREN CallParamList RPAREN
    {
        SymbolEntry *se;
        se = identifiers->lookup($1);
        if(se == nullptr)
        {
            fprintf(stderr, "function \"%s\" is undefined\n", (char*)$1);
            delete [](char*)$1;
            assert(se != nullptr);
        }
        SymbolEntry *tmp=new TemporarySymbolEntry(((FunctionType*)(se->getType()))->getRetType(),SymbolTable::getLabel());
        $$ = new CallFunc(tmp,se,(CallParamSeqNode*)$3);
    }
    |
    ADD UnaryExp
    {
        //SymbolEntry *se = new TemporarySymbolEntry($2->getOperand()->getType(), SymbolTable::getLabel());
        //$$ = new UnaryExpr(se, UnaryExpr::UPLUS, $2);
        $$ = $2;
    }
    | 
    SUB UnaryExp
    {
        SymbolEntry *se = new TemporarySymbolEntry($2->getOperand()->getType(), SymbolTable::getLabel());
        $$ = new UnaryExpr(se, UnaryExpr::UMINUS, $2);
    }
    |
    NOT UnaryExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new UnaryExpr(se, UnaryExpr::NOT, $2);
    }
    ;

MulExp  
    :
    UnaryExp {$$ = $1;}
    |
    MulExp MUL UnaryExp
    {
        SymbolEntry *se=nullptr;
        if(($1->getType()->isTypeInt()||$1->getType()->isTypeIntArray()) && ($3->getType()->isTypeInt()||$3->getType()->isTypeIntArray()))
        {
            se= new TemporarySymbolEntry(TypeSystem::intType,SymbolTable::getLabel());
        }
        else
        {
            se = new TemporarySymbolEntry(TypeSystem::floatType,SymbolTable::getLabel());
        }
        $$ = new BinaryExpr(se, BinaryExpr::MUL, $1, $3);
    }
    |
    MulExp DIV UnaryExp
    {
        SymbolEntry *se=nullptr;
        if(($1->getType()->isTypeInt()||$1->getType()->isTypeIntArray()) && ($3->getType()->isTypeInt()||$3->getType()->isTypeIntArray()))
        {
            se = new TemporarySymbolEntry(TypeSystem::intType,SymbolTable::getLabel());
        }
        else
        {
            se = new TemporarySymbolEntry(TypeSystem::floatType,SymbolTable::getLabel());
        }
        $$ = new BinaryExpr(se, BinaryExpr::DIV, $1, $3);
    }
    |
    MulExp MOD UnaryExp
    {
        SymbolEntry *se=nullptr;
        if(($1->getType()->isTypeInt()||$1->getType()->isTypeIntArray()) && ($3->getType()->isTypeInt()||$3->getType()->isTypeIntArray()))
        {
            se = new TemporarySymbolEntry(TypeSystem::intType,SymbolTable::getLabel());
        }
        else
        {
            se = new TemporarySymbolEntry(TypeSystem::floatType,SymbolTable::getLabel());
        }
        $$ = new BinaryExpr(se, BinaryExpr::MOD, $1, $3);
    }
    ;

AddExp
    :
    MulExp {$$ = $1;}
    |
    AddExp ADD MulExp
    {
        SymbolEntry *se=nullptr;
        if(($1->getType()->isTypeInt()||$1->getType()->isTypeIntArray()) && ($3->getType()->isTypeInt()||$3->getType()->isTypeIntArray()))
        {
            se = new TemporarySymbolEntry(TypeSystem::intType,SymbolTable::getLabel());
        }
        else
        {
            se = new TemporarySymbolEntry(TypeSystem::floatType,SymbolTable::getLabel());
        }
        $$ = new BinaryExpr(se, BinaryExpr::ADD, $1, $3);
    }
    |
    AddExp SUB MulExp
    {
        SymbolEntry *se=nullptr;
        if(($1->getType()->isTypeInt()||$1->getType()->isTypeIntArray()) && ($3->getType()->isTypeInt()||$3->getType()->isTypeIntArray()))
        {
            se = new TemporarySymbolEntry(TypeSystem::intType,SymbolTable::getLabel());
        }
        else
        {
            se = new TemporarySymbolEntry(TypeSystem::floatType,SymbolTable::getLabel());
        }
        $$ = new BinaryExpr(se, BinaryExpr::SUB, $1, $3);
    }
    ;

RelExp
    :
    AddExp {$$ = $1;}
    |
    RelExp LESS AddExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::LESS, $1, $3);
    }
    |
    RelExp GREATER AddExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::GREATER, $1, $3);
    }
    |
    RelExp LESSEQUAL AddExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::LESSEQUAL, $1, $3);
    }
    |
    RelExp GREATEREQUAL AddExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::GREATEREQUAL, $1, $3);
    }
    ;
EquExp
    :
    RelExp {$$ = $1;}
    |
    EquExp TRUEEQUAL RelExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::TRUEEQUAL, $1, $3);
    } 
    |
    EquExp FALSEEQUAL RelExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::FALSEEQUAL, $1, $3);
    }  
    ; 
LAndExp
    :
    EquExp {$$ = $1;}
    |
    LAndExp AND EquExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::AND, $1, $3);
    }
    ;
LOrExp
    :
    LAndExp {$$ = $1;}
    |
    LOrExp OR LAndExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::OR, $1, $3);
    }
    ;
Type
    : INT 
    {
        $$ = TypeSystem::intType;
        cur = TypeSystem::intType;
    }
    | FLOAT {
        $$ = TypeSystem::floatType;
        cur=TypeSystem::floatType;
    }
    | VOID 
    {
        $$ = TypeSystem::voidType;
        cur = TypeSystem::voidType;
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
    :
    ID
    {
        //重定义判断
        if(identifiers->lookupCur($1)!=nullptr) {
            fprintf(stderr, "identifier %s is redefined\n", $1);
            exit(EXIT_FAILURE);
        }
        SymbolEntry *se;
        se=new IdentifierSymbolEntry(cur, $1, identifiers->getLevel());
        identifiers->install($1, se);
        $$ = new VarDeclNode(new Id(se),nullptr,false);
    }
    |ID ASSIGN Exp
    {
        if(identifiers->lookupCur($1)!=nullptr) {
            fprintf(stderr, "identifier %s is redefined\n", $1);
            exit(EXIT_FAILURE);
        }
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
    :
    ID ASSIGN Exp
    {
        if(identifiers->lookupCur($1)!=nullptr) {
            fprintf(stderr, "identifier %s is redefined\n", $1);
            exit(EXIT_FAILURE);
        }
        Type* type;
        if(cur->isInt()){
            type=new ConstIntType(4);
        }
        else if(cur->isFloat()){
            type=new ConstFloatType(4);
        }
        SymbolEntry *se= new IdentifierSymbolEntry(type, $1, identifiers->getLevel());
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
    :VarDeclStmt {$$ = $1;}
    |ConstDeclStmt{$$ = $1;}
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
        //高维指定为-1
        SymbolEntry *dim = new ConstantSymbolEntry(TypeSystem::constIntType, -1);
        ArrayNode* arrayList = new ArrayNode();
        arrayList->addNode(new Constant(dim));
        SymbolEntry *se = new IdentifierSymbolEntry(type, $2, identifiers->getLevel());
        identifiers->install($2, se);
        Id* id = new Id(se);
        id->addArrayList(arrayList);
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
        //高维指定为-1
        SymbolEntry *dim1 = new ConstantSymbolEntry(TypeSystem::constIntType, -1);
        dynamic_cast<ArrayNode*>($5)->addFirstNode(new Constant(dim1));
        SymbolEntry *se = new IdentifierSymbolEntry(type, $2, identifiers->getLevel());    
        identifiers->install($2, se);
        Id* id = new Id(se);
        id->addArrayList(dynamic_cast<ArrayNode*>($5));
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
FuncDef
    :
    Type ID 
    {
        if(identifiers->lookupCur($2)!=nullptr) {
            fprintf(stderr, "function %s is redefined\n", $2);
            exit(EXIT_FAILURE);
        }
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
CallParamList
    :
    CallParamList COMMA Exp
    {
        ((CallParamSeqNode*)$1)->insertParam($3);
        $$ = $1;
    }
    |
    Exp
    {
        CallParamSeqNode* node=new CallParamSeqNode();
        node->insertParam($1);
        $$ = node;
    }
    |
    %empty
    {
        CallParamSeqNode* node=new CallParamSeqNode();
        $$ = node;
    }
    ;

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
