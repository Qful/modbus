#include <stdio.h>
#include <modbus.h>

int main(void)
{
    printf("Compiled with libmodbus version %s (%06X)\n", LIBMODBUS_VERSION_STRING, LIBMODBUS_VERSION_HEX);
    printf("Linked with libmodbus version %d.%d.%d\n",
           libmodbus_version_major, libmodbus_version_minor, libmodbus_version_micro);

    if (LIBMODBUS_VERSION_CHECK(3, 1, 4)) {
        printf("The functions to read/write float values are available (3.1.4).\n");
    }

    if (LIBMODBUS_VERSION_CHECK(3, 1, 4)) {
        printf("brand new API (3.1.4)!\n");
    }

    return 0;
}
