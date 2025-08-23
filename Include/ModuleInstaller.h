#ifndef MODULEINSTALLER_H
#define MODULEINSTALLER_H

#include <QDebug>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QMessageBox>
#include <PyTools.h>
#include <Settings.h>

class ModuleInstaller : public QObject
{
    Q_OBJECT

public:

    static bool installModule(QString archive)
    {
        FramelessMessageBox msg(QMessageBox::Question, settings.getApplicationName(),
                                ModuleInstaller::tr("Are you sure you want to install this module?"),
                                QMessageBox::Yes | QMessageBox::No);
        msg.setInformativeText(QString("[%1]").arg(QFileInfo(archive).fileName()));
        msg.setDefaultButton(QMessageBox::No);

        int ret = msg.exec();

        if (ret == QMessageBox::No)
            return false;

        if (!testArchive(archive))
            return false;

        if (requiresElevation(archive) && !isAdmin())
        {
            FramelessMessageBox msg1(QMessageBox::Question, settings.getApplicationName(),
                                     ModuleInstaller::tr("The installation requires elevation. Do you want to restart the application with elevated permissions?"),
                                     QMessageBox::Yes | QMessageBox::No);
            msg1.setInformativeText(QString("[%1]").arg(QFileInfo(archive).fileName()));
            msg1.setDefaultButton(QMessageBox::No);

            int ret = msg1.exec();

            if (ret == QMessageBox::Yes)
                restartAsAdmin(archive);
            return false;
        }

        QString appdata = settings.getAppDataPath();
        QTemporaryDir tempdir;

        if (!tempdir.isValid())
            return false;

        if (!extractModule(archive, tempdir.path()))
            return false;

        if (!checkFilesForOverwrite(tempdir.path(), appdata))
            return false;

        tempdir.setAutoRemove(false);

        if (!copyDirectory(tempdir.path()+"/modules", appdata+"/modules"))
            return false;

        if (!copyDirectory(tempdir.path()+"/packages", appdata+"/packages"))
            return false;

        if (!copyDirectory(tempdir.path()+"/examples", appdata+"/examples"))
            return false;

        if (!runScript(tempdir.path()+"/post-install.bat"))
            return false;

        tempdir.setAutoRemove(true);

        FramelessMessageBox msg2(QMessageBox::Information, settings.getApplicationName(),
                                 ModuleInstaller::tr("Successfully installed"),
                                 QMessageBox::Ok);
        msg2.setInformativeText(QString("[%1]").arg(QFileInfo(archive).fileName()));
        msg2.exec();
        return true;
    }

    static QString get7ZipVersion()
    {
        QProcess process;
        process.setWorkingDirectory(QDir::currentPath());
        process.start(QFileInfo(settings.getApplicationPath() + "/7zip/7z.exe").absoluteFilePath(), QStringList());
        process.waitForFinished(5000);

        QString output = process.readAllStandardOutput().trimmed();
        QString error =  process.readAllStandardError().trimmed();
        process.close();

        if (error.length() == 0 && output.split(":", Qt::SkipEmptyParts).length() > 1)
            output = output.split(":", Qt::SkipEmptyParts).first().replace("x86", "32 bit").replace("x64", "64 bit");
        else
            qDebug() << "7zip error:" << error << output;
        return output.trimmed();
    }

private:

    static bool testArchive(QString archive)
    {
        /* Arguments list */
        QString program = QFileInfo(settings.getApplicationPath() + "/7zip/7z.exe").absoluteFilePath();
        QStringList args = {"t", QFileInfo(archive).absoluteFilePath()};

        /* Call 7zip */
        QProcess process;
        process.setWorkingDirectory(QDir::currentPath());
        process.start(program, args);

        while (process.state() == QProcess::Running)
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50);
        process.waitForFinished();

        QString out = process.readAllStandardOutput().trimmed();
        QString err =  process.readAllStandardError().trimmed();
        process.close();

