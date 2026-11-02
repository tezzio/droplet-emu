// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include "common/settings.h"
#include "core/core.h"
#include "core/hle/service/sockets/online_redirect.h"
#include "core/internal_network/network_interface.h"
#include "ui_configure_network.h"
#include "yuzu/configuration/configure_network.h"

namespace OnlineRedirect = Service::Sockets::OnlineRedirect;

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
    connect(ui->copy_friend_code, &QPushButton::clicked, this, [this] {
        QGuiApplication::clipboard()->setText(ui->friend_code->text());
    });
    connect(ui->other_friend_code, &QPushButton::clicked, this, &ConfigureNetwork::ChangeCode);
    connect(ui->add_friend, &QPushButton::clicked, this, &ConfigureNetwork::AddFriends);
    connect(ui->new_friend, &QLineEdit::returnPressed, this, &ConfigureNetwork::AddFriends);
    connect(ui->remove_friend, &QPushButton::clicked, this, [this] {
        std::vector<std::string> codes;
        for (const QListWidgetItem* item : ui->online_friends->selectedItems()) {
            codes.push_back(item->data(Qt::UserRole).toString().toStdString());
        }
        if (!codes.empty()) {
            EditProfile([&](OnlineRedirect::Profile& p) {
                for (const std::string& code : codes) {
                    OnlineRedirect::RemoveFriend(p, code);
                }
            });
        }
    });
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
}

// Editable while a game runs too: the game uses them on its next try to go online.
void ConfigureNetwork::UpdateOnlineFields() {
    const bool on = ui->online_enabled->isChecked();
    for (QWidget* w : {static_cast<QWidget*>(ui->online_server), static_cast<QWidget*>(ui->online_friends),
                       static_cast<QWidget*>(ui->new_friend), static_cast<QWidget*>(ui->add_friend),
                       static_cast<QWidget*>(ui->remove_friend), static_cast<QWidget*>(ui->other_friend_code),
                       static_cast<QWidget*>(ui->restore_profile)}) {
        w->setEnabled(on);
    }
    ui->copy_friend_code->setEnabled(on && profile.has_value());
    ui->backup_profile->setEnabled(on && profile.has_value());
}

void ConfigureNetwork::LoadProfileView() {
    profile_server = ui->online_server->text().trimmed().toStdString();
    profile = profile_server.empty() ? std::nullopt : OnlineRedirect::LoadProfile(profile_server, false);
    ShowProfile();
}

void ConfigureNetwork::ShowProfile() {
    ui->friend_code->setText(profile ? QString::fromStdString(OnlineRedirect::FormatCode(profile->code))
                                     : tr("made when you first play online on this server"));
    ui->online_friends->clear();
    if (profile) {
        for (const std::string& code : profile->friends) {
            auto* item = new QListWidgetItem(QString::fromStdString(OnlineRedirect::FormatCode(code)));
            item->setData(Qt::UserRole, QString::fromStdString(code));
            ui->online_friends->addItem(item);
        }
    }
    UpdateOnlineFields();
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
    EditProfile([&](OnlineRedirect::Profile& p) {
        for (const std::string& code : codes) {
            OnlineRedirect::AddFriend(p, code);
        }
    });
    ui->new_friend->clear();
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
    ui->online_enabled->setChecked(Settings::values.online_enabled.GetValue() && !airplane_mode);
    LoadProfileView();
}
