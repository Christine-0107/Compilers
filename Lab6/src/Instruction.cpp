#include "Instruction.h"
#include "BasicBlock.h"
#include <iostream>
#include "Function.h"
#include "Type.h"
extern FILE* yyout;

Instruction::Instruction(unsigned instType, BasicBlock *insert_bb)
{
    prev = next = this;
    opcode = -1;
    this->instType = instType;
    if (insert_bb != nullptr)
    {
        insert_bb->insertBack(this);
        parent = insert_bb;
    }
}

Instruction::~Instruction()
{
    parent->remove(this);
}

BasicBlock *Instruction::getParent()
{
    return parent;
}

void Instruction::setParent(BasicBlock *bb)
{
    parent = bb;
}

void Instruction::setNext(Instruction *inst)
{
    next = inst;
}

void Instruction::setPrev(Instruction *inst)
{
    prev = inst;
}

Instruction *Instruction::getNext()
{
    return next;
}

Instruction *Instruction::getPrev()
{
    return prev;
}

BinaryInstruction::BinaryInstruction(unsigned opcode, Operand *dst, Operand *src1, Operand *src2, BasicBlock *insert_bb) : Instruction(BINARY, insert_bb)
{
    this->opcode = opcode;
    operands.push_back(dst);
    operands.push_back(src1);
    operands.push_back(src2);
    dst->setDef(this);
    src1->addUse(this);
    src2->addUse(this);
}

BinaryInstruction::~BinaryInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum() == 0)
        delete operands[0];
    operands[1]->removeUse(this);
    operands[2]->removeUse(this);
}

void BinaryInstruction::output() const
{
    std::string s1, s2, s3, op, type;
    s1 = operands[0]->toStr();
    s2 = operands[1]->toStr();
    s3 = operands[2]->toStr();
    type = operands[0]->getType()->toStr();
    switch (opcode)
    {
    case ADD:
        op = "add";
        break;
    case SUB:
        op = "sub";
        break;
    case MUL:
        op = "mul";
        break;
    case DIV:
        op = "sdiv";
        break;
    case MOD:
        op = "srem";
        break;
    default:
        break;
    }
    fprintf(yyout, "  %s = %s %s %s, %s\n", s1.c_str(), op.c_str(), type.c_str(), s2.c_str(), s3.c_str());
}

CmpInstruction::CmpInstruction(unsigned opcode, Operand *dst, Operand *src1, Operand *src2, BasicBlock *insert_bb): Instruction(CMP, insert_bb){
    this->opcode = opcode;
    operands.push_back(dst);
    operands.push_back(src1);
    operands.push_back(src2);
    dst->setDef(this);
    src1->addUse(this);
    src2->addUse(this);
}

CmpInstruction::~CmpInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum() == 0)
        delete operands[0];
    operands[1]->removeUse(this);
    operands[2]->removeUse(this);
}

void CmpInstruction::output() const
{
    std::string s1, s2, s3, op, type;
    s1 = operands[0]->toStr();
    s2 = operands[1]->toStr();
    s3 = operands[2]->toStr();
    type = operands[1]->getType()->toStr();
    switch (opcode)
    {
    case E:
        op = "eq";
        break;
    case NE:
        op = "ne";
        break;
    case L:
        op = "slt";
        break;
    case LE:
        op = "sle";
        break;
    case G:
        op = "sgt";
        break;
    case GE:
        op = "sge";
        break;
    default:
        op = "";
        break;
    }

    fprintf(yyout, "  %s = icmp %s %s %s, %s\n", s1.c_str(), op.c_str(), type.c_str(), s2.c_str(), s3.c_str());
}

UncondBrInstruction::UncondBrInstruction(BasicBlock *to, BasicBlock *insert_bb) : Instruction(UNCOND, insert_bb)
{
    branch = to;
}

void UncondBrInstruction::output() const
{
    fprintf(yyout, "  br label %%B%d\n", branch->getNo());
}

void UncondBrInstruction::setBranch(BasicBlock *bb)
{
    branch = bb;
}

BasicBlock *UncondBrInstruction::getBranch()
{
    return branch;
}

CondBrInstruction::CondBrInstruction(BasicBlock*true_branch, BasicBlock*false_branch, Operand *cond, BasicBlock *insert_bb) : Instruction(COND, insert_bb){
    this->true_branch = true_branch;
    this->false_branch = false_branch;
    cond->addUse(this);
    operands.push_back(cond);
}

CondBrInstruction::~CondBrInstruction()
{
    operands[0]->removeUse(this);
}

void CondBrInstruction::output() const
{
    std::string cond, type;
    cond = operands[0]->toStr();
    type = operands[0]->getType()->toStr();
    int true_label = true_branch->getNo();
    int false_label = false_branch->getNo();
    fprintf(yyout, "  br %s %s, label %%B%d, label %%B%d\n", type.c_str(), cond.c_str(), true_label, false_label);
}

void CondBrInstruction::setFalseBranch(BasicBlock *bb)
{
    false_branch = bb;
}

BasicBlock *CondBrInstruction::getFalseBranch()
{
    return false_branch;
}

void CondBrInstruction::setTrueBranch(BasicBlock *bb)
{
    true_branch = bb;
}

BasicBlock *CondBrInstruction::getTrueBranch()
{
    return true_branch;
}

RetInstruction::RetInstruction(Operand *src, BasicBlock *insert_bb) : Instruction(RET, insert_bb)
{
    if(src != nullptr)
    {
        operands.push_back(src);
        src->addUse(this);
    }
}

RetInstruction::~RetInstruction()
{
    if(!operands.empty())
        operands[0]->removeUse(this);
}

