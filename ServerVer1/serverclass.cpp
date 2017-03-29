#include "serverclass.h"

ServerClass::ServerClass( QObject *parent) : QObject(parent), m_ptxtInfo(nullptr)
{

}


bool ServerClass::StartServer(int nPort, QTextBrowser* txtInfo)
{
    m_pTcpServer = new QTcpServer();
    this->setTxtBrowser(txtInfo);

    if(!m_pTcpServer->listen(QHostAddress::Any, nPort))
    {
        qDebug()<<m_pTcpServer->errorString();
        this->m_ptxtInfo->append(m_pTcpServer->errorString());
        m_pTcpServer->close();
        return false;
    }
    else if(!this->m_dbManager.ConnectToDataBase())
    {
        qDebug()<<this->m_dbManager.GetLastError();
        this->m_ptxtInfo->append(this->m_dbManager.GetLastError());
        return false;
    }
    qDebug()<<"Server started";
    this->m_dbManager.FillMapUsername(this->m_mapClients);

    connect(this->m_pTcpServer, SIGNAL(newConnection()), this, SLOT(slotNewConnection()));

    return true;
}


void ServerClass::ReadClientREG_LOG(QDataStream &in, QTcpSocket* pClientSocket, const MsgType &typeMsg)
{
    UserInfo StructUI;
    QString strMsg, strUsername, strPassword;


    if(typeMsg == REG)
        in>>StructUI;
    in>>strMsg;


    int i = 0;
    while(strMsg[i] != '.')
    {
        strUsername+=strMsg[i];
        ++i;
    }
    ++i;
    strPassword.append(strMsg.mid(i));

    if(typeMsg == REG)
    {
        StructUI.pClientSocket = pClientSocket;


        if(this->m_dbManager.isUsernameBusy(strUsername))
        {
            this->SendToClient(pClientSocket, REG_ERROR, NULL);
            return;
        }


        this->m_dbManager.WriteToDataBase(strUsername, strPassword, StructUI);


        qDebug()<<"Client "<<strUsername<<"registrate at server";
        this->m_ptxtInfo->append("Client " + strUsername +"registrate at server");
    }
    else if(typeMsg == LOGIN)
    {

        if(!this->m_dbManager.isCorrectLogin(strUsername, strPassword))
        {
            this->SendToClient(pClientSocket, LOG_ERROR, NULL);
            return;
        }


        StructUI = this->m_mapClients.value(strUsername);
        StructUI.pClientSocket = pClientSocket;

        qDebug()<<"Client "<<strUsername<<"login to the server";
        this->m_ptxtInfo->append("Client "+strUsername+"login to the server");
    }


    this->m_mapClients.insert(strUsername, StructUI);


    QByteArray arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_DefaultCompiledVersion);



    foreach(auto currUsername,this->m_mapClients.keys())
    {
        if(currUsername == strUsername)
        {

            arrBlock.clear();
            out.device()->reset();
            out<<this->m_mapClients;
            this->SendToClient(pClientSocket, OK, arrBlock);


            arrBlock.clear();
            out.device()->reset();
            out<<qMakePair(strUsername, StructUI);
        }
        else if(this->m_mapClients.value(currUsername).pClientSocket)
        {
            this->SendToClient(this->m_mapClients.value(currUsername).pClientSocket, UPDATE, arrBlock);
        }
    }
}



void ServerClass::ReadClientMESSAGE(QDataStream &in)
{
    QString strMsg, strSender, strReciever;

    in>>strMsg;

    int i = 0;
    while(strMsg[i] != '.')
    {
        strSender+=strMsg[i];
        ++i;
    }
    ++i;
    while(strMsg[i] != '.')
    {
        strReciever+=strMsg[i];
        ++i;
    }
    ++i;
    strMsg = strMsg.mid(i);

    qDebug()<<"Client "<<strSender<<" send message to "<<strReciever;
    this->m_ptxtInfo->append("Client "+strSender+" send message to "+strReciever);

    QByteArray arrBlock;
    arrBlock.append(strReciever + '.' + strMsg);


    this->SendToClient(this->m_mapClients.value(strReciever).pClientSocket, MESSAGE, arrBlock);

}



void ServerClass::SendToClient(QTcpSocket *pClientSocket,const MsgType &type, const QByteArray &arrBlockMsg)
{
    QByteArray arrBlock;
    QDataStream out(&arrBlock, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    out<<quint32(0)<<type;

    if(!arrBlockMsg.isEmpty())
        out<<arrBlockMsg;

    out.device()->seek(0);
    out<<quint32(arrBlock.size() - sizeof(quint32));

    pClientSocket->write(arrBlock);
}


QString ServerClass::GetIP()
{
    auto address = QNetworkInterface::allAddresses();

    for(int i = 0; i<address.size();++i)
    {
        if(address.at(i)!=QHostAddress::LocalHost&&address.at(i).toIPv4Address())
        {
            return address.at(i).toString();
        }
    }
    return QHostAddress(QHostAddress::LocalHost).toString();
}


void ServerClass::setTxtBrowser(QTextBrowser *txtInfo)
{
    this->m_ptxtInfo = txtInfo;
}


void ServerClass::slotNewConnection()
{
    QTcpSocket* pClientSocket = this->m_pTcpServer->nextPendingConnection();

    connect(pClientSocket, SIGNAL(disconnected()), pClientSocket, SLOT(deleteLater()));
    connect(pClientSocket, SIGNAL(readyRead()), SLOT(slotReadClient()));
    qDebug()<<"Client connect to Server";
    this->SendToClient(pClientSocket, OK, NULL);
}


void ServerClass::slotReadClient()
{
    QTcpSocket* pClientSocket = static_cast<QTcpSocket*>(sender());

    QDataStream in(pClientSocket);
    in.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    int typeMsg = 0;

    for(;;)
    {
        if(!this->m_nNextBlockSize)
        {
            if(pClientSocket->bytesAvailable()<sizeof(quint64))
                break;
            in>>this->m_nNextBlockSize;
        }

        if(pClientSocket->bytesAvailable()<this->m_nNextBlockSize)
            break;

        in>>typeMsg;

        if(typeMsg == MESSAGE)
            this->ReadClientMESSAGE(in);
        else
            this->ReadClientREG_LOG(in, pClientSocket, static_cast<MsgType>(typeMsg));
    }
}


