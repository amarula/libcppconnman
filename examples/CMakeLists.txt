if(BUILD_CONNMAN)
  add_executable(connmanctl_dbus connmanctl.cpp)
  target_link_libraries(connmanctl_dbus PRIVATE GConnmanDbus)
  pkg_check_modules(READLINE REQUIRED IMPORTED_TARGET readline)
  target_link_libraries(connmanctl_dbus PRIVATE PkgConfig::READLINE)

  install(
    TARGETS connmanctl_dbus
    EXPORT ${PROJECT_NAME}-config
    COMPONENT ${PROJECT_NAME})
endif(BUILD_CONNMAN)
