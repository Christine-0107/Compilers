#include "Ast.h"
#include "SymbolTable.h"
#include "Unit.h"
#include "Instruction.h"
#include "IRBuilder.h"
#include <string>
#include <list>
#include "Type.h"

extern FILE *yyout;
int Node::counter = 0;
IRBuilder* Node::builder = nullptr;
Type* returnTypeDef = nullptr;
Type* returnType=nullptr;
int count=0;
double globalVal=0;
double constVal=0;
int isCond=0;

Node::Node()
{
    seq = counter++;
}

void Node::backPatch(std::vector<Instruction*> &list, BasicBlock*bb)
{
    for(auto &inst:list)
    {
        if(inst->isCond())
            dynamic_cast<CondBrInstruction*>(inst)->setTrueBranch(bb);
        else if(inst->isUncond())
            dynamic_cast<UncondBrInstruction*>(inst)->setBranch(bb);
    }
}

std::vector<Instruction*> Node::merge(std::vector<Instruction*> &list1, std::vector<Instruction*> &list2)
{
    std::vector<Instruction*> res(list1);
    res.insert(res.end(), list2.begin(), list2.end());
    return res;
}

void Ast::genCode(Unit *unit)
{
    IRBuilder *builder = new IRBuilder(unit);
    Node::setIRBuilder(builder);
    root->genCode();
}

void FuncParamNode::genCode()
{
    if(id!=nullptr)
        id->genCode();
}

void FuncParamSeqNode::genCode()
{
    BasicBlock *bb,*entry;
    bb=builder->getInsertBB();
    Function *func=bb->getParent();
    entry=func->getEntry();
    for(auto param=paramList.begin();param!=paramList.end();++param)
    {
        auto paramId=(*param)->getId();
        if(paramId==nullptr)
            continue;
        func->insertParam(paramId->getOperand());
        IdentifierSymbolEntry *idSe=dynamic_cast<IdentifierSymbolEntry*>(paramId->getSymbolEntry());
        Type *type=new PointerType(paramId->getType());
        SymbolEntry *addrSe = new TemporarySymbolEntry(type, SymbolTable::getLabel());
        Operand *addr = new Operand(addrSe);
        Instruction *allocaIn=new AllocaInstruction(addr,idSe);
        entry->insertFront(allocaIn);
        idSe->setAddr(addr);
        Operand *src=paramId->getOperand();
        new StoreInstruction(addr, src, entry);
    }
}

void FunctionDef::genCode()
{
    Unit *unit = builder->getUnit();
    Function *func = new Function(unit, se);
    BasicBlock *entry = func->getEntry();
    // set the insert point to the entry basicblock of this function.
    builder->setInsertBB(entry);
    if(this->paramList!=nullptr)
        paramList->genCode();
    stmt->genCode();

    /**
     * Construct control flow graph. You need do set successors and predecessors for each basic block.
     * Todo
    */ 
    for(auto block=func->begin();block!=func->end();block++)
    {
        for(auto i=(*block)->begin();i!=(*block)->rbegin();i=i->getNext())
        {
            if(i->isRet())
            {
                i=i->getNext();
                for(;i!=(*block)->end();i=i->getNext())
                    (*block)->remove(i);
                break;
            }
        }
        for(auto i=(*block)->begin();i!=(*block)->rbegin();i=i->getNext())
        {
            if(i->isCond() || i->isUncond())
                (*block)->remove(i);
        }
        Instruction* finalInstruction=(*block)->rbegin();
        if(finalInstruction->isCond())
        {
            BasicBlock *trueBranch=dynamic_cast<CondBrInstruction*>(finalInstruction)->getTrueBranch();
            BasicBlock *falseBranch=dynamic_cast<CondBrInstruction*>(finalInstruction)->getFalseBranch();
            (*block)->addSucc(trueBranch);
            (*block)->addSucc(falseBranch);
            trueBranch->addPred(*block);
            falseBranch->addPred(*block);
        }
        if(finalInstruction->isUncond())
        {
            BasicBlock *branch=dynamic_cast<UncondBrInstruction*>(finalInstruction)->getBranch();
            (*block)->addSucc(branch);
            branch->addPred(*block);
        }
    }
    bool isret=false;
    for(auto block=func->begin();block!=func->end();block++)
    {
        for(auto i=(*block)->begin();i!=(*block)->end();i=i->getNext())
        {
            if(i->isRet())
            {
                isret=true;
            }
        }
    }

    if(!isret)
    {
        BasicBlock *finalBB=nullptr;
        if(func->size()==0)
        {
            auto retBB=new BasicBlock(func);
            auto funcSe=dynamic_cast<IdentifierSymbolEntry*>(func->getSymPtr());
            auto funcType=dynamic_cast<FunctionType*>(funcSe->getType());
            auto isvoid=funcType->getRetType()->isVoid();
            if(isvoid)
            {
                new RetInstruction(nullptr,retBB);
            }        
            /*else
            {
                auto zeroSe=new ConstantSymbolEntry(TypeSystem::intType,0);
                auto zeroOperand=new Operand(zeroSe);
                new RetInstruction(zeroOperand,retBB);
            }*/
            return;
        }
        else if(func->size()==1)
        {
            finalBB=(*(func->begin()));
        }
        else
        {
            for(auto block=func->begin();block!=func->end();block++)
            {
                if((*block)->succEmpty()&&(!((*block)->predEmpty())))
                {
                    finalBB=(*block);
                    break;   
                }
            }
        }
        auto retBB=new BasicBlock(func);
        builder->setInsertBB(finalBB);
        new UncondBrInstruction(retBB,finalBB);
        finalBB->addSucc(retBB);
        retBB->addPred(finalBB);
        builder->setInsertBB(retBB);
        auto funcSe=dynamic_cast<IdentifierSymbolEntry*>(func->getSymPtr());
        auto funcType=dynamic_cast<FunctionType*>(funcSe->getType());
        auto isvoid=funcType->getRetType()->isVoid();
        if(isvoid)
        {
            new RetInstruction(nullptr,retBB);
        }
        /*else
        {
            auto zeroSe=new ConstantSymbolEntry(TypeSystem::intType,0);
            auto zeroOperand=new Operand(zeroSe);
            new RetInstruction(zeroOperand,retBB);
        }*/
    }
    /**
     * int example()
     * {
     *     while(1)
     *     {
     *         a=1;
     *     }
     * }
    */
    if(func->size()>1)
    {
        auto funcSe=dynamic_cast<IdentifierSymbolEntry*>(func->getSymPtr());
        auto funcType=dynamic_cast<FunctionType*>(funcSe->getType());
        auto isvoid=funcType->getRetType()->isVoid();
        for(auto block=func->begin();block!=func->end();block++)
        {
            if(((*block)->succEmpty())&&(!((*block)->predEmpty()))&&((*block)->empty()))
            {
                if(isvoid)
                {
                    new RetInstruction(nullptr,(*block));
                }
                else
                {
                    auto zeroSe=new ConstantSymbolEntry(funcType->getRetType(),0);
                    auto zeroOperand=new Operand(zeroSe);
                    new RetInstruction(zeroOperand,(*block));
                }
            }
        }        
    }

}

void UnaryExpr::genCode()
{
    if(op==UPLUS)
    {
        expr->genCode();
    }
    else if(op==UMINUS)
    {
        BasicBlock *bb = builder->getInsertBB();
        if(bb==nullptr)
        {
            double val=0;
            if(expr->getSymPtr()->isConstant())
                val=(dynamic_cast<ConstantSymbolEntry*>(expr->getSymPtr()))->getValue();
            else if(expr->getSymPtr()->isVariable())
                val=(dynamic_cast<IdentifierSymbolEntry*>(expr->getSymPtr()))->getValue();
            else
            {
                expr->genCode();
                val=globalVal;
                globalVal=0;
            }
            globalVal=-val;
            return;
        }
        expr->genCode();
        Operand *src2;
        if(expr->getOperand()->getType()->isBool())
        {
            src2=new Operand(new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel()));
            new ZextInstruction(src2, expr->getOperand(), bb);
        }
        else
        {
            src2=expr->getOperand();
        }
        Operand *src1=new Operand(new ConstantSymbolEntry(src2->getType(),0));
        SymbolEntry *new_se = new TemporarySymbolEntry(src2->getType(), SymbolTable::getLabel());
        dst = new Operand(new_se);
        if(src2->getType()->isTypeFloat())
            new BinaryInstruction(BinaryInstruction::FSUB, dst, src1, src2, bb);
        else
            new BinaryInstruction(BinaryInstruction::SUB, dst, src1, src2, bb);
    }
    else if(op==NOT)
    {
        BasicBlock *bb=builder->getInsertBB();
        expr->genCode();
        Operand *src;
        if(expr->getOperand()->getType()->isTypeInt()||expr->getOperand()->getType()->isTypeIntArray())
        {
            src= new Operand(new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel()));
            new CmpInstruction(CmpInstruction::NE, src, expr->getOperand(),new Operand(new ConstantSymbolEntry(TypeSystem::intType, 0)),bb);
        }
        else if(expr->getOperand()->getType()->isTypeFloat()||expr->getOperand()->getType()->isTypeFloatArray())
        {
            src= new Operand(new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel()));
            new CmpInstruction(CmpInstruction::NE, src, expr->getOperand(),new Operand(new ConstantSymbolEntry(TypeSystem::floatType, 0)),bb);
        }
        else
        {
            src=expr->getOperand();
        }
        new XorInstruction(dst, src, bb);
    }
}

