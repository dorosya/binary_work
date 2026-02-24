#pragma once
#include <QDialog>

class QLineEdit;
class QSpinBox;
class QRadioButton;

class OpenDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit OpenDialog(QWidget* parent = nullptr);

    bool isCreate() const;
    QString baseName() const;
    int maxNameLen() const;
    QString prsName() const;

private:
    QRadioButton* m_rbOpen = nullptr;
    QRadioButton* m_rbCreate = nullptr;
    QLineEdit* m_base = nullptr;
    QSpinBox* m_maxLen = nullptr;
    QLineEdit* m_prs = nullptr;
};
