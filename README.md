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

## Configuration Options

| Option | Type | Default | Descrption |
| ------ | ---- | ------- | ---------- |
| name | string | required | a unique name for the server |
| port | int | 80 | the port the server will run on |
| root | string | required | the root path of the website |
| address | string | 127.0.0.1 | local address that the sever will run on |
| http | table | null | if set, http will be enabled |
| http.port | int | 80 | the port to run http on |
| https | table | null | if set, https will be enabled |
| https.port | int | 443 | the port to run https on |
| https.key | string | required | the server's TLS key |
| https.cert | string | required | the server's TLS cert |
| threads | int | automatic | the number of worker threads the server will use |
| backlog | int | 0 | the maximum number of pending connections before it declines. 0 means an implementation defined minimum. |

# Running

You can run the server using `./minhttp`.

To change which file the server uses for its configuration, you can use the `-C <file>` flag. To run it as a daemon, pass the `-d` flag.
