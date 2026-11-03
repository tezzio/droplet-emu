// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include "common/fs/path_util.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/hle/service/sockets/online_redirect.h"
#include "core/internal_network/network_interface.h"
#include "ui_configure_network.h"
#include "yuzu/configuration/configure_network.h"

namespace OnlineRedirect = Service::Sockets::OnlineRedirect;

namespace {
constexpr int CodeRole = Qt::UserRole;
constexpr int KindRole = Qt::UserRole + 1;
// A friend list row: a friend, a request received or sent (from the server), or a code this
// emulator added (when the server's lists aren't there)
enum FriendKind { FriendRow = 1, ReceivedRow, SentRow, AddedRow };
} // namespace

ConfigureNetwork::ConfigureNetwork(const Core::System& system_, QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureNetwork>()), system{system_} {
    ui->setupUi(this);
    for (const auto& iface : Network::GetAvailableNetworkInterfaces())
        ui->network_interface->addItem(QString::fromStdString(iface.name));
    // Online play needs the network: the two boxes can't both be on.
    connect(ui->online_enabled, &QCheckBox::toggled, this, [this](bool on) {
        if (on) {
            ui->airplane_mode->setChecked(false);
        }
        UpdateOnlineFields();
        RefreshFriends();
    });
    connect(ui->airplane_mode, &QCheckBox::toggled, this, [this](bool on) {
        if (on) {
            ui->online_enabled->setChecked(false);
        }
    });
    connect(ui->online_server, &QLineEdit::editingFinished, this, [this] {
        if (ui->online_server->text().trimmed().toStdString() != profile_server) {
            LoadProfileView();
        }
    });
    connect(ui->choose_server_ca, &QPushButton::clicked, this, &ConfigureNetwork::ChooseServerCa);
    connect(ui->copy_friend_code, &QPushButton::clicked, this, [this] {
        QGuiApplication::clipboard()->setText(ui->friend_code->text());
    });
    connect(ui->other_friend_code, &QPushButton::clicked, this, &ConfigureNetwork::ChangeCode);
    connect(ui->add_friend, &QPushButton::clicked, this, &ConfigureNetwork::AddFriends);
    connect(ui->new_friend, &QLineEdit::returnPressed, this, &ConfigureNetwork::AddFriends);
    connect(ui->remove_friend, &QPushButton::clicked, this, &ConfigureNetwork::RemoveFriends);
    connect(ui->accept_friend, &QPushButton::clicked, this, [this] { AnswerRequests("accept"); });
    connect(ui->decline_friend, &QPushButton::clicked, this, [this] { AnswerRequests("decline"); });
    connect(ui->online_friends, &QListWidget::itemSelectionChanged, this,
            &ConfigureNetwork::UpdateFriendButtons);
    connect(ui->backup_profile, &QPushButton::clicked, this, &ConfigureNetwork::BackUpProfile);
    connect(ui->restore_profile, &QPushButton::clicked, this, &ConfigureNetwork::RestoreProfile);
    this->SetConfiguration();
}

ConfigureNetwork::~ConfigureNetwork() = default;

void ConfigureNetwork::ApplyConfiguration() {
    Settings::values.network_interface = ui->network_interface->currentText().toStdString();
    Settings::values.airplane_mode = ui->airplane_mode->isChecked();
    const std::string server = ui->online_server->text().trimmed().toStdString();
    bool online = ui->online_enabled->isChecked();
    if (online && server.empty()) {
        QMessageBox::warning(this, tr("Online play"),
                             tr("Enter the server address to play online. Online play stays off "
                                "until then."));
        online = false;
        ui->online_enabled->setChecked(false);
    }
    Settings::values.online_enabled = online;
    Settings::values.online_server = server;
    Settings::values.online_server_ca = ui->online_server_ca->text().trimmed().toStdString();
}

// Editable while a game runs too: the game uses them on its next try to go online.
void ConfigureNetwork::UpdateOnlineFields() {
    const bool on = ui->online_enabled->isChecked();
    for (QWidget* w : {static_cast<QWidget*>(ui->online_server), static_cast<QWidget*>(ui->online_server_ca),
                       static_cast<QWidget*>(ui->choose_server_ca), static_cast<QWidget*>(ui->online_friends),
                       static_cast<QWidget*>(ui->new_friend), static_cast<QWidget*>(ui->add_friend),
                       static_cast<QWidget*>(ui->remove_friend), static_cast<QWidget*>(ui->other_friend_code),
                       static_cast<QWidget*>(ui->restore_profile)}) {
        w->setEnabled(on);
    }
    ui->copy_friend_code->setEnabled(on && profile.has_value());
    ui->backup_profile->setEnabled(on && profile.has_value());
    UpdateFriendButtons();
}

