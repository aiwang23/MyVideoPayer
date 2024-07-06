//
// Created by 10484 on 24-6-10.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MainWindow.h" resolved

#include "mainwindow.h"

#include "FunctionTransfer.h"
#include "ui_MainWindow.h"
#include <QMessageBox>
#include <QTimer>
#include <QFileDialog>
#include <QThread>

#include "setvideourldialog.h"
#include "videoslider.h"

Q_DECLARE_METATYPE(std::function<void()>)

namespace Window
{
    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
    {
        ui->setupUi(this);

        // 列表和播放区域的比例
        ui->splitter->setStretchFactor(0, 10);
        ui->splitter->setStretchFactor(1, 1);

        FunctionTransfer::init(QThread::currentThread());\
        ui->horizontalSliderVolume->setValue(100);

        qRegisterMetaType<PlayerCore::PlayerState>();
        qRegisterMetaType<std::function<void()> >();
        ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        ui->displayWindow->setMouseTracking(true);
        ui->displayWindow->installEventFilter(this);

        // 初始化播放器
        mPlayer = std::make_unique<PlayerCore::PlayerCore>();
        mPlayer->SetVideoPlayerCallBack(this);

        mTimer = new QTimer;
        mTimer->setInterval(500);

        ui->pushButtonPause->hide();

        this->setWindowTitle("播放器");
        mVolume = mPlayer->GetVolume();

        InitMenu();
        ConnectSignalSlots();
    }

    MainWindow::~MainWindow()
    {
        delete ui;
    }

    void MainWindow::closeEvent(QCloseEvent *event)
    {
        mPlayer->Stop(true);
    }

    bool MainWindow::eventFilter(QObject *target, QEvent *event)
    {
        return QMainWindow::eventFilter(target, event);
    }

    void MainWindow::onOpenVideoFileFailed(const int &code)
    {
        FunctionTransfer::runInMainThread([=]()
        {
            QMessageBox::critical(nullptr,
                                  "tips", QString("open file failed %1").arg(code));
        });
    }

    void MainWindow::onOpenSdlFailed()
    {
        FunctionTransfer::runInMainThread([=]()
        {
            QMessageBox::critical(nullptr, "tips", QString("open Sdl failed"));
        });
    }

    void MainWindow::onTotalTimeChanged(const int64_t &uSec)
    {
        FunctionTransfer::runInMainThread([=]()
        {
            const qint64 Sec = uSec / 1000000;

            ui->horizontalSliderProgress->setRange(0, Sec);

            QString totalTime;
            const QString hStr = QString("0%1").arg(Sec / 3600);
            const QString mStr = QString("0%1").arg(Sec / 60 % 60);
            const QString sStr = QString("0%1").arg(Sec % 60);
            if (hStr == "00")
            {
                totalTime = QString("%1:%2").arg(mStr.right(2)).arg(sStr.right(2));
            }
            else
            {
                totalTime = QString("%1:%2:%3").arg(hStr).arg(mStr.right(2)).arg(sStr.right(2));
            }

            ui->labelTotalTime->setText(totalTime);
        });
    }

    void MainWindow::onPlayerStateChanged(const PlayerCore::PlayerState &state,
                                          const bool &hasVideo,
                                          const bool &hasAudio)
    {
        FunctionTransfer::runInMainThread([=]()
        {
            if (state == PlayerCore::STOP)
            {
                // ui->stackedWidget->setCurrentWidget(ui->page_open);

                ui->pushButtonPause->hide();
                ui->displayWindow->Clear();

                ui->horizontalSliderProgress->setValue(0);
                ui->labelCurrentTime->setText("00:00");
                ui->labelTotalTime->setText("00:00");

                this->mTimer->stop();
                ui->stackedWidget->setCurrentIndex(0);
            }
            else if (state == PlayerCore::PLAYING)
            {
                if (hasVideo)
                {
                    // ui->stackedWidget->setCurrentWidget(ui->page_video);
                    ui->stackedWidget->setCurrentIndex(1);
                }
                else
                {
                    // ui->stackedWidget->setCurrentWidget(ui->page_audio);
                    ui->stackedWidget->setCurrentIndex(2);
                }

                ui->pushButtonPlay->hide();
                ui->pushButtonPause->show();

                mTimer->start();
            }
            else if (state == PlayerCore::PAUSE)
            {
                ui->pushButtonPause->hide();
                ui->pushButtonPlay->show();
            }
        });
    }

