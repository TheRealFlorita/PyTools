#include <PyTools.h>
#include <QDirIterator>
#include <QMimeData>
#include <QTimer>
#include <DocumentationViewer.h>
#include <ModuleInstaller.h>
#include <FramelessFileDialog.h>
#include <PyProcess.h>
#include <PyDock.h>


PyTools::PyTools(QWidget *parent)  : FramelessMainWindow(parent)
{
    setAcceptDrops(true);
    setObjectName("PyTools");

    setWindowTitle(settings.getApplicationName());
    connect(this, &PyTools::dpiScaleChanged, &settings, &Settings::setGlobalDpiScale);

    /* Install translators */
    QApplication::installTranslator(&qtTranslator);
    QApplication::installTranslator(&ptTranslator);

    /* Central widget */
    xmlApp = new XmlApplication(this);
    xmlApp->setMinimumWidth(200);
    connect(this, &PyTools::dpiScaleChanged, xmlApp, &XmlApplication::updateDpiScale);
    setCentralWidget(xmlApp);

    /* Create dock widget */
    pyDock = new PyDock(this);
    pyDock->setMinimumWidth(200);
    connect(pyDock, &PyDock::runButtonClicked, this, &PyTools::runButtonClicked);
    connect(pyDock->process(), &PyProcess::readXml, this, &PyTools::openSession);
    connect(pyDock->process(), &PyProcess::writeXml, this, &PyTools::saveSession);
    connect(this, &PyTools::dpiScaleChanged, pyDock, &PyDock::updateDpiScale);

    /* Create status bar */
    statusBar = new HdStatusBar(this);
    statusBar->setDynamicHeight(settings.getLabelHeight(statusBar)+4);
    pyDock->setStatusBar(statusBar);
    connect(this, &PyTools::dpiScaleChanged, statusBar, &HdStatusBar::updateDpiScale);

    /* Create progress bar */
    progressBar = new HdProgressBar(this);
    progressBar->setDynamicSize(120, statusBar->getDynamicHeight());
    pyDock->setProgressBar(progressBar);
    connect(this, &PyTools::dpiScaleChanged, progressBar, &HdProgressBar::updateDpiScale);

    /* Move status bar and progress bar to toolbar */
    statusWidget = new HdToolBar(this);
    statusWidget->setStyleSheet("HdToolBar {background-color: transparent;}");
    statusWidget->setDynamicHeight(statusBar->getDynamicHeight());
    connect(this, &PyTools::dpiScaleChanged, statusWidget, &HdToolBar::updateDpiScale);

    statusWidget->addWidget(statusBar);
    statusWidget->addWidget(progressBar);
    addToolBar(Qt::BottomToolBarArea,statusWidget);
    statusWidget->setFloatable(false);
    statusWidget->setMovable(false);

    /* Create menu's */
    createSessionMenu();
    createModulesMenu();
    createActionsMenu();
    createHelpMenu();

    /* Add action Close */
    QAction *actionClose = new QAction(PyTools::tr("Close"), this);
    actionClose->setShortcut(QKeySequence("Alt+F4"));
    connect(actionClose, &QAction::triggered, this, &PyTools::close);
    connect(this, &PyTools::languageChanged, actionClose, [actionClose](){ actionClose->setText(PyTools::tr("Close")); });
    mainMenu()->addAction(actionClose);

    /* Look for installed modules */
    initializeModules();

    /* Load module */
    QString first_session = settings.getApplicationPath() + "/FirstSession.xml";
    QString last_session = settings.getAppDataPath() + "/LastSession.xml";
    if (QFile::exists(last_session))
        QTimer::singleShot(0, this, [this, last_session](){openSession(last_session);});
    else if (QFile::exists(first_session))
        QTimer::singleShot(0, this, [this, first_session](){openSession(first_session);});

    /* Create folder and file */
    if (!QFile::exists(last_session))
        settings.save();

    connect(this, &PyTools::screenChanged, this, &PyTools::onScreenChanged);
}

PyTools::~PyTools()
{
    if (fileDialog != Q_NULLPTR)
        fileDialog->deleteLater();
}

