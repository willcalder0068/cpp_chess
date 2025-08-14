#ifndef PROMPTDIALOG_H
#define PROMPTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QString>

class PromptDialog : public QDialog {
    Q_OBJECT  // Declare the class as a Qt meta-object, meaning it can be manipulated at runtime (after compilation); creates a moc_ file

public:
    PromptDialog(const QString &promptText, QWidget *parent = nullptr);  // Used to instantiate new dialogs with user
    QString getInputText() const;  // Retrieve input from user

// Signals connect to slot actions; the signal is triggered, and the slot acts
public slots:  // Public, so it can be accessed from a signal outside of the class
    void followParent();  // Follow the main window; used to keep the dialog embedded at all times

private:
    QLineEdit *inputField;  // Pointer to where input can be retrieved
};

#endif