add_definitions(-DTRANSLATION_DOMAIN="fluent_kwin_deco")

find_package(KF5 REQUIRED COMPONENTS CoreAddons GuiAddons ConfigWidgets WindowSystem I18n)
find_package(Qt5 CONFIG REQUIRED COMPONENTS DBus)

### XCB
find_package(XCB COMPONENTS XCB)
set_package_properties(XCB PROPERTIES
  DESCRIPTION "X protocol C-language Binding"
  URL "http://xcb.freedesktop.org"
  TYPE OPTIONAL
  PURPOSE "Required to pass style properties to native Windows on X11 Platform"
)

if(UNIX AND NOT APPLE)

  set(FLUENT_HAVE_X11 ${XCB_XCB_FOUND})
  if (XCB_XCB_FOUND)
    find_package(Qt5 REQUIRED CONFIG COMPONENTS X11Extras)
  endif()

else()

  set(FLUENT_HAVE_X11 FALSE)

endif()

################# configuration #################
configure_file(config-fluent.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-fluent.h )

################# includes #################
include_directories(${CMAKE_SOURCE_DIR}/libfluentcommon)
include_directories(${CMAKE_BINARY_DIR}/libfluentcommon)

################# newt target #################
### plugin classes
set(fluentdecoration_SRCS
    fluentbutton.cpp
    fluentdecoration.cpp
    fluentexceptionlist.cpp
    fluentsettingsprovider.cpp)

kconfig_add_kcfg_files(fluentdecoration_SRCS fluentsettings.kcfgc)

### config classes
### they are kept separately because they might move in a separate library in the future
set(fluentdecoration_config_SRCS
    config/fluentconfigwidget.cpp
    config/fluentdetectwidget.cpp
    config/fluentexceptiondialog.cpp
    config/fluentexceptionlistwidget.cpp
    config/fluentexceptionmodel.cpp
    config/fluentitemmodel.cpp
)

set(fluentdecoration_config_PART_FORMS
   config/ui/fluentconfigurationui.ui
   config/ui/fluentdetectwidget.ui
   config/ui/fluentexceptiondialog.ui
   config/ui/fluentexceptionlistwidget.ui
)

ki18n_wrap_ui(fluentdecoration_config_PART_FORMS_HEADERS ${fluentdecoration_config_PART_FORMS})

### build library
add_library(fluentdecoration MODULE
    ${fluentdecoration_SRCS}
    ${fluentdecoration_config_SRCS}
    ${fluentdecoration_config_PART_FORMS_HEADERS})

target_link_libraries(fluentdecoration
    PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::DBus
    PRIVATE
        fluentcommon5
        KDecoration2::KDecoration
        KF5::ConfigCore
        KF5::CoreAddons
        KF5::ConfigWidgets
        KF5::GuiAddons
        KF5::I18n
        KF5::WindowSystem)

if(FLUENT_HAVE_X11)
  target_link_libraries(fluentdecoration
    PUBLIC
      Qt5::X11Extras
      XCB::XCB)
endif()


install(TARGETS fluentdecoration DESTINATION ${PLUGIN_INSTALL_DIR}/org.kde.kdecoration2)
install(FILES config/fluentdecorationconfig.desktop DESTINATION  ${SERVICES_INSTALL_DIR})
