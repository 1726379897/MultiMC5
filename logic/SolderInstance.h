#pragma once

#include "logic/OneSixInstance.h"
class Task;

class SolderInstance : public OneSixInstance
{
	Q_OBJECT
public:
	explicit SolderInstance(const QString &rootDir, SettingsObject *settings,
							QObject *parent = 0);
    virtual ~SolderInstance(){};

	void setPackUrl(QString url);
	QString getPackUrl();

	virtual std::shared_ptr<Task> doUpdate() override;
};
