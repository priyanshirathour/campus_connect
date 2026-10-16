#include "services/ClubService.h"
#include "exceptions/CampusConnectException.h"

#include <algorithm>

ClubService::ClubService(Repository &repository, StudentService &students) : repository_(repository), students_(students) { refresh(); }

void ClubService::refresh()
{
    clubsById_.clear();
    for (const auto &club : repository_.allClubs()) {
        clubsById_[club.id] = club;
    }
}

const Club &ClubService::get(const std::string &id) const
{
    const auto it = clubsById_.find(id);
    if (it == clubsById_.end())
        throw ClubNotFoundException(id);
    return it->second;
}

std::vector<Club> ClubService::all() const
{
    std::vector<Club> clubs;
    clubs.reserve(clubsById_.size());
    for (const auto &entry : clubsById_) {
        clubs.push_back(entry.second);
    }

    std::sort(clubs.begin(), clubs.end(), [](const Club &first, const Club &second) {
        return first.name < second.name;
    });
    return clubs;
}

void ClubService::add(const Club &club)
{
    if (club.id.empty() || club.name.empty())
        throw InvalidInputException("Club ID and name are required.");
    if (clubsById_.count(club.id))
        throw InvalidInputException("Club already exists: " + club.id);

    repository_.save(club);
    clubsById_[club.id] = club;
}

void ClubService::addMember(const std::string &clubId, const std::string &studentId)
{
    students_.get(studentId);
    Club club = get(clubId);
    const auto alreadyMember = std::find(club.memberIds.begin(), club.memberIds.end(), studentId);
    if (alreadyMember == club.memberIds.end()) {
        club.memberIds.push_back(studentId);
        repository_.save(club);
        clubsById_[clubId] = club;
    }
}
