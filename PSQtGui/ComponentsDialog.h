#pragma once
#include <QDialog>

class QTableWidget;
class QLineEdit;
class QComboBox;
class QAction;

namespace ps { class CatalogService; }

class ComponentsDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit ComponentsDialog(ps::CatalogService* service, QWidget* parent = nullptr);

private slots:
    void onAdd();
    void onEdit();
    void onCancel();
    void onSave();
    void onDelete();
    void onSelectionChanged();

private:
    enum class Mode { View, Add, Edit };

    void setMode(Mode m);
    void reloadTable();
    void showError(const QString& msg);

    ps::CatalogService* m_service = nullptr;

    QAction* m_actAdd = nullptr;
    QAction* m_actEdit = nullptr;
    QAction* m_actCancel = nullptr;
    QAction* m_actSave = nullptr;
    QAction* m_actDelete = nullptr;

    QTableWidget* m_table = nullptr;
    QLineEdit* m_name = nullptr;
    QComboBox* m_type = nullptr;

    Mode m_mode = Mode::View;
    QString m_editOldName;
};
