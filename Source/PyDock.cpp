#include <PyDock.h>

#include <QBoxLayout>
#include <QFileInfo>
#include <QTextCursor>
#include <QTextFrame>

#include <HdShadowEffect.h>
#include <PyProcess.h>
#include <PyTools.h>


PyDock::PyDock(PyTools *parent) : FramelessDockWidget(parent)
{
    pyTools = parent;

    /* Create tool bar */
    toolBar = new HdToolBar(this);
    setTitleBarWidget(toolBar);
    connect(this, &PyDock::dpiScaleChanged, toolBar, &HdToolBar::updateDpiScale);

    /* Create run button */
    runButton = new FramelessDockButton(FramelessDockButton::ButtonType::BUTTON_RUN, this);
    runButton->setCheckable(true);
    runButton->setChecked(false);
    connect(runButton, &FramelessDockButton::clicked, this, &PyDock::runButtonClicked);
    connect(this, &PyDock::dpiScaleChanged, runButton, &FramelessDockButton::updateDpiScale);

    /* Create toggle button for terminal */
    dockingButton = new FramelessDockButton(FramelessDockButton::ButtonType::BUTTON_DOCK, this);
    dockingButton->setCheckable(true);
    dockingButton->setChecked(false);
    connect(this, &PyDock::topLevelChanged, dockingButton, &FramelessDockButton::setChecked);
    connect(this, &PyDock::topLevelChanged, dockingButton, &FramelessDockButton::setHidden);
    connect(dockingButton, &FramelessDockButton::clicked, this, &FramelessDockWidget::setFloating);
    connect(this, &PyDock::dpiScaleChanged, dockingButton, &FramelessDockButton::updateDpiScale);

    /* Create spacer */
    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    spacer->setFixedHeight(0);

    /* Add to layout */
    toolBar->addWidget(runButton);
    toolBar->addWidget(spacer);
    toolBar->addWidget(dockingButton);

    /* Create QTextEdit for printing messages from process */
    terminal = new QTextEdit(this);
    terminal->setReadOnly(true);
    terminal->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    if (settings.wordWrapIsOn())
        terminal->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    else
        terminal->setWordWrapMode(QTextOption::NoWrap);
    terminal->setContentsMargins(0,0,0,0);
    terminal->setMinimumHeight(1);
    setWidget(terminal);
    connect(this, &PyDock::dpiScaleChanged, this, &PyDock::updateDpiScaleTerminal);

    /* Set tab width */
    terminal->ensurePolished();
    QFontMetrics metrics(terminal->font());
    terminal->setTabStopDistance(4*metrics.horizontalAdvance(" "));

    /* Add drop shadow effect */
    HdShadowEffect *effect = new HdShadowEffect(toolBar, qreal(1.0), qreal(12.0), settings.getColor("drop-shadow/color"));
    effect->setDistance(1);
    connect(this, &PyDock::dpiScaleChanged, effect, &HdShadowEffect::updateDpiScale);
    toolBar->setGraphicsEffect(effect);
    toolBar->raise();

    /* Restore dock location */
    Qt::DockWidgetArea area = Qt::DockWidgetArea(settings.getValue("settings/dock-location", int(Qt::RightDockWidgetArea)).toInt());
    if (area == Qt::NoDockWidgetArea)
        area = Qt::RightDockWidgetArea;
    parent->addDockWidget(area, this);

    /* Restore dock size */
    setRatio(settings.getValue("settings/dock-wratio", 0.5).toDouble(),
             settings.getValue("settings/dock-hratio", 0.2).toDouble());
    resizeToRatio();

    /* Set-up python process */
    pyProcess = new PyProcess(pyTools);
    connect(pyProcess, &PyProcess::pyProcessStarted, this, &PyDock::refreshRunButton);
    connect(pyProcess, &PyProcess::pyProcessCancelled, this, &PyDock::refreshRunButton);
    connect(pyProcess, &PyProcess::pyProcessFinished, this, &PyDock::refreshRunButton);
    connect(pyProcess, &PyProcess::pyProcessStarted, this, &PyDock::clearTerminal);
    connect(pyProcess, &PyProcess::readyReadPyProcessOutput, this, &PyDock::terminalPrint);
    connect(pyProcess, &PyProcess::readyReadPyProcessError, this, &PyDock::terminalErrorPrint);
    connect(pyProcess, &PyProcess::printRegular, this, &PyDock::printRegular);
    connect(pyProcess, &PyProcess::printBold, this, &PyDock::printBold);
    connect(pyProcess, &PyProcess::printCursive, this, &PyDock::printCursive);
    connect(pyProcess, &PyProcess::printBoldCursive, this, &PyDock::printBoldCursive);
    connect(pyProcess, &PyProcess::printProportionalFont, this, &PyDock::printProportional);
    connect(pyProcess, &PyProcess::printMonospaceFont, this, &PyDock::printMonospace);
    connect(pyProcess, &PyProcess::increaseIndent, this, &PyDock::increaseIndent);
    connect(pyProcess, &PyProcess::decreaseIndent, this, &PyDock::decreaseIndent);
    connect(pyProcess, &PyProcess::resetIndent, this, &PyDock::resetIndent);

    /* Finalise */
    setDynamicTitleBarHeight(settings.getTabBarHeight(parent));
    setTextColor(QColor(255,255,255));
    connect(&settings, &Settings::pythonPathChanged, this, [this](){ if (!pyProcess->isRunning()) clearTerminal();} );
    connect(&settings, &Settings::pythonPathChanged, this, [this](){ setFloating(false); resizeToRatio();} );
    connect(&settings, &Settings::aboutToQuit, this, &PyDock::deleteLater);
    connect(this, &PyDock::topLevelChanged, this, &PyDock::onScreenChanged);
    connect(this, &PyDock::screenChanged, this, &PyDock::onScreenChanged);
    emit dockingButton->toggled(dockingButton->isChecked());

    clearTerminal();
}

