#pragma once

#include "../Common/CommonHeaders.h"
#include "Bus.h"

// WARNING: If the opcodes and addressing modes are not implemented, then linker will throw a LINK2019 code while assigning their function pointer to lookup.

// NTSC CPU clock cycle delay = 558.65921787709497206703910614525 nanoseconds or 559 nanoseconds
namespace NES::CPU {
	class R6502 {
	public:

		enum StateFlags : u8 {
			C = (1 << 0),		// Carry Bit State
			Z = (1 << 1),		// Zero
			I = (1 << 2),		// Disable Interrupts | change is delayed by 1 instruction
			D = (1 << 3),		// Decimal Mode (lacks functionality | unused)
			B = (1 << 4),		// Break
			U = (1 << 5),		// Unused
			V = (1 << 6),		// Overflow
			N = (1 << 7),		// Negative

			count
		};


		// Addressing Modes | https://www.nesdev.org/wiki/CPU_addressing_modes

		// IMM -> #$00
		// ZP0 -> $00
		// ZPX -> $00, X
		// ZPY -> $00, Y
		// ABS -> $0000
		// ABX -> $0000, X
		// ABY -> $0000, Y
		// IND -> ($0000)
		// IZX -> ($00, X)
		// IZY -> ($00), Y
		// REL -> $0000 [Relative to Program-Counter]

		u8 IMP() { // Implicit/Implied | Instructions like RTS or CLC have no address operand, the destination of results are implied. | Accumulator Address Mode
			assert (_cycles > 0);
			read = &R6502::read_accumulator;
			write = &R6502::write_accumulator;
			return 0;
		}

		u8 IMM() { // Immediate | Uses the 8-bit operand itself as the value for the operation, rather than fetching a value from another memory address. [operand itself is in a Memory Location]
			assert(_cycles > 0);
			_address_abs = _program_counter++; // Reading costs 1 cycle
			return 0;
		}

		u8 ZP0() { // Zero-Page | Fetches the value from an 8-bit address on the zero page.
			assert(_cycles > 0);
			_address_abs = (read_memory(_program_counter++)) & 0x00FF; // Reading costs 1 cycle
			read = &R6502::read_memory;
			write = &R6502::write_memory;
			return 0;
		}

		u8 ZPX() { // Zero-Page Indexed X-Offset | Uses value stored in X-register to index in Zero Page
			assert(_cycles > 0);
			_address_abs = (read_memory(_program_counter++)) & 0x00FF; // Reading costs 1 cycle
			_address_abs += _x_register; clock(); // Reading from X register cost 1 cycle
			read = &R6502::read_memory;
			write = &R6502::write_memory;
			return 0;
		}

		u8 ZPY() { // Zero-Page Indexed Y-Offset | Uses value stored in Y-register to index in Zero Page
			assert(_cycles > 0);
			_address_abs = (read_memory(_program_counter++)) & 0x00FF; // Reading costs 1 cycle
			_address_abs += _y_register; clock(); // Reading from Y register cost 1 cycle
			read = &R6502::read_memory;
			write = &R6502::write_memory;
			return 0;
		}

		u8 REL() { // Relative | For Branching Instructions -> can't jump to anywhere in the address range; They can only jump thats in the vicinity of the branch instruction, no more than 127 memory locations
			assert(_cycles > 0);
			_address_rel = read_memory(_program_counter);
			// NOTE: if sign bit of the unsigned address is 1, then we set all high bits to 1. -> reason, to use binary arithmetic.
			if (_address_rel & 0x80) _address_rel |= 0xFF00;
			read = &R6502::read_memory;
			write = &R6502::write_memory;

			return 0;
		}

		u8 ABS() { // Absolute | Fetches a 2-byte address from the program counter
			assert(_cycles > 0);
			u16 l_address = read_memory(_program_counter++);// Reading costs 1 cycle
			u16 h_address = read_memory(_program_counter++);// Reading costs 1 cycle
			_address_abs = (h_address << 8) | l_address; // h_address shifted 8 bits to the left and OR'ed with l_address
			read = &R6502::read_memory;
			write = &R6502::write_memory;
			return 0;
		}

		u8 ABX() { // Absolute Indexed X-Offset | Uses value stored in X-register to offset the absolute address
			assert(_cycles > 0);
			u16 l_address = read_memory(_program_counter++); // Reading costs 1 cycle
			u16 h_address = read_memory(_program_counter++); // Reading costs 1 cycle
			_address_abs = (h_address << 8) | l_address; // h_address shifted 8 bits to the left and OR'ed with l_address
			_address_abs += _x_register; clock(); // Reading from X register cost 1 cycle

			if (h_address != (_address_abs >> 8)) { // if the memory Page has changed, then 
				clock(); // Memory Page Change/Page Wrap Cost 1 cycle [OOPS Cycle]
				return 1;
			}
			read = &R6502::read_memory;
			write = &R6502::write_memory;
			return 0;
		}

		u8 ABY() { // Absolute Indexed Y-Offset | Uses value stored in Y-register to offset the absolute address
			assert(_cycles > 0);
			u16 l_address = read_memory(_program_counter++); // Reading costs 1 cycle
			u16 h_address = read_memory(_program_counter++); // Reading costs 1 cycle
			_address_abs = (h_address << 8) | l_address; // h_address shifted 8 bits to the left and OR'ed with l_address
			_address_abs += _y_register; clock(); // Reading from Y register cost 1 cycle

			if (h_address != (_address_abs >> 8)) { // if the memory Page has changed, then 
				clock(); // Memory Page Change/Page Wrap Cost 1 cycle [OOPS Cycle]
				return 1;
			}
			read = &R6502::read_memory;
			write = &R6502::write_memory;
			return 0;
		}

