# User guide

## Website

For coordinator self-signup, set a unique `CAMPUS_CONNECT_COORDINATOR_INVITE` value in PowerShell before starting `build\Release\campus_connect.exe --api`; share it with coordinators securely. Without it, coordinator signup is closed. Then run `node web/server.js` from the project root in a second terminal. Open `http://127.0.0.1:8000/` and choose **Create an account**. Students link an existing profile with its ID and matching campus email; no email verification is sent. You can also provision accounts locally with `--provision-student` or `--provision-coordinator`; passwords are typed into a hidden prompt.

The website supports live event search, category filtering, event details, saved events, club browsing, and role-aware sign-in. The student dashboard covers registration/cancellation, waitlist position, notifications, recommendations, certificates, and feedback. The coordinator dashboard covers events, clubs, membership, attendance, certificates, feedback reports, and summary downloads. Sessions expire after eight hours and are cleared when the C++ API restarts. The same core operations remain available in the console.

## Student view

1. Choose Student view at the main menu.
2. Use demo IDs `S1001`, `S1002`, or `S1003`.
3. Browse/search events; register or cancel; check registration state, waitlist, notifications, recommendations and certificates; submit feedback.

## Coordinator view

Coordinator features are available in the Coordinator menu (the report assumes coordinator identity is verified outside this app): create an event/club, add a student to a club, mark attendance, issue certificates, review feedback, print a summary, or export the summary.

## Seeded records

| ID | Record |
|---|---|
| S1001 | Aarav Sharma |
| S1002 | Priya Singh |
| S1003 | Rohan Verma |
| C101 | Coding Club |
| C102 | Robotics Club |
| C103 | Design Club |
| E101 | DSA Workshop, capacity 2 |
| E102 | Campus Hackathon |
| E103 | Robotics Showcase |
| E104 | Design Jam |

## Complete demo walkthrough

1. Register `S1001` for `E101`.
2. Register `S1002` for `E101`.
3. Register `S1003` for `E101`. Confirm the returned `WAITLISTED` status.
4. Cancel `E101` as `S1001`. Confirm `S1003` appears as `CONFIRMED` and has a new notification.
5. Coordinator marks `S1003` PRESENT for `E101`.
6. Coordinator issues a certificate for `S1003`/`E101`.
7. Student `S1003` views the certificate and submits feedback.
