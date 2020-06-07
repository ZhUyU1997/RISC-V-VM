OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv", "elf64-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
	ram  : org = 0x00000000, len = 256K
}

SECTIONS
{
	.text :
	{
		*(.text*)
		*(.iplt)
		*(.note.gnu.build-id)
	} > ram

	.data ALIGN(8) :
	{
		*(.sdata*)
		*(.data*)
        *(.rodata*)
		*(.srodata*)
		. = ALIGN(8);
	} > ram

	.bss ALIGN(8) (NOLOAD) :
	{
		*(.bss*)
		*(.sbss*)
		*(COMMON)
		. = ALIGN(8);
	} > ram

    PROVIDE(__stack_end = 0x00000000+256K);
	/DISCARD/ : { *(.dynsym) }
	/DISCARD/ : { *(.dynstr*) }
	/DISCARD/ : { *(.dynamic*) }
	/DISCARD/ : { *(.plt*) }
	/DISCARD/ : { *(.interp*) }
	/DISCARD/ : { *(.gnu*) }
    /DISCARD/ : { *(.debug*) }
}
