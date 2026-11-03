// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt_common/util/content.h"
#include "qt_common/util/game.h"

#include "common/fs/fs.h"
#include "core/hle/service/acc/profile_manager.h"
#include "frontend_common/content_manager.h"
#include "frontend_common/data_manager.h"
#include "frontend_common/firmware_manager.h"

#include "compress.h"
#include "qt_common/abstract/frontend.h"
#include "qt_common/abstract/progress.h"
#include "qt_common/qt_common.h"

#include <JlCompress.h>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentRun>

namespace QtCommon::Content {

// Content //
void VerifyGameContents(const std::string& game_path) {
    auto progress =
        QtCommon::Frontend::newProgressDialog(tr("Verifying integrity..."), tr("Cancel"), 0, 100);
    progress->show();

    QGuiApplication::processEvents();

    const auto callback = [&](size_t total_size, size_t processed_size) {
        QGuiApplication::processEvents();
        progress->setValue(static_cast<int>((processed_size * 100) / total_size));
        return progress->wasCanceled();
    };

    const auto result = ContentManager::VerifyGameContents(*system, game_path, callback);

    switch (result) {
    case ContentManager::GameVerificationResult::Success:
        QtCommon::Frontend::Information(rootObject, tr("Integrity verification succeeded!"),
                                        tr("The operation completed successfully."));
        break;
    case ContentManager::GameVerificationResult::Failed:
        QtCommon::Frontend::Critical(rootObject, tr("Integrity verification failed!"),
                                     tr("File contents may be corrupt or missing."));
        break;
    case ContentManager::GameVerificationResult::NotImplemented:
        QtCommon::Frontend::Warning(
            rootObject, tr("Integrity verification couldn't be performed"),
            tr("File contents could not be checked for validity."));
    }
}

void FixProfiles() {
    // Reset user save files after config is initialized and migration is done.
    // Doing it at init time causes profiles to read from the wrong place entirely if NAND dir is
    // not default
    // TODO: better solution
    system->GetProfileManager().ResetUserSaveFile();
    std::vector<std::string> orphaned = system->GetProfileManager().FindOrphanedProfiles();
    std::vector<std::string> good = system->GetProfileManager().FindGoodProfiles();

    // no orphaned dirs--all good :)
    if (orphaned.empty())
        return;

    // otherwise, let the user know
    QString qorphaned;

    // max. of 8 orphaned profiles is fair, I think
    // 36 = 32 (UUID) + 4 (<br>)
    qorphaned.reserve(8 * 36);

    for (const std::string& s : orphaned) {
        qorphaned = qorphaned % QStringLiteral("<br>") % QString::fromStdString(s);
    }

    QString qgood;

    // max. of 8 good profiles is fair, I think
    // 36 = 32 (UUID) + 4 (<br>)
    qgood.reserve(8 * 36);

    for (const std::string& s : good) {
        qgood = qgood % QStringLiteral("<br>") % QString::fromStdString(s);
    }

    QtCommon::Frontend::Critical(
        tr("Orphaned Profiles Detected!"),
        tr("UNEXPECTED BAD THINGS MAY HAPPEN IF YOU DON'T READ THIS!<br>"
           "Droplet has detected the following save directories with no attached profile:<br>"
           "%1<br><br>"
           "The following profiles are valid:<br>"
           "%2<br><br>"
           "Click \"OK\" to open your save folder and fix up your profiles.<br>"
           "Hint: copy the contents of the largest or last-modified folder elsewhere, "
           "delete all orphaned profiles, and move your copied contents to the good "
           "profile.<br><br>"
           "Still confused? See the <a "
           "href='https://gitlab.com/seighttt/droplet/-/blob/main/docs/user/Orphaned.md'>"
           "help page</a>.<br>")
            .arg(qorphaned, qgood));

    QtCommon::Game::OpenSaveFolder();
}

void ClearDataDir(FrontendCommon::DataManager::DataDir dir, const std::string& user_id) {
    using namespace QtCommon::Frontend;
    auto result = Warning(tr("Really clear data?"), tr("Important data may be lost!"), Yes | No);

    if (result != Yes)
        return;

    result = Warning(tr("Are you REALLY sure?"),
                     tr("Once deleted, your data will NOT come back!\n"
                        "Only do this if you're 100% sure you want to delete this data."),
                     Yes | No);

    if (result != Yes)
        return;

    auto dialog = newProgressDialog(tr("Clearing..."), QString(), 0, 0);
    dialog->show();

    FrontendCommon::DataManager::ClearDir(dir, user_id);

    dialog->close();
}

void ExportDataDir(FrontendCommon::DataManager::DataDir data_dir, const std::string& user_id,
                   const QString& name, std::function<void()> callback) {
    using namespace QtCommon::Frontend;
    const std::string dir = FrontendCommon::DataManager::GetDataDirString(data_dir, user_id);

    const QString zip_dump_location = GetSaveFileName(
        tr("Select Export Location"), tr("%1.zip").arg(name), tr("Zipped Archives (*.zip)"));

    if (zip_dump_location.isEmpty())
        return;

    auto progress = QtCommon::Frontend::newProgressDialogPtr(
        tr("Exporting data. This may take a while..."), tr("Cancel"), 0, 100);

    progress->setTitle(tr("Exporting"));
    progress->show();

    auto progress_callback = [=](size_t total_size, size_t processed_size) {
        QMetaObject::invokeMethod(
            progress, "setValue", Qt::DirectConnection,
            Q_ARG(int, static_cast<int>((processed_size * 100) / total_size)));
        return !progress->wasCanceled();
    };

    QFuture<bool> future = QtConcurrent::run([=]() {
        return QtCommon::Compress::compressDir(zip_dump_location, QString::fromStdString(dir),
                                               QtCommon::Compress::Options(), progress_callback);
    });

    QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(rootObject);

    QObject::connect(watcher, &QFutureWatcher<bool>::finished, rootObject, [=]() {
        progress->close();

        if (watcher->result()) {
            Information(tr("Exported Successfully"), tr("Data was exported successfully."));
        } else if (progress->wasCanceled()) {
            Information(tr("Export Cancelled"), tr("Export was cancelled by the user."));
        } else {
            Critical(
                tr("Export Failed"),
                tr("Ensure you have write permissions on the targeted directory and try again."));
        }

        progress->deleteLater();
        watcher->deleteLater();
        if (callback)
            callback();
    });

    watcher->setFuture(future);
}

void ImportDataDir(FrontendCommon::DataManager::DataDir data_dir, const std::string& user_id,
                   std::function<void()> callback) {
    const std::string dir = FrontendCommon::DataManager::GetDataDirString(data_dir, user_id);

    using namespace QtCommon::Frontend;

    const QString zip_dump_location =
        GetOpenFileName(tr("Select Import Location"), {}, tr("Zipped Archives (*.zip)"));

    if (zip_dump_location.isEmpty())
        return;

    StandardButton button =
        Warning(tr("Import Warning"),
                tr("All previous data in this directory will be deleted. Are you sure you wish to "
                   "proceed?"),
                StandardButton::Yes | StandardButton::No);

    if (button != QtCommon::Frontend::Yes)
        return;

    QtProgressDialog* progress =
        newProgressDialogPtr(tr("Importing data. This may take a while..."), tr("Cancel"), 0, 100);

    progress->setTitle(tr("Importing"));
    progress->show();

    // to prevent GUI mangling we have to run this in a thread as well
    QFuture<bool> delete_future = QtConcurrent::run([=]() {
        FrontendCommon::DataManager::ClearDir(data_dir, user_id);
        return !progress->wasCanceled();
    });

    QFutureWatcher<bool>* delete_watcher = new QFutureWatcher<bool>(rootObject);
    delete_watcher->setFuture(delete_future);

    QObject::connect(delete_watcher, &QFutureWatcher<bool>::finished, rootObject, [=]() {
        auto progress_callback = [=](size_t total_size, size_t processed_size) {
            QMetaObject::invokeMethod(
                progress, "setValue", Qt::DirectConnection,
                Q_ARG(int, static_cast<int>((processed_size * 100) / total_size)));
            return !progress->wasCanceled();
        };

        QFuture<bool> future = QtConcurrent::run([=]() {
            return !QtCommon::Compress::extractDir(zip_dump_location, QString::fromStdString(dir),
                                                   progress_callback)
                        .empty();
        });

        QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>(rootObject);

        QObject::connect(watcher, &QFutureWatcher<bool>::finished, rootObject, [=]() {
            progress->close();

            // this sucks
            if (watcher->result()) {
                Information(tr("Imported Successfully"), tr("Data was imported successfully."));
            } else if (progress->wasCanceled()) {
                Information(tr("Import Cancelled"), tr("Import was cancelled by the user."));
            } else {
                Critical(tr("Import Failed"),
                         tr("Ensure you have read permissions on the targeted directory and try "
                            "again."));
            }

            progress->deleteLater();
            watcher->deleteLater();
            if (callback)
                callback();
        });

        watcher->setFuture(future);
    });
}

} // namespace QtCommon::Content
