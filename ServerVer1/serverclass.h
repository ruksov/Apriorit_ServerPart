#ifndef SERVERCLASS_H
#define SERVERCLASS_H

#include <QObject>
#include <QtNetwork>
#include <QTextBrowser>
#include "userinfo.h"
#include "databaseclass.h"
#include "emessagetype.h"

class ServerClass : public QObject
{
    Q_OBJECT

private:
    QTcpServer* m_pTcpServer;
    DataBaseClass m_dbManager;
    QMap<QString, UserInfo> m_mapClients;
    QTextBrowser* m_ptxtInfo;
    quint16 m_nNextBlockSize;

    //Чтение сообщения клиента в зависимости от метки
    void ReadClientREG_LOG(QDataStream& in, QTcpSocket* pClientSocket,const MsgType& typeMsg);
    void ReadClientMESSAGE(QDataStream& in);

    //Отправка сообщения клиенту
    void SendToClient(QTcpSocket* pClientSocket, const MsgType& type, const QByteArray& arrBlockMsg);

public:
    explicit ServerClass(QObject *parent = 0 );


    bool StartServer(int nPort, QTextBrowser* txtInfo);
    QString GetIP();
    void setTxtBrowser(QTextBrowser* txtInfo);

public slots:
    void slotNewConnection();
    void slotReadClient();
};

#endif // SERVERCLASS_H
