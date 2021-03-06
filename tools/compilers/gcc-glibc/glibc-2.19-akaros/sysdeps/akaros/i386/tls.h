/* Definition for thread-local data handling.  nptl/i386 version.
   Copyright (C) 2002-2007, 2009 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _TLS_H
#define _TLS_H	1

#include <dl-sysdep.h>
#ifndef __ASSEMBLER__
# include <stdbool.h>
# include <stddef.h>
# include <stdint.h>
# include <stdlib.h>
//# include <list.h>
# include <sysdep.h>
# include <kernel-features.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <ros/procinfo.h>
#include <ros/procdata.h>
#include <ros/arch/mmu.h>


/* Type for the dtv.  */
typedef union dtv
{
  size_t counter;
  struct
  {
    void *val;
    bool is_static;
  } pointer;
} dtv_t;


typedef struct
{
  void *tcb;		/* Pointer to the TCB.  Not necessarily the
			   thread descriptor used by libpthread.  */
  dtv_t *dtv;
  void *self;		/* Pointer to the thread descriptor.  */
  int multiple_threads;
  uintptr_t sysinfo;
  uintptr_t stack_guard;
  uintptr_t pointer_guard;
  int gscope_flag;
#ifndef __ASSUME_PRIVATE_FUTEX
  int private_futex;
#else
  int __unused1;
#endif
  /* Reservation of some values for the TM ABI.  */
  void *__private_tm[5];
} tcbhead_t;

# define TLS_MULTIPLE_THREADS_IN_TCB 1

typedef struct rthread {
    tcbhead_t header;
} rthread_t;

#else /* __ASSEMBLER__ */
//# include <tcb-offsets.h>
#endif

/* Alignment requirement for the stack.  For IA-32 this is governed by
   the SSE memory functions.  */
#define STACK_ALIGN	16

#ifndef __ASSEMBLER__
/* Get system call information.  */
# include <sysdep.h>

/* The old way: using LDT.  */

/* Structure passed to `modify_ldt', 'set_thread_area', and 'clone' calls.  */
struct user_desc
{
  unsigned int entry_number;
  unsigned long int base_addr;
  unsigned int limit;
  unsigned int seg_32bit:1;
  unsigned int contents:2;
  unsigned int read_exec_only:1;
  unsigned int limit_in_pages:1;
  unsigned int seg_not_present:1;
  unsigned int useable:1;
  unsigned int empty:25;
};

/* Initializing bit fields is slow.  We speed it up by using a union.  */
union user_desc_init
{
  struct user_desc desc;
  unsigned int vals[4];
};


/* Get the thread descriptor definition.  */
//# include <nptl/descr.h>

/* This is the size of the initial TCB.  Can't be just sizeof (tcbhead_t),
   because NPTL getpid, __libc_alloca_cutoff etc. need (almost) the whole
   struct rthread even when not linked with -lpthread.  */
# define TLS_INIT_TCB_SIZE sizeof (struct rthread)

/* Alignment requirements for the initial TCB.  */
# define TLS_INIT_TCB_ALIGN __alignof__ (struct rthread)

/* This is the size of the TCB.  */
# define TLS_TCB_SIZE sizeof (struct rthread)

/* Alignment requirements for the TCB.  */
# define TLS_TCB_ALIGN __alignof__ (struct rthread)

/* The TCB can have any size and the memory following the address the
   thread pointer points to is unspecified.  Allocate the TCB there.  */
# define TLS_TCB_AT_TP	1


/* Install the dtv pointer.  The pointer passed is to the element with
   index -1 which contain the length.  */
# define INSTALL_DTV(descr, dtvp) \
  ((tcbhead_t *) (descr))->dtv = (dtvp) + 1

/* Install new dtv for current thread.  */
# define INSTALL_NEW_DTV(dtvp) \
  ({ struct rthread *__pd;						      \
     THREAD_SETMEM (__pd, header.dtv, (dtvp)); })

/* Return dtv of given thread descriptor.  */
# define GET_DTV(descr) \
  (((tcbhead_t *) (descr))->dtv)

#define THREAD_SELF_SYSINFO	THREAD_GETMEM (THREAD_SELF, header.sysinfo)
#define THREAD_SYSINFO(pd)	((pd)->header.sysinfo)

/* Macros to load from and store into segment registers.  */
# ifndef TLS_GET_GS
#  define TLS_GET_GS() \
  ({ int __seg; __asm ("movw %%gs, %w0" : "=q" (__seg)); __seg & 0xffff; })
# endif
# ifndef TLS_SET_GS
#  define TLS_SET_GS(val) \
  __asm ("movw %w0, %%gs" :: "q" (val))
# endif