void BinaryExpr::genCode()
{
    
    BasicBlock *bb = builder->getInsertBB();
    if(bb==nullptr)
    {
        double val1=0,val2=0;
        if(expr1->getSymPtr()->isConstant())
            val1=(dynamic_cast<ConstantSymbolEntry*>(expr1->getSymPtr()))->getValue();
        else if(expr1->getSymPtr()->isVariable())
            val1=(dynamic_cast<IdentifierSymbolEntry*>(expr1->getSymPtr()))->getValue();
        else
        {
            expr1->genCode();
            val1=globalVal;
            globalVal=0;
        }
        if(expr2->getSymPtr()->isConstant())
            val2=(dynamic_cast<ConstantSymbolEntry*>(expr2->getSymPtr()))->getValue();
        else if(expr2->getSymPtr()->isVariable())
            val2=(dynamic_cast<IdentifierSymbolEntry*>(expr2->getSymPtr()))->getValue();
        else
        {
            expr2->genCode();
            val2=globalVal;
            globalVal=0;
        }
        double result=0;
        // not support the init of bool type
        // not support mod
        if(op >= ADD && op <= DIV)
        {
            switch (op)
            {
            case ADD:
                result=val1+val2;
                break;
            case SUB:
                result=val1-val2;
                break;
            case MUL:
                result=val1*val2;
                break;
            case DIV:
                result=val1/val2;
                break;
            default:
                break;      
            }            
        }
        globalVal=result;
        return;
    }
    Function *func = bb->getParent();
    if (op == AND)
    {
        BasicBlock *trueBB = new BasicBlock(func);  // if the result of lhs is true, jump to the trueBB.
        BasicBlock *falseBB=new BasicBlock(func);
        BasicBlock *endBB=new BasicBlock(func);
        expr1->genCode();
        bb=builder->getInsertBB();
        Instruction* finalInstruction=bb->rbegin();
        if((!finalInstruction->isCond()) && (!finalInstruction->isUncond()))
        {
            Operand *src1 = expr1->getOperand();
            Operand *src2 = new Operand(new ConstantSymbolEntry(src1->getType(), 0));
            SymbolEntry *new_se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
            Operand *new_src = new Operand(new_se);
            new CmpInstruction(CmpInstruction::NE,new_src,src1,src2,bb);
            expr1->trueList().push_back(new CondBrInstruction(trueBB,falseBB,new_src,bb));
            expr1->falseList().push_back(new UncondBrInstruction(endBB,falseBB));
        }

        backPatch(expr1->trueList(), trueBB);
        
        builder->setInsertBB(trueBB);               // set the insert point to the trueBB so that intructions generated by expr2 will be inserted into it.
        expr2->genCode();
        bb=builder->getInsertBB();
        finalInstruction=bb->rbegin();
        if((!finalInstruction->isCond()) && (!finalInstruction->isUncond()))
        {
            Operand *src1 = expr2->getOperand();
            Operand *src2 = new Operand(new ConstantSymbolEntry(src1->getType(), 0));
            SymbolEntry *new_se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
            Operand *new_src = new Operand(new_se);
            new CmpInstruction(CmpInstruction::NE,new_src,src1,src2,bb);
            BasicBlock *trueBranch=new BasicBlock(func);
            expr2->trueList().push_back(new CondBrInstruction(trueBranch,falseBB,new_src,bb));
            expr2->falseList().push_back(new UncondBrInstruction(endBB,falseBB));
        }
        true_list = expr2->trueList();
        false_list = merge(expr1->falseList(), expr2->falseList());
    }
    else if(op == OR)
    {
        // Todo
        BasicBlock *falseBB = new BasicBlock(func);
        BasicBlock *trueBB=new BasicBlock(func);
        BasicBlock *endBB=new BasicBlock(func);
        expr1->genCode();
        bb=builder->getInsertBB();
        Instruction* finalInstruction=bb->rbegin();
        if((!finalInstruction->isCond()) && (!finalInstruction->isUncond()))
        {
            Operand *src1 = expr1->getOperand();
            Operand *src2 = new Operand(new ConstantSymbolEntry(src1->getType(), 0));
            SymbolEntry *new_se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
            Operand *new_src = new Operand(new_se);
            new CmpInstruction(CmpInstruction::NE,new_src,src1,src2,bb);
            expr1->trueList().push_back(new CondBrInstruction(trueBB,falseBB,new_src,bb));
            expr1->falseList().push_back(new UncondBrInstruction(endBB,falseBB));
        }

        backPatch(expr1->falseList(),falseBB);
        
        builder->setInsertBB(falseBB);
        expr2->genCode();
        bb=builder->getInsertBB();
        finalInstruction=bb->rbegin();
        if((!finalInstruction->isCond()) && (!finalInstruction->isUncond()))
        {
            Operand *src1 = expr2->getOperand();
            Operand *src2 = new Operand(new ConstantSymbolEntry(src1->getType(), 0));
            SymbolEntry *new_se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
            Operand *new_src = new Operand(new_se);
            new CmpInstruction(CmpInstruction::NE,new_src,src1,src2,bb);
            BasicBlock *falseBranch=new BasicBlock(func);
            expr2->trueList().push_back(new CondBrInstruction(trueBB,falseBranch,new_src,bb));
            expr2->falseList().push_back(new UncondBrInstruction(endBB,falseBranch));
        }
        true_list=merge(expr1->trueList(),expr2->trueList());
        false_list=expr2->falseList();
    }
    else if(op >= LESS && op <= FALSEEQUAL)
    {
        // Todo
        expr1->genCode();
        expr2->genCode();
        /*Operand *src1 = expr1->getOperand();
        Operand *src2 = expr2->getOperand();*/
        Operand *src1,*src2;
        if(expr1->getOperand()->getType()->isTypeInt()&&expr2->getOperand()->getType()->isBool())
        {
            src1 = expr1->getOperand();
            SymbolEntry *tmp=new TemporarySymbolEntry(src1->getType(),SymbolTable::getLabel());
            src2 = new Operand(tmp);
            new ZextInstruction(src2, expr2->getOperand(), bb);
        }
        else if(expr1->getOperand()->getType()->isBool()&&expr2->getOperand()->getType()->isTypeInt())
        {
            src2 = expr2->getOperand();
            SymbolEntry *tmp=new TemporarySymbolEntry(src2->getType(),SymbolTable::getLabel());
            src1 = new Operand(tmp);
            new ZextInstruction(src1, expr1->getOperand(), bb);
        }
        else
        {
            src1 = expr1->getOperand();
            src2 = expr2->getOperand();
        }
        int opcode;
        switch(op)
        {
        case LESS:
            opcode = CmpInstruction::L;
            break;
        case GREATER:
            opcode = CmpInstruction::G;
            break;
        case LESSEQUAL:
            opcode = CmpInstruction::LE;
            break;
        case GREATEREQUAL:
            opcode = CmpInstruction::GE;
            break;
        case TRUEEQUAL:
            opcode = CmpInstruction::E;
            break;
        case FALSEEQUAL:
            opcode = CmpInstruction::NE;
            break;
        }
        if(expr1->getOperand()->getType()->isTypeInt()&&expr2->getOperand()->getType()->isTypeFloat())
        {
            src1=new Operand(new TemporarySymbolEntry(TypeSystem::floatType, SymbolTable::getLabel()));
            new CastInstruction(CastInstruction::ITOF,src1,expr1->getOperand(),bb);
            new CmpInstruction(opcode,dst,src1,src2,bb);
        }
        else if(expr1->getOperand()->getType()->isTypeFloat()&&expr2->getOperand()->getType()->isTypeInt())
        {
            src2=new Operand(new TemporarySymbolEntry(TypeSystem::floatType, SymbolTable::getLabel()));
            new CastInstruction(CastInstruction::ITOF,src2,expr2->getOperand(),bb);
            new CmpInstruction(opcode,dst,src1,src2,bb);
        }
        else
        {
            new CmpInstruction(opcode,dst,src1,src2,bb);
        }
        BasicBlock *trueBranch,*falseBranch,*endBranch;
        trueBranch=new BasicBlock(func);
        falseBranch=new BasicBlock(func);
        endBranch=new BasicBlock(func);
        true_list.push_back(new CondBrInstruction(trueBranch,falseBranch,dst,bb));
        false_list.push_back(new UncondBrInstruction(endBranch,falseBranch));
    }
    else if(op >= ADD && op <= MOD)
    {
        expr1->genCode();
        expr2->genCode();
        Operand *src1 = expr1->getOperand();
        Operand *src2 = expr2->getOperand();
        Type *dst_type=symbolEntry->getType();
        Type *src1_type=expr1->getOperand()->getEntry()->getType();
        Type *src2_type=expr2->getOperand()->getEntry()->getType();
        if(dst_type->isTypeFloat())
        {
            if(src1_type->isTypeInt())
            {
                src1=new Operand(new TemporarySymbolEntry(TypeSystem::floatType, SymbolTable::getLabel()));
                new CastInstruction(CastInstruction::ITOF,src1,expr1->getOperand(),bb);
            }
            if(src2_type->isTypeInt())
            {
                src2=new Operand(new TemporarySymbolEntry(TypeSystem::floatType, SymbolTable::getLabel()));
                new CastInstruction(CastInstruction::ITOF,src2,expr2->getOperand(),bb);
            }
        }
        else if(dst_type->isTypeInt())
        {
            if(src1_type->isTypeFloat())
            {
                src1=new Operand(new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel()));
                new CastInstruction(CastInstruction::FTOI,src1,expr1->getOperand(),bb);
            }
            if(src2_type->isTypeFloat())
            {
                src2=new Operand(new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel()));
                new CastInstruction(CastInstruction::FTOI,src2,expr2->getOperand(),bb);
            }            
        }        
        int opcode;
        switch (op)
        {
        case ADD:
            opcode = BinaryInstruction::ADD;
            break;
        case SUB:
            opcode = BinaryInstruction::SUB;
            break;
        case MUL:
            opcode = BinaryInstruction::MUL;
            break;
        case DIV:
            opcode = BinaryInstruction::DIV;
            break;
        case MOD:
            opcode = BinaryInstruction::MOD;
            break; 
        default:
            break;       
        }
        if(!(symbolEntry->getType()->isTypeFloat()))
        {
            new BinaryInstruction(opcode, dst, src1, src2, bb);
        }
        else
        {
            if(opcode==BinaryInstruction::MOD)
            {
                // too complex
            }
            // from int to float
            opcode+=(BinaryInstruction::FADD-BinaryInstruction::ADD);
            new BinaryInstruction(opcode, dst, src1, src2, bb);
        }
   
    }
}

