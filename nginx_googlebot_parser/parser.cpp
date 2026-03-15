/**
 * nginx_googlebot_parser — high-performance Nginx access log parser (C++17).
 * Extracts Googlebot visits. STL only; zero-copy parsing via std::string_view; large read buffer.
 * Log format: Nginx combined ($remote_addr - $remote_user [$time_local] "$request" ... "$http_user_agent").
 * Usage: nginx_googlebot_parser [log_file...] — no args: read from stdin. Output: TSV, one row per visit.
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr std::size_t kReadBufferSize = 1024 * 1024;  // 1 MiB; larger = fewer syscalls
constexpr std::string_view kGooglebotMarker("Googlebot");

/** Parse one Nginx combined log line. All string_views refer into line; keep line valid. Returns true on success. */
bool ParseNginxCombinedLine(
    std::string_view line,
    std::string_view& remote_addr,
    std::string_view& time_local,
    std::string_view& request,
    std::string_view& status,
    std::string_view& body_bytes_sent,
    std::string_view& http_referer,
    std::string_view& http_user_agent)
{
    remote_addr = time_local = request = status = body_bytes_sent = http_referer = http_user_agent = {};

    // "IP - "
    std::size_t pos = 0;
    std::size_t idx = line.find(" - ");
    if (idx == std::string_view::npos) return false;
    remote_addr = line.substr(0, idx);
    pos = idx + 3;

    // " - [date] "
    idx = line.find(" [", pos);
    if (idx == std::string_view::npos) return false;
    pos = idx + 2;
    idx = line.find("] ", pos);
    if (idx == std::string_view::npos) return false;
    time_local = line.substr(pos, idx - pos);
    pos = idx + 2;

    // "\"request\" "
    if (pos >= line.size() || line[pos] != '"') return false;
    ++pos;
    idx = line.find("\" ", pos);
    if (idx == std::string_view::npos) return false;
    request = line.substr(pos, idx - pos);
    pos = idx + 2;

    // status and body_bytes_sent (up to next quote)
    std::size_t space = line.find(' ', pos);
    if (space == std::string_view::npos) return false;
    status = line.substr(pos, space - pos);
    pos = space + 1;
    idx = line.find(" \"", pos);
    if (idx == std::string_view::npos) return false;
    body_bytes_sent = line.substr(pos, idx - pos);
    pos = idx + 2;

    // referer and user_agent (strip surrounding quotes)
    idx = line.find("\" \"", pos);
    if (idx == std::string_view::npos) return false;
    if (pos < line.size() && line[pos] == '"') ++pos;
    http_referer = line.substr(pos, idx - pos);
    pos = idx + 3;
    if (pos < line.size() && line[pos] == '"') ++pos;
    idx = line.find('"', pos);
    if (idx == std::string_view::npos) return false;
    http_user_agent = line.substr(pos, idx - pos);
    return true;
}

/** Output field, replacing tab/newline/cr with space for safe TSV. */
inline void PrintTsvField(std::string_view field) {
    for (char c : field) {
        if (c == '\t' || c == '\n' || c == '\r') std::cout.put(' ');
        else std::cout.put(c);
    }
}

/** Emit one log line as TSV (tab-separated). */
void PrintTsvLine(
    std::string_view remote_addr,
    std::string_view time_local,
    std::string_view request,
    std::string_view status,
    std::string_view body_bytes_sent,
    std::string_view http_referer,
    std::string_view http_user_agent)
{
    PrintTsvField(remote_addr);
    std::cout.put('\t');
    PrintTsvField(time_local);
    std::cout.put('\t');
    PrintTsvField(request);
    std::cout.put('\t');
    PrintTsvField(status);
    std::cout.put('\t');
    PrintTsvField(body_bytes_sent);
    std::cout.put('\t');
    PrintTsvField(http_referer);
    std::cout.put('\t');
    PrintTsvField(http_user_agent);
    std::cout.put('\n');
}

/** True if User-Agent contains "Googlebot". */
inline bool IsGooglebot(std::string_view user_agent) {
    return user_agent.find(kGooglebotMarker) != std::string_view::npos;
}

/** Split buffer into lines, parse, emit only Googlebot rows. line_tail = incomplete line from previous chunk; next_tail = incomplete for next call. Avoid overwriting next_tail before reading line_tail (may alias). */
void ProcessBuffer(
    std::string_view chunk,
    std::string_view& line_tail,
    std::string& next_tail)
{
    std::string_view full;
    std::string combined;
    if (!line_tail.empty()) {
        combined.reserve(line_tail.size() + chunk.size());
        combined.assign(line_tail.data(), line_tail.size());
        combined.append(chunk.data(), chunk.size());
        full = combined;
    } else {
        full = chunk;
    }

    std::size_t start = 0;
    for (;;) {
        std::size_t end = full.find('\n', start);
        if (end == std::string_view::npos) {
            next_tail.assign(full.substr(start));
            line_tail = std::string_view(next_tail.data(), next_tail.size());
            return;
        }
        std::string_view line = full.substr(start, end - start);
        start = end + 1;
        if (line.empty()) continue;

        std::string_view remote_addr, time_local, request, status, body_bytes_sent, referer, user_agent;
        if (!ParseNginxCombinedLine(line, remote_addr, time_local, request, status, body_bytes_sent, referer, user_agent))
            continue;
        if (!IsGooglebot(user_agent)) continue;

        PrintTsvLine(remote_addr, time_local, request, status, body_bytes_sent, referer, user_agent);
    }
}

/** Process one file: large-buffer read, line-by-line parse. */
void ProcessFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "Error: cannot open " << path << "\n";
        return;
    }
    std::vector<char> buf(kReadBufferSize);
    std::string next_tail;
    std::string_view line_tail;

    while (in.read(buf.data(), buf.size()) || in.gcount() > 0) {
        std::size_t got = static_cast<std::size_t>(in.gcount());
        ProcessBuffer(std::string_view(buf.data(), got), line_tail, next_tail);
        if (!line_tail.empty())
            line_tail = std::string_view(next_tail.data(), next_tail.size());
    }
    if (!next_tail.empty()) {
        std::string_view last(next_tail.data(), next_tail.size());
        std::string_view dummy;
        std::string empty;
        ProcessBuffer(last, dummy, empty);
    }
}

/** Read from stdin. */
void ProcessStdin() {
    std::vector<char> buf(kReadBufferSize);
    std::string next_tail;
    std::string_view line_tail;

    while (std::cin.read(buf.data(), buf.size()) || std::cin.gcount() > 0) {
        std::size_t got = static_cast<std::size_t>(std::cin.gcount());
        ProcessBuffer(std::string_view(buf.data(), got), line_tail, next_tail);
        if (!line_tail.empty())
            line_tail = std::string_view(next_tail.data(), next_tail.size());
    }
    if (!next_tail.empty()) {
        std::string_view last(next_tail.data(), next_tail.size());
        std::string_view dummy;
        std::string empty;
        ProcessBuffer(last, dummy, empty);
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    if (argc < 2) {
        ProcessStdin();
        return 0;
    }
    for (int i = 1; i < argc; ++i)
        ProcessFile(argv[i]);
    return 0;
}
