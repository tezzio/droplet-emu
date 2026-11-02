#!/bin/sh
# Reservoir's container entry point.
#   reservoir [serve] [server options]   make the certificates on the first start, then run
#   reservoir set-admin-password         choose the admin panel's password (asks twice)
#   reservoir make-pki                   make the certificates only
# Everything the server keeps (accounts, friends, bans, settings, the admin password's hash and
# the certificates) is in /data.
set -eu
DATA=/data
PKI=$DATA/pki

make_pki() {
    python -I /app/make_pki.py --out "$PKI" --name "${RESERVOIR_NAME:-Reservoir}"
}

command=${1:-serve}
[ $# -gt 0 ] && shift
case "$command" in
    serve)
        [ -f "$PKI/server.key" ] || make_pki
        if [ -n "${RESERVOIR_TRUST_PROXY:-}" ]; then
            set -- --trust-proxy "$RESERVOIR_TRUST_PROXY" "$@"
        fi
        # No --log: events go to stdout, where docker logs (and its rotation) keep them
        exec python -I /app/npln_stub.py --public --host 0.0.0.0 --port 8443 \
            --web-host 0.0.0.0 --web-port 8090 --quiet-log --data "$DATA" --pki "$PKI" "$@"
        ;;
    set-admin-password)
        exec python -I /app/npln_stub.py --data "$DATA" --set-admin-password
        ;;
    make-pki)
        make_pki
        ;;
    *)
        exec "$command" "$@"
        ;;
esac