void Constant::genCode()
{
    // we don't need to generate code.
}

void ArrayNode::genCode()
{
    for(auto expr:exprList)
    {
        expr->genCode();
    }
}

void Id::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    Operand *addr = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getAddr();
    dst = new Operand(new TemporarySymbolEntry(dst->getType(), SymbolTable::getLabel()));
    if(!getType()->isArray()){
        new LoadInstruction(dst, addr, bb);
    }
    else{
        //数组寻址A[i1,i2,...,ik]
        //((…((i1n2 + i2)n3 + i3)…)nk + ik) * w + base– ((…((low1n2+low2)n3 + low3)…)nk + lowk) * w
        //其中low为0，i为arrayList->exprList[i]，n为dimension
        //关键在计算(…((i1n2 + i2)n3 + i3)…)nm + im，迭代
        Operand* i1 = nullptr;
        std::vector<ExprNode*> exprList;
        if(arrayList!=nullptr){
            arrayList->genCode();
            exprList = arrayList->getExprList();
            i1 = exprList[0]->getOperand();
        }
        std::vector<int> dims;
        if(getType()->isIntArray()){
            dims = dynamic_cast<IntArrayType*>(getType())->getDims();
        }
        else if(getType()->isFloatArray()){
            dims = dynamic_cast<FloatArrayType*>(getType())->getDims();
        }
        else if(getType()->isConstIntArray()){
            dims = dynamic_cast<ConstIntArrayType*>(getType())->getDims();
        }
        else{
            dims = dynamic_cast<ConstFloatArrayType*>(getType())->getDims();
        }
        //函数参数高维空
        if(dims[0]==-1){
            TemporarySymbolEntry* se = new TemporarySymbolEntry(getType(), SymbolTable::getLabel());
            Operand* addr1 = new Operand(se);
            new LoadInstruction(addr1, addr, bb);
            addr = addr1;
        }
        if(arrayList!=nullptr){
            //计算(…((i1n2 + i2)n3 + i3)…)nm + im
            for(int i=1;i<(int)exprList.size();i++){
                Operand* n2 = new Operand(new ConstantSymbolEntry(TypeSystem::intType, dims[i]));
                TemporarySymbolEntry* se1 = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
                Operand* i1n2 = new Operand(se1);
                new BinaryInstruction(BinaryInstruction::MUL, i1n2, i1, n2, bb); //先计算i1n2
                TemporarySymbolEntry* se2 = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
                i1 = new Operand(se2);
                new BinaryInstruction(BinaryInstruction::ADD, i1, i1n2, exprList[i]->getOperand(), bb); //再计算i1=i1n2+i2
            }
            //如果需要寻址的是高一维度的，需要乘nm
            if((int)exprList.size() < (int)dims.size()){
                Operand* nm = new Operand(new ConstantSymbolEntry(TypeSystem::intType, dims[(int)exprList.size()]));
                TemporarySymbolEntry* se1 = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
                Operand* inm = new Operand(se1);
                new BinaryInstruction(BinaryInstruction::MUL, inm, i1, nm, bb);
                i1 = inm;
            }
        }
        Operand* temp = nullptr;
        if(arrayList!=nullptr){
            TemporarySymbolEntry* se1 = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
            temp = new Operand(se1);
            Operand* align = new Operand(new ConstantSymbolEntry(TypeSystem::constIntType, 4));
            new BinaryInstruction(BinaryInstruction::MUL, temp, i1, align, bb); //temp=i1*4
        }
        else{
            temp = new Operand(new ConstantSymbolEntry(TypeSystem::constIntType, 0));
        }
        TemporarySymbolEntry* se = new TemporarySymbolEntry(getType(), SymbolTable::getLabel());
        Operand* offset = new Operand(se);
        // 全局变量load
        if(dynamic_cast<IdentifierSymbolEntry*>(getSymbolEntry())->isGlobal()){
            TemporarySymbolEntry* se1 = new TemporarySymbolEntry(getType(), SymbolTable::getLabel());
            Operand* addr1 = new Operand(se1);
            new LoadInstruction(addr1, addr, bb);
            addr = addr1;
            //se->setGlobalArray();
            //dynamic_cast<TemporarySymbolEntry*>(dst->getEntry())->setGlobalArray();
        }
        if(arrayList!=nullptr && exprList.size()==dims.size()){
            new BinaryInstruction(BinaryInstruction::ADD, offset, temp, addr, bb);  //offset = temp + base
            if(dst->getType()->isTypeFloatArray())
                dst->getEntry()->setType(TypeSystem::floatType);
            new LoadInstruction(dst, offset, bb);
            
        }
        //指针
        else{
            if(dst->getType()->isIntArray()){
                dst->getEntry()->setType(new IntArrayType(*(dynamic_cast<IntArrayType*>(dst->getType()))));
                dynamic_cast<IntArrayType*>(dst->getType())->setPointer(true);
            }
            else if(dst->getType()->isConstIntArray()){
                dst->getEntry()->setType(new ConstIntArrayType(*(dynamic_cast<ConstIntArrayType*>(dst->getType()))));
                dynamic_cast<ConstIntArrayType*>(dst->getType())->setPointer(true);
            }
            else if(dst->getType()->isFloatArray()){
                dst->getEntry()->setType(new FloatArrayType(*(dynamic_cast<FloatArrayType*>(dst->getType()))));
                dynamic_cast<FloatArrayType*>(dst->getType())->setPointer(true);
            }
            else if(dst->getType()->isConstFloatArray()){
                dst->getEntry()->setType(new ConstFloatArrayType(*(dynamic_cast<ConstFloatArrayType*>(dst->getType()))));
                dynamic_cast<ConstFloatArrayType*>(dst->getType())->setPointer(true);
            }
            new BinaryInstruction(BinaryInstruction::ADD, dst, temp, addr, bb); 
        }
    }
}

void IfStmt::genCode()
{
    Function *func;
    BasicBlock *bb,*then_bb, *end_bb;

    bb = builder->getInsertBB();
    func = bb->getParent();
    then_bb = new BasicBlock(func);
    end_bb = new BasicBlock(func);

    cond->genCode();
    Instruction* finalInstruction=bb->rbegin();
    if((!finalInstruction->isCond()) && (!finalInstruction->isUncond()))
    {
        Operand *src1 = cond->getOperand();
        Operand *src2 = new Operand(new ConstantSymbolEntry(src1->getType(), 0));
        SymbolEntry *new_se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        Operand *new_src = new Operand(new_se);
        new CmpInstruction(CmpInstruction::NE, new_src, src1, src2, bb);
        true_list.push_back(new CondBrInstruction(then_bb, end_bb, new_src, bb));
    }
    backPatch(cond->trueList(), then_bb);
    backPatch(cond->falseList(), end_bb);

    builder->setInsertBB(then_bb);
    thenStmt->genCode();
    then_bb = builder->getInsertBB();
    new UncondBrInstruction(end_bb, then_bb);

    builder->setInsertBB(end_bb);
}

void IfElseStmt::genCode()
{
    // Todo
    Function *func;
    BasicBlock *then_bb,*else_bb,*end_bb,*bb;

    bb=builder->getInsertBB();
    func=bb->getParent();
    then_bb = new BasicBlock(func);
    else_bb = new BasicBlock(func);
    end_bb = new BasicBlock(func);

    cond->genCode();
    Instruction* finalInstruction=bb->rbegin();
    if((!finalInstruction->isCond()) && (!finalInstruction->isUncond()))
    {
        Operand *src1 = cond->getOperand();
        Operand *src2 = new Operand(new ConstantSymbolEntry(src1->getType(), 0));
        SymbolEntry *new_se = new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel());
        Operand *new_src = new Operand(new_se);
        new CmpInstruction(CmpInstruction::NE, new_src, src1, src2, bb);
        true_list.push_back(new CondBrInstruction(then_bb, else_bb, new_src, bb));
    }
    backPatch(cond->trueList(), then_bb);
    backPatch(cond->falseList(), else_bb);

    builder->setInsertBB(then_bb);
    thenStmt->genCode();
    then_bb = builder->getInsertBB();
    new UncondBrInstruction(end_bb, then_bb);

    builder->setInsertBB(else_bb);
    elseStmt->genCode();
    else_bb = builder->getInsertBB();
    new UncondBrInstruction(end_bb, else_bb);

    builder->setInsertBB(end_bb);
}    

void WhileStmt::genCode()
{
    nestWhile.push(this);
    BasicBlock *cond_bb,*stmt_bb,*end_bb,*bb;
    bb=builder->getInsertBB();
    Function *func=bb->getParent();
    cond_bb=new BasicBlock(func);
    stmt_bb=new BasicBlock(func);
    end_bb=new BasicBlock(func);
    //used for nested while
    this->cond_bb=cond_bb;
    this->stmt_bb=stmt_bb;
    this->end_bb=end_bb;

    new UncondBrInstruction(cond_bb,bb);
    builder->setInsertBB(cond_bb);
    cond->genCode();
    backPatch(cond->trueList(),stmt_bb);
    backPatch(cond->falseList(),end_bb);

    builder->setInsertBB(stmt_bb);
    whileStmt->genCode();
    stmt_bb=builder->getInsertBB();
    new UncondBrInstruction(cond_bb,stmt_bb);

    builder->setInsertBB(end_bb);
    nestWhile.pop();
}

