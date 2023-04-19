/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTime>
#include <QTimer>
#include <QScrollBar>

#include <QJsonDocument>
#include <QtCore/QDateTime>
//#include <QtMqtt/QMqttClient>
#include <QtMqtt/qmqttclient.h>
#include <QtWidgets/QMessageBox>


//A07 抽水泵    60s
//A02 净水下漏
//A03 污水下漏   45s
//A04 电磁阀
//A05 蠕动泵    15s
//A06 暂时不动

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    timeStart = 0;
    subscribe = 0;
    m_client = new QMqttClient(this);
    m_client->setHostname(ui->lineEditHost->text());
    m_client->setPort(ui->spinBoxPort->value());
    m_client->setUsername("administrator");
    m_client->setPassword("yfzx.2021");

    connect(m_client, &QMqttClient::stateChanged, this, &MainWindow::updateLogStateChange);
    connect(m_client, &QMqttClient::disconnected, this, &MainWindow::brokerDisconnected);

    connect(m_client, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic) {
        const QString content = QDateTime::currentDateTime().toString()
                    + QLatin1String(" Received Topic: ")
                    + topic.name()
                    + QLatin1String(" Message: ")
                    + message
                    + QLatin1Char('\n');
        insertColorText(content, QColor("black"), ui->editLog);
        //ui->editLog->insertPlainText(content);

        qjData = QJsonDocument::fromJson(message).object();

        //移动滚动条到底部
         QScrollBar *scrollbar = ui->editLog->verticalScrollBar();
        if (scrollbar)
        {
            scrollbar->setValue(ui->editLog->verticalScrollBar()->maximum());
        }
    });

//    connect(m_client, &QMqttClient::pingResponseReceived, this, [this]() {
//        ui->buttonAuto->setEnabled(true);
//        const QString content = QDateTime::currentDateTime().toString()
//                    + QLatin1String(" PingResponse")
//                    + QLatin1Char('\n');
        //ui->editLog->insertPlainText(content);
//    });

    connect(ui->lineEditHost, &QLineEdit::textChanged, m_client, &QMqttClient::setHostname);
    connect(ui->spinBoxPort, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::setClientPort);

    on_buttonConnect_clicked();

    updateLogStateChange();

    ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    ui->dateTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_buttonConnect_clicked()
{
    if (m_client->state() == QMqttClient::Disconnected) {
        insertColorText(tr("Reconnected!"),QColor("red"), ui->editLog, QColor("green"));
        ui->lineEditHost->setEnabled(false);
        ui->spinBoxPort->setEnabled(false);
        ui->buttonConnect->setText(tr("Disconnect"));
        m_client->connectToHost();

        sleep(1000);
        on_buttonSubscribe_clicked();
    } else {
        insertColorText(tr("Disconnected!"),QColor("red"), ui->editLog, QColor("green"));
        ui->lineEditHost->setEnabled(true);
        ui->spinBoxPort->setEnabled(true);
        ui->buttonConnect->setText(tr("Connect"));
        m_client->disconnectFromHost();
    }
}

void MainWindow::on_buttonQuit_clicked()
{
    QApplication::quit();
}

void MainWindow::updateLogStateChange()
{
    const QString content = QDateTime::currentDateTime().toString()
                    + QLatin1String(": State Change")
                    + QString::number(m_client->state())
                    + QLatin1Char('\n');
    insertColorText(content,QColor("black"), ui->editLog);
    //ui->editLog->insertPlainText(content);
}

void MainWindow::brokerDisconnected()
{
    ui->lineEditHost->setEnabled(true);
    ui->spinBoxPort->setEnabled(true);
    ui->buttonConnect->setText(tr("Connect"));
}

void MainWindow::setClientPort(int p)
{
    m_client->setPort(p);
}

void MainWindow::on_buttonPublish_clicked()
{
    if (m_client->publish(ui->linePubTopic->text(), ui->lineEditMessage->text().toUtf8()) == -1)
        QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message"));
}

