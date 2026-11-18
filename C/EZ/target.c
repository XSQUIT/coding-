#include <stdio.h>
#include <unistd.h>

int main(void) {
    int isAdmin = 0;

    printf("[Target] PID: %d\n", getpid());
    printf("[Target] isAdmin address in RAM: %p\n", (void*)&isAdmin);
    printf("[Target] Waiting for auth check... Press ENTER to verify.\n");

    // Pauses execution while keeping the process alive in RAM
    getchar();

    if (isAdmin == 1) {
        printf("\nYahahaha, you found me! Runtime memory patched! (isAdmin = %d)\n", isAdmin);
    } else {
        printf("\nAccess denied. (isAdmin = %d)\n", isAdmin);
    }

    return 0;
}
