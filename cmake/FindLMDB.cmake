find_path(LMDB_INCLUDE_DIR NAMES  lmdb.h HINTS include)
find_library(LMDB_LIBRARIES NAMES lmdb   HINTS lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LMDB DEFAULT_MSG LMDB_INCLUDE_DIR LMDB_LIBRARIES)

mark_as_advanced(LMDB_INCLUDE_DIR LMDB_LIBRARIES)
