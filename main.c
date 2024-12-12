/* Beeboot UEFI bootloader using POSIX-UEFI.
 * Copyright (C) 2024 Jake Steinburger (UnmappedStack) under the Mozilla Public License 2.0.
 * NOTE: The display_memmap() function is largely based on the example memmap.c posix-uefi program and is exempt from the license.
 */

#include <uefi.h>

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
    printf("[TEST] Getting memory map (showing only 10 entries)...\n");
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
            if (i > 10) break;
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
    printf("Read file, got contents: %s\n", buf);
}

void test_uefi_api() {
    printf("[TEST] Testing UEFI API...\n");
    printf("System table address: %p\n", ST);
    printf("Boot services table address: %p\n", ST->BootServices);
    printf("Runtime services table address: %p\n", ST->RuntimeServices);
    efi_time_t time;
    efi_status_t status = ST->RuntimeServices->GetTime(&time, NULL);
    if (EFI_ERROR(status)) {
        printf("Failed to get current time from runtime services.\n");
        for (;;);
    }
    printf("Current time is %i:%i:%i on %i/%i/%i\n", 
            time.Hour, time.Minute, time.Second,
            time.Day, time.Month, time.Year);
}

int main(int argc, char **argv) {
    test_filesystem();
    display_memmap();
    test_uefi_api();
    for (;;);
    return 0;
}
