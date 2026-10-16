#include "services/EventService.h"
#include "exceptions/CampusConnectException.h"
#include "utils/Utils.h"

#include <algorithm>

EventService::EventService(Repository& repository, ClubService& clubs):repository_(repository),clubs_(clubs){refresh();}

void EventService::refresh() {
	eventsById_.clear();
	for (const auto& event : repository_.allEvents()) {
		eventsById_[event.id] = event;
	}
}

const Event& EventService::get(const std::string& id) const {
	const auto event = eventsById_.find(id);
	if (event == eventsById_.end()) {
		throw EventNotFoundException(id);
	}
	return event->second;
}

std::vector<Event> EventService::all() const {
	std::vector<Event> events;
	events.reserve(eventsById_.size());
	for (const auto& entry : eventsById_) {
		events.push_back(entry.second);
	}
	return events;
}

std::vector<Event> EventService::sortedByDate() const {
	auto events = all();
	std::sort(events.begin(), events.end(), [](const Event& first, const Event& second) {
		return first.date + first.time < second.date + second.time;
	});
	return events;
}

std::vector<Event> EventService::search(const std::string& query) const {
	const auto normalizedQuery = toLowerCopy(query);
	std::vector<Event> matches;

	for (const auto& event : all()) {
		const bool matchesTitle = toLowerCopy(event.title).find(normalizedQuery) != std::string::npos;
		const bool matchesCategory = toLowerCopy(event.category).find(normalizedQuery) != std::string::npos;
		const bool matchesDescription = toLowerCopy(event.description).find(normalizedQuery) != std::string::npos;
		if (matchesTitle || matchesCategory || matchesDescription) {
			matches.push_back(event);
		}
	}

	std::sort(matches.begin(), matches.end(), [](const Event& first, const Event& second) {
		return first.date + first.time < second.date + second.time;
	});
	return matches;
}

void EventService::add(const Event& event) {
	if (event.id.empty() || event.title.empty()) {
		throw InvalidInputException("Event ID and title are required.");
	}
	if (event.capacity < 1) {
		throw InvalidInputException("Event capacity must be at least 1.");
	}
	if (eventsById_.count(event.id)) {
		throw InvalidInputException("Event already exists: " + event.id);
	}

	clubs_.get(event.clubId);
	repository_.save(event);
	eventsById_[event.id] = event;
}

void EventService::update(const Event& event) {
	if (!eventsById_.count(event.id)) {
		throw EventNotFoundException(event.id);
	}
	if (event.capacity < 1) {
		throw InvalidInputException("Event capacity must be at least 1.");
	}

	clubs_.get(event.clubId);
	repository_.save(event);
	eventsById_[event.id] = event;
}
