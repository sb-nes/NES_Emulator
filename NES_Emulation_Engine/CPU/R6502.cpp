#include "R6502.h"

#if CPU_TEST
#include <iostream>
#endif // CPU_TEST


namespace NES::CPU {
	namespace {

		const char hexChar[] = "0123456789ABCDEF";

		std::string hexString(u32 value, u8 length) {
			std::string t(length, '0');

			for (int i{ length - 1 }; i >= 0; --i, value >>= 4) {
				t[i] = hexChar[value & 0xF];
			}

			return "0x" + t;
		}

		std::string binString(u32 value, u8 length) {
			std::string t(length, '0');

			for (int i{ length - 1 }; i >= 0; --i, value >>= 1) {
				t[i] = value & 0x1 ? '1' : '0';
			}

			return "0b" + t;
		}

	} // anonymous namespace

	// WARNING: If the opcodes and addressing modes are not implemented, then linker will throw a LINK2019 code while assigning their function pointer to lookup.

	// OPDCODES/Instructions | TODO: Put all read into _data from addressing modes to opcodes
	u8 R6502::ADC() { // Add With Carry | adds the carry flag in the status bit and a memory value to the accumulator. | A = A + memory + C | C = (result > $FF)
		_data = read(_address_abs); // Reading costs 1 cycle
		u16 temp = _accumulator + _data + GetFlag(StateFlags::C);

		// Handle Overflow
		SetFlag(StateFlags::C, temp > 255);
		SetFlag(StateFlags::Z, (temp && 0x00FF) == 0);
		SetFlag(StateFlags::N, temp & 0x80);
		SetFlag(StateFlags::V, (~((u16)_accumulator ^ (u16)_data) & ((u16)_accumulator ^ (u16)temp)) & 0x0080 );

		_accumulator = temp & 0x00FF;

#if CPU_TEST
		std::cout << _accumulator << " " << hexString(_accumulator, 2) << "\n";
		std::cout << "Carry Flag: " << hexString(GetFlag(StateFlags::C), 1) << "\n";
		std::cout << "Zero Flag: " << hexString(GetFlag(StateFlags::Z), 1) << "\n";
		std::cout << "Overflow Flag: " << hexString(GetFlag(StateFlags::V), 1) << "\n";
		std::cout << "Negative Flag: " << hexString(GetFlag(StateFlags::N), 1) << "\n\n";
#endif // CPU_TEST

		return 1;
	}

	// Bitwise AND
	u8 R6502::AND() { // A = A & memory | ANDs a memory value and the accumulator, bit by bit.
		_accumulator &= _data;
		SetFlag(StateFlags::Z, _accumulator == 0);
		SetFlag(StateFlags::Z, _data >> 7);
		return 0;
	}

	// Equivalent to multiplying an unsigned value by 2, with carry indicating overflow. | read-modify-write instruction -> Costs extra cycle [first writes the original data in the location, then writes the modified data]
	u8 R6502::ASL() { // Arithmetic Shift Left | shifts all of the bits of a memory value or the accumulator one position to the left, moving the value of each bit into the next bit. Bit 7 is shifted into the carry flag, and 0 is shifted into bit 0.

		// TODO: Implement the rest functionality

		SetFlag(StateFlags::Z, _data == 0);

		return 0;
	}

	/// Branch Opcodes ///
	u8 R6502::BCC() {
		return 0;
	}
	u8 R6502::BCS() {
		return 0;
	}
	u8 R6502::BEQ() {
		return 0;
	}
	/// END ///

