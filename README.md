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

# Running

You run the server using `./minhttp`.

To change which file the server uses for its configuration, you can use the `-C <file>` flag. To run it as a daemon, pass the `-d` flag.
