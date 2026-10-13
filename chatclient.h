#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QAbstractSocket>

class QStackedWidget;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QListWidget;
class QLabel;

class ChatClient : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChatClient(QWidget *parent = nullptr);
    ~ChatClient();

private slots:
    void onConnectClicked();
    void onLoginClicked();
    void onRegisterClicked();
    void onSendClicked();

    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError err);

private:
    void setupConnectPage();   // 页面 0：连接 + 登录/注册
    void setupChatPage();      // 页面 1：聊天
    void handleMessage(const QString &msg);
    void appendChat(const QString &text);
    void sendLine(const QString &line);

    QTcpSocket     *m_socket    = nullptr;
    QStackedWidget *m_stack     = nullptr;

    // ---- 连接页控件 ----
    QLineEdit   *m_ipEdit       = nullptr;
    QLineEdit   *m_portEdit     = nullptr;
    QLineEdit   *m_userEdit     = nullptr;
    QLineEdit   *m_passEdit     = nullptr;
    QPushButton *m_connectBtn   = nullptr;
    QPushButton *m_loginBtn     = nullptr;
    QPushButton *m_registerBtn  = nullptr;
    QLabel      *m_statusLabel  = nullptr;

    // ---- 聊天页控件 ----
    QListWidget *m_friendList   = nullptr;   // 好友列表（预留）
    QTextEdit   *m_chatView     = nullptr;
    QLineEdit   *m_msgEdit      = nullptr;
    QPushButton *m_sendBtn      = nullptr;

    QByteArray   m_buffer;       // 接收缓冲区，用于拆包
    bool         m_connected = false;
    bool         m_loggedIn  = false;
};

#endif // CHATCLIENT_H