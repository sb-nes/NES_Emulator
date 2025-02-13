#pragma once
#include <iostream>

#include "../Common/CommonHeaders.h"

namespace NES::Utilities {

	namespace {

		const char hexChar[] = "0123456789ABCDEF";

		std::string hexString(u32 value, u8 length) {
			std::string t(length, '0');

			for (int i{ length - 1 }; i >= 0; --i, value >>= 4) {
				t[i] = hexChar[value & 0xF];
			}

			return "0x"+t;
		}

	} // anonymous namespace

	// TODO: Build a Disasm
	void disasm(std::array<u8, 2048> &ram, u32 start, u32 end) { // Disassembler - [Start, End)
		std::cout << "\nDisassemble Memory: "<< start << "-" << end << "\n";

		for (u32 i{ start }; i < end; ++i) {
			std::cout << hexString(i, 4) << "\t"; // Address

			if (((i + 1) & 0x0F) == 0) {
				std::cout << "\n";

				for (u8 j{ 0 }; j < 0x10; ++j) {
					std::cout << hexString(ram[(i - 15) + j], 2) << "\t"; // Data
				}

				std::cout << "\n\n";
			}
		}

		std::cout << "\nPress Any Key To Continue Execution! \n";
		getchar();
	}

	void disasm(std::array<u8, 2048>& ram) { // Disassembler
		disasm(ram, 0, ram.size());
	}

}