#include "mxzmq.h"
#include <QFileInfo>
#include <QRegularExpression>
#include <QTCore/QDir>
#include <QJsonDocument>
#include <snappy.h>

MXZmq::MXZmq(QObject *parent, QString topic, zmqpp::endpoint_t endpoint)
    : QThread{parent}, topic(topic) {
  _context = new zmqpp::context();
  _socket = new zmqpp::socket(*_context, zmqpp::socket_type::subscribe);
  _socket->set(zmqpp::socket_option::receive_timeout, SOCKET_TIMEOUT);
  if (!setEndpoint(endpoint)) {
    setEndpoint(QString(DEFAULT_ENDPOINT));
  }
  qDebug() << "ZMQ initialized, enpoint is " << _endpoint;
}

MXZmq::~MXZmq() {
  if (_connected) {
    disconnect(); // must be blocking!
  }
  _socket->close();
}


void MXZmq::connect() {
  if (!_connected) {
    _socket->connect(_endpoint);
    _socket->subscribe(topic.toStdString());
    QThread::usleep(CONNECT_DELAY);
    _connected = true;
    start();
  }
}

void MXZmq::disconnect() {
  if (_connected) {
    requestInterruption();
    while (isRunning()) {
      // NOOP
    }
    _socket->unsubscribe(topic.toStdString());
    _socket->disconnect(_endpoint);
    _connected = false;
  }
}

void MXZmq::run() {
  if (!_connected) {
    return;
  }
  // alla STL
  // std::unique_ptr<zmqpp::message_t> message(new zmqpp::message_t);
  QScopedPointer<zmqpp::message_t> message(new zmqpp::message_t);
  int parts = 0;
  while (!isInterruptionRequested()) {
    if (_socket->receive(*message)) {
      parts = message->parts();
      // no compression part
      if (2 == parts) {
        format = FORMAT_PLAIN;
        _payload = message->get(1);
      }
      // compression format part is present
      else if (3 == parts) {
        format = QString::fromStdString(message->get(1));
        _payload = message->get(2);
      }
      // wrong number of parts
      else {
        emit gotWrongMessage(parts);
        continue;
      }
      try {
        emit gotNewMessage(payloadData());
      } catch(std::invalid_argument &ex) {
        emit gotInvalidPayload(ex, payload());
      }
    } else {
      emit gotNoMessage();
    }
  }
}

// Valid endpoints are:
// - tcp://hostname[.domain]:port
// - ipc://</full/path/to/file>   ON UNIX ONLY!!!

bool MXZmq::validateEndpoint(const QString &endpoint) {
  static QRegularExpression re_tcp(R"(^tcp://([a-z0-9.]+):([0-9]{3,5})$)");
#if defined(Q_OS_MAC) or defined(Q_OS_UNIX)
  static QRegularExpression re_ipc(R"(^(ipc://)(/(\w+/{0,1})+\w*(\.\w+){0,1}$))");
  if (!re_tcp.match(endpoint).hasMatch() && !re_ipc.match(endpoint).hasMatch()) {
    goto fail;
  }
  if (re_ipc.match(endpoint).hasMatch()) {
    QString path = re_ipc.match(endpoint).captured(2);
    QFileInfo fi = QFileInfo(path);
    if (fi.isDir()) goto fail;
    if (!QFileInfo(fi.dir().path()).isWritable()) goto fail;
  }
#else
  if (!re_tcp.match(endpoint).hasMatch()) goto fail;
#endif
  return true;
  fail:
  return false;
}



// Accessori

QJsonObject MXZmq::payloadData() {
  return payloadDocument().object();
}


QJsonDocument MXZmq::payloadDocument() {
  QJsonDocument doc;
  if (FORMAT_PLAIN == format) {
    doc = QJsonDocument::fromJson(_payload.data());
  } else if (FORMAT_COMPRESSED == format) {
    std::string data;
    if (snappy::Uncompress(_payload.data(), _payload.size(), &data)) {
      doc = QJsonDocument::fromJson(data.data());
    } else {
      throw std::invalid_argument("Compression error");
    }
  }
  if (doc.isNull()) {
    throw std::invalid_argument("Not a valid JSON object");
  }
  return doc;
}


// Slots

bool MXZmq::setEndpoint(const QString &text) {
  bool ok = validateEndpoint(text);
  if (ok) {
    _endpoint = text.toStdString();
  }
  return ok;
}

bool MXZmq::setEndpoint(const zmqpp::endpoint_t &text) {
  return setEndpoint(QString::fromStdString(text));
}



