# ISBN Book Scanner (Qt/C++/QML)

A Qt 6 desktop application that scans EAN-13 book barcodes, looks up book metadata, stores a local library, and synchronizes changes with the companion FastAPI server.

## Features

- The camera preview starts hidden; **Show Camera Preview** starts capture, and **Hide Camera Preview** stops it and reclaims the preview area. Barcode decoding uses Qt Multimedia and the bundled ZXing-C++ library.
- Enter a 10- or 13-digit ISBN manually in the scanner panel and select **Lookup** when scanning is unavailable.
- The top-right cogwheel menu contains account and sync actions.
- Camera and manual ISBN lookups go through the authenticated sync server; provider credentials stay on the server.
- Import and export the library as CSV from the cogwheel menu.
- Local SQLite storage for server-returned book details, cover URLs, and a persistent sync queue; returned cover images are cached locally.
- Search by title, author, or ISBN; inspect details and remove books.
- Register and log in to a sync account, upload books, download remote changes, and synchronize deletions.

## Requirements

- CMake 3.16 or newer.
- A C++20 compiler.
- Qt 6 with Core, Gui, Qml, Quick, QuickControls2, QuickDialogs2, Sql, Network, and Multimedia.
- Qt Creator with a configured desktop kit, or an equivalent CMake environment.

ZXing-C++ is included as a Git submodule under `3rdparty/zxing-cpp`, pinned to tag `v3.1.1`. Android builds also require the Android Qt kit and the `android_openssl` CMake integration referenced by `CMakeLists.txt`.

### Check out ZXing-C++

After cloning the Qt client repository, initialize its submodules:

```sh
git submodule update --init -- 3rdparty/zxing-cpp
```

To explicitly switch the ZXing checkout to the version used by this client:

```sh
git -C 3rdparty/zxing-cpp fetch --tags
git -C 3rdparty/zxing-cpp switch --detach v3.1.1
```

This scanner only reads barcodes, so ZXing writer support is disabled and its nested `zint` submodule is not needed.

## Build with Qt Creator

1. Open this project's `CMakeLists.txt` in Qt Creator.
2. Select a kit with the required Qt modules and compiler.
3. Configure and build the `ISBNBookScanner` target, then run it.

From a shell where Qt's CMake package is available:

```sh
cmake -S . -B build
cmake --build build --config Release
```

## Local data

The SQLite database is shared with the Python scanner and stores the book records returned by the sync server.

| Platform | Database path |
| --- | --- |
| Windows | `%APPDATA%\Bookshelf\ISBNBookScanner\bookshelf.db` |
| Linux default | `~/.local/share/Bookshelf/ISBNBookScanner/bookshelf.db` |
| Linux with `XDG_DATA_HOME` | `$XDG_DATA_HOME/Bookshelf/ISBNBookScanner/bookshelf.db` |

Both clients use the same schema:

| Table | Columns and behavior |
| --- | --- |
| `books` | `isbn`, `title`, `authors`, `cover_url`, `publication_date`, `publisher`, `page_count` |
| `sync_queue` | `isbn`, `action_type`; one pending action per ISBN |
| `sync_state` | `username`, `checkpoint` |

Cover images are cached in `QStandardPaths::CacheLocation/covers`.

| Platform | Cache path |
| --- | --- |
| Windows | `%LOCALAPPDATA%\Bookshelf\ISBNBookScanner\cache\covers` |
| Linux default | `~/.cache/Bookshelf/ISBNBookScanner/covers` |
| Linux with `XDG_CACHE_HOME` | `$XDG_CACHE_HOME/Bookshelf/ISBNBookScanner/covers` |

Each cache filename is the SHA-256 hash of its ISBN with an `.img` extension. The cache can be deleted at any time; images are downloaded again from their stored URLs.

The updated schema does not migrate older client databases. Delete the old client database before running either app.

## Synchronization

Start the companion `poc-vc-py-bookshelf-sync-server` using its README instructions. ISBN lookup requires an authenticated server connection.

### Server URL and security

Register and Log In prompt for the server URL as well as account credentials. The URL is saved in system settings and shared with the Python app. `BOOKSHELF_SYNC_URL` overrides the saved URL/default (`http://127.0.0.1:8000`). Use an `http://` or `https://` URL.

HTTPS requires a Qt TLS backend and a certificate trusted by the system. Certificate checks remain enabled, and redirects cannot downgrade HTTPS to HTTP. The Compose server itself uses HTTP, so HTTPS requires a TLS-terminating proxy or HTTPS-enabled hosting in front of it.

### Sync behavior

1. Registration creates an account and signs in automatically. Login performs a differential sync.
2. On the first sync for an account, remote records are downloaded and local books missing from the server are uploaded.
3. Later syncs exchange updates and deletions. Local pending actions take precedence over conflicting remote updates.
4. Choose **Sync Now** to flush queued local actions and pull remote updates.
5. Choose **Log Out of Sync** to clear the in-memory authentication token.

The companion server uses a development JWT secret and should not be exposed beyond a trusted environment without secure configuration.

## CSV Import and Export

Use **Import CSV...** and **Export CSV...** in the cogwheel menu. CSV files must have the exact headers `ISBN`, `Title`, `Author`, `Cover URL`, `First Publication Date`, `Publisher`, and `Page Count`, in that order. Imported books are queued for synchronization.
