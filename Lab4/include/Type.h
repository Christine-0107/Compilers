#ifndef __TYPE_H__
#define __TYPE_H__
#include <vector>
#include <string>

class Type
{
private:
    int kind;
protected:
    enum {INT, FLOAT, VOID, CONSTINT, CONSTFLOAT, INT_ARRAY, FLOAT_ARRAY, CONSTINT_ARRAY, CONSTFLOAT_ARRAY, ARRAY, FUNC};
public:
    Type(int kind) : kind(kind) {};
    virtual ~Type() {};
    virtual std::string toStr() = 0;
    bool isInt() const {return kind == INT;};
    bool isFloat() const {return kind == FLOAT;};
    bool isVoid() const {return kind == VOID;};
    bool isConstInt() const {return kind == CONSTINT;};
    bool isConstFloat() const {return kind == CONSTFLOAT;};
    bool isIntArray() const {return kind == INT_ARRAY;};
    bool isFloatArray() const {return kind == FLOAT_ARRAY;};
    bool isConstIntArray() const {return kind == CONSTINT_ARRAY;};
    bool isConstFloatArray() const {return kind == CONSTFLOAT_ARRAY;};
    bool isArray() const {return kind == INT_ARRAY || kind == FLOAT_ARRAY || kind == CONSTINT_ARRAY || kind == CONSTFLOAT_ARRAY;};
    bool isFunc() const {return kind == FUNC;};
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
    void setParamsType(std::vector<Type*> paramsType);
    std::string toStr();
};

class TypeSystem
{
private:
    static IntType commonInt;
    static FloatType commonFloat;
    static VoidType commonVoid;
    static ConstIntType commonConstInt;
    static ConstFloatType commonConstFloat;
    static IntArrayType commonIntArray;
    static FloatArrayType commonFloatArray;
    static ConstIntArrayType commonConstIntArray;
    static ConstFloatArrayType commonConstFloatArray;
public:
    static Type *intType;
    static Type *floatType;
    static Type *voidType;
    static Type *constIntType;
    static Type *constFloatType;
    static Type *intArrayType;
    static Type *floatArrayType;
    static Type *constIntArrayType;
    static Type *constFloatArrayType;
};

#endif
