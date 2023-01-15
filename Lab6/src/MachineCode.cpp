#include "MachineCode.h"
#include "string.h"
#include <queue>
extern FILE* yyout;
int MachineBlock::label=0;

MachineOperand::MachineOperand(int tp, int val)
{
    this->type = tp;
    this->isfloat=false;
    this->fval=0.0;
    if(tp == MachineOperand::IMM)
        this->val = val;
    else 
        this->reg_no = val;
}

MachineOperand::MachineOperand(std::string label)
{
    this->type = MachineOperand::LABEL;
    this->label = label;
}

bool MachineOperand::operator==(const MachineOperand&a) const
{
    if (this->type != a.type)
        return false;
    if (this->type == IMM)
        return this->val == a.val;
    return this->reg_no == a.reg_no;
}

bool MachineOperand::operator<(const MachineOperand&a) const
{
    if(this->type == a.type)
    {
        if(this->type == IMM)
            return this->val < a.val;
        return this->reg_no < a.reg_no;
    }
    return this->type < a.type;

    if (this->type != a.type)
        return false;
    if (this->type == IMM)
        return this->val == a.val;
    return this->reg_no == a.reg_no;
}

void MachineOperand::PrintReg()
{
    switch (reg_no)
    {
    case 11:
        fprintf(yyout, "fp");
        break;
    case 13:
        fprintf(yyout, "sp");
        break;
    case 14:
        fprintf(yyout, "lr");
        break;
    case 15:
        fprintf(yyout, "pc");
        break;
    default:
        {
            if(isfloat)
            {
                fprintf(yyout, "s%d", reg_no-16);
            }
            else
            {
                fprintf(yyout, "r%d", reg_no);
            }
            break;
        }
    }
}

void MachineOperand::output() 
{
    /* HINT：print operand
    * Example:
    * immediate num 1 -> print #1;
    * register 1 -> print r1;
    * lable addr_a -> print addr_a; */
    switch (this->type)
    {
    case IMM:
        if(this->isfloat)
        {
            fprintf(yyout, "#%u", reinterpret_cast<uint32_t&>(this->fval));
        }
        else
        {
            fprintf(yyout, "#%d", this->val);
        }   
        break;
    case VREG:
        fprintf(yyout, "v%d", this->reg_no);
        break;
    case REG:
        PrintReg();
        break;
    case LABEL:
        if (this->label.substr(0, 2) == ".L")
            fprintf(yyout, "%s", this->label.c_str());
        else if(this->label.substr(0, 1) == "@")
            fprintf(yyout, "addr_%s%d", this->label.erase(0,1).c_str(), parent->getParent()->getParent()->getParent()->getUnitInstCount());
        else
            // func or lib func
            fprintf(yyout, "%s", this->label.c_str());
    default:
        break;
    }
}

void MachineInstruction::PrintCond()
{
    // TODO
    switch (cond)
    {
    case EQ:
        fprintf(yyout, "eq");
        break;  
    case NE:
        fprintf(yyout, "ne");
        break; 
    case LT:
        fprintf(yyout, "lt");
        break;
    case LE:
        fprintf(yyout, "le");
        break;
    case GT:
        fprintf(yyout, "gt");
        break;
    case GE:
        fprintf(yyout, "ge");
        break;
    default:
        break;
    }
}

BinaryMInstruction::BinaryMInstruction(
    MachineBlock* p, int op, 
    MachineOperand* dst, MachineOperand* src1, MachineOperand* src2, 
    int cond)
{
    this->parent = p;
    this->type = MachineInstruction::BINARY;
    this->op = op;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src1);
    this->use_list.push_back(src2);
    dst->setParent(this);
    src1->setParent(this);
    src2->setParent(this);
}

