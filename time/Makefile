ROOT=..
!include .\common.mak

all:
    cd "$(MAKEDIR)\libtime"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS)
    cd "$(MAKEDIR)\time"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS)

clean:
    cd "$(MAKEDIR)\libtime"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
    cd "$(MAKEDIR)\time"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
    cd "$(MAKEDIR)"
    -rmdir lib32 2>nul
    -rmdir lib64 2>nul
    -rmdir include 2>nul

realclean:
    cd "$(MAKEDIR)\libtime"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS) realclean
    cd "$(MAKEDIR)\time"
    @$(MAKE) /NOLOGO /$(MAKEFLAGS) realclean
    cd "$(MAKEDIR)"
    -rmdir lib32 2>nul
    -rmdir lib64 2>nul
    -rmdir include 2>nul
