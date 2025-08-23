#include <PyProcess.h>

#include <QFileInfo>
#include <QTextCursor>
#include <QUuid>

#include <pugixml.hpp>
#include <Settings.h>
#include <FramelessInputDialog.h>
#include <FramelessFileDialog.h>
#include <FramelessMessageBox.h>

#include <PyTools.h>

PyProcess::PyProcess(PyTools *parent) : QProcess(parent)
{
    pyTools = parent;
    printsEnabled = true;
    terminatedByUser = false;

    /* Create local server */
    localServer = new QLocalServer(this);
    localServer->setSocketOptions(QLocalServer::WorldAccessOption);

    /* Setup process environment */
    processEnvironment = QProcessEnvironment::systemEnvironment();

    const QStringList keys = processEnvironment.keys();
    for(const QString &key: keys)
        if (key.contains("Py", Qt::CaseInsensitive))
            processEnvironment.remove(key);

    processEnvironment.insert("PATH","%SystemRoot%;%SystemRoot%/system32");
    processEnvironment.insert("PT_PYTHONPATH", QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/packages;");
    processEnvironment.insert("PT_SERVER_NAME", "");
    processEnvironment.insert("PYTHONUNBUFFERED", "1");
    processEnvironment.insert("PYTHONDONTWRITEBYTECODE", "1");
    processEnvironment.insert("PYTHONIOENCODING", "UTF-8");
    processEnvironment.insert("PYTHONUTF8", "1");
    processEnvironment.insert("PYTHONNOUSERSITE", "1");
    setProcessEnvironment(processEnvironment);

    connect(this, &QProcess::readyReadStandardOutput, this, &PyProcess::readStandardOutput, Qt::UniqueConnection);
    connect(this, &QProcess::readyReadStandardError, this, &PyProcess::readStandardError, Qt::UniqueConnection);
    connect(this, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &PyProcess::finalizePyProcess, Qt::UniqueConnection);
}

QString PyProcess::getPythonVersion(bool shortstring)
{
    if (!QFileInfo::exists(settings.getPythonPath()))
        return QString("Python executable not found!");

    QProcess version;
    version.start(settings.getPythonPath(), {"-B",  "-E", "-c", "import sys\nprint('Python', sys.version)"});
    version.waitForFinished(5000);
    QString output = version.readAllStandardOutput().trimmed();
    version.close();

    if (shortstring && output.contains(" (tags"))
        output = output.split(" (tags").first();
    return output;
}

QString PyProcess::getEmbeddedPythonVersion(bool shortstring)
{
    if (!QFileInfo::exists(settings.getEmbeddedPythonPath()))
        return QString("Python executable not found!");

    QProcess version;
    version.start(settings.getEmbeddedPythonPath(), {"-B",  "-E", "-c", "import sys\nprint('Python', sys.version)"});
    version.waitForFinished(5000);
    QString output = version.readAllStandardOutput().trimmed();
    version.close();

    if (shortstring && output.contains(" (tags"))
        output = output.split(" (tags").first();
    return output;
}

void PyProcess::addKillLaterTask(QString imageName, int pid)
{
    taskKillList.append(QPair<QString, int>(imageName, pid));
}

void PyProcess::startPyProcess(QString script, QString stdinput)
{
    if (!QFileInfo::exists(settings.getPythonPath()))
        return;

    if (pyTools != Q_NULLPTR)
        disconnect(this, &PyProcess::pyProcessFinished, pyTools, &PyTools::startModule);

    errorTermination = false;

    /* Check if process is available */
    if (isRunning())
    {
        FramelessMessageBox *msg = new FramelessMessageBox(QMessageBox::Information, settings.getApplicationName(), PyProcess::tr("Python is already running"), QMessageBox::Ok);
        msg->setAutoDeleteOnClose();
        msg->show();
        emit pyProcessCancelled();
        return;
    }

    /* Check if Python file exists */
    QFileInfo pyfile(script);
    if (!pyfile.exists())
    {
        FramelessMessageBox *msg = new FramelessMessageBox(QMessageBox::Critical, settings.getApplicationName(), PyProcess::tr("Python file not found"), QMessageBox::Ok);
        msg->setInformativeText(QString("[%1]").arg(pyfile.absoluteFilePath()));
        msg->setAutoDeleteOnClose();
        msg->show();
        emit pyProcessCancelled();
        return;
    }

    timer.restart();
    printsEnabled = true;
    terminatedByUser = false;
    taskKillList.clear();

    /* Set-up local server */
    localServer->listen("pt-" + QUuid::createUuid().toString(QUuid::WithoutBraces));
    connect(localServer, &QLocalServer::newConnection, this, &PyProcess::onNewConnection, Qt::UniqueConnection);
    processEnvironment.insert("PT_SERVER_NAME", localServer->fullServerName());
    setProcessEnvironment(processEnvironment);

    /* Set working directory to script folder */
    setWorkingDirectory(QFileInfo(script).absolutePath());

    /* Start process */
    QProcess::start(settings.getPythonPath(), QStringList() << pyfile.absoluteFilePath());
    emit pyProcessStarted();
    emit pyProcessStatusChanged(PyProcess::tr("Starting Python..."), 2500);

    /* Keep GUI responsive while process is starting */
    while (state() == QProcess::Starting)
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 100);

    /* Write to standard input channel */
    write(stdinput.toUtf8().data());
    waitForBytesWritten();
    closeWriteChannel();
    return;
}

