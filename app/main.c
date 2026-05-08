#include <stdint.h>

int main(void)
{
    volatile uint32_t boot_counter = 0U;

    for (;;)
    {
        boot_counter++;
    }
}