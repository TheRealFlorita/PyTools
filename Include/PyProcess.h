#ifndef PYPROCESS_H
#define PYPROCESS_H

#include <QElapsedTimer>
#include <QMessageBox>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>

class PyTools;

class PyProcess : public QProcess
{
    Q_OBJECT

private:
    PyTools *pyTools;
    QLocalServer *localServer;
    QProcessEnvironment processEnvironment;
    QElapsedTimer timer;
    QList<QPair<QString, int>> taskKillList;
    QList<QPair<QMessageBox*,QString>> handles;
    bool printsEnabled = true;
    bool terminatedByUser;
    bool errorTermination;

signals:
    void pyProcessCancelled();
    void pyProcessStarted();
    void pyProcessFinished();
    void pyProcessMessageSent(QMessageBox::Icon, QString);
    void pyProcessStatusChanged(QString text,int timeout);
    void printRegular();
    void printBold();
    void printCursive();
    void printBoldCursive();
    void printProportionalFont();
    void printMonospaceFont();
    void increaseIndent();
    void decreaseIndent();
    void resetIndent();
    void readXml(QString fpath);
    void writeXml(QString fpath);
    void readyReadPyProcessOutput(QString text);
    void readyReadPyProcessError(QString text);
    void msgBoxClosed(QPair<QMessageBox*,QString>);

public:
    explicit PyProcess(PyTools *parent = Q_NULLPTR);
    void startPyProcess(QString script, QString stdinput);
    void killPyProcess();
    bool isRunning();
    static QString getPythonVersion(bool shortstring = false);
    static QString getEmbeddedPythonVersion(bool shortstring = false);
    static bool processIsRunning(QString name, int pid = 0, int timeout = 10000);
    static bool terminateProcess(QString name, int pid = 0);
    void closeExcelBooks(int pid=0);

private:
    void finalizePyProcess(int exitcode, QProcess::ExitStatus exitstatus);
    void onNewConnection();
    void processClientRequest(QLocalSocket *client);
    void addKillLaterTask(QString imageName, int pid = 0);
    void readStandardOutput();
    void readStandardError();
    void showErrorMessage(QString text);
    void removeMsgBoxHandle(QPair<QMessageBox*,QString> handle);

};

#endif
