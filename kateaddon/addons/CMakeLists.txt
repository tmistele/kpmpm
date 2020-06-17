# Most plugins will need to link against KF5TextEditor to have access to its plugin interface.
find_package(KF5TextEditor ${KF5_DEP_VERSION} QUIET REQUIRED)

ecm_optional_add_subdirectory(preview)
