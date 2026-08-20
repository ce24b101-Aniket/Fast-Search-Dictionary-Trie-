# FastSearch — Trie-Based Dictionary & Autocomplete Engine

A full-stack dictionary search application built with **C++20**, **SQLite**, **React**, **TypeScript**, and **Docker**.

FastSearch uses a **Trie (Prefix Tree)** as its in-memory search index to provide efficient prefix matching and autocomplete. It began as a console-based data-structures project and evolved into a containerized web application with a REST API, persistent storage, and a responsive frontend.

## Highlights

- Trie-based prefix search and autocomplete
- REST API built in C++
- Persistent SQLite dictionary and search-history storage
- React + TypeScript frontend
- Docker Compose setup for the complete application
- Layered backend architecture with clear separation of concerns
- Automated backend tests

## Project Showcase

### Search Interface

A clean interface for searching dictionary words and prefixes.

![FastSearch Search Interface](Deployable/docs/screenshots/search.png)

### Trie-Based Autocomplete

As the user enters a prefix, FastSearch traverses the Trie and returns matching suggestions.

![FastSearch Autocomplete Suggestions](Deployable/docs/screenshots/suggestions.png)

### Dictionary Management

Add, search for, and manage dictionary words through the administration interface.

![FastSearch Dictionary Management](Deployable/docs/screenshots/manage.png)

## Architecture

```text
React (Vite + TypeScript)
            |
            | HTTP
            v
REST API (C++ + cpp-httplib)
            |
            v
      SearchService
       /           \
      v             v
Trie Search Index   SQLite Repositories
                    ├── WordRepository
                    └── SearchHistoryRepository
```

SQLite is the persistent source of truth. On startup, the application loads the dictionary data from SQLite and rebuilds the in-memory Trie for fast search operations.

## Tech Stack

| Area | Technologies |
|---|---|
| Backend | C++20, CMake, cpp-httplib, nlohmann/json |
| Data structure | Trie / Prefix Tree |
| Database | SQLite |
| Frontend | React, TypeScript, Vite |
| Testing | GoogleTest |
| Containerization | Docker, Docker Compose, Nginx |

## Quick Start with Docker

### Prerequisites

- Docker Desktop
- Docker Compose

From the repository root, run:

```bash
cd Deployable
docker compose up --build
```

Once the containers start:

- Frontend: <http://localhost:5173>
- Backend API: <http://localhost:8080>
- Health check: <http://localhost:8080/api/health>

The health endpoint should return:

```json
{"status":"ok"}
```

To stop the application while keeping the database data:

```bash
docker compose down
```

To stop the application and reset the stored database:

```bash
docker compose down -v
```

> The SQLite database is stored in the named Docker volume `fastsearch_data`.

## Local Development

### Backend

From the `Deployable` directory:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/fastsearch_tests
./build/fastsearch_server fastsearch.db 8080
```

### Frontend

Open a second terminal:

```bash
cd Deployable/frontend
npm install
npm run dev
```

The frontend will be available at <http://localhost:5173>.

## Project Structure

```text
Fast-search project/
├── Deployable/
│   ├── CMakeLists.txt           # Backend build configuration
│   ├── Dockerfile               # Backend Docker image
│   ├── docker-compose.yml       # Backend + frontend + persistent volume
│   ├── src/
│   │   ├── core/                # Trie, TrieNode, Normalizer
│   │   ├── db/                  # SQLite database and repositories
│   │   ├── service/             # SearchService business logic
│   │   ├── api/                 # REST API routes
│   │   └── main.cpp             # Application startup
│   ├── tests/                   # GoogleTest suite
│   ├── frontend/                # React + TypeScript application
│   └── docs/screenshots/        # README screenshots
├── Fullstack/
├── Phase1/
└── Phase2/
```

## Concepts Demonstrated

- Trie / Prefix Tree implementation
- Prefix-based autocomplete and lookup
- C++20 application development
- REST API design
- SQLite persistence
- Repository and service-layer patterns
- React and TypeScript frontend development
- Docker-based local deployment
- Automated testing with GoogleTest
- Layered software architecture

## Future Improvements

- Continuous integration pipeline
- Search-performance benchmark suite
- Expanded API documentation
- Interview-oriented architecture notes
- Additional search ranking and filtering options

## Author

Built by **Aniket Raval** as a personal software engineering portfolio project.

FastSearch demonstrates the journey from implementing a core data structure to designing and building a complete, persistent, containerized full-stack application.
