*This project has been created as part of the 42 curriculum by rabi-aka.*

# ft_irc

## Description

A small IRC server written in C++98. You start it with a port and a password,
then you connect to it with a real IRC client (irssi, HexChat...) or with `nc`.

Only the mandatory part of the subject is done: no bonus, nothing extra.

What it can do:

- many clients at the same time, with **one** `poll()` and non-blocking sockets
  (no fork, no threads)
- log in with `PASS`, `NICK`, `USER`
- `JOIN` / `PART` channels, talk with `PRIVMSG` / `NOTICE`, leave with `QUIT`
- channel operators: `KICK`, `INVITE`, `TOPIC`, `MODE` with `i` `t` `k` `o` `l`

## Instructions

```
make
./ircserv 6667 mypassword
```

Stop it with ctrl+C (everything is closed and freed properly).

Test with nc:

```
nc -C 127.0.0.1 6667
PASS mypassword
NICK bob
USER bob 0 * :Bob
JOIN #42
PRIVMSG #42 :hello
```

Test with irssi:

```
irssi
/connect 127.0.0.1 6667 mypassword
/join #42
```

## How the code is organised

| File           | What is inside                                                     |
|----------------|--------------------------------------------------------------------|
| `main.cpp`     | checks the arguments, sets the signals, starts the server          |
| `Server.cpp`   | the network part: socket, the `poll()` loop, accept / read / write |
| `Commands.cpp` | one function per IRC command (`cmdJoin`, `cmdKick`, ...)           |
| `Helpers.cpp`  | small tools: send a reply, broadcast, find a nick, disconnect      |
| `Client.*`     | the data of one user (nick, buffers, ...)                          |
| `Channel.*`    | the data of one channel (members, operators, modes)                |
| `Utils.*`      | text helpers (`lower`, `split`, `splitLine`)                       |

## How it works (the 5 ideas to remember)

1. **One loop, one `poll()`.** On every turn we build the list of sockets to
   watch, `poll()` sleeps until something happens, and we only touch the
   sockets that `poll()` marked as ready. We never call `recv`/`send` without it.
2. **Inbox.** Data can arrive in pieces (`JO` then `IN #a\r\n`). Every piece is
   added to the client's `inbox`, and we only handle complete lines (ending
   with `\n` or `\r\n`). This is the `com^Dman^Dd` test of the subject.
3. **Outbox.** We never send directly. Messages are added to the client's
   `outbox`, and we ask `poll()` for `POLLOUT` only when the outbox is not
   empty. If `send()` takes only a part, the rest stays for the next turn.
   A slow or frozen client (ctrl+Z) can never block the others.
4. **Dead flag.** A client is never deleted in the middle of the loop. It is
   marked `dead`, and deleted at the end of the turn. No dangling references.
5. **Users are fds.** A channel just stores the socket fd of its members,
   operators and invited users in `std::set<int>`.

Rules of the subject that are easy to check in the code:

- `fcntl` is only used as `fcntl(fd, F_SETFL, O_NONBLOCK)`
- `errno` is never used
- there is a single `poll()` call, in `Server::run()`

## Resources

- [RFC 1459](https://www.rfc-editor.org/rfc/rfc1459) and
  [RFC 2812](https://www.rfc-editor.org/rfc/rfc2812) - the IRC protocol
- [Modern IRC client protocol](https://modern.ircdocs.horse/) - easier to read
  than the RFCs, with every numeric reply
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) -
  sockets and `poll()`
- man pages: `socket`, `bind`, `listen`, `accept`, `poll`, `recv`, `send`, `fcntl`

### AI usage

AI (Claude) was used to audit a first version of the project against the
subject, to design this simpler architecture and write it with me, and to
build the test scenarios (partial data, frozen client, flood, valgrind).
I read, ran and tested every part, and I can explain all of it.
