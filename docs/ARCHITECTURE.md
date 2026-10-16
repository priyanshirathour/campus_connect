# Architecture and data-structure mapping

## Layers

1. **Presentation:** `CampusConnectApp` supplies student and coordinator menus. `web/` provides event discovery plus role-specific student and coordinator dashboards over the local HTTP API.
2. **Services:** student, club, event, registration, notification, recommendation, attendance, certificate, feedback, authentication and report operations.
3. **Data structures / algorithms:** ID maps, FIFO queues, adjacency-list graph/BFS, linear text search and sorting.
4. **Persistence:** `Repository` serializes data to MongoDB through `mongocxx`/`bsoncxx`; `saveSnapshots()` writes text snapshots with `fstream` for the report.

`campus_connect --api` starts a separate, loopback-only C++ HTTP process. `WebApiServer` serializes records from services, delegates event search to `EventService::search`, and routes role-protected student and coordinator actions through the same C++ services used by the console. The Node server serves static assets and proxies only allowlisted endpoints. Password hashes use libsodium Argon2id; session cookies are HttpOnly, SameSite, and held in memory for up to eight hours. Student signup links an existing profile using its ID and matching email. Coordinator signup requires a server-side `CAMPUS_CONNECT_COORDINATOR_INVITE`; no default credentials or invitation codes exist. Saved events remain browser-local preferences.

## Required data structures

| Requirement | Implementation | Purpose |
|---|---|---|
| OOP entity classes | `Models.h` | Explicit domain records |
| `unordered_map` | Service indexes keyed by ID | Average constant-time lookup |
| `queue` | `WaitQueue` wraps `std::queue` | FIFO fairness and unread notification order |
| Adjacency-list graph | `RecommendationGraph` | Student-interest-club relationships |
| Graph traversal | BFS | Recommendation reachability |
| Sorting | `EventService::sortedByDate` | Chronological events |
| Searching | `EventService::search` | Case-insensitive title/category/description matching |
| Exceptions | `CampusConnectException` hierarchy | Predictable validation failures |
| Persistence | MongoDB repository + text snapshots | Durable records and report-compatible files |

## Registration lifecycle

- An active registration is rejected as a duplicate.
- If confirmed count is below event capacity, the record becomes `CONFIRMED`; otherwise it becomes `WAITLISTED` and enters the event's FIFO queue.
- Cancelling a waitlisted record removes the student from that queue.
- Cancelling a confirmed record frees a seat and promotes the queue's oldest active waiter. The promoted record is updated and a notification is persisted.
- Attendance requires an active registration. A certificate requires attendance marked `PRESENT`.

## Persistence model

MongoDB is the primary source of truth for runtime application state. The plain-text files in `data/` are snapshots exported on normal exit to preserve the report's `fstream` file-I/O component; they are not an alternate live database.

The event snapshot columns are `id|clubId|title|date|time|capacity|venue|category|description`. Club snapshots remain `id|name|description`. These text files are not used by the live website API.
