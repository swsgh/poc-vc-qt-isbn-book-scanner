# ISBN Book Scanner (Qt/C++)

A Qt 6 desktop application that scans EAN-13 book barcodes, looks up book metadata, stores a local library, and synchronizes changes with the companion FastAPI server.

## Features

- Camera capture starts with the application; barcode decoding uses Qt Multimedia and the bundled ZXing-C++ library.
- Book metadata lookup through the Open Library and Google Books providers.
- Local SQLite storage for book details, cover images, and a persistent sync queue.
- Search by title, author, or ISBN; inspect details and remove books.
- Authenticated upload, remote updates, and delete synchronization with the sync server.

## Requirements

- CMake 3.16 or newer.
- A C++20 compiler.
- Qt 6 with Core, Gui, Widgets, Sql, Network, Multimedia, MultimediaWidgets, and OpenGLWidgets.
- Qt Creator with a configured desktop kit, or an equivalent CMake environment.

ZXing-C++ is included under `3rdparty/zxing-cpp`. Android builds also require the Android Qt kit and the `android_openssl` CMake integration referenced by `CMakeLists.txt`.

## Build with Qt Creator

1. Open this project's `CMakeLists.txt` in Qt Creator.
2. Select a kit with the required Qt modules and compiler.
3. Configure and build the `ISBNBookScanner` target, then run it.

From a shell where Qt's CMake package is available:

```sh
cmake -S . -B build
cmake --build build --config Release
```

## Local data and synchronization

The SQLite database (`scanned_books.db`) is stored in Qt's application data location. Pending uploads and deletes are recorded in its `sync_queue` table so they can be retried after connectivity returns.

The client currently targets `http://127.0.0.1:8000`. Its startup login and server URL are hard-coded in `MainWindow::initializeApplication`; configure them for your local server and create a matching account through the server's registration endpoint before running the client. The companion server project is `poc-vc-py-bookshelf-sync-server`.

This is a proof of concept. Replace development credentials and use a properly secured server configuration before exposing either application beyond a local development environment.