void RetInstruction::output() const
{
    if(operands.empty())
    {
        fprintf(yyout, "  ret void\n");
    }
    else
    {
        std::string ret, type;
        ret = operands[0]->toStr();
        type = operands[0]->getType()->toStr();
        fprintf(yyout, "  ret %s %s\n", type.c_str(), ret.c_str());
    }
}

AllocaInstruction::AllocaInstruction(Operand *dst, SymbolEntry *se, BasicBlock *insert_bb) : Instruction(ALLOCA, insert_bb)
{
    operands.push_back(dst);
    dst->setDef(this);
    this->se = se;
}

AllocaInstruction::~AllocaInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum() == 0)
        delete operands[0];
}

void AllocaInstruction::output() const
{
    std::string dst, type;
    dst = operands[0]->toStr();
    type = se->getType()->toStr();
    fprintf(yyout, "  %s = alloca %s, align 4\n", dst.c_str(), type.c_str());
}

LoadInstruction::LoadInstruction(Operand *dst, Operand *src_addr, BasicBlock *insert_bb) : Instruction(LOAD, insert_bb)
{
    operands.push_back(dst);
    operands.push_back(src_addr);
    dst->setDef(this);
    src_addr->addUse(this);
}

LoadInstruction::~LoadInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum() == 0)
        delete operands[0];
    operands[1]->removeUse(this);
}

void LoadInstruction::output() const
{
    std::string dst = operands[0]->toStr();
    std::string src = operands[1]->toStr();
    std::string src_type;
    std::string dst_type;
    dst_type = operands[0]->getType()->toStr();
    src_type = operands[1]->getType()->toStr();
    fprintf(yyout, "  %s = load %s, %s %s, align 4\n", dst.c_str(), dst_type.c_str(), src_type.c_str(), src.c_str());
}

StoreInstruction::StoreInstruction(Operand *dst_addr, Operand *src, BasicBlock *insert_bb) : Instruction(STORE, insert_bb)
{
    operands.push_back(dst_addr);
    operands.push_back(src);
    dst_addr->addUse(this);
    src->addUse(this);
}

StoreInstruction::~StoreInstruction()
{
    operands[0]->removeUse(this);
    operands[1]->removeUse(this);
}

void StoreInstruction::output() const
{
    std::string dst = operands[0]->toStr();
    std::string src = operands[1]->toStr();
    std::string dst_type = operands[0]->getType()->toStr();
    std::string src_type = operands[1]->getType()->toStr();

    fprintf(yyout, "  store %s %s, %s %s, align 4\n", src_type.c_str(), src.c_str(), dst_type.c_str(), dst.c_str());
}

CallInstruction::CallInstruction(Operand* dst, SymbolEntry *se, std::vector<Operand*> params, BasicBlock* insert_bb) : Instruction(CALL, insert_bb) 
{
    this->se=se;
    operands.push_back(dst);
    dst->setDef(this);
    for(auto param=params.begin();param!=params.end();++param)
    {
        operands.push_back(*param);
        (*param)->addUse(this);
    }
}

CallInstruction::~CallInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum()==0)
        delete operands[0];
    for(long unsigned int i=1;i<operands.size();++i)
        operands[i]->removeUse(this);
}

void CallInstruction::output() const
{
    bool isVoid=((FunctionType*)(operands[0]->getType()))->isVoid();
    if(isVoid)
    {
        fprintf(yyout, "  ");
    }
    else
    {
        fprintf(yyout, "  %s = ", operands[0]->toStr().c_str());
        
    }
    fprintf(yyout, "call %s %s(", ((FunctionType*)se->getType())->getRetType()->toStr().c_str(), se->toStr().c_str());
    for(long unsigned int i=1;i<operands.size();++i)
    {
        if(i>=2)
            fprintf(yyout, ", ");
        fprintf(yyout, "%s %s", operands[i]->getType()->toStr().c_str(), operands[i]->toStr().c_str());
    }
    fprintf(yyout, ")\n");
}  

XorInstruction::XorInstruction(Operand *dst, Operand *src, BasicBlock *insert_bb) : Instruction(XOR, insert_bb)
{
    operands.push_back(dst);
    operands.push_back(src);
    dst->setDef(this);
    src->addUse(this);
}

void XorInstruction::output() const {
    fprintf(yyout, "  %s = xor %s %s, true\n", operands[0]->toStr().c_str(),operands[1]->getType()->toStr().c_str(), operands[1]->toStr().c_str());
}

XorInstruction:: ~XorInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum()==0)
        delete operands[0];
    operands[1]->removeUse(this);
}

ZextInstruction::ZextInstruction(Operand *dst, Operand *src, BasicBlock *insert_bb) : Instruction(ZEXT, insert_bb)
{
    operands.push_back(dst);
    operands.push_back(src);
    dst->setDef(this);
    src->addUse(this);
}

void ZextInstruction::output() const 
{
    fprintf(yyout, "  %s = zext %s %s to i32\n", operands[0]->toStr().c_str(),operands[1]->getType()->toStr().c_str(), operands[1]->toStr().c_str());
}

ZextInstruction:: ~ZextInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum()==0)
        delete operands[0];
    operands[1]->removeUse(this);
}

CastInstruction::CastInstruction(unsigned opcode, Operand *dst, Operand *src, BasicBlock *insert_bb) : Instruction(CAST, insert_bb)
{
    this->opcode=opcode;
    operands.push_back(dst);
    operands.push_back(src);
    dst->setDef(this);
    src->addUse(this);
}

void CastInstruction::output() const 
{
    
}

CastInstruction:: ~CastInstruction()
{
    operands[0]->setDef(nullptr);
    if(operands[0]->usersNum()==0)
        delete operands[0];
    operands[1]->removeUse(this);
}

