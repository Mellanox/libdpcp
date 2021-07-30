noinst_PROGRAMS = gtest

CXXFLAGS = $(GTEST_CXXFLAGS)

gtest_LDADD = \
	$(top_builddir)/src/libdpcp.la

gtest_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/dcmd/linux \
	-I$(top_srcdir)/tests/gtest

gtest_LDFLAGS  = $(GTEST_LDFLAGS) -no-install $(VERBS_LIBS) -lrdmacm -lpthread
gtest_CXXFLAGS = $(GTEST_CXXFLAGS) -g -O3 -fno-tree-vectorize
gtest_DEPENDENCIES = $(gtest_LDADD)

gtest_SOURCES = \
	main.cpp\
	\
	common/gtest-all.cpp\
	common/sys.cpp\
	\
	common/base.cpp\
	\
	dcmd/dcmd_base.cpp\
	dcmd/dcmd_provider.cpp\
	dcmd/dcmd_compchannel.cpp\
	dcmd/dcmd_device.cpp\
	dcmd/dcmd_ctx.cpp\
	dcmd/dcmd_obj.cpp\
	\
	dpcp/dpcp_base.cpp\
	dpcp/provider_tests.cpp\
	dpcp/obj_tests.cpp\
	dpcp/pd_tests.cpp\
	dpcp/td_tests.cpp\
	dpcp/mkey_tests.cpp\
	dpcp/uar_tests.cpp\
	dpcp/rq_tests.cpp\
	dpcp/rq_dpp_tests.cpp\
	dpcp/tir_tests.cpp\
	dpcp/fr_tests.cpp\
	dpcp/adapter_tests.cpp

noinst_HEADERS = \
	common/gtest.h \
	common/tap.h \
	common/def.h \
	common/sys.h \
	common/log.h \
	common/cmn.h \
	\
	common/base.h \
	\
	dcmd/dcmd_base.h \
	dpcp/dpcp_base.h


.PHONY: test gdb valgrind


all-local: gtest


#
# List variables
#
GTEST_FILTER        ?= *
GTEST_EXTRA_ARGS    =
LAUNCHER            =
VALGRIND_EXTRA_ARGS =

GTEST_ARGS = \
	--gtest_filter=$(GTEST_FILTER) \
	$(GTEST_EXTRA_ARGS)

VALGRIND_ARGS = \
	--tool=memcheck \
	--leak-check=full \
	--track-origins=yes \
	--fair-sched=try \
	$(VALGRIND_EXTRA_ARGS)

#
# List of rules
#
libdpcp:
	$(MAKE) -C $(top_builddir)

help:
	@echo
	@echo "Targets:"
	@echo "  list          : List unit tests."
	@echo "  test          : Run unit tests."
	@echo "  test_gdb      : Run unit tests with GDB."
	@echo "  test_valgrind : Run unit tests with Valgrind."
	@echo
	@echo "Environment variables:"
	@echo "  GTEST_FILTER        : Unit tests filter (\"$(GTEST_FILTER)\")"
	@echo "  GTEST_EXTRA_ARGS    : Additional arguments for gtest (\"$(GTEST_EXTRA_ARGS)\")"
	@echo "  LAUNCHER            : Custom launcher for gtest executable (\"$(LAUNCHER)\")"
	@echo "  VALGRIND_EXTRA_ARGS : Additional arguments for Valgrind (\"$(VALGRIND_EXTRA_ARGS)\")"
	@echo

#
# List unit tests
#
list: gtest
	$(abs_builddir)/gtest --gtest_list_tests $(GTEST_ARGS)

#
# Run unit tests
#
test: libdpcp gtest
	@rm -f core.*
	env $(LAUNCHER) $(abs_builddir)/gtest $(GTEST_ARGS)
	
#
# Run unit tests with GDB
#
test_gdb: libdpcp gtest
	echo -e 'r\ninit-if-undefined $$_exitcode=-1\nif $$_exitcode>=0\n\tq\nend' > .gdbcommands
	env $(LAUNCHER) \
		gdb -x .gdbcommands --args $(GDB_ARGS) \
			$(abs_builddir)/gtest $(GTEST_ARGS)

#
# Run unit tests with valgrind
#
test_valgrind: libdpcp gtest
	env $(LAUNCHER) LD_LIBRARY_PATH="$(VALGRIND_LIBPATH):${LD_LIBRARY_PATH}" \
	valgrind $(VALGRIND_ARGS) $(abs_builddir)/gtest $(GTEST_ARGS)