void BinaryMInstruction::output() 
{
    // TODO: 
    // Complete other instructions
    switch (this->op)
    {
    case BinaryMInstruction::ADD:
        fprintf(yyout, "\tadd ");
        break;
    case BinaryMInstruction::SUB:
        fprintf(yyout, "\tsub ");
        break;
    case BinaryMInstruction::MUL:
        fprintf(yyout, "\tmul ");
        break;
    case BinaryMInstruction::DIV:
        fprintf(yyout, "\tsdiv ");
        break;
    case BinaryMInstruction::FADD:
        fprintf(yyout, "\tvadd.f32 ");
        break;
    case BinaryMInstruction::FSUB:
        fprintf(yyout, "\tvsub.f32 ");
        break;
    case BinaryMInstruction::FMUL:
        fprintf(yyout, "\tvmul.f32 ");
        break;
    case BinaryMInstruction::FDIV:
        fprintf(yyout, "\tvdiv.f32 ");
        break;
    case BinaryMInstruction::AND:
        fprintf(yyout, "\tand ");
        break;
    case BinaryMInstruction::OR:
        fprintf(yyout, "\torr ");
        break;
    default:
        break;
    }
    this->PrintCond();
    this->def_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[1]->output();
    fprintf(yyout, "\n");
}


LoadMInstruction::LoadMInstruction(MachineBlock* p,int op, MachineOperand* dst, MachineOperand* src1, MachineOperand* src2,
    int cond)
{
    this->parent = p;
    this->type = MachineInstruction::LOAD;
    this->op = op;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src1);
    if (src2)
        this->use_list.push_back(src2);
    dst->setParent(this);
    src1->setParent(this);
    if (src2)
        src2->setParent(this);
}

void LoadMInstruction::output()
{
    switch (op)
    {
    case LoadMInstruction::LDR:
        fprintf(yyout, "\tldr ");
        break;
    case LoadMInstruction::FLDR:
        fprintf(yyout, "\tvldr.32 ");
        break;    
    default:
        break;
    }
   
    this->def_list[0]->output();
    fprintf(yyout, ", ");

    // Load immediate num, eg: ldr r1, =8
    if(this->use_list[0]->isImm())
    {
        if(this->use_list[0]->isFloat())
        {
            float v=this->use_list[0]->getFVal();
            fprintf(yyout, "=%u\n", reinterpret_cast<uint32_t&>(v));
        }
        else
        {
            fprintf(yyout, "=%d\n", this->use_list[0]->getVal());
        }
        return;
    }

    // Load address
    if(this->use_list[0]->isReg()||this->use_list[0]->isVReg())
        fprintf(yyout, "[");

    this->use_list[0]->output();
    if( this->use_list.size() > 1 )
    {
        fprintf(yyout, ", ");
        if(use_list[0]->isReg() && use_list[0]->getReg()==11
        && use_list[1]->isImm() && use_list[1]->getVal()>=0)
        {;
            auto off=this->getParent()->getParent()->getSavedRegsSize();
            off+=2;
            auto r_off=use_list[1]->getVal();
            use_list[1]->setVal(r_off+off*4);
            //printf("from %d to %d\n",r_off,r_off+off*4);
        }
        this->use_list[1]->output();
    }

    if(this->use_list[0]->isReg()||this->use_list[0]->isVReg())
        fprintf(yyout, "]");
    fprintf(yyout, "\n");
}

StoreMInstruction::StoreMInstruction(MachineBlock* p, int op, 
    MachineOperand* src1, MachineOperand* src2, MachineOperand* src3, 
    int cond)
{
    // TODO
    this->parent=p;
    this->type = MachineInstruction::STORE;
    this->op = op;
    this->cond = cond;
    this->use_list.push_back(src1);
    this->use_list.push_back(src2);
    if(src3)
        this->use_list.push_back(src3);
    src1->setParent(this);
    src2->setParent(this);
    if(src3)
        src3->setParent(this);
}

void StoreMInstruction::output()
{
    // TODO
    switch (op)
    {
    case StoreMInstruction::STR:
        fprintf(yyout, "\tstr ");
        break;
    case StoreMInstruction::FSTR:
        fprintf(yyout, "\tvstr.32 ");
        break;        
    default:
        break;
    }
    this->use_list[0]->output();
    fprintf(yyout, ", ");

    // Store address
    if(this->use_list[1]->isReg()||this->use_list[1]->isVReg())
        fprintf(yyout, "[");

    this->use_list[1]->output();
    if( this->use_list.size() > 2 )
    {
        fprintf(yyout, ", ");
        this->use_list[2]->output();
    }

    if(this->use_list[1]->isReg()||this->use_list[1]->isVReg())
        fprintf(yyout, "]");
    fprintf(yyout, "\n");
}