void PyProcess::finalizePyProcess(int exitcode, QProcess::ExitStatus exitstatus)
{
    Q_UNUSED(exitcode)

    /* Close channels */
    close();

    /* Reset local server */
    localServer->disconnect();
    localServer->close();

    /* Force close all processes in taskkill list */
    for (int i = 0; i < taskKillList.length(); ++i)
    {
        QString appIM = taskKillList[i].first;
        int appPID = taskKillList[i].second;

        /* Close workbooks for excel processes */
        if (appIM.contains("excel.exe", Qt::CaseInsensitive))
            closeExcelBooks(appPID);
        else
            terminateProcess(appIM, appPID);
    }
    taskKillList.clear();

    /* Timer */
    emit printRegular();
    emit resetIndent();
    emit readyReadPyProcessOutput("\r\n*** " + QDateTime::fromMSecsSinceEpoch(timer.elapsed()).toUTC().toString("hh:mm:ss") + " ***\r\n");

    if (terminatedByUser)
        emit pyProcessStatusChanged(PyProcess::tr("Python terminated by user"), 30000);
    else if (exitstatus == QProcess::CrashExit || errorTermination)
        emit pyProcessStatusChanged(PyProcess::tr("Python terminated with error"), 30000);
    else
        emit pyProcessStatusChanged(PyProcess::tr("Python finished successfully"), 30000);
    emit pyProcessFinished();
    return;
}

void PyProcess::killPyProcess()
{
    /* Check validity of process and kill it */
    if (isRunning())
    {
        terminatedByUser = true;
        kill();
    }
}

bool PyProcess::isRunning()
{
    if (state() != QProcess::NotRunning)
        return true;
    return false;
}

bool PyProcess::processIsRunning(QString name, int pid, int timeout)
{
    /* Use tasklist to get list of running processes */
    if (name.isEmpty() && pid <= 0)
        return false;

    /* Arguments list */
    QStringList args;
    if (!name.isEmpty())
        args << "/fi" << QString("imagename eq %1").arg(name);
    if (pid > 0)
        args << "/fi" << QString("pid eq %1").arg(QString::number(pid));
    args << "/fo" << "csv" << "/nh";

    /* Call tasklist */
    QProcess tasklist;
    tasklist.start("tasklist", args);
    tasklist.waitForFinished(timeout);
    QString output = tasklist.readAll();
    tasklist.close();

    /* Return false is no running processes are found */
    if (output.startsWith("error: the search filter", Qt::CaseInsensitive) || (output.startsWith("info: no tasks are", Qt::CaseInsensitive)))
        return false;

    return true;
}

