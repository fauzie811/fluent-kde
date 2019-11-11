### configuration plugin
add_definitions(-DTRANSLATION_DOMAIN=\"fluent_style_config\")
set(kstyle_fluent_config_PART_SRCS fluentstyleconfig.cpp)

if(FLUENT_USE_KDE4)

  kde4_add_kcfg_files(kstyle_fluent_config_PART_SRCS ../fluentstyleconfigdata.kcfgc)
  kde4_add_ui_files(kstyle_fluent_config_PART_SRCS ui/fluentstyleconfig.ui)
  kde4_add_plugin(kstyle_fluent_config ${kstyle_fluent_config_PART_SRCS})

  target_link_libraries(kstyle_fluent_config ${KDE4_KDEUI_LIBS} )
  install(TARGETS kstyle_fluent_config DESTINATION ${PLUGIN_INSTALL_DIR})

else()

  set(kstyle_fluent_config_PART_SRCS ${kstyle_fluent_config_PART_SRCS} fluentstyleconfigmodule.cpp)

  kconfig_add_kcfg_files(kstyle_fluent_config_PART_SRCS ../fluentstyleconfigdata.kcfgc)
  set(kstyle_fluent_config_PART_FORMS ui/fluentstyleconfig.ui)
  ki18n_wrap_ui(kstyle_fluent_config_PART_FORMS_HEADERS ${kstyle_fluent_config_PART_FORMS})

  add_library(kstyle_fluent_config MODULE
    ${kstyle_fluent_config_PART_SRCS}
    ${kstyle_fluent_config_PART_FORMS_HEADERS}
 )

  target_link_libraries(kstyle_fluent_config Qt5::Core Qt5::Gui Qt5::Widgets Qt5::DBus)
  target_link_libraries(kstyle_fluent_config KF5::ConfigCore KF5::ConfigWidgets KF5::I18n)
  install(TARGETS kstyle_fluent_config DESTINATION ${QT_PLUGIN_INSTALL_DIR})
  install(FILES fluentstyleconfig.desktop DESTINATION  ${SERVICES_INSTALL_DIR})

endif()

########### next target ###############
if(NOT FLUENT_USE_KDE4)

  find_package(KF5KCMUtils CONFIG)
  set_package_properties(KF5KCMUtils PROPERTIES
      DESCRIPTION "KF5 KCM utils Framework"
      URL "https://projects.kde.org/projects/frameworks/kcmutils"
      TYPE OPTIONAL
      PURPOSE "Required for fluent-settings5 application to build")

  if( KF5KCMUtils_FOUND )

      set(fluent_settings_SOURCES main.cpp)
      add_executable(fluent-settings5 ${fluent_settings_SOURCES} )
      target_link_libraries(fluent-settings5 Qt5::Core Qt5::Gui Qt5::Widgets )
      target_link_libraries(fluent-settings5 KF5::I18n KF5::KCMUtils)

      install(TARGETS fluent-settings5 ${INSTALL_TARGETS_DEFAULT_ARGS})

      # icon
      ecm_install_icons(ICONS sc-apps-fluent-settings.svgz
        DESTINATION ${ICON_INSTALL_DIR}
        THEME hicolor )
    endif()
endif()
