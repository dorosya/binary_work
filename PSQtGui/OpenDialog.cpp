#include "OpenDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>

OpenDialog::OpenDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("Открыть / Создать"));
    setModal(true);
    resize(420, 180);

    auto* root = new QVBoxLayout(this);

    auto* row = new QHBoxLayout();
    m_rbOpen = new QRadioButton(QString::fromUtf8("Open"), this);
    m_rbCreate = new QRadioButton(QString::fromUtf8("Create"), this);
    m_rbOpen->setChecked(true);
    row->addWidget(m_rbOpen);
    row->addWidget(m_rbCreate);
    row->addStretch(1);
    root->addLayout(row);

    auto* form = new QFormLayout();
    m_base = new QLineEdit(this);
    m_base->setPlaceholderText(QString::fromUtf8("например: data"));
    form->addRow(QString::fromUtf8("Базовое имя файла:"), m_base);

    m_maxLen = new QSpinBox(this);
    m_maxLen->setRange(1, 5000);
    m_maxLen->setValue(32);
    form->addRow(QString::fromUtf8("maxNameLen (только Create):"), m_maxLen);

    m_prs = new QLineEdit(this);
    m_prs->setPlaceholderText(QString::fromUtf8("(необязательно)"));
    form->addRow(QString::fromUtf8("Имя .prs (только Create):"), m_prs);

    root->addLayout(form);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(bb);
}

bool OpenDialog::isCreate() const { return m_rbCreate->isChecked(); }
QString OpenDialog::baseName() const { return m_base->text().trimmed(); }
int OpenDialog::maxNameLen() const { return m_maxLen->value(); }
QString OpenDialog::prsName() const { return m_prs->text().trimmed(); }
