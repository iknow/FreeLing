INCLUDES = -I$(top_srcdir)/libcfg+
EXTRA_DIST = *.h platon/str/*.h

lib_LTLIBRARIES = libcfg+.la

libcfg__la_SOURCES = cfg+.c cfgfile.c cmdline.c parse.c props.c shared.c platon/str/dynfgets.c platon/str/strctype.c platon/str/strdyn.c platon/str/strplus.c

libcfg__la_CPPFLAGS = -fPIC
libcfg__la_LDFLAGS = -release 0.6.2
