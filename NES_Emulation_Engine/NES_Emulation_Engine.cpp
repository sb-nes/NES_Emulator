// NES_OLC.cpp : 'main' function -> Program Execution Entry Point.

#include <iostream>
#include <crtdbg.h>
#include "CPU/R6502.h"
//#include "Utilities/Disassembler.h"

using namespace NES;

int main()
{
#if _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); //Google it, dammit
#endif

    std::cout << "Initializing!\n";

    CPU::R6502* Cpu = new CPU::R6502();

    Cpu->SetBus(new CPU::Bus());
    Cpu->reset();

    while (1) {
        std::cout << "test";
        if (getchar()) break;
    }

    delete Cpu;

    std::cout << "Done...\n Press Any Key To Continue! \n";
    getchar();
}
