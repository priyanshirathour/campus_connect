const form = document.querySelector("#signup-form");
const tabs = [...document.querySelectorAll("[data-role]")];
const studentFields = document.querySelector("#student-fields");
const coordinatorFields = document.querySelector("#coordinator-fields");
const emailInput = document.querySelector("#email");
const passwordInput = document.querySelector("#password");
const confirmationInput = document.querySelector("#confirm-password");
const errorMessage = document.querySelector("#signup-error");
const submitButton = document.querySelector("#signup-submit");

let activeRole = "student";

for (const tab of tabs) {
    tab.addEventListener("click", () => {
        activeRole = tab.dataset.role;
        const isStudent = activeRole === "student";
        for (const item of tabs) {
            const selected = item === tab;
            item.classList.toggle("active", selected);
            item.setAttribute("aria-selected", String(selected));
        }
        // Only submit fields for the selected role; role authorization is still enforced by C++.
        studentFields.hidden = !isStudent;
        coordinatorFields.hidden = isStudent;
        document.querySelector("#student-id").required = isStudent;
        document.querySelector("#display-name").required = !isStudent;
        document.querySelector("#invite-code").required = !isStudent;
        document.querySelector("#signup-title").textContent = isStudent
            ? "Student account"
            : "Coordinator account";
        document.querySelector("#signup-note").textContent = isStudent
            ? "Your ID and email must match an existing student profile."
            : "Coordinator signup requires an invitation code from a campus administrator.";
    });
}

form.addEventListener("submit", async (event) => {
    event.preventDefault();
    errorMessage.hidden = true;

    if (passwordInput.value !== confirmationInput.value) {
        errorMessage.textContent = "Those passwords do not match.";
        errorMessage.hidden = false;
        confirmationInput.value = "";
        confirmationInput.focus();
        return;
    }

    submitButton.disabled = true;
    submitButton.firstChild.textContent = "Creating account... ";
    const account = {
        role: activeRole,
        email: emailInput.value.trim(),
        password: passwordInput.value
    };
    if (activeRole === "student") {
        account.studentId = document.querySelector("#student-id").value.trim();
    } else {
        account.displayName = document.querySelector("#display-name").value.trim();
        account.inviteCode = document.querySelector("#invite-code").value;
    }

    try {
        const response = await fetch("/api/auth/signup", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            cache: "no-store",
            body: JSON.stringify(account)
        });
        const result = await response.json();
        if (!response.ok) {
            throw new Error(result.error || "Could not create the account. Check your details and try again.");
        }
        window.location.assign("/dashboard.html");
    } catch (error) {
        errorMessage.textContent = error.message || "The sign-up service is unavailable. Try again shortly.";
        errorMessage.hidden = false;
        passwordInput.value = "";
        confirmationInput.value = "";
        submitButton.disabled = false;
        submitButton.firstChild.textContent = "Create account ";
    }
});