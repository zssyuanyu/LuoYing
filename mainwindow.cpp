#include "mainwindow.h"
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QUrl>
#include <QHBoxLayout>
#include <QWidget>
#include <QProcess>
#include <QTimer>
#include <QTcpSocket>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include <QLabel>
#include <QScrollBar>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

static constexpr int kMaxHistoryMessages = 20;
static constexpr int kServerPort = 8080;

// ============ 消息气泡 ============

class MessageBubble : public QWidget
{
public:
    MessageBubble(const QString &text, bool isUser, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(16, 4, 16, 4);
        layout->setSpacing(0);

        m_label = new QLabel(this);
        m_label->setTextFormat(Qt::PlainText);
        m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_label->setWordWrap(true);

        if (isUser) {
            m_label->setStyleSheet(QStringLiteral(
                "QLabel {"
                "  background-color: #3a6ea5;"
                "  color: #ffffff;"
                "  border-radius: 12px;"
                "  padding: 8px 16px;"
                "  font-size: 14px;"
                "}"));
        } else {
            m_label->setStyleSheet(QStringLiteral(
                "QLabel {"
                "  background-color: #2a2a2a;"
                "  color: #e0e0e0;"
                "  border-radius: 12px;"
                "  padding: 8px 16px;"
                "  font-size: 14px;"
                "}"));
        }

        applyText(text);

        if (isUser) {
            layout->addStretch(1);
            layout->addWidget(m_label, 0, Qt::AlignTop);
        } else {
            layout->addWidget(m_label, 0, Qt::AlignTop);
            layout->addStretch(1);
        }
    }

    void setText(const QString &text)
    {
        applyText(text);
    }

private:
    void applyText(const QString &text)
    {
        m_label->ensurePolished();

        m_label->setMinimumWidth(0);
        m_label->setMaximumWidth(QWIDGETSIZE_MAX);
        m_label->setWordWrap(false);
        m_label->setText(text);

        m_label->adjustSize();
        const int naturalWidth = m_label->sizeHint().width();
        const int maxBubbleWidth = 640;

        if (naturalWidth <= maxBubbleWidth) {
            m_label->setFixedWidth(naturalWidth);
        } else {
            m_label->setMinimumWidth(0);
            m_label->setMaximumWidth(maxBubbleWidth);
            m_label->setWordWrap(true);
        }
    }

    QLabel *m_label = nullptr;
};

// ============ MainWindow ============

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("络樱 - 本地 Agent"));
    resize(900, 700);

    setStyleSheet(QStringLiteral("QMainWindow { background-color: #1a1a1a; }"));

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(central);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { background-color: #1a1a1a; border: none; }"
        "QScrollBar:vertical {"
        "  background: #1a1a1a;"
        "  width: 8px;"
        "  margin: 0;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #3a3a3a;"
        "  border-radius: 4px;"
        "  min-height: 30px;"
        "}"
        "QScrollBar::handle:vertical:hover { background: #4a4a4a; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
        ));

    m_messageContainer = new QWidget();
    m_messageContainer->setStyleSheet(QStringLiteral("background-color: #1a1a1a;"));
    m_messageLayout = new QVBoxLayout(m_messageContainer);
    m_messageLayout->setContentsMargins(0, 12, 0, 12);
    m_messageLayout->setSpacing(0);
    m_messageLayout->addStretch(1);

    m_scrollArea->setWidget(m_messageContainer);
    mainLayout->addWidget(m_scrollArea, 1);

    QWidget *inputWidget = new QWidget(central);
    inputWidget->setStyleSheet(QStringLiteral("background-color: #1a1a1a;"));
    QHBoxLayout *inputLayout = new QHBoxLayout(inputWidget);
    inputLayout->setContentsMargins(16, 8, 16, 16);
    inputLayout->setSpacing(10);

    m_input = new QLineEdit(inputWidget);
    m_input->setPlaceholderText(QStringLiteral("输入消息..."));
    m_input->setMinimumHeight(40);
    m_input->setStyleSheet(QStringLiteral(
        "QLineEdit {"
        "  background-color: #2a2a2a;"
        "  color: #e0e0e0;"
        "  border: 1px solid #3a3a3a;"
        "  border-radius: 20px;"
        "  padding: 0 16px;"
        "  font-size: 14px;"
        "}"
        "QLineEdit:focus { border: 1px solid #3a6ea5; }"
        ));

    m_sendBtn = new QPushButton(QStringLiteral("发送"), inputWidget);
    m_sendBtn->setCursor(Qt::PointingHandCursor);
    m_sendBtn->setFixedHeight(40);
    m_sendBtn->setMinimumWidth(80);
    m_sendBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #3a6ea5;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 20px;"
        "  padding: 0 24px;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover { background-color: #4a7eb5; }"
        "QPushButton:pressed { background-color: #2a5e95; }"
        "QPushButton:disabled { background-color: #3a3a3a; color: #888888; }"
        ));

    inputLayout->addWidget(m_input, 1);
    inputLayout->addWidget(m_sendBtn);
    mainLayout->addWidget(inputWidget);

    m_manager = new QNetworkAccessManager(this);

    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::sendMessage);
    connect(m_input, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);

    addMessage(QStringLiteral("络樱"), QStringLiteral("正在启动本地推理引擎..."));
    startServer();
}