PyDock::~PyDock()
{
    pyProcess->killPyProcess();
    while (pyProcess->waitForFinished());
    pyProcess->deleteLater();

    settings.setValue("settings/dock-location", getParentMainWindow()->dockWidgetArea(this));
    settings.setValue("settings/dock-wratio", QString::number(getRatio().first, 'f', 6));
    settings.setValue("settings/dock-hratio", QString::number(getRatio().second, 'f', 6));
    settings.setValue("settings/word-wrap", (terminal->wordWrapMode() == QTextOption::WrapAtWordBoundaryOrAnywhere));
}

PyProcess* PyDock::process()
{
    return pyProcess;
}

void PyDock::refreshRunButton()
{
    runButton->setChecked(pyProcess->isRunning());
}

void PyDock::setTitleBarHeight(int h)
{
    dynamicBarHeight = false;
    h = std::max(12, h);

    toolBar->setFixedHeight(h);
    runButton->setFixedSize(h,h);
    dockingButton->setFixedSize(h,h);
}

void PyDock::setDynamicTitleBarHeight(int h)
{
    dynamicBarHeight = true;
    barHeight = std::max(12, h);

    toolBar->setDynamicHeight(h);
    runButton->setDynamicSize(h,h);
    dockingButton->setDynamicSize(h,h);
}

void PyDock::setStatusBar(HdStatusBar *statusbar)
{
    connect(pyProcess, &PyProcess::pyProcessStatusChanged, statusbar, &HdStatusBar::showMessage);
    connect(this, &PyDock::terminalCleared, statusbar, &HdStatusBar::clearMessage);
}

void PyDock::setProgressBar(HdProgressBar *progressbar)
{
    connect(pyProcess, &PyProcess::pyProcessStarted, progressbar, [progressbar](){progressbar->setRange(0,0);});
    connect(pyProcess, &PyProcess::pyProcessFinished, progressbar, [progressbar](){progressbar->setRange(0,1);});
}

void PyDock::setTextColor(QColor color)
{
    textColor = color;
}

void PyDock::clearTerminal()
{
    setWindowTitle(pyProcess->getPythonVersion(true));
    terminal->clear();
    setTextColor(QColor(190,190,190));
    terminalPrint(pyProcess->getPythonVersion(false) + "\r\n\r\n");
    setTextColor(QColor(255,255,255));

    indent = 0.0;
    bold = false;
    cursive = false;
    monospace = false;
    setBlockFormat();

    emit terminalCleared();
}

void PyDock::setBlockFormat()
{
    /* Move cursor and anchor to end */
    QTextCursor textCursor = terminal->textCursor();
    textCursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    textCursor.deletePreviousChar();

    QTextBlockFormat blockFormat = QTextBlockFormat(textCursor.block().blockFormat());
    blockFormat.setTopMargin(0.0);
    blockFormat.setBottomMargin(0.0);
    blockFormat.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
    blockFormat.setLeftMargin(indent);

    QTextCharFormat charformat = textCursor.block().charFormat();
    if (bold)
        charformat.setFontWeight(QFont::Bold);
    else
        charformat.setFontWeight(QFont::Normal);

    QFont font = charformat.font();
    if (monospace)
        font.setFamily(settings.getFontType("monospace"));
    else
        font.setFamily(settings.getFontType("regular"));
    charformat.setFont(font);

    charformat.setFontItalic(cursive);
    textCursor.setCharFormat(charformat);

    textCursor.insertBlock(blockFormat);
    textCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
    textCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    textCursor.insertText("");
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}