void PyTools::initializeModules()
{
    /* Remove old actions */
    const QList<QAction *> mactions = modulesMenu->actions();
    for (QAction *maction: mactions)
        if (!maction->data().toString().isEmpty())
            maction->deleteLater();

    /* Find modules and add actions */
    QMap<QString, QString> modules;
    QStringList paths = {settings.getApplicationPath()+"/modules", settings.getAppDataPath()+"/modules"};
    for (QString &path: paths)
    {
        QDirIterator it(path, {"*.module.xml", "*.modules.xml"}, QDir::AllEntries, QDirIterator::Subdirectories);
        while (it.hasNext())
        {
            QString xmlpath = it.next();

            /* Read file */
            pugi::xml_document tmp;
            QFile file(xmlpath);
            if (file.open(QIODevice::ReadOnly))
            {
                tmp.load_string(file.readAll().toStdString().c_str());
                file.close();
            }

            pugi::xpath_node_set module_nodes = tmp.select_nodes("./application/modules/*[self::module or self::expandable-module]");
            for (size_t i = 0; i < module_nodes.size(); ++i)
                modules[module_nodes[i].node().attribute("name").value()] = xmlpath;
        }
    }

    QStringList keys = modules.keys();
    for (QString &name: keys)
    {
        QAction *action = new QAction(QString(name).replace("&", "&&"), modulesMenu);
        action->setData(modules[name]);
        connect(action, &QAction::triggered, this, &PyTools::pyModuleTriggered);
        modulesMenu->addAction(action);
    }
}

void PyTools::loadActions()
{
    /* Remove old actions */
    const QList<QAction *> mactions = actionsMenu->actions();
    for (QAction *maction: mactions)
        if (maction->data().toString().isEmpty())
            maction->deleteLater();

    /* Add new actions */
    QList<pugi::xml_node> actions = xmlApp->getActionNodes().values();
    for (pugi::xml_node &node: actions)
    {
        PyAction *action = new PyAction(actionsMenu, node);
        connect(action, &PyAction::triggered, this, &PyTools::pyActionTriggered);

        if (actionsMenu->actions().length() > 0)
            actionsMenu->insertAction(actionsMenu->actions().constFirst(), action);
        else
            actionsMenu->addAction(action);
    }

    if (xmlApp->getActionNodes().size() == 0)
        actionsMenu->addAction(new QAction("     ", actionsMenu));
}

void PyTools::createSessionMenu()
{
    /* Create menu */
    sessionMenu = new QMenu(PyTools::tr("Session"), this);
    sessionMenu->setWindowFlag(Qt::NoDropShadowWindowHint);
    connect(this, &PyTools::languageChanged, [this](){ sessionMenu->setTitle(PyTools::tr("Session")); });
    sessionMenu->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    mainMenu()->addMenu(sessionMenu);

    /* Add action Reset */
    QAction *actionReset = new QAction(PyTools::tr("Reset"), this);
    actionReset->setShortcut(QKeySequence("Ctrl+N"));
    connect(actionReset, &QAction::triggered, this, &PyTools::resetSessionTriggered);
    connect(this, &PyTools::languageChanged, actionReset, [actionReset](){ actionReset->setText(PyTools::tr("Reset")); });
    sessionMenu->addAction(actionReset);

    /* Add action Open */
    QAction *actionOpen = new QAction(PyTools::tr("Open"), this);
    actionOpen->setShortcut(QKeySequence("Ctrl+O"));
    connect(actionOpen, &QAction::triggered, this, &PyTools::openSessionTriggered);
    connect(this, &PyTools::languageChanged, actionOpen, [actionOpen](){ actionOpen->setText(PyTools::tr("Open")); });
    sessionMenu->addAction(actionOpen);

    /* Add action Save */
    QAction *actionSave = new QAction(PyTools::tr("Save"), this);
    actionSave->setShortcut(QKeySequence("Ctrl+S"));
    connect(actionSave, &QAction::triggered, this, &PyTools::saveSessionTriggered);
    connect(this, &PyTools::languageChanged, actionSave, [actionSave](){ actionSave->setText(PyTools::tr("Save")); });
    sessionMenu->addAction(actionSave);

    /* Add action Restore previous session */
    QAction *restore = new QAction(PyTools::tr("Restore previous session"), this);
    restore->setShortcut(QKeySequence("Ctrl+R"));
    connect(restore, &QAction::triggered, restore, [this](){if (QFile::exists(settings.getAppDataPath() + "/LastSession.xml"))
                                                                openSession(settings.getAppDataPath() + "/LastSession.xml"); });
    connect(this, &PyTools::languageChanged, restore, [restore](){ restore->setText(PyTools::tr("Restore previous session")); });
    sessionMenu->addAction(restore);
}

