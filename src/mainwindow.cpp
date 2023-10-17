#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "defines.h"
#include "qcustomplot.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  loadSettings();

  setWindowTitle(APP_NAME " - " APP_VERSION);

  _sequenceChartsModel = new QStandardItemModel(0, 0, ui->sequenceChart);
  ui->sequenceChartsSeriesList->setModel(_sequenceChartsModel);

  _fullChartsModel = new QStandardItemModel(0, 0, ui->fullChart);
  ui->fullChartSeriesList->setModel(_fullChartsModel);

  _zmq = new MXZmq(this, ui->topicLine->text(), ui->endpointLine->text().toStdString());
  // if _zmq init fails, endpoint is reset to default, so we want to synchro back the UI
  ui->endpointLine->setText(_zmq->endpoint());

  connect(ui->connectButton, &QPushButton::toggled, this, &MainWindow::connectButtonToggled);

  connect(ui->endpointLine, &QLineEdit::textEdited, this, &MainWindow::endpointLineEdited);

  connect(_zmq, &MXZmq::gotNewMessage, this, &MainWindow::newMessageReceived);

  connect(_zmq, &MXZmq::gotInvalidPayload, this, &MainWindow::invalidPayloadReceived);

  connect(_zmq, &MXZmq::gotWrongMessage, this, &MainWindow::invalidMessageReceived);

  connect(_zmq, &MXZmq::gotNoMessage, this, &MainWindow::noMessageReceived);

  connect(_sequenceChartsModel, &QStandardItemModel::itemChanged, this, [=](QStandardItem *item) {
    bool onOff = (item->checkState() == Qt::Checked);
    _sequenceCharts[item->text()]->setVisible(onOff);
    ui->sequenceChart->replot();
  });

  connect(_fullChartsModel, &QStandardItemModel::itemChanged, this, [=](QStandardItem *item) {
    bool onOff = (item->checkState() == Qt::Checked);
    _fullCharts[item->text()]->setVisible(onOff);
    ui->fullChart->replot();
  });

  // timer for replotting charts
  _replotTimer->setInterval(20);
  connect(_replotTimer, &QTimer::timeout, this, [=]() {
    if (_idle) return;
    ui->sequenceChart->xAxis->rescale(true);
    ui->sequenceChart->yAxis->rescale(true);
    ui->sequenceChart->replot();
    ui->fullChart->xAxis->rescale(true);
    ui->fullChart->yAxis->rescale(true);
    ui->fullChart->replot();
  });

}

MainWindow::~MainWindow() {
  delete _sequenceChartsModel;
  delete _fullChartsModel;
  delete ui;
}

void MainWindow::appendLogMessage(bool status) {
  ui->logMessageArea->appendPlainText(status ? "Checked" : "Not checked");
}

void MainWindow::saveSettings(){
  _settings.beginGroup("MainWindow");
  _settings.setValue("endpoint", ui->endpointLine->text());
  _settings.setValue("topic", ui->topicLine->text());
  _settings.setValue("geometry", saveGeometry());
  _settings.endGroup();
}

void MainWindow::loadSettings(){
  _settings.beginGroup("MainWindow");
  auto endpoint = _settings.value("endpoint", DEFAULT_ENDPOINT).toString();
  ui->endpointLine->setText(endpoint);

  auto topic = _settings.value("topic", DEFAULT_TOPIC).toString();
  ui->topicLine->setText(topic);

  auto geometry = _settings.value("geometry").toByteArray();
  if (geometry.isEmpty()) {
    setGeometry(800, 800, 600, 600);
  } else {
    restoreGeometry(geometry);
  }

  _settings.endGroup();
}

QColor MainWindow::colorSequence(unsigned long n) {
  if (n >= _palette.count()) n %= _palette.count();
  return _palette.at(n);
}

QColor MainWindow::textColorSequence(unsigned long n) {
  QColor bg = colorSequence(n), result;
  int h, s, l;
  bg.getHsl(&h, &s, &l);
  if (l < 256 * 0.25) {
    result = QColor("#EEEEEE");
  } else {
    result = QColor("#000000");
  }
  return result;
}

