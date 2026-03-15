# 🚀 High-Performance SEO Log Infrastructure (C++ & Python Hybrid)

### 25 Years of Experience Meets Modern SEO Automation

I am a **Technical SEO Architect** and **Systems Engineer** with a background dating back to the FidoNet and early Rambler era. I don't just "do SEO"—I build the high-speed infrastructure required to analyze enterprise-level data where standard tools fail.

## 🛠 The Problem
Modern SEO crawlers and Python-only scripts often struggle with multi-gigabyte server logs (`access.log`). They consume too much RAM and take hours to process.

## ⚡ The Solution: Hybrid Engine
This project demonstrates a high-performance approach to **Crawl Budget Analysis**:
1.  **C++ Core**: A streaming parser that handles massive log files with minimal memory footprint, filtering bot traffic (Googlebot, Bingbot) at native speeds.
2.  **Python Wrapper**: An automation layer that takes the optimized stream and performs complex data manipulation.
3.  **SQL Integration**: Direct injection into **SQLite/PostgreSQL** for deep structural analysis of how search engines interact with your database-driven site.

## 📈 Key Features
- **Zero-Copy Parsing**: Optimized C++ logic for maximum I/O throughput.
- **Database-Centric**: Designed for SEOs who understand that search visibility starts at the DB schema level.
- **Scalable**: Ready for enterprise-level e-commerce platforms with millions of pages.

---

### About the Author
I've seen the web evolve from the first nodes to the AI era. My specialty is bridging the gap between **low-level systems programming** and **high-level search engine optimization**.

**Skills:** C/C++, Python, SQL/RDBMS Architecture, Technical SEO, AI Video Automation.

---
*Looking for a Technical SEO lead who understands the "guts" of the internet? Let's connect.*



## nginx_googlebot_parser, SQLite & SEO dashboard

A small toolkit for technical analysis of Googlebot visits in Nginx access logs.

- **`nginx_googlebot_parser` (C++17)**: high‑performance Nginx access log parser that filters lines with Googlebot and prints the result as TSV.
- **`save_googlebot_to_sqlite.py` (Python 3)**: imports the TSV output into a SQLite database (typically `seo_analysis.db`) for further analysis and reporting.
- **`dashboard.py` (Streamlit)**: dark‑themed SEO dashboard on top of `seo_analysis.db` with Googlebot trends, top sections, Googlebot vs real users comparison and fake‑bot detector (reverse DNS).
- **`generate_fake_access_log.py` / `fill_db.py`**: helpers to quickly generate synthetic logs or seed the database for demos and local testing.

### Requirements

- C++17‑capable compiler (MSVC, clang, gcc).
- CMake 3.10+.
- Python 3.7+.
- SQLite (uses the standard Python `sqlite3` module; no separate server is required).

### Windows installation and usage

#### 1. Environment setup

- Install **Visual Studio** with the “Desktop development with C++” workload, or **Build Tools for Visual Studio** (MSVC).
- Install **CMake 3.10+** and add `cmake.exe` to `PATH` (“Add CMake to system PATH” during installation).
- Install **Python 3.7+** with the “Add Python to PATH” option.

Verify versions in PowerShell:

```powershell
cmake --version
python --version
```

#### 2. Building `nginx_googlebot_parser` with CMake

