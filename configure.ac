# Indicate that we require autoconf 2.63 or later
#
AC_PREREQ(2.63)


dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 1: Setting product version
dnl===
dnl===-----------------------------------------------------------------------===

# Update version number here:
#
define([prj_ver_major], 1)
define([prj_ver_minor], 1)
define([prj_ver_revision], 48)
define([prj_ver_release], esyscmd([echo ${PRJ_RELEASE:=0}]))


# Initialize autoconf and define the package name, version number and
# email address for reporting bugs.
#
AC_INIT(dpcp, [prj_ver_major.prj_ver_minor.prj_ver_revision], support@mellanox.com)

# Definitions will be placed in this file rather than
# in the DEFS variable
#
AC_CONFIG_HEADER([config.h])

PRJ_MAJOR=prj_ver_major
PRJ_MINOR=prj_ver_minor
PRJ_REVISION=prj_ver_revision
PRJ_RELEASE=prj_ver_release

AC_DEFINE_UNQUOTED(PRJ_MAJOR, ${PRJ_MAJOR}, [Project Major Version])
AC_SUBST(PRJ_MAJOR)
AC_DEFINE_UNQUOTED(PRJ_MINOR, ${PRJ_MINOR}, [Project Minor Version])
AC_SUBST(PRJ_MINOR)
AC_DEFINE_UNQUOTED(PRJ_REVISION, ${PRJ_REVISION}, [Project Revision])
AC_SUBST(PRJ_REVISION)
AC_DEFINE_UNQUOTED(PRJ_RELEASE, ${PRJ_RELEASE}, [Project Release])
AC_SUBST(PRJ_RELEASE)

BUILD_VER=`git describe --long --abbrev=40 --dirty --tags 2> /dev/null || echo ""`
if test -n "$BUILD_VER"; then BUILD_VER=`echo $BUILD_VER | sed -e 's/-dirty/+/' | sed s/.*-g//`; else  BUILD_VER=""; fi

AC_DEFINE_UNQUOTED(BUILD_VER, "${BUILD_VER}", [SCM Version])
AC_SUBST(BUILD_VER)

dateopt=""
    if test -n "$SOURCE_DATE_EPOCH" ; then
    dateopt="-u -d @$SOURCE_DATE_EPOCH"
fi
AC_SUBST([BUILD_DATE], [$(date $dateopt +'%b/%d/%Y')])
AC_SUBST([BUILD_TIME], [$(date $dateopt +'%H:%M:%S')])
AC_SUBST([BUILD_DATE_CHANGELOG], [$(date $dateopt +'%a, %d %b %Y %T %z')])

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 2: Initialization & Setup
dnl===
dnl===-----------------------------------------------------------------------===

# Verify that the source directory is valid.
#
AC_CONFIG_SRCDIR(src)

# Place for the extra autoconf files.
#
AC_CONFIG_AUX_DIR(config/aux)

# Place all our m4 macro into the config subdirectory.
#
AC_CONFIG_MACRO_DIR(config/m4)

# Compute the canonical target-system type variable, target, and its three
# individual parts target_cpu, target_vendor, and target_os.
#
AC_CANONICAL_TARGET

# Init automake and libtool
#
AM_INIT_AUTOMAKE(foreign [subdir-objects])

# Set non-verbose make by default
#
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

FUNC_CONFIGURE_INIT()


dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 3: Checking for programs we need
dnl===
dnl===-----------------------------------------------------------------------===
show_section_title "Configure build tools"

# Find compiler, libtools, etc
#
AC_PROG_CC
AC_PROG_CXX

# Check for pkg-config package
#
AC_CHECK_PROG(
    [have_pkg_config],
    [pkg-config],
    m4_ifdef([PKG_PROG_PKG_CONFIG], yes, no_pkg_m4),
    no_pkg_config)
case "$have_pkg_config" in
    no_pkg_m4)
        AC_MSG_ERROR([
        *** You do not have pkg.m4 properly installed.
        *** aclocal can not find one.
        *** Set the environment variable ACLOCAL="aclocal -I/path/to/pkg.m4"
        ])
        ;;
    no_pkg_config)
        AC_MSG_ERROR([
        *** The pkg-config is required to build the library.
        *** Make sure it is installed or set path to pkg-config.
        ])
        ;;
    *)
        ;;
esac
PKG_PROG_PKG_CONFIG()

LT_INIT
AC_SUBST([LIBTOOL_STATIC], [--enable-static])

LINKING_OPTIMIZATION()


dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 4: Setting compiler specific options
dnl===
dnl===-----------------------------------------------------------------------===
show_section_title "Setting compiler specific options"

CFLAGS="-D_GNU_SOURCE -fPIC $CFLAGS"
CXXFLAGS="-D_GNU_SOURCE -fPIC $CXXFLAGS"
CFLAGS_SECURITY=""

AC_MSG_CHECKING([for cpu])
AC_MSG_RESULT([$target_cpu])
case $target_cpu in
    aarch64|arm64)
        # harden compiler/linker flags for L4T builds
        AX_APPEND_COMPILE_FLAGS(m4_flatten([
            -Wl,-z,relro,-z,now
            -fstack-protector-strong
            -Wl,-z,noexecstack
            -fstack-clash-protection 
        ]), [CFLAGS_SECURITY])
        CFLAGS_SECURITY="$CFLAGS_SECURITY -D_FORTIFY_SOURCE=2"
        ;;
esac

AC_MSG_CHECKING([for compiler])
case $CC in
    gcc*|g++*)
        AC_MSG_RESULT([gcc])
        CFLAGS="$CFLAGS $CFLAGS_SECURITY -Wall -Wextra -Werror -Wundef -Wformat=2 -Wformat-security -ffunction-sections -fdata-sections -Wsequence-point -pipe -Winit-self -Wmissing-include-dirs"
        CXXFLAGS="$CXXFLAGS $CFLAGS_SECURITY -Wshadow -Wall -Wextra -Werror -Wundef -Wformat=2 -Wformat-security -ffunction-sections -fdata-sections -Wsequence-point -pipe -Winit-self -Wmissing-include-dirs"
        ;;
    icc*|icpc*)
        AC_MSG_RESULT([icc])
        CFLAGS="$CFLAGS -Wall -Werror"
        CXXFLAGS="$CXXFLAGS -Wall -Werror"
        ;;
    clang*|clang++*)
        AC_MSG_RESULT([clang])
        CFLAGS="$CFLAGS $CFLAGS_SECURITY -Wall -Werror -Wno-format-security -Wno-self-assign"
        CXXFLAGS="$CXXFLAGS $CFLAGS_SECURITY -Wall -Werror -Wno-overloaded-virtual"
        ;;
    *)
        AC_MSG_RESULT([unknown])
        ;;
esac

AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 5: Checking for project configuration
dnl===
dnl===-----------------------------------------------------------------------===
show_section_title "Configure project"

# OFED configuration.
#
AC_MSG_CHECKING([for OFED path])
AC_ARG_WITH(ofed,
    AS_HELP_STRING([--with-ofed], [Path to OFED install]),
    [ac_cv_ofed_path=$withval],
    [if test -e "/etc/infiniband/info" ; then
         ac_cv_ofed_path=`grep prefix /etc/infiniband/info | awk -F "=" '{print $2}'`
     else
         ac_cv_ofed_path="/usr"
     fi])
CPPFLAGS="$CPPFLAGS -I$ac_cv_ofed_path/include"
if test -d "$ac_cv_ofed_path/lib64" ; then
    LDFLAGS="$LDFLAGS -L$ac_cv_ofed_path/lib64"
elif test -d "$ac_cv_ofed_path/lib/$(uname -m)-linux-*" ; then
    LDFLAGS="$LDFLAGS -L$(ls -d $ac_cv_ofed_path/lib/$(uname -m)-linux-*)"
else
    LDFLAGS="$LDFLAGS -L$ac_cv_ofed_path/lib"
fi
AC_MSG_RESULT($ac_cv_ofed_path)

VERBS_CAPABILITY_SETUP()

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 6: Checking for header files
dnl===
dnl===-----------------------------------------------------------------------===
show_section_title "Check for header files"

# Look for Standard headers
#
AC_HEADER_STDC


dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 7: Checking for libraries
dnl===
dnl===-----------------------------------------------------------------------===
show_section_title "Check for libraries"
AX_CHECK_CUDA

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 8: Checking for types and structures
dnl===
dnl===-----------------------------------------------------------------------===
show_section_title "Check for functions, types and structures"


dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 9: Configure makefiles
dnl===
dnl===-----------------------------------------------------------------------===
show_section_title "Configure makefiles"
AC_CONFIG_FILES([
		Makefile
		src/Makefile
		tests/gtest/Makefile
		contrib/scripts/dpcp.spec
		debian/changelog
		])

AC_OUTPUT

show_summary_title
