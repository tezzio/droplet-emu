"""Tests for Reservoir's accounts (npln_accounts.py), sign-in (npln_stub.py) and admin API
(npln_dashboard.py). The live part starts a server on 127.0.0.1 with its own data folder under
test-data and certificates of its own (its UDP services use their fixed ports, so no other
server may run on this machine).

    python -I tests/test_server.py      (needs grpcio and cryptography)
"""
import http.client
import json
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
TOOLS = HERE if (HERE / 'npln_stub.py').exists() else HERE.parent / 'server'  # next to it, or ../server
ONLINE = TOOLS.parent
sys.path.insert(0, str(TOOLS))
import npln_accounts  # noqa: E402
import npln_dashboard  # noqa: E402

A, B, C, D = '111122223333', '444455556666', '777788889999', '123412341234'
failures = []


def check(cond, what):
    print(('ok   ' if cond else 'FAIL ') + what)
    if not cond:
        failures.append(what)


def unit(folder):
    legacy = folder / 'friends.json'
    legacy.write_text(json.dumps({
        A: {'secret': npln_accounts.digest('secret-a-0123456789'), 'friends': [B], 'name': 'Ann', 'seen': 1},
        B: {'secret': npln_accounts.digest('secret-b-0123456789'), 'friends': [A], 'name': 'Bob', 'seen': 2}}))
    acc = npln_accounts.Accounts(folder / 'accounts.db', legacy_friends=legacy)
    check(acc.friends_of(A) == [B] and acc.friends_of(B) == [A], 'friends.json is imported')
    check(acc.login(A, 'secret-a-0123456789') is None, 'an imported player signs in with their secret')
    check(acc.login(A, 'another-secret-0000') is not None, 'another secret is refused')
    check(acc.login(A, 'secret-a-0123456789', added=[], removed=[]) is None and acc.friends_of(A) == [B],
          'signing in with an empty list keeps the friends')
    check(acc.login(C, 'secret-c-0123456789', added=[A]) is None, 'a new code signs up (registration open)')
    check(acc.requests_for(A) == [C] and acc.pending_from(C) == [A], 'an add that is not returned is a request')
    acc.login(A, 'secret-a-0123456789', added=[C])
    check(sorted(acc.friends_of(A)) == [B, C], 'adding back makes them friends')
    acc.login(A, 'secret-a-0123456789', removed=[C])
    check(acc.friends_of(A) == [B] and acc.requests_for(A) == [C], 'an explicit removal ends the friendship')
    check(acc.set('registration', 'approval') is None, 'registration can be set to approval')
    check('approve' in (acc.login(D, 'secret-d-0123456789') or ''), 'a new code waits for approval')
    check(acc.admin('approve', D) == 'ok' and acc.login(D, 'secret-d-0123456789') is None, 'after approval it signs in')
    acc.set('registration', 'closed')
    check(acc.login('999999999999', 'secret-x-0123456789') is not None, 'closed registration refuses new codes')
    check(acc.login(B, 'secret-b-0123456789') is None, 'closed registration still lets known codes in')
    acc.set('registration', 'open')
    check(acc.admin('release', B) == 'ok' and acc.login(B, 'brand-new-secret-99') is None,
          'a released code is claimed by the next sign-in')
    check(acc.login(B, 'secret-b-0123456789') is not None and acc.friends_of(B) == [A],
          'after the claim the old secret is refused and the friends stay')
    check(acc.admin('ban', C, text='test') == 'ok' and acc.login(C, 'secret-c-0123456789') == 'banned',
          'a banned code is refused')
    acc.admin('unban', C)
    check(acc.admin('transfer', B, other=D) == 'ok' and acc.friends_of(D) == [A] and acc.friends_of(B) == [],
          "transfer moves the friendships to the new code")
    check(acc.admin('befriend', C, other=D) == 'ok' and C in acc.friends_of(D), 'the admin can make two codes friends')
    check(acc.admin('unfriend', C, other=D) == 'ok' and C not in acc.friends_of(D), 'and remove a friendship')
    check(acc.set('room_max_players', 99) is not None and acc.set('room_max_players', 8) is None,
          'settings are range checked')
    check(acc.set('blocked_games', 'nonsense') is not None and acc.set('blocked_games', 't-e96531c6-lp1') is None,
          'blocked games take tenant ids')
    check(acc.login(A, 'secret-a-0123456789', tenant='t-e96531c6-lp1') is not None, 'a blocked game is refused')
    acc.set('blocked_games', '')
    acc.set('max_friends', 1)
    acc.login(C, 'secret-c-0123456789', added=[A, B, D])
    check(len(acc.pending_from(C)) + len(acc.friends_of(C)) <= 2, 'max_friends caps new adds')
    check(acc.room_size('t-54a666f5-lp1') == 4 and acc.room_size('t-e96531c6-lp1') == 24,
          'each game has its own room size')
    check(acc.set_game('t-e96531c6-lp1', room_size='12') is None and acc.room_size('t-e96531c6-lp1') == 12,
          'the admin can change a game\'s room size')
    check(acc.set_game('t-e96531c6-lp1', enabled=False) is None and not acc.game_enabled('t-e96531c6-lp1')
          and acc.login(A, 'secret-a-0123456789', tenant='t-e96531c6-lp1') is not None, 'a closed game refuses sign-ins')
    acc.set_game('t-e96531c6-lp1', enabled=True, room_size='')
    check(acc.game_enabled('t-e96531c6-lp1') and acc.room_size('t-e96531c6-lp1') == 24, 'and open it again')
    reopened = npln_accounts.Accounts(folder / 'accounts.db')
    check(reopened.get('room_max_players') == 8 and reopened.friends_of(D) == [A], 'settings and friends survive a restart')
    check(any(r['code'] == D and r['friends'] == [A] for r in reopened.rows()), 'the admin view lists accounts')
    codes(folder)


