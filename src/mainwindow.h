#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include "mxzmq.h"
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  void saveSettings();
  void loadSettings();

private slots:
  void appendLogMessage(bool status);
  void connectButtonToggled(bool checked);
  void endpointLineEdited(const QString &text);
  void newMessageReceived(const QJsonObject &obj);
  void invalidPayloadReceived(const std::invalid_argument &ex, const QString &msg);
  void invalidMessageReceived(int parts);
  void noMessageReceived();
  void closeEvent(QCloseEvent *event);

private:
  unsigned long long _messageCount = 0;
  Ui::MainWindow *ui;
  MXZmq *_zmq;
  QSettings _settings = QSettings(APP_DOMAIN, APP_NAME);
  QMap<QString, QCPGraph *> _sequenceCharts;
  QMap<QString, QCPCurve *> _fullCharts;
};
#endif // MAINWINDOW_H
