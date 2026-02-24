#include "MainWindow.h"

#include <QMenuBar>
#include <QMessageBox>
#include <QApplication>
#include <QAction>

#include "services/CatalogService.h"
#include "core/Errors.h"

#include "OpenDialog.h"
#include "ComponentsDialog.h"
#include "SpecificationDialog.h"

// Важно: QString::toStdString() использует локальную кодировку (toLocal8Bit),
// что ломает имена с кириллицей. Всегда передаем в core UTF-8.
static std::string ToUtf8Std(const QString& s)
{
    const auto bytes = s.toUtf8();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

MainWindow::MainWindow()
{
    setWindowTitle(QString::fromUtf8("Многосвязные списки"));
    resize(720, 480);

    m_service = std::make_unique<ps::CatalogService>();

    // Меню как в методичке: Открыть / Компоненты / Спецификация
    auto* mb = menuBar();

    auto* aOpen = mb->addAction(QString::fromUtf8("Открыть"));
    connect(aOpen, &QAction::triggered, this, &MainWindow::onOpenOrCreate);

    auto* aComponents = mb->addAction(QString::fromUtf8("Компоненты"));
    connect(aComponents, &QAction::triggered, this, &MainWindow::onComponents);

    auto* aSpec = mb->addAction(QString::fromUtf8("Спецификация"));
    connect(aSpec, &QAction::triggered, this, &MainWindow::onSpecification);

    mb->addSeparator();
    auto* aClose = mb->addAction(QString::fromUtf8("Закрыть файлы"));
    connect(aClose, &QAction::triggered, this, &MainWindow::onCloseFiles);

    auto* aAbout = mb->addAction(QString::fromUtf8("О программе"));
    connect(aAbout, &QAction::triggered, this, &MainWindow::onAbout);
}

MainWindow::~MainWindow() = default;

void MainWindow::onOpenOrCreate()
{
    OpenDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    try
    {
        if (dlg.isCreate())
            m_service->Create(
                ToUtf8Std(dlg.baseName()),
                dlg.maxNameLen(),
                dlg.prsName().isEmpty() ? std::optional<std::string>{}
                                        : std::optional<std::string>{ToUtf8Std(dlg.prsName())});
        else
            m_service->Open(ToUtf8Std(dlg.baseName()));

        QMessageBox::information(this, QString::fromUtf8("OK"), QString::fromUtf8("Файлы успешно открыты."));
    }
    catch (const ps::PsException& ex)
    {
        QMessageBox::critical(this, QString::fromUtf8("Ошибка"), QString::fromUtf8(ex.what()));
    }
    catch (const std::exception& ex)
    {
        QMessageBox::critical(this, QString::fromUtf8("Ошибка"), QString::fromUtf8(ex.what()));
    }
}

void MainWindow::ensureServiceOpenOrWarn()
{
    if (!m_service->HasOpenFiles())
        throw ps::ValidationException("Файлы не открыты. Выполните Create/Open.");
}

void MainWindow::onComponents()
{
    try
    {
        ensureServiceOpenOrWarn();
        ComponentsDialog dlg(m_service.get(), this);
        dlg.exec();
    }
    catch (const ps::PsException& ex)
    {
        QMessageBox::warning(this, QString::fromUtf8("Внимание"), QString::fromUtf8(ex.what()));
    }
}

void MainWindow::onSpecification()
{
    try
    {
        ensureServiceOpenOrWarn();
        SpecificationDialog dlg(m_service.get(), this);
        dlg.exec();
    }
    catch (const ps::PsException& ex)
    {
        QMessageBox::warning(this, QString::fromUtf8("Внимание"), QString::fromUtf8(ex.what()));
    }
}

void MainWindow::onCloseFiles()
{
    try
    {
        m_service->Close();
        QMessageBox::information(this, QString::fromUtf8("OK"), QString::fromUtf8("Файлы закрыты."));
    }
    catch (const std::exception& ex)
    {
        QMessageBox::critical(this, QString::fromUtf8("Ошибка"), QString::fromUtf8(ex.what()));
    }
}

void MainWindow::onAbout()
{
    QMessageBox::information(
        this,
        QString::fromUtf8("О программе"),
        QString::fromUtf8("ЛР1 №2 — GUI на Qt (Widgets)\n"
                          "Меню: Открыть / Компоненты / Спецификация.\n"
                          "Реализация опирается на сервис CatalogService."));
}