```powershell
cd nginx_googlebot_parser
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

After the build completes, the executable will be located for example at:

- `nginx_googlebot_parser\build\Release\nginx_googlebot_parser.exe`

Add this path to `PATH` or call the utility using the full path.

#### 3. Building via `build.bat` (alternative)

If `build.bat` is configured in the project and the compiler environment is already activated (via “x64 Native Tools Command Prompt for VS” or a similar script):

```powershell
cd nginx_googlebot_parser
.\build.bat
```

#### 4. Preparing logs on Windows

- Copy Nginx access log files to the Windows machine (SCP, SFTP, network share, etc.).
- Make sure the log format matches **combined log** (see below).

### Input log format

- Expected format: **Nginx combined log**:
  - `$remote_addr - $remote_user [$time_local] "$request" ... "$http_user_agent"`.
- The parser keeps only lines where `User-Agent` contains the substring `Googlebot`.

### `nginx_googlebot_parser` output

- Googlebot hits are written in **TSV** (tab‑separated) format.
- Column order:
  1. `remote_addr`
  2. `time_local`
  3. `request`
  4. `status`
  5. `body_bytes_sent`
  6. `http_referer`
  7. `http_user_agent`

### Using `nginx_googlebot_parser`

Read logs from a single file (Unix‑like example):

```bash
nginx_googlebot_parser access.log > googlebot_visits.tsv
```

Read from multiple files:

```bash
nginx_googlebot_parser access.log.1 access.log.2 > googlebot_visits.tsv
```

Read from `stdin` (no arguments):

```bash
cat access.log | nginx_googlebot_parser > googlebot_visits.tsv
```

Equivalent commands for Windows PowerShell (assuming `nginx_googlebot_parser.exe` is on `PATH`):

```powershell
nginx_googlebot_parser.exe access.log > googlebot_visits.tsv
nginx_googlebot_parser.exe access.log.1 access.log.2 > googlebot_visits.tsv
Get-Content access.log | nginx_googlebot_parser.exe > googlebot_visits.tsv
```

### Loading data into SQLite: `save_googlebot_to_sqlite.py`

The script reads TSV produced by `nginx_googlebot_parser` (from `stdin` or from a pre‑saved file) and inserts rows into the `googlebot_visits` table in a SQLite database.

#### Creating/updating a crawl database and loading directly from the parser

```bash
nginx_googlebot_parser access.log | python save_googlebot_to_sqlite.py -d seo_analysis.db
```

- If `seo_analysis.db` does not exist, it will be created.
- If the `googlebot_visits` table is missing, it will be created with the following schema:
  - `id` — auto‑increment primary key.
  - `remote_addr`, `time_local`, `request`, `status`, `body_bytes_sent`, `http_referer`, `http_user_agent`.
  - `created_at` — row insertion time (UTC, via `datetime('now')`).
- Indexes are created on:
  - `time_local`
  - `status`
  - `request`

#### Loading from a prepared TSV file

```bash
python save_googlebot_to_sqlite.py -d seo_analysis.db -i googlebot_visits.tsv
```

#### Additional parameters

- `-b, --batch` — insert batch size (default: `5000` rows).

### Typical analysis in SQLite

Connect to the database:

```bash
sqlite3 seo_analysis.db
```

Example queries:

- Daily Googlebot visits:

```sql
SELECT substr(time_local, 1, 11) AS day, COUNT(*) AS visits
FROM googlebot_visits
GROUP BY day
ORDER BY day;
```

- Top pages by Googlebot visits:

```sql
SELECT request, COUNT(*) AS visits
FROM googlebot_visits
GROUP BY request
ORDER BY visits DESC
LIMIT 50;
```

### License

Specify your license here (for example, MIT, Apache 2.0, etc.).

### End‑to‑end example

```bash
# 1) On the Nginx server: copy logs to your workstation
scp user@server:/var/log/nginx/access.log ./access.log

# 2) On your workstation: extract Googlebot hits
nginx_googlebot_parser access.log > googlebot_visits.tsv

# 3) Load data into SQLite
python save_googlebot_to_sqlite.py -d seo_analysis.db -i googlebot_visits.tsv

# 4) Open SQLite shell and run queries
sqlite3 seo_analysis.db
sqlite> SELECT request, COUNT(*) AS visits
        FROM googlebot_visits
        ORDER BY visits DESC
        LIMIT 10;
```

### Synthetic test data

For quick local testing and demos you have two options.

#### Option 1: seed the database directly

```bash
python fill_db.py
streamlit run dashboard.py
```

This creates `seo_analysis.db` (if needed), populates `googlebot_visits` and a lightweight `bot_hits` table with 1000 synthetic rows, and starts the dashboard.

#### Option 2: generate a synthetic Nginx access.log

Generate a test log with 50,000 mixed requests (real Googlebot UAs, fake bots, regular users, 404s and 5xx):

```bash
python generate_fake_access_log.py --lines 50000 --output access.log
```

Then run the usual flow into `seo_analysis.db` and launch the dashboard:

```bash
nginx_googlebot_parser access.log > googlebot_visits.tsv
python save_googlebot_to_sqlite.py -d seo_analysis.db -i googlebot_visits.tsv
streamlit run dashboard.py
```

