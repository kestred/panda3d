/* Filename: contextSwitch.c
 * Created by:  drose (21Jun07)
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * PANDA 3D SOFTWARE
 * Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
 *
 * All use of this software is subject to the terms of the Panda 3d
 * Software license.  You should have received a copy of this license
 * along with this source code; you will also find a current copy of
 * the license at http://etc.cmu.edu/panda3d/docs/license/ .
 *
 * To contact the maintainers of this program write to
 * panda3d-general@lists.sourceforge.net .
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "contextSwitch.h"

#include <stdlib.h>

#ifdef THREAD_SIMPLE_IMPL

#if defined(HAVE_UCONTEXT_H)

/* The getcontext() / setcontext() implementation.  Easy-peasy. */

static void
begin_context(ContextFunction *thread_func, void *data) {
  (*thread_func)(data);
}

void
init_thread_context(struct ThreadContext *context, 
                    unsigned char *stack, size_t stack_size,
                    ContextFunction *thread_func, void *data) {
  getcontext(&context->_ucontext);

  context->_ucontext.uc_stack.ss_sp = stack;
  context->_ucontext.uc_stack.ss_size = stack_size;
  context->_ucontext.uc_stack.ss_flags = 0;
  context->_ucontext.uc_link = NULL;

  makecontext(&context->_ucontext, (void (*)())&begin_context, 2, thread_func, data);
}

void save_thread_context(struct ThreadContext *context,
                         ContextFunction *next_context, void *data) {
  /* getcontext requires us to use a volatile auto variable to
     differentiate between pass 1 (immediate return) and pass 2
     (return from setcontext). */
  volatile int context_return = 0;

  getcontext(&context->_ucontext);
  if (context_return) {
    /* We have just returned from setcontext.  In this case, return
       from the function.  The stack is still good. */
    return;
  }

  context_return = 1;

  /* We are still in the calling thread.  In this case, we cannot
     return from the function without damaging the stack.  Insted,
     call next_context() and trust the caller to call
     switch_to_thread_context() in there somewhere. */

  (*next_context)(data);

  /* We shouldn't get here. */
  abort();
}

void
switch_to_thread_context(struct ThreadContext *context) {
  setcontext(&context->_ucontext);

  /* Shouldn't get here. */
  abort();
}

#else

/* The setjmp() / longjmp() implementation.  A bit hackier. */

/* The approach is: hack our way onto the new stack pointer right now,
   then call setjmp() to record that stack pointer in the
   _jmp_context.  Then restore back to the original stack pointer. */

#if defined(_M_IX86)
/* Here is own own implementation of setjmp and longjmp for I386, via
   Windows syntax. */

int cs_setjmp(cs_jmp_buf env);
void cs_longjmp(cs_jmp_buf env);

#elif defined(__i386__)
/* Here is own own implementation of setjmp and longjmp for I386, via
   GNU syntax. */

#if defined(IS_LINUX)
/* On Linux, the underscores are not implicit. */
#define cs_setjmp _cs_setjmp
#define cs_longjmp _cs_longjmp
#endif

int cs_setjmp(cs_jmp_buf env);
void cs_longjmp(cs_jmp_buf env);

__asm__
("_cs_setjmp:\n"
 "popl %edx\n"
 "popl %eax\n"
 "pushl %eax\n"
 "movl %ebx,0(%eax)\n"
 "movl %edi,4(%eax)\n"
 "movl %esi,8(%eax)\n"
 "movl %ebp,12(%eax)\n"
 "movl %esp,16(%eax)\n"
 "movl %edx,20(%eax)\n"
 "fnsave 24(%eax)\n"
 "xorl %eax,%eax\n"
 "jmp *%edx\n");

__asm__
("_cs_longjmp:\n"
 "popl %edx\n"
 "popl %eax\n"
 "movl 0(%eax),%ebx\n"
 "movl 4(%eax),%edi\n"
 "movl 8(%eax),%esi\n"
 "movl 12(%eax),%ebp\n"
 "movl 16(%eax),%esp\n"
 "movl 20(%eax),%edx\n"
 "frstor 24(%eax)\n"
 "mov $1,%eax\n"
 "jmp *%edx\n");