// The server's certificate (ca.der from its status page): games trust the server with it. A game
// reads it when it starts.
void ConfigureNetwork::ChooseServerCa() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Server certificate"), ui->online_server_ca->text(),
        tr("Certificates (*.der *.pem *.crt *.cer);;All files (*)"));
    if (!path.isEmpty()) {
        ui->online_server_ca->setText(QDir::toNativeSeparators(path));
        RefreshFriends();
    }
}

void ConfigureNetwork::ShowServerCaHint() {
    // Without a choice here: the one Droplet got from the server, else one in the data folder
    std::error_code ec;
    QString hint = tr("Droplet gets it from the server");
    if (!profile_server.empty() &&
        std::filesystem::exists(OnlineRedirect::SavedServerCa(profile_server), ec)) {
        hint = tr("got from the server");
    } else {
        for (const char* name : {"ca.der", "ca.pem"}) {
            const auto path = OnlineRedirect::OnlineDir() / name;
            if (std::filesystem::exists(path, ec)) {
                hint = tr("using %1").arg(QString::fromStdString(Common::FS::PathToUTF8String(path)));
                break;
            }
        }
    }
    ui->online_server_ca->setPlaceholderText(hint);
}

void ConfigureNetwork::LoadProfileView() {
    profile_server = ui->online_server->text().trimmed().toStdString();
    profile = profile_server.empty() ? std::nullopt : OnlineRedirect::LoadProfile(profile_server, false);
    ShowServerCaHint();
    ShowProfile();
    RefreshFriends();
}

void ConfigureNetwork::ShowProfile() {
    ui->friend_code->setText(profile ? QString::fromStdString(OnlineRedirect::FormatCode(profile->code))
                                     : tr("made when you first play online on this server"));
    ui->online_friends->clear();
    const auto add = [this](const QString& text, const std::string& code, FriendKind kind) {
        auto* item = new QListWidgetItem(text);
        item->setData(CodeRole, QString::fromStdString(code));
        item->setData(KindRole, kind);
        ui->online_friends->addItem(item);
    };
    const auto group = [this](const QString& title) {
        auto* item = new QListWidgetItem(title);
        item->setFlags(Qt::ItemIsEnabled);
        QFont font = item->font();
        font.setBold(true);
        item->setFont(font);
        ui->online_friends->addItem(item);
    };
    const auto label = [](const OnlineRedirect::FriendEntry& entry) {
        QString text = QString::fromStdString(OnlineRedirect::FormatCode(entry.code));
        if (!entry.name.empty()) {
            text += QStringLiteral("  ") + QString::fromStdString(entry.name);
        }
        return text;
    };
    if (profile && server_lists) {
        if (!server_lists->received.empty()) {
            group(tr("Friend requests"));
            for (const auto& entry : server_lists->received) {
                add(label(entry), entry.code, ReceivedRow);
            }
        }
        if (!server_lists->friends.empty()) {
            group(tr("Friends"));
            for (const auto& entry : server_lists->friends) {
                add(entry.online ? tr("%1 (online)").arg(label(entry)) : label(entry), entry.code, FriendRow);
            }
        }
        if (!server_lists->sent.empty()) {
            group(tr("Requests you sent"));
            for (const auto& entry : server_lists->sent) {
                add(label(entry), entry.code, SentRow);
            }
        }
    } else if (profile) {
        for (const std::string& code : profile->friends) {
            add(QString::fromStdString(OnlineRedirect::FormatCode(code)), code, AddedRow);
        }
    }
    UpdateOnlineFields();
}

std::string ConfigureNetwork::ServerCaPath() const {
    // The certificate for the server in the field, before OK applies either: the one chosen
    // here, the one Droplet got from that server, or one in the data folder
    const QString chosen = ui->online_server_ca->text().trimmed();
    if (!chosen.isEmpty()) {
        return chosen.toStdString();
    }
    std::error_code ec;
    for (const auto& file : {OnlineRedirect::SavedServerCa(profile_server), OnlineRedirect::OnlineDir() / "ca.der",
                             OnlineRedirect::OnlineDir() / "ca.pem"}) {
        if (std::filesystem::exists(file, ec)) {
            return Common::FS::PathToUTF8String(file);
        }
    }
    return {};
}

