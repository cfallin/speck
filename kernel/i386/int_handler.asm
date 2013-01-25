; kernel/i386/int_handler.asm

bits 32

section .text

extern idt

; init routine
global int_handler_init
int_handler_init:
	push ebp
	mov ebp, esp

	mov esi, int_handler_table ; get table addr
	mov ecx, int_handler_table_size/8 ; size
	; loop through the handlers
.handler_loop:
	mov ebx, [esi] ; get vector
	shl ebx, 3 ; ebx = idt offset
	add ebx, idt ; ebx = desc addr
	mov eax, [esi+4] ; handler addr
	mov [ebx], ax ; handler addr low
	shr eax, 16
	mov [ebx+6], ax ; handler addr high
	mov word [ebx+2], 8 ; cs
	mov byte [ebx+4], 0 ; reserved
	mov byte [ebx+5], 0x8f ; access byte - trap, DPL0
	add esi, 8 ; next
	loop .handler_loop

	; set the pagefault handler to an interrupt gate - we don't want a nested
	; interrupt triggering another pagefault and overwriting CR2
	mov byte [idt+(14*8)+5], 0x8e

	; set the perms of the syscall - trap, DPL3
	mov byte [idt+(0x80*8)+5], 0xef
	; set the perms of the resched call - same as above
	mov byte [idt+(0x81*8)+5], 0xef

	; load the IDTR
extern idt_desc
	mov word [idt_desc], (256*8)-1 ; size
	mov dword [idt_desc+2], idt ; addr
	lidt [idt_desc]

	; set up the 8259s

	; initialize the master PIC
	mov al, 0x11
	out 0x20, al
	mov al, 0x20
	out 0x21, al
	mov al, 0x04
	out 0x21, al
	mov al, 0x01
	out 0x21, al
	mov al, 0x00
	out 0x21, al

	; initialize the slave PIC
	mov al, 0x11
	out 0xa0, al
	mov al, 0x28
	out 0xa1, al
	mov al, 0x02
	out 0xa1, al
	mov al, 0x01
	out 0xa1, al
	mov al, 0x00
	out 0xa1, al

	; unmask all interrupts
	mov al, 0x00
	out 0x21, al
	out 0xa1, al

	leave
	ret

; define the common handler
int_handler_common:
	; save regs - see include/kernel/i386/context.h
	push gs
	push fs
	push es
	push ds
	push ebp
	push edi
	push esi
	push edx
	push ecx
	push ebx
	push eax

	; load seg regs
	mov eax, 0x10
	mov ds, ax
	mov es, ax

	; gcc expects this
	cld

	; call handler
	push esp
extern int_handler_dispatch
	call int_handler_dispatch
	mov esp, eax

	; restore regs
	pop eax
	pop ebx
	pop ecx
	pop edx
	pop esi
	pop edi
	pop ebp
	pop ds
	pop es
	pop fs
	pop gs
	add esp, 8 ; pop vector,errcode off of stack

	; return
	iret


; define the individial handlers

%macro int_handler 2
global int_handler_%1
int_handler_%1:
%if (%2 == 0)
	push dword 0 ; fake error code
%endif
	push dword %1 ; vector #
	jmp int_handler_common
%endmacro

; define each one
int_handler 0x00, 0
int_handler 0x01, 0
int_handler 0x02, 0
int_handler 0x03, 0
int_handler 0x04, 0
int_handler 0x05, 0
int_handler 0x06, 0
int_handler 0x07, 0
int_handler 0x08, 1
int_handler 0x09, 0
int_handler 0x0a, 1
int_handler 0x0b, 1
int_handler 0x0c, 1
int_handler 0x0d, 1
int_handler 0x0e, 1
int_handler 0x0f, 1
int_handler 0x10, 0
int_handler 0x11, 0
int_handler 0x12, 0
int_handler 0x13, 0
int_handler 0x14, 0
int_handler 0x15, 0
int_handler 0x16, 0
int_handler 0x17, 0
int_handler 0x18, 0
int_handler 0x19, 0
int_handler 0x1a, 0
int_handler 0x1b, 0
int_handler 0x1c, 0
int_handler 0x1d, 0
int_handler 0x1e, 0
int_handler 0x1f, 0
int_handler 0x20, 0
int_handler 0x21, 0
int_handler 0x22, 0
int_handler 0x23, 0
int_handler 0x24, 0
int_handler 0x25, 0
int_handler 0x26, 0
int_handler 0x27, 0
int_handler 0x28, 0
int_handler 0x29, 0
int_handler 0x2a, 0
int_handler 0x2b, 0
int_handler 0x2c, 0
int_handler 0x2d, 0
int_handler 0x2e, 0
int_handler 0x2f, 0
int_handler 0x80, 0
int_handler 0x81, 0

; define the table of all handlers - makes init code easier
section .data
int_handler_table:
dd 0x00, int_handler_0x00
dd 0x01, int_handler_0x01
dd 0x02, int_handler_0x02
dd 0x03, int_handler_0x03
dd 0x04, int_handler_0x04
dd 0x05, int_handler_0x05
dd 0x06, int_handler_0x06
dd 0x07, int_handler_0x07
dd 0x08, int_handler_0x08
dd 0x09, int_handler_0x09
dd 0x0a, int_handler_0x0a
dd 0x0b, int_handler_0x0b
dd 0x0c, int_handler_0x0c
dd 0x0d, int_handler_0x0d
dd 0x0e, int_handler_0x0e
dd 0x0f, int_handler_0x0f
dd 0x10, int_handler_0x10
dd 0x11, int_handler_0x11
dd 0x12, int_handler_0x12
dd 0x13, int_handler_0x13
dd 0x14, int_handler_0x14
dd 0x15, int_handler_0x15
dd 0x16, int_handler_0x16
dd 0x17, int_handler_0x17
dd 0x18, int_handler_0x18
dd 0x19, int_handler_0x19
dd 0x1a, int_handler_0x1a
dd 0x1b, int_handler_0x1b
dd 0x1c, int_handler_0x1c
dd 0x1d, int_handler_0x1d
dd 0x1e, int_handler_0x1e
dd 0x1f, int_handler_0x1f
dd 0x20, int_handler_0x20
dd 0x21, int_handler_0x21
dd 0x22, int_handler_0x22
dd 0x23, int_handler_0x23
dd 0x24, int_handler_0x24
dd 0x25, int_handler_0x25
dd 0x26, int_handler_0x26
dd 0x27, int_handler_0x27
dd 0x28, int_handler_0x28
dd 0x29, int_handler_0x29
dd 0x2a, int_handler_0x2a
dd 0x2b, int_handler_0x2b
dd 0x2c, int_handler_0x2c
dd 0x2d, int_handler_0x2d
dd 0x2e, int_handler_0x2e
dd 0x2f, int_handler_0x2f
dd 0x80, int_handler_0x80
dd 0x81, int_handler_0x81
int_handler_table_size equ $-int_handler_table