MovMInstruction::MovMInstruction(MachineBlock* p, int op, 
    MachineOperand* dst, MachineOperand* src,
    int cond)
{
    // TODO
    this->parent=p;
    this->type=MachineInstruction::MOV;
    this->cond=cond;
    this->op=op;
    this->def_list.push_back(dst);
    this->use_list.push_back(src);
    dst->setParent(this);
    src->setParent(this);    
}

void MovMInstruction::output() 
{
    // TODO
    switch (this->op)
    {
    case MOV:
        fprintf(yyout, "\tmov");
        break;
    case MVN:
        fprintf(yyout, "\tmvn");
        break;
    case MOVT:
        fprintf(yyout, "\tmovt");
        break;        
    case VMOV:
        fprintf(yyout, "\tvmov");
        break;        
    case FMOV:
        fprintf(yyout, "\tvmov.f32");
        break;
    default:
        break; 
    }
    PrintCond();
    fprintf(yyout, " ");
    this->def_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[0]->output();
    fprintf(yyout, "\n");
}

BranchMInstruction::BranchMInstruction(MachineBlock* p, int op, 
    MachineOperand* dst, 
    int cond)
{
    // TODO
    this->parent=p;
    this->type=MachineInstruction::BRANCH;
    this->cond=cond;
    this->op=op;
    this->def_list.push_back(dst);
    dst->setParent(this);
}

void BranchMInstruction::output()
{
    // TODO
    switch(this->op)
    {
    case B:
        fprintf(yyout, "\tb");
        PrintCond();
        break;
    case BL:
        fprintf(yyout, "\tbl");
        break;
    case BX:
        fprintf(yyout, "\tbx");
        break;
    default:
        break;
    }
    fprintf(yyout, "\t");
    this->def_list[0]->output();
    fprintf(yyout, "\n");
}

CmpMInstruction::CmpMInstruction(MachineBlock* p, int op, 
    MachineOperand* src1, MachineOperand* src2, 
    int cond)
{
    // TODO
    this->parent = p;
    this->type = MachineInstruction::CMP;
    this->op = op;
    this->cond = cond;
    this->use_list.push_back(src1);
    this->use_list.push_back(src2);
    src1->setParent(this);
    src2->setParent(this);
}

void CmpMInstruction::output()
{
    // TODO
    // Jsut for reg alloca test
    // delete it after test
    switch (this->op)
    {
    case CmpMInstruction::CMP:
        fprintf(yyout, "\tcmp ");
        break;
    case CmpMInstruction::FCMP:
        fprintf(yyout, "\tvcmp.f32 ");
        break;        
    default:
        break;
    }
    
    this->use_list[0]->output();
    fprintf(yyout, ", ");
    this->use_list[1]->output();
    fprintf(yyout, "\n");  
    if(this->op==CmpMInstruction::FCMP)
        fprintf(yyout, "vmrs APSR_nzcv, FPSCR\n");
}

ZextMInstruction::ZextMInstruction(MachineBlock* p, 
    MachineOperand* dst, MachineOperand* src, 
    int cond)
{
    // TODO
    this->parent = p;
    this->type = MachineInstruction::ZEXT;
    this->op=-1;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src);
    dst->setParent(this);
    src->setParent(this);
}

void ZextMInstruction::output()
{
    fprintf(yyout, "\tuxtb ");
    def_list[0]->output();
    fprintf(yyout, ", ");
    use_list[0]->output();
    fprintf(yyout, "\n");
}

VcvtMInstruction::VcvtMInstruction(MachineBlock* p, int op,
    MachineOperand* dst, MachineOperand* src, 
    int cond)
{
    // TODO
    this->parent = p;
    this->type = MachineInstruction::VCVT;
    this->op=op;
    this->cond = cond;
    this->def_list.push_back(dst);
    this->use_list.push_back(src);
    dst->setParent(this);
    src->setParent(this);
}

