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
#include "OperationFunction.h"
#include <cstdlib>
#define sp 29
using namespace std;
void printDebug(FILE* snapShot, int cycle, MyRegister* reg, int pc,
                   Decoder* IF_ins, Decoder* ID_ins, Decoder* EX_ins,
                   Decoder* MEM_ins, Decoder* WB_ins, int branch, int stall,
                   int forwardToBranchRs, int forwardToBranchRt, int forwardToExRs, int forwardToExRt);
void printSnapShot(FILE* snapShot, int cycle, MyRegister* reg, int pc,
                   Decoder* IF_ins, Decoder* ID_ins, Decoder* EX_ins,
                   Decoder* MEM_ins, Decoder* WB_ins, int branch, int stall,
                   int forwardToBranchRs, int forwardToBranchRt, int forwardToExRs, int forwardToExRt);
void print(FILE* debug, int cycle, MyRegister* reg, ProgramCounter* pc);
void printErrorDump(FILE* errorDump, int cycle, int writeTo0, int addressOverflow, int misalignment, int numOverflow);
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
   // insOut = fopen("ins1.txt", "w");
    snapShot = fopen("snapshot.rpt", "w");
    errorFile = fopen("error_dump.rpt", "w");
 //   debug = fopen("debug.rpt", "w");

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

    int cycle = -1;
    controlUnit = new ControlUnit(reg, pc, dMemory, errorFile);

