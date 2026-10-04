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
    Widgets
    Sql
    Network
    Multimedia
    MultimediaWidgets
)

# 2. Configure ZXing-C++ Subdirectory Flags (Scoped Dependencies)
# Disable optional components to accelerate build times
set(BUILD_WRAPPERS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_BLACKBOX_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)

# Force static linkage to keep your application deployment standalone
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(3rdparty/zxing-cpp)

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
    mainwindow.cpp
    mainwindow.h
    barcodescannerview.h
    barcodescannerview.cpp
    bookmetadataprovider.h
    bookmetadataprovider.cpp
    bookinfo.h
    covercache.h
    covercache.cpp
    abstractbookprovider.h
    openlibraryprovider.h
    openlibraryprovider.cpp
    googlebooksprovider.h
    googlebooksprovider.cpp
    bookdatabasemanager.h
    bookdatabasemanager.cpp
    bookshelfwidget.h
    bookshelfwidget.cpp
    bookdetailssidebar.h
    bookdetailssidebar.cpp
    booksyncmanager.h
    booksyncmanager.cpp
    README.md
)

# 4. Target Link Libraries & Platform Setup
target_link_libraries(ISBNBookScanner PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    ZXing::ZXing
)

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