void VcvtMInstruction::output()
{
    switch (this->op)
    {
    case VcvtMInstruction::ITOF:
        fprintf(yyout, "\tvcvt.f32.s32 ");
        break;
    case VcvtMInstruction::FTOI:
        fprintf(yyout, "\tvcvt.s32.f32 ");
        break;    
    default:
        break;
    }
    def_list[0]->output();
    fprintf(yyout, ", ");
    use_list[0]->output();
    fprintf(yyout, "\n");
}

StackMInstrcution::StackMInstrcution(MachineBlock* p, int op, 
    std::vector<MachineOperand*> srcs,
    int cond)
{
    // TODO
    this->parent=p;
    this->type=MachineInstruction::STACK;
    this->op=op;
    this->cond=cond;
    for(auto src=srcs.begin();src!=srcs.end();++src)
    {
        this->use_list.push_back(*src);
        (*src)->setParent(this);
    } 
}

void StackMInstrcution::output()
{
    // TODO
    switch (this->op)
    {
    case PUSH:
        fprintf(yyout, "\tpush {");
        break;
    case POP:
        fprintf(yyout, "\tpop {");
        break;
    case FPUSH:
        fprintf(yyout, "\tvpush {");
        break;
    case FPOP:
        fprintf(yyout, "\tvpop {");
        break;
    default:
        break;
    }
    auto src=use_list.begin();
    if(src!=use_list.end())
    {
        (*src)->output();
        ++src;
        for(;src!=use_list.end();++src)
        {
            fprintf(yyout, ", ");
            (*src)->output(); 
        }
    }
    fprintf(yyout, "}\n");
}

MachineFunction::MachineFunction(MachineUnit* p, SymbolEntry* sym_ptr) 
{ 
    this->parent = p; 
    this->sym_ptr = sym_ptr; 
    this->stack_size = 0;
};

void MachineBlock::InsertInstBefore(MachineInstruction* point,MachineInstruction* inst)
{
    auto pos=find(inst_list.begin(),inst_list.end(),point);
    inst_list.insert(pos,inst);
}

void MachineBlock::InsertInstAfter(MachineInstruction* point,MachineInstruction* inst)
{
    auto pos=find(inst_list.begin(),inst_list.end(),point);
    if(pos==inst_list.end())
    {
        inst_list.push_back(inst);
        return;
    }
    ++pos;
    inst_list.insert(pos,inst);
}

void MachineBlock::output()
{
    if(inst_list.size()==0)
        return;
    int instCount=0;
    fprintf(yyout, ".L%d:\n", this->no);
    auto cur_func=parent;
    auto saved_regs=cur_func->getSavedRegs();
    saved_regs.insert(11);
    saved_regs.insert(14);
    std::vector<MachineOperand*> rregs;
    std::vector<MachineOperand*> sregs;
    for(auto i=saved_regs.begin();i!=saved_regs.end();++i)
    {
        if((*i)<16)
        {
            auto reg=new MachineOperand(MachineOperand::REG,(*i));
            rregs.push_back(reg);
        }
        else
        {
            auto reg=new MachineOperand(MachineOperand::REG,(*i));
            reg->setFloat();
            sregs.push_back(reg);           
        }

    }
    auto sp=new MachineOperand(MachineOperand::REG,13);
    auto stack_size=parent->getStackSize();
    for(auto iter : inst_list)
    {
        if(iter->isBx())
        {
            auto off=new MachineOperand(MachineOperand::IMM,stack_size);
            if(stack_size>255 || stack_size<-255)
            {
                // at the end of function,r1 must be useless
                auto r1=new MachineOperand(MachineOperand::REG,1);
                auto loadInst=new LoadMInstruction(this, LoadMInstruction::LDR, r1, off);
                InsertInstBefore(iter,loadInst);
                loadInst->output();
                off=new MachineOperand(*r1);
            }
            // must be here,the size of stack may has been changed
            auto addInst=new BinaryMInstruction(this,BinaryMInstruction::ADD,sp,sp,off);
            InsertInstBefore(iter,addInst);
            addInst->output();            
            if(!sregs.empty())
            {
                auto fstackInst=new StackMInstrcution(this,StackMInstrcution::FPOP,sregs);
                InsertInstBefore(iter,fstackInst);
                fstackInst->output();                
            }
            if(!rregs.empty())
            {
                auto stackInst=new StackMInstrcution(this,StackMInstrcution::POP,rregs);
                InsertInstBefore(iter,stackInst);
                stackInst->output();                
            }

        }
        iter->output();
        instCount++;
        if(instCount>500)
        {
            fprintf(yyout, "\tb .B%d\n", label);
            fprintf(yyout, ".LTORG\n");
            parent->getParent()->PrintBridgeLabel();
            fprintf(yyout, ".B%d:\n", label++);
            instCount=0;
        }
    }
        
}

