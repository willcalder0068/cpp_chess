#include <QHBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QTimer>
#include "promptdialog.h"

PromptDialog::PromptDialog(const QString &promptText, QWidget *parent)
    : QDialog(parent) {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);  // Set the window as frameless (embedded within the parent)
        setFixedSize(480, 80);

        followParent();  // Call the slot function independent of the signal to orient the dialog box

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(20);

        // Where the text to the user goes
        auto *label = new QLabel(promptText, this);
        label->setMinimumWidth(300);

        // Text box
        inputField = new QLineEdit(this);
        inputField->setMinimumWidth(100);

        // Confirm button
        auto *okButton = new QPushButton("OK", this);
        connect(okButton, &QPushButton::clicked, this, &QDialog::accept);  // If the button is clicked, mark the dialog as accepted

        layout->addWidget(label);
        layout->addWidget(inputField);
        layout->addWidget(okButton);
}

// Retrieve text from user
QString PromptDialog::getInputText() const {
    return inputField->text();
}

// If the main window is moved, prompt placement will be in the same place relative to it
void PromptDialog::followParent() {
    if (parentWidget()) {
        QPoint globalTopLeft = parentWidget()->mapToGlobal(QPoint(0, 0));  // Retrieves the top left corner of the parentWisget
        int offsetX = globalTopLeft.x();
        int offsetY = globalTopLeft.y() + parentWidget()->height() - 80;  // Embed it below

        move(offsetX, offsetY);
    }
}