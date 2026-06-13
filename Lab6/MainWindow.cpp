#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTextEdit>
#include <QHeaderView>
#include <QDateTime>
#include <QMessageBox>
#include <QTabWidget>
#include <QStatusBar>
#include <QDateTimeEdit>
#include <QApplication>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), httpClient(new HttpClient(this)) {
    
    setupUI();
    setupConnections();
    loadInitialData();
    
    // Настраиваем таймеры
    autoRefreshTimer = new QTimer(this);
    autoRefreshTimer->setInterval(autoRefreshInterval);
    
    clockTimer = new QTimer(this);
    clockTimer->start(1000); // Обновлять каждую секунду
    
    // Статус бар
    statusBar = new QStatusBar(this);
    statusLabel = new QLabel("Готово", statusBar);
    connectionLabel = new QLabel("Подключение: ❌", statusBar);
    statusBar->addPermanentWidget(connectionLabel);
    statusBar->addWidget(statusLabel);
    setStatusBar(statusBar);
}

MainWindow::~MainWindow() {
    if (autoRefreshTimer) autoRefreshTimer->stop();
    if (clockTimer) clockTimer->stop();
}

void MainWindow::setupUI() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    tabWidget = new QTabWidget(centralWidget);
    
    // === ВКЛАДКА 1: Текущая температура ===
    currentTab = new QWidget();
    QVBoxLayout *currentLayout = new QVBoxLayout(currentTab);
    
    // Заголовок
    QLabel *currentTitle = new QLabel("Текущая температура", currentTab);
    currentTitle->setStyleSheet("font-size: 16px; font-weight: bold; margin: 10px;");
    currentLayout->addWidget(currentTitle);
    
    // Информация о текущей температуре
    QGroupBox *currentInfoBox = new QGroupBox("Информация", currentTab);
    QGridLayout *infoLayout = new QGridLayout(currentInfoBox);
    
    currentTempLabel = new QLabel("-- °C", currentInfoBox);
    currentTempLabel->setStyleSheet("font-size: 36px; font-weight: bold; color: #0066cc;");
    
    currentTimeLabel = new QLabel("Время: --", currentInfoBox);
    currentTimeLabel->setStyleSheet("font-size: 14px;");
    
    lastUpdateLabel = new QLabel("Последнее обновление: --", currentInfoBox);
    lastUpdateLabel->setStyleSheet("font-size: 12px; color: #666;");
    
    infoLayout->addWidget(currentTempLabel, 0, 0, 1, 2);
    infoLayout->addWidget(currentTimeLabel, 1, 0);
    infoLayout->addWidget(lastUpdateLabel, 1, 1);
    currentInfoBox->setLayout(infoLayout);
    currentLayout->addWidget(currentInfoBox);
    
    // График последних значений
    QGroupBox *chartBox = new QGroupBox("Последние 60 значений", currentTab);
    QVBoxLayout *chartLayout = new QVBoxLayout(chartBox);
    currentChart = new ChartWidget(chartBox);
    currentChart->setMinimumHeight(300);
    chartLayout->addWidget(currentChart);
    currentLayout->addWidget(chartBox);
    
    // === ВКЛАДКА 2: История ===
    historyTab = new QWidget();
    QVBoxLayout *historyLayout = new QVBoxLayout(historyTab);
    
    // Панель управления
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    
    QLabel *periodLabel = new QLabel("Период:", historyTab);
    periodCombo = new QComboBox(historyTab);
    periodCombo->addItems({"Последний час", "Последние 24 часа", "Последняя неделя", "Произвольный период"});
    
    refreshButton = new QPushButton("Обновить", historyTab);
    refreshButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_BrowserReload));
    
    autoRefreshCheck = new QCheckBox("Автообновление (30 сек)", historyTab);
    autoRefreshCheck->setChecked(true);
    
    controlsLayout->addWidget(periodLabel);
    controlsLayout->addWidget(periodCombo);
    controlsLayout->addStretch();
    controlsLayout->addWidget(autoRefreshCheck);
    controlsLayout->addWidget(refreshButton);
    
    historyLayout->addLayout(controlsLayout);
    
    // Статистика
    statsLabel = new QLabel("Средняя температура: --°C | Измерений: --", historyTab);
    statsLabel->setStyleSheet("font-weight: bold; padding: 5px; background: #f0f0f0;");
    historyLayout->addWidget(statsLabel);
    
    // График истории
    QGroupBox *historyChartBox = new QGroupBox("График температуры", historyTab);
    QVBoxLayout *historyChartLayout = new QVBoxLayout(historyChartBox);
    historyChart = new ChartWidget(historyChartBox);
    historyChart->setMinimumHeight(400);
    historyChartLayout->addWidget(historyChart);
    historyLayout->addWidget(historyChartBox);
    
    // === ВКЛАДКА 3: Статистика ===
    statsTab = new QWidget();
    QVBoxLayout *statsLayout = new QVBoxLayout(statsTab);
    
    QHBoxLayout *statsControlsLayout = new QHBoxLayout();
    QLabel *statsPeriodLabel = new QLabel("Статистика за:", statsTab);
    statsPeriodCombo = new QComboBox(statsTab);
    statsPeriodCombo->addItems({"Последний час", "Последние 6 часов", "Последние 24 часа", "Последняя неделя"});
    
    statsControlsLayout->addWidget(statsPeriodLabel);
    statsControlsLayout->addWidget(statsPeriodCombo);
    statsControlsLayout->addStretch();
    statsLayout->addLayout(statsControlsLayout);
    
    // Таблица и график в splitter
    QSplitter *statsSplitter = new QSplitter(Qt::Horizontal, statsTab);
    
    // Таблица статистики
    QWidget *tableWidget = new QWidget(statsSplitter);
    QVBoxLayout *tableLayout = new QVBoxLayout(tableWidget);
    QLabel *tableTitle = new QLabel("Таблица средних значений", tableWidget);
    tableTitle->setStyleSheet("font-weight: bold;");
    
    statsTable = new QTableWidget(tableWidget);
    statsTable->setColumnCount(4);
    statsTable->setHorizontalHeaderLabels({"Время", "Среднее", "Мин.", "Макс."});
    statsTable->horizontalHeader()->setStretchLastSection(true);
    statsTable->setAlternatingRowColors(true);
    
    tableLayout->addWidget(tableTitle);
    tableLayout->addWidget(statsTable);
    
    // График статистики
    QWidget *chartWidget = new QWidget(statsSplitter);
    QVBoxLayout *chartWidgetLayout = new QVBoxLayout(chartWidget);
    QLabel *chartTitle = new QLabel("График статистики", chartWidget);
    chartTitle->setStyleSheet("font-weight: bold;");
    
    statsChart = new ChartWidget(chartWidget);
    statsChart->setMinimumHeight(300);
    
    chartWidgetLayout->addWidget(chartTitle);
    chartWidgetLayout->addWidget(statsChart);
    
    statsSplitter->addWidget(tableWidget);
    statsSplitter->addWidget(chartWidget);
    statsSplitter->setSizes({300, 500});
    statsLayout->addWidget(statsSplitter);
    
    // === ВКЛАДКА 4: Логи ===
    logsTab = new QWidget();
    QVBoxLayout *logsLayout = new QVBoxLayout(logsTab);
    
    // Панель управления логами
    QHBoxLayout *logsControlsLayout = new QHBoxLayout();
    
    QLabel *logLevelLabel = new QLabel("Уровень логов:", logsTab);
    logLevelCombo = new QComboBox(logsTab);
    logLevelCombo->addItems({"Все", "Ошибки", "Предупреждения", "Информация"});
    
    QLabel *logCountLabel = new QLabel("Количество:", logsTab);
    logCountSpin = new QSpinBox(logsTab);
    logCountSpin->setRange(10, 1000);
    logCountSpin->setValue(100);
    logCountSpin->setSingleStep(50);
    
    clearLogsButton = new QPushButton("Очистить", logsTab);
    
    logsControlsLayout->addWidget(logLevelLabel);
    logsControlsLayout->addWidget(logLevelCombo);
    logsControlsLayout->addWidget(logCountLabel);
    logsControlsLayout->addWidget(logCountSpin);
    logsControlsLayout->addStretch();
    logsControlsLayout->addWidget(clearLogsButton);
    logsLayout->addLayout(logsControlsLayout);
    
    // Текстовое поле для логов
    logTextEdit = new QTextEdit(logsTab);
    logTextEdit->setReadOnly(true);
    logTextEdit->setFont(QFont("Courier New", 10));
    logsLayout->addWidget(logTextEdit);
    
    // Добавляем все вкладки
    tabWidget->addTab(currentTab, "Текущая");
    tabWidget->addTab(historyTab, "История");
    tabWidget->addTab(statsTab, "Статистика");
    tabWidget->addTab(logsTab, "Логи");
    
    // Главный layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(tabWidget);
    centralWidget->setLayout(mainLayout);
}