void MachineFunction::output()
{
    fprintf(yyout, "\t.text\n");
    fprintf(yyout, "\t.global %s\n", this->sym_ptr->toStr().c_str() + 1);
    fprintf(yyout, "\t.type %s , %%function\n", this->sym_ptr->toStr().c_str() + 1);
    fprintf(yyout, "%s:\n", this->sym_ptr->toStr().c_str() + 1);
    // TODO
    /* Hint:
    *  1. Save fp
    *  2. fp = sp
    *  3. Save callee saved register
    *  4. Allocate stack space for local variable */
    std::set<int>rregs;
    std::set<int>sregs;
    for(auto regNo=saved_regs.begin();regNo!=saved_regs.end();++regNo)
    {
        if((*regNo)<16)
            rregs.insert((*regNo));
        else
            sregs.insert((*regNo));
    }
    fprintf(yyout, "\tpush {");
    if(!rregs.empty())
    {
        for(auto regNo=rregs.begin();regNo!=rregs.end();++regNo)
        {
            auto reg=new MachineOperand(MachineOperand::REG,(*regNo));
            reg->output();
            fprintf(yyout,", ");
        }        
    }
    fprintf(yyout,"fp, lr}\n");
    if(!sregs.empty())
    {
        fprintf(yyout, "\tvpush {");
        auto regNo=sregs.begin();
        if(regNo!=sregs.end())
        {
            auto reg=new MachineOperand(MachineOperand::REG,(*regNo));
            reg->setFloat();
            reg->output();            
        }
        ++regNo;
        for(;regNo!=sregs.end();++regNo)
        {

            fprintf(yyout,", ");
            auto reg=new MachineOperand(MachineOperand::REG,(*regNo));
            reg->setFloat();
            reg->output();  
        }        
        fprintf(yyout,"}\n");
    }
    fprintf(yyout, "\tmov fp, sp\n");
    if(stack_size!=0)
    {
        //printf("first:%d\n",getStackSize());
        if(stack_size>255 || stack_size<-255)
        {
            fprintf(yyout, "\tldr r4,=%d\n", stack_size);
            fprintf(yyout, "\tsub sp, sp, r4\n");
        }
        else
        {
            fprintf(yyout, "\tsub sp, sp, #%d\n", stack_size);
        }    
    }
    // Traverse all the block in block_list to print assembly code.
    /*for(auto iter : block_list)
        iter->output();*/
    int insCnt=0;
    std::queue<MachineBlock*> q;
    std::set<MachineBlock*> v;
    q.push(block_list[0]);
    v.insert(block_list[0]);
    while(!q.empty()) {
        MachineBlock* cur = q.front();
        q.pop();
        cur->output();
        insCnt+=cur->getSize();
        if(insCnt>200)
        {
            fprintf(yyout, "\tb .F%d\n", parent->getUnitInstCount());
            fprintf(yyout, ".LTORG\n");
            parent->PrintBridgeLabel();
            fprintf(yyout, ".F%d:\n", parent->getUnitInstCount()-1);
            insCnt=0;
        }
        for(auto iter : cur->getSuccs()) {
            if(v.find(iter) == v.end()) {
                q.push(iter);
                v.insert(iter);
            }
        }
    }
}

