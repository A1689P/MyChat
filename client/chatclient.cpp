#include "chatclient.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QListWidget>
#include <QStackedWidget>
#include <QMessageBox>
#include <QIntValidator>
#include <QDateTime>

ChatClient::ChatClient(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Qt 聊天客户端"));
    resize(880, 600);

    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected,
            this, &ChatClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &ChatClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &ChatClient::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred,
            this, &ChatClient::onSocketError);

    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    setupConnectPage();   // index 0
    setupChatPage();      // index 1

    m_stack->setCurrentIndex(0);
}

ChatClient::~ChatClient()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->disconnectFromHost();
}

// ============================================================
//  页面 1：连接服务器 + 登录/注册
// ============================================================
void ChatClient::setupConnectPage()
{
    QWidget *page = new QWidget;
    page->setMaximumWidth(420);

    QVBoxLayout *outer = new QVBoxLayout(page);

    // ---------- 服务器连接 ----------
    QGroupBox   *serverBox  = new QGroupBox(QStringLiteral("1. 服务器连接"));
    QFormLayout *serverForm = new QFormLayout(serverBox);

    m_ipEdit   = new QLineEdit(QStringLiteral("127.0.0.1"));
    m_portEdit = new QLineEdit(QStringLiteral("8000"));
    m_portEdit->setValidator(new QIntValidator(1, 65535, this));

    m_connectBtn = new QPushButton(QStringLiteral("连接服务器"));

    serverForm->addRow(QStringLiteral("服务器 IP："), m_ipEdit);
    serverForm->addRow(QStringLiteral("端口："),      m_portEdit);
    serverForm->addRow(QString(),                     m_connectBtn);

    // ---------- 账号 ----------
    QGroupBox   *accountBox  = new QGroupBox(QStringLiteral("2. 登录 / 注册"));
    QFormLayout *accountForm = new QFormLayout(accountBox);

    m_userEdit = new QLineEdit;
    m_userEdit->setPlaceholderText(QStringLiteral("用户名"));

    m_passEdit = new QLineEdit;
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setPlaceholderText(QStringLiteral("密码"));

    m_loginBtn    = new QPushButton(QStringLiteral("登  录"));
    m_registerBtn = new QPushButton(QStringLiteral("注  册"));

    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addWidget(m_loginBtn);
    btnRow->addWidget(m_registerBtn);

    accountForm->addRow(QStringLiteral("用户名："), m_userEdit);
    accountForm->addRow(QStringLiteral("密  码："), m_passEdit);
    accountForm->addRow(QString(),                  btnRow);

    // ---------- 状态栏 ----------
    m_statusLabel = new QLabel(QStringLiteral("状态：未连接"));
    m_statusLabel->setStyleSheet("color:#888;");

    outer->addStretch();
    outer->addWidget(serverBox);
    outer->addSpacing(12);
    outer->addWidget(accountBox);
    outer->addSpacing(12);
    outer->addWidget(m_statusLabel, 0, Qt::AlignHCenter);
    outer->addStretch();

    // 没连接前，登录/注册不可用
    m_loginBtn->setEnabled(false);
    m_registerBtn->setEnabled(false);

    connect(m_connectBtn,  &QPushButton::clicked, this, &ChatClient::onConnectClicked);
    connect(m_loginBtn,    &QPushButton::clicked, this, &ChatClient::onLoginClicked);
    connect(m_registerBtn, &QPushButton::clicked, this, &ChatClient::onRegisterClicked);
    connect(m_passEdit,    &QLineEdit::returnPressed, this, &ChatClient::onLoginClicked);

    // 居中包裹
    QWidget     *wrapper    = new QWidget;
    QHBoxLayout *wrapLayout = new QHBoxLayout(wrapper);
    wrapLayout->addStretch();
    wrapLayout->addWidget(page);
    wrapLayout->addStretch();

    m_stack->addWidget(wrapper);
}