    void MainWindow::onDisplayVideo(std::shared_ptr<Utils::MyYUVFrame> videoFrame)
    {
        ui->displayWindow->InputOneFrame(videoFrame);
    }


    void MainWindow::ConnectSignalSlots()
    {
        connect(ui->pushButtonOpenFiles, &QPushButton::clicked,
                this, &MainWindow::slotBtnClicked);
        connect(ui->pushButtonOpenStream, &QPushButton::clicked,
                this, &MainWindow::slotBtnClicked);
        connect(ui->pushButtonPlay, &QPushButton::clicked,
                this, &MainWindow::slotBtnClicked);
        connect(ui->pushButtonPause, &QPushButton::clicked,
                this, &MainWindow::slotBtnClicked);
        connect(ui->pushButtonStop, &QPushButton::clicked,
                this, &MainWindow::slotBtnClicked);
        connect(ui->pushButtonMute, &QPushButton::clicked,
                this, &MainWindow::slotBtnClicked);
        connect(ui->horizontalSliderVolume, &QSlider::valueChanged,
                this, &MainWindow::slotSliderMoved);
        connect(ui->horizontalSliderProgress, &VideoSlider::sigValueChanged,
                this, &MainWindow::slotSliderMoved);
        connect(ui->listWidget, &QListWidget::itemDoubleClicked,
                this, &MainWindow::slotItemDoubleClicked);
        connect(ui->listWidget, &QListWidget::customContextMenuRequested,
                this, &MainWindow::slotMenuShow);
        connect(mTimer, &QTimer::timeout,
                this, &MainWindow::slotTimerTimeOut);
    }

    void MainWindow::InitMenu()
    {
        mMenu = new QMenu(ui->listWidget);
        mAddStreamAction = new QAction(QString("打开网络流"));
        mAddFilesAction = new QAction(QString("打开文件"));
        mDeleteVideoAction = new QAction(QString("删除"));
        mClearVideoAction = new QAction(QString("清空"));
        mMenu->addAction(mAddStreamAction);
        mMenu->addAction(mAddFilesAction);
        mMenu->addSeparator();
        mMenu->addAction(mDeleteVideoAction);
        mMenu->addAction(mClearVideoAction);

        connect(mAddStreamAction, &QAction::triggered,
                this, &MainWindow::slotActionClicked);
        connect(mAddFilesAction, &QAction::triggered,
                this, &MainWindow::slotActionClicked);
        connect(mDeleteVideoAction, &QAction::triggered,
                this, &MainWindow::slotActionClicked);
        connect(mClearVideoAction, &QAction::triggered,
                this, &MainWindow::slotActionClicked);
    }

    void MainWindow::slotSliderMoved(const int val) const
    {
        if (QObject::sender() == ui->horizontalSliderProgress)
        {
            mPlayer->Seek(static_cast<int64_t>(val) * 1000000);
        }
        else if (QObject::sender() == ui->horizontalSliderVolume)
        {
            mPlayer->SetVolume(val / 100.0);
            ui->labelVolume->setText(QString("%0").arg(val));
        }
    }

    void MainWindow::slotTimerTimeOut()
    {
        if (QObject::sender() == mTimer)
        {
            // 刚好有个winAPI 叫GetCurrentTime
#undef GetCurrentTime
            int64_t sec = mPlayer->GetCurrentTime();

            ui->horizontalSliderProgress->setValue(static_cast<int>(sec));
            QString hStr = QString("0%0").arg(sec / 3600);
            QString mStr = QString("0%0").arg(sec / 60 % 60);
            QString sStr = QString("0%0").arg(sec % 60);
            QString currTime;
            if (hStr == "00")
                currTime = QString("%0:%1").arg(mStr.right(2)).arg(sStr.right(2));
            else
                currTime = QString("%1:%2:%3")
                        .arg(hStr).arg(mStr.right(2)).arg(sStr.right(2));
            ui->labelCurrentTime->setText(currTime);
        }
    }

