const userArea = document.querySelector("#user-area");
const dashboardStatus = document.querySelector("#dashboard-status");
const toast = document.querySelector("#dashboard-toast");
let session;
let events = [];
let clubs = [];
let students = [];

function element(tag, className, text) {
    const item = document.createElement(tag);
    if (className) item.className = className;
    if (text !== undefined) item.textContent = text;
    return item;
}

async function requestJson(path, options = {}) {
    const response = await fetch(path, { cache: "no-store", ...options });
    const result = await response.json();
    if (response.status === 401) {
        window.location.assign("/login.html");
        throw new Error("Your session expired. Sign in again.");
    }
    if (!response.ok) throw new Error(result.error || "The request could not be completed.");
    return result;
}

function showToast(message, isError = false) {
    toast.textContent = message;
    toast.classList.toggle("error", isError);
    toast.hidden = false;
    window.clearTimeout(showToast.timeout);
    showToast.timeout = window.setTimeout(() => { toast.hidden = true; }, 3500);
}

function fillSelect(select, records, labelFor) {
    select.replaceChildren();
    for (const record of records) {
        const option = document.createElement("option");
        option.value = record.id;
        option.textContent = labelFor(record);
        select.append(option);
    }
}

function renderHeader() {
    userArea.replaceChildren();
    const identity = element("span", "signed-in-name", session.displayName);
    const role = element("span", "signed-in-role", session.role === "coordinator" ? "COORDINATOR" : "STUDENT");
    const signOut = element("button", "sign-out-button", "Sign out");
    signOut.type = "button";
    signOut.addEventListener("click", async () => {
        await requestJson("/api/auth/logout", { method: "POST" });
        window.location.assign("/");
    });
    userArea.append(identity, role, signOut);
}

function renderMetrics(container, metrics) {
    container.replaceChildren(...metrics.map(([label, value]) => {
        const metric = element("div", "metric");
        metric.append(element("span", "metric-value", String(value)));
        metric.append(element("span", "metric-label", label));
        return metric;
    }));
}

function renderRecords(container, records, makeRecord) {
    container.replaceChildren();
    if (!records.length) {
        container.append(element("p", "empty-record", "Nothing to show yet."));
        return;
    }
    container.append(...records.map(makeRecord));
}

function makeRecord(title, detail, state = "") {
    const row = element("div", "record-item");
    const main = element("div", "record-main");
    main.append(element("span", "record-title", title));
    if (detail) main.append(element("span", "record-detail", detail));
    row.append(main);
    if (state) row.append(element("span", `record-state${state === "WAITLISTED" ? " waitlisted" : ""}`, state));
    return row;
}

function setOptions() {
    const eventLabel = (event) => `${event.date} · ${event.title}`;
    document.querySelectorAll("#register-event, #feedback-event, #attendance-event, #certificate-event, #report-event")
        .forEach((select) => fillSelect(select, events, eventLabel));
    document.querySelectorAll("#event-club, #member-club").forEach((select) => {
        fillSelect(select, clubs, (club) => `${club.name} (${club.id})`);
    });
    document.querySelectorAll("#member-student, #attendance-student, #certificate-student").forEach((select) => {
        fillSelect(select, students, (student) => `${student.name} (${student.id})`);
    });
}

async function loadStudentDashboard() {
    const data = await requestJson("/api/student/dashboard");
    events = await requestJson("/api/events");
    setOptions();
    document.querySelector("#dashboard-title").textContent = `Welcome, ${data.profile.name}`;
    document.querySelector("#dashboard-subtitle").textContent = `${data.profile.department} · ${data.profile.email}`;
    const active = data.registrations.filter((item) => item.status !== "CANCELLED");
    const waitlisted = active.filter((item) => item.status === "WAITLISTED").length;
    renderMetrics(document.querySelector("#student-metrics"), [
        ["Active registrations", active.length], ["Waitlisted", waitlisted],
        ["Unread notifications", data.notifications.length], ["Certificates", data.certificates.length]
    ]);

    renderRecords(document.querySelector("#registration-list"), data.registrations, (registration) => {
        const event = events.find((item) => item.id === registration.eventId);
        const detail = event ? `${event.date} · ${event.time} · ${event.venue}` : registration.createdAt;
        const row = makeRecord(event?.title || registration.eventId, detail, registration.status);
        if (registration.waitlistPosition) {
            row.querySelector(".record-detail").textContent += ` · Waitlist position ${registration.waitlistPosition}`;
        }
        if (registration.status !== "CANCELLED") {
            const cancel = element("button", "record-action", "Cancel");
            cancel.type = "button";
            cancel.addEventListener("click", () => runAction(async () => {
                await requestJson("/api/student/registrations/cancel", {
                    method: "POST", headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({ eventId: registration.eventId })
                });
                showToast("Registration cancelled.");
                await loadStudentDashboard();
            }));
            row.append(cancel);
        }
        return row;
    });

    renderRecords(document.querySelector("#notification-list"), data.notifications,
        (notification) => makeRecord(notification.message, notification.createdAt));
    renderRecords(document.querySelector("#recommended-clubs"), data.recommendedClubs,
        (club) => makeRecord(club.name, club.description));
    renderRecords(document.querySelector("#recommended-events"), data.recommendedEvents,
        (event) => makeRecord(event.title, `${event.date} · ${event.venue}`));
    renderRecords(document.querySelector("#certificate-list"), data.certificates,
        (certificate) => makeRecord(certificate.title, certificate.issuedAt));

    document.querySelector("#student-dashboard").hidden = false;
    document.querySelector("#dashboard-role").textContent = "STUDENT DASHBOARD";
}

