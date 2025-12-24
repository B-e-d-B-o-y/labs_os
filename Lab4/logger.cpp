#include "serial.h"
#include "utils.h"
#include <iostream>
#include <vector>
#include <ctime>
#include <string>
#include <thread>
#include <chrono>
#include <fstream>

#ifdef _WIN32
#include <windows.h>  
#endif

const std::string MEASUREMENTS_FILE = "measurements.log";
const std::string HOURLY_FILE = "hourly_average.log";
const std::string DAILY_FILE = "daily_average.log";

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

void writeFilteredLog(const std::string& filename, const std::vector<std::pair<time_t, float>>& all, time_t now, time_t maxAgeSec) {
    std::vector<std::pair<time_t, float>> valid;
    for (const auto& [ts, val] : all) {
        if (now - ts <= maxAgeSec) {
            valid.emplace_back(ts, val);
        }
    }

    std::ofstream out(filename, std::ios::trunc);
    for (const auto& [ts, val] : valid) {
        out << ts << " " << val << "\n";
    }
}

void writeYearFilteredLog(const std::string& filename, const std::vector<std::pair<time_t, float>>& all, int currentYear) {
    std::vector<std::pair<time_t, float>> valid;
    for (const auto& [ts, val] : all) {
        std::tm* tm = std::localtime(&ts);
        int year = tm->tm_year + 1900;
        if (year == currentYear) {
            valid.emplace_back(ts, val);
        }
    }

    std::ofstream out(filename, std::ios::trunc);
    for (const auto& [ts, val] : valid) {
        out << ts << " " << val << "\n";
    }
}

void logMeasurement(float temp) {
    time_t now = getCurrentTime();
    auto all = readLogFile(MEASUREMENTS_FILE);
    writeFilteredLog(MEASUREMENTS_FILE, all, now, 24 * 3600);
    std::ofstream out(MEASUREMENTS_FILE, std::ios::app);
    out << now << " " << temp << "\n";
}

void logHourlyAverage(float avg) {
    time_t now = getCurrentTime();
    auto all = readLogFile(HOURLY_FILE);
    writeFilteredLog(HOURLY_FILE, all, now, 30 * 24 * 3600);
    std::ofstream out(HOURLY_FILE, std::ios::app);
    out << now << " " << avg << "\n";
}

void logDailyAverage(float avg) {
    int year = getCurrentYear();
    auto all = readLogFile(DAILY_FILE);
    writeYearFilteredLog(DAILY_FILE, all, year);
    std::ofstream out(DAILY_FILE, std::ios::app);
    out << getCurrentTime() << " " << avg << "\n";
}

// === Вспомогательная функция: среднее за вчерашний день ===
float calculateYesterdayDailyAverage() {
    time_t now = getCurrentTime();
    std::tm* today = std::localtime(&now);
    int yday = today->tm_yday;
    int year = today->tm_year + 1900;


    auto hourlyData = readLogFile(HOURLY_FILE);
    std::vector<float> yesterdayTemps;

    for (const auto& [ts, temp] : hourlyData) {
        std::tm* tm = std::localtime(&ts);
        if (tm->tm_year + 1900 == year && tm->tm_yday == yday - 1) {
            yesterdayTemps.push_back(temp);
        }
    }

    if (yesterdayTemps.empty()) return 0.0f;

    float sum = 0.0f;
    for (float t : yesterdayTemps) sum += t;
    return sum / static_cast<float>(yesterdayTemps.size());
}

// === Основная логика ===
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); 
#endif
    const std::string PORT_NAME = 
#ifdef _WIN32
        "COM5";
#else
        "/dev/ttyS5"; 
#endif

    try {
        SerialPort port(PORT_NAME);
        std::cout << "Listening on " << PORT_NAME << "...\n";

        time_t lastHourCheck = getCurrentTime();
        time_t lastDayCheck = getCurrentTime();

        while (true) {
            std::string line = port.readLine(2000); // ждать до 2 сек
            if (line.empty()) continue;

            float temp;
            if (!validatePacket(line, temp)) {
                std::cerr << "Invalid data: " << line << "\n";
                continue;
            }

            std::cout << "Received: " << temp << "°C\n";
            logMeasurement(temp);

            // === Проверка на смену часа ===
            time_t now = getCurrentTime();
            std::tm* tm_now = std::localtime(&now);
            std::tm* tm_last = std::localtime(&lastHourCheck);

            if (tm_now->tm_hour != tm_last->tm_hour) {
                // Читаем все измерения за последний час
                auto allMeasurements = readLogFile(MEASUREMENTS_FILE);
                std::vector<float> lastHourTemps;
                time_t hourStart = now - 3600;

                for (const auto& [ts, t] : allMeasurements) {
                    if (ts >= hourStart) {
                        lastHourTemps.push_back(t);
                    }
                }

                if (!lastHourTemps.empty()) {
                    float sum = 0.0f;
                    for (float t : lastHourTemps) sum += t;
                    float hourlyAvg = sum / static_cast<float>(lastHourTemps.size());
                    logHourlyAverage(hourlyAvg);
                    std::cout << "[Hourly avg] " << hourlyAvg << "°C\n";
                }

                lastHourCheck = now;
            }

            // === Проверка на смену дня (в полночь) ===
            std::tm* tm_day_last = std::localtime(&lastDayCheck);
            if (tm_now->tm_yday != tm_day_last->tm_yday) {
                // Рассчитываем среднее за вчерашний день
                float dailyAvg = calculateYesterdayDailyAverage();
                if (dailyAvg != 0.0f) {
                    logDailyAverage(dailyAvg);
                    std::cout << "[Daily avg] " << dailyAvg << "°C\n";
                }
                lastDayCheck = now;
            }

            // Небольшая задержка, чтобы не грузить CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}