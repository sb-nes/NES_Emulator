#pragma once
#include "../Mapper.h"

namespace NES::Cartridge {
	class NROM : public Mapper {
	public:
		NROM(u8 prg_banks, u8 chr_banks) : Mapper(prg_banks, chr_banks) {} // to execute the Parameterized Constructor of Base class execute the Parameterized Constructor of Base class 

		bool cpuMapRead(u16 address, u32& mapped_address) override;
		bool cpuMapWrite(u16 address, u32& mapped_address) override;
		bool ppuMapRead(u16 address, u32& mapped_address) override;
		bool ppuMapWrite(u16 address, u32& mapped_address) override;
	private:
	};
}