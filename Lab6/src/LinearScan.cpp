#include <algorithm>
#include "LinearScan.h"
#include "MachineCode.h"
#include "LiveVariableAnalysis.h"

LinearScan::LinearScan(MachineUnit *unit)
{
    this->unit = unit;
    for (int i = 4; i < 11; i++)
        regs.push_back(i);
    for (int i = 21; i < 48; i++)
        sregs.push_back(i);
}

void LinearScan::allocateRegisters()
{
    for (auto &f : unit->getFuncs())
    {
        func = f;
        bool success;
        success = false;
        while (!success)        // repeat until all vregs can be mapped
        {
            computeLiveIntervals();
            success = linearScanRegisterAllocation();
            if (success)        // all vregs can be mapped to real regs
                modifyCode();
            else                // spill vregs that can't be mapped to real regs
                genSpillCode();
        }
    }
}

void LinearScan::makeDuChains()
{
    LiveVariableAnalysis lva;
    lva.pass(func);
    du_chains.clear();
    int i = 0;
    std::map<MachineOperand, std::set<MachineOperand *>> liveVar;
    for (auto &bb : func->getBlocks())
    {
        liveVar.clear();
        for (auto &t : bb->getLiveOut())
            liveVar[*t].insert(t);
        int no;
        no = i = bb->getInsts().size() + i;
        for (auto inst = bb->getInsts().rbegin(); inst != bb->getInsts().rend(); inst++)
        {
            (*inst)->setNo(no--);
            for (auto &def : (*inst)->getDef())
            {
                if (def->isVReg())
                {
                    auto &uses = liveVar[*def];
                    du_chains[def].insert(uses.begin(), uses.end());
                    auto &kill = lva.getAllUses()[*def];
                    std::set<MachineOperand *> res;
                    set_difference(uses.begin(), uses.end(), kill.begin(), kill.end(), inserter(res, res.end()));
                    liveVar[*def] = res;
                }
            }
            for (auto &use : (*inst)->getUse())
            {
                if (use->isVReg())
                    liveVar[*use].insert(use);
            }
        }
    }
}

void LinearScan::computeLiveIntervals()
{
    makeDuChains();
    intervals.clear();
    for (auto &du_chain : du_chains)
    {
        int t = -1;
        for (auto &use : du_chain.second)
            t = std::max(t, use->getParent()->getNo());
        Interval *interval = new Interval({du_chain.first->getParent()->getNo(), t, false, 0, 0, du_chain.first->isFloat(), {du_chain.first}, du_chain.second});
        intervals.push_back(interval);
    }
    for (auto& interval : intervals) {
        auto uses = interval->uses;
        auto begin = interval->start;
        auto end = interval->end;
        for (auto block : func->getBlocks()) {
            auto liveIn = block->getLiveIn();
            auto liveOut = block->getLiveOut();
            bool in = false;
            bool out = false;
            for (auto use : uses)
                if (liveIn.count(use)) {
                    in = true;
                    break;
                }
            for (auto use : uses)
                if (liveOut.count(use)) {
                    out = true;
                    break;
                }
            if (in && out) {
                begin = std::min(begin, (*(block->begin()))->getNo());
                end = std::max(end, (*(block->rbegin()))->getNo());
            } else if (!in && out) {
                for (auto i : block->getInsts())
                    if (i->getDef().size() > 0 &&
                        i->getDef()[0] == *(uses.begin())) {
                        begin = std::min(begin, i->getNo());
                        break;
                    }
                end = std::max(end, (*(block->rbegin()))->getNo());
            } else if (in && !out) {
                begin = std::min(begin, (*(block->begin()))->getNo());
                int temp = 0;
                for (auto use : uses)
                    if (use->getParent()->getParent() == block)
                        temp = std::max(temp, use->getParent()->getNo());
                end = std::max(temp, end);
            }
        }
        interval->start = begin;
        interval->end = end;
    }
    bool change;
    change = true;
    while (change)
    {
        change = false;
        std::vector<Interval *> t(intervals.begin(), intervals.end());
        for (size_t i = 0; i < t.size(); i++)
            for (size_t j = i + 1; j < t.size(); j++)
            {
                Interval *w1 = t[i];
                Interval *w2 = t[j];
                if (**w1->defs.begin() == **w2->defs.begin())
                {
                    std::set<MachineOperand *> temp;
                    set_intersection(w1->uses.begin(), w1->uses.end(), w2->uses.begin(), w2->uses.end(), inserter(temp, temp.end()));
                    if (!temp.empty())
                    {
                        change = true;
                        w1->defs.insert(w2->defs.begin(), w2->defs.end());
                        w1->uses.insert(w2->uses.begin(), w2->uses.end());
                        // w1->start = std::min(w1->start, w2->start);
                        // w1->end = std::max(w1->end, w2->end);
                        auto w1Min = std::min(w1->start, w1->end);
                        auto w1Max = std::max(w1->start, w1->end);
                        auto w2Min = std::min(w2->start, w2->end);
                        auto w2Max = std::max(w2->start, w2->end);
                        w1->start = std::min(w1Min, w2Min);
                        w1->end = std::max(w1Max, w2Max);
                        auto it = std::find(intervals.begin(), intervals.end(), w2);
                        if (it != intervals.end())
                            intervals.erase(it);
                    }
                }
            }
    }
    sort(intervals.begin(), intervals.end(), compareStart);
}