//    printSnapShot(snapShot, cycle, reg, pc);
  //  print(debug, cycle, reg, pc);
    Decoder* IF_ins = new Decoder();
    Decoder* ID_ins = new Decoder();
    Decoder* EX_ins = new Decoder();
    Decoder* MEM_ins = new Decoder();
    Decoder* WB_ins = new Decoder();

    IF_ID IF_ID_buffer;
    ID_EX ID_EX_buffer;
    EX_MEM EX_MEM_buffer;
    MEM_WB MEM_WB_buffer;

    ControlSignalsGenerator controlSignalsGenerator;
    OperationFunction opFunc;
    unsigned int oldPC = pc->PC;
    MyRegister oldReg;

    unsigned int IF_PC_plus4 = 0;
    while(1){
        int branch = 0;
        unsigned int branchNewPC;
        int forwardToBranchRs = 0, forwardToBranchRt = 0, forwardToExRs = 0, forwardToExRt = 0;
        int doStallID = 0;
        int writeTo0 = 0, addressOverflow = 0, misalignment = 0, numberOverflow = 0;
        int stallForRs = 0, stallForRt = 0;

        oldReg = *reg;
        //WB

        if(MEM_WB_buffer.regWrite == 1){
            /*write to zero check*/
            if(MEM_WB_buffer.writeRegNum == 0){
                writeTo0 = 1;
            }
            else{
                if(MEM_WB_buffer.doWriteMemToReg == 1){
                    reg->reg[MEM_WB_buffer.writeRegNum] = MEM_WB_buffer.dataFromMem;
                }
                else if(MEM_WB_buffer.doWriteMemToReg == 0){
                    reg->reg[MEM_WB_buffer.writeRegNum] = MEM_WB_buffer.dataFromAlu;
                }
            }

        }
            //jal
        if(WB_ins->instructionName == "jal"){
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
                /*overflow check*/
                if(offset + 3 >= 1024 || offset >= 1024){
                    addressOverflow = 1;
                }
                /*misalignment check*/
                if(offset % 4 != 0){
                    misalignment = 1;
                }
               if(addressOverflow == 0 && misalignment == 0)
                    MEM_WB_buffer.dataFromMem = dMemory->memory[offset] << 24 | dMemory->memory[offset+1] << 16 | dMemory->memory[offset+2] << 8 | dMemory->memory[offset+3];
            }
            else if(MEM_ins->instructionName == "lh"){
                if(offset + 1 >= 1024 || offset >= 1024){
                    addressOverflow = 1;
                }
                if(offset%2 != 0){
                    misalignment = 1;
                }
                if(addressOverflow == 0 && misalignment == 0){
                    if( (dMemory->memory[offset] >> 7) == 0 ){
                        MEM_WB_buffer.dataFromMem = dMemory->memory[offset] << 8 | dMemory->memory[offset+1];
                    }
                    else{
                        MEM_WB_buffer.dataFromMem = dMemory->memory[offset] << 8 | dMemory->memory[offset+1];
                        unsigned int t = 0xFFFF0000;
                        MEM_WB_buffer.dataFromMem = MEM_WB_buffer.dataFromMem | t;
                    }
                }
            }
            else if(MEM_ins->instructionName == "lhu"){
                if(offset + 1 >= 1024 || offset >= 1024){
                    addressOverflow = 1;
                }
                if(offset%2 != 0){
                    misalignment = 1;
                }
                if(addressOverflow == 0 && misalignment == 0)
                    MEM_WB_buffer.dataFromMem = dMemory->memory[offset] << 8 | dMemory->memory[offset+1];
            }
            else if(MEM_ins->instructionName == "lb"){
                if(offset  >= 1024 ){
                    addressOverflow = 1;
                }
                if(addressOverflow == 0){
                    if( (dMemory->memory[offset] >> 7) == 0 ){
                    MEM_WB_buffer.dataFromMem = dMemory->memory[offset];
                    }
                    else{
                        MEM_WB_buffer.dataFromMem = dMemory->memory[offset];
                        unsigned int t = 0xFFFFFF00;
                        MEM_WB_buffer.dataFromMem = MEM_WB_buffer.dataFromMem | t;
                    }
                }
            }
            else if(MEM_ins->instructionName == "lbu"){
                if(offset  >= 1024){
                    addressOverflow = 1;
                }
                if(addressOverflow == 0){
                    MEM_WB_buffer.dataFromMem = dMemory->memory[offset];
                }
            }
        }
        if(EX_MEM_buffer.memWrite == 1){
            unsigned int offset = EX_MEM_buffer.aluResult;
            unsigned int rtValue = EX_MEM_buffer.readReg2;
            if(MEM_ins->instructionName == "sw"){
                if(offset + 3 >= 1024 || offset >= 1024){
                    addressOverflow = 1;
                }
                if(offset%4 != 0){
                    misalignment = 1;
                }
                if(addressOverflow == 0 && misalignment == 0){
                    dMemory->memory[offset] = rtValue >> 24;
                    dMemory->memory[offset+1] = rtValue >> 16;
                    dMemory->memory[offset+2] = rtValue >> 8;
                    dMemory->memory[offset+3] = rtValue;
                }
            }
            else if(MEM_ins->instructionName == "sh"){
                if(offset + 1 >= 1024 || offset >= 1024){
                    addressOverflow = 1;
                }
                if(offset%2 != 0){
                    misalignment = 1;
                }
                if(addressOverflow == 0 && misalignment == 0){
                    dMemory->memory[offset] = rtValue >> 8;
                    dMemory->memory[offset+1] = rtValue;
                }
            }
            else if(MEM_ins->instructionName == "sb"){
                if(offset >= 1024){
                    addressOverflow = 1;
                }
                if(addressOverflow == 0){
                    dMemory->memory[offset] = rtValue;
                }
            }
        }
        //EX
        /*hazard handling for branches in ID STAGE*/
        if(ID_ins->instructionName == "beq" || ID_ins->instructionName == "bne" || ID_ins->instructionName == "bgtz" || ID_ins->instructionName == "jr"){
            /*rs*/
            if(ID_ins->rs == 31 && MEM_ins->instructionName == "jal"){
                //ID_EX_buffer.readReg1 = EX_MEM_buffer.newPC;
                ID_ins->readRsValue = EX_MEM_buffer.newPC;
                forwardToBranchRs = 1;
            }
            else if(ID_ins->rs != 0 && ID_ins->rs == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1){//dependency
                /*if not forwardable*/
                if(MEM_ins->instructionName == "lw" || MEM_ins->instructionName == "lh" ||
                   MEM_ins->instructionName == "lhu" || MEM_ins->instructionName == "lb" ||
                   MEM_ins->instructionName == "lbu"){
                        doStallID = 1;
                   }
                /*if forwardable*/
                else{
                    //ID_EX_buffer.readReg1 = EX_MEM_buffer.aluResult;
                    ID_ins->readRsValue = EX_MEM_buffer.aluResult;
                    forwardToBranchRs = 1;
                }
            }

            /*rt*/
            if(ID_ins->rt == 31 && MEM_ins->instructionName == "jal" && doStallID == 0){
                //ID_EX_buffer.readReg2 = EX_MEM_buffer.newPC;
                ID_ins->readRtValue = EX_MEM_buffer.newPC;
                forwardToBranchRt = 1;
            }
            else if(ID_ins->rt != 0 && ID_ins->rt == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1 && doStallID == 0){
                /*if not forwardable*/
                if(MEM_ins->instructionName == "lw" || MEM_ins->instructionName == "lh" ||
                   MEM_ins->instructionName == "lhu" || MEM_ins->instructionName == "lb" ||
                   MEM_ins->instructionName == "lbu"){
                        doStallID = 1;
                   }
                /*if forwardable*/
                else{
                  //  ID_EX_buffer.readReg2 = EX_MEM_buffer.aluResult;
                  ID_ins->readRtValue = EX_MEM_buffer.aluResult;
                    forwardToBranchRt = 1;
                }

            }
            if(ID_ins->instructionName == "jr" && MEM_ins->instructionName == "jal" && doStallID == 0){
                ID_ins->readRsValue = EX_MEM_buffer.newPC;
                forwardToBranchRs = 1;
            }
        }
        else{
            if(ID_ins->rs == 31 && MEM_ins->instructionName == "jal" && ID_ins->readRs == 1){
                doStallID = 1;
                stallForRs = 1;
            }
            else if(ID_ins->rs != 0 && ID_ins->rs == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1 && ID_ins->readRs == 1){
                    doStallID = 1;
                    stallForRs = 1;
            }
            if(ID_ins->rt == 31 && MEM_ins->instructionName == "jal" && doStallID == 0 && ID_ins->readRt == 1){
                doStallID = 1;
                stallForRt = 1;
            }
            else if(ID_ins->rt != 0 && ID_ins->rt == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1 && doStallID == 0 && ID_ins->readRt == 1){
                    doStallID = 1;
                    stallForRt = 1;
            }
            //if(ID_ins->rs == 31 && EX)
        }

         //hazard handling (ONLY forwarding for instructions other than branches IN EX STAGE. CANNOT be stalled. stalling has be done in ID)
        if(EX_ins->instructionName != "beq" && EX_ins->instructionName != "bne" && EX_ins->instructionName != "bgtz"){
            if(EX_ins->rs == 31 && MEM_ins->instructionName == "jal" && EX_ins->readRs == 1){
                // definitely forwardable
                forwardToExRs = 1;
                ID_EX_buffer.readReg1 = EX_MEM_buffer.newPC;
            }
            else if(EX_ins->rs != 0 && EX_ins->rs == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1 && EX_ins->readRs == 1){
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
            if(EX_ins->rt == 31 && MEM_ins->instructionName == "jal" && EX_ins->readRt == 1){
                // definitely forwardable
                forwardToExRt = 1;
                ID_EX_buffer.readReg2 = EX_MEM_buffer.newPC;
            }
            else if(EX_ins->rt !=0 && EX_ins->rt == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1 && EX_ins->readRt == 1){
                if(EX_ins->instructionType == R || EX_ins->instructionName == "sw" ||
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
                        forwardToExRt = 1;
                        ID_EX_buffer.readReg2 = EX_MEM_buffer.aluResult;
                    }
                }
            }
        }


        //ALU result

        if(EX_ins->instructionName == "add"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, ID_EX_buffer.readReg2);
            unsigned int rsSign = ID_EX_buffer.readReg1 >> 31;
            unsigned int rtSign = ID_EX_buffer.readReg2 >> 31;
            unsigned int rdSign = EX_MEM_buffer.aluResult >> 31;
            if( rsSign == rtSign && rsSign != rdSign )
                numberOverflow = 1;
        }
        else if(EX_ins->instructionName == "addu"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, ID_EX_buffer.readReg2);
        }
        else if(EX_ins->instructionName == "sub"){
            EX_MEM_buffer.aluResult = opFunc.sub(ID_EX_buffer.readReg1, ID_EX_buffer.readReg2);
            unsigned int rsSign = ID_EX_buffer.readReg1 >> 31;
            unsigned rt2sComplementSign = (~ID_EX_buffer.readReg2 + 1) >> 31;
            unsigned int rdSign = EX_MEM_buffer.aluResult >> 31;
            if(rsSign == rt2sComplementSign && rsSign != rdSign)
                numberOverflow = 1;
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
            unsigned int rsSign = ID_EX_buffer.readReg1 >> 31;
            unsigned int immediateSign = EX_ins->immediate >> 31;
            unsigned int rtSign = EX_MEM_buffer.aluResult >> 31;
            if( rsSign == immediateSign && rsSign != rtSign )
                numberOverflow = 1;
        }
        else if(EX_ins->instructionName == "addiu"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
        }
        else if(EX_ins->instructionName == "lw"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
            if( (ID_EX_buffer.readReg1 >> 31) == (EX_ins->immediate >> 31) && (EX_ins->immediate >> 31) !=  (EX_MEM_buffer.aluResult>>31) )
                numberOverflow = 1;
        }
        else if(EX_ins->instructionName == "lh"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
            if( (ID_EX_buffer.readReg1 >> 31) == (EX_ins->immediate >> 31) && (EX_ins->immediate >> 31) !=  (EX_MEM_buffer.aluResult>>31) )
                numberOverflow = 1;
        }
        else if(EX_ins->instructionName == "lhu"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
            if( (ID_EX_buffer.readReg1 >> 31) == (EX_ins->immediate >> 31) && (EX_ins->immediate >> 31) !=  (EX_MEM_buffer.aluResult>>31) )
                numberOverflow = 1;
        }
        else if(EX_ins->instructionName == "lb"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
            if( (ID_EX_buffer.readReg1 >> 31) == (EX_ins->immediate >> 31) && (EX_ins->immediate >> 31) !=  (EX_MEM_buffer.aluResult>>31) )
                numberOverflow = 1;
        }
        else if(EX_ins->instructionName == "lbu"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
            if( (ID_EX_buffer.readReg1 >> 31) == (EX_ins->immediate >> 31) && (EX_ins->immediate >> 31) !=  (EX_MEM_buffer.aluResult>>31) )
                numberOverflow = 1;
        }
        else if(EX_ins->instructionName == "sw"){
            //printf("Reg1 = %d", reg->reg[0]);
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
            if( (ID_EX_buffer.readReg1 >> 31) == (EX_ins->immediate >> 31) && (EX_ins->immediate >> 31) !=  (EX_MEM_buffer.aluResult>>31) )
                numberOverflow = 1;
        }
        else if(EX_ins->instructionName == "sh"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
            if( (ID_EX_buffer.readReg1 >> 31) == (EX_ins->immediate >> 31) && (EX_ins->immediate >> 31) !=  (EX_MEM_buffer.aluResult>>31) )
                numberOverflow = 1;
        }
        else if(EX_ins->instructionName == "sb"){
            EX_MEM_buffer.aluResult = opFunc.add(ID_EX_buffer.readReg1, EX_ins->immediate);
            if( (ID_EX_buffer.readReg1 >> 31) == (EX_ins->immediate >> 31) && (EX_ins->immediate >> 31) !=  (EX_MEM_buffer.aluResult>>31) )
                numberOverflow = 1;
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
        else
            EX_MEM_buffer.regDestIndex = EX_ins->rt;


        EX_MEM_buffer.newPC = ID_EX_buffer.newPC;
        //control signals
        EX_MEM_buffer.memRead = ID_EX_buffer.memRead;
        EX_MEM_buffer.memWrite = ID_EX_buffer.memWrite;
        EX_MEM_buffer.regWrite = ID_EX_buffer.regWrite;
        EX_MEM_buffer.writeMemToReg = ID_EX_buffer.writeMemToReg;


        //ID
        /*hazard handling for branches and stall*/
        if(ID_ins->instructionName == "beq" || ID_ins->instructionName == "bne" || ID_ins->instructionName == "bgtz" || ID_ins->instructionName == "jr"){
            if( (ID_ins->rs == 31 || ID_ins->rt == 31 || ID_ins->instructionName == "jr") && EX_ins->instructionName == "jal" ){
                forwardToBranchRs = 0;
                forwardToBranchRt = 0;
                doStallID = 1;
            }
            else if(ID_ins->rs != 0 && ID_ins->rs == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1 && ID_ins->readRs == 1){//dependency
                /*stall*/
                forwardToBranchRs = 0;
                forwardToBranchRt = 0;
                doStallID = 1;
            }
            /*if id is to stall because of rs, there is no need to check rt*/
            else if(ID_ins->rt != 0 && ID_ins->rt == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1 && ID_ins->readRt == 1){
                /*stall*/
                forwardToBranchRs = 0;
                forwardToBranchRt = 0;
                doStallID = 1;
            }
        }
        else{
            if(ID_ins->rs != 0 && ID_ins->rs == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1 && ID_ins->readRs == 1){
                /*if not forwardalble*/
                if(EX_ins->instructionName == "lw" || EX_ins->instructionName == "lh" || EX_ins->instructionName == "lhu"||
                   EX_ins->instructionName == "lb" || EX_ins->instructionName == "lbu"){
                    stallForRs = 1;
                    doStallID = 1;
                   }
                else{
                    stallForRs = 0;
                }
            }
            if(ID_ins->rt != 0 && ID_ins->rt == EX_MEM_buffer.regDestIndex && EX_MEM_buffer.regWrite == 1 && ID_ins->readRt == 1){
                if(EX_ins->instructionName == "lw" || EX_ins->instructionName == "lh" || EX_ins->instructionName == "lhu"||
                   EX_ins->instructionName == "lb" || EX_ins->instructionName == "lbu"){
                    stallForRt = 1;
                    doStallID = 1;
                   }
                else{
                    stallForRt = 0;
                }
            }
            if(stallForRs == 0 && stallForRt == 0)
                doStallID = 0;
        }
        ID_EX_buffer.newPC = IF_ID_buffer.newPC;
        if(forwardToBranchRs == 0)
            ID_EX_buffer.readReg1 = reg->reg[ID_ins->rs];
        if(forwardToBranchRt == 0)
            ID_EX_buffer.readReg2 = reg->reg[ID_ins->rt];

        if(forwardToBranchRs == 0)
            ID_ins->readRsValue = reg->reg[ID_ins->rs];
        if(forwardToBranchRt == 0)
            ID_ins->readRtValue = reg->reg[ID_ins->rt];



        if(ID_ins->instructionName == "beq"){
            /*calculate the destination*/
            branchNewPC = IF_ID_buffer.newPC + 4 * ID_ins->immediate;
            /*overflow detection*/
            unsigned int pcSign = IF_ID_buffer.newPC >> 31;
            unsigned int cTimes4Sign = ID_ins->immediate << 2;
            cTimes4Sign >>= 31;
            unsigned int pcAfterSign = branchNewPC >> 31;
            if(pcSign == cTimes4Sign && pcSign != pcAfterSign)
                numberOverflow = 1;
            /*if branch is taken*/
            if(ID_ins->readRsValue == ID_ins->readRtValue){
                branch = 1;
            }
        }
        else if(ID_ins->instructionName == "bne"){
            /*calculate the destination*/
            branchNewPC = IF_ID_buffer.newPC + 4 * ID_ins->immediate;
            /*overflow detection*/
            unsigned int pcSign = IF_ID_buffer.newPC >> 31;
            unsigned int cTimes4Sign = ID_ins->immediate << 2;
            cTimes4Sign >>= 31;
            unsigned int pcAfterSign = branchNewPC >> 31;
            if(pcSign == cTimes4Sign && pcSign != pcAfterSign)
                numberOverflow = 1;
            /*if branch is taken*/
            if(ID_ins->readRsValue != ID_ins->readRtValue){
                branch = 1;
              //  IF_ins->setToNop();
            }
        }
        else if(ID_ins->instructionName == "bgtz"){
           // system("PAUSE");
            branchNewPC = IF_ID_buffer.newPC + 4 * ID_ins->immediate;
            unsigned int signBit = (ID_ins->readRsValue) >> 31;
            /*overflow detection*/
            unsigned int pcSign = IF_ID_buffer.newPC >> 31;
            unsigned int cTimes4Sign = ID_ins->immediate << 2;
            cTimes4Sign >>= 31;
            unsigned int pcAfterSign = branchNewPC >> 31;
            if(pcSign == cTimes4Sign && pcSign != pcAfterSign)
                numberOverflow = 1;
            if(signBit == 0 && ID_ins->readRsValue != 0)
                branch = 1;
        }
        else if(ID_ins->instructionName == "jr"){
            branch = 1;
            branchNewPC = ID_ins->readRsValue;
          //  IF_ins->setToNop();
        }
        else if(ID_ins->instructionName == "j"){
            branch = 1;
            unsigned int pcFrom31To28;
            pcFrom31To28 = IF_ID_buffer.newPC & 0xF0000000;
            branchNewPC = pcFrom31To28 | (4*(ID_ins->address));
         //   IF_ins->setToNop();
        }
        else if(ID_ins->instructionName == "jal"){
            branch = 1;
           // reg->reg[31] = IF_ID_buffer.newPC;
            unsigned int pcFrom31To28 = IF_ID_buffer.newPC & 0xF0000000;
            branchNewPC = pcFrom31To28 | ((ID_ins->address) <<2 );
         //   IF_ins->setToNop();
        }
            //control signals
        ID_EX_buffer.writeMemToReg = controlSignalsGenerator.genMemtoReg(ID_ins);
        ID_EX_buffer.regWrite = controlSignalsGenerator.genRegWrite(ID_ins);
        ID_EX_buffer.memRead = controlSignalsGenerator.genMemRead(ID_ins);
        ID_EX_buffer.memWrite = controlSignalsGenerator.genMemWrite(ID_ins);
        ID_EX_buffer.regDest = controlSignalsGenerator.genRegDst(ID_ins);
        //IF
        /*print file*/
        if(cycle > -1){
            printSnapShot(snapShot, cycle, &oldReg, oldPC, IF_ins,
                          ID_ins, EX_ins, MEM_ins, WB_ins, branch, doStallID,
                          forwardToBranchRs, forwardToBranchRt, forwardToExRs, forwardToExRt);
          /*  printDebug(debug, cycle, &oldReg, oldPC, IF_ins,
                          ID_ins, EX_ins, MEM_ins, WB_ins, branch, doStallID,
                          forwardToBranchRs, forwardToBranchRt, forwardToExRs, forwardToExRt);*/
            printErrorDump(errorFile, cycle + 1, writeTo0, addressOverflow, misalignment, numberOverflow);
        }


        /*printf("cycle %d\n", cycle);
        reg->print();
        //oldReg.print();
        printf("PC: 0x%x\n", oldPC);
        printf("IF: 0x%x\t", IF_ID_buffer.instruction);
        if(doStallID == 1)  printf("to be stalled\n");
        IF_ins->print();
        cout << "ID: " << ID_ins->returnName() << '\t';
        if(doStallID == 1)  printf("to be stalled");
        if(branch == 1) printf("\tbranch is taken");
        if(forwardToBranchRs == 1)  printf("\t fwd to rs\n");
        if(forwardToBranchRt == 1)  printf("\t fwd to rt\n");
        ID_ins->print();
        cout << "EX: " << EX_ins->returnName() << '\t';
        EX_ins->print();
        if(forwardToExRs) printf("\t fwd to rs\n");
        if(forwardToExRt) printf("\t fwd to rt\n");
        cout << "DM: " << MEM_ins->returnName() << '\t';
        MEM_ins->print();
        cout << "WB: " << WB_ins->returnName() << '\t';
        WB_ins->print();*/
        //system("PAUSE");
        /*if */
        if(misalignment == 1 || addressOverflow == 1){
            break;
        }
        if(WB_ins->instructionName == "halt")
            break;
        if(IF_ins->instructionName == "halt" && ID_ins->instructionName == "halt" && EX_ins->instructionName == "halt" &&
           MEM_ins->instructionName == "halt" && WB_ins->instructionName == "halt"){
                break;
        }



        if(doStallID == 1){
            delete WB_ins;
            WB_ins = MEM_ins;
            MEM_ins = EX_ins;
            /*insert NOP*/
            EX_ins = new Decoder();
            /*flush ID_EX*/
            ID_EX_buffer.setToZero();
            //IF ID remain unchanged

         //   IF_ID_buffer.newPC = pc->PC + 4;
            //IF_PC_plus4 = pc->PC+4;
        }
        else{
            if(branch == 1){
                pc->PC = branchNewPC;
                //flush IF
                IF_ins->setToNop();
            }
            delete WB_ins;
            WB_ins = MEM_ins;
            MEM_ins = EX_ins;
            EX_ins = ID_ins;
            ID_ins = IF_ins;
            IF_ID_buffer.newPC = IF_PC_plus4;
            unsigned char* instruction = iMemory->getMemoryPointer(pc->PC);
            IF_ID_buffer.instruction = (instruction[0]<<24) | (instruction[1]<<16) | (instruction[2]<<8) | instruction[3];
            oldPC = pc->PC;//the pc we need to print is the pc where the ins in fetched,  so we need to copy the save the pc here
            IF_ins = new Decoder(instruction);
           // IF_ID_buffer.newPC = pc->PC + 4;
            IF_PC_plus4 = pc->PC+4;
            /*printf("test\n");
            IF_ins->print();
            printf("\nPC=%X\n", pc->PC);
            printf("newPC = %X", IF_ID_buffer.newPC);
            printf("test\n");*/

        }

      //  oldPC = pc->PC;

        if(doStallID == 0){
        //oldPC = pc->PC;
           /* if(branch == 1){
                pc->PC = branchNewPC;
            }
            else if(branch == 0){
                pc->PC = IF_ID_buffer.newPC;
            }*/
            pc->PC += 4;
        }
        if(cycle > 500000)
            break;

        cycle++;

        //iterate


       // reg->print();



       //system("PAUSE");
    }
    if(dMemory != NULL) delete dMemory;
    if(iMemory != NULL) delete iMemory;
    if(controlUnit != NULL) delete controlUnit;
    if(IF_ins != NULL)  delete IF_ins;
    if(ID_ins != NULL) delete ID_ins;
    if(EX_ins != NULL) delete EX_ins;
    if(MEM_ins != NULL) delete MEM_ins;
    if(WB_ins != NULL) delete WB_ins;
    if(pc != NULL)  delete pc;
    if(reg != NULL) delete reg;
    fclose(iImage);
    fclose(dImage);
    //fclose(insOut);
    fclose(snapShot);
    fclose(errorFile);
  //  fclose(debug);
    return 0;
}
void printSnapShot(FILE* snapShot, int cycle, MyRegister* reg, int pc,
                   Decoder* IF_ins, Decoder* ID_ins, Decoder* EX_ins,
                   Decoder* MEM_ins, Decoder* WB_ins, int branch, int stall,
                   int forwardToBranchRs, int forwardToBranchRt, int forwardToExRs, int forwardToExRt){
    fprintf(snapShot, "cycle %d\n", cycle);
    reg->printSnapShot(snapShot);
    fprintf(snapShot, "PC: 0x%08X\n", pc);

    fprintf(snapShot, "IF: 0x%08X", IF_ins->instruction);
    if(stall == 1)  fprintf(snapShot, " to_be_stalled");
    else if(branch == 1)    fprintf(snapShot, " to_be_flushed");
    fprintf(snapShot, "\n");

    fprintf(snapShot, "ID: %s", ID_ins->returnName().data());
    if(stall == 1)  fprintf(snapShot, " to_be_stalled");
    else{
        if(forwardToBranchRs == 1) fprintf(snapShot, " fwd_EX-DM_rs_$%d", ID_ins->rs);
        if(forwardToBranchRt == 1) fprintf(snapShot, " fwd_EX-DM_rt_$%d", ID_ins->rt);
    }
    fprintf(snapShot, "\n");

    fprintf(snapShot, "EX: %s", EX_ins->returnName().data());
    if(forwardToExRs == 1) fprintf(snapShot, " fwd_EX-DM_rs_$%d", EX_ins->rs);
    if(forwardToExRt == 1) fprintf(snapShot, " fwd_EX-DM_rt_$%d", EX_ins->rt);
    fprintf(snapShot, "\n");

    fprintf(snapShot, "DM: %s", MEM_ins->returnName().data());
    fprintf(snapShot, "\n");

    fprintf(snapShot, "WB: %s", WB_ins->returnName().data());
    //fprintf(snapShot, "\n");

    fprintf(snapShot,"\n\n\n");
}
void printErrorDump(FILE* errorDump, int cycle, int writeTo0, int addressOverflow, int misalignment, int numOverflow){
    if(writeTo0 == 1){
        fprintf(errorDump, "In cycle %d: Write $0 Error\n", cycle);
    }
    if(addressOverflow == 1){
        fprintf(errorDump, "In cycle %d: Address Overflow\n", cycle);
    }
    if(misalignment == 1){
        fprintf(errorDump, "In cycle %d: Misalignment Error\n", cycle);
    }
    if(numOverflow == 1){
        fprintf(errorDump, "In cycle %d: Number Overflow\n", cycle);
    }
}
void print(FILE* debug, int cycle, MyRegister* reg, ProgramCounter* pc){
    fprintf(debug, "cycle %d\n", cycle);
    reg->printSnapShot(debug);
    fprintf(debug, "PC: 0x%08X", pc->PC);
    fprintf(debug, "\n\n\n");
}