void BreakStmt::genCode()
{
    BasicBlock *bb=builder->getInsertBB();
    Function *func=bb->getParent();
    WhileStmt *whileStmt=nestWhile.top();
    BasicBlock *end_bb=whileStmt->getEndBB();
    new UncondBrInstruction(end_bb,bb);
    BasicBlock *endwhile_bb=new BasicBlock(func);
    builder->setInsertBB(endwhile_bb);
}

void ContinueStmt::genCode()
{
    BasicBlock *bb=builder->getInsertBB();
    Function *func=bb->getParent();
    WhileStmt *whileStmt=nestWhile.top();
    BasicBlock *cond_bb=whileStmt->getCondBB();
    new UncondBrInstruction(cond_bb,bb);
    BasicBlock *nextwhile_bb=new BasicBlock(func);
    builder->setInsertBB(nextwhile_bb);
}

void CompoundStmt::genCode()
{
    // Todo
    if(stmt!=nullptr)
        stmt->genCode();
}

void SeqNode::genCode()
{
    // Todo
    stmt1->genCode();
    stmt2->genCode();
}

void ExpStmt::genCode()
{
    expr->genCode();
}

void ArrayDeclNode::genCode()
{
    //Todo
}

void VarDeclNode::genCode()
{
    IdentifierSymbolEntry *se = dynamic_cast<IdentifierSymbolEntry *>(id->getSymPtr());
    if(se->isGlobal())
    {
        Operand *addr;
        SymbolEntry *addr_se;
        addr_se = new IdentifierSymbolEntry(*se);
        addr_se->setType(new PointerType(se->getType()));
        addr = new Operand(addr_se);
        se->setAddr(addr);
        if(expr!=nullptr && !se->getType()->isArray())
        {
            if(dynamic_cast<ExprNode*>(expr)->getSymPtr()->isConstant())
            {
                double v=((ConstantSymbolEntry*)(dynamic_cast<ExprNode*>(expr))->getSymPtr())->getValue();
                se->setValue(v);
            }
            else
            {
                // can only handle 2*3 -2 and so on
                expr->genCode();
                se->setValue(globalVal);
                globalVal=0;
            }
        }
        builder->getUnit()->insertId(se);
        /*if(expr!=nullptr)
        {
            BasicBlock *bb=builder->getInsertBB();
            expr->genCode();
            Operand *src=dynamic_cast<ExprNode*>(expr)->getOperand();
            new StoreInstruction(addr,src,bb);
        }*/
    }
    else if(se->isLocal())
    {
        Function *func = builder->getInsertBB()->getParent();
        BasicBlock *entry = func->getEntry();
        Instruction *alloca;
        Operand *addr;
        SymbolEntry *addr_se;
        Type *type;
        type = new PointerType(se->getType());
        addr_se = new TemporarySymbolEntry(type, SymbolTable::getLabel());
        addr = new Operand(addr_se);
        alloca = new AllocaInstruction(addr, se);                   // allocate space for local id in function stack.
        entry->insertFront(alloca);                                 // allocate instructions should be inserted into the begin of the entry block.
        se->setAddr(addr);                                          // set the addr operand in symbol entry so that we can use it in subsequent code generation.
        if(expr!=nullptr)
        {
            if(!se->getType()->isArray()){
                BasicBlock *bb=builder->getInsertBB();
                expr->genCode();
                Operand *src=dynamic_cast<ExprNode*>(expr)->getOperand();
                new StoreInstruction(addr,src,bb);
            }
            else{
                std::vector<int> indexes;
                int size;
                if(se->getType()->isIntArray()){
                    indexes = dynamic_cast<IntArrayType*>(se->getType())->getDims();
                    size = dynamic_cast<IntArrayType*>(se->getType())->getSize() / dynamic_cast<IntType*>(TypeSystem::intType)->getSize();
                }
                else if(se->getType()->isFloatArray()){
                    indexes = dynamic_cast<FloatArrayType*>(se->getType())->getDims();
                    size = dynamic_cast<FloatArrayType*>(se->getType())->getSize() / dynamic_cast<FloatType*>(TypeSystem::floatType)->getSize();
                }
                else if(se->getType()->isConstIntArray()){
                    indexes = dynamic_cast<ConstIntArrayType*>(se->getType())->getDims();
                    size = dynamic_cast<ConstIntArrayType*>(se->getType())->getSize() / dynamic_cast<IntType*>(TypeSystem::intType)->getSize();
                }
                else if(se->getType()->isConstFloatArray()){
                    indexes = dynamic_cast<ConstFloatArrayType*>(se->getType())->getDims();
                    size = dynamic_cast<ConstFloatArrayType*>(se->getType())->getSize() / dynamic_cast<FloatType*>(TypeSystem::floatType)->getSize();
                }
                std::vector<Operand *> offset;
                for (int i = 0; i < (int)indexes.size(); i++)
                {
                    offset.push_back(new Operand(new ConstantSymbolEntry(TypeSystem::intType, 0)));
                }
                BasicBlock *bb=builder->getInsertBB();
                Operand *zeroReg = new Operand(new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel()));
                Operand *zero = new Operand(new ConstantSymbolEntry(TypeSystem::intType, 0));
                new BinaryInstruction(BinaryInstruction::ADD, zeroReg, zero, zero, bb);
                Operand *step = new Operand(new ConstantSymbolEntry(TypeSystem::intType, 4));
                Operand *eleptr = new Operand(new TemporarySymbolEntry(new PointerType(TypeSystem::intType), SymbolTable::getLabel()));
                new GepInstruction(eleptr, se->getAddr(), offset, bb);
                if(dynamic_cast<ArrayDeclNode*>(expr)->getLeafNode()!=nullptr){
                    dynamic_cast<ArrayDeclNode*>(expr)->getLeafNode()->genCode();
                    new StoreInstruction(eleptr, dynamic_cast<ArrayDeclNode*>(expr)->getLeafNode()->getOperand(), bb);
                    new BinaryInstruction(BinaryInstruction::ADD, eleptr, eleptr, step, bb);
                }
                for(int i=0;i<size;i++){
                    if(((dynamic_cast<ArrayDeclNode*>(expr)->getArrayDeclList())[i])->getLeafNode()!=nullptr){
                        ((dynamic_cast<ArrayDeclNode*>(expr)->getArrayDeclList())[i])->getLeafNode()->genCode();
                        new StoreInstruction(eleptr, ((dynamic_cast<ArrayDeclNode*>(expr)->getArrayDeclList())[i])->getLeafNode()->getOperand(), bb);
                        new BinaryInstruction(BinaryInstruction::ADD, eleptr, eleptr, step, bb);
                    }
                }
            }
        }
    }
}

void VarDeclSeqNode::insertVarDecl(VarDeclNode *varDecl)
{
    varDeclList.push_back(varDecl);
}

void VarDeclSeqNode::genCode()
{
    for(auto it=varDeclList.begin();it!=varDeclList.end();++it)
    {
        (*it)->genCode();
    }
}

void VarDeclStmt::genCode()
{
    varDeclSeqNode->genCode();
}

void ConstDeclNode::genCode()
{
    IdentifierSymbolEntry *se = dynamic_cast<IdentifierSymbolEntry *>(id->getSymPtr());
    if(se->isGlobal())
    {
        Operand *addr;
        SymbolEntry *addr_se;
        addr_se = new IdentifierSymbolEntry(*se);
        addr_se->setType(new PointerType(se->getType()));
        addr = new Operand(addr_se);
        se->setAddr(addr);
        builder->getUnit()->insertId(se);
        BasicBlock *bb=builder->getInsertBB();
        expr->genCode();
        if(expr->getSymPtr()->isConstant())
        {
            double v=((ConstantSymbolEntry*)expr->getSymPtr())->getValue();
            se->setValue(v);
        }
        else
        {
            se->setValue(globalVal);
        }
        globalVal=0;
        Operand *src=dynamic_cast<ExprNode*>(expr)->getOperand();
        new StoreInstruction(addr,src,bb);
    }
    else if(se->isLocal())
    {
        Function *func = builder->getInsertBB()->getParent();
        BasicBlock *entry = func->getEntry();
        Instruction *alloca;
        Operand *addr;
        SymbolEntry *addr_se;
        Type *type;
        type = new PointerType(se->getType());
        addr_se = new TemporarySymbolEntry(type, SymbolTable::getLabel());
        addr = new Operand(addr_se);
        alloca = new AllocaInstruction(addr, se);                   // allocate space for local id in function stack.
        entry->insertFront(alloca);                                 // allocate instructions should be inserted into the begin of the entry block.
        se->setAddr(addr);                                          // set the addr operand in symbol entry so that we can use it in subsequent code generation.

        BasicBlock *bb=builder->getInsertBB();
        expr->genCode();
        Operand *src=dynamic_cast<ExprNode*>(expr)->getOperand();
        new StoreInstruction(addr,src,bb);
    }

}

void ConstDeclSeqNode::insertConstDecl(ConstDeclNode *constDecl)
{
    constDeclList.push_back(constDecl);
}

void ConstDeclSeqNode::genCode()
{
    for(auto it=constDeclList.begin();it!=constDeclList.end();++it)
    {
        (*it)->genCode();
    }
}

void ConstDeclStmt::genCode()
{
    constDeclSeqNode->genCode();
}

void ReturnStmt::genCode()
{
    // Todo
    BasicBlock *bb = builder->getInsertBB();
    if(retValue==nullptr)
    {
        //return nullptr
        new RetInstruction(nullptr,bb);
        return;
    }
    retValue->genCode();
    new RetInstruction(retValue->getOperand(),bb);
}