# ifndef __NR_set_thread_area
#  define __NR_set_thread_area 243
# endif
# ifndef TLS_FLAG_WRITABLE
#  define TLS_FLAG_WRITABLE		0x00000001
# endif

// XXX Enable for the real world.
#if 0
# ifndef __ASSUME_SET_THREAD_AREA
#  error "we need set_thread_area"
# endif
#endif

# ifdef __PIC__
#  define TLS_EBX_ARG "r"
#  define TLS_LOAD_EBX "xchgl %3, %%ebx\n\t"
# else
#  define TLS_EBX_ARG "b"
#  define TLS_LOAD_EBX
# endif

#if defined NEED_DL_SYSINFO
# define INIT_SYSINFO \
  _head->sysinfo = GLRO(dl_sysinfo)
#else
# define INIT_SYSINFO
#endif

#ifndef LOCK_PREFIX
# ifdef UP
#  define LOCK_PREFIX  /* nothing */
# else
#  define LOCK_PREFIX "lock;"
# endif
#endif

/* Code to initially initialize the thread pointer.  This might need
   special attention since 'errno' is not yet available and if the
   operation can cause a failure 'errno' must not be touched.  */
# define TLS_INIT_TP(thrdescr, secondcall) tls_init_tp(thrdescr)

/* Return the address of the dtv for the current thread.  */
# define THREAD_DTV() \
  ({ struct rthread *__pd;						      \
     THREAD_GETMEM (__pd, header.dtv); })


/* Return the thread descriptor for the current thread.

   The contained asm must *not* be marked volatile since otherwise
   assignments like
        pthread_descr self = thread_self();
   do not get optimized away.  */
# define THREAD_SELF \
  ({ struct rthread *__self;						      \
     asm ("movl %%gs:%c1,%0" : "=r" (__self)				      \
	  : "i" (offsetof (struct rthread, header.self)));		      \
     __self;})

/* Magic for libthread_db to know how to do THREAD_SELF.  */
# define DB_THREAD_SELF \
  REGISTER_THREAD_AREA (32, offsetof (struct user_regs_struct, xgs), 3) \
  REGISTER_THREAD_AREA (64, 26 * 8, 3) /* x86-64's user_regs_struct->gs */


/* Read member of the thread descriptor directly.  */
# define THREAD_GETMEM(descr, member) \
  ({ __typeof (descr->member) __value;					      \
     if (sizeof (__value) == 1)						      \
       asm volatile ("movb %%gs:%P2,%b0"				      \
		     : "=q" (__value)					      \
		     : "0" (0), "i" (offsetof (struct rthread, member)));     \
     else if (sizeof (__value) == 4)					      \
       asm volatile ("movl %%gs:%P1,%0"					      \
		     : "=r" (__value)					      \
		     : "i" (offsetof (struct rthread, member)));	      \
     else								      \
       {								      \
	 if (sizeof (__value) != 8)					      \
	   /* There should not be any value with a size other than 1,	      \
	      4 or 8.  */						      \
	   abort ();							      \
									      \
	 asm volatile ("movl %%gs:%P1,%%eax\n\t"			      \
		       "movl %%gs:%P2,%%edx"				      \
		       : "=A" (__value)					      \
		       : "i" (offsetof (struct rthread, member)),	      \
			 "i" (offsetof (struct rthread, member) + 4));	      \
       }								      \
     __value; })


/* Same as THREAD_GETMEM, but the member offset can be non-constant.  */
# define THREAD_GETMEM_NC(descr, member, idx) \
  ({ __typeof (descr->member[0]) __value;				      \
     if (sizeof (__value) == 1)						      \
       asm volatile ("movb %%gs:%P2(%3),%b0"				      \
		     : "=q" (__value)					      \
		     : "0" (0), "i" (offsetof (struct rthread, member[0])),   \
		     "r" (idx));					      \
     else if (sizeof (__value) == 4)					      \
       asm volatile ("movl %%gs:%P1(,%2,4),%0"				      \
		     : "=r" (__value)					      \
		     : "i" (offsetof (struct rthread, member[0])),	      \
		       "r" (idx));					      \
     else								      \
       {								      \
	 if (sizeof (__value) != 8)					      \
	   /* There should not be any value with a size other than 1,	      \
	      4 or 8.  */						      \
	   abort ();							      \
									      \
	 asm volatile  ("movl %%gs:%P1(,%2,8),%%eax\n\t"		      \
			"movl %%gs:4+%P1(,%2,8),%%edx"			      \
			: "=&A" (__value)				      \
			: "i" (offsetof (struct rthread, member[0])),	      \
			  "r" (idx));					      \
       }								      \
     __value; })


