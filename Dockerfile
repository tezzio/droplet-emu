# Reservoir: a private online server for games on Nintendo's NPLN platform, for the Droplet
# emulator. See README.md; docker compose up -d builds and starts it.
FROM python:3.13-alpine@sha256:bf12ecc0bea3f171d55145dd02c78b5eeb09f30ee2ac9cc2b4c9e4e8f5e30741

LABEL org.opencontainers.image.title="Reservoir" \
      org.opencontainers.image.description="Private online server for NPLN games, played through the Droplet emulator" \
      org.opencontainers.image.licenses="AGPL-3.0-or-later" \
      org.opencontainers.image.source="https://gitlab.com/seighttt/droplet/-/tree/server"

COPY requirements.txt /tmp/requirements.txt
RUN pip install --no-cache-dir --root-user-action=ignore --only-binary=:all: -r /tmp/requirements.txt \
    && rm /tmp/requirements.txt \
    && adduser -D -H -u 10001 -s /sbin/nologin reservoir \
    && install -d -o reservoir -g reservoir -m 0700 /data

COPY server/ /app/
COPY docker-entrypoint.sh /usr/local/bin/reservoir
RUN chmod 0755 /usr/local/bin/reservoir

USER reservoir
WORKDIR /app
VOLUME /data
# 8443 the game server (gRPC over TLS); 8090 the status page and admin panel (HTTP: put HTTPS
# in front, see compose.yaml); the rest are Pia's NAT check, ICE and quality reports.
EXPOSE 8443/tcp 8090/tcp 3478/tcp 3478/udp 3479/tcp 3479/udp 10025/udp 10125/udp 33334/udp 34343/udp
HEALTHCHECK --interval=30s --timeout=5s --start-period=30s \
    CMD wget -q -O /dev/null http://127.0.0.1:8090/api/stats || exit 1
ENTRYPOINT ["reservoir"]
CMD ["serve"]
