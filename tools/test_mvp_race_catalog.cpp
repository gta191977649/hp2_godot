#include "bware/inc/bFile.hpp"
#include "indep/inc/EventTree.hpp"
#include "indep/inc/Fred.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: mvp_race_catalog <complete game asset source>\n";
        return 2;
    }
    try {
        hp2::GameResources resources(std::filesystem::u8path(argv[1]));
        const hp2::TrackInfoResources tracks(resources.Load("GLOBAL/GLOBALB.LZC"));
        const hp2::EventTreeResources events(resources);
        for (std::size_t i = 0; i < tracks.Records().size(); ++i) {
            const auto id = static_cast<int>(i + 1);
            const auto* track = tracks.Find(id);
            if (!track) continue;
            std::cout << "track," << id << ',' << static_cast<int>(track->baseNumber) << ','
                << track->timeOfDay << ',' << static_cast<int>(track->routeKind) << '\n';
            // The selector groups time variants and evaluates both directions
            // on the base track. Progression may lock individual entries.
            for (int mode = 3; mode <= 7; ++mode) {
                hp2::FrontendPlayerManager frontend;
                frontend.gameType = mode;
                for (int reverse = 0; reverse <= 1; ++reverse) {
                    if (!hp2::IsTrackUnavailable(frontend, tracks, track->baseNumber, reverse)) {
                        const bool reward = hp2::IsTrackAnEventReward(events, track->baseNumber, reverse);
                        std::cout << "challenge," << mode << ',' << id << ',' << reverse << ',' << reward << '\n';
                    }
                }
            }
        }
        for (std::size_t i = 0; i < 2; ++i) {
            const auto* tree = events.PrimaryTree(i);
            if (!tree) throw std::runtime_error("Missing authored primary event tree");
            for (const auto& event : tree->Events()) {
                const auto* goal = event.GetObjective();
                if (!goal) throw std::runtime_error("Missing authored event objective");
                std::cout << "objective," << tree->TreeType() << ',' << static_cast<int>(event.number) << ','
                    << goal->kind << ',' << goal->value << '\n';
                for (std::size_t leg = 0; leg < event.trackIndices.size(); ++leg) {
                    const auto* track = event.GetTrack(static_cast<int>(leg));
                    if (!track) throw std::runtime_error("Missing authored event track");
                    RaceParameters race{};
                    track->SetRace(race, tracks);
                    if (!tracks.Find(race.selectedScenario))
                        throw std::runtime_error("Event references an absent authored track");
                    std::cout << "career," << tree->TreeType() << ',' << static_cast<int>(event.number) << ','
                        << static_cast<int>(event.kind) << ',' << leg + 1 << ',' << race.selectedScenario << ','
                        << race.reverse << ',' << race.totalLaps << '\n';
                }
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
