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

#include <avr/io.h>
#include <util/delay.h>

#include "config.h"
#include "icp.h"

ICP::ICP()
{
	DDRD &= ~_BV(TDO);
	DDRD |= _BV(TDI);
	DDRD |= _BV(TMS);
	DDRD |= _BV(TCK);

	clrBit(TCK);
	clrBit(TDI);
	clrBit(TMS);
}

void ICP::connect()
{
	setBit(TCK);
	setBit(TDI);
	setBit(TMS);

	_delay_us(500);

	clrBit(TCK);
	_delay_us(1);
	setBit(TCK);
	_delay_us(50);

	for (uint8_t n = 0; n < 165; ++n)
	{
		clrBit(TMS);
		_delay_us(2);
		setBit(TMS);
		_delay_us(2);
	}

	for (uint8_t n = 0; n < 105; ++n)
	{
		clrBit(TDI);
		_delay_us(2);
		setBit(TDI);
		_delay_us(2);
	}

	for (uint8_t n = 0; n < 90; ++n)
	{
		clrBit(TCK);
		_delay_us(2);
		setBit(TCK);
		_delay_us(2);
	}

	for (uint16_t n = 0; n < 25600; ++n)
	{
		clrBit(TMS);
		_delay_us(2);
		setBit(TMS);
		_delay_us(2);
	}

	_delay_us(8);

	clrBit(TMS);

	m_mode = Mode::ICP;
	startMode();

	for (uint16_t n = 0; n < 25600; ++n)
	{
		setBit(TCK);
		_delay_us(2);
		clrBit(TCK);
		_delay_us(2);
	}

	reset();
}

void ICP::disconnect()
{
	// for debugging purposes it's convenient to leave connection in ICP mode as it will survive host reset/upload
	// (TCK must be held high in READY state, if it's set low during host reset/upload, target will disconnect)
	switchMode(Mode::ICP);
}

void ICP::reset()
{
	if (m_mode == Mode::ERROR)
		return;

	setBit(TCK);

	setBit(TMS);
	_delay_us(2);
	clrBit(TMS);
	_delay_us(2);

	m_mode = Mode::READY;
}

void ICP::switchMode(Mode mode)
{
	if (m_mode == mode)
		return;

	if (m_mode != Mode::READY)
		reset();

	m_mode = mode;
	startMode();

	if (m_mode == Mode::ICP)
	{
		_delay_us(800);

		ping();
	}
}

void ICP::startMode() const
{
	clrBit(TCK);
	_delay_us(2);

	for (uint8_t m = 0x80; m; m >>= 1)
	{
		if (uint8_t(m_mode) & m)
			setBit(TDI);
		else
			clrBit(TDI);

		setBit(TCK);
		_delay_us(2);
		clrBit(TCK);
		_delay_us(2);
	}

	setBit(TCK);
	_delay_us(2);
	clrBit(TCK);
	_delay_us(2);

	setBit(TCK);
	_delay_us(2);
	clrBit(TCK);
	_delay_us(2);
}

bool ICP::check()
{
	switchMode(Mode::ICP);

	sendData(ICP_SET_IB_OFFSET_L);
	sendData(0x69);
	sendData(ICP_SET_IB_OFFSET_H);
	sendData(0xFF);

	sendData(ICP_GET_IB_OFFSET);
	auto b = receiveData();
	(void)receiveData();

	return (b == 0x69);
}

void ICP::ping() const
{
	if (m_mode != Mode::ICP)
		return;

	sendData(ICP_PING);
	sendData(0xFF);
}

bool ICP::readFlash(uint32_t address, uint8_t* buffer, uint8_t bufferSize)
{
	switchMode(Mode::ICP);

#if CHIP_TYPE != 1
	sendData(0x46);
	sendData(0xFE);
	sendData(0xFF);
#endif

	sendData(ICP_SET_IB_OFFSET_L);
	sendData(address & 0x000000FF);
	sendData(ICP_SET_IB_OFFSET_H);
	sendData((address & 0x0000FF00) >> 8);
#if CHIP_TYPE == 4 || CHIP_TYPE == 7
	sendData(ICP_SET_XPAGE);
	sendData((address & 0x00FF0000) >> 16);
#endif

	sendData(ICP_READ_FLASH);

	for (uint8_t n = 0; n < bufferSize; ++n)
		buffer[n] = receiveData();

	reset();

	return true;
}

bool ICP::writeFlash(uint32_t address, const uint8_t* buffer, uint16_t bufferSize)
{
	if (bufferSize == 0)
		return false;

	switchMode(Mode::ICP);

#if CHIP_TYPE != 1
	sendData(0x46);
	sendData(0xF0);
	sendData(0xFF);
#endif

	sendData(ICP_SET_IB_OFFSET_L);
	sendData(address & 0x000000FF);
	sendData(ICP_SET_IB_OFFSET_H);
	sendData((address & 0x0000FF00) >> 8);
#if CHIP_TYPE == 4 || CHIP_TYPE == 7
	sendData(ICP_SET_XPAGE);
	sendData((address & 0x00FF0000) >> 16);
#endif

	sendData(ICP_SET_IB_DATA);
	sendData(buffer[0]);

	sendData(0x6E);
	sendData(0x15);
	sendData(0x0A);
	sendData(0x09);
	sendData(0x06);

	for (uint16_t n = 1; n < bufferSize; ++n)
	{
		sendData(buffer[n]);
		_delay_us(5);
		sendData(0x00);
	}

	sendData(0x00);
	sendData(0xAA);
	sendData(0x00);
	sendData(0x00);
	_delay_us(5);

	return true;
}

bool ICP::eraseFlash(uint32_t address)
{
	switchMode(Mode::ICP);

#if CHIP_TYPE != 1
	sendData(0x46);
	sendData(0xF0);
	sendData(0xFF);
#endif

	sendData(ICP_SET_IB_OFFSET_L);
	sendData(address & 0x000000FF);
	sendData(ICP_SET_IB_OFFSET_H);
	sendData((address & 0x0000FF00) >> 8);
#if CHIP_TYPE == 4 || CHIP_TYPE == 7
	sendData(ICP_SET_XPAGE);
	sendData((address & 0x00FF0000) >> 16);
#endif

	sendData(ICP_SET_IB_DATA);
	sendData(0x00);

	sendData(0xE6);
	sendData(0x15);
	sendData(0x0A);
	sendData(0x09);
	sendData(0x06);

	sendData(0x00);
	_delay_ms(300);
	sendData(0x00);
	bool status = getBit(TDO);
	sendData(0x00);

	return status;
}

void ICP::sendData(uint8_t value)
{
	for (uint8_t m = 0x80; m; m >>= 1)
	{
		if (value & m)
			setBit(TDI);
		else
			clrBit(TDI);

		pulseClock();
	}

	pulseClock();

	clrBit(TDI);
}

uint8_t ICP::receiveData()
{
	uint8_t value = 0;
	for (uint8_t m = 1; m; m <<= 1)
	{
		pulseClock();

		if (getBit(TDO))
			value |= m;
	}

	pulseClock();

	return value;
}

void ICP::pulseClock()
{
	_delay_us(1);
	setBit(TCK);
	_delay_us(1);
	clrBit(TCK);
}
