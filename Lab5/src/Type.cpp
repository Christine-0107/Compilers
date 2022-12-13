#include "Type.h"
#include <sstream>

IntType TypeSystem::commonInt = IntType(32);
BoolType TypeSystem::commonBool = BoolType(1);
VoidType TypeSystem::commonVoid = VoidType();
FloatType TypeSystem::commonFloat = FloatType(4);
ConstIntType TypeSystem::commonConstInt=ConstIntType();
ConstFloatType TypeSystem::commonConstFloat=ConstFloatType();
IntArrayType TypeSystem::commonIntArray=IntArrayType();
FloatArrayType TypeSystem::commonFloatArray=FloatArrayType();
ConstIntArrayType TypeSystem::commonConstIntArray=ConstIntArrayType();
ConstFloatArrayType TypeSystem::commonConstFloatArray=ConstFloatArrayType();

Type* TypeSystem::intType = &commonInt;
Type* TypeSystem::voidType = &commonVoid;
Type* TypeSystem::boolType = &commonBool;
Type* TypeSystem::floatType = &commonFloat;
Type* TypeSystem::constIntType = &commonConstInt;
Type* TypeSystem::constFloatType = &commonConstFloat;
Type* TypeSystem::intArrayType = &commonIntArray;
Type* TypeSystem::floatArrayType = &commonFloatArray;
Type* TypeSystem::constIntArrayType = &commonConstIntArray;
Type* TypeSystem::constFloatArrayType = &commonConstFloatArray;

std::string IntType::toStr()
{
    std::ostringstream buffer;
    buffer << "i" << size;
    return buffer.str();
}

std::string BoolType::toStr()
{
    std::ostringstream buffer;
    buffer << "i" << size;
    return buffer.str();
}

std::string VoidType::toStr()
{
    return "void";
}

std::string FloatType::toStr()
{
    return "float";
}

std::string ConstIntType::toStr()
{
    return "constant int";
}

std::string ConstFloatType::toStr()
{
    return "constant float";
}

std::string IntArrayType::toStr()
{
    return "int array";
}

std::string FloatArrayType::toStr()
{
    return "float array";
}

std::string ConstIntArrayType::toStr()
{
    return "const int array";
}

std::string ConstFloatArrayType::toStr()
{
    return "const float array";
}

void FunctionType::setParamsType(std::vector<Type*> paramsType)
{
    this->paramsType=paramsType;
}

std::string FunctionType::toStr()
{
    return returnType->toStr();
}

std::string PointerType::toStr()
{
    std::ostringstream buffer;
    buffer << valueType->toStr() << "*";
    return buffer.str();
}
