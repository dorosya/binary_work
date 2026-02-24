#pragma once
#include <QMainWindow>
#include <memory>

namespace ps { class CatalogService; }

class MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();

private slots:
    void onOpenOrCreate();
    void onComponents();
    void onSpecification();
    void onCloseFiles();
    void onAbout();

private:
    void ensureServiceOpenOrWarn();

    std::unique_ptr<ps::CatalogService> m_service;
};
