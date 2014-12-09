#include "SolderInstance.h"

#include "logic/minecraft/InstanceVersion.h"
#include "logic/tasks/SequentialTask.h"
#include "logic/OneSixUpdate.h"
#include "MultiMC.h"
#include <pathutils.h>
#include <JlCompress.h>

struct ModEntry
{
	QString name;
	QString version;
	QString url;
	QString md5;
	QString mcVersion;
	QString filename()
	{
		return name + "-" + version + ".jar";
	}
	QString cacheFile()
	{
		return PathCombine(QLatin1Literal("technic_dl"), mcVersion, filename());
	}
	QString path()
	{
		return PathCombine(MMC->metacache()->getBasePath("cache"), QLatin1Literal("technic_dl"), mcVersion);
	}
	QString filePath()
	{
		return PathCombine(path(), filename());
	}
};

class SolderUpdate : public Task
{
	Q_OBJECT
public:
	explicit SolderUpdate(SolderInstance *inst, QObject *parent = 0)
		: Task(parent), m_inst(inst){};
	virtual void executeTask();

private slots:
	void packVersionStart();
	void packVersionFinished();
	void packVersionFailed();

	void packStart();
	void packFinished();
	void packFailed();
	void versionProgress(int, qint64, qint64);
	void packProgress(qint64,qint64);

private:
	NetActionPtr packVersionDownload;
	NetJobPtr packDownload;

	SolderInstance *m_inst;
	QList <ModEntry> m_mods;
};

void SolderUpdate::executeTask()
{
	auto status = m_inst->settings().get("packStatus").toString();
	if(status == "NotInstalled")
	{
		packVersionStart();
	}
	else
	{
		emitSucceeded();
	}
}

void SolderUpdate::versionProgress(int, qint64 min, qint64 max)
{
	double part = double(min)/double(max);
	setProgress(part * 5);
}

void SolderUpdate::packProgress(qint64 min, qint64 max)
{
	double part = double(min)/double(max);
	setProgress(5.0 + part * 95.0);
}

void SolderUpdate::packVersionStart()
{
	setStatus(tr("Downloading version information"));
	auto version = m_inst->solderVersion();

	auto temp = ByteArrayDownload::make(QUrl(version->url()));
	packVersionDownload = temp;
	connect(packVersionDownload.get(), SIGNAL(succeeded(int)), SLOT(packVersionFinished()));
	connect(packVersionDownload.get(), SIGNAL(failed(int)), SLOT(packVersionFailed()));
	connect(packVersionDownload.get(), SIGNAL(progress(int, qint64, qint64)),
			SLOT(versionProgress(int, qint64, qint64)));
	packVersionDownload->start();
}

void SolderUpdate::packVersionFinished()
{
	ByteArrayDownloadPtr packMeta = std::dynamic_pointer_cast<ByteArrayDownload> (packVersionDownload);
	QString solderFilePath = PathCombine(m_inst->instanceRoot(), "solder.json");
	QFile file(solderFilePath);
	file.open(QIODevice::WriteOnly);
	file.write(packMeta->m_data);
	file.close();
	packStart();
}

void SolderUpdate::packVersionFailed()
{
	emitFailed(tr("Couldn't get pack version..."));
}

void SolderUpdate::packStart()
{
	QString solderFilePath = PathCombine(m_inst->instanceRoot(), "solder.json");
	QFile file(solderFilePath);
	file.open(QIODevice::ReadOnly);
	QJsonDocument dox = QJsonDocument::fromJson(file.readAll());
	file.close();
	auto obj = dox.object();
	auto mcVersion = obj.value("minecraft").toString();
	// FIXME: bullshit.
	m_inst->setIntendedVersionId(mcVersion);
	auto mods = obj.value("mods").toArray();
	for(auto mod: mods)
	{
		auto modObj = mod.toObject();
		ModEntry e;
		e.name = modObj.value("name").toString();
		e.version = modObj.value("version").toString();
		e.url = modObj.value("url").toString();
		e.md5 = modObj.value("md5").toString();
		e.mcVersion = mcVersion;
		m_mods.append(e);
	}
	QLOG_INFO() << "Using minecraft" << mcVersion;

	setStatus(tr("Downloading pack data"));
	packDownload = std::make_shared<NetJob>("Solder pack packages");
	for(auto & mod: m_mods)
	{
		auto entry = MMC->metacache()->resolveEntry("cache", mod.cacheFile());
		auto dl = CacheDownload::make(QUrl(mod.url), entry);
		packDownload->addNetAction(dl);
	}
	connect(packDownload.get(), SIGNAL(succeeded()), SLOT(packFinished()));
	connect(packDownload.get(), SIGNAL(failed()), SLOT(packFailed()));
	connect(packDownload.get(), SIGNAL(progress(qint64,qint64)), SLOT(packProgress(qint64,qint64)));
	packDownload->start();
}

void SolderUpdate::packFinished()
{
	setStatus(tr("Extracting packages"));
	for(auto & mod: m_mods)
	{
		auto filename = mod.filePath();
		auto files = JlCompress::extractDir(filename, m_inst->minecraftRoot());
		QLOG_INFO() << "Extracted" << filename << files.join(", ");
	}
	auto status = m_inst->settings().set("packStatus", QString("Extracted"));
	emitSucceeded();
}

void SolderUpdate::packFailed()
{
	emitFailed(tr("Couldn't get pack data..."));
}

SolderInstance::SolderInstance(const QString &rootDir, SettingsObject *settings,
							   QObject *parent)
	: OneSixInstance(rootDir, settings, parent)
{
	settings->registerSetting("solderPack", "");
	settings->registerSetting("packStatus", "NotInstalled");
}

//FIXME: this is bullshit. we will need to convert instance configs to JSON at some point
void SolderInstance::setSolderVersion(SolderVersionPtr url)
{
	m_solderVersion = url;
	QJsonDocument doc(url->toJson());
	settings().set("solderPack", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

//FIXME: this is bullshit. we will need to convert instance configs to JSON at some point
SolderVersionPtr SolderInstance::solderVersion()
{
	if(m_solderVersion)
		return m_solderVersion;
	QString packed = settings().get("solderPack").toString();
	auto doc = QJsonDocument::fromJson(packed.toUtf8());
	return SolderVersion::fromJson(doc.object());
}

std::shared_ptr<Task> SolderInstance::doUpdate()
{
	auto task = std::make_shared<SequentialTask>(this);
	task->addTask(std::make_shared<SolderUpdate>(this));
	task->addTask(std::make_shared<OneSixUpdate>(this));
	return task;
}

#include "SolderInstance.moc"