	// Bit Test
	u8 R6502::BIT() { // modifies flags, but does not change memory or registers. | A & memory | Bits 7 and 6 of the memory value are loaded directly into the negative and overflow flags
		_data = read(_address_abs);
		_data &= _accumulator;

		SetFlag(StateFlags::Z, _data == 0);
		SetFlag(StateFlags::V, _data & 0b01000000);
		SetFlag(StateFlags::N, _data & 0b10000000);

#if CPU_TEST
		std::cout << "Bit Test for Status Register: " << "\n";
		std::cout << "Bit Test Result: " << binString(_accumulator, 8) << "\n";
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	/// Branch Opcodes ///
	u8 R6502::BMI() {
		return 0;
	}
	u8 R6502::BNE() {
		return 0;
	}
	u8 R6502::BPL() {
		return 0;
	}
	/// END ///
	
	// Break (Software Interrupt Request) - used as a crash handler
	u8 R6502::BRK() { // bit different from IRQ | NMI can override due to precedence if occurring at the same time. IRQ is skipped. [occurs due to a bug]

		SetFlag(StateFlags::B, 1);
		_address_abs = 0xFFFE;  // IRQ/BRK vector, which may point at a mapper's interrupt handler (or, less often, a handler for APU interrupts) | $FFFE–$FFFF
		interrupt();

#if CPU_TEST
		std::cout << "BREAK [BRK]: " << "\n";
		std::cout << "Stack Pointer Before BRK: " << hexString(_stack_pointer + 3, 2) << "\n";
		std::cout << "Program Counter: " << hexString(_program_counter, 4) << "\n";
		std::cout << "Stack Pointer After BRK: " << hexString(_stack_pointer, 2) << "\n";
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n\n";

		DisassembleRAM(0x01B0, 0x0200);
#endif // CPU_TEST

		return 0;
	}

	/// Branch Opcodes ///
	u8 R6502::BVC() {
		return 0;
	}
	u8 R6502::BVS() {
		return 0;
	}
	/// END ///

	/// Clear STATUS FLAGS ///

	// Clear Carry
	u8 R6502::CLC() { // clears the carry flag. | C = 0
		SetFlag(StateFlags::C, false);

#if CPU_TEST
		std::cout << "Clear Carry Status: " << "\n";
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Clear Decimal ->  The decimal flag normally controls whether binary-coded decimal mode (BCD) is enabled, but this mode is permanently disabled on the NES' 2A03 CPU. However, the flag itself still functions and can be used to store state.
	u8 R6502::CLD() { // clears the decimal flag. | D = 0
		SetFlag(StateFlags::D, false);

#if CPU_TEST
		std::cout << "Clear Decimal Status: " << "\n";
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Clear Interrupt Disable
	u8 R6502::CLI() {
		_delay_change_value = 0;
		delay_assign = &R6502::assign_delay_interrupt_disable_change; // The effect of changing Interrupt Disable [I] flag is delayed 1 instruction, because the flag is changed after IRQ is polled, delaying the effect until IRQ is polled in the next instruction like with CLI and SEI.

#if CPU_TEST
		std::cout << "Clear Interrupt Disable Status: " << "\n";
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Clear Overflow
	u8 R6502::CLV() {
		SetFlag(StateFlags::V, false);

#if CPU_TEST
		std::cout << "Clear Overflow Status: " << "\n";
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n\n";
#endif // CPU_TEST

		return 0;
	}
	/// END ///

	/// Compare Memory and Register Values ///

	// Compare A | Carry and Zero are often most easily remembered as inequalities.
	u8 R6502::CMP() { // A - memory
		_data = _accumulator - _data;
		// TODO: Optimize
		//if (_data >= 0) {
		//	SetFlag(StateFlags::Z, true);
		//	if (_data > 0)
		//		SetFlag(StateFlags::C, true);
		//} else {
		//	SetFlag(StateFlags::N, true); // result bit 7
		//}
		
		// or using bitwise for faster operations? TODO: Profile the code for timing...
		SetFlag(StateFlags::Z, _data == 0); 
		SetFlag(StateFlags::C, _data >= 0);
		SetFlag(StateFlags::N, _data & 0x80); // result bit 7
		return 0;
	}

	// Compare X | Carry and Zero are often most easily remembered as inequalities.
	u8 R6502::CPX() {
		_data = _x_register - _data;
		SetFlag(StateFlags::Z, _data == 0);
		SetFlag(StateFlags::C, _data >= 0);
		SetFlag(StateFlags::N, _data & 0x80); // result bit 7
		return 0;
	}

	// Compare Y | Carry and Zero are often most easily remembered as inequalities.
	u8 R6502::CPY() {
		_data = _y_register - _data;
		SetFlag(StateFlags::Z, _data == 0);
		SetFlag(StateFlags::C, _data >= 0);
		SetFlag(StateFlags::N, _data & 0x80); // result bit 7
		return 0;
	}
	/// END ///

	/// Decrement Values ///

	// Decrement Memory
	u8 R6502::DEC() { // memory = memory - 1
		--_address_abs;
		SetFlag(StateFlags::Z, _address_abs == 0);
		SetFlag(StateFlags::Z, _address_abs >> 7);
		return 0;
	}

	// Decrement X
	u8 R6502::DEX() { // X = X - 1
		--_x_register;
		SetFlag(StateFlags::Z, _x_register == 0);
		SetFlag(StateFlags::N, _x_register >> 7);

#if CPU_TEST
		std::cout << "Decrement X Register: " << "\n";
		std::cout << "X Register: " << _x_register << " " << hexString(_x_register, 2) << "\n";
		std::cout << "Zero Flag: " << hexString(GetFlag(StateFlags::Z), 1) << "\n";
		std::cout << "Negative Flag: " << hexString(GetFlag(StateFlags::N), 1) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Decrement Y
	u8 R6502::DEY() { // Y = Y - 1
		--_y_register;
		SetFlag(StateFlags::Z, _y_register == 0);
		SetFlag(StateFlags::N, _y_register >> 7);

#if CPU_TEST
		std::cout << "Decrement Y Register: " << "\n";
		std::cout << "Y Register: " << _y_register << " " << hexString(_y_register, 2) << "\n";
		std::cout << "Zero Flag: " << hexString(GetFlag(StateFlags::Z), 1) << "\n";
		std::cout << "Negative Flag: " << hexString(GetFlag(StateFlags::N), 1) << "\n\n";
#endif // CPU_TEST

		return 0;
	}
	/// END ///

	// Bitwise Exclusive OR
	u8 R6502::EOR() { // A = A ^ memory
		_accumulator ^= _data;
		SetFlag(StateFlags::Z, _accumulator == 0);
		SetFlag(StateFlags::Z, _accumulator >> 7);
		return 0;
	}

	/// Increment Values ///

	// Increment Memory
	u8 R6502::INC() { // memory = memory + 1
		++_address_abs;
		SetFlag(StateFlags::Z, _address_abs == 0);
		SetFlag(StateFlags::Z, _address_abs >> 7);
		return 0;
	}

	// Increment X
	u8 R6502::INX() { // X = X + 1
		++_x_register;
		SetFlag(StateFlags::Z, _x_register == 0);
		SetFlag(StateFlags::N, _x_register >> 7);

#if CPU_TEST
		std::cout << "Increment X Register: " << "\n";
		std::cout << "X Register: " << _x_register << " " << hexString(_x_register, 2) << "\n";
		std::cout << "Zero Flag: " << hexString(GetFlag(StateFlags::Z), 1) << "\n";
		std::cout << "Negative Flag: " << hexString(GetFlag(StateFlags::N), 1) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Increment Y
	u8 R6502::INY() { // Y = Y + 1
		++_y_register;
		SetFlag(StateFlags::Z, _y_register == 0);
		SetFlag(StateFlags::N, _y_register >> 7);

#if CPU_TEST
		std::cout << "Increment Y Register: " << "\n";
		std::cout << "Y Register: " << _y_register << " " << hexString(_y_register, 2) << "\n";
		std::cout << "Zero Flag: " << hexString(GetFlag(StateFlags::Z), 1) << "\n";
		std::cout << "Negative Flag: " << hexString(GetFlag(StateFlags::N), 1) << "\n\n";
#endif // CPU_TEST

		return 0;
	}
	/// END ///

	// TODO: Jump
	u8 R6502::JMP() {
		_program_counter = _address_abs;

#if CPU_TEST
		std::cout << "Jump To: " << _program_counter << " " << hexString(_program_counter, 4) << "\n\n";
#endif // CPU_TEST
		return 0;
	}

	// TODO: Jump to Subroutine
	u8 R6502::JSR() {
		_program_counter = _address_abs;

#if CPU_TEST
		std::cout << "Jump To Subroutine At: " << _program_counter << " " << hexString(_program_counter, 4) << "\n\n";
#endif // CPU_TEST
		return 0;
	}

	/// Load to Registers ///

	// Load A
	u8 R6502::LDA() { // A = memory
		_accumulator = read(_address_abs);

		SetFlag(StateFlags::Z, _accumulator == 0);
		SetFlag(StateFlags::N, _accumulator >> 7);

#if CPU_TEST
		std::cout << "Accumulator: " << _accumulator << " " << hexString(_accumulator, 2) << "\n";
		std::cout << "Zero Flag: " << hexString(GetFlag(StateFlags::Z),1) << "\n";
		std::cout << "Negative Flag: " << hexString(GetFlag(StateFlags::N),1) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Load X
	u8 R6502::LDX() { // X = memory
		_x_register = read(_address_abs);
		SetFlag(StateFlags::Z, _x_register == 0);
		SetFlag(StateFlags::N, _x_register >> 7);

#if CPU_TEST
		std::cout << "X Register: " << _x_register << " " << hexString(_x_register, 2) << "\n";
		std::cout << "Zero Flag: " << hexString(GetFlag(StateFlags::Z), 1) << "\n";
		std::cout << "Negative Flag: " << hexString(GetFlag(StateFlags::N), 1) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Load Y
	u8 R6502::LDY() { // Y = memory
		_y_register = read(_address_abs);
		SetFlag(StateFlags::Z, _y_register == 0);
		SetFlag(StateFlags::N, _y_register >> 7);

#if CPU_TEST
		std::cout << "Y Register: " << _y_register << " " << hexString(_y_register, 2) << "\n";
		std::cout << "Zero Flag: " << hexString(GetFlag(StateFlags::Z), 1) << "\n";
		std::cout << "Negative Flag: " << hexString(GetFlag(StateFlags::N), 1) << "\n\n";
#endif // CPU_TEST

		return 0;
	}
	/// END ///

	// Logical Shift Right
	u8 R6502::LSR() { // value = value >> 1 or 0 -> [76543210] -> C
		// TODO: Implement Later

		return 0;
	}

	// No Operation
	u8 R6502::NOP() { 
		// has no effect; it merely wastes space and CPU cycles. 
		// This instruction can be useful when writing timed code to delay for a desired amount of time, 
		// as padding to ensure something does or does not cross a page, or to disable code in a binary.

#if CPU_TEST
		std::cout << "NO OPERATION! " << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Bitwise OR
	u8 R6502::ORA() { // A = A | memory
		_accumulator |= _data;
		SetFlag(StateFlags::Z, _accumulator == 0);
		SetFlag(StateFlags::Z, _data >> 7);
		return 0;
	}

	/// Stack Push-Pull ///

	// Push A
	u8 R6502::PHA() { // ($0100 + SP) = A | SP = SP - 1
		_data = _accumulator;
		write_memory(0x0100 + _stack_pointer);
		--_stack_pointer; // Decrement Stack

#if CPU_TEST
		std::cout << "Push Accumulator [PHA]: " << "\n";
		std::cout << "Stack Pointer Before PHA: " << hexString(_stack_pointer + 1, 2) << "\n";
		std::cout << "Accumulator: " << hexString(_program_counter, 4) << "\n";
		std::cout << "Stack Pointer After PHA: " << hexString(_stack_pointer, 2) << "\n";
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n\n";

		DisassembleRAM(0x01B0, 0x0200);
#endif // CPU_TEST

		return 0;
	}

	// Push Processor Status
	u8 R6502::PHP() { // ($0100 + SP) = NV11DIZC | SP = SP - 1 | Break pushed as 1 -> This flag exists only in the flags byte pushed to the stack, not as real state in the CPU.
		_data = _status_register | 0x30;
		write_memory(0x0100 + _stack_pointer);
		--_stack_pointer; // Decrement Stack

#if CPU_TEST
		std::cout << "Push Processor Status [PHP]: " << "\n";
		std::cout << "Stack Pointer Before PHP: " << hexString(_stack_pointer + 1, 2) << "\n";
		std::cout << "Status Register before Push: " << binString(_status_register, 8) << "\n";
		_status_register = 0x00; // Extreme, but for testing
		std::cout << "Stack Pointer After PHP: " << hexString(_stack_pointer, 2) << "\n\n";

		DisassembleRAM(0x01B0, 0x0200);
#endif // CPU_TEST

		return 0;
	}

	// Pull A
	u8 R6502::PLA() { // SP = SP + 1 | A = value($0100 + SP)
		++_stack_pointer; // Increment Stack
		_accumulator = read(0x0100 + _stack_pointer);

		SetFlag(StateFlags::Z, _accumulator == 0);
		SetFlag(StateFlags::N, _accumulator & 0x80);

#if CPU_TEST
		std::cout << "Pull Accumulator [PLA]: " << "\n";
		std::cout << "Stack Pointer Before PLA: " << hexString(_stack_pointer - 1, 2) << "\n";
		std::cout << "Accumulator: " << hexString(_program_counter, 4) << "\n";
		std::cout << "Stack Pointer After PLA: " << hexString(_stack_pointer, 2) << "\n";
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n\n";

		DisassembleRAM(0x01B0, 0x0200);
#endif // CPU_TEST

		return 0;
	}

	// Pull Processor Status
	u8 R6502::PLP() { // SP = SP + 1 | NVxxDIZC = ($0100 + SP)

#if CPU_TEST
		std::cout << "Status Register before Pull: " << binString(_status_register, 8) << "\n";
#endif

		++_stack_pointer;
		_data = read(0x0100 + _stack_pointer) & 0xCF;
		_data |= StateFlags::U;

		_delay_change_value = (_data & StateFlags::I) >> 2;
		_data &= ~StateFlags::I;
		delay_assign = &R6502::assign_delay_interrupt_disable_change; // The effect of changing Interrupt Disable [I] flag is delayed 1 instruction, because the flag is changed after IRQ is polled, delaying the effect until IRQ is polled in the next instruction like with CLI and SEI.
		
		_status_register |= _data; clock();

#if CPU_TEST
		std::cout << "Pull Processor Status [PLP]: " << "\n";
		std::cout << "Stack Pointer Before PLP: " << hexString(_stack_pointer - 1, 2) << "\n";
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n";
		std::cout << "Stack Pointer After PLP: " << hexString(_stack_pointer, 2) << "\n\n";

		DisassembleRAM(0x01B0, 0x0200);
#endif // CPU_TEST

		return 0;
	}
	/// END ///

	// TODO: Implement These Later
	u8 R6502::ROL() {
		return 0;
	}
	u8 R6502::ROR() {
		return 0;
	}

	// Return from Interrupt
	u8 R6502::RTI() { // pull NVxxDIZC flags from stack | pull PC from stack
		// Read Status Register from Stack | Changing the Interrupt Disable Flag is immediate
		++_stack_pointer;
		_status_register = read(0x0100 + _stack_pointer);
		_status_register &= ~StateFlags::B;
		_status_register |= StateFlags::U;

		++_stack_pointer;
		_program_counter = (u16)read(0x0100 + _stack_pointer); // Address - Low
		++_stack_pointer;
		_program_counter |= (u16)read(0x0100 + _stack_pointer) << 8; // Address - High

#if CPU_TEST
		std::cout << "Return From Interrupt [RTI]: " << "\n";
		std::cout << "Stack Pointer Before RTI: " << hexString(_stack_pointer + 3, 2) << "\n";
		std::cout << "Program Counter: " << hexString(_program_counter, 4) << "\n";
		std::cout << "Stack Pointer After RTI: " << hexString(_stack_pointer, 2) << "\n";
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n\n";

		DisassembleRAM(0x01B0, 0x0200);
#endif // CPU_TEST

		return 0;
	}


	u8 R6502::RTS() {
		return 0;
	}

	// Subtract with Carry
	u8 R6502::SBC() { // A = A - memory - ~C or A = A + ~memory + C
		u16 value = ((u16)_data) ^ 0x00FF; // NOT/Invert

		u16 temp = _accumulator + value + GetFlag(StateFlags::C); // Do i need to explicit convert it?

		// Handle Overflow
		SetFlag(StateFlags::C, temp > 0x00FF);
		SetFlag(StateFlags::Z, (temp && 0x00FF) == 0);
		SetFlag(StateFlags::V, (~((u16)_accumulator ^ (u16)_data) & ((u16)_accumulator ^ (u16)temp)) & 0x0080);
		SetFlag(StateFlags::N, temp & 0x80);

		_accumulator = temp & 0x00FF;

		return 1;
	}

	/// Set STATUS FLAGS ///

	// Set Carry
	u8 R6502::SEC() { // C = 1
		SetFlag(StateFlags::C, true);

#if CPU_TEST
		std::cout << "Set Carry Status: " << "\n";
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Set Decimal
	u8 R6502::SED() { // D = 1
		SetFlag(StateFlags::D, true);

#if CPU_TEST
		std::cout << "Set Decimal Status: " << "\n";
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Set Interrupt Disable
	u8 R6502::SEI() { // I = 1
		_delay_change_value = 1;
		delay_assign = &R6502::assign_delay_interrupt_disable_change; // The effect of changing Interrupt Disable [I] flag is delayed 1 instruction, because the flag is changed after IRQ is polled, delaying the effect until IRQ is polled in the next instruction like with CLI and SEI.

#if CPU_TEST
		std::cout << "Set Interrupt Disable Status: " << "\n";
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n\n";
#endif // CPU_TEST

		return 0;
	}
	/// END ///

	/// Store Registers ///

	// Store A
	u8 R6502::STA() { // memory = A
		_data = _accumulator;
		write_memory(_address_abs);

#if CPU_TEST
		std::cout << "Storing Accumulator value: " << _accumulator << " " << hexString(_accumulator, 2) << " in: " << hexString(_address_abs, 4) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Store X
	u8 R6502::STX() { // memory = X
		_data = _x_register;
		write_memory(_address_abs);

#if CPU_TEST
		std::cout << "Storing X Register value: " << _x_register << " " << hexString(_x_register, 2) << " in: " << hexString(_address_abs, 4) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Store Y
	u8 R6502::STY() { // memory = Y
		_data = _y_register;
		write_memory(_address_abs);

#if CPU_TEST
		std::cout << "Storing Y Register value: " << _y_register << " " << hexString(_y_register, 2) << " in: " << hexString(_address_abs, 4) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	/// Transfer Registers ///

	// Transfer A to X
	u8 R6502::TAX() { // X = A | copies the accumulator value to the X register.
		_x_register = _accumulator;
		SetFlag(StateFlags::Z, _x_register == 0);
		SetFlag(StateFlags::N, _x_register >> 7);

#if CPU_TEST
		std::cout << "Copy Accumulator to X Register: \n";
		std::cout << "Accumulator: " << _accumulator << " " << hexString(_accumulator, 2) << "\n";
		std::cout << "X Register: " << _x_register << " " << hexString(_x_register, 2) << "\n";
		std::cout << "Y Register: " << _y_register << " " << hexString(_y_register, 2) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Transfer A to Y
	u8 R6502::TAY() { // Y = A | copies the accumulator value to the Y register.
		_y_register = _accumulator;
		SetFlag(StateFlags::Z, _y_register == 0);
		SetFlag(StateFlags::N, _y_register >> 7);

#if CPU_TEST
		std::cout << "Copy Accumulator to Y Register: \n";
		std::cout << "Accumulator: " << _accumulator << " " << hexString(_accumulator, 2) << "\n";
		std::cout << "X Register: " << _x_register << " " << hexString(_x_register, 2) << "\n";
		std::cout << "Y Register: " << _y_register << " " << hexString(_y_register, 2) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Transfer Stack Pointer to X
	u8 R6502::TSX() { // X = SP | copies the stack pointer value to the X register. | Does it copy the address, or the value? -> the pointer address
		_x_register = _stack_pointer;
		SetFlag(StateFlags::Z, _x_register == 0);
		SetFlag(StateFlags::N, _x_register >> 7);

#if CPU_TEST
		std::cout << "Copy Stack Pointer to X Register: \n";
		std::cout << "X Register: " << _x_register << " " << hexString(_x_register, 2) << "\n";
		std::cout << "Stack Pointer: " << _stack_pointer << " " << hexString(_stack_pointer, 2) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Transfer X to A
	u8 R6502::TXA() { // A = X | copies the X register value to the accumulator.
		_accumulator = _x_register;
		SetFlag(StateFlags::Z, _accumulator == 0);
		SetFlag(StateFlags::N, _accumulator >> 7);

#if CPU_TEST
		std::cout << "Copy X Register to Accumulator: \n";
		std::cout << "Accumulator: " << _accumulator << " " << hexString(_accumulator, 2) << "\n";
		std::cout << "X Register: " << _x_register << " " << hexString(_x_register, 2) << "\n";
		std::cout << "Y Register: " << _y_register << " " << hexString(_y_register, 2) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Transfer X to Stack Pointer
	u8 R6502::TXS() { // SP = X | copies the X register value to the stack pointer.
		_stack_pointer = _x_register;

#if CPU_TEST
		std::cout << "Copy X Register to Stack Pointer: \n";
		std::cout << "X Register: " << _x_register << " " << hexString(_x_register, 2) << "\n";
		std::cout << "Stack Pointer: " << _stack_pointer << " " << hexString(_stack_pointer, 2) << "\n\n";
#endif // CPU_TEST

		return 0;
	}

	// Transfer Y to A
	u8 R6502::TYA() { // A = Y | copies the Y register value to the accumulator.
		_accumulator = _y_register;
		SetFlag(StateFlags::Z, _accumulator == 0);
		SetFlag(StateFlags::N, _accumulator >> 7);

#if CPU_TEST
		std::cout << "Copy Y Register to Accumulator: \n";
		std::cout << "Accumulator: " << _accumulator << " " << hexString(_accumulator, 2) << "\n";
		std::cout << "X Register: " << _x_register << " " << hexString(_x_register, 2) << "\n";
		std::cout << "Y Register: " << _y_register << " " << hexString(_y_register, 2) << "\n\n";
#endif // CPU_TEST

		return 0;
	}
	/// END ///

	// Illegal Opcodes
	u8 R6502::XXX() { return 0; }

	void R6502::print_status_register() {
		std::cout << "Status Register: " << binString(_status_register, 8) << "\n\n";
	}
}