bool PyProcess::terminateProcess(QString name, int pid)
{
    if (name.isEmpty() && pid <= 0)
        return false;
    else if (!processIsRunning(name, pid, false))
        return false;

    /* Arguments list */
    QStringList args;
    if (!name.isEmpty())
        args << "/fi" << QString("imagename eq %1").arg(name);
    if (pid > 0)
        args << "/fi" << QString("pid eq %1").arg(QString::number(pid));
    args << "/f" << "/t"; // terminates child processes

    /* Call taskkill */
    QProcess::startDetached("taskkill", args);
    return true;
}

void PyProcess::closeExcelBooks(int pid)
{
    if (!QFileInfo::exists(settings.getEmbeddedPythonPath()))
        return;

    if (!processIsRunning("excel.exe", pid, false))
        return;

    /* Define script */
    QString xlclose = "import xlwings\n"
                      "for app in xlwings.apps:\n";
    if (pid > 0)
    {
        xlclose +=    "    if app.pid != int(" + QString::number(pid) + "):\n"
                      "        continue\n";
    }
    xlclose +=        "    try:\n"
                      "        for book in app.books:\n"
                      "            book.close()\n"
                      "        app.quit()\n"
                      "        app.kill()\n"
                      "    except:\n"
                      "        pass";

    QProcess *process = new QProcess(this);
    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [pid](){ terminateProcess("excel.exe", pid); });
    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), process, &QProcess::deleteLater);
    process->start(settings.getEmbeddedPythonPath(), {"-B",  "-E", "-c", xlclose});
}

void PyProcess::onNewConnection()
{
    QLocalSocket *pipe = localServer->nextPendingConnection();
    connect(pipe, &QLocalSocket::disconnected, pipe, &QLocalSocket::deleteLater);

    /* Wait for connection and ready read */
    if (!pipe->waitForConnected(2500))
        qDebug() << pipe->error() << pipe->errorString();
    else if (!pipe->waitForReadyRead(2500))
        qDebug() << "connection timed out";
    else
        processClientRequest(pipe);

    pipe->deleteLater();
}

void PyProcess::removeMsgBoxHandle(QPair<QMessageBox*,QString> handle)
{
    handles.removeOne(handle);
}