// ============================================================
//  页面 2：聊天界面
// ============================================================
void ChatClient::setupChatPage()
{
    QWidget     *page       = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(page);

    // -------- 左侧：好友列表 --------
    QWidget     *left       = new QWidget;
    left->setFixedWidth(200);
    QVBoxLayout *leftLayout = new QVBoxLayout(left);

    QLabel *friendTitle = new QLabel(QStringLiteral("好友列表"));
    friendTitle->setStyleSheet("font-weight:bold; padding:4px;");

    m_friendList = new QListWidget;
    m_friendList->addItem(QStringLiteral("【公共聊天室】"));
    m_friendList->setCurrentRow(0);

    leftLayout->addWidget(friendTitle);
    leftLayout->addWidget(m_friendList);

    // -------- 右侧：聊天区 --------
    QWidget     *right       = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(right);

    m_chatView = new QTextEdit;
    m_chatView->setReadOnly(true);
    m_chatView->setStyleSheet("font-size:14px;");

    QHBoxLayout *inputLayout = new QHBoxLayout;
    m_msgEdit = new QLineEdit;
    m_msgEdit->setPlaceholderText(QStringLiteral("输入消息，按回车发送…"));
    m_sendBtn = new QPushButton(QStringLiteral("发送"));
    inputLayout->addWidget(m_msgEdit);
    inputLayout->addWidget(m_sendBtn);

    rightLayout->addWidget(m_chatView);
    rightLayout->addLayout(inputLayout);

    mainLayout->addWidget(left);
    mainLayout->addWidget(right, 1);

    m_stack->addWidget(page);

    connect(m_sendBtn, &QPushButton::clicked,      this, &ChatClient::onSendClicked);
    connect(m_msgEdit, &QLineEdit::returnPressed,  this, &ChatClient::onSendClicked);
}

// ============================================================
//  槽函数
// ============================================================
void ChatClient::onConnectClicked()
{
    // 已连接 → 断开
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
        return;
    }

    const QString ip   = m_ipEdit->text().trimmed();
    const QString port = m_portEdit->text().trimmed();
    if (ip.isEmpty() || port.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请填写服务器 IP 和端口"));
        return;
    }

    m_statusLabel->setText(QStringLiteral("状态：正在连接…"));
    m_statusLabel->setStyleSheet("color:#c80;");
    m_connectBtn->setEnabled(false);

    m_socket->connectToHost(ip, port.toUShort());
}

void ChatClient::onConnected()
{
    m_connected = true;

    m_connectBtn->setText(QStringLiteral("断开连接"));
    m_connectBtn->setEnabled(true);
    m_loginBtn->setEnabled(true);
    m_registerBtn->setEnabled(true);

    m_statusLabel->setText(QStringLiteral("状态：已连接 %1:%2")
                               .arg(m_ipEdit->text(), m_portEdit->text()));
    m_statusLabel->setStyleSheet("color:green;");
}

void ChatClient::onDisconnected()
{
    m_connected = false;
    m_loggedIn  = false;
    m_buffer.clear();

    m_connectBtn->setText(QStringLiteral("连接服务器"));
    m_connectBtn->setEnabled(true);
    m_loginBtn->setEnabled(false);
    m_registerBtn->setEnabled(false);

    m_statusLabel->setText(QStringLiteral("状态：已断开"));
    m_statusLabel->setStyleSheet("color:#888;");

    m_stack->setCurrentIndex(0);
}

void ChatClient::onSocketError(QAbstractSocket::SocketError)
{
    m_connectBtn->setEnabled(true);
    m_connectBtn->setText(QStringLiteral("连接服务器"));
    m_statusLabel->setText(QStringLiteral("状态：连接失败 - %1")
                               .arg(m_socket->errorString()));
    m_statusLabel->setStyleSheet("color:red;");

    QMessageBox::warning(this, QStringLiteral("连接错误"), m_socket->errorString());
}