void MainWindow::on_buttonSubscribe_clicked()
{
    //if(subscribe == 0)
    {
        auto subscription = m_client->subscribe(ui->lineSubTopic->text());//ui->lineSubTopic->text());
        if (!subscription) {
            QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not subscribe. Is there a valid connection?"));
            return;
        }
        //subscribe = 1;
        ui->buttonSubscribe->setText(tr("Cancel"));
    }
//    else
//    {
//        m_client->unsubscribe(tr("/data/4GMQTT000201"));//ui->lineSubTopic->text());
//        ui->buttonSubscribe->setText(tr("subscribe"));
//        subscribe = 0;
//    }
}

void MainWindow::on_buttonAuto_clicked()
{
    //ui->buttonAuto->setEnabled(false);
    //m_client->requestPing();
    on_PublishPlan();
}

void MainWindow::on_buttonSchedule_clicked()
{
    if(timeStart == 0)
    {
        timeStart = 1;
        ui->buttonSchedule->setText(tr("Stop"));

        preTimeId = startTimer(PRE_TIME, Qt::PreciseTimer);
    }
    else
    {
        ui->buttonSchedule->setText(tr("Schedule"));
        timeStart = 0;

        killTimer(timeId);
        killTimer(preTimeId);
    }
}

void MainWindow::on_buttonReset_clicked()
{

    insertColorText(tr("on_buttonReset_clicked!"),QColor("red"), ui->editLog, QColor("green"));

    strPubText = QObject::tr("{\"A02\":100000,\"res\":\"123\"}");
    if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
        insertColorText(tr("Could not publish message2!"),QColor("red"), ui->editLog, QColor("green"));
        //QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message2"));
    sleep(1000);

    strPubText = QObject::tr("{\"A03\":100000,\"res\":\"123\"}");
    if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
        insertColorText(tr("Could not publish message3!"),QColor("red"), ui->editLog, QColor("green"));
        //QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message3"));
    sleep(1000);

    strPubText = QObject::tr("{\"A05\":100000,\"res\":\"123\"}");
    if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
        insertColorText(tr("Could not publish message5!"),QColor("red"), ui->editLog, QColor("green"));
        //QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message5"));
    sleep(1000);

    strPubText = QObject::tr("{\"A07\":100000,\"res\":\"123\"}");
    if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
        insertColorText(tr("Could not publish message7!"),QColor("red"), ui->editLog, QColor("green"));
    sleep(1000);

}

void MainWindow::on_buttonClear_clicked()
{
    ui->editLog->clear();
}

void MainWindow::on_PublishPlan()
{
    QDateTime qTime = QDateTime::currentDateTime();
    QString allTime = qTime.toString("yyyy-MM-dd HH:mm:ss") + tr("  on_PublishPlan!");
    insertColorText(allTime, QColor("red"), ui->editLog, QColor("green"));

    sleep(1000);
    //抽水
    strPubText = QObject::tr("{\"A07\":110000,\"res\":\"123\"}");
    if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
        insertColorText(tr("Could not publish A07 open!"),QColor("red"), ui->editLog, QColor("green"));
        //ui->editLog->insertPlainText(tr("Could not publish A07 open!"));

    publishA7Open();

    sleep(PUBTIME_A7OPEN);

    //抽水结束
    strPubText = QObject::tr("{\"A07\":100000,\"res\":\"123\"}");
    if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
        insertColorText(tr("Could not publish A07 close!"),QColor("red"), ui->editLog, QColor("green"));
        //ui->editLog->insertPlainText(tr("Could not publish A07 close!"));

    publishA7Close();

    sleep(1000);


    //取样
    strPubText = QObject::tr("{\"A05\":110000,\"res\":\"123\"}");
    if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
        insertColorText(tr("Could not publish A05 open!"),QColor("red"), ui->editLog, QColor("green"));
        //ui->editLog->insertPlainText(tr("Could not publish A05 open!"));

    publishA5Open();

    sleep(PUBTIME_A5OPEN);

    //取样结束
    strPubText = QObject::tr("{\"A05\":100000,\"res\":\"123\"}");
    if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
        insertColorText(tr("Could not publish A05 close!"),QColor("red"), ui->editLog, QColor("green"));
        //ui->editLog->insertPlainText(tr("Could not publish A05 close!"));

    publishA5Close();

    sleep(1000);


    //打开阀2，清理水箱
    strPubText = QObject::tr("{\"A02\":110000,\"res\":\"123\"}");
    if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
        insertColorText(tr("Could not publish A02 open!"),QColor("red"), ui->editLog, QColor("green"));
        //ui->editLog->insertPlainText(tr("Could not publish A02 open!"));

    publishA2Open();

    sleep(PUBTIME_A2OPEN);


    //关闭阀2
    strPubText = QObject::tr("{\"A02\":100000,\"res\":\"123\"}");
    if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
        insertColorText(tr("Could not publish A02 close!"),QColor("red"), ui->editLog, QColor("green"));
        //ui->editLog->insertPlainText(tr("Could not publish A02 close!"));

    publishA2Close();

    sleep(1000);


    //打开阀3，清理污水
    strPubText = QObject::tr("{\"A03\":110000,\"res\":\"123\"}");
    if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
        insertColorText(tr("Could not publish A03 open!"),QColor("red"), ui->editLog, QColor("green"));
        //ui->editLog->insertPlainText(tr("Could not publish A03 open!"));

    publishA3Open();

    sleep(PUBTIME_A3OPEN);

    //关闭阀3
    strPubText = QObject::tr("{\"A03\":100000,\"res\":\"123\"}");
    if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
        insertColorText(tr("Could not publish A03 close!"),QColor("red"), ui->editLog, QColor("green"));
        //ui->editLog->insertPlainText(tr("Could not publish A03 close!"));

    publishA3Close();

    sleep(1000);

}


