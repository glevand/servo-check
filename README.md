# servo-check - Servo motor sensor check.

Analyse servo motor sensor data.

## Usage

```
servo-check - Servo motor sensor check.
Usage: Usage: servo-check [flags] data-file
Option flags:
  -h --help          - Show this help and exit.
  -v --verbose       - Verbose execution.
  -V --version       - Display the program version number.
  --e-len            - Encoder filter length (e-len > 1).  Default: '400'.
  --p-len            - Potentiometer filter length (p-len > 1).  Default: '400'.
  --error-limit      - Error detection limit (error-limit > 1).  Default: '400'.
  --phase-lag        - Phase adjustment in seconds.  Default: '0.200500'.
  -f --show-filtered - Print filtered signal data to stdout.
  -s --show-stats    - Print signal stats to stdout at program exit.
Send bug reports to geoff@infradead.org.
```

## Typical Output

```
System OK
------------------------------------------------------------------------
params:      e-len = 400, p-len = 400
enc:         min = {3.693000, -10849}, max = {2.691000, 17839}
pot:         min = {3.696000, -10655}, max = {2.688000, 18143}
signal diff: min = {0.003000, 194}, max = {-0.003000, 304} => -110
sample diff: enc = {3.112000, 9409}, pot = {3.112000, 9752} => 343
------------------------------------------------------------------------
```

```
Sensor Error: 2.881500
------------------------------------------------------------------------
params:      e-len = 400, p-len = 400
enc:         min = {1.018000, 0}, max = {2.691000, 17839}
pot:         min = {3.696000, -10655}, max = {2.688000, 18143}
signal diff: min = {2.678000, -10655}, max = {-0.003000, 304} => -10959
sample diff: enc = {3.690000, 7363}, pot = {3.690000, -10655} => 18018
------------------------------------------------------------------------
```

## Licence

All files in the [servo-check project](./servo-check), unless otherwise noted, are covered by an [MIT Plus License](./servo-check/mit-plus-license.txt).  The text of the license describes what usage is allowed.