void PyTools::createModulesMenu()
{
    /* Create menu */
    modulesMenu = new QMenu(PyTools::tr("Modules"), this);
    modulesMenu->setWindowFlag(Qt::NoDropShadowWindowHint);
    connect(this, &PyTools::languageChanged, [this](){ modulesMenu->setTitle(PyTools::tr("Modules")); });
    modulesMenu->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    mainMenu()->addMenu(modulesMenu);

    /* Add action Install module */
    QAction *install = new QAction(PyTools::tr("Install module..."), this);
    connect(install, &QAction::triggered, this, &PyTools::installModuleTriggered);
    connect(this, &PyTools::languageChanged, install, [install](){ install->setText(PyTools::tr("Install module...")); });
    modulesMenu->addAction(install);

    /* Add action Browse modules */
    QAction *mbrowse = new QAction(PyTools::tr("Browse modules..."), this);
    connect(mbrowse, &QAction::triggered, mbrowse, [](){ QProcess::startDetached("explorer", QStringList() << QString(settings.getAppDataPath()+"/modules").replace("/","\\")); });
    connect(this, &PyTools::languageChanged, mbrowse, [mbrowse](){ mbrowse->setText(PyTools::tr("Browse modules...")); });
    modulesMenu->addAction(mbrowse);

    /* Add action Browse examples */
    QAction *ebrowse = new QAction(PyTools::tr("Browse examples..."), this);
    connect(ebrowse, &QAction::triggered, ebrowse, [](){ QProcess::startDetached("explorer", QStringList() << QString(settings.getAppDataPath()+"/examples").replace("/","\\")); });
    connect(this, &PyTools::languageChanged, ebrowse, [ebrowse](){ ebrowse->setText(PyTools::tr("Browse examples...")); });
    modulesMenu->addAction(ebrowse);

    modulesMenu->addSeparator();
}

void PyTools::createActionsMenu()
{
    /* Create menu */
    actionsMenu = new QMenu(PyTools::tr("Actions"), this);
    actionsMenu->setWindowFlag(Qt::NoDropShadowWindowHint);
    connect(this, &PyTools::languageChanged, actionsMenu, [this](){ actionsMenu->setTitle(PyTools::tr("Actions")); });
    actionsMenu->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    mainMenu()->addMenu(actionsMenu);

    actionsMenu->addAction(new QAction("      ", actionsMenu));
}

