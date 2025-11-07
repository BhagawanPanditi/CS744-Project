# CS744 Key-Value Store (Phase 1)

## Overview

A end-to-end **Key-Value Store** system built for **CS744: Design and Engineering of Computing Systems (Phase 1)**. The system provides a concurrent HTTP-based key-value interface with:

* **MySQL persistence layer** (via Docker)
* **In-memory LRU cache** for fast reads
* **Thread pool** for concurrent request handling
* **Client load generator** for benchmarking performance

---

## ⚙️ Setup

### 1. Clone the Repository

```bash
git clone https://github.com/<your-username>/CS744-Project.git
cd CS744-Project
```

### 2. Run Setup

```bash
./setup.sh
```

This script initializes the environment and sets up MySQL using Docker.

> 💡 If Docker is already installed and running, you can comment out the Docker-related commands in `setup.sh` before running it.

---

## 🏗️ Build Instructions

### Terminal 1 — Build the Server

```bash
cd server
make clean
make all
```

### Terminal 2 — Build the Client

```bash
cd client
make clean
make all
```

You will get two executables:

| Component   | Path               | Description           |
| ----------- | ------------------ | --------------------- |
| `kv_server` | `server/kv_server` | Key-Value HTTP server |
| `kv_client` | `client/kv_client` | Load generator client |

---

## 🚀 Running the System

### Start the Server

```bash
./kv_server <port> <cache_entries> <threads>
```

**Arguments:**

| Argument          | Description                                              | Default |
| ----------------- | -------------------------------------------------------- | ------- |
| `<port>`          | HTTP server port                                         | 8080    |
| `<cache_entries>` | Number of cache entries                                  | 1000    |
| `<threads>`       | Number of worker                                         | 8       |

### Run the Client (in another terminal)

```bash
./kv_client <workload_type> <load_level> <duration_in_sec>
```

**Arguments:**

| Argument            | Description                            |
| ------------------- | -------------------------------------- |
| `<workload_type>`   | Type of workload to run (see below)    |
| `<load_level>`      | Number of client threads (parallelism) |
| `<duration_in_sec>` | Duration for which to run the workload |

**Examples:**

```bash
./kv_client put_all_create 32 30
./kv_client get_all 64 60
./kv_client get_mix 16 45
```

**Workload Types:**

| Type             | Description                            |
| ---------------- | -------------------------------------- |
| `put_all_create` | Continuous key insertions              |
| `put_all_delete` | Continuous key deletions               |
| `get_all`        | Uniform reads from all keys            |
| `get_popular`    | Reads from a small set of popular keys |
| `get_mix`        | Mixed workload (PUT/GET/DELETE)        |

---

## 🧩 Architecture Overview

```
+--------------+        HTTP        +--------------+        SQL        +-------------+
|  kv_client   |  <-------------->  |  kv_server   |  <-------------->  |  MySQL DB   |
| (Load Gen)   |                    | (LRU Cache)  |                    | (Docker)    |
+--------------+                    +--------------+                    +-------------+
```

* **Cache:** In-memory LRU cache (reduces DB hits)
* **ThreadPool:** Manages parallel request execution
* **DB Layer:** Thread-safe MySQL connector for persistence. Each query opens a new MySQL connection to ensure thread safety

---

## 📊 Output Example at Client (Load Generator)

```
========== RESULTS ==========
Workload: get_all
Threads: 32
Duration: 30 s
Total Requests: 120000
Successful: 119800
Throughput: 3993 req/s
Avg Latency: 2.4 ms
=============================
```
