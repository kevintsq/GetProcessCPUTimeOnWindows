##############################################################################
##
##  Common makefile for Detours test programs.
##
##  Microsoft Research Detours Package
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

!IF "$(ROOT)" == ""
ROOT = ..\..
!ENDIF
!include "$(ROOT)\system.mak"

!IF "$(DETOURS_SOURCE_BROWSING)" == ""
DETOURS_SOURCE_BROWSING=0
!ENDIF

##############################################################################

!IFNDEF CLIB
CLIB=/MT
!ENDIF

AFLAGS=/nologo /c /Fl
CFLAGS=$(CLIB) /Gm- /W4 /WX /O2 /GL

!IF $(DETOURS_SOURCE_BROWSING)==1
CFLAGS=$(CFLAGS) /FR
!ELSE
CFLAGS=$(CFLAGS) /I$(INCD)
!ENDIF

LIBFLAGS=/nologo
LINKFLAGS=/release /incremental:no /profile /nodefaultlib:oldnames.lib /LTCG

!if defined(DETOURS_WIN_7) && defined(DETOURS_CL_17_OR_NEWER)
CFLAGS=$(CFLAGS) /D_USING_V110_SDK71_
!endif

!IF "$(DETOURS_TARGET_PROCESSOR)" == "X86"

ASM=ml

!ELSEIF "$(DETOURS_TARGET_PROCESSOR)" == "X64"

ASM=ml64

!ELSEIF "$(DETOURS_TARGET_PROCESSOR)" == "IA64"

ASM=ias
AFLAGS=-F COFF32_PLUS
CFLAGS=$(CFLAGS) /wd4163 # intrinsic rdtebex not available; using newer Windows headers with older compiler
#CFLAGS=$(CFLAGS) /wd4996 /wd4068

!ELSEIF "$(DETOURS_TARGET_PROCESSOR)" == "ARM"

ASM=armasm
AFLAGS=-coff_thumb2_only
CFLAGS=$(CFLAGS) /D_ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE

CFLAGS=$(CFLAGS) /D_$(DETOURS_TARGET_PROCESSOR:X64=AMD64)_ # redundant with windows.h except for midl proxies

!ENDIF

DEPS = $(LIBD)\detours.lib
LIBS = $(DEPS)

##############################################################################
##

.SUFFIXES: .cpp .h .obj .rc .res

!ifdef DETOURS_ANALYZE
.cpp{$(OBJD)}.obj:
    $(CC) $(CFLAGS) /Fd$(OBJD)\vc.pdb /Fo$(OBJD)\ /c $<
!else
.cpp{$(OBJD)}.obj::
    $(CC) $(CFLAGS) /Fd$(OBJD)\vc.pdb /Fo$(OBJD)\ /c $<
!endif

.rc{$(OBJD)}.res:
    rc /DDETOURS_BITS=$(DETOURS_BITS) /fo$(@) /i$(INCD) $(*B).rc
