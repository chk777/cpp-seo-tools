# Nginx Googlebot Parser + SQLite Pipeline

High-performance Technical SEO pipeline: C++17 Nginx access log parser that extracts Googlebot visits, and a Python script that stores them in SQLite.

## Features

- **Parser (C++17):** STL only, no external libraries. Zero-copy parsing with `std::string_view`; 1 MiB read buffer for fast disk I/O. Handles large logs (gigabytes).
- **Log format:** Nginx combined (`$remote_addr - $remote_user [$time_local] "$request" $status $body_bytes_sent "$http_referer" "$http_user_agent"`).
- **Filter:** Only lines whose User-Agent contains `Googlebot` are emitted.
- **Python:** Reads TSV from stdin or file; creates/updates SQLite with indexes for common SEO queries.

## Build (parser)

**Requirements:** C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+).

**CMake (recommended):**

```bash
cd nginx_googlebot_parser
mkdir build && cd build
cmake ..
cmake --build .
```

Binary: `build/nginx_googlebot_parser` (or `nginx_googlebot_parser.exe` on Windows).

**Manual (GCC/Clang):**

```bash
g++ -std=c++17 -O3 -Wall -o nginx_googlebot_parser parser.cpp
```

**Windows (MSVC):**

```bat
cl /EHsc /std:c++17 /O2 parser.cpp /Fe:nginx_googlebot_parser.exe
```

## Usage

**Parser**

- From file(s): `./nginx_googlebot_parser access.log` or multiple files.
- From stdin: `zcat access.log.gz | ./nginx_googlebot_parser`.

Output: one TSV row per Googlebot visit — `remote_addr`, `time_local`, `request`, `status`, `body_bytes_sent`, `http_referer`, `http_user_agent`.

**Save to SQLite**

- Pipe: `./nginx_googlebot_parser access.log | python save_googlebot_to_sqlite.py -d crawl.db`
- From file: `python save_googlebot_to_sqlite.py -d crawl.db -i googlebot_visits.tsv`

Options: `-d FILE` (required, DB path), `-i FILE` (input TSV; default stdin), `-b N` (batch size, default 5000).

**Example SQLite queries**

```sql
SELECT time_local, request, status FROM googlebot_visits ORDER BY time_local DESC LIMIT 100;
SELECT status, COUNT(*) FROM googlebot_visits GROUP BY status;
SELECT DISTINCT request FROM googlebot_visits WHERE status = '200';
```

## Repo layout

```
├── README.md
├── nginx_googlebot_parser/
│   ├── CMakeLists.txt
│   └── parser.cpp
└── save_googlebot_to_sqlite.py
```

## License

MIT.
