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
    char* strtype;
    StmtNode* stmttype;
    ExprNode* exprtype;
    Type* type;
}

%start Program
%token <strtype> ID 
%token <itype> INTEGER
%token IF ELSE CONST WHILE BREAK CONTINUE
%token INT VOID
%token LPAREN RPAREN LBRACE RBRACE SEMICOLON COMMA
%token ASSIGN LESS GREATER GREATEREQUAL LESSEQUAL TRUEEQUAL FALSEEQUAL
%token ADD SUB MUL DIV MOD 
%token OR AND NOT
%token RETURN

%nterm <stmttype> Stmts Stmt AssignStmt BlockStmt ExpStmt IfStmt WhileStmt BreakStmt ContinueStmt ReturnStmt FuncParam FuncParamList FuncDef CallParamList DeclStmt VarDeclStmt VarDeclList VarDecl ConstDeclStmt ConstDeclList ConstDecl 
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
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::MUL, $1, $3);
    }
    |
    MulExp DIV UnaryExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::DIV, $1, $3);
    }
    |
    MulExp MOD UnaryExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::MOD, $1, $3);
    }
    ;

AddExp
    :
    MulExp {$$ = $1;}
    |
    AddExp ADD MulExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        $$ = new BinaryExpr(se, BinaryExpr::ADD, $1, $3);
    }
    |
    AddExp SUB MulExp
    {
        SymbolEntry *se = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
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
        $$ = new VarDeclNode(new Id(se),nullptr);
    }
    |ID ASSIGN Exp
    {
        if(identifiers->lookupCur($1)!=nullptr) {
            fprintf(stderr, "identifier %s is redefined\n", $1);
            exit(EXIT_FAILURE);
        }
        SymbolEntry *se= new IdentifierSymbolEntry(cur, $1, identifiers->getLevel());
        identifiers->install($1, se);
        $$ = new VarDeclNode(new Id(se),$3);
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
        SymbolEntry *se= new IdentifierSymbolEntry(cur, $1, identifiers->getLevel());
        identifiers->install($1, se);
        $$ = new ConstDeclNode(new Id(se),$3);
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

%%

int yyerror(char const* message)
{
    std::cerr<<message<<std::endl;
    return -1;
}