MainWindow::~MainWindow()
{
    if (m_serverProcess && m_ownServer
        && m_serverProcess->state() != QProcess::NotRunning) {
        m_serverProcess->terminate();
        if (!m_serverProcess->waitForFinished(5000)) {
            m_serverProcess->kill();
            m_serverProcess->waitForFinished(2000);
        }
    }
}

// ============ 服务管理 ============

bool MainWindow::isServerRunning()
{
    QTcpSocket socket;
    socket.connectToHost(QStringLiteral("127.0.0.1"), kServerPort);
    if (socket.waitForConnected(300)) {
        socket.disconnectFromHost();
        return true;
    }
    return false;
}

QString MainWindow::findServerPath()
{
    const QString p1 = QCoreApplication::applicationDirPath()
    + QStringLiteral("/llama-server.exe");
    if (QFile::exists(p1)) return p1;

    const QString p2 = QStringLiteral(
        "D:/C++_python_html/C++/llama.cpp/llama.cpp-master/build/bin/Release/llama-server.exe");
    if (QFile::exists(p2)) return p2;

    return QString();
}

QString MainWindow::findModelPath()
{
    const QString p1 = QCoreApplication::applicationDirPath()
    + QStringLiteral("/models/Qwen2.5-7B-Instruct-Q4_K_M.gguf");
    if (QFile::exists(p1)) return p1;

    const QString p2 = QStringLiteral(
        "D:/C++_python_html/C++/llama.cpp/llama.cpp-master/models/Qwen2.5-7B-Instruct-Q4_K_M.gguf");
    if (QFile::exists(p2)) return p2;

    return QString();
}

QString MainWindow::findWorkDir()
{
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 3; ++i) {
        if (QFile::exists(dir.absoluteFilePath(QStringLiteral("CMakeLists.txt")))) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return QCoreApplication::applicationDirPath();
}

void MainWindow::startServer()
{
    if (isServerRunning()) {
        m_ownServer = false;
        m_serverReady = true;
        m_sendBtn->setEnabled(true);
        addMessage(QStringLiteral("络樱"), QStringLiteral("你好，我是络樱。有什么可以帮你的？"));
        return;
    }

    const QString serverPath = findServerPath();
    const QString modelPath = findModelPath();

    if (serverPath.isEmpty()) {
        addMessage(QStringLiteral("络樱"),
                   QStringLiteral("[错误] 找不到 llama-server.exe"));
        return;
    }
    if (modelPath.isEmpty()) {
        addMessage(QStringLiteral("络樱"),
                   QStringLiteral("[错误] 找不到模型文件"));
        return;
    }

    m_serverProcess = new QProcess(this);
    m_serverProcess->setWorkingDirectory(QFileInfo(serverPath).absolutePath());

    QStringList args;
    args << QStringLiteral("-m") << modelPath
         << QStringLiteral("--host") << QStringLiteral("127.0.0.1")
         << QStringLiteral("--port") << QString::number(kServerPort)
         << QStringLiteral("-t") << QStringLiteral("4")
         << QStringLiteral("-c") << QStringLiteral("4096")
         << QStringLiteral("--jinja");

    m_ownServer = true;
    m_serverProcess->start(serverPath, args);

    m_sendBtn->setEnabled(false);
    m_sendBtn->setText(QStringLiteral("加载中..."));

    m_readyCheckCount = 0;
    m_readyTimer = new QTimer(this);
    connect(m_readyTimer, &QTimer::timeout, this, &MainWindow::checkServerReady);
    m_readyTimer->start(500);
}