void PyProcess::processClientRequest(QLocalSocket *client)
{
    QByteArray bytes;
    while(client->bytesAvailable() > 0)
        bytes.append(client->readAll());

    pugi::xml_document xmlRequest;
    xmlRequest.load_string(QString(bytes).toStdString().c_str());
    pugi::xml_node request = xmlRequest.document_element();
    QString requestType = QString(request.name()).toLower();

    if (requestType == "spamrequest")
    {
        QString spam = request.attribute("spam").value();
        bool block = QString(request.attribute("block").value()).toLower().replace("true","1").toInt();

        /* Open message box */
        FramelessMessageBox *msg = new FramelessMessageBox(QMessageBox::Information, settings.getApplicationName(), "Info:", QMessageBox::Ok);
        msg->setInformativeText(spam);
        msg->setAutoDeleteOnClose();

        if (block)
        {
            msg->exec();
            client->write("succes");
        }
        else
        {
            msg->show();
            msg->raise();
        }
    }

    else if (requestType == "simplequestionrequest")
    {
        QString question = request.attribute("question").value();
        QString defaultReply = request.attribute("default-reply").value();

        /* Open message box */
        FramelessMessageBox *msg = new FramelessMessageBox(QMessageBox::Question, settings.getApplicationName(), "Question:", QMessageBox::Yes | QMessageBox::No);
        msg->setInformativeText(question);
        msg->setAutoDeleteOnClose();

        if (defaultReply.toLower() == "yes")
            msg->setDefaultButton(QMessageBox::Yes);
        else
            msg->setDefaultButton(QMessageBox::No);

        int reply = msg->exec();

        if(reply == QMessageBox::Yes)
            client->write("yes");
        else if (reply == QMessageBox::No)
            client->write("no");
        else
            client->write("fail");
    }

    else if (requestType == "userinputrequest")
    {
        QString text = request.attribute("text").value();
        bool mask = bool(QString(request.attribute("mask").value()).toInt());

        /* Open message box */
        FramelessInputDialog dlg;
        dlg.setSizeGripEnabled(false);
        dlg.setDpiScale(settings.getGlobalDpiScale());
        dlg.setWindowTitle(settings.getApplicationName());
        dlg.setLabelText(text);
        dlg.setInputMode(QInputDialog::TextInput);

        if (mask)
            dlg.setTextEchoMode(QLineEdit::Password);

        dlg.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        QString input;
        if (dlg.exec())
            input = dlg.textValue();

        client->write(input.toUtf8());
    }

    else if (requestType == "getopenfilerequest")
    {
        QString windowTitle = request.attribute("window-title").value();
        QString nameFilter = request.attribute("name-filter").value();
        QString directory = request.attribute("directory").value();

        /* Open file dialog */
        FramelessFileDialog fdlg;

        fdlg.setAcceptMode(QFileDialog::AcceptOpen);
        fdlg.setFileMode(QFileDialog::ExistingFile);

        if (!windowTitle.isEmpty())
            fdlg.setWindowTitle(windowTitle);

        if (!directory.isEmpty() && QDir(directory).exists())
            fdlg.setDirectory(directory);
        else
            fdlg.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));

        if (!nameFilter.isEmpty())
            fdlg.setNameFilter(nameFilter);

        /* Execute */
        QString openfile = "fail";
        if(fdlg.exec())
            openfile = QFileInfo(fdlg.selectedFiles().constFirst()).absoluteFilePath();

        client->write(openfile.toUtf8());
    }

    else if (requestType == "getsavefilerequest")
    {
        QString windowTitle = request.attribute("window-title").value();
        QString nameFilter = request.attribute("name-filter").value();
        QString directory = request.attribute("directory").value();

        QString defaultsuffix;

        static QRegularExpression expression;
        expression.setPattern("\\((.*)\\)");
        QRegularExpressionMatch match = expression.match(nameFilter);

        if (match.hasMatch())
        {
            QStringList exts = match.captured(1).remove("*").remove(",").remove(".").remove(";").split(" ", Qt::SkipEmptyParts);
            if (exts.length() > 0)
                defaultsuffix = exts.first();
        }

        /* Open file dialog */
        FramelessFileDialog fdlg;
        fdlg.setAcceptMode(QFileDialog::AcceptSave);
        fdlg.setFileMode(QFileDialog::AnyFile);

        if (!windowTitle.isEmpty())
            fdlg.setWindowTitle(windowTitle);

        if (!directory.isEmpty() && QDir(directory).exists())
            fdlg.setDirectory(directory);
        else
            fdlg.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

        if (!nameFilter.isEmpty())
            fdlg.setNameFilter(nameFilter);

        if (!defaultsuffix.isEmpty())
            fdlg.setDefaultSuffix(defaultsuffix);

        /* Execute */
        QString savefile = "fail";
        if(fdlg.exec())
            savefile = QFileInfo(fdlg.selectedFiles().constFirst()).absoluteFilePath();

        client->write(savefile.toUtf8());
    }

    else if (requestType == "statusupdaterequest")
    {
        QString status = request.attribute("status").value();
        int timeout = QString(request.attribute("time-out").value()).toInt();
        emit pyProcessStatusChanged(status, timeout);
    }

    else if (requestType == "taskkillrequest")
    {
        QString im = request.attribute("im").value();
        int pid = QString(request.attribute("pid").value()).toInt();
        bool atexit = QString(request.attribute("at-exit").value()).toLower().trimmed().replace("true","1").toInt();

        if (atexit)
            taskKillList.append(QPair<QString, int>(im, pid));
        else
        {
            if (terminateProcess(im, pid))
                client->write("succes");
            else
                client->write("fail");
        }
    }

    else if (requestType == "deletetaskkillrequest")
    {
        QString im = request.attribute("im").value();
        int pid = QString(request.attribute("pid").value()).toInt();
        taskKillList.removeAll(QPair<QString, int>(im, pid));
    }

    else if (requestType == "xmlreadrequest")
    {
        QFileInfo xmlfile(request.attribute("xmlfilepath").value());

        if (xmlfile.exists() && xmlfile.fileName().endsWith(".xml", Qt::CaseInsensitive))
        {
            emit readXml(xmlfile.absoluteFilePath());
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            client->write("succes");
        }
        else
            client->write("fail");
    }

    else if (requestType == "xmlwriterequest")
    {
        QString xmlfile = request.attribute("xmlfilepath").value();
        emit writeXml(QFileInfo(xmlfile).absoluteFilePath());
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        client->write("succes");
    }

    else if (requestType == "enableprintsrequest")
    {
        printsEnabled = true;
    }

    else if (requestType == "disableprintsrequest")
    {
        printsEnabled = false;
    }

    else if (requestType == "printregularrequest")
    {
        emit printRegular();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        client->write("succes");
    }

    else if (requestType == "printboldrequest")
    {
        emit printBold();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        client->write("succes");
    }

    else if (requestType == "printcursiverequest")
    {

        emit printCursive();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        client->write("succes");
    }

    else if (requestType == "printboldcursiverequest")
    {
        emit printBoldCursive();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        client->write("succes");
    }

    else if (requestType == "increaseindentrequest")
    {
        emit increaseIndent();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        client->write("succes");
    }

    else if (requestType == "decreaseindentrequest")
    {
        emit decreaseIndent();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        client->write("succes");
    }

    else if (requestType == "resetindentrequest")
    {
        emit resetIndent();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        client->write("succes");
    }

    else if (requestType == "restartmodulerequest")
    {
        connect(this, &PyProcess::pyProcessFinished, pyTools, &PyTools::startModule, Qt::UniqueConnection);
        client->write("succes");
        client->disconnectFromServer();
        return;
    }

    /* Disconnect */
    if (client->state() != QLocalSocket::UnconnectedState && client->isOpen())
        if (!client->waitForDisconnected(2500))
            client->disconnectFromServer();
}

void PyProcess::readStandardOutput()
{
    setCurrentReadChannel(QProcess::StandardOutput);

    while (canReadLine())
    {
        if (printsEnabled)
            emit readyReadPyProcessOutput(readLine());
        else
            readLine();
    }
}

void PyProcess::readStandardError()
{
    QString error;
    setCurrentReadChannel(QProcess::StandardError);

    while (canReadLine())
    {
        if (printsEnabled)
            error += readLine();
        else
            readLine();
    }

    if (error.trimmed().length() > 1 && printsEnabled)
    {
        showErrorMessage(error);
        emit readyReadPyProcessError(error);
    }
}

void PyProcess::showErrorMessage(QString text)
{
    /* Show message box */
    if (text.contains("Exception:", Qt::CaseInsensitive))
    {
        errorTermination = true;

        qsizetype i = text.length() - text.indexOf("Exception:", 0, Qt::CaseInsensitive);
        QString line = text.right(i).remove("Exception:", Qt::CaseInsensitive).trimmed();

        foreach(auto handle, handles)
            if (line == handle.second)
            {
                handle.first->close();
                handle.first->deleteLater();
            }

        FramelessMessageBox *msg = new FramelessMessageBox(QMessageBox::Critical, settings.getApplicationName(), "Exception:", QMessageBox::Ok);
        msg->setAutoDeleteOnClose();
        msg->setInformativeText(line);
        QPair<QMessageBox*,QString> msgpair = qMakePair(msg,line);
        handles.push_back(msgpair);
        connect(msg, &QMessageBox::destroyed, this, [this,msgpair](){ removeMsgBoxHandle(msgpair);});
        msg->show();
        msg->raise();
    }
    else if (text.contains("Error:", Qt::CaseInsensitive))
    {
        errorTermination = true;

        qsizetype i = text.length() - text.indexOf("Error:", 0, Qt::CaseInsensitive);
        QString line = text.right(i).remove("Error:", Qt::CaseInsensitive).trimmed();

        foreach(auto handle, handles)
            if (line == handle.second)
            {
                handle.first->close();
                handle.first->deleteLater();
            }

        FramelessMessageBox *msg = new FramelessMessageBox(QMessageBox::Critical, settings.getApplicationName(), "Error:", QMessageBox::Ok);
        msg->setAutoDeleteOnClose();
        msg->setInformativeText(line);
        QPair<QMessageBox*,QString> msgpair = qMakePair(msg,line);
        handles.push_back(msgpair);
        connect(msg, &QMessageBox::destroyed, this, [this,msgpair](){ removeMsgBoxHandle(msgpair);});
        msg->show();
        msg->raise();
    }
    else if (text.contains("Warning:", Qt::CaseInsensitive))
    {
        QStringList lines;
        while (text.contains("Warning:", Qt::CaseInsensitive))
        {
            qsizetype i = text.lastIndexOf("Warning:", -1, Qt::CaseInsensitive);
            QString line = text.right(text.length() - i).remove("Warning:", Qt::CaseInsensitive).trimmed();
            text = text.left(i);
            lines.prepend(line);
        }

        for (const QString &line: lines)
        {
            foreach(auto handle, handles)
                if (line == handle.second)
                {
                    handle.first->close();
                    handle.first->deleteLater();
                }

            FramelessMessageBox *msg = new FramelessMessageBox(QMessageBox::Warning, settings.getApplicationName(), "Warning:", QMessageBox::Ok);
            msg->setAutoDeleteOnClose();
            msg->setInformativeText(line);
            QPair<QMessageBox*,QString> msgpair = qMakePair(msg,line);
            handles.push_back(msgpair);
            connect(msg, &QMessageBox::destroyed, this, [this,msgpair](){ removeMsgBoxHandle(msgpair);});
            msg->show();
            msg->raise();
        }
    }
    else if (text.contains("Information:", Qt::CaseInsensitive))
    {
        QStringList lines;
        while (text.contains("Information:", Qt::CaseInsensitive))
        {
            qsizetype i = text.lastIndexOf("Information:", -1, Qt::CaseInsensitive);
            QString line = text.right(text.length() - i).remove("Information:", Qt::CaseInsensitive).trimmed();
            text = text.left(i);
            lines.prepend(line);
        }

        for (const QString &line: lines)
        {
            foreach(auto handle, handles)
                if (line == handle.second)
                {
                    handle.first->close();
                    handle.first->deleteLater();
                }

            FramelessMessageBox *msg = new FramelessMessageBox(QMessageBox::Warning, settings.getApplicationName(), "Information:", QMessageBox::Ok);
            msg->setAutoDeleteOnClose();
            msg->setInformativeText(line);
            QPair<QMessageBox*,QString> msgpair = qMakePair(msg,line);
            handles.push_back(msgpair);
            connect(msg, &QMessageBox::destroyed, this, [this,msgpair](){ removeMsgBoxHandle(msgpair);});
            msg->show();
            msg->raise();
        }
    }

    while (handles.length() > 6)
    {
        QPair<QMessageBox*,QString> handle = handles.takeFirst();
        handle.first->close();
        handle.first->deleteLater();
    }
}
