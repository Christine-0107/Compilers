%option noyywrap
%{

    #define YY_NO_UNPUT
    #define YY_NO_INPUT

    #include "parser.h"
    #include <ostream>
    #include <fstream>
    #include <sstream>
    #include <cstring>
    #include <iomanip>
    #include <vector>
    using namespace std;
    extern FILE *yyin; 
    extern FILE *yyout;
    extern bool dump_tokens;
    int offset=0;
    std::string tab="\t";
    std::string comment="";
    int commentLineno=0,commentOffset=0;
    enum TOKENTYPE
    {
        IID
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
    class Lab4SymbolTable
    {
        private:
            //std::vector<ScopeTable> scopeTables;
            ScopeTable scopeTables[1000];
            int tail;
            int curScope;
        public:
            Lab4SymbolTable()
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
    Lab4SymbolTable* lab4SymbolTable=new Lab4SymbolTable();   
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
%}

BLOCKCOMMENTBEGIN "/*"
BLOCKCOMMENTELEMENT .|\n
BLOCKCOMMENTEND "*/"
LINECOMMENT  \/\/[^\n]*
FFLOAT ((([0-9]*[.][0-9]*([eE][+-]?[0-9]+)?)|([0-9]+[eE][+-]?[0-9]+))[fLlL]?)
HEXADECIMAL_FLOAT (0[xX](([0-9A-Fa-f]*[.][0-9A-Fa-f]*([pP][+-]?[0-9]+)?)|([0-9A-Fa-f]+[pP][+-]?[0-9]+))[fLlL]?)
INTEGER ([1-9][0-9]*|0)
OCTAL (0([1-7][0-7]*|0))
HEXADECIMAL (0(x|X)([1-9a-fA-F][0-9a-fA-F]*|0))
ID [[:alpha:]_][[:alpha:][:digit:]_]*
EOL (\r\n|\n|\r)
WHITE [\t ]
%x BLOCKCOMMENT
%%
{BLOCKCOMMENTBEGIN} {
    BEGIN BLOCKCOMMENT;
    if(dump_tokens)
    {
        comment="";
        offset+=2;
        commentLineno=yylineno;
        commentOffset=offset;
    }  
}
<BLOCKCOMMENT>{BLOCKCOMMENTELEMENT} {
    if(dump_tokens) 
    {
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
    }   
}
<BLOCKCOMMENT>{BLOCKCOMMENTEND} {
    BEGIN INITIAL;
    if(dump_tokens) 
    {
        comment="BLOCKCOMMENT"+tab+comment;
        DEBUG_FOR_LAB4(comment,commentLineno,commentOffset);  
        offset+=2;
    }    
}
{LINECOMMENT} {
    if(dump_tokens)
    { 
        string yycontext(yytext);
        yycontext=yycontext.substr(2,yycontext.length()-2);
        offset+=2;
        yycontext="LINECOMMENT"+tab+yycontext;
        DEBUG_FOR_LAB4(yycontext);
        offset+=yycontext.length();
    } 
}
"int" {
    if(dump_tokens) 
    {
        DEBUG_FOR_LAB4("INT\tint");
        offset+=strlen(yytext);
    }
    return INT; 
}
"float" {
    if(dump_tokens) 
    {
        DEBUG_FOR_LAB4("FLOAT\tfloat");
        offset+=strlen(yytext);
    }
    return FLOAT;  
}
"void" {
    if(dump_tokens) 
    {
        DEBUG_FOR_LAB4("VOID\tvoid");
        offset+=strlen(yytext);
    }
    return VOID;   
}
"const" {
    if(dump_tokens)
    { 
        DEBUG_FOR_LAB4("CONST\tconst");
        offset+=strlen(yytext);
    }
    return CONST;   
}
"if" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("IF\tif");
        offset+=strlen(yytext);
    }
    return IF;  
};
"else" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("ELSE\telse");
        offset+=strlen(yytext);
    }
    return ELSE;   
};
"while" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("WHILE\twhile");
        offset+=strlen(yytext);
    }
    return WHILE;
};
"break" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("BREAK\tbreak");
        offset+=strlen(yytext);
    }
    return BREAK;  
};
"continue" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("CONTINUE\tcontinue");
        offset+=strlen(yytext);
    }
    return CONTINUE;
};
"return" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("RETURN\treturn");
        offset+=strlen(yytext);
    }
    return RETURN;    
}
"==" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("TRUEASSIGN\t==");
        offset+=strlen(yytext);
    }
    return TRUEEQUAL;  
}
"!=" {
    if(dump_tokens)
    {  
        DEBUG_FOR_LAB4("FALSEASSIGN\t!=");
        offset+=strlen(yytext);
    }
    return FALSEEQUAL;   
}
">=" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("GREATERASSIGN\t>=");
        offset+=strlen(yytext);
    }
    return GREATEREQUAL;
}
"<=" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("LESSASSIGN\t<=");
        offset+=strlen(yytext);
    }
    return LESSEQUAL;
}
"=" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("ASSIGN\t=");
        offset+=strlen(yytext);
    }
    return ASSIGN;
}
"<" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("LESS\t<");
        offset+=strlen(yytext);
    }
    return LESS; 
}
">" {
    if(dump_tokens)
    {  
        DEBUG_FOR_LAB4("GREATER\t>");
        offset+=strlen(yytext);
    }
    return GREATER;  
}
"!" {
    if(dump_tokens)
    {  
        DEBUG_FOR_LAB4("NOT\t!");
        offset+=strlen(yytext);
    }
    return NOT;  
}
"+" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("ADD\t+");
        offset+=strlen(yytext);
    }
    return ADD;
}
"-" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("SUB\t-");
        offset+=strlen(yytext);
    }
    return SUB;
}
"*" {
    if(dump_tokens)
    {  
        DEBUG_FOR_LAB4("MUL\t*");
        offset+=strlen(yytext);
    }
    return MUL;
}
"/" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("DIV\t/");
        offset+=strlen(yytext);
    }
    return DIV;
}
"%" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("MOD\t%");
        offset+=strlen(yytext);
    }
    return MOD;
}
"&&" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("AND\t&");
        offset+=strlen(yytext);
    }
    return AND;
}
"||" {
    if(dump_tokens)
    {  
        DEBUG_FOR_LAB4("OR\t|");
        offset+=strlen(yytext);
    }
    return OR;
}
";" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("SEMICOLON\t;");
        offset+=strlen(yytext);
    }
    return SEMICOLON; 
}
"," {
    if(dump_tokens)
    {  
        DEBUG_FOR_LAB4("COMMA\t,");
        offset+=strlen(yytext);
    }
    return COMMA;
}
"(" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("LPAREN\t(");
        offset+=strlen(yytext);
    }
    return LPAREN;
}
")" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("RPAREN\t)");
        offset+=strlen(yytext);
    }
    return RPAREN;
}
"{" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("LBRACE\t{");
        offset+=strlen(yytext);
        lab4SymbolTable->scopeForward();
    }
    return LBRACE;
}
"}" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("RBRACE\t}");
        offset+=strlen(yytext);
        lab4SymbolTable->scopeBackward();
    }
    return RBRACE;
}
"[" {
    if(dump_tokens)
    {  
        DEBUG_FOR_LAB4("LSBRACE\t[");
        offset+=strlen(yytext);
    }
    return LBRACKET;
}
"]" {
    if(dump_tokens) 
    { 
        DEBUG_FOR_LAB4("RSBRACE\t]");
        offset+=strlen(yytext);
    }
    return RBRACKET;
}
{INTEGER} {
    if(dump_tokens) 
    { 
        std::string yycontext(yytext);
        std::string str="INTEGER"+tab+yycontext;
        DEBUG_FOR_LAB4(str);
        offset+=strlen(yytext);
    }
    int val=std::strtol(yytext,NULL,10);
    yylval.itype=val;
    return INTEGER;
}
{OCTAL} {
    int val=std::strtol(yytext,NULL,8);
    if(dump_tokens) 
    { 
        std::string yycontext=std::to_string((long long)val);
        std::string str="OCTAL"+tab+yycontext;
        DEBUG_FOR_LAB4(str);
        offset+=strlen(yytext);
    }   
    yylval.itype=val;
    return INTEGER;
}
{HEXADECIMAL} {
    int val=std::strtol(yytext,NULL,16);
    if(dump_tokens) 
    { 
        std::string yycontext=std::to_string((long long)val);
        std::string str="HEXADECIMAL"+tab+yycontext;
        DEBUG_FOR_LAB4(str);
        offset+=strlen(yytext);
    }
    yylval.itype=val;
    return INTEGER;
}
{FFLOAT} {
    if(dump_tokens)
    {  
        std::string yycontext(yytext);
        std::string str="FFLOAT"+tab+yycontext;
        DEBUG_FOR_LAB4(str);
        offset+=strlen(yytext);
    }
    float val;
    sscanf(yytext,"%f",&val);
    yylval.ftype=val;
    return FFLOAT; 
}
{HEXADECIMAL_FLOAT} {
    float val;
    sscanf(yytext, "%f", &val);
    if(dump_tokens) 
    { 
        std::string yycontext(yytext);
        std::string str="HEXADECIMAL_FLOAT"+tab+yycontext;
        DEBUG_FOR_LAB4(str);
        offset+=strlen(yytext);
    }
    yylval.ftype = val;
    return FFLOAT;  
}
{ID} {
    if(dump_tokens)  
    { 
        std::string yycontext(yytext);
        lab4SymbolTable->addSymbol(IID,yycontext,yylineno,offset);
        std::string str="ID"+tab+yycontext
        +tab+to_string((long long)lab4SymbolTable->getCurScope());
        DEBUG_FOR_LAB4(str);
        offset+=strlen(yytext);
    }
    char *lexeme;
    lexeme = new char[strlen(yytext) + 1];
    strcpy(lexeme, yytext);
    yylval.strtype = lexeme;
    return ID; 
}
{EOL} {
    if(dump_tokens) 
    { 
        yylineno++;
        offset=0;
    }
}
{WHITE} {
    if(dump_tokens)
    {
        offset+=strlen(yytext);
    }
}
%%