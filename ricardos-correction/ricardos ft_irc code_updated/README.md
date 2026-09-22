*This project has been created as part of the 42 curriculum by rabi-aka, mohchams.*

# ft_irc

## Description

An IRC server written in C++98. You start it with a port and a password, then
you connect to it with a real IRC client (irssi is our reference client) or
with `nc`, and you can chat with other people connected to the same server.

What the server does:

- handles many clients at the same time with a single `poll()` and
  non-blocking sockets (no fork, no threads)
- registration with `PASS`, `NICK`, `USER`
- `JOIN`, `PART`, `PRIVMSG` (to a channel or to a user), `QUIT`
- channel operators with `KICK`, `INVITE`, `TOPIC` and `MODE` (`i t k o l`)

## Instructions

```
make
./ircserv 6667 mypassword
```

Stop the server with ctrl+C (every socket is closed, nothing leaks).

With nc (`-C` sends `\r\n` like a real client, but plain `\n` works too):

```
nc -C 127.0.0.1 6667
PASS mypassword
NICK bob
USER bob 0 * :Bob
JOIN #42
PRIVMSG #42 :hello
```

With irssi:

```
irssi
/connect 127.0.0.1 6667 mypassword
/join #42
```

## Files

| File                 | Content                                                        |
|----------------------|----------------------------------------------------------------|
| `srcs/main.cpp`      | checks the arguments, sets the signals, starts the server      |
| `srcs/Server.cpp`    | socket creation, the `poll()` loop, accept / read / write      |
| `srcs/Commands.cpp`  | PASS NICK USER PING QUIT JOIN PART PRIVMSG                      |
| `srcs/Operators.cpp` | TOPIC KICK INVITE MODE                                          |
| `srcs/Utils.cpp`     | reply / broadcast / find a nick / disconnect / string helpers  |
| `Client`             | one connected user: nick, user, buffers, flags                  |
| `Channel`            | one channel: members, operators, invited users, modes           |

## How it works

1. `Server::run()` builds the list of sockets to watch (the listening socket
   plus every client), calls `poll()`, and only touches the sockets that
   `poll()` marked as ready. `recv()` and `send()` are never called without it.
2. Data can arrive in pieces (`JO` then `IN #a\r\n`). Every piece goes to the
   client's `inbuf`, and a command is only handled once a full line is there.
3. Nothing is sent directly. Replies go to the client's `outbuf`, and we ask
   `poll()` for `POLLOUT` only while there is something to send. A slow client
   never blocks the others.
4. A client is never deleted in the middle of the loop: it is marked `dead`
   and closed at the end of the loop, so no reference becomes invalid.
5. Channels store the socket fd of their members, operators and invited users.

Subject rules you can check quickly: `fcntl` is only called as
`fcntl(fd, F_SETFL, O_NONBLOCK)`, `errno` is never used, and there is a single
`poll()` in `Server::run()`.

## Resources

- [RFC 1459](https://www.rfc-editor.org/rfc/rfc1459) and
  [RFC 2812](https://www.rfc-editor.org/rfc/rfc2812), the IRC protocol
- [Modern IRC client protocol](https://modern.ircdocs.horse/), easier to read
  than the RFCs, with all the numeric replies
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) for
  sockets and `poll()`
- man pages: `socket`, `bind`, `listen`, `accept`, `poll`, `recv`, `send`, `fcntl`

### AI usage

AI was used to understand more the subject, to help make the
structure simpler, and to write test scripts (partial data, frozen client,
many clients, valgrind). We read, ran and tested every part of the code.