		u8 IND() { // Indirect | R6502's way of implementing pointers in the NES
			assert(_cycles > 0);
			u16 l_address_i = read_memory(_program_counter++); // Reading costs 1 cycle
			u16 h_address_i = read_memory(_program_counter++); // Reading costs 1 cycle
			u16 address_i = (h_address_i << 8) | l_address_i; // h_address shifted 8 bits to the left and OR'ed with l_address

			if (l_address_i == 0x00FF) { // Page Boundary Glitch -> Indirect JMP ($ADDR) Glitch -> Reads the wrong address when crossing a page due to 6502's memory access logic
				_address_abs = (read_memory(address_i & 0xFF00) << 8) | read_memory(address_i);
			} else {
				_address_abs = (read_memory(address_i + 1) << 8) | read_memory(address_i); // Reading costs 2 cycle
			}
			read = &R6502::read_memory;
			write = &R6502::write_memory;
			return 0;
		}

		u8 IZX() { // Indirect Indexed X-Offset | Question: WHY????
			assert(_cycles > 0);
			u16 t_i = read_memory(_program_counter++); // Reading costs 1 cycle
			u16 l_address_i = read_memory((u16)(t_i + (u16)_x_register) * 0x00FF); // Reading costs 1 cycle
			u16 h_address_i = read_memory((u16)(t_i + (u16)_x_register + 1) * 0x00FF); // Reading costs 1 cycle
			_address_abs = (h_address_i << 8) | l_address_i;
			read = &R6502::read_memory;
			write = &R6502::write_memory;

			return 0;
		}

		u8 IZY() { // Indirect	Indexed Y-Offset | Uses value stored in Y-register to offset the indirect address/ Pointer
			assert(_cycles > 0);
			u16 t_i = read_memory(_program_counter++); // Reading costs 1 cycle

			u16 l_address_i = read_memory(t_i & 0x00FF); // Reading costs 1 cycle
			u16 h_address_i = read_memory((t_i + 1) & 0x00FF); // Reading costs 1 cycle
			u16 _address_abs = (h_address_i << 8) | l_address_i; // h_address shifted 8 bits to the left and OR'ed with l_address
			_address_abs += _y_register; clock(); // Reading from Y register cost 1 cycle
			_data = read_memory(_address_abs); // Reading costs 1 cycle

			// TODO: fix this page wrap code
			if (h_address_i != (_address_abs >> 8)) { // if the memory Page has changed, then 
				clock(); // Memory Page Change/Page Wrap Cost 1 cycle [OOPS Cycle]
				return 1;
			}
			read = &R6502::read_memory;
			write = &R6502::write_memory;

			return 0;
		}

		// Indexed addressing modes use the X or Y register to help determine the address

		// OPCODES/Instructions: https://www.nesdev.org/wiki/Instruction_reference | https://www.oxyron.de/html/opcodes02.html
		u8 ADC(); // Add With Carry
		u8 AND(); // AND operation
		u8 ASL();
		u8 BCC();
		u8 BCS();
		u8 BEQ();
		u8 BIT();
		u8 BMI();
		u8 BNE();
		u8 BPL();
		u8 BRK(); // Break
		u8 BVC();
		u8 BVS();
		u8 CLC(); // Clear Carry Bit of Status/FLAG
		u8 CLD();
		u8 CLI();
		u8 CLV();
		u8 CMP();
		u8 CPX();
		u8 CPY();
		u8 DEC();
		u8 DEX(); // Decrement X Register
		u8 DEY(); // Decrement Y Register
		u8 EOR();
		u8 INC();
		u8 INX();
		u8 INY();
		u8 JMP(); // Jump??
		u8 JSR();
		u8 LDA(); // Load Accumulator
		u8 LDX(); // Load X Register with value
		u8 LDY(); // Load Y Register with value
		u8 LSR();
		u8 NOP();
		u8 ORA();
		u8 PHA();
		u8 PHP();
		u8 PLA();
		u8 PLP();
		u8 ROL();
		u8 ROR();
		u8 RTI();
		u8 RTS();
		u8 SBC();
		u8 SEC();
		u8 SED();
		u8 SEI(); // Set Enable Interrupt
		u8 STA(); // Store Value in Accumulator into Memory Address
		u8 STX();
		u8 STY();
		u8 TAX();
		u8 TAY();
		u8 TSX();
		u8 TXA();
		u8 TXS();
		u8 TYA();

		u8 XXX(); // Illegal Opcodes

		R6502() { }
		~R6502() { delete _bus; }

		// External Signals
		void clock() { // Per Clock Signal
			if (_cycles == 0) {
				assert(_cycles == 0);
				++_cycles; // Since, whenever i read, i use one cpu cycle in the read function
				_opcode = read_memory(_program_counter++);

				_cycles = _lookup[_opcode >> 4][_opcode & 0x0F].cycles;
				_cycles += (this->*_lookup[_opcode >> 4][_opcode & 0x0F].addrmode)();
				_cycles += (this->*_lookup[_opcode >> 4][_opcode & 0x0F].opcode)();

				(this->*delay_change)();
				(this->*delay_assign)(); // fbrereto -> https://stackoverflow.com/questions/2898316/using-a-member-function-pointer-within-a-class

#if CPU_TEST
				--_instructions_count;
#endif // CPU_TEST

			} else {
			// wait for set time
			--_cycles;
			}
		}

