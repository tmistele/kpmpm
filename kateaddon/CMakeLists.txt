# Most plugins will need to link against KF5TextEditor to have access to its plugin interface.
find_package(KF5TextEditor ${KF5_DEP_VERSION} QUIET REQUIRED)

add_library(ktexteditorpreviewplugin MODULE "")
target_compile_definitions(ktexteditorpreviewplugin PRIVATE TRANSLATION_DOMAIN="ktexteditorpreviewplugin")
target_link_libraries(ktexteditorpreviewplugin PRIVATE KF5::TextEditor kmarkdownwebviewpart)

include(ECMQtDeclareLoggingCategory)
ecm_qt_declare_logging_category(
  DEBUG_SOURCES
  HEADER ktexteditorpreview_debug.h
  IDENTIFIER KTEPREVIEW
  CATEGORY_NAME "ktexteditorpreviewplugin"
)
target_sources(ktexteditorpreviewplugin PRIVATE ${DEBUG_SOURCES})

target_sources(
  ktexteditorpreviewplugin
  PRIVATE
    ktexteditorpreviewplugin.cpp
    ktexteditorpreviewview.cpp
    previewwidget.cpp
    kpartview.cpp
)

install(TARGETS ktexteditorpreviewplugin DESTINATION ${KDE_INSTALL_PLUGINDIR}/ktexteditor)
