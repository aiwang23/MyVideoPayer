//
// Created by 10484 on 24-6-11.
//

// You may need to build the project (run Qt uic code generator) to get "ui_SetVideoUrlDialog.h" resolved

#include "setvideourldialog.h"

#include <QFileDialog>

#include "ui_SetVideoUrlDialog.h"

namespace Window
{
    SetVideoUrlDialog::SetVideoUrlDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SetVideoUrlDialog)
    {
        ui->setupUi(this);
        qRegisterMetaType<std::function<void()> >();

        connect(ui->pushButtonOK, &QPushButton::clicked,
            this, &QDialog::accept);
        connect(ui->pushButtonQuit, &QPushButton::clicked,
            this, &QDialog::close);
    }

    SetVideoUrlDialog::~SetVideoUrlDialog()
    {
        delete ui;
    }

    QString SetVideoUrlDialog::GetVideoURL() const
    {
        return ui->lineEditURL->text().trimmed();
    }

} // Window
