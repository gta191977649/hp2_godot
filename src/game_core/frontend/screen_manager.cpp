#include "screen_manager.hpp"
#include <algorithm>

namespace hp2::frontend {

void ScreenManager::register_screen(std::string name) {
    if (!is_registered(name)) registry_.push_back(std::move(name));
}

bool ScreenManager::is_registered(const std::string& name) const {
    return std::find(registry_.begin(), registry_.end(), name) != registry_.end();
}

const std::string& ScreenManager::current_screen() const {
    static const std::string empty;
    return current_ ? *current_ : empty;
}

RequestOutcome ScreenManager::request_screen(const std::string& name, bool push_history) {
    if (current_) {
        if (push_history) {
            if (history_.size() >= kHistoryCapacity) return RequestOutcome::HistoryFull;
            history_.push_back(HistoryEntry{*current_});
        }
        current_.reset();
    }
    if (name.empty()) return RequestOutcome::ClearedToNone;
    if (!is_registered(name)) return RequestOutcome::NotFound;
    current_ = name;
    return RequestOutcome::Switched;
}

std::optional<RequestOutcome> ScreenManager::go_back() {
    if (history_.empty()) return std::nullopt;
    const std::string popped = history_.back().name;
    history_.pop_back();
    return request_screen(popped, /*push_history=*/false);
}

void ScreenManager::reset() {
    history_.clear();
}

} // namespace hp2::frontend