void printDebug(FILE* snapShot, int cycle, MyRegister* reg, int pc,
                   Decoder* IF_ins, Decoder* ID_ins, Decoder* EX_ins,
                   Decoder* MEM_ins, Decoder* WB_ins, int branch, int stall,
                   int forwardToBranchRs, int forwardToBranchRt, int forwardToExRs, int forwardToExRt){
    fprintf(snapShot, "cycle %d\n", cycle);
    reg->printSnapShot(snapShot);
    fprintf(snapShot, "PC: 0x%08X\n", pc);

    fprintf(snapShot, "IF: 0x%08X\n", IF_ins->instruction);
    IF_ins->printDebug(snapShot);
    if(stall == 1)  fprintf(snapShot, " to_be_stalled");
    else if(branch == 1)    fprintf(snapShot, " to_be_flushed");
    fprintf(snapShot, "\n");

    fprintf(snapShot, "ID: %s\n", ID_ins->returnName().data());
    ID_ins->printDebug(snapShot);
    if(stall == 1)  fprintf(snapShot, " to_be_stalled");
    else{
        if(forwardToBranchRs == 1) fprintf(snapShot, " fwd_EX-DM_rs_$%d", ID_ins->rs);
        if(forwardToBranchRt == 1) fprintf(snapShot, " fwd_EX-DM_rt_$%d", ID_ins->rt);
    }
    fprintf(snapShot, "\n");

    fprintf(snapShot, "EX: %s\n", EX_ins->returnName().data());
    EX_ins->printDebug(snapShot);
    if(forwardToExRs == 1) fprintf(snapShot, " fwd_EX-DM_rs_$%d", EX_ins->rs);
    if(forwardToExRt == 1) fprintf(snapShot, " fwd_EX-DM_rt_$%d", EX_ins->rt);
    fprintf(snapShot, "\n");

    fprintf(snapShot, "DM: %s\n", MEM_ins->returnName().data());
    MEM_ins->printDebug(snapShot);
    fprintf(snapShot, "\n");

    fprintf(snapShot, "WB: %s\n", WB_ins->returnName().data());
    WB_ins->printDebug(snapShot);
    //fprintf(snapShot, "\n");

    fprintf(snapShot,"\n\n\n");
}
