# Visual Studio / WSL support

For the Visual Studio WSL toolchain on Ubuntu 22, install:

```bash
sudo apt-get install -y g++ gdb make ninja-build rsync zip
```

After that, create or select a Visual Studio configuration that targets WSL with GCC.
The Meeting SDK runner is a Linux CMake project, so the WSL target is the supported flow for local development before building the Azure image.
