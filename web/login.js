const form = document.querySelector("#login-form");
const emailInput = document.querySelector("#email");
const passwordInput = document.querySelector("#password");
const errorMessage = document.querySelector("#login-error");
const submitButton = document.querySelector("#login-submit");
const tabs = [...document.querySelectorAll("[data-role]")];

let activeRole = "student";

for (const tab of tabs) {
    tab.addEventListener("click", () => {
        activeRole = tab.dataset.role;
        for (const item of tabs) {
            const selected = item === tab;
            item.classList.toggle("active", selected);
            item.setAttribute("aria-selected", String(selected));
        }
        document.querySelector("#login-title").textContent = activeRole === "student"
            ? "Student sign in"
            : "Coordinator sign in";
        emailInput.autocomplete = "username";
    });
}

document.querySelector("#password-toggle").addEventListener("click", (event) => {
    const visible = passwordInput.type === "text";
    passwordInput.type = visible ? "password" : "text";
    event.currentTarget.textContent = visible ? "Show" : "Hide";
    event.currentTarget.setAttribute("aria-pressed", String(!visible));
});

form.addEventListener("submit", async (event) => {
    event.preventDefault();
    errorMessage.hidden = true;
    submitButton.disabled = true;
    submitButton.firstChild.textContent = "Signing in... ";

    try {
        const response = await fetch("/api/auth/login", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            cache: "no-store",
            body: JSON.stringify({
                email: emailInput.value.trim(),
                password: passwordInput.value,
                role: activeRole
            })
        });
        const result = await response.json();
        if (!response.ok) {
            throw new Error(result.error || "Sign in failed. Check your details and try again.");
        }
        window.location.assign("/dashboard.html");
    } catch (error) {
        errorMessage.textContent = error.message || "The sign-in service is unavailable. Try again shortly.";
        errorMessage.hidden = false;
        passwordInput.value = "";
        submitButton.disabled = false;
        submitButton.firstChild.textContent = "Sign in ";
    }
});