void ConfigureNetwork::GetServerCa() {
    const int call = ++friend_calls;
    friend_busy = true;
    ui->friends_status->setText(tr("Getting the server's certificate…"));
    ShowProfile();
    auto* watcher = new QFutureWatcher<std::string>(this);
    connect(watcher, &QFutureWatcher<std::string>::finished, this, [this, watcher, call] {
        const std::string error = watcher->result();
        watcher->deleteLater();
        if (call != friend_calls) {
            return;
        }
        friend_busy = false;
        ShowServerCaHint();
        if (error.empty()) {
            RefreshFriends();
            return;
        }
        ui->friends_status->setText(
            tr("Couldn't get the server's certificate: %1. Check the server address, or pick the "
               "certificate (ca.der, from the server's status page).")
                .arg(QString::fromStdString(error)));
        ShowProfile();
    });
    watcher->setFuture(QtConcurrent::run(
        [server = profile_server] { return OnlineRedirect::FetchServerCa(server); }));
}

void ConfigureNetwork::RefreshFriends() {
    server_lists.reset();
    // A server without its certificate here yet: get it first (games need it too)
    if (ui->online_enabled->isChecked() && !profile_server.empty() && ServerCaPath().empty()) {
        GetServerCa();
        return;
    }
    if (!ui->online_enabled->isChecked() || !profile) {
        ++friend_calls; // an answer still on its way is for something no longer shown
        friend_busy = false;
        ui->friends_status->clear();
        ShowProfile();
        return;
    }
    ui->friends_status->setText(tr("Loading your friends from the server…"));
    ShowProfile();
    RunFriendCalls({}, nullptr);
}

void ConfigureNetwork::RunFriendCalls(std::vector<std::pair<std::string, std::string>> calls,
                                      std::function<void(const FriendCallResult&)> done) {
    if (!profile) {
        return;
    }
    const int call = ++friend_calls;
    friend_busy = true;
    UpdateFriendButtons();
    // Each (action, code) in turn, then the lists as they are after them
    const auto work = [p = *profile, ca = ServerCaPath(), calls = std::move(calls)] {
        FriendCallResult result;
        for (const auto& [action, code] : calls) {
            const auto answer = OnlineRedirect::FriendAction(p, action, code, ca);
            if (!answer.reached) {
                result.lists = answer;
                return result;
            }
            if (answer.error.empty()) {
                result.done.push_back(code);
            } else {
                result.errors += QString::fromStdString(OnlineRedirect::FormatCode(code) + ": " +
                                                        answer.error) + QLatin1Char('\n');
            }
        }
        result.lists = OnlineRedirect::FetchFriends(p, ca);
        return result;
    };
    auto* watcher = new QFutureWatcher<FriendCallResult>(this);
    connect(watcher, &QFutureWatcher<FriendCallResult>::finished, this, [this, watcher, call, done] {
        const FriendCallResult result = watcher->result();
        watcher->deleteLater();
        if (call != friend_calls) {
            return;
        }
        friend_busy = false;
        if (done) {
            done(result);
        }
        UseServerLists(result.lists);
        if (!result.errors.isEmpty()) {
            QMessageBox::warning(this, tr("Friends"), result.errors.trimmed());
        }
    });
    watcher->setFuture(QtConcurrent::run(work));
}

void ConfigureNetwork::UseServerLists(const OnlineRedirect::FriendLists& lists) {
    if (lists.reached && lists.error.empty()) {
        server_lists = lists;
        // Sign-ins send the codes this emulator added: they include the server's friends and the
        // requests sent, also those made on another PC
        bool changed = false;
        for (const auto* group : {&lists.friends, &lists.sent}) {
            for (const auto& entry : *group) {
                if (profile && std::find(profile->friends.begin(), profile->friends.end(), entry.code) ==
                                   profile->friends.end()) {
                    OnlineRedirect::AddFriend(*profile, entry.code);
                    changed = true;
                }
            }
        }
        if (changed) {
            OnlineRedirect::SaveProfile(*profile);
        }
        ui->friends_status->setText(
            lists.received.empty()
                ? (lists.friends.empty() && lists.sent.empty()
                       ? tr("No friends yet: add a friend's code to send them a request.")
                       : QString())
                : lists.received.size() == 1
                      ? tr("A friend request for you: select it to accept or decline it.")
                      : tr("%1 friend requests for you: select one to accept or decline it.")
                            .arg(lists.received.size()));
    } else {
        server_lists.reset();
        ui->friends_status->setText(
            lists.reached ? QString::fromStdString(lists.error)
                          : tr("Friend requests need the server (%1). Codes added here are sent when "
                               "you next play online.")
                                .arg(QString::fromStdString(lists.error)));
    }
    ShowProfile();
}

