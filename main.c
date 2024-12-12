/* Beeboot UEFI bootloader using POSIX-UEFI.
 * Copyright (C) 2024 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0.
 * NOTE: The display_memmap() function is largely based on the example memmap.c posix-uefi program and is exempt from the license.
 */

#include <uefi.h>
#include <page.h>

#define kmalloc(p) \
    malloc(p * PAGE_SIZE)

const char *types[] = {
    "EfiReservedMemoryType",
    "EfiLoaderCode",
    "EfiLoaderData",
    "EfiBootServicesCode",
    "EfiBootServicesData",
    "EfiRuntimeServicesCode",
    "EfiRuntimeServicesData",
    "EfiConventionalMemory",
    "EfiUnusableMemory",
    "EfiACPIReclaimMemory",
    "EfiACPIMemoryNVS",
    "EfiMemoryMappedIO",
    "EfiMemoryMappedIOPortSpace",
    "EfiPalCode"
};

void display_memmap() {
    printf("[TEST] Getting memory map (showing only 5 entries)...\n");
    efi_memory_descriptor_t *memory_map = NULL, *mement;
    uintn_t memory_map_size = 0, map_key = 0, desc_size = 0;
    efi_status_t status = BS->GetMemoryMap(&memory_map_size, NULL, &map_key, &desc_size, NULL);
    if (status != EFI_BUFFER_TOO_SMALL || !memory_map_size) {
        printf("[PANIC] Couldn't get memory map (checkpoint 1)\n");
        for (;;);
    }
    memory_map_size += desc_size * 4;
    memory_map = (efi_memory_descriptor_t*) malloc(memory_map_size);
    status = BS->GetMemoryMap(&memory_map_size, memory_map, &map_key, &desc_size, NULL);
    if (EFI_ERROR(status)) {
        printf("[PANIC] Couldn't get memory map (checkpoint 2)\n");
        for (;;);
    }
    printf("Address              Size Type\n");
    size_t i = 0;
    for(mement = memory_map; (uint8_t*) mement < (uint8_t*) memory_map + memory_map_size;
        mement = NextMemoryDescriptor(mement, desc_size)) {
            if (i > 5) break;
            printf("%016x %8d %02x %s\n", mement->PhysicalStart, mement->NumberOfPages, mement->Type, types[mement->Type]);
            i++;
    }
    free(memory_map);
}

void test_filesystem() {
    printf("[TEST] Testing file system...\n");
    FILE *f = fopen("testfile.txt", "r");
    if (f == NULL) {
        printf("[PANIC] Could not open testfile.txt\n");
        for (;;);
    }
    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char*) malloc(len);
    fread(buf, len, 1, f);
    buf[len] = 0;
    fclose(f);
    printf("Read file, got contents: %s", buf);
}

void test_uefi_api() {
    printf("[TEST] Testing UEFI API...\n");
    printf("ST address: %p\n", ST);
    printf("BST address: %p, ", ST->BootServices);
    printf("RST address: %p\n", ST->RuntimeServices);
    efi_time_t time;
    efi_status_t status = ST->RuntimeServices->GetTime(&time, NULL);
    if (EFI_ERROR(status)) {
        printf("Failed to get current time from runtime services.\n");
        for (;;);
    }
    printf("Current time is %i:%i:%i on %i/%i/%i\n", 
            time.Hour, time.Minute, time.Second,
            time.Day, time.Month, time.Year);
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r" (cr3));
    printf("CR3 set by firmware: %p, memory is identity mapped on boot.\n", cr3);
}

void map_pages(uint64_t pml4_addr[], uint64_t virt_addr, uint64_t phys_addr, uint64_t num_pages, uint64_t flags) {
    virt_addr &= ~TOPBITS;
    uint64_t pml1 = (virt_addr >> 12) & 511;
    uint64_t pml2 = (virt_addr >> (12 + 9)) & 511;
    uint64_t pml3 = (virt_addr >> (12 + 18)) & 511;
    uint64_t pml4 = (virt_addr >> (12 + 27)) & 511;
    for (; pml4 < 512; pml4++) {
        uint64_t *pml3_addr = NULL;
        if (pml4_addr[pml4] == 0) {
            pml4_addr[pml4] = (uint64_t)kmalloc(1);
            pml3_addr = (uint64_t*)(pml4_addr[pml4]);
            memset((uint8_t*)pml3_addr, 0, 4096);
            pml4_addr[pml4] |= flags | KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE;
        } else {
            pml3_addr = (uint64_t*)(PAGE_ALIGN_DOWN(pml4_addr[pml4]));
        }
        
        for (; pml3 < 512; pml3++) {
            uint64_t *pml2_addr = NULL;
            if (pml3_addr[pml3] == 0) {
                pml3_addr[pml3] = (uint64_t)kmalloc(1);
                pml2_addr = (uint64_t*)(pml3_addr[pml3]);
                memset((uint8_t*)pml2_addr, 0, 4096);
                pml3_addr[pml3] |= flags | KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE;
            } else {
                pml2_addr = (uint64_t*)(PAGE_ALIGN_DOWN(pml3_addr[pml3]));
            }

            for (; pml2 < 512; pml2++) {
                uint64_t *pml1_addr = NULL;
                if (pml2_addr[pml2] == 0) {
                    pml2_addr[pml2] = (uint64_t)kmalloc(1);
                    pml1_addr = (uint64_t*)(pml2_addr[pml2]);
                    memset((uint8_t*)pml1_addr, 0, 4096);
                    pml2_addr[pml2] |= flags | KERNEL_PFLAG_PRESENT | KERNEL_PFLAG_WRITE;
                } else {
                    pml1_addr = (uint64_t*)(PAGE_ALIGN_DOWN(pml2_addr[pml2]));
                }
                for (; pml1 < 512; pml1++) {
                    pml1_addr[pml1] = phys_addr | flags;
                    num_pages--;
                    phys_addr += 4096;
                    if (num_pages == 0) return;
                }
                pml1 = 0;
            }
            pml2 = 0;
        }
        pml3 = 0;
    }
    printf("\n[PANIC] Failed to allocate pages: No more avaliable virtual memory. Halting.\n");
    for (;;);
} 


void replace_page_tree() {
    
}

int main(int argc, char **argv) {
    test_filesystem();
    display_memmap();
    test_uefi_api();
    replace_page_tree();
    for (;;);
    return 0;
}