MachineOperand* Instruction::genMachineOperand(Operand* ope)
{
    auto se = ope->getEntry();
    MachineOperand* mope = nullptr;
    if(se->isConstant())
    {
        if(se->getType()->isTypeFloat())
        {
            // from double to float safely
            mope = new MachineOperand(MachineOperand::IMM, 0);
            std::string str=std::to_string(dynamic_cast<ConstantSymbolEntry*>(se)->getValue());
            float v=0;
            sscanf(str.c_str(),"%f",&v);
            mope->setFVal(v);
        }
        else
        {
            mope = new MachineOperand(MachineOperand::IMM, dynamic_cast<ConstantSymbolEntry*>(se)->getValue());
        }
        
    }
    else if(se->isTemporary())
    {
        //printf("%d\n",(dynamic_cast<TemporarySymbolEntry*>(se)->getLabel()));
        mope = new MachineOperand(MachineOperand::VREG, dynamic_cast<TemporarySymbolEntry*>(se)->getLabel());
        if(se->getType()->isTypeFloat())
        {
            mope->setFloat();
        }
    }
    else if(se->isVariable())
    {
        auto id_se = dynamic_cast<IdentifierSymbolEntry*>(se);
        if(id_se->isParam())
        {
            auto pos=parent->getParent()->getParamPos(ope);
            if(pos>=0 && pos <=3)
            {
                if(ope->getType()->isTypeFloat())
                {
                    mope = new MachineOperand(MachineOperand::REG,pos+16);
                    mope -> setFloat();
                }
                else
                {
                    mope = new MachineOperand(MachineOperand::REG,pos);
                }
            }
            else
            {
                /*printf("Failed: Insruction.cpp: line 433\n");
                exit(0);*/
            }
        }
        else if(id_se->isGlobal())
        {
            mope = new MachineOperand(id_se->toStr().c_str());
        }
        else
        {
            exit(0);
        }
    }
    return mope;
}

MachineOperand* Instruction::genMachineReg(int reg) 
{
    return new MachineOperand(MachineOperand::REG, reg);
}

MachineOperand* Instruction::genMachineVReg(bool isfloat) 
{
    auto mope=new MachineOperand(MachineOperand::VREG, SymbolTable::getLabel());
    if(isfloat)
        mope->setFloat();
    return mope;
}


MachineOperand* Instruction::genMachineImm(int val) 
{
    return new MachineOperand(MachineOperand::IMM, val);
}

MachineOperand* Instruction::genMachineLabel(int block_no)
{
    std::ostringstream buf;
    buf << ".L" << block_no;
    std::string label = buf.str();
    return new MachineOperand(label);
}

void AllocaInstruction::genMachineCode(AsmBuilder* builder)
{
    /* HINT:
    * Allocate stack space for local variabel
    * Store frame offset in symbol entry */
    auto cur_func = builder->getFunction();
    int offset;
    if(!se->getType()->isArray()){
        offset = cur_func->AllocSpace(4);
    }
    else{
        if(se->getType()->isIntArray()){
            offset = cur_func->AllocSpace(dynamic_cast<IntArrayType*>(se->getType())->getSize() / dynamic_cast<IntType*>(TypeSystem::intType)->getSize() * 4);
        }
        else if(se->getType()->isConstIntArray()){
            offset = cur_func->AllocSpace(dynamic_cast<ConstIntArrayType*>(se->getType())->getSize() / dynamic_cast<IntType*>(TypeSystem::intType)->getSize() * 4);
        }
        else if(se->getType()->isFloatArray()){
            offset = cur_func->AllocSpace(dynamic_cast<FloatArrayType*>(se->getType())->getSize() / dynamic_cast<FloatType*>(TypeSystem::floatType)->getSize() * 4);
        }
        else if(se->getType()->isConstFloatArray()){
            offset = cur_func->AllocSpace(dynamic_cast<ConstFloatArrayType*>(se->getType())->getSize() / dynamic_cast<FloatType*>(TypeSystem::floatType)->getSize() * 4);
        }
    }
    
    //int offset = cur_func->AllocSpace(se->getType()->getSize() / TypeSystem::intType->getSize() * 4);
    dynamic_cast<TemporarySymbolEntry*>(operands[0]->getEntry())->setOffset(-offset);
}

void LoadInstruction::genMachineCode(AsmBuilder* builder)
{
    auto cur_block = builder->getBlock();
    MachineInstruction* cur_inst = nullptr;
    // Load global operand
    if(operands[1]->getEntry()->isVariable()
    && dynamic_cast<IdentifierSymbolEntry*>(operands[1]->getEntry())->isGlobal())
    {
        if(operands[1]->getEntry()->getType()->isPtr()&& dynamic_cast<PointerType*>(operands[1]->getEntry()->getType())->getType()->isArray())
        {
            auto dst = genMachineOperand(operands[0]);
            auto src = genMachineOperand(operands[1]);
            // example: load r0, addr_a
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, dst, src);
            cur_block->InsertInst(cur_inst);
        }
        else
        {
            auto dst = genMachineOperand(operands[0]);
            auto internal_reg1 = genMachineVReg();
            auto internal_reg2 = new MachineOperand(*internal_reg1);
            auto src = genMachineOperand(operands[1]);
            // example: load r0, addr_a
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg1, src);
            cur_block->InsertInst(cur_inst);
            // example: load r1, [r0]
            if(dst->isFloat())
                cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::FLDR, dst, internal_reg2);
            else
                cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, dst, internal_reg2);
            cur_block->InsertInst(cur_inst);            
        }

    }
    // Load local operand
    else if(operands[1]->getEntry()->isTemporary()
    && operands[1]->getDef()
    && operands[1]->getDef()->isAlloc())
    {
        // example: load r1, [r0, #4]
        auto dst = genMachineOperand(operands[0]);
        auto src1 = genMachineReg(11);
        auto src2 = genMachineImm(dynamic_cast<TemporarySymbolEntry*>(operands[1]->getEntry())->getOffset());
        if(src2->getVal()>255 || src2->getVal()<-255)
        {
            auto internal_reg = genMachineVReg();
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR,  internal_reg, src2);
            cur_block->InsertInst(cur_inst);
            src2 = new MachineOperand(*internal_reg);
        }
        if(!(operands[0]->getEntry()->getType()->isTypeFloat()))
        {
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, dst, src1, src2);
        }
        else
        {
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::FLDR, dst, src1, src2);
        }
        cur_block->InsertInst(cur_inst);
    }
    // Load operand from temporary variable
    else
    {
        // example: load r1, [r0]
        auto dst = genMachineOperand(operands[0]);        
        auto src = genMachineOperand(operands[1]);
        if(dst->isFloat())
        {
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::FLDR, dst, src);
        }
        else
        {
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, dst, src);
        }
        cur_block->InsertInst(cur_inst);
    }
}

