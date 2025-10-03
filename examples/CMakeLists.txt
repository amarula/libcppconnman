add_compile_definitions(LCM_LOG_DEFAULT_ENABLED=1)

if(BUILD_CONNMAN)
  add_executable(connmanctl_dbus connmanctl.cpp)
  target_link_libraries(connmanctl_dbus PRIVATE GConnmanDbus)

  install(
    TARGETS connmanctl_dbus
    EXPORT ${PROJECT_NAME}-config
    COMPONENT ${PROJECT_NAME})
endif(BUILD_CONNMAN)
