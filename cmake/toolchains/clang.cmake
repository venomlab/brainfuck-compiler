set(CMAKE_C_COMPILER clang-23)
set(CMAKE_CXX_COMPILER clang++-23)

set(CMAKE_AR llvm-ar-23)
set(CMAKE_RANLIB llvm-ranlib-23)

set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
