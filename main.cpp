#include <QFontDatabase>
#include <QDirIterator>
#include <PyTools.h>

Settings settings;

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor);

    /* Application */
    QApplication a(argc, argv);
    QStringList arguments = QApplication::arguments();

    /* Load settings immediately after creating application */
    settings.load();

    /* Install application fonts */
    QDirIterator fontfiles(settings.getApplicationPath() + "/fonts/", QStringList() << "*.ttf" << "*.otf", QDir::Files);
    while (fontfiles.hasNext())
        QFontDatabase::addApplicationFont(fontfiles.next());

    /* Font settings */
    QFont font;
    font.setStyleStrategy(QFont::StyleStrategy(QFont::PreferAntialias));
    font.setHintingPreference(QFont::PreferDefaultHinting);
    QApplication::setFont(font);

    /* Create main window */
    PyTools w;
    w.show();

    /* Load arguments */
    if (arguments.size() > 1)
    {
        QString argument = QString(arguments.at(1)).remove("`").trimmed();
        if (!argument.isEmpty() && QFile::exists(argument))
        {
            if (argument.endsWith(".xml", Qt::CaseInsensitive))
                w.openSession(argument);
            else if (argument.endsWith(".zip", Qt::CaseInsensitive) ||
                     argument.endsWith(".rar", Qt::CaseInsensitive) ||
                     argument.endsWith(".7z", Qt::CaseInsensitive))
                w.installModule(argument);
        }
    }
    
    return a.exec();
}
