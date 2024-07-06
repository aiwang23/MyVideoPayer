//
// Created by 10484 on 24-6-10.
//

#ifndef DISPLAYWINDOW_H
#define DISPLAYWINDOW_H

#include <QWidget>
#include <QResizeEvent>

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>

#include "VideoPlayerCallBack.h"

#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4

namespace Window
{
    struct FaceInfoNode
    {
        QRect faceRect;
    };

    QT_BEGIN_NAMESPACE

    namespace Ui
    {
        class DisplayWindow;
    }

    QT_END_NAMESPACE

    class DisplayWindow : public QOpenGLWidget, public QOpenGLFunctions
    {
        Q_OBJECT

    public:
        explicit DisplayWindow(QWidget *parent = nullptr);

        ~DisplayWindow() override;

        void Clear();

        void SetVideoWidth(int w, int h);

        void InputOneFrame(std::shared_ptr<Utils::MyYUVFrame> videoFrame);

        void ResetGLVertex(int window_W, int window_H);

    protected:
        /// 初始化
        void initializeGL() override;

        /// 重新调整大小
        void resizeGL(int window_W, int window_H) override;

        /// 渲染到屏幕上
        void paintGL() override;

    private:
        Ui::DisplayWindow *ui;

        bool mIsOpenGLInited = false;        // openGL初始化函数是否执行过了
        QOpenGLShader *m_pVSHader = nullptr; // 顶点着色器程序对象
        QOpenGLShader *m_pFSHader = nullptr; // 片段着色器对象
        QOpenGLShaderProgram *m_program = nullptr;
        int m_colAttr = 0;
        int m_posAttr = 0;
        QOpenGLShaderProgram *m_pShaderProgram = nullptr; //着色器程序容器
        int textureUniformY = 0;                          // y纹理数据位置
        int textureUniformU = 0;                          // u纹理数据位置
        int textureUniformV = 0;                          // v纹理数据位置
        GLfloat *m_vertexVertices{};                      // 顶点矩阵
        QOpenGLTexture *m_pTextureY = nullptr;            // y纹理对象
        QOpenGLTexture *m_pTextureU = nullptr;            // u纹理对象
        QOpenGLTexture *m_pTextureV = nullptr;            // v纹理对象
        GLuint id_y = 0;                                  // y纹理对象ID
        GLuint id_u = 0;                                  // u纹理对象ID
        GLuint id_v = 0;                                  // v纹理对象ID
        qint64 mLastGetFrameTime = 0;                     // 上一次获取到帧的时间戳
        bool mCurrentVideoKeepAspectRatio = false;        //当前模式是否是按比例 当检测到与全局变量不一致的时候 则重新设置openGL矩阵
        bool mIsShowFaceRect = false;
        QList<FaceInfoNode> mFaceInfoList;
        int m_nVideoW = 0;        // 视频分辨率宽
        int m_nVideoH = 0;        // 视频分辨率高
        float mPicIndex_X = 0.0f; // 按比例显示情况下 图像偏移量百分比 (相对于窗口大小的)
        float mPicIndex_Y = 0.0f; // 按比例显示情况下 图像偏移量百分比 (相对于窗口大小的)
        std::shared_ptr<Utils::MyYUVFrame> mVideoFrame;
        bool gVideoKeepAspectRatio = true; // 按比例显示
    };
} // Window

#endif //DISPLAYWINDOW_H