void StoreInstruction::genMachineCode(AsmBuilder* builder)
{
    // TODO
    auto cur_block = builder->getBlock();
    MachineInstruction* cur_inst = nullptr;
    if(operands[1]->getEntry()->isVariable() && dynamic_cast<IdentifierSymbolEntry*>(operands[1]->getEntry())->isParam())
    {
        auto pos=parent->getParent()->getParamPos(operands[1]);
        if(pos>3)
        {
            auto off=4*(pos-4);
            dynamic_cast<TemporarySymbolEntry*>(operands[0]->getEntry())->setOffset(off);
            return;
        }
    }
    MachineOperand* src = genMachineOperand(operands[1]);
    if(src->isImm())
    {
        MachineOperand* internal_reg = nullptr;   
        if(src->isFloat())
        {
            auto ldr_reg = genMachineVReg();
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, ldr_reg, src);
            cur_block->InsertInst(cur_inst);
            internal_reg = genMachineVReg(true);
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, internal_reg, ldr_reg);
        }
        else
        {
            internal_reg = genMachineVReg();
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg, src);
        }
        cur_block->InsertInst(cur_inst);
        src = new MachineOperand(*internal_reg);
    }
    // store global operand
    if(operands[0]->getEntry()->isVariable()
    && dynamic_cast<IdentifierSymbolEntry*>(operands[0]->getEntry())->isGlobal())
    {
        auto internal_reg1 = genMachineVReg();
        auto internal_reg2 = new MachineOperand(*internal_reg1);
        auto dst = genMachineOperand(operands[0]);
        // example: load r0, addr_a
        cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg1, dst);
        cur_block->InsertInst(cur_inst);
        // example: store r1, [r0]
        cur_inst = new StoreMInstruction(cur_block, StoreMInstruction::STR, src, internal_reg2);
        cur_block->InsertInst(cur_inst);
    }
    // store local operand
    else if(operands[0]->getEntry()->isTemporary()
    && operands[0]->getDef()
    && operands[0]->getDef()->isAlloc())
    {
        // example: store r1, [r0, #4]
        auto fp = genMachineReg(11);
        auto offset = genMachineImm(dynamic_cast<TemporarySymbolEntry*>(operands[0]->getEntry())->getOffset());
        if(offset->getVal()>255 || offset->getVal()<-255)
        {
            auto internal_reg = genMachineVReg();
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg, offset);
            cur_block->InsertInst(cur_inst);
            offset = new MachineOperand(*internal_reg);
        }   
        if(!(operands[1]->getEntry()->getType()->isTypeFloat()))
        { 
            cur_inst = new StoreMInstruction(cur_block, StoreMInstruction::STR, src, fp, offset);
        }
        else
        {
            cur_inst = new StoreMInstruction(cur_block, StoreMInstruction::FSTR, src, fp, offset);
        }
        cur_block->InsertInst(cur_inst);
    }
    // store operand from temporary variable
    else
    {

        auto dst = genMachineOperand(operands[0]);
        if(operands[1]->getEntry()->getType()->isTypeFloat())
        {
            cur_inst = new StoreMInstruction(cur_block, StoreMInstruction::FSTR, src, dst);
        }
        else
        {
            cur_inst = new StoreMInstruction(cur_block, StoreMInstruction::STR, src, dst);
        }
        cur_block->InsertInst(cur_inst);
    }
}

