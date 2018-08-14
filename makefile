
outDir = out

all: $(outDir) eol

eol: $(outDir)\eol.exe

$(outDir)\eol.exe: eol.cpp
    cl -nologo -O2 -Og -W4 -GF -GS -Fe: $(outDir)\eol.exe -Fo: $(outDir)\eol.obj eol.cpp

$(outDir):
    -mkdir $(outDir) >nul 2>&1

clean:
    -del 2>nul /q $(outDir)\*.obj

clobber:
    -rmdir /s /q $(outDir) >nul 2>&1

fresh: clobber all
