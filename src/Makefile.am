lib_LTLIBRARIES = libdpcp.la

libdpcp_la_CXXFLAGS =
libdpcp_la_CFLAGS =
libdpcp_la_LIBADD = \
	 $(VERBS_LIBS)

libdpcp_la_LDFLAGS = -no-undefined -version-number @PRJ_MAJOR@:@PRJ_MINOR@:@PRJ_REVISION@


libdpcp_la_CPPFLAGS = \
	-I$(top_srcdir)/src/dcmd/linux \
	-I$(top_srcdir)/src/utils/linux

libdpcp_la_SOURCES = \
	dpcp/adapter.cpp \
	dpcp/cq.cpp \
	dpcp/dpcp.cpp \
	dpcp/dpcp_obj.cpp \
	dpcp/eq.cpp \
	dpcp/internal.h \
	dpcp/fr.cpp \
	dpcp/mkey.cpp \
	dpcp/rq.cpp \
	dpcp/tir.cpp \
	dpcp/tis.cpp \
	dpcp/dek.cpp \
	dpcp/sq.cpp \
	dpcp/parser_graph_node.cpp \
	dpcp/flow_table.cpp \
	dpcp/flow_group.cpp \
	dpcp/flow_action.cpp \
	dpcp/flow_rule_ex.cpp \
	dpcp/flow_matcher.cpp \
	dpcp/forwardable_obj.cpp \
	dpcp/tag_buffer_table_obj.cpp \
	\
	dcmd/dcmd.h \
	dcmd/prm.h \
	\
	dcmd/base/base_provider.h \
	dcmd/base/base_device.h \
	dcmd/base/base_ctx.h \
	dcmd/base/base_obj.h \
	dcmd/base/base_uar.h \
	dcmd/base/base_umem.h \
	dcmd/base/base_flow.h \
	dcmd/base/base_action.h \
	\
	dcmd/linux/def.h \
	dcmd/linux/provider.cpp \
	dcmd/linux/provider.h \
	dcmd/linux/compchannel.cpp \
	dcmd/linux/compchannel.h \
	dcmd/linux/device.cpp \
	dcmd/linux/device.h \
	dcmd/linux/ctx.cpp \
	dcmd/linux/ctx.h \
	dcmd/linux/obj.cpp \
	dcmd/linux/obj.h \
	dcmd/linux/uar.cpp \
	dcmd/linux/uar.h \
	dcmd/linux/umem.cpp \
	dcmd/linux/umem.h \
	dcmd/linux/flow.cpp \
	dcmd/linux/flow.h \
	dcmd/linux/action.cpp \
	dcmd/linux/action.h \
	\
	utils/os.h \
	\
	utils/linux/log.cpp \
	utils/linux/log.h \
	utils/linux/utils.cpp \
	utils/linux/utils.h

otherincludedir = $(includedir)/mellanox
otherinclude_HEADERS = \
	api/dpcp.h

install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/libdpcp.la
	rm -f $(DESTDIR)$(libdir)/libdpcp.a

uninstall-hook:
	rm -f $(DESTDIR)$(libdir)/libdpcp.so*
