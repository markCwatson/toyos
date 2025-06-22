## clang-format

On MacOS:

```shell
brew install clang-format
```

On Linux

```shell
sudo apt update
sudo apt install clang-format
```

Apply formatting

```shell
# Format single file in-place
clang-format -i src/drivers/pci/pci.c

# Format all C/H files in src directory
find src -name "*.c" -o -name "*.h" | xargs clang-format -i

# Preview formatting without changing file (dry run)
clang-format src/drivers/pci/pci.c

# Format specific files
clang-format -i src/drivers/pci/pci.c src/drivers/pci/pci.h
```
