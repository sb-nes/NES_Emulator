
#include "RAM.h"
#include "../Utilities/Disassembler.h"

namespace NES::Memory {

	void RAM::disassemble_wram() {
		Utilities::disasm(_ram);
	}

	void RAM::disassemble_wram(u32 start, u32 end) { // Disassembler - [Start, End)
		Utilities::disasm(_ram, start, end);
	}

}