/* Same as THREAD_SETMEM, but the member offset can be non-constant.  */
# define THREAD_SETMEM(descr, member, value) \
  ({ if (sizeof (descr->member) == 1)					      \
       asm volatile ("movb %b0,%%gs:%P1" :				      \
		     : "iq" (value),					      \
		       "i" (offsetof (struct rthread, member)));	      \
     else if (sizeof (descr->member) == 4)				      \
       asm volatile ("movl %0,%%gs:%P1" :				      \
		     : "ir" (value),					      \
		       "i" (offsetof (struct rthread, member)));	      \
     else								      \
       {								      \
	 if (sizeof (descr->member) != 8)				      \
	   /* There should not be any value with a size other than 1,	      \
	      4 or 8.  */						      \
	   abort ();							      \
									      \
	 asm volatile ("movl %%eax,%%gs:%P1\n\t"			      \
		       "movl %%edx,%%gs:%P2" :				      \
		       : "A" (value),					      \
			 "i" (offsetof (struct rthread, member)),	      \
			 "i" (offsetof (struct rthread, member) + 4));	      \
       }})


/* Set member of the thread descriptor directly.  */
# define THREAD_SETMEM_NC(descr, member, idx, value) \
  ({ if (sizeof (descr->member[0]) == 1)				      \
       asm volatile ("movb %b0,%%gs:%P1(%2)" :				      \
		     : "iq" (value),					      \
		       "i" (offsetof (struct rthread, member)),		      \
		       "r" (idx));					      \
     else if (sizeof (descr->member[0]) == 4)				      \
       asm volatile ("movl %0,%%gs:%P1(,%2,4)" :			      \
		     : "ir" (value),					      \
		       "i" (offsetof (struct rthread, member)),		      \
		       "r" (idx));					      \
     else								      \
       {								      \
	 if (sizeof (descr->member[0]) != 8)				      \
	   /* There should not be any value with a size other than 1,	      \
	      4 or 8.  */						      \
	   abort ();							      \
									      \
	 asm volatile ("movl %%eax,%%gs:%P1(,%2,8)\n\t"			      \
		       "movl %%edx,%%gs:4+%P1(,%2,8)" :			      \
		       : "A" (value),					      \
			 "i" (offsetof (struct rthread, member)),	      \
			 "r" (idx));					      \
       }})


/* Atomic compare and exchange on TLS, returning old value.  */
#define THREAD_ATOMIC_CMPXCHG_VAL(descr, member, newval, oldval) \
  ({ __typeof (descr->member) __ret;					      \
     __typeof (oldval) __old = (oldval);				      \
     if (sizeof (descr->member) == 4)					      \
       asm volatile (LOCK_PREFIX "cmpxchgl %2, %%gs:%P3"		      \
		     : "=a" (__ret)					      \
		     : "0" (__old), "r" (newval),			      \
		       "i" (offsetof (struct rthread, member)));	      \
     else								      \
       /* Not necessary for other sizes in the moment.  */		      \
       abort ();							      \
     __ret; })


/* Atomic logical and.  */
#define THREAD_ATOMIC_AND(descr, member, val) \
  (void) ({ if (sizeof ((descr)->member) == 4)				      \
	      asm volatile (LOCK_PREFIX "andl %1, %%gs:%P0"		      \
			    :: "i" (offsetof (struct rthread, member)),	      \
			       "ir" (val));				      \
	    else							      \
	      /* Not necessary for other sizes in the moment.  */	      \
	      abort (); })


/* Atomic set bit.  */
#define THREAD_ATOMIC_BIT_SET(descr, member, bit) \
  (void) ({ if (sizeof ((descr)->member) == 4)				      \
	      asm volatile (LOCK_PREFIX "orl %1, %%gs:%P0"		      \
			    :: "i" (offsetof (struct rthread, member)),	      \
			       "ir" (1 << (bit)));			      \
	    else							      \
	      /* Not necessary for other sizes in the moment.  */	      \
	      abort (); })


/* Call the user-provided thread function.  */
#define CALL_THREAD_FCT(descr) \
  ({ void *__res;							      \
     int __ignore1, __ignore2;						      \
     asm volatile ("pushl %%eax\n\t"					      \
		   "pushl %%eax\n\t"					      \
		   "pushl %%eax\n\t"					      \
		   "pushl %%gs:%P4\n\t"					      \
		   "call *%%gs:%P3\n\t"					      \
		   "addl $16, %%esp"					      \
		   : "=a" (__res), "=c" (__ignore1), "=d" (__ignore2)	      \
		   : "i" (offsetof (struct rthread, start_routine)),	      \
		     "i" (offsetof (struct rthread, arg)));		      \
     __res; })


/* Set the stack guard field in TCB head.  */
#define THREAD_SET_STACK_GUARD(value) \
  THREAD_SETMEM (THREAD_SELF, header.stack_guard, value)
