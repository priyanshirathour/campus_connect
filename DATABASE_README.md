# MongoDB database setup (separate guide)

## 1. Verify the local MongoDB service

Open PowerShell:

```powershell
mongod --version
Get-Service MongoDB
mongosh --version
```

The server must be `Running`. Connect and verify the shell:

```powershell
mongosh "mongodb://127.0.0.1:27017/"
```

Inside `mongosh`, run `show dbs`, then `exit`.

## 2. C++ driver

The setup used by this project is vcpkg under `C:\dev\vcpkg`. Check the driver:

```cmd
C:\dev\vcpkg\vcpkg.exe list | findstr mongo
```

If needed, install it from Developer Command Prompt:

```cmd
cd C:\dev\vcpkg
vcpkg install mongo-cxx-driver:x64-windows
```

## 3. Build with the vcpkg toolchain

From the project root:

```cmd
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

## 4. Database and collections

The application connects to `mongodb://127.0.0.1:27017/`, database `campus_connect`. MongoDB creates the database and collections as data is first written. No manual database creation is required.

After the application runs, inspect records:

```javascript
use campus_connect
show collections
db.students.find()
db.clubs.find()
db.events.find()
db.registrations.find()
db.attendance.find()
db.certificates.find()
db.feedback.find()
db.notifications.find()
```

Expected collections: `students`, `clubs`, `events`, `registrations`, `attendance`, `certificates`, `feedback`, `notifications`.
Provisioned login credentials are stored separately in the `accounts` collection; passwords are never stored in plaintext.

## 5. Seed data behavior

On first start, if each base collection is empty, the app inserts demo students, clubs, and events. Existing collection data is retained; the demo seed does not clear an existing database.

## 6. Security

The program is configured for a local development database without authentication, matching the PC setup. Do not expose an unauthenticated MongoDB instance to the internet. For deployments, enable authentication, use least-privilege users, TLS, and backups.
