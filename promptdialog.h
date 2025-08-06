#ifndef PROMPTDIALOG_H
#define PROMPTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QString>

class PromptDialog : public QDialog {
    Q_OBJECT  // Declare the class as a Qt meta-object, meaning it can be manipulated at runtime (after compilation); creates a moc_ file

public:
    PromptDialog(const QString &promptText, QWidget *parent = nullptr);
    QString getInputText() const;

// Signals connect to slot actions; the signal is triggered, and the slot acts
public slots:
    void followParent();

private:
    QLineEdit *inputField;
};

#endif