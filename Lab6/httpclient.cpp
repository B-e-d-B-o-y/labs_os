#include "httpclient.h"
#include <QUrl>
#include <QNetworkRequest>
#include <QDateTime>
#include <QDebug>

HttpClient::HttpClient(QObject *parent) : QObject(parent) {
    networkManager = new QNetworkAccessManager(this);
    
    // Подключаем обработчики ошибок
    connect(networkManager, &QNetworkAccessManager::finished,
            this, [this](QNetworkReply *reply) {
        if (reply->error() != QNetworkReply::NoError) {
            onNetworkError(reply->error());
        }
        reply->deleteLater();
    });
}

void HttpClient::fetchCurrentTemperature() {
    QUrl url(baseUrl + "/current");
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onCurrentTempReply(reply);
    });
}

void HttpClient::fetchHistory(qint64 startTime, qint64 endTime) {
    QUrl url(baseUrl + QString("/history?start=%1&end=%2").arg(startTime).arg(endTime));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onHistoryReply(reply);
    });
}

void HttpClient::fetchHourlyStats(qint64 startTime, qint64 endTime) {
    QUrl url(baseUrl + QString("/hourly?start=%1&end=%2").arg(startTime).arg(endTime));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        // TODO: Implement hourly stats parsing
        reply->deleteLater();
    });
}

void HttpClient::fetchDailyStats(qint64 startTime, qint64 endTime) {
    QUrl url(baseUrl + QString("/daily?start=%1&end=%2").arg(startTime).arg(endTime));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        // TODO: Implement daily stats parsing
        reply->deleteLater();
    });
}

void HttpClient::fetchLogs(int limit, const QString &level) {
    QUrl url(baseUrl + QString("/logs?limit=%1&level=%2").arg(limit).arg(level));
    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onLogsReply(reply);
    });
}

void HttpClient::onCurrentTempReply(QNetworkReply *reply) {
    bool success = false;
    QJsonObject json = parseJsonReply(reply, success);
    
    if (success) {
        double temp = json.value("value").toDouble();
        qint64 timestamp = json.value("timestamp").toVariant().toLongLong();
        QString timeStr = json.value("time_str").toString();
        
        if (timeStr.isEmpty()) {
            timeStr = QDateTime::fromSecsSinceEpoch(timestamp)
                     .toString("dd.MM.yyyy HH:mm:ss");
        }
        
        emit currentTempUpdated(QString::number(temp, 'f', 1), timeStr);
    } else {
        emit connectionError("Не удалось получить текущую температуру");
    }
}

void HttpClient::onHistoryReply(QNetworkReply *reply) {
    bool success = false;
    QJsonObject json = parseJsonReply(reply, success);
    
    if (success) {
        QVector<QPair<qint64, double>> data;
        double sum = 0;
        int count = 0;
        
        QJsonArray measurements = json.value("measurements").toArray();
        for (const QJsonValue &val : measurements) {
            QJsonObject obj = val.toObject();
            qint64 timestamp = obj.value("timestamp").toVariant().toLongLong();
            double temp = obj.value("value").toDouble();
            
            data.append(qMakePair(timestamp, temp));
            sum += temp;
            count++;
        }
        
        double average = count > 0 ? sum / count : 0;
        
        // Определяем период
        QString period;
        if (data.size() > 0) {
            qint64 startTime = data.first().first;
            qint64 endTime = data.last().first;
            qint64 duration = endTime - startTime;
            
            if (duration <= 3600) period = "1 час";
            else if (duration <= 86400) period = "24 часа";
            else if (duration <= 604800) period = "1 неделя";
            else period = QString("%1 дней").arg(duration / 86400);
        }
        
        emit historyDataUpdated(data);
        emit statsDataUpdated(average, count, period);
    } else {
        emit connectionError("Не удалось получить историю");
    }
}

void HttpClient::onLogsReply(QNetworkReply *reply) {
    bool success = false;
    QJsonObject json = parseJsonReply(reply, success);
    
    if (success) {
        QVector<QStringList> logs;
        
        QJsonArray logsArray = json.value("logs").toArray();
        for (const QJsonValue &val : logsArray) {
            QJsonObject obj = val.toObject();
            QStringList logEntry;
            
            logEntry << QString::number(obj.value("timestamp").toVariant().toLongLong());
            logEntry << obj.value("level").toString();
            logEntry << obj.value("module").toString();
            logEntry << obj.value("message").toString();
            
            logs.append(logEntry);
        }
        
        emit logsUpdated(logs);
    } else {
        emit connectionError("Не удалось получить логи");
    }
}

void HttpClient::onNetworkError(QNetworkReply::NetworkError error) {
    QString errorStr;
    switch (error) {
        case QNetworkReply::ConnectionRefusedError:
            errorStr = "Сервер отказал в подключении";
            break;
        case QNetworkReply::HostNotFoundError:
            errorStr = "Сервер не найден";
            break;
        case QNetworkReply::TimeoutError:
            errorStr = "Таймаут соединения";
            break;
        default:
            errorStr = QString("Код ошибки: %1").arg(error);
    }
    
    emit connectionError(errorStr);
}

QJsonObject HttpClient::parseJsonReply(QNetworkReply *reply, bool &success) {
    success = false;
    QJsonObject empty;
    
    if (reply->error() != QNetworkReply::NoError) {
        return empty;
    }
    
    QByteArray response = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error:" << parseError.errorString();
        return empty;
    }
    
    if (!doc.isObject()) {
        return empty;
    }
    
    success = true;
    return doc.object();
}