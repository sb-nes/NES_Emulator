#pragma once

#if _DEBUG

/// TEST_INTERRUPT_HANDLER ///
// LDA ZP
// data
// SEC IMP -> for normal subtraction
// SBC IMM
// data
// LDA ZP
// data
// RTI

#define TEST_INTERRUPT_HANDLER			\
_ram->write(0x0700, 0xA9);				\
_ram->write(0x0701, 0x02);				\
_ram->write(0x0702, 0x38);				\
_ram->write(0x0703, 0xE9);				\
_ram->write(0x0704, 0x04);				\
_ram->write(0x0705, 0xA9);				\
_ram->write(0x0706, 0x8D);				\
_ram->write(0x0707, 0x40);				

/// Subroutine ///
// LDA
// data
// INC ZP
// data
// LDX ZP
// data
// DEC ZPX
// data
// LDA IMM
// data
// CMP
// data
// LDX
// data
// CPX
// data
// LDY
// data
// CPY
// data
// RTI

#define TEST_SUBROUTINE					\
_ram->write(0x0730, 0xA9);				\
_ram->write(0x0731, 0x40);				\
_ram->write(0x0732, 0xE6);				\
_ram->write(0x0733, 0x10);				\
_ram->write(0x0734, 0xA6);				\
_ram->write(0x0735, 0x10);				\
_ram->write(0x0736, 0xD6);				\
_ram->write(0x0737, 0x06);				\
_ram->write(0x0738, 0xA9);				\
_ram->write(0x0739, 0x18);				\
_ram->write(0x073A, 0xC9);				\
_ram->write(0x073B, 0x17);				\
_ram->write(0x073C, 0xA2);				\
_ram->write(0x073D, 0x15);				\
_ram->write(0x073E, 0xE0);				\
_ram->write(0x073F, 0x18);				\
_ram->write(0x0740, 0xA0);				\
_ram->write(0x0741, 0xC1);				\
_ram->write(0x0742, 0xC0);				\
_ram->write(0x0743, 0xC0);				\
_ram->write(0x0744, 0x60);				


/// CPU TEST A ///
// data
// data
// LDA immediate
// 50
// LDA zero page
// $0010

// ADC immediate
// $0010
// ADC Absolute
// $0010
// $0010

// LDY ZP
// $0010
// LDX IMM
// $0010

// LDY ZPX
// $0010

// JMP ABS
// $0010
// $0010

// $0010
// $0010

// LDA ABY
// $0010
// $0010

// Data
// Data

// TSX
// LDA ZP
// Data
// TAX
// TXS

// STA ZP
// Data

#define TEST_PROGRAM_A					\
_ram->write(0x0010, 0x02);				\
_ram->write(0x0015, 0x08);				\
_ram->write(0x0000, 0xA9);				\
_ram->write(0x0001, 0x32);				\
_ram->write(0x0002, 0xA5);				\
_ram->write(0x0003, 0x10);				\
										\
_ram->write(0x0004, 0x69);				\
_ram->write(0x0005, 0x15);				\
_ram->write(0x0006, 0x6D);				\
_ram->write(0x0007, 0x15);				\
_ram->write(0x0008, 0x00);				\
										\
_ram->write(0x0009, 0xB4);				\
_ram->write(0x000A, 0x10);				\
_ram->write(0x000B, 0xA0);				\
_ram->write(0x000C, 0x03);				\
										\
_ram->write(0x000D, 0xA0);				\
_ram->write(0x000E, 0x03);				\
										\
_ram->write(0x000D, 0x6C);				\
_ram->write(0x000E, 0x0E);				\
_ram->write(0x000F, 0x01);				\
										\
_ram->write(0x010E, 0x2D);				\
_ram->write(0x010F, 0x04);				\
										\
_ram->write(0x042D, 0xBE);				\
_ram->write(0x042E, 0xFE);				\
_ram->write(0x042F, 0x01);				\
										\
_ram->write(0x0201, 0x6B);				\
_ram->write(0x0204, 0x5B);				\
										\
_ram->write(0x0430, 0xBA);				\
_ram->write(0x0431, 0xA5);				\
_ram->write(0x0432, 0x15);				\
_ram->write(0x0433, 0xAA);				\
_ram->write(0x0434, 0x9A);				\
										\
_ram->write(0x0435, 0x85);				\
_ram->write(0x0436, 0x00);		

/// CPU TEST B ///
// SEC
// SED
// SEI

// CLC
// CLI
// CLV
// CLD

// data -> Bit Mask
// data
// LDA ZP
// data
// BIT ZP0
// data
// BIT ABS
// data
// data

// INX
// INX
// DEX
// INY
// DEY

// data
// LDX
// Data 
// TXS

// NOP
// PHA
// PHP
// PLP
// PLA

// AND
// data
// EOR
// data
// ORA
// data

// JSR
// low
// high

// ORA
// data

