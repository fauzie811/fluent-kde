project(fluent)
set(PROJECT_VERSION "5.17.80")
set(PROJECT_VERSION_MAJOR 5)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

option(USE_KDE4 "Build a widget style for KDE4 (and nothing else)")

include(GenerateExportHeader)
include(WriteBasicConfigVersionFile)
include(FeatureSummary)

if(USE_KDE4)
  find_package(KDE4 REQUIRED)
  include(KDE4Defaults)
  include(MacroLibrary)

  add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
  include_directories(${KDE4_INCLUDES})

  add_subdirectory(libfluentcommon)
  add_subdirectory(kstyle)
else()
  find_package(ECM 0.0.9 REQUIRED NO_MODULE)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_SOURCE_DIR}/cmake)

  include(ECMInstallIcons)
  include(KDEInstallDirs)
  include(KDECMakeSettings)
  include(KDECompilerSettings NO_POLICY_SCOPE)
  include(KDEClangFormat)

  include(GtkUpdateIconCache)

  option(WITH_DECORATIONS "Build Fluent window decorations for KWin" ON)
  if(WITH_DECORATIONS)
    find_package(KDecoration2 REQUIRED)
    add_subdirectory(kdecoration)
  endif()

  add_subdirectory(colors)
  add_subdirectory(libfluentcommon)
  add_subdirectory(kstyle)
  add_subdirectory(misc)

  if(EXISTS ${CMAKE_SOURCE_DIR}/po AND IS_DIRECTORY ${CMAKE_SOURCE_DIR}/po)
    find_package(KF5I18n CONFIG REQUIRED)
    ki18n_install(po)
  endif()

  include(ECMSetupVersion)
  ecm_setup_version(${PROJECT_VERSION} VARIABLE_PREFIX FLUENT
                    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/FluentConfigVersion.cmake"
                    )

  # create a Config.cmake and a ConfigVersion.cmake file and install them
  set(CMAKECONFIG_INSTALL_DIR "${CMAKECONFIG_INSTALL_PREFIX}/Fluent")

  ecm_configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/FluentConfig.cmake.in"
                                    "${CMAKE_CURRENT_BINARY_DIR}/FluentConfig.cmake"
                                    PATH_VARS KDE_INSTALL_FULL_DATADIR
                                    INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
                                    )

  # add clang-format target for all our real source files
  file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
  kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

  install(FILES  "${CMAKE_CURRENT_BINARY_DIR}/FluentConfig.cmake"
                 "${CMAKE_CURRENT_BINARY_DIR}/FluentConfigVersion.cmake"
          DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
          COMPONENT Devel
         )
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