def codes(folder):
    """SW- friend codes, choosing a code, and each code's IP history."""
    norm, fmt = npln_accounts.normalize_code, npln_accounts.format_code
    check(norm('SW-1234-5678-9012') == norm('sw 1234 5678 9012') == norm('123456789012') == '123456789012'
          and norm('SW-0000-0000-0000') is None and norm('SW-1234-5678') is None,
          'codes are read with or without SW-, dashes and spaces')
    check(fmt('123456789012') == 'SW-1234-5678-9012', 'codes are shown as SW-1234-5678-9012')
    import npln_stub
    check(npln_stub.parse_identity('droplet-online-v1;code=123456789012;secret=0123456789abcdef')[0] == '123456789012'
          and npln_stub.parse_identity('ounce2dn-online-v1;code=123456789012;secret=0123456789abcdef') is not None,
          "both the Droplet and the older identity tokens sign in")
    acc = npln_accounts.Accounts(folder / 'codes.db')
    E, F, G, H, K = '100000000001', '100000000002', '100000000003', '100000000004', '100000000005'
    e, g = 'secret-e-0123456789', 'secret-g-0123456789'
    acc.login(E, e, added=[G], ip='203.0.113.5')
    acc.login(G, g, added=[E], ip='198.51.100.7')
    acc.login(E, e, ip='203.0.113.5')
    acc.login(E, e, ip='198.51.100.9')
    acc.login(F, 'secret-f-0123456789', ip='203.0.113.5')
    rows = {r['code']: r for r in acc.rows()}
    check([(x['ip'], x['logins']) for x in rows[E]['ips']] == [('198.51.100.9', 1), ('203.0.113.5', 2)],
          'each code keeps the addresses it signed in from')
    check(rows[E]['same_ip'] == [F] and rows[F]['same_ip'] == [E], 'codes that share an address are listed')
    check(acc.login(H, e, added=[G], ip='203.0.113.5') is None and acc.friends_of(H) == [G] and acc.friends_of(E) == [],
          'a code its player chose is theirs, and the friends come along')
    check({r['code']: r for r in acc.rows()}[H]['same_emulator'] == [E], "the admin sees an emulator's codes")
    check(acc.login(E, e) is None and acc.friends_of(E) == [G] and acc.friends_of(H) == [],
          'changing back brings the friends back')
    why = acc.login(G, e) or ''
    check('another emulator' in why and fmt(G) in why, "another player's code is refused")
    acc.admin('ban', E, text='test')
    check(acc.login(H, e) == 'banned' and acc.login(K, e) == 'banned' and not acc.exists(K),
          "a ban covers the emulator's other codes and new ones")
    acc.admin('unban', E)
    check(acc.login(H, e) is None, 'and lifting it frees them again')
    check(acc.admin('delete', F) == 'ok' and {r['code']: r for r in acc.rows()}[E]['same_ip'] == [H],
          'deleting an account drops its addresses')
    acc.set('ip_history_days', 1)
    acc.db.execute('UPDATE code_ips SET last = last - 2 * 86400 WHERE code = ? AND ip = ?', (E, '198.51.100.9'))
    acc.pruned = 0
    acc.login(G, g, ip='198.51.100.7')
    check([x['ip'] for x in {r['code']: r for r in acc.rows()}[E]['ips']] == ['203.0.113.5'],
          'addresses older than ip_history_days are forgotten')