void AssignStmt::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    expr->genCode();
    Operand *addr = dynamic_cast<IdentifierSymbolEntry*>(lval->getSymPtr())->getAddr();
    Operand *src = expr->getOperand();
    Type *dstType=dynamic_cast<PointerType*>(addr->getType())->getType();
    if(dstType->isTypeInt()&&src->getType()->isTypeFloat())
    {
        src=new Operand(new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel()));
        new CastInstruction(CastInstruction::FTOI,src,expr->getOperand(),bb);
        
    }
    else if(dstType->isTypeFloat()&&src->getType()->isTypeInt())
    {
        src=new Operand(new TemporarySymbolEntry(TypeSystem::floatType, SymbolTable::getLabel()));
        new CastInstruction(CastInstruction::ITOF,src,expr->getOperand(),bb);
    }
    /***
     * We haven't implemented array yet, the lval can only be ID. So we just store the result of the `expr` to the addr of the id.
     * If you want to implement array, you have to caculate the address first and then store the result into it.
     */
    //printf("addr:%s\n",addr->toStr().c_str());
    //printf("src:%s\n",src->toStr().c_str());
    //new StoreInstruction(addr, src, bb);
    if(!lval->getType()->isArray()){
        new StoreInstruction(addr, src, bb);
    }
    else{
        //caculate the address
        //数组寻址A[i1,i2,...,ik]
        //((…((i1n2 + i2)n3 + i3)…)nk + ik) * w + base– ((…((low1n2+low2)n3 + low3)…)nk + lowk) * w
        //其中low为0，i为arrayList->exprList[i]，n为dimension
        //关键在计算(…((i1n2 + i2)n3 + i3)…)nm + im，迭代
        ArrayNode* arrayList = dynamic_cast<Id*>(lval)->getArrayList();
        arrayList->genCode();
        Operand* i1 = nullptr;
        std::vector<ExprNode*> exprList;
        exprList = arrayList->getExprList();
        i1 = exprList[0]->getOperand();
        std::vector<int> dims;
        if(lval->getType()->isIntArray()){
            dims = dynamic_cast<IntArrayType*>(lval->getType())->getDims();
        }
        else if(lval->getType()->isFloatArray()){
            dims = dynamic_cast<FloatArrayType*>(lval->getType())->getDims();
        }
        //函数参数高维空
        if(dims[0]==-1){
            TemporarySymbolEntry* se = new TemporarySymbolEntry(lval->getType(), SymbolTable::getLabel());
            Operand* addr1 = new Operand(se);
            new LoadInstruction(addr1, addr, bb);
            addr = addr1;
        }
        //计算(…((i1n2 + i2)n3 + i3)…)nm + im
        for(int i=1;i<(int)exprList.size();i++){
            Operand* n2 = new Operand(new ConstantSymbolEntry(TypeSystem::intType, dims[i]));
            TemporarySymbolEntry* se1 = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
            Operand* i1n2 = new Operand(se1);
            new BinaryInstruction(BinaryInstruction::MUL, i1n2, i1, n2, bb); //先计算i1n2
            TemporarySymbolEntry* se2 = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
            i1 = new Operand(se2);
            new BinaryInstruction(BinaryInstruction::ADD, i1, i1n2, exprList[i]->getOperand(), bb); //再计算i1=i1n2+i2
        }
        Operand* temp = nullptr;
        TemporarySymbolEntry* se1 = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
        temp = new Operand(se1);
        Operand* align = new Operand(new ConstantSymbolEntry(TypeSystem::constIntType, 4));
        new BinaryInstruction(BinaryInstruction::MUL, temp, i1, align, bb); //temp=i1*4
        TemporarySymbolEntry* se = new TemporarySymbolEntry(lval->getType(), SymbolTable::getLabel());
        Operand* offset = new Operand(se);
        // 全局变量load
        if(dynamic_cast<IdentifierSymbolEntry*>(dynamic_cast<Id*>(lval)->getSymbolEntry())->isGlobal()){
            TemporarySymbolEntry* se1 = new TemporarySymbolEntry(lval->getType(), SymbolTable::getLabel());
            Operand* addr1 = new Operand(se1);
            new LoadInstruction(addr1, addr, bb);
            addr = addr1;
            //se->setGlobalArray();
            //dynamic_cast<TemporarySymbolEntry*>(dst->getEntry())->setGlobalArray();
        }
        new BinaryInstruction(BinaryInstruction::ADD, offset, temp, addr, bb);  //offset = temp + base
        new StoreInstruction(offset, src, bb);
    }
}

void CallParamSeqNode::genCode()
{
    for(auto param=paramList.begin();param!=paramList.end();++param)
    {
        (*param)->genCode();
        operandList.push_back((*param)->getOperand());
    }
}

void CallFunc::genCode()
{
    if(((IdentifierSymbolEntry*)funcSe)->isLibFunc())
        builder->getUnit()->insertId(funcSe);
    BasicBlock *bb=builder->getInsertBB();
    callParamSeqNode->genCode();
    new CallInstruction(dst, funcSe, callParamSeqNode->getParams(), bb);
}

void NullStmt::genCode()
{
    // do
}

void Ast::typeCheck()
{
    if(root != nullptr)
        root->typeCheck();
}

void FuncParamNode::typeCheck()
{
    if(id!=nullptr){
        id->typeCheck();
    }
}

void FuncParamSeqNode::typeCheck()
{
    for(int i = 0;i<(int)paramList.size();i++){
        paramList[i]->typeCheck();
    }
}

void FunctionDef::typeCheck()
{
    // Todo
    if(paramList!=nullptr){
        paramList->typeCheck();
    }
    returnTypeDef=((FunctionType*)se->getType())->getRetType();
    returnType=nullptr;
    stmt->typeCheck();
    if(returnType==nullptr){ //表示不含有return语句
        if(!returnTypeDef->isVoid()){
            fprintf(stderr, "FunctionDef TypeCheck: return type %s not found ", returnTypeDef->toStr().c_str());
            exit(EXIT_FAILURE);
        }
        else{
            //Todo
        }
    }
    else{ //表示含有return语句
        //函数类型为int，返回非int
        if(returnTypeDef->isTypeInt() && !returnType->isTypeInt()){
            fprintf(stderr, "FunctionDef TypeCheck: type %s and %s mismatch ",
                returnTypeDef->toStr().c_str(), returnType->toStr().c_str());
            exit(EXIT_FAILURE);
        }
        //函数类型为float，返回非float或int
        if(returnTypeDef->isTypeFloat() && !returnType->isValue()){
            fprintf(stderr, "FunctionDef TypeCheck: type %s and %s mismatch ",
                returnTypeDef->toStr().c_str(), returnType->toStr().c_str());
            exit(EXIT_FAILURE);
        }
        //函数类型为void，返回非void
        if(returnTypeDef->isVoid() && !returnType->isVoid()){
            fprintf(stderr, "FunctionDef TypeCheck: type %s and %s mismatch ",
                returnTypeDef->toStr().c_str(), returnType->toStr().c_str());
            exit(EXIT_FAILURE);
        }
        //函数类型为int数组，返回非int数组
        if(returnTypeDef->isTypeIntArray() && !returnType->isTypeIntArray()){
            fprintf(stderr, "FunctionDef TypeCheck: type %s and %s mismatch ",
                returnTypeDef->toStr().c_str(), returnType->toStr().c_str());
            exit(EXIT_FAILURE);
        }
        //函数类型为float数组，返回非float数组
        if(returnTypeDef->isTypeFloatArray() && !returnType->isTypeFloatArray()){
            fprintf(stderr, "FunctionDef TypeCheck: type %s and %s mismatch ",
                returnTypeDef->toStr().c_str(), returnType->toStr().c_str());
            exit(EXIT_FAILURE);
        }
    }
    returnTypeDef=nullptr;
}

void BinaryExpr::typeCheck()
{
    double val1=0,val2=0;
    if(expr1->getSymPtr()->isConstant())
        val1=(dynamic_cast<ConstantSymbolEntry*>(expr1->getSymPtr()))->getValue();
    else if(expr1->getSymPtr()->isVariable())
        val1=(dynamic_cast<IdentifierSymbolEntry*>(expr1->getSymPtr()))->getValue();
    else
    {
        expr1->typeCheck();
        val1=constVal;
        constVal=0;
    }
    //printf("val1 = %lf\n",val1);
    if(expr2->getSymPtr()->isConstant())
        val2=(dynamic_cast<ConstantSymbolEntry*>(expr2->getSymPtr()))->getValue();
    else if(expr2->getSymPtr()->isVariable())
        val2=(dynamic_cast<IdentifierSymbolEntry*>(expr2->getSymPtr()))->getValue();
    else
    {
        expr2->typeCheck();
        val2=constVal;
        constVal=0;
    }
    //printf("val2 = %lf\n", val2);
    double result=0;
    if(op >= ADD && op <= DIV)
    {
        switch (op)
        {
        case ADD:
            result=val1+val2;
            break;
        case SUB:
            result=val1-val2;
            break;
        case MUL:
            result=val1*val2;
            break;
        case DIV:
            result=val1/val2;
            break;
        default:
            result=0;
            break;      
        }            
    }
    constVal=result;
    Type* type1=expr1->getSymPtr()->getType();
    Type* type2=expr2->getSymPtr()->getType();
    if(type1->isArray()||type2->isArray()){
        return;
    }
    if(!(type1->isValue()||type1->isBool())){
        fprintf(stderr, "BinaryExpr TypeCheck: type %s wrong ", type1->toStr().c_str());
        exit(EXIT_FAILURE);
    }
    if(!(type2->isValue()||type2->isBool())){
        fprintf(stderr, "BinaryExpr TypeCheck: type %s wrong ", type2->toStr().c_str());
        exit(EXIT_FAILURE);
    }
    if(op==AND || op==OR){
        if((type1->isTypeInt()||type1->isBool()||type1->isTypeFloat()) && (type2->isTypeInt()||type2->isBool()||type2->isFloat())){
            //symbolEntry->setType(TypeSystem::intType);
            if(isCond==0){
                symbolEntry->setType(TypeSystem::intType);
            }
            else{
                symbolEntry->setType(TypeSystem::boolType);
            }
        }
        else{
            fprintf(stderr, "BinaryExpr TypeCheck: type wrong ");
            exit(EXIT_FAILURE);
        }
    }
    else if(op>=LESS && op<=FALSEEQUAL){
        if(isCond==0){
            symbolEntry->setType(TypeSystem::intType);
        }
        else{
            symbolEntry->setType(TypeSystem::boolType);
        }
    }
    else{
        // Int类型
        if((type1->isTypeInt()||type1->isBool())&&(type2->isTypeInt()||type2->isBool())){
            symbolEntry->setType(TypeSystem::intType);
        }
        // Float类型
        else if(type1->isTypeFloat()){
            symbolEntry->setType(TypeSystem::floatType);
        }
        else if(type2->isTypeFloat()){
            symbolEntry->setType(TypeSystem::floatType);
        }
        // Error
        else{
            fprintf(stderr, "BinaryExpr TypeCheck: type %s and %s mismatch in line xx",
                type1->toStr().c_str(), type2->toStr().c_str());
            exit(EXIT_FAILURE);
        }
    }
}

