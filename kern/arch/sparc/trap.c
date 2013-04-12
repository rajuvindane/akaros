#include <arch/arch.h>
#include <assert.h>
#include <trap.h>
#include <string.h>
#include <process.h>
#include <syscall.h>
#include <monitor.h>
#include <manager.h>
#include <stdio.h>
#include <smp.h>
#include <slab.h>
#include <mm.h>
#include <umem.h>
#include <pmap.h>
#include <kdebug.h>

#ifdef __SHARC__
#pragma nosharc
#endif

#ifdef __DEPUTY__
#pragma nodeputy
#endif

/* Warning: SPARC's trap handlers do not increment the ktrap depth */

/* These are the stacks the kernel will load when it receives a trap from user
 * space.  The deal is that they get set right away in entry.S, and can always
 * be used for finding the top of the stack (from which you should subtract the
 * sizeof the trapframe.  Note, we need to have a junk value in the array so
 * that this is NOT part of the BSS.  If it is in the BSS, it will get 0'd in
 * kernel_init(), which is after these values get set.
 *
 * TODO: if these end up becoming contended cache lines, move this to
 * per_cpu_info. */
uintptr_t core_stacktops[MAX_NUM_CPUS] = {0xcafebabe, 0};

void advance_pc(struct hw_trapframe *state)
{
	state->pc = state->npc;
	state->npc += 4;
}

/* Set stacktop for the current core to be the stack the kernel will start on
 * when trapping/interrupting from userspace */
void set_stack_top(uintptr_t stacktop)
{
	core_stacktops[core_id()] = stacktop;
}

/* Note the assertion assumes we are in the top page of the stack. */
uintptr_t get_stack_top(void)
{
	uintptr_t sp, stacktop;
	stacktop = core_stacktops[core_id()];
	asm volatile("mov %%sp,%0" : "=r"(sp));
	assert(ROUNDUP(sp, PGSIZE) == stacktop);
	return stacktop;
}

/* Starts running the current TF. */
void pop_kernel_ctx(struct kernel_ctx *ctx)
{
	/* TODO! also do save_kernel_tf() in kern/arch/sparc/trap.h */
	panic("Not implemented.  =(");
}

/* Does nothing on sparc... */
void send_nmi(uint32_t os_coreid)
{
}

void
idt_init(void)
{
}

void
sysenter_init(void)
{
}

/* Helper.  For now, this copies out the TF to pcpui, and sets cur_ctx to point
 * to it. */
static void set_current_ctx_hw(struct per_cpu_info *pcpui,
                               struct hw_trapframe *hw_tf)
{
	if (irq_is_enabled())
		warn("Turn off IRQs until cur_ctx is set!");
	assert(!pcpui->cur_ctx);
	pcpui->actual_ctx.type = ROS_HW_CTX;
	pcpui->actual_ctx.hw_tf = *hw_tf;
	pcpui->cur_ctx = &pcpui->actual_ctx;
}

static void set_current_ctx_sw(struct per_cpu_info *pcpui,
                               struct sw_trapframe *sw_tf)
{
	if (irq_is_enabled())
		warn("Turn off IRQs until cur_ctx is set!");
	assert(!pcpui->cur_ctx);
	pcpui->actual_ctx.type = ROS_SW_CTX;
	pcpui->actual_ctx.sw_tf = *sw_tf;
	pcpui->cur_ctx = &pcpui->actual_ctx;
}

static int format_trapframe(struct hw_trapframe *hw_tf, char *buf, int bufsz)
{
	// slightly hackish way to read out the instruction that faulted.
	// not guaranteed to be right 100% of the time
	uint32_t insn;
	if(!(current && !memcpy_from_user(current, &insn, (void*)hw_tf->pc, 4)))
		insn = -1;

	int len = snprintf(buf,bufsz,"TRAP frame at %p on core %d\n",
	                   hw_tf, core_id());

	for(int i = 0; i < 8; i++)
	{
		len += snprintf(buf+len,bufsz-len,
		                "  g%d   0x%08x  o%d   0x%08x"
		                "  l%d   0x%08x  i%d   0x%08x\n",
		                i, hw_tf->gpr[i], i, hw_tf->gpr[i+8],
		                i, hw_tf->gpr[i+16], i, hw_tf->gpr[i+24]);
	}

	len += snprintf(buf+len,bufsz-len,
	                "  psr  0x%08x  pc   0x%08x  npc  0x%08x  insn 0x%08x\n",
	                hw_tf->psr, hw_tf->pc, hw_tf->npc,insn);
	len += snprintf(buf+len,bufsz-len,
	                "  y    0x%08x  fsr  0x%08x  far  0x%08x  tbr  0x%08x\n",
	                hw_tf->y, hw_tf->fault_status, hw_tf->fault_addr,
	                hw_tf->tbr);
	len += snprintf(buf+len,bufsz-len,
	                "  timestamp  %21lld\n", hw_tf->timestamp);

	return len;
}

