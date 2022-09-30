# mayv2
My attempt at creating a 64-bit OS

## Compilation toolchain setup
1. Install NASM (version 2.15.05 recommended and tested). Using the system's package manager like `apt` is enough.
2. Create a GCC cross-compiler without standard headers and libraries. Read `docs/gcc12crossNoStd.md` for the procedure.

## Create an ISO image
1. Follow the steps to [setup the compilation toolchain](#compilation-toolchain-setup)
2. Make the ISO
```bash
cd mayv2_clone_dir
make clean
make directories
./configure
make all
```