void MainWindow::setupConnections() {
    // Кнопки
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(autoRefreshCheck, &QCheckBox::toggled, this, &MainWindow::onAutoRefreshToggled);
    connect(clearLogsButton, &QPushButton::clicked, logTextEdit, &QTextEdit::clear);
    
    // Комбобоксы
    connect(periodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onPeriodChanged);
    connect(statsPeriodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index){ onRefreshClicked(); });
    connect(logLevelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index){ onRefreshClicked(); });
    connect(logCountSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value){ onRefreshClicked(); });
    
    // Таймеры
    connect(autoRefreshTimer, &QTimer::timeout, this, &MainWindow::onRefreshClicked);
    connect(clockTimer, &QTimer::timeout, this, &MainWindow::updateDateTime);
    
    // HTTP клиент
    connect(httpClient, &HttpClient::currentTempUpdated,
            this, &MainWindow::onUpdateCurrentTemp);
    connect(httpClient, &HttpClient::historyDataUpdated,
            this, &MainWindow::onUpdateHistoryData);
    connect(httpClient, &HttpClient::statsDataUpdated,
            this, &MainWindow::onUpdateStatsData);
    connect(httpClient, &HttpClient::logsUpdated,
            this, &MainWindow::onUpdateLogs);
    connect(httpClient, &HttpClient::connectionError,
            this, &MainWindow::onConnectionError);
}

