#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QDateTime>
#include "httpclient.h"
#include "chartwidget.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QComboBox;
class QLineEdit;
class QTableWidget;
class QTextEdit;
class QGroupBox;
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRefreshClicked();
    void onAutoRefreshToggled(bool checked);
    void onPeriodChanged(int index);
    void onUpdateCurrentTemp(const QString &temp, const QString &time);
    void onUpdateHistoryData(const QVector<QPair<qint64, double>> &data);
    void onUpdateStatsData(double avg, int count, const QString &period);
    void onUpdateLogs(const QVector<QStringList> &logs);
    void onConnectionError(const QString &error);
    void updateDateTime();

private:
    void setupUI();
    void setupConnections();
    void loadInitialData();
    
    // UI элементы
    QWidget *centralWidget;
    QTabWidget *tabWidget;
    
    // Вкладка "Текущая температура"
    QWidget *currentTab;
    QLabel *currentTempLabel;
    QLabel *currentTimeLabel;
    QLabel *lastUpdateLabel;
    ChartWidget *currentChart;
    
    // Вкладка "История"
    QWidget *historyTab;
    QComboBox *periodCombo;
    QPushButton *refreshButton;
    QCheckBox *autoRefreshCheck;
    QLabel *statsLabel;
    ChartWidget *historyChart;
    
    // Вкладка "Статистика"
    QWidget *statsTab;
    QComboBox *statsPeriodCombo;
    QTableWidget *statsTable;
    ChartWidget *statsChart;
    
    // Вкладка "Логи"
    QWidget *logsTab;
    QComboBox *logLevelCombo;
    QSpinBox *logCountSpin;
    QTextEdit *logTextEdit;
    QPushButton *clearLogsButton;
    
    // Нижняя панель
    QStatusBar *statusBar;
    QLabel *statusLabel;
    QLabel *connectionLabel;
    
    // Таймеры
    QTimer *autoRefreshTimer;
    QTimer *clockTimer;
    
    // HTTP клиент
    HttpClient *httpClient;
    
    // Настройки
    int autoRefreshInterval = 30000; // 30 секунд
};

#endif // MAINWINDOW_H