void print_trapframe(struct hw_trapframe *hw_tf)
{
	char buf[1024];
	int len = format_trapframe(hw_tf, buf, sizeof(buf));
	cputbuf(buf,len);
}

#define TRAPNAME_MAX	32

static char*
get_trapname(uint8_t tt, char buf[TRAPNAME_MAX])
{
	static const char* trapnames[] = {
		[0x00] "reset",
		[0x01] "instruction access exception",
		[0x02] "illegal instruction",
		[0x03] "privileged instruction",
		[0x04] "floating point disabled",
		[0x05] "window overflow",
		[0x06] "window underflow",
		[0x07] "memory address not aligned",
		[0x08] "floating point exception",
		[0x09] "data access exception",
		[0x20] "register access error",
		[0x21] "instruction access error",
		[0x24] "coprocessor disabled",
		[0x25] "unimplemented FLUSH",
		[0x28] "coprocessor exception",
		[0x29] "data access error",
		[0x2A] "division by zero",
		[0x2B] "data store error",
		[0x2C] "data MMU miss",
		[0x3C] "instruction MMU miss"
	};

	if(tt >= 0x80)
		snprintf(buf,TRAPNAME_MAX,"user trap 0x%02x",tt);
	else if(tt >= 0x10 && tt < 0x20)
		snprintf(buf,TRAPNAME_MAX,"interrupt 0x%x",tt-0x10);
	else if(tt >= sizeof(trapnames)/sizeof(trapnames[0]) || !trapnames[tt])
		snprintf(buf,TRAPNAME_MAX,"(unknown trap 0x%02x)",tt);
	else
	{
		strncpy(buf,trapnames[tt],TRAPNAME_MAX);
		buf[TRAPNAME_MAX-1] = 0;
	}

	return buf;
}

/* Assumes that any IPI you get is really a kernel message */
void handle_ipi(struct hw_trapframe *hw_tf)
{
	struct per_cpu_info *pcpui = &per_cpu_info[core_id()];
	if (!in_kernel(hw_tf))
		set_current_ctx_hw(pcpui, hw_tf);
	else if((void*)hw_tf->pc == &__cpu_halt) // break out of the __cpu_halt loop
		advance_pc(hw_tf);

	inc_irq_depth(pcpui);
	handle_kmsg_ipi(hw_tf, 0);
	dec_irq_depth(pcpui);
}

void unhandled_trap(struct hw_trapframe *state)
{
	char buf[TRAPNAME_MAX];
	uint32_t trap_type = (state->tbr >> 4) & 0xFF;
	get_trapname(trap_type,buf);

	static spinlock_t screwup_lock = SPINLOCK_INITIALIZER;
	spin_lock(&screwup_lock);

	if(in_kernel(state))
	{
		print_trapframe(state);
		panic("Unhandled trap in kernel!\nTrap type: %s",buf);
	}
	else
	{
		char tf_buf[1024];
		int tf_len = format_trapframe(state,tf_buf,sizeof(tf_buf));

		warn("Unhandled trap in user!\nTrap type: %s\n%s",buf,tf_buf);
		backtrace();
		spin_unlock(&screwup_lock);

		assert(current);
		enable_irq();
		proc_destroy(current);
		/* Not sure if SPARC has a central point that would run proc_restartcore
		 */
		proc_restartcore();
	}
}

static hw_trapframe *stack_fucked(struct hw_trapframe *state)
{
	warn("You just got stack fucked!");
	extern char tflush1, tflush2;
	if(state->pc == (uint32_t)&tflush1 || state->pc == (uint32_t)&tflush2)
		return (struct hw_trapframe*)(bootstacktop - core_id()*KSTKSIZE
		                                   - sizeof(struct hw_trapframe));
	return state;
}

