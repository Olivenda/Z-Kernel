# Drivers

This directory houses basic device helpers:

- `serial.c`: initializes COM1 for optional debug output.
- `keyboard.c`: polls the PS/2 controller for raw scancodes and echoes printable keys.
- `cpu.c`: probes CPUID to expose Intel/AMD feature hints for the kernel.

Add each driver as its own source file or subdirectory to keep the kernel core organized.
