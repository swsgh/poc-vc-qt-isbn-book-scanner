# ISBN Book Scanner (Qt/C++)

A Qt 6 desktop application that scans EAN-13 book barcodes, looks up book metadata, stores a local library, and synchronizes changes with the companion FastAPI server.

## Features

- Camera capture starts with the application; barcode decoding uses Qt Multimedia and the bundled ZXing-C++ library.
- Book metadata lookup through the Open Library and Google Books providers.
- Local SQLite storage for book details, cover images, and a persistent sync queue.
- Search by title, author, or ISBN; inspect details and remove books.
- Register and log in to a sync account, upload books, download remote changes, and synchronize deletions.

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

The SQLite database (`scanned_books.db`) is stored in Qt's application data location. Pending uploads and deletes are recorded in its `sync_queue` table. Per-account differential sync checkpoints are stored locally as well.

Start the companion `poc-vc-py-bookshelf-sync-server` using its README instructions. The Qt client uses `http://127.0.0.1:8000` by default; set `BOOKSHELF_SYNC_URL` to use another server URL. Use the **Sync** menu to register or log in. Registration signs in automatically; login performs a differential sync, and **Sync now** flushes queued local actions and pulls remote updates. Authentication tokens are held in memory and are cleared when you log out or close the app.

On the first sync for an account, remote records are downloaded and local books missing from the server are uploaded. Local pending actions take precedence over conflicting remote updates. This is a proof of concept; the companion server uses a development JWT secret and should not be exposed beyond a trusted environment without secure configuration.
