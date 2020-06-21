
configure_file(markdownpartconfig.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/markdownpartconfig.h)

set(markdownpart_SRCS
    searchtoolbar.cpp
    markdownpartfactory.cpp
    markdownpart.cpp
    markdownsourcedocument.cpp
    markdownbrowserextension.cpp
)

ki18n_wrap_ui(markdownpart_SRCS searchtoolbar.ui)

qt5_add_resources(markdownpart_SRCS
    markdownpart.qrc
)

# SHARED instead of MODULE so that kateaddon/ can link to it directly
# TODO: Better way to achieve this?
add_library(kmarkdownwebviewpart  SHARED ${markdownpart_SRCS})
kcoreaddons_desktop_to_json(kmarkdownwebviewpart kmarkdownwebviewpart.desktop)
# For kateaddon
generate_export_header(kmarkdownwebviewpart BASE_NAME kmarkdownviewpart)

target_link_libraries(kmarkdownwebviewpart
    KMarkdownWebView
    KF5::I18n
    KF5::Parts
)

install(TARGETS kmarkdownwebviewpart  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/parts)

# Install a second time, so that kateaddon can link to it directly
# TODO: Better way to achieve this? Make one of the two a symlink?
install(TARGETS kmarkdownwebviewpart ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES kmarkdownwebviewpart.desktop  DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

install(
    FILES org.kde.kmarkdownwebviewpart.metainfo.xml
    DESTINATION ${KDE_INSTALL_METAINFODIR}
)