void PyTools::createHelpMenu()
{
    /* Create Qt menu */
    helpMenu()->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

    /* Create Application Documentation action */
    QAction *helpAction1 = new QAction(PyTools::tr("Documentation"), this);
    connect(this, &PyTools::languageChanged, helpAction1, [helpAction1](){ helpAction1->setText(settings.getValue("application/name").toString() + " " + PyTools::tr("documentation")); });
    connect(helpAction1, &QAction::triggered, this, [this](){DocumentationViewer::openDocumentation(settings.getValue("application/documentation").toString());});
    helpAction1->setShortcut(QKeySequence("F1"));
    helpMenu()->addAction(helpAction1);

    /* Create Module Documentation action */
    QAction *helpAction2 = new QAction(PyTools::tr("Documentation"), this);
    connect(this, &PyTools::languageChanged, helpAction2, [this, helpAction2](){ helpAction2->setText(xmlApp->currentModule()->getName() + " " + PyTools::tr("documentation")); });
    connect(this, &PyTools::moduleChanged, helpAction2, [this, helpAction2](){ helpAction2->setEnabled(xmlApp->currentModule()->hasDocumentation()); });
    connect(helpAction2, &QAction::triggered, this, [this](){DocumentationViewer::openDocumentation(xmlApp->currentModule()->getDocumentationFilePath());});
    helpAction2->setShortcut(QKeySequence("F2"));
    helpMenu()->addAction(helpAction2);

    helpMenu()->addSeparator();

    /* Create About me action */
    QAction *aboutAction = new QAction(PyTools::tr("About me"), this);
    connect(this, &PyTools::languageChanged, aboutAction, [aboutAction](){ aboutAction->setText(PyTools::tr("About me")); });
    connect(aboutAction, &QAction::triggered, this, &PyTools::aboutMe);
    helpMenu()->addAction(aboutAction);

    /* Create About Qt dialog */
    QAction *aboutQt = new QAction(PyTools::tr("About Qt"), this);
    connect(this, &PyTools::languageChanged, aboutQt, [aboutQt](){ aboutQt->setText(PyTools::tr("About Qt")); });
    connect(aboutQt, &QAction::triggered, this, [this](){ FramelessMessageBox::aboutQt(this, PyTools::tr("About Qt")); });
    helpMenu()->addAction(aboutQt);

    /* Create Qt Source Code dialog */
    QAction *sourceQt = new QAction(PyTools::tr("Qt source"), this);
    connect(this, &PyTools::languageChanged, sourceQt, [sourceQt](){ sourceQt->setText(PyTools::tr("Qt source")); });
    connect(sourceQt, &QAction::triggered, this, &PyTools::openQtSourceWindow);
    helpMenu()->addAction(sourceQt);
}

void PyTools::openQtSourceWindow()
{
    FramelessMessageBox msg;
    msg.setWindowTitle("Qt source");
    msg.setText("<h3>Qt source</h3>");
    msg.setInformativeText("<p>You can download or recompile the Qt libraries for your platform. Please follow the instructions on:</p>"
                           "<p><a href='https://doc.qt.io/qt-6/build-sources.html'>https://doc.qt.io/qt-6/build-sources.html</a></p>");
    QIcon logo(settings.getApplicationPath() + "/icons/qt_logo.svg");
    msg.setIconPixmap(logo.pixmap(qRound(dpi*64), qRound(dpi*64)));
    msg.exec();
}

void PyTools::aboutMe()
{
    FramelessMessageBox msg;
    msg.setWindowTitle(PyTools::tr("About me"));

    QString cpversion = QString::number(__GNUC__) + "." +\
                        QString::number(__GNUC_MINOR__) + "." + \
                        QString::number(__GNUC_PATCHLEVEL__);
    QString pyversion = PyProcess::getEmbeddedPythonVersion();
    QString qwkversion = "1.4.1";

    if (pyversion.split(" ").length() < 2 || !pyversion.startsWith("Py", Qt::CaseInsensitive))
        pyversion.clear();
    else if (pyversion.contains("32 bit", Qt::CaseInsensitive))
        pyversion = "Python " + pyversion.split(" ").at(1) + " (32 bit)";
    else if (pyversion.contains("64 bit", Qt::CaseInsensitive))
        pyversion = "Python " + pyversion.split(" ").at(1) + " (64 bit)";
    else
        pyversion.clear();

    QString szversion = ModuleInstaller::get7ZipVersion();

    QString txt = "<h3>About me</h3>"
                  "<p>PyTools is a free (open source) software.</p>"
                  "<p>The source code of PyTools is released under the terms of the<br>"
                  "GNU General Public License version 3 (GPLv3).</p>";
    msg.setText(txt);

    QString inf = "<p>The program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE "
                  "WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.</p>"
                  "<p>Qt ships with PyTools under the LGPL Open Source license which confers "
                  "various rights to you as the user, including the right to recompile the Qt "
                  "libraries for your platform. To do this, please follow the instructions on the "
                  "Qt website.</p>"
                  "<p>This software uses:<br>"
                  " •  Qt %1 (MinGW - 64 bit)<br>"
                  " •  %2<br>"
                  " •  Pugixml %3.%4.%5<br>"
                  " •  %6<br>"
                  " •  QWindowKit %7 (by stdware)</p>"
                  "<p>This software is built with MinGW-w64 %8</p>";

    msg.setInformativeText(inf.arg(QString(QT_VERSION_STR),
                                   pyversion,
                                   QString::number(PUGIXML_VERSION / 1000),
                                   QString::number((PUGIXML_VERSION % 1000) / 10),
                                   QString::number((PUGIXML_VERSION % 1000) % 10),
                                   szversion,
                                   qwkversion,
                                   cpversion));

    msg.exec();
}

