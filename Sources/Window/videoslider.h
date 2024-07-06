//
// Created by 10484 on 24-6-12.
//

#ifndef VIDEOSLIDER_H
#define VIDEOSLIDER_H

#include <QSlider>
#include <QWidget>

namespace Window
{
    QT_BEGIN_NAMESPACE

    namespace Ui
    {
        class VideoSlider;
    }

    QT_END_NAMESPACE

    class VideoSlider final : public QSlider
    {
        Q_OBJECT

    public:
        explicit VideoSlider(QWidget *parent = nullptr);

        ~VideoSlider() override;

        bool SetValue(int val);

    protected:
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;

    signals:
        void sigValueChanged(int);

    };
} // Window

#endif //VIDEOSLIDER_H
