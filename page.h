#pragma once

#define PAGE_ALIGN_DOWN(addr) ((addr / 4096) * 4096)
#define PAGE_ALIGN_UP(x) ((((x) + 4095) / 4096) * 4096)

#define TOPBITS 0xFFFF000000000000
#define KERNEL_PFLAG_PRESENT 0b1
#define KERNEL_PFLAG_WRITE   0b10
#define PAGE_SIZE 4096
#define KERNEL_STACK_PAGES 2LL
#define KERNEL_STACK_PTR 0xFFFFFFFFFFFFF000LL
#define KERNEL_STACK_ADDR KERNEL_STACK_PTR-(KERNEL_STACK_PAGES*PAGE_SIZE)
#define PAGE_ALIGN_DOWN(addr) ((addr / 4096) * 4096) // works cos of integer division
#define PAGE_ALIGN_UP(x) ((((x) + 4095) / 4096) * 4096)
#define KERNEL_SWITCH_PAGE_TREE(TREE_ADDRESS) \
    __asm__ volatile (\
       "movq %0, %%cr3"\
       :\
       :  "r" (TREE_ADDRESS)\
    )

#define KERNEL_SWITCH_STACK(STACK_PTR) \
    __asm__ volatile (\
       "movq %0, %%rsp\n"\
       "movq $0, %%rbp\n"\
       "push $0"\
       :\
       :  "r" (STACK_PTR)\
    )
