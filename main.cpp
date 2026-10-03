#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>

// 全局日志文件
static QFile *g_logFile = nullptr;
static QTextStream *g_logStream = nullptr;

static void myMessageHandler(QtMsgType type,
                             const QMessageLogContext &,
                             const QString &msg)
{
    if (!g_logStream) return;

    QString level;
    switch (type) {
    case QtDebugMsg:    level = QStringLiteral("DEBUG");   break;
    case QtInfoMsg:     level = QStringLiteral("INFO");    break;
    case QtWarningMsg:  level = QStringLiteral("WARN");    break;
    case QtCriticalMsg: level = QStringLiteral("CRIT");    break;
    case QtFatalMsg:    level = QStringLiteral("FATAL");   break;
    }

    (*g_logStream) << QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"))
                   << " [" << level << "] " << msg << "\n";
    g_logStream->flush();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // ===== 日志初始化 =====
    // 在 exe 同目录下建 log 文件夹
    const QString logDirPath = QCoreApplication::applicationDirPath()
                               + QStringLiteral("/log");
    QDir logDir(logDirPath);

    // 如果 log 文件夹已存在，先清空里面的文件
    if (logDir.exists()) {
        logDir.removeRecursively();
    }
    logDir.mkpath(logDirPath);

    // 打开本次运行的日志文件（用时间戳命名）
    const QString logFileName = QStringLiteral("luoying_%1.log")
                                    .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    g_logFile = new QFile(logDirPath + QStringLiteral("/") + logFileName);
    if (g_logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        g_logStream = new QTextStream(g_logFile);
    }

    qInstallMessageHandler(myMessageHandler);

    qInfo() << "=== Luoying started ===";
    qInfo() << "Log file:" << logFileName;
    // ====================

    MainWindow *w = new MainWindow();
    w->show();

    const int ret = a.exec();

    qInfo() << "=== Luoying exited with code" << ret << "===";

    if (g_logStream) {
        g_logStream->flush();
        delete g_logStream;
        g_logStream = nullptr;
    }
    if (g_logFile) {
        g_logFile->close();
        delete g_logFile;
        g_logFile = nullptr;
    }

    return ret;
}