void BinaryInstruction::genMachineCode(AsmBuilder* builder)
{
    // TODO:
    // complete other instructions
    auto cur_block = builder->getBlock();
    auto dst = genMachineOperand(operands[0]);
    auto src1 = genMachineOperand(operands[1]);
    auto src2 = genMachineOperand(operands[2]);
    /* HINT:
    * The source operands of ADD instruction in ir code both can be immediate num.
    * However, it's not allowed in assembly code.
    * So you need to insert LOAD/MOV instrucrion to load immediate num into register.
    * As to other instructions, such as MUL, CMP, you need to deal with this situation, too.*/
    MachineInstruction* cur_inst = nullptr;
    if(operands[2]->getEntry()->getType()->isPtr()){
        auto fp = genMachineReg(11);
        int stack_size=dynamic_cast<TemporarySymbolEntry*>(operands[2]->getEntry())->getOffset();
        auto off = genMachineImm(stack_size);
        if(stack_size>255 || stack_size<-255)
        {
            auto internal_reg = genMachineVReg();
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg, off);
            cur_block->InsertInst(cur_inst);  
            off = new MachineOperand(*internal_reg);          
        }
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::ADD, src2, fp, off);
        cur_block->InsertInst(cur_inst);
    }
    if(src1->isImm())
    {
        if(src1->isFloat())
        {
            auto internal_reg=genMachineVReg();
            cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src1);
            cur_block->InsertInst(cur_inst);
            auto new_internal_reg = new MachineOperand(*internal_reg);
            src1 = genMachineVReg(true);
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, src1, new_internal_reg);
            cur_block->InsertInst(cur_inst);

        }
        else
        {
            auto internal_reg=genMachineVReg();
            cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src1);
            cur_block->InsertInst(cur_inst);
            src1=new MachineOperand(*internal_reg);
        }
    }
    if(opcode==MUL || opcode==DIV || opcode==MOD)
    {
        if(src2->isImm())
        {
          if(src2->isFloat())
            {
                auto internal_reg=genMachineVReg();
                cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src2);
                cur_block->InsertInst(cur_inst);
                auto new_internal_reg = new MachineOperand(*internal_reg);
                src2 = genMachineVReg(true);
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, src2, new_internal_reg);
                cur_block->InsertInst(cur_inst);

            }
            else
            {
                auto internal_reg=genMachineVReg();
                cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src2);
                cur_block->InsertInst(cur_inst);
                src2=new MachineOperand(*internal_reg);
            }
        }  
    }
    else
    {
        if(src2->isImm() && (src2->isFloat()||(src2->getVal()>255 || src2->getVal()<-255)))
        {
          if(src2->isFloat())
            {
                auto internal_reg=genMachineVReg();
                cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src2);
                cur_block->InsertInst(cur_inst);
                auto new_internal_reg = new MachineOperand(*internal_reg);
                src2 = genMachineVReg(true);
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, src2, new_internal_reg);
                cur_block->InsertInst(cur_inst);

            }
            else
            {
                auto internal_reg=genMachineVReg();
                cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src2);
                cur_block->InsertInst(cur_inst);
                src2=new MachineOperand(*internal_reg);
            }
        }          
    }
    switch (opcode)
    {
    case ADD:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::ADD, dst, src1, src2);
        break;
    case SUB:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::SUB, dst, src1, src2);
        break;
    case MUL:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::MUL, dst, src1, src2);
        break;
    case DIV:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::DIV, dst, src1, src2);
        break;
    case MOD:
    {
        // a%b=a-a/b*b
        // a/b
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::DIV,dst,src1,src2);
        cur_block->InsertInst(cur_inst);
        auto dst_mulRes=new MachineOperand(*dst);
        // a/b*b
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::MUL,dst_mulRes,dst,src2);
        cur_block->InsertInst(cur_inst);
        dst=new MachineOperand(*dst_mulRes);
        // a-a/b*b
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::SUB,dst,src1,dst_mulRes);
        break;
    }
    case AND:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::AND, dst, src1, src2);
        break;
    case OR:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::OR, dst, src1, src2);
        break;
    case FADD:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::FADD, dst, src1, src2);
        break; 
    case FSUB:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::FSUB, dst, src1, src2);
        break; 
    case FMUL:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::FMUL, dst, src1, src2);
        break; 
    case FDIV:
        cur_inst = new BinaryMInstruction(cur_block, BinaryMInstruction::FDIV, dst, src1, src2);
        break;       
    default:
        break;
    }
    cur_block->InsertInst(cur_inst);
}

void CmpInstruction::genMachineCode(AsmBuilder* builder)
{
    // TODO
    auto cur_block = builder->getBlock();
    auto src1 = genMachineOperand(operands[1]);
    auto src2 = genMachineOperand(operands[2]);
    MachineInstruction* cur_inst=nullptr;
    if(src1->isImm())
    {
        if(src1->isFloat())
        {
            auto ldr_reg = genMachineVReg();
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, ldr_reg, src1);
            cur_block->InsertInst(cur_inst);
            auto internal_reg = genMachineVReg(true);
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, internal_reg, ldr_reg);
            cur_block->InsertInst(cur_inst);
            src1 = new MachineOperand(*internal_reg);        
        }
        else
        {
            auto internal_reg = genMachineVReg();
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg, src1);
            cur_block->InsertInst(cur_inst);
            src1 = new MachineOperand(*internal_reg);        
        }    
    }
    if(src2->isImm())
    {
        if(src2->isFloat())
        {
            auto ldr_reg = genMachineVReg();
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, ldr_reg, src2);
            cur_block->InsertInst(cur_inst);
            auto internal_reg = genMachineVReg(true);
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, internal_reg, ldr_reg);
            cur_block->InsertInst(cur_inst);
            src2 = new MachineOperand(*internal_reg);        
        }
        else
        {
            auto internal_reg = genMachineVReg();
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg, src2);
            cur_block->InsertInst(cur_inst);
            src2 = new MachineOperand(*internal_reg);        
        }
    }
    if(src1->isFloat() && src2->isFloat())
        cur_inst = new CmpMInstruction(cur_block, CmpMInstruction::FCMP, src1, src2, opcode);
    else
        cur_inst = new CmpMInstruction(cur_block, CmpMInstruction::CMP, src1, src2, opcode);
    cur_block->InsertInst(cur_inst);
    cur_block->setCond(opcode);
    auto dst=genMachineOperand(operands[0]);
    auto trueOperand=genMachineImm(1);
    auto falseOperand=genMachineImm(0);
    cur_inst=new MovMInstruction(cur_block,MovMInstruction::MOV,dst,trueOperand,opcode);
    cur_block->InsertInst(cur_inst);
    int fopcode;
    switch (opcode)
    {
    case E:
        fopcode=NE;
        break;
    case NE:
        fopcode=E;
        break;
    case L:
        fopcode=GE;
        break;
    case G:
        fopcode=LE;
        break;
    case LE:
        fopcode=G;
        break;
    case GE:
        fopcode=L;
        break;
    default:
        break;
    }
    cur_inst=new MovMInstruction(cur_block,MovMInstruction::MOV,dst,falseOperand,fopcode);
    cur_block->InsertInst(cur_inst);
}

