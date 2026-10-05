cmake_minimum_required(VERSION 3.16)

project(ISBNBookScanner VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic Qt code generation tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

# 1. Locate and Import Essential Qt 6 Submodules
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    QuickDialogs2
    Sql
    Network
    Multimedia
)

qt_policy(SET QTP0001 NEW)

# 2. Configure ZXing-C++ v3.1.1 options
set(ZXING_C_API OFF CACHE BOOL "" FORCE)
set(ZXING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZXING_EXAMPLES_QT OFF CACHE BOOL "" FORCE)
set(ZXING_BLACKBOX_TESTS OFF CACHE BOOL "" FORCE)
set(ZXING_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(ZXING_WRITERS OFF CACHE STRING "" FORCE)

# Force static linkage to keep your application deployment standalone
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(_app_automoc ${CMAKE_AUTOMOC})
set(CMAKE_AUTOMOC OFF)
add_subdirectory(3rdparty/zxing-cpp)
set(CMAKE_AUTOMOC ${_app_automoc})

# --- FIX: INJECT ANDROID OPENSSL TLS BACKEND BINARIES ---
if(ANDROID)
    # Automatically locate and fetch the pre-packaged Android OpenSSL scripts
    # from your local configured Android SDK directories
    include(${ANDROID_SDK_ROOT}/android_openssl/CMakeLists.txt)
endif()

# 3. Application Source Compilation Definition
# The MANUAL_FINALIZATION flag allows for flexible deployment asset injection if needed later
qt_add_executable(ISBNBookScanner MANUAL_FINALIZATION
    main.cpp
    bookapplicationcontroller.cpp
    bookapplicationcontroller.h
    barcodescannercontroller.h
    barcodescannercontroller.cpp
    bookmetadataprovider.h
    bookmetadataprovider.cpp
    bookinfo.h
    bookcsv.h
    bookcsv.cpp
    covercache.h
    covercache.cpp
    abstractbookprovider.h
    openlibraryprovider.h
    openlibraryprovider.cpp
    googlebooksprovider.h
    googlebooksprovider.cpp
    bookdatabasemanager.h
    bookdatabasemanager.cpp
    bookcollectionmodel.h
    bookcollectionmodel.cpp
    booksyncmanager.h
    booksyncmanager.cpp
    booksynccoordinator.h
    booksynccoordinator.cpp
    README.md
)

# 4. Target Link Libraries & Platform Setup
target_link_libraries(ISBNBookScanner PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickDialogs2
    Qt6::Sql
    Qt6::Network
    Qt6::Multimedia
    ZXing::ZXing
)

# 5. QML Module Registration
qt_add_qml_module(ISBNBookScanner
    URI ISBNBookScanner
    VERSION 1.0
    SOURCES
        scannerframeitem.h
        scannerframeitem.cpp
    QML_FILES
        MainView.qml
        BookshelfView.qml
        BookDetailsView.qml
        ScannerPreview.qml
        ScannerToggle.qml
        ScannerControls.qml
        SettingsMenu.qml
        AccountDialog.qml
)

if(ANDROID)
    qt_add_android_permission(ISBNBookScanner
        NAME "android.permission.NEARBY_WIFI_DEVICES"
        ATTRIBUTES usesPermissionFlags neverForLocation
    )

    qt_add_android_permission(ISBNBookScanner
        NAME "android.permission.ACCESS_LOCAL_NETWORK"
    )

    set_property(TARGET ISBNBookScanner PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/android")
endif()

# Platform-Specific Tailoring
if(WIN32)
    # Hides the terminal window on Windows platforms using MinGW/MSVC
    set_target_properties(ISBNBookScanner PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
    # Packages the binary inside a standard macOS application bundle directory structure
    set_target_properties(ISBNBookScanner PROPERTIES MACOSX_BUNDLE TRUE)
endif()

# Mandatory finalization macro required when using MANUAL_FINALIZATION with qt_add_executable
qt_finalize_executable(ISBNBookScanner)
