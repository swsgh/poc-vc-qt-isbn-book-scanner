# 📚 PoC Vibe Coded ISBN Book Scanner (Qt/C++)

A modern, high-performance desktop client designed to capture, track, and catalog library data using an integrated barcode camera tracker. Built natively in **C++** using **Qt 6**, this cross-platform proof-of-concept features an advanced local data layer paired with real-time cloud synchronization functionality.

---

## 🎨 User Interface & Layout Realignment

The interface has been meticulously structured using responsive Qt layouts tailored for a sleek **Dark Mode** aesthetic (`#121212` canvas theme):

1. **Top Section:** Holds the `BarcodeScannerView` (constrained camera alignment area) stacked directly over a contextual, multi-state `m_isbnLabel` notice box.
2. **Bottom Section:** A horizontal split view containing:
   - **Bookshelf Grid Layer (Left, 3/4 layout split):** A vertical scroll wrapper rendering dynamic book card thumbnails dynamically. Text fields are removed to preserve a beautiful, high-density matrix format.
   - **Book Details Standalone Sidebar (Right, 1/4 layout split):** A decoupled sliding widget panel tracking full title headers, author metadata strings, and an explicit **Close Button (✕)** that collapses the container horizontally to let the shelf take over full width seamlessly.
3. **Utility Controls Bar (Absolute Bottom):** Anchors an interactive **Search Line Input (`QLineEdit`)** to filter the active shelf viewport alongside a custom neon amber **Offline Indicator Badge** that reveals itself dynamically when connection states drop out.

---

## ⚙️ Core Architecture & Component Map

The application completely strips out layout routing loops from `MainWindow` and isolates domain concerns into standalone code pairs:

- **`BarcodeScannerView` / `BookMetadataProvider`:** Direct loops handling physical camera hardware capturing streams to fetch ISBN metadata fields.
- **`BookDatabaseManager`:** Manages the native thread-safe local **SQLite** storage cache backend (`scanned_books.db`). 
- **`BookDetailsSidebar`:** Independent widget module encapsulating form field labels, hover/click styling sheets, a canvas placeholder drawing routine (`QPainter`), and the primary dangerous operation delete trigger.
- **`BookSyncManager`:** Communicates asynchronously with the backend over **`QNetworkAccessManager`**. It streams `QHttpMultiPart` data payloads (combining stringified text properties and raw binary cover image `QByteArray` fields) directly to a central cloud architecture.

---

## 🛡️ Offline-First & Resilient Queue Synchronization

The application is completely resilient against network drops. If internet availability cuts out, the desktop application remains **100% operational**:

- **Local Storage Continuity:** Books continue to scan, process metadata, save to disk, and populate the bookshelf layout grid offline.
- **SQLite Sync Queue Tracker:** Deletions and uploads are written to a physical local `sync_queue` database table instead of unstable volatile RAM configurations. 
- **Dynamic Connection Handshake Probe:** On startup, the sync client probes the network. If a timeout happens, the system activates the amber `m_offlineIndicatorLabel` warning banner and pauses background uploads safely.
- **Automated Backlog Flush Loop:** As soon as an authenticated session registers online, the client processes pending sync items sequentially (backlogged deletions fire *before* uploads to enforce data safety rules), resolving conflict metrics gracefully.

---

## 🚀 Compilation & Build Setup

### Prerequisites
- **Qt Creator 20+** (or modern VS Code layout workspace bindings)
- **Qt 6 SDK** with target kits installed
- A running server instance of the companion FastAPI Python backend repository

### Project Generation via CMake
Ensure your **`CMakeLists.txt`** links the critical networking libraries:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Sql)

target_link_libraries(ISBNBookScanner PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    Qt6::Network
    Qt6::Sql
)
```

### Build Instructions
1. Open the project root workspace folder inside **Qt Creator**.
2. Select your desktop compiler kit choice (e.g., *Desktop Qt 6.x.x LLVM MinGW 64-bit*).
3. Ensure your local Python sync backend server is running on `http://127.0.0.1:8000`.
4. Trigger **Run CMake**, then hit **Ctrl + R** to compile and launch the tracker frame layout instantly.

---

## 👥 AI Collaborative Coding Vibe Settings
This codebase is fully adapted for automated software engineering routines. To plug an AI agent or language model workspace client directly into this sandbox context:
- **VS Code Split Workflow:** Open this repository directory path in VS Code to run an AI file modification agent session, and keep **Qt Creator** active on your primary screen to trigger manual compilation builds.
- **Native ACP / MCP Setup:** Open `Preferences` ➔ `AI` ➔ `MCP Servers` in Qt Creator to open the Model Context Protocol bindings. This lets the coding agent automatically inspect CMake stack trace compilation logs or evaluate compiler configurations natively on your machine.
