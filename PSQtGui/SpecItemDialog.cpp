#include "SpecItemDialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QSpinBox>

SpecItemDialog::SpecItemDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8("Элемент спецификации"));
    setModal(true);
    resize(380, 140);

    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    m_combo = new QComboBox(this);
    form->addRow(QString::fromUtf8("Комплектующее:"), m_combo);

    m_qty = new QSpinBox(this);
    m_qty->setRange(1, 65535);
    m_qty->setValue(1);
    form->addRow(QString::fromUtf8("Количество:"), m_qty);

    root->addLayout(form);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(bb);
}

void SpecItemDialog::setComponents(const QStringList& names)
{
    m_combo->clear();
    m_combo->addItems(names);
}

void SpecItemDialog::setSelected(const QString& name)
{
    const int idx = m_combo->findText(name);
    if (idx >= 0) m_combo->setCurrentIndex(idx);
}

void SpecItemDialog::setQty(int qty) { m_qty->setValue(qty); }

QString SpecItemDialog::selectedName() const { return m_combo->currentText(); }
int SpecItemDialog::qty() const { return m_qty->value(); }
