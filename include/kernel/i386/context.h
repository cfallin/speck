/*
 * include/kernel/i386/context.h
 */

#ifndef _KERNEL_I386_CONTEXT_H_
#define _KERNEL_I386_CONTEXT_H_

// stack frame layout common to user and kernel thread frames
typedef struct stack_context
{
	int eax, ebx, ecx, edx, esi, edi, ebp;
	int ds, es, fs, gs;
	int vector, errcode; // pushed by assembly wrappers
	int eip, cs, eflags; // (non-usermode part of) iret frame
} stack_context;

// user stack frame
typedef struct user_context
{
	stack_context ctx; // context info common to user and kernel mode
	int user_esp, user_ss; // (usermode part of) iret frame
} user_context;

typedef struct context
{
	void *stack_base;
	void *sp;
} context;

void context_create_user(context *ctx, int eip, int esp);
void context_create_kernel(context *ctx, int eip);

void context_free(context *ctx);

#endif
