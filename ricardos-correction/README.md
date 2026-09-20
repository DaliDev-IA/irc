*This project has been created as part of the 42 curriculum by mohchams.*

# ft_irc

## Description

`ft_irc` is an IRC server written in C++98. It implements the IRC features
required by the ft_irc project at 42:

- TCP sockets (IPv4)
- fully non-blocking I/O
- a single `epoll` event loop handling the listening socket and every
  connected client (no `fork`, no threads)
- support for multiple simultaneous clients
- IRC client registration (`PASS` / `NICK` / `USER`)
- channels, channel operators and channel modes
- proper handling of TCP fragmentation: partial commands are buffered per
  client and only processed once a complete line has been received (`\r\n`,
  or a bare `\n` so that `nc` without `-C` also works), and several complete
  commands received in a single packet are all processed in order
- clean shutdown on `SIGINT` / `SIGTERM` (every socket is closed and all
  memory is released)

## Instructions

Compilation:

```
make
```

Other standard targets are available: `clean`, `fclean`, `re`.

Execution:

```
./ircserv <port> <password>
```

Example:

```
./ircserv 6667 secret
```

The server can be tested with any standard IRC client (the password set on
the command line must be sent with the `PASS` command), or with a raw tool
such as `netcat`:

```
nc -C 127.0.0.1 6667
```

## Implemented IRC commands

- `PASS`
- `NICK`
- `USER`
- `PING`
- `CAP`
- `JOIN`
- `PART`
- `PRIVMSG`
- `NOTICE`
- `TOPIC`
- `INVITE`
- `KICK`
- `MODE`
- `WHO`
- `QUIT`

Any other command is answered with `421 Unknown command`.

`CAP`, `WHO`, `MODE <nick>` and the `MODE #channel b` ban-list query are
answered so that IRC clients that send them automatically on connect or on
join (e.g. Irssi) do not display errors; they are not features required by
the ft_irc subject itself.

## Channel modes

- `i` — invite-only channel
- `t` — restrict the `TOPIC` command to channel operators
- `k` — set/remove a channel key (password)
- `o` — give/take channel operator privilege
- `l` — set/remove the channel's user limit

## Architecture

The project is organized around four main components:

- `Server` — owns the listening socket, the `epoll` instance, the list of
  connected clients and channels, and dispatches parsed commands to their
  handlers.
- `Client` — per-connection state: registration status, nickname/username,
  and the persistent receive/send buffers.
- `Channel` — a channel's members, operators, invited users and modes.
- `Parser` — turns one raw IRC line into a structured command (name,
  parameters, trailing parameter).

Data flow for a single client event:

```
epoll_wait
  -> recv() into the client's persistent receive buffer
  -> extract every complete line (CRLF or LF) from that buffer
  -> parse each command
  -> run the matching command handler
  -> queue the resulting reply/broadcast into the client's send buffer
  -> EPOLLOUT is armed, and send() flushes the buffer on the next event
```

## Resources

- RFC 1459 — Internet Relay Chat Protocol
- RFC 2812 — Internet Relay Chat: Client Protocol
- Linux man pages: `socket(2)`, `bind(2)`, `listen(2)`, `accept(2)`,
  `fcntl(2)`, `epoll(7)`, `recv(2)`, `send(2)`

### AI usage

AI assistance was used during this project as a support tool, not as a
replacement for the students' own work. Concretely, it was used to:

- review the networking architecture;
- explain epoll and non-blocking socket behavior;
- review IRC protocol edge cases;
- design and review test scenarios;
- debug error-handling paths;
- review memory and file-descriptor safety;
- help audit the implementation against the project requirements.

The implementation was reviewed and tested by the students. Decisions and
final validation remained under student control, and AI-generated
suggestions were checked against the code, runtime tests and the project
requirements before being relied on.
