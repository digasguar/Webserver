*This project has been created as part of the 42 curriculum by asgalean, dgasco-g, mcuello*

# Webserv

A fully custom HTTP/1.1 server written from scratch in C++98, built around a single-threaded, non-blocking `epoll` event loop.

## Description

`webserv` is a 42 school project whose goal is to understand how an HTTP server actually works by building one, without relying on any web framework or HTTP library. The server:

- Parses a custom, nginx-inspired configuration file to define one or more virtual servers, each with its own port, locations, and rules.
- Accepts and serves real HTTP/1.1 requests over TCP, handling many client connections concurrently through a single `epoll_wait` loop (no threads, no `fork()` for client handling).
- Implements the core of the HTTP/1.1 protocol needed to serve static websites, handle file uploads/deletions, run CGI scripts, and correctly manage persistent (`keep-alive`) connections.

It is meant to behave closely enough to a real server (like nginx) that a normal browser, `curl`, or a stress-testing tool such as `siege` can talk to it without noticing the difference.

## Features

### Core

- **Non-blocking I/O multiplexed with `epoll`**: one event loop services every listening socket and every client socket; no request blocks another.
- **Multiple virtual servers**: any number of `server {}` blocks, each on its own `host:port`, all sharing the same `epoll` instance.
- **HTTP methods**: `GET`, `POST`, `DELETE`.
- **Static file serving**: correct `Content-Type` resolution by extension (HTML, CSS, JS, images, audio, video...), directory `index` files, and `autoindex` (directory listing) when no index exists.
- **File upload and deletion**: `POST` writes the request body to disk (optionally under a dedicated `upload_store`), `DELETE` removes files — both with path-traversal protection (`realpath`-based boundary checks against the location's `root`).
- **Chunked transfer encoding**, both reading it from requests and writing it in responses.
- **Keep-alive connections** with an inactivity timeout, so idle clients are cleaned up without needing to close every connection after one request.
- **Custom error pages**: `error_page <code> <path>` per server, for every status code the server can emit (400, 403, 404, 405, 411, 413, 414, 431, 500, 501, 502, 504, 505); falls back to a plain-text response if no page is configured or the configured one can't be read — it never guesses a path on its own.
- **Redirections**: a `return <code> <path>;` directive per location, restricted to real redirect status codes (300, 301, 302, 303, 307, 308).
- **Configurable body size limit** (`client_max_body_size`), accepting plain bytes or human-friendly suffixes (`k`/`kb`, `m`/`mb`, `g`/`gb`, case-insensitive).
- **CGI execution** (`cgi_extension <ext> <interpreter>`): runs scripts (Python, Bash, etc.) through `execve`, feeding them the standard CGI environment variables and the request body over a pipe, non-blocking and without stalling the rest of the server if a script hangs or writes a lot of output.
- **Robust request parsing**, rejecting malformed requests with the right status code (`400`, `411`, `413`, `414`, `431`, `501`, `505`) instead of guessing or crashing.

### Bonus

- **Cookies and session-based login**: a minimal `/login` flow that issues a session cookie and gates the rest of the site behind a valid session.

## Instructions

### Compilation

```sh
make          # builds the mandatory part -> ./webserv
make bonus    # builds the bonus part (cookies/sessions) -> ./webserv_bonus
make clean    # removes object files
make fclean   # removes object files and both binaries
make re       # fclean + all
make rebonus  # fclean + bonus
```

Requires a C++98-capable compiler (`g++`) and Linux (the server uses `epoll`, so it won't build on macOS/BSD).

### Running

```sh
./webserv [path/to/config.conf]
```

If no path is given, it defaults to `config/default.conf`. A minimal config looks like this:

```nginx
server {
    listen 8080;
    client_max_body_size 10m;
    error_page 404 ./html/err/404.html;

    location / {
        root ./html;
        index index.html;
        methods GET;
        autoindex off;
    }

    location /uploads {
        root ./html;
        methods GET POST DELETE;
        upload_store ./html/uploads;
        autoindex on;
    }
}
```

Several ready-to-use configs are provided under `config/` (including multi-server and CGI examples). Once running, point a browser or `curl` at the configured `host:port`, e.g.:

```sh
curl http://127.0.0.1:8080/
```

## Project structure

```
src/        implementation (.cpp)
includes/   headers (.hpp)
config/     example .conf files
html/       served website: static pages, error pages (html/err/), uploads, cgi-bin/
bonus/      standalone bonus build (cookies/sessions), mirrors src/ and includes/
```

## Technical choices

- **Single process, single thread, one `epoll` loop** for every listening and client socket — concurrency comes entirely from multiplexing, not from threads or `fork()` per connection.
- **Every socket is non-blocking**, including file descriptors used for CGI pipes and FIFOs, so a slow client or a hung script can never stall the rest of the server.
- **Config syntax deliberately mirrors nginx** (`server`/`location` blocks, `root`, `listen`, `error_page`, `return`, etc.) to stay familiar to anyone who has touched a real web server config.

## Resources

### Documentation

- [RFC 7230](https://www.rfc-editor.org/rfc/rfc7230) – [RFC 7231](https://www.rfc-editor.org/rfc/rfc7231) – HTTP/1.1 message syntax and semantics.
- [RFC 3875](https://www.rfc-editor.org/rfc/rfc3875) – The Common Gateway Interface (CGI).
- [`epoll(7)`](https://man7.org/linux/man-pages/man7/epoll.7.html) man page.
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) – sockets fundamentals.
- [nginx `location` directive docs](https://nginx.org/en/docs/http/ngx_http_core_module.html#location) – inspiration for the config file syntax.

### Articles & talks

- Ali Naqvi, [*Writing an Nginx-like Web Server from Scratch*](https://www.alimnaqvi.com/blog/webserv).
- m4nnb3ll, [*Webserv: Building a Non-Blocking Web Server in C++98*](https://m4nnb3ll.medium.com/webserv-building-a-non-blocking-web-server-in-c-98-a-42-project-04c7365e4ec7).
- [*How HTTP Servers Work*](https://http.dev/http-connection)

### Videos

- [*Building a web server from scratch*](https://www.youtube.com/watch?v=V6ArZlHzZ6w)
- [*How HTTP Servers Work*](https://www.youtube.com/watch?v=YwHErWJIh6Y&t=1402s)
- Eliezer de Leon, [*What is a multiplexer*](https://www.youtube.com/watch?v=1B4SiOewm5Q)

### Use of AI

[Claude Code](https://claude.com/claude-code) (Anthropic) was used throughout development as a debugging and code-review assistant. Concretely, it was used to:

- Debug and explain existing behavior
- Review code written by the team 
- Implement small, explicitly requested fixes
- Produce documentation

AI was not used to author the server's core architecture (the `epoll` loop, the config parser, the HTTP parsing state machine, or the CGI/cookie implementations), which were designed and written by the team.
