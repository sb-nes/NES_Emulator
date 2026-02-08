#pragma once
#include "../Common/CommonHeaders.h"

namespace NES::Utilities {
	static unsigned x_u = 0; // unsigned -> unsigned  int

	// Bit-field utilities
	template <unsigned bitno, unsigned nbits = 1, typename T = u8>
	struct RegBit {
		T data;
		enum { mask = (1U << nbits) - 1U }; // 1U -> unsigned value 1

		// copy-assignment operator
		template<typename T2>
		RegBit& operator= (T2 value) {
			data = (data & ~(mask << bitno)) | ((nbits > 1 ? value & mask : !!value) << bitno);
			return *this;
		}

		// Type-Cast operator -> It also returns the value of data, when the object is called.
		operator unsigned() const { return (data >> bitno) & mask; }

		// Pre-Increment Operator
		RegBit& operator++ () { return *this = *this + 1; }
		// Post-Increment Operator
		unsigned operator++ (int) { unsigned r = *this; ++*this; return r; }
	};
}