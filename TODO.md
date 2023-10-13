# Done

* mainwindow.cpp: create slots for:
  - connect button
  - new message
  - topic line edited
* mainwindow.cpp: update destructor for disconecting from server
* mainwindow.cpp: deal with defaults (also add APP_DOMAIN to CMakeLists.txt)

# To Dos

* unique_ptr in run
* QSharedPointer per membri
* Add a tabset with two tabs, log message area in the second within vertical layout
* In the first tab, add a grid with two model based items lists and two charts
  * promote both charts to QCustomPlot
  * also add SOURCES_DIR to include_directories in cmake
* Give names to each item
* Revise the messages emitted by MXZmq