void PyTools::loadXmlFile(QString filepath, QString name)
{
    QString last_session = QFileInfo(settings.getAppDataPath() + "/LastSession.xml").absoluteFilePath();
    QString temp_session = QFileInfo(settings.getAppDataPath() + "/TempSession.xml").absoluteFilePath();

    if ((xmlApp->isValid()) && (QFileInfo(filepath).absoluteFilePath().toLower() == last_session.toLower()))
        xmlApp->saveCurrentModule(temp_session);
    else if (xmlApp->isValid())
        xmlApp->saveCurrentModule(last_session);

    if (!xmlApp->loadXmlFile(filepath))
    {
        if (xmlApp->loadXmlFile(last_session))
            this->loadXmlFile(last_session);
        else
            this->loadXmlFile(QFileInfo(settings.getApplicationPath() + "/FirstSession.xml").absoluteFilePath());
    }

    if (QFileInfo(filepath).absoluteFilePath().toLower() == last_session.toLower())
    {
        QElapsedTimer timer;
        timer.start();
        while (QFile::exists(last_session) && timer.elapsed() <= 250)
            QFile::remove(last_session);

        QFile::rename(temp_session, last_session);

        timer.restart();
        while (QFile::exists(temp_session) && timer.elapsed() <= 250)
            QFile::remove(temp_session);
    }

    xmlApp->selectModule(name);

    loadActions();

    if (!xmlApp->currentModule()->getName().isEmpty())
        setWindowTitle(settings.getApplicationName() + " - " + xmlApp->currentModule()->getName());
    else
        setWindowTitle(settings.getApplicationName());

    /* Set application language */
    bool ok = false;

    if (qtTranslator.load(QString(settings.getApplicationPath() + "/translations/qt_%1.qm").arg(xmlApp->getLanguageCode())))
        ok = true;

    if (ptTranslator.load(QString(settings.getApplicationPath() + "/translations/pt_%1.qm").arg(xmlApp->getLanguageCode())))
        ok = true;

    if (ok)
        emit languageChanged();

    emit moduleChanged();

    settings.setPythonPath(xmlApp->getPythonPath());
}

void PyTools::resetSessionTriggered()
{
    xmlApp->currentModule()->reset();
}

void PyTools::openSessionTriggered()
{
    createFileDialog();

    fileDialog->setWindowTitle(PyTools::tr("Open XML file"));
    fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog->setFileMode(QFileDialog::ExistingFile);
    fileDialog->setNameFilter(QString("%1 (*.xml)").arg(PyTools::tr("XML file")));

    if (fileDialog->exec())
        openSession(fileDialog->selectedFiles().constFirst());
}

void PyTools::saveSessionTriggered()
{
    createFileDialog();

    fileDialog->setWindowTitle(PyTools::tr("Save XML file"));
    fileDialog->setAcceptMode(QFileDialog::AcceptSave);
    fileDialog->setFileMode(QFileDialog::AnyFile);
    fileDialog->setNameFilter(QString("%1 (*.xml)").arg(PyTools::tr("XML file")));

    if (fileDialog->exec())
        saveSession(fileDialog->selectedFiles().constFirst());
}

void PyTools::pyModuleTriggered()
{
    QAction *action = static_cast<QAction *>(sender());
    loadXmlFile(action->data().toString(), action->text());
}

void PyTools::installModule(QString archive)
{
    QStringList extensions = {"7z", "zip", "rar"};
    if (extensions.contains(archive.split(".").last().trimmed().toLower()))
        if (ModuleInstaller::installModule(QFileInfo(archive).absoluteFilePath()))
            initializeModules();
}

void PyTools::installModuleTriggered()
{
    createFileDialog();

    fileDialog->setWindowTitle(PyTools::tr("Select archive file"));
    fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog->setFileMode(QFileDialog::ExistingFile);
    fileDialog->setNameFilter(QString("%1 (*.7z *.zip *.rar)").arg(PyTools::tr("Archive file")));

    if (fileDialog->exec())
        installModule(fileDialog->selectedFiles().constFirst());
}

