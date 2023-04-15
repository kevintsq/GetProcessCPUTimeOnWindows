!include ..\common.mak

LIBS=$(LIBS) kernel32.lib

##############################################################################

all: dirs \
    $(BIND)\libtime$(DETOURS_BITS).dll \
!IF $(DETOURS_SOURCE_BROWSING)==1
    $(OBJD)\libtime$(DETOURS_BITS).bsc
!ENDIF
    option

##############################################################################

clean:
    -del $(BIND)\libtime*.* 2>nul
    -rmdir /q /s $(OBJD) 2>nul

realclean: clean
    -rmdir /q /s $(OBJDS) 2>nul

dirs:
    @if not exist $(BIND) mkdir $(BIND) && echo.   Created $(BIND)
    @if not exist $(OBJD) mkdir $(OBJD) && echo.   Created $(OBJD)

$(OBJD)\libtime.obj : libtime.cpp

$(OBJD)\libtime.res : libtime.rc

$(BIND)\libtime$(DETOURS_BITS).dll: $(OBJD)\libtime.obj $(OBJD)\libtime.res $(DEPS)
    cl /LD $(CFLAGS) /Fe$@ /Fd$(@R).pdb \
        $(OBJD)\libtime.obj $(OBJD)\libtime.res \
        /link $(LINKFLAGS) /subsystem:console \
        /export:DetourFinishHelperProcess,@1,NONAME \
        $(LIBS)

$(OBJD)\libtime$(DETOURS_BITS).bsc : $(OBJD)\libtime.obj
    bscmake /v /n /o $@ $(OBJD)\libtime.sbr

############################################### Install non-bit-size binaries.

!IF "$(DETOURS_OPTION_PROCESSOR)" != ""

$(OPTD)\libtime$(DETOURS_OPTION_BITS).dll:
$(OPTD)\libtime$(DETOURS_OPTION_BITS).pdb:

$(BIND)\libtime$(DETOURS_OPTION_BITS).dll : $(OPTD)\libtime$(DETOURS_OPTION_BITS).dll
    @if exist $? copy /y $? $(BIND) >nul && echo $@ copied from $(DETOURS_OPTION_PROCESSOR).
$(BIND)\libtime$(DETOURS_OPTION_BITS).pdb : $(OPTD)\libtime$(DETOURS_OPTION_BITS).pdb
    @if exist $? copy /y $? $(BIND) >nul && echo $@ copied from $(DETOURS_OPTION_PROCESSOR).

option: \
    $(BIND)\libtime$(DETOURS_OPTION_BITS).dll \
    $(BIND)\libtime$(DETOURS_OPTION_BITS).pdb \

!ELSE

option:

!ENDIF
