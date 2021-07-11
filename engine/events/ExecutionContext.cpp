#include "events/ExecutionContext.hpp"

void __attribute__((noinline)) ExecutionContext::yieldTo(ExecutionContext* context) {

#ifdef ARCHITECTURE_X86_64
    asm volatile(
        "mov %%rsp, (%[old_ptr]);"
        "mov (%[new_ptr]), %%rsp;"
        :
        : [ old_ptr ] "a"(&stackPtr), [ new_ptr ] "b"(&context->stackPtr)
        : /*"rax", "rbx",*/ "rcx", "rdx", "rsi", "rdi", "rbp",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
        "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
        "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15");
#endif

#ifdef ARCHITECTURE_ARM_64
    asm volatile(
        "mov x2, sp\n\t"
        "str x2, [%[old_ptr]]\n\t"
        "ldr x3, [%[new_ptr]]\n\t"
        "mov sp, x3\n\t"
        :
        : [ old_ptr ] "r"(&stackPtr), [ new_ptr ] "r"(&context->stackPtr)
        : /*"x0", "x1",*/ "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
        "x16", "x17", /*"x18",*/ "x19", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29", "x30",
        "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",
        "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31");
#endif
}

void __attribute__((noinline)) ExecutionContext::bootstrap_interal(void (*callback)(void*), void* stack_pointer, void* data, bool* should_exit_ptr) {
    yieldTo(this);

    if (*should_exit_ptr)
        return;
    *should_exit_ptr = true;

#ifdef ARCHITECTURE_X86_64
    asm volatile("mov %[stack_pointer], %%rsp;"
                 "sub $256, %%rsp;"
                 : "+r"(callback), "+r"(data)
                 : [ stack_pointer ] "r"(stack_pointer)
                 :);
#endif

#ifdef ARCHITECTURE_ARM_64
    asm volatile("sub sp, %[stack_pointer], #256\n\t"
                 : "+r"(callback), "+r"(data)
                 : [ stack_pointer ] "r"(stack_pointer)
                 :);
#endif

    callback(data);
    // LOGE("callbacks passed to bootstrap() should never return!!!");
    // while (true) { }
}