def matchmaking(folder):
    """Mario Kart World's queues in-process: a lone ticket gets a lobby, later tickets backfill it
    while its host allows backfill, and rooms keep each game's size."""
    import npln_stub
    npln_stub.ACCOUNTS = npln_accounts.Accounts(folder / 'accounts.db')
    state = npln_stub.STATE = npln_stub.State(folder / 'audit.jsonl')
    mkw, config = 't-e96531c6-lp1', 'tenants/t-e96531c6-lp1/matchmakingConfigs/Normal24'
    attrs = {'batching_key': 'cv13', 'rom_id': 'cv13'}
    start = time.time()
    state.ticket('mt-0001', 'u-0000000000000001', config, attrs, tenant=mkw)
    state.match(start + 1)
    check(state.tickets['mt-0001']['status'] == npln_stub.SEARCHING, 'mkw: a lone ticket waits first')
    state.match(start + 16)
    sid = state.tickets['mt-0001']['session']
    check(sid and state.sessions[sid]['max_players'] == 24, 'mkw: then it gets a 24-player lobby of its own')
    state.ticket('mt-0002', 'u-0000000000000002', config, attrs, tenant=mkw)
    state.tickets['mt-0002']['created'] = start + 17
    state.match(start + 17)
    check(state.tickets['mt-0002']['status'] == npln_stub.SEARCHING, 'mkw: no backfill before the host allows it')
    state.sessions[sid]['docs']['__gs/m']['ebf'] = True
    state.match(start + 17.5)
    check(state.tickets['mt-0002']['status'] == npln_stub.SEARCHING,
          'mkw: no backfill into a lobby everyone has left (no participation record)')
    state.sessions[sid]['docs']['__pgn/All/__pus/us-0000000000000001'] = {}
    state.match(start + 18)
    check(state.tickets['mt-0002']['session'] == sid and len(state.sessions[sid]['members']) == 2
          and '__us/us-0000000000000002' in state.sessions[sid]['docs'], 'mkw: a later ticket joins the lobby (backfill)')
    state.ticket('mt-0003', 'u-0000000000000003', config, dict(attrs, batching_key='cv14'), tenant=mkw)
    state.tickets['mt-0003']['created'] = start + 19
    state.match(start + 19)
    check(state.tickets['mt-0003']['status'] == npln_stub.SEARCHING, 'mkw: another game version is not mixed in')
    state.sessions[sid]['docs']['__gs/m']['bfmax'] = 2
    state.ticket('mt-0004', 'u-0000000000000004', config, attrs, tenant=mkw)
    state.tickets['mt-0004']['created'] = start + 20
    state.match(start + 20)
    check(state.tickets['mt-0004']['session'] != sid, "mkw: the host's bfmax caps backfill")
    knockout = 'tenants/t-e96531c6-lp1/matchmakingConfigs/Survival24'
    state.ticket('mt-0010', 'u-0000000000000010', knockout, attrs, tenant=mkw)
    state.tickets['mt-0010']['created'] = start + 100
    state.match(start + 160)
    check(state.tickets['mt-0010']['status'] == npln_stub.SEARCHING,
          'mkw: a lone Knockout ticket waits longer (its lobbies take no one later)')
    state.ticket('mt-0011', 'u-0000000000000011', knockout, attrs, tenant=mkw)
    state.tickets['mt-0011']['created'] = start + 170
    state.match(start + 180)
    check(state.tickets['mt-0010'].get('session') and
          state.tickets['mt-0010'].get('session') == state.tickets['mt-0011'].get('session'),
          'mkw: so a second Knockout ticket a minute later shares its lobby')
    # The referee: both players' requests for one race get one accepted result, named by a UUID.
    from npln_stub import field, encode_map, decode, map_value, Fixed64, run_writes
    events = []
    log = lambda **e: events.append(e)
    request_id = '339459791_1_0_1_e0b87'
    for ussid, uid in ((1, 'u-0000000000000001'), (2, 'u-0000000000000002')):
        doc = field(1, f'docs/@@RefereeRequest/{ussid}/Request/{request_id}') + field(2, encode_map(
            {'users': [1, 2], 'ussid_map': {'0': 1, '1': 2}, 'course_id': 339459791, 'mode': 'normal',
             'sub_mode': 1}))
        run_writes(state.sessions[sid], [decode(field(2, field(1, doc)))], log, uid)
    results = {k: v for k, v in state.sessions[sid]['docs'].items() if k.startswith('@@RefereeResult/')}
    name = next(iter(results), '').removeprefix('@@RefereeResult/')
    check(len(results) == 1 and len(name) == 36 and [name[i] for i in (8, 13, 18, 23)] == ['-'] * 4
          and next(iter(results.values())) == {'request_id': request_id, 'status': 0, 'result': [],
                                               'user_session_count': 2},
          'mkw: the referee accepts a race once, under a UUID')
    # Finishing: each report (players as ussid_map indices) merges into the official order; the
    # race closes with ratings (by ussid) once every player still in the session is placed, a
    # player who left doesn't hold it open, and a tie (equal result_order) is a draw.
    docs = state.sessions[sid]['docs']
    result_path = next(iter(results))
    for ussid, uid in ((1, 'u-0000000000000001'), (2, 'u-0000000000000002')):
        docs[f'__pgn/All/__pus/us-{uid[2:]}'] = {}
        docs[f'PlayerStatic/{ussid}'] = {'rating_point_raw_value': 1153138688, 'rating_point_coefficient': 0}

    def report(ussid, uid, uuid, order, ranks=None):
        doc = field(1, f'docs/@@RefereeReport/{uuid}/User/{ussid}') + field(2, encode_map(
            {'result': order, 'result_order': ranks if ranks is not None else list(range(len(order)))}))
        run_writes(state.sessions[sid], [decode(field(2, field(1, doc)))], log, uid)

    def doubles(d):
        return {k: struct.unpack('<d', struct.pack('<Q', v))[0] for k, v in d.get('ratings', {}).items()}

    def start_race(request, mode='normal'):
        doc = field(1, f'docs/@@RefereeRequest/1/Request/{request}') + field(2, encode_map(
            {'users': [1, 2], 'ussid_map': {'0': 1, '1': 2}, 'course_id': 339459791, 'mode': mode,
             'sub_mode': 1}))
        run_writes(state.sessions[sid], [decode(field(2, field(1, doc)))], log, 'u-0000000000000001')
        return next(p for p, d in docs.items() if p.startswith('@@RefereeResult/') and d.get('request_id') == request)

    report(1, 'u-0000000000000001', name, [0])
    check(docs[result_path]['status'] == 0 and docs[result_path]['result'] == [0],
          'mkw: a finisher is placed while the race goes on')
    report(2, 'u-0000000000000002', name, [0, 1])
    final = docs[result_path]
    rating = doubles(final)
    check(final['status'] == 1 and final['result'] == [0, 1] and final['user_session_count'] == 2
          and rating.get('1', 0) > 1500 > rating.get('2', 9999) and abs(rating['1'] + rating['2'] - 3000) < 1e-6
          and all(isinstance(v, Fixed64) for v in final['coefficients'].values()),
          'mkw: the last finisher closes the race with ratings by ussid')
    path_battle = start_race('3957780982_1_0_1_1074d', 'battle')
    report(2, 'u-0000000000000002', path_battle.split('/')[1], [0, 1], [0, 0])
    tie = doubles(docs[path_battle])
    check(docs[path_battle]['status'] == 1 and abs(tie.get('1', 0) - 1500) < 1e-6 and abs(tie.get('2', 0) - 1500) < 1e-6,
          'mkw: a tie (equal result_order, a team battle) closes as a draw')
    sections = {}
    for section, dropout in ((1, 3), (2, 2)):
        request = f'Surv_457453114_1_0_{section}_ef659'
        doc = field(1, f'docs/@@RefereeRequest/1/Request/{request}') + field(2, encode_map(
            {'users': [1, 2], 'ussid_map': {'0': 1, '1': 2}, 'course_id': 457453114, 'mode': 'survival',
             'sub_mode': 2, 'option': {'total_player_count': 4, 'dropout_rank': dropout}}))
        run_writes(state.sessions[sid], [decode(field(2, field(1, doc)))], log, 'u-0000000000000001')
        sections[section] = next(p for p, d in docs.items() if p.startswith('@@RefereeResult/')
                                 and d.get('request_id') == request)
    report(1, 'u-0000000000000001', sections[1].split('/')[1], [0, 2, 1, 3])
    report(1, 'u-0000000000000001', sections[2].split('/')[1], [2, 0])
    check(docs[sections[1]]['status'] == 1 and docs[sections[2]]['status'] == 1,
          'mkw: Knockout: a player out after section 1 does not hold section 2 open')
    path2 = start_race('339459791_1_0_1_e0b88')
    del docs['__pgn/All/__pus/us-0000000000000002']
    report(1, 'u-0000000000000001', path2.split('/')[1], [0])
    check(docs[path2]['status'] == 1 and docs[path2]['result'] == [0, 1] and docs[path2]['user_session_count'] == 1,
          'mkw: a player who left does not hold the race open')
    # Room IDs: a short alias names the caller's room (the request as View Room Info sends it),
    # and the bare digits find it again.
    alias_call = {'uid': 'u-0000000000000001', 'log': log}
    created = decode(npln_stub.REPLIES['CreateGameSessionShortAlias'](
        field(1, 'tenants/current') + field(2, field(2, f'tenants/current/gameSessions/{sid}')), alias_call))
    name = next(v for n, wt, v in created if n == 1)
    alias = name.rsplit('/', 1)[-1]
    check(re.fullmatch(r'tenants/[^/]+/GameSessionShortAliases/\d{6}', name) is not None,
          'mkw: a room ID is named tenants/<t>/GameSessionShortAliases/<id> (the game aborts on any other form)')
    found = decode(npln_stub.REPLIES['GetGameSessionShortAlias'](
        field(1, f'tenants/current/GameSessionShortAliases/{alias}'), alias_call))
    check(any(n == 2 and v.endswith(f'/gameSessions/{sid}') for n, wt, v in found),
          'mkw: a room ID (short alias) leads back to its room')
    value = map_value(decode(encode_map({'r': Fixed64.double(2.5)}))[0][2]['msg'][1][2]['msg'])
    check(isinstance(value, Fixed64) and value == Fixed64.double(2.5), 'mkw: doubles keep their type (5)')
    room = {'id': 'gs-9000', 'tenant': 't-54a666f5-lp1', 'room': True, 'members': [], 'tickets': [],
            'docs': {}, 'updated': {}, 'watchers': [], 'attributes': {}}
    joined = [npln_stub.add_room_member(room, f'u-{i:016x}') for i in range(1, 7)]
    check(joined == [True] * 4 + [False] * 2, 'raiders: rooms take 4 players')
    room.update(id='gs-9001', tenant=mkw, members=[], tickets=[], docs={})
    check(all(npln_stub.add_room_member(room, f'u-{i:016x}') for i in range(1, 25))
          and not npln_stub.add_room_member(room, f'u-{99:016x}'), 'mkw: rooms take 24 players')
    # Game versions: a room keeps its host's version and refuses joiners on another one (requests
    # shaped like the game's); a keyword search only finds rooms of the searcher's version.
    host, same, other, unknown = (f'u-00000000000000a{i}' for i in range(1, 5))
    for uid in (host, same, other, unknown):
        state.users[uid] = {'uid': uid, 'ip': '127.0.0.1', 'peer': 'test', 'since': start,
                            'last': time.time(), 'left': None, 'streams': {}, 'presence': {}, 'code': None}

    def member(attrs):
        return field(1, 'tenants/current/users/current') + field(2, encode_map(attrs))
    npln_stub.REPLIES['CreateGameSessionCreationTicket'](
        field(1, 'tenants/current') + field(2, field(2, 'tenants/current/matchmakingConfigs/Private24')
                                             + field(3, member({'batching_key': 'cv13', 'rom_id': 'cv13'}))
                                             + field(6, field(11, encode_map({'course_id': 0})))),
        {'uid': host, 'log': log, 'meta': {'npln-tenant-id': mkw}})
    vroom = next(s for s in state.sessions.values() if s.get('host') == host)

    def join(uid, attrs):
        try:
            npln_stub.REPLIES['JoinGameSession'](
                field(1, f'tenants/current/gameSessions/{vroom["id"]}') + field(3, member(attrs)),
                {'uid': uid, 'log': log})
        except npln_stub.RpcError as e:
            return e.code.name
        return uid in vroom['members']
    check(vroom.get('version') == 'cv13' and join(other, {'batching_key': 'cv14'}) == 'FAILED_PRECONDITION'
          and other not in vroom['members'], 'versions: a room refuses a player on another game version')
    check(join(same, {'batching_key': 'cv13'}) is True and join(unknown, {}) is True,
          'versions: the same version, or an unknown one, joins')
    snap = state.snapshot(private=True)
    check(any(p['version'] == 'cv14' for p in snap['players'])
          and any(r['version'] == 'cv13' for r in snap['rooms'])
          and all('game_version' in t for t in snap['tickets']),
          'versions: the admin view shows players\', rooms\' and tickets\' versions')
    raiders = 't-54a666f5-lp1'
    keyword_room = {'id': 'gs-9100', 'name': f'tenants/{raiders}/gameSessions/gs-9100', 'tenant': raiders,
                    'room': True, 'version': 'match 1', 'host': host, 'members': [host], 'tickets': [None],
                    'attributes': {'session_keyword': '4242'}, 'docs': {'__gs/m': {'cp': True}},
                    'updated': {}, 'watchers': [], 'created': time.time()}
    state.sessions['gs-9100'] = keyword_room
    for tid, uid, match_hash in (('mt-0100', other, 2), ('mt-0101', same, 1)):
        state.ticket(tid, uid, f'tenants/{raiders}/matchmakingConfigs/spiral_search_config',
                     {'search_keyword': '4242', 'match_hash': match_hash}, tenant=raiders)
    state.match(time.time())
    check(state.tickets['mt-0100'].get('session') != 'gs-9100' and state.tickets['mt-0101'].get('session') == 'gs-9100',
          'versions: a keyword search only finds a room of its own version')
    # Game builds (Droplet sends the game's build ID at sign-in): players on known builds that
    # differ never meet, in rooms, keyword searches, Call for Help, backfill or new lobbies.
    token = 'droplet-online-v1;code=123456789012;secret=0123456789abcdef'
    check(npln_stub.identity_build(token + ';build=0A1B2C3D4E5F6071;friends=') == '0a1b2c3d4e5f6071'
          and npln_stub.identity_build(token + ';build=not-hex') is None
          and npln_stub.identity_build(token) is None, 'builds: the sign-in token carries the game build')
    aa, bb = 'aa' * 8, 'bb' * 8
    b_host, b_same, b_other, b_none, b_other2, b_same2 = (f'u-00000000000000b{i}' for i in range(1, 7))
    for uid, build in ((b_host, aa), (b_same, aa), (b_other, bb), (b_none, None), (b_other2, bb), (b_same2, aa)):
        state.users[uid] = {'uid': uid, 'ip': '127.0.0.1', 'peer': 'test', 'since': start,
                            'last': time.time(), 'left': None, 'streams': {}, 'presence': {}, 'code': None}
        state.note_build(uid, build)
    npln_stub.REPLIES['CreateGameSessionCreationTicket'](
        field(1, 'tenants/current') + field(2, field(2, 'tenants/current/matchmakingConfigs/Private24')
                                             + field(3, member({}))
                                             + field(6, field(11, encode_map({'course_id': 0})))),
        {'uid': b_host, 'log': log, 'meta': {'npln-tenant-id': mkw}})
    vroom = next(s for s in state.sessions.values() if s.get('host') == b_host)
    check(not vroom.get('version') and join(b_other, {}) == 'FAILED_PRECONDITION' and b_other not in vroom['members']
          and join(b_same, {}) is True and join(b_none, {}) is True,
          'builds: a room refuses a player on another build; the same build, or an unknown one, joins')
    state.sessions['gs-9300'] = dict(keyword_room, id='gs-9300', name=f'tenants/{raiders}/gameSessions/gs-9300',
                                     version=None, host=b_host, members=[b_host], tickets=[None],
                                     attributes={'session_keyword': '4343'}, docs={'__gs/m': {'cp': True}},
                                     updated={}, watchers=[], created=time.time())
    for tid, uid in (('mt-0200', b_other), ('mt-0201', b_same)):
        state.ticket(tid, uid, f'tenants/{raiders}/matchmakingConfigs/spiral_search_config',
                     {'search_keyword': '4343', 'match_hash': 1}, tenant=raiders)
    state.match(time.time())
    check(state.tickets['mt-0200'].get('session') != 'gs-9300' and state.tickets['mt-0201'].get('session') == 'gs-9300',
          'builds: a keyword search only finds a room of its own build')
    calling = {'match_hash': 1, 'select_dungeon_type': 1, 'select_stage_id_1': 5}
    helping = {'match_hash': 1, 'playable_stage_id_1': [5]}
    caller = {'uid': b_host, 'bot': False, 'attributes': calling}
    check(state._fits(caller, {'uid': b_same, 'attributes': helping})
          and state._fits(caller, {'uid': b_none, 'attributes': helping})
          and not state._fits(caller, {'uid': b_other, 'attributes': helping}),
          'builds: Call for Help pairs a caller only with helpers on the same build')
    check([h['uid'] for h in state._agreeing({'uid': b_none}, [{'uid': u} for u in (b_other, b_same, b_other2)])]
          == [b_other, b_other2], "builds: a caller of unknown build gets helpers that agree with each other")
    later, battrs = start + 400, {'batching_key': 'cv99', 'rom_id': 'cv99'}

    def ticket(tid, uid, created):
        state.ticket(tid, uid, config, battrs, tenant=mkw)
        state.tickets[tid]['created'] = created
    ticket('mt-0300', b_host, later)
    state.match(later + 16)
    bsid = state.tickets['mt-0300'].get('session')
    lobby = state.sessions[bsid]
    lobby['docs']['__gs/m']['ebf'] = True
    lobby['docs'][f'__pgn/All/__pus/us-{b_host[2:]}'] = {}
    ticket('mt-0301', b_other, later + 17)
    ticket('mt-0302', b_same, later + 17.5)
    state.match(later + 18)
    check(state.tickets['mt-0302'].get('session') == bsid and state.tickets['mt-0301'].get('session') != bsid,
          "builds: backfill only takes players on the lobby's build")
    lobby['docs']['__gs/m']['ebf'] = False
    ticket('mt-0303', b_same2, later + 17)
    state.match(later + 24)
    check(state.tickets['mt-0301']['status'] == state.tickets['mt-0303']['status'] == npln_stub.SEARCHING,
          "builds: two players on different builds don't get a lobby together")
    ticket('mt-0304', b_other2, later + 17)
    ticket('mt-0305', b_none, later + 17)
    state.match(later + 25)
    new = state.tickets['mt-0301'].get('session')
    check(new and new == state.tickets['mt-0304'].get('session') == state.tickets['mt-0305'].get('session')
          and state.tickets['mt-0303']['status'] == npln_stub.SEARCHING,
          'builds: a new lobby takes players on one build, and players whose build is unknown')
    public, private = state.snapshot(), state.snapshot(private=True)
    check(any(p['build'] == aa for p in public['players']) and any(t.get('build') == bb for t in public['tickets'])
          and any(r.get('build') == aa for r in private['rooms']),
          "builds: the status page and admin panel show players', tickets' and rooms' builds")


