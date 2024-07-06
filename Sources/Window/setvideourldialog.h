//
// Created by 10484 on 24-6-11.
//

#ifndef SETVIDEOURLDIALOG_H
#define SETVIDEOURLDIALOG_H

#include <QDialog>

namespace Window
{
    QT_BEGIN_NAMESPACE

    namespace Ui
    {
        class SetVideoUrlDialog;
    }

    QT_END_NAMESPACE

    class SetVideoUrlDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit SetVideoUrlDialog(QWidget *parent = nullptr);

        ~SetVideoUrlDialog() override;

        [[nodiscard]] QString GetVideoURL() const;

    private:
        Ui::SetVideoUrlDialog *ui;
    };
} // Window

#endif //SETVIDEOURLDIALOG_H
