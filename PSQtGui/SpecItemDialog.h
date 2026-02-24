#pragma once
#include <QDialog>

class QComboBox;
class QSpinBox;

class SpecItemDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit SpecItemDialog(QWidget* parent = nullptr);

    void setComponents(const QStringList& names);
    void setSelected(const QString& name);
    void setQty(int qty);

    QString selectedName() const;
    int qty() const;

private:
    QComboBox* m_combo = nullptr;
    QSpinBox* m_qty = nullptr;
};