		// Reset, IRQ and NMI use CPU vectors, provided by the cartridge at the end of the unmapped space.
		// The MOS 6502 and by extension the 2A03/2A07 has a quirk that can cause an interrupt to use the wrong vector if two different interrupts occur very close to one another.

		void reset() { // Reset -> Can occur at any point of time | Resets to initial state/power-up state
			_accumulator = 0;
			_x_register = 0;
			_y_register = 0;

			_stack_pointer = 0xFD;
			_status_register = 0x00 | StateFlags::U | StateFlags::I; // Disable Interrupts at Init.

			_cycles = 2;
			_address_abs = 0xFFFC; // Reset vector, which points at code to initialize the NES chipset | $FFFC–$FFFD
			u16 l_address = read_memory(_address_abs + 0);
			u16 h_address = read_memory(_address_abs + 1);

			_program_counter = (h_address << 8) | l_address;

			_address_abs = 0x0000;
			_address_abs = 0x0000;
			_data = 0x00;

			_ticks = 0;
			_bus->reset();

#if CPU_TEST
			_cycles = 0;
#else
			_cycles = 8; // Since it takes time...
#endif // CPU_TEST

			clock();
		}

		/// INTERRUPTS ///
		// An interrupt is a signal that causes a CPU to pause, save what it was doing, and call a routine that handles the signal. 
		// The signal usually indicates an event originating in other circuits in a system. 
		// The routine, called an interrupt service routine (ISR) or interrupt handler, processes the event and then returns to the main program.
		
		// Interupt Request
		void irq() { // Can occur at any point of time | Can be disabled. | level-sensitive (reacts to a low signal level) | Triggered by external hardware
			if (GetFlag(StateFlags::I) == 0) {

				SetFlag(StateFlags::B, 0);
				_address_abs = 0xFFFE;  // IRQ/BRK vector, which may point at a mapper's interrupt handler (or, less often, a handler for APU interrupts) | $FFFE–$FFFF
				interrupt();

				_cycles = 7; // These take time...
			}
		}

		// Non-Maskable Interrupt
		void nmi() { // Can occur at any point of time | edge-sensitive (reacts to high-to-low transitions in the signal) 

			SetFlag(StateFlags::B, 0);
			_address_abs = 0xFFFA;  //  NMI vector, which points at an NMI handler | $FFFA–$FFFB
			interrupt();

			_cycles = 8; // These take time...
		}
		/// END INTERRUPTS ///

		Bus* CreateBus() { return new CPU::Bus(); }
		void SetBus(Bus* bus) { _bus = bus; }
		void AddInstruction(u8 opcode, u8 value){}
		[[nodiscard]] constexpr Bus* GetBus() { return _bus; }

		void DisassembleRAM() { _bus->disassembleRAM(); }
		void DisassembleRAM(u32 start, u32 end) { // Disassembler - [Start, End)
			_bus->disassembleRAM(start, end); 
		}

#if CPU_TEST
		void set_instructions_count(u16 count) { _instructions_count = count; }
		[[nodiscard]]u16 get_instructions_count() { return _instructions_count; }
#endif // CPU_TEST

	private:
		Bus* _bus{ nullptr };
		bool _write_to_mem{ false };

		// Registers -> X, Y, Status -> All 8-bits
		u8		_x_register{ 0x00 };
		u8		_y_register{ 0x00 };
		u8		_status_register{ 0x00 }; // state of the CPU using StateFlags

		u8		_accumulator{ 0x00 }; // For Mathematical Calculations
		u8		_stack_pointer{ 0x00 }; // Points to the location on the bus -> indexes into a 256-byte stack at $0100-$01FF on the bus
		u16		_program_counter{ 0x0000 }; // Stores the Address of the next program byte -> Supposed to be an array

		u8		_opcode{ 0x00 };
		u8		_cycles{ 0 };
		u8		_data{ 0x00 };
		u8		_ticks{ 0 };

		u16		_address_abs{ 0x0000 }; // Absolute Address
		u16		_address_rel{ 0x00 }; // Relative Address

		void	(R6502::*write)(u16) {}; // Write Function Pointer
		u8		(R6502::* read)(u16, bool) {}; // Write Function Pointer
		void	(R6502::*delay_assign)() = &R6502::do_nothing_like_its_nobodys_business; // Delay Interrupt Disable Change Function Pointer
		void	(R6502::*delay_change)() = &R6502::do_nothing_like_its_nobodys_business; // Delay Interrupt Disable Change Function Pointer
		u8		_delay_change_value{ 0 };

#if CPU_TEST
		u16		_instructions_count{ 0 };
#endif // CPU_TEST


		struct Instruction {
			std::string	  name = "???";
			u8(R6502::*opcode)(void) = &XXX; // function pointer for the Operation
			u8(R6502::*addrmode)(void) = &IMP; // function pointer for the Address Mode
			u8			  cycles = 2;
		};

