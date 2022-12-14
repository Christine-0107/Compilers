%option noyywrap
%{
    /*
    * You will need to comment this line in lab5.
    */
    #define ONLY_FOR_LEX
    
    #ifdef ONLY_FOR_LEX
    #else
    #define YYSTYPE void *
    #include "parser.h"
    #endif

    #define YY_NO_UNPUT
    #define YY_NO_INPUT
    #include <string>

    #ifdef ONLY_FOR_LEX
    #include <ostream>
    #include <fstream>
    #include <sstream>
    #include <cstring>
    #include <iomanip>
    #include <vector>
    using namespace std;
    extern FILE *yyin; 
    extern FILE *yyout;
    int offset;
    std::string tab="\t";
    std::string comment="";
    int commentLineno=0,commentOffset=0;
    enum TOKENTYPE
    {
        INT,FLOAT,VOID,NUMBER,ID,IF,ELSE,WHILE,BREAK,CONTINUE,RETURN, 
        TRUEEQU,NOTEQU,GREATEREQU,LESSEQU,GREATER,LESS,EQUAL,ADD,SUB,MUL,
        DIV,MOD,AND,OR,NOT,COMMA,SEMICOLON,LPAREN,RPAREN,LBRACE,RBRACE,LSBRACE,
        RSBRACE
    };
    struct symbol
    {
        TOKENTYPE type;
        std::string lexeme;
        int lineno;
        int offset;
        unsigned long scope;
    };
    class ScopeTable
    {
        private:
            std::vector<symbol> sTable; 
            int parentscope;
        public:
            ScopeTable(){parentscope=-1;}
            void setParentScope(int parentscope)
            {
                this->parentscope=parentscope;
            }
            int getParentScope()
            {
                return this->parentscope;
            }
   
            int searchSymbol(TOKENTYPE type,std::string lexeme)
            {
                std::vector<symbol>::iterator it;
                int flag=0;
                for(it=sTable.begin();it!=sTable.end();++it)
                {
                    if((it->type==type)&&(it->lexeme==lexeme))
                    {
                        flag=1;
                        break;
                    }
                }
                return flag;
            }         
            void addSymbol(TOKENTYPE type,std::string lexeme,int lineno,int offset,unsigned long scope)
            {
                if(searchSymbol(type,lexeme)==0)
                {
                    symbol s;
                    s.type=type;
                    s.lexeme=lexeme;
                    s.lineno=lineno;
                    s.offset=offset;
                    s.scope=scope;
                    sTable.push_back(s);
                }
                return;
            }

    }; 
    class SymbolTable
    {
        private:
            //std::vector<ScopeTable> scopeTables;
            ScopeTable scopeTables[1000];
            int tail;
            int curScope;
        public:
            SymbolTable()
            {
                tail=0;
                curScope=-1;
                scopeForward();
            }
            void scopeForward()
            {
                ScopeTable *scopeTable=new ScopeTable();
                scopeTables[tail]=*scopeTable;
                //scopeTables.push_back(*scopeTable);
                scopeTables[tail].setParentScope(curScope);
                curScope=tail;
                ++tail;    
            }
            void scopeBackward()
            {
                curScope=scopeTables[curScope].getParentScope();
            }
            void addSymbol(TOKENTYPE type,std::string lexeme,int lineno,int offset)
            {
                unsigned long scope=(unsigned long)(&scopeTables[curScope]);
                scopeTables[curScope].addSymbol(type,lexeme,lineno,offset,scope);
            }
            unsigned long getCurScope()
            {
                return (unsigned long)(&scopeTables[curScope]);
            }
    };
    SymbolTable* symbolTable=new SymbolTable();   
    void DEBUG_FOR_LAB4(std::string s){
        std::string DEBUG_INFO = "[DEBUG LAB4]: \t" + s 
        + "\t" + to_string((long long)yylineno) 
        + "\t" + to_string((long long)offset) 
        + "\n";
        fputs(DEBUG_INFO.c_str(), yyout);
    }
    void DEBUG_FOR_LAB4(std::string s,int mylineno,int myoffset){
        std::string DEBUG_INFO = "[DEBUG LAB4]: \t" + s 
        + "\t" + to_string((long long)mylineno) 
        + "\t" + to_string((long long)myoffset) 
        + "\n";
        fputs(DEBUG_INFO.c_str(), yyout);
    }
    #endif
%}

