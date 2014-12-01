#include "SolderInstance.h"

#include "logic/minecraft/InstanceVersion.h"
#include "logic/tasks/SequentialTask.h"
#include "OneSixUpdate.h"
#include "MultiMC.h"

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

private:
	NetActionPtr packVersionDownload;
	NetJobPtr packDownload;

	SolderInstance *m_inst;
};

void SolderUpdate::executeTask()
{
	packVersionStart();
}

void SolderUpdate::versionProgress(int, qint64 min, qint64 max)
{
	double part = double(min)/double(max);
	setProgress(part * 5);
}

void SolderUpdate::packVersionStart()
{
	setStatus(tr("Downloading version information"));
	packVersionDownload = ByteArrayDownload::make(QUrl(m_inst->getPackUrl()));
	connect(packVersionDownload.get(), SIGNAL(succeeded(int)), SLOT(packVersionFinished()));
	connect(packVersionDownload.get(), SIGNAL(failed(int)), SLOT(packVersionFailed()));
	connect(packVersionDownload.get(), SIGNAL(progress(int, qint64, qint64)),
			SLOT(versionProgress(int, qint64, qint64)));
}

void SolderUpdate::packVersionFinished()
{
	// process data here
	packStart();
}

void SolderUpdate::packVersionFailed()
{
	emitFailed(tr("Couldn't get pack version..."));
}

void SolderUpdate::packStart()
{
	setStatus(tr("Downloading pack data"));
	/*
	NetJob* job = std::make_shared<NetJob>("Solder pack packages");
	*/
}

void SolderUpdate::packFinished()
{
}

void SolderUpdate::packFailed()
{
}

SolderInstance::SolderInstance(const QString &rootDir, SettingsObject *settings,
							   QObject *parent)
	: OneSixInstance(rootDir, settings, parent)
{
	settings->registerSetting("solderPackURL", "");
}

void SolderInstance::setPackUrl(QString url)
{
	settings().set("solderPackURL", url);
}

QString SolderInstance::getPackUrl()
{
	return settings().get("solderPackURL").toString();
}

std::shared_ptr<Task> SolderInstance::doUpdate()
{
	auto task = new SequentialTask(this);
	task->addTask(std::make_shared<SolderUpdate>(this));
	task->addTask(std::make_shared<OneSixUpdate>(this));
	return OneSixInstance::doUpdate();
}

#include "SolderInstance.moc"