		// TODO: to check table with reference
		const std::array<std::array<R6502::Instruction, 16>, 16> _lookup{ // Row-Major 16x16
			{ // Array Bracket

			{ // 0x
				{
					{ "BRK", &R6502::BRK, &R6502::IMM, 7 },
					{ "ORA", &R6502::ORA, &R6502::IZX, 6 },

					{}, // Illegal -> KIL
					{ "???", &R6502::XXX, &R6502::IZX, 8 }, // Illegal -> SLO
					{ "???", &R6502::NOP, &R6502::ZP0, 3 }, // Illegal -> NOP

					{ "ORA", &R6502::ORA, &R6502::ZP0, 3 },
					{ "ASL", &R6502::ORA, &R6502::ZP0, 5 },

					{ "???", &R6502::XXX, &R6502::ZP0, 5 }, // Illegal -> SLO

					{ "PHP", &R6502::PHP, &R6502::IMP, 3 },
					{ "ORA", &R6502::ORA, &R6502::IMM, 2 },
					{ "ASL", &R6502::ASL, &R6502::IMP, 2 },

					{ "???", &R6502::XXX, &R6502::IMM, 2 }, // Illegal -> ANC
					{ "???", &R6502::NOP, &R6502::ABS, 4 }, // Illegal -> NOP

					{ "ORA", &R6502::ORA, &R6502::ABS, 4 },
					{ "ASL", &R6502::ASL, &R6502::ABS, 6 },

					{ "???", &R6502::XXX, &R6502::ABS, 6 }, // Illegal -> SLO
				}
			},

			{ // 1x
				{
					{ "BPL", &R6502::BPL, &R6502::REL, 2 }, // * -> Cycle count can increase
					{ "ORA", &R6502::ORA, &R6502::IZY, 5 },	// * -> Cycle count can increase

					{}, // Illegal -> KIL
					{ "???", &R6502::XXX, &R6502::IZY, 8 }, // Illegal -> SLO
					{ "???", &R6502::NOP, &R6502::ZPX, 4 }, // Illegal -> NOP

					{ "ORA", &R6502::ORA, &R6502::ZPX, 4 },
					{ "ASL", &R6502::ORA, &R6502::ZPX, 6 },

					{ "???", &R6502::XXX, &R6502::ZPX, 6 }, // Illegal -> SLO

					{ "CLC", &R6502::CLC, &R6502::IMP, 2 },
					{ "ORA", &R6502::ORA, &R6502::ABY, 4 }, // * -> Cycle count can increase

					{ "???", &R6502::NOP, &R6502::IMP, 2 }, // Illegal -> NOP
					{ "???", &R6502::XXX, &R6502::ABY, 7 }, // Illegal -> SLO
					{ "???", &R6502::NOP, &R6502::ABX, 4 }, // Illegal -> NOP | * -> Cycle count can increase

					{ "ORA", &R6502::ORA, &R6502::ABX, 4 }, // * -> Cycle count can increase
					{ "ASL", &R6502::ASL, &R6502::ABX, 7 },

					{ "???", &R6502::XXX, &R6502::ABX, 7 }, // Illegal -> SLO
				}
			},

			{ // 2x 
				{
					{ "JSR", &R6502::JSR, &R6502::ABS, 6 },
					{ "AND", &R6502::AND, &R6502::IZX, 6 },

					{}, // Illegal -> KIL
					{ "???", &R6502::XXX, &R6502::IZX, 8 }, // Illegal -> RLA

					{ "BIT", &R6502::BIT, &R6502::ZP0, 3 },
					{ "AND", &R6502::AND, &R6502::ZP0, 3 },
					{ "ROL", &R6502::ROL, &R6502::ZP0, 5 },

					{ "???", &R6502::XXX, &R6502::ZP0, 5 }, // Illegal -> RLA

					{ "PLP", &R6502::PLP, &R6502::IMP, 4 },
					{ "AND", &R6502::AND, &R6502::IMM, 2 },
					{ "ROL", &R6502::ROL, &R6502::IMP, 2 },

					{ "???", &R6502::XXX, &R6502::IMM, 2 }, // Illegal -> ANC

					{ "BIT", &R6502::BIT, &R6502::ABS, 4 },
					{ "AND", &R6502::AND, &R6502::ABS, 4 },
					{ "ROL", &R6502::ROL, &R6502::ABS, 6 },

					{ "???", &R6502::XXX, &R6502::ABS, 6 }, // Illegal -> RLA
				}
			},

			{ // 3x 
				{
					{ "BMI", &R6502::BMI, &R6502::REL, 2 }, // * -> Cycle count can increase
					{ "AND", &R6502::AND, &R6502::IZY, 5 },	// * -> Cycle count can increase

					{}, // Illegal -> KIL
					{ "???", &R6502::XXX, &R6502::IZY, 8 }, // Illegal -> RLA
					{ "???", &R6502::NOP, &R6502::ZPX, 4 }, // Illegal -> NOP

					{ "ORA", &R6502::ORA, &R6502::ZPX, 4 },
					{ "ASL", &R6502::ORA, &R6502::ZPX, 6 },

					{ "???", &R6502::XXX, &R6502::ZPX, 6 }, // Illegal -> RLA

					{ "SEC", &R6502::SEC, &R6502::IMP, 2 },
					{ "AND", &R6502::AND, &R6502::ABY, 4 }, // * -> Cycle count can increase

					{ "???", &R6502::NOP, &R6502::IMP, 2 }, // Illegal -> NOP
					{ "???", &R6502::XXX, &R6502::ABY, 7 }, // Illegal -> RLA
					{ "???", &R6502::NOP, &R6502::ABX, 4 }, // Illegal -> NOP | * -> Cycle count can increase

					{ "AND", &R6502::AND, &R6502::ABX, 4 }, // * -> Cycle count can increase
					{ "ROL", &R6502::ROL, &R6502::ABX, 7 },

					{ "???", &R6502::XXX, &R6502::ABX, 7 }, // Illegal -> RLA
				}
			},

			{ // 4x 
				{
					{ "RTI", &R6502::RTI, &R6502::IMP, 6 },
					{ "EOR", &R6502::EOR, &R6502::IZX, 6 },

					{}, // Illegal -> KIL
					{ "???", &R6502::XXX, &R6502::IZX, 8 }, // Illegal -> SRE
					{ "???", &R6502::NOP, &R6502::ZP0, 3 }, // Illegal -> NOP

					{ "EOR", &R6502::EOR, &R6502::ZP0, 3 },
					{ "LSR", &R6502::LSR, &R6502::ZP0, 5 },

					{ "???", &R6502::XXX, &R6502::ZP0, 5 }, // Illegal -> SRE

					{ "PHA", &R6502::PHA, &R6502::IMP, 3 },
					{ "EOR", &R6502::EOR, &R6502::IMM, 2 },
					{ "LSR", &R6502::LSR, &R6502::IMP, 2 },

					{ "???", &R6502::XXX, &R6502::IMM, 2 }, // Illegal -> ALR

					{ "JMP", &R6502::JMP, &R6502::ABS, 3 },
					{ "EOR", &R6502::EOR, &R6502::ABS, 4 },
					{ "LSR", &R6502::LSR, &R6502::ABS, 6 },

					{ "???", &R6502::XXX, &R6502::ABS, 6 }, // Illegal -> SRE
				}
			},

			{ // 5x 
				{
					{ "BVC", &R6502::BVC, &R6502::REL, 2 }, // * -> Cycle count can increase
					{ "EOR", &R6502::EOR, &R6502::IZY, 5 }, // * -> Cycle count can increase

					{}, // Illegal -> KIL
					{ "???", &R6502::XXX, &R6502::IZY, 8 }, // Illegal -> SRE
					{ "???", &R6502::NOP, &R6502::ZPX, 4 }, // Illegal -> NOP

					{ "EOR", &R6502::EOR, &R6502::ZPX, 4 },
					{ "LSR", &R6502::LSR, &R6502::ZPX, 6 },

					{ "???", &R6502::XXX, &R6502::ZPX, 6 }, // Illegal -> SRE

					{ "CLI", &R6502::CLI, &R6502::IMP, 2 },
					{ "EOR", &R6502::EOR, &R6502::ABY, 4 }, // * -> Cycle count can increase

					{ "???", &R6502::NOP, &R6502::IMP, 2 }, // Illegal -> NOP
					{ "???", &R6502::XXX, &R6502::ABY, 7 }, // Illegal -> SRE
					{ "???", &R6502::NOP, &R6502::ABX, 4 }, // Illegal -> NOP | * -> Cycle count can increase

					{ "EOR", &R6502::EOR, &R6502::ABX, 4 }, // * -> Cycle count can increase
					{ "LSR", &R6502::LSR, &R6502::ABX, 7 },

					{ "???", &R6502::XXX, &R6502::ABX, 7 }, // Illegal -> SRE
				}
			},

			{ // 6x 
				{
					{ "RTS", &R6502::RTS, &R6502::IMP, 6 },
					{ "ADC", &R6502::ADC, &R6502::IZX, 6 },

					{}, // Illegal -> KIL
					{ "???", &R6502::XXX, &R6502::IZX, 8 }, // Illegal -> RRA
					{ "???", &R6502::NOP, &R6502::ZP0, 3 }, // Illegal -> NOP

					{ "ADC", &R6502::ADC, &R6502::ZP0, 3 },
					{ "ROR", &R6502::ROR, &R6502::ZP0, 5 },

					{ "???", &R6502::XXX, &R6502::ZP0, 5 }, // Illegal -> RRA

					{ "PLA", &R6502::PLA, &R6502::IMP, 4 },
					{ "ADC", &R6502::ADC, &R6502::IMM, 2 },
					{ "ROR", &R6502::ROR, &R6502::IMP, 2 },

					{ "???", &R6502::XXX, &R6502::IMM, 2 }, // Illegal -> ARR

					{ "JMP", &R6502::JMP, &R6502::IND, 5 },
					{ "ADC", &R6502::ADC, &R6502::ABS, 4 },
					{ "ROR", &R6502::ROR, &R6502::ABS, 6 },

					{ "???", &R6502::XXX, &R6502::ABS, 6 }, // Illegal -> RRA
				}
			},

			{ // 7x 
				{
					{ "BVS", &R6502::BVS, &R6502::REL, 2 }, // * -> Cycle count can increase
					{ "ADC", &R6502::ADC, &R6502::IZY, 5 }, // * -> Cycle count can increase

					{}, // Illegal -> KIL
					{ "???", &R6502::XXX, &R6502::IZY, 8 }, // Illegal -> RRA
					{ "???", &R6502::NOP, &R6502::ZPX, 4 }, // Illegal -> NOP

					{ "ADC", &R6502::EOR, &R6502::ZPX, 4 },
					{ "ROR", &R6502::LSR, &R6502::ZPX, 6 },

					{ "???", &R6502::XXX, &R6502::ZPX, 6 }, // Illegal -> RRA

					{ "SEI", &R6502::SEI, &R6502::IMP, 2 },
					{ "ADC", &R6502::ADC, &R6502::ABY, 4 }, // * -> Cycle count can increase

					{ "???", &R6502::NOP, &R6502::IMP, 2 }, // Illegal -> NOP
					{ "???", &R6502::XXX, &R6502::ABY, 7 }, // Illegal -> RRA
					{ "???", &R6502::NOP, &R6502::ABX, 4 }, // Illegal -> NOP | * -> Cycle count can increase

					{ "ADC", &R6502::EOR, &R6502::ABX, 4 }, // * -> Cycle count can increase
					{ "ROR", &R6502::LSR, &R6502::ABX, 7 },

					{ "???", &R6502::XXX, &R6502::ABX, 7 }, // Illegal -> RRA
				}
			},

			{ // 8x 
				{
					{ "???", &R6502::NOP, &R6502::IMM, 2 }, // Illegal -> NOP

					{ "STA", &R6502::ADC, &R6502::IZX, 6 },

					{ "???", &R6502::NOP, &R6502::IMM, 2 }, // Illegal -> NOP
					{ "???", &R6502::NOP, &R6502::IZX, 6 }, // Illegal -> SAX

					{ "STY", &R6502::STY, &R6502::ZP0, 3 },
					{ "STA", &R6502::STA, &R6502::ZP0, 3 },
					{ "STX", &R6502::STX, &R6502::ZP0, 3 },

					{ "???", &R6502::NOP, &R6502::ZP0, 3 }, // Illegal -> SAX

					{ "DEY", &R6502::DEY, &R6502::IMP, 2 },

					{ "???", &R6502::NOP, &R6502::IMM, 2 }, // Illegal -> NOP

					{ "TXA", &R6502::TXA, &R6502::IMP, 2 },

					{ "???", &R6502::XXX, &R6502::IMM, 2 }, // Illegal -> XAA | RED

					{ "STY", &R6502::STY, &R6502::ABS, 4 },
					{ "STA", &R6502::STA, &R6502::ABS, 4 },
					{ "STX", &R6502::STX, &R6502::ABS, 4 },

					{ "???", &R6502::NOP, &R6502::ABS, 4 }, // Illegal -> SAX
				}
			},

			{ // 9x 
				{
					{ "BCC", &R6502::BCC, &R6502::REL, 2 }, // * -> Cycle count can increase
					{ "STA", &R6502::STA, &R6502::IZY, 6 },

					{}, // Illegal -> KIL
					{ "???", &R6502::XXX, &R6502::IZY, 6 }, // Illegal -> AHX | BLUE

					{ "STY", &R6502::STY, &R6502::ZPX, 4 },
					{ "STA", &R6502::STA, &R6502::ZPX, 4 },
					{ "STX", &R6502::STX, &R6502::ZPY, 4 },

					{ "???", &R6502::XXX, &R6502::ZPY, 4 }, // Illegal -> SAX

					{ "TYA", &R6502::TYA, &R6502::IMP, 2 },
					{ "STA", &R6502::STA, &R6502::ABY, 5 },
					{ "TXS", &R6502::TXS, &R6502::IMP, 2 },

					{ "???", &R6502::XXX, &R6502::ABY, 5 }, // Illegal -> TAS | BLUE
					{ "???", &R6502::XXX, &R6502::ABX, 5 }, // Illegal -> SHY | BLUE

					{ "STA", &R6502::STA, &R6502::ABX, 5 },

					{ "???", &R6502::XXX, &R6502::ABY, 5 }, // Illegal -> SHX | BLUE
					{ "???", &R6502::XXX, &R6502::ABY, 5 }, // Illegal -> AHX | BLUE
				}
			},

			{ // Ax 
				{
					{ "LDY", &R6502::LDY, &R6502::IMM, 2 },
					{ "LDA", &R6502::LDA, &R6502::IZX, 6 },
					{ "LDX", &R6502::LDX, &R6502::IMM, 2 },

					{ "???", &R6502::XXX, &R6502::IZX, 6 }, // Illegal -> LAX

					{ "LDY", &R6502::LDY, &R6502::ZP0, 3 },
					{ "LDA", &R6502::LDA, &R6502::ZP0, 3 },
					{ "LDX", &R6502::LDX, &R6502::ZP0, 3 },

					{ "???", &R6502::XXX, &R6502::ZP0, 3 }, // Illegal -> LAX

					{ "TAY", &R6502::TAY, &R6502::IMP, 2 },
					{ "LDA", &R6502::LDA, &R6502::IMM, 2 },
					{ "TAX", &R6502::TAX, &R6502::IMP, 2 },

					{ "???", &R6502::XXX, &R6502::IMM, 2 }, // Illegal -> LAX | RED

					{ "LDY", &R6502::LDY, &R6502::ABS, 4 },
					{ "LDA", &R6502::LDA, &R6502::ABS, 4 },
					{ "LDX", &R6502::LDX, &R6502::ABS, 4 },

					{ "???", &R6502::XXX, &R6502::ABS, 4 }, // Illegal -> LAX
				}
			},

			{ // Bx 
				{
					{ "BCS", &R6502::BCS, &R6502::REL, 2 }, // * -> Cycle count can increase
					{ "LDA", &R6502::LDA, &R6502::IZY, 5 }, // * -> Cycle count can increase

					{}, // Illegal -> KIL
					{ "???", &R6502::XXX, &R6502::IZY, 5 }, // Illegal -> LAX | * -> Cycle count can increase

					{ "LDY", &R6502::LDY, &R6502::ZPX, 4 },
					{ "LDA", &R6502::LDA, &R6502::ZPX, 4 },
					{ "LDX", &R6502::LDX, &R6502::ZPY, 4 },

					{ "???", &R6502::XXX, &R6502::ZPY, 4 }, // Illegal -> LAX

					{ "CLV", &R6502::CLV, &R6502::IMP, 2 },
					{ "LDA", &R6502::LDA, &R6502::ABY, 4 }, // * -> Cycle count can increase
					{ "TSX", &R6502::TSX, &R6502::IMP, 2 },

					{ "???", &R6502::XXX, &R6502::ABY, 4 }, // Illegal -> LAS | // * -> Cycle count can increase

					{ "LDY", &R6502::LDY, &R6502::ABX, 4 }, // * -> Cycle count can increase
					{ "LDA", &R6502::LDA, &R6502::ABX, 4 },	// * -> Cycle count can increase
					{ "LDX", &R6502::LDX, &R6502::ABY, 4 },	// * -> Cycle count can increase

					{ "???", &R6502::XXX, &R6502::ABX, 4 }, // Illegal -> LAX | * -> Cycle count can increase
				}
			},

			{ // Cx 
				{
					{ "CPY", &R6502::CPY, &R6502::IMM, 2 },
					{ "CMP", &R6502::CMP, &R6502::IZX, 6 },

					{ "???", &R6502::NOP, &R6502::IMM, 2 }, // Illegal -> NOP
					{ "???", &R6502::XXX, &R6502::IZX, 8 }, // Illegal -> DCP

					{ "CPY", &R6502::CPY, &R6502::ZP0, 3 },
					{ "CMP", &R6502::CMP, &R6502::ZP0, 3 },
					{ "DEC", &R6502::DEC, &R6502::ZP0, 5 },

					{ "???", &R6502::XXX, &R6502::ZP0, 5 }, // Illegal -> DCP

					{ "INY", &R6502::INY, &R6502::IMP, 2 },
					{ "CMP", &R6502::CMP, &R6502::IMM, 2 },
					{ "DEX", &R6502::DEX, &R6502::IMP, 2 },

					{ "???", &R6502::XXX, &R6502::IMM, 2 }, // Illegal -> AXS

					{ "CPY", &R6502::CPY, &R6502::ABS, 4 },
					{ "CMP", &R6502::CMP, &R6502::ABS, 4 },
					{ "DEC", &R6502::DEC, &R6502::ABS, 6 },

					{ "???", &R6502::XXX, &R6502::ABS, 6 }, // Illegal -> DCP
				}
			},

			{ // Dx 
				{
					{ "BNE", &R6502::BNE, &R6502::REL, 2 }, // * -> Cycle count can increase
					{ "CMP", &R6502::CMP, &R6502::IZY, 5 }, // * -> Cycle count can increase

					{}, // Illegal -> KIL
					{ "???", &R6502::XXX, &R6502::IZY, 8 }, // Illegal -> DCP
					{ "???", &R6502::NOP, &R6502::ZPX, 4 }, // Illegal -> NOP

					{ "CMP", &R6502::CMP, &R6502::ZPX, 4 },
					{ "DEC", &R6502::DEC, &R6502::ZPX, 6 },

					{ "???", &R6502::XXX, &R6502::ZPX, 6 }, // Illegal -> SCP

					{ "CLD", &R6502::CLD, &R6502::IMP, 2 },
					{ "CMP", &R6502::CMP, &R6502::ABY, 4 }, // * -> Cycle count can increase

					{ "???", &R6502::NOP, &R6502::IMP, 2 }, // Illegal -> NOP
					{ "???", &R6502::XXX, &R6502::ABY, 7 }, // Illegal -> DCP
					{ "???", &R6502::NOP, &R6502::ABX, 4 }, // Illegal -> NOP | * -> Cycle count can increase

					{ "CMP", &R6502::CMP, &R6502::ABX, 4 }, // * -> Cycle count can increase
					{ "DEC", &R6502::DEC, &R6502::ABX, 7 },

					{ "???", &R6502::XXX, &R6502::ABX, 7 }, // Illegal -> DCP
				}
			},

			{ // Ex 
				{
					{ "CPX", &R6502::CPX, &R6502::IMM, 2 },
					{ "SBC", &R6502::SBC, &R6502::IZX, 6 },

					{ "???", &R6502::NOP, &R6502::IMM, 2 }, // Illegal -> NOP
					{ "???", &R6502::XXX, &R6502::IZX, 8 }, // Illegal -> ISC

					{ "CPX", &R6502::CPX, &R6502::ZP0, 3 },
					{ "SBC", &R6502::SBC, &R6502::ZP0, 3 },
					{ "INC", &R6502::INC, &R6502::ZP0, 5 },

					{ "???", &R6502::XXX, &R6502::ZP0, 5 }, // Illegal -> ISC

					{ "INX", &R6502::INX, &R6502::IMP, 2 },
					{ "SBC", &R6502::SBC, &R6502::IMM, 2 },
					{ "NOP", &R6502::NOP, &R6502::IMP, 2 },

					{ "???", &R6502::XXX, &R6502::IMM, 2 }, // Illegal -> SBC

					{ "CPX", &R6502::CPX, &R6502::ABS, 4 },
					{ "SBC", &R6502::SBC, &R6502::ABS, 4 },
					{ "INC", &R6502::INC, &R6502::ABS, 6 },

					{ "???", &R6502::XXX, &R6502::ABS, 6 }, // Illegal -> ISC
				}
			},

			{ // Fx 
				{
					{ "BEQ", &R6502::BEQ, &R6502::REL, 2 }, // * -> Cycle count can increase
					{ "SBC", &R6502::SBC, &R6502::IZY, 5 }, // * -> Cycle count can increase

					{}, // Illegal -> KIL
					{ "???", &R6502::XXX, &R6502::IZY, 8 }, // Illegal -> ISC
					{ "???", &R6502::NOP, &R6502::ZPX, 4 }, // Illegal -> NOP

					{ "SBC", &R6502::SBC, &R6502::ZPX, 4 },
					{ "INC", &R6502::INC, &R6502::ZPX, 6 },

					{ "???", &R6502::XXX, &R6502::ZPX, 6 }, // Illegal -> ISC

					{ "SED", &R6502::SED, &R6502::IMP, 2 },
					{ "SBC", &R6502::SBC, &R6502::ABY, 4 }, // * -> Cycle count can increase

					{ "???", &R6502::NOP, &R6502::IMP, 2 }, // Illegal -> NOP
					{ "???", &R6502::XXX, &R6502::ABY, 7 }, // Illegal -> ISC
					{ "???", &R6502::NOP, &R6502::ABX, 4 }, // Illegal -> NOP | * -> Cycle count can increase

					{ "SBC", &R6502::SBC, &R6502::ABX, 4 }, // * -> Cycle count can increase
					{ "INC", &R6502::INC, &R6502::ABX, 7 },

					{ "???", &R6502::XXX, &R6502::ABX, 7 }, // Illegal -> ISC
				}
			},

			} // Array Bracket
		}; // Lookup Table for OP-Codes : 
		// NOTE: * -> add 1 cycle if page boundary is crossed and/or add 1 cycle on branches if taken.

		
		void SetFlag(StateFlags status, bool value) {
			_status_register = value ? _status_register | status : _status_register & ~status; // Bitwise OR if value is true, otherwise Bitwise XOR
		}