void MachineUnit::PrintGlobalDecl()
{
    // TODO:
    // You need to print global variable/const declarition code;
    if(!decl_list.empty())
    {
        fprintf(yyout, "\t.data\n");
        for(auto id=decl_list.begin();id!=decl_list.end();++id)
        {
            int size=0;
            if(((*id)->getType())->isTypeInt()||((*id)->getType())->isTypeFloat())
            {
                size=4;
            }
            else if(((*id)->getType())->isBool())
            {
                size=1;
            }
            else if(((*id)->getType())->isIntArray()){
                size=dynamic_cast<IntArrayType*>((*id)->getType())->getSize()/8;
            }
            else if(((*id)->getType())->isFloatArray()){
                size=dynamic_cast<FloatArrayType*>((*id)->getType())->getSize()/8;
            }
            else if(((*id)->getType())->isConstIntArray()){
                size=dynamic_cast<ConstIntArrayType*>((*id)->getType())->getSize()/8;
            }
            else if(((*id)->getType())->isConstFloatArray()){
                size=dynamic_cast<ConstFloatArrayType*>((*id)->getType())->getSize()/8;
            }
            else
            {
                size=0;
            }
            if(((*id)->getType())->isArray() && dynamic_cast<IdentifierSymbolEntry*>(*id)->arrayValue.empty()){
                fprintf(yyout, "\t.comm\t%s,%d,4\n", (*id)->toStr().erase(0,1).c_str(), size);
            }
            else{
                fprintf(yyout,"\t.global %s\n",(*id)->toStr().erase(0,1).c_str());
                fprintf(yyout, "\t.align 4\n");
                
                fprintf(yyout,"\t.size %s, %d\n",(*id)->toStr().erase(0,1).c_str(),size);
                fprintf(yyout,"%s:\n", (*id)->toStr().erase(0,1).c_str());
                //fprintf(yyout, "\t.word %d\n", int((dynamic_cast<IdentifierSymbolEntry*>(*id))->getValue()));
                if(!((*id)->getType())->isArray()){
                    if(((*id)->getType())->isTypeInt())
                        {
                            fprintf(yyout, "\t.word %d\n", int((dynamic_cast<IdentifierSymbolEntry*>(*id))->getValue()));
                        }
                        else if(((*id)->getType())->isTypeFloat())
                        {
                            float v=(dynamic_cast<IdentifierSymbolEntry*>(*id))->getValue();
                            fprintf(yyout, "\t.word %u\n", reinterpret_cast<uint32_t&>(v));
                        }
                        else
                        {
                            exit(0);
                        }
                }
                else{
                    for (auto value: dynamic_cast<IdentifierSymbolEntry*>(*id)->arrayValue) {
                        fprintf(yyout, "\t.word %d\n", int(value));
                    }
                }
            }
            
            
        }
    }
}

void MachineUnit::PrintBridgeLabel()
{
    for(auto id=decl_list.begin();id!=decl_list.end();++id)
    {
        fprintf(yyout,"addr_%s%d:\n",(*id)->toStr().erase(0,1).c_str(),unitInstCount);
        fprintf(yyout,"\t.word %s\n",(*id)->toStr().erase(0,1).c_str());
    }
    unitInstCount++;
}

void MachineUnit::output()
{
    // TODO
    /* Hint:
    * 1. You need to print global variable/const declarition code;
    * 2. Traverse all the function in func_list to print assembly code;
    * 3. Don't forget print bridge label at the end of assembly code!! */
    fprintf(yyout, "\t.arch armv8-a\n");
    fprintf(yyout, "\t.arch_extension crc\n");
    fprintf(yyout, "\t.arm\n");
    PrintGlobalDecl();
    for(auto iter : func_list)
        iter->output();
    PrintBridgeLabel();
}

bool MachineInstruction::isBx()
{
    return type==BRANCH && op==BranchMInstruction::BX;
}