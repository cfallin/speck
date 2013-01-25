/*
 * kernel/i386/string.S
 */

/* note - this is in AT&T asm only because it was taken from one of my old
   projects and I'm too lazy to rewrite it */

.text

// void *memcpy(void *to, void *from, int count)
.globl memcpy
memcpy:
	pushl %ebp
	movl %esp,%ebp
	pusha
	movl 8(%ebp),%edi
	movl 12(%ebp),%esi
	movl 16(%ebp),%ecx
	cld
	rep movsb
	movl 8(%ebp),%eax
	movl %eax,28(%esp)
	popa
	leave
	ret
	
// void *memset(void *block, int c, int size);
.global memset
memset:
	pushl %ebp
	movl %esp,%ebp
	pusha
	movl 8(%ebp),%edi
	movl 12(%ebp),%eax
	movl 16(%ebp),%ecx
	cld
	rep stosb
	movl 8(%ebp),%eax
	movl %eax,28(%esp)
	popa
	leave
	ret

// int strcmp(char *s1, char *s2)
.global strcmp
strcmp:
	pushl %ebp
	movl %esp, %ebp
	pusha
	
	movl 8(%ebp),%esi
	movl 12(%ebp), %edi
	
1:
	movb (%esi),%al
	incl %esi
	movb (%edi),%bl
	incl %edi

	orb %al, %al
	jz 2f
	orb %bl, %bl
	jz 2f
	cmp %al, %bl
	jne 2f

	jmp 1b

2:
	cmp %al, %bl
	ja 3f
	jb 4f
	jmp 5f

3:
	movl $1, 28(%esp)
	jmp 6f
4:
	movl $-1, 28(%esp)
	jmp 6f
5:
	movl $0, 28(%esp)
6:
	popa
	leave
	ret

// int strncmp(char *s1, char *s2, size_t len)
.global strncmp
strncmp:
	pushl %ebp
	movl %esp, %ebp
	pusha

	movl 8(%ebp),%esi
	movl 12(%ebp),%edi
	movl 16(%ebp),%ecx

1:
	movb (%esi),%al
	incl %esi
	movb (%edi),%bl
	incl %edi

	orb %al, %al
	jz 2f
	orb %bl, %bl
	jz 2f
	cmp %al, %bl
	jne 2f
	decl %ecx
	jz 2f

	jmp 1b

2:
	cmp %al, %bl
	ja 3f
	jb 4f
	jmp 5f

3:
	movl $1, 28(%esp)
	jmp 6f
4:
	movl $-1, 28(%esp)
	jmp 6f
5:
	movl $0, 28(%esp)
6:
	popa
	leave
	ret

// char *strcpy(char *dest, char *src)
.global strcpy
strcpy:
	pushl %ebp
	movl %esp, %ebp
	pusha

	movl 8(%ebp), %edi
	movl 12(%ebp), %esi
	movl %edi, 28(%esp)

1:
	movb (%esi), %al
	incl %esi
	movb %al, (%edi)
	incl %edi
	
	orb %al, %al
	jnz 1b
	
	popa
	leave
	ret

// char *strncpy(char *dest, char *src, size_t len)
.global strncpy
strncpy:
	pushl %ebp
	movl %esp, %ebp
	pusha

	movl 8(%ebp), %edi
	movl 12(%ebp), %esi
	movl 16(%ebp), %ecx
	movl %edi, 28(%esp)

1:
	orl %ecx, %ecx
	jz 2f
	
	movb (%esi), %al
	incl %esi
	movb %al, (%edi)
	incl %edi

	decl %ecx

	orb %al, %al
	jnz 1b
	
2:
	popa
	leave
	ret

// size_t strlen(char *str)
.global strlen
strlen:
	pushl %ebp
	movl %esp, %ebp
	pusha

	movl 8(%ebp), %esi
	
1:
	movb (%esi), %al
	orb %al, %al
	jz 2f
	incl %esi
	jmp 1b

2:
	movl 8(%ebp), %eax
	subl %eax, %esi
	movl %esi, 28(%esp)	
	
	popa
	leave
	ret