		constexpr u8 GetFlag(StateFlags status) {
			assert(status < R6502::count);

			switch (status)
			{
			case R6502::C: return _status_register & 0x01;
			case R6502::Z: return (_status_register & 0x02) >> 1;
			case R6502::I: return (_status_register & 0x04) >> 2;
			case R6502::D: return (_status_register & 0x08) >> 3;
			case R6502::B: return (_status_register & 0x10) >> 4;
			case R6502::U: return (_status_register & 0x20) >> 5;
			case R6502::V: return (_status_register & 0x40) >> 6;
			case R6502::N: return (_status_register & 0x80) >> 7;
			default: return 0;
			}
		}

		void debug_status_register();

		// Writes to the Memory on the Address Bus
		void write_memory(u16 address) {
			_bus->write(address, _data);
			clock();
		}

		// Writes to the Accumulator on the Chip
		void write_accumulator(u16) {
			// no clock
			_accumulator = _data;
		}

		// Reads from the Memory on the Address Bus
		u8 read_accumulator(u16 address, bool bReadOnly = false) {
			u8 data{ _accumulator };
			return data;
		}

		// Reads from the Memory on the Address Bus
		u8 read_memory(u16 address, bool bReadOnly = false) {
			u8 data{ _bus->read(address) };
			clock();
			return data;
		}

