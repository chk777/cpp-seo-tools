#!/usr/bin/env python3
"""
Save Googlebot visits (TSV from nginx_googlebot_parser) to SQLite.
Usage: nginx_googlebot_parser access.log | python save_googlebot_to_sqlite.py -d crawl.db
       python save_googlebot_to_sqlite.py -d crawl.db -i googlebot_visits.tsv
Requires: Python 3.7+, stdlib only (sqlite3).
"""

import argparse
import sqlite3
import sys
from pathlib import Path
from typing import List, Optional, Tuple


# Column order matches parser TSV output
COLUMNS = (
    "remote_addr",
    "time_local",
    "request",
    "status",
    "body_bytes_sent",
    "http_referer",
    "http_user_agent",
)


def create_schema(conn: sqlite3.Connection) -> None:
    """Create googlebot_visits table and indexes if not present."""
    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS googlebot_visits (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            remote_addr TEXT NOT NULL,
            time_local TEXT NOT NULL,
            request TEXT NOT NULL,
            status TEXT NOT NULL,
            body_bytes_sent TEXT NOT NULL,
            http_referer TEXT NOT NULL,
            http_user_agent TEXT NOT NULL,
            created_at TEXT DEFAULT (datetime('now'))
        )
        """
    )
    # Indexes for common Technical SEO queries
    conn.execute(
        "CREATE INDEX IF NOT EXISTS idx_googlebot_time ON googlebot_visits(time_local)"
    )
    conn.execute(
        "CREATE INDEX IF NOT EXISTS idx_googlebot_status ON googlebot_visits(status)"
    )
    conn.execute(
        "CREATE INDEX IF NOT EXISTS idx_googlebot_request ON googlebot_visits(request)"
    )
    conn.commit()


def parse_tsv_line(line: str) -> Optional[Tuple[str, ...]]:
    """Parse one TSV line (7 fields). Returns tuple or None on error."""
    line = line.rstrip("\n\r")
    if not line:
        return None
    parts = line.split("\t")
    if len(parts) != 7:
        return None
    return tuple(p.strip() for p in parts)


def read_tsv(stream) -> List[Tuple[str, ...]]:
    """Read TSV from stream; return list of row tuples."""
    rows = []
    for line in stream:
        row = parse_tsv_line(line)
        if row is not None:
            rows.append(row)
    return rows


def insert_batch(conn: sqlite3.Connection, rows: List[Tuple[str, ...]]) -> int:
    """Insert rows into googlebot_visits. Returns count inserted."""
    if not rows:
        return 0
    conn.executemany(
        """
        INSERT INTO googlebot_visits
        (remote_addr, time_local, request, status, body_bytes_sent, http_referer, http_user_agent)
        VALUES (?, ?, ?, ?, ?, ?, ?)
        """,
        rows,
    )
    conn.commit()
    return len(rows)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Save Googlebot visits (TSV) to SQLite"
    )
    parser.add_argument(
        "-d",
        "--db",
        required=True,
        metavar="FILE",
        help="SQLite database path (created if missing)",
    )
    parser.add_argument(
        "-i",
        "--input",
        metavar="FILE",
        help="Input TSV file; default: stdin",
    )
    parser.add_argument(
        "-b",
        "--batch",
        type=int,
        default=5000,
        metavar="N",
        help="Insert batch size (default 5000)",
    )
    args = parser.parse_args()

    db_path = Path(args.db)
    conn = sqlite3.connect(str(db_path))
    create_schema(conn)

    if args.input:
        with open(args.input, "r", encoding="utf-8", errors="replace") as f:
            rows = read_tsv(f)
    else:
        rows = read_tsv(sys.stdin)

    total = 0
    for i in range(0, len(rows), args.batch):
        batch = rows[i : i + args.batch]
        total += insert_batch(conn, batch)

    conn.close()
    print(f"Inserted {total} Googlebot visits into {db_path}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
