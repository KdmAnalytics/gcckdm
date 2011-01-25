
GCCPLUGINS_DIR = `$(CXX) -print-file-name=plugin`

GCCKDMHEADERS = $(top_srcdir)/include/gcckdm/kdmtriplewriter/GimpleKdmTripleWriter.hh\
  $(top_srcdir)/include/gcckdm/kdmtriplewriter/KdmTripleWriter.hh\
  $(top_srcdir)/include/gcckdm/kdmtriplewriter/ActionData.hh\
  $(top_srcdir)/include/gcckdm/kdmtriplewriter/Exception.hh\
  $(top_srcdir)/include/gcckdm/kdmtriplewriter/ExpandedLocationFunctors.hh\
  $(top_srcdir)/include/gcckdm/kdmtriplewriter/PathHash.hh\
  $(top_srcdir)/include/gcckdm/kdmtriplewriter/TripleWriter.hh\
  $(top_srcdir)/include/gcckdm/kdmtriplewriter/UidNode.hh\
  $(top_srcdir)/include/gcckdm/kdmtriplewriter/KdmTripleWriterFwd.hh\
  $(top_srcdir)/include/gcckdm/utilities/null_deleter.hpp\
  $(top_srcdir)/include/gcckdm/utilities/null_ptr.hpp\
  $(top_srcdir)/include/gcckdm/utilities/Singleton.hh\
  $(top_srcdir)/include/gcckdm/utilities/unique_ptr.hpp\
  $(top_srcdir)/include/gcckdm/GccKdmUtilities.hh\
  $(top_srcdir)/include/gcckdm/KdmPredicate.hh\
  $(top_srcdir)/include/gcckdm/KdmType.hh\
  $(top_srcdir)/include/gcckdm/KdmKind.hh\
  $(top_srcdir)/include/gcckdm/GccKdmFwd.hh\
  $(top_srcdir)/include/gcckdm/GccAstListener.hh\
  $(top_srcdir)/include/gcckdm/GccKdmVersion.hh\
  $(top_srcdir)/include/gcckdm/GccKdmConfig.hh
                               


lib_LTLIBRARIES =  libGccKdm.la
libGccKdm_la_SOURCES = gcckdm/kdmtriplewriter/GimpleKdmTripleWriter.cc\
                       gcckdm/kdmtriplewriter/KdmTripleWriter.cc\
                       gcckdm/GccKdmUtilities.cc\
                       gcckdm/KdmPredicate.cc\
                       gcckdm/GccKdmPlugin.cc\
                       gcckdm/KdmType.cc\
                       $(GCCKDMHEADERS)
                       
libGccKdm_la_CPPFLAGS = -I$(top_srcdir)/include -I$(GCCPLUGINS_DIR)/include
libGccKdm_la_CXXFLAGS = -Wno-deprecated
