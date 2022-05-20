#include "config.h"

#include "util.h"

#include "inclib/vdf_parser.hpp"

#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

#include <QJsonDocument>
#include <QJsonObject>

#include <QDebug>

QString MultiBound::Config::configPath;
QString MultiBound::Config::instanceRoot;
QString MultiBound::Config::workshopRoot;
QString MultiBound::Config::steamcmdDLRoot;
QString MultiBound::Config::steamcmdWorkshopRoot;
QString MultiBound::Config::starboundPath;
QString MultiBound::Config::starboundRoot;

QString MultiBound::Config::workshopDecryptionKey;

bool MultiBound::Config::steamcmdEnabled = true;
bool MultiBound::Config::steamcmdUpdateSteamMods = true;

namespace vdf = tyti::vdf;

void MultiBound::Config::load() {
    // defaults
    configPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation); // already has "multibound" attached
#if defined(Q_OS_WIN)
    //; // same place as the exe on windows, like mb1
    if (QDir(QCoreApplication::applicationDirPath()).exists("config.json"))
        configPath = QCoreApplication::applicationDirPath();
    starboundPath = QDir::cleanPath(qs("C:/Program Files (x86)/Steam/SteamApps/common/Starbound/win64/starbound.exe"));
#elif defined(Q_OS_MACOS)
    QDir home(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    starboundPath = Util::splicePath(home, "/Library/Application Support/Steam/steamapps/common/Starbound/osx/Starbound.app/Contents/MacOS/starbound");
#else
    QDir home(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    starboundPath = Util::splicePath(home, "/.steam/steam/steamapps/common/Starbound/linux/run-client.sh");
#endif
    if (auto d = QDir(configPath); !d.exists()) d.mkpath(".");
    instanceRoot = Util::splicePath(configPath, "/instances/");

    steamcmdDLRoot = configPath;

    QJsonObject cfg;
    if (QFile f(Util::splicePath(configPath, "/config.json")); f.exists()) {
        f.open(QFile::ReadOnly);
        cfg = QJsonDocument::fromJson(f.readAll()).object();

        starboundPath = cfg["starboundPath"].toString(starboundPath);
        instanceRoot = cfg["instanceRoot"].toString(instanceRoot);
        steamcmdDLRoot = cfg["steamcmdRoot"].toString(steamcmdDLRoot);

        workshopDecryptionKey = cfg["workshopDecryptionKey"].toString(workshopDecryptionKey);

        steamcmdUpdateSteamMods = cfg["steamcmdUpdateSteamMods"].toBool(steamcmdUpdateSteamMods);
    }

    if (auto d = QDir(instanceRoot); !d.exists()) d.mkpath(".");

    QDir r(starboundPath);
    while (r.dirName().toLower() != "starbound" || !r.exists()) { auto old = r; r.cdUp(); if (r == old) break; }
    starboundRoot = QDir::cleanPath(r.absolutePath());

    while (r.dirName().toLower() != "steamapps") { auto old = r; r.cdUp(); if (r == old) break; }
    if (auto rr = r.absolutePath(); rr.length() >= 10) workshopRoot = Util::splicePath(rr, "/workshop/content/211820/");

    steamcmdWorkshopRoot = Util::splicePath(QDir(steamcmdDLRoot).absolutePath(), "/steamapps/workshop/content/211820/");

    if (workshopDecryptionKey.isEmpty()) { // attempt to pull key from Steam
        QString vdfLoc;
        // just default locations for now
        // TODO: figure out mac
        // TODO: go by registry on Windows
#if defined(Q_OS_WIN)
        vdfLoc = QDir::cleanPath(qs("C:/Program Files (x86)/Steam/config/config.vdf"));
#else
        vdfLoc = Util::splicePath(home, "/.steam/steam/config/config.vdf");
#endif

        if (QFile f(vdfLoc); f.exists()) {
            std::ifstream s(vdfLoc.toStdString());
            auto doc = vdf::read(s);
            auto depot = Util::vdfPath(&doc, QStringList() << "Software" << "Valve" << "Steam" << "depots" << "211820", true);
            auto cc = depot->attribs["DecryptionKey"];
            workshopDecryptionKey = QString::fromStdString(cc);
            if (!workshopDecryptionKey.isEmpty()) save();
        }
    }
}

void MultiBound::Config::save() {
    QJsonObject cfg;
    cfg["starboundPath"] = starboundPath;
    cfg["instanceRoot"] = instanceRoot;
    cfg["steamcmdRoot"] = steamcmdDLRoot;

    cfg["workshopDecryptionKey"] = workshopDecryptionKey;

    cfg["steamcmdUpdateSteamMods"] = steamcmdUpdateSteamMods;

    QFile f(Util::splicePath(configPath, "/config.json"));
    f.open(QFile::WriteOnly);
    f.write(QJsonDocument(cfg).toJson());
}
