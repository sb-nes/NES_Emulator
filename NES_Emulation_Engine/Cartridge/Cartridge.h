#pragma once

#include <fstream>

#include "../Common/CommonHeaders.h"
#include "Mapper.h"
#include "MapperTypes.h"


namespace NES::Cartridge {

	// A NES 2.0 file contains a sixteen-byte header, followed by Trainer, PRG-ROM, CHR-ROM and Miscellaneous ROM data.

	

	class GameCard {
	public:


		void init_program_memory(std::ifstream& reader){
			_program_memory.resize(_program_banks_count * 16384); // Each Program ROM chip size is 16KB
			reader.read((char*)_program_memory.data(), _program_memory.size());
		}

		void init_character_memory(std::ifstream& reader){
			_character_memory.resize(_character_banks_count * 8192); // Each Character ROM chip size is 8KB
			reader.read((char*)_character_memory.data(), _character_memory.size());
		}

		void set_program_banks_count(u8 count) { _program_banks_count = count; }
		void set_character_banks_count(u8 count) { _character_banks_count = count; }
		u8 get_program_banks_count() { return _program_banks_count; }
		u8 get_character_banks_count() { return _character_banks_count; }

		void set_cartridge_size(u64 size) { _size = size; }

		void set_mapper(std::shared_ptr<Mapper> map) { _mapper = map; }
		std::shared_ptr<Mapper> get_mapper() { return _mapper; }

		// Writes Data to the Address Location on the Bus
		void cpu_write(u16 address, u8 data);
		// Reads Data from the Address Location on the Bus
		[[nodiscard]] u8 cpu_read(u16 address);

		// Writes Data to the Address Location on the Bus
		[[nodiscard]] bool ppu_write(u16 address, u8 data);
		// Reads Data from the Address Location on the Bus
		[[nodiscard]] bool ppu_read(u16 address, u8& data);

	private:
		std::vector<u8>				_program_memory; // PRG-ROM
		std::vector<u8>				_character_memory; // CHR-ROM | CHR Memory | Pattern Memory

		u8							_mapper_id{ 0 }; // which mapper currently in use
		std::shared_ptr<Mapper>		_mapper;

		u8							_program_banks_count{ 0 };
		u8							_character_banks_count{ 0 };

		u64							_size{ 0 };
	};

	GameCard* load_file(std::string file);
}