void PyTools::openSession(QString filepath)
{
    if (filepath.endsWith(".xml", Qt::CaseInsensitive))
        loadXmlFile(filepath);
}

void PyTools::saveSession(QString filepath)
{
    if (filepath.endsWith(".xml", Qt::CaseInsensitive))
        xmlApp->saveCurrentModule(filepath);
}

void PyTools::pyActionTriggered()
{
    PyAction *action = static_cast<PyAction *>(sender());

    if (action->getRunSilent())
    {
        PyProcess* process = new PyProcess(this);
        connect(process, &PyProcess::pyProcessFinished, process, &PyProcess::deleteLater);
        process->startPyProcess({QFileInfo(action->getFilePath()).absoluteFilePath()}, xmlApp->currentModule()->toString());
    }
    else
        pyDock->process()->startPyProcess({QFileInfo(action->getFilePath()).absoluteFilePath()}, xmlApp->currentModule()->toString());
}

void PyTools::runButtonClicked(bool checked)
{
    if (checked && !pyDock->process()->isRunning())
        startModule();
    else if (!checked)
        stopModule();
}

void PyTools::startModule()
{
    pyDock->process()->startPyProcess(xmlApp->currentModule()->getFilePath(), xmlApp->currentModule()->toString());
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void PyTools::stopModule()
{
    pyDock->process()->killPyProcess();
}

void PyTools::createFileDialog()
{
    if (fileDialog == Q_NULLPTR)
    {
        fileDialog = new FramelessFileDialog();
        fileDialog->setDirectory(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        connect(this, &PyTools::dpiScaleChanged, fileDialog, &FramelessFileDialog::updateDpiScale);
    }
}

void PyTools::onScreenChanged()
{
    int height = settings.getLabelHeight(statusBar)+4;
    statusWidget->setDynamicHeight(height);
    statusBar->setDynamicHeight(height);
    progressBar->setDynamicSize(120, height);
}

void PyTools::dragEnterEvent(QDragEnterEvent * event)
{
    if(event->mimeData()->hasUrls())
        event->accept();
}

void PyTools::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasUrls())
    {
        QList<QUrl> urls = event->mimeData()->urls();
        if (urls.size() >= 1)
        {
            bool firstxml = true;

            for (QUrl &url: urls)
            {
                QString filepath = url.toLocalFile().toLower();
                if (filepath.endsWith(".7z") || filepath.endsWith(".zip") || filepath.endsWith(".rar"))
                    installModule(url.toLocalFile());
                else if ((filepath.endsWith(".xml")) && firstxml)
                {
                    openSession(url.toLocalFile());
                    firstxml = false;
                }
            }
        }
  }
}

void PyTools::showEvent(QShowEvent *event)
{
    FramelessMainWindow::showEvent(event);
    QTimer::singleShot(0, this, [this](){ setDpiScale(getDpiScale()); });
}

void PyTools::resizeEvent(QResizeEvent* event)
{
    FramelessMainWindow::resizeEvent(event);
    pyDock->resizeToRatio();
}

void PyTools::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F5)
        startModule();
    else if (event->key() == Qt::Key_F6)
        stopModule();
    else if (event->key() == Qt::Key_F7)
        QProcess::startDetached(settings.getEditorFilePath(), {xmlApp->currentModule()->getFilePath()});
    else if (event->key() == Qt::Key_F8)
        settings.openSettingsFileInEditor();
    else if (event->key() == Qt::Key_F11)
        pyDock->setFloating(!pyDock->isFloating());
    else if (event->key() == Qt::Key_F12)
        pyDock->resetDock();

    FramelessMainWindow::keyPressEvent(event);
}

void PyTools::closeEvent(QCloseEvent *event)
{
    double scale = getDpiScale() / (screen()->logicalDotsPerInchX() / 96.0);
    scale = std::min(100, std::max(1, qRound(scale/getDpiStepSize()))) * getDpiStepSize();
    settings.setGlobalDpiScale(scale);

    emit settings.aboutToQuit();

    FramelessMainWindow::closeEvent(event);
}
