set(FLUENT_USE_KDE4 ${USE_KDE4})
if(FLUENT_USE_KDE4)

  ############ Language and toolchain features
  ############ copied from ECM
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel" AND NOT WIN32)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
  endif()

endif()

################# Qt/KDE #################
if(FLUENT_USE_KDE4)

  ### XCB
  if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(XCB xcb x11-xcb)
    add_feature_info("x11-xcb" XCB_FOUND "Required to pass style properties to native Windows on X11 Platform")
    set_feature_info("x11-xcb" "Required to pass style properties to native Windows on X11 Platform" "http://xcb.freedesktop.org")
    set(FLUENT_HAVE_X11 ${XCB_FOUND})
  else()
    set(FLUENT_HAVE_X11 FALSE)
  endif()

  set(FLUENT_HAVE_QTQUICK FALSE)
  set(FLUENT_HAVE_KWAYLAND FALSE)

  ### KStyle
  set(FLUENT_HAVE_KSTYLE FALSE)

else()

  find_package(Qt5 REQUIRED CONFIG COMPONENTS Widgets DBus)
  find_package(KF5 REQUIRED COMPONENTS
      I18n
      Config
      GuiAddons
      ConfigWidgets
      WindowSystem)

  find_package(Qt5 COMPONENTS Quick)
  set(FLUENT_HAVE_QTQUICK ${Qt5Quick_FOUND})

  find_package( KF5FrameworkIntegration CONFIG )
  set_package_properties(KF5FrameworkIntegration PROPERTIES
      DESCRIPTION "KF5 Framework Integration"
      URL "https://projects.kde.org/projects/frameworks/frameworkintegration"
      TYPE OPTIONAL
      PURPOSE "Required to use KStyle convenience functionalities in style")

  set(FLUENT_HAVE_KSTYLE ${KF5FrameworkIntegration_FOUND})

  find_package(XCB COMPONENTS XCB)
  set_package_properties(XCB PROPERTIES
      DESCRIPTION "X protocol C-language Binding"
      URL "http://xcb.freedesktop.org"
      TYPE OPTIONAL
      PURPOSE "Required to pass style properties to native Windows on X11 Platform"
  )

  find_package(KF5Wayland CONFIG)
  set(FLUENT_HAVE_KWAYLAND ${KF5Wayland_FOUND})

  if(UNIX AND NOT APPLE)

    set(FLUENT_HAVE_X11 ${XCB_XCB_FOUND})
    if (XCB_XCB_FOUND)
      find_package(Qt5 REQUIRED CONFIG COMPONENTS X11Extras)
    endif()

  else()

    set(FLUENT_HAVE_X11 FALSE)

  endif()


endif()

################# includes #################
include_directories(
  animations
  debug
)

include_directories(${CMAKE_SOURCE_DIR}/libfluentcommon)
include_directories(${CMAKE_BINARY_DIR}/libfluentcommon)

################# configuration #################
configure_file(config-fluent.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-fluent.h )

########### next target ###############
set(fluent_PART_SRCS
    animations/fluentanimation.cpp
    animations/fluentanimations.cpp
    animations/fluentanimationdata.cpp
    animations/fluentbaseengine.cpp
    animations/fluentbusyindicatordata.cpp
    animations/fluentbusyindicatorengine.cpp
    animations/fluentdialdata.cpp
    animations/fluentdialengine.cpp
    animations/fluentenabledata.cpp
    animations/fluentgenericdata.cpp
    animations/fluentheaderviewdata.cpp
    animations/fluentheaderviewengine.cpp
    animations/fluentscrollbardata.cpp
    animations/fluentscrollbarengine.cpp
    animations/fluentspinboxengine.cpp
    animations/fluentspinboxdata.cpp
    animations/fluentstackedwidgetdata.cpp
    animations/fluentstackedwidgetengine.cpp
    animations/fluenttabbarengine.cpp
    animations/fluenttabbardata.cpp
    animations/fluenttoolboxengine.cpp
    animations/fluenttransitiondata.cpp
    animations/fluenttransitionwidget.cpp
    animations/fluentwidgetstateengine.cpp
    animations/fluentwidgetstatedata.cpp
    debug/fluentwidgetexplorer.cpp
    fluentaddeventfilter.cpp
    fluentframeshadow.cpp
    fluenthelper.cpp
    fluentmdiwindowshadow.cpp
    fluentmnemonics.cpp
    fluentpropertynames.cpp
    fluentshadowhelper.cpp
    fluentsplitterproxy.cpp
    fluentstyle.cpp
    fluentstyleplugin.cpp
    fluenttileset.cpp
    fluentwindowmanager.cpp
)

if(NOT FLUENT_USE_KDE4)
  set(fluent_PART_SRCS
      ${fluent_PART_SRCS}
      fluentblurhelper.cpp
  )
endif()

if(FLUENT_USE_KDE4)

  kde4_add_kcfg_files(fluent_PART_SRCS fluentstyleconfigdata.kcfgc)
  kde4_add_plugin(fluent ${fluent_PART_SRCS} kstylekde4compat.cpp)
  target_link_libraries(fluent  ${KDE4_KDEUI_LIBS})
  target_link_libraries(fluent fluentcommon4)

  if(FLUENT_HAVE_X11)
    target_link_libraries(fluent ${X11_XCB_LIBRARIES})
    target_link_libraries(fluent ${XCB_LIBRARIES})
  endif()

  install(TARGETS fluent  DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/styles/)

else()

  kconfig_add_kcfg_files(fluent_PART_SRCS fluentstyleconfigdata.kcfgc)
  add_library(fluent MODULE ${fluent_PART_SRCS})
  target_link_libraries(fluent Qt5::Core Qt5::Gui Qt5::Widgets Qt5::DBus)
  if( FLUENT_HAVE_QTQUICK )
    target_link_libraries(fluent Qt5::Quick)
  endif()
  target_link_libraries(fluent KF5::ConfigCore KF5::ConfigWidgets KF5::GuiAddons KF5::WindowSystem)
  target_link_libraries(fluent fluentcommon5)

  if( KF5FrameworkIntegration_FOUND )
  target_link_libraries(fluent KF5::Style)
  endif()

  if (WIN32)
    # As stated in http://msdn.microsoft.com/en-us/library/4hwaceh6.aspx M_PI only gets defined
    # when if _USE_MATH_DEFINES is defined
    target_compile_definitions(fluent PRIVATE _USE_MATH_DEFINES _BSD_SOURCE)
  endif()

  if(FLUENT_HAVE_X11)
    target_link_libraries(fluent ${XCB_LIBRARIES})
    target_link_libraries(fluent Qt5::X11Extras)
  endif()

  if(FLUENT_HAVE_KWAYLAND)
    target_link_libraries(fluent KF5::WaylandClient)
  endif()

  install(TARGETS fluent DESTINATION ${QT_PLUGIN_INSTALL_DIR}/styles/)

endif()

########### install files ###############
install(FILES fluent.themerc  DESTINATION  ${DATA_INSTALL_DIR}/kstyle/themes)

########### subdirectories ###############
add_subdirectory(config)