void UncondBrInstruction::genMachineCode(AsmBuilder* builder)
{
    // TODO
    auto cur_block = builder->getBlock();
    std::ostringstream buf;
    buf << ".L" << branch->getNo();
    std::string label = buf.str();
    MachineOperand* operand=new MachineOperand(label);
    MachineInstruction* cur_inst=new BranchMInstruction(cur_block, BranchMInstruction::B, operand);
    cur_block->InsertInst(cur_inst);
}

void CondBrInstruction::genMachineCode(AsmBuilder* builder)
{
    // TODO
    auto cur_block = builder->getBlock();
    std::ostringstream buf_trueBranch,buf_falseBranch;
    buf_trueBranch << ".L" << true_branch->getNo();
    buf_falseBranch << ".L" << false_branch->getNo();
    std::string true_label = buf_trueBranch.str(), false_label = buf_falseBranch.str();
    MachineOperand* true_operand=new MachineOperand(true_label);
    MachineOperand* false_operand=new MachineOperand(false_label);
    MachineInstruction* cur_inst = new BranchMInstruction(cur_block, BranchMInstruction::B, true_operand, cur_block->getCond());
    cur_block->InsertInst(cur_inst);
    cur_inst = new BranchMInstruction(cur_block, BranchMInstruction::B, false_operand);
    cur_block->InsertInst(cur_inst);
}

void RetInstruction::genMachineCode(AsmBuilder* builder)
{
    // TODO
    /* HINT:
    * 1. Generate mov instruction to save return value in r0
    * 2. Restore callee saved registers and sp, fp
    * 3. Generate bx instruction */
    auto cur_block = builder->getBlock();
    MachineInstruction* cur_inst = nullptr;
    auto func=this->parent->getParent();
    auto rettype=dynamic_cast<FunctionType*>(func->getSymPtr()->getType())->getRetType();
    //  Generate mov instruction to save return value in r0
    if(!operands.empty())
    {
        
        if(rettype->isTypeFloat())
        {
            auto src = genMachineOperand(operands[0]);
            if(src->isImm())
            {
                if(src->isFloat())
                {
                    auto internal_reg=genMachineVReg();
                    cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src);
                    cur_block->InsertInst(cur_inst);
                    auto new_internal_reg = new MachineOperand(*internal_reg);
                    src = genMachineVReg(true);
                    cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, src, new_internal_reg);
                    cur_block->InsertInst(cur_inst);

                }
                else
                {
                    auto internal_reg=genMachineVReg();
                    cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src);
                    cur_block->InsertInst(cur_inst);
                    src=new MachineOperand(*internal_reg);
                }
            }
            if(!operands[0]->getType()->isTypeFloat())
            {
                auto mreg=genMachineVReg(true);
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, mreg, src);
                cur_block->InsertInst(cur_inst);
                auto vreg = new MachineOperand(*mreg);
                src = genMachineVReg(true);
                cur_inst = new VcvtMInstruction(cur_block, VcvtMInstruction::ITOF, src, vreg);
                cur_block->InsertInst(cur_inst);
            }
            auto dst=new MachineOperand(MachineOperand::REG,16);
            dst->setFloat();
            cur_inst=new MovMInstruction(cur_block,MovMInstruction::VMOV,dst,src);
            cur_block->InsertInst(cur_inst);

        }
        else
        {
            auto src = genMachineOperand(operands[0]);
            if(src->isImm())
            {
                if(src->isFloat())
                {
                    auto internal_reg=genMachineVReg();
                    cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src);
                    cur_block->InsertInst(cur_inst);          
                    auto new_internal_reg = new MachineOperand(*internal_reg);
                    src = genMachineVReg(true);
                    cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, src, new_internal_reg);
                    cur_block->InsertInst(cur_inst);         
                }
                else
                {
                    auto internal_reg=genMachineVReg();
                    cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src);
                    cur_block->InsertInst(cur_inst);
                    src=new MachineOperand(*internal_reg);
                }
            }
            if(operands[0]->getType()->isTypeFloat())
            {
                auto vreg = genMachineVReg(true);
                cur_inst = new VcvtMInstruction(cur_block, VcvtMInstruction::FTOI, vreg, src);
                cur_block->InsertInst(cur_inst);
                auto mreg = new MachineOperand(*vreg);
                src = genMachineVReg();
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, src, mreg);
                cur_block->InsertInst(cur_inst);             
            }
            auto dst=new MachineOperand(MachineOperand::REG,0);
            cur_inst=new MovMInstruction(cur_block,MovMInstruction::MOV,dst,src);
            cur_block->InsertInst(cur_inst);
        }
    }
    auto lr=new MachineOperand(MachineOperand::REG,14);
    /*
    * The function named allocateRegisters executes after the function named genMachineCode()(main.cpp)
    * The vector of regs saved is updated when the function named allocateRegisters executes
    * Restoring callee saved registers can't be finished in this function
    */
    cur_inst=new BranchMInstruction(cur_block,BranchMInstruction::BX,lr);
    cur_block->InsertInst(cur_inst);
}

