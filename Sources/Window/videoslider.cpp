//
// Created by 10484 on 24-6-12.
//

// You may need to build the project (run Qt uic code generator) to get "ui_VideoSlider.h" resolved

#include "videoslider.h"
#include <QMouseEvent>
#include <QStyle>
#include <QTimer>

namespace Window
{
    VideoSlider::VideoSlider(QWidget *parent) : QSlider(parent)
    {
    }

    VideoSlider::~VideoSlider()
    = default;

    bool VideoSlider::SetValue(const int val)
    {
        QSlider::setValue(val);
        return true;
    }

    void VideoSlider::mousePressEvent(QMouseEvent *event)
    {
        const int value = QStyle::sliderValueFromPosition(minimum(), maximum(),
                                                          event->pos().x(), width());
        emit sigValueChanged(value);
        setValue(value);
        QSlider::mousePressEvent(event);
    }

    void VideoSlider::mouseMoveEvent(QMouseEvent *event)
    {
        const int value = QStyle::sliderValueFromPosition(minimum(), maximum(),
                                                  event->pos().x(), width());
        emit sigValueChanged(value);
        QSlider::mouseMoveEvent(event);
    }

    void VideoSlider::mouseReleaseEvent(QMouseEvent *event)
    {
        // const int value = QStyle::sliderValueFromPosition(minimum(), maximum(),
        //                                   event->pos().x(), width());
        // emit sigValueChanged(value);
        QSlider::mouseReleaseEvent(event);
    }
} // Window