#define THREAD_COPY_STACK_GUARD(descr) \
  ((descr)->header.stack_guard						      \
   = THREAD_GETMEM (THREAD_SELF, header.stack_guard))


/* Set the pointer guard field in the TCB head.  */
#define THREAD_SET_POINTER_GUARD(value) \
  THREAD_SETMEM (THREAD_SELF, header.pointer_guard, value)
#define THREAD_COPY_POINTER_GUARD(descr) \
  ((descr)->header.pointer_guard					      \
   = THREAD_GETMEM (THREAD_SELF, header.pointer_guard))


/* Get and set the global scope generation counter in the TCB head.  */
#define THREAD_GSCOPE_FLAG_UNUSED 0
#define THREAD_GSCOPE_FLAG_USED   1
#define THREAD_GSCOPE_FLAG_WAIT   2
#define THREAD_GSCOPE_RESET_FLAG() \
  do									      \
    { int __res;							      \
      asm volatile ("xchgl %0, %%gs:%P1"				      \
		    : "=r" (__res)					      \
		    : "i" (offsetof (struct rthread, header.gscope_flag)),    \
		      "0" (THREAD_GSCOPE_FLAG_UNUSED));			      \
      if (__res == THREAD_GSCOPE_FLAG_WAIT)				      \
	lll_futex_wake (&THREAD_SELF->header.gscope_flag, 1, LLL_PRIVATE);    \
    }									      \
  while (0)
#define THREAD_GSCOPE_SET_FLAG() \
  THREAD_SETMEM (THREAD_SELF, header.gscope_flag, THREAD_GSCOPE_FLAG_USED)
#define THREAD_GSCOPE_WAIT() \
  GL(dl_wait_lookup_done) ()

/* Reading from the LDT.  Could also use %gs, but that would require including
 * half of libc's TLS header.  Sparc will probably ignore the vcoreid, so don't
 * rely on it too much.  The intent of it is vcoreid is the caller's vcoreid,
 * and that vcoreid might be in the TLS of the caller (it will be for transition
 * stacks) and we could avoid a trap on x86 to sys_getvcoreid(). */
static inline void *__get_tls_desc(uint32_t vcoreid)
{
	return (void*)(__procdata.ldt[vcoreid].sd_base_31_24 << 24 |
	               __procdata.ldt[vcoreid].sd_base_23_16 << 16 |
	               __procdata.ldt[vcoreid].sd_base_15_0);
}

/* passing in the vcoreid, since it'll be in TLS of the caller */
static inline void __set_tls_desc(void *tls_desc, uint32_t vcoreid)
{
	/* Keep this technique in sync with sysdeps/akaros/i386/tls.h */
	segdesc_t tmp = SEG(STA_W, (uint32_t)tls_desc, 0xffffffff, 3);
	__procdata.ldt[vcoreid] = tmp;

	/* GS is still the same (should be!), but it needs to be reloaded to force a
	 * re-read of the LDT. */
	uint32_t gs = (vcoreid << 3) | 0x07;
	asm volatile("movl %0,%%gs" : : "r" (gs) : "memory");
}

static const char* tls_init_tp(void* thrdescr)
{
  // TCB lives at thrdescr.
  // The TCB's head pointer points to itself :-)
  tcbhead_t* head = (tcbhead_t*)thrdescr;
  head->tcb = thrdescr;
  head->self = thrdescr;

  //TODO: think about how to avoid this. Probably add a field to the 
  // rthreads struct that we manually fill in in _start(). 
  int core_id = __ros_syscall_noerrno(SYS_getvcoreid, 0, 0, 0, 0, 0, 0);

  /* Bug with this whole idea (TODO: (TLSV))*/
  if(__procdata.ldt == NULL)
  {
    size_t sz= (sizeof(segdesc_t)*__procinfo.max_vcores+PGSIZE-1)/PGSIZE*PGSIZE;
    
	/* Can't directly call mmap because it tries to set errno, and errno doesn't
	 * exist yet (it relies on tls, and we are currently in the process of
	 * setting it up...) */
	void *ldt = (void*)__ros_syscall_noerrno(SYS_mmap, 0, sz,
	                                         PROT_READ | PROT_WRITE,
	                                         MAP_ANONYMOUS | MAP_POPULATE,
	                                         -1, 0);
    if (ldt == MAP_FAILED)
      return "tls couldn't allocate memory\n";

    __procdata.ldt = ldt;
    // force kernel crossing
	__ros_syscall_noerrno(SYS_getpid, 0, 0, 0, 0, 0, 0);
  }

  __set_tls_desc(thrdescr, core_id);
  return NULL;
}

#endif /* __ASSEMBLER__ */

#endif	/* tls.h */
