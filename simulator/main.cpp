#include <iostream>
#include <cstdio>
#include "MyRegister.h"
#include "ProgramCounter.h"
#include "Memory.h"
#include "Decoder.h"
#include "ControlUnit.h"
#include "IF_ID.h"
#include "ID_EX.h"
#include "EX_MEM.h"
#include "MEM_WB.h"
#include "ControlSignalsGenerator.h"
#include <cstdlib>
#define sp 29
using namespace std;
void printSnapShot(FILE* snapShot, int cycle, MyRegister* reg, ProgramCounter* pc);
void print(FILE* debug, int cycle, MyRegister* reg, ProgramCounter* pc);
int main()
{

    Memory* iMemory;
    Memory* dMemory;
    ControlUnit* controlUnit;
    MyRegister *reg;
    ProgramCounter *pc;
   // unsigned int readSp;
    size_t result;

    unsigned char readArray[4];
    unsigned int readProgramCounter;
    FILE *dImage;
    FILE *iImage;
    FILE *insOut;
    FILE *snapShot;
    FILE *errorFile;

    FILE* debug;

    dImage = fopen("dimage.bin", "rb");
    iImage = fopen("iimage.bin", "rb");
    insOut = fopen("ins1.txt", "w");
    snapShot = fopen("snapshot.rpt", "w");
    errorFile = fopen("error_dump.rpt", "w");
    debug = fopen("debug.rpt", "w");

    //read iimage
    result = fread(readArray, 4, 1, iImage);
    readProgramCounter = readArray[0] << 24 | readArray[1] << 16 | readArray[2] << 8 | readArray[3];
    pc = new ProgramCounter(readProgramCounter);
    iMemory = new Memory(iImage, pc->PC);

    //read dimage
    reg = new MyRegister(dImage);
   // reg->print();
    dMemory = new Memory(dImage, 0);
    //Decoder d1(iMemory->memory + pc->PC);
    //d1.print();
    //printf("words = %d\n", iMemory->words);

   /* for(int i=0; i<iMemory->words; i++){
        Decoder d2(iMemory->memory + pc->PC + i*4);
        d2.print();
        d2.fprint(insOut);
    }*/

    int cycle = 0, shutDown = 0;
    controlUnit = new ControlUnit(reg, pc, dMemory, errorFile);

    printSnapShot(snapShot, cycle, reg, pc);
    print(debug, cycle, reg, pc);
    Decoder* IF_ins = new Decoder();
    Decoder* ID_ins = new Decoder();
    Decoder* EX_ins = new Decoder();
    Decoder* MEM_ins = new Decoder();
    Decoder* WB_ins = new Decoder();

    IF_ID IF_ID_buffer();
    ID_EX ID_EX_buffer();
    EX_MEM EX_MEM_buffer();
    MEM_WB MEM_WB_buffer();

    ControlSignalsGenerator controlSignalsGenerator();
    OperationFunction opFunc();



    while(1){
        int branch = 0, branchNewPC;
        int forwardToBranchRs = 0, forwardToBranchRt = 0, forwardToExRs = 0, forwardToExRt = 0;
        int doStallEX = 0, doStallID = 0;
        //WB
        if(MEM_WB_buffer.regWrite == 1){
            if(MEM_WB_buffer.doWriteMemToReg == 1){
                reg->reg[MEM_WB_buffer.writeRegNum] = MEM_WB_buffer.dataFromMem;
            }
            else if(MEM_WB_buffer.doWriteMemToReg == 0){
                reg->reg[MEM_WB_buffer.writeRegNum] = MEM_WB_buffer.dataFromAlu;
            }
        }

            //jal
        if(WB_ins->instructionName = "jal"){
            reg->reg[31] = MEM_WB_buffer.newPC;
        }

        //MEM
        MEM_WB_buffer.dataFromAlu = EX_MEM_buffer.aluResult;
        MEM_WB_buffer.newPC = EX_MEM_buffer.newPC;
        MEM_WB_buffer.writeRegNum = EX_MEM_buffer.regDestIndex;
            //control signals
        MEM_WB_buffer.doWriteMemToReg = EX_MEM_buffer.writeMemToReg;
        MEM_WB_buffer.regWrite = EX_MEM_buffer.regWrite;


        if(EX_MEM_buffer.memRead == 1){
            unsigned int offset = EX_MEM_buffer.aluResult;
            if(MEM_ins->instructionName == "lw"){
                MEM_WB_buffer.dataFromMem = dMemory->memory[offset] << 24 | dMemory->memory[offset+1] << 16 | dMemory->memory[offset+2] << 8 | dMemory->memory[offset+3];
            }
            else if(MEM_ins->instructionName == "lh"){
                if( (dMemory->memory[offset] >> 7) == 0 ){
                    MEM_WB_buffer.dataFromMem = dMemory->memory[offset] << 8 | dMemory->memory[offset+1];
                }
                else{
                    MEM_WB_buffer.dataFromMem = dMemory->memory[offset] << 8 | dMemory->memory[offset+1];
                    unsigned int t = 0xFFFF0000;
                    MEM_WB_buffer.dataFromMem = MEM_WB_buffer.dataFromMem | t;
                }
            }
            else if(MEM_ins->instructionName == "lhu"){
                MEM_WB_buffer.dataFromMem = dMemory->memory[offset] << 8 | dMemory->memory[offset+1];
            }
            else if(MEM_ins->instructionName == "lb"){
                if( (dMemory->memory[offset] >> 7) == 0 ){
                    MEM_WB_buffer.dataFromMem = dMemory->memory[offset];
                }
                else{
                    MEM_WB_buffer.dataFromMem = dMemory->memory[offset];
                    unsigned int t = 0xFFFFFF00;
                    MEM_WB_buffer.dataFromMem = MEM_WB_buffer.dataFromMem | t;
                }
            }
            else if(MEM_ins->instructionName == "lbu"){
                MEM_WB_buffer.dataFromMem = dMemory->memory[offset];
            }
        }
        if(EX_MEM_buffer.memWrite == 1){
            unsigned int offset = EX_MEM_buffer.aluResult;
            unsigned int rtValue = EX_MEM_buffer.readReg2;
            if(MEM_ins->instructionName == "sw"){
                dMemory->memory[offset] = rtValue >> 24;
                dMemory->memory[offset+1] = rtValue >> 16;
                dMemory->memory[offset+2] = rtValue >> 8;
                dMemory->memory[offset+3] = rtValue;
            }
            else if(MEM_ins->instructionName == "sh"){
                dMemory->memory[offset+2] = rtValue >> 8;
                dMemory->memory[offset+3] = rtValue;
            }
            else if(MEM_ins->instructionName == "sb"){
                dMemory->memory[offset+3] = rtValue;
            }
        }
        //EX
        /*hazard handling for branches*/
        if(ID_ins->instructionName == "beq" || ID_ins->instructionName == "bne" || ID_ins->instructionName == "bgtz"){
            if(ID_ins->rs != 0 && ID_ins->rs == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1){//dependency
                /*if not forwardable*/
                if(MEM_ins->instructionName == "lw" || MEM_ins->instructionName == "lh" ||
                   MEM_ins->instructionName == "lhu" || MEM_ins->instructionName == "lb" ||
                   MEM_ins->instructionName == "lbu"){
                        doStallID = 1;
                   }
                /*if forwardable*/
                else{
                    ID_EX_buffer.readReg1 = EX_MEM_buffer.aluResult;
                    forwardToBranchRs = 1;
                }

            }
            else if(ID_ins->rt != 0 && ID_ins->rt == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1){
                /*if not forwardable*/
                if(MEM_ins->instructionName == "lw" || MEM_ins->instructionName == "lh" ||
                   MEM_ins->instructionName == "lhu" || MEM_ins->instructionName == "lb" ||
                   MEM_ins->instructionName == "lbu"){
                        doStallID = 1;
                   }
                /*if forwardable*/
                else{
                    ID_EX_buffer.readReg2 = EX_MEM_buffer.aluResult;
                    forwardToBranchRt = 1;
                }

            }
        }

         //hazard handling (forwarding for instructions other than branches)
        if(EX_ins->instructionName != "beq" && EX_ins->instructionName != "bne" && EX_ins->instructionName != "bgtz"){
            if(EX_ins->rs != 0 && EX_ins->rs == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1){
                /*if not forwardable
                if(MEM_ins->instructionName == "lw" || MEM_ins->instructionName == "lh" ||
                   MEM_ins->instructionName == "lhu" || MEM_ins->instructionName == "lb" ||
                   MEM_ins->instructionName == "lbu"){
                    doStallEX = 1;
                }*/

                /*if forwordable*/
                if(MEM_ins->instructionName != "lw" && MEM_ins->instructionName != "lh" &&
                   MEM_ins->instructionName != "lhu" && MEM_ins->instructionName != "lb" &&
                   MEM_ins->instructionName != "lbu"){
                    forwardToExRs = 1;
                    ID_EX_buffer.readReg1 = EX_MEM_buffer.aluResult;
                }
            }
            if(EX_ins->rt !=0 && EX_ins->rt == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1){
                if(ID_ins->instructionType == R || EX_ins->instructionName == "sw" ||
                   EX_ins->instructionName == "sh" || EX_ins->instructionName == "sb"){
                    /*if not forwardable
                    if(MEM_ins->instructionName == "lw" || MEM_ins->instructionName == "lh" ||
                        MEM_ins->instructionName == "lhu" || MEM_ins->instructionName == "lb" ||
                        MEM_ins->instructionName == "lbu"){
                            doStallEX = 1;
                        }*/
                    /*if forwardable*/
                    if(MEM_ins->instructionName != "lw" && MEM_ins->instructionName != "lh" &&
                    MEM_ins->instructionName != "lhu" && MEM_ins->instructionName != "lb" &&
                    MEM_ins->instructionName != "lbu"){
                        forwardToBranchRt = 1;
                        ID_EX_buffer.readReg2 = EX_MEM_buffer.aluResult;
                    }
                }
            }
        }


        //ALU result

        if(EX_ins->instructionName == "add"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, ID_EX_buffer.readReg2);
        }
        else if(EX_ins->instructionName == "addu"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, ID_EX_buffer.readReg2);
        }
        else if(EX_ins->instructionName == "sub"){
            EX_MEM_buffer.aluResult = opFunc.sub(ID_EX_buffer.readReg1, ID_EX_buffer.readReg2);
        }
        else if(EX_ins->instructionName == "and"){
            EX_MEM_buffer.aluResult = opFunc.andFun(ID_EX_buffer.readReg1, ID_EX_buffer.readReg2);
        }
        else if(EX_ins->instructionName == "or"){
            EX_MEM_buffer.aluResult = opFunc.orFun(ID_EX_buffer.readReg1, ID_EX_buffer.readReg2);
        }
        else if(EX_ins->instructionName == "xor"){
            EX_MEM_buffer.aluResult = opFunc.xorFun(ID_EX_buffer.readReg1, ID_EX_buffer.readReg2);
        }
        else if(EX_ins->instructionName == "nor"){
            EX_MEM_buffer.aluResult = opFunc.nor(ID_EX_buffer.readReg1, ID_EX_buffer.readReg2);
        }
        else if(EX_ins->instructionName == "nand"){
            EX_MEM_buffer.aluResult = opFunc.nand(ID_EX_buffer.readReg1, ID_EX_buffer.readReg2);
        }
        else if(EX_ins->instructionName == "slt"){
            EX_MEM_buffer.aluResult = opFunc.slt(ID_EX_buffer.readReg1, ID_EX_buffer.readReg2);
        }
        else if(EX_ins->instructionName == "sll"){
            EX_MEM_buffer.aluResult = opFunc.sll(ID_EX_buffer.readReg2, EX_ins->shamt);
        }
        else if(EX_ins->instructionName == "srl"){
            EX_MEM_buffer.aluResult = opFunc.srl(ID_EX_buffer.readReg2, EX_ins->shamt);
        }
        else if(EX_ins->instructionName == "sra"){
            EX_MEM_buffer.aluResult = opFunc.sra(ID_EX_buffer.readReg2, EX_ins->shamt);
        }
        else if(EX_ins->instructionName == "addi"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "addiu"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "lw"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "lh"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "lhu"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "lb"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "lbu"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "sw"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "sh"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "sb"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "lui"){
            EX_MEM_buffer.aluResult = opFunc.lui(EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "andi"){
            EX_MEM_buffer.aluResult = opFunc.andFun(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "ori"){
            EX_MEM_buffer.aluResult = opFunc.orFun(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "nori"){
            EX_MEM_buffer.aluResult = opFunc.nor(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "slti"){
            EX_MEM_buffer.aluResult = opFunc.slt(ID_EX_buffer.readReg1, EX_ins->immediate);
        }

        EX_MEM_buffer.readReg2 = ID_EX_buffer.readReg2;

        if(ID_EX_buffer.regDest == 1)
            EX_MEM_buffer.regDestIndex = EX_ins->rd;
        else if(ID_EX_buffer.regDest == 0)
            EX_MEM_buffer.regDestIndex = EX_ins->rt;

        EX_MEM_buffer.newPC = ID_EX_buffer.newPC;
        //control signals
        EX_MEM_buffer.memRead = ID_EX_buffer.memRead;
        EX_MEM_buffer.memWrite = ID_EX_buffer.memWrite;
        EX_MEM_buffer.regWrite = ID_EX_buffer.regWrite;
        EX_MEM_buffer.writeMemToReg = ID_EX_buffer.writeMemToReg;


        //ID
        /*hazard handling for branches or stall*/
        if(ID_ins->instructionName == "beq" || ID_ins->instructionName == "bne" || ID_ins->instructionName == "bgtz"){
            if(ID_ins->rs != 0 && ID_ins->rs == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1){//dependency
                /*stall*/
                forwardToBranchRs = 0;
                forwardToBranchRt = 0;
                doStallID = 1;
            }
            else if(ID_ins->rt != 0 && ID_ins->rt == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1){
                /*stall*/
                forwardToBranchRs = 0;
                forwardToBranchRt = 0;
                doStallID = 1;
            }
        }
        else{
            if(ID_ins->rs != 0 && ID_ins->rs == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1){
                if(EX_ins->instructionName == "lw" || EX_ins->instructionName == "lh" || EX_ins->instructionName == "lhu"||
                   EX_ins->instructionName == "lb" || EX_ins->instructionName == "lbu"){
                    doStallID = 1;
                   }
            }
        }
        ID_EX_buffer.newPC = IF_ID_buffer.newPC;
        ID_EX_buffer.readReg1 = reg->reg[ID_ins->rs];
        ID_EX_buffer.readReg2 = reg->reg[ID_ins->rt];



        if(ID_ins->instructionName == "beq"){

            if(ID_EX_buffer.readReg1 == ID_EX_buffer.readReg2){
                branch = 1;
                branchNewPC = IF_ID_buffer.newPC + 4 * ID_ins->immediate;
            }
        }
        else if(ID_ins->instructionName == "bne"){

            if(ID_EX_buffer.readReg1 != ID_EX_buffer.readReg2){
                branch = 1;
                branchNewPC = IF_ID_buffer.newPC + 4 * ID_ins->immediate;
            }
        }
        else if(ID_ins->instructionName == "bgtz"){

            unsigned int signBit = ID_EX_buffer.readReg1 >> 31;
            if(signBit == 0 && ID_EX_buffer.readReg1 != 0)
                ranch = 1;
                branchNewPC = IF_ID_buffer.newPC + 4 * ID_ins->immediate;
        }
        else if(ID_ins->instructionName == "jr"){
            branch = 1;
            branchNewPC = ID_EX_buffer.readReg1;
        }
        else if(ID_ins->instructionName == "j"){
            branch = 1;
            unsigned int pcFrom31To28;
            pcFrom31To28 = IF_ID_buffer.newPC & 0xF0000000;
            branchNewPC = pcFrom31To28 | (4*(ID_ins->address));
        }
        else if(ID_ins->instructionName == "jal"){
            branch = 1;
            reg->reg[31] = IF_ID_buffer.newPC;
            unsigned int pcFrom31To28 = IF_ID_buffer.newPC & 0xF0000000;
            branchNewPC = pcFrom31To28 | ((ID_ins->address) <<2 );
        }
            //control signals
        ID_EX_buffer.writeMemToReg = controlSignalsGenerator.genMemtoReg(ID_ins);
        ID_EX_buffer.regWrite = controlSignalsGenerator.genRegWrite(ID_ins);
        ID_EX_buffer.memRead = controlSignalsGenerator.genMemRead(ID_ins);
        ID_EX_buffer.memWrite = controlSignalsGenerator.genMemWrite(ID_ins);
        ID_EX_buffer.regDest = controlSignalsGenerator.genRegDst(ID_ins);
        //IF
        unsigned int instruction = iMemory->getMemoryPointer(pc->PC);
        IF_ins = new Decoder(instruction);
        IF_ID_buffer.newPC = pc->PC + 4;
        IF_ID_buffer.instruction = instruction;
        if(branch == 1){
            pc->PC = branchNewPC;
        }
        else if(branch == 0){
            pc->PC = MEM_WB_buffer.newPC;
        }
        //iterate
        delete WB_ins;
        WB_ins = MEM_ins;
        MEM_ins = EX_ins;
        EX_ins = ID_ins;
        ID_ins = IF_ins;

       // reg->print();



       //system("PAUSE");
    }
    delete dMemory;
    delete iMemory;
    delete controlUnit;
    delete pc;
    delete reg;
    fclose(iImage);
    fclose(dImage);
    fclose(insOut);
    fclose(snapShot);
    fclose(errorFile);
    fclose(debug);
    return 0;
}
void printSnapShot(FILE* snapShot, int cycle, MyRegister* reg, ProgramCounter* pc){
    fprintf(snapShot, "cycle %d\n", cycle);
    reg->printSnapShot(snapShot);
    fprintf(snapShot, "PC: 0x%08X", pc->PC);
    fprintf(snapShot,"\n\n\n");
}
void print(FILE* debug, int cycle, MyRegister* reg, ProgramCounter* pc){
    fprintf(debug, "cycle %d\n", cycle);
    reg->printSnapShot(debug);
    fprintf(debug, "PC: 0x%08X", pc->PC);
    fprintf(debug, "\n\n\n");
}
