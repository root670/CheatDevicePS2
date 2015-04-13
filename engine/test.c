/*
 * Simple test driver for cheat engine
 *
 * Copyright (C) 2009-2010 Mathias Lafeldt <misfire@debugon.org>
 *
 * This file is part of PS2rd, the PS2 remote debugger.
 *
 * PS2rd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PS2rd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PS2rd.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id$
 */

#include <tamtypes.h>
#include <stdio.h>
#include <string.h>

extern void CodeHandler();

extern u32 maxcodes;
extern u32 numcodes;
extern u32 codelist[];


static void clear_codes(void)
{
	int i;

	for (i = 0; i < numcodes * 2; i++)
		codelist[i] = 0;

	numcodes = 0;
}

static void add_code(u32 addr, u32 val)
{
	codelist[numcodes * 2] = addr;
	codelist[numcodes * 2 + 1] = val;
	numcodes++;
}

static int test_type_0(void)
{
	const u32 addr = 0x00400000;
	u32 *p = (u32*)addr;

	clear_codes();
	p[0] = 0xffffffff;
	add_code(addr + 0, 0x00000012);
	add_code(addr + 1, 0x00000034);
	add_code(addr + 2, 0x00000056);
	add_code(addr + 3, 0x00000078);
	CodeHandler();

	if (p[0] != 0x78563412) {
		printf("test for type 0 failed\n");
		return -1;
	}

	return 0;
}

static int test_type_1(void)
{
	const u32 addr = 0x00400100;
	u32 *p = (u32*)addr;

	clear_codes();
	p[0] = 0xffffffff;
	add_code(addr + 0x10000000, 0x00001234);
	add_code(addr + 0x10000002, 0x00005678);
	CodeHandler();

	if (p[0] != 0x56781234) {
		printf("test for type 1 failed\n");
		return -1;
	}

	return 0;
}

static int test_type_2(void)
{
	const u32 addr = 0x00400200;
	u32 *p = (u32*)addr;

	clear_codes();
	p[0] = 0xffffffff;
	p[1] = 0xffffffff;
	add_code(addr + 0x20000000, 0x12345678);
	add_code(addr + 0x20000004, 0x9abcdef0);
	CodeHandler();

	if (p[0] != 0x12345678 || p[1] != 0x9abcdef0) {
		printf("test for type 2 failed\n");
		return -1;
	}

	return 0;
}

static int test_type_3(void)
{
	const u32 addr = 0x00400300;
	u32 *p = (u32*)addr;

	clear_codes();
	p[0] = 0x33333333;
	p[1] = 0x33333333;
	p[2] = 0x33333333;
	add_code(0x30000011, addr);
	add_code(0x301000c6, addr);
	add_code(0x30208907, addr + 4);
	add_code(0x3030b2e9, addr + 4);
	add_code(0x30400000, addr + 8);
	add_code(0xb16b00b5, 0x00000000);
	add_code(0x30500000, addr + 8);
	add_code(0x12345678, 0x00000000);
	CodeHandler();

	if (p[0] != 0x3333337e || p[1] != 0x33330951 || p[2] != 0xd269dd70) {
		printf("test for type 3 failed\n");
		return -1;
	}

	return 0;
}

static int test_type_4(void)
{
	const u32 addr = 0x00400400;
	u32 *p = (u32*)addr;

	clear_codes();
	memset(p, -1, 32);
	add_code(addr + 0x40000000, 0x00040002);
	add_code(0x80000000, 0x00100000);
	CodeHandler();

	if (p[0] != 0x80000000 || p[2] != 0x80100000 ||
	    p[4] != 0x80200000 || p[6] != 0x80300000) {
		printf("test for type 4 failed\n");
		return -1;
	}

	return 0;
}

static int test_type_5(void)
{
	static u8 source[16] = {
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf
	};
	static u8 dest[16];

	clear_codes();
	memset(dest, -1, sizeof(dest));
	add_code((u32)source + 0x50000000, sizeof(source));
	add_code((u32)dest, 0x00000000);
	CodeHandler();

	if (memcmp(source, dest, sizeof(source))) {
		printf("test for type 5 failed\n");
		return -1;
	}

	return 0;
}

static int test_type_6(void)
{
	const u32 addr = 0x00400600;
	u32 *p = (u32*)addr;

	clear_codes();
	p[0] = 0x00400604;
	p[2] = 0xffffffff;
	p[3] = 0xffffffff;
	add_code(addr + 0x60000000, 0x00000012);
	add_code(0x00000000, 0x00000004);
	add_code(addr + 0x60000000, 0x00001234);
	add_code(0x00010000, 0x00000006);
	add_code(addr + 0x60000000, 0x12345678);
	add_code(0x00020000, 0x00000008);
	CodeHandler();

	if (p[2] != 0x1234ff12 || p[3] != 0x12345678) {
		printf("test for type 6 failed\n");
		return -1;
	}

	return 0;
}