void MainWindow::timerEvent(QTimerEvent * ev)
{
    if(ev->timerId() == preTimeId)
    {
        //insertColorText(tr("pretimeid!"),QColor("red"), ui->editLog, QColor("green"));
        QDateTime datetime = QDateTime::currentDateTime();
        uint stime = datetime.toTime_t();
        QDateTime settime = ui->dateTimeEdit->dateTime();
        uint etime = settime.toTime_t();

        if(stime == etime)
        {
            timeId = startTimer(SCHEDULE_TIME, Qt::PreciseTimer);
            killTimer(preTimeId);

            on_PublishPlan();
        }

    }
    else if(ev->timerId() == timeId)
    {
        insertColorText(tr("time in!"),QColor("black"), ui->editLog);

        //if(timeStart == 1)
        //{
            //timeStart = 0;
            if (m_client->state() == QMqttClient::Disconnected)
            {
                on_buttonConnect_clicked();
                on_buttonReset_clicked();
            }
            on_PublishPlan();
        //}
        //else
        //{
        //    timeStart = 1;
            //on_buttonReset_clicked();
        //}

        //insertColorText(tr("time out!\n"),QColor("black"), ui->editLog);
        //ui->editLog->insertPlainText(tr("time out!"));
    }
}


void MainWindow::sleep(unsigned int msec)
{
    QTime reachTime = QTime::currentTime().addMSecs(msec);

    while(QTime::currentTime() < reachTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}


void MainWindow::insertColorText(QString text, QColor fontColor, QPlainTextEdit* plainTextEdit,QColor backColor)
{
    QTextCharFormat fmt;
    fmt.setForeground(QBrush(fontColor));
    fmt.setUnderlineColor("red");
    fmt.setBackground(QBrush(backColor));
    plainTextEdit->mergeCurrentCharFormat(fmt);
    plainTextEdit->appendPlainText(text);
}


void MainWindow::publishA7Open( )
{
    int a = 0;
    int index = 0;

    if (qjData.contains("A07"))
    {
        // 包含指定的 key
        QJsonValue value = qjData.value("A07");  // 获取指定 key 对应的 value
        if (value.isDouble())
        {
            // 判断 value 是否为1
            a = value.toInt();  // 将 value 转化为字符串
            strPubText = QObject::tr("{\"A07\":110000,\"res\":\"123\"}");

            while(a!=1&&index<=REPUB_TIMES_OPEN)
            {
                if (m_client->state() == QMqttClient::Disconnected)
                {
                    on_buttonConnect_clicked();
                }
                if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
                    insertColorText(tr("publishA7Open###Could not publish A07 Open!"),QColor("red"), ui->editLog, QColor("green"));

                sleep(1000);

                // 包含指定的 key
                value = qjData.value("A07");  // 获取指定 key 对应的 value
                if (value.isDouble())
                {  // 判断 value 是否为1
                    a = value.toInt();  // 将 value 转化为字符串
                }
                index++;
            }
            insertColorText(tr("publishA7Open###finish!"),QColor("red"), ui->editLog, QColor("green"));
        }
    }
}


void MainWindow::publishA7Close( )
{
    int a = 0;
    int index = 0;

    if (qjData.contains("A07"))
    {
        // 包含指定的 key
        QJsonValue value = qjData.value("A07");  // 获取指定 key 对应的 value
        if (value.isDouble())
        {
            // 判断 value 是否为1
            a = value.toInt();  // 将 value 转化为字符串
            strPubText = QObject::tr("{\"A07\":100000,\"res\":\"123\"}");

            while(a!=0&&index<=REPUB_TIMES_CLOSE)
            {
                if (m_client->state() == QMqttClient::Disconnected)
                {
                    on_buttonConnect_clicked();
                }
                if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
                    insertColorText(tr("publishA7Close###Could not publish A07 Close!"),QColor("red"), ui->editLog, QColor("green"));

                sleep(1000);

                // 包含指定的 key
                value = qjData.value("A07");  // 获取指定 key 对应的 value
                if (value.isDouble())
                {  // 判断 value 是否为1
                    a = value.toInt();  // 将 value 转化为字符串
                }
                index++;
            }
            insertColorText(tr("publishA7Close###finish!"),QColor("red"), ui->editLog, QColor("green"));
        }
    }
}


void MainWindow::publishA2Open( )
{
    int a = 0;
    int index = 0;

    if (qjData.contains("A02"))
    {
        // 包含指定的 key
        QJsonValue value = qjData.value("A02");  // 获取指定 key 对应的 value
        if (value.isDouble())
        {
            // 判断 value 是否为1
            a = value.toInt();  // 将 value 转化为字符串
            strPubText = QObject::tr("{\"A02\":110000,\"res\":\"123\"}");

            while(a!=1&&index<=REPUB_TIMES_OPEN)
            {
                if (m_client->state() == QMqttClient::Disconnected)
                {
                    on_buttonConnect_clicked();
                }
                if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
                    insertColorText(tr("publishA2Open###Could not publish A02 Open!"),QColor("red"), ui->editLog, QColor("green"));

                sleep(1000);

                // 包含指定的 key
                value = qjData.value("A02");  // 获取指定 key 对应的 value
                if (value.isDouble())
                {  // 判断 value 是否为1
                    a = value.toInt();  // 将 value 转化为字符串
                }
                index++;
            }
            insertColorText(tr("publishA2Open###finish!"),QColor("red"), ui->editLog, QColor("green"));
        }
    }
}


void MainWindow::publishA2Close( )
{
    int a = 0;
    int index = 0;

    if (qjData.contains("A02"))
    {
        // 包含指定的 key
        QJsonValue value = qjData.value("A02");  // 获取指定 key 对应的 value
        if (value.isDouble())
        {
            // 判断 value 是否为1
            a = value.toInt();  // 将 value 转化为字符串
            strPubText = QObject::tr("{\"A02\":100000,\"res\":\"123\"}");

            while(a!=0&&index<=REPUB_TIMES_CLOSE)
            {
                if (m_client->state() == QMqttClient::Disconnected)
                {
                    on_buttonConnect_clicked();
                }
                if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
                    insertColorText(tr("publishA2Close###Could not publish A02 Close!"),QColor("red"), ui->editLog, QColor("green"));

                sleep(1000);

                // 包含指定的 key
                value = qjData.value("A02");  // 获取指定 key 对应的 value
                if (value.isDouble())
                {  // 判断 value 是否为1
                    a = value.toInt();  // 将 value 转化为字符串
                }
                index++;
            }
            insertColorText(tr("publishA2Close###finish!"),QColor("red"), ui->editLog, QColor("green"));
        }
    }
}


void MainWindow::publishA3Open( )
{
    int a = 0;
    int index = 0;

    if (qjData.contains("A03"))
    {
        // 包含指定的 key
        QJsonValue value = qjData.value("A03");  // 获取指定 key 对应的 value
        if (value.isDouble())
        {
            // 判断 value 是否为1
            a = value.toInt();  // 将 value 转化为字符串
            strPubText = QObject::tr("{\"A03\":110000,\"res\":\"123\"}");

            while(a!=1&&index<=REPUB_TIMES_OPEN)
            {
                if (m_client->state() == QMqttClient::Disconnected)
                {
                    on_buttonConnect_clicked();
                }
                if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
                    insertColorText(tr("publishA3Open###Could not publish A03 Open!"),QColor("red"), ui->editLog, QColor("green"));

                sleep(1000);

                // 包含指定的 key
                value = qjData.value("A03");  // 获取指定 key 对应的 value
                if (value.isDouble())
                {  // 判断 value 是否为1
                    a = value.toInt();  // 将 value 转化为字符串
                }
                index++;
            }
            insertColorText(tr("publishA3Open###finish!"),QColor("red"), ui->editLog, QColor("green"));
        }
    }
}


void MainWindow::publishA3Close( )
{
    int a = 0;
    int index = 0;

    if (qjData.contains("A03"))
    {
        // 包含指定的 key
        QJsonValue value = qjData.value("A03");  // 获取指定 key 对应的 value
        if (value.isDouble())
        {
            // 判断 value 是否为1
            a = value.toInt();  // 将 value 转化为字符串
            strPubText = QObject::tr("{\"A03\":100000,\"res\":\"123\"}");

            while(a!=0&&index<=REPUB_TIMES_CLOSE)
            {
                if (m_client->state() == QMqttClient::Disconnected)
                {
                    on_buttonConnect_clicked();
                }
                if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
                    insertColorText(tr("publishA3Close###Could not publish A03 Close!"),QColor("red"), ui->editLog, QColor("green"));

                sleep(1000);

                // 包含指定的 key
                value = qjData.value("A03");  // 获取指定 key 对应的 value
                if (value.isDouble())
                {  // 判断 value 是否为1
                    a = value.toInt();  // 将 value 转化为字符串
                }
                index++;
            }
            insertColorText(tr("publishA3Close###finish!"),QColor("red"), ui->editLog, QColor("green"));
        }
    }
}


void MainWindow::publishA5Open( )
{
    int a = 0;
    int index = 0;

    if (qjData.contains("A05"))
    {
        // 包含指定的 key
        QJsonValue value = qjData.value("A05");  // 获取指定 key 对应的 value
        if (value.isDouble())
        {
            // 判断 value 是否为1
            a = value.toInt();  // 将 value 转化为字符串
            strPubText = QObject::tr("{\"A05\":110000,\"res\":\"123\"}");

            while(a!=1&&index<=REPUB_TIMES_OPEN)
            {
                if (m_client->state() == QMqttClient::Disconnected)
                {
                    on_buttonConnect_clicked();
                }
                if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
                    insertColorText(tr("publishA5Open###Could not publish A05 Open!"),QColor("red"), ui->editLog, QColor("green"));

                sleep(1000);

                // 包含指定的 key
                value = qjData.value("A05");  // 获取指定 key 对应的 value
                if (value.isDouble())
                {  // 判断 value 是否为1
                    a = value.toInt();  // 将 value 转化为字符串
                }
                index++;
            }
            insertColorText(tr("publishA5Open###finish!"),QColor("red"), ui->editLog, QColor("green"));
        }
    }
}


void MainWindow::publishA5Close( )
{
    int a = 0;
    int index = 0;

    if (qjData.contains("A05"))
    {
        // 包含指定的 key
        QJsonValue value = qjData.value("A05");  // 获取指定 key 对应的 value
        if (value.isDouble())
        {
            // 判断 value 是否为1
            a = value.toInt();  // 将 value 转化为字符串
            strPubText = QObject::tr("{\"A05\":100000,\"res\":\"123\"}");

            while(a!=0&&index<=REPUB_TIMES_CLOSE)
            {
                if (m_client->state() == QMqttClient::Disconnected)
                {
                    on_buttonConnect_clicked();
                }
                if (m_client->publish(ui->linePubTopic->text(), strPubText.toUtf8()) == -1)
                    insertColorText(tr("publishA5Close###Could not publish A05 Close!"),QColor("red"), ui->editLog, QColor("green"));

                sleep(1000);

                // 包含指定的 key
                value = qjData.value("A05");  // 获取指定 key 对应的 value
                if (value.isDouble())
                {  // 判断 value 是否为1
                    a = value.toInt();  // 将 value 转化为字符串
                }
                index++;
            }
            insertColorText(tr("publishA5Close###finish!"),QColor("red"), ui->editLog, QColor("green"));
        }
    }
}


