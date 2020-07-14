set(kmarkdownview_SRCS
    kmarkdownviewpage.cpp
    kmarkdownview.cpp
    kabstractmarkdownsourcedocument.cpp
    kmarkdownhtmlview.cpp
)

qt5_add_resources(kmarkdownview_SRCS
    data/webenginedata.qrc
)

configure_file(kmarkdownview-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/kmarkdownview-config.h)

add_library(KMarkdownWebView SHARED ${kmarkdownview_SRCS})
generate_export_header(KMarkdownWebView BASE_NAME kmarkdownview)

target_link_libraries(KMarkdownWebView Qt5::WebEngineWidgets)

install(TARGETS KMarkdownWebView ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