void MainWindow::checkServerReady()
{
    ++m_readyCheckCount;

    if (m_readyCheckCount > 120) {
        m_readyTimer->stop();
        m_serverReady = true;
        m_sendBtn->setEnabled(true);
        m_sendBtn->setText(QStringLiteral("发送"));
        addMessage(QStringLiteral("络樱"), QStringLiteral("[错误] 模型加载超时"));
        return;
    }

    if (m_serverProcess && m_serverProcess->state() == QProcess::NotRunning) {
        m_readyTimer->stop();
        m_serverReady = true;
        m_sendBtn->setEnabled(true);
        m_sendBtn->setText(QStringLiteral("发送"));
        addMessage(QStringLiteral("络樱"), QStringLiteral("[错误] llama-server 启动失败"));
        return;
    }

    if (isServerRunning()) {
        m_readyTimer->stop();
        m_serverReady = true;
        m_sendBtn->setEnabled(true);
        m_sendBtn->setText(QStringLiteral("发送"));
        addMessage(QStringLiteral("络樱"), QStringLiteral("你好，我是络樱。有什么可以帮你的？"));
    }
}

// ============ 系统信息 ============

QString MainWindow::getSystemInfo()
{
    QStringList lines;

#ifdef Q_OS_WIN
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    lines << QStringLiteral("CPU 逻辑核心数: %1").arg(sysInfo.dwNumberOfProcessors);

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        const double totalGB = memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
        const double availGB = memInfo.ullAvailPhys / (1024.0 * 1024.0 * 1024.0);
        lines << QStringLiteral("总内存: %1 GB").arg(totalGB, 0, 'f', 1);
        lines << QStringLiteral("可用内存: %1 GB").arg(availGB, 0, 'f', 1);
    }

    lines << QStringLiteral("磁盘空间:");
    const DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if (!(drives & (1 << i))) continue;

        const wchar_t letter = static_cast<wchar_t>(L'A' + i);
        wchar_t rootPath[4] = { letter, L':', L'\\', L'\0' };

        const UINT type = GetDriveTypeW(rootPath);
        if (type != DRIVE_FIXED) continue;

        ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
        if (GetDiskFreeSpaceExW(rootPath, &freeBytes, &totalBytes, &totalFreeBytes)) {
            const double totalGB = totalBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
            const double freeGB = freeBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
            lines << QStringLiteral("  %1盘: 总 %2 GB, 可用 %3 GB")
                         .arg(QChar(letter))
                         .arg(totalGB, 0, 'f', 1)
                         .arg(freeGB, 0, 'f', 1);
        }
    }
#else
    lines << QStringLiteral("系统信息查询仅支持 Windows");
#endif

    return lines.join(QStringLiteral("\n"));
}

// ============ 命令白名单 ============

bool MainWindow::isCommandAllowed(const QString &cmd) const
{
    const QString lower = cmd.toLower().trimmed();

    static const QStringList blacklist = {
        QStringLiteral("format"),
        QStringLiteral("del "),
        QStringLiteral("erase "),
        QStringLiteral("rmdir"),
        QStringLiteral("rd "),
        QStringLiteral("rm "),
        QStringLiteral("shutdown"),
        QStringLiteral("restart"),
        QStringLiteral("reg delete"),
        QStringLiteral("reg add"),
        QStringLiteral("taskkill"),
        QStringLiteral("net user"),
        QStringLiteral("netsh"),
        QStringLiteral("bcdedit"),
        QStringLiteral("diskpart"),
        QStringLiteral("cipher"),
        QStringLiteral("takeown"),
        QStringLiteral("icacls"),
        QStringLiteral("attrib"),
        QStringLiteral("move "),
        QStringLiteral("ren "),
        QStringLiteral("copy "),
    };

    for (const QString &bad : blacklist) {
        if (lower.contains(bad)) {
            return false;
        }
    }

    static const QStringList whitelist = {
        QStringLiteral("dir"),
        QStringLiteral("type"),
        QStringLiteral("where"),
        QStringLiteral("ipconfig"),
        QStringLiteral("systeminfo"),
        QStringLiteral("ver"),
        QStringLiteral("hostname"),
        QStringLiteral("whoami"),
        QStringLiteral("tasklist"),
        QStringLiteral("ping"),
        QStringLiteral("tracert"),
        QStringLiteral("nslookup"),
        QStringLiteral("echo"),
    };

    for (const QString &good : whitelist) {
        if (lower.startsWith(good)) {
            return true;
        }
    }

    return false;
}

