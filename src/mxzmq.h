#ifndef MXZMQ_H
#define MXZMQ_H

#include <QThread>
#include <zmqpp/zmqpp.hpp>
#include <QJsonObject>
#include "defines.h"

class MXZmq : public QThread
{
  Q_OBJECT
public:
  explicit MXZmq(QObject *parent, QString topic, zmqpp::endpoint_t endpoint);
  ~MXZmq();
  // Methods
  void run() override;
  void connect();
  void disconnect();
  QJsonObject payloadData();
  QJsonDocument payloadDocument();
  bool validateEndpoint(const QString &endpoint);

  // Accessors
  bool isConnected() { return _connected; }
  QString payload() { return QString::fromStdString(_payload); }
  QString endpoint() { return QString::fromStdString(_endpoint); }

  // Public properties
  QString format = "unknown";
  QString topic;

public slots:
  bool setEndpoint(const QString &text);
  bool setEndpoint(const zmqpp::endpoint_t &text);

signals:
  void gotNewMessage(const QJsonObject &obj);
  void gotInvalidPayload(const QString &msg);
  void gotWrongMessage(int parts);
  void gotNoMessage();

private:
  QSharedPointer<zmqpp::context> _context;
  QSharedPointer<zmqpp::socket> _socket;
  zmqpp::endpoint_t _endpoint;
  std::string _payload = "";
  bool _connected = false;

};

#endif // MXZMQ_H
