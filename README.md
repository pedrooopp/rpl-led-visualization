# RPL Network LED Visualizer

**Making an RPL/TSCH wireless sensor network visible, every control packet and every hop of a UDP message is mapped to an LED pattern on the node that handles it, and the whole network is deliberately slowed down so a human can follow along.**

Built on [Contiki-NG](https://github.com/contiki-ng/contiki-ng) (v4.9) for the TI **CC1350 LaunchPad**, with a ready-made **Cooja simulation** so it can be tried without hardware. Group project for the RSI course, 2023, Facultad de Ingeniería, Universidad de la República (Uruguay).

## The problem

In a low-power mesh network the interesting things (DODAG formation, DIO/DAO exchanges, multi-hop forwarding) happen in milliseconds and are only visible in packet captures or logs. That makes RPL hard to teach and hard to demo.

## The idea

1. **Hook the stack.** Small patches to Contiki-NG's RPL, IPv6 and shell code post a Contiki *event* whenever a node sends a DIS/DIO/DAO, forwards a packet, or originates/receives a UDP message.
2. **Turn events into LED patterns.** Two protothreads on each node (`red_leds_process`, `green_leds_process`) wait for those events and play a distinct blink pattern.
3. **Slow the network down.** The TSCH timeslot is stretched 10× and the RPL timers 3×, so a packet's path across the network plays out over seconds instead of milliseconds.
4. **Drive it from a shell.** A new `udp-send` shell command makes any node send a UDP message to any other node, so you can watch the message hop through the DODAG.

## LED patterns

| Event | Red LED | Green LED |
|---|---|---|
| DIO, multicast | solid for 1 s | off |
| DIO, unicast | solid for 1 s | blinking (0.2 s) for 1 s |
| DAO | blinking (0.33 s) for 1 s | off |
| DIS | blinking (0.2 s) for 1 s | off |
| UDP message sent | off | solid for 1 s |
| UDP message received | off | solid for 1 s |
| Intermediate hop (packet forwarded) | off | solid for 0.5 s |

The border router runs the same LED logic as the clients, so the root of the DODAG is visualized too.

## Repository layout

```
├── cliente/                         # Client node application (LED processes, UDP, shell)
│   └── variables_compartidas.h      # Events shared between the app and the patched Contiki-NG sources
├── border-router/                   # Border router (DODAG root) + the small web server it exposes
├── contiki-ng-patches/              # Modified Contiki-NG files, laid out like the Contiki-NG tree
│   ├── apply-patches.sh             # Copies them into a Contiki-NG checkout (with backups)
│   └── CHANGES-vs-v4.9.diff         # Exactly what changed relative to upstream v4.9
└── simulation/                      # Cooja simulation (1 border router + 6 clients)
```

Every change inside the Contiki-NG sources is tagged with a `@rsi2023 change` comment, so `grep -rn "@rsi2023" contiki-ng-patches` lists them all. Code comments are in Spanish.

### What the patches change

| File (in Contiki-NG) | Change |
|---|---|
| `os/net/routing/rpl-{lite,classic}/rpl-icmp6.c` | Post an event when a DIS, DIO or DAO is sent |
| `os/net/routing/rpl-{lite,classic}/rpl-timers.c` | Multiply RPL timers by `FACTOR` (3) |
| `os/net/ipv6/uip6.c` | Post a *hop* event when a packet is forwarded |
| `os/services/shell/shell-commands.c` | New `udp-send` command |
| `os/net/mac/tsch/tsch-timeslot-timing.c` | Timeslot length × `FACTOR` (10) |
| `os/net/mac/tsch/tsch.c`, `tsch-types.h`, `arch/.../cc13xx-50kbps-tsch.h` | Timing values widened to 32 bits so the longer timeslot fits |

## Getting started

**Requirements:** a Contiki-NG v4.9 checkout and its toolchain. For hardware: CC1350 LaunchPads (one border router, the rest clients). For the simulation: Cooja (ships with Contiki-NG).

1. **Clone this repository into the Contiki-NG tree.** The location matters: the patched sources include the shared header by that path, and the Makefiles expect Contiki-NG three levels up.

2. **Apply the Contiki-NG patches** (RPL-Lite by default; pass `classic` for RPL-Classic). Originals are saved as `*.orig` next to each patched file.

3. **Choose the RPL implementation** (optional). RPL-Lite is the default. For RPL-Classic, uncomment `MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC` in both `cliente/Makefile` and `border-router/Makefile`, then run `make distclean` in each.

### On hardware (CC1350)

```bash
cd rpl-led-visualization/cliente        && make TARGET=cc26x0-cc13x0 BOARD=launchpad/cc1350 cliente.upload
cd ../border-router                               && make TARGET=cc26x0-cc13x0 BOARD=launchpad/cc1350 border-router.upload
```

Then connect your computer to the border router (replace the port with yours):

```bash
make TARGET=zoul PORT=/dev/ttyACM0 connect-router
```

> The slowed-down mode only works on the CC1350, because it relies on the patched `cc13xx-50kbps-tsch.h` timing table.

### In Cooja (no hardware)

Open `simulation/rpl-network.csc` in Cooja. Node 1 is the border router, nodes 2–7 are clients. The LED visualizer skin shows each node's LEDs live. To bridge the simulated border router to your machine:

```bash
cd border-router && make TARGET=zoul connect-router-cooja
```

## Using it

Once the DODAG has formed you should see DIO and DAO patterns ripple through the nodes. Open a node's shell and send a message by passing the destination IPv6 address as eight space-separated 16-bit hex groups instead of colons:

```
> udp-send fd00 0 0 0 212 4b00 1234 5678
```

The sender lights its green LED, each intermediate node flashes its green LED as it forwards the packet, and the destination lights green when the message arrives.

## Notes

- Verified: the patched files are minimal edits of Contiki-NG v4.9 (see `CHANGES-vs-v4.9.diff`), `apply-patches.sh` applies cleanly to a v4.9 tree, and the application and patched `uip6.c` compile with the project's include layout. A full firmware build needs the CC13xx ARM toolchain or Cooja, which were not available when this repository was reorganized.
- `border-router/webserver/` and `border-router/sky/` come from Contiki-NG's RPL border router example and keep their original license headers; the files in `contiki-ng-patches/` are derived from Contiki-NG (BSD 3-clause).
- The network is sized for at most 8 nodes (`NETSTACK_MAX_ROUTE_ENTRIES` in `project-conf.h`); raise it for larger deployments.
