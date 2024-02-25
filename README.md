# minhttp

An http server that is highly extendible and supported. (note: in dev and not yet highly extendible or supported)

# How to Build

```
make
```

# Configuration

There is an example configuration in `./config.toml.example`. You can rename/copy it to `config.toml`,
then update the existing config.
To run a second server, simply add it to the config.

```toml
[[server]]
name = "example"
host = "www.example.com"
root = "/var/example"

[[server]]
name = "workgroup"
host = "www.workgroup.com"
root = "/var/workgroup"
```

List of options supported:
| Option | Type | Default | Descrption |
| ------ | ---- | ------- | ---------- |
| name | string | required | a unique name for the server |
| port | int | 80 | the port the server will run on |
| root | string | required | the root path of the website |
| host | string | required | the hostname of the server (eg www.example.com) |
| logfile | string | null | an optional file for the server to write log info to |
| backlog | int | 16 | maximum number of pending awaiting connections before the server starts declining them |
| keyfile | string | null | the servers TLS key, setting this automatically implies HTTPS support |
| certfile | string | null | the server's TLS certificate, setting this automatically implies HTTPS support |
| threads | int | automatic | number of threads the server should use |

# Running

You run the server using `./minhttp`.

To change which file the server uses for its configuration, you can use the `-C <file>` flag. To run it as a daemon, pass the `-d` flag.
