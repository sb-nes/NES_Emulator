#include "R6502.h"

#if CPU_TEST
#include <iostream>
#endif // CPU_TEST


namespace NES::CPU {
	namespace {


	} // anonymous namespace

	// If the opcodes and addressing modes are not implemented, then linker will throw a LINK2019 code while assigning their function pointer to lookup.

	// Addressing Modes | https://www.nesdev.org/wiki/CPU_addressing_modes

	u8 R6502::IMP() { // Implicit/Implied | Instructions like RTS or CLC have no address operand, the destination of results are implied.
		assert(_cycles > 0);
		return 0;
	}
	u8 R6502::ACC() {  // Implicit/Implied | The data is fetched from the accumulator to be worked upon, though not always. || is it needed?
		assert(_cycles > 0);
		_data = _accumulator;
		return 0;
	}
	u8 R6502::IMM() { // Immediate | Uses the 8-bit operand itself as the value for the operation, rather than fetching a value from a memory address.
		assert(_cycles > 0);
		_data = 0x00FF & _program_counter++; // Reading costs 1 cycle
		return 0;
	}
	u8 R6502::ZP0() { // Zero-Page | Fetches the value from an 8-bit address on the zero page.
		assert(_cycles > 0);
		_address_abs = (read(_program_counter++)) & 0x00FF; // Reading costs 1 cycle
		_data = read(_address_abs); // Reading costs 1 cycle
		return 0;
	}
	u8 R6502::ZPX() { // Zero-Page Indexed X-Offset | Uses value stored in X-register to index in Zero Page
		assert(_cycles > 0);
		_address_abs = (read(_program_counter++)) & 0x00FF; // Reading costs 1 cycle
		_address_abs += _x_register; clock(); // Reading from X register cost 1 cycle
		_data = read(_address_abs); // Reading costs 1 cycle
		return 0;
	}
	u8 R6502::ZPY() { // Zero-Page Indexed Y-Offset | Uses value stored in Y-register to index in Zero Page
		assert(_cycles > 0);
		_address_abs = (read(_program_counter++)) & 0x00FF; // Reading costs 1 cycle
		_address_abs += _y_register; clock(); // Reading from Y register cost 1 cycle
		_data = read(_address_abs); // Reading costs 1 cycle
		return 0;
	}
	u8 R6502::REL() { // Relative | For Branching Instructions -> can't jump to anywhere in the address range; They can only jump thats in the vicinity of the branch instruction, no more than 127 memory locations
		assert(_cycles > 0);
		_address_rel = read(_program_counter++);
		// NOTE: if sign bit of the unsigned address is 1, then we set all high bits to 1. -> reason, to use binary arithmetic.
		if (_address_rel & 0x80) _address_rel |= 0xFF00;
		return 0;
	}
	u8 R6502::ABS() { // Absolute | Fetches a 2-byte address from the program counter
		assert(_cycles > 0);
		u16 l_address = read(_program_counter++);// Reading costs 1 cycle
		u16 h_address = read(_program_counter++);// Reading costs 1 cycle
		_address_abs = (h_address << 8) | l_address; // h_address shifted 8 bits to the left and OR'ed with l_address
		_data = read(_address_abs); // Reading costs 1 cycle
		return 0;
	}
	u8 R6502::ABX() { // Absolute Indexed X-Offset | Uses value stored in X-register to offset the absolute address
		assert(_cycles > 0);
		u16 l_address = read(_program_counter++); // Reading costs 1 cycle
		u16 h_address = read(_program_counter++); // Reading costs 1 cycle
		_address_abs = (h_address << 8) | l_address; // h_address shifted 8 bits to the left and OR'ed with l_address
		_address_abs += _x_register; clock(); // Reading from X register cost 1 cycle
		_data = read(_address_abs); // Reading costs 1 cycle

		if (h_address != (_address_abs >> 2)) { // if the memory Page has changed, then 
			clock(); // Memory Page Change/Page Wrap Cost 1 cycle [OOPS Cycle]
			return 1;
		}
		return 0;
	}
	u8 R6502::ABY() { // Absolute Indexed Y-Offset | Uses value stored in Y-register to offset the absolute address
		assert(_cycles > 0);
		u16 l_address = read(_program_counter++); // Reading costs 1 cycle
		u16 h_address = read(_program_counter++); // Reading costs 1 cycle
		_address_abs = (h_address << 8) | l_address; // h_address shifted 8 bits to the left and OR'ed with l_address
		_address_abs += _y_register; clock(); // Reading from Y register cost 1 cycle
		_data = read(_address_abs); // Reading costs 1 cycle

		if (h_address != (_address_abs >> 2)) { // if the memory Page has changed, then 
			clock(); // Memory Page Change/Page Wrap Cost 1 cycle [OOPS Cycle]
			return 1;
		}
		return 0;
	}
	u8 R6502::IND() { // Indirect | R6502's way of implementing pointers in the NES
		assert(_cycles > 0);
		u16 l_address_i = read(_program_counter++); // Reading costs 1 cycle
		u16 h_address_i = read(_program_counter++); // Reading costs 1 cycle
		u16 address_i = (h_address_i << 8) | l_address_i; // h_address shifted 8 bits to the left and OR'ed with l_address

		if (l_address_i == 0x00FF) { // Page Boundary Glitch -> Indirect JMP ($ADDR) Glitch -> Reads the wrong address when crossing a page due to 6502's memory access logic
			_address_abs = (read(address_i & 0xFF00) << 8) | read(address_i);
		}
		else {
			_address_abs = (read(address_i + 1) << 8) | read(address_i); // Reading costs 2 cycle
		}
		_data = read(_address_abs); // Reading costs 1 cycle

		return 0;
	}
	u8 R6502::IZX() { // Indirect Indexed X-Offset | Question: WHY????
		assert(_cycles > 0);
		u16 t_i = read(_program_counter++); // Reading costs 1 cycle
		u16 l_address_i = read((u16)(t_i + (u16)_x_register) * 0x00FF); // Reading costs 1 cycle
		u16 h_address_i = read((u16)(t_i + (u16)_x_register + 1) * 0x00FF); // Reading costs 1 cycle
		_address_abs = (h_address_i << 8) | l_address_i;
		_data = read(_address_abs); // Reading costs 1 cycle

		return 0;
	}
	u8 R6502::IZY() { // Indirect	Indexed Y-Offset | Uses value stored in Y-register to offset the indirect address/ Pointer
		assert(_cycles > 0);
		u16 t_i = read(_program_counter++); // Reading costs 1 cycle

		u16 l_address_i = read(t_i & 0x00FF); // Reading costs 1 cycle
		u16 h_address_i = read((t_i + 1) & 0x00FF); // Reading costs 1 cycle
		u16 _address_abs = (h_address_i << 8) | l_address_i; // h_address shifted 8 bits to the left and OR'ed with l_address
		_address_abs += _y_register; clock(); // Reading from Y register cost 1 cycle
		_data = read(_address_abs); // Reading costs 1 cycle

		// TODO: fix this page wrap code
		if (h_address_i != (_address_abs >> 2)) { // if the memory Page has changed, then 
			clock(); // Memory Page Change/Page Wrap Cost 1 cycle [OOPS Cycle]
			return 1;
		}
		return 0;
	}

