/*
 * kernel/i386/context.c
 */

#include <kernel/i386/context.h>
#include <kernel/malloc.h>

void context_create_user(context *ctx, int eip, int esp)
{
	user_context *c;

	// allocate a stack
	ctx->stack_base = malloc(8192);
//	memset(ctx->stack_base,0,8192);
	ctx->sp = (void *)((int)ctx->stack_base + 8192 - sizeof(user_context));
	c = (user_context *)ctx->sp;

	// fill out the block
	c->user_esp = esp;
	c->user_ss = 0x23;

	c->ctx.ds = c->ctx.es = c->ctx.fs = c->ctx.gs = 0x23;
	c->ctx.cs = 0x1b;
	c->ctx.eip = eip;
	c->ctx.eflags = 0x202;
}

void context_create_kernel(context *ctx, int eip)
{
	stack_context *c;

	// allocate a stack
	ctx->stack_base = malloc(8192);
//	memset(ctx->stack_base,0,8192);
	ctx->sp = (void *)((int)ctx->stack_base + 8192 - sizeof(stack_context));
	c = (stack_context *)ctx->sp;

	// fill it out
	c->ds = c->es = c->fs = c->gs = 0x10;
	c->cs = 8;
	c->eip = eip;
	c->eflags = 0x202;
}

void context_free(context *ctx)
{
	free(ctx->stack_base);
}
