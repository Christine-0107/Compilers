#ifndef __MACHINECODE_H__
#define __MACHINECODE_H__
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <fstream>
#include "SymbolTable.h"
#include "Type.h"

/* Hint:
* MachineUnit: Compiler unit
* MachineFunction: Function in assembly code 
* MachineInstruction: Single assembly instruction  
* MachineOperand: Operand in assembly instruction, such as immediate number, register, address label */

/* Todo:
* We only give the example code of "class BinaryMInstruction" and "class AccessMInstruction" (because we believe in you !!!),
* You need to complete other the member function, especially "output()" ,
* After that, you can use "output()" to print assembly code . */

class MachineUnit;
class MachineFunction;
class MachineBlock;
class MachineInstruction;
class BranchMInstruction;

class MachineOperand
{
private:
    MachineInstruction* parent;
    int type;
    int val;  // value of immediate number
    // must add a var
    // can't transfer from float to val safely
    float fval;
    bool isfloat;
    int reg_no; // register no
    std::string label; // address label
public:
    enum { IMM, VREG, REG, LABEL };
    MachineOperand(int tp, int val);
    MachineOperand(std::string label);
    bool operator == (const MachineOperand&) const;
    bool operator < (const MachineOperand&) const;
    bool isImm() { return this->type == IMM; }; 
    bool isReg() { return this->type == REG; };
    bool isVReg() { return this->type == VREG; };
    bool isLabel() { return this->type == LABEL; };
    int getVal() {return this->val; };
    float getFVal() {return this->fval;};
    int getReg() {return this->reg_no;};
    void setVal(int s_val) {this->val=s_val;};
    void setFVal(float f_val) {this->fval=f_val;this->isfloat=true;};
    void setReg(int regno) {this->type = REG; this->reg_no = regno;};
    bool isFloat() {return this->isfloat;};
    void setFloat() {this->isfloat=true;};
    std::string getLabel() {return this->label; };
    void setParent(MachineInstruction* p) { this->parent = p; };
    MachineInstruction* getParent() { return this->parent;};
    void PrintReg();
    void output();
};

class MachineInstruction
{
protected:
    MachineBlock* parent;
    int no;
    int type;  // Instruction type
    int cond = MachineInstruction::NONE;  // Instruction execution condition, optional !!
    int op;  // Instruction opcode
    // Instruction operand list, sorted by appearance order in assembly instruction
    std::vector<MachineOperand*> def_list;
    std::vector<MachineOperand*> use_list;
    void addDef(MachineOperand* ope) { def_list.push_back(ope); };
    void addUse(MachineOperand* ope) { use_list.push_back(ope); };
    // Print execution code after printing opcode
    void PrintCond();
    enum instType { BINARY, LOAD, STORE, MOV, BRANCH, CMP, ZEXT, STACK, VCVT };
public:
    enum condType { EQ, NE, LT, LE , GT, GE, NONE };
    virtual void output() = 0;
    void setNo(int no) {this->no = no;};
    int getNo() {return no;};
    int getType() {return type;};
    std::vector<MachineOperand*>& getDef() {return def_list;};
    std::vector<MachineOperand*>& getUse() {return use_list;};
    MachineBlock* getParent() {return parent;};
    bool isBx();
};

class BinaryMInstruction : public MachineInstruction
{
public:
    enum opType { ADD, SUB, MUL, DIV, FADD, FSUB, FMUL, FDIV, MOD, AND, OR };
    BinaryMInstruction(MachineBlock* p, int op, 
                    MachineOperand* dst, MachineOperand* src1, MachineOperand* src2, 
                    int cond = MachineInstruction::NONE);
    void output();
};

class LoadMInstruction : public MachineInstruction
{
public:
    enum opType { LDR, FLDR};
    LoadMInstruction(MachineBlock* p, int op,
                    MachineOperand* dst, MachineOperand* src1, MachineOperand* src2 = nullptr, 
                    int cond = MachineInstruction::NONE);
    void output();
};

class StoreMInstruction : public MachineInstruction
{
public:
    enum opType { STR, FSTR};
    StoreMInstruction(MachineBlock* p, int op,
                    MachineOperand* src1, MachineOperand* src2, MachineOperand* src3 = nullptr, 
                    int cond = MachineInstruction::NONE);
    void output();
};

class MovMInstruction : public MachineInstruction
{
public:
    enum opType { MOV, MVN, MOVT, VMOV, FMOV};
    MovMInstruction(MachineBlock* p, int op, 
                MachineOperand* dst, MachineOperand* src,
                int cond = MachineInstruction::NONE);
    void output();
};


class BranchMInstruction : public MachineInstruction
{
public:
    enum opType { B, BL, BX };
    BranchMInstruction(MachineBlock* p, int op, 
                MachineOperand* dst, 
                int cond = MachineInstruction::NONE);
    void output();
};