bool LinearScan::linearScanRegisterAllocation()
{
    // Todo
    bool ret=true;
    active.clear();
    regs.clear();
    sregs.clear();
    for(int i=4;i<11;++i)
        regs.push_back(i);
    for (int i = 21; i < 48; i++)
        sregs.push_back(i);
    for(auto &interval : intervals)
    {
        expireOldIntervals(interval);
        if(interval->isfloat)
        {
            if(sregs.empty())
            {
                spillAtInterval(interval);
                ret=false;
            }
            else
            {
                interval->rreg=(sregs[sregs.size()-1]);
                sregs.pop_back();
                active.push_back(interval);
                sort(active.begin(),active.end(),compareEnd);
            }
        }
        else
        {
            if(regs.empty())
            {
                spillAtInterval(interval);
                ret=false;
            }
            else
            {
                interval->rreg=(regs[regs.size()-1]);
                regs.pop_back();
                active.push_back(interval);
                sort(active.begin(),active.end(),compareEnd);
            }            
        }

    }
    return ret;
}

void LinearScan::modifyCode()
{
    for (auto &interval : intervals)
    {
        func->addSavedRegs(interval->rreg);
        for (auto def : interval->defs)
            def->setReg(interval->rreg);
        for (auto use : interval->uses)
            use->setReg(interval->rreg);
    }
}

void LinearScan::genSpillCode()
{
    for(auto &interval:intervals)
    {
        if(!interval->spill)
            continue;
        // TODO
        /* HINT:
         * The vreg should be spilled to memory.
         * 1. insert ldr inst before the use of vreg
         * 2. insert str inst after the def of vreg
         */ 
        interval->disp=func->AllocSpace(4);
        for(auto use : interval->uses)
        {
            auto newUse=new MachineOperand(*use);
            MachineBlock* mb=use->getParent()->getParent();
            auto fp=new MachineOperand(MachineOperand::REG,11);
            auto off=new MachineOperand(MachineOperand::IMM,-interval->disp);
            if(use->isFloat())
            {
                auto ldr=new LoadMInstruction(mb,LoadMInstruction::FLDR, newUse,fp,off);
                mb->InsertInstBefore(use->getParent(),ldr);
            }
            else
            {
                auto ldr=new LoadMInstruction(mb,LoadMInstruction::LDR, newUse,fp,off);
                mb->InsertInstBefore(use->getParent(),ldr);
            }
            
        }
        for(auto def : interval->defs)
        {
            auto newDef=new MachineOperand(*def);
            MachineBlock* mb=def->getParent()->getParent();
            auto fp=new MachineOperand(MachineOperand::REG,11);
            auto off=new MachineOperand(MachineOperand::IMM,-interval->disp);
            if(def->isFloat())
            {
                auto str=new StoreMInstruction(mb,StoreMInstruction::FSTR,newDef,fp,off);
                mb->InsertInstAfter(def->getParent(),str);
            }
            else
            {
                auto str=new StoreMInstruction(mb,StoreMInstruction::STR,newDef,fp,off);
                mb->InsertInstAfter(def->getParent(),str);                
            }

        }
    }
}

void LinearScan::expireOldIntervals(Interval *interval)
{
    // Todo
    // There may be a bug?
    auto i=0;
    for(i=0;i<(int)active.size();i++)
    {
        if(active[i]->end < interval->start)
        {
            if(active[i]->isfloat)
            {
                sregs.push_back(active[i]->rreg);
            }
            else
            {
                regs.push_back(active[i]->rreg);
            }
            
        }
        else
        {
            break;
        }
    }
    active.erase(active.begin(),active.begin()+i);
}

void LinearScan::spillAtInterval(Interval *interval)
{
    // Todo
    auto spill=active.rbegin();
    if((*spill)->end > interval->end)
    {
        interval->rreg = (*spill)->rreg;
        (*spill)->spill=true;
        //active.pop_back();
        active.push_back(interval);
        sort(active.begin(), active.end(), compareEnd);
    }
    else
    {
        interval->spill=true;
    }
}

bool LinearScan::compareStart(Interval *a, Interval *b)
{
    return a->start < b->start;
}

bool LinearScan::compareEnd(Interval *a, Interval *b)
{
    return a->end < b->end;
}