BLOCKCOMMENTBEGIN "/*"
BLOCKCOMMENTELEMENT .|\n
BLOCKCOMMENTEND "*/"
LINECOMMENT  \/\/[^\n]*
FLOATDECIMIAL (([1-9][0-9]*|0).[0-9]+)
DECIMIAL ([1-9][0-9]*|0)
OCTAL (0([1-7][0-7]*|0))
HEXADECIMAL (0(x|X)([1-9a-fA-F][0-9a-fA-F]*|0))
ID [[:alpha:]_][[:alpha:][:digit:]_]*
EOL (\r\n|\n|\r)
WHITE [\t ]
%x BLOCKCOMMENT
%%
{BLOCKCOMMENTBEGIN} {
    #ifdef ONLY_FOR_LEX
        BEGIN BLOCKCOMMENT;
        comment="";
        offset+=2;
        commentLineno=yylineno;
        commentOffset=offset;
    #else
        return BLOCKCOMMENTBEGIN;
    #endif
}
<BLOCKCOMMENT>{BLOCKCOMMENTELEMENT} {
    #ifdef ONLY_FOR_LEX
        string yycontext(yytext);
        comment+=yycontext;
        if(!strcmp(yytext,"\n"))
        {
            yylineno++;
            offset=0;
        }
        else
        {
            offset+=strlen(yytext);
        }
    #else
        return BLOCKCOMMENTELEMENT;
    #endif
}
<BLOCKCOMMENT>{BLOCKCOMMENTEND} {
    #ifdef ONLY_FOR_LEX
        comment="BLOCKCOMMENT"+tab+comment;
        DEBUG_FOR_LAB4(comment,commentLineno,commentOffset);
        BEGIN INITIAL;
        offset+=2;
    #else
        return BLOCKCOMMENTEND;
    #endif
}