void MainWindow::connectButtonToggled(bool checked) {
  if (checked) {
    _zmq->connect();
    ui->connectButton->setText("Disconnect");
    ui->statusbar->showMessage("Connected", 5000);
    ui->endpointLine->setEnabled(false);
    ui->topicLine->setEnabled(false);
    _replotTimer->start();
  } else {
    _zmq->disconnect();
    ui->connectButton->setText("Connect");
    ui->statusbar->showMessage("Disconnected", 5000);
    ui->endpointLine->setEnabled(true);
    ui->topicLine->setEnabled(true);
    _replotTimer->stop();
  }
}

void MainWindow::endpointLineEdited(const QString &text) {
  if (_zmq->setEndpoint(text)) {
    ui->connectButton->setEnabled(true);
    ui->endpointLine->setStyleSheet("QLineEdit { color: black; }");
  } else {
    ui->connectButton->setEnabled(false);
    ui->endpointLine->setStyleSheet("QLineEdit { color: red; }");
  }
}

void MainWindow::newMessageReceived(const QJsonObject &obj) {
  QJsonValue val;
  _idle = false;
  foreach(auto &k, obj.keys()) {
    val = obj.value(k);
    try {
      // scalar: add it to the lower chart
      if (val.isDouble()) {
        if (!_sequenceCharts.contains(k)) {
          unsigned long int n = _sequenceCharts.count();
          QStandardItem *item = new QStandardItem(k);
          item->setCheckable(true);
          item->setCheckState(Qt::Checked);
          item->setData(QBrush(colorSequence(n)), Qt::BackgroundRole);
          item->setData(QBrush(textColorSequence(n)), Qt::ForegroundRole);
          _sequenceChartsModel->appendRow(item);
          _sequenceCharts[k] = ui->sequenceChart->addGraph();
          _sequenceCharts[k]->setName(k);
          _sequenceCharts[k]->setPen(QPen(colorSequence(n)));
        }
        _sequenceCharts[k]->addData(_messageCount, val.toDouble());
      }
      // Array: add it to upper chart
      else if (val.isArray()) {
        QJsonArray ary = val.toArray();
        if (ary.count() != 2)
          throw std::invalid_argument("XY plot data must have two columns");
        if (!ary.at(0).isArray() && !ary.at(1).isArray())
          throw std::invalid_argument("XY plot data must contain two arrays");
        if (ary.at(0).toArray().count() != ary.at(1).toArray().count())
          throw std::invalid_argument("XY plot data columns must have the same size");
        if (!_fullCharts.contains(k)) {
          unsigned long int n = _fullCharts.count();
          QStandardItem *item = new QStandardItem(k);
          item->setCheckable(true);
          item->setCheckState(Qt::Checked);
          item->setData(QBrush(colorSequence(n)), Qt::BackgroundRole);
          item->setData(QBrush(textColorSequence(n)), Qt::ForegroundRole);
          _fullChartsModel->appendRow(item);
          _fullCharts[k] = new QCPCurve(ui->fullChart->xAxis, ui->fullChart->yAxis);
          _fullCharts[k]->setName(k);
          _fullCharts[k]->setPen(QPen(colorSequence(n)));
        }
        unsigned long n = ary.at(0).toArray().count();
        _fullCharts[k]->data().data()->clear();
        for (int i = 0; i < n; i++) {
          _fullCharts[k]->addData(ary.at(0).toArray().at(i).toDouble(), ary.at(1).toArray().at(i).toDouble());
        }
      }
      // Unsupported type
      else {
        throw std::invalid_argument(std::string("Unexpected type in JSON object " + k.toStdString()));
      }
    } catch (std::invalid_argument &ex) {
      ui->statusbar->showMessage("Error dealing with message");
      ui->logMessageArea->appendPlainText(ex.what());
    }
  }
  _messageCount++;
  ui->statusbar->clearMessage();
}

void MainWindow::invalidPayloadReceived(const std::invalid_argument &ex, const QString &payload) {
  ui->logMessageArea->appendPlainText(QString::asprintf("Invalid JSON: %s\npayload: %s", ex.what(), payload.toStdString().c_str()));
}

void MainWindow::invalidMessageReceived(int parts) {
  ui->logMessageArea->appendPlainText(QString::asprintf("Message with wrong number of parts (%d)", parts));
}

void MainWindow::noMessageReceived() {
  ui->statusbar->showMessage("No incoming messages", SOCKET_TIMEOUT*2);
  _idle = true;
}


void MainWindow::closeEvent(QCloseEvent *event) {
  _zmq->disconnect();
  delete _zmq;
  saveSettings();
}
