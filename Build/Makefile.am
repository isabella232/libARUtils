###############################################################################
# THE LIBRARIES TO BUILD
###############################################################################
ACLOCAL_AMFLAGS                                             =   -I m4

# The list of header files that belong to the library (to be installed later)
HEADER_FILES                                                =   ../Includes/libARUtils/ARUTILS_Error.h \
                                                                ../Includes/libARUtils/ARUTILS_FileSystem.h \
                                                                ../Includes/libARUtils/ARUTILS_Http.h \
                                                                ../Includes/libARUtils/ARUTILS_Ftp.h \                                                                
                                                                ../Includes/libARUtils/ARUtils.h

# The sources to add to the library and to add to the source distribution
SOURCE_FILES                                                =   $(HEADER_FILES)\
                                                                ../Sources/ARUTILS_Error.c \
                                                                ../Sources/ARUTILS_FileSystem.c \
                                                                ../Sources/ARUTILS_Http.c \
                                                                ../Sources/ARUTILS_Http.h \
                                                                ../Sources/ARUTILS_Ftp.c \
                                                                ../Sources/ARUTILS_Ftp.h                                                                

# The library names to build (note we are building static and shared libs)
if DEBUG_MODE
lib_LTLIBRARIES                                      =   libarutils_dbg.la
libarutils_dbg_ladir                                 =   $(includedir)/libARUtils
libarutils_dbg_la_HEADERS                            =   $(HEADER_FILES)
libarutils_dbg_la_SOURCES                            =   $(SOURCE_FILES)
else
lib_LTLIBRARIES                                      =   libarutils.la
libarutils_ladir                                     =   $(includedir)/libARUtils
libarutils_la_HEADERS                                =   $(HEADER_FILES)
libarutils_la_SOURCES                                =   $(SOURCE_FILES)
endif


check_PROGRAMS                                              =   ../TestBench/Linux/autoTest
# bin_PROGRAMS                                              =   ../TestBench/Linux/autoTest

___TestBench_Linux_autoTest_SOURCES                         =   ../TestBench/Linux/autoTest.c \
                                                                ../TestBench/Linux/test_http_connection.c \
                                                                ../TestBench/Linux/test_ftp_connection.c                                                                

if DEBUG_MODE
___TestBench_Linux_autoTest_LDADD                           =   libarutils_dbg.la \
                                                                -larsal_dbg         \
                                                                -lcurl
else
___TestBench_Linux_autoTest_LDADD                           =   libarutils.la     \
                                                                -larsal             \
                                                                -lcurl
endif

CLEAN_FILES                                                 =   libarutils.la       \
                                                                libarutils_dbg.la
                                
EXTRA_DIST                                                  =   bootstrap

# Ensure the distribution of the doxygen configuration file
# Doxygen support
if HAVE_DOXYGEN
include doxygen.am
EXTRA_DIST                                                  +=  doxygen.ac  \
                                                                doxygen.am  \
                                                                $(DX_CONFIG)
endif

