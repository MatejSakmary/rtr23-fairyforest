#pragma once

#include "../fairy_forest.hpp"
#include <vector>
namespace ff
{
    template <typename T>
    struct SlotMap
    {
      private:
        std::vector<T> slots = {};
        std::vector<u32> versions = {};
        std::vector<size_t> free_list = {};

      public:
        struct Id
        {
            u32 index = {};
            u32 version = {};
        };
        static inline constexpr Id EMPTY_ID = {0, 0};
        auto create_slot(T && v = {}) -> Id
        {
            if (free_list.size() > 0)
            {
                u32 const index = free_list.back();
                free_list.pop_back();
                slots[index] = std::move(v);
                return Id{index, versions[index]};
            }
            else
            {
                u32 const index = static_cast<u32>(slots.size());
                slots.emplace_back(std::move(v));
                versions.emplace_back(1u);
                return Id{index, 1u};
            }
        }
        auto destroy_slot(Id id) -> bool
        {
            if (this->is_id_valid(id))
            {
                slots[static_cast<size_t>(id.index)] = {};
                versions[static_cast<size_t>(id.index)] += 1;
                if (versions[static_cast<size_t>(id.index)] < std::numeric_limits<u32>::max())
                {
                    free_list.push_back(id.index);
                }
                return true;
            }
            return false;
        }
        auto slot(Id id) -> T *
        {
            if (is_id_valid(id))
            {
                return &slots[static_cast<size_t>(id.index)];
            }
            return nullptr;
        }
        auto slot(Id id) const -> T const *
        {
            if (is_id_valid(id))
            {
                return &slots[static_cast<size_t>(id.index)];
            }
            return nullptr;
        }
        auto is_id_valid(Id id) const -> bool
        {
            auto const uz_index = static_cast<size_t>(id.index);
            return uz_index < slots.size() && versions[uz_index] == id.version;
        }
        auto size() const -> usize
        {
            return slots.size() - free_list.size();
        }
        auto capacity() const -> usize
        {
            return slots.size();
        }
    };
} // namespace ff