// ============ 工具定义 ============

QJsonArray MainWindow::buildTools()
{
    QJsonArray tools;

    {
        QJsonObject tool;
        tool[QStringLiteral("type")] = QStringLiteral("function");

        QJsonObject fn;
        fn[QStringLiteral("name")] = QStringLiteral("get_current_time");
        fn[QStringLiteral("description")] =
            QStringLiteral("获取当前系统日期和时间。当用户询问现在几点、今天几号等问题时调用。");

        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = QJsonObject();
        params[QStringLiteral("required")] = QJsonArray();
        fn[QStringLiteral("parameters")] = params;

        tool[QStringLiteral("function")] = fn;
        tools.append(tool);
    }

    {
        QJsonObject tool;
        tool[QStringLiteral("type")] = QStringLiteral("function");

        QJsonObject fn;
        fn[QStringLiteral("name")] = QStringLiteral("read_file");
        fn[QStringLiteral("description")] =
            QStringLiteral("读取络樱工作目录下的文本文件内容。当用户要求查看某个文件时调用。");

        QJsonObject props;
        QJsonObject pathProp;
        pathProp[QStringLiteral("type")] = QStringLiteral("string");
        pathProp[QStringLiteral("description")] = QStringLiteral("相对于络樱工作目录的文件路径");
        props[QStringLiteral("path")] = pathProp;

        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] = QJsonArray{QStringLiteral("path")};
        fn[QStringLiteral("parameters")] = params;

        tool[QStringLiteral("function")] = fn;
        tools.append(tool);
    }

    {
        QJsonObject tool;
        tool[QStringLiteral("type")] = QStringLiteral("function");

        QJsonObject fn;
        fn[QStringLiteral("name")] = QStringLiteral("list_directory");
        fn[QStringLiteral("description")] =
            QStringLiteral("列出络樱工作目录下的文件和子目录。当用户要求查看有哪些文件时调用。");

        QJsonObject props;
        QJsonObject pathProp;
        pathProp[QStringLiteral("type")] = QStringLiteral("string");
        pathProp[QStringLiteral("description")] =
            QStringLiteral("相对于工作目录的路径，空字符串表示根目录");
        props[QStringLiteral("path")] = pathProp;

        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] = QJsonArray();
        fn[QStringLiteral("parameters")] = params;

        tool[QStringLiteral("function")] = fn;
        tools.append(tool);
    }

    {
        QJsonObject tool;
        tool[QStringLiteral("type")] = QStringLiteral("function");

        QJsonObject fn;
        fn[QStringLiteral("name")] = QStringLiteral("get_system_info");
        fn[QStringLiteral("description")] =
            QStringLiteral("获取系统信息，包括 CPU 核心数、内存使用情况、所有固定磁盘的空间。"
                           "当用户询问系统状态、剩余内存、磁盘空间等问题时调用。");

        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = QJsonObject();
        params[QStringLiteral("required")] = QJsonArray();
        fn[QStringLiteral("parameters")] = params;

        tool[QStringLiteral("function")] = fn;
        tools.append(tool);
    }

    {
        QJsonObject tool;
        tool[QStringLiteral("type")] = QStringLiteral("function");

        QJsonObject fn;
        fn[QStringLiteral("name")] = QStringLiteral("write_file");
        fn[QStringLiteral("description")] =
            QStringLiteral("在络樱工作目录下创建或覆盖一个文本文件。当用户要求写入、保存、创建文件时调用。");

        QJsonObject props;
        QJsonObject pathProp;
        pathProp[QStringLiteral("type")] = QStringLiteral("string");
        pathProp[QStringLiteral("description")] = QStringLiteral("相对于工作目录的文件路径");
        props[QStringLiteral("path")] = pathProp;

        QJsonObject contentProp;
        contentProp[QStringLiteral("type")] = QStringLiteral("string");
        contentProp[QStringLiteral("description")] = QStringLiteral("要写入的文本内容");
        props[QStringLiteral("content")] = contentProp;

        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] =
            QJsonArray{QStringLiteral("path"), QStringLiteral("content")};
        fn[QStringLiteral("parameters")] = params;

        tool[QStringLiteral("function")] = fn;
        tools.append(tool);
    }

    {
        QJsonObject tool;
        tool[QStringLiteral("type")] = QStringLiteral("function");

        QJsonObject fn;
        fn[QStringLiteral("name")] = QStringLiteral("create_directory");
        fn[QStringLiteral("description")] =
            QStringLiteral("在络樱工作目录下创建一个子目录。当用户要求创建文件夹时调用。");

        QJsonObject props;
        QJsonObject pathProp;
        pathProp[QStringLiteral("type")] = QStringLiteral("string");
        pathProp[QStringLiteral("description")] = QStringLiteral("相对于工作目录的目录路径");
        props[QStringLiteral("path")] = pathProp;

        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] = QJsonArray{QStringLiteral("path")};
        fn[QStringLiteral("parameters")] = params;

        tool[QStringLiteral("function")] = fn;
        tools.append(tool);
    }

    {
        QJsonObject tool;
        tool[QStringLiteral("type")] = QStringLiteral("function");

        QJsonObject fn;
        fn[QStringLiteral("name")] = QStringLiteral("execute_command");
        fn[QStringLiteral("description")] =
            QStringLiteral("执行只读类的 Windows 命令，例如 dir、type、where、ipconfig、systeminfo、tasklist 等。"
                           "当用户要求查看目录内容、网络信息、运行进程等时调用。"
                           "不支持删除、修改、格式化等破坏性命令。");

        QJsonObject props;
        QJsonObject cmdProp;
        cmdProp[QStringLiteral("type")] = QStringLiteral("string");
        cmdProp[QStringLiteral("description")] =
            QStringLiteral("要执行的命令，例如 dir 或 ipconfig");
        props[QStringLiteral("command")] = cmdProp;

        QJsonObject params;
        params[QStringLiteral("type")] = QStringLiteral("object");
        params[QStringLiteral("properties")] = props;
        params[QStringLiteral("required")] = QJsonArray{QStringLiteral("command")};
        fn[QStringLiteral("parameters")] = params;

        tool[QStringLiteral("function")] = fn;
        tools.append(tool);
    }

    return tools;
}

