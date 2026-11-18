#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: sudo ./injector <PID> <HEX_ADDRESS>\n");
        return 1;
    }

    pid_t pid = atoi(argv);
    mach_vm_address_t target_addr = strtoull(argv, NULL, 16);

    mach_port_t task;
    // 1. Ask the XNU kernel for a handle (task port) to the running target process
    kern_return_t kr = task_for_pid(mach_task_self(), pid, &task);
    if (kr != KERN_SUCCESS) {
        printf("[-] task_for_pid failed (Error: %d). Check entitlements or sudo.\n", kr);
        return 1;
    }

    // 2. Overwrite the 4 bytes at target_addr with the integer 1
    int new_value = 1;
    kr = mach_vm_write(task, target_addr, (vm_offset_t)&new_value, sizeof(new_value));
    if (kr != KERN_SUCCESS) {
        printf("[-] mach_vm_write failed (Error: %d).\n", kr);
        return 1;
    }

    printf("[+] Successfully injected %d into PID %d at 0x%llx!\n", new_value, pid, target_addr);
    return 0;
}