static int test_type_7(void)
{
	const u32 addr = 0x00400700;
	u32 *p = (u32*)addr;

	clear_codes();
	p[0] = 0x56565656;
	p[1] = 0x56565656;
	add_code(addr + 0x70000000, 0x00000078);
	add_code(addr + 0x70000001, 0x00200078);
	add_code(addr + 0x70000002, 0x00400078);
	add_code(addr + 0x70000004, 0x00107878);
	add_code(addr + 0x70000004, 0x00300f0f);
	add_code(addr + 0x70000004, 0x00501234);
	CodeHandler();

	if (p[0] != 0x562e507e || p[1] != 0x56561c3a) {
		printf("test for type 7 failed\n");
		return -1;
	}

	return 0;
}

static int test_type_c(void)
{
	const u32 addr = 0x00400c00;
	u32 *p = (u32*)addr;

	clear_codes();
	memset(p, 0xff, 12);
	add_code(addr + 0xc0000000, 0x12345678);
	add_code(addr + 0x20000004, 0x11111111);
	add_code(addr + 0x20000008, 0x22222222);
	CodeHandler();

	if (p[1] != 0xffffffff || p[2] != 0xffffffff) {
		printf("test #1 for type C failed\n");
		return -1;
	}

	p[0] = 0x12345678;
	CodeHandler();

	if (p[1] != 0x11111111 || p[2] != 0x22222222) {
		printf("test #2 for type C failed\n");
		return -1;
	}

	return 0;
}

static int test_type_d(void)
{
	const u32 addr = 0x00400d00;
	u16 *p = (u16*)addr;

	clear_codes();
	p[0] = 0x1234;
	p[1] = 0x1235;
	p[2] = 0x1233;
	p[3] = 0x1235;
	p[4] = 0x0101;
	p[5] = 0x1006;
	p[6] = 0x0000;
	p[7] = 0x1011;
	p[8] = 0x0000;
	add_code(addr + 0xd0000000, 0x10010034);
	add_code(addr + 0xd0000002, 0x0f110034);
	add_code(addr + 0xd0000004, 0x0e210034);
	add_code(addr + 0xd0000006, 0x0d310034);
	add_code(addr + 0xd0000008, 0x0c410034);
	add_code(addr + 0xd000000a, 0x0b510034);
	add_code(addr + 0xd000000c, 0x0a610000);
	add_code(addr + 0xd000000e, 0x09710034);
	add_code(addr + 0xd0000000, 0x08001234);
	add_code(addr + 0xd0000002, 0x07101234);
	add_code(addr + 0xd0000004, 0x06201234);
	add_code(addr + 0xd0000006, 0x05301234);
	add_code(addr + 0xd0000008, 0x04401234);
	add_code(addr + 0xd000000a, 0x03501234);
	add_code(addr + 0xd000000c, 0x02600000);
	add_code(addr + 0xd000000e, 0x00701234);
	add_code(addr + 0x10000010, 0x00001234);
	CodeHandler();

	if (p[8] != 0x1234) {
		printf("test for type D failed\n");
		return -1;
	}

	return 0;
}

static int test_type_e(void)
{
	const u32 addr = 0x00400e00;
	u16 *p = (u16*)addr;

	clear_codes();
	p[0] = 0x1234;
	p[1] = 0x1235;
	p[2] = 0x1233;
	p[3] = 0x1235;
	p[4] = 0x0101;
	p[5] = 0x1006;
	p[6] = 0x0000;
	p[7] = 0x1011;
	p[8] = 0x0000;
	add_code(0xe1100034, addr + 0x00000000);
	add_code(0xe10f0034, addr + 0x10000002);
	add_code(0xe10e0034, addr + 0x20000004);
	add_code(0xe10d0034, addr + 0x30000006);
	add_code(0xe10c0034, addr + 0x40000008);
	add_code(0xe10b0034, addr + 0x5000000a);
	add_code(0xe10a0000, addr + 0x6000000c);
	add_code(0xe1090034, addr + 0x7000000e);
	add_code(0xe0081234, addr + 0x00000000);
	add_code(0xe0071234, addr + 0x10000002);
	add_code(0xe0061234, addr + 0x20000004);
	add_code(0xe0051234, addr + 0x30000006);
	add_code(0xe0041234, addr + 0x40000008);
	add_code(0xe0031234, addr + 0x5000000a);
	add_code(0xe0020000, addr + 0x6000000c);
	add_code(0xe0011234, addr + 0x7000000e);
	add_code(addr + 0x10000010, 0x00001234);
	CodeHandler();

	if (p[8] != 0x1234) {
		printf("test for type E failed\n");
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	printf("Start testing...\n");

	test_type_0();
	test_type_1();
	test_type_2();
	test_type_3();
	test_type_4();
	test_type_5();
	test_type_6();
	test_type_7();
	test_type_c();
	test_type_d();
	test_type_e();

	printf("Testing done.\n");

	return 0;
}
