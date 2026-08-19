# ⚡ fastsearch

> A Trie-backed autocomplete and dictionary management engine built with **C++**, **SQLite**, **React**, and **Docker**.

FastSearch is a full-stack dictionary and autocomplete application designed around a **Trie data structure** for efficient prefix-based search.

Type a prefix and FastSearch walks the Trie to provide matching words instantly, while the backend handles dictionary operations, search history, and persistent storage.

---

## 📸 Screenshots

### 🔎 Search Interface

The main search interface allows users to enter a word or prefix and explore the dictionary.

![FastSearch Search Interface](docs/screenshots/search.png)

---

### ⚡ Trie-Based Autocomplete

Type a prefix such as `con` and FastSearch traverses the Trie to return matching words.

![FastSearch Autocomplete Suggestions](docs/screenshots/suggestions.png)

---

### 🛠️ Dictionary Management

The management interface allows words to be added and existing words to be looked up or managed.

![FastSearch Dictionary Management](docs/screenshots/manage.png)

---

## ✨ Features

- ⚡ **Trie-based autocomplete** for fast prefix searching
- 🔎 **Prefix traversal** with visual feedback
- 📚 **Dictionary management** through the web interface
- ➕ Add new words to the dictionary
- 🔍 Look up existing words
- 💾 **SQLite-based persistent storage**
- 📊 Search and dictionary analytics
- 🌐 REST API powered by the C++ backend
- 🖥️ React-based frontend
- 🚀 Production frontend served using Nginx
- 🐳 Fully containerized using Docker
- 🔄 Docker Compose orchestration for frontend and backend
- 💽 Persistent Docker volume for dictionary data
- 🧩 Modular C++ architecture using CMake

---

## 🏗️ Architecture

```text
                        ┌───────────────────────┐
                        │       Browser         │
                        │   React Web Interface │
                        └───────────┬───────────┘
                                    │
                                    │ HTTP
                                    ▼
                        ┌───────────────────────┐
                        │       Frontend        │
                        │    React + Vite       │
                        │        Nginx          │
                        └───────────┬───────────┘
                                    │
                                    │ REST API
                                    ▼
                  ┌─────────────────────────────────┐
                  │          C++ Backend            │
                  │                                 │
                  │  API → Service → Trie / DB     │
                  └───────────────┬─────────────────┘
                                  │
                    ┌─────────────┴─────────────┐
                    │                           │
                    ▼                           ▼
             ┌──────────────┐           ┌──────────────┐
             │     Trie     │           │    SQLite    │
             │              │           │   Database   │
             │ Prefix Search│           │              │
             └──────────────┘           └──────────────┘
```

---

## 🧠 Core Idea — Trie-Based Autocomplete

The core search engine uses a **Trie**, also known as a prefix tree.

Instead of scanning every dictionary word for every query, the search process follows the characters of the supplied prefix through the Trie.

For example:

```text
Prefix: con

        root
          │
          c
          │
          o
          │
          n
       ┌──┼──┐
       │  │  │
    concrete
    concreting
    conduit
```

Once the node representing `con` is reached, the engine can explore the subtree below that node to find matching words.

This makes the data structure particularly suitable for **autocomplete and prefix-based search**.

---

## 🔄 Search Flow

```text
User enters prefix
        │
        ▼
Frontend sends API request
        │
        ▼
C++ REST API
        │
        ▼
Search Service
        │
        ▼
Trie traversal
        │
        ▼
Matching words
        │
        ▼
Frontend displays suggestions
```

---

## 🧩 Project Structure

```text
Deployable/
│
├── Dockerfile
├── docker-compose.yml
├── CMakeLists.txt
├── README.md
│
├── db/
│   └── ...
│
├── docs/
│   ├── ...
│   └── screenshots/
│       ├── search.png
│       ├── suggestions.png
│       └── manage.png
│
├── frontend/
│   ├── ...
│   ├── package.json
│   └── ...
│
├── src/
│   ├── api/
│   ├── core/
│   ├── db/
│   ├── service/
│   └── main.cpp
│
├── tests/
│   └── ...
│
└── third_party/
    └── ...
```

### Backend modules

The C++ backend is organized into separate components:

- **Core** — Trie and text normalization logic
- **Database** — SQLite database management and repositories
- **Service** — Search and application logic
- **API** — HTTP/API layer
- **Main** — Backend application entry point

---

## 🛠️ Tech Stack

### Backend

- **C++**
- **CMake**
- **SQLite**
- REST API
- Trie data structure

### Frontend

- **React**
- **TypeScript**
- **Vite**
- **Nginx**

### DevOps / Deployment

