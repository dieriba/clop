# Building clp

## Clone

```sh
git clone --recurse-submodules https://github.com/dieriba/clp.git
cd clp
```

The `--recurse-submodules` flag is required to pull in `libs/c_lib`, which is a dependency of the library.

## Build

```sh
make        # builds target/build/lib/libclp.a
make tests  # builds and links the test binary at target/tests/clp/test
```

## Linking against clp

Add the following to your compiler invocation:

```
-I./includes
target/build/lib/libclp.a
libs/c_lib/target/build/lib/libd_lib.a
```