def login_request(code, secret, friends=(), removed=(), build=None):
    from npln_stub import field
    token = f'ounce2dn-online-v1;code={code};secret={secret}'
    if build:
        token += f';build={build}'
    token += f';friends={",".join(friends)}'
    if removed:
        token += f';removed={",".join(removed)}'
    return field(1, 'tenants/t-54a666f5-lp1') + field(3, field(1, token))


def live(folder):
    import grpc
    npln_dashboard.write_password_file(folder / 'admin-password.json', 'test password 123')
    import make_pki
    pki = make_pki.make(folder / 'pki')
    stub = subprocess.Popen([sys.executable, '-I', str(TOOLS / 'npln_stub.py'), '--port', '18443',
                             '--pki', str(pki), '--web-port', '18090', '--data', str(folder),
                             '--quiet-log', '--log', str(folder / 'server.jsonl')],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    try:
        for _ in range(50):
            time.sleep(0.2)
            try:
                probe = http.client.HTTPConnection('127.0.0.1', 18090, timeout=1)
                probe.request('GET', '/api/stats')
                probe.getresponse().read()
                break
            except OSError:
                continue
        creds = grpc.ssl_channel_credentials((pki / 'ca.pem').read_bytes())
        channel = grpc.secure_channel('localhost:18443', creds)
        login = channel.unary_unary('/nn.npln.auth.v1.Auth/IssuePrearrangedUserToken')

        def sign_in(*args):
            try:
                login(login_request(*args), timeout=5)
                return None
            except grpc.RpcError as e:
                return e.details()

        check(sign_in(A, 'live-secret-a-012345', [B]) is None, 'live: A signs in')
        check(sign_in(B, 'live-secret-b-012345', [A]) is None, 'live: B signs in')
        check('another emulator' in (sign_in(A, 'wrong-secret-0123456') or ''), 'live: a wrong secret is refused')

        def admin(method, path, body=None):
            conn = http.client.HTTPConnection('127.0.0.1', 18090, timeout=5)
            headers = {'X-Reservoir-Admin': '1', 'Content-Type': 'application/json', 'Cookie': cookie}
            conn.request(method, path, json.dumps(body) if body is not None else None, headers)
            r = conn.getresponse()
            return r.status, json.loads(r.read() or b'{}'), r.getheader('Set-Cookie')

        cookie = ''
        status, _, set_cookie = admin('POST', '/api/admin/login', {'password': 'test password 123'})
        cookie = (set_cookie or '').split(';')[0]
        check(status == 200 and cookie, 'live: admin sign-in')
        status, data, _ = admin('GET', '/api/admin/accounts')
        rows = {r['code']: r for r in data.get('accounts', [])}
        check(status == 200 and rows.get(A, {}).get('friends') == [B], 'live: the admin sees A and B as friends')
        check([e['ip'] for e in rows.get(A, {}).get('ips', [])] == ['127.0.0.1'], 'live: the admin sees where A signed in from')
        check(any(s['key'] == 'registration' for s in data.get('settings', [])), 'live: the admin sees the settings')
        status, data, _ = admin('POST', '/api/admin/settings', {'key': 'registration', 'value': 'closed'})
        check(data.get('result') == 'ok', 'live: the admin closes registration')
        check('not taking new players' in (sign_in(C, 'live-secret-c-012345') or ''), 'live: a new code is refused')
        admin('POST', '/api/admin/settings', {'key': 'registration', 'value': 'open'})
        status, data, _ = admin('POST', '/api/admin/account', {'op': 'release', 'code': A})
        check(data.get('result') == 'ok' and sign_in(A, 'reclaimed-secret-0123') is None,
              'live: a released code is reclaimed by a new emulator')
        status, data, _ = admin('POST', '/api/admin/account', {'op': 'ban', 'code': B, 'text': 'test'})
        check(sign_in(B, 'live-secret-b-012345', [A]) == 'banned', 'live: a banned account is refused')
        status, data, _ = admin('POST', '/api/admin/account', {'op': 'nonsense', 'code': B})
        check(status == 400, 'live: unknown account actions are refused')
        conn = http.client.HTTPConnection('127.0.0.1', 18090, timeout=5)
        conn.request('GET', '/api/admin/accounts', headers={'X-Reservoir-Admin': '1'})
        check(conn.getresponse().status == 401, 'live: the admin API needs a session')
        status, data, _ = admin('GET', '/api/admin/state')
        check(status == 200 and 'rooms' in data, 'live: the admin state lists rooms')
        check(sign_in(C, 'live-secret-c-012345', (), (), '0A1B2C3D4E5F6071') is None, 'live: C signs in with a game build')
        status, data, _ = admin('GET', '/api/admin/state')
        conn = http.client.HTTPConnection('127.0.0.1', 18090, timeout=5)
        conn.request('GET', '/api/stats')
        public = json.loads(conn.getresponse().read())
        check(any(p.get('build') == '0a1b2c3d4e5f6071' for p in data.get('players', []))
              and any(p.get('build') == '0a1b2c3d4e5f6071' for p in public.get('players', [])),
              'live: the admin panel and status page show the build a player signed in with')
        conn = http.client.HTTPConnection('127.0.0.1', 18090, timeout=5)
        conn.request('GET', '/ca.der')
        r = conn.getresponse()
        check(r.status == 200 and r.read() == (pki / 'ca.der').read_bytes(),
              "live: players can download the server's CA")
        conn.request('GET', '/ca.key')
        r = conn.getresponse()
        r.read()
        check(r.status == 404, "live: the CA's private key is never served")
    finally:
        stub.terminate()
        err = stub.communicate(timeout=10)[1].decode(errors='replace')
        if 'Traceback' in err:
            print(err[-3000:])
            failures.append('the stub raised an exception')


def main():
    base = ONLINE / 'test-data'
    base.mkdir(exist_ok=True)
    folder = Path(tempfile.mkdtemp(dir=base))
    try:
        (folder / 'unit').mkdir()
        unit(folder / 'unit')
        (folder / 'match').mkdir()
        matchmaking(folder / 'match')
        (folder / 'live').mkdir()
        live(folder / 'live')
    finally:
        shutil.rmtree(folder, ignore_errors=True)
    print(f'{len(failures)} failed' if failures else 'all passed')
    sys.exit(1 if failures else 0)


if __name__ == '__main__':
    main()
