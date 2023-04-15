!include ..\common.mak

LIBS=$(LIBS) kernel32.lib

##############################################################################

all: dirs \
    $(BIND)\time.exe \
!IF $(DETOURS_SOURCE_BROWSING)==1
    $(OBJD)\time.bsc \
!ENDIF
	option

##############################################################################

clean:
    -del *~ 2>nul
    -del $(BIND)\time.* 2>nul
    -rmdir /q /s $(OBJD) 2>nul

realclean: clean
    -rmdir /q /s $(OBJDS) 2>nul

dirs:
    @if not exist $(BIND) mkdir $(BIND) && echo.   Created $(BIND)
    @if not exist $(OBJD) mkdir $(OBJD) && echo.   Created $(OBJD)

$(OBJD)\time.obj : time.cpp

$(OBJD)\time.res : time.rc

$(BIND)\time.exe : $(OBJD)\time.obj $(OBJD)\time.res $(DEPS)
    cl $(CFLAGS) /Fe$@ /Fd$(@R).pdb \
        $(OBJD)\time.obj $(OBJD)\time.res \
        /link $(LINKFLAGS) /subsystem:console $(LIBS)

$(OBJD)\time.bsc : $(OBJD)\time.obj
    bscmake /v /n /o $@ $(OBJD)\time.sbr

############################################### Install non-bit-size binaries.

option: