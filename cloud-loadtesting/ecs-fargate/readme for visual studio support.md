# Visual Studio / WSL support

For the Visual Studio WSL toolchain on Ubuntu 22, install:

```bash
sudo apt-get install -y g++ gdb make ninja-build rsync zip
```

After that, create or select a Visual Studio configuration that targets WSL with GCC.
The samples are Linux CMake projects, so the WSL target is the supported flow.
