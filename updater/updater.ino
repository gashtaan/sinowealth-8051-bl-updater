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

#include "icp.h"
#include "serial.h"
#include "data.h"
#include "config.h"

static const uint8_t VDD = 6; // D6

uint8_t buffer[16] = {};

ICP icp;

bool eraseBootloader()
{
	serialWrite("Erasing bootloader\r\n");

	for (uint32_t a = (CHIP_FLASH_SIZE - 0x1000); a < CHIP_FLASH_SIZE; a += CHIP_SECTOR_SIZE)
		if (!icp.eraseFlash(a))
			return false;

	return true;
}

bool programBootloader()
{
	serialWrite("Programming bootloader\r\n");

	for (uint16_t n = 0; n < sizeof(bootloader); n += sizeof(buffer))
	{
		for (uint16_t m = 0; m < sizeof(buffer); ++m)
			buffer[m] = pgm_read_byte_near(bootloader + n + m);

		if (!icp.writeFlash((CHIP_FLASH_SIZE - 0x1000 + n), buffer, sizeof(buffer)))
			return false;
	}

	return true;
}

bool verifyBootloader()
{
	serialWrite("Verifying bootloader\r\n");

	for (uint16_t n = 0; n < sizeof(bootloader); n += sizeof(buffer))
	{
		icp.readFlash((CHIP_FLASH_SIZE - 0x1000 + n), buffer, sizeof(buffer));

		for (uint16_t m = 0; m < sizeof(buffer); ++m)
			if (buffer[m] != pgm_read_byte_near(bootloader + n + m))
				return false;
	}

	return true;
}

int main()
{
	serialInit();
	serialWrite("\r\nSinoWealth 8051-based MCU bootloader updater\r\n");

	// power-up the device
	DDRD |= _BV(VDD);
	setBit(VDD);
	_delay_ms(1);

	icp.connect();

	if (icp.check())
	{
		serialWrite("Connection established\r\n");

		if (eraseBootloader() && programBootloader() && verifyBootloader())
			serialWrite("Done!\r\n");
		else
			serialWrite("Failed!\r\n");
	}
	else
	{
		serialWrite("Connection failed\r\n");
	}

	icp.disconnect();

	// power-down the device
	clrBit(VDD);
}
