#pragma once
#include <QDialog>

class QComboBox;
class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace ps { class CatalogService; }

class SpecificationDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit SpecificationDialog(ps::CatalogService* service, QWidget* parent = nullptr);

private slots:
    void onFind();
    void onAddFromOwner();
    void onContextMenuRequested(const QPoint& pos);
    void onAdd();
    void onEdit();
    void onDelete();

private:
    void rebuildTree();
    void reloadOwners(const QString& preferredOwner = {});
    void selectOwner(const QString& ownerName);
    QStringList componentChoices(const QString& ownerName) const;
    void openAddDialogForOwner(const QString& ownerName);
    void addChildrenRec(QTreeWidgetItem* parentItem, const QString& ownerName, int depth);
    void showError(const QString& msg);

    ps::CatalogService* m_service = nullptr;

    QLineEdit* m_search = nullptr;
    QPushButton* m_find = nullptr;
    QComboBox* m_owner = nullptr;
    QPushButton* m_addLink = nullptr;
    QPushButton* m_refresh = nullptr;
    QTreeWidget* m_tree = nullptr;

    QTreeWidgetItem* m_ctxItem = nullptr;
};
