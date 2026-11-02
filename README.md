# Reservoir

Reservoir is a private online server for games built on Nintendo's NPLN online platform, such as
Splatoon Raiders and Mario Kart World, played through the
[Droplet](https://gitlab.com/seighttt/droplet) emulator. It covers sign-in, friend codes and
friend lists, matchmaking, rooms (with room IDs), the session sync the games use, and Mario Kart
World's race referee. It also has a status page and an admin panel.

Reservoir is for emulators only. It never talks to Nintendo's servers, and it contains no
Nintendo code, keys or assets. Players need their own copies of the games. This project is not
affiliated with or endorsed by Nintendo.

Reservoir began as a proof of concept, inspired by how easy it is to set up a Minecraft server on
an Oracle Cloud instance in a few minutes: hosting private online play for friends should be that
simple too. There are no official Reservoir servers. The project's developer does not host any
servers for players: every server is run independently by whoever set it up, and the project does
not run, endorse or take responsibility for any of them. The project has no official Discord server
either; the only official place is this GitLab project.

## Run a server with Docker

You need Docker with the Compose plugin, on a machine your players can reach (a home PC for
friends on the same network or VPN, or a cloud server).

```sh
git clone -b server https://gitlab.com/seighttt/droplet.git reservoir
cd reservoir
cp .env.example .env              # optional: your server's name, and a domain for HTTPS
docker compose up -d
docker compose exec reservoir reservoir set-admin-password
```

On its first start the server makes its own certificate authority (CA). Players need that CA's
certificate, `ca.der`, so that Droplet trusts your server. They can download it from the status
page, or you can copy it out and send it:

```sh
docker compose cp reservoir:/data/pki/ca.der .
```

The status page and admin panel are at <http://127.0.0.1:8090/> and `/admin` on the server
itself. To reach them from elsewhere over HTTPS, point a domain name at the server, set
`RESERVOIR_DOMAIN` in `.env`, open ports 80 and 443, and start the `https` profile:

```sh
docker compose --profile https up -d
```

### Ports to open

| Port | Protocol | What |
|---|---|---|
| 8443 | TCP | the game server (gRPC over TLS) |
| 3478, 3479 | TCP and UDP | ICE (players connecting to each other) |
| 10025, 10125, 33334 | UDP | the games' NAT check |
| 34343 | UDP | the games' connection-quality reports |
| 80, 443 | TCP | only with the `https` profile |

### Updating, backups and logs

- Update: `git pull && docker compose up -d --build`.
- Everything the server keeps lives in the `data` volume: accounts, friendships, bans, settings,
  the admin password's hash, and the certificates. Back that volume up.
- Keep `pki/ca.key` private. If you lose the CA, the server makes a new one, and every player
  has to download the new `ca.der`.
- Logs: `docker compose logs -f reservoir` (JSON lines, events only, no message contents).

### The admin password

The admin panel (`/admin`) manages players, friend codes, bans, rooms and settings. It has one
password, which you choose on the server:

```sh
docker compose exec reservoir reservoir set-admin-password
```

- It asks twice and needs at least 12 characters. A few random, unrelated words make a strong
  password that is easy to type. Don't reuse a password from anywhere else.
- The server keeps only a scrypt hash of it (`admin-password.json` in the data volume, readable
  only by the server). The password itself is never saved or logged.
- To change the password, or if you forget it, run the same command again. It takes effect at
  once. Anyone who can log in to the machine can do this, so the panel is only as safe as the
  machine: keep its SSH key private.
- Changing the password doesn't sign out anyone already signed in. To end every session, restart
  the server: `docker compose restart reservoir`. Players online are disconnected too, and can
  join again.
- Five wrong passwords from one address lock that address out for 15 minutes. Sign-ins (with the
  address they came from) and every admin action are in the panel's audit log.
- A sign-in lasts 8 hours; Sign out ends it sooner.
- Open the panel only over HTTPS (with a domain and the `https` profile) or through an SSH tunnel
  (see [With or without a domain](#5-with-or-without-a-domain)). Never open port 8090 to the
  internet: it is plain HTTP, so your password would cross the internet unencrypted.
  `compose.yaml` keeps it on 127.0.0.1 for that reason.

## Hosting for free on Oracle Cloud

Oracle Cloud's [Always Free tier](https://www.oracle.com/cloud/free/) includes small virtual
machines that cost nothing and don't expire, which is plenty for a Reservoir server for you and your
friends. Signing up needs a credit or debit card so Oracle can check who you are; Always Free
resources are not charged. Oracle stops Always Free machines it sees as idle: if yours stops, start
it again in the console. (The console and firewall steps below are the ones this project's own
server uses; this Docker setup on Oracle is new, so tell us if a step doesn't match what you see.)

### 1. Create the machine

1. Sign up for the free tier and sign in to the Oracle Cloud console.
2. Compute > Instances > Create instance.
3. Image: **Canonical Ubuntu 24.04**. Shape: an Always Free one, such as **VM.Standard.A1.Flex**
   (Ampere) with 1 OCPU and 6 GB of memory. If Oracle says it is out of capacity, use
   **VM.Standard.E2.1.Micro**, which is also Always Free.
4. Networking: keep "Assign a public IPv4 address" on.
5. SSH keys: let Oracle generate a key pair and **download the private key**. You need it to log in.
6. Create. When the machine is running, note its **public IP address** on the instance page.

### 2. Open the ports in Oracle's firewall

Networking > Virtual cloud networks > your network > Security > the default security list > Add
Ingress Rules. Use the source CIDR `0.0.0.0/0` and add each line of the ports table above: TCP 8443,
TCP and UDP 3478-3479, and UDP 10025, 10125, 33334 and 34343. Add TCP 80 and 443 only if you'll use
a domain (step 5).

### 3. Log in and install Docker

On your PC, in the folder with the downloaded key (`ubuntu` is the login name on Ubuntu images):

```sh
chmod 600 ssh-key.key
ssh -i ssh-key.key ubuntu@SERVER_IP
```

On the server, open the same ports in its own firewall (Oracle's Ubuntu images block everything
else), then install Docker and log out:

```sh
sudo iptables -I INPUT -p tcp -m multiport --dports 8443,3478,3479 -j ACCEPT
sudo iptables -I INPUT -p udp -m multiport --dports 3478,3479,10025,10125,33334,34343 -j ACCEPT
sudo netfilter-persistent save
sudo apt update && sudo apt install -y docker.io docker-compose-v2 git
sudo usermod -aG docker ubuntu
exit
```

With a domain, use `--dports 8443,3478,3479,80,443` in the first line.

### 4. Start Reservoir

Log in again (so Docker works without `sudo`), then:

```sh
git clone -b server https://gitlab.com/seighttt/droplet.git reservoir
cd reservoir
cp .env.example .env
docker compose up -d
docker compose exec reservoir reservoir set-admin-password
docker compose cp reservoir:/data/pki/ca.der ca.der
```

`set-admin-password` sets the admin panel's password; see [The admin password](#the-admin-password)
for choosing it and keeping it safe. Back on your PC, fetch the certificate players need:

```sh
scp -i ssh-key.key ubuntu@SERVER_IP:reservoir/ca.der .
```

### 5. With or without a domain

Domains cost money; the server works without one.

- **Without a domain**, players connect to `SERVER_IP:8443`. The game's connection is encrypted with
  your server's own certificate either way: Droplet checks it against `ca.der`, not against the
  address you typed. The status page and admin panel are plain HTTP, though, so keep them off the
  internet (the admin password would cross it unencrypted) and reach them through an SSH tunnel:
  run `ssh -i ssh-key.key -L 8090:127.0.0.1:8090 ubuntu@SERVER_IP`, keep it open, and browse to
  <http://127.0.0.1:8090/admin>. Send players `ca.der` yourself.
- **With a domain**, add an **A** record for a name, such as `reservoir.example.org`, pointing at
  `SERVER_IP` where you manage the domain. Open TCP 80 and 443 in both firewalls, set
  `RESERVOIR_DOMAIN=reservoir.example.org` in `.env`, and run
  `docker compose --profile https up -d`.
  The status page (with `ca.der` to download) is then at `https://reservoir.example.org/`, and the
  admin panel at `/admin`.

### 6. Connect Droplet

In Droplet's Configure > System > Network, turn online play on, set the server to `SERVER_IP:8443`
(or `reservoir.example.org:8443`) and pick `ca.der`.

Now and then, update the machine with `sudo apt update && sudo apt upgrade`, and Reservoir with
`cd reservoir && git pull && docker compose up -d --build`.

## Playing on a Reservoir server

In Droplet, open Configure > System > Network and do three things:

1. Turn online play on.
2. Enter the server's address with its port: a name such as `reservoir.example.org:8443`, or the
   server's IP address, such as `203.0.113.7:8443`.
3. Pick the server's `ca.der`.

Your friend code (`SW-xxxx-xxxx-xxxx`) is made the first time you play online. **Change code…**
picks another one, if nobody else on the server has it. Add friends by their codes; you become
friends once you have both added each other.

Players can only play together on the same version of a game (the same update). Droplet tells the
server the exact build of the game each player runs, so matchmaking, rooms and room searches keep
players on different builds apart, in every game. When a player's emulator doesn't send its build
(an older Droplet), the server goes by the version the game reports, where it can tell: always in
Mario Kart World, and in Splatoon Raiders once the player has searched for a match. The status
page and admin panel show each player's version and build, so you can see why two players never
meet.

## Running without Docker

You need Python 3.12 or newer and the packages in `requirements.txt`.

```sh
python -m pip install -r requirements.txt
python -I server/make_pki.py --out data/pki
python -I server/npln_stub.py --set-admin-password --data data
python -I server/npln_stub.py --data data --pki data/pki
```

By default the server listens on 127.0.0.1 only. `--host` sets its addresses,
comma-separated (`--host 127.0.0.1,192.168.1.20` adds a LAN address). Public addresses, or
0.0.0.0, also need `--public`; use `--quiet-log` with them. `python -I server/npln_stub.py
--help` lists every option.

## Tests

```sh
python -I tests/test_server.py
```

The tests start a server on 127.0.0.1 with fixed UDP ports, so stop any other Reservoir on the
same machine first.

## License

Reservoir is free software under the GNU Affero General Public License, version 3 or later
(`LICENSE`). If you run a modified version for others, you must offer them its source.
