#include "SolderVersion.h"
#include "logic/VersionFilterData.h"
#include <QObject>
#include <modutils.h>

QString SolderVersion::name()
{
	return id;
}

QString SolderVersion::descriptor()
{
	return id;
}

QString SolderVersion::typeString() const
{
	if (is_latest)
		return QObject::tr("Latest");

	if (is_recommended)
		return QObject::tr("Recommended");

	return QString();
}

bool SolderVersion::operator<(BaseVersion &a)
{
	SolderVersion *pa = dynamic_cast<SolderVersion *>(&a);
	if (!pa)
		return true;

	auto & first = id;
	auto & second = pa->id;
	if(first.startsWith('v'))
	{
		first.remove(0,1);
	}
	if(second.startsWith('v'))
	{
		second.remove(0,1);
	}
	Util::Version left(first);
	Util::Version right(second);
	return left < right;
}

bool SolderVersion::operator>(BaseVersion &a)
{
	SolderVersion *pa = dynamic_cast<SolderVersion *>(&a);
	if (!pa)
		return false;

	auto & first = id;
	auto & second = pa->id;
	if(first.startsWith('v'))
	{
		first.remove(0,1);
	}
	if(second.startsWith('v'))
	{
		second.remove(0,1);
	}
	Util::Version left(first);
	Util::Version right(second);
	return left > right;
}

QString SolderVersion::filename()
{
	return QString();
}

QString SolderVersion::url()
{
	return base_url + pack_name + "/" + id;
}
