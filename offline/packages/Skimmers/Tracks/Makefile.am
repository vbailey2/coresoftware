AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = \
  -I$(includedir) \
  -isystem$(OFFLINE_MAIN)/include \
  -isystem$(ROOTSYS)/include

AM_LDFLAGS = \
  -L$(libdir) \
  -L$(OFFLINE_MAIN)/lib \
  -L$(OFFLINE_MAIN)/lib64

pkginclude_HEADERS = \
  TriggeredTrackSkimmer.h

lib_LTLIBRARIES = \
  libTriggeredTrackSkimmer.la

libTriggeredTrackSkimmer_la_SOURCES = TriggeredTrackSkimmer.cc

libTriggeredTrackSkimmer_la_LIBADD = \
  -lphool \
  -ltrackbase_historic_io \
  -ltrack_io \
  -lg4vertex \
  -lSubsysReco

BUILT_SOURCES = testexternals.cc

noinst_PROGRAMS = \
  testexternals

testexternals_SOURCES = testexternals.cc
testexternals_LDADD   = \
  libTriggeredTrackSkimmer.la


testexternals.cc:
	echo "//*** this is a generated file. Do not commit, do not edit" > $@
	echo "int main()" >> $@
	echo "{" >> $@
	echo "  return 0;" >> $@
	echo "}" >> $@

clean-local:
	rm -f $(BUILT_SOURCES)