void MainWindow::loadInitialData() {
    statusLabel->setText("Загрузка данных...");
    onRefreshClicked();
}

void MainWindow::onRefreshClicked() {
    statusLabel->setText("Обновление данных...");
    connectionLabel->setText("Подключение: ⏳");
    
    // Обновляем текущую температуру
    httpClient->fetchCurrentTemperature();
    
    // Обновляем историю в зависимости от выбранного периода
    qint64 now = QDateTime::currentSecsSinceEpoch();
    qint64 startTime = 0;
    
    switch (periodCombo->currentIndex()) {
        case 0: // Последний час
            startTime = now - 3600;
            break;
        case 1: // Последние 24 часа
            startTime = now - 86400;
            break;
        case 2: // Последняя неделя
            startTime = now - 604800;
            break;
        case 3: // Произвольный (последние 24 часа по умолчанию)
            startTime = now - 86400;
            break;
    }
    
    httpClient->fetchHistory(startTime, now);
    
    // Обновляем логи
    QString level = "ALL";
    switch (logLevelCombo->currentIndex()) {
        case 1: level = "ERROR"; break;
        case 2: level = "WARNING"; break;
        case 3: level = "INFO"; break;
    }
    httpClient->fetchLogs(logCountSpin->value(), level);
    
    // Обновляем статистику
    httpClient->fetchHourlyStats(now - 86400, now); // За последние 24 часа
}

void MainWindow::onAutoRefreshToggled(bool checked) {
    if (checked) {
        autoRefreshTimer->start();
        statusLabel->setText("Автообновление включено");
    } else {
        autoRefreshTimer->stop();
        statusLabel->setText("Автообновление выключено");
    }
}

void MainWindow::onPeriodChanged(int index) {
    onRefreshClicked();
}

void MainWindow::onUpdateCurrentTemp(const QString &temp, const QString &time) {
    currentTempLabel->setText(temp + " °C");
    currentTimeLabel->setText("Время: " + time);
    lastUpdateLabel->setText("Последнее обновление: " + 
                            QDateTime::currentDateTime().toString("HH:mm:ss"));
    
    // Обновляем статус
    connectionLabel->setText("Подключение: ✅");
    statusLabel->setText("Данные обновлены");
}

void MainWindow::onUpdateHistoryData(const QVector<QPair<qint64, double>> &data) {
    // Обновляем график
    historyChart->updateChart(data, "Температура за период");
    
    // Обновляем мини-график на вкладке "Текущая"
    if (data.size() > 0) {
        // Берем последние 60 значений
        QVector<QPair<qint64, double>> recentData;
        int start = qMax(0, data.size() - 60);
        for (int i = start; i < data.size(); i++) {
            recentData.append(data[i]);
        }
        currentChart->updateChart(recentData, "Последние измерения");
    }
}

void MainWindow::onUpdateStatsData(double avg, int count, const QString &period) {
    statsLabel->setText(QString("Средняя температура: %1°C | Измерений: %2 | Период: %3")
                       .arg(avg, 0, 'f', 2)
                       .arg(count)
                       .arg(period));
}

void MainWindow::onUpdateLogs(const QVector<QStringList> &logs) {
    logTextEdit->clear();
    
    QString html = "<style>"
                   ".error { color: red; font-weight: bold; }"
                   ".warning { color: orange; }"
                   ".info { color: green; }"
                   ".timestamp { color: gray; font-size: 0.9em; }"
                   "</style>";
    
    for (const QStringList &log : logs) {
        if (log.size() >= 4) {
            QString timestamp = QDateTime::fromSecsSinceEpoch(log[0].toLongLong())
                                .toString("dd.MM.yyyy HH:mm:ss");
            QString level = log[1];
            QString module = log[2];
            QString message = log[3];
            
            QString levelClass;
            if (level == "ERROR") levelClass = "error";
            else if (level == "WARNING") levelClass = "warning";
            else if (level == "INFO") levelClass = "info";
            
            html += QString("<div><span class='timestamp'>[%1]</span> "
                           "<span class='%2'>[%3]</span> "
                           "<span class='%2'>[%4]</span> %5</div>")
                   .arg(timestamp, levelClass, level, module, message);
        }
    }
    
    logTextEdit->setHtml(html);
    logTextEdit->moveCursor(QTextCursor::End);
}

void MainWindow::onConnectionError(const QString &error) {
    connectionLabel->setText("Подключение: ❌");
    statusLabel->setText("Ошибка: " + error);
    
    QMessageBox::warning(this, "Ошибка соединения", 
                        "Не удалось подключиться к серверу.\n"
                        "Убедитесь, что сервер Lab5 запущен на localhost:8080\n"
                        "Ошибка: " + error);
}

void MainWindow::updateDateTime() {
    // Обновляем время в статус баре
    QString time = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
    statusBar->showMessage("Текущее время: " + time);
}