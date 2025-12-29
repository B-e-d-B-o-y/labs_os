#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstring>
#include <sqlite3.h>
#include <fstream>    // ← для чтения index.html
#include <map>        // ← для парсинга параметров
#include <algorithm>  // ← для std::find

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

#include "serial.h"
#include "utils.h"

const char* DB_PATH = "temperature.db";
const int HTTP_PORT = 8080;

// ========== DATABASE ==========

bool initDatabase() {
    sqlite3* db;
    int rc = sqlite3_open(DB_PATH, &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[DB] Cannot open database: " << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        return false;
    }

    const char* sql1 = R"(
        CREATE TABLE IF NOT EXISTS measurements (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            temperature REAL NOT NULL
        );
    )";

    const char* sql2 = R"(
        CREATE TABLE IF NOT EXISTS hourly_averages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            average REAL NOT NULL
        );
    )";

    const char* sql3 = R"(
        CREATE TABLE IF NOT EXISTS daily_averages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            average REAL NOT NULL
        );
    )";

    char* errMsg = nullptr;
    rc = sqlite3_exec(db, sql1, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[DB] Error creating measurements: " << errMsg << "\n";
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    rc = sqlite3_exec(db, sql2, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[DB] Error creating hourly_averages: " << errMsg << "\n";
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    rc = sqlite3_exec(db, sql3, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[DB] Error creating daily_averages: " << errMsg << "\n";
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    std::cout << "[DB] Database initialized\n";
    return true;
}

bool saveMeasurementToDB(float temp) {
    sqlite3* db;
    int rc = sqlite3_open(DB_PATH, &db);
    if (rc != SQLITE_OK) return false;

    time_t now = std::time(nullptr);
    const char* sql = "INSERT INTO measurements (timestamp, temperature) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }
    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(now));
    sqlite3_bind_double(stmt, 2, static_cast<double>(temp));
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return true;
}

std::string buildHistoryJSON(time_t start, time_t end) {
    sqlite3* db;
    sqlite3_open(DB_PATH, &db);

    std::vector<std::pair<time_t, float>> data;
    double sum = 0.0;
    int count = 0;

    const char* sql = "SELECT timestamp, temperature FROM measurements WHERE timestamp BETWEEN ? AND ? ORDER BY timestamp;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, start);
    sqlite3_bind_int64(stmt, 2, end);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        time_t ts = sqlite3_column_int64(stmt, 0);
        double temp = sqlite3_column_double(stmt, 1);
        data.emplace_back(ts, static_cast<float>(temp));
        sum += temp;
        count++;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::stringstream ss;
    ss << "{\n";
    ss << "  \"measurements\": [\n";
    for (size_t i = 0; i < data.size(); ++i) {
        ss << "    {\"value\":" << data[i].second << ",\"timestamp\":" << data[i].first << "}";
        if (i < data.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "  ],\n";
    ss << "  \"average\":" << (count > 0 ? sum / count : 0.0) << "\n";
    ss << "}";
    return ss.str();
}

std::string getLastMeasurementJSON() {
    sqlite3* db;
    sqlite3_open(DB_PATH, &db);

    const char* sql = "SELECT timestamp, temperature FROM measurements ORDER BY timestamp DESC LIMIT 1;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    time_t ts = 0;
    float temp = 0.0f;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ts = sqlite3_column_int64(stmt, 0);
        temp = static_cast<float>(sqlite3_column_double(stmt, 1));
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::stringstream ss;
    ss << "{\n"
       << "  \"value\":" << temp << ",\n"
       << "  \"timestamp\":" << ts << "\n"
       << "}";
    return ss.str();
}

// ========== FILE READER ==========

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

// ========== SERIAL THREAD ==========

static std::vector<float> hourlyBuffer;
static int totalMeasurements = 0;

void serialReaderThread() {
    const std::string PORT_NAME =
#ifdef _WIN32
        "COM5";
#else
        "/dev/ttyS5";
#endif

    try {
        SerialPort port(PORT_NAME);
        std::cout << "[Serial] Listening on " << PORT_NAME << "\n";

        while (true) {
            std::string line = port.readLine(2000);
            if (line.empty()) continue;

            float temp;
            if (!validatePacket(line, temp)) {
                std::cerr << "[Serial] Invalid  " << line << "\n";
                continue;
            }

            if (saveMeasurementToDB(temp)) {
                std::cout << "[DB] Saved: " << temp << " C\n";
            }

            totalMeasurements++;
            hourlyBuffer.push_back(temp);

            if (hourlyBuffer.size() >= 60) {
                float sum = 0.0f;
                for (float t : hourlyBuffer) sum += t;
                float avg = sum / static_cast<float>(hourlyBuffer.size());
                std::cout << "[Hourly avg] " << avg << " C\n";

                static int hourlyBlocks = 0;
                hourlyBlocks++;
                if (hourlyBlocks >= 24) {
                    std::cout << "[Daily avg] " << avg << " C\n";
                    hourlyBlocks = 0;
                }
                hourlyBuffer.clear();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Serial] Error: " << e.what() << "\n";
    }
}

// ========== URL & HTTP ==========

std::string urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            std::string hex = str.substr(i + 1, 2);
            result += static_cast<char>(std::stoi(hex, nullptr, 16));
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

void parseQuery(const std::string& query, std::map<std::string, std::string>& params) {
    size_t start = 0;
    while (start < query.length()) {
        size_t eq = query.find('=', start);
        size_t amp = query.find('&', start);
        if (eq == std::string::npos) break;
        std::string key = query.substr(start, eq - start);
        std::string val = (amp == std::string::npos) ? query.substr(eq + 1) : query.substr(eq + 1, amp - eq - 1);
        params[urlDecode(key)] = urlDecode(val);
        if (amp == std::string::npos) break;
        start = amp + 1;
    }
}

void handleRequest(SOCKET clientSocket, const std::string& request) {
    if (request.empty()) return;

    size_t endLine = request.find("\r\n");
    if (endLine == std::string::npos) return;

    std::string firstLine = request.substr(0, endLine);
    if (firstLine.find("GET ") != 0) {
        std::string response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        send(clientSocket, response.c_str(), response.length(), 0);
        return;
    }

    size_t pathStart = 4;
    size_t pathEnd = firstLine.find(' ', pathStart);
    if (pathEnd == std::string::npos) return;

    std::string pathAndQuery = firstLine.substr(pathStart, pathEnd - pathStart);
    size_t qPos = pathAndQuery.find('?');
    std::string path = qPos == std::string::npos ? pathAndQuery : pathAndQuery.substr(0, qPos);
    std::string query = qPos == std::string::npos ? "" : pathAndQuery.substr(qPos + 1);

    std::string response;
    std::string contentType = "text/plain";

    if (path == "/") {
        response = readFile("web/index.html");
        if (response.empty()) {
            response = "HTTP/1.1 404 Not Found\r\n\r\nFile web/index.html not found";
            send(clientSocket, response.c_str(), response.length(), 0);
            return;
        }
        contentType = "text/html";
    } else if (path == "/current") {
        response = getLastMeasurementJSON();
        contentType = "application/json";
    } else if (path == "/history") {
        std::map<std::string, std::string> params;
        parseQuery(query, params);
        if (params.count("start") && params.count("end")) {
            try {
                time_t start = std::stoll(params["start"]);
                time_t end = std::stoll(params["end"]);
                response = buildHistoryJSON(start, end);
                contentType = "application/json";
            } catch (...) {
                response = "HTTP/1.1 400 Bad Request\r\n\r\nInvalid timestamps";
                send(clientSocket, response.c_str(), response.length(), 0);
                return;
            }
        } else {
            response = "HTTP/1.1 400 Bad Request\r\n\r\nMissing start or end parameter";
            send(clientSocket, response.c_str(), response.length(), 0);
            return;
        }
    } else {
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
        send(clientSocket, response.c_str(), response.length(), 0);
        return;
    }

    std::string httpResponse =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + contentType + "\r\n"
        "Connection: close\r\n"
        "Content-Length: " + std::to_string(response.length()) + "\r\n"
        "\r\n" + response;

    send(clientSocket, httpResponse.c_str(), httpResponse.length(), 0);
}

void httpServerThread() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "[HTTP] Failed to create socket\n";
        return;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(HTTP_PORT);

    if (bind(serverSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[HTTP] Bind failed\n";
        closesocket(serverSocket);
        return;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cerr << "[HTTP] Listen failed\n";
        closesocket(serverSocket);
        return;
    }

    std::cout << "[HTTP] Server running on http://localhost:" << HTTP_PORT << "\n";

    while (true) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) continue;

        char buffer[4096];
        int bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            handleRequest(clientSocket, std::string(buffer));
        }
        closesocket(clientSocket);
    }

    closesocket(serverSocket);
#ifdef _WIN32
    WSACleanup();
#endif
}

// ========== MAIN ==========

int main() {
    if (!initDatabase()) {
        return 1;
    }

    std::thread serialThread(serialReaderThread);
    httpServerThread();

    serialThread.join();
    return 0;
}