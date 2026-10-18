# Pulse (React frontend + C++ backend)

## Run backend (Windows)
Needs: CMake + MSVC C++ (Visual Studio Build Tools, "Desktop development with C++"). Internet needed once (downloads SQLite).
Open **x64 Native Tools Command Prompt for VS** (or "Developer PowerShell for VS"):

    cd backend-cpp
    build.bat
    build\Release\pulse_server.exe

Prints `API running on http://localhost:4000`. Data is stored in `data.sqlite` (delete it to reset).
Run the exe from inside `backend-cpp` so it finds `.env`.

If CMake is missing: `winget install Kitware.CMake`.
If the SQLite download fails: download sqlite-amalgamation zip from sqlite.org, and use `cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_SQLITE_SRC=<unzipped folder>`.

## Run backend (Linux/Mac)
    cd backend-cpp && cmake -S . -B build && cmake --build build && ./build/pulse_server

## Run frontend
    npm install
    npm run dev

## OOP map (backend-cpp/src)
- Abstraction: `Controller`, `BaseRepository`, `Notification`, `IMiddleware`, `IPasswordHasher` (pure virtual)
- Inheritance: 4 repositories, 6 controllers, 5 notification types, 2 middlewares
- Polymorphism: `registerRoutes()`, `getMessage()`, `remove()` (TweetRepository overrides base cascade), `IPasswordHasher`
- Encapsulation: private members + getters in `Tweet`, RAII `Database`
- Factory: `NotificationFactory::create`
- Dependency injection: constructors in `main.cpp` (composition root)

## Auth extras
- **Forgot password:** Sign in page > "Forgot password?". No email server, so the 6-digit code is shown on screen (demo) and printed in the server console. Set `SHOW_RESET_CODE=0` in `.env` to hide it on screen.
- **Guest login:** "Continue as guest" = read-only (browse feed/explore/profiles/threads only).
- Frontend uses 127.0.0.1 instead of localhost (faster on Windows) and lazy-loads pages; DB has indexes for speed.
