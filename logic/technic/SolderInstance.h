#pragma once

#include "logic/OneSixInstance.h"
#include "SolderVersion.h"
class Task;

class SolderInstance : public OneSixInstance
{
	Q_OBJECT
public:
	explicit SolderInstance(const QString &rootDir, SettingsObject *settings,
							QObject *parent = 0);
    virtual ~SolderInstance(){};

	void setSolderVersion(SolderVersionPtr url);
	SolderVersionPtr solderVersion();

	virtual std::shared_ptr<Task> doUpdate() override;
private:
	SolderVersionPtr m_solderVersion;
};
