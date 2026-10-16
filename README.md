# Campus Connect

Campus Connect is a Windows C++17 club and event management application with a console interface and a companion website. MongoDB stores live application data. The website reads and authenticates through a loopback-only HTTP API implemented in C++; business rules remain in C++ services.

## What it includes

- Student and coordinator account signup/sign-in, with Argon2id password hashes and HttpOnly session cookies.
- Event and club discovery, search, category filters, event details, and locally saved events in the website.
- Student dashboard for registration/cancellation, waitlist position, notifications, recommendations, certificates, and feedback.
- Coordinator dashboard for events, clubs, membership, attendance, certificates, feedback reports, and summary downloads.
- Console workflows remain available for core student and coordinator operations.
- MongoDB persistence plus plain-text snapshots written on normal console exit.
- A FIFO wait queue, recommendation graph with BFS, service-level validation, and core data-structure tests.

## First-time setup

These instructions assume Windows 10/11, PowerShell, and a Visual Studio Developer PowerShell/Command Prompt.

### 1. Install prerequisites

- Visual Studio Build Tools with the MSVC C++ workload and Windows SDK.
- CMake 3.20 or newer and Git.
- Node.js 20 or newer.
- MongoDB Community Server. MongoDB Compass is optional, for browsing the database with a GUI.
- vcpkg. The manifest in `vcpkg.json` installs the MongoDB C++ driver, cpp-httplib, nlohmann-json, and libsodium during CMake configuration.

If vcpkg is not installed, install it once (or use your existing vcpkg location):

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
& C:\dev\vcpkg\bootstrap-vcpkg.bat
```

Open the repository folder in VS Code. In the commands below, replace `C:\dev\vcpkg` if your vcpkg installation is elsewhere.

### 2. Start MongoDB

Open the Windows **Services** app, find **MongoDB**, and choose **Start** if it is stopped. The app connects to the local server at `127.0.0.1:27017` and database `campus_connect`.

### 3. Configure, build, and test

Open **Developer PowerShell for VS** in the repository folder. Set the vcpkg path, then configure and build:

```powershell
$env:VCPKG_ROOT = "C:\dev\vcpkg"
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

CMake uses the `vcpkg.json` manifest to install project dependencies. Build and tests do not require MongoDB to be running; launching the app does. If linking fails because `campus_connect.exe` is in use, stop the running C++ API task and rebuild.

## First run and demo data

Run the console app once:

```powershell
.\build\Release\campus_connect.exe
```

On a new database, the app seeds demo students, clubs, and events. Existing records are not cleared. Exit with menu option `0` to write the text snapshots under `data/`.

Demo student profiles:

| Student ID | Email | Name |
|---|---|---|
| `S1001` | `aarav@campus.local` | Aarav Sharma |
| `S1002` | `priya@campus.local` | Priya Singh |
| `S1003` | `rohan@campus.local` | Rohan Verma |

Demo clubs are `C101` Coding Club, `C102` Robotics Club, and `C103` Design Club. Demo events are `E101` DSA Workshop (capacity 2), `E102` Campus Hackathon, `E103` Robotics Showcase, and `E104` Design Jam.

## Start the website

The website needs two processes. Open two PowerShell terminals in the repository folder.

**Terminal 1: C++ API**

```powershell
.\build\Release\campus_connect.exe --api
```

**Terminal 2: website**

```powershell
node web/server.js
```

Open **http://127.0.0.1:8000/**. Use **Create account** to sign up or **Sign in** for an existing account. After sign-in, choose **Dashboard** to open student or coordinator tools. Each dashboard action calls a role-protected C++ service endpoint; the console app remains available too.

To stop either process, press `Ctrl+C` in its terminal.

## Accounts and login

### Student signup

Choose **Create account**, select **Student**, then enter a demo student ID, its matching email from the table above, and a password of at least 12 characters. Signup links the account to an existing student profile; it does not send an email verification message.

An administrator can instead provision a student account locally:

```powershell
.\build\Release\campus_connect.exe --provision-student S1001
```

### Coordinator signup

Coordinator self-signup requires an invitation code configured in the API process environment. Set a unique code in Terminal 1 before starting the API, and share it with coordinators through a secure channel:

```powershell
$env:CAMPUS_CONNECT_COORDINATOR_INVITE = "<your private invitation code>"
.\build\Release\campus_connect.exe --api
```

Without this setting, coordinator signup is closed. Alternatively, an administrator can provision a coordinator account locally:

```powershell
.\build\Release\campus_connect.exe --provision-coordinator coordinator@campus.local "Club Coordinator"
```

Both provisioning commands prompt for a password twice without echoing it. No default passwords or invitation codes are stored in the repository.

### Session behavior

Passwords are stored as libsodium Argon2id hashes, never plaintext. The browser receives an HttpOnly, SameSite cookie; the session lives in C++ process memory, expires after eight hours, and is invalidated when the API restarts. The API and website bind to loopback and are intended for local development, not direct public deployment.

## How the pieces fit

1. The browser loads HTML, CSS, and JavaScript from `web/` through the Node server on port `8000`.
2. `web/server.js` proxies only allowlisted routes to the C++ API on port `8081` and forwards session cookies.
3. `src/web/WebApiServer.cpp` maps HTTP routes to C++ services and serializes their results as JSON.
4. Services own validation and business rules. `src/database/Repository.cpp` owns MongoDB/BSON persistence.

The C++ API endpoints include:

| Method | Route | Purpose |
|---|---|---|
| `GET` | `/api/health` | API health check |
| `GET` | `/api/events` | List events; `?q=text` uses C++ event search |
| `GET` | `/api/clubs` | List clubs |
| `POST` | `/api/auth/signup` | Create a student or invite-gated coordinator account |
| `POST` | `/api/auth/login` | Authenticate an account and issue a session cookie |
| `GET` | `/api/auth/session` | Read the current session identity |
| `POST` | `/api/auth/logout` | Revoke the session and clear its cookie |
| `GET` | `/api/student/dashboard` | Student's own registrations, notifications, recommendations, and certificates |
| `POST` | `/api/student/registrations` | Register the signed-in student for an event |
| `POST` | `/api/student/registrations/cancel` | Cancel the signed-in student's registration |
| `POST` | `/api/student/feedback` | Submit feedback as the signed-in student |
| `GET` | `/api/coordinator/dashboard` | Coordinator's event, club, student, and summary data |
| `POST` | `/api/coordinator/events` | Create an event |
| `POST` | `/api/coordinator/clubs` | Create a club |
| `POST` | `/api/coordinator/attendance` | Mark a registered student's attendance |
| `POST` | `/api/coordinator/certificates` | Issue a certificate after attendance validation |
| `GET` | `/api/coordinator/report` | Download a system summary |

## Check the database

MongoDB is the live source of truth. For a graphical view, connect MongoDB Compass to `mongodb://127.0.0.1:27017` and open the `campus_connect` database. Collections include `students`, `clubs`, `events`, registrations and account credentials. Do not share the `passwordHash` field from `accounts`.

The files in `data/` are exit-time snapshots for the project report; they are not used by the live website.

See [DATABASE_README.md](DATABASE_README.md) for MongoDB details, [docs/USER_GUIDE.md](docs/USER_GUIDE.md) for walkthroughs, and [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the layer and data-structure map.
