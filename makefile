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
LIBS =   -ldebug -lcjson

OBJS    =  obj/logo.o  obj/xmlviewerlist.o obj/xmlviewerexpat.o obj/xmlviewerjson.o obj/xmlviewertree.o obj/xmlviewerabout.o obj/$(EXE).o

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

obj/xmlviewertree.o: xmlviewertree.c xmlviewertree.h
	$(CC) -c  $(CFLAGS) -o $@ $<

obj/xmlviewerabout.o: xmlviewerabout.c xmlviewerabout.h
	$(CC) -c  $(CFLAGS) -o $@ $<
