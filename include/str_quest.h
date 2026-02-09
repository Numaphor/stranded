#ifndef STR_QUEST_H
#define STR_QUEST_H

#include "bn_vector.h"
#include "bn_string_view.h"

#include "str_collectible.h"

namespace str
{
    enum class QuestID
    {
        COLLECT_HEARTS,
        SLAY_ENEMIES,
        _COUNT
    };

    enum class QuestStatus
    {
        NotStarted,
        Active,
        Completed
    };

    struct QuestSlot
    {
        QuestID id = QuestID::COLLECT_HEARTS;
        QuestStatus status = QuestStatus::NotStarted;
        int progress = 0;
    };

    struct QuestDef
    {
        QuestID id;
        bn::string_view title;
        bn::string_view objective;       // First line of objective (short, fits GBA screen)
        bn::string_view objective_line2; // Optional second line (empty if not used)
        int target_count;
        bn::string_view reward_description;
    };

    class QuestManager
    {
    public:
        QuestManager();

        void start_quest(QuestID id);
        void notify_collected(CollectibleType type);
        void notify_kill();
        void complete_quest(QuestID id);

        [[nodiscard]] QuestStatus get_quest_status(QuestID id) const;
        [[nodiscard]] int get_quest_progress(QuestID id) const;
        [[nodiscard]] bool is_quest_completable(QuestID id) const;

        [[nodiscard]] QuestID get_first_available_quest() const;
        [[nodiscard]] QuestID get_first_active_quest() const;
        [[nodiscard]] QuestID get_first_completable_quest() const;

        [[nodiscard]] const QuestDef* get_quest_def(QuestID id) const;

        // For World to apply reward: returns last completed quest and clears it (call once per turn-in).
        [[nodiscard]] QuestID get_and_clear_last_completed();

    private:
        static constexpr int MAX_QUESTS = 8;
        bn::vector<QuestSlot, MAX_QUESTS> _log;
        QuestID _last_completed = QuestID::_COUNT;

        QuestSlot* _find_slot(QuestID id);
        const QuestSlot* _find_slot(QuestID id) const;
        void _ensure_slot(QuestID id);
    };
}

#endif
