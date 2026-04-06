#include "SpecificationDialog.h"
#include "SpecItemDialog.h"

#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include "core/Errors.h"
#include "domain/Models.h"
#include "services/CatalogService.h"

static constexpr int ROLE_NAME = Qt::UserRole + 1;
static constexpr int ROLE_TYPE = Qt::UserRole + 2;

static std::string ToUtf8Std(const QString& s)
{
    const auto bytes = s.toUtf8();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

SpecificationDialog::SpecificationDialog(ps::CatalogService* service, QWidget* parent)
    : QDialog(parent), m_service(service)
{
    setWindowTitle(QString::fromUtf8("Спецификация"));
    resize(640, 420);
    setModal(true);

    auto* root = new QVBoxLayout(this);

    auto* searchRow = new QHBoxLayout();
    m_search = new QLineEdit(this);
    m_find = new QPushButton(QString::fromUtf8("Найти"), this);
    searchRow->addWidget(m_search, 1);
    searchRow->addWidget(m_find);
    connect(m_find, &QPushButton::clicked, this, &SpecificationDialog::onFind);
    root->addLayout(searchRow);

    auto* ownerRow = new QHBoxLayout();
    m_owner = new QComboBox(this);
    m_addLink = new QPushButton(QString::fromUtf8("Добавить связь"), this);
    m_refresh = new QPushButton(QString::fromUtf8("Обновить"), this);
    ownerRow->addWidget(m_owner, 1);
    ownerRow->addWidget(m_addLink);
    ownerRow->addWidget(m_refresh);
    connect(m_addLink, &QPushButton::clicked, this, &SpecificationDialog::onAddFromOwner);
    connect(m_refresh, &QPushButton::clicked, this, &SpecificationDialog::rebuildTree);
    root->addLayout(ownerRow);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &SpecificationDialog::onContextMenuRequested);
    root->addWidget(m_tree, 1);

    rebuildTree();
}

void SpecificationDialog::showError(const QString& msg)
{
    QMessageBox::critical(this, QString::fromUtf8("Ошибка"), msg);
}

void SpecificationDialog::reloadOwners(const QString& preferredOwner)
{
    QString ownerToKeep = preferredOwner;
    if (ownerToKeep.isEmpty()) ownerToKeep = m_owner->currentText();

    m_owner->clear();

    try
    {
        for (const auto& component : m_service->ListComponents())
        {
            if (component.type == ps::ComponentType::Detail) continue;
            m_owner->addItem(QString::fromUtf8(component.name.c_str()));
        }
    }
    catch (const ps::PsException& ex)
    {
        showError(QString::fromUtf8(ex.what()));
    }

    selectOwner(ownerToKeep);
}

void SpecificationDialog::selectOwner(const QString& ownerName)
{
    const int index = m_owner->findText(ownerName);
    if (index >= 0) m_owner->setCurrentIndex(index);
}

QStringList SpecificationDialog::componentChoices(const QString& ownerName) const
{
    QStringList choices;
    for (const auto& component : m_service->ListComponents())
    {
        const auto componentName = QString::fromUtf8(component.name.c_str());
        if (componentName == ownerName) continue;
        choices << componentName;
    }
    return choices;
}

void SpecificationDialog::rebuildTree()
{
    const auto selectedOwner = m_owner->currentText();

    m_tree->clear();
    reloadOwners(selectedOwner);

    try
    {
        for (const auto& root : m_service->ListSpecificationRoots())
        {
            auto* rootItem = new QTreeWidgetItem(m_tree);
            rootItem->setText(0, QString::fromUtf8(root.name.c_str()));
            rootItem->setData(0, ROLE_NAME, QString::fromUtf8(root.name.c_str()));
            rootItem->setData(0, ROLE_TYPE, static_cast<int>(root.type));
            rootItem->setToolTip(0, QString::fromUtf8(ps::ToString(root.type).c_str()));

            addChildrenRec(rootItem, QString::fromUtf8(root.name.c_str()), 0);
        }

        m_tree->expandAll();
    }
    catch (const ps::PsException& ex)
    {
        showError(QString::fromUtf8(ex.what()));
    }
}

void SpecificationDialog::addChildrenRec(QTreeWidgetItem* parentItem, const QString& ownerName, int depth)
{
    if (depth > 50) return;

    const auto items = m_service->ListSpecItems(ToUtf8Std(ownerName));
    for (const auto& item : items)
    {
        auto* child = new QTreeWidgetItem(parentItem);
        child->setText(0, QString::fromUtf8(item.partName.c_str()));
        child->setData(0, ROLE_NAME, QString::fromUtf8(item.partName.c_str()));
        child->setData(0, ROLE_TYPE, static_cast<int>(item.type));
        child->setToolTip(
            0,
            QString::fromUtf8("qty=%1, тип=%2")
                .arg(item.qty)
                .arg(QString::fromUtf8(ps::ToString(item.type).c_str())));

        if (item.type != ps::ComponentType::Detail)
            addChildrenRec(child, QString::fromUtf8(item.partName.c_str()), depth + 1);
    }
}