void PyDock::updateDpiScaleTerminal()
{
    QTextEdit dummy(this);
    dummy.setText("TeXtpad");

    if (dummy.fontPointSize() > 0)
    {
        terminal->selectAll();
        terminal->setFontPointSize(dummy.fontPointSize());

        QTextCursor textCursor = terminal->textCursor();
        textCursor.clearSelection();
        textCursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        terminal->setTextCursor(textCursor);
    }
}

void PyDock::printRegular()
{
    bold = false;
    cursive = false;
    setBlockFormat();
}

void PyDock::printBold()
{
    bold = true;
    cursive = false;
    setBlockFormat();
}

void PyDock::printCursive()
{
    bold = false;
    cursive = true;
    setBlockFormat();
}

void PyDock::printBoldCursive()
{
    bold = true;
    cursive = true;
    setBlockFormat();
}

void PyDock::printProportional()
{
    monospace = false;
    setBlockFormat();
}

void PyDock::printMonospace()
{
    monospace = true;
    setBlockFormat();
}

void PyDock::increaseIndent()
{
    indent += terminal->tabStopDistance();
    setBlockFormat();
}

void PyDock::decreaseIndent()
{
    indent = std::max(0.0, indent - terminal->tabStopDistance());
    setBlockFormat();
}


void PyDock::resetIndent()
{
    indent = 0.0;
    setBlockFormat();
}

void PyDock::terminalPrint(QString text)
{

    /* Move cursor and anchor to end */
    QTextCursor textCursor = terminal->textCursor();
    textCursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);

    terminal->setTextCursor(textCursor);

    terminal->setTextColor(textColor);
    terminal->insertPlainText(text);
    terminal->ensureCursorVisible();
}

void PyDock::terminalErrorPrint(QString text)
{
    resetIndent();

    /* Move cursor and anchor to end */
    QTextCursor textCursor = terminal->textCursor();
    textCursor = terminal->textCursor();
    textCursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    terminal->setTextCursor(textCursor);

    /* Set text color based on message type */
    if (text.contains("Exception:", Qt::CaseInsensitive))
    {
        int i = int(text.length() - text.indexOf("Exception:", 0, Qt::CaseInsensitive));
        QString line = "\nException:\n" + text.right(i).remove("Exception:", Qt::CaseInsensitive).trimmed();
        terminal->setTextColor(QColor(180,0,180));
        terminal->insertPlainText(line + "\n");
    }
    else if (text.contains("Error:", Qt::CaseInsensitive))
    {
        int i = int(text.length() - text.indexOf("Error:", 0, Qt::CaseInsensitive));
        QString line = "\nError:\n" + text.right(i).remove("Error:", Qt::CaseInsensitive).trimmed();
        terminal->setTextColor(QColor(220,0,0));
        terminal->insertPlainText(line + "\n");
    }
    else if (text.contains("Warning:", Qt::CaseInsensitive))
    {
        terminal->setTextColor(QColor(220,140,0));
        QString line;
        while (text.contains("Warning:", Qt::CaseInsensitive))
        {
            int i = int(text.lastIndexOf("Warning:", -1, Qt::CaseInsensitive));
            line.prepend("\nWarning:\n" + text.right(text.length() - i).remove("Warning:", Qt::CaseInsensitive).trimmed() + "\n");
            text = text.left(i).trimmed();
        }
        terminal->insertPlainText(line);
    }
    else if (text.contains("Information:", Qt::CaseInsensitive))
    {
        terminal->setTextColor(QColor(0,0,220));
        QString line;
        while (text.contains("Information:", Qt::CaseInsensitive))
        {
            int i = int(text.lastIndexOf("Information:", -1, Qt::CaseInsensitive));
            line.prepend("\nInformation:\n" + text.right(text.length() - i).remove("Information:", Qt::CaseInsensitive).trimmed() + "\n");
            text = text.left(i).trimmed();
        }
        terminal->insertPlainText(line);
    }

    /* Print full error message */
    if (!text.trimmed().isEmpty())
        terminal->insertPlainText("\n" + text.trimmed() + "\n");

    terminal->ensureCursorVisible();
}

void PyDock::onScreenChanged()
{
    if (dynamicBarHeight)
        setDynamicTitleBarHeight(settings.getTabBarHeight(this));
}

void PyDock::showEvent(QShowEvent *event)
{
    FramelessDockWidget::showEvent(event);
}

void PyDock::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Backspace)
        clearTerminal();

    FramelessDockWidget::keyPressEvent(event);
}
