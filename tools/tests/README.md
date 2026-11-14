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

An existing Visual Studio developer shell also works with `$env:CC='cl'`.
The runner enables MSVC's C11 atomics support and keeps generated objects in
the temporary test directory. The Python protocol probe additionally needs
Pillow and cryptography in the selected environment.

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
- Ordinary camera address/code/mode/name edits and disable/enable retain the
  paired leaf. Only the exact re-pair flag clears it; suffix keys, longer
  values and encoded NULs cannot opt in. The real certificate verifier rejects
  a changed leaf after ordinary edits and accepts the existing paired leaf.
  Missing namespaces remain configurable; failed or incomplete NVS reads never
  expose a partial unpinned configuration or allow an ordinary save to erase it.
- A wake gesture completed while the display lock is busy does not consume
  the next independent press. Legacy stripe decoding retains an untouched
  previous frame while still invalidating one changed by a clear or stripe.
  Untouched failures also retain the displayed geometry, so the next aspect-ratio
  change still clears its letterboxes.
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

The six tests in `test_probe_camera_snapshot.py` exercise the host-side protocol
probe separately. These checks do not replace an ESP-IDF build or real-device
validation of TLS, power management and display behavior.

Transport regressions compile actual snapshot fetch/read/write/close functions:
authenticated session reuse, failure reconnect, stream protocol compatibility and
identity rejection during reused TLS I/O. Rendering regressions compile the real
JPEG output, decode and presentation functions with stub JPEG/LCD I/O. They check
zero LCD writes before validation, RGB565 colors and dynamic letterboxes for
landscape/portrait/small sources, odd-width/cross-block output guards, memory and
decode/input/checksum failure, cancellation, display-lock failure and partial LCD
failure. The real FPS helper is checked against successful presentation intervals,
delayed updates, clock reset and the exact five-second Loading boundary.
Navigation tests compile actual page switching with an allocator budget that
allows one visible detail page while the cancelled camera still owns its buffer.
They cycle all pages and four AMS slots, verify stop-before-delete, retain widgets
on repeated refresh and reject the retired control page slot. MQTT
client-ID cases distinguish two device MACs with the same nonce, another boot,
and the fallback when MAC reading fails.

Power-off cases compile the real deferred worker and request/policy functions.
Virtual time checks the exact 600-second deadline and countdown resets. Stubs
exercise new activity before/after commit, display-lock/input/allocation failures,
all terminal hardware error paths, MQTT guards across multiple/offline unfinished
jobs, and clearing old wake sources before GPIO-only deep sleep. Physical deep
sleep and button restart are tested separately; accelerated diagnostics do not
replace a real ten-minute battery/current measurement.