{LINECOMMENT} {
    #ifdef ONLY_FOR_LEX
        string yycontext(yytext);
        yycontext=yycontext.substr(2,yycontext.length()-2);
        offset+=2;
        yycontext="LINECOMMENT"+tab+yycontext;
        DEBUG_FOR_LAB4(yycontext);
        offset+=yycontext.length();
    #else
        return LINECOMMENT;
    #endif
}
"int" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("INT\tint");
        offset+=strlen(yytext);
    #else
        return INT;
    #endif
}
"float" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("FLOAT\tfloat");
        offset+=strlen(yytext);
    #else
        return FLOAT;
    #endif
}
"void" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("VOID\tvoid");
        offset+=strlen(yytext);
    #else
        return VOID;
    #endif 
}
"const" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("CONST\tconst");
        offset+=strlen(yytext);
    #else
        return CONST;
    #endif 
}
"if" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("IF\tif");
        offset+=strlen(yytext);
    #else
        return IF;
    #endif
};
"else" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("ELSE\telse");
        offset+=strlen(yytext);
    #else
        return ELSE;
    #endif
};
"while" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("WHILE\twhile");
        offset+=strlen(yytext);
    #else
        return WHILE;
    #endif
};
"break" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("BREAK\tbreak");
        offset+=strlen(yytext);
    #else
        return BREAK;
    #endif
};
"continue" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("CONTINUE\tcontinue");
        offset+=strlen(yytext);
    #else
        return CONTINUE;
    #endif
};
"return" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("RETURN\treturn");
        offset+=strlen(yytext);
    #else
        return RETURN;
    #endif
}
"==" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("TRUEASSIGN\t==");
        offset+=strlen(yytext);
    #else
        return TRUEASSIGN;
    #endif
}
"!=" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("FALSEASSIGN\t!=");
        offset+=strlen(yytext);
    #else
        return FALSEASSIGN;
    #endif
}
">=" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("GREATERASSIGN\t>=");
        offset+=strlen(yytext);
    #else
        return GREATERASSIGN;
    #endif
}
"<=" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("LESSASSIGN\t<=");
        offset+=strlen(yytext);
    #else
        return LESSASSIGN;
    #endif
}
"=" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("ASSIGN\t=");
        offset+=strlen(yytext);
    #else
        return ASSIGN;
    #endif
}
"<" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("LESS\t<");
        offset+=strlen(yytext);
    #else
        return LESS;
    #endif
}
">" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("GREATER\t>");
        offset+=strlen(yytext);
    #else
        return GREATER;
    #endif
}
"+" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("ADD\t+");
        offset+=strlen(yytext);
    #else
        return ADD;
    #endif
}
"-" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("SUB\t-");
        offset+=strlen(yytext);
    #else
        return SUB;
    #endif
}
"*" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("MUL\t*");
        offset+=strlen(yytext);
    #else
        return MUL;
    #endif
}
"/" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("DIV\t/");
        offset+=strlen(yytext);
    #else
        return DIV;
    #endif
}
"%" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("MOD\t%");
        offset+=strlen(yytext);
    #else
        return MOD;
    #endif
}
"&" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("AND\t&");
        offset+=strlen(yytext);
    #else
        return AND;
    #endif
}
"|" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("OR\t|");
        offset+=strlen(yytext);
    #else
        return OR;
    #endif
}
"!" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("NOT\t!");
        offset+=strlen(yytext);
    #else
        return NOT;
    #endif
}
";" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("SEMICOLON\t;");
        offset+=strlen(yytext);
    #else
        return SEMICOLON;
    #endif
}
"," {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("COMMA\t,");
        offset+=strlen(yytext);
    #else
        return COMMA;
    #endif
}
"(" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("LPAREN\t(");
        offset+=strlen(yytext);
    #else
        return LPAREN;
    #endif
}
")" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("RPAREN\t)");
        offset+=strlen(yytext);
    #else
    return RPAREN;
    #endif
}
"{" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("LBRACE\t{");
        offset+=strlen(yytext);
        symbolTable->scopeForward();
    #else
        return LBRACE;
    #endif
}
"}" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("RBRACE\t}");
        offset+=strlen(yytext);
        symbolTable->scopeBackward();
    #else
        return RBRACE;
    #endif
}
"[" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("LSBRACE\t[");
        offset+=strlen(yytext);
    #else
        return LSBRACE;
    #endif
}
"]" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("RSBRACE\t]");
        offset+=strlen(yytext);
    #else
        return RSBRACE;
    #endif
}
{DECIMIAL} {
    #ifdef ONLY_FOR_LEX
        std::string yycontext(yytext);
        std::string str="DECIMIAL"+tab+yycontext;
        DEBUG_FOR_LAB4(str);
        offset+=strlen(yytext);
    #else
        return DECIMIAL;
    #endif
}
{OCTAL} {
    #ifdef ONLY_FOR_LEX
        long long val=std::strtol(yytext,NULL,8);
        std::string yycontext=std::to_string((long long)val);
        std::string str="OCTAL"+tab+yycontext;
        DEBUG_FOR_LAB4(str);
        offset+=strlen(yytext);
    #else
        return OCTAL;
    #endif
}
{HEXADECIMAL} {
    #ifdef ONLY_FOR_LEX
        long long val=std::strtol(yytext,NULL,16);
        std::string yycontext=std::to_string((long long)val);
        std::string str="HEXADECIMAL"+tab+yycontext;
        DEBUG_FOR_LAB4(str);
        offset+=strlen(yytext);
    #else
        return HEXADECIMAL;
    #endif
}
{FLOATDECIMIAL} {
    #ifdef ONLY_FOR_LEX
        std::string yycontext(yytext);
        std::string str="FLOATDECIMIAL"+tab+yycontext;
        DEBUG_FOR_LAB4(str);
        offset+=strlen(yytext);
    #else
        return FLOATDECIMIAL;
    #endif
}
{ID} {
    #ifdef ONLY_FOR_LEX    
        std::string yycontext(yytext);
        symbolTable->addSymbol(ID,yycontext,yylineno,offset);
        std::string str="ID"+tab+yycontext
        +tab+to_string((long long)symbolTable->getCurScope());
        DEBUG_FOR_LAB4(str);
        offset+=strlen(yytext);
    #else
        return RSBRACE;
    #endif
}
{EOL} {
    yylineno++;
    offset=0;
}
{WHITE} {
    offset+=strlen(yytext);
}
%%

#ifdef ONLY_FOR_LEX
int main(int argc, char **argv){
    if(argc != 5){
        fprintf(stderr, "Argument Not Enough");
        exit(EXIT_FAILURE);
    }

    if(!(yyin = fopen(argv[1], "r"))){
        fprintf(stderr, "No such file or directory: %s", argv[1]);
        exit(EXIT_FAILURE);
    }

    if(!(yyout = fopen(argv[3], "w"))){
        fprintf(stderr, "No such file or directory: %s", argv[3]);
        exit(EXIT_FAILURE);
    }
    yylineno=1;
    offset=0;
    yylex();
    return 0;
}
#endif
