#include "Controller.h"

namespace NES::Input {
	
	u8 Standard::read(u16 address) {
		u8 data = 0x00;
		if (_read_mode) {
			data = _register & 0x01;
			_register >>= 1; // Shift the register Right by 1
		}
		return data;
	}

	bool Standard::write(u16 address, u8 data) {
		_strobe_latch = data;
		return true;
	}

	void Standard::clock() {
		if (_strobe_latch > 0) {
			_read_mode = !_read_mode;
		}
	}
}