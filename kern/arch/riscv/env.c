#include <trap.h>
#include <env.h>
#include <assert.h>
#include <arch/arch.h>
#include <pmap.h>

void
env_push_ancillary_state(env_t* e)
{
	if(e->scp_ctx.hw_tf.sr & SR_EF)
		save_fp_state(&e->env_ancillary_state);
}

void
save_fp_state(ancillary_state_t* silly)
{
	return; // don't save FP state for now
	uintptr_t sr = mfpcr(PCR_SR);
	mtpcr(PCR_SR, sr | SR_EF);

	asm("mffsr %0" : "=r"(silly->fsr));

	asm("fsd f0,%0" : "=m"(silly->fpr[0]));
	asm("fsd f1,%0" : "=m"(silly->fpr[1]));
	asm("fsd f2,%0" : "=m"(silly->fpr[2]));
	asm("fsd f3,%0" : "=m"(silly->fpr[3]));
	asm("fsd f4,%0" : "=m"(silly->fpr[4]));
	asm("fsd f5,%0" : "=m"(silly->fpr[5]));
	asm("fsd f6,%0" : "=m"(silly->fpr[6]));
	asm("fsd f7,%0" : "=m"(silly->fpr[7]));
	asm("fsd f8,%0" : "=m"(silly->fpr[8]));
	asm("fsd f9,%0" : "=m"(silly->fpr[9]));
	asm("fsd f10,%0" : "=m"(silly->fpr[10]));
	asm("fsd f11,%0" : "=m"(silly->fpr[11]));
	asm("fsd f12,%0" : "=m"(silly->fpr[12]));
	asm("fsd f13,%0" : "=m"(silly->fpr[13]));
	asm("fsd f14,%0" : "=m"(silly->fpr[14]));
	asm("fsd f15,%0" : "=m"(silly->fpr[15]));
	asm("fsd f16,%0" : "=m"(silly->fpr[16]));
	asm("fsd f17,%0" : "=m"(silly->fpr[17]));
	asm("fsd f18,%0" : "=m"(silly->fpr[18]));
	asm("fsd f19,%0" : "=m"(silly->fpr[19]));
	asm("fsd f20,%0" : "=m"(silly->fpr[20]));
	asm("fsd f21,%0" : "=m"(silly->fpr[21]));
	asm("fsd f22,%0" : "=m"(silly->fpr[22]));
	asm("fsd f23,%0" : "=m"(silly->fpr[23]));
	asm("fsd f24,%0" : "=m"(silly->fpr[24]));
	asm("fsd f25,%0" : "=m"(silly->fpr[25]));
	asm("fsd f26,%0" : "=m"(silly->fpr[26]));
	asm("fsd f27,%0" : "=m"(silly->fpr[27]));
	asm("fsd f28,%0" : "=m"(silly->fpr[28]));
	asm("fsd f29,%0" : "=m"(silly->fpr[29]));
	asm("fsd f30,%0" : "=m"(silly->fpr[30]));
	asm("fsd f31,%0" : "=m"(silly->fpr[31]));

	mtpcr(PCR_SR, sr);
}

void
env_pop_ancillary_state(env_t* e)
{ 
	if(e->scp_ctx.hw_tf.sr & SR_EF)
		restore_fp_state(&e->env_ancillary_state);
}

