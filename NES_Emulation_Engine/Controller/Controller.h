#pragma once
#include "../Common/CommonHeaders.h"

namespace NES::Input {

	class Controller {
	public:
		Controller(u8 id) {
			_id = id;
			_register = 0x00;
		}

		virtual void clock() = 0;
		virtual u8 read(u16 address) = 0;
		virtual bool write(u16 address, u8 data) = 0;

		[[nodiscard]] u8 get_register() { return _register; }
		[[nodiscard]] u8 get_latch() { return _strobe_latch; }
		void reset() { _register = 0x00; }
		void set(u8 value) { _register = value; }

	protected:
		u8			_id{ 0 };
		u8			_register{ 0x00 }; // 4021 8-bit Shift Register
		u8			_strobe_latch{ 0 }; // D0 shared by both controllers
		bool		_read_mode{ false };
	};


	class Standard : public Controller {
	public:
		Standard(u8 id) : Controller(id) {}

		void clock() override;
		[[nodiscard]] u8 read(u16 address) override;
		bool write(u16 address, u8 data) override;
	private:
	};
}
