# ISBN Book Scanner (Qt/C++)

A Qt 6 desktop application that scans EAN-13 book barcodes, looks up book metadata, stores a local library, and synchronizes changes with the companion FastAPI server.

## Features

- The scanner starts hidden; **Show Camera** starts capture, and **Hide Camera** stops it and reclaims the preview area. Barcode decoding uses Qt Multimedia and the bundled ZXing-C++ library.
- Enter a 10- or 13-digit ISBN manually in the scanner panel and select **Lookup** when scanning is unavailable.
- The top-right cogwheel menu contains account and sync actions plus **Clear Library Database**, which removes local books and queues server deletions.
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

The SQLite database is shared with the Python scanner. On Windows its path is `%APPDATA%\Bookshelf\ISBNBookScanner\bookshelf.db`. On Linux it is `~/.local/share/Bookshelf/ISBNBookScanner/bookshelf.db`, or `$XDG_DATA_HOME/Bookshelf/ISBNBookScanner/bookshelf.db` when `XDG_DATA_HOME` is set. Both clients use `books(isbn, title, authors, engine_source, cover_blob)`, `sync_queue(isbn, action_type)` with one pending action per ISBN, and `sync_state(username, checkpoint)`.

Start the companion `poc-vc-py-bookshelf-sync-server` using its README instructions. Register and Log In prompt for the server URL as well as account credentials; the URL is saved in system settings and shared with the Python app. `BOOKSHELF_SYNC_URL` overrides the saved URL/default (`http://127.0.0.1:8000`). Use an `http://` or `https://` URL. HTTPS requires a Qt TLS backend and a certificate trusted by the system; certificate checks remain enabled, and redirects cannot downgrade HTTPS to HTTP. The Compose server itself uses HTTP, so HTTPS requires a TLS-terminating proxy or HTTPS-enabled hosting in front of it. Registration signs in automatically; login performs a differential sync, and **Sync now** flushes queued local actions and pulls remote updates. Authentication tokens are held in memory and are cleared when you log out or close the app.

On the first sync for an account, remote records are downloaded and local books missing from the server are uploaded. Local pending actions take precedence over conflicting remote updates. This is a proof of concept; the companion server uses a development JWT secret and should not be exposed beyond a trusted environment without secure configuration.