    void MainWindow::slotBtnClicked(bool isChecked)
    {
        if (QObject::sender() == ui->pushButtonPlay)
        {
            mPlayer->Play();
        }
        else if (QObject::sender() == ui->pushButtonPause)
        {
            mPlayer->Pause();
        }
        else if (QObject::sender() == ui->pushButtonStop)
        {
            mPlayer->Stop();
        }
        else if (QObject::sender() == ui->pushButtonOpenFiles)
        {
            auto [url, name] = doOpenFiles();
            mPlayer->StartPlay(url.toStdString());
        }
        else if (QObject::sender() == ui->pushButtonOpenStream)
        {
            auto [url, name] = doOpenStream();
            mPlayer->StartPlay(url.toStdString());
        }
        else if (QObject::sender() == ui->pushButtonMute)
        {
            const bool isMute = isChecked;
            mPlayer->SetMute(isMute);

            if (isMute)
            {
                this->mVolume = mPlayer->GetVolume();

                ui->horizontalSliderVolume->setValue(0);
                ui->horizontalSliderVolume->setEnabled(false);
                ui->labelVolume->setText(QString("%1").arg(0));
            }
            else
            {
                int volume = mVolume * 100.0;
                ui->horizontalSliderVolume->setValue(volume);
                ui->horizontalSliderVolume->setEnabled(true);
                ui->labelVolume->setText(QString("%1").arg(volume));
            }
        }
    }

    void MainWindow::slotItemDoubleClicked(QListWidgetItem *item)
    {
        if (QObject::sender() == ui->listWidget)
        {
            const QString name = item->text();
            QString url;
            for (const auto &file: mfileList)
            {
                if (file.name == name)
                {
                    url = file.url;
                    break;
                }
            }
            if (!url.isEmpty())
                mPlayer->StartPlay(url.toStdString());
        }
    }

    void MainWindow::slotMenuShow()
    {
        // QCursor::pos() 鼠标位置
        mMenu->exec(QCursor::pos());
    }

    void MainWindow::slotActionClicked()
    {
        if (QObject::sender() == mAddStreamAction)
            doOpenStream();
        else if (QObject::sender() == mAddFilesAction)
            doOpenFiles();
        else if (QObject::sender() == mDeleteVideoAction)
        {
            int idx = ui->listWidget->currentRow();
            doDelete(idx);
        }
        else if (QObject::sender() == mClearVideoAction)
            doClear();
    }

    MyFileInfo MainWindow::doOpenFiles()
    {
        auto urls = QFileDialog::getOpenFileUrls();
        MyFileInfo firstFileInfo;
        for (const auto &url: urls)
        {
            MyFileInfo file_info;
            file_info.url = url.toLocalFile();
            file_info.name = url.fileName();
            mfileList.append(file_info);

            if (firstFileInfo.url.isEmpty() && firstFileInfo.name.isEmpty())
                firstFileInfo = file_info;

            const auto item = new QListWidgetItem;
            item->setText(file_info.name);
            ui->listWidget->addItem(item);
        }
        ui->labelFiles->setText(QString("列表: %0个文件").arg(mfileList.size()));
        return firstFileInfo;
    }

    MyFileInfo MainWindow::doOpenStream()
    {
        SetVideoUrlDialog dialog;
        MyFileInfo firstFileInfo;
        if (dialog.exec() == QDialog::Accepted)
        {
            MyFileInfo file_info;
            file_info.name = file_info.url = dialog.GetVideoURL();
            mfileList.append(file_info);

            if (firstFileInfo.url.isEmpty() && firstFileInfo.name.isEmpty())
                firstFileInfo = file_info;

            const auto item = new QListWidgetItem;
            item->setText(file_info.name);
            ui->listWidget->addItem(item);
        }
        ui->labelFiles->setText(QString("列表: %0个文件").arg(mfileList.size()));
        return firstFileInfo;
    }

    void MainWindow::doClear()
    {
        mfileList.clear();
        ui->listWidget->clear();
        mPlayer->Stop();
        ui->labelFiles->setText(QString("列表: %0个文件").arg(mfileList.size()));
    }

    void MainWindow::doDelete(const int idx)
    {
        if (idx < 0 || idx >= mfileList.size() || idx >= ui->listWidget->count())
            return;

        if (mPlayer->GetState() == PlayerCore::PLAYING &&
            mPlayer->GetURL() == mfileList.at(idx).url.toStdString())
            mPlayer->Stop();

        mfileList.removeAt(idx);

        auto delItem = ui->listWidget->item(idx);
        ui->listWidget->removeItemWidget(delItem);
        delete delItem;
        ui->labelFiles->setText(QString("列表: %0个文件").arg(mfileList.size()));
    }
} // Window