void ConfigureNetwork::UpdateFriendButtons() {
    const bool on = ui->online_enabled->isChecked() && !friend_busy;
    bool received = false;
    bool removable = false;
    for (const QListWidgetItem* item : ui->online_friends->selectedItems()) {
        const int kind = item->data(KindRole).toInt();
        received |= kind == ReceivedRow;
        removable |= kind == FriendRow || kind == SentRow || kind == AddedRow;
    }
    ui->accept_friend->setEnabled(on && received);
    ui->decline_friend->setEnabled(on && received);
    ui->remove_friend->setEnabled(on && removable);
    ui->add_friend->setEnabled(on);
}

void ConfigureNetwork::AnswerRequests(const std::string& action) {
    std::vector<std::pair<std::string, std::string>> calls;
    for (const QListWidgetItem* item : ui->online_friends->selectedItems()) {
        if (item->data(KindRole).toInt() == ReceivedRow) {
            calls.emplace_back(action, item->data(CodeRole).toString().toStdString());
        }
    }
    if (calls.empty()) {
        return;
    }
    RunFriendCalls(std::move(calls), [this, action](const FriendCallResult& result) {
        if (action == "accept" && !result.done.empty()) {
            EditProfile([&](OnlineRedirect::Profile& p) {
                for (const std::string& code : result.done) {
                    OnlineRedirect::AddFriend(p, code);
                }
            });
        }
    });
}

void ConfigureNetwork::RemoveFriends() {
    std::vector<std::pair<std::string, std::string>> calls;
    std::vector<std::string> added;
    bool friends = false;
    for (const QListWidgetItem* item : ui->online_friends->selectedItems()) {
        const std::string code = item->data(CodeRole).toString().toStdString();
        switch (item->data(KindRole).toInt()) {
        case FriendRow:
            calls.emplace_back("remove", code);
            friends = true;
            break;
        case SentRow:
            calls.emplace_back("cancel", code);
            break;
        case AddedRow:
            added.push_back(code);
            break;
        default:
            break;
        }
    }
    if (friends && QMessageBox::question(this, tr("Remove friends"),
                                         tr("Remove the selected friends? To be friends again, one of "
                                            "you sends a new friend request.")) != QMessageBox::Yes) {
        return;
    }
    if (!added.empty()) {
        EditProfile([&](OnlineRedirect::Profile& p) {
            for (const std::string& code : added) {
                OnlineRedirect::RemoveFriend(p, code);
            }
        });
    }
    if (!calls.empty()) {
        RunFriendCalls(std::move(calls), [this](const FriendCallResult& result) {
            if (!result.done.empty()) {
                EditProfile([&](OnlineRedirect::Profile& p) {
                    for (const std::string& code : result.done) {
                        OnlineRedirect::RemoveFriend(p, code);
                    }
                });
            }
        });
    }
}

// Saved at once, Cancel or not: the server only reads them at the next sign-in.
void ConfigureNetwork::EditProfile(const std::function<void(OnlineRedirect::Profile&)>& edit) {
    if (profile_server.empty()) {
        QMessageBox::warning(this, tr("Online play"), tr("Enter the server address first."));
        return;
    }
    if (!profile) {
        profile = OnlineRedirect::LoadProfile(profile_server, true);
    }
    if (!profile) {
        return;
    }
    edit(*profile);
    if (!OnlineRedirect::SaveProfile(*profile)) {
        QMessageBox::warning(this, tr("Online play"),
                             tr("Could not save your friend list in %1.")
                                 .arg(QString::fromStdString(OnlineRedirect::ProfilesDir().string())));
    }
    ShowProfile();
}

void ConfigureNetwork::AddFriends() {
    const auto codes = OnlineRedirect::ParseCodes(ui->new_friend->text().toStdString());
    if (codes.empty()) {
        QMessageBox::warning(this, tr("Add a friend"),
                             tr("A friend code has 12 digits, like SW-1234-5678-9012."));
        return;
    }
    ui->new_friend->clear();
    if (server_lists && profile) {
        std::vector<std::pair<std::string, std::string>> calls;
        for (const std::string& code : codes) {
            calls.emplace_back("request", code);
        }
        RunFriendCalls(std::move(calls), [this](const FriendCallResult& result) {
            if (!result.done.empty()) {
                EditProfile([&](OnlineRedirect::Profile& p) {
                    for (const std::string& code : result.done) {
                        OnlineRedirect::AddFriend(p, code);
                    }
                });
            }
        });
        return;
    }
    // Without the server's lists (out of reach, or an older server): the next sign-in sends the
    // codes, and there they are friend requests
    EditProfile([&](OnlineRedirect::Profile& p) {
        for (const std::string& code : codes) {
            OnlineRedirect::AddFriend(p, code);
        }
    });
}

