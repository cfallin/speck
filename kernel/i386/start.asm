; kernel/i386/start.asm

bits 32

section .data
; multiboot values passed in ebx and eax, respectively, on boot
global mboot_info, mboot_valid
mboot_info:  dd 0
mboot_valid: dd 0

section .text

; multiboot header
align 4
dd 0x1badb002           ; magic
dd 2                    ; flags - require mem_* fields
dd 0 - (0x1badb002 + 2) ; checksum

; entry point
global _start
_start:
	cli ; no ints yet
	mov esp, 0x90000 ; get a valid stack
	mov ebp, esp

	; save the multiboot info
	mov [mboot_info], ebx
	cmp eax, 0x2badb002
	sete [mboot_valid]

	; zero the bss
extern _sbss, _ebss
	mov edi, _ebss
	mov ecx, _ebss
	sub ecx, _sbss
	shr ecx, 2
	xor eax, eax
	rep stosd

	; load our GDT
	lgdt [gdt]
	; load our IDT
	lidt [idt_desc]

	; load new segment reg values
	mov eax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	; load new CS
	jmp 0x8:.new_cs
.new_cs:

	; call the main kernel function
extern kernel_main
	call kernel_main
	; shouldn't return

	cli
	hlt
	jmp $

section .data

; gdt
global gdt
gdt:
	dw gdt_size
	dd gdt
	dw 0
; kernel code
	dw 0xffff, 0
	db 0, 0x9a, 0xcf, 0
; kernel data
	dw 0xffff, 0
	db 0, 0x92, 0xcf, 0
; user code
	dw 0xffff, 0
	db 0, 0xfa, 0xcf, 0
; user data
	dw 0xffff, 0
	db 0, 0xf2, 0xcf, 0
; system tss
	db 0, 0, 0, 0, 0, 0x89, 0, 0
gdt_size equ $-gdt

; idt descriptor
global idt_desc
idt_desc:
	dw 2047
	dd idt

; idt
section .bss
global idt
idt: resb 2048