void CallInstruction::genMachineCode(AsmBuilder* builder)
{
    //TODO
    auto cur_block=builder->getBlock();
    MachineInstruction* cur_inst=nullptr;
    int rregIndex=0;
    int sregIndex=0;
    std::vector<Operand*>params;
    IdentifierSymbolEntry* func_se=dynamic_cast<IdentifierSymbolEntry*>(se);
    auto paramsType=(dynamic_cast<FunctionType*>(func_se->getType()))->getParamsType();
    for(long unsigned int i=0;i<paramsType.size();++i)
    {
        if(paramsType[i]->isTypeFloat() && sregIndex<4)
        {    
            auto src=genMachineOperand(operands[i+1]);
            if(src->isImm())
            {
                if(src->isFloat())
                {
                    auto internal_reg=genMachineVReg();
                    cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src);
                    cur_block->InsertInst(cur_inst);          
                    auto new_internal_reg = new MachineOperand(*internal_reg);
                    src = genMachineVReg(true);
                    cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, src, new_internal_reg);
                    cur_block->InsertInst(cur_inst);         
                }
                else
                {
                    auto internal_reg=genMachineVReg();
                    cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src);
                    cur_block->InsertInst(cur_inst);
                    src=new MachineOperand(*internal_reg);
                }
            }            
            if(!(operands[i+1]->getEntry()->getType()->isTypeFloat()))
            {
                auto mreg=genMachineVReg(true);
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, mreg, src);
                cur_block->InsertInst(cur_inst);
                auto vreg = new MachineOperand(*mreg);
                src = genMachineVReg(true);
                cur_inst = new VcvtMInstruction(cur_block, VcvtMInstruction::ITOF, src, vreg);                
                cur_block->InsertInst(cur_inst);
            }
            auto dst=new MachineOperand(MachineOperand::REG, sregIndex+16);
            dst->setFloat(); 
            cur_inst=new MovMInstruction(cur_block,MovMInstruction::VMOV,dst,src);
            cur_block->InsertInst(cur_inst);
            sregIndex++;
        }
        else if(rregIndex<4)
        {
            auto src=genMachineOperand(operands[i+1]);
            if(src->isImm())
            {
                if(src->isFloat())
                {
                    auto internal_reg=genMachineVReg();
                    cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src);
                    cur_block->InsertInst(cur_inst);          
                    auto new_internal_reg = new MachineOperand(*internal_reg);
                    src = genMachineVReg(true);
                    cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, src, new_internal_reg);
                    cur_block->InsertInst(cur_inst);         
                }
                else
                {
                    auto internal_reg=genMachineVReg();
                    cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src);
                    cur_block->InsertInst(cur_inst);
                    src=new MachineOperand(*internal_reg);
                }
            }
            if(operands[i+1]->getEntry()->getType()->isTypeFloat())
            {
                auto vreg = genMachineVReg(true);
                cur_inst = new VcvtMInstruction(cur_block, VcvtMInstruction::FTOI, vreg, src);
                cur_block->InsertInst(cur_inst);
                auto mreg = new MachineOperand(*vreg);
                src = genMachineVReg();
                cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, src, mreg);
                cur_block->InsertInst(cur_inst);                  
            }
            auto dst=new MachineOperand(MachineOperand::REG, rregIndex);    
            cur_inst=new MovMInstruction(cur_block,MovMInstruction::MOV,dst,src);
            cur_block->InsertInst(cur_inst);
            rregIndex++;            
        }
        else
        {
            params.insert(params.begin(),operands[i+1]);
        }
    }
    if(!params.empty())
    {
        std::vector<MachineOperand*>args;
        for(long unsigned int i=0;i<params.size();++i)
        {
            args.clear();
            auto arg=genMachineOperand(params[i]);
            if(arg->isImm())
            {
                if(params[i]->getEntry()->getType()->isTypeFloat())
                {
                    auto internal_reg=genMachineVReg();
                    cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,arg);
                    cur_block->InsertInst(cur_inst);
                    auto new_internal_reg = new MachineOperand(*internal_reg);
                    arg = genMachineVReg(true);
                    cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, arg, new_internal_reg);
                    cur_block->InsertInst(cur_inst);
                }
                else
                {
                    MachineOperand* internal_reg = genMachineVReg();
                    cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg, arg);
                    cur_block->InsertInst(cur_inst);
                    arg = new MachineOperand(*internal_reg);                   
                }
            }
            args.insert(args.begin(),arg);        
            cur_inst=new StackMInstrcution(cur_block,StackMInstrcution::PUSH,args);
            cur_block->InsertInst(cur_inst);            
        }
    }
    cur_inst=new BranchMInstruction(cur_block, BranchMInstruction::BL, new MachineOperand(func_se->getName()));
    cur_block->InsertInst(cur_inst);
    if(!(dynamic_cast<FunctionType*>(func_se->getType())->getRetType()->isVoid()))
    {
        // mov r0, dst
        if(dynamic_cast<FunctionType*>(func_se->getType())->getRetType()->isFloat())
        {
            auto dst=genMachineOperand(operands[0]);
            auto src=new MachineOperand(MachineOperand::REG,16);
            src->setFloat();
            cur_inst=new MovMInstruction(cur_block,MovMInstruction::VMOV,dst,src);
            cur_block->InsertInst(cur_inst);
        }
        else
        {
            auto dst=genMachineOperand(operands[0]);
            auto src=new MachineOperand(MachineOperand::REG,0);
            cur_inst=new MovMInstruction(cur_block,MovMInstruction::MOV,dst,src);
            cur_block->InsertInst(cur_inst);            
        }

    }
    if(!params.empty())
    {
        auto src1=genMachineReg(13);
        auto src2=genMachineImm(4*(params.size()));
        if(src2->getVal()>255 || src2->getVal()<-255)
        {
            auto internal_reg = genMachineVReg();
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg, src2);
            cur_block->InsertInst(cur_inst);
            src2 = new MachineOperand(*internal_reg);
        }
        auto dst=genMachineReg(13);
        cur_inst=new BinaryMInstruction(cur_block, BinaryMInstruction::ADD, dst, src1, src2);
        cur_block->InsertInst(cur_inst);
    }
}

