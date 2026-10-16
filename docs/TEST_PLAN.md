# Test plan

## Automated core tests

`tests/test_core.cpp` verifies FIFO ordering, duplicate prevention, removal without reordering, graph traversal and missing graph vertices. Run with:

```cmd
ctest --test-dir build -C Release --output-on-failure
```

## Manual integration tests

1. **Duplicate registration:** register the same student/event twice; second attempt must show a duplicate-registration error.
2. **Full event:** fill `E101` to capacity and register a third student; status must be `WAITLISTED`.
3. **FIFO promotion:** cancel the first confirmed registration; the earliest waitlisted student must become confirmed and receive a notification.
4. **Cancel waitlist entry:** cancel a waitlisted student; their ID should disappear from that event waitlist without changing the order of others.
5. **Invalid IDs:** try an unknown student/event/club; the application should report a helpful message, not crash.
6. **Attendance validation:** marking attendance for a student without active registration must fail.
7. **Certificate validation:** issuing before marking present must fail; issuing after marking present must create a certificate.
8. **Feedback rating:** values outside 1–5 must be rejected.
9. **MongoDB persistence:** exit normally, restart, and verify records remain.
10. **Text snapshots:** after normal exit, confirm all files in `data/` are updated.
