#include "SpecificationDialog.h"
#include "SpecItemDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QMessageBox>

#include "services/CatalogService.h"
#include "core/Errors.h"
#include "domain/Models.h"

static constexpr int ROLE_NAME = Qt::UserRole + 1;
static constexpr int ROLE_TYPE = Qt::UserRole + 2;

// Важно: QString::toStdString() использует локальную кодировку (toLocal8Bit),
// что ломает поиск компонентов/спецификаций с кириллицей на Linux.
static std::string ToUtf8Std(const QString& s)
{
    const auto bytes = s.toUtf8();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

SpecificationDialog::SpecificationDialog(ps::CatalogService* service, QWidget* parent)
    : QDialog(parent), m_service(service)
{
    setWindowTitle(QString::fromUtf8("Спецификация"));
    resize(520, 380);
    setModal(true);

    auto* root = new QVBoxLayout(this);

    auto* top = new QHBoxLayout();
    m_search = new QLineEdit(this);
    m_find = new QPushButton(QString::fromUtf8("Найти"), this);
    top->addWidget(m_search, 1);
    top->addWidget(m_find);
    root->addLayout(top);

    connect(m_find, &QPushButton::clicked, this, &SpecificationDialog::onFind);

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

void SpecificationDialog::rebuildTree()
{
    m_tree->clear();

    try
    {
        auto comps = m_service->ListComponents();

        // корни: изделия
        for (const auto& c : comps)
        {
            if (c.type != ps::ComponentType::Product) continue;

            auto* rootItem = new QTreeWidgetItem(m_tree);
            rootItem->setText(0, QString::fromUtf8(c.name.c_str()));
            rootItem->setData(0, ROLE_NAME, QString::fromUtf8(c.name.c_str()));
            rootItem->setData(0, ROLE_TYPE, static_cast<int>(c.type));

            addChildrenRec(rootItem, QString::fromUtf8(c.name.c_str()), 0);
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

    auto owner = ToUtf8Std(ownerName);
    auto items = m_service->ListSpecItems(owner);

    for (const auto& it : items)
    {
        auto* child = new QTreeWidgetItem(parentItem);
        child->setText(0, QString::fromUtf8(it.partName.c_str()));
        child->setData(0, ROLE_NAME, QString::fromUtf8(it.partName.c_str()));
        child->setData(0, ROLE_TYPE, static_cast<int>(it.type));
        // qty не показываем (в методичке на дереве только имена), но можно хранить в tooltip
        child->setToolTip(0, QString::fromUtf8("qty=%1, тип=%2").arg(it.qty).arg(QString::fromUtf8(ps::ToString(it.type).c_str())));

        if (it.type != ps::ComponentType::Detail)
        {
            addChildrenRec(child, QString::fromUtf8(it.partName.c_str()), depth + 1);
        }
    }
}

void SpecificationDialog::onFind()
{
    const auto target = m_search->text().trimmed();
    if (target.isEmpty()) return;

    // простой DFS по items
    QList<QTreeWidgetItem*> stack;
    for (int i = 0; i < m_tree->topLevelItemCount(); i++)
        stack.push_back(m_tree->topLevelItem(i));

    while (!stack.isEmpty())
    {
        auto* it = stack.takeLast();
        if (it->text(0).compare(target, Qt::CaseInsensitive) == 0)
        {
            m_tree->setCurrentItem(it);
            m_tree->scrollToItem(it);
            return;
        }
        for (int k = 0; k < it->childCount(); k++)
            stack.push_back(it->child(k));
    }

    QMessageBox::information(this, QString::fromUtf8("Найти"), QString::fromUtf8("Элемент не найден."));
}

void SpecificationDialog::onContextMenuRequested(const QPoint& pos)
{
    m_ctxItem = m_tree->itemAt(pos);
    if (!m_ctxItem) return;

    QMenu menu(this);
    auto* aAdd = menu.addAction(QString::fromUtf8("Добавить"));
    auto* aEdit = menu.addAction(QString::fromUtf8("Изменить"));
    auto* aDel = menu.addAction(QString::fromUtf8("Удалить"));

    connect(aAdd, &QAction::triggered, this, &SpecificationDialog::onAdd);
    connect(aEdit, &QAction::triggered, this, &SpecificationDialog::onEdit);
    connect(aDel, &QAction::triggered, this, &SpecificationDialog::onDelete);

    // если выбран деталь — добавлять в неё нельзя
    const auto t = static_cast<ps::ComponentType>(m_ctxItem->data(0, ROLE_TYPE).toInt());
    if (t == ps::ComponentType::Detail) aAdd->setEnabled(false);

    // редактировать/удалять корень (изделие) нельзя через спецификацию
    if (!m_ctxItem->parent())
    {
        aEdit->setEnabled(false);
        aDel->setEnabled(false);
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
        // список всех компонентов для выбора
        QStringList allNames;
        for (const auto& c : m_service->ListComponents())
            allNames << QString::fromUtf8(c.name.c_str());

        SpecItemDialog dlg(this);
        dlg.setComponents(allNames);

        if (dlg.exec() != QDialog::Accepted) return;

        m_service->InputSpecItem(ToUtf8Std(ownerName), ToUtf8Std(dlg.selectedName()), static_cast<std::uint16_t>(dlg.qty()));
        rebuildTree();
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
        // достанем текущий qty из спецификации
        int oldQty = 1;
        auto specItems = m_service->ListSpecItems(ToUtf8Std(parentName));
        for (const auto& si : specItems)
        {
            if (QString::fromUtf8(si.partName.c_str()) == oldPartName)
            {
                oldQty = static_cast<int>(si.qty);
                break;
            }
        }

        QStringList allNames;
        for (const auto& c : m_service->ListComponents())
            allNames << QString::fromUtf8(c.name.c_str());

        SpecItemDialog dlg(this);
        dlg.setComponents(allNames);
        dlg.setSelected(oldPartName);
        dlg.setQty(oldQty);

        if (dlg.exec() != QDialog::Accepted) return;

        const auto newPart = dlg.selectedName();
        const auto newQty = dlg.qty();

        // реализация "изменить" через Delete+Input
        m_service->DeleteSpecItem(ToUtf8Std(parentName), ToUtf8Std(oldPartName));
        m_service->InputSpecItem(ToUtf8Std(parentName), ToUtf8Std(newPart), static_cast<std::uint16_t>(newQty));

        rebuildTree();
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

    if (QMessageBox::question(this, QString::fromUtf8("Удалить"),
        QString::fromUtf8("Удалить \"%1\" из спецификации \"%2\"?").arg(partName).arg(parentName)) != QMessageBox::Yes)
        return;

    try
    {
        m_service->DeleteSpecItem(ToUtf8Std(parentName), ToUtf8Std(partName));
        rebuildTree();
    }
    catch (const ps::PsException& ex)
    {
        showError(QString::fromUtf8(ex.what()));
    }
}
