#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QPair>

class HttpClient : public QObject {
    Q_OBJECT

public:
    explicit HttpClient(QObject *parent = nullptr);
    
    void fetchCurrentTemperature();
    void fetchHistory(qint64 startTime, qint64 endTime);
    void fetchHourlyStats(qint64 startTime, qint64 endTime);
    void fetchDailyStats(qint64 startTime, qint64 endTime);
    void fetchLogs(int limit = 100, const QString &level = "ALL");
    
signals:
    void currentTempUpdated(const QString &temperature, const QString &time);
    void historyDataUpdated(const QVector<QPair<qint64, double>> &data);
    void statsDataUpdated(double average, int count, const QString &period);
    void logsUpdated(const QVector<QStringList> &logs);
    void connectionError(const QString &error);

private slots:
    void onCurrentTempReply(QNetworkReply *reply);
    void onHistoryReply(QNetworkReply *reply);
    void onLogsReply(QNetworkReply *reply);
    void onNetworkError(QNetworkReply::NetworkError error);

private:
    QNetworkAccessManager *networkManager;
    QString baseUrl = "http://localhost:8080";
    
    QJsonObject parseJsonReply(QNetworkReply *reply, bool &success);
};

#endif // HTTPCLIENT_H