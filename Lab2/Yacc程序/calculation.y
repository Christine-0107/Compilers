%{
/****************************************************************************
calculation.y
ParserWizard generated YACC file.

Date: 2022年10月09日
****************************************************************************/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef YYSTYPE
#define YYSTYPE double
#endif
int yylex();
extern int yyparse();
FILE* yyin;
void yyerror(const char* s);
%}


%token ADD
%token SUB
%token MUL
%token DIV
%token L_PAREN
%token R_PAREN
%token NUMBER

%left ADD SUB
%left MUL DIV
%right UMINUS

%%
// 用分号替换回车
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
        // NUMBER现在是一个终结符，词法分析阶段将他的值存在yylval中，隐式的存在$1中，在语法分析阶段通过yylval获取它的值。
		|	NUMBER  { $$ = $1; }
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
        else if(isdigit(t)){
            yylval = 0;
            while(isdigit(t)){
                yylval = yylval * 10 + t - '0';
                t = getchar();
            }
            ungetc(t,stdin);
            return NUMBER;
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

