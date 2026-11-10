# Camera review regressions

Run from the repository root:

```sh
CC=cc python -m unittest discover -s tools -p 'test_*.py' -v
```

On Windows with Zig installed:

```powershell
$env:CC='zig cc'
python -m unittest discover -s tools -p 'test_*.py' -v
```

`test_camera_review.py` compiles the actual firmware function bodies in temporary
native C executables. Hardware, task scheduling, TLS and NVS I/O are stubbed in
these fixtures; no duplicate parser, worker, navigation or HTTP handler logic is
maintained. A missing native compiler skips this suite and must not be reported
as passing it.

The firmware regression cases cover:

- Successful image followed by an offline poll, untouched failure, partial draw,
  or startup with no connection.
- Saved AMS4 restoration across an early g-code delta, eventual topology,
  explicit empty lists, missing/invalid lists and another printer's report.
- Camera settings GET/POST with legacy identity, NVS precedence, sparse slots,
  empty/example identities and NVS read failure.
- First certificate-pin persistence failure and retry, plus rejection before
  persistence when the certificate identity is invalid or unseen.
- Screen idle timeout on USB and battery across inactive, paused, finished,
  failed, unknown, offline and running states; print completion and unplugging;
  activity on another configured printer; key wake and camera resume; plugging
  in while idle waking once. The actual MQTT parser, wake event sequence, both
  independent screen policy state machines and display callbacks are exercised
  together. Cases cover start/finish events on both power sources, baseline and
  duplicate reports, reconnect/stateless telemetry, another printer and resume,
  event/timeout priority, display-lock retries, deadline reset while already lit,
  coalesced events and sequence wrap. Display wake does not depend on sound
  notification classification.

Every screen/power case is compiled and run unchanged for Passport with PM
enabled and CYD with PM disabled. Both use the same policy source and expected
state transitions. Touch CLICK-before-RELEASE, hold-to-wake and boot suppression
are also checked in both builds; there are no CYD-specific sleep exemptions.

A separate integration case compiles the actual CYD BSP external-supply
function with the real MQTT parser, screen callbacks and shared policy. With
USB-host detection false, it verifies idle sleep, two hours of simulated
RUNNING staying lit, and FINISH lighting once then timing out after 30 seconds.
This catches treating an unavailable USB link detector as absence of supply.

Camera transport cases also compile actual fetch/close/read/write functions to
check authenticated snapshot reuse, reconnect after request/decode/setup failure,
unchanged reconnect behavior for stream models, rejection of changed identity
during reused-session I/O, and worker cleanup on stop/offline. Real TLS, JPEG
decode and display speed are measured on the board, not by these I/O stubs.

The six tests in `test_probe_camera_snapshot.py` exercise the host-side protocol
probe separately. These checks do not replace an ESP-IDF build or real-device
validation of TLS, power management and display behavior.
