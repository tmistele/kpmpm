# 3.1 is required for `target_sources`.
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(kpmpm VERSION 0.1)

set(QT_MIN_VERSION "5.10.0")
set(KF_MIN_VERSION "5.40.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# We append to the module path so modules can be overridden from the command line.
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# Allow adding Qt resource files with `add_executable` or `target_sources` instead of
# `qt5_add_resources`. See https://cmake.org/cmake/help/v3.0/manual/cmake-qt.7.html#autorcc.
set(CMAKE_AUTORCC ON)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(GenerateExportHeader)
include(FeatureSummary)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

include(ECMOptionalAddSubdirectory)
include(ECMAddAppIcon)
include(ECMInstallIcons)

# forbid some old things
add_definitions(-DQT_NO_FOREACH)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Widgets
)

find_package(KF5 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    I18n
    Parts
)

find_package(Qt5WebEngineWidgets "5.7.0" CONFIG REQUIRED)

# Qt 5.13 deprecated QComboBox::currentIndexChanged(QString) and Qt 5.14 undid that...
if (NOT Qt5Widgets_VERSION VERSION_LESS 5.14.0)
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050d00)
endif()


add_definitions(
    -DQT_DEPRECATED_WARNINGS
    -DQT_NO_SIGNALS_SLOTS_KEYWORDS
    -DQT_STRICT_ITERATORS
    -DQT_USE_QSTRINGBUILDER
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_URL_CAST_FROM_STRING
)

add_subdirectory(viewer)
add_subdirectory(kateaddon)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

# this needs at least ECM 5.64...
if(${ECM_VERSION} VERSION_GREATER "5.63.0")
    include(KDEClangFormat)

    # add clang-format target for all our real source files
    file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
    kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
endif()
