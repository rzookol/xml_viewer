#
# Makefile for xmlviewer
# Part of Plot.mcc MUI package
# (c) 2008-2025 Michal Zukowski
#


EXE = xmlviewer
FLEXCAT = ../../ambient/ambient/FlexCat
CC  = gcc
CFLAGS += -noixemul -Wall
#-DMEMTRACK
LDFLAGS += -noixemul  -O3
LIBS =   -ldebug -lyaml -liffparse

OBJS    =  obj/logo.o  obj/xmlviewerlist.o obj/xmlviewerexpat.o obj/xmlviewerjson.o obj/xmlvieweryaml.o obj/xmlvieweriff.o obj/xmlviewerformat.o obj/xmlviewerdata.o obj/xmlviewertree.o obj/xmlviewerabout.o obj/cjson.o obj/$(EXE).o

all:    $(EXE)

clean:
	-rm -rf $(OBJS) $(EXE)
locales:
	$(FLEXCAT) catalogs/xmlviewer.cd  catalogs/polski/xmlviewer.ct catalog catalogs/polski/xmlviewer.catalog

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

obj/$(EXE).o: $(EXE).c xmlviewerlist.h
	$(CC) -c  $(CFLAGS) -o $@ $<

obj/logo.o: logo.c
	$(CC) -c  $(CFLAGS) -o $@ $<

obj/xmlviewerlist.o: xmlviewerlist.c  xmlviewerlist.h
	$(CC) -c  $(CFLAGS) -o $@ $<

obj/xmlviewerexpat.o: xmlviewerexpat.c  xmlviewerexpat.h
        $(CC) -c  $(CFLAGS) -o $@ $<

obj/xmlviewerjson.o: xmlviewerjson.c  xmlviewerjson.h
        $(CC) -c  $(CFLAGS) -o $@ $<

obj/xmlvieweryaml.o: xmlvieweryaml.c xmlvieweryaml.h
        $(CC) -c  $(CFLAGS) -o $@ $<

obj/xmlvieweriff.o: xmlvieweriff.c xmlvieweriff.h
        $(CC) -c  $(CFLAGS) -o $@ $<

obj/xmlviewerformat.o: xmlviewerformat.c xmlviewerformat.h
        $(CC) -c  $(CFLAGS) -o $@ $<

obj/xmlviewerdata.o: xmlviewerdata.c xmlviewerdata.h
        $(CC) -c  $(CFLAGS) -o $@ $<

obj/xmlviewertree.o: xmlviewertree.c xmlviewertree.h
	$(CC) -c  $(CFLAGS) -o $@ $<

obj/xmlviewerabout.o: xmlviewerabout.c xmlviewerabout.h
	$(CC) -c  $(CFLAGS) -o $@ $<
	
obj/cjson.o: cJSON.c cJSON.h
	$(CC) -c  $(CFLAGS) -o $@ $<