async function loadCoordinatorDashboard() {
    const data = await requestJson("/api/coordinator/dashboard");
    students = data.students;
    events = data.events;
    clubs = data.clubs;
    setOptions();
    document.querySelector("#dashboard-title").textContent = `Welcome, ${session.displayName}`;
    document.querySelector("#dashboard-subtitle").textContent = "Manage events and club operations.";
    renderMetrics(document.querySelector("#coordinator-metrics"), Object.entries(data.summary));
    renderRecords(document.querySelector("#coordinator-event-list"), events,
        (event) => makeRecord(event.title, `${event.id} · ${event.date} ${event.time} · ${event.venue} · Capacity ${event.capacity}`));
    renderRecords(document.querySelector("#coordinator-club-list"), clubs,
        (club) => makeRecord(club.name, `${club.id} · ${club.memberCount} members · ${club.description}`));
    document.querySelector("#coordinator-dashboard").hidden = false;
    document.querySelector("#dashboard-role").textContent = "COORDINATOR DASHBOARD";
}

async function runAction(action) {
    try {
        await action();
        dashboardStatus.textContent = "";
    } catch (error) {
        showToast(error.message, true);
    }
}

function formObject(form) {
    return Object.fromEntries(new FormData(form).entries());
}

function addJsonForm(formId, endpoint, transform, successMessage, reload) {
    document.querySelector(formId).addEventListener("submit", (event) => {
        event.preventDefault();
        runAction(async () => {
            await requestJson(endpoint, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(transform())
            });
            showToast(successMessage);
            await reload();
        });
    });
}

function wireStudentActions() {
    document.querySelector("#register-form").addEventListener("submit", (event) => {
        event.preventDefault();
        runAction(async () => {
            const result = await requestJson("/api/student/registrations", {
                method: "POST", headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ eventId: document.querySelector("#register-event").value })
            });
            showToast(`Registration ${result.status.toLowerCase()}.`);
            await loadStudentDashboard();
        });
    });

    document.querySelector("#mark-notifications-read").addEventListener("click", () => runAction(async () => {
        await requestJson("/api/student/notifications/read", { method: "POST" });
        showToast("Notifications marked read.");
        await loadStudentDashboard();
    }));

    document.querySelector("#feedback-form").addEventListener("submit", (event) => {
        event.preventDefault();
        runAction(async () => {
            await requestJson("/api/student/feedback", {
                method: "POST", headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    eventId: document.querySelector("#feedback-event").value,
                    rating: Number(document.querySelector("#feedback-rating").value),
                    comment: document.querySelector("#feedback-comment").value
                })
            });
            event.target.reset();
            showToast("Feedback submitted.");
        });
    });
}

function wireCoordinatorActions() {
    addJsonForm("#event-form", "/api/coordinator/events", () => {
        const data = formObject(document.querySelector("#event-form"));
        data.capacity = Number(data.capacity);
        return data;
    }, "Event created.", loadCoordinatorDashboard);

    addJsonForm("#club-form", "/api/coordinator/clubs", () => {
        const data = formObject(document.querySelector("#club-form"));
        data.interests = data.interests.split(",").map((interest) => interest.trim()).filter(Boolean);
        return data;
    }, "Club created.", loadCoordinatorDashboard);

    document.querySelector("#membership-form").addEventListener("submit", (event) => {
        event.preventDefault();
        runAction(async () => {
            await requestJson("/api/coordinator/clubs/members", {
                method: "POST", headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    clubId: document.querySelector("#member-club").value,
                    studentId: document.querySelector("#member-student").value
                })
            });
            showToast("Club membership updated.");
            await loadCoordinatorDashboard();
        });
    });

    document.querySelector("#attendance-form").addEventListener("submit", (event) => {
        event.preventDefault();
        runAction(async () => {
            await requestJson("/api/coordinator/attendance", {
                method: "POST", headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    studentId: document.querySelector("#attendance-student").value,
                    eventId: document.querySelector("#attendance-event").value,
                    present: document.querySelector("#attendance-present").value === "true"
                })
            });
            showToast("Attendance saved.");
        });
    });

    document.querySelector("#certificate-form").addEventListener("submit", (event) => {
        event.preventDefault();
        runAction(async () => {
            const result = await requestJson("/api/coordinator/certificates", {
                method: "POST", headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    studentId: document.querySelector("#certificate-student").value,
                    eventId: document.querySelector("#certificate-event").value
                })
            });
            showToast(`Certificate ready: ${result.title}`);
        });
    });

    document.querySelector("#feedback-report-form").addEventListener("submit", (event) => {
        event.preventDefault();
        runAction(async () => {
            const eventId = document.querySelector("#report-event").value;
            const report = await requestJson(`/api/coordinator/feedback?eventId=${encodeURIComponent(eventId)}`);
            const container = document.querySelector("#feedback-report");
            container.replaceChildren(makeRecord("Average rating", `${report.averageRating.toFixed(2)} / 5`));
            if (report.entries.length) {
                container.append(...report.entries.map((entry) => makeRecord(`${entry.rating} / 5`, entry.comment)));
            } else {
                container.append(element("p", "empty-record", "No feedback has been submitted for this event."));
            }
        });
    });
}

async function startDashboard() {
    try {
        session = await requestJson("/api/auth/session");
        if (!session.authenticated) {
            window.location.assign("/login.html");
            return;
        }
        renderHeader();
        if (session.role === "student") {
            wireStudentActions();
            await loadStudentDashboard();
        } else if (session.role === "coordinator") {
            wireCoordinatorActions();
            await loadCoordinatorDashboard();
        } else {
            throw new Error("This account role does not have a dashboard.");
        }
        dashboardStatus.textContent = "";
    } catch (error) {
        dashboardStatus.textContent = error.message;
        dashboardStatus.classList.add("error");
    }
}

startDashboard();