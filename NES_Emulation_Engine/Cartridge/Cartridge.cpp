#include <filesystem>

#include "Cartridge.h"
#include "MapperTypes.h"

namespace NES::Cartridge {

	namespace {

		bool check_ines_format(char* name) { return (name == "NES"); }

		struct iNES_Header { // Format for iNES Header - 16 bytes
			char name[4]; // is it needed? -> 4 bytes | NES<EOF> | check_id

			u8 PRG_ROM_Count; // PRG-ROM division/banks
			u8 CHR_ROM_Count; // CHR-ROM division/banks

			// mapper[D0...D3] -> [4,5,6,7] bit | bit 3 -> alternate nametables | bit 2 -> has 512-byte trainer data | 
			// bit 1 -> has battery or non-volatile memory | bit 0 -> Nametable Layout [vertical or horizontal] [Hardwired]
			u8 flag_6;
			// mapper[D4...D7] -> [4,5,6,7] bit | bit 2 and 3 -> NES 2.0 file format identifier | bit 0 and 1 -> Console Type
			// 0 -> NES/Famicom, 1 -> Nintendo Vs. System, 2 -> Nintendo Playchoice 10, 3 -> Extended Console Type
			u8 flag_7;

			u8 program_ram_size;

			u8 tv_system1;
			u8 tv_system2;

			char unused[5];
		} header; 

	} // anonymous namespace

	// Writes Data to the Address Location on the Bus
	void GameCard::cpu_write(u16 address, u8 data) {
		assert(address > 0x401F);
		u32 mapped_address{ 0 };
		if (_mapper->cpuMapWrite(address, mapped_address)) {
			_program_memory[mapped_address] = data;
		}
	}

	// Reads Data from the Address Location on the Bus
	u8 GameCard::cpu_read(u16 address) {
		assert(address > 0x401F);
		u32 mapped_address{ 0 };
		if (_mapper->cpuMapRead(address, mapped_address)) {
			return _program_memory[mapped_address];
		}
		return false;
	}

	// Writes Data to the Address Location on the Bus
	bool GameCard::ppu_write(u16 address, u8 data) {
		u32 mapped_address{ 0 };
		if (_mapper->ppuMapWrite(address, mapped_address)) {
			_character_memory[mapped_address] = data;
			return true;
		}
		return false;
	}

	// Reads Data from the Address Location on the Bus
	bool GameCard::ppu_read(u16 address, u8& data) {
		u32 mapped_address{ 0 };
		if (_mapper->ppuMapRead(address, mapped_address)) {
			data = _character_memory[mapped_address];
			return true;
		}
		return false;
	}

	// for .NES files [iNES format]
	GameCard* load_file(std::string file) {
		assert(std::filesystem::exists(file));

		GameCard* card = new GameCard();
		card->set_cartridge_size(std::filesystem::file_size(file));
		{
			std::ifstream reader(file, std::ios::binary);
			if (reader.is_open()) {
				reader.read((char*)&header, sizeof(iNES_Header));

				assert(check_ines_format(header.name), "Not an INES/.NES format ROM!!");

				// Trainer Area -> 512 bytes -> training information -> check bit 2 of flag 6
				if (header.flag_6 & 0x04) {
					// skip data/ advance read head | TODO: later read and store it.
					reader.seekg(512, std::ios_base::cur); // seek 512 bytes from current position
				}

				u8 mapper_id = (header.flag_7 & 0xF0) | (header.flag_6 >> 4);
				u8 format = (header.flag_7 & 0x0C) >> 2;

				// Currently only use version 1.0
				format = 1;

				switch (format) {
				case 0: // version 0.0 ???
					break;

				case 1: // version 1.0
					card->set_program_banks_count(header.PRG_ROM_Count);
					card->init_program_memory(reader);
					card->set_character_banks_count(header.CHR_ROM_Count);
					card->init_character_memory(reader);

					card->set_mapper(std::make_shared<NROM>(card->get_program_banks_count(), card->get_character_banks_count()));
					break;

				case 2: // version 2.0
					break;

				default:
					assert(false, "How did you even manage to do it?");
					break;
				}

				reader.close(); // End Read
			}
		}
		return card;
	}
}