		// Handles interrupt calls and points to the Respective Handler
		void interrupt() {

			// Write Next Program Counter to the Stack
			_data = (_program_counter >> 8) & 0x00FF;
			write_memory(0x0100 + _stack_pointer);
			--_stack_pointer;
			_data = _program_counter & 0x00FF;
			write_memory(0x0100 + _stack_pointer);
			--_stack_pointer;

			// Write Status Flag to the Stack
			_data = _status_register;
			write_memory(0x0100 + _stack_pointer);
			--_stack_pointer;
			SetFlag(StateFlags::I, 1);

			// Get Interrupt Handler's Address
			u16 l_address = read_memory(_address_abs + 0);
			u16 h_address = read_memory(_address_abs + 1);

			_program_counter = (h_address << 8) | l_address;
		}

		void do_nothing_like_its_nobodys_business() { }

		void assign_delay_interrupt_disable_change() {
			delay_change = &R6502::interrupt_disable_change;
			delay_assign = &R6502::do_nothing_like_its_nobodys_business;
		}
		
		void interrupt_disable_change() {
			SetFlag(StateFlags::I, _delay_change_value);
			delay_change = &R6502::do_nothing_like_its_nobodys_business;
#if CPU_TEST
			debug_status_register();
#endif // CPU_TEST
		}
	};
}