void ConfigureNetwork::ChangeCode() {
    bool ok = false;
    const QString text = QInputDialog::getText(
        this, tr("Change your friend code"),
        tr("Choose any 12 digits, like SW-1234-5678-9012. A code nobody has on this server becomes "
           "yours the next time you play online, and your friends come along. If another player "
           "has it, the game shows an error when you go online: choose another.\n\nA code you had "
           "here before comes back as well (on a new emulator, once the server's admin released "
           "it for you). To move your code to another PC, back it up here and restore it there."),
        QLineEdit::Normal, QString(), &ok);
    if (!ok) {
        return;
    }
    const auto codes = OnlineRedirect::ParseCodes(text.toStdString());
    if (codes.size() != 1) {
        QMessageBox::warning(this, tr("Change your friend code"),
                             tr("A friend code has 12 digits, like SW-1234-5678-9012."));
        return;
    }
    if (profile && profile->code == codes.front()) {
        return;
    }
    // The secret stays: with it the server knows the new code is this emulator's, moves its
    // friends there and lets it switch back.
    EditProfile([&](OnlineRedirect::Profile& p) {
        p.code = codes.front();
        std::erase(p.friends, p.code);
    });
    RefreshFriends();
}

void ConfigureNetwork::BackUpProfile() {
    if (!profile) {
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Back up your friend code"),
        tr("Droplet friend code %1.txt").arg(QString::fromStdString(OnlineRedirect::FormatCode(profile->code))),
        tr("Text files (*.txt)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
        file.write(QByteArray::fromStdString(OnlineRedirect::ProfileText(*profile))) < 0) {
        QMessageBox::warning(this, tr("Back up your friend code"), tr("Could not write %1.").arg(path));
        return;
    }
    file.close();
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    QMessageBox::information(this, tr("Back up your friend code"),
                             tr("Saved. Keep this file private: whoever has it can sign in as you on "
                                "this server."));
}

void ConfigureNetwork::RestoreProfile() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Restore your friend code"), QString(),
                                                      tr("Text files (*.txt);;All files (*)"));
    if (path.isEmpty()) {
        return;
    }
    QFile file(path);
    std::optional<OnlineRedirect::Profile> restored;
    if (file.open(QIODevice::ReadOnly) && file.size() < 64 * 1024) {
        restored = OnlineRedirect::ParseProfile(file.readAll().toStdString());
    }
    if (!restored) {
        QMessageBox::warning(this, tr("Restore your friend code"),
                             tr("%1 is not a friend code backup.").arg(path));
        return;
    }
    if (!restored->server.empty() && restored->server != profile_server &&
        QMessageBox::question(this, tr("Restore your friend code"),
                              tr("This backup is from the server %1. Use it for %2?")
                                  .arg(QString::fromStdString(restored->server),
                                       QString::fromStdString(profile_server))) != QMessageBox::Yes) {
        return;
    }
    EditProfile([&](OnlineRedirect::Profile& p) {
        p = *restored;
        p.server = profile_server;
    });
    RefreshFriends();
}

void ConfigureNetwork::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureNetwork::RetranslateUI() {
    ui->retranslateUi(this);
    ShowProfile();
}

void ConfigureNetwork::SetConfiguration() {
    const bool runtime_lock = !system.IsPoweredOn();
    auto const network_interface = Settings::values.network_interface.GetValue();
    auto const airplane_mode = Settings::values.airplane_mode.GetValue();
    ui->network_interface->setCurrentText(QString::fromStdString(network_interface));
    ui->network_interface->setEnabled(runtime_lock);
    ui->airplane_mode->setChecked(airplane_mode);
    ui->online_server->setText(QString::fromStdString(Settings::values.online_server.GetValue()));
    ui->online_server_ca->setText(QString::fromStdString(Settings::values.online_server_ca.GetValue()));
    ShowServerCaHint();
    ui->online_enabled->setChecked(Settings::values.online_enabled.GetValue() && !airplane_mode);
    LoadProfileView();
}
