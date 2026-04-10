#include "CatalogService.h"
#include "../core/Errors.h"
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace ps
{
    static std::string TrimGuiName(const std::string& s)
    {
        auto b = s.find_first_not_of(' ');
        if (b == std::string::npos) return "";
        auto e = s.find_last_not_of(' ');
        return s.substr(b, e - b + 1);
    }

    bool CatalogService::HasOpenFiles() const { return m_products.IsOpen() && m_specs.IsOpen(); }

    std::string CatalogService::EnsureExt(const std::string& base, const std::string& ext)
    {
        if (base.size() >= ext.size() && base.substr(base.size() - ext.size()) == ext) return base;
        return base + ext;
    }

    void CatalogService::EnsureOpen() const
    {
        if (!HasOpenFiles()) throw ValidationException("Файлы не открыты. Выполните Create или Open.");
    }

    void CatalogService::Create(const std::string& baseName, std::uint16_t maxNameLen, const std::optional<std::string>& prsNameOpt)
    {
        auto prd = EnsureExt(baseName, ".prd");
        auto prs = prsNameOpt.has_value() ? EnsureExt(*prsNameOpt, ".prs") : EnsureExt(baseName, ".prs");
        m_products.Create(prd, maxNameLen, prs);
        m_specs.Create(prs);
    }

    void CatalogService::Open(const std::string& baseName)
    {
        auto prd = EnsureExt(baseName, ".prd");
        m_products.Open(prd);
        auto prs = m_products.PrsPath();
        if (prs.empty()) prs = EnsureExt(baseName, ".prs");
        m_specs.Open(prs);
    }

    void CatalogService::Close()
    {
        m_products.Close();
        m_specs.Close();
    }

    void CatalogService::InputComponent(const std::string& name, ComponentType type)
    {
        EnsureOpen();
        m_products.AddComponent(name, type);
    }

    void CatalogService::UpdateComponent(const std::string& oldName, const std::string& newName, ComponentType newType)
    {
        EnsureOpen();

        auto oldOpt = m_products.FindActiveByName(oldName);
        if (!oldOpt.has_value()) throw ValidationException("Компонент не найден.");

        auto oldRec = *oldOpt;
        auto nm = TrimGuiName(newName);
        if (nm.empty()) throw ValidationException("Пустое имя компонента.");
        if (nm.size() > m_products.MaxNameLen()) throw ValidationException("Имя компонента длиннее maxNameLen (Create).");

        if (TrimGuiName(oldRec.name) != nm && m_products.FindActiveByName(nm).has_value())
            throw ValidationException("Дублирование имен компонентов.");

        m_products.UpdateComponent(oldRec.fileOffset, nm, newType);
        m_products.RebuildAlphabeticalLinks();
    }

    std::vector<SpecRecord> CatalogService::ReadSpecChain(std::uint32_t firstSpecPtr)
    {
        std::vector<SpecRecord> out;
        std::uint32_t cur = firstSpecPtr;
        while (cur != NullPtr)
        {
            auto r = m_specs.ReadRecordAt(cur);
            if (!r.deleted) out.push_back(r);
            cur = r.nextPtr;
        }
        return out;
    }

    bool CatalogService::WouldCreateCycle(std::uint32_t ownerPtr, std::uint32_t partPtr)
    {
        std::vector<std::uint32_t> stack{ partPtr };
        std::unordered_set<std::uint32_t> visited;

        while (!stack.empty())
        {
            auto current = stack.back();
            stack.pop_back();

            if (!visited.insert(current).second) continue;
            if (current == ownerPtr) return true;

            auto component = m_products.ReadRecordAt(current);
            if (component.deleted || component.type == ComponentType::Detail) continue;

            for (const auto& child : ReadSpecChain(component.firstSpecPtr))
                stack.push_back(child.componentPtr);
        }

        return false;
    }

    void CatalogService::InputSpecItem(const std::string& ownerName, const std::string& partName, std::uint16_t qty)
    {
        EnsureOpen();

        auto ownerOpt = m_products.FindActiveByName(ownerName);
        if (!ownerOpt.has_value()) throw ValidationException("Компонент-родитель не найден.");

        auto partOpt = m_products.FindActiveByName(partName);
        if (!partOpt.has_value()) throw ValidationException("Комплектующее отсутствует в списке компонентов.");

        auto owner = *ownerOpt;
        auto part = *partOpt;

        if (owner.type == ComponentType::Detail) throw ValidationException("Для детали нельзя добавлять спецификацию.");
        if (owner.fileOffset == part.fileOffset) throw ValidationException("Компонент не может входить в собственную спецификацию.");
        if (WouldCreateCycle(owner.fileOffset, part.fileOffset))
            throw ValidationException("Добавление связи создаёт цикл в структуре.");

        for (const auto& spec : ReadSpecChain(owner.firstSpecPtr))
        {
            if (spec.componentPtr == part.fileOffset)
                throw ValidationException("Такая связь уже указана в спецификации.");
        }

        auto newSpecOff = m_specs.AddSpecItem(part.fileOffset, qty);
        if (owner.firstSpecPtr == NullPtr)
        {
            m_products.UpdatePointers(owner.fileOffset, newSpecOff, owner.nextPtr);
            return;
        }

        std::uint32_t cur = owner.firstSpecPtr;
        while (true)
        {
            auto r = m_specs.ReadRecordAt(cur);
            if (r.nextPtr == NullPtr)
            {
                m_specs.UpdateNext(cur, newSpecOff);
                break;
            }
            cur = r.nextPtr;
        }
    }

    void CatalogService::UpdateSpecItem(const std::string& ownerName, const std::string& oldPartName, const std::string& newPartName, std::uint16_t qty)
    {
        EnsureOpen();

        auto ownerOpt = m_products.FindActiveByName(ownerName);
        if (!ownerOpt.has_value()) throw ValidationException("Компонент-родитель не найден.");

        auto newPartOpt = m_products.FindActiveByName(newPartName);
        if (!newPartOpt.has_value()) throw ValidationException("Комплектующее отсутствует в списке компонентов.");

        auto owner = *ownerOpt;
        auto newPart = *newPartOpt;

        if (owner.type == ComponentType::Detail) throw ValidationException("У детали нет спецификации.");
        if (owner.fileOffset == newPart.fileOffset) throw ValidationException("Компонент не может входить в собственную спецификацию.");
        if (WouldCreateCycle(owner.fileOffset, newPart.fileOffset))
            throw ValidationException("Добавление связи создаёт цикл в структуре.");

        std::uint32_t targetSpecOffset = NullPtr;
        std::uint32_t cur = owner.firstSpecPtr;
        while (cur != NullPtr)
        {
            auto spec = m_specs.ReadRecordAt(cur);
            if (!spec.deleted)
            {
                auto part = m_products.ReadRecordAt(spec.componentPtr);
                if (part.name == oldPartName)
                {
                    targetSpecOffset = spec.fileOffset;
                }
                else if (part.fileOffset == newPart.fileOffset)
                {
                    throw ValidationException("Такая связь уже указана в спецификации.");
                }
            }
            cur = spec.nextPtr;
        }

        if (targetSpecOffset == NullPtr)
            throw ValidationException("Комплектующее в спецификации не найдено.");

        m_specs.UpdateSpecItem(targetSpecOffset, newPart.fileOffset, qty);
    }

    void CatalogService::DeleteComponent(const std::string& name)
    {
        EnsureOpen();
        auto recOpt = m_products.FindActiveByName(name);
        if (!recOpt.has_value()) throw ValidationException("Компонент не найден.");

        auto rec = *recOpt;
        if (m_specs.HasActiveReferenceToComponent(rec.fileOffset))
            throw ValidationException("Невозможно удалить: на компонент есть ссылки в спецификациях других компонентов.");

        m_products.MarkDeleted(rec.fileOffset, true);
    }

    void CatalogService::DeleteSpecItem(const std::string& ownerName, const std::string& partName)
    {
        EnsureOpen();

        auto ownerOpt = m_products.FindActiveByName(ownerName);
        if (!ownerOpt.has_value()) throw ValidationException("Компонент-родитель не найден.");

        auto owner = *ownerOpt;
        if (owner.type == ComponentType::Detail) throw ValidationException("У детали нет спецификации.");
        if (owner.firstSpecPtr == NullPtr) throw ValidationException("Спецификация пуста.");

        std::uint32_t cur = owner.firstSpecPtr;
        while (cur != NullPtr)
        {
            auto sr = m_specs.ReadRecordAt(cur);
            auto comp = m_products.ReadRecordAt(sr.componentPtr);

            if (!sr.deleted && comp.name == partName)
            {
                m_specs.MarkDeleted(sr.fileOffset, true);
                return;
            }

            cur = sr.nextPtr;
        }

        throw ValidationException("Комплектующее в спецификации не найдено.");
    }

    void CatalogService::RestoreAll()
    {
        EnsureOpen();
        for (const auto& r : m_products.ReadAllRecords())
        {
            if (r.deleted) m_products.MarkDeleted(r.fileOffset, false);
        }
        m_products.RebuildAlphabeticalLinks();

        std::unordered_set<std::uint32_t> visitedSpecOffsets;
        for (const auto& component : m_products.ReadAllRecords())
        {
            std::uint32_t cur = component.firstSpecPtr;
            while (cur != NullPtr)
            {
                if (!visitedSpecOffsets.insert(cur).second) break;

                auto spec = m_specs.ReadRecordAt(cur);
                if (spec.deleted) m_specs.MarkDeleted(spec.fileOffset, false);
                cur = spec.nextPtr;
            }
        }
    }

    void CatalogService::RestoreComponent(const std::string& name)
    {
        EnsureOpen();

        bool found = false;
        for (const auto& r : m_products.ReadAllRecords())
        {
            if (r.name == name)
            {
                found = true;
                if (r.deleted) m_products.MarkDeleted(r.fileOffset, false);
            }
        }

        if (!found) throw ValidationException("Компонент не найден.");
        m_products.RebuildAlphabeticalLinks();
    }

    void CatalogService::RestoreSpecItem(const std::string& ownerName, const std::string& partName)
    {
        EnsureOpen();

        auto ownerOpt = m_products.FindActiveByName(ownerName);
        if (!ownerOpt.has_value()) throw ValidationException("Компонент-родитель не найден.");

        auto partOpt = m_products.FindActiveByName(partName);
        if (!partOpt.has_value()) throw ValidationException("Комплектующее отсутствует в списке компонентов.");

        auto owner = *ownerOpt;
        auto part = *partOpt;

        if (owner.type == ComponentType::Detail) throw ValidationException("У детали нет спецификации.");
        if (owner.fileOffset == part.fileOffset) throw ValidationException("Компонент не может входить в собственную спецификацию.");
        if (WouldCreateCycle(owner.fileOffset, part.fileOffset))
            throw ValidationException("Добавление связи создаёт цикл в структуре.");

        std::uint32_t cur = owner.firstSpecPtr;
        std::uint32_t deletedInChain = NullPtr;
        while (cur != NullPtr)
        {
            auto sr = m_specs.ReadRecordAt(cur);
            if (sr.componentPtr == part.fileOffset)
            {
                if (!sr.deleted)
                    throw ValidationException("Такая связь уже активна в спецификации.");
                if (deletedInChain == NullPtr)
                    deletedInChain = sr.fileOffset;
            }

            cur = sr.nextPtr;
        }

        if (deletedInChain != NullPtr)
        {
            m_specs.MarkDeleted(deletedInChain, false);
            return;
        }

        std::unordered_set<std::uint32_t> linkedSpecOffsets;
        for (const auto& component : m_products.ReadAllRecords())
        {
            std::uint32_t specPtr = component.firstSpecPtr;
            while (specPtr != NullPtr)
            {
                if (!linkedSpecOffsets.insert(specPtr).second) break;
                auto sr = m_specs.ReadRecordAt(specPtr);
                specPtr = sr.nextPtr;
            }
        }

        std::uint32_t targetOffset = NullPtr;
        for (const auto& sr : m_specs.ReadAllRecords())
        {
            if (!sr.deleted || sr.componentPtr != part.fileOffset)
                continue;
            if (linkedSpecOffsets.find(sr.fileOffset) != linkedSpecOffsets.end())
                continue;

            if (targetOffset != NullPtr)
                throw ValidationException("Нельзя однозначно восстановить связь: найдено несколько удалённых записей.");

            targetOffset = sr.fileOffset;
        }

        if (targetOffset == NullPtr)
            throw ValidationException("Удалённая связь в спецификации не найдена.");

        m_specs.UpdateNext(targetOffset, NullPtr);
        m_specs.MarkDeleted(targetOffset, false);

        if (owner.firstSpecPtr == NullPtr)
        {
            m_products.UpdatePointers(owner.fileOffset, targetOffset, owner.nextPtr);
            return;
        }

        std::uint32_t last = owner.firstSpecPtr;
        while (true)
        {
            auto sr = m_specs.ReadRecordAt(last);
            if (sr.nextPtr == NullPtr) break;
            last = sr.nextPtr;
        }
        m_specs.UpdateNext(last, targetOffset);
    }

    std::vector<ComponentRecord> CatalogService::ListComponents()
    {
        EnsureOpen();

        std::vector<ComponentRecord> out;
        std::uint32_t cur = m_products.Header().headPtr;
        while (cur != NullPtr)
        {
            auto r = m_products.ReadRecordAt(cur);
            if (!r.deleted) out.push_back(r);
            cur = r.nextPtr;
        }
        return out;
    }

    std::vector<ComponentRecord> CatalogService::ListSpecificationRoots()
    {
        EnsureOpen();

        auto components = ListComponents();
        std::unordered_set<std::uint32_t> referenced;

        for (const auto& owner : components)
        {
            if (owner.type == ComponentType::Detail) continue;

            for (const auto& spec : ReadSpecChain(owner.firstSpecPtr))
                referenced.insert(spec.componentPtr);
        }

        std::vector<ComponentRecord> roots;
        for (const auto& component : components)
        {
            if (component.type == ComponentType::Product)
            {
                roots.push_back(component);
                continue;
            }

            if (component.type == ComponentType::Node && referenced.find(component.fileOffset) == referenced.end())
                roots.push_back(component);
        }

        return roots;
    }

    std::vector<SpecItemView> CatalogService::ListSpecItems(const std::string& ownerName)
    {
        EnsureOpen();

        auto ownerOpt = m_products.FindActiveByName(ownerName);
        if (!ownerOpt.has_value()) throw ValidationException("Компонент-родитель не найден.");

        auto owner = *ownerOpt;
        if (owner.type == ComponentType::Detail) throw ValidationException("У детали нет спецификации.");

        std::vector<SpecItemView> out;
        for (const auto& s : ReadSpecChain(owner.firstSpecPtr))
        {
            auto c = m_products.ReadRecordAt(s.componentPtr);
            SpecItemView v;
            v.partName = c.name;
            v.qty = s.qty;
            v.type = c.type;
            out.push_back(v);
        }
        return out;
    }

    void CatalogService::PrintTreeRec(std::string& out, const ComponentRecord& node, const std::string& prefix, bool isLast, int depth)
    {
        if (depth > 50)
        {
            out += prefix + (isLast ? "\\-- " : "+-- ") + "[...] (слишком глубокая спецификация)\n";
            return;
        }

        out += prefix + (isLast ? "\\-- " : "+-- ");
        out += node.name + " (" + ToString(node.type) + ")\n";

        if (node.type == ComponentType::Detail) return;

        auto children = ReadSpecChain(node.firstSpecPtr);
        for (std::size_t i = 0; i < children.size(); i++)
        {
            auto childComp = m_products.ReadRecordAt(children[i].componentPtr);
            auto nextPrefix = prefix + (isLast ? "    " : "|   ");
            PrintTreeRec(out, childComp, nextPrefix, i + 1 == children.size(), depth + 1);
        }
    }

    std::string CatalogService::PrintSpecTree(const std::string& name)
    {
        EnsureOpen();

        auto compOpt = m_products.FindActiveByName(name);
        if (!compOpt.has_value()) throw ValidationException("Компонент не найден.");

        auto comp = *compOpt;
        if (comp.type == ComponentType::Detail) throw ValidationException("Для детали Print(имя) недопустима.");

        std::string out = comp.name + " (" + ToString(comp.type) + ")\n";
        auto children = ReadSpecChain(comp.firstSpecPtr);
        for (std::size_t i = 0; i < children.size(); i++)
        {
            auto childComp = m_products.ReadRecordAt(children[i].componentPtr);
            PrintTreeRec(out, childComp, "", i + 1 == children.size(), 0);
        }
        return out;
    }

    std::string CatalogService::HelpText() const
    {
        std::ostringstream oss;
        oss
            << "Команды:\n"
            << "  Create имяФайла(максДлинаИмени[, имяФайлаСпецификаций])\n"
            << "  Create имяФайла максДлина [имяФайлаСпецификаций]\n"
            << "  Open имяФайла\n"
            << "  Input(имяКомпонента, тип)                 // тип: Изделие | Узел | Деталь\n"
            << "  Input(имяКомпонента/имяКомплектующего[, qty])\n"
            << "  Delete(имяКомпонента)\n"
            << "  Delete(имяКомпонента/имяКомплектующего)\n"
            << "  Restore(имяКомпонента)\n"
            << "  Restore(имяКомпонента/имяКомплектующего)\n"
            << "  Restore(*)\n"
            << "  Truncate\n"
            << "  Print(имяКомпонента)\n"
            << "  Print(*)\n"
            << "  Help [имяФайла]\n"
            << "  Exit\n";
        return oss.str();
    }

    void CatalogService::Truncate()
    {
        EnsureOpen();
        TruncateRebuildFiles();
        m_products.RebuildAlphabeticalLinks();
    }

    void CatalogService::TruncateRebuildFiles()
    {
        const auto prdOld = m_products.PrdPath();
        const auto prsOld = m_products.PrsPath();
        const auto prdTmp = prdOld + ".tmp";
        const auto prsTmp = prsOld + ".tmp";

        auto allComponents = m_products.ReadAllRecords();
        std::vector<ComponentRecord> activeComps;
        for (auto& c : allComponents)
        {
            if (!c.deleted) activeComps.push_back(c);
        }

        std::unordered_map<std::uint32_t, std::uint32_t> remap;

        ProductFile newPrd;
        newPrd.Create(prdTmp, m_products.MaxNameLen(), prsTmp);
        SpecFile newPrs;
        newPrs.Create(prsTmp);

        for (auto& c : activeComps)
        {
            auto appended = newPrd.AddComponent(c.name, c.type);
            remap[c.fileOffset] = appended.fileOffset;
        }

        for (auto& c : activeComps)
        {
            if (c.type == ComponentType::Detail) continue;

            std::uint32_t newFirst = NullPtr;
            std::uint32_t newPrev = NullPtr;
            std::uint32_t cur = c.firstSpecPtr;

            while (cur != NullPtr)
            {
                auto sr = m_specs.ReadRecordAt(cur);
                cur = sr.nextPtr;

                if (sr.deleted) continue;

                auto it = remap.find(sr.componentPtr);
                if (it == remap.end()) continue;

                auto newSpecOff = newPrs.AddSpecItem(it->second, sr.qty);
                if (newFirst == NullPtr) newFirst = newSpecOff;
                else newPrs.UpdateNext(newPrev, newSpecOff);
                newPrev = newSpecOff;
            }

            auto newOwnerOff = remap[c.fileOffset];
            auto newOwner = newPrd.ReadRecordAt(newOwnerOff);
            newPrd.UpdatePointers(newOwner.fileOffset, newFirst, newOwner.nextPtr);
        }

        m_products.Close();
        m_specs.Close();

        std::remove(prdOld.c_str());
        std::remove(prsOld.c_str());
        std::rename(prdTmp.c_str(), prdOld.c_str());
        std::rename(prsTmp.c_str(), prsOld.c_str());

        m_products.Open(prdOld);
        m_specs.Open(prsOld);
    }
}