- **Docker**
- **Docker Compose**
- **WSL 2**
- Persistent Docker volumes

---

## 🚀 Running the Project Locally

### Prerequisites

Make sure you have:

- Docker Desktop
- WSL 2 on Windows
- Git

Docker Desktop must be running before starting the application.

---

### 1. Clone the repository

```bash
git clone <your-repository-url>
cd <repository-directory>/Deployable
```

---

### 2. Build and start the application

From the `Deployable` directory:

```bash
docker compose up --build
```

The first build may take a few minutes because Docker needs to build both the C++ backend and frontend images.

---

### 3. Open the application

Once the containers are running, open:

```text
http://localhost:5173
```

The FastSearch web interface should appear.

---

### 4. Check the backend

The backend health endpoint is available at:

```text
http://localhost:8080/api/health
```

A successful response should indicate that the backend is healthy.

---

## 🐳 Docker Services

The application is divided into two primary containers:

```text
┌──────────────────────────┐
│   fastsearch-frontend    │
│                          │
│ React + Vite + Nginx     │
│        :5173             │
└────────────┬─────────────┘
             │
             │ API requests
             ▼
┌──────────────────────────┐
│    fastsearch-backend    │
│                          │
│ C++ REST API             │
│        :8080             │
└────────────┬─────────────┘
             │
             ▼
┌──────────────────────────┐
│     SQLite Database      │
│                          │
│ Persistent Docker Volume │
└──────────────────────────┘
```

---

## 💾 Data Persistence

FastSearch uses a named Docker volume for persistent application data.

This means dictionary data can survive container restarts.

To stop the application:

```bash
Ctrl + C
```

Then:

```bash
docker compose down
```

Start it again with:

```bash
docker compose up
```

The persistent data remains available because the named volume is preserved.

> **Important:** Avoid `docker compose down -v` if you want to preserve the database volume.

---

## 🔌 API

The backend exposes HTTP endpoints used by the frontend for dictionary and search operations.

The health endpoint can be used to verify that the backend is running:

```text
GET /api/health
```

Base backend URL:

```text
http://localhost:8080
```

The frontend communicates with the backend through the configured API endpoints.

---

## 📖 Example

Suppose the dictionary contains:

```text
concrete
concreting
conduit
```

The user enters:

```text
con
```

The Trie traversal follows:

```text
root
  ↓
 c
  ↓
 o
  ↓
 n
```

The application then returns matching words:

```text
concrete
concreting
conduit
```

The suggestions are displayed directly in the frontend.

---

## ⚙️ Building the Backend Manually

The backend uses CMake.

A typical build process is:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The Docker build performs the backend compilation automatically.

---

## 🧪 Testing

The repository contains a `tests/` directory for project tests.

The Docker production build disables the test target when building the server image:

```text
- DFASTSEARCH_BUILD_TESTS=OFF
```

Tests can be built separately when working on the project locally.

---

## 📊 Analytics

FastSearch also includes an **Analytics** section in the web interface for inspecting application/search-related information.

The application therefore provides three main user-facing areas:

```text
Search
Manage
Analytics
```

---

## 🔐 Design Goals

FastSearch was designed with the following goals:

- Efficient prefix-based searching
- Clean separation between frontend and backend
- Persistent dictionary storage
- Modular C++ architecture
- Containerized deployment
- Easy local setup
- Extensible API architecture

---

## 🚀 Future Improvements

Possible future enhancements include:

- [ ] Ranking suggestions by popularity or frequency
- [ ] Fuzzy / typo-tolerant search
- [ ] Improved search analytics
- [ ] Authentication and user-specific dictionaries
- [ ] Larger dictionary datasets
- [ ] More comprehensive automated tests
- [ ] API documentation
- [ ] Performance benchmarking for large Trie datasets
- [ ] CI/CD pipeline
- [ ] Cloud deployment

---

## 🎯 What This Project Demonstrates

This project brings together several important software engineering concepts:

### Data Structures

- Trie
- Tree traversal
- Prefix searching

### Backend Engineering

- C++
- Modular architecture
- REST APIs
- Service/repository separation

### Database

- SQLite
- Persistent storage
- Repository pattern

### Frontend

- React
- TypeScript
- Vite
- Responsive application interface

### DevOps

- Docker
- Docker Compose
- Multi-stage Docker builds
- Nginx
- Persistent volumes
- WSL 2

---

## 👨‍💻 Project

**fastsearch** — a full-stack Trie-powered autocomplete and dictionary management system.

Built to explore efficient data structures, backend architecture, persistent storage, and containerized application deployment.

---

## Author

Aniket Raval
B.Tech Civil Engineering, IIT Madras
