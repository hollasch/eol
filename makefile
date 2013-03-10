
all: eol

eol: eol.exe

eol.exe: eol.c
	cl -nologo -Ox -W4 -GF -GS eol.c

clean:
	-del 2>nul /q eol.obj

clobber: clean
	-del 2>nul /q eol.exe

fresh: clobber all

$(BINDIR)\eol.exe: eol.exe
    if defined BINDIR copy /y $? %%BINDIR%%

install: $(BINDIR)\eol.exe
