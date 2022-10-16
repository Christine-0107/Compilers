%{
/****************************************************************************
idListCal.y
ParserWizard generated YACC file.

Date: 2022年10月09日
****************************************************************************/
#include <cctype>
#include <iostream>
#include <map>
#include <vector>
using namespace std;
#ifndef YYSTYPE
#define YYSTYPE double
#endif
int yylex();
extern int yyparse();
FILE* yyin;
void yyerror(const char* s);
struct Node{
    string name;
    double value;
};
class SymbolTable
{
public:
    vector<Node> nameToValue;
public:
    SymbolTable(){}
    int lookup(const string& name);
    int insert(const string& name);
    void setValue(int pos,double value);
    double getValue(int pos);
};
SymbolTable symbolTable;
char idStr[50];
%}


%token ADD
%token SUB
%token MUL
%token DIV
%token L_PAREN
%token R_PAREN
%token NUMBER
%token EQUAL
%token ID

%left ADD SUB
%left MUL DIV
%right UMINUS

%%

lines	:	lines expr ';'	{ printf("%f\n", $2); }
		|	lines ';'
		|
		;

expr	:	expr ADD expr	{ $$ = $1 + $3; }
		|	expr SUB expr	{ $$ = $1 - $3; }
		|	expr MUL expr	{ $$ = $1 * $3; }
		|	expr DIV expr	{ $$ = $1 / $3; }
		|	L_PAREN expr R_PAREN	{ $$ = $2; }
		|	SUB expr %prec UMINUS	{ $$ = -$2; }
		|	NUMBER  { $$ = $1; }
        |   ID { $$ = symbolTable.getValue($1); }
        |   ID EQUAL expr   { symbolTable.setValue($1,$3);
                                $$ = $3; }
		;

/*NUMBER	:	'0'				{ $$ = 0.0; }
		|	'1'				{ $$ = 1.0; }
		|	'2'				{ $$ = 2.0; }
		|	'3'				{ $$ = 3.0; }
		|	'4'				{ $$ = 4.0; }
		|	'5'				{ $$ = 5.0; }
		|	'6'				{ $$ = 6.0; }
		|	'7'				{ $$ = 7.0; }
		|	'8'				{ $$ = 8.0; }
		|	'9'				{ $$ = 9.0; }
		;*/
		
		
%%
//符号表成员函数实现
int SymbolTable::lookup(const string& name){
    for(int i = 0; i < nameToValue.size(); i++){
        if(nameToValue[i].name.compare(name) == 0){
            return i;
        }
    }
    return -1;
}

int SymbolTable::insert(const string& name){
    Node node;
    node.name = name;
    node.value = 0.0;
    nameToValue.insert(std::end(nameToValue), node);
    return nameToValue.size()-1;
}

void SymbolTable::setValue(int pos,double value){
    nameToValue[pos].value = value;
}

double SymbolTable::getValue(int pos){
    return nameToValue[pos].value;
}

// program section
int yylex()
{
    //return getchar();
    int t;
    while(1)
    {
        t = getchar();
        if(t == ' ' || t == '\t' || t == '\n'){
            ;
        }
        else if(t == '+'){
            return ADD;
        }
        else if(t == '-'){
            return SUB;
        }
        else if(t == '*'){
            return MUL;
        }
        else if(t == '/'){
            return DIV;
        }
        else if(t == '('){
            return L_PAREN;
        }
        else if(t == ')'){
            return R_PAREN;
        }
        else if(t == '='){
            return EQUAL;
        }
        else if(isdigit(t)){
            yylval = 0;
            while(isdigit(t)){
                yylval = yylval * 10 + t - '0';
                t = getchar();
            }
            ungetc(t,stdin);
            return NUMBER;
        }
        else if((t>='a' && t<='z') || (t>='A' && t<='Z') || (t=='_')){
            int ti = 0;
            while((t>='a' && t<='z') || (t>='A' && t<='Z') || (t=='_') || (t>='0' && t<='9')){
                idStr[ti] = t;
                t = getchar();
                ti++;
            }
            idStr[ti] = '\0';
            ungetc(t,stdin);

            int p;
            string name = idStr;
            p=symbolTable.lookup(name);
            if(p == -1){
                p = symbolTable.insert(name);
            }
            yylval = p;
            return ID;
        }
        else{
            return t;
        }
        
    }
}

int main(void)
{
    yyin = stdin;
    do{
        yyparse();
    }while(!feof(yyin));
    return 0;
}

void yyerror(const char* s)
{
    fprintf(stderr,"Parse error:%s\n",s);
    exit(1);
}