void UnaryExpr::typeCheck()
{
    // Todo
    double val=0;
    if(expr->getSymPtr()->isConstant())
        val=(dynamic_cast<ConstantSymbolEntry*>(expr->getSymPtr()))->getValue();
    else if(expr->getSymPtr()->isVariable())
        val=(dynamic_cast<IdentifierSymbolEntry*>(expr->getSymPtr()))->getValue();
    else
    {
        expr->typeCheck();
        val=constVal;
        constVal=0;
    }
    double result=0;
    switch(op)
    {
    case UPLUS:
        result=val;
        break;
    case UMINUS:
        result=-val;
        break;
    default:
        result=0;
        break;
    }
    constVal=result;
    Type* type=expr->getSymPtr()->getType();
    
    if(!(type->isValue()||type->isBool()||type->isTypeIntArray()||type->isTypeFloatArray())){
        fprintf(stderr, "UnaryExpr TypeCheck: type %s wrong ", type->toStr().c_str());
        exit(EXIT_FAILURE);
    }
    if(op==NOT){
        if(type->isTypeInt()||type->isBool()||type->isTypeIntArray()||type->isTypeFloat()||type->isTypeFloatArray()){
            if(isCond==0){
                symbolEntry->setType(TypeSystem::intType);
            }
            else{
                symbolEntry->setType(TypeSystem::boolType);
            }
        }
        else{
            fprintf(stderr, "UnaryExpr TypeCheck: type wrong ");
            exit(EXIT_FAILURE);
        }
    }
    else{
        // Int类型
        if(type->isTypeInt()||type->isBool()){
            if(isCond==0){
                symbolEntry->setType(TypeSystem::intType);
            }
            else{
                symbolEntry->setType(TypeSystem::boolType);
            }
        }
        // Float类型
        else if((type->isTypeFloat()||type->isTypeFloatArray())/*&&(isCond==0)*/){
            symbolEntry->setType(TypeSystem::floatType);
        }
        else{
            fprintf(stderr, "UnaryExpr TypeCheck: type wrong ");
            exit(EXIT_FAILURE);
        }
    }
}

void Constant::typeCheck()
{
    // Todo
}

static std::vector<int> exprValue;

void ArrayNode::typeCheck()
{
    //Todo
    for(auto expr:exprList){
        expr->typeCheck();
        if(expr->getSymPtr()->isConstant())
        {
            exprValue.push_back((int)((ConstantSymbolEntry*)(expr->getSymPtr()))->getValue());
        }
        else
        {
            //printf("constVal = %lf\n",constVal);
            exprValue.push_back((int)constVal);
        }
        constVal=0;
    }
}

void Id::typeCheck()
{
    // Todo
    if(getSymbolEntry()->getType()->isConstInt()){
        constVal = dynamic_cast<IdentifierSymbolEntry*>(getSymbolEntry())->getValue();
    }
    // 数组相关
    // 初始化维度信息
    if(isArray() && arrayList!=nullptr){
        exprValue.clear();
        arrayList->typeCheck();
        if((getType()->isIntArray() && dynamic_cast<IntArrayType*>(getType())->getDims().empty()) ||
            (getType()->isConstIntArray() && dynamic_cast<ConstIntArrayType*>(getType())->getDims().empty()) ||
            (getType()->isFloatArray() && dynamic_cast<FloatArrayType*>(getType())->getDims().empty()) ||
            (getType()->isConstFloatArray() && dynamic_cast<ConstFloatArrayType*>(getType())->getDims().empty())){
            if(!arrayList->getExprList().empty()){
                std::vector<ExprNode*> exprList=arrayList->getExprList();
                int i=0;
                for(auto expr:exprList){
                    // 既不是常数，也不是int
                    if(!(expr->getSymPtr()->isConstant() || expr->getSymPtr()->getType()->isTypeInt() || expr->getSymPtr()->getType()->isTypeIntArray())){
                        fprintf(stderr, "Array dimensions type wrong! \n");
                        exit(EXIT_FAILURE);
                    }

                    if(getType()->isIntArray()){
                        dynamic_cast<IntArrayType*>(getType())->addDim(exprValue[i]);
                        //printf("%d\n", exprValue[i]);
                    }
                    else if(getType()->isConstIntArray()) {
                        dynamic_cast<ConstIntArrayType*>(getType())->addDim(exprValue[i]);
                    }   
                    else if(getType()->isFloatArray()){
                        dynamic_cast<FloatArrayType*>(getType())->addDim(exprValue[i]);
                    }
                    else {
                        dynamic_cast<ConstFloatArrayType*>(getType())->addDim(exprValue[i]);
                    }
                    i++;
                    /*if(expr->getSymPtr()->isConstant()){
                        if(getType()->isIntArray()){
                            dynamic_cast<IntArrayType*>(getType())->addDim((int)((ConstantSymbolEntry*)(expr->getSymPtr()))->getValue());
                        }
                        else if(getType()->isConstIntArray()) {
                            dynamic_cast<ConstIntArrayType*>(getType())->addDim((int)((ConstantSymbolEntry*)(expr->getSymPtr()))->getValue());
                        }   
                        else if(getType()->isFloatArray()){
                            dynamic_cast<FloatArrayType*>(getType())->addDim((int)((ConstantSymbolEntry*)(expr->getSymPtr()))->getValue());
                        }
                        else {
                            dynamic_cast<ConstFloatArrayType*>(getType())->addDim((int)((ConstantSymbolEntry*)(expr->getSymPtr()))->getValue());
                        }
                    }
                    else{
                        if(getType()->isIntArray()){
                            dynamic_cast<IntArrayType*>(getType())->addDim((int)((IdentifierSymbolEntry*)(expr->getSymPtr()))->getValue());
                            //printf("%d", (int)((IdentifierSymbolEntry*)(expr->getSymPtr()))->getValue(), "\n");
                        }
                        else if(getType()->isConstIntArray()) {
                            dynamic_cast<ConstIntArrayType*>(getType())->addDim((int)((IdentifierSymbolEntry*)(expr->getSymPtr()))->getValue());
                        }  
                        else if(getType()->isFloatArray()){
                            dynamic_cast<FloatArrayType*>(getType())->addDim((int)((IdentifierSymbolEntry*)(expr->getSymPtr()))->getValue());
                        }
                        else {
                            dynamic_cast<ConstFloatArrayType*>(getType())->addDim((int)((IdentifierSymbolEntry*)(expr->getSymPtr()))->getValue());
                        }
                    }*/
                }
            }
        }
        
    }
}

void IfStmt::typeCheck()
{
    // Todo
    isCond=1;
    cond->typeCheck();
    isCond=0;
    if(thenStmt!=nullptr){
        thenStmt->typeCheck();
    }
    Type* type1=cond->getSymPtr()->getType();
    if(!type1->isBool()){
        if(type1->isTypeInt()||type1->isTypeIntArray()||type1->isTypeFloat()||type1->isTypeFloatArray()){
            SymbolEntry *se = new ConstantSymbolEntry(type1, 1);
            Constant *trueExp = new Constant(se);
            cond = new BinaryExpr(new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel()), BinaryExpr::AND, cond, trueExp);
        }
        else{
            fprintf(stderr, "IfStmt typeCheck: type %s wrong ", type1->toStr().c_str());
            exit(EXIT_FAILURE);
        }
    }
}

void IfElseStmt::typeCheck()
{
    // Todo
    isCond=1;
    cond->typeCheck();
    isCond=0;
    if(thenStmt!=nullptr){
        thenStmt->typeCheck();
    }
    if(elseStmt!=nullptr){
        elseStmt->typeCheck();
    }
    Type* type1=cond->getSymPtr()->getType();
    if(!type1->isBool()){
        if(type1->isTypeInt()||type1->isTypeIntArray()){
            SymbolEntry *se = new ConstantSymbolEntry(type1, 1);
            Constant *trueExp = new Constant(se);
            cond = new BinaryExpr(new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel()), BinaryExpr::AND, cond, trueExp);
        }
        else{
            fprintf(stderr, "IfElseStmt typeCheck: type %s wrong ", type1->toStr().c_str());
            exit(EXIT_FAILURE);
        }
    }
}

