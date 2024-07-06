//
// Created by 10484 on 24-6-10.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <PlayerCore.h>
#include <qlistwidget.h>
#include <QMainWindow>
#include <QPropertyAnimation>
#include "VideoPlayerCallBack.h"

struct MyFileInfo
{
    QString url;
    QString name;
};

namespace Window
{
    QT_BEGIN_NAMESPACE

    namespace Ui
    {
        class MainWindow;
    }

    QT_END_NAMESPACE

    class MainWindow final : public QMainWindow, public PlayerCore::VideoPlayerCallBack
    {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);

        ~MainWindow() override;

    protected:
        // 退出
        void closeEvent(QCloseEvent *event) override;

        bool eventFilter(QObject *target, QEvent *event) override;

        ///打开文件失败
        void onOpenVideoFileFailed(const int &code) override;

        ///打开sdl失败的时候回调此函数
        void onOpenSdlFailed() override;

        ///获取到视频时长的时候调用此函数
        void onTotalTimeChanged(const int64_t &uSec) override;

        ///播放器状态改变的时候回调此函数
        void onPlayerStateChanged(const PlayerCore::PlayerState &state,
                                  const bool &hasVideo, const bool &hasAudio) override;

        ///显示视频数据，此函数不宜做耗时操作，否则会影响播放的流畅性。
        void onDisplayVideo(std::shared_ptr<Utils::MyYUVFrame> videoFrame) override;

    private:
        void ConnectSignalSlots();

        void InitMenu();

    private slots:
        /**
         * 拖动条
         * @param val
         */
        void slotSliderMoved(int val) const;

        /**
         * 控制栏播放时间更新 控制栏隐藏
         */
        void slotTimerTimeOut();

        /**
         * 按钮点击
         * @param isChecked 是否点了静音按钮
         */
        void slotBtnClicked(bool isChecked);

        /**
         * 双击播放
         * @param item 
         */
        void slotItemDoubleClicked(QListWidgetItem *item);

        /**
         * 右键菜单
         */
        void slotMenuShow();

        /**
         * 右键的菜单项
         */
        void slotActionClicked();

    private:
        MyFileInfo doOpenFiles();

        MyFileInfo doOpenStream();

        void doClear();

        void doDelete(int idx);

    private:
        Ui::MainWindow *ui;
        QTimer *mTimer = nullptr;                    //定时器-获取当前视频时间
        std::unique_ptr<PlayerCore::PlayerCore> mPlayer;
        float mVolume = 0;

        /// @brief 右键菜单
        QMenu *mMenu = nullptr;
        QAction *mAddStreamAction = nullptr;
        QAction *mAddFilesAction = nullptr;
        QAction *mDeleteVideoAction = nullptr;
        QAction *mClearVideoAction = nullptr;
        QList<MyFileInfo> mfileList;
    };
} // Window

#endif //MAINWINDOW_H
