#ifndef __TYPE_H__
#define __TYPE_H__
#include <vector>
#include <string>

class Type
{
private:
    int kind;
protected:
    enum {INT, FLOAT, VOID, CONSTINT, CONSTFLOAT, INT_ARRAY, FLOAT_ARRAY, CONSTINT_ARRAY, CONSTFLOAT_ARRAY, ARRAY, FUNC, PTR, BOOL};
public:
    Type(int kind) : kind(kind) {};
    virtual ~Type() {};
    virtual std::string toStr() = 0;
    bool isInt() const {return kind == INT;};
    bool isFloat() const {return kind == FLOAT;};
    bool isVoid() const {return kind == VOID;};
    bool isFunc() const {return kind == FUNC;};
    bool isBool() const {return kind==BOOL;};
    bool isPtr() const {return kind == PTR;};
    bool isConstInt() const {return kind == CONSTINT;};
    bool isConstFloat() const {return kind == CONSTFLOAT;};
    bool isIntArray() const {return kind == INT_ARRAY;};
    bool isFloatArray() const {return kind == FLOAT_ARRAY;};
    bool isConstIntArray() const {return kind == CONSTINT_ARRAY;};
    bool isConstFloatArray() const {return kind == CONSTFLOAT_ARRAY;};
    bool isValue() const {return kind == INT || kind == FLOAT || kind == CONSTINT || kind == CONSTFLOAT;};
    bool isArray() const {return kind == INT_ARRAY || kind == FLOAT_ARRAY || kind == CONSTINT_ARRAY || kind == CONSTFLOAT_ARRAY;};
    bool isTypeInt() const {return kind==INT || kind==CONSTINT;};
    bool isTypeFloat() const {return kind==FLOAT ||kind==CONSTFLOAT;};
    bool isTypeIntArray() const {return kind==INT_ARRAY || kind == CONSTINT_ARRAY;};
    bool isTypeFloatArray() const {return kind == FLOAT_ARRAY || kind == CONSTFLOAT_ARRAY;};
};

class IntType : public Type
{
private:
    int size;
public:
    IntType(int size) : Type(Type::INT), size(size){};
    std::string toStr();
    int getSize(){return size;}
};

class BoolType : public Type
{
private:
    int size;
public:
    BoolType(int size) : Type(Type::BOOL), size(size){};
    std::string toStr();
};

class VoidType : public Type
{
public:
    VoidType() : Type(Type::VOID){};
    std::string toStr();
};

class FloatType : public Type
{
private:
    int size;
public:
    FloatType(int size) : Type(Type::FLOAT), size(size){};
    std::string toStr();
    int getSize(){return size;}
};

class ConstIntType : public Type
{
private:
    int size;
public:
    ConstIntType(int size) : Type(Type::CONSTINT), size(size){};
    std::string toStr();
};

class ConstFloatType : public Type
{
private:
    int size;
public:
    ConstFloatType(int size) : Type(Type::CONSTFLOAT), size(size){};
    std::string toStr();
};

class IntArrayType : public Type
{
private:
    std::vector<int> dims;
    int size = 32;
    bool isPointer;
public:
    IntArrayType() : Type(Type::INT_ARRAY){};
    std::string toStr();
    void addDim(int dim){
        dims.push_back(dim);
        if(dim>0){
            size*=dim; 
        }
        else{
            size=32;
        }
        //printf("%d\n",size);
    }
    std::vector<int> getDims(){return dims;}
    int getSize(){return size;}
    void setPointer(bool flag){this->isPointer = flag;}
    bool getPointer(){return isPointer;}
};

class FloatArrayType : public Type
{
private:
    std::vector<int> dims;
    int size = 4;
    bool isPointer;
public:
    FloatArrayType() : Type(Type::FLOAT_ARRAY){};
    std::string toStr();
    void addDim(int dim){
        dims.push_back(dim);
        if(dim>0){
            size*=dim; 
        }
        else{
            size=4;
        }
    }
    std::vector<int> getDims(){return dims;}
    int getSize(){return size;}
    void setPointer(bool flag){this->isPointer = flag;}
    bool getPointer(){return isPointer;}
};

class ConstIntArrayType : public Type
{
private:
    std::vector<int> dims;
    int size = 32;
    bool isPointer;
public:
    ConstIntArrayType() : Type(Type::CONSTINT_ARRAY){};
    std::string toStr();
    void addDim(int dim){
        dims.push_back(dim);
        if(dim>0){
            size*=dim; 
        }
        else{
            size=32;
        }
    }
    std::vector<int> getDims(){return dims;}
    int getSize(){return size;}
    void setPointer(bool flag){this->isPointer = flag;}
    bool getPointer(){return isPointer;}
};

class ConstFloatArrayType : public Type
{
private:
    std::vector<int> dims;
    int size = 4;
    bool isPointer;
public:
    ConstFloatArrayType() : Type(Type::CONSTFLOAT_ARRAY){};
    std::string toStr();
    void addDim(int dim){
        dims.push_back(dim);
        if(dim>0){
            size*=dim; 
        }
        else{
            size=4;
        }
    }
    std::vector<int> getDims(){return dims;}
    int getSize(){return size;}
    void setPointer(bool flag){this->isPointer = flag;}
    bool getPointer(){return isPointer;}
};

class FunctionType : public Type
{
private:
    Type *returnType;
    std::vector<Type*> paramsType;
public:
    FunctionType(Type* returnType, std::vector<Type*> paramsType) : 
    Type(Type::FUNC), returnType(returnType), paramsType(paramsType){};
    void setParamsType(std::vector<Type*> paramsType);
    Type* getRetType() {return returnType;};
    std::vector<Type*> getParamsType() {return paramsType;};
    std::string toStr();
};

class PointerType : public Type
{
private:
    Type *valueType;
public:
    PointerType(Type* valueType) : Type(Type::PTR) {this->valueType = valueType;};
    std::string toStr();
    Type* getType() {return valueType;};
};

class TypeSystem
{
private:
    static IntType commonInt;
    static BoolType commonBool;
    static VoidType commonVoid;
    static FloatType commonFloat;
    static ConstIntType commonConstInt;
    static ConstFloatType commonConstFloat;
    static IntArrayType commonIntArray;
    static FloatArrayType commonFloatArray;
    static ConstIntArrayType commonConstIntArray;
    static ConstFloatArrayType commonConstFloatArray;
public:
    static Type *intType;
    static Type *voidType;
    static Type *boolType;
    static Type *floatType;
    static Type *constIntType;
    static Type *constFloatType;
    static Type *intArrayType;
    static Type *floatArrayType;
    static Type *constIntArrayType;
    static Type *constFloatArrayType;
};

#endif