void XorInstruction::genMachineCode(AsmBuilder* builder)
{
    //TODO
    auto cur_block=builder->getBlock();
    auto dst =genMachineOperand(operands[0]);
    auto true_operand=genMachineImm(1);
    auto false_operand=genMachineImm(0);
    auto cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, true_operand, MachineInstruction::EQ);
    cur_block->InsertInst(cur_inst);
    cur_inst = new MovMInstruction(cur_block, MovMInstruction::MOV, dst, false_operand, MachineInstruction::NE);
    cur_block->InsertInst(cur_inst);
}

void ZextInstruction::genMachineCode(AsmBuilder* builder)
{
    //TODO
    auto cur_block=builder->getBlock();
    MachineInstruction* cur_inst=nullptr;
    auto dst=genMachineOperand(operands[0]);
    auto src=genMachineOperand(operands[1]);
    if(src->isImm())
    {
        auto internal_reg=genMachineVReg();
        cur_inst=new LoadMInstruction(cur_block,LoadMInstruction::LDR, internal_reg,src);
        cur_block->InsertInst(cur_inst);
        src=new MachineOperand(*internal_reg);
    }
    cur_inst = new ZextMInstruction(cur_block, dst, src);
    cur_block->InsertInst(cur_inst);

}

void CastInstruction::genMachineCode(AsmBuilder* builder)
{
    auto cur_block=builder->getBlock();
    MachineInstruction* cur_inst=nullptr;        
    auto dst=genMachineOperand(operands[0]);
    auto src=genMachineOperand(operands[1]);   
    switch (this->opcode)
    {
    case CastInstruction::ITOF:
    {
        if(src->isImm())
        {
            auto internal_reg=genMachineVReg();
            cur_inst=new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg,src);
            cur_block->InsertInst(cur_inst);
            src=new MachineOperand(*internal_reg);            
        }
        auto mreg=genMachineVReg(true);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, mreg, src);
        cur_block->InsertInst(cur_inst);
        auto vreg = new MachineOperand(*mreg);
        cur_inst = new VcvtMInstruction(cur_block, VcvtMInstruction::ITOF, dst, vreg);
        cur_block->InsertInst(cur_inst);
        break;
    }  
    case CastInstruction::FTOI:
    {
        if (src->isImm()) 
        {
            auto internal_reg= genMachineVReg();
            cur_inst = new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg, src);
            cur_block->InsertInst(cur_inst);
            auto new_internal_reg = new MachineOperand(*internal_reg);
            src = genMachineVReg(true);
            cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, src, new_internal_reg);
            cur_block->InsertInst(cur_inst);
        }
        auto vreg = genMachineVReg(true);
        cur_inst = new VcvtMInstruction(cur_block, VcvtMInstruction::FTOI, vreg, src);
        cur_block->InsertInst(cur_inst);
        auto mreg = new MachineOperand(*vreg);
        cur_inst = new MovMInstruction(cur_block, MovMInstruction::VMOV, dst, mreg);
        cur_block->InsertInst(cur_inst);
        break;
    }
    default:
        break;
    }   
}

GepInstruction::GepInstruction(Operand *dst, Operand *base, std::vector<Operand *> offs, BasicBlock *insert_bb) : Instruction(GEP, insert_bb)
{
    operands.push_back(dst);
    operands.push_back(base);
    dst->setDef(this);
    base->addUse(this);
    for (auto off : offs)
    {
        operands.push_back(off);
        off->addUse(this);
    }
}

void GepInstruction::output() const
{
    Operand *dst = operands[0];
    Operand *base = operands[1];
    std::string arrType = base->getType()->toStr();
    fprintf(yyout, "  %s = getelementptr inbounds %s, %s %s, i32 0",
            dst->toStr().c_str(), arrType.substr(0, arrType.size() - 1).c_str(),
            arrType.c_str(), base->toStr().c_str());
    for (unsigned long int i = 2; i < operands.size(); i++)
    {
        fprintf(yyout, ", i32 %s", operands[i]->toStr().c_str());
    }
    fprintf(yyout, "\n");
}

void GepInstruction::genMachineCode(AsmBuilder *builder)
{
    auto cur_block = builder->getBlock();
    auto dst = genMachineOperand(operands[0]);
    auto base = genMachineVReg();
    // 全局变量，先load
    if (operands[1]->getEntry()->isVariable() && dynamic_cast<IdentifierSymbolEntry *>(operands[1]->getEntry())->isGlobal())
    {
        auto src = genMachineOperand(operands[1]);
        cur_block->InsertInst(new LoadMInstruction(cur_block,LoadMInstruction::LDR,  base, src));
        base = new MachineOperand(*base);
    }
    // 偏移
    int offset = ((TemporarySymbolEntry *)operands[1]->getEntry())->getOffset();
    if (offset >= -255)
    {
        cur_block->InsertInst(new BinaryMInstruction(cur_block, BinaryMInstruction::ADD, base, genMachineReg(11), genMachineImm(offset)));
        base = new MachineOperand(*base);
    }
    auto src2 = genMachineImm(offset);
    if(offset<-255)
    {
        auto internal_reg = genMachineVReg();
        cur_block->InsertInst(new LoadMInstruction(cur_block, LoadMInstruction::LDR, internal_reg, src2));
        src2 = new MachineOperand(*internal_reg);
        cur_block->InsertInst(new BinaryMInstruction(cur_block, BinaryMInstruction::ADD, base, genMachineReg(11), src2));
        base = new MachineOperand(*base);
    }
    cur_block->InsertInst(new MovMInstruction(cur_block, MovMInstruction::MOV, dst, base));
}