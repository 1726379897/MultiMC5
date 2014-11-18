#pragma once

#include <QObject>
#include <QHash>
#include <memory>
#include "QuickModVersionRef.h"

class SettingsObject;
class MultiMC;
class QuickModModel;
class QTimer;
class QuickModMetadata;
class BaseQuickModVersion;
class OneSixInstance;
class QuickModDownloadAction;
class QuickModBaseDownloadAction;
class QuickModIndexModel;

typedef std::shared_ptr<QuickModMetadata> QuickModMetadataPtr;
typedef std::shared_ptr<BaseQuickModVersion> QuickModVersionPtr;

class QuickModDatabase : public QObject
{
	friend class QuickModModel;
	friend class QuickModBaseDownloadAction;
	friend class QuickModDownloadAction;
	friend class MultiMC;
	Q_OBJECT

private: /* methods */
	/// constructor. see MultiMC.cpp
	QuickModDatabase();
	
	/// add a separately constructed mod
	void addMod(QuickModMetadataPtr mod);

	/// add a separately constructed version
	void addVersion(QuickModVersionPtr version);

	/**
	 * retrieve ETag from last download of url.
	 * FIXME: use metacache instead.
	 */
	QByteArray lastETagForURL(const QUrl &url) const;

	/**
	 * set ETag for last download of url
	 * FIXME: use metacache instead.
	 */
	void setLastETagForURL(const QUrl &url, const QByteArray &checksum);

public: /* methods */
	/// get the current list of installed mod UIDs
	QList<QuickModRef> getPackageUIDs() const;

	/// same as above, in light blue
	QList<QuickModMetadataPtr> allModMetadata(const QuickModRef &uid) const;

	/// same as above, but only return the first entry found
	QuickModMetadataPtr someModMetadata(const QuickModRef &uid) const;

	/// get a version by version ref
	QuickModVersionPtr version(const QuickModVersionRef &version) const;

	/// get a version by uid, version and repo
	QuickModVersionPtr version(const QString &uid, const QString &version, const QString &repo) const;

	/// get the latest version by mod ref and minecraft version.
	QuickModVersionRef latestVersionForMinecraft(const QuickModRef &modUid, const QString &mcVersion) const;

	/// get a list of minecraft versions supported by the specified mod
	QStringList minecraftVersions(const QuickModRef &uid) const;

	/// get ALL version of a mod for a particular minecraft version
	QList<QuickModVersionRef> versions(const QuickModRef &uid, const QString &mcVersion) const;

	/// do we have the specified mod (optionally also specific metadata)?
	bool haveUid(const QuickModRef &uid, const QString &repo = QString()) const;

	/// given an instance, return mods that have updates available
	QList<QuickModRef> updatedModsForInstance(std::shared_ptr<OneSixInstance> instance) const;

	/// returns the pretties possible string for displaying to the user
	QString userFacingUid(const QString &uid) const;

	/// download and insert a quickmod file or index into the database
	void registerMod(const QString &fileName);
	void registerMod(const QUrl &url);

	/// update quickmod files
	void updateFiles();

public: // repo and index related
	/// get all remembered indices
	QHash<QString, QUrl> indices() const { return m_indices; }

	void addRepo(const QString &name, const QUrl &indexUrl);
	void removeRepo(const QString &name);

signals:
	void aboutToReset();
	void reset();
	void justAddedMod(QuickModRef mod);
	void modIconUpdated(QuickModRef mod);
	void modLogoUpdated(QuickModRef mod);

private slots:
	void delayedFlushToDisk();
	void flushToDisk();
	void loadFromDisk();

private: /* data */
	//    uid            repo     data
	QHash<QuickModRef, QHash<QString, QuickModMetadataPtr>> m_metadata;
	//    uid            version  data
	QHash<QuickModRef, QHash<QString, QuickModVersionPtr>> m_versions;

	// FIXME: use metacache.
	//    url   checksum
	QHash<QUrl, QByteArray> m_etags;

	//    repo     url
	QHash<QString, QUrl> m_indices;

	bool m_isDirty = false;
	std::unique_ptr<QTimer> m_timer;

	QString m_dbFile;
	QString m_configFile;
	
	// FIXME: get rid of this.
	std::shared_ptr<SettingsObject> m_settings;
};
