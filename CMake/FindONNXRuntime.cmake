find_path(
  ONNXRuntime_INCLUDE_DIR
  NAMES onnxruntime_cxx_api.h
  HINTS
    ${ONNXRUNTIME_ROOT}
    $ENV{ONNXRUNTIME_ROOT}
  PATH_SUFFIXES
    include
    include/onnxruntime/core/session
)

find_library(
  ONNXRuntime_LIBRARY
  NAMES onnxruntime
  HINTS
    ${ONNXRUNTIME_ROOT}
    $ENV{ONNXRUNTIME_ROOT}
  PATH_SUFFIXES
    lib
    lib64
    build/Windows/Release/Release
    runtimes/win-x64/native
)

find_file(
  ONNXRuntime_DLL
  NAMES onnxruntime.dll
  HINTS
    ${ONNXRUNTIME_ROOT}
    $ENV{ONNXRUNTIME_ROOT}
  PATH_SUFFIXES
    lib
    bin
    runtimes/win-x64/native
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ONNXRuntime DEFAULT_MSG ONNXRuntime_LIBRARY ONNXRuntime_INCLUDE_DIR)

if(ONNXRuntime_FOUND AND NOT TARGET ONNXRuntime::onnxruntime)
  add_library(ONNXRuntime::onnxruntime UNKNOWN IMPORTED)
  set_target_properties(
    ONNXRuntime::onnxruntime
    PROPERTIES
      IMPORTED_LOCATION "${ONNXRuntime_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${ONNXRuntime_INCLUDE_DIR}"
  )
endif()
