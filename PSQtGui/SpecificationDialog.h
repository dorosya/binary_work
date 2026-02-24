#pragma once
#include <QDialog>

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
    void onContextMenuRequested(const QPoint& pos);
    void onAdd();
    void onEdit();
    void onDelete();

private:
    void rebuildTree();
    void addChildrenRec(QTreeWidgetItem* parentItem, const QString& ownerName, int depth);
    void showError(const QString& msg);

    ps::CatalogService* m_service = nullptr;

    QLineEdit* m_search = nullptr;
    QPushButton* m_find = nullptr;
    QTreeWidget* m_tree = nullptr;

    // context target
    QTreeWidgetItem* m_ctxItem = nullptr;
};