void WhileStmt::typeCheck()
{
    //Todo
    isCond=1;
    cond->typeCheck();
    isCond=0;
    Type* type1=cond->getSymPtr()->getType();
    if(!type1->isBool()){
        if(type1->isTypeInt()||type1->isTypeIntArray()){
            SymbolEntry *se = new ConstantSymbolEntry(type1, 1);
            Constant *trueExp = new Constant(se);
            cond = new BinaryExpr(new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel()), BinaryExpr::AND, cond, trueExp);
        }
        else{
            fprintf(stderr, "WhileStmt typeCheck: type %s wrong ", type1->toStr().c_str());
            exit(EXIT_FAILURE);
        }
    }
    count++;
    if(whileStmt!=nullptr){
        whileStmt->typeCheck();
    }
    count--;
}

void BreakStmt::typeCheck()
{
    //Todo
    if(count==0){
        fprintf(stderr, "Break stmt not in While stmt ");
        exit(EXIT_FAILURE);
    }
}

void ContinueStmt::typeCheck()
{
    //Todo
    if(count==0){
        fprintf(stderr, "Continue stmt not in While stmt ");
        exit(EXIT_FAILURE);
    }
}

void CompoundStmt::typeCheck()
{
    // Todo
    if(stmt!=nullptr){
        stmt->typeCheck();
    }
}

void SeqNode::typeCheck()
{
    // Todo
    if(stmt1!=nullptr){
        stmt1->typeCheck();
    }
    if(stmt2!=nullptr){
        stmt2->typeCheck();
    }
}

static std::vector<ExprNode*> initExpr;
static int currentDim;
static std::vector<int> dims;
static Type* type;

void ArrayDeclNode::typeCheck()
{
    //Todo
    currentDim++;
    bool flag = true;
    if(this->leafNode != nullptr){
        this->leafNode->typeCheck();
        initExpr.push_back(this->leafNode);
        //printf(this->leafNode->getSymPtr()->toStr().c_str(),"\n");
        flag = false;
    }
    int size = 0;
    for(auto & child : arrayDeclList){
        child->typeCheck();
        size++;
    }
    if(flag){
        int capacity = 1;
        for(int i = currentDim; i < int(dims.size()); i++) {
            //printf("%d\n", dims[i]);
            capacity *= dims[i];
        }
        //printf("%d\n", capacity);
        //if(capacity==0){
            //capacity=1;
        //}
        while(size++ < dims[currentDim] || initExpr.size() % capacity != 0) {
            if(type->isTypeInt()) {
                initExpr.push_back(new Constant(new ConstantSymbolEntry(TypeSystem::constIntType, 0)));
            }
            else if(type->isTypeFloat()){
                initExpr.push_back(new Constant(new ConstantSymbolEntry(TypeSystem::constFloatType, 0)));
            }
        }
    }
    currentDim--;

}

void VarDeclNode::typeCheck()
{
    // 如果是数组，需要将数组的初始化列表展开成均为叶节点
    // Todo
    id->typeCheck();
    if(expr!=nullptr){
        if(!id->getType()->isArray()){
            expr->typeCheck();
        }
        else{
            initExpr.clear();
            dims.clear();
            currentDim = -1;
            if(id->getType()->isIntArray()){
                dims = dynamic_cast<IntArrayType*>(id->getType())->getDims();
                type = TypeSystem::intType;
            }
            else if(id->getType()->isFloatArray()){
                dims = dynamic_cast<FloatArrayType*>(id->getType())->getDims();
                type = TypeSystem::floatType;
            }
            else if(id->getType()->isConstIntArray()){
                dims = dynamic_cast<ConstIntArrayType*>(id->getType())->getDims();
                type = TypeSystem::constIntType;
            }
            else if(id->getType()->isConstFloatArray()){
                dims = dynamic_cast<ConstFloatArrayType*>(id->getType())->getDims();
                type = TypeSystem::constFloatType;
            }
            expr->typeCheck(); //此expr为数组初始化列表
            //static std::vector<ExprNode*> initVals = initExpr;
            this->expr = new ArrayDeclNode();
            for(auto & child : initExpr){
                ArrayDeclNode* newNode = new ArrayDeclNode();
                newNode->setLeafNode(child);
                //printf(child->getSymPtr()->toStr().c_str());
                dynamic_cast<ArrayDeclNode*>(this->expr)->addDecl(newNode);
            }
        }
    }

    if(dynamic_cast<IdentifierSymbolEntry*>(id->getSymPtr())->isGlobal()) {
        // 对于初始化值不为空的，要进行初始化赋值
        if(expr != nullptr) {
            IdentifierSymbolEntry* se = (IdentifierSymbolEntry*)id->getSymPtr();
            if(se->getType()->isArray()) {
                if(se->arrayValue.empty()) {
                    for(auto val : dynamic_cast<ArrayDeclNode*>(expr)->getArrayDeclList()) {
                        ExprNode* leafNode = val->getLeafNode();
                        double value= ((ConstantSymbolEntry*)((ExprNode*)leafNode)->getSymPtr())->getValue();
                        se->arrayValue.push_back(value);
                    }
                }
            }
        }
    }
}

void VarDeclSeqNode::typeCheck()
{
    for(int i = 0;i<(int)varDeclList.size();i++){
        varDeclList[i]->typeCheck();
    }
}

void VarDeclStmt::typeCheck()
{
    varDeclSeqNode->typeCheck();
}

void ConstDeclNode::typeCheck()
{
    // Todo
    id->typeCheck();
    if(expr==nullptr){
        fprintf(stderr, "Const Declare Stmt do not have initial ");
        exit(EXIT_FAILURE);
    }
    IdentifierSymbolEntry *se = dynamic_cast<IdentifierSymbolEntry *>(id->getSymPtr());
    expr->typeCheck();
    if(expr->getSymPtr()->isConstant())
    {
        double v=((ConstantSymbolEntry*)expr->getSymPtr())->getValue();
        se->setValue(v);
        //printf("%lf\n",v);
    }
    else
    {
        se->setValue(constVal);
        //printf("%lf\n",constVal);
    }
    constVal=0;
    // assign value
    //IdentifierSymbolEntry* se = (IdentifierSymbolEntry*)id->getSymPtr();
    //dynamic_cast<IdentifierSymbolEntry*>(id->getSymPtr())->setValue(((ConstantSymbolEntry*)(expr)->getSymPtr())->getValue());
    //printf("%f", se->getValue(),"\n");
}

void ConstDeclSeqNode::typeCheck()
{
    for(int i = 0;i<(int)constDeclList.size();i++){
        constDeclList[i]->typeCheck();
    }
}

void ConstDeclStmt::typeCheck()
{
    constDeclSeqNode->typeCheck();
}

void ReturnStmt::typeCheck()
{
    // Todo
    if(returnTypeDef==nullptr){
        fprintf(stderr, "Return stmt not in functions ");
        exit(EXIT_FAILURE);
    }
    if(retValue==nullptr){
        returnType=new VoidType();
    }
    else{
        retValue->typeCheck();
        returnType=retValue->getSymPtr()->getType();
        if(retValue->getSymPtr()->getType()->isTypeIntArray()){
            returnType=TypeSystem::intType;
        }
        
    }
}

void AssignStmt::typeCheck()
{
    // Todo
    lval->typeCheck();
    expr->typeCheck();
    Type* type1=lval->getSymPtr()->getType();
    Type* type2=expr->getSymPtr()->getType();
    if(type1->isIntArray()){
        type1=TypeSystem::intType;
    }
    else if(type1->isFloatArray()){
        type1=TypeSystem::floatType;
    }
    if(type2->isIntArray()){
        type2=TypeSystem::intType;
    }
    else if(type2->isFloatArray()){
        type2=TypeSystem::floatType;
    }
    if(true){
        return;
    }
    if(type1!=type2){
        fprintf(stderr, "AssignStmt TypeCheck: type %s and %s mismatch ",
            type1->toStr().c_str(), type2->toStr().c_str());
        exit(EXIT_FAILURE);
    }
    if(type1->isConstInt()||type1->isConstFloat()){
        fprintf(stderr, "AssignStmt TypeCheck: type %s cannot be assigned ",
            type1->toStr().c_str());
        exit(EXIT_FAILURE);
    }
}

void CallParamSeqNode::typeCheck()
{
    for(int i = 0;i<(int)paramList.size();i++){
        paramList[i]->typeCheck();
    }
}

void ExpStmt::typeCheck()
{
    expr->typeCheck();
}

void NullStmt::typeCheck()
{

}

void CallFunc::typeCheck()
{
    callParamSeqNode->typeCheck();
    //实参与形参匹配
    std::vector<Type*> funcParamsTypeList=((FunctionType*)funcSe->getType())->getParamsType();
    std::vector<ExprNode*> callParamList=callParamSeqNode->getParamList();
    //检查数目是否匹配
    if(funcParamsTypeList.size() != callParamList.size()){
        fprintf(stderr, "Call params number and Function params number mismatch ");
        exit(EXIT_FAILURE);
    }
    if(funcParamsTypeList.size()==0){
        symbolEntry->setType(((FunctionType*)funcSe->getType())->getRetType());
        return;
    }
    //检查类型是否匹配
    int flag=0;
    if(isCond==1){
        isCond=0;
        flag=1;
    }
    for(int i=0;i<(int)funcParamsTypeList.size();i++){
        Type* funcType=funcParamsTypeList[i];
        Type* callType=callParamList[i]->getSymPtr()->getType();
        if(funcType->isVoid()){
            fprintf(stderr, "Function params type cannot be void ");
            exit(EXIT_FAILURE);
        }
        if(funcType->isTypeInt()&&!callType->isTypeInt()&&!callType->isTypeIntArray()&&!callType->isTypeFloat()&&!callType->isTypeFloatArray()){
            fprintf(stderr, "CallFunc TypeCheck: funcType %s and callType %s mismatch ",
                funcType->toStr().c_str(), callType->toStr().c_str());
            exit(EXIT_FAILURE);
        }
        if(funcType->isTypeFloat()&&!callType->isValue()&&!callType->isTypeFloatArray()){
            fprintf(stderr, "CallFunc TypeCheck: funcType %s and callType %s mismatch ",
                funcType->toStr().c_str(), callType->toStr().c_str());
            exit(EXIT_FAILURE);
        }
        if(funcType->isTypeIntArray()&&!callType->isTypeIntArray()){
            fprintf(stderr, "CallFunc TypeCheck: funcType %s and callType %s mismatch ",
                funcType->toStr().c_str(), callType->toStr().c_str());
            exit(EXIT_FAILURE);
        }
        if(funcType->isTypeFloatArray()&&!callType->isTypeFloatArray()){
            fprintf(stderr, "CallFunc TypeCheck: funcType %s and callType %s mismatch ",
                funcType->toStr().c_str(), callType->toStr().c_str());
            exit(EXIT_FAILURE);
        }
        //数组维度匹配问题
    }
    if(flag==1){
        isCond=1;
    }
    symbolEntry->setType(((FunctionType*)funcSe->getType())->getRetType());
}