void fill_misaligned(struct hw_trapframe *state)
{
	state = stack_fucked(state);
	state->tbr = (state->tbr & ~0xFFF) | 0x070;
	address_unaligned(state);
}

void fill_pagefault(struct hw_trapframe *state)
{
	state = stack_fucked(state);
	state->tbr = (state->tbr & ~0xFFF) | 0x090;
	data_access_exception(state);
}

void spill_misaligned(struct hw_trapframe *state)
{
	fill_misaligned(state);
}

void spill_pagefault(struct hw_trapframe *state)
{
	fill_pagefault(state);
}

void address_unaligned(struct hw_trapframe *state)
{
	unhandled_trap(state);
}

void instruction_access_exception(struct hw_trapframe *state)
{
	if(in_kernel(state) || handle_page_fault(current,state->pc,PROT_EXEC))
		unhandled_trap(state);
}

void data_access_exception(struct hw_trapframe *state)
{
	int prot = (state->fault_status & MMU_FSR_WR) ? PROT_WRITE : PROT_READ;

	if(in_kernel(state) || handle_page_fault(current,state->fault_addr,prot))
		unhandled_trap(state);
}

void illegal_instruction(struct hw_trapframe *state)
{
	unhandled_trap(state);
}

void real_fp_exception(struct hw_trapframe *state,
                       ancillary_state_t *sillystate)
{
	unhandled_trap(state);
}

void fp_exception(struct hw_trapframe *state)
{
	ancillary_state_t sillystate;
	save_fp_state(&sillystate);	

	// since our FP HW exception behavior is sketchy, reexecute
	// any faulting FP instruction in SW, which may call
	// real_fp_exception above
	emulate_fpu(state,&sillystate);

	restore_fp_state(&sillystate);
}

void fp_disabled(struct hw_trapframe *state)
{
	if(in_kernel(state))
		panic("kernel executed an FP instruction!");

	state->psr |= PSR_EF;
}

void handle_pop_tf(struct hw_trapframe *state)
{
	struct per_cpu_info *pcpui = &per_cpu_info[core_id()];
	set_current_ctx_hw(pcpui, state);

	struct hw_trapframe hw_tf, *hw_tf_p = &hw_tf;
	if (memcpy_from_user(current, &hw_tf, (void*)state->gpr[8],sizeof(hw_tf))) {
		enable_irq();
		proc_destroy(current);
		proc_restartcore();
	}

	proc_secure_trapframe(&hw_tf);
	set_current_ctx_hw(pcpui, hw_tf_p);
	proc_restartcore();
}

void handle_set_tf(struct hw_trapframe *state)
{
	advance_pc(state);
	if (memcpy_to_user(current,(void*)state->gpr[8],state,sizeof(*state))) {
		proc_incref(current, 1);
		enable_irq();
		proc_destroy(current);
		proc_restartcore();
	}
}

void handle_syscall(struct hw_trapframe *state)
{
	struct per_cpu_info *pcpui = &per_cpu_info[core_id()];
	uint32_t a0 = state->gpr[1];
	uint32_t a1 = state->gpr[8];

	advance_pc(state);
	enable_irq();
	struct per_cpu_info* coreinfo = &per_cpu_info[core_id()];

	set_current_ctx_hw(pcpui, state);

	prep_syscalls(current, (struct syscall*)a0, a1);

	proc_restartcore();
}

void
flush_windows()
{
	register int foo asm("g1");
	register int nwin asm("g2");
	extern int NWINDOWS;

	nwin = NWINDOWS;
	foo = nwin;

	asm volatile ("1: deccc %0; bne,a 1b; save %%sp,-64,%%sp"
	              : "=r"(foo) : "r"(foo));

	foo = nwin;
	asm volatile ("1: deccc %0; bne,a 1b; restore"
	              : "=r"(foo) : "r"(foo));
}
   
void handle_flushw(struct hw_trapframe *state)
{
	// don't actually need to do anything here.
	// trap_entry flushes user windows to the stack.
	advance_pc(state);
}

void handle_breakpoint(struct hw_trapframe *state)
{
	advance_pc(state);
	monitor(state);
}