QString MainWindow::executeTool(const QString &name, const QJsonObject &args)
{
    const QString workDir = findWorkDir();

    if (name == QStringLiteral("get_current_time")) {
        return QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }

    if (name == QStringLiteral("get_system_info")) {
        return getSystemInfo();
    }

    if (name == QStringLiteral("read_file")) {
        const QString rel = args[QStringLiteral("path")].toString();
        const QString full = QDir(workDir).absoluteFilePath(rel);

        if (!full.startsWith(workDir)) {
            return QStringLiteral("[错误] 路径越界，只能访问络樱工作目录内的文件");
        }

        QFile f(full);
        if (!f.exists()) {
            return QStringLiteral("[错误] 文件不存在: ") + rel;
        }
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QStringLiteral("[错误] 无法打开文件: ") + rel;
        }

        const QByteArray data = f.read(8192);
        const bool truncated = !f.atEnd();
        f.close();

        QString result = QString::fromUtf8(data);
        if (truncated) {
            result += QStringLiteral("\n...[文件被截断，只显示了前 8KB]");
        }
        if (result.isEmpty()) {
            return QStringLiteral("[空文件]");
        }
        return result;
    }

    if (name == QStringLiteral("list_directory")) {
        const QString rel = args[QStringLiteral("path")].toString();
        const QString full = QDir(workDir).absoluteFilePath(rel);

        if (!full.startsWith(workDir)) {
            return QStringLiteral("[错误] 路径越界");
        }

        QDir dir(full);
        if (!dir.exists()) {
            return QStringLiteral("[错误] 目录不存在: ") + rel;
        }

        const QFileInfoList entries = dir.entryInfoList(
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
            QDir::Name);

        if (entries.isEmpty()) {
            return QStringLiteral("[空目录]");
        }

        QStringList lines;
        for (const QFileInfo &info : entries) {
            QString line = info.fileName();
            if (info.isDir()) {
                line += QStringLiteral("/");
            } else {
                line += QStringLiteral("  (%1 字节)").arg(info.size());
            }
            lines << line;
        }
        return lines.join(QStringLiteral("\n"));
    }

    if (name == QStringLiteral("write_file")) {
        const QString rel = args[QStringLiteral("path")].toString();
        const QString content = args[QStringLiteral("content")].toString();
        const QString full = QDir(workDir).absoluteFilePath(rel);

        if (!full.startsWith(workDir)) {
            return QStringLiteral("[错误] 路径越界，只能写入络樱工作目录内的文件");
        }

        QFile f(full);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            return QStringLiteral("[错误] 无法写入文件: ") + rel;
        }
        const QByteArray data = content.toUtf8();
        const qint64 written = f.write(data);
        f.close();

        if (written != data.size()) {
            return QStringLiteral("[错误] 写入不完整，可能磁盘已满");
        }
        return QStringLiteral("已写入 %1 字节到 %2").arg(written).arg(rel);
    }

    if (name == QStringLiteral("create_directory")) {
        const QString rel = args[QStringLiteral("path")].toString();
        const QString full = QDir(workDir).absoluteFilePath(rel);

        if (!full.startsWith(workDir)) {
            return QStringLiteral("[错误] 路径越界，只能在络樱工作目录内创建目录");
        }

        QDir dir;
        if (dir.mkpath(full)) {
            return QStringLiteral("已创建目录: ") + rel;
        }
        return QStringLiteral("[错误] 创建目录失败: ") + rel;
    }

    if (name == QStringLiteral("execute_command")) {
        const QString cmd = args[QStringLiteral("command")].toString().trimmed();

        if (!isCommandAllowed(cmd)) {
            return QStringLiteral("[错误] 命令被拒绝。只允许执行只读类命令"
                                  "（如 dir、type、where、ipconfig、systeminfo、tasklist）。");
        }

        QProcess proc;
        proc.setWorkingDirectory(workDir);
        proc.start(QStringLiteral("cmd.exe"),
                   QStringList{QStringLiteral("/c"), cmd});
        proc.waitForFinished(10000);

        const QByteArray out = proc.readAllStandardOutput();
        const QByteArray err = proc.readAllStandardError();

        QString result = QString::fromLocal8Bit(out);
        if (!err.isEmpty()) {
            result += QStringLiteral("\n[stderr]\n") + QString::fromLocal8Bit(err);
        }
        if (result.isEmpty()) {
            result = QStringLiteral("[命令无输出]");
        }
        if (result.size() > 4000) {
            result = result.left(4000) + QStringLiteral("\n...[输出被截断]");
        }
        return result;
    }

    return QStringLiteral("未知工具: ") + name;
}

