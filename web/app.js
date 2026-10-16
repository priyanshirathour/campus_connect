const EVENTS_PATH = "/api/events";
const CLUBS_PATH = "/api/clubs";
const SAVED_KEY = "campus-connect-saved-events";

const eventList = document.querySelector("#event-list");
const clubGrid = document.querySelector("#club-grid");
const categoryList = document.querySelector("#category-list");
const searchInput = document.querySelector("#event-search");
const emptyState = document.querySelector("#empty-state");
const dialog = document.querySelector("#event-dialog");
const userArea = document.querySelector("#user-area");

let events = [];
let clubs = [];
let activeCategory = "All";
let activeView = "all";
let savedEvents = readSavedEvents();
let totalEventCount = 0;
let eventRequestNumber = 0;
let searchTimer;

function readSavedEvents() {
    try {
        // Saved events are a display preference, never an authentication/session store.
        const stored = JSON.parse(localStorage.getItem(SAVED_KEY) || "[]");
        return new Set(Array.isArray(stored) ? stored : []);
    } catch {
        return new Set();
    }
}

function parseEvents(records) {
    if (!Array.isArray(records)) return [];
    return records.filter((event) => event && event.id && event.title && /^\d{4}-\d{2}-\d{2}$/.test(event.date));
}

function parseClubs(records) {
    if (!Array.isArray(records)) return [];
    return records.filter((club) => club && club.id && club.name);
}

function formatDate(dateString, options) {
    const date = new Date(`${dateString}T12:00:00`);
    return Number.isNaN(date.valueOf()) ? dateString : new Intl.DateTimeFormat("en", options).format(date);
}

function clubName(clubId) {
    return clubs.find((club) => club.id === clubId)?.name || "Campus Connect";
}

function makeElement(tag, className, text) {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (text !== undefined) element.textContent = text;
    return element;
}

function renderCategories() {
    categoryList.replaceChildren();
    const categories = ["All", ...new Set(events.map((event) => event.category).filter(Boolean))];
    for (const category of categories) {
        const button = makeElement("button", `filter-link${activeCategory === category ? " selected" : ""}`, category);
        button.type = "button";
        button.addEventListener("click", () => {
            activeCategory = category;
            renderCategories();
            renderEvents();
        });
        categoryList.append(button);
    }
}

function renderEvents() {
    const visibleEvents = events.filter((event) => {
        const matchesCategory = activeCategory === "All" || event.category === activeCategory;
        const matchesView = activeView !== "saved" || savedEvents.has(event.id);
        return matchesCategory && matchesView;
    });

    eventList.replaceChildren(...visibleEvents.map(createEventCard));
    emptyState.hidden = visibleEvents.length > 0;
    document.querySelector("#result-count").textContent = String(visibleEvents.length).padStart(2, "0");
    document.querySelector("#all-count").textContent = String(totalEventCount);
    document.querySelector("#saved-count").textContent = String(savedEvents.size);
}

function createEventCard(event, index) {
    const card = makeElement("article", "event-card");
    card.style.animationDelay = `${Math.min(index * 45, 180)}ms`;

    const dateBox = makeElement("div", "event-date");
    dateBox.append(
        makeElement("span", "month", formatDate(event.date, { month: "short" })),
        makeElement("span", "day", formatDate(event.date, { day: "2-digit" })),
        makeElement("span", "weekday", formatDate(event.date, { weekday: "short" }))
    );

    const info = makeElement("div", "event-meta");
    info.append(makeElement("span", "event-tag", event.category));
    info.append(makeElement("h3", "", event.title));
    info.append(makeElement("p", "", `${event.time} · ${event.venue} · ${clubName(event.clubId)}`));

    const actions = makeElement("div", "event-actions");
    actions.append(makeElement("span", "capacity", `CAPACITY ${event.capacity}`));
    const details = makeElement("button", "event-action", "Details");
    details.type = "button";
    details.addEventListener("click", () => openEventDetails(event));
    actions.append(details);

    const save = makeElement("button", "event-action", savedEvents.has(event.id) ? "Saved" : "Save");
    save.type = "button";
    save.setAttribute("aria-pressed", String(savedEvents.has(event.id)));
    save.setAttribute("aria-label", `${savedEvents.has(event.id) ? "Remove saved event" : "Save event"}: ${event.title}`);
    save.addEventListener("click", () => {
        if (savedEvents.has(event.id)) savedEvents.delete(event.id);
        else savedEvents.add(event.id);
        localStorage.setItem(SAVED_KEY, JSON.stringify([...savedEvents]));
        renderEvents();
    });
    actions.append(save);

    card.append(dateBox, info, actions);
    return card;
}

function renderClubs() {
    clubGrid.replaceChildren(...clubs.map((club) => {
        const card = makeElement("article", "club-card");
        card.append(makeElement("span", "club-id", club.id));
        card.append(makeElement("h3", "", club.name));
        card.append(makeElement("p", "", club.description || "A student-led campus community."));
        return card;
    }));
    document.querySelector("#club-count").textContent = String(clubs.length).padStart(2, "0");
    document.querySelector("#club-summary").textContent = `${clubs.length} student groups. Plenty of ways to find your people.`;
}

