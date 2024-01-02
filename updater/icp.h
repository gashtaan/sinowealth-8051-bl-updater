/*
   https://github.com/gashtaan/sinowealth-8051-bl-updater

   Copyright (C) 2024, Michal Kovacik

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define clrBit(b) (PORTD &= ~_BV(b))
#define setBit(b) (PORTD |= _BV(b))
#define getBit(b) (PIND & _BV(b))

constexpr uint8_t reverseBits(uint8_t b)
{
	return (b & 0x80 ? 0x01 : 0) | (b & 0x40 ? 0x02 : 0) | (b & 0x20 ? 0x04 : 0) | (b & 0x10 ? 0x08 : 0) | (b & 0x08 ? 0x10 : 0) | (b & 0x04 ? 0x20 : 0) | (b & 0x02 ? 0x40 : 0) | (b & 0x01 ? 0x80 : 0);
}

class ICP
{
public:
	ICP();

	static const uint8_t TDO = 2;	// D2
	static const uint8_t TMS = 3;	// D3
	static const uint8_t TDI = 4;	// D4
	static const uint8_t TCK = 5;	// D5

	void connect();
	void disconnect();
	bool check();
	void ping() const;

	bool readFlash(uint32_t address, uint8_t* buffer, uint8_t bufferSize);
	bool writeFlash(uint32_t address, const uint8_t* buffer, uint16_t bufferSize);
	bool eraseFlash(uint32_t address);

private:
	enum class Mode
	{
		ERROR = 0,
		READY = 1,
		ICP = 150
	};

	enum ICPCommand : uint8_t
	{
		ICP_SET_IB_OFFSET_L = 0x40,
		ICP_SET_IB_OFFSET_H = 0x41,
		ICP_SET_IB_DATA = 0x42,
		ICP_GET_IB_OFFSET = 0x43,
		ICP_READ_FLASH = 0x44,
		ICP_PING = 0x49,
		ICP_SET_XPAGE = 0x4C,
	};

	void reset();
	void switchMode(Mode mode);
	void startMode() const;

	static void sendData(uint8_t value);
	static uint8_t receiveData();
	static void pulseClock();

	Mode m_mode = Mode::ERROR;
};
