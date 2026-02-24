#include "ComponentsDialog.h"

#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QMessageBox>

#include "services/CatalogService.h"
#include "core/Errors.h"
#include "domain/Models.h"

static std::string ToUtf8Std(const QString& s)
{
    const auto bytes = s.toUtf8();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

static int typeToIndex(ps::ComponentType t)
{
    switch (t)
    {
    case ps::ComponentType::Product: return 0;
    case ps::ComponentType::Node: return 1;
    case ps::ComponentType::Detail: return 2;
    default: return 2;
    }
}
static ps::ComponentType indexToType(int i)
{
    if (i == 0) return ps::ComponentType::Product;
    if (i == 1) return ps::ComponentType::Node;
    return ps::ComponentType::Detail;
}

ComponentsDialog::ComponentsDialog(ps::CatalogService* service, QWidget* parent)
    : QDialog(parent), m_service(service)
{
    setWindowTitle(QString::fromUtf8("Список компонентов"));
    resize(520, 360);
    setModal(true);

    auto* root = new QVBoxLayout(this);

    // тулбар как в примере
    auto* tb = new QToolBar(this);
    m_actAdd = tb->addAction(QString::fromUtf8("Добавить"));
    m_actEdit = tb->addAction(QString::fromUtf8("Изменить"));
    m_actCancel = tb->addAction(QString::fromUtf8("Отменить"));
    m_actSave = tb->addAction(QString::fromUtf8("Сохранить"));
    m_actDelete = tb->addAction(QString::fromUtf8("Удалить"));

    connect(m_actAdd, &QAction::triggered, this, &ComponentsDialog::onAdd);
    connect(m_actEdit, &QAction::triggered, this, &ComponentsDialog::onEdit);
    connect(m_actCancel, &QAction::triggered, this, &ComponentsDialog::onCancel);
    connect(m_actSave, &QAction::triggered, this, &ComponentsDialog::onSave);
    connect(m_actDelete, &QAction::triggered, this, &ComponentsDialog::onDelete);

    root->addWidget(tb);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({QString::fromUtf8("Наименование"), QString::fromUtf8("Тип")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    root->addWidget(m_table, 1);

    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ComponentsDialog::onSelectionChanged);

    // нижняя панель ввода
    auto* bottom = new QHBoxLayout();
    bottom->addWidget(new QLabel(QString::fromUtf8("Наименование"), this));
    m_name = new QLineEdit(this);
    bottom->addWidget(m_name, 2);

    bottom->addWidget(new QLabel(QString::fromUtf8("Тип"), this));
    m_type = new QComboBox(this);
    m_type->addItems({QString::fromUtf8("Изделие"), QString::fromUtf8("Узел"), QString::fromUtf8("Деталь")});
    bottom->addWidget(m_type, 1);

    root->addLayout(bottom);

    reloadTable();
    setMode(Mode::View);
}

void ComponentsDialog::showError(const QString& msg)
{
    QMessageBox::critical(this, QString::fromUtf8("Ошибка"), msg);
}

void ComponentsDialog::setMode(Mode m)
{
    m_mode = m;

    const bool editing = (m_mode != Mode::View);
    m_name->setEnabled(editing);
    m_type->setEnabled(editing);

    m_actSave->setEnabled(editing);
    m_actCancel->setEnabled(editing);

    const bool hasSel = !m_table->selectedItems().isEmpty();
    m_actEdit->setEnabled(!editing && hasSel);
    m_actDelete->setEnabled(!editing && hasSel);
    m_actAdd->setEnabled(!editing);

    if (m_mode == Mode::View)
    {
        onSelectionChanged();
    }
    else if (m_mode == Mode::Add)
    {
        m_editOldName.clear();
        m_name->clear();
        m_type->setCurrentIndex(2);
        m_name->setFocus();
    }
}

void ComponentsDialog::reloadTable()
{
    try
    {
        auto list = m_service->ListComponents();

        m_table->setRowCount(0);
        m_table->setRowCount(static_cast<int>(list.size()));

        for (int i = 0; i < static_cast<int>(list.size()); i++)
        {
            const auto& r = list[static_cast<std::size_t>(i)];
            auto* itName = new QTableWidgetItem(QString::fromUtf8(r.name.c_str()));
            auto* itType = new QTableWidgetItem(QString::fromUtf8(ps::ToString(r.type).c_str()));
            m_table->setItem(i, 0, itName);
            m_table->setItem(i, 1, itType);
        }
        if (m_table->rowCount() > 0) m_table->selectRow(0);
    }
    catch (const ps::PsException& ex)
    {
        showError(QString::fromUtf8(ex.what()));
    }
}

void ComponentsDialog::onSelectionChanged()
{
    if (m_mode != Mode::View) return;

    const auto items = m_table->selectedItems();
    if (items.isEmpty())
    {
        m_name->clear();
        m_type->setCurrentIndex(2);
        m_actEdit->setEnabled(false);
        m_actDelete->setEnabled(false);
        return;
    }

    const auto name = m_table->item(m_table->currentRow(), 0)->text();
    const auto typeText = m_table->item(m_table->currentRow(), 1)->text();

    m_name->setText(name);
    if (typeText == QString::fromUtf8("Изделие")) m_type->setCurrentIndex(0);
    else if (typeText == QString::fromUtf8("Узел")) m_type->setCurrentIndex(1);
    else m_type->setCurrentIndex(2);

    m_actEdit->setEnabled(true);
    m_actDelete->setEnabled(true);
}

void ComponentsDialog::onAdd()
{
    setMode(Mode::Add);
}

void ComponentsDialog::onEdit()
{
    if (m_table->selectedItems().isEmpty()) return;
    m_editOldName = m_table->item(m_table->currentRow(), 0)->text();
    setMode(Mode::Edit);
    m_name->setFocus();
}

void ComponentsDialog::onCancel()
{
    setMode(Mode::View);
}

void ComponentsDialog::onSave()
{
    try
    {
        const auto nm = m_name->text().trimmed();
        const auto tp = indexToType(m_type->currentIndex());

        if (m_mode == Mode::Add)
        {
            m_service->InputComponent(ToUtf8Std(nm), tp);
        }
        else if (m_mode == Mode::Edit)
        {
            m_service->UpdateComponent(ToUtf8Std(m_editOldName), ToUtf8Std(nm), tp);
        }

        reloadTable();
        setMode(Mode::View);
    }
    catch (const ps::PsException& ex)
    {
        showError(QString::fromUtf8(ex.what()));
    }
}

void ComponentsDialog::onDelete()
{
    if (m_table->selectedItems().isEmpty()) return;
    const auto name = m_table->item(m_table->currentRow(), 0)->text();

    if (QMessageBox::question(this, QString::fromUtf8("Удалить"),
        QString::fromUtf8("Удалить компонент \"%1\"?").arg(name)) != QMessageBox::Yes)
        return;

    try
    {
        m_service->DeleteComponent(ToUtf8Std(name));
        reloadTable();
        setMode(Mode::View);
    }
    catch (const ps::PsException& ex)
    {
        showError(QString::fromUtf8(ex.what()));
    }
}
