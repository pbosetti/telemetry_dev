#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "defines.h"
#include "qcustomplot.h"

/*
  _     _  __                      _      
 | |   (_)/ _| ___  ___ _   _  ___| | ___ 
 | |   | | |_ / _ \/ __| | | |/ __| |/ _ \
 | |___| |  _|  __/ (__| |_| | (__| |  __/
 |_____|_|_|  \___|\___|\__, |\___|_|\___|
                        |___/             
*/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  loadSettings();

  _zmq = new MXZmq(this, ui->topicLine->text(),
                   ui->endpointLine->text().toStdString());
  // if _zmq init fails, endpoint is reset to default, so we want to synchro
  // back the UI
  ui->endpointLine->setText(_zmq->endpoint());

  _sequenceChartsModel = new QStandardItemModel(0, 0, ui->sequenceChart);
  ui->sequenceChartSeriesList->setModel(_sequenceChartsModel);
  _fullChartsModel = new QStandardItemModel(0, 0, ui->fullChart);
  ui->fullChartSeriesList->setModel(_fullChartsModel);

  setWindowTitle(APP_NAME " - " APP_VERSION);

  connect(ui->connectButton, &QPushButton::toggled, this,
          &MainWindow::connectButtonToggled);

  connect(ui->endpointLine, &QLineEdit::textEdited, this,
          &MainWindow::endpointLineEdited);

  connect(_zmq, &MXZmq::gotNewMessage, this, &MainWindow::newMessageReceived);

  connect(_zmq, &MXZmq::gotInvalidPayload, this, &MainWindow::invalidPayloadReceived);

  connect(_zmq, &MXZmq::gotNoMessage, this, &MainWindow::noMessageReceived);

  connect(_zmq, &MXZmq::gotWrongMessage, this, &MainWindow::invalidMessageReceived);

  connect(_sequenceChartsModel, &QStandardItemModel::itemChanged, this, [=](QStandardItem *item) {
    bool onOff = item->checkState() == Qt::Checked ? true : false;
    _sequenceCharts[item->text()]->setVisible(onOff);
    ui->sequenceChart->replot();
  });

  connect(_fullChartsModel, &QStandardItemModel::itemChanged, this, [=](QStandardItem *item) {
    bool onOff = item->checkState() == Qt::Checked ? true : false;
    _fullCharts[item->text()]->setVisible(onOff);
    ui->fullChart->replot();
  });
}

MainWindow::~MainWindow() {
  delete _sequenceChartsModel;
  delete _fullChartsModel;
  delete _zmq;
  delete ui; 
}


/*
  __  __      _   _               _     
 |  \/  | ___| |_| |__   ___   __| |___ 
 | |\/| |/ _ \ __| '_ \ / _ \ / _` / __|
 | |  | |  __/ |_| | | | (_) | (_| \__ \
 |_|  |_|\___|\__|_| |_|\___/ \__,_|___/
                                        
*/

void MainWindow::saveSettings() {
  _settings.beginGroup("MainWindow");
  _settings.setValue("geometry", saveGeometry());
  _settings.setValue("endpoint", ui->endpointLine->text());
  _settings.setValue("topic", ui->topicLine->text());
  _settings.endGroup();
}

void MainWindow::loadSettings() {
  _settings.beginGroup("MainWindow");
  const auto topic = _settings.value("topic", DEFAULT_TOPIC).toString();
  ui->topicLine->setText(topic);

  const auto endpoint =
      _settings.value("endpoint", DEFAULT_ENDPOINT).toString();
  ui->endpointLine->setText(endpoint);

  const auto geometry = _settings.value("geometry", QByteArray()).toByteArray();
  if (geometry.isEmpty())
    setGeometry(800, 800, 600, 600);
  else
    restoreGeometry(geometry);
  _settings.endGroup();
}

