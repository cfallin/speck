/*
 * kernel/i386/kernel.ld
 */

OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS
{
	. = 0x100000;
	.text :
	{
		_stext = .;
		*(.text)
		_etext = .;
	}
	.rodata :
	{
		_srodata = .;
		*(.rodata)
		_erodata = .;
	}
	.data :
	{
		_sdata = .;
		*(.data)
		_edata = .;
	}
	.bss :
	{
		_sbss = .;
		*(.bss)
		*(COMMON)
		_ebss = .;
	}
}