// ---- 收数据：按 \n 拆包（关键！）----
void ChatClient::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    int idx;
    while ((idx = m_buffer.indexOf('\n')) >= 0) {
        QByteArray line = m_buffer.left(idx);
        m_buffer.remove(0, idx + 1);

        if (line.endsWith('\r')) line.chop(1);
        if (line.isEmpty())      continue;

        handleMessage(QString::fromUtf8(line));
    }
}

void ChatClient::handleMessage(const QString &msg)
{
    // ------ 注册回复 ------
    if (msg.startsWith(QStringLiteral("r,"))) {
        const QString r = msg.mid(2);
        if (r == QStringLiteral("ok")) {
            QMessageBox::information(this, QStringLiteral("注册"),
                                     QStringLiteral("🎉 注册成功，请登录！"));
            m_passEdit->clear();
            m_passEdit->setFocus();
        } else if (r == QStringLiteral("exist")) {
            QMessageBox::warning(this, QStringLiteral("注册"),
                                 QStringLiteral("该用户名已被注册！"));
        } else {
            QMessageBox::warning(this, QStringLiteral("注册"),
                                 QStringLiteral("注册失败：%1").arg(r));
        }
    }
    // ------ 登录回复 ------
    else if (msg.startsWith(QStringLiteral("l,"))) {
        const QString r = msg.mid(2);
        if (r == QStringLiteral("ok")) {
            m_loggedIn = true;
            m_stack->setCurrentIndex(1);
            m_chatView->clear();
            appendChat(QStringLiteral("── 已进入聊天室 ──"));
            m_msgEdit->setFocus();
            QMessageBox::information(this, QStringLiteral("登录"),
                                     QStringLiteral("🎉 登录成功！"));
        } else {
            QMessageBox::warning(this, QStringLiteral("登录"),
                                 QStringLiteral("登录失败：%1").arg(r));
        }
    }
    // ------ 聊天消息 ------
    else if (msg.startsWith(QStringLiteral("c,"))) {
        appendChat(msg.mid(2));
    }
    // ------ 其它（兜底显示）------
    else {
        appendChat(msg);
    }
}

void ChatClient::appendChat(const QString &text)
{
    const QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_chatView->append(QStringLiteral("[%1] %2").arg(ts, text.toHtmlEscaped()));
    // 自动滚到底
    m_chatView->moveCursor(QTextCursor::End);
}

// ---- 登录 ----
void ChatClient::onLoginClicked()
{
    if (!m_connected) return;

    const QString u = m_userEdit->text().trimmed();
    const QString p = m_passEdit->text();

    if (u.isEmpty() || p.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请输入用户名和密码"));
        return;
    }
    if (u.contains(',') || p.contains(',')) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("用户名/密码不能包含英文逗号"));
        return;
    }

    sendLine(QStringLiteral("l,%1,%2").arg(u, p));
}

// ---- 注册 ----
void ChatClient::onRegisterClicked()
{
    if (!m_connected) return;

    const QString u = m_userEdit->text().trimmed();
    const QString p = m_passEdit->text();

    if (u.isEmpty() || p.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请输入用户名和密码"));
        return;
    }
    if (u.contains(',') || p.contains(',')) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("用户名/密码不能包含英文逗号"));
        return;
    }

    sendLine(QStringLiteral("r,%1,%2").arg(u, p));
}

// ---- 发送聊天消息 ----
void ChatClient::onSendClicked()
{
    if (!m_loggedIn) return;

    const QString text = m_msgEdit->text().trimmed();
    if (text.isEmpty())    return;
    if (text.contains('\n')) return;   // 防止破坏协议

    sendLine(QStringLiteral("c,%1").arg(text));
    m_msgEdit->clear();
}

// ---- 统一发送：自动补 \n ----
void ChatClient::sendLine(const QString &line)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) return;

    m_socket->write(line.toUtf8() + '\n');
    m_socket->flush();
}