void UnaryExpr::output(int level)
{
    std::string op_str;
    switch(op)
    {
        case UPLUS:
            op_str = "uplus";
            break;
        case UMINUS:
            op_str = "uminus";
            break;
        case NOT:
            op_str = "not";
            break;
    }
    fprintf(yyout, "%*cUnaryExpr\top: %s\n", level, ' ', op_str.c_str());
    expr->output(level + 4);
}

void BinaryExpr::output(int level)
{
    std::string op_str;
    switch(op)
    {
        case ADD:
            op_str = "add";
            break;
        case SUB:
            op_str = "sub";
            break;
        case MUL:
            op_str = "mul";
            break;   
        case MOD:
            op_str = "mod";
            break;                    
        case AND:
            op_str = "and";
            break;
        case OR:
            op_str = "or";
            break;
        case LESS:
            op_str = "less";
            break;
       case GREATER:
            op_str = "greater";
            break;
       case LESSEQUAL:
            op_str = "lessequal";
            break;
       case GREATEREQUAL:
            op_str = "greaterequal";
            break;
       case TRUEEQUAL:
            op_str = "trueequal";
            break;
       case FALSEEQUAL:
            op_str = "falseequal";
            break;
        default:
            op_str = "";
            break;
    }
    fprintf(yyout, "%*cBinaryExpr\top: %s\n", level, ' ', op_str.c_str());
    expr1->output(level + 4);
    expr2->output(level + 4);
}

void Ast::output()
{
    fprintf(yyout, "program\n");
    if(root != nullptr)
        root->output(4);
}

void Constant::output(int level)
{
    std::string type, value;
    type = symbolEntry->getType()->toStr();
    value = symbolEntry->toStr();
    fprintf(yyout, "%*cIntegerLiteral\tvalue: %s\ttype: %s\n", level, ' ',
            value.c_str(), type.c_str());
}

void ArrayNode::addNode(ExprNode *node){
    exprList.push_back(node);
}

void ArrayNode::addFirstNode(ExprNode *node){
    exprList.insert(exprList.begin(), node);
}

void ArrayNode::output(int level){
    fprintf(yyout, "%*cArrayNode\n", level, ' ');
    for(long unsigned int i=0;i<exprList.size();i++){
        exprList[i]->output(level+4);
    }
}

bool Id::isArray(){
    return getType()->isArray();
}

void Id::output(int level)
{
    std::string name, type;
    int scope;
    name = symbolEntry->toStr();
    type = symbolEntry->getType()->toStr();
    scope = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getScope();
    fprintf(yyout, "%*cId\tname: %s\tscope: %d\ttype: %s\n", level, ' ',
            name.c_str(), scope, type.c_str());
    if(isArray() && arrayList!=nullptr){
        fprintf(yyout, "%*cArrayLists\n", level+4, ' ');
        arrayList->output(level+8);
    }
}

Type* Id::getType()
{
    return symbolEntry->getType();
}

void CompoundStmt::output(int level)
{
    fprintf(yyout, "%*cCompoundStmt\n", level, ' ');
    if(stmt!=nullptr)
        stmt->output(level + 4);
    else
        fprintf(yyout, "%*cNullStmt\n", level+4, ' ');
}

void ExpStmt::output(int level)
{
    fprintf(yyout, "%*cExpStmt\n", level, ' ');
    expr->output(level+4);
}

void NullStmt::output(int level)
{
    fprintf(yyout, "%*cNullStmt\n", level, ' ');
}

void SeqNode::output(int level)
{
    stmt1->output(level);
    stmt2->output(level);
}

void ArrayDeclNode::addDecl(ArrayDeclNode* next){
    arrayDeclList.push_back(next);
}

void ArrayDeclNode::setLeafNode(ExprNode* leaf){
    leafNode = leaf;
}

bool ArrayDeclNode::isLeaf(){
    return arrayDeclList.empty();
}

void ArrayDeclNode::output(int level)
{
    fprintf(yyout, "%*cArrayDeclNode\n", level, ' ');
    for(long unsigned int i=0;i<arrayDeclList.size();i++){
        arrayDeclList[i]->output(level+4);
    }
    if(leafNode!=nullptr){
        leafNode->output(level+4);
    }
}

void VarDeclNode::output(int level)
{
    fprintf(yyout, "%*cVarDeclNode\n", level, ' ');
    id->output(level + 4);
    if(expr!=nullptr)
        expr->output(level + 4);
}

void VarDeclSeqNode::output(int level)
{
    fprintf(yyout, "%*cVarDeclSeqNode\n", level, ' ');
    for(auto it=varDeclList.begin();it!=varDeclList.end();++it)
    {
        (*it)->output(level+4);
    }
}

void VarDeclStmt::output(int level)
{
    fprintf(yyout, "%*cVarDeclStmt\n", level, ' ');
    varDeclSeqNode->output(level+4);
}

void ConstDeclNode::output(int level)
{
    fprintf(yyout, "%*cConstDeclNode\n", level, ' ');
    id->output(level + 4);
    expr->output(level + 4);
}

void ConstDeclSeqNode::output(int level)
{
    fprintf(yyout, "%*cConstDeclSeqNode\n", level, ' ');
    for(auto it=constDeclList.begin();it!=constDeclList.end();++it)
    {
        (*it)->output(level+4);
    }
}

void ConstDeclStmt::output(int level)
{
    fprintf(yyout, "%*cConstDeclStmt\n", level, ' ');
    constDeclSeqNode->output(level+4);
}

void IfStmt::output(int level)
{
    fprintf(yyout, "%*cIfStmt\n", level, ' ');
    cond->output(level + 4);
    thenStmt->output(level + 4);
}

void IfElseStmt::output(int level)
{
    fprintf(yyout, "%*cIfElseStmt\n", level, ' ');
    cond->output(level + 4);
    thenStmt->output(level + 4);
    elseStmt->output(level + 4);
}

void WhileStmt::output(int level)
{
    fprintf(yyout, "%*cWhileStmt\n", level, ' ');
    cond->output(level + 4);
    whileStmt->output(level + 4);
}

void BreakStmt::output(int level)
{
    fprintf(yyout, "%*cBreakStmt\n", level, ' ');
}

void ContinueStmt::output(int level)
{
    fprintf(yyout, "%*cContinueStmt\n", level, ' ');
}

void ReturnStmt::output(int level)
{
    fprintf(yyout, "%*cReturnStmt\n", level, ' ');
    if(retValue!=nullptr)
        retValue->output(level + 4);
    else
        fprintf(yyout, "%*cvoid\n", level+4, ' ');
}

void AssignStmt::output(int level)
{
    fprintf(yyout, "%*cAssignStmt\n", level, ' ');
    lval->output(level + 4);
    expr->output(level + 4);
}

Type* FuncParamNode::getType()
{
    if(id!=nullptr)
        return id->getType();
    return nullptr;
}

void FuncParamNode::output(int level)
{
    fprintf(yyout, "%*cFuncParamNode\n", level, ' ');
    if(id!=nullptr)
        id->output(level+4);
}

void FuncParamSeqNode::insertParam(FuncParamNode *param)
{
    paramList.push_back(param);
}

std::vector<Type*> FuncParamSeqNode::getParamsType()
{
    std::vector<Type*> paramType;
    for(auto it=paramList.begin();it!=paramList.end();++it)
    {
        if((*it)->getType()!=nullptr)
            paramType.push_back((*it)->getType());
    }
    return paramType;
}

void FuncParamSeqNode::output(int level)
{
    fprintf(yyout, "%*cFuncParamSeqNode\n", level, ' ');
    for(auto it=paramList.begin();it!=paramList.end();++it)
    {
        (*it)->output(level+4);
    }
}

void FunctionDef::output(int level)
{
    std::string name, type;
    name = se->toStr();
    type = se->getType()->toStr();
    fprintf(yyout, "%*cFunctionDefine function name: %s, type: %s\n", level, ' ', 
            name.c_str(), type.c_str());
    paramList->output(level+4);
    stmt->output(level + 4);
}

void CallParamSeqNode::insertParam(ExprNode *param)
{
    paramList.push_back(param);
}

void CallParamSeqNode::output(int level)
{
    fprintf(yyout, "%*cCallParamSeqNode\n", level, ' ');
    std::vector<ExprNode*>::iterator it;
    for(it=paramList.begin();it!=paramList.end();++it)
    {
        (*it)->output(level+4);
    }
}

void CallFunc::output(int level)
{
    fprintf(yyout, "%*cCallFunc\tname:%s\ttype:%s\n", level, ' ',symbolEntry->toStr().c_str(),symbolEntry->getType()->toStr().c_str());
    if(callParamSeqNode!=nullptr)
    {
        callParamSeqNode->output(level+4);
    }
}