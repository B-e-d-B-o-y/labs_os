#include "serial.h"
#include "utils.h"
#include <iostream>
#include <vector>
#include <ctime>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>

const std::string MEASUREMENTS_FILE = "measurements.log";
const std::string HOURLY_FILE = "hourly_average.log";
const std::string DAILY_FILE = "daily_average.log";

// Глобальные переменные для усреднения и очистки
static std::vector<float> hourlyBuffer;
static time_t lastCleanup = 0;        // последняя очистка measurements.log
static time_t lastHourlyCleanup = 0;  // последняя очистка hourly_average.log
static time_t lastDailyCleanup = 0;   // последняя очистка daily_average.log

time_t getCurrentTime() {
    return std::time(nullptr);
}

int getCurrentYear() {
    std::time_t t = getCurrentTime();
    std::tm* tm = std::localtime(&t);
    return tm->tm_year + 1900;
}

std::vector<std::pair<time_t, float>> readLogFile(const std::string& filename) {
    std::vector<std::pair<time_t, float>> data;
    std::ifstream in(filename);
    if (!in.is_open()) return data;

    time_t ts;
    float val;
    while (in >> ts >> val) {
        data.emplace_back(ts, val);
    }
    return data;
}

void cleanupMeasurementsLog(time_t now) {
    auto all = readLogFile(MEASUREMENTS_FILE);
    std::vector<std::pair<time_t, float>> valid;
    for (const auto& [ts, val] : all) {
        if (now - ts <= 24 * 3600) {
            valid.emplace_back(ts, val);
        }
    }
    std::ofstream out(MEASUREMENTS_FILE, std::ios::trunc);
    for (const auto& [ts, val] : valid) {
        out << ts << " " << val << "\n";
    }
    lastCleanup = now;
}

void cleanupHourlyLog(time_t now) {
    auto all = readLogFile(HOURLY_FILE);
    std::vector<std::pair<time_t, float>> valid;
    for (const auto& [ts, val] : all) {
        if (now - ts <= 30 * 24 * 3600) {
            valid.emplace_back(ts, val);
        }
    }
    std::ofstream out(HOURLY_FILE, std::ios::trunc);
    for (const auto& [ts, val] : valid) {
        out << ts << " " << val << "\n";
    }
    lastHourlyCleanup = now;
}

void cleanupDailyLog(time_t now) {
    int year = getCurrentYear();
    auto all = readLogFile(DAILY_FILE);
    std::vector<std::pair<time_t, float>> valid;
    for (const auto& [ts, val] : all) {
        std::tm* tm = std::localtime(&ts);
        if (tm->tm_year + 1900 == year) {
            valid.emplace_back(ts, val);
        }
    }
    std::ofstream out(DAILY_FILE, std::ios::trunc);
    for (const auto& [ts, val] : valid) {
        out << ts << " " << val << "\n";
    }
    lastDailyCleanup = now;
}

// Основная запись измерения — БЕЗ полной перезаписи!
void logMeasurement(float temp) {
    time_t now = getCurrentTime();
    // Просто дописываем в конец
    std::ofstream out(MEASUREMENTS_FILE, std::ios::app);
    out << now << " " << temp << "\n";

    // Обрезаем раз в час (3600 сек)
    if (now - lastCleanup >= 3600 || lastCleanup == 0) {
        cleanupMeasurementsLog(now);
    }
}

void logHourlyAverage(float avg) {
    time_t now = getCurrentTime();
    std::ofstream out(HOURLY_FILE, std::ios::app);
    out << now << " " << avg << "\n";

    // Обрезаем раз в 6 часов (можно и реже)
    if (now - lastHourlyCleanup >= 6 * 3600 || lastHourlyCleanup == 0) {
        cleanupHourlyLog(now);
    }
}

void logDailyAverage(float avg) {
    time_t now = getCurrentTime();
    std::ofstream out(DAILY_FILE, std::ios::app);
    out << now << " " << avg << "\n";

    // Обрезаем раз в день
    if (now - lastDailyCleanup >= 24 * 3600 || lastDailyCleanup == 0) {
        cleanupDailyLog(now);
    }
}

static int totalMeasurements = 0;

void processTemperature(float temp) {
    time_t now = getCurrentTime();
    logMeasurement(temp);
    totalMeasurements++;

    hourlyBuffer.push_back(temp);

    if (hourlyBuffer.size() >= 60) {
        float sum = 0.0f;
        for (float t : hourlyBuffer) sum += t;
        float hourlyAvg = sum / static_cast<float>(hourlyBuffer.size());

        logHourlyAverage(hourlyAvg);
        std::cout << "[Hourly avg] " << hourlyAvg << " C\n";

        static int hourlyBlocks = 0;
        hourlyBlocks++;
        if (hourlyBlocks >= 24) {
            logDailyAverage(hourlyAvg);
            std::cout << "[Daily avg] " << hourlyAvg << " C\n";
            hourlyBlocks = 0;
        }

        hourlyBuffer.clear();
    }
}

int main() {
    const std::string PORT_NAME =
#ifdef _WIN32
        "COM5";
#else
        "/dev/pts1";
#endif

    try {
        SerialPort port(PORT_NAME);
        std::cout << "Listening on " << PORT_NAME << "...\n";

        while (true) {
            std::string line = port.readLine(2000);
            if (line.empty()) continue;

            float temp;
            if (!validatePacket(line, temp)) {
                std::cerr << "Invalid  " << line << "\n";
                continue;
            }

            std::cout << "Received: " << temp << " C\n";
            processTemperature(temp);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}