#endif  /* __i386__ */

/* Ideally, including setjmp.h would have defined JB_SP, which will
   tell us where in the context structure we can muck with the stack
   pointer.  If it didn't define this symbol, we have to guess it. */
#ifndef CS_JB_SP

#if defined(IS_OSX) && defined(__i386__)
/* We have determined this value empirically, via test_setjmp.cxx in
   this directory. */
#define CS_JB_SP 9

#elif defined(WIN32)
/* We have determined this value empirically, via test_setjmp.cxx in
   this directory. */
#define CS_JB_SP 4

#endif

#endif  /* CS_JB_SP */

static struct ThreadContext *st_context;
static unsigned char *st_stack;
static size_t st_stack_size;
static ContextFunction *st_thread_func;
static void *st_data;

static cs_jmp_buf orig_stack;

/* We can't declare this function static--gcc might want to inline it
   in that case, and then the code crashes.  I hope this doesn't mean
   that the stack is still not getting restored correctly in the above
   assembly code. */
void
setup_context_2(void) {
  /* Here we are running on the new stack.  Copy the key data onto our
     new stack. */
  ContextFunction *volatile thread_func = st_thread_func;
  void *volatile data = st_data;

  if (cs_setjmp(st_context->_jmp_context) == 0) {
    /* The _jmp_context is set up and ready to run.  Now restore the
       original stack and return.  We can't simply return from this
       function, since it might overwrite some of the stack data on
       the way out. */
    cs_longjmp(orig_stack);

    /* Shouldn't get here. */
    abort();
  }

  /* We come here the first time the thread starts. */
  (*thread_func)(data);

  /* We shouldn't get here, since we don't expect the thread_func to
     return. */
  abort();
}

static void
setup_context_1(void) {
  /* Save the current stack frame so we can return to it (at the end
     of setup_context_2()). */
  if (cs_setjmp(orig_stack) == 0) {
    /* First, switch to the new stack.  Save the current context using
       setjmp().  This saves out all of the processor register values,
       though it doesn't muck with the stack. */
    static cs_jmp_buf temp;
    if (cs_setjmp(temp) == 0) {
      /* This is the initial return from setjmp.  Still the original
         stack. */

      /* Now we overwrite the stack pointer value in the saved
         register context.  This doesn't work with all implementations
         of setjmp/longjmp. */
      (*(void **)&temp[CS_JB_SP]) = (st_stack + st_stack_size);

      /* And finally, we place ourselves on the new stack by using
         longjmp() to reload the modified context. */
      cs_longjmp(temp);

      /* Shouldn't get here. */
      abort();
    }

    /* This is the second return from setjmp.  Now we're on the new
       stack. */
    setup_context_2();

    /* Shouldn't get here. */
    abort();
  }

  /* By now we are back to the original stack. */
}

void
init_thread_context(struct ThreadContext *context, 
                    unsigned char *stack, size_t stack_size,
                    ContextFunction *thread_func, void *data) {
  /* Copy all of the input parameters to static variables, then begin
     the stack-switching process. */
  st_context = context;
  st_stack = stack;
  st_stack_size = stack_size;
  st_thread_func = thread_func;
  st_data = data;

  setup_context_1();
}  

void save_thread_context(struct ThreadContext *context,
                         ContextFunction *next_context, void *data) {
  if (cs_setjmp(context->_jmp_context) != 0) {
    /* We have just returned from longjmp.  In this case, return from
       the function.  The stack is still good. */
    return;
  }

  /* We are still in the calling thread.  In this case, we cannot
     return from the function without damaging the stack.  Insted,
     call next_context() and trust the caller to call
     switch_to_thread_context() in there somewhere. */

  (*next_context)(data);

  /* We shouldn't get here. */
  abort();
}

void
switch_to_thread_context(struct ThreadContext *context) {
  cs_longjmp(context->_jmp_context);

  /* Shouldn't get here. */
  abort();
}

#endif  /* HAVE_UCONTEXT_H */
#endif  /* THREAD_SIMPLE_IMPL */