void
restore_fp_state(ancillary_state_t* silly)
{
	return; // don't restore FP state for now
	uintptr_t sr = mfpcr(PCR_SR);
	mtpcr(PCR_SR, sr | SR_EF);

	asm("mtfsr %0" : : "r"(silly->fsr));

	asm("fld f0,%0" : : "m"(silly->fpr[0]));
	asm("fld f1,%0" : : "m"(silly->fpr[1]));
	asm("fld f2,%0" : : "m"(silly->fpr[2]));
	asm("fld f3,%0" : : "m"(silly->fpr[3]));
	asm("fld f4,%0" : : "m"(silly->fpr[4]));
	asm("fld f5,%0" : : "m"(silly->fpr[5]));
	asm("fld f6,%0" : : "m"(silly->fpr[6]));
	asm("fld f7,%0" : : "m"(silly->fpr[7]));
	asm("fld f8,%0" : : "m"(silly->fpr[8]));
	asm("fld f9,%0" : : "m"(silly->fpr[9]));
	asm("fld f10,%0" : : "m"(silly->fpr[10]));
	asm("fld f11,%0" : : "m"(silly->fpr[11]));
	asm("fld f12,%0" : : "m"(silly->fpr[12]));
	asm("fld f13,%0" : : "m"(silly->fpr[13]));
	asm("fld f14,%0" : : "m"(silly->fpr[14]));
	asm("fld f15,%0" : : "m"(silly->fpr[15]));
	asm("fld f16,%0" : : "m"(silly->fpr[16]));
	asm("fld f17,%0" : : "m"(silly->fpr[17]));
	asm("fld f18,%0" : : "m"(silly->fpr[18]));
	asm("fld f19,%0" : : "m"(silly->fpr[19]));
	asm("fld f20,%0" : : "m"(silly->fpr[20]));
	asm("fld f21,%0" : : "m"(silly->fpr[21]));
	asm("fld f22,%0" : : "m"(silly->fpr[22]));
	asm("fld f23,%0" : : "m"(silly->fpr[23]));
	asm("fld f24,%0" : : "m"(silly->fpr[24]));
	asm("fld f25,%0" : : "m"(silly->fpr[25]));
	asm("fld f26,%0" : : "m"(silly->fpr[26]));
	asm("fld f27,%0" : : "m"(silly->fpr[27]));
	asm("fld f28,%0" : : "m"(silly->fpr[28]));
	asm("fld f29,%0" : : "m"(silly->fpr[29]));
	asm("fld f30,%0" : : "m"(silly->fpr[30]));
	asm("fld f31,%0" : : "m"(silly->fpr[31]));

	mtpcr(PCR_SR, sr);
}

static int
user_mem_walk_recursive(env_t* e, uintptr_t start, size_t len,
                        mem_walk_callback_t callback, void* arg,
                        mem_walk_callback_t pt_callback, void* pt_arg,
                        pte_t* pt, int level)
{
	int ret = 0;
	int pgshift = L1PGSHIFT - level*(L1PGSHIFT-L2PGSHIFT);
	uintptr_t pgsize = 1UL << pgshift;

	uintptr_t start_idx = (start >> pgshift) & (NPTENTRIES-1);
	uintptr_t end_idx = ((start+len-1) >> pgshift) & (NPTENTRIES-1);

	for(uintptr_t idx = start_idx; idx <= end_idx; idx++)
	{
		uintptr_t pgaddr = ROUNDDOWN(start, pgsize) + (idx-start_idx)*pgsize;
		pte_t* pte = &pt[idx];

		if(*pte & PTE_T)
		{
			assert(level < NPTLEVELS-1);
			uintptr_t st = MAX(pgaddr, start);
			size_t ln = MIN(start + len, pgaddr + pgsize) - st;
			if((ret = user_mem_walk_recursive(e, st, ln, callback, arg,
			                                  pt_callback, pt_arg,
			                                  KADDR(PTD_ADDR(*pte)), level+1)))
				goto out;
			if(pt_callback != NULL && (ret = pt_callback(e, pte, (void*)pgaddr, arg)))
				goto out;
		}
		else if(callback != NULL && !PAGE_UNMAPPED(*pte))
			if((ret = callback(e, pte, (void*)pgaddr, arg)))
				goto out;
	}
out:
	return ret;
}

int
env_user_mem_walk(env_t* e, void* start, size_t len,
                  mem_walk_callback_t callback, void* arg)
{
	assert(PGOFF(start) == 0 && PGOFF(len) == 0);
	return user_mem_walk_recursive(e, (uintptr_t)start, len, callback, arg,
	                               NULL, NULL, e->env_pgdir, 0);
}

void
env_pagetable_free(env_t* e)
{
	int pt_free(env_t* e, pte_t* pte, void* va, void* arg)
	{
		page_decref(pa2page(PTD_ADDR(*pte)));
		return 0;
	}

	assert(user_mem_walk_recursive(e, 0, KERNBASE, NULL, NULL,
	                               pt_free, NULL, e->env_pgdir, 0) == 0);
}
