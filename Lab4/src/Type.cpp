#include "Type.h"
#include <sstream>

IntType TypeSystem::commonInt = IntType(4);
FloatType TypeSystem::commonFloat = FloatType(4);
VoidType TypeSystem::commonVoid = VoidType();
ConstIntType TypeSystem::commonConstInt=ConstIntType();
ConstFloatType TypeSystem::commonConstFloat=ConstFloatType();
IntArrayType TypeSystem::commonIntArray=IntArrayType();
FloatArrayType TypeSystem::commonFloatArray=FloatArrayType();
ConstIntArrayType TypeSystem::commonConstIntArray=ConstIntArrayType();
ConstFloatArrayType TypeSystem::commonConstFloatArray=ConstFloatArrayType();

Type* TypeSystem::intType = &commonInt;
Type* TypeSystem::floatType = &commonFloat;
Type* TypeSystem::voidType = &commonVoid;
Type* TypeSystem::constIntType = &commonConstInt;
Type* TypeSystem::constFloatType = &commonConstFloat;
Type* TypeSystem::intArrayType = &commonIntArray;
Type* TypeSystem::floatArrayType = &commonFloatArray;
Type* TypeSystem::constIntArrayType = &commonConstIntArray;
Type* TypeSystem::constFloatArrayType = &commonConstFloatArray;

std::string IntType::toStr()
{
    return "int";
}

std::string FloatType::toStr()
{
    return "float";
}

std::string VoidType::toStr()
{
    return "void";
}

std::string ConstIntType::toStr()
{
    return "constant int";
}

std::string ConstFloatType::toStr()
{
    return "constant float";
}

void IntArrayType::addDim(int dim){
    dims.push_back(dim);
}

std::vector<int> IntArrayType::getDim(){
    return dims;
}

std::string IntArrayType::toStr()
{
    return "int array";
}

void FloatArrayType::addDim(int dim){
    dims.push_back(dim);
}

std::vector<int> FloatArrayType::getDim(){
    return dims;
}

std::string FloatArrayType::toStr()
{
    return "float array";
}

void ConstIntArrayType::addDim(int dim){
    dims.push_back(dim);
}

std::vector<int> ConstIntArrayType::getDim(){
    return dims;
}

std::string ConstIntArrayType::toStr()
{
    return "const int array";
}

void ConstFloatArrayType::addDim(int dim){
    dims.push_back(dim);
}

std::vector<int> ConstFloatArrayType::getDim(){
    return dims;
}

std::string ConstFloatArrayType::toStr()
{
    return "const float array";
}

std::string FunctionType::toStr()
{
    std::ostringstream buffer;
    buffer << returnType->toStr() << "()";
    return buffer.str();
}