// ASL
// LSR

// ROL
// SEC
// ROL

// CLC
// ROR
// SEC
// ROR
#define TEST_PROGRAM_B					\
_ram->write(0x0437, 0x38);				\
_ram->write(0x0438, 0xF8);				\
_ram->write(0x0439, 0x78);				\
										\
_ram->write(0x043A, 0x18);				\
_ram->write(0x043B, 0x58);				\
_ram->write(0x043C, 0xB8);				\
_ram->write(0x043D, 0xD8);				\
										\
_ram->write(0x00FA, 0xFF);				\
_ram->write(0x032C, 0xFA);				\
_ram->write(0x043E, 0xA5);				\
_ram->write(0x043F, 0xFA);				\
_ram->write(0x0440, 0x24);				\
_ram->write(0x0441, 0x2C);				\
_ram->write(0x0442, 0x2C);				\
_ram->write(0x0443, 0x2C);				\
_ram->write(0x0444, 0x03);				\
										\
_ram->write(0x0445, 0xE8);				\
_ram->write(0x0446, 0xE8);				\
_ram->write(0x0447, 0xCA);				\
_ram->write(0x0448, 0xC8);				\
_ram->write(0x0449, 0x88);				\
										\
_ram->write(0x00AB, 0xFD);				\
_ram->write(0x044A, 0xA6);				\
_ram->write(0x044B, 0xAB);				\
_ram->write(0x044C, 0x9A);				\
										\
_ram->write(0x044D, 0xEA);				\
_ram->write(0x044E, 0x48);				\
_ram->write(0x044F, 0x08);				\
_ram->write(0x0450, 0x28);				\
_ram->write(0x0451, 0x68);				\
										\
_ram->write(0x0454, 0x29);				\
_ram->write(0x0455, 0x68);				\
_ram->write(0x0456, 0x49);				\
_ram->write(0x0457, 0x68);				\
_ram->write(0x0458, 0x09);				\
_ram->write(0x0459, 0x0F);				\
										\
_ram->write(0x045A, 0x20);				\
_ram->write(0x045B, 0x30);				\
_ram->write(0x045C, 0x07);				\
										\
_ram->write(0x045D, 0x09);				\
_ram->write(0x045E, 0x0A);				\
										\
_ram->write(0x045F, 0x0A);				\
_ram->write(0x0460, 0x4A);				\
										\
_ram->write(0x0461, 0x2A);				\
_ram->write(0x0462, 0x38);				\
_ram->write(0x0463, 0x2A);				\
										\
_ram->write(0x0464, 0x18);				\
_ram->write(0x0465, 0x6A);				\
_ram->write(0x0466, 0x38);				\
_ram->write(0x0467, 0x6A);				

/// CPU Branching TEST ///
// BCC
// memory to skip
// LDY IMM
// data
// BNE
// memory to skip
// SEC
// BCS
// memory to skip
// LDA
// data
// BEQ
// memory to skip
// BPL
// memory to skip
// BVC
// memory to skip
// BVS
// memory to skip
// BCC
//	
// LOOP:						   \
// DEY <- start <-------------------\
// CPY								|\
// data 0x07						| \
// BNE								| /
// memory to skip -> goto start ____|/
// CLC								/
// CLD							   /
// CLI
// CLV
#define TEST_PROGRAM_BRANCH				\
_ram->write(0x0468, 0x90); 				\
_ram->write(0x0469, 0x04); 				\
										\
_ram->write(0x046A, 0xA0); 				\
_ram->write(0x046B, 0x0B); 				\
										\
_ram->write(0x046C, 0xD0); 				\
_ram->write(0x046D, 0x09); 				\
										\
_ram->write(0x046E, 0x38); 				\
_ram->write(0x046F, 0xB0); 				\
_ram->write(0x0470, 0xF9); 				\
										\
_ram->write(0x0477, 0xA5); 				\
_ram->write(0x0478, 0x01); 				\
										\
_ram->write(0x0479, 0xF0); 				\
_ram->write(0x047A, 0x02); 				\
										\
_ram->write(0x047B, 0x10); 				\
_ram->write(0x047C, 0x00); 				\
										\
_ram->write(0x047D, 0x50); 				\
_ram->write(0x047E, 0x01); 				\
										\
_ram->write(0x047F, 0x70); 				\
_ram->write(0x0480, 0x03); 				\
										\
_ram->write(0x0484, 0x88); 				\
										\
_ram->write(0x0485, 0xC0); 				\
_ram->write(0x0486, 0x07); 				\
										\
_ram->write(0x0487, 0xD0); 				\
_ram->write(0x0488, 0xFB);				\
										\
_ram->write(0x0489, 0x18);				\
_ram->write(0x048A, 0xD8);				\
_ram->write(0x048B, 0x58);				\
_ram->write(0x048C, 0xB8);				

#else
#define CPU_TEST_CODE 
#endif