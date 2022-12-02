#ifndef __TYPE_H__
#define __TYPE_H__
#include <vector>
#include <string>

class Type
{
private:
    int kind;
protected:
    //enum {INT, VOID, FUNC, PTR};
    enum {INT, FLOAT, VOID, CONSTINT, CONSTFLOAT, INT_ARRAY, FLOAT_ARRAY, CONSTINT_ARRAY, CONSTFLOAT_ARRAY, ARRAY, FUNC, PTR};
public:
    Type(int kind) : kind(kind) {};
    virtual ~Type() {};
    virtual std::string toStr() = 0;
    bool isInt() const {return kind == INT;};
    bool isVoid() const {return kind == VOID;};
    bool isFunc() const {return kind == FUNC;};
    bool isFloat() const {return kind == FLOAT;};
    bool isConstInt() const {return kind == CONSTINT;};
    bool isConstFloat() const {return kind == CONSTFLOAT;};
    bool isIntArray() const {return kind == INT_ARRAY;};
    bool isFloatArray() const {return kind == FLOAT_ARRAY;};
    bool isConstIntArray() const {return kind == CONSTINT_ARRAY;};
    bool isConstFloatArray() const {return kind == CONSTFLOAT_ARRAY;};
    bool isArray() const {return kind == INT_ARRAY || kind == FLOAT_ARRAY || kind == CONSTINT_ARRAY || kind == CONSTFLOAT_ARRAY;};
};

class IntType : public Type
{
private:
    int size;
public:
    IntType(int size) : Type(Type::INT), size(size){};
    std::string toStr();
};

class FloatType : public Type
{
private:
    int size;
public:
    FloatType(int size) : Type(Type::FLOAT), size(size){};
    std::string toStr();
};

class VoidType : public Type
{
public:
    VoidType() : Type(Type::VOID){};
    std::string toStr();
};

class ConstIntType : public Type
{
public:
    ConstIntType() : Type(Type::CONSTINT){};
    std::string toStr();
};

class ConstFloatType : public Type
{
public:
    ConstFloatType() : Type(Type::CONSTFLOAT){};
    std::string toStr();
};

class IntArrayType : public Type
{
public:
    IntArrayType() : Type(Type::INT_ARRAY){};
    std::string toStr();
};

class FloatArrayType : public Type
{
public:
    FloatArrayType() : Type(Type::FLOAT_ARRAY){};
    std::string toStr();
};

class ConstIntArrayType : public Type
{
public:
    ConstIntArrayType() : Type(Type::CONSTINT_ARRAY){};
    std::string toStr();
};

class ConstFloatArrayType : public Type
{
public:
    ConstFloatArrayType() : Type(Type::CONSTFLOAT_ARRAY){};
    std::string toStr();
};

class FunctionType : public Type
{
private:
    Type *returnType;
    std::vector<Type*> paramsType;
public:
    FunctionType(Type* returnType, std::vector<Type*> paramsType) : 
    Type(Type::FUNC), returnType(returnType), paramsType(paramsType){};
    Type* getRetType() {return returnType;};
    void setParamsType(std::vector<Type*> paramsType);
    std::string toStr();
};

class PointerType : public Type
{
private:
    Type *valueType;
public:
    PointerType(Type* valueType) : Type(Type::PTR) {this->valueType = valueType;};
    std::string toStr();
};

class TypeSystem
{
private:
    static IntType commonInt;
    static IntType commonBool;
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