// ============ 对话 ============

void MainWindow::addMessage(const QString &role, const QString &text)
{
    const bool isUser = (role == QStringLiteral("你"));
    auto *bubble = new MessageBubble(text, isUser, m_messageContainer);
    m_messageLayout->insertWidget(m_messageLayout->count() - 1, bubble);
    scrollToBottom();
}

void MainWindow::updateCurrentBubble(const QString &text)
{
    if (!m_currentBubble.isNull()) {
        m_currentBubble->setText(text);
        scrollToBottom();
    }
}

void MainWindow::scrollToBottom()
{
    QTimer::singleShot(50, this, [this]() {
        if (!m_scrollArea) return;
        QScrollBar *bar = m_scrollArea->verticalScrollBar();
        bar->setValue(bar->maximum());
    });
}

QJsonArray MainWindow::buildMessages()
{
    QJsonArray messages;

    QJsonObject sysMsg;
    sysMsg[QStringLiteral("role")] = QStringLiteral("system");
    sysMsg[QStringLiteral("content")] = m_systemPrompt;
    messages.append(sysMsg);

    const int total = m_history.size();
    const int startIdx = (total > kMaxHistoryMessages)
                             ? (total - kMaxHistoryMessages)
                             : 0;
    for (int i = startIdx; i < total; ++i) {
        messages.append(m_history.at(i));
    }

    return messages;
}

void MainWindow::sendMessage()
{
    if (!m_currentReply.isNull() || !m_serverReady) {
        return;
    }

    const QString text = m_input->text().trimmed();
    if (text.isEmpty()) {
        return;
    }

    addMessage(QStringLiteral("你"), text);
    m_input->clear();

    QJsonObject userMsg;
    userMsg[QStringLiteral("role")] = QStringLiteral("user");
    userMsg[QStringLiteral("content")] = text;
    m_history.append(userMsg);

    m_toolRound = 0;

    // 新建一个空的 AI 气泡，用于流式更新
    m_currentBubble = new MessageBubble(QString(), false, m_messageContainer);
    m_messageLayout->insertWidget(m_messageLayout->count() - 1, m_currentBubble);
    m_assistantText.clear();
    m_contentBytes.clear();
    m_streamBuffer.clear();
    m_pendingToolCalls = QJsonArray();

    // 第一次请求走流式（如果模型决定调用工具，也能通过 SSE 发过来）
    sendRequest(true);
}

