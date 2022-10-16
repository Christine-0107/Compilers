%{
/****************************************************************************
expression.y
ParserWizard generated YACC file.

Date: 2022年10月10日
****************************************************************************/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef YYSTYPE
#define YYSTYPE char*
#endif
char idStr[50];
char numStr[50];
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
%token ID

%left L_PAREN
%left ADD SUB
%left MUL DIV
%right UMINUS
%right R_PAREN

%%

lines	:	lines expr ';'	{ printf("%s\n", $2); }
		|	lines ';'
		|
		;

expr	:	expr ADD expr	{ $$ = (char *)malloc(50*sizeof(char)); 
                                strcpy($$,$1);
                                strcat($$,$3);
                                strcat($$,"+ "); }
		|	expr SUB expr	{ $$ = (char *)malloc(50*sizeof(char)); 
                                strcpy($$,$1);
                                strcat($$,$3);
                                strcat($$,"- "); }
		|	expr MUL expr	{ $$ = (char *)malloc(50*sizeof(char)); 
                                strcpy($$,$1);
                                strcat($$,$3);
                                strcat($$,"* "); }
		|	expr DIV expr	{ $$ = (char *)malloc(50*sizeof(char)); 
                                strcpy($$,$1);
                                strcat($$,$3);
                                strcat($$,"/ "); }
		|	L_PAREN expr R_PAREN    { $$ = (char *)malloc(50*sizeof(char)); 
                                        strcpy($$,$2); }	
		|	SUB expr %prec UMINUS	{ $$ = (char *)malloc(50*sizeof(char));
                                        strcpy($$,"- ");
                                        strcat($$,$2); }
		|	NUMBER  { $$ = (char *)malloc(50*sizeof(char));
                        strcpy($$,$1);
                        strcat($$," "); }
        |   ID      { $$ = (char *)malloc(50*sizeof(char));
                        strcpy($$,$1);
                        strcat($$," "); }
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
            int ti = 0;
            while(isdigit(t)){
                numStr[ti] = t;
                t = getchar();
                ti++;
            }
            numStr[ti] = '\0';
            yylval = numStr;
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
            yylval = idStr;
            ungetc(t,stdin);
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

