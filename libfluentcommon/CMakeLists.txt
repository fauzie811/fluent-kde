set(FLUENT_COMMON_USE_KDE4 ${USE_KDE4})

if (FLUENT_COMMON_USE_KDE4)
    ############ Language and toolchain features
    ############ copied from ECM
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel" AND NOT WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    endif ()
endif ()

################# dependencies #################
### Qt/KDE
if (NOT FLUENT_COMMON_USE_KDE4)
    find_package(Qt5 REQUIRED CONFIG COMPONENTS Widgets)
endif ()

################# configuration #################
configure_file(config-fluentcommon.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-fluentcommon.h )

################# fluentstyle target #################
set(fluentcommon_LIB_SRCS
    fluentboxshadowrenderer.cpp
)

if (FLUENT_COMMON_USE_KDE4)
    kde4_add_library(fluentcommon4 SHARED ${fluentcommon_LIB_SRCS})

    generate_export_header(fluentcommon4
        BASE_NAME fluentcommon
        EXPORT_FILE_NAME fluentcommon_export.h)

    target_link_libraries(fluentcommon4 ${KDE4_KDEUI_LIBS})

    set_target_properties(fluentcommon4 PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR})

    install(TARGETS fluentcommon4 ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
else ()
    add_library(fluentcommon5 ${fluentcommon_LIB_SRCS})

    generate_export_header(fluentcommon5
        BASE_NAME fluentcommon
        EXPORT_FILE_NAME fluentcommon_export.h)

    target_link_libraries(fluentcommon5
        PUBLIC
            Qt5::Core
            Qt5::Gui)

    set_target_properties(fluentcommon5 PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR})

    install(TARGETS fluentcommon5 ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
endif ()