function renderUserArea(session) {
    userArea.replaceChildren();
    if (!session?.authenticated) {
        const signIn = makeElement("a", "sign-in-link", "Sign in");
        signIn.href = "login.html";
        const signUp = makeElement("a", "create-account-nav");
        signUp.href = "signup.html";
        signUp.setAttribute("aria-label", "Create account");
        signUp.append(
            makeElement("span", "create-account-desktop", "Create account"),
            makeElement("span", "create-account-mobile", "Join")
        );
        userArea.append(signIn, signUp);
        return;
    }

    const identity = makeElement("span", "signed-in-name", session.displayName);
    const role = makeElement("span", "signed-in-role", session.role === "coordinator" ? "COORDINATOR" : "STUDENT");
    const dashboard = makeElement("a", "dashboard-link", "Dashboard");
    dashboard.href = "dashboard.html";
    const signOut = makeElement("button", "sign-out-button", "Sign out");
    signOut.type = "button";
    signOut.addEventListener("click", async () => {
        signOut.disabled = true;
        try {
            const response = await fetch("/api/auth/logout", { method: "POST", cache: "no-store" });
            if (!response.ok) throw new Error("Could not sign out.");
            renderUserArea(null);
        } catch (error) {
            signOut.disabled = false;
            console.error(error);
        }
    });
    userArea.append(identity, role, dashboard, signOut);
}

function openEventDetails(event) {
    document.querySelector("#dialog-category").textContent = event.category;
    document.querySelector("#dialog-title").textContent = event.title;
    document.querySelector("#dialog-description").textContent = event.description || "Join your campus community for this upcoming event.";
    document.querySelector("#dialog-when").textContent = `${formatDate(event.date, { weekday: "long", month: "long", day: "numeric", year: "numeric" })} · ${event.time}`;
    document.querySelector("#dialog-where").textContent = event.venue;
    document.querySelector("#dialog-host").textContent = clubName(event.clubId);
    document.querySelector("#dialog-capacity").textContent = `${event.capacity} attendees`;
    dialog.showModal();
}

function updateFeature() {
    const featured = events[0];
    if (!featured) return;
    document.querySelector("#feature-date").textContent = `${formatDate(featured.date, { month: "long", day: "numeric" }).toUpperCase()} · ${featured.category.toUpperCase()}`;
    document.querySelector("#feature-title").textContent = featured.title;
    document.querySelector("#feature-description").textContent = `${clubName(featured.clubId)} · ${featured.time} · ${featured.venue}`;
    document.querySelector("#feature-index").textContent = "01";
    document.querySelector("#feature-total").textContent = String(events.length).padStart(2, "0");
    document.querySelector("#feature-action").addEventListener("click", () => openEventDetails(featured));
}

async function loadApi(path) {
    const response = await fetch(path, { cache: "no-store" });
    if (!response.ok) throw new Error(`Could not load ${path} (${response.status})`);
    return response.json();
}

async function loadEvents(query = "") {
    // Ignore out-of-order responses so a slower earlier search cannot replace newer results.
    const requestNumber = ++eventRequestNumber;
    const path = query ? `${EVENTS_PATH}?q=${encodeURIComponent(query)}` : EVENTS_PATH;
    const records = await loadApi(path);
    if (requestNumber !== eventRequestNumber) return;

    events = parseEvents(records);
    if (!query) totalEventCount = events.length;
    renderCategories();
    renderEvents();
    document.querySelector("#snapshot-status").textContent = `Showing ${events.length} live events from C++ services.`;
}

async function start() {
    document.querySelector("#today-label").textContent = new Intl.DateTimeFormat("en", { weekday: "long", month: "long", day: "numeric" }).format(new Date());
    try {
        const [eventRecords, clubRecords, session] = await Promise.all([
            loadApi(EVENTS_PATH), loadApi(CLUBS_PATH), loadApi("/api/auth/session")
        ]);
        events = parseEvents(eventRecords);
        clubs = parseClubs(clubRecords);
        totalEventCount = events.length;
        renderUserArea(session);
        renderCategories();
        renderEvents();
        renderClubs();
        updateFeature();
        document.querySelector("#snapshot-status").textContent = `Showing ${events.length} live events from C++ services.`;
    } catch (error) {
        document.querySelector("#snapshot-status").textContent = "C++ services are unavailable. Start Campus Connect in API mode to load events.";
        document.querySelector("#snapshot-status").classList.add("load-error");
        emptyState.hidden = false;
        console.error(error);
    }
}

searchInput.addEventListener("input", () => {
    window.clearTimeout(searchTimer);
    searchTimer = window.setTimeout(() => {
        loadEvents(searchInput.value.trim()).catch((error) => {
            document.querySelector("#snapshot-status").textContent = "C++ event search is unavailable.";
            console.error(error);
        });
    }, 180);
});
document.querySelectorAll("[data-view]").forEach((button) => {
    button.addEventListener("click", () => {
        activeView = button.dataset.view;
        document.querySelectorAll("[data-view]").forEach((item) => item.classList.toggle("selected", item === button));
        document.querySelector("#list-heading").childNodes[0].textContent = activeView === "saved" ? "Saved events " : "Coming up ";
        renderEvents();
    });
});
document.querySelector("#clear-filters").addEventListener("click", () => {
    activeCategory = "All";
    activeView = "all";
    searchInput.value = "";
    document.querySelectorAll("[data-view]").forEach((item) => item.classList.toggle("selected", item.dataset.view === "all"));
    loadEvents().catch((error) => console.error(error));
});
document.querySelector("#dialog-close").addEventListener("click", () => dialog.close());
document.querySelector("#event-dialog").addEventListener("click", (event) => {
    if (event.target === dialog) dialog.close();
});
start();