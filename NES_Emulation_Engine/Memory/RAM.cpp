
#include "RAM.h"
#include "../Utilities/Disassembler.h"

namespace NES::Memory {

	void RAM::disassemble_wram() {
		Utilities::disasm(_ram);
	}

}