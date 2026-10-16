const http = require("node:http");
const fs = require("node:fs");
const path = require("node:path");

const port = Number(process.env.PORT) || 8000;
const webRoot = __dirname;
const routes = new Map([
    ["/", path.join(webRoot, "index.html")],
    ["/index.html", path.join(webRoot, "index.html")],
    ["/styles.css", path.join(webRoot, "styles.css")],
    ["/app.js", path.join(webRoot, "app.js")],
    ["/login.html", path.join(webRoot, "login.html")],
    ["/login.css", path.join(webRoot, "login.css")],
    ["/login.js", path.join(webRoot, "login.js")],
    ["/signup.html", path.join(webRoot, "signup.html")],
    ["/signup.css", path.join(webRoot, "signup.css")],
    ["/signup.js", path.join(webRoot, "signup.js")],
    ["/dashboard.html", path.join(webRoot, "dashboard.html")],
    ["/dashboard.css", path.join(webRoot, "dashboard.css")],
    ["/dashboard.js", path.join(webRoot, "dashboard.js")]
]);
// Proxy only declared paths and methods; browsers cannot use this server as a general C++ HTTP tunnel.
const apiRoutes = new Map([
    ["/api/health", new Set(["GET"])],
    ["/api/events", new Set(["GET"])],
    ["/api/clubs", new Set(["GET"])],
    ["/api/auth/login", new Set(["POST"])],
    ["/api/auth/signup", new Set(["POST"])],
    ["/api/auth/session", new Set(["GET"])],
    ["/api/auth/logout", new Set(["POST"])],
    ["/api/student/dashboard", new Set(["GET"])],
    ["/api/student/registrations", new Set(["POST"])],
    ["/api/student/registrations/cancel", new Set(["POST"])],
    ["/api/student/notifications/read", new Set(["POST"])],
    ["/api/student/feedback", new Set(["POST"])],
    ["/api/coordinator/dashboard", new Set(["GET"])],
    ["/api/coordinator/events", new Set(["POST"])],
    ["/api/coordinator/clubs", new Set(["POST"])],
    ["/api/coordinator/clubs/members", new Set(["POST"])],
    ["/api/coordinator/attendance", new Set(["POST"])],
    ["/api/coordinator/certificates", new Set(["POST"])],
    ["/api/coordinator/feedback", new Set(["GET"])],
    ["/api/coordinator/report", new Set(["GET"])]
]);
const contentTypes = new Map([
    [".html", "text/html; charset=utf-8"],
    [".css", "text/css; charset=utf-8"],
    [".js", "text/javascript; charset=utf-8"],
    [".txt", "text/plain; charset=utf-8"]
]);

const server = http.createServer((request, response) => {
    if (request.method !== "GET" && request.method !== "HEAD" && request.method !== "POST") {
        response.writeHead(405, { Allow: "GET, HEAD, POST" }).end();
        return;
    }

    let url;
    let pathname;
    try {
        url = new URL(request.url, "http://localhost");
        pathname = decodeURIComponent(url.pathname);
    } catch {
        response.writeHead(400).end("Invalid URL");
        return;
    }

    if (apiRoutes.has(pathname)) {
        const allowedMethods = apiRoutes.get(pathname);
        const method = request.method === "HEAD" ? "GET" : request.method;
        if (!allowedMethods.has(method)) {
            response.writeHead(405, { Allow: [...allowedMethods].join(", ") }).end();
            return;
        }

        const headers = { Accept: "application/json" };
        if (request.headers["content-type"]) headers["Content-Type"] = request.headers["content-type"];
        // Forward the opaque cookie in both directions; Node never reads or stores the session token.
        if (request.headers.cookie) headers.Cookie = request.headers.cookie;
        const upstream = http.request({
            hostname: "127.0.0.1",
            port: 8081,
            path: `${pathname}${url.search}`,
            method,
            headers
        }, (apiResponse) => {
            const responseHeaders = {
                "Content-Type": apiResponse.headers["content-type"] || "application/json; charset=utf-8",
                "Cache-Control": "no-store"
            };
            if (apiResponse.headers["set-cookie"]) {
                responseHeaders["Set-Cookie"] = apiResponse.headers["set-cookie"];
            }
            response.writeHead(apiResponse.statusCode || 502, responseHeaders);
            apiResponse.pipe(response);
        });
        upstream.on("error", () => {
            response.writeHead(503, { "Content-Type": "application/json; charset=utf-8" });
            response.end(JSON.stringify({ error: "Campus Connect C++ API is unavailable." }));
        });
        if (method === "POST") request.pipe(upstream);
        else upstream.end();
        return;
    }

    if (pathname.startsWith("/api/")) {
        response.writeHead(404).end("Unknown API route");
        return;
    }

    const filePath = routes.get(pathname);
    if (!filePath) {
        response.writeHead(404).end("Not found");
        return;
    }

    fs.readFile(filePath, (error, content) => {
        if (error) {
            response.writeHead(404).end("Snapshot or website file not found");
            return;
        }

        response.writeHead(200, {
            "Content-Type": contentTypes.get(path.extname(filePath)) || "application/octet-stream",
            "Cache-Control": "no-store"
        });
        response.end(request.method === "HEAD" ? undefined : content);
    });
});

server.listen(port, "127.0.0.1", () => {
    console.log(`Campus Connect website: http://127.0.0.1:${port}/`);
});