        if (!out.contains("Everything is Ok", Qt::CaseInsensitive))
        {
            FramelessMessageBox msg(QMessageBox::Critical, settings.getApplicationName(),
                                    ModuleInstaller::tr("Archive is corrupted"),
                                    QMessageBox::Ok);
            msg.setInformativeText(QString("[%1]").arg(err));
            msg.exec();

            if (!err.isEmpty())
                qDebug() << err;

            QStringList lines = out.split("\r\n", Qt::SkipEmptyParts);
            foreach(QString line, lines)
                qDebug() << line;
            return false;
        }
        return true;
    }

    static bool requiresElevation(QString archive)
    {
        /* Arguments list */
        QString program = QFileInfo(settings.getApplicationPath() + "/7zip/7z.exe").absoluteFilePath();
        QStringList args = {"l", QFileInfo(archive).absoluteFilePath(), "elevate"};

        /* Call 7zip */
        QProcess process;
        process.setWorkingDirectory(QDir::currentPath());
        process.start(program, args);

        while (process.state() == QProcess::Running)
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50);
        process.waitForFinished();

        QString out = process.readAllStandardOutput().trimmed();
        process.close();

        QStringList lines = out.split("\r\n", Qt::SkipEmptyParts);
        foreach(QString line, lines)
            qDebug() << line.trimmed();

        if (out.endsWith("1 files", Qt::CaseInsensitive))
        {
            qDebug() << "requires elevation";
            return true;
        }
        return false;
    }

    static bool extractModule(QString archive, QString folder)
    {
        /* Arguments list */
        QString program = QFileInfo(settings.getApplicationPath() + "/7zip/7z.exe").absoluteFilePath();
        QStringList args = {"x", QFileInfo(archive).absoluteFilePath(), "-o"+QFileInfo(folder).absoluteFilePath()};

        /* Call 7zip */
        QProcess process;
        process.setWorkingDirectory(QDir::currentPath());
        process.start(program, args);

        while (process.state() == QProcess::Running)
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50);
        process.waitForFinished();

        QString err = process.readAllStandardError().trimmed();
        QStringList out = QString(process.readAllStandardOutput()).split("\r\n", Qt::SkipEmptyParts);
        process.close();

        foreach(QString line, out)
            qDebug() << line.trimmed();

        if (!err.isEmpty())
        {
            FramelessMessageBox msg(QMessageBox::Critical, settings.getApplicationName(),
                                    ModuleInstaller::tr("Extracting archive failed"),
                                    QMessageBox::Ok);
            msg.setInformativeText(QString("[%1]").arg(err));
            msg.exec();
            return false;
        }
        return true;
    }

    static bool checkFilesForOverwrite(QString source, QString destination)
    {
        if (!QDir(source).exists())
            return false;

        source = QDir(source).path();
        destination = QDir(destination).path();

        QDirIterator it(source, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            QFileInfo file = QFileInfo(it.next().replace(source, destination, Qt::CaseInsensitive));
            if (file.exists() && file.isFile())
            {
                FramelessMessageBox msg(QMessageBox::Question, settings.getApplicationName(),
                                        ModuleInstaller::tr("Overwrite file?"),
                                        QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel);
                msg.setInformativeText(QString("[%1]").arg(file.filePath()));
                msg.setDefaultButton(QMessageBox::Cancel);

                int ret = msg.exec();

                if (ret == QMessageBox::Cancel)
                {
                    FramelessMessageBox msg(QMessageBox::Information, settings.getApplicationName(),
                                            ModuleInstaller::tr("Installation cancelled"),
                                            QMessageBox::Ok);
                    msg.exec();
                    return false;
                }
                else if (ret == QMessageBox::YesToAll)
                    return true;
            }
        }
        return true;
    }

    static bool copyDirectory(QString source, QString destination)
    {
        QDir dir(source);
        if (!dir.exists())
            return true;

        source = dir.path();
        destination = QDir(destination).path();

        QDirIterator itdir(source, QDir::Dirs, QDirIterator::Subdirectories);
        while (itdir.hasNext())
        {
            QString directory = itdir.next().replace(source, destination, Qt::CaseInsensitive);
            if (!dir.mkpath(directory))
            {
                FramelessMessageBox msg(QMessageBox::Critical, settings.getApplicationName(),
                                        ModuleInstaller::tr("Creating directory failed"),
                                        QMessageBox::Ok);
                msg.setInformativeText(QString("[%1]").arg(directory));
                msg.exec();
                return false;
            }
        }

        QElapsedTimer timer;
        QDirIterator itfile(source, QDir::Files, QDirIterator::Subdirectories);
        while (itfile.hasNext())
        {
            QString file = itfile.next();
            QString copy = QString(file).replace(source, destination, Qt::CaseInsensitive);

            timer.restart();
            while (QFile::exists(copy) && timer.elapsed() < 1000)
                QFile::remove(copy);

            if (!QFile::copy(file, copy))
            {
                FramelessMessageBox msg(QMessageBox::Critical, settings.getApplicationName(),
                                        ModuleInstaller::tr("File copy failed"),
                                        QMessageBox::Ok);
                msg.setInformativeText(QString("[%1]").arg(copy));
                msg.exec();
                return false;
            }
        }
        return true;
    }

    static bool runScript(QString script)
    {
        if (!QFile::exists(script))
            return true;

        /* Arguments list */
        QString program = "cmd";
        QStringList args = {"/C", QFileInfo(script).absoluteFilePath()};

        /* Call cmd */
        QProcess process;
        process.setWorkingDirectory(QDir::currentPath());
        process.start(program, args);

        while (process.state() == QProcess::Running)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        process.waitForFinished();

        QString err = process.readAllStandardError().trimmed();
        qDebug() << process.readAllStandardOutput().trimmed();
        process.close();

        if (!err.isEmpty())
        {
            FramelessMessageBox msg(QMessageBox::Critical, settings.getApplicationName(),
                                    ModuleInstaller::tr("Installation script error"),
                                    QMessageBox::Ok);
            msg.setInformativeText(QString("[%1]").arg(err));
            msg.exec();
            return false;
        }
        return true;
    }

    static bool canWriteToInstallationDirectory()
    {
        QFile test(settings.getApplicationPath()+"/test");
        bool canWrite = test.open(QFile::WriteOnly);

        if (canWrite)
            test.remove();
        return canWrite;
    }

    static bool isAdmin()
    {
        QProcess powershell;
        powershell.start("powershell", {"if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {Write-Output 'false'; Exit} else {Write-Output 'true'; Exit} Exit"});
        powershell.waitForFinished();
        return QVariant(powershell.readAll().trimmed()).toBool(); // trim necessary
    }

    static void restartAsAdmin(QString archive)
    {
        emit settings.aboutToQuit();
        QProcess::startDetached("powershell", {"-Command", "Start-Process", "'"+QApplication::applicationFilePath()+"'", "'`\""+archive+"`\"'" ,"-Verb", "RunAs"});
        QApplication::quit();
    }
};

#endif
