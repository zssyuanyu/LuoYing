#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QJsonArray>
#include <QJsonObject>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

class QProcess;
class QTimer;
class MessageBubble;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void sendMessage();
    void onStreamReadyRead();
    void onReplyFinished();
    void checkServerReady();

private:
    void addMessage(const QString &role, const QString &text);
    void updateCurrentBubble(const QString &text);
    QJsonArray buildMessages();
    QJsonArray buildTools();
    QString executeTool(const QString &name, const QJsonObject &args);
    QString getSystemInfo();
    bool isCommandAllowed(const QString &cmd) const;
    void sendRequest(bool stream);
    void processToolCalls(const QJsonArray &toolCalls);
    void scrollToBottom();
    QString resolvePath(const QString &input);
    void startServer();
    bool isServerRunning();
    QString findServerPath();
    QString findModelPath();
    QString findWorkDir();

    QNetworkAccessManager *m_manager = nullptr;

    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_messageContainer = nullptr;
    QVBoxLayout *m_messageLayout = nullptr;
    QLineEdit *m_input = nullptr;
    QPushButton *m_sendBtn = nullptr;

    QJsonArray m_history;
    QPointer<QNetworkReply> m_currentReply;
    QPointer<MessageBubble> m_currentBubble;

    QString m_assistantText;
    QByteArray m_contentBytes;
    QByteArray m_streamBuffer;
    QJsonArray m_pendingToolCalls;

    QProcess *m_serverProcess = nullptr;
    QTimer *m_readyTimer = nullptr;
    bool m_ownServer = false;
    bool m_serverReady = false;
    int m_readyCheckCount = 0;

    int m_toolRound = 0;
    static constexpr int kMaxToolRounds = 5;

    const QString m_systemPrompt =
        QStringLiteral("你叫络樱，是一个本地运行的 AI 助手。"
                       "你的底层技术来自阿里云的 Qwen 模型，"
                       "但你对外只用“络樱”这个身份，不要自称 Qwen 或通义千问。"
                       "如果用户明确问起 Qwen 或阿里云的技术问题，你可以正常回答。"
                       "当用户问“我叫什么”但对话里从未提到过用户的名字时，"
                       "你回答“你还没告诉我你的名字”，不要自己编一个名字。"
                       "当用户一次提出多个任务时，逐一完成，不要遗漏任何一个。"
                       "你可以调用工具来读写文件、查看系统信息、执行安全的查询命令。"
                       "执行命令时只能使用只读类命令（如 dir、type、where、ipconfig）。");
};

#endif // MAINWINDOW_H