/*
  ____  _       _       
 / ___|| | ___ | |_ ___ 
 \___ \| |/ _ \| __/ __|
  ___) | | (_) | |_\__ \
 |____/|_|\___/ \__|___/
                        
*/

void MainWindow::appendLogMessage(bool status) {
  ui->logMessageArea->appendPlainText(status ? "Checked" : "Not checked");
}

void MainWindow::connectButtonToggled(bool checked) {
  if (checked) {
    _zmq->connect();
    ui->connectButton->setText("Disconnect");
    ui->statusbar->showMessage("Connected", 5000);
    ui->endpointLine->setEnabled(false);
    ui->topicLine->setEnabled(false);
  } else {
    _zmq->disconnect();
    ui->connectButton->setText("Connect");
    ui->statusbar->showMessage("Disconnected", 5000);
    ui->endpointLine->setEnabled(true);
    ui->topicLine->setEnabled(true);
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
  foreach (auto &k, obj.keys()) {
    val = obj.value(k);
    try {
      // single value: add it to lower plot
      if (val.isDouble()) {
        if (!_sequenceCharts.contains(k)) {
          QStandardItem *item = new QStandardItem(k);
          item->setCheckable(true);
          item->setCheckState(Qt::Checked);
          _sequenceChartsModel->appendRow(item);
          _sequenceCharts[k] = ui->sequenceChart->addGraph();
          _sequenceCharts[k]->setName(k);
        }
        _sequenceCharts[k]->addData(_messageCount, val.toDouble());
        ui->sequenceChart->xAxis->rescale(true);
        ui->sequenceChart->yAxis->rescale(true);
        ui->sequenceChart->replot();
      }

      // Array: add it to upper plot
      else if (val.isArray()) {
        // check structure
        QJsonArray ary = val.toArray();
        if (ary.count() != 2)
          throw std::invalid_argument("XY plot must have two columns");
        if (ary.at(0).toArray().count() != ary.at(1).toArray().count())
          throw std::invalid_argument("XY plot colums must have same lengths");
        if (!_fullCharts.contains(k)) {
          QStandardItem *item = new QStandardItem(k);
          item->setCheckable(true);
          item->setCheckState(Qt::Checked);
          _fullChartsModel->appendRow(item);
          _fullCharts[k] = new QCPCurve(ui->fullChart->xAxis, ui->fullChart->yAxis);
          _fullCharts[k]->setName(k);
        }
        unsigned long n = ary.at(0).toArray().count();
        _fullCharts[k]->data().data()->clear();
        for (int i = 0; i < n; i++) {
          _fullCharts[k]->addData(ary.at(0).toArray().at(i).toDouble(), ary.at(1).toArray().at(i).toDouble());
        }
        ui->fullChart->xAxis->rescale(true);
        ui->fullChart->yAxis->rescale(true);
        ui->fullChart->replot();
      }

      // Unsupported type
      else {
        throw std::invalid_argument(std::string("Unexpected type in JSON object ") + k.toStdString());
      }
    } catch (std::invalid_argument &ex) {
      ui->statusbar->showMessage("Error dealing with message");
      ui->logMessageArea->appendPlainText(ex.what());
    }
  }
  ui->statusbar->clearMessage();
  _messageCount++;
}

void MainWindow::invalidPayloadReceived(const std::invalid_argument &ex, const QString &msg) {
  ui->logMessageArea->appendPlainText(QString::asprintf("Payload error: %s\npayload: %s", ex.what(), msg.toStdString().c_str()));
}

void MainWindow::invalidMessageReceived(int parts) {
  ui->logMessageArea->appendPlainText(QString::asprintf("Message with %d parts", parts));
}

void MainWindow::noMessageReceived() {
  ui->statusbar->showMessage("No incoming messages", SOCKET_TIMEOUT*2);
}

void MainWindow::closeEvent(QCloseEvent *event) {
  qDebug() << "Saving settings to " << _settings.fileName();
  _zmq->disconnect();
  saveSettings();
}