void MainWindow::sendRequest(bool stream)
{
    QJsonObject requestBody;
    requestBody[QStringLiteral("model")] = QStringLiteral("qwen2.5-7b-instruct");
    requestBody[QStringLiteral("messages")] = buildMessages();
    requestBody[QStringLiteral("tools")] = buildTools();
    requestBody[QStringLiteral("stream")] = stream;

    const QByteArray data = QJsonDocument(requestBody).toJson();

    QNetworkRequest request(QUrl(QStringLiteral("http://127.0.0.1:8080/v1/chat/completions")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    m_currentReply = m_manager->post(request, data);

    if (stream) {
        connect(m_currentReply, &QNetworkReply::readyRead,
                this, &MainWindow::onStreamReadyRead);
    }
    connect(m_currentReply, &QNetworkReply::finished,
            this, &MainWindow::onReplyFinished);
}

void MainWindow::onStreamReadyRead()
{
    if (m_currentReply.isNull()) {
        return;
    }

    m_streamBuffer.append(m_currentReply->readAll());

    int newlineIdx = -1;
    while ((newlineIdx = m_streamBuffer.indexOf('\n')) != -1) {
        QByteArray line = m_streamBuffer.left(newlineIdx);
        m_streamBuffer.remove(0, newlineIdx + 1);

        line = line.trimmed();
        if (line.isEmpty() || !line.startsWith("data:")) {
            continue;
        }

        const QByteArray payload = line.mid(5).trimmed();
        if (payload == "[DONE]") {
            continue;
        }

        QJsonParseError err{};
        const QJsonObject obj = QJsonDocument::fromJson(payload, &err).object();
        if (err.error != QJsonParseError::NoError) {
            continue;
        }

        const QJsonArray choices = obj[QStringLiteral("choices")].toArray();
        if (choices.isEmpty()) {
            continue;
        }

        const QJsonObject delta = choices[0].toObject()
                                      [QStringLiteral("delta")].toObject();

        // 1. 文本片段
        const QString piece = delta[QStringLiteral("content")].toString();
        if (!piece.isEmpty()) {
            m_assistantText += piece;
            updateCurrentBubble(m_assistantText);
        }

        // 2. tool_calls 片段
        const QJsonArray tcDelta = delta[QStringLiteral("tool_calls")].toArray();
        for (const QJsonValue &v : tcDelta) {
            const QJsonObject tc = v.toObject();
            const int idx = tc[QStringLiteral("index")].toInt();

            // 确保数组足够大
            while (m_pendingToolCalls.size() <= idx) {
                m_pendingToolCalls.append(QJsonObject());
            }

            QJsonObject acc = m_pendingToolCalls[idx].toObject();

            const QString id = tc[QStringLiteral("id")].toString();
            if (!id.isEmpty()) {
                acc[QStringLiteral("id")] = id;
            }
            const QString type = tc[QStringLiteral("type")].toString();
            if (!type.isEmpty()) {
                acc[QStringLiteral("type")] = type;
            }

            const QJsonObject fnDelta = tc[QStringLiteral("function")].toObject();
            QJsonObject fnAcc = acc[QStringLiteral("function")].toObject();

            const QString fnName = fnDelta[QStringLiteral("name")].toString();
            if (!fnName.isEmpty()) {
                fnAcc[QStringLiteral("name")] = fnName;
            }

            const QString argsPiece = fnDelta[QStringLiteral("arguments")].toString();
            if (!argsPiece.isEmpty()) {
                const QString prevArgs = fnAcc[QStringLiteral("arguments")].toString();
                fnAcc[QStringLiteral("arguments")] = prevArgs + argsPiece;
            }

            acc[QStringLiteral("function")] = fnAcc;
            m_pendingToolCalls[idx] = acc;
        }
    }
}

void MainWindow::processToolCalls(const QJsonArray &toolCalls)
{
    // 把 assistant 消息（带 tool_calls）加入历史
    QJsonObject assistantMsg;
    assistantMsg[QStringLiteral("role")] = QStringLiteral("assistant");

    QJsonArray cleanCalls;
    for (const QJsonValue &v : toolCalls) {
        const QJsonObject call = v.toObject();
        // 只保留必要字段
        QJsonObject clean;
        clean[QStringLiteral("id")] = call[QStringLiteral("id")];
        clean[QStringLiteral("type")] = QStringLiteral("function");
        clean[QStringLiteral("function")] = call[QStringLiteral("function")];
        cleanCalls.append(clean);
    }
    assistantMsg[QStringLiteral("tool_calls")] = cleanCalls;
    m_history.append(assistantMsg);

    // 执行每个工具
    for (const QJsonValue &v : toolCalls) {
        const QJsonObject call = v.toObject();
        const QString callId = call[QStringLiteral("id")].toString();
        const QJsonObject fn = call[QStringLiteral("function")].toObject();
        const QString fnName = fn[QStringLiteral("name")].toString();
        const QString fnArgsStr = fn[QStringLiteral("arguments")].toString();

        addMessage(QStringLiteral("络樱"),
                   QStringLiteral("[调用工具: %1]").arg(fnName));

        QJsonParseError err{};
        const QJsonObject fnArgs = QJsonDocument::fromJson(fnArgsStr.toUtf8(), &err).object();
        QString result;
        if (err.error != QJsonParseError::NoError) {
            result = QStringLiteral("[错误] 工具参数解析失败: ") + err.errorString();
        } else {
            result = executeTool(fnName, fnArgs);
        }

        QJsonObject toolMsg;
        toolMsg[QStringLiteral("role")] = QStringLiteral("tool");
        toolMsg[QStringLiteral("tool_call_id")] = callId;
        toolMsg[QStringLiteral("content")] = result;
        m_history.append(toolMsg);
    }

    // 再次请求，这次用非流式（工具结果通常不需要流式显示）
    // 但要新建一个气泡用于最终回复
    m_currentBubble = new MessageBubble(QString(), false, m_messageContainer);
    m_messageLayout->insertWidget(m_messageLayout->count() - 1, m_currentBubble);
    m_assistantText.clear();
    m_contentBytes.clear();
    m_streamBuffer.clear();
    m_pendingToolCalls = QJsonArray();

    sendRequest(true);
}

void MainWindow::onReplyFinished()
{
    if (m_currentReply.isNull()) {
        return;
    }

    // 处理缓冲区里最后一个不完整的行
    QByteArray remaining = m_streamBuffer.trimmed();
    m_streamBuffer.clear();
    if (!remaining.isEmpty() && remaining.startsWith("data:")) {
        const QByteArray payload = remaining.mid(5).trimmed();
        if (payload != "[DONE]") {
            QJsonParseError err{};
            const QJsonObject obj = QJsonDocument::fromJson(payload, &err).object();
            if (err.error == QJsonParseError::NoError) {
                const QJsonArray choices = obj[QStringLiteral("choices")].toArray();
                if (!choices.isEmpty()) {
                    const QJsonObject delta = choices[0].toObject()
                    [QStringLiteral("delta")].toObject();
                    const QString piece = delta[QStringLiteral("content")].toString();
                    if (!piece.isEmpty()) {
                        m_assistantText += piece;
                        updateCurrentBubble(m_assistantText);
                    }
                }
            }
        }
    }

    const auto netError = m_currentReply->error();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    // 处理工具调用
    if (!m_pendingToolCalls.isEmpty()) {
        // 如果气泡是空的，删掉它（工具调用会单独显示）
        if (m_assistantText.isEmpty() && !m_currentBubble.isNull()) {
            m_currentBubble->deleteLater();
            m_currentBubble = nullptr;
        }

        if (m_toolRound >= kMaxToolRounds) {
            addMessage(QStringLiteral("络樱"), QStringLiteral("[错误] 工具调用轮数超限"));
            return;
        }
        ++m_toolRound;

        const QJsonArray calls = m_pendingToolCalls;
        m_pendingToolCalls = QJsonArray();
        processToolCalls(calls);
        return;
    }

    // 普通文本回复
    if (!m_assistantText.isEmpty()) {
        QJsonObject assistantMsg;
        assistantMsg[QStringLiteral("role")] = QStringLiteral("assistant");
        assistantMsg[QStringLiteral("content")] = m_assistantText;
        m_history.append(assistantMsg);
        m_currentBubble = nullptr;
    } else if (netError != QNetworkReply::NoError
               && netError != QNetworkReply::RemoteHostClosedError) {
        // 忽略 llama-server 关闭连接的误报
        updateCurrentBubble(QStringLiteral("[错误] ") + m_currentReply->errorString());
    } else {
        updateCurrentBubble(QStringLiteral("[空回复]"));
    }
}