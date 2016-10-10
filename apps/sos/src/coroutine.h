int r = setjmp(syscall_loop_buf);

    if (r == 0) {
// change the current
         asm("mov sp, %0" 
                         :
                                         : "r" (cr->cr_stack_top)
                                                     );

                                                             proc_tmp->callback_fn(proc_tmp->pid);
                                                                 }
