#include "str_quest.h"

#include "bn_vector.h"

namespace str
{
    static const QuestDef QUEST_DEFS[] = {
        {QuestID::COLLECT_HEARTS, "Collect 3 hearts", "Pick up 3 red hearts", "from the ground here.", 3, "Here's some healing."},
        {QuestID::SLAY_ENEMIES, "Slay 5 enemies", "Defeat 5 spearguards", "and return to me.", 5, "You've earned a reward."},
    };

    static constexpr int QUEST_DEF_COUNT = sizeof(QUEST_DEFS) / sizeof(QUEST_DEFS[0]);

    QuestManager::QuestManager() { _log.clear(); }

    QuestSlot* QuestManager::_find_slot(QuestID id)
    {
        for (int i = 0; i < _log.size(); ++i)
            if (_log[i].id == id)
                return &_log[i];
        return nullptr;
    }

    const QuestSlot* QuestManager::_find_slot(QuestID id) const
    {
        for (int i = 0; i < _log.size(); ++i)
            if (_log[i].id == id)
                return &_log[i];
        return nullptr;
    }

    void QuestManager::_ensure_slot(QuestID id)
    {
        if (_find_slot(id))
            return;
        QuestSlot slot;
        slot.id = id;
        slot.status = QuestStatus::NotStarted;
        slot.progress = 0;
        _log.push_back(slot);
    }

    void QuestManager::start_quest(QuestID id)
    {
        _ensure_slot(id);
        QuestSlot* s = _find_slot(id);
        if (s && s->status == QuestStatus::NotStarted)
        {
            s->status = QuestStatus::Active;
            s->progress = 0;
        }
    }

    void QuestManager::notify_collected(CollectibleType type)
    {
        if (type == CollectibleType::HEALTH)
        {
            QuestSlot* s = _find_slot(QuestID::COLLECT_HEARTS);
            if (s && s->status == QuestStatus::Active)
            {
                const QuestDef* def = get_quest_def(QuestID::COLLECT_HEARTS);
                if (def && s->progress < def->target_count)
                    s->progress++;
            }
        }
    }

    void QuestManager::notify_kill()
    {
        QuestSlot* s = _find_slot(QuestID::SLAY_ENEMIES);
        if (s && s->status == QuestStatus::Active)
        {
            const QuestDef* def = get_quest_def(QuestID::SLAY_ENEMIES);
            if (def && s->progress < def->target_count)
                s->progress++;
        }
    }

    void QuestManager::complete_quest(QuestID id)
    {
        QuestSlot* s = _find_slot(id);
        if (s)
        {
            s->status = QuestStatus::Completed;
            _last_completed = id;
        }
    }

    QuestID QuestManager::get_and_clear_last_completed()
    {
        QuestID id = _last_completed;
        _last_completed = QuestID::_COUNT;
        return id;
    }

    QuestStatus QuestManager::get_quest_status(QuestID id) const
    {
        const QuestSlot* s = _find_slot(id);
        return s ? s->status : QuestStatus::NotStarted;
    }

    int QuestManager::get_quest_progress(QuestID id) const
    {
        const QuestSlot* s = _find_slot(id);
        return s ? s->progress : 0;
    }

    bool QuestManager::is_quest_completable(QuestID id) const
    {
        const QuestSlot* s = _find_slot(id);
        const QuestDef* def = get_quest_def(id);
        return s && def && s->status == QuestStatus::Active && s->progress >= def->target_count;
    }

    QuestID QuestManager::get_first_available_quest() const
    {
        for (int i = 0; i < static_cast<int>(QuestID::_COUNT); ++i)
        {
            QuestID id = static_cast<QuestID>(i);
            if (get_quest_status(id) == QuestStatus::NotStarted)
                return id;
        }
        return QuestID::_COUNT;
    }

    QuestID QuestManager::get_first_active_quest() const
    {
        for (int i = 0; i < _log.size(); ++i)
            if (_log[i].status == QuestStatus::Active)
                return _log[i].id;
        return QuestID::_COUNT;
    }

    QuestID QuestManager::get_first_completable_quest() const
    {
        for (int i = 0; i < static_cast<int>(QuestID::_COUNT); ++i)
        {
            QuestID id = static_cast<QuestID>(i);
            if (is_quest_completable(id))
                return id;
        }
        return QuestID::_COUNT;
    }

    const QuestDef* QuestManager::get_quest_def(QuestID id) const
    {
        int idx = static_cast<int>(id);
        if (idx >= 0 && idx < QUEST_DEF_COUNT)
            return &QUEST_DEFS[idx];
        return nullptr;
    }
}
