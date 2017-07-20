if(NOT UNIX)
    message(FATAL_ERROR "Your system does not support Breakpad integration yet.")
endif()

set(BREAKPAD_ROOT_HINTS ${BREAKPAD_ROOT_DIR} ENV BREAKPAD_ROOT_DIR)

find_path(
    BREAKPAD_CLIENT_INCLUDE_DIR
    NAMES "client/linux/handler/exception_handler.h"
    HINTS ${BREAKPAD_ROOT_HINTS}
    PATH_SUFFIXES breakpad
)

find_library(
    BREAKPAD_CLIENT_LIBRARY
    NAMES libbreakpad_client.a
    HINTS ${BREAKPAD_ROOT_HINTS}
    PATH_SUFFIXES lib
)

mark_as_advanced(BREAKPAD_CLIENT_INCLUDE_DIR BREAKPAD_CLIENT_LIBRARY)

add_library(Breakpad::Client STATIC IMPORTED)
set_target_properties(
    Breakpad::Client
    PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    INTERFACE_INCLUDE_DIRECTORIES "${BREAKPAD_CLIENT_INCLUDE_DIR}"
    IMPORTED_LOCATION "${BREAKPAD_CLIENT_LIBRARY}"
)
