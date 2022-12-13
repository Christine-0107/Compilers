#include "Ast.h"
#include "SymbolTable.h"
#include "Unit.h"
#include "Instruction.h"
#include "IRBuilder.h"
#include <string>
#include "Type.h"

extern FILE *yyout;
int Node::counter = 0;
IRBuilder* Node::builder = nullptr;
Type* returnTypeDef = nullptr;
Type* returnType=nullptr;
int count=0;
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
        new BinaryInstruction(BinaryInstruction::SUB, dst, src1, src2, bb);
    }
    else if(op==NOT)
    {
        BasicBlock *bb=builder->getInsertBB();
        expr->genCode();
        Operand *src;
        if(expr->getOperand()->getType()->isInt())
        {
            src= new Operand(new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel()));
            new CmpInstruction(CmpInstruction::NE, src, expr->getOperand(),new Operand(new ConstantSymbolEntry(TypeSystem::intType, 0)),bb);
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
        if(expr1->getOperand()->getType()->isInt()&&expr2->getOperand()->getType()->isBool())
        {
            src1 = expr1->getOperand();
            SymbolEntry *tmp=new TemporarySymbolEntry(src1->getType(),SymbolTable::getLabel());
            src2 = new Operand(tmp);
            new ZextInstruction(src2, expr2->getOperand(), bb);
        }
        else if(expr1->getOperand()->getType()->isBool()&&expr2->getOperand()->getType()->isInt())
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
        new CmpInstruction(opcode,dst,src1,src2,bb);
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
        }
        new BinaryInstruction(opcode, dst, src1, src2, bb);
    }
}

void Constant::genCode()
{
    // we don't need to generate code.
}

void Id::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    Operand *addr = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getAddr();
    new LoadInstruction(dst, addr, bb);
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
    nestWhile.pop();
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
        if(expr!=nullptr)
        {
            int v=((ConstantSymbolEntry*)expr->getSymPtr())->getValue();
            se->setValue((float)v);
        }
        builder->getUnit()->insertId(se);
        if(expr!=nullptr)
        {
            BasicBlock *bb=builder->getInsertBB();
            expr->genCode();
            Operand *src=dynamic_cast<ExprNode*>(expr)->getOperand();
            new StoreInstruction(addr,src,bb);
        }
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
            BasicBlock *bb=builder->getInsertBB();
            expr->genCode();
            Operand *src=dynamic_cast<ExprNode*>(expr)->getOperand();
            new StoreInstruction(addr,src,bb);
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
        //add global defination
        if(expr!=nullptr)
        {
            int v=((ConstantSymbolEntry*)expr->getSymPtr())->getValue();
            se->setValue((float)v);
        }
        builder->getUnit()->insertId(se);

        BasicBlock *bb=builder->getInsertBB();
        expr->genCode();
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

    /***
     * We haven't implemented array yet, the lval can only be ID. So we just store the result of the `expr` to the addr of the id.
     * If you want to implement array, you have to caculate the address first and then store the result into it.
     */
    //printf("addr:%s\n",addr->toStr().c_str());
    //printf("src:%s\n",src->toStr().c_str());
    new StoreInstruction(addr, src, bb);
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
    id->typeCheck();
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
    // Todo
    expr1->typeCheck();
    expr2->typeCheck();
    Type* type1=expr1->getSymPtr()->getType();
    Type* type2=expr2->getSymPtr()->getType();
   
    if(!(type1->isValue()||type1->isBool())){
        fprintf(stderr, "BinaryExpr TypeCheck: type %s wrong ", type1->toStr().c_str());
        exit(EXIT_FAILURE);
    }
    if(!(type2->isValue()||type2->isBool())){
        fprintf(stderr, "BinaryExpr TypeCheck: type %s wrong ", type2->toStr().c_str());
        exit(EXIT_FAILURE);
    }
    if(op==AND || op==OR){
        if((type1->isTypeInt()||type1->isBool()) && (type2->isTypeInt()||type2->isBool())){
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
        if(type1->isTypeInt()&&type2->isTypeInt()){
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
    expr->typeCheck();
    Type* type=expr->getSymPtr()->getType();
    
    if(!(type->isValue()||type->isBool())){
        fprintf(stderr, "UnaryExpr TypeCheck: type %s wrong ", type->toStr().c_str());
        exit(EXIT_FAILURE);
    }
    if(op==NOT){
        if(type->isTypeInt()||type->isBool()){
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
        else if(type->isTypeFloat()&&(isCond==0)){
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

void Id::typeCheck()
{
    // Todo
    // 数组相关
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
        if(type1->isTypeInt()){
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
        if(type1->isTypeInt()){
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
        if(type1->isTypeInt()){
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

void VarDeclNode::typeCheck()
{
    // Todo
    id->typeCheck();
    if(expr!=nullptr){
        expr->typeCheck();
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
    expr->typeCheck();
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
    }
}

void AssignStmt::typeCheck()
{
    // Todo
    lval->typeCheck();
    expr->typeCheck();
    Type* type1=lval->getSymPtr()->getType();
    Type* type2=expr->getSymPtr()->getType();
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
        if(funcType->isTypeInt()&&!callType->isTypeInt()){
            fprintf(stderr, "CallFunc TypeCheck: funcType %s and callType %s mismatch ",
                funcType->toStr().c_str(), callType->toStr().c_str());
            exit(EXIT_FAILURE);
        }
        if(funcType->isTypeFloat()&&!callType->isValue()){
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
        case AND:
            op_str = "and";
            break;
        case OR:
            op_str = "or";
            break;
        case LESS:
            op_str = "less";
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

void Id::output(int level)
{
    std::string name, type;
    int scope;
    name = symbolEntry->toStr();
    type = symbolEntry->getType()->toStr();
    scope = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getScope();
    fprintf(yyout, "%*cId\tname: %s\tscope: %d\ttype: %s\n", level, ' ',
            name.c_str(), scope, type.c_str());
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

void VarDeclNode::output(int level)
{
    fprintf(yyout, "%*cVarDeclNode\n", level, ' ');
    id->output(level + 4);
    if(expr!=nullptr)
        expr->output(level+4);
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
    expr->output(level+4);
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
    else
    {
        fprintf(yyout, "%*cCallFunc NULL\n", level+4, ' ');
    }
}