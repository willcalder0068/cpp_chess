#include <QHBoxLayout>
#include <QPushButton>
#include <QApplication>
#include "promptdialog.h"

PromptDialog::PromptDialog(const QString &promptText, QWidget *parent)
    : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setFixedSize(480, 80);

    followParent();  // Call the slot function independent of the signal to orient the dialog box

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(20);

    auto *label = new QLabel(promptText, this);
    label->setMinimumWidth(300);

    inputField = new QLineEdit(this);
    inputField->setMinimumWidth(100);

    auto *okButton = new QPushButton("OK", this);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);  // If the button is clicked, mark the dialog as accepted

    layout->addWidget(label);
    layout->addWidget(inputField);
    layout->addWidget(okButton);
}

QString PromptDialog::getInputText() const {
    return inputField->text();
}

// Whenever the signal is emitted, the slot function is called
void PromptDialog::followParent() {
    if (parentWidget()) {
        QPoint parentTopLeft = parentWidget()->geometry().topLeft();
        move(parentTopLeft.x(), parentTopLeft.y() + 520);  // Follow the parent widget (but stay below the chess table)
    }
}