void SpecificationDialog::onFind()
{
    const auto target = m_search->text().trimmed();
    if (target.isEmpty()) return;

    QList<QTreeWidgetItem*> stack;
    for (int i = 0; i < m_tree->topLevelItemCount(); i++)
        stack.push_back(m_tree->topLevelItem(i));

    while (!stack.isEmpty())
    {
        auto* item = stack.takeLast();
        if (item->text(0).compare(target, Qt::CaseInsensitive) == 0)
        {
            m_tree->setCurrentItem(item);
            m_tree->scrollToItem(item);
            selectOwner(item->data(0, ROLE_NAME).toString());
            return;
        }

        for (int i = 0; i < item->childCount(); i++)
            stack.push_back(item->child(i));
    }

    QMessageBox::information(this, QString::fromUtf8("Найти"), QString::fromUtf8("Элемент не найден."));
}

void SpecificationDialog::openAddDialogForOwner(const QString& ownerName)
{
    if (ownerName.isEmpty()) return;

    const auto choices = componentChoices(ownerName);
    if (choices.isEmpty())
    {
        QMessageBox::information(this, QString::fromUtf8("Спецификация"), QString::fromUtf8("Нет доступных компонентов для добавления связи."));
        return;
    }

    SpecItemDialog dlg(this);
    dlg.setComponents(choices);
    if (dlg.exec() != QDialog::Accepted) return;

    m_service->InputSpecItem(
        ToUtf8Std(ownerName),
        ToUtf8Std(dlg.selectedName()),
        static_cast<std::uint16_t>(dlg.qty()));

    rebuildTree();
    selectOwner(ownerName);
}

void SpecificationDialog::onAddFromOwner()
{
    try
    {
        openAddDialogForOwner(m_owner->currentText());
    }
    catch (const ps::PsException& ex)
    {
        showError(QString::fromUtf8(ex.what()));
    }
}

void SpecificationDialog::onContextMenuRequested(const QPoint& pos)
{
    m_ctxItem = m_tree->itemAt(pos);
    if (!m_ctxItem) return;

    QMenu menu(this);
    auto* addAction = menu.addAction(QString::fromUtf8("Добавить"));
    auto* editAction = menu.addAction(QString::fromUtf8("Изменить"));
    auto* deleteAction = menu.addAction(QString::fromUtf8("Удалить"));

    connect(addAction, &QAction::triggered, this, &SpecificationDialog::onAdd);
    connect(editAction, &QAction::triggered, this, &SpecificationDialog::onEdit);
    connect(deleteAction, &QAction::triggered, this, &SpecificationDialog::onDelete);

    const auto type = static_cast<ps::ComponentType>(m_ctxItem->data(0, ROLE_TYPE).toInt());
    if (type == ps::ComponentType::Detail) addAction->setEnabled(false);

    if (!m_ctxItem->parent())
    {
        editAction->setEnabled(false);
        deleteAction->setEnabled(false);
    }

    menu.exec(m_tree->viewport()->mapToGlobal(pos));
}

void SpecificationDialog::onAdd()
{
    if (!m_ctxItem) return;

    const auto ownerName = m_ctxItem->data(0, ROLE_NAME).toString();
    const auto ownerType = static_cast<ps::ComponentType>(m_ctxItem->data(0, ROLE_TYPE).toInt());
    if (ownerType == ps::ComponentType::Detail) return;

    try
    {
        openAddDialogForOwner(ownerName);
    }
    catch (const ps::PsException& ex)
    {
        showError(QString::fromUtf8(ex.what()));
    }
}

void SpecificationDialog::onEdit()
{
    if (!m_ctxItem || !m_ctxItem->parent()) return;

    const auto parentName = m_ctxItem->parent()->data(0, ROLE_NAME).toString();
    const auto oldPartName = m_ctxItem->data(0, ROLE_NAME).toString();

    try
    {
        int oldQty = 1;
        for (const auto& item : m_service->ListSpecItems(ToUtf8Std(parentName)))
        {
            if (QString::fromUtf8(item.partName.c_str()) == oldPartName)
            {
                oldQty = static_cast<int>(item.qty);
                break;
            }
        }

        SpecItemDialog dlg(this);
        dlg.setComponents(componentChoices(parentName));
        dlg.setSelected(oldPartName);
        dlg.setQty(oldQty);

        if (dlg.exec() != QDialog::Accepted) return;

        m_service->UpdateSpecItem(
            ToUtf8Std(parentName),
            ToUtf8Std(oldPartName),
            ToUtf8Std(dlg.selectedName()),
            static_cast<std::uint16_t>(dlg.qty()));

        rebuildTree();
        selectOwner(parentName);
    }
    catch (const ps::PsException& ex)
    {
        showError(QString::fromUtf8(ex.what()));
    }
}

void SpecificationDialog::onDelete()
{
    if (!m_ctxItem || !m_ctxItem->parent()) return;

    const auto parentName = m_ctxItem->parent()->data(0, ROLE_NAME).toString();
    const auto partName = m_ctxItem->data(0, ROLE_NAME).toString();

    const auto question = QString::fromUtf8("Удалить \"%1\" из спецификации \"%2\"?")
        .arg(partName)
        .arg(parentName);
    if (QMessageBox::question(this, QString::fromUtf8("Удалить"), question) != QMessageBox::Yes)
        return;

    try
    {
        m_service->DeleteSpecItem(ToUtf8Std(parentName), ToUtf8Std(partName));
        rebuildTree();
        selectOwner(parentName);
    }
    catch (const ps::PsException& ex)
    {
        showError(QString::fromUtf8(ex.what()));
    }
}