class CmpMInstruction : public MachineInstruction
{
public:
    enum opType { CMP, FCMP};
    CmpMInstruction(MachineBlock* p, int op, 
                MachineOperand* src1, MachineOperand* src2, 
                int cond = MachineInstruction::NONE);
    void output();
};

class ZextMInstruction : public MachineInstruction
{
public:
    enum opType { ZEXT };
    ZextMInstruction(MachineBlock* p, 
                MachineOperand* dst, MachineOperand* src, 
                int cond = MachineInstruction::NONE);
    void output();
};

class StackMInstrcution : public MachineInstruction
{
public:
    enum opType { PUSH, POP, FPUSH, FPOP};
    StackMInstrcution(MachineBlock* p, int op, 
                std::vector<MachineOperand*> srcs,
                int cond = MachineInstruction::NONE);
    void output();
};

class VcvtMInstruction : public MachineInstruction
{
public:
    enum opType {ITOF, FTOI};
    VcvtMInstruction(MachineBlock* p, int op, 
                MachineOperand* dst, MachineOperand* src, 
                int cond = MachineInstruction::NONE);
    void output();
};


class MachineBlock
{
private:
    MachineFunction* parent;
    int no;  
    int cond;
    std::vector<MachineBlock *> pred, succ;
    std::vector<MachineInstruction*> inst_list;
    std::set<MachineOperand*> live_in;
    std::set<MachineOperand*> live_out;
    static int label;
public:
    std::vector<MachineInstruction*>& getInsts() {return inst_list;};
    std::vector<MachineInstruction*>::iterator begin() { return inst_list.begin(); };
    std::vector<MachineInstruction*>::iterator end() { return inst_list.end(); };
    std::vector<MachineInstruction*>::reverse_iterator rbegin() { return inst_list.rbegin(); };
    MachineFunction* getParent() {return this->parent;};
    MachineBlock(MachineFunction* p, int no) { this->parent = p; this->no = no; };
    void InsertInst(MachineInstruction* inst) { this->inst_list.push_back(inst); };
    void InsertInstBefore(MachineInstruction* point,MachineInstruction* inst);
    void InsertInstAfter(MachineInstruction* point,MachineInstruction* inst);
    void addPred(MachineBlock* p) { this->pred.push_back(p); };
    void addSucc(MachineBlock* s) { this->succ.push_back(s); };
    std::set<MachineOperand*>& getLiveIn() {return live_in;};
    std::set<MachineOperand*>& getLiveOut() {return live_out;};
    std::vector<MachineBlock*>& getPreds() {return pred;};
    std::vector<MachineBlock*>& getSuccs() {return succ;};
    void setCond(int cmpCond) {this->cond=cmpCond;}; 
    int getCond() {return cond;};
    int getSize() {return (int)inst_list.size();};
    void output();
};

class MachineFunction
{
private:
    MachineUnit* parent;
    std::vector<MachineBlock*> block_list;
    int stack_size;
    std::set<int> saved_regs;
    SymbolEntry* sym_ptr;
public:
    std::vector<MachineBlock*>& getBlocks() {return block_list;};
    std::vector<MachineBlock*>::iterator begin() { return block_list.begin(); };
    std::vector<MachineBlock*>::iterator end() { return block_list.end(); };
    MachineFunction(MachineUnit* p, SymbolEntry* sym_ptr);
    /* HINT:
    * Alloc stack space for local variable;
    * return current frame offset ;
    * we store offset in symbol entry of this variable in function AllocInstruction::genMachineCode()
    * you can use this function in LinearScan::genSpillCode() */
    int AllocSpace(int size) { this->stack_size += size; return this->stack_size; };
    int getStackSize() {return this->stack_size;};
    std::set<int> getSavedRegs() {return this->saved_regs;};
    void InsertBlock(MachineBlock* block) { this->block_list.push_back(block); };
    void addSavedRegs(int regno) {saved_regs.insert(regno);};
    int getSavedRegsSize() {return (int)saved_regs.size();};
    MachineUnit* getParent() {return parent;};
    void output();
};

class MachineUnit
{
private:
    int unitInstCount=0;
    std::vector<MachineFunction*> func_list;
    std::vector<SymbolEntry *> decl_list;
    std::vector<SymbolEntry *> libfunc_list;
    void PrintGlobalDecl();
public:
    std::vector<MachineFunction*>& getFuncs() {return func_list;};
    std::vector<MachineFunction*>::iterator begin() { return func_list.begin(); };
    std::vector<MachineFunction*>::iterator end() { return func_list.end(); };
    void InsertFunc(MachineFunction* func) { func_list.push_back(func);};
    void InsertDecl(SymbolEntry *se) {decl_list.push_back(se);};
    void InsertLibFunc(SymbolEntry *se) {libfunc_list.push_back(se);};
    void PrintBridgeLabel();
    int getUnitInstCount() {return this->unitInstCount;};
    void output();
};

#endif