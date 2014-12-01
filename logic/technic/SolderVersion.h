#pragma once
#include <QString>
#include <memory>
#include "logic/BaseVersion.h"

struct SolderVersion;
typedef std::shared_ptr<SolderVersion> SolderVersionPtr;

struct SolderVersion : public BaseVersion
{
	virtual QString descriptor() override;
	virtual QString name() override;
	virtual QString typeString() const override;
	virtual bool operator<(BaseVersion &a) override;
	virtual bool operator>(BaseVersion &a) override;

	QString filename();
	QString url();

	QString base_url;
	QString pack_name;
	QString id;

	bool is_recommended = false;
	bool is_latest = false;
	bool is_complete = false;
};

Q_DECLARE_METATYPE(SolderVersionPtr)