	// Indexed addressing modes use the X or Y register to help determine the address

	// OPDCODES/Instructions
	u8 R6502::ADC() { // Add With Carry | adds the carry flag in the status bit and a memory value to the accumulator. | A = A + memory + C | C = ?
		u16 temp = _accumulator + _data + GetFlag(StateFlags::C);

		// Handle Overflow
		SetFlag(StateFlags::C, temp > 255);
		SetFlag(StateFlags::Z, (temp && 0x00FF) == 0);
		SetFlag(StateFlags::N, temp & 0x80);
		SetFlag(StateFlags::V, (~((u16)_accumulator ^ (u16)_data) & ((u16)_accumulator ^ (u16)temp)) & 0x0080 );

		_accumulator = temp & 0x00FF;

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
		_data &= _accumulator;

		SetFlag(StateFlags::V, (_data & 0x0F000000) >> 6);
		SetFlag(StateFlags::Z, (_data & 0xF0000000) >> 7);

		SetFlag(StateFlags::Z, _data == 0);
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
	
	// Break (Software Interrupt Request)
	u8 R6502::BRK() {
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
		return 0;
	}

	// Clear Decimal ->  The decimal flag normally controls whether binary-coded decimal mode (BCD) is enabled, but this mode is permanently disabled on the NES' 2A03 CPU. However, the flag itself still functions and can be used to store state.
	u8 R6502::CLD() { // clears the decimal flag. | D = 0
		SetFlag(StateFlags::D, false);
		return 0;
	}

	// Clear Interrupt Disable
	u8 R6502::CLI() {
		SetFlag(StateFlags::I, false);
		return 0;
	}

	// Clear Overflow
	u8 R6502::CLV() {
		SetFlag(StateFlags::V, false);
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
		SetFlag(StateFlags::Z, _x_register >> 7);
		return 0;
	}

	// Decrement Y
	u8 R6502::DEY() { // Y = Y - 1
		--_y_register;
		SetFlag(StateFlags::Z, _y_register == 0);
		SetFlag(StateFlags::Z, _y_register >> 7);
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
		SetFlag(StateFlags::Z, _x_register >> 7);
		return 0;
	}

	// Increment Y
	u8 R6502::INY() { // Y = Y + 1
		++_y_register;
		SetFlag(StateFlags::Z, _y_register == 0);
		SetFlag(StateFlags::Z, _y_register >> 7);
		return 0;
	}
	/// END ///

	// TODO: Jump
	u8 R6502::JMP() {
		return 0;
	}

	// TODO: Jump to Subroutine
	u8 R6502::JSR() {
		return 0;
	}

	/// Load to Registers ///

	// Load A
	u8 R6502::LDA() { // A = memory
		_accumulator = _data;
		SetFlag(StateFlags::Z, _accumulator == 0);
		SetFlag(StateFlags::Z, _accumulator >> 7);

#if CPU_TEST
		std::cout << _accumulator << "\n";
#endif // CPU_TEST

		return 0;
	}

	// Load X
	u8 R6502::LDX() { // X = memory
		_x_register = _data;
		SetFlag(StateFlags::Z, _x_register == 0);
		SetFlag(StateFlags::Z, _x_register >> 7);
		return 0;
	}

	// Load Y
	u8 R6502::LDY() { // Y = memory
		_y_register = _data;
		SetFlag(StateFlags::Z, _y_register == 0);
		SetFlag(StateFlags::Z, _y_register >> 7);
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
		write(0x0100 + _stack_pointer, _accumulator);
		--_stack_pointer; // Decrement Stack
		return 0;
	}

	// Push Processor Status
	u8 R6502::PHP() { // ($0100 + SP) = NV11DIZC | SP = SP - 1 | Break pushed as 1 -> This flag exists only in the flags byte pushed to the stack, not as real state in the CPU.
		write(0x0100 + _stack_pointer, _status_register | 0x30);
		--_stack_pointer; // Decrement Stack
		return 0;
	}

	// Pull A
	u8 R6502::PLA() { // SP = SP + 1 | A = value($0100 + SP)
		++_stack_pointer; // Increment Stack
		_accumulator = read(0x0100 + _stack_pointer);

		SetFlag(StateFlags::Z, _accumulator == 0);
		SetFlag(StateFlags::N, _accumulator & 0x80);

		return 0;
	}

	// Pull Processor Status
	u8 R6502::PLP() { // SP = SP + 1 | NVxxDIZC = ($0100 + SP)
		++_stack_pointer;
		_data = read(0x0100 + _stack_pointer) & 0xCF;
		_data = (GetFlag(StateFlags::U) | GetFlag(StateFlags::B)) | _data; // TODO: Fix this
		// The effect of changing Interrupt Disable [I] flag is delayed 1 instruction, because the flag is changed after IRQ is polled, delaying the effect until IRQ is polled in the next instruction like with CLI and SEI. TODO: Handle This.
		// Maybe, i can make a function which changes this later...
		_status_register = _data; clock();
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
		// Read Status Register from Stack | Leave the Interrupt Disable Flag as 1
		++_stack_pointer;
		_status_register = read(0x0100 + _stack_pointer);
		_status_register &= ~StateFlags::B;
		_status_register &= ~StateFlags::U;

		++_stack_pointer;
		_program_counter = (u16)read(0x0100 + _stack_pointer); // Address - Low
		++_stack_pointer;
		_program_counter |= (u16)read(0x0100 + _stack_pointer) << 8; // Address - High

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
		return 0;
	}

	// Set Decimal
	u8 R6502::SED() { // D = 1
		SetFlag(StateFlags::D, true);
		return 0;
	}

	// Set Interrupt Disable
	u8 R6502::SEI() { // I = 1
		SetFlag(StateFlags::I, true);
		return 0;
	}
	/// END ///

	/// Store Registers ///

	// Store A
	u8 R6502::STA() { // memory = A
		return 0;
	}

	// Store X
	u8 R6502::STX() { // memory = X
		return 0;
	}

	// Store Y
	u8 R6502::STY() { // memory = Y
		return 0;
	}

	/// Transfer Registers ///

	// Transfer A to X
	u8 R6502::TAX() { // X = A | copies the accumulator value to the X register.
		_x_register = _accumulator;
		SetFlag(StateFlags::Z, _x_register == 0);
		SetFlag(StateFlags::Z, _x_register >> 7);
		return 0;
	}

	// Transfer A to Y
	u8 R6502::TAY() { // Y = A | copies the accumulator value to the Y register.
		_y_register = _accumulator;
		SetFlag(StateFlags::Z, _y_register == 0);
		SetFlag(StateFlags::Z, _y_register >> 7);
		return 0;
	}

	// Transfer Stack Pointer to X
	u8 R6502::TSX() { // X = SP | copies the stack pointer value to the X register. | Does it copy the address, or the value? -> the pointer address
		_x_register = _stack_pointer;
		SetFlag(StateFlags::Z, _x_register == 0);
		SetFlag(StateFlags::Z, _x_register >> 7);
		return 0;
	}

	// Transfer X to A
	u8 R6502::TXA() { // A = X | copies the X register value to the accumulator.
		_accumulator = _x_register;
		SetFlag(StateFlags::Z, _accumulator == 0);
		SetFlag(StateFlags::Z, _accumulator >> 7);
		return 0;
	}

	// Transfer X to Stack Pointer
	u8 R6502::TXS() { // SP = X | copies the X register value to the stack pointer.
		_stack_pointer = _x_register;
		return 0;
	}

	// Transfer Y to A
	u8 R6502::TYA() { // A = Y | copies the Y register value to the accumulator.
		_accumulator = _y_register;
		SetFlag(StateFlags::Z, _accumulator == 0);
		SetFlag(StateFlags::Z, _accumulator >> 7);
		return 0;
	}
	/// END ///

	// Illegal Opcodes
	u8 R6502::XXX() { return 0; }

	// Writes to the Memory on the Address Bus
	void R6502::write(u16 address, u8 data) {
		_bus->write(address, data);
		clock();
	}

	// Reads from the Memory on the Address Bus
	u8 R6502::read(u16 address, bool bReadOnly) {
		u8 data{ _bus->read(address) };
		clock();
		return data;
	}
}