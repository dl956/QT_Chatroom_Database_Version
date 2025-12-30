# Qt TCP Chat System

## UI Screenshots

![screenshot](screenshot.png)

## Overview

This project is a full-stack chat application featuring a C++ backend server and a Qt/QML-based cross-platform GUI frontend. It supports multiple users, public and private messaging, user registration/login, persistent message storage (MySQL), and real-time online user lists.

---

## Features

- **Multi-user registration and authentication**
- **Message persistence with MySQL database**
- **Online user management**
- **Supports public ("broadcast") and private messaging**
- **Cross-platform desktop GUI (Windows/Linux/macOS) via Qt/QML**
- **JSON-based protocol, length-prefixed over TCP**
- **Rich logging with log rotation**

---

## Tech Stack

### Backend

- **C++17**
- **Boost.Asio** (for high-performance asynchronous TCP server)
- **MySQL Connector/C++ X DevAPI** (for interacting with MySQL database using modern C++ styles)
- **nlohmann::json** (for JSON serialization/deserialization)
- **Standard Library concurrency** (`std::thread`, `std::mutex`, etc.)
- Custom logging module

### Frontend

- **Qt 5/6 (QML/QtQuick/QtQuick.Controls)**
- **C++/Qt (QObject bindings for QML)**
- **Custom QAbstractListModel** (for chat message model)
- **TCP socket communication** with backend server (via QTcpSocket)
- **QML for modern, fluid UI**

---

## Structure

```
- server/
    - main.cpp, server.hpp/cpp, session.hpp/cpp, logger.hpp/cpp, ...
    - db_pool.hpp, user_store.hpp/cpp, message_store.hpp/cpp, protocol.hpp
- client/
    - main.cpp (Qt entry)
    - tcp_client.h/cpp
    - message_model.h/cpp
    - main.qml (QML UI)
```

---

## How To Build

### Backend

1. **Requirements:**
    - C++17 compiler (GCC, clang, MSVC)
    - Boost
    - MySQL Connector/C++ 8 X DevAPI
    - nlohmann::json

2. **Compile (Example):**
```sh
mkdir build && cd build
cmake .. -A x64 -DCMAKE_TOOLCHAIN_FILE="vcpkg.cmake path"
cmake --build . --config Release
```

4. **Configure a MySQL database (see `chatdb` schema).**

---

### Frontend

1. **Requirements:**
    - Qt >= 5.15 (QtQuick, QtQuick.Controls, QtNetwork)
    - qmake or CMake

2. **Build:**
```sh
cd client mkdir build && cd build 
cmake .. -DCMAKE_PREFIX_PATH="your QT path
cmake --build . --config Release 
cd Release 
run windeployqt & 'windeployqt.exe path' --qmldir 'client path' .\qt_chat_client.exe
```
---

## Database Schema (Example)
```sh
docker run -d --name mymysql \
    -e MYSQL_ROOT_PASSWORD=mypassword \
    -p 3306:3306 \
    -p 33060:33060 \
    mysql:8.0


CREATE DATABASE chatdb DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;
USE chatdb;

CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(64) NOT NULL UNIQUE,
    password VARCHAR(128) NOT NULL
);

CREATE TABLE messages (
    id INT AUTO_INCREMENT PRIMARY KEY,
    sender VARCHAR(64) NOT NULL,
    recipient VARCHAR(64),
    text TEXT NOT NULL,
    ts BIGINT NOT NULL
);
```

---

## Launch

- **Start backend server:**  
  `./chat_server`  
  (listens on TCP port 9000 by default)

- **Start frontend client:**  
  Launch the Qt GUI executable.

---

## License

MIT License.  